#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-mateconf.h>

#include "global.h"


/*
  Shifts data right

  data[i+1] = data[i]

  data[i] are int*, so we just move the pointer, not the data.
  But moving data loses data[n-1], so we save data[n-1] and reuse
  it as new data[0]. In fact, we rotate data[].

*/

static void
shift_right(LoadGraph *g)
{
	unsigned i;
	int* last_data;

	/* data[g->draw_width - 1] becomes data[0] */
	last_data = g->data[g->draw_width - 1];

	/* data[i+1] = data[i] */
	for(i = g->draw_width - 1; i != 0; --i)
		g->data[i] = g->data[i-1];

	g->data[0] = last_data;
}


/* Redraws the backing pixmap for the load graph and updates the window */
static void
load_graph_draw (LoadGraph *g)
{
    GtkStyle *style;
    guint i, j;

    /* we might get called before the configure event so that
     * g->disp->allocation may not have the correct size
     * (after the user resized the applet in the prop dialog). */

    if (!g->pixmap)
		g->pixmap = gdk_pixmap_new (gtk_widget_get_window (g->disp),
					    g->draw_width, g->draw_height,
					    gtk_widget_get_visual (g->disp)->depth);
	
    style = gtk_widget_get_style (g->disp);

    /* Create GC if necessary. */
    if (!g->gc)
    {
		g->gc = gdk_gc_new (gtk_widget_get_window (g->disp));
		gdk_gc_copy (g->gc, style->black_gc);
    }

    /* Allocate colors. */
    if (!g->colors_allocated)
    {
		GdkColormap *colormap;

		colormap = gdk_drawable_get_colormap (gtk_widget_get_window (g->disp));
		
		for (i = 0; i < g->n; i++)
	 	   gdk_colormap_alloc_color (colormap, &(g->colors [i]),
					     FALSE, TRUE);

		g->colors_allocated = 1;
    }
	
    /* Erase Rectangle */
    gdk_draw_rectangle (g->pixmap,
			style->black_gc,
			TRUE, 0, 0,
			g->draw_width,
			g->draw_height);

    for (i = 0; i < g->draw_width; i++)
		g->pos [i] = g->draw_height - 1;

    for (j = 0; j < g->n; j++)
    {
		gdk_gc_set_foreground (g->gc, &(g->colors [j]));

		for (i = 0; i < g->draw_width; i++) {
			if (g->data [i][j] != 0) {
				gdk_draw_line (g->pixmap, g->gc,
					       g->draw_width - i - 1,
					       g->pos[i],
					       g->draw_width - i - 1,
					       g->pos[i] - (g->data [i][j] - 1));

				g->pos [i] -= g->data [i][j];
			}
		}
    }
	
    gdk_draw_drawable (gtk_widget_get_window (g->disp),
		       style->fg_gc [gtk_widget_get_state (g->disp)],
		       g->pixmap,
		       0, 0,
		       0, 0,
		       g->draw_width,
		       g->draw_height);
}

/* Updates the load graph when the timeout expires */
static gboolean
load_graph_update (LoadGraph *g)
{
    if (g->data == NULL)
	return TRUE;

    shift_right(g);

    if (g->tooltip_update)
	multiload_applet_tooltip_update(g);

    g->get_data (g->draw_height, g->data [0], g);

    load_graph_draw (g);
    return TRUE;
}

void
load_graph_unalloc (LoadGraph *g)
{
    guint i;

    if (!g->allocated)
		return;

    for (i = 0; i < g->draw_width; i++)
    {
		g_free (g->data [i]);
    }

    g_free (g->data);
    g_free (g->pos);

    g->pos = NULL;
    g->data = NULL;
    
    g->size = mate_panel_applet_mateconf_get_int(g->multiload->applet, "size", NULL);
    g->size = MAX (g->size, 10);

    if (g->pixmap) {
		g_object_unref (g->pixmap);
		g->pixmap = NULL;
    }

    if (g->gc) {
		g_object_unref (g->gc);
		g->gc = NULL;
    }

    g->allocated = FALSE;
}

