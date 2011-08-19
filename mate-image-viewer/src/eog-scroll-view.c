#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#endif

#include "eog-marshal.h"
#include "eog-scroll-view.h"
#include "eog-debug.h"
#include "uta.h"
#include "zoom.h"

/* Maximum size of delayed repaint rectangles */
#define PAINT_RECT_WIDTH 128
#define PAINT_RECT_HEIGHT 128

/* Scroll step increment */
#define SCROLL_STEP_SIZE 32

/* Maximum zoom factor */
#define MAX_ZOOM_FACTOR 20
#define MIN_ZOOM_FACTOR 0.02

#define CHECK_MEDIUM 8
#define CHECK_BLACK 0x00000000
#define CHECK_DARK 0x00555555
#define CHECK_GRAY 0x00808080
#define CHECK_LIGHT 0x00cccccc
#define CHECK_WHITE 0x00ffffff

/* Default increment for zooming.  The current zoom factor is multiplied or
 * divided by this amount on every zooming step.  For consistency, you should
 * use the same value elsewhere in the program.
 */
#define IMAGE_VIEW_ZOOM_MULTIPLIER 1.05

/* States for automatically adjusting the zoom factor */
typedef enum {
	ZOOM_MODE_FIT,		/* Image is fitted to scroll view even if the latter changes size */
	ZOOM_MODE_FREE		/* The image remains at its current zoom factor even if the scrollview changes size  */
} ZoomMode;

/* Progressive loading state */
typedef enum {
	PROGRESSIVE_NONE,	/* We are not loading an image or it is already loaded */
	PROGRESSIVE_LOADING,	/* An image is being loaded */
	PROGRESSIVE_POLISHING	/* We have finished loading an image but have not scaled it with interpolation */
} ProgressiveState;

/* Signal IDs */
enum {
	SIGNAL_ZOOM_CHANGED,
	SIGNAL_LAST
};
static gint view_signals [SIGNAL_LAST];

typedef enum {
	EOG_SCROLL_VIEW_CURSOR_NORMAL,
	EOG_SCROLL_VIEW_CURSOR_HIDDEN,
	EOG_SCROLL_VIEW_CURSOR_DRAG
} EogScrollViewCursor;

/* Drag 'n Drop */
static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, 0},
};

enum {
	PROP_0,
	PROP_USE_BG_COLOR,
	PROP_BACKGROUND_COLOR
};

/* Private part of the EogScrollView structure */
struct _EogScrollViewPrivate {
	/* some widgets we rely on */
	GtkWidget *display;
	GtkAdjustment *hadj;
	GtkAdjustment *vadj;
	GtkWidget *hbar;
	GtkWidget *vbar;
	GtkWidget *menu;

	/* actual image */
	EogImage *image;
	guint image_changed_id;
	guint frame_changed_id;
	GdkPixbuf *pixbuf;

	/* zoom mode, either ZOOM_MODE_FIT or ZOOM_MODE_FREE */
	ZoomMode zoom_mode;

	/* whether to allow zoom > 1.0 on zoom fit */
	gboolean upscale;

	/* the actual zoom factor */
	double zoom;

	/* the minimum possible (reasonable) zoom factor */
	double min_zoom;

	/* Current scrolling offsets */
	int xofs, yofs;

	/* Microtile arrays for dirty region.  This represents the dirty region
	 * for interpolated drawing.
	 */
	EogUta *uta;

	/* handler ID for paint idle callback */
	guint idle_id;

	/* Interpolation type when zoomed in*/
	GdkInterpType interp_type_in;

	/* Interpolation type when zoomed out*/
	GdkInterpType interp_type_out;

	/* Scroll wheel zoom */
	gboolean scroll_wheel_zoom;

	/* Scroll wheel zoom */
	gdouble zoom_multiplier;

	/* dragging stuff */
	int drag_anchor_x, drag_anchor_y;
	int drag_ofs_x, drag_ofs_y;
	guint dragging : 1;

	/* status of progressive loading */
	ProgressiveState progressive_state;

	/* how to indicate transparency in images */
	EogTransparencyStyle transp_style;
	guint32 transp_color;

	/* the type of the cursor we are currently showing */
	EogScrollViewCursor cursor;

	gboolean  use_bg_color;
	GdkColor *background_color;
	GdkColor *override_bg_color;

	cairo_surface_t *background_surface;
};

static void scroll_by (EogScrollView *view, int xofs, int yofs);
static void set_zoom_fit (EogScrollView *view);
static void request_paint_area (EogScrollView *view, GdkRectangle *area);
static void set_minimum_zoom_factor (EogScrollView *view);

#define EOG_SCROLL_VIEW_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_SCROLL_VIEW, EogScrollViewPrivate))

G_DEFINE_TYPE (EogScrollView, eog_scroll_view, GTK_TYPE_TABLE)


/*===================================
    widget size changing handler &
        util functions
  ---------------------------------*/

/* Disconnects from the EogImage and removes references to it */
static void
free_image_resources (EogScrollView *view)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->image_changed_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->image), priv->image_changed_id);
		priv->image_changed_id = 0;
	}

	if (priv->frame_changed_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->image), priv->frame_changed_id);
		priv->frame_changed_id = 0;
	}

	if (priv->image != NULL) {
		eog_image_data_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}
}

/* Computes the size in pixels of the scaled image */
static void
compute_scaled_size (EogScrollView *view, double zoom, int *width, int *height)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->pixbuf) {
		*width = floor (gdk_pixbuf_get_width (priv->pixbuf) * zoom + 0.5);
		*height = floor (gdk_pixbuf_get_height (priv->pixbuf) * zoom + 0.5);
	} else
		*width = *height = 0;
}

/* Computes the offsets for the new zoom value so that they keep the image
 * centered on the view.
 */
static void
compute_center_zoom_offsets (EogScrollView *view,
			     double old_zoom, double new_zoom,
			     int width, int height,
			     double zoom_x_anchor, double zoom_y_anchor,
			     int *xofs, int *yofs)
{
	EogScrollViewPrivate *priv;
	int old_scaled_width, old_scaled_height;
	int new_scaled_width, new_scaled_height;
	double view_cx, view_cy;

	priv = view->priv;

	compute_scaled_size (view, old_zoom,
			     &old_scaled_width, &old_scaled_height);

	if (old_scaled_width < width)
		view_cx = (zoom_x_anchor * old_scaled_width) / old_zoom;
	else
		view_cx = (priv->xofs + zoom_x_anchor * width) / old_zoom;

	if (old_scaled_height < height)
		view_cy = (zoom_y_anchor * old_scaled_height) / old_zoom;
	else
		view_cy = (priv->yofs + zoom_y_anchor * height) / old_zoom;

	compute_scaled_size (view, new_zoom,
			     &new_scaled_width, &new_scaled_height);

	if (new_scaled_width < width)
		*xofs = 0;
	else {
		*xofs = floor (view_cx * new_zoom - zoom_x_anchor * width + 0.5);
		if (*xofs < 0)
			*xofs = 0;
	}

	if (new_scaled_height < height)
		*yofs = 0;
	else {
		*yofs = floor (view_cy * new_zoom - zoom_y_anchor * height + 0.5);
		if (*yofs < 0)
			*yofs = 0;
	}
}

/* Sets the scrollbar values based on the current scrolling offset */
static void
update_scrollbar_values (EogScrollView *view)
{
	EogScrollViewPrivate *priv;
	int scaled_width, scaled_height;
	int xofs, yofs;
	gdouble page_size,page_increment,step_increment;
	gdouble lower, upper, value;
	GtkAllocation allocation;

	priv = view->priv;

	if (!gtk_widget_get_visible (GTK_WIDGET (priv->hbar))
	    && !gtk_widget_get_visible (GTK_WIDGET (priv->vbar)))
		return;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);
	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	if (gtk_widget_get_visible (GTK_WIDGET (priv->hbar))) {
		/* Set scroll increments */
		page_size = MIN (scaled_width, allocation.width);
		page_increment = allocation.width / 2;
		step_increment = SCROLL_STEP_SIZE;

		/* Set scroll bounds and new offsets */
		lower = 0;
		upper = scaled_width;
		xofs = CLAMP (priv->xofs, 0, upper - page_size);
		if (gtk_adjustment_get_value (priv->hadj) != xofs) {
			value = xofs;
			priv->xofs = xofs;

			g_signal_handlers_block_matched (
				priv->hadj, G_SIGNAL_MATCH_DATA,
				0, 0, NULL, NULL, view);

			gtk_adjustment_configure (priv->hadj, value, lower,
						  upper, step_increment,
						  page_increment, page_size);

			g_signal_handlers_unblock_matched (
				priv->hadj, G_SIGNAL_MATCH_DATA,
				0, 0, NULL, NULL, view);
		}
	}

	if (gtk_widget_get_visible (GTK_WIDGET (priv->vbar))) {
		page_size = MIN (scaled_height, allocation.height);
		page_increment = allocation.height / 2;
		step_increment = SCROLL_STEP_SIZE;

		lower = 0;
		upper = scaled_height;
		yofs = CLAMP (priv->yofs, 0, upper - page_size);

		if (gtk_adjustment_get_value (priv->vadj) != yofs) {
			value = yofs;
			priv->yofs = yofs;

			g_signal_handlers_block_matched (
				priv->vadj, G_SIGNAL_MATCH_DATA,
				0, 0, NULL, NULL, view);

			gtk_adjustment_configure (priv->vadj, value, lower,
						  upper, step_increment,
						  page_increment, page_size);

			g_signal_handlers_unblock_matched (
				priv->vadj, G_SIGNAL_MATCH_DATA,
				0, 0, NULL, NULL, view);
		}
	}
}

