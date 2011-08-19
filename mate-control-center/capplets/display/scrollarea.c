/* Copyright 2006, 2007, 2008, Soren Sandmann <sandmann@daimi.au.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gdk/gdkprivate.h> /* For GDK_PARENT_RELATIVE_BG */
#include "scrollarea.h"
#include "foo-marshal.h"

G_DEFINE_TYPE (FooScrollArea, foo_scroll_area, GTK_TYPE_CONTAINER);

static GtkWidgetClass *parent_class;

typedef struct BackingStore BackingStore;

typedef void (* ExposeFunc) (cairo_t *cr, GdkRegion *region, gpointer data);

#if 0
static void          backing_store_draw (BackingStore *store, 
					 GdkDrawable *dest,
					 GdkRegion *clip,
					 int dest_x,
					 int dest_y);
static void          backing_store_scroll (BackingStore *store,
					   int dx, int dy);
static void          backing_store_invalidate_rect (BackingStore *store,
						    GdkRectangle *rect);
static void          backing_store_invalidate_region (BackingStore *store,
						      GdkRegion *region);
static void          backing_store_invalidate_all (BackingStore *store);
static BackingStore *backing_store_new (GdkWindow *window,
					int width, int height);
static void          backing_store_resize (BackingStore *store,
					   int width, int height);
static void          backing_store_process_updates (BackingStore *store,
						    ExposeFunc func,
						    gpointer data);
static void	     backing_store_free (BackingStore *store);
#endif

typedef struct InputPath InputPath;
typedef struct InputRegion InputRegion;
typedef struct AutoScrollInfo AutoScrollInfo;

struct InputPath
{
    gboolean			is_stroke;
    cairo_fill_rule_t		fill_rule;
    double			line_width;
    cairo_path_t	       *path;		/* In canvas coordinates */

    FooScrollAreaEventFunc	func;
    gpointer			data;

    InputPath		       *next;
};

/* InputRegions are mutually disjoint */
struct InputRegion
{
    GdkRegion *region;		/* the boundary of this area in canvas coordinates */
    InputPath *paths;
};

struct AutoScrollInfo
{
    int				dx;
    int				dy;
    int				timeout_id;
    int				begin_x;
    int				begin_y;
    double			res_x;
    double			res_y;
    GTimer		       *timer;
};

struct FooScrollAreaPrivate
{
    GdkWindow		       *input_window;
    
    int				width;
    int				height;
    
    GtkAdjustment	       *hadj;
    GtkAdjustment	       *vadj;
    int			        x_offset;
    int				y_offset;
    
    int				min_width;
    int				min_height;

    GPtrArray		       *input_regions;
    
    AutoScrollInfo	       *auto_scroll_info;
    
    /* During expose, this region is set to the region
     * being exposed. At other times, it is NULL
     *
     * It is used for clipping of input areas
     */
    GdkRegion		       *expose_region;
    InputRegion		       *current_input;
    
    gboolean			grabbed;
    FooScrollAreaEventFunc	grab_func;
    gpointer			grab_data;

    GdkPixmap		       *pixmap;
    GdkRegion		       *update_region;		/* In canvas coordinates */
};

enum
{
    VIEWPORT_CHANGED,
    PAINT,
    INPUT,
    LAST_SIGNAL,
};

static guint signals [LAST_SIGNAL] = { 0 };

static void foo_scroll_area_size_request (GtkWidget *widget,
					  GtkRequisition *requisition);
static gboolean foo_scroll_area_expose (GtkWidget *widget,
					GdkEventExpose *expose);
static void foo_scroll_area_size_allocate (GtkWidget *widget,
					   GtkAllocation *allocation);
static void foo_scroll_area_set_scroll_adjustments (FooScrollArea *scroll_area,
						    GtkAdjustment    *hadjustment,
						    GtkAdjustment    *vadjustment);
static void foo_scroll_area_realize (GtkWidget *widget);
static void foo_scroll_area_unrealize (GtkWidget *widget);
static void foo_scroll_area_map (GtkWidget *widget);
static void foo_scroll_area_unmap (GtkWidget *widget);
static gboolean foo_scroll_area_button_press (GtkWidget *widget,
					      GdkEventButton *event);
static gboolean foo_scroll_area_button_release (GtkWidget *widget,
						GdkEventButton *event);
static gboolean foo_scroll_area_motion (GtkWidget *widget,
					GdkEventMotion *event);

static void
foo_scroll_area_map (GtkWidget *widget)
{
    FooScrollArea *area = FOO_SCROLL_AREA (widget);
    
    GTK_WIDGET_CLASS (parent_class)->map (widget);
    
    if (area->priv->input_window)
	gdk_window_show (area->priv->input_window);
}