static void
load_graph_alloc (LoadGraph *g)
{
    guint i;

    if (g->allocated)
		return;

    g->data = g_new0 (gint *, g->draw_width);
    g->pos = g_new0 (guint, g->draw_width);

    g->data_size = sizeof (guint) * g->n;

    for (i = 0; i < g->draw_width; i++) {
		g->data [i] = g_malloc0 (g->data_size);
    }

    g->allocated = TRUE;
}

static gint
load_graph_configure (GtkWidget *widget, GdkEventConfigure *event,
		      gpointer data_ptr)
{
    GtkAllocation allocation;
    LoadGraph *c = (LoadGraph *) data_ptr;
    
    load_graph_unalloc (c);

    gtk_widget_get_allocation (c->disp, &allocation);

    c->draw_width = allocation.width;
    c->draw_height = allocation.height;
    c->draw_width = MAX (c->draw_width, 1);
    c->draw_height = MAX (c->draw_height, 1);
    
    load_graph_alloc (c);
 
    if (!c->pixmap)
	c->pixmap = gdk_pixmap_new (gtk_widget_get_window (c->disp),
				    c->draw_width,
				    c->draw_height,
				    gtk_widget_get_visual (c->disp)->depth);

    gdk_draw_rectangle (c->pixmap,
			(gtk_widget_get_style (widget))->black_gc,
			TRUE, 0,0,
			c->draw_width,
			c->draw_height);
    gdk_draw_drawable (gtk_widget_get_window (widget),
		       (gtk_widget_get_style (c->disp))->fg_gc [gtk_widget_get_state (widget)],
		       c->pixmap,
		       0, 0,
		       0, 0,
		       c->draw_width,
		       c->draw_height);

    return TRUE;
}

static gint
load_graph_expose (GtkWidget *widget, GdkEventExpose *event,
		   gpointer data_ptr)
{
    LoadGraph *g = (LoadGraph *) data_ptr;

    gdk_draw_drawable (gtk_widget_get_window (widget),
		       (gtk_widget_get_style (widget))->fg_gc [gtk_widget_get_state (widget)],
		       g->pixmap,
		       event->area.x, event->area.y,
		       event->area.x, event->area.y,
		       event->area.width, event->area.height);

    return FALSE;
}

static void
load_graph_destroy (GtkWidget *widget, gpointer data_ptr)
{
    LoadGraph *g = (LoadGraph *) data_ptr;

    load_graph_stop (g);
    netspeed_delete(g->netspeed_in);
    netspeed_delete(g->netspeed_out);

    gtk_widget_destroy(widget);
}

static gboolean
load_graph_clicked (GtkWidget *widget, GdkEventButton *event, LoadGraph *load)
{
	load->multiload->last_clicked = load->id;

	return FALSE;
}

static gboolean
load_graph_enter_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	LoadGraph *graph;
	graph = (LoadGraph *)data;

	graph->tooltip_update = TRUE;
	multiload_applet_tooltip_update(graph);

	return TRUE;
}

static gboolean
load_graph_leave_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	LoadGraph *graph;
	graph = (LoadGraph *)data;

	graph->tooltip_update = FALSE;

	return TRUE;
}

static void
load_graph_load_config (LoadGraph *g)
{
	
    gchar name [BUFSIZ], *temp;
    guint i;

	if (!g->colors)
		g->colors = g_new0(GdkColor, g->n);
		
	for (i = 0; i < g->n; i++)
	{
		g_snprintf(name, sizeof(name), "%s_color%u", g->name, i);
		temp = mate_panel_applet_mateconf_get_string(g->multiload->applet, name, NULL);
		if (!temp)
			temp = g_strdup ("#000000");
		gdk_color_parse(temp, &(g->colors[i]));
		g_free(temp);
	}
}