static void
eog_scroll_view_set_cursor (EogScrollView *view, EogScrollViewCursor new_cursor)
{
	GdkCursor *cursor = NULL;
	GdkDisplay *display;
	GtkWidget *widget;

	if (view->priv->cursor == new_cursor) {
		return;
	}

	widget = gtk_widget_get_toplevel (GTK_WIDGET (view));
	display = gtk_widget_get_display (widget);
	view->priv->cursor = new_cursor;

	switch (new_cursor) {
		case EOG_SCROLL_VIEW_CURSOR_NORMAL:
			gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
			break;
                case EOG_SCROLL_VIEW_CURSOR_HIDDEN:
                        cursor = gdk_cursor_new (GDK_BLANK_CURSOR);
                        break;
		case EOG_SCROLL_VIEW_CURSOR_DRAG:
			cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
			break;
	}

	if (cursor) {
		gdk_window_set_cursor (gtk_widget_get_window (widget), cursor);
		gdk_cursor_unref (cursor);
		gdk_flush();
	}
}

/* Changes visibility of the scrollbars based on the zoom factor and the
 * specified allocation, or the current allocation if NULL is specified.
 */
static void
check_scrollbar_visibility (EogScrollView *view, GtkAllocation *alloc)
{
	EogScrollViewPrivate *priv;
	int bar_height;
	int bar_width;
	int img_width;
	int img_height;
	GtkRequisition req;
	int width, height;
	gboolean hbar_visible, vbar_visible;

	priv = view->priv;

	if (alloc) {
		width = alloc->width;
		height = alloc->height;
	} else {
		GtkAllocation allocation;

		gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);
		width = allocation.width;
		height = allocation.height;
	}

	compute_scaled_size (view, priv->zoom, &img_width, &img_height);

	/* this should work fairly well in this special case for scrollbars */
	gtk_widget_size_request (priv->hbar, &req);
	bar_height = req.height;
	gtk_widget_size_request (priv->vbar, &req);
	bar_width = req.width;

	eog_debug_message (DEBUG_WINDOW, "Widget Size allocate: %i, %i   Bar: %i, %i\n",
			   width, height, bar_width, bar_height);

	hbar_visible = vbar_visible = FALSE;
	if (priv->zoom_mode == ZOOM_MODE_FIT)
		hbar_visible = vbar_visible = FALSE;
	else if (img_width <= width && img_height <= height)
		hbar_visible = vbar_visible = FALSE;
	else if (img_width > width && img_height > height)
		hbar_visible = vbar_visible = TRUE;
	else if (img_width > width) {
		hbar_visible = TRUE;
		if (img_height <= (height - bar_height))
			vbar_visible = FALSE;
		else
			vbar_visible = TRUE;
	}
        else if (img_height > height) {
		vbar_visible = TRUE;
		if (img_width <= (width - bar_width))
			hbar_visible = FALSE;
		else
			hbar_visible = TRUE;
	}

	if (hbar_visible != gtk_widget_get_visible (GTK_WIDGET (priv->hbar)))
		g_object_set (G_OBJECT (priv->hbar), "visible", hbar_visible, NULL);

	if (vbar_visible != gtk_widget_get_visible (GTK_WIDGET (priv->vbar)))
		g_object_set (G_OBJECT (priv->vbar), "visible", vbar_visible, NULL);
}

#define DOUBLE_EQUAL_MAX_DIFF 1e-6
#define DOUBLE_EQUAL(a,b) (fabs (a - b) < DOUBLE_EQUAL_MAX_DIFF)

/* Returns whether the zoom factor is 1.0 */
static gboolean
is_unity_zoom (EogScrollView *view)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;
	return DOUBLE_EQUAL (priv->zoom, 1.0);
}

/* Returns whether the image is zoomed in */
static gboolean
is_zoomed_in (EogScrollView *view)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;
	return priv->zoom - 1.0 > DOUBLE_EQUAL_MAX_DIFF;
}

/* Returns whether the image is zoomed out */
static gboolean
is_zoomed_out (EogScrollView *view)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;
	return DOUBLE_EQUAL_MAX_DIFF + priv->zoom - 1.0 < 0.0;
}

/* Returns wether the image is movable, that means if it is larger then
 * the actual visible area.
 */
static gboolean
is_image_movable (EogScrollView *view)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;

	return (gtk_widget_get_visible (priv->hbar) || gtk_widget_get_visible (priv->vbar));
}


/* Computes the image offsets with respect to the window */
/*
static void
get_image_offsets (EogScrollView *view, int *xofs, int *yofs)
{
	EogScrollViewPrivate *priv;
	int scaled_width, scaled_height;
	int width, height;

	priv = view->priv;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

	width = GTK_WIDGET (priv->display)->allocation.width;
	height = GTK_WIDGET (priv->display)->allocation.height;

	// Compute image offsets with respect to the window
	if (scaled_width <= width)
		*xofs = (width - scaled_width) / 2;
	else
		*xofs = -priv->xofs;

	if (scaled_height <= height)
		*yofs = (height - scaled_height) / 2;
	else
		*yofs = -priv->yofs;
}
*/

/*===================================
          drawing core
  ---------------------------------*/


/* Pulls a rectangle from the specified microtile array.  The rectangle is the
 * first one that would be glommed together by art_rect_list_from_uta(), and its
 * size is bounded by max_width and max_height.  The rectangle is also removed
 * from the microtile array.
 */
static void
pull_rectangle (EogUta *uta, EogIRect *rect, int max_width, int max_height)
{
	uta_find_first_glom_rect (uta, rect, max_width, max_height);
	uta_remove_rect (uta, rect->x0, rect->y0, rect->x1, rect->y1);
}

/* Paints a rectangle with the background color if the specified rectangle
 * intersects the dirty rectangle.
 */
static void
paint_background (EogScrollView *view, EogIRect *r, EogIRect *rect)
{
	EogScrollViewPrivate *priv;
	EogIRect d;

	priv = view->priv;

	eog_irect_intersect (&d, r, rect);
	if (!eog_irect_empty (&d)) {
		gdk_window_clear_area (gtk_widget_get_window (priv->display),
				       d.x0, d.y0,
				       d.x1 - d.x0, d.y1 - d.y0);
	}
}

static void
get_transparency_params (EogScrollView *view, int *size, guint32 *color1, guint32 *color2)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;

	/* Compute transparency parameters */
	switch (priv->transp_style) {
	case EOG_TRANSP_BACKGROUND: {
		GdkColor color = gtk_widget_get_style (GTK_WIDGET (priv->display))->bg[GTK_STATE_NORMAL];

		*color1 = *color2 = (((color.red & 0xff00) << 8)
				       | (color.green & 0xff00)
				       | ((color.blue & 0xff00) >> 8));
		break; }

	case EOG_TRANSP_CHECKED:
		*color1 = CHECK_GRAY;
		*color2 = CHECK_LIGHT;
		break;

	case EOG_TRANSP_COLOR:
		*color1 = *color2 = priv->transp_color;
		break;

	default:
		g_assert_not_reached ();
	};

	*size = CHECK_MEDIUM;
}

#ifdef HAVE_RSVG
static cairo_surface_t *
create_background_surface (EogScrollView *view)
{
	int check_size;
	guint32 check_1 = 0;
	guint32 check_2 = 0;
	cairo_surface_t *surface;
	cairo_t *check_cr;

	get_transparency_params (view, &check_size, &check_1, &check_2);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, check_size * 2, check_size * 2);
	check_cr = cairo_create (surface);
	cairo_set_source_rgba (check_cr,
			       ((check_1 & 0xff0000) >> 16) / 255.,
			       ((check_1 & 0x00ff00) >> 8)  / 255.,
			        (check_1 & 0x0000ff)        / 255.,
				1.);
	cairo_rectangle (check_cr, 0., 0., check_size, check_size);
	cairo_fill (check_cr);
	cairo_translate (check_cr, check_size, check_size);
	cairo_rectangle (check_cr, 0., 0., check_size, check_size);
	cairo_fill (check_cr);

	cairo_set_source_rgba (check_cr,
			       ((check_2 & 0xff0000) >> 16) / 255.,
			       ((check_2 & 0x00ff00) >> 8)  / 255.,
			        (check_2 & 0x0000ff)        / 255.,
				1.);
	cairo_translate (check_cr, -check_size, 0);
	cairo_rectangle (check_cr, 0., 0., check_size, check_size);
	cairo_fill (check_cr);
	cairo_translate (check_cr, check_size, -check_size);
	cairo_rectangle (check_cr, 0., 0., check_size, check_size);
	cairo_fill (check_cr);
	cairo_destroy (check_cr);

	return surface;
}