static void
foo_scroll_area_unmap (GtkWidget *widget)
{
    FooScrollArea *area = FOO_SCROLL_AREA (widget);
    
    if (area->priv->input_window)
	gdk_window_hide (area->priv->input_window);
    
    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
foo_scroll_area_finalize (GObject *object)
{
    FooScrollArea *scroll_area = FOO_SCROLL_AREA (object);
    
    g_object_unref (scroll_area->priv->hadj);
    g_object_unref (scroll_area->priv->vadj);
    
    g_ptr_array_free (scroll_area->priv->input_regions, TRUE);
    
    g_free (scroll_area->priv);

    G_OBJECT_CLASS (foo_scroll_area_parent_class)->finalize (object);
}

static void
foo_scroll_area_class_init (FooScrollAreaClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
    
    object_class->finalize = foo_scroll_area_finalize;
    widget_class->size_request = foo_scroll_area_size_request;
    widget_class->expose_event = foo_scroll_area_expose;
    widget_class->size_allocate = foo_scroll_area_size_allocate;
    widget_class->realize = foo_scroll_area_realize;
    widget_class->unrealize = foo_scroll_area_unrealize;
    widget_class->button_press_event = foo_scroll_area_button_press;
    widget_class->button_release_event = foo_scroll_area_button_release;
    widget_class->motion_notify_event = foo_scroll_area_motion;
    widget_class->map = foo_scroll_area_map;
    widget_class->unmap = foo_scroll_area_unmap;
    
    class->set_scroll_adjustments = foo_scroll_area_set_scroll_adjustments;
    
    parent_class = g_type_class_peek_parent (class);
    
    signals[VIEWPORT_CHANGED] =
	g_signal_new ("viewport_changed",
		      G_OBJECT_CLASS_TYPE (object_class),
		      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		      G_STRUCT_OFFSET (FooScrollAreaClass,
				       viewport_changed),
		      NULL, NULL,
		      foo_marshal_VOID__BOXED_BOXED,
		      G_TYPE_NONE, 2,
		      GDK_TYPE_RECTANGLE,
		      GDK_TYPE_RECTANGLE);
    
    signals[PAINT] =
	g_signal_new ("paint",
		      G_OBJECT_CLASS_TYPE (object_class),
		      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		      G_STRUCT_OFFSET (FooScrollAreaClass,
				       paint),
		      NULL, NULL,
		      foo_marshal_VOID__POINTER_BOXED_POINTER,
		      G_TYPE_NONE, 3,
		      G_TYPE_POINTER,
		      GDK_TYPE_RECTANGLE, 
		      G_TYPE_POINTER);
    
    widget_class->set_scroll_adjustments_signal =
	g_signal_new ("set_scroll_adjustments",
		      G_OBJECT_CLASS_TYPE (object_class),
		      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		      G_STRUCT_OFFSET (FooScrollAreaClass,
				       set_scroll_adjustments),
		      NULL, NULL,
		      foo_marshal_VOID__OBJECT_OBJECT,
		      G_TYPE_NONE, 2,
		      GTK_TYPE_ADJUSTMENT,
		      GTK_TYPE_ADJUSTMENT);
}

static GtkAdjustment *
new_adjustment (void)
{
    return GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
}

static void
foo_scroll_area_init (FooScrollArea *scroll_area)
{
    GtkWidget *widget;

    widget = GTK_WIDGET (scroll_area);

    gtk_widget_set_has_window (widget, FALSE);
    gtk_widget_set_redraw_on_allocate (widget, FALSE);
    
    scroll_area->priv = g_new0 (FooScrollAreaPrivate, 1);
    scroll_area->priv->width = 0;
    scroll_area->priv->height = 0;
    scroll_area->priv->hadj = g_object_ref_sink (new_adjustment());
    scroll_area->priv->vadj = g_object_ref_sink (new_adjustment());
    scroll_area->priv->x_offset = 0.0;
    scroll_area->priv->y_offset = 0.0;
    scroll_area->priv->min_width = -1;
    scroll_area->priv->min_height = -1;
    scroll_area->priv->auto_scroll_info = NULL;
    scroll_area->priv->input_regions = g_ptr_array_new ();
    scroll_area->priv->pixmap = NULL;
    scroll_area->priv->update_region = gdk_region_new ();

    gtk_widget_set_double_buffered (widget, FALSE);
}

static void
translate_cairo_device (cairo_t       *cr,
			int            x_offset,
			int            y_offset)
{
    cairo_surface_t *surface = cairo_get_target (cr);
    double dev_x;
    double dev_y;
    
    cairo_surface_get_device_offset (surface, &dev_x, &dev_y);
    dev_x += x_offset;
    dev_y += y_offset;
    cairo_surface_set_device_offset (surface, dev_x, dev_y);
}

#if 0
static void
print_region (const char *header, GdkRegion *region)
{
    GdkRectangle *rects;
    int n_rects;
    int i;
    
    g_print ("%s\n", header);
    
    gdk_region_get_rectangles (region, &rects, &n_rects);
    for (i = 0; i < n_rects; ++i)
    {
	GdkRectangle *rect = &(rects[i]);
	g_print ("  %d %d %d %d\n",
		 rect->x, rect->y, rect->width, rect->height);
    }
}
#endif

typedef void (* PathForeachFunc) (double  *x,
				  double  *y,
				  gpointer data);

static void
path_foreach_point (cairo_path_t     *path,
		    PathForeachFunc   func,
		    gpointer	      user_data)
{
    int i;
    
    for (i = 0; i < path->num_data; i += path->data[i].header.length)
    {
	cairo_path_data_t *data = &(path->data[i]);
	
	switch (data->header.type)
	{
	case CAIRO_PATH_MOVE_TO:
	case CAIRO_PATH_LINE_TO:
	    func (&(data[1].point.x), &(data[1].point.y), user_data);
	    break;
	    
	case CAIRO_PATH_CURVE_TO:
	    func (&(data[1].point.x), &(data[1].point.y), user_data);
	    func (&(data[2].point.x), &(data[2].point.y), user_data);
	    func (&(data[3].point.x), &(data[3].point.y), user_data);
	    break;
	    
	case CAIRO_PATH_CLOSE_PATH:
	    break;
	}
    }
}

typedef struct
{
    double x1, y1, x2, y2;
} Box;

#if 0
static void
update_box (double *x, double *y, gpointer data)
{
    Box *box = data;
    
    if (*x < box->x1)
	box->x1 = *x;

    if (*y < box->y1)
	box->y1 = *y;

    if (*y > box->y2)
	box->y2 = *y;
    
    if (*x > box->x2)
	box->x2 = *x;
}
#endif

#if 0
static void
path_compute_extents (cairo_path_t *path,
		      GdkRectangle *rect)
{
    if (rect)
    {
	Box box = { G_MAXDOUBLE, G_MAXDOUBLE, G_MINDOUBLE, G_MINDOUBLE };

	path_foreach_point (path, update_box, &box);

	rect->x = box.x1;
	rect->y = box.y1;
	rect->width = box.x2 - box.x1;
	rect->height = box.y2 - box.y1;
    }
}
#endif

static void
input_path_free_list (InputPath *paths)
{
    if (!paths)
	return;

    input_path_free_list (paths->next);
    cairo_path_destroy (paths->path);
    g_free (paths);
}

static void
input_region_free (InputRegion *region)
{
    input_path_free_list (region->paths);
    gdk_region_destroy (region->region);

    g_free (region);
}

static void
get_viewport (FooScrollArea *scroll_area,
	      GdkRectangle  *viewport)
{
    GtkAllocation allocation;
    GtkWidget *widget = GTK_WIDGET (scroll_area);

    gtk_widget_get_allocation (widget, &allocation);

    viewport->x = scroll_area->priv->x_offset;
    viewport->y = scroll_area->priv->y_offset;
    viewport->width = allocation.width;
    viewport->height = allocation.height;
}

static void
allocation_to_canvas (FooScrollArea *area,
		      int           *x,
		      int           *y)
{
    *x += area->priv->x_offset;
    *y += area->priv->y_offset;
}

static void
clear_exposed_input_region (FooScrollArea *area,
			    GdkRegion *exposed)	/* in canvas coordinates */
{
    int i;
    GdkRegion *viewport;
    GdkRectangle allocation;

    gtk_widget_get_allocation (GTK_WIDGET (area), &allocation);
    allocation.x = 0;
    allocation.y = 0;
    allocation_to_canvas (area, &allocation.x, &allocation.y);
    viewport = gdk_region_rectangle (&allocation);
    gdk_region_subtract (viewport, exposed);
    
    for (i = 0; i < area->priv->input_regions->len; ++i)
    {
	InputRegion *region = area->priv->input_regions->pdata[i];

	gdk_region_intersect (region->region, viewport);

	if (gdk_region_empty (region->region))
	{
	    input_region_free (region);
	    g_ptr_array_remove_index_fast (area->priv->input_regions, i--);
	}
    }

    gdk_region_destroy (viewport);
    
#if 0
	path = region->paths;
	while (path != NULL)
	{
	    GdkRectangle rect;

	    path_compute_extents (path->path, &rect);

	    if (gdk_region_rect_in (area->priv->expose_region, &rect) == GDK_OVERLAP_RECTANGLE_IN)
		g_print ("we would have deleted it\n");
#if 0
	    else
		g_print ("nope (%d %d %d %d)\n", );
#endif
	    
	    path = path->next;
	}
	
	/* FIXME: we should also delete paths (and path segments)
	 * completely contained in the expose_region
	 */
    }
#endif
}