LoadGraph *
load_graph_new (MultiloadApplet *ma, guint n, const gchar *label,
		guint id, guint speed, guint size, gboolean visible, 
		const gchar *name, LoadGraphDataFunc get_data)
{
    LoadGraph *g;
    MatePanelAppletOrient orient;
    
    g = g_new0 (LoadGraph, 1);
    g->netspeed_in = netspeed_new(g);
    g->netspeed_out = netspeed_new(g);
    g->visible = visible;
    g->name = name;
    g->n = n;
    g->id = id;
    g->speed  = MAX (speed, 50);
    g->size   = MAX (size, 10);
    g->pixel_size = mate_panel_applet_get_size (ma->applet);
    g->tooltip_update = FALSE;
    g->show_frame = TRUE;
    g->multiload = ma;
		
    g->main_widget = gtk_vbox_new (FALSE, 0);

    g->box = gtk_vbox_new (FALSE, 0);
    
    orient = mate_panel_applet_get_orient (g->multiload->applet);
    switch (orient)
    {
    case MATE_PANEL_APPLET_ORIENT_UP:
    case MATE_PANEL_APPLET_ORIENT_DOWN:
    {
	g->orient = FALSE;
	break;
    }
    case MATE_PANEL_APPLET_ORIENT_LEFT:
    case MATE_PANEL_APPLET_ORIENT_RIGHT:
    {
	g->orient = TRUE;
	break;
    }
    default:
	g_assert_not_reached ();
    }
    
    if (g->show_frame)
    {
	g->frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (g->frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (g->frame), g->box);
	gtk_box_pack_start (GTK_BOX (g->main_widget), g->frame, TRUE, TRUE, 0);
    }
    else
    {
	g->frame = NULL;
	gtk_box_pack_start (GTK_BOX (g->main_widget), g->box, TRUE, TRUE, 0);
    }

    load_graph_load_config (g);

    g->get_data = get_data;

    g->timer_index = -1;

    if (g->orient)
    	gtk_widget_set_size_request (g->main_widget, -1, g->size);
    else
        gtk_widget_set_size_request (g->main_widget, g->size, -1);

    g->disp = gtk_drawing_area_new ();
    gtk_widget_set_events (g->disp, GDK_EXPOSURE_MASK |
				    GDK_ENTER_NOTIFY_MASK |
    				    GDK_LEAVE_NOTIFY_MASK |
				    GDK_BUTTON_PRESS_MASK);
	
    g_signal_connect (G_OBJECT (g->disp), "expose_event",
			G_CALLBACK (load_graph_expose), g);
    g_signal_connect (G_OBJECT(g->disp), "configure_event",
			G_CALLBACK (load_graph_configure), g);
    g_signal_connect (G_OBJECT(g->disp), "destroy",
			G_CALLBACK (load_graph_destroy), g);
    g_signal_connect (G_OBJECT(g->disp), "button-press-event",
		        G_CALLBACK (load_graph_clicked), g);
    g_signal_connect (G_OBJECT(g->disp), "enter-notify-event",
                      G_CALLBACK(load_graph_enter_cb), g);
    g_signal_connect (G_OBJECT(g->disp), "leave-notify-event",
                      G_CALLBACK(load_graph_leave_cb), g);
	
    gtk_box_pack_start (GTK_BOX (g->box), g->disp, TRUE, TRUE, 0);    
    gtk_widget_show_all(g->box);

    return g;
}

void
load_graph_start (LoadGraph *g)
{
    if (g->timer_index != -1)
		g_source_remove (g->timer_index);

    g->timer_index = g_timeout_add (g->speed,
                                    (GSourceFunc) load_graph_update, g);
}

void
load_graph_stop (LoadGraph *g)
{
    if (g->timer_index != -1)
		g_source_remove (g->timer_index);
    
    g->timer_index = -1;
}