static void
draw_svg_background (EogScrollView *view, cairo_t *cr, EogIRect *render_rect, EogIRect *image_rect)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->background_surface == NULL)
		priv->background_surface = create_background_surface (view);

	cairo_set_source_surface (cr, priv->background_surface,
				  - (render_rect->x0 - image_rect->x0) % (CHECK_MEDIUM * 2),
				  - (render_rect->y0 - image_rect->y0) % (CHECK_MEDIUM * 2));
	cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
	cairo_rectangle (cr,
			 0,
			 0,
			 render_rect->x1 - render_rect->x0,
			 render_rect->y1 - render_rect->y0);
	cairo_fill (cr);
}

static cairo_surface_t *
draw_svg_on_image_surface (EogScrollView *view, EogIRect *render_rect, EogIRect *image_rect)
{
	EogScrollViewPrivate *priv;
	cairo_t *cr;
	cairo_surface_t *surface;
	cairo_matrix_t matrix, translate, scale;
	EogTransform *transform;

	priv = view->priv;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      render_rect->x1 - render_rect->x0,
					      render_rect->y1 - render_rect->y0);
	cr = cairo_create (surface);

	cairo_save (cr);
	draw_svg_background (view, cr, render_rect, image_rect);
	cairo_restore (cr);

	cairo_matrix_init_identity (&matrix);
	transform = eog_image_get_transform (priv->image);
	if (transform) {
		cairo_matrix_t affine;
		double image_offset_x = 0., image_offset_y = 0.;

		eog_transform_get_affine (transform, &affine);
		cairo_matrix_multiply (&matrix, &affine, &matrix);

		switch (eog_transform_get_transform_type (transform)) {
		case EOG_TRANSFORM_ROT_90:
		case EOG_TRANSFORM_FLIP_HORIZONTAL:
			image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
			break;
		case EOG_TRANSFORM_ROT_270:
		case EOG_TRANSFORM_FLIP_VERTICAL:
			image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
			break;
		case EOG_TRANSFORM_ROT_180:
		case EOG_TRANSFORM_TRANSPOSE:
		case EOG_TRANSFORM_TRANSVERSE:
			image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
			image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
			break;
		case EOG_TRANSFORM_NONE:
		default:
			break;
		}

		cairo_matrix_init_translate (&translate, image_offset_x, image_offset_y);
		cairo_matrix_multiply (&matrix, &matrix, &translate);
	}

	cairo_matrix_init_scale (&scale, priv->zoom, priv->zoom);
	cairo_matrix_multiply (&matrix, &matrix, &scale);
	cairo_matrix_init_translate (&translate, image_rect->x0, image_rect->y0);
	cairo_matrix_multiply (&matrix, &matrix, &translate);
	cairo_matrix_init_translate (&translate, -render_rect->x0, -render_rect->y0);
	cairo_matrix_multiply (&matrix, &matrix, &translate);

	cairo_set_matrix (cr, &matrix);

	rsvg_handle_render_cairo (eog_image_get_svg (priv->image), cr);
	cairo_destroy (cr);

	return surface;
}

static void
draw_svg (EogScrollView *view, EogIRect *render_rect, EogIRect *image_rect)
{
	EogScrollViewPrivate *priv;
	cairo_t *cr;
	cairo_surface_t *surface;
	GdkWindow *window;

	priv = view->priv;

	window = gtk_widget_get_window (GTK_WIDGET (priv->display));
	surface = draw_svg_on_image_surface (view, render_rect, image_rect);

	cr = gdk_cairo_create (window);
	cairo_set_source_surface (cr, surface, render_rect->x0, render_rect->y0);
	cairo_paint (cr);
	cairo_destroy (cr);
}
#endif

/* Paints a rectangle of the dirty region */
static void
paint_rectangle (EogScrollView *view, EogIRect *rect, GdkInterpType interp_type)
{
	EogScrollViewPrivate *priv;
	GdkPixbuf *tmp;
	char *str;
	GtkAllocation allocation;
	int scaled_width, scaled_height;
	int xofs, yofs;
	EogIRect r, d;
	int check_size;
	guint32 check_1 = 0;
	guint32 check_2 = 0;

	priv = view->priv;

	if (!gtk_widget_is_drawable (priv->display))
		return;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	if (scaled_width < 1 || scaled_height < 1)
	{
		r.x0 = 0;
		r.y0 = 0;
		r.x1 = allocation.width;
		r.y1 = allocation.height;
		paint_background (view, &r, rect);
		return;
	}

	/* Compute image offsets with respect to the window */

	if (scaled_width <= allocation.width)
		xofs = (allocation.width - scaled_width) / 2;
	else
		xofs = -priv->xofs;

	if (scaled_height <= allocation.height)
		yofs = (allocation.height - scaled_height) / 2;
	else
		yofs = -priv->yofs;

	eog_debug_message (DEBUG_WINDOW, "zoom %.2f, xofs: %i, yofs: %i scaled w: %i h: %i\n",
			   priv->zoom, xofs, yofs, scaled_width, scaled_height);

	/* Draw background if necessary, in four steps */

	/* Top */
	if (yofs > 0) {
		r.x0 = 0;
		r.y0 = 0;
		r.x1 = allocation.width;
		r.y1 = yofs;
		paint_background (view, &r, rect);
	}

	/* Left */
	if (xofs > 0) {
		r.x0 = 0;
		r.y0 = yofs;
		r.x1 = xofs;
		r.y1 = yofs + scaled_height;
		paint_background (view, &r, rect);
	}

	/* Right */
	if (xofs >= 0) {
		r.x0 = xofs + scaled_width;
		r.y0 = yofs;
		r.x1 = allocation.width;
		r.y1 = yofs + scaled_height;
		if (r.x0 < r.x1)
			paint_background (view, &r, rect);
	}

	/* Bottom */
	if (yofs >= 0) {
		r.x0 = 0;
		r.y0 = yofs + scaled_height;
		r.x1 = allocation.width;
		r.y1 = allocation.height;
		if (r.y0 < r.y1)
			paint_background (view, &r, rect);
	}


	/* Draw the scaled image
	 *
	 * FIXME: this is not using the color correction tables!
	 */

	if (!priv->pixbuf)
		return;

	r.x0 = xofs;
	r.y0 = yofs;
	r.x1 = xofs + scaled_width;
	r.y1 = yofs + scaled_height;

	eog_irect_intersect (&d, &r, rect);
	if (eog_irect_empty (&d))
		return;

	switch (interp_type) {
	case GDK_INTERP_NEAREST:
		str = "NEAREST";
		break;
	default:
		str = "ALIASED";
	}

	eog_debug_message (DEBUG_WINDOW, "%s: x0: %i,\t y0: %i,\t x1: %i,\t y1: %i\n",
			   str, d.x0, d.y0, d.x1, d.y1);

#ifdef HAVE_RSVG
	if (eog_image_is_svg (view->priv->image) && interp_type != GDK_INTERP_NEAREST) {
		draw_svg (view, &d, &r);
		return;
	}
#endif
	/* Short-circuit the fast case to avoid a memcpy() */

	if (is_unity_zoom (view)
	    && gdk_pixbuf_get_colorspace (priv->pixbuf) == GDK_COLORSPACE_RGB
	    && !gdk_pixbuf_get_has_alpha (priv->pixbuf)
	    && gdk_pixbuf_get_bits_per_sample (priv->pixbuf) == 8) {
		guchar *pixels;
		int rowstride;

		rowstride = gdk_pixbuf_get_rowstride (priv->pixbuf);

		pixels = (gdk_pixbuf_get_pixels (priv->pixbuf)
			  + (d.y0 - yofs) * rowstride
			  + 3 * (d.x0 - xofs));

		gdk_draw_rgb_image_dithalign (gtk_widget_get_window (GTK_WIDGET (priv->display)),
					      gtk_widget_get_style (GTK_WIDGET (priv->display))->black_gc,
					      d.x0, d.y0,
					      d.x1 - d.x0, d.y1 - d.y0,
					      GDK_RGB_DITHER_MAX,
					      pixels,
					      rowstride,
					      d.x0 - xofs, d.y0 - yofs);
		return;
	}

	/* For all other cases, create a temporary pixbuf */

	tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, d.x1 - d.x0, d.y1 - d.y0);

	if (!tmp) {
		g_message ("paint_rectangle(): Could not allocate temporary pixbuf of "
			   "size (%d, %d); skipping", d.x1 - d.x0, d.y1 - d.y0);
		return;
	}

	/* Compute transparency parameters */
	get_transparency_params (view, &check_size, &check_1, &check_2);

	/* Draw! */
	gdk_pixbuf_composite_color (priv->pixbuf,
				    tmp,
				    0, 0,
				    d.x1 - d.x0, d.y1 - d.y0,
				    -(d.x0 - xofs), -(d.y0 - yofs),
				    priv->zoom, priv->zoom,
				    is_unity_zoom (view) ? GDK_INTERP_NEAREST : interp_type,
				    255,
				    d.x0 - xofs, d.y0 - yofs,
				    check_size,
				    check_1, check_2);

	gdk_draw_rgb_image_dithalign (gtk_widget_get_window (priv->display),
				      gtk_widget_get_style (priv->display)->black_gc,
				      d.x0, d.y0,
				      d.x1 - d.x0, d.y1 - d.y0,
				      GDK_RGB_DITHER_MAX,
				      gdk_pixbuf_get_pixels (tmp),
				      gdk_pixbuf_get_rowstride (tmp),
				      d.x0 - xofs, d.y0 - yofs);

	g_object_unref (tmp);
}