static void
setup_background_cr (GdkWindow *window,
		     cairo_t   *cr,
		     int        x_offset,
		     int        y_offset)
{
    GdkWindowObject *private = (GdkWindowObject *)window;
    
    if (private->bg_pixmap == GDK_PARENT_RELATIVE_BG && private->parent)
    {
	x_offset += private->x;
	y_offset += private->y;
	
	setup_background_cr (GDK_WINDOW (private->parent), cr, x_offset, y_offset);
    }
    else if (private->bg_pixmap &&
	     private->bg_pixmap != GDK_PARENT_RELATIVE_BG &&
	     private->bg_pixmap != GDK_NO_BG)
    {
	gdk_cairo_set_source_pixmap (cr, private->bg_pixmap, -x_offset, -y_offset);
    }
    else
    {
	gdk_cairo_set_source_color (cr, &private->bg_color);
    }
}

static void
initialize_background (GtkWidget *widget,
		       cairo_t   *cr)
{
    setup_background_cr (gtk_widget_get_window (widget), cr, 0, 0);

    cairo_paint (cr);
}

static void
clip_to_region (cairo_t *cr, GdkRegion *region)
{
    int n_rects;
    GdkRectangle *rects;

    gdk_region_get_rectangles (region, &rects, &n_rects);

    cairo_new_path (cr);
    while (n_rects--)
    {
	GdkRectangle *rect = &(rects[n_rects]);

	cairo_rectangle (cr, rect->x, rect->y, rect->width, rect->height);
    }
    cairo_clip (cr);

    g_free (rects);
}

static void
simple_draw_drawable (GdkDrawable *dst,
		      GdkDrawable *src,
		      int	   src_x,
		      int	   src_y,
		      int          dst_x,
		      int          dst_y,
		      int          width,
		      int          height)
{
    GdkGC *gc = gdk_gc_new (dst);

    gdk_draw_drawable (dst, gc, src, src_x, src_y, dst_x, dst_y, width, height);

    g_object_unref (gc);
}

static gboolean
foo_scroll_area_expose (GtkWidget *widget,
			GdkEventExpose *expose)
{
    FooScrollArea *scroll_area = FOO_SCROLL_AREA (widget);
    cairo_t *cr;
    GdkRectangle extents;
    GdkRegion *region;
    int x_offset, y_offset;
    GdkGC *gc;
    GtkAllocation widget_allocation;
    GdkWindow *window = gtk_widget_get_window (widget);

    /* I don't think expose can ever recurse for the same area */
    g_assert (!scroll_area->priv->expose_region);
    
    /* Note that this function can be called at a time
     * where the adj->value is different from x_offset. 
     * Ie., the GtkScrolledWindow changed the adj->value
     * without emitting the value_changed signal. 
     *
     * Hence we must always use the value we got 
     * the last time the signal was emitted, ie.,
     * priv->{x,y}_offset.
     */
    
    x_offset = scroll_area->priv->x_offset;
    y_offset = scroll_area->priv->y_offset;
    
    scroll_area->priv->expose_region = expose->region;

    /* Setup input areas */
    clear_exposed_input_region (scroll_area, scroll_area->priv->update_region);
    
    scroll_area->priv->current_input = g_new0 (InputRegion, 1);
    scroll_area->priv->current_input->region = gdk_region_copy (scroll_area->priv->update_region);
    scroll_area->priv->current_input->paths = NULL;
    g_ptr_array_add (scroll_area->priv->input_regions,
		     scroll_area->priv->current_input);

    region = scroll_area->priv->update_region;
    scroll_area->priv->update_region = gdk_region_new ();
    
    /* Create cairo context */
    cr = gdk_cairo_create (scroll_area->priv->pixmap);
    translate_cairo_device (cr, -x_offset, -y_offset);
    clip_to_region (cr, region);
    initialize_background (widget, cr);

    /* Create regions */
    gdk_region_get_clipbox (region, &extents);

    g_signal_emit (widget, signals[PAINT], 0, cr, &extents, region);

    /* Destroy stuff */
    cairo_destroy (cr);
    
    scroll_area->priv->expose_region = NULL;
    scroll_area->priv->current_input = NULL;

    /* Finally draw the backing pixmap */
    gc = gdk_gc_new (window);

    gdk_gc_set_clip_region (gc, expose->region);

    gtk_widget_get_allocation (widget, &widget_allocation);
    gdk_draw_drawable (window, gc, scroll_area->priv->pixmap,
		       0, 0, widget_allocation.x, widget_allocation.y,
		       widget_allocation.width, widget_allocation.height);

    g_object_unref (gc);
    gdk_region_destroy (region);
    
    return TRUE;
}

void
foo_scroll_area_get_viewport (FooScrollArea *scroll_area,
			      GdkRectangle  *viewport)
{
    g_return_if_fail (FOO_IS_SCROLL_AREA (scroll_area));
    
    if (!viewport)
	return;
    
    get_viewport (scroll_area, viewport);
}

static void
process_event (FooScrollArea	       *scroll_area,
	       FooScrollAreaEventType	input_type,
	       int			x,
	       int			y);

static void
emit_viewport_changed (FooScrollArea *scroll_area,
		       GdkRectangle  *new_viewport,
		       GdkRectangle  *old_viewport)
{
    int px, py;
    g_signal_emit (scroll_area, signals[VIEWPORT_CHANGED], 0, 
		   new_viewport, old_viewport);
    
    gdk_window_get_pointer (scroll_area->priv->input_window, &px, &py, NULL);
    
#if 0
    g_print ("procc\n");
#endif
    
    process_event (scroll_area, FOO_MOTION, px, py);
}

static void
clamp_adjustment (GtkAdjustment *adj)
{
    if (gtk_adjustment_get_upper (adj) >= gtk_adjustment_get_page_size (adj))
	gtk_adjustment_set_value (adj, CLAMP (gtk_adjustment_get_value (adj), 0.0,
					      gtk_adjustment_get_upper (adj)
					       - gtk_adjustment_get_page_size (adj)));
    else
	gtk_adjustment_set_value (adj, 0.0);
    
    gtk_adjustment_changed (adj);
}

