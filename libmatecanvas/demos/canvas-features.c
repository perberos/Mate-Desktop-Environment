#include <config.h>
#include <libmatecanvas/libmatecanvas.h>
#include "canvas_demo.h"

#ifndef MATE_PAD_SMALL
#define MATE_PAD_SMALL 4
#endif

/* Event handler for the item to be reparented.  When the user clicks on the item, it will be
 * reparented to another group.
 */
static gint
item_event (MateCanvasItem *item, GdkEvent *event, gpointer data)
{
	MateCanvasItem *parent1;
	MateCanvasItem *parent2;

	if ((event->type != GDK_BUTTON_PRESS) || (event->button.button != 1))
		return FALSE;

	parent1 = g_object_get_data (G_OBJECT (item), "parent1");
	parent2 = g_object_get_data (G_OBJECT (item), "parent2");

	if (item->parent == parent1)
		mate_canvas_item_reparent (item, MATE_CANVAS_GROUP (parent2));
	else
		mate_canvas_item_reparent (item, MATE_CANVAS_GROUP (parent1));

	return TRUE;
}

GtkWidget *
create_canvas_features (void)
{
	GtkWidget *vbox;
	GtkWidget *w;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *canvas;
	MateCanvasItem *parent1;
	MateCanvasItem *parent2;
	MateCanvasItem *group;
	MateCanvasItem *item;

	vbox = gtk_vbox_new (FALSE, MATE_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), MATE_PAD_SMALL);
	gtk_widget_show (vbox);

	/* Instructions */

	w = gtk_label_new ("Reparent test:  click on the items to switch them between parents");
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	/* Frame and canvas */

	alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
	gtk_widget_show (alignment);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (alignment), frame);
	gtk_widget_show (frame);

	canvas = mate_canvas_new ();
	gtk_widget_set_size_request (canvas, 400, 200);
	mate_canvas_set_scroll_region (MATE_CANVAS (canvas), 0, 0, 400, 200);
	gtk_container_add (GTK_CONTAINER (frame), canvas);
	gtk_widget_show (canvas);

	/* First parent and box */

	parent1 = mate_canvas_item_new (mate_canvas_root (MATE_CANVAS (canvas)),
					 mate_canvas_group_get_type (),
					 "x", 0.0,
					 "y", 0,0,
					 NULL);

	mate_canvas_item_new (MATE_CANVAS_GROUP (parent1),
			       mate_canvas_rect_get_type (),
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", 200.0,
			       "y2", 200.0,
			       "fill_color", "tan",
			       NULL);

	/* Second parent and box */

	parent2 = mate_canvas_item_new (mate_canvas_root (MATE_CANVAS (canvas)),
					 mate_canvas_group_get_type (),
					 "x", 200.0,
					 "y", 0,0,
					 NULL);

	mate_canvas_item_new (MATE_CANVAS_GROUP (parent2),
			       mate_canvas_rect_get_type (),
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", 200.0,
			       "y2", 200.0,
			       "fill_color", "#204060",
			       NULL);

	/* Big circle to be reparented */

	item = mate_canvas_item_new (MATE_CANVAS_GROUP (parent1),
				      mate_canvas_ellipse_get_type (),
				      "x1", 10.0,
				      "y1", 10.0,
				      "x2", 190.0,
				      "y2", 190.0,
				      "outline_color", "black",
				      "fill_color", "mediumseagreen",
				      "width_units", 3.0,
				      NULL);
	g_object_set_data (G_OBJECT (item), "parent1", parent1);
	g_object_set_data (G_OBJECT (item), "parent2", parent2);
	g_signal_connect (item, "event",
			  G_CALLBACK (item_event),
			  NULL);

	/* A group to be reparented */

	group = mate_canvas_item_new (MATE_CANVAS_GROUP (parent2),
				       mate_canvas_group_get_type (),
				       "x", 100.0,
				       "y", 100.0,
				       NULL);

	mate_canvas_item_new (MATE_CANVAS_GROUP (group),
			       mate_canvas_ellipse_get_type (),
			       "x1", -50.0,
			       "y1", -50.0,
			       "x2", 50.0,
			       "y2", 50.0,
			       "outline_color", "black",
			       "fill_color", "wheat",
			       "width_units", 3.0,
			       NULL);
	mate_canvas_item_new (MATE_CANVAS_GROUP (group),
			       mate_canvas_ellipse_get_type (),
			       "x1", -25.0,
			       "y1", -25.0,
			       "x2", 25.0,
			       "y2", 25.0,
			       "fill_color", "steelblue",
			       NULL);

	g_object_set_data (G_OBJECT (group), "parent1", parent1);
	g_object_set_data (G_OBJECT (group), "parent2", parent2);
	g_signal_connect (group, "event",
			  G_CALLBACK (item_event),
			  NULL);

	/* Done */

	return vbox;
}