/* Idle handler for the drawing process.  We pull a rectangle from the dirty
 * region microtile array, paint it, and leave the rest to the next idle
 * iteration.
 */
static gboolean
paint_iteration_idle (gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;
	EogIRect rect;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	g_assert (priv->uta != NULL);

	pull_rectangle (priv->uta, &rect, PAINT_RECT_WIDTH, PAINT_RECT_HEIGHT);

	if (eog_irect_empty (&rect)) {
		eog_uta_free (priv->uta);
		priv->uta = NULL;
	} else {
		if (is_zoomed_in (view))
			paint_rectangle (view, &rect, priv->interp_type_in);
		else if (is_zoomed_out (view))
			paint_rectangle (view, &rect, priv->interp_type_out);
		else
			paint_rectangle (view, &rect, GDK_INTERP_NEAREST);
	}
		
	if (!priv->uta) {
		priv->idle_id = 0;
		return FALSE;
	}

	return TRUE;
}

/* Paints the requested area in non-interpolated mode.  Then, if we are
 * configured to use interpolation, we queue an idle handler to redraw the area
 * with interpolation.  The area is in window coordinates.
 */
static void
request_paint_area (EogScrollView *view, GdkRectangle *area)
{
	EogScrollViewPrivate *priv;
	EogIRect r;
	GtkAllocation allocation;

	priv = view->priv;

	eog_debug_message (DEBUG_WINDOW, "x: %i, y: %i, width: %i, height: %i\n",
			   area->x, area->y, area->width, area->height);

	if (!gtk_widget_is_drawable (priv->display))
		return;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);
	r.x0 = MAX (0, area->x);
	r.y0 = MAX (0, area->y);
	r.x1 = MIN (allocation.width, area->x + area->width);
	r.y1 = MIN (allocation.height, area->y + area->height);

	eog_debug_message (DEBUG_WINDOW, "r: %i, %i, %i, %i\n", r.x0, r.y0, r.x1, r.y1);

	if (r.x0 >= r.x1 || r.y0 >= r.y1)
		return;

	/* Do nearest neighbor, 1:1 zoom or active progressive loading synchronously for speed.  */
	if ((is_zoomed_in (view) && priv->interp_type_in == GDK_INTERP_NEAREST) ||
	    (is_zoomed_out (view) && priv->interp_type_out == GDK_INTERP_NEAREST) ||
	    is_unity_zoom (view) ||
	    priv->progressive_state == PROGRESSIVE_LOADING) {
		paint_rectangle (view, &r, GDK_INTERP_NEAREST);
		return;
	}

	if (priv->progressive_state == PROGRESSIVE_POLISHING)
		/* We have already a complete image with nearest neighbor mode.
		 * It's sufficient to add only a antitaliased idle update
		 */
		priv->progressive_state = PROGRESSIVE_NONE;
	else if (!priv->image || !eog_image_is_animation (priv->image))
		/* do nearest neigbor before anti aliased version,
		   except for animations to avoid a "blinking" effect. */
		paint_rectangle (view, &r, GDK_INTERP_NEAREST);

	/* All other interpolation types are delayed.  */
	if (priv->uta)
		g_assert (priv->idle_id != 0);
	else {
		g_assert (priv->idle_id == 0);
		priv->idle_id = g_idle_add (paint_iteration_idle, view);
	}

	priv->uta = uta_add_rect (priv->uta, r.x0, r.y0, r.x1, r.y1);
}


/* =======================================

    scrolling stuff

    --------------------------------------*/