static gboolean
set_adjustment_values (FooScrollArea *scroll_area)
{
    GtkAllocation allocation;

    GtkAdjustment *hadj = scroll_area->priv->hadj;
    GtkAdjustment *vadj = scroll_area->priv->vadj;
    
    /* Horizontal */
    gtk_widget_get_allocation (GTK_WIDGET (scroll_area), &allocation);
    g_object_freeze_notify (G_OBJECT (hadj));
    gtk_adjustment_set_page_size (hadj, allocation.width);
    gtk_adjustment_set_step_increment (hadj, 0.1 * allocation.width);
    gtk_adjustment_set_page_increment (hadj, 0.9 * allocation.width);
    gtk_adjustment_set_lower (hadj, 0.0);
    gtk_adjustment_set_upper (hadj, scroll_area->priv->width);
    g_object_thaw_notify (G_OBJECT (hadj));
    
    /* Vertical */
    g_object_freeze_notify (G_OBJECT (vadj));
    gtk_adjustment_set_page_size (vadj, allocation.height);
    gtk_adjustment_set_step_increment (vadj, 0.1 * allocation.height);
    gtk_adjustment_set_page_increment (vadj, 0.9 * allocation.height);
    gtk_adjustment_set_lower (vadj, 0.0);
    gtk_adjustment_set_upper (vadj, scroll_area->priv->height);
    g_object_thaw_notify (G_OBJECT (vadj));

    clamp_adjustment (hadj);
    clamp_adjustment (vadj);
    
    return TRUE;
}

static void
foo_scroll_area_realize (GtkWidget *widget)
{
    FooScrollArea *area = FOO_SCROLL_AREA (widget);
    GdkWindowAttr attributes;
    GtkAllocation widget_allocation;
    GdkWindow *window;
    gint attributes_mask;

    gtk_widget_get_allocation (widget, &widget_allocation);
    gtk_widget_set_realized (widget, TRUE);
    
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget_allocation.x;
    attributes.y = widget_allocation.y;
    attributes.width = widget_allocation.width;
    attributes.height = widget_allocation.height;
    attributes.wclass = GDK_INPUT_ONLY;
    attributes.event_mask = gtk_widget_get_events (widget);
    attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
			      GDK_BUTTON_RELEASE_MASK |
			      GDK_BUTTON1_MOTION_MASK |
			      GDK_BUTTON2_MOTION_MASK |
			      GDK_BUTTON3_MOTION_MASK |
			      GDK_POINTER_MOTION_MASK |
			      GDK_ENTER_NOTIFY_MASK |
			      GDK_LEAVE_NOTIFY_MASK);
    
    attributes_mask = GDK_WA_X | GDK_WA_Y;

    window = gtk_widget_get_parent_window (widget);
    gtk_widget_set_window (widget, window);
    g_object_ref (window);
    
    area->priv->input_window = gdk_window_new (window,
					       &attributes, attributes_mask);
    area->priv->pixmap = gdk_pixmap_new (window,
					 widget_allocation.width,
					 widget_allocation.height,
					 -1);
    gdk_window_set_user_data (area->priv->input_window, area);
    
    gtk_widget_style_attach (widget);
}

static void
foo_scroll_area_unrealize (GtkWidget *widget)
{
    FooScrollArea *area = FOO_SCROLL_AREA (widget);
    
    if (area->priv->input_window)
    {
	gdk_window_set_user_data (area->priv->input_window, NULL);
	gdk_window_destroy (area->priv->input_window);
	area->priv->input_window = NULL;
    }
    
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static GdkPixmap *
create_new_pixmap (GtkWidget *widget,
		   GdkPixmap *old)
{
    GtkAllocation widget_allocation;
    GdkPixmap *new;

    gtk_widget_get_allocation (widget, &widget_allocation);
    new = gdk_pixmap_new (gtk_widget_get_window (widget),
			  widget_allocation.width,
			  widget_allocation.height,
			  -1);

    /* Unfortunately we don't know in which direction we were resized,
     * so we just assume we were dragged from the south-east corner.
     *
     * Although, maybe we could get the root coordinates of the input-window?
     * That might just work, actually. We need to make sure marco uses
     * static gravity for the window before this will be useful.
     */
    simple_draw_drawable (new, old, 0, 0, 0, 0, -1, -1);

    return new;
}
		   
static void
allocation_to_canvas_region (FooScrollArea *area,
			     GdkRegion *region)
{
    gdk_region_offset (region, area->priv->x_offset, area->priv->y_offset);
}
			     

static void
foo_scroll_area_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
    FooScrollArea *scroll_area = FOO_SCROLL_AREA (widget);
    GdkRectangle new_viewport;
    GdkRectangle old_viewport;
    GdkRegion *old_allocation;
    GdkRegion *invalid;
    GtkAllocation widget_allocation;

    get_viewport (scroll_area, &old_viewport);

    gtk_widget_get_allocation (widget, &widget_allocation);
    old_allocation = gdk_region_rectangle (&widget_allocation);
    gdk_region_offset (old_allocation,
		       -widget_allocation.x, -widget_allocation.y);
    invalid = gdk_region_rectangle (allocation);
    gdk_region_offset (invalid, -allocation->x, -allocation->y);
    gdk_region_xor (invalid, old_allocation);
    allocation_to_canvas_region (scroll_area, invalid);
    foo_scroll_area_invalidate_region (scroll_area, invalid);
    gdk_region_destroy (old_allocation);
    gdk_region_destroy (invalid);

    gtk_widget_set_allocation (widget, allocation);
    
    if (scroll_area->priv->input_window)
    {
	GdkPixmap *new_pixmap;
	
	gdk_window_move_resize (scroll_area->priv->input_window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);

	new_pixmap = create_new_pixmap (widget, scroll_area->priv->pixmap);

	g_object_unref (scroll_area->priv->pixmap);

	scroll_area->priv->pixmap = new_pixmap;
    }
    
    get_viewport (scroll_area, &new_viewport);
    
    emit_viewport_changed (scroll_area, &new_viewport, &old_viewport);
}

static void
emit_input (FooScrollArea *scroll_area,
	    FooScrollAreaEventType type,
	    int			   x,
	    int			   y,
	    FooScrollAreaEventFunc func,
	    gpointer		data)
{
    FooScrollAreaEvent event;
    
    if (!func)
	return;

    if (type != FOO_MOTION)
	emit_input (scroll_area, FOO_MOTION, x, y, func, data);
    
#if 0
    x += scroll_area->priv->x_offset;
    y += scroll_area->priv->y_offset;
#endif
    
    event.type = type;
    event.x = x;
    event.y = y;
    
    func (scroll_area, &event, data);
}

#if 0
static void
print_path (const char *header,
	    cairo_path_t *path)
{
    int i;

    g_print ("%s\n", header);

    for (i=0; i < path->num_data; i += path->data[i].header.length)
    {
	cairo_path_data_t *data = &(path->data[i]);
	
	switch (data->header.type)
	{
	case CAIRO_PATH_MOVE_TO:
	    g_print ("move to:    %f, %f\n", data[1].point.x, data[1].point.y);
	    break;
	    
	case CAIRO_PATH_LINE_TO:
	    g_print ("line to:    %f, %f\n", data[1].point.x, data[1].point.y);
	    break;
	    
	case CAIRO_PATH_CURVE_TO:
	    g_print ("curve to:   %f, %f\n", data[1].point.x, data[1].point.y);
	    g_print ("            %f, %f\n", data[1].point.x, data[1].point.y);
	    g_print ("            %f, %f\n", data[1].point.x, data[1].point.y);
	    break;
	    
	case CAIRO_PATH_CLOSE_PATH:
	    break;
	}
    }
}
#endif