/* Scrolls the view to the specified offsets.  */
static void
scroll_to (EogScrollView *view, int x, int y, gboolean change_adjustments)
{
	EogScrollViewPrivate *priv;
	GtkAllocation allocation;
	int xofs, yofs;
	GdkWindow *window;
	int src_x, src_y;
	int dest_x, dest_y;
	int twidth, theight;

	priv = view->priv;

	/* Check bounds & Compute offsets */
	if (gtk_widget_get_visible (priv->hbar)) {
		x = CLAMP (x, 0, gtk_adjustment_get_upper (priv->hadj)
				 - gtk_adjustment_get_page_size (priv->hadj));
		xofs = x - priv->xofs;
	} else
		xofs = 0;

	if (gtk_widget_get_visible (priv->vbar)) {
		y = CLAMP (y, 0, gtk_adjustment_get_upper (priv->vadj)
				 - gtk_adjustment_get_page_size (priv->vadj));
		yofs = y - priv->yofs;
	} else
		yofs = 0;

	if (xofs == 0 && yofs == 0)
		return;

	priv->xofs = x;
	priv->yofs = y;

	if (!gtk_widget_is_drawable (priv->display))
		goto out;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	if (abs (xofs) >= allocation.width || abs (yofs) >= allocation.height) {
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		goto out;
	}

	window = gtk_widget_get_window (GTK_WIDGET (priv->display));

	/* Ensure that the uta has the full size */

	twidth = (allocation.width + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT;
	theight = (allocation.height + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT;

	if (priv->uta)
		g_assert (priv->idle_id != 0);
	else
		priv->idle_id = g_idle_add (paint_iteration_idle, view);

	priv->uta = uta_ensure_size (priv->uta, 0, 0, twidth, theight);

	/* Copy the uta area.  Our synchronous handling of expose events, below,
	 * will queue the new scrolled-in areas.
	 */
	src_x = xofs < 0 ? 0 : xofs;
	src_y = yofs < 0 ? 0 : yofs;
	dest_x = xofs < 0 ? -xofs : 0;
	dest_y = yofs < 0 ? -yofs : 0;

	uta_copy_area (priv->uta,
		       src_x, src_y,
		       dest_x, dest_y,
		       allocation.width - abs (xofs),
		       allocation.height - abs (yofs));

	/* Scroll the window area and process exposure synchronously. */

	gdk_window_scroll (window, -xofs, -yofs);
	gdk_window_process_updates (window, TRUE);

 out:
	if (!change_adjustments)
		return;

	g_signal_handlers_block_matched (
		priv->hadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
	g_signal_handlers_block_matched (
		priv->vadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);

	gtk_adjustment_set_value (priv->hadj, x);
	gtk_adjustment_set_value (priv->vadj, y);

	g_signal_handlers_unblock_matched (
		priv->hadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
	g_signal_handlers_unblock_matched (
		priv->vadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
}

/* Scrolls the image view by the specified offsets.  Notifies the adjustments
 * about their new values.
 */
static void
scroll_by (EogScrollView *view, int xofs, int yofs)
{
	EogScrollViewPrivate *priv;

	priv = view->priv;

	scroll_to (view, priv->xofs + xofs, priv->yofs + yofs, TRUE);
}


/* Callback used when an adjustment is changed */
static void
adjustment_changed_cb (GtkAdjustment *adj, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	scroll_to (view, gtk_adjustment_get_value (priv->hadj),
		   gtk_adjustment_get_value (priv->vadj), FALSE);
}


/* Drags the image to the specified position */
static void
drag_to (EogScrollView *view, int x, int y)
{
	EogScrollViewPrivate *priv;
	int dx, dy;

	priv = view->priv;

	dx = priv->drag_anchor_x - x;
	dy = priv->drag_anchor_y - y;

	x = priv->drag_ofs_x + dx;
	y = priv->drag_ofs_y + dy;

	scroll_to (view, x, y, TRUE);
}

static void
set_minimum_zoom_factor (EogScrollView *view)
{
	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	view->priv->min_zoom = MAX (1.0 / gdk_pixbuf_get_width (view->priv->pixbuf),
				    MAX(1.0 / gdk_pixbuf_get_height (view->priv->pixbuf),
					MIN_ZOOM_FACTOR) );
	return;
}

/**
 * set_zoom:
 * @view: A scroll view.
 * @zoom: Zoom factor.
 * @have_anchor: Whether the anchor point specified by (@anchorx, @anchory)
 * should be used.
 * @anchorx: Horizontal anchor point in pixels.
 * @anchory: Vertical anchor point in pixels.
 *
 * Sets the zoom factor for an image view.  The anchor point can be used to
 * specify the point that stays fixed when the image is zoomed.  If @have_anchor
 * is %TRUE, then (@anchorx, @anchory) specify the point relative to the image
 * view widget's allocation that will stay fixed when zooming.  If @have_anchor
 * is %FALSE, then the center point of the image view will be used.
 **/
static void
set_zoom (EogScrollView *view, double zoom,
	  gboolean have_anchor, int anchorx, int anchory)
{
	EogScrollViewPrivate *priv;
	GtkAllocation allocation;
	int xofs, yofs;
	double x_rel, y_rel;

	g_assert (zoom > 0.0);

	priv = view->priv;

	if (priv->pixbuf == NULL)
		return;

	if (zoom > MAX_ZOOM_FACTOR)
		zoom = MAX_ZOOM_FACTOR;
	else if (zoom < MIN_ZOOM_FACTOR)
		zoom = MIN_ZOOM_FACTOR;

	if (DOUBLE_EQUAL (priv->zoom, zoom))
		return;
	if (DOUBLE_EQUAL (priv->zoom, priv->min_zoom) && zoom < priv->zoom)
		return;

	priv->zoom_mode = ZOOM_MODE_FREE;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	/* compute new xofs/yofs values */
	if (have_anchor) {
		x_rel = (double) anchorx / allocation.width;
		y_rel = (double) anchory / allocation.height;
	} else {
		x_rel = 0.5;
		y_rel = 0.5;
	}

	compute_center_zoom_offsets (view, priv->zoom, zoom,
				     allocation.width, allocation.height,
				     x_rel, y_rel,
				     &xofs, &yofs);

	/* set new values */
	priv->xofs = xofs; /* (img_width * x_rel * zoom) - anchorx; */
	priv->yofs = yofs; /* (img_height * y_rel * zoom) - anchory; */
#if 0
	g_print ("xofs: %i  yofs: %i\n", priv->xofs, priv->yofs);
#endif
	if (zoom <= priv->min_zoom)
		priv->zoom = priv->min_zoom;
	else
		priv->zoom = zoom;

	/* we make use of the new values here */
	check_scrollbar_visibility (view, NULL);
	update_scrollbar_values (view);

	/* repaint the whole image */
	gtk_widget_queue_draw (GTK_WIDGET (priv->display));

	g_signal_emit (view, view_signals [SIGNAL_ZOOM_CHANGED], 0, priv->zoom);
}

/* Zooms the image to fit the available allocation */
static void
set_zoom_fit (EogScrollView *view)
{
	EogScrollViewPrivate *priv;
	GtkAllocation allocation;
	double new_zoom;

	priv = view->priv;

	priv->zoom_mode = ZOOM_MODE_FIT;

	if (!gtk_widget_get_mapped (GTK_WIDGET (view)))
		return;

	if (priv->pixbuf == NULL)
		return;

	gtk_widget_get_allocation (GTK_WIDGET(priv->display), &allocation);

	new_zoom = zoom_fit_scale (allocation.width, allocation.height,
				   gdk_pixbuf_get_width (priv->pixbuf),
				   gdk_pixbuf_get_height (priv->pixbuf),
				   priv->upscale);

	if (new_zoom > MAX_ZOOM_FACTOR)
		new_zoom = MAX_ZOOM_FACTOR;
	else if (new_zoom < MIN_ZOOM_FACTOR)
		new_zoom = MIN_ZOOM_FACTOR;

	priv->zoom = new_zoom;
	priv->xofs = 0;
	priv->yofs = 0;

	g_signal_emit (view, view_signals [SIGNAL_ZOOM_CHANGED], 0, priv->zoom);
}

/*===================================

   internal signal callbacks

  ---------------------------------*/

/* Key press event handler for the image view */
static gboolean
display_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;
	GtkAllocation allocation;
	gboolean do_zoom;
	double zoom;
	gboolean do_scroll;
	int xofs, yofs;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	do_zoom = FALSE;
	do_scroll = FALSE;
	xofs = yofs = 0;
	zoom = 1.0;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	/* EogScrollView doesn't handle/have any Alt+Key combos */
	if (event->state & GDK_MOD1_MASK) {
		return FALSE;
	}

	switch (event->keyval) {
	case GDK_Up:
		do_scroll = TRUE;
		xofs = 0;
		yofs = -SCROLL_STEP_SIZE;
		break;

	case GDK_Page_Up:
		do_scroll = TRUE;
		if (event->state & GDK_CONTROL_MASK) {
			xofs = -(allocation.width * 3) / 4;
			yofs = 0;
		} else {
			xofs = 0;
			yofs = -(allocation.height * 3) / 4;
		}
		break;

	case GDK_Down:
		do_scroll = TRUE;
		xofs = 0;
		yofs = SCROLL_STEP_SIZE;
		break;

	case GDK_Page_Down:
		do_scroll = TRUE;
		if (event->state & GDK_CONTROL_MASK) {
			xofs = (allocation.width * 3) / 4;
			yofs = 0;
		} else {
			xofs = 0;
			yofs = (allocation.height * 3) / 4;
		}
		break;

	case GDK_Left:
		do_scroll = TRUE;
		xofs = -SCROLL_STEP_SIZE;
		yofs = 0;
		break;

	case GDK_Right:
		do_scroll = TRUE;
		xofs = SCROLL_STEP_SIZE;
		yofs = 0;
		break;

	case GDK_plus:
	case GDK_equal:
	case GDK_KP_Add:
		do_zoom = TRUE;
		zoom = priv->zoom * priv->zoom_multiplier;
		break;

	case GDK_minus:
	case GDK_KP_Subtract:
		do_zoom = TRUE;
		zoom = priv->zoom / priv->zoom_multiplier;
		break;

	case GDK_1:
		do_zoom = TRUE;
		zoom = 1.0;
		break;

	default:
		return FALSE;
	}

	if (do_zoom) {
		gint x, y;

		gdk_window_get_pointer (gtk_widget_get_window (widget),
					&x, &y, NULL);
		set_zoom (view, zoom, TRUE, x, y);
	}

	if (do_scroll)
		scroll_by (view, xofs, yofs);

	return TRUE;
}


/* Button press event handler for the image view */
static gboolean
eog_scroll_view_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (!gtk_widget_has_focus (priv->display))
		gtk_widget_grab_focus (GTK_WIDGET (priv->display));

	if (priv->dragging)
		return FALSE;

	switch (event->button) {
		case 1:
		case 2:
                        if (event->button == 1 && !priv->scroll_wheel_zoom &&
			    !(event->state & GDK_CONTROL_MASK))
				break;

			if (is_image_movable (view)) {
				eog_scroll_view_set_cursor (view, EOG_SCROLL_VIEW_CURSOR_DRAG);

				priv->dragging = TRUE;
				priv->drag_anchor_x = event->x;
				priv->drag_anchor_y = event->y;

				priv->drag_ofs_x = priv->xofs;
				priv->drag_ofs_y = priv->yofs;

				return TRUE;
			}
		default:
			break;
	}

	return FALSE;
}

static void
eog_scroll_view_style_set (GtkWidget *widget, GtkStyle *old_style)
{
	GtkStyle *style;
	EogScrollViewPrivate *priv;

	style = gtk_widget_get_style (widget);
	priv = EOG_SCROLL_VIEW (widget)->priv;

	gtk_widget_set_style (priv->display, style);
}


/* Button release event handler for the image view */
static gboolean
eog_scroll_view_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (!priv->dragging)
		return FALSE;

	switch (event->button) {
		case 1:
		case 2:
			drag_to (view, event->x, event->y);
			priv->dragging = FALSE;

			eog_scroll_view_set_cursor (view, EOG_SCROLL_VIEW_CURSOR_NORMAL);
			break;

		default:
			break;
	}

	return TRUE;
}

/* Scroll event handler for the image view.  We zoom with an event without
 * modifiers rather than scroll; we use the Shift modifier to scroll.
 * Rationale: images are not primarily vertical, and in EOG you scan scroll by
 * dragging the image with button 1 anyways.
 */
static gboolean
eog_scroll_view_scroll_event (GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;
	double zoom_factor;
	int xofs, yofs;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	/* Compute zoom factor and scrolling offsets; we'll only use either of them */
	/* same as in gtkscrolledwindow.c */
	xofs = gtk_adjustment_get_page_increment (priv->hadj) / 2;
	yofs = gtk_adjustment_get_page_increment (priv->vadj) / 2;

	switch (event->direction) {
	case GDK_SCROLL_UP:
		zoom_factor = priv->zoom_multiplier;
		xofs = 0;
		yofs = -yofs;
		break;

	case GDK_SCROLL_LEFT:
		zoom_factor = 1.0 / priv->zoom_multiplier;
		xofs = -xofs;
		yofs = 0;
		break;

	case GDK_SCROLL_DOWN:
		zoom_factor = 1.0 / priv->zoom_multiplier;
		xofs = 0;
		yofs = yofs;
		break;

	case GDK_SCROLL_RIGHT:
		zoom_factor = priv->zoom_multiplier;
		xofs = xofs;
		yofs = 0;
		break;

	default:
		g_assert_not_reached ();
		return FALSE;
	}

        if (priv->scroll_wheel_zoom) {
		if (event->state & GDK_SHIFT_MASK)
			scroll_by (view, yofs, xofs);
		else if (event->state & GDK_CONTROL_MASK)
			scroll_by (view, xofs, yofs);
		else
			set_zoom (view, priv->zoom * zoom_factor,
				  TRUE, event->x, event->y);
	} else {
		if (event->state & GDK_SHIFT_MASK)
			scroll_by (view, yofs, xofs);
		else if (event->state & GDK_CONTROL_MASK)
			set_zoom (view, priv->zoom * zoom_factor,
				  TRUE, event->x, event->y);
		else
			scroll_by (view, xofs, yofs);
        }

	return TRUE;
}

/* Motion event handler for the image view */
static gboolean
eog_scroll_view_motion_event (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;
	gint x, y;
	GdkModifierType mods;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (!priv->dragging)
		return FALSE;

	if (event->is_hint)
		gdk_window_get_pointer (gtk_widget_get_window (GTK_WIDGET (priv->display)), &x, &y, &mods);
	else {
		x = event->x;
		y = event->y;
	}

	drag_to (view, x, y);
	return TRUE;
}

static void
display_map_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	eog_debug (DEBUG_WINDOW);

	set_zoom_fit (view);
	check_scrollbar_visibility (view, NULL);
	gtk_widget_queue_draw (GTK_WIDGET (priv->display));
}

static void
eog_scroll_view_size_allocate (GtkWidget *widget, GtkAllocation *alloc)
{
	EogScrollView *view;

	view = EOG_SCROLL_VIEW (widget);
	check_scrollbar_visibility (view, alloc);

	GTK_WIDGET_CLASS (eog_scroll_view_parent_class)->size_allocate (widget
									,alloc);
}

static void
display_size_change (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (priv->zoom_mode == ZOOM_MODE_FIT) {
		GtkAllocation alloc;

		alloc.width = event->width;
		alloc.height = event->height;

		set_zoom_fit (view);
		check_scrollbar_visibility (view, &alloc);
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	} else {
		int scaled_width, scaled_height;
		int x_offset = 0;
		int y_offset = 0;

		compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

		if (priv->xofs + event->width > scaled_width)
			x_offset = scaled_width - event->width - priv->xofs;

		if (priv->yofs + event->height > scaled_height)
			y_offset = scaled_height - event->height - priv->yofs;

		scroll_by (view, x_offset, y_offset);
	}

	update_scrollbar_values (view);
}


static gboolean
eog_scroll_view_focus_in_event (GtkWidget     *widget,
			    GdkEventFocus *event,
			    gpointer data)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "focus_in_event");
	return FALSE;
}