static void
process_event (FooScrollArea	       *scroll_area,
	       FooScrollAreaEventType	input_type,
	       int			x,
	       int			y)
{
    GtkWidget *widget = GTK_WIDGET (scroll_area);
    int i;

    allocation_to_canvas (scroll_area, &x, &y);
    
    if (scroll_area->priv->grabbed)
    {
	emit_input (scroll_area, input_type, x, y,
		    scroll_area->priv->grab_func,
		    scroll_area->priv->grab_data);
	return;
    }

    
#if 0
    x += widget->allocation.x;
    y += widget->allocation.y;
#endif

#if 0
    g_print ("number of input regions: %d\n", scroll_area->priv->input_regions->len);
#endif
    
    for (i = 0; i < scroll_area->priv->input_regions->len; ++i)
    {
	InputRegion *region = scroll_area->priv->input_regions->pdata[i];

#if 0
	g_print ("%d ", i);
	print_region ("region:", region->region);
#endif
	
	if (gdk_region_point_in (region->region, x, y))
	{
	    InputPath *path;

	    path = region->paths;
	    while (path)
	    {
		cairo_t *cr;
		gboolean inside;

		cr = gdk_cairo_create (gtk_widget_get_window (widget));
		cairo_set_fill_rule (cr, path->fill_rule);
		cairo_set_line_width (cr, path->line_width);
		cairo_append_path (cr, path->path);

		if (path->is_stroke)
		    inside = cairo_in_stroke (cr, x, y);
		else
		    inside = cairo_in_fill (cr, x, y);

		cairo_destroy (cr);
		
		if (inside)
		{
		    emit_input (scroll_area, input_type,
				x, y,
				path->func,
				path->data);
		    return;
		}
		
		path = path->next;
	    }

	    /* Since the regions are all disjoint, no other region
	     * can match. Of course we could be clever and try and
	     * sort the regions, but so far I have been unable to
	     * make this loop show up on a profile.
	     */
	    return;
	}
    }
}

static void
process_gdk_event (FooScrollArea *scroll_area,
		   int		  x,
		   int	          y,
		   GdkEvent      *event)
{
    FooScrollAreaEventType input_type;
    
    if (event->type == GDK_BUTTON_PRESS)
	input_type = FOO_BUTTON_PRESS;
    else if (event->type == GDK_BUTTON_RELEASE)
	input_type = FOO_BUTTON_RELEASE;
    else if (event->type == GDK_MOTION_NOTIFY)
	input_type = FOO_MOTION;
    else
	return;
    
    process_event (scroll_area, input_type, x, y);
}

static gboolean
foo_scroll_area_button_press (GtkWidget *widget,
			      GdkEventButton *event)
{
    FooScrollArea *area = FOO_SCROLL_AREA (widget);
    
    process_gdk_event (area, event->x, event->y, (GdkEvent *)event);
    
    return TRUE;
}

static gboolean
foo_scroll_area_button_release (GtkWidget *widget,
				GdkEventButton *event)
{
    FooScrollArea *area = FOO_SCROLL_AREA (widget);
    
    process_gdk_event (area, event->x, event->y, (GdkEvent *)event);
    
    return FALSE;
}

static gboolean
foo_scroll_area_motion (GtkWidget *widget,
			GdkEventMotion *event)
{
    FooScrollArea *area = FOO_SCROLL_AREA (widget);
    
    process_gdk_event (area, event->x, event->y, (GdkEvent *)event);
    return TRUE;
}

void
foo_scroll_area_set_size_fixed_y (FooScrollArea	       *scroll_area,
				  int			width,
				  int			height,
				  int			old_y,
				  int			new_y)
{
    scroll_area->priv->width = width;
    scroll_area->priv->height = height;
    
#if 0
    g_print ("diff: %d\n", new_y - old_y);
#endif
    g_object_thaw_notify (G_OBJECT (scroll_area->priv->vadj));
    gtk_adjustment_set_value (scroll_area->priv->vadj, new_y);
    
    set_adjustment_values (scroll_area);
    g_object_thaw_notify (G_OBJECT (scroll_area->priv->vadj));
}

void
foo_scroll_area_set_size (FooScrollArea	       *scroll_area,
			  int			width,
			  int			height)
{
    g_return_if_fail (FOO_IS_SCROLL_AREA (scroll_area));
    
    /* FIXME: Default scroll algorithm should probably be to
     * keep the same *area* outside the screen as before.
     *
     * For wrapper widgets that will do something roughly
     * right. For widgets that don't change size, it
     * will do the right thing. Except for idle-layouting
     * widgets.
     *
     * Maybe there should be some generic support for those
     * widgets. Can that even be done?
     *
     * Should we have a version of this function using 
     * fixed points?
     */
    
    scroll_area->priv->width = width;
    scroll_area->priv->height = height;
    
    set_adjustment_values (scroll_area);
}

static void
foo_scroll_area_size_request (GtkWidget      *widget,
			      GtkRequisition *requisition)
{
    FooScrollArea *scroll_area = FOO_SCROLL_AREA (widget);
    
    requisition->width = scroll_area->priv->min_width;
    requisition->height = scroll_area->priv->min_height;
    
#if 0
    g_print ("request %d %d\n", requisition->width, requisition->height);
#endif
}

#if 0
static void
translate_point (double *x, double *y, gpointer data)
{
    int *translation = data;

    *x += translation[0];
    *y += translation[1];
}
#endif

#if 0
static void
path_translate (cairo_path_t  *path,
		int	       dx,
		int	       dy)
{
    int translation[2] = {dx, dy};
    
    path_foreach_point (path, translate_point, translation);
}
#endif

static void
translate_input_regions (FooScrollArea *scroll_area,
			 int		dx,
			 int		dy)
{
#if 0
    int i;

    for (i = 0; i < scroll_area->priv->input_regions->len; ++i)
    {
	InputRegion *region = scroll_area->priv->input_regions->pdata[i];
	InputPath *path;
	
	gdk_region_offset (region->region, dx, dy);

	path = region->paths;
	while (path != NULL)
	{
	    path_translate (path->path, dx, dy);
	    path = path->next;
	}
    }
#endif
}