static gboolean
eog_scroll_view_focus_out_event (GtkWidget     *widget,
			     GdkEventFocus *event,
			     gpointer data)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "focus_out_event");
	return FALSE;
}

/* Expose event handler for the drawing area.  First we process the whole dirty
 * region by drawing a non-interpolated version, which is "instantaneous", and
 * we do this synchronously.  Then, if we are set to use interpolation, we queue
 * an idle handler to handle interpolated drawing there.
 */
static gboolean
display_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	EogScrollView *view;
	GdkRectangle *rects;
	gint n_rects;
	int i;

	g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);
	g_return_val_if_fail (EOG_IS_SCROLL_VIEW (data), FALSE);

	view = EOG_SCROLL_VIEW (data);

	gdk_region_get_rectangles (event->region, &rects, &n_rects);

	for (i = 0; i < n_rects; i++) {
		request_paint_area (view, rects + i);
	}

	g_free (rects);

	return TRUE;
}


/*==================================

   image loading callbacks

   -----------------------------------*/
/*
static void
image_loading_update_cb (EogImage *img, int x, int y, int width, int height, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;
	GdkRectangle area;
	int xofs, yofs;
	int sx0, sy0, sx1, sy1;

	view = (EogScrollView*) data;
	priv = view->priv;

	eog_debug_message (DEBUG_IMAGE_LOAD, "x: %i, y: %i, width: %i, height: %i\n",
			   x, y, width, height);

	if (priv->pixbuf == NULL) {
		priv->pixbuf = eog_image_get_pixbuf (img);
		set_zoom_fit (view);
		check_scrollbar_visibility (view, NULL);
	}
	priv->progressive_state = PROGRESSIVE_LOADING;

	get_image_offsets (view, &xofs, &yofs);

	sx0 = floor (x * priv->zoom + xofs);
	sy0 = floor (y * priv->zoom + yofs);
	sx1 = ceil ((x + width) * priv->zoom + xofs);
	sy1 = ceil ((y + height) * priv->zoom + yofs);

	area.x = sx0;
	area.y = sy0;
	area.width = sx1 - sx0;
	area.height = sy1 - sy0;

	if (GTK_WIDGET_DRAWABLE (priv->display))
		gdk_window_invalidate_rect (GTK_WIDGET (priv->display)->window, &area, FALSE);
}


static void
image_loading_finished_cb (EogImage *img, gpointer data)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	view = (EogScrollView*) data;
	priv = view->priv;

	if (priv->pixbuf == NULL) {
		priv->pixbuf = eog_image_get_pixbuf (img);
		priv->progressive_state = PROGRESSIVE_NONE;
		set_zoom_fit (view);
		check_scrollbar_visibility (view, NULL);
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));

	}
	else if (priv->interp_type != GDK_INTERP_NEAREST &&
		 !is_unity_zoom (view))
	{
		// paint antialiased image version
		priv->progressive_state = PROGRESSIVE_POLISHING;
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	}
}

static void
image_loading_failed_cb (EogImage *img, char *msg, gpointer data)
{
	EogScrollViewPrivate *priv;

	priv = EOG_SCROLL_VIEW (data)->priv;

	g_print ("loading failed: %s.\n", msg);

	if (priv->pixbuf != 0) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = 0;
	}

	if (GTK_WIDGET_DRAWABLE (priv->display)) {
		gdk_window_clear (GTK_WIDGET (priv->display)->window);
	}
}

static void
image_loading_cancelled_cb (EogImage *img, gpointer data)
{
	EogScrollViewPrivate *priv;

	priv = EOG_SCROLL_VIEW (data)->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (GTK_WIDGET_DRAWABLE (priv->display)) {
		gdk_window_clear (GTK_WIDGET (priv->display)->window);
	}
}
*/
static void
image_changed_cb (EogImage *img, gpointer data)
{
	EogScrollViewPrivate *priv;

	priv = EOG_SCROLL_VIEW (data)->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	priv->pixbuf = eog_image_get_pixbuf (img);

	set_zoom_fit (EOG_SCROLL_VIEW (data));
	check_scrollbar_visibility (EOG_SCROLL_VIEW (data), NULL);

	gtk_widget_queue_draw (GTK_WIDGET (priv->display));
}

/*===================================
         public API
  ---------------------------------*/

void
eog_scroll_view_hide_cursor (EogScrollView *view)
{
       eog_scroll_view_set_cursor (view, EOG_SCROLL_VIEW_CURSOR_HIDDEN);
}

void
eog_scroll_view_show_cursor (EogScrollView *view)
{
       eog_scroll_view_set_cursor (view, EOG_SCROLL_VIEW_CURSOR_NORMAL);
}

/* general properties */
void
eog_scroll_view_set_zoom_upscale (EogScrollView *view, gboolean upscale)
{
	EogScrollViewPrivate *priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->upscale != upscale) {
		priv->upscale = upscale;

		if (priv->zoom_mode == ZOOM_MODE_FIT) {
			set_zoom_fit (view);
			gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		}
	}
}

void
eog_scroll_view_set_antialiasing_in (EogScrollView *view, gboolean state)
{
	EogScrollViewPrivate *priv;
	GdkInterpType new_interp_type;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	new_interp_type = state ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST;

	if (priv->interp_type_in != new_interp_type) {
		priv->interp_type_in = new_interp_type;
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	}
}

void
eog_scroll_view_set_antialiasing_out (EogScrollView *view, gboolean state)
{
	EogScrollViewPrivate *priv;
	GdkInterpType new_interp_type;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	new_interp_type = state ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST;

	if (priv->interp_type_out != new_interp_type) {
		priv->interp_type_out = new_interp_type;
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	}
}

void
eog_scroll_view_set_transparency (EogScrollView *view, EogTransparencyStyle style, GdkColor *color)
{
	EogScrollViewPrivate *priv;
	guint32 col = 0;
	guint32 red, green, blue;
	gboolean changed = FALSE;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (color != NULL) {
		red = (color->red >> 8) << 16;
		green = (color->green >> 8) << 8;
		blue = (color->blue >> 8);
		col = red + green + blue;
	}

	if (priv->transp_style != style) {
		priv->transp_style = style;
		changed = TRUE;
	}

	if (priv->transp_style == EOG_TRANSP_COLOR && priv->transp_color != col) {
		priv->transp_color = col;
		changed = TRUE;
	}

	if (changed && priv->pixbuf != NULL && gdk_pixbuf_get_has_alpha (priv->pixbuf)) {
		if (priv->background_surface) {
			cairo_surface_destroy (priv->background_surface);
			/* Will be recreated if needed during redraw */
			priv->background_surface = NULL;
		}
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	}
}

/* zoom api */

static double preferred_zoom_levels[] = {
	1.0 / 100, 1.0 / 50, 1.0 / 20,
	1.0 / 10.0, 1.0 / 5.0, 1.0 / 3.0, 1.0 / 2.0, 1.0 / 1.5,
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
        11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0
};
static const gint n_zoom_levels = (sizeof (preferred_zoom_levels) / sizeof (double));

void
eog_scroll_view_zoom_in (EogScrollView *view, gboolean smooth)
{
	EogScrollViewPrivate *priv;
	double zoom;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (smooth) {
		zoom = priv->zoom * priv->zoom_multiplier;
	}
	else {
		int i;
		int index = -1;

		for (i = 0; i < n_zoom_levels; i++) {
			if (preferred_zoom_levels [i] - priv->zoom
					> DOUBLE_EQUAL_MAX_DIFF) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			zoom = priv->zoom;
		}
		else {
			zoom = preferred_zoom_levels [i];
		}
	}
	set_zoom (view, zoom, FALSE, 0, 0);

}

void
eog_scroll_view_zoom_out (EogScrollView *view, gboolean smooth)
{
	EogScrollViewPrivate *priv;
	double zoom;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (smooth) {
		zoom = priv->zoom / priv->zoom_multiplier;
	}
	else {
		int i;
		int index = -1;

		for (i = n_zoom_levels - 1; i >= 0; i--) {
			if (priv->zoom - preferred_zoom_levels [i]
					> DOUBLE_EQUAL_MAX_DIFF) {
				index = i;
				break;
			}
		}
		if (index == -1) {
			zoom = priv->zoom;
		}
		else {
			zoom = preferred_zoom_levels [i];
		}
	}
	set_zoom (view, zoom, FALSE, 0, 0);
}

void
eog_scroll_view_zoom_fit (EogScrollView *view)
{
	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	set_zoom_fit (view);
	check_scrollbar_visibility (view, NULL);
	gtk_widget_queue_draw (GTK_WIDGET (view->priv->display));
}

void
eog_scroll_view_set_zoom (EogScrollView *view, double zoom)
{
	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	set_zoom (view, zoom, FALSE, 0, 0);
}

double
eog_scroll_view_get_zoom (EogScrollView *view)
{
	g_return_val_if_fail (EOG_IS_SCROLL_VIEW (view), 0.0);

	return view->priv->zoom;
}

gboolean
eog_scroll_view_get_zoom_is_min (EogScrollView *view)
{
	g_return_val_if_fail (EOG_IS_SCROLL_VIEW (view), FALSE);

	set_minimum_zoom_factor (view);

	return DOUBLE_EQUAL (view->priv->zoom, MIN_ZOOM_FACTOR) ||
	       DOUBLE_EQUAL (view->priv->zoom, view->priv->min_zoom);
}

gboolean
eog_scroll_view_get_zoom_is_max (EogScrollView *view)
{
	g_return_val_if_fail (EOG_IS_SCROLL_VIEW (view), FALSE);

	return DOUBLE_EQUAL (view->priv->zoom, MAX_ZOOM_FACTOR);
}

static void
display_next_frame_cb (EogImage *image, gint delay, gpointer data)
{
 	EogScrollViewPrivate *priv;
	EogScrollView *view;

	if (!EOG_IS_SCROLL_VIEW (data))
		return;

	view = EOG_SCROLL_VIEW (data);
	priv = view->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	priv->pixbuf = eog_image_get_pixbuf (image);
	gtk_widget_queue_draw (GTK_WIDGET (priv->display)); 
}

void
eog_scroll_view_set_image (EogScrollView *view, EogImage *image)
{
	EogScrollViewPrivate *priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->image == image) {
		return;
	}

	if (priv->image != NULL) {
		free_image_resources (view);
		if (gtk_widget_is_drawable (priv->display) && image == NULL) {
			gdk_window_clear (gtk_widget_get_window (priv->display));
		}
	}
	g_assert (priv->image == NULL);
	g_assert (priv->pixbuf == NULL);

	priv->progressive_state = PROGRESSIVE_NONE;
	if (image != NULL) {
		eog_image_data_ref (image);

		if (priv->pixbuf == NULL) {
			priv->pixbuf = eog_image_get_pixbuf (image);
			priv->progressive_state = PROGRESSIVE_NONE;
			set_zoom_fit (view);
			check_scrollbar_visibility (view, NULL);
			gtk_widget_queue_draw (GTK_WIDGET (priv->display));

		}
		else if ((is_zoomed_in (view) && priv->interp_type_in != GDK_INTERP_NEAREST) ||
			 (is_zoomed_out (view) && priv->interp_type_out != GDK_INTERP_NEAREST))
		{
			/* paint antialiased image version */
			priv->progressive_state = PROGRESSIVE_POLISHING;
			gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		}

		priv->image_changed_id = g_signal_connect (image, "changed",
							   (GCallback) image_changed_cb, view);
		if (eog_image_is_animation (image) == TRUE ) {
			eog_image_start_animation (image);
			priv->frame_changed_id = g_signal_connect (image, "next-frame", 
								    (GCallback) display_next_frame_cb, view);
		}
	}

	priv->image = image;
}

gboolean
eog_scroll_view_scrollbars_visible (EogScrollView *view)
{
	if (!gtk_widget_get_visible (GTK_WIDGET (view->priv->hbar)) &&
	    !gtk_widget_get_visible (GTK_WIDGET (view->priv->vbar)))
		return FALSE;

	return TRUE;
}

/*===================================
    object creation/freeing
  ---------------------------------*/

static void
eog_scroll_view_init (EogScrollView *view)
{
	EogScrollViewPrivate *priv;

	priv = view->priv = EOG_SCROLL_VIEW_GET_PRIVATE (view);

	priv->zoom = 1.0;
	priv->min_zoom = MIN_ZOOM_FACTOR;
	priv->zoom_mode = ZOOM_MODE_FIT;
	priv->upscale = FALSE;
	priv->uta = NULL;
	priv->interp_type_in = GDK_INTERP_BILINEAR;
	priv->interp_type_out = GDK_INTERP_BILINEAR;
	priv->scroll_wheel_zoom = FALSE;
	priv->zoom_multiplier = IMAGE_VIEW_ZOOM_MULTIPLIER;
	priv->image = NULL;
	priv->pixbuf = NULL;
	priv->progressive_state = PROGRESSIVE_NONE;
	priv->transp_style = EOG_TRANSP_BACKGROUND;
	priv->transp_color = 0;
	priv->cursor = EOG_SCROLL_VIEW_CURSOR_NORMAL;
	priv->menu = NULL;
	priv->background_color = NULL;
	priv->override_bg_color = NULL;
	priv->background_surface = NULL;
}

static void
eog_scroll_view_dispose (GObject *object)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (object));

	view = EOG_SCROLL_VIEW (object);
	priv = view->priv;

	if (priv->uta != NULL) {
		eog_uta_free (priv->uta);
		priv->uta = NULL;
	}

	if (priv->idle_id != 0) {
		g_source_remove (priv->idle_id);
		priv->idle_id = 0;
	}

	if (priv->background_color != NULL) {
		gdk_color_free (priv->background_color);
		priv->background_color = NULL;
	}

	if (priv->override_bg_color != NULL) {
		gdk_color_free (priv->override_bg_color);
		priv->override_bg_color = NULL;
	}

	if (priv->background_surface != NULL) {
		cairo_surface_destroy (priv->background_surface);
		priv->background_surface = NULL;
	}

	free_image_resources (view);

	G_OBJECT_CLASS (eog_scroll_view_parent_class)->dispose (object);
}