#if 0
static void
paint_region (FooScrollArea *area, GdkRegion *region)
{
    int n_rects;
    GdkRectangle *rects;
    region = gdk_region_copy (region);
    
    gdk_region_get_rectangles (region, &rects, &n_rects);

    gdk_region_offset (region,
		       GTK_WIDGET (area)->allocation.x,
		       GTK_WIDGET (area)->allocation.y);

    GdkGC *gc = gdk_gc_new (GTK_WIDGET (area)->window);
    gdk_gc_set_clip_region (gc, region);
    gdk_draw_rectangle (GTK_WIDGET (area)->window, gc, TRUE, 0, 0, -1, -1);
    g_object_unref (gc);
    g_free (rects);
}
#endif

static void
foo_scroll_area_scroll (FooScrollArea *area,
			gint dx, 
			gint dy)
{
    GdkRectangle allocation;
    GdkRectangle src_area;
    GdkRectangle move_area;
    GdkRegion *invalid_region;

    gtk_widget_get_allocation (GTK_WIDGET (area), &allocation);
    allocation.x = 0;
    allocation.y = 0;

    src_area = allocation;
    src_area.x -= dx;
    src_area.y -= dy;

    invalid_region = gdk_region_rectangle (&allocation);
    
    if (gdk_rectangle_intersect (&allocation, &src_area, &move_area))
    {
	GdkRegion *move_region;

#if 0
	g_print ("scrolling %d %d %d %d (%d %d)\n",
		 move_area.x, move_area.y,
		 move_area.width, move_area.height,
		 dx, dy);
#endif
	
	simple_draw_drawable (area->priv->pixmap, area->priv->pixmap,
			      move_area.x, move_area.y,
			      move_area.x + dx, move_area.y + dy,
			      move_area.width, move_area.height);
	gtk_widget_queue_draw (GTK_WIDGET (area));
	
	move_region = gdk_region_rectangle (&move_area);
	gdk_region_offset (move_region, dx, dy);
	gdk_region_subtract (invalid_region, move_region);
	gdk_region_destroy (move_region);
    }

#if 0
    paint_region (area, invalid_region);
#endif
    
    allocation_to_canvas_region (area, invalid_region);

    foo_scroll_area_invalidate_region (area, invalid_region);
    
    gdk_region_destroy (invalid_region);
}

static void
foo_scrollbar_adjustment_changed (GtkAdjustment *adj,
				  FooScrollArea *scroll_area)
{
    GtkWidget *widget = GTK_WIDGET (scroll_area);
    gint dx = 0;
    gint dy = 0;
    GdkRectangle old_viewport, new_viewport;
    
    get_viewport (scroll_area, &old_viewport);
    
    if (adj == scroll_area->priv->hadj)
    {
	/* FIXME: do we treat the offset as int or double, and,
	 * if int, how do we round?
	 */
	dx = (int)gtk_adjustment_get_value (adj) - scroll_area->priv->x_offset;
	scroll_area->priv->x_offset = gtk_adjustment_get_value (adj);
    }
    else if (adj == scroll_area->priv->vadj)
    {
	dy = (int)gtk_adjustment_get_value (adj) - scroll_area->priv->y_offset;
	scroll_area->priv->y_offset = gtk_adjustment_get_value (adj);
    }
    else
    {
	g_assert_not_reached ();
    }
    
    if (gtk_widget_get_realized (widget))
    {
	foo_scroll_area_scroll (scroll_area, -dx, -dy);
    
#if 0
	window_scroll_area (widget->window, &widget->allocation, -dx, -dy);
#endif
	translate_input_regions (scroll_area, -dx, -dy);

#if 0
	gdk_window_process_updates (widget->window, TRUE);
#endif
    }
    
    get_viewport (scroll_area, &new_viewport);
    
    emit_viewport_changed (scroll_area, &new_viewport, &old_viewport);
}

static void
set_one_adjustment (FooScrollArea *scroll_area,
		    GtkAdjustment *adjustment,
		    GtkAdjustment **location)
{
    g_return_if_fail (location != NULL);
    
    if (adjustment == *location)
	return;
    
    if (!adjustment)
	adjustment = new_adjustment ();
    
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
    
    if (*location)
    {
	g_signal_handlers_disconnect_by_func (
	    *location, foo_scrollbar_adjustment_changed, scroll_area);
	
	g_object_unref (*location);
    }
    
    *location = adjustment;
    
    g_object_ref_sink (*location);
    
    g_signal_connect (*location, "value_changed",
		      G_CALLBACK (foo_scrollbar_adjustment_changed),
		      scroll_area);
}

static void
foo_scroll_area_set_scroll_adjustments (FooScrollArea *scroll_area,
					GtkAdjustment *hadjustment,
					GtkAdjustment *vadjustment)
{
    set_one_adjustment (scroll_area, hadjustment, &scroll_area->priv->hadj);
    set_one_adjustment (scroll_area, vadjustment, &scroll_area->priv->vadj);
    
    set_adjustment_values (scroll_area);
}

FooScrollArea *
foo_scroll_area_new (void)
{
    return g_object_new (FOO_TYPE_SCROLL_AREA, NULL);
}

void
foo_scroll_area_set_min_size (FooScrollArea *scroll_area,
			      int		   min_width,
			      int            min_height)
{
    scroll_area->priv->min_width = min_width;
    scroll_area->priv->min_height = min_height;
    
    /* FIXME: think through invalidation.
     *
     * Goals: - no repainting everything on size_allocate(),
     *        - make sure input boxes are invalidated when
     *          needed
     */
    gtk_widget_queue_resize (GTK_WIDGET (scroll_area));
}

#if 0
static void
warn_about_adding_input_outside_expose (const char *func)
{
    static gboolean warned = FALSE;
    
    if (!warned)
    {
	g_warning ("%s() can only be called "
		   "from the paint handler for the FooScrollArea\n", func);
	
	warned = TRUE;
    }
}
#endif

static void
user_to_device (double *x, double *y,
		gpointer data)
{
    cairo_t *cr = data;
    
    cairo_user_to_device (cr, x, y);
}

static InputPath *
make_path (FooScrollArea *area,
	   cairo_t *cr,
	   gboolean is_stroke,
	   FooScrollAreaEventFunc func,
	   gpointer data)
{
    InputPath *path = g_new0 (InputPath, 1);

    path->is_stroke = is_stroke;
    path->fill_rule = cairo_get_fill_rule (cr);
    path->line_width = cairo_get_line_width (cr);
    path->path = cairo_copy_path (cr);
    path_foreach_point (path->path, user_to_device, cr);
    path->func = func;
    path->data = data;
    path->next = area->priv->current_input->paths;
    area->priv->current_input->paths = path;
    return path;
}

/* FIXME: we probably really want a
 *
 *	foo_scroll_area_add_input_from_fill (area, cr, ...);
 * and
 *      foo_scroll_area_add_input_from_stroke (area, cr, ...);
 * as well.
 */
void
foo_scroll_area_add_input_from_fill (FooScrollArea           *scroll_area,
				     cairo_t	             *cr,
				     FooScrollAreaEventFunc   func,
				     gpointer                 data)
{
    g_return_if_fail (FOO_IS_SCROLL_AREA (scroll_area));
    g_return_if_fail (cr != NULL);
    g_return_if_fail (scroll_area->priv->current_input);

    make_path (scroll_area, cr, FALSE, func, data);
}