static void
eog_scroll_view_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (object));

	view = EOG_SCROLL_VIEW (object);
	priv = view->priv;

	switch (property_id) {
	case PROP_USE_BG_COLOR:
		g_value_set_boolean (value, priv->use_bg_color);
		break;
	case PROP_BACKGROUND_COLOR:
		//FIXME: This doesn't really handle the NULL color.
		g_value_set_boxed (value, priv->background_color);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eog_scroll_view_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (object));

	view = EOG_SCROLL_VIEW (object);
	priv = view->priv;

	switch (property_id) {
	case PROP_USE_BG_COLOR:
		eog_scroll_view_set_use_bg_color (view, g_value_get_boolean (value));
		break;
	case PROP_BACKGROUND_COLOR:
	{
		const GdkColor *color = g_value_get_boxed (value);
		eog_scroll_view_set_background_color (view, color);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
eog_scroll_view_class_init (EogScrollViewClass *klass)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;

	gobject_class->dispose = eog_scroll_view_dispose;
        gobject_class->set_property = eog_scroll_view_set_property;
        gobject_class->get_property = eog_scroll_view_get_property;

	/**
	 * EogScrollView:background-color:
	 *
	 * This is the default background color used for painting the background
	 * of the image view. If set to %NULL the color is determined by the
	 * active GTK theme.
	 */
	g_object_class_install_property (
		gobject_class, PROP_BACKGROUND_COLOR,
		g_param_spec_boxed ("background-color", NULL, NULL,
				    GDK_TYPE_COLOR,
				    G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	g_object_class_install_property (
		gobject_class, PROP_USE_BG_COLOR,
		g_param_spec_boolean ("use-background-color", NULL, NULL, FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	view_signals [SIGNAL_ZOOM_CHANGED] =
		g_signal_new ("zoom_changed",
			      EOG_TYPE_SCROLL_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EogScrollViewClass, zoom_changed),
			      NULL, NULL,
			      eog_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);

	widget_class->size_allocate = eog_scroll_view_size_allocate;
	widget_class->style_set = eog_scroll_view_style_set;

	g_type_class_add_private (klass, sizeof (EogScrollViewPrivate));
}

static void
view_on_drag_begin_cb (GtkWidget        *widget,
		       GdkDragContext   *context,
		       gpointer          user_data)
{
	EogScrollView *view;
	EogImage *image;
	GdkPixbuf *thumbnail;
	gint width, height;

	view = EOG_SCROLL_VIEW (user_data);
	image = view->priv->image;

	thumbnail = eog_image_get_thumbnail (image);

	if  (thumbnail) {
		width = gdk_pixbuf_get_width (thumbnail);
		height = gdk_pixbuf_get_height (thumbnail);
		gtk_drag_set_icon_pixbuf (context, thumbnail, width/2, height/2);
		g_object_unref (thumbnail);
	}
}

static void
view_on_drag_data_get_cb (GtkWidget        *widget,
			  GdkDragContext   *drag_context,
			  GtkSelectionData *data,
			  guint             info,
			  guint             time,
			  gpointer          user_data)
{
	EogScrollView *view;
	EogImage *image;
	gchar *uris[2];
	GFile *file;

	view = EOG_SCROLL_VIEW (user_data);

	image = view->priv->image;

	file = eog_image_get_file (image);
	uris[0] = g_file_get_uri (file);
	uris[1] = NULL;

	gtk_selection_data_set_uris (data, uris);

	g_free (uris[0]);
	g_object_unref (file);
}

GtkWidget*
eog_scroll_view_new (void)
{
	GtkWidget *widget;
	GtkTable *table;
	EogScrollView *view;
	EogScrollViewPrivate *priv;

	widget = g_object_new (EOG_TYPE_SCROLL_VIEW,
			       "can-focus", TRUE,
			       "n_rows", 2,
			       "n_columns", 2,
			       "homogeneous", FALSE,
			       NULL);

	table = GTK_TABLE (widget);
	view = EOG_SCROLL_VIEW (widget);
	priv = view->priv;

	priv->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 100, 0, 10, 10, 100));
	g_signal_connect (priv->hadj, "value_changed",
			  G_CALLBACK (adjustment_changed_cb),
			  view);
	priv->hbar = gtk_hscrollbar_new (priv->hadj);
	priv->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 100, 0, 10, 10, 100));
	g_signal_connect (priv->vadj, "value_changed",
			  G_CALLBACK (adjustment_changed_cb),
			  view);
	priv->vbar = gtk_vscrollbar_new (priv->vadj);
	priv->display = g_object_new (GTK_TYPE_DRAWING_AREA,
				      "can-focus", TRUE,
				      NULL);
	/* We don't want to be double-buffered as we are SuperSmart(tm) */
	gtk_widget_set_double_buffered (GTK_WIDGET (priv->display), FALSE);

	gtk_widget_add_events (GTK_WIDGET (priv->display),
			       GDK_EXPOSURE_MASK
			       | GDK_BUTTON_PRESS_MASK
			       | GDK_BUTTON_RELEASE_MASK
			       | GDK_POINTER_MOTION_MASK
			       | GDK_POINTER_MOTION_HINT_MASK
			       | GDK_SCROLL_MASK
			       | GDK_KEY_PRESS_MASK);
	g_signal_connect (G_OBJECT (priv->display), "configure_event", G_CALLBACK (display_size_change), view);
	g_signal_connect (G_OBJECT (priv->display), "expose_event", G_CALLBACK (display_expose_event), view);
	g_signal_connect (G_OBJECT (priv->display), "map_event", G_CALLBACK (display_map_event), view);
	g_signal_connect (G_OBJECT (priv->display), "button_press_event", G_CALLBACK (eog_scroll_view_button_press_event), view);
	g_signal_connect (G_OBJECT (priv->display), "motion_notify_event", G_CALLBACK (eog_scroll_view_motion_event), view);
	g_signal_connect (G_OBJECT (priv->display), "button_release_event", G_CALLBACK (eog_scroll_view_button_release_event), view);
	g_signal_connect (G_OBJECT (priv->display), "scroll_event", G_CALLBACK (eog_scroll_view_scroll_event), view);
	g_signal_connect (G_OBJECT (priv->display), "focus_in_event", G_CALLBACK (eog_scroll_view_focus_in_event), NULL);
	g_signal_connect (G_OBJECT (priv->display), "focus_out_event", G_CALLBACK (eog_scroll_view_focus_out_event), NULL);

	g_signal_connect (G_OBJECT (widget), "key_press_event", G_CALLBACK (display_key_press_event), view);

	gtk_drag_source_set (priv->display, GDK_BUTTON1_MASK,
			     target_table, G_N_ELEMENTS (target_table),
			     GDK_ACTION_COPY);
	g_signal_connect (G_OBJECT (priv->display), "drag-data-get",
			  G_CALLBACK (view_on_drag_data_get_cb), widget);
	g_signal_connect (G_OBJECT (priv->display), "drag-begin",
			  G_CALLBACK (view_on_drag_begin_cb), widget);

	gtk_table_attach (table, priv->display,
			  0, 1, 0, 1,
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  0,0);
	gtk_table_attach (table, priv->hbar,
			  0, 1, 1, 2,
			  GTK_FILL,
			  GTK_FILL,
			  0, 0);
	gtk_table_attach (table, priv->vbar,
			  1, 2, 0, 1,
			  GTK_FILL, GTK_FILL,
			  0, 0);

	gtk_widget_show_all (widget);

	return widget;
}

static void
eog_scroll_view_popup_menu (EogScrollView *view, GdkEventButton *event)
{
	GtkWidget *popup;
	int button, event_time;

	popup = view->priv->menu;

	if (event) {
		button = event->button;
		event_time = event->time;
	} else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

static gboolean
view_on_button_press_event_cb (GtkWidget *view, GdkEventButton *event,
			       gpointer user_data)
{
    /* Ignore double-clicks and triple-clicks */
    if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
	    eog_scroll_view_popup_menu (EOG_SCROLL_VIEW (view), event);

	    return TRUE;
    }

    return FALSE;
}

void
eog_scroll_view_set_popup (EogScrollView *view,
			   GtkMenu *menu)
{
	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));
	g_return_if_fail (view->priv->menu == NULL);

	view->priv->menu = g_object_ref (menu);

	gtk_menu_attach_to_widget (GTK_MENU (view->priv->menu),
				   GTK_WIDGET (view),
				   NULL);

	g_signal_connect (G_OBJECT (view), "button_press_event",
			  G_CALLBACK (view_on_button_press_event_cb), NULL);
}

static gboolean
_eog_gdk_color_equal0 (const GdkColor *a, const GdkColor *b)
{
	if (a == NULL || b == NULL)
		return (a == b);

	return gdk_color_equal (a, b);
}

static gboolean
_eog_replace_gdk_color (GdkColor **dest, const GdkColor *new)
{
	GdkColor *old = *dest;

	if (_eog_gdk_color_equal0 (old, new))
		return FALSE;

	if (old != NULL)
		gdk_color_free (old);

	*dest = (new) ? gdk_color_copy (new) : NULL;

	return TRUE;
}

static void
_eog_scroll_view_update_bg_color (EogScrollView *view)
{
	const GdkColor *selected;
	EogScrollViewPrivate *priv = view->priv;

	if (priv->override_bg_color)
		selected = priv->override_bg_color;
	else if (priv->use_bg_color)
		selected = priv->background_color;
	else
		selected = NULL;

	if (priv->transp_style == EOG_TRANSP_BACKGROUND
	    && priv->background_surface != NULL) {
		/* Delete the SVG background to have it recreated with
		 * the correct color during the next SVG redraw */
		cairo_surface_destroy (priv->background_surface);
		priv->background_surface = NULL;
	}

	gtk_widget_modify_bg (GTK_WIDGET (view),
			      GTK_STATE_NORMAL,
			      selected);
}

void
eog_scroll_view_set_background_color (EogScrollView *view,
				      const GdkColor *color)
{
	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	if (_eog_replace_gdk_color (&view->priv->background_color, color))
		_eog_scroll_view_update_bg_color (view);
}

void
eog_scroll_view_override_bg_color (EogScrollView *view,
				   const GdkColor *color)
{
	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	if (_eog_replace_gdk_color (&view->priv->override_bg_color, color))
		_eog_scroll_view_update_bg_color (view);
}

void
eog_scroll_view_set_use_bg_color (EogScrollView *view, gboolean use)
{
	EogScrollViewPrivate *priv;

	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (use != priv->use_bg_color) {
		priv->use_bg_color = use;

		_eog_scroll_view_update_bg_color (view);

		g_object_notify (G_OBJECT (view), "use-background-color");
	}
}

void
eog_scroll_view_set_scroll_wheel_zoom (EogScrollView *view,
				       gboolean       scroll_wheel_zoom)
{
	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

        view->priv->scroll_wheel_zoom = scroll_wheel_zoom;
}

void
eog_scroll_view_set_zoom_multiplier (EogScrollView *view,
				     gdouble        zoom_multiplier)
{
	g_return_if_fail (EOG_IS_SCROLL_VIEW (view));

        view->priv->zoom_multiplier = 1.0 + zoom_multiplier;
}