void
foo_scroll_area_add_input_from_stroke (FooScrollArea           *scroll_area,
				       cairo_t	                *cr,
				       FooScrollAreaEventFunc   func,
				       gpointer                 data)
{
    g_return_if_fail (FOO_IS_SCROLL_AREA (scroll_area));
    g_return_if_fail (cr != NULL);
    g_return_if_fail (scroll_area->priv->current_input);

    make_path (scroll_area, cr, TRUE, func, data);
}

void
foo_scroll_area_invalidate (FooScrollArea *scroll_area)
{
    GtkAllocation allocation;
    GtkWidget *widget = GTK_WIDGET (scroll_area);

    gtk_widget_get_allocation (widget, &allocation);
    foo_scroll_area_invalidate_rect (scroll_area,
				     scroll_area->priv->x_offset, scroll_area->priv->y_offset,
				     allocation.width,
				     allocation.height);
}

static void
canvas_to_window (FooScrollArea *area,
		  GdkRegion *region)
{
    GtkAllocation allocation;
    GtkWidget *widget = GTK_WIDGET (area);
    
    gtk_widget_get_allocation (widget, &allocation);
    gdk_region_offset (region,
		       -area->priv->x_offset + allocation.x,
		       -area->priv->y_offset + allocation.y);
}

static void
window_to_canvas (FooScrollArea *area,
		  GdkRegion *region)
{
    GtkAllocation allocation;
    GtkWidget *widget = GTK_WIDGET (area);

    gtk_widget_get_allocation (widget, &allocation);
    gdk_region_offset (region,
		       area->priv->x_offset - allocation.x,
		       area->priv->y_offset - allocation.y);
}

void
foo_scroll_area_invalidate_region (FooScrollArea *area,
				   GdkRegion     *region)
{
    GtkWidget *widget;

    g_return_if_fail (FOO_IS_SCROLL_AREA (area));

    widget = GTK_WIDGET (area);

    gdk_region_union (area->priv->update_region, region);

    if (gtk_widget_get_realized (widget))
    {
	canvas_to_window (area, region);
	
	gdk_window_invalidate_region (gtk_widget_get_window (widget),
	                              region, TRUE);
	
	window_to_canvas (area, region);
    }
}

void
foo_scroll_area_invalidate_rect (FooScrollArea *scroll_area,
				 int	        x,
				 int	        y,
				 int	        width,
				 int	        height)
{
    GdkRectangle rect = { x, y, width, height };
    GdkRegion *region;
    
    g_return_if_fail (FOO_IS_SCROLL_AREA (scroll_area));

    region = gdk_region_rectangle (&rect);

    foo_scroll_area_invalidate_region (scroll_area, region);

    gdk_region_destroy (region);
}

void
foo_scroll_area_begin_grab (FooScrollArea *scroll_area,
			    FooScrollAreaEventFunc func,
			    gpointer       input_data)
{
    g_return_if_fail (FOO_IS_SCROLL_AREA (scroll_area));
    g_return_if_fail (!scroll_area->priv->grabbed);
    
    scroll_area->priv->grabbed = TRUE;
    scroll_area->priv->grab_func = func;
    scroll_area->priv->grab_data = input_data;
    
    /* FIXME: we should probably take a server grab */
    /* Also, maybe there should be support for setting the grab cursor */
}

void
foo_scroll_area_end_grab (FooScrollArea *scroll_area)
{
    g_return_if_fail (FOO_IS_SCROLL_AREA (scroll_area));
    
    scroll_area->priv->grabbed = FALSE;
    scroll_area->priv->grab_func = NULL;
    scroll_area->priv->grab_data = NULL;
}

gboolean
foo_scroll_area_is_grabbed (FooScrollArea *scroll_area)
{
    return scroll_area->priv->grabbed;
}

void
foo_scroll_area_set_viewport_pos (FooScrollArea  *scroll_area,
				  int		  x,
				  int		  y)
{
    g_object_freeze_notify (G_OBJECT (scroll_area->priv->hadj));
    g_object_freeze_notify (G_OBJECT (scroll_area->priv->vadj));
    gtk_adjustment_set_value (scroll_area->priv->hadj, x);
    gtk_adjustment_set_value (scroll_area->priv->vadj, y);

    set_adjustment_values (scroll_area);
    g_object_thaw_notify (G_OBJECT (scroll_area->priv->hadj));
    g_object_thaw_notify (G_OBJECT (scroll_area->priv->vadj));
}

static gboolean
rect_contains (const GdkRectangle *rect, int x, int y)
{
    return (x >= rect->x		&&
	    y >= rect->y		&&
	    x  < rect->x + rect->width	&&
	    y  < rect->y + rect->height);
}

static void
stop_scrolling (FooScrollArea *area)
{
#if 0
    g_print ("stop scrolling\n");
#endif
    if (area->priv->auto_scroll_info)
    {
	g_source_remove (area->priv->auto_scroll_info->timeout_id);
	g_timer_destroy (area->priv->auto_scroll_info->timer);
	g_free (area->priv->auto_scroll_info);
	
	area->priv->auto_scroll_info = NULL;
    }
}

static gboolean
scroll_idle (gpointer data)
{
    GdkRectangle viewport, new_viewport;
    FooScrollArea *area = data;
    AutoScrollInfo *info = area->priv->auto_scroll_info;
#if 0
    int dx, dy;
#endif
    int new_x, new_y;
    double elapsed;
    
    get_viewport (area, &viewport);
    
#if 0
    g_print ("old info: %d %d\n", info->dx, info->dy);
    
    g_print ("timeout (%d %d)\n", dx, dy);
#endif
    
#if 0
    viewport.x += info->dx;
    viewport.y += info->dy;
#endif
    
#if 0
    g_print ("new info %d %d\n", info->dx, info->dy);
#endif

    elapsed = g_timer_elapsed (info->timer, NULL);

    info->res_x = elapsed * info->dx / 0.2;
    info->res_y = elapsed * info->dy / 0.2;

#if 0
    g_print ("%f %f\n", info->res_x, info->res_y);
#endif
    
    new_x = viewport.x + info->res_x;
    new_y = viewport.y + info->res_y;

#if 0
    g_print ("%f\n", elapsed * (info->dx / 0.2));
#endif
    
#if 0
    g_print ("new_x, new_y\n: %d %d\n", new_x, new_y);
#endif
    
    foo_scroll_area_set_viewport_pos (area, new_x, new_y);
#if 0
				      viewport.x + info->dx,
				      viewport.y + info->dy);
#endif

    get_viewport (area, &new_viewport);

    if (viewport.x == new_viewport.x		&&
	viewport.y == new_viewport.y		&&
	(info->res_x > 1.0			||
	 info->res_y > 1.0			||
	 info->res_x < -1.0			||
	 info->res_y < -1.0))
    {
	stop_scrolling (area);
	
	/* stop scrolling if it didn't have an effect */
	return FALSE;
    }
    
    return TRUE;
}

static void
ensure_scrolling (FooScrollArea *area,
		  int		 dx,
		  int		 dy)
{
    if (!area->priv->auto_scroll_info)
    {
#if 0
	g_print ("start scrolling\n");
#endif
	area->priv->auto_scroll_info = g_new0 (AutoScrollInfo, 1);
	area->priv->auto_scroll_info->timeout_id =
	    g_idle_add (scroll_idle, area);
	area->priv->auto_scroll_info->timer = g_timer_new ();
    }
    
#if 0
    g_print ("setting scrolling to %d %d\n", dx, dy);
#endif

#if 0
    g_print ("dx, dy: %d %d\n", dx, dy);
#endif
    
    area->priv->auto_scroll_info->dx = dx;
    area->priv->auto_scroll_info->dy = dy;
}

void
foo_scroll_area_auto_scroll (FooScrollArea *scroll_area,
			     FooScrollAreaEvent *event)
{
    GdkRectangle viewport;
    
    get_viewport (scroll_area, &viewport);
    
    if (rect_contains (&viewport, event->x, event->y))
    {
	stop_scrolling (scroll_area);
    }
    else
    {
	int dx, dy;
	
	dx = dy = 0;
	
	if (event->y < viewport.y)
	{
	    dy = event->y - viewport.y;
	    dy = MIN (dy + 2, 0);
	}
	else if (event->y >= viewport.y + viewport.height)
	{
	    dy = event->y - (viewport.y + viewport.height - 1);
	    dy = MAX (dy - 2, 0);
	}
	
	if (event->x < viewport.x)
	{
	    dx = event->x - viewport.x;
	    dx = MIN (dx + 2, 0);
	}
	else if (event->x >= viewport.x + viewport.width)
	{
	    dx = event->x - (viewport.x + viewport.width - 1);
	    dx = MAX (dx - 2, 0);
	}

#if 0
	g_print ("dx, dy: %d %d\n", dx, dy);
#endif
	
	ensure_scrolling (scroll_area, dx, dy);
    }
}

void
foo_scroll_area_begin_auto_scroll (FooScrollArea *scroll_area)
{
    /* noop  for now */
}

void
foo_scroll_area_end_auto_scroll (FooScrollArea *scroll_area)
{
    stop_scrolling (scroll_area);
}



#if 0
/*
 * Backing Store
 */
struct BackingStore
{
    GdkPixmap *pixmap;
    GdkRegion *update_region;
    int width;
    int height;
};

static BackingStore *
backing_store_new (GdkWindow *window,
		   int width, int height)
{
    BackingStore *store = g_new0 (BackingStore, 1);
    GdkRectangle rect = { 0, 0, width, height };
    
    store->pixmap = gdk_pixmap_new (window, width, height, -1);
    store->update_region = gdk_region_rectangle (&rect);
    store->width = width;
    store->height = height;

    return store;
}

static void
backing_store_free (BackingStore *store)
{
    g_object_unref (store->pixmap);
    gdk_region_destroy (store->update_region);
    g_free (store);
}

static void
backing_store_draw (BackingStore  *store, 
		    GdkDrawable   *dest,
		    GdkRegion     *clip,
		    int		   x,
		    int            y)
{
    GdkGC *gc = gdk_gc_new (dest);
    
    gdk_gc_set_clip_region (gc, clip);

    gdk_draw_drawable (dest, gc, store->pixmap,
		       0, 0, x, y, store->width, store->height);

    g_object_unref (gc);
}

static void
backing_store_scroll (BackingStore *store,
		      int           dx,
		      int           dy)
{
    GdkGC *gc = gdk_gc_new (store->pixmap);
    GdkRectangle rect;
    
    gdk_draw_drawable (store->pixmap, gc, store->pixmap,
		       0, 0, dx, dy,
		       store->width, store->height);

    /* Invalidate vertically */
    rect.x = 0;
    rect.width = store->width;

    if (dy > 0)
    {
	rect.y = 0;
	rect.height = dy;
    }
    else
    {
	rect.y = store->height + dy;
	rect.y = -dy;
    }

    gdk_region_union_with_rect (store->update_region, &rect);
    
    /* Invalidate horizontally */
    rect.y = 0;
    rect.height = store->height;

    if (dx > 0)
    {
	rect.x = 0;
	rect.width = dx;
    }
    else
    {
	rect.x = store->width + dx;
	rect.width = -dx;
    }

    gdk_region_union_with_rect (store->update_region, &rect);
}

static void
backing_store_invalidate_rect (BackingStore *store,
			       GdkRectangle *rect)
{
    gdk_region_union_with_rect (store->update_region, rect);
}

static void
backing_store_invalidate_region (BackingStore *store,
				 GdkRegion *region)
{
    gdk_region_union (store->update_region, region);
}

static void
backing_store_invalidate_all (BackingStore *store)
{
    GdkRectangle rect = { 0, 0, store->width, store->height };
    gdk_region_destroy (store->update_region);
    store->update_region = gdk_region_rectangle (&rect);
}

static void
backing_store_resize (BackingStore *store,
		      int           width,
		      int           height)
{
    GdkPixmap *pixmap = gdk_pixmap_new (store->pixmap, width, height, -1);

    /* Unfortunately we don't know in which direction we were resized,
     * so we just assume we were dragged from the south-east corner.
     *
     * Although, maybe we could get the root coordinates of the input-window?
     * That might just work, actually. We need to make sure marco uses
     * static gravity for the window before this will be useful.
     */
    simple_draw_drawable (pixmap, store->pixmap, 0, 0, 0, 0, -1, -1);

    g_object_unref (store->pixmap);

    store->pixmap = pixmap;

    /* FIXME: invalidate uncovered strip only */

    backing_store_invalidate_all (store);
}

static void
cclip_to_region (cairo_t *cr, GdkRegion *region)
{
    int n_rects;
    GdkRectangle *rects;

    gdk_region_get_rectangles (region, &rects, &n_rects);

    cairo_new_path (cr);
    while (n_rects--)
    {
	GdkRectangle *rect = &(rects[n_rects]);

	cairo_rectangle (cr, rect->x, rect->y, rect->width, rect->height);
    }
    cairo_clip (cr);

    g_free (rects);
}

static void
backing_store_process_updates (BackingStore *store,
			       ExposeFunc    func,
			       gpointer      data)
{
    cairo_t *cr = gdk_cairo_create (store->pixmap);
    GdkRegion *region = store->update_region;
    store->update_region = gdk_region_new ();

    cclip_to_region (cr, store->update_region);

    func (cr, store->update_region, data);

    gdk_region_destroy (region);
    cairo_destroy (cr);
}

#endif
