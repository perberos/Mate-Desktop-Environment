/* Eye Of MATE -- Print Preview Widget
 *
 * Copyright (C) 2006-2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@mate.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>

#include "eog-image.h"
#include "eog-print-preview.h"

#define EOG_PRINT_PREVIEW_GET_PRIVATE(object)				\
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_PRINT_PREVIEW, EogPrintPreviewPrivate))

G_DEFINE_TYPE (EogPrintPreview, eog_print_preview, GTK_TYPE_ASPECT_FRAME)

struct _EogPrintPreviewPrivate {
	GtkWidget *area;
	GdkPixbuf *image;
	GdkPixbuf *image_scaled;

	/* The surface to set to the cairo context, created from the image */
	cairo_surface_t *surface;

        /* Flag whether we have to create surface */
	gboolean flag_create_surface;

	/* the alignment of the image in the page */
	gfloat image_x_align, image_y_align;

	/* real paper size, in inches */
	gfloat p_width, p_height;

	/* page margins, in inches */
	gfloat l_margin, r_margin, t_margin, b_margin;

	/* page margins, relatives to the widget size */
	gint l_rmargin, r_rmargin, t_rmargin, b_rmargin;

	/* image width, relative to the widget size */
	gint r_width, r_height;

	/* scale of the image, as defined by the user */
	gfloat i_scale;

	/* scale of the page, relative to the widget size */
	gfloat p_scale;

	/* whether we are currently grabbing the image */
	gboolean grabbed;

	/* the last cursor position */
	gdouble cursorx, cursory;

	/* if we reject to move the image,
	   store the delta here */
	gdouble r_dx, r_dy;
};

/* Signal IDs */
enum {
	SIGNAL_IMAGE_MOVED,
	SIGNAL_LAST
};
static gint preview_signals [SIGNAL_LAST];

enum {
	PROP_IMAGE = 1,
	PROP_IMAGE_X_ALIGN,
	PROP_IMAGE_Y_ALIGN,
	PROP_IMAGE_SCALE,
	PROP_PAPER_WIDTH,
	PROP_PAPER_HEIGHT,
	PROP_PAGE_LEFT_MARGIN,
	PROP_PAGE_RIGHT_MARGIN,
	PROP_PAGE_TOP_MARGIN,
	PROP_PAGE_BOTTOM_MARGIN
};

static void eog_print_preview_draw (EogPrintPreview *preview, cairo_t *cr);
static void eog_print_preview_finalize (GObject *object);
static void update_relative_sizes (EogPrintPreview *preview);
static void create_surface (EogPrintPreview *preview);
static void create_image_scaled (EogPrintPreview *preview);
static gboolean create_surface_when_idle (EogPrintPreview *preview);

static void
eog_print_preview_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	EogPrintPreviewPrivate *priv = EOG_PRINT_PREVIEW (object)->priv;

	switch (prop_id) {
	case PROP_IMAGE:
		g_value_set_object (value, priv->image);
		break;
	case PROP_IMAGE_X_ALIGN:
		g_value_set_float (value, priv->image_x_align);
		break;
	case PROP_IMAGE_Y_ALIGN:
		g_value_set_float (value, priv->image_y_align);
		break;
	case PROP_IMAGE_SCALE:
		g_value_set_float (value, priv->i_scale);
		break;
	case PROP_PAPER_WIDTH:
		g_value_set_float (value, priv->p_width);
		break;
	case PROP_PAPER_HEIGHT:
		g_value_set_float (value, priv->p_height);
		break;
	case PROP_PAGE_LEFT_MARGIN:
		g_value_set_float (value, priv->l_margin);
		break;
	case PROP_PAGE_RIGHT_MARGIN:
		g_value_set_float (value, priv->r_margin);
		break;
	case PROP_PAGE_TOP_MARGIN:
		g_value_set_float (value, priv->t_margin);
		break;
	case PROP_PAGE_BOTTOM_MARGIN:
		g_value_set_float (value, priv->b_margin);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
eog_print_preview_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	EogPrintPreviewPrivate *priv = EOG_PRINT_PREVIEW (object)->priv;
	gboolean paper_size_changed = FALSE;

	switch (prop_id) {
	case PROP_IMAGE:
		if (priv->image) {
			g_object_unref (priv->image);
		}
		priv->image = GDK_PIXBUF (g_value_dup_object (value));

		if (priv->image_scaled) {
			g_object_unref (priv->image_scaled);
			priv->image_scaled = NULL;
		}

		priv->flag_create_surface = TRUE;
		break;
	case PROP_IMAGE_X_ALIGN:
		priv->image_x_align = g_value_get_float (value);
		break;
	case PROP_IMAGE_Y_ALIGN:
		priv->image_y_align = g_value_get_float (value);
		break;
	case PROP_IMAGE_SCALE:
		priv->i_scale = g_value_get_float (value);
		priv->flag_create_surface = TRUE;
		break;
	case PROP_PAPER_WIDTH:
		priv->p_width = g_value_get_float (value);
		paper_size_changed = TRUE;
		break;
	case PROP_PAPER_HEIGHT:
		priv->p_height = g_value_get_float (value);
		paper_size_changed = TRUE;
		break;
	case PROP_PAGE_LEFT_MARGIN:
		priv->l_margin = g_value_get_float (value);
		break;
	case PROP_PAGE_RIGHT_MARGIN:
		priv->r_margin = g_value_get_float (value);
		break;
	case PROP_PAGE_TOP_MARGIN:
		priv->t_margin = g_value_get_float (value);
		break;
	case PROP_PAGE_BOTTOM_MARGIN:
		priv->b_margin = g_value_get_float (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}

	if (paper_size_changed) {
		g_object_set (object,
			      "ratio", priv->p_width/priv->p_height,
			      NULL);
	}

	update_relative_sizes (EOG_PRINT_PREVIEW (object));
	gtk_widget_queue_draw (priv->area);
}

static void
eog_print_preview_class_init (EogPrintPreviewClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;

	gobject_class->get_property = eog_print_preview_get_property;
	gobject_class->set_property = eog_print_preview_set_property;
	gobject_class->finalize     = eog_print_preview_finalize;

/**
 * EogPrintPreview:image:
 *
 * The "image" property defines the image that is previewed
 * in the widget.
 */
	g_object_class_install_property (gobject_class,
					 PROP_IMAGE,
					 g_param_spec_object ("image",
							      "Image to show in the preview",
							      "",
							      G_TYPE_OBJECT,
							      G_PARAM_READWRITE));

/**
 * EogPrintPreview:image-x-align:
 *
 * The "image-x-align" property defines the horizontal alignment
 * of the image in the widget.
 */
	g_object_class_install_property (gobject_class,
					 PROP_IMAGE_X_ALIGN,
					 g_param_spec_float ("image-x-align",
							      "Horizontal alignment for the image",
							      "",
							      0,
							      1,
							      0.5,
							      G_PARAM_READWRITE));

/**
 * EogPrintPreview:image-y-align:
 *
 * The "image-y-align" property defines the horizontal alignment
 * of the image in the widget.
 */
	g_object_class_install_property (gobject_class,
					 PROP_IMAGE_Y_ALIGN,
					 g_param_spec_float ("image-y-align",
							      "Vertical alignment for the image",
							      "",
							      0,
							      1,
							      0.5,
							      G_PARAM_READWRITE));

/**
 * EogPrintPreview:image-scale:
 *
 * The "image-scale" property defines the scaling of the image
 * that the user wants for the printing.
 */
	g_object_class_install_property (gobject_class,
					 PROP_IMAGE_SCALE,
					 g_param_spec_float ("image-scale",
							     "The scale for the image",
							      "",
							      0,
							      1,
							      1,
							      G_PARAM_READWRITE));

/**
 * EogPrintPreview:paper-width:
 *
 * The width of the previewed paper, in inches.
 */
	g_object_class_install_property (gobject_class,
					 PROP_PAPER_WIDTH,
					 g_param_spec_float ("paper-width",
							     "Real paper width in inches",
							     "",
							     0,
							     100,
							     8.5,
							     G_PARAM_READWRITE));

/**
 * EogPrintPreview:paper-height:
 *
 * The height of the previewed paper, in inches.
 */
	g_object_class_install_property (gobject_class,
					 PROP_PAPER_HEIGHT,
					 g_param_spec_float ("paper-height",
							     "Real paper height in inches",
							      "",
							      0,
							      200,
							      11,
							      G_PARAM_READWRITE));

/**
 * EogPrintPreview:page-left-margin:
 *
 * The size of the page's left margin, in inches.
 */
	g_object_class_install_property (gobject_class,
					 PROP_PAGE_LEFT_MARGIN,
					 g_param_spec_float ("page-left-margin",
							     "Left margin of the page in inches",
							     "",
							     0,
							     100,
							     0.25,
							     G_PARAM_READWRITE));

/**
 * EogPrintPreview:page-right-margin:
 *
 * The size of the page's right margin, in inches.
 */
	g_object_class_install_property (gobject_class,
					 PROP_PAGE_RIGHT_MARGIN,
					 g_param_spec_float ("page-right-margin",
							     "Right margin of the page in inches",
							      "",
							      0,
							      200,
							      0.25,
							      G_PARAM_READWRITE));
/**
 * EogPrintPreview:page-top-margin:
 *
 * The size of the page's top margin, in inches.
 */
	g_object_class_install_property (gobject_class,
					 PROP_PAGE_TOP_MARGIN,
					 g_param_spec_float ("page-top-margin",
							     "Top margin of the page in inches",
							     "",
							      0,
							      100,
							      0.25,
							      G_PARAM_READWRITE));

/**
 * EogPrintPreview:page-bottom-margin:
 *
 * The size of the page's bottom margin, in inches.
 */
	g_object_class_install_property (gobject_class,
					 PROP_PAGE_BOTTOM_MARGIN,
					 g_param_spec_float ("page-bottom-margin",
							     "Bottom margin of the page in inches",
							      "",
							      0,
							      200,
							      0.56,
							      G_PARAM_READWRITE));

/**
 * EogPrintPreview::image-moved:
 * @preview: the object which received the signal
 *
 * The #EogPrintPreview::image-moved signal is emitted when the position
 * of the image is changed.
 */
	preview_signals [SIGNAL_IMAGE_MOVED] =
		g_signal_new ("image_moved",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE,
			      0, NULL);

	g_type_class_add_private (klass, sizeof (EogPrintPreviewPrivate));
}

static void
eog_print_preview_finalize (GObject *object)
{
	EogPrintPreviewPrivate *priv;

	priv = EOG_PRINT_PREVIEW (object)->priv;

	if (priv->image) {
		g_object_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->image_scaled) {
		g_object_unref (priv->image_scaled);
		priv->image_scaled = NULL;
	}

	if (priv->surface) {
		cairo_surface_destroy (priv->surface);
		priv->surface = NULL;
	}

	G_OBJECT_CLASS (eog_print_preview_parent_class)->finalize (object);
}

static void
eog_print_preview_init (EogPrintPreview *preview)
{
	EogPrintPreviewPrivate *priv;
	gfloat ratio;

	priv = preview->priv = EOG_PRINT_PREVIEW_GET_PRIVATE (preview);

	priv->area = GTK_WIDGET (gtk_drawing_area_new ());

	gtk_container_add (GTK_CONTAINER (preview), priv->area);

	priv->p_width  =  8.5;
	priv->p_height = 11.0;

	ratio = priv->p_width/priv->p_height;

	gtk_aspect_frame_set (GTK_ASPECT_FRAME (preview),
			      0.5, 0.5, ratio, FALSE);

	priv->image = NULL;
	priv->image_scaled = NULL;
	priv->image_x_align = 0.5;
	priv->image_y_align = 0.5;
	priv->i_scale = 1;

	priv->surface = NULL;
	priv->flag_create_surface = TRUE;

	priv->p_scale = 0;

	priv->l_margin = 0.25;
	priv->r_margin = 0.25;
	priv->t_margin = 0.25;
	priv->b_margin = 0.56;

	priv->grabbed = FALSE;
	priv->cursorx = 0;
	priv->cursory = 0;
	priv->r_dx    = 0;
	priv->r_dy    = 0;
}

static gboolean button_press_event_cb   (GtkWidget *widget, GdkEventButton *bev, gpointer user_data);
static gboolean button_release_event_cb (GtkWidget *widget, GdkEventButton *bev, gpointer user_data);
static gboolean motion_notify_event_cb  (GtkWidget *widget, GdkEventMotion *mev, gpointer user_data);
static gboolean key_press_event_cb      (GtkWidget *widget, GdkEventKey *event, gpointer user_data);

static void expose_event_cb (GtkDrawingArea *drawing_area, GdkEventExpose *eev, gpointer  user_data);
static void size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation, gpointer user_data);

/**
 * eog_print_preview_new_with_pixbuf:
 * @pixbuf: a #GdkPixbuf
 *
 * Creates a new #EogPrintPreview widget, and sets the #GdkPixbuf to preview
 * on it.
 *
 * Returns: A new #EogPrintPreview widget.
 **/
GtkWidget *
eog_print_preview_new_with_pixbuf (GdkPixbuf *pixbuf)
{
	EogPrintPreview *preview;

	g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

	preview = EOG_PRINT_PREVIEW (eog_print_preview_new ());

	preview->priv->image = g_object_ref (pixbuf);

	update_relative_sizes (preview);

	return GTK_WIDGET (preview);
}

/**
 * eog_print_preview_new:
 *
 * Creates a new #EogPrintPreview widget, setting it to the default values,
 * and leaving the page empty. You still need to set the #EogPrintPreview:image
 * property to make it useful.
 *
 * Returns: A new and empty #EogPrintPreview widget.
 **/
GtkWidget *
eog_print_preview_new (void)
{
	EogPrintPreview *preview;
	GtkWidget *area;

	preview = g_object_new (EOG_TYPE_PRINT_PREVIEW, NULL);

	area = preview->priv->area;

	gtk_widget_set_events (area,
			       GDK_EXPOSURE_MASK            |
			       GDK_POINTER_MOTION_MASK      |
			       GDK_BUTTON_PRESS_MASK        |
			       GDK_BUTTON_RELEASE_MASK      |
			       GDK_KEY_PRESS_MASK);

	g_object_set (G_OBJECT (area),
		      "can-focus", TRUE,
		      NULL);

/* 	update_relative_sizes (preview); */

	g_signal_connect (G_OBJECT (area), "expose-event",
			  G_CALLBACK (expose_event_cb), preview);

	g_signal_connect (G_OBJECT (area), "motion-notify-event",
			  G_CALLBACK (motion_notify_event_cb), preview);

 	g_signal_connect (G_OBJECT (area), "button-press-event",
 			  G_CALLBACK (button_press_event_cb), preview);

	g_signal_connect (G_OBJECT (area), "button-release-event",
			  G_CALLBACK (button_release_event_cb), preview);

	g_signal_connect (G_OBJECT (area), "key-press-event",
			  G_CALLBACK (key_press_event_cb), preview);

	g_signal_connect (area, "size-allocate",
			  G_CALLBACK (size_allocate_cb), preview);

	return GTK_WIDGET (preview);
}

static void
expose_event_cb (GtkDrawingArea *drawing_area,
		 GdkEventExpose *eev,
		 gpointer  user_data)
{
	GtkWidget *widget;
	EogPrintPreviewPrivate *priv;
	cairo_t *cr;

	widget = GTK_WIDGET (drawing_area);
	priv = EOG_PRINT_PREVIEW (user_data)->priv;

	update_relative_sizes (EOG_PRINT_PREVIEW (user_data));

	cr = gdk_cairo_create (gtk_widget_get_window (widget));

	eog_print_preview_draw (EOG_PRINT_PREVIEW (user_data), cr);

	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
		fprintf (stderr, "Cairo is unhappy: %s\n",
			 cairo_status_to_string (cairo_status (cr)));
	}

	cairo_destroy (cr);

	gdk_window_get_pointer (gtk_widget_get_window (widget), NULL, NULL, NULL);
}

/**
 * get_current_image_coordinates:
 * @preview: an #EogPrintPreview
 * @x0: A pointer where to store the x coordinate.
 * @y0: A pointer where to store the y coordinate.
 *
 * This function returns the current image coordinates, according
 * with the properties of the given @preview widget.
 **/
static void
get_current_image_coordinates (EogPrintPreview *preview,
			       gint *x0, gint *y0)
{
	EogPrintPreviewPrivate *priv;
	GtkAllocation allocation;

	priv = preview->priv;
	gtk_widget_get_allocation (GTK_WIDGET (priv->area), &allocation);

	*x0 = (gint)((1 - priv->image_x_align)*priv->l_rmargin +  priv->image_x_align*(allocation.width - priv->r_rmargin - priv->r_width));
	*y0 = (gint)((1 - priv->image_y_align)*priv->t_rmargin +  priv->image_y_align*(allocation.height - priv->b_rmargin - priv->r_height));
}

/**
 * press_inside_image_area:
 * @preview: an #EogPrintPreview
 * @x: the points x coordinate
 * @y: the points y coordinate
 *
 * Returns whether the given point is inside the image area.
 *
 * Returns: %TRUE if the given point is inside of the image area,
 * %FALSE otherwise.
 **/
static gboolean
press_inside_image_area (EogPrintPreview *preview,
			 guint x,
			 guint y)
{
	EogPrintPreviewPrivate *priv;
	gint x0, y0;

	priv = preview->priv;
	get_current_image_coordinates (preview, &x0, &y0);

	if (x >= x0 &&  y >= y0 &&
	    x <= x0 + priv->r_width && y <= y0 + priv->r_height)
		return TRUE;

	return FALSE;
}

static void
create_image_scaled (EogPrintPreview *preview)
{
	EogPrintPreviewPrivate *priv = preview->priv;

	if (!priv->image_scaled) {
		gint i_width, i_height;
		GtkAllocation allocation;

		gtk_widget_get_allocation (priv->area, &allocation);
		i_width = gdk_pixbuf_get_width (priv->image);
		i_height = gdk_pixbuf_get_height (priv->image);

                if ((i_width > allocation.width) ||
		    (i_height > allocation.height)) {
			gdouble scale;
			scale = MIN ((gdouble) allocation.width/i_width,
				     (gdouble) allocation.height/i_height);
			priv->image_scaled = gdk_pixbuf_scale_simple (priv->image,
								      i_width*scale,
								      i_height*scale,
								      GDK_INTERP_TILES);
		} else {
			priv->image_scaled = priv->image;
			g_object_ref (priv->image_scaled);
		}
	}
}

static GdkPixbuf *
create_preview_buffer (EogPrintPreview *preview)
{
	GdkPixbuf *pixbuf;
	gint width, height;
	GdkInterpType type = GDK_INTERP_TILES;

	if (preview->priv->image == NULL) {
		return NULL;
	}

	create_image_scaled (preview);

	width  = gdk_pixbuf_get_width (preview->priv->image);
	height = gdk_pixbuf_get_height (preview->priv->image);

	width   *= preview->priv->i_scale * preview->priv->p_scale;
	height  *= preview->priv->i_scale * preview->priv->p_scale;

	if (width < 1 || height < 1)
		return NULL;

	/* to use GDK_INTERP_TILES for small pixbufs is expensive and unnecessary */
	if (width < 25 || height < 25)
		type = GDK_INTERP_NEAREST;

	if (preview->priv->image_scaled) {
		pixbuf = gdk_pixbuf_scale_simple (preview->priv->image_scaled,
						  width, height, type);
	} else {
		pixbuf = gdk_pixbuf_scale_simple (preview->priv->image,
						  width, height, type);
	}

	return pixbuf;
}

/*
  Function inspired from gdk_cairo_set_source_pixbuf (). The main reason is
  that I want to save the cairo_surface_t created from the scaled buffer to
  improve performance.
*/
static cairo_surface_t *
create_surface_from_pixbuf (GdkPixbuf *pixbuf)
{
  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);
  guchar *gdk_pixels = gdk_pixbuf_get_pixels (pixbuf);
  int gdk_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  int n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  int cairo_stride;
  guchar *cairo_pixels;
  cairo_format_t format;
  cairo_surface_t *surface;
  static const cairo_user_data_key_t key;
  int j;

  if (n_channels == 3)
    format = CAIRO_FORMAT_RGB24;
  else
    format = CAIRO_FORMAT_ARGB32;

  cairo_stride = cairo_format_stride_for_width (format, width);
  cairo_pixels = g_malloc (height * cairo_stride);
  surface = cairo_image_surface_create_for_data ((unsigned char *)cairo_pixels,
						 format,
						 width, height, cairo_stride);
  cairo_surface_set_user_data (surface, &key,
			       cairo_pixels, (cairo_destroy_func_t)g_free);

  for (j = height; j; j--)
    {
      guchar *p = gdk_pixels;
      guchar *q = cairo_pixels;

      if (n_channels == 3)
	{
	  guchar *end = p + 3 * width;

	  while (p < end)
	    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	      q[0] = p[2];
	      q[1] = p[1];
	      q[2] = p[0];
#else
	      q[1] = p[0];
	      q[2] = p[1];
	      q[3] = p[2];
#endif
	      p += 3;
	      q += 4;
	    }
	}
      else
	{
	  guchar *end = p + 4 * width;
	  guint t1,t2,t3;

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

	  while (p < end)
	    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	      MULT(q[0], p[2], p[3], t1);
	      MULT(q[1], p[1], p[3], t2);
	      MULT(q[2], p[0], p[3], t3);
	      q[3] = p[3];
#else
	      q[0] = p[3];
	      MULT(q[1], p[0], p[3], t1);
	      MULT(q[2], p[1], p[3], t2);
	      MULT(q[3], p[2], p[3], t3);
#endif

	      p += 4;
	      q += 4;
	    }

#undef MULT
	}

      gdk_pixels += gdk_rowstride;
      cairo_pixels += cairo_stride;
    }

  return surface;
}

static void
create_surface (EogPrintPreview *preview)
{
	EogPrintPreviewPrivate *priv = preview->priv;
	GdkPixbuf *pixbuf;

	if (priv->surface) {
		cairo_surface_destroy (priv->surface);
		priv->surface = NULL;
	}

	pixbuf = create_preview_buffer (preview);
	if (pixbuf) {
		priv->surface = create_surface_from_pixbuf (pixbuf);
		g_object_unref (pixbuf);
	}
	priv->flag_create_surface = FALSE;
}

static gboolean
create_surface_when_idle (EogPrintPreview *preview)
{
	create_surface (preview);

	return FALSE;
}

static gboolean
button_press_event_cb (GtkWidget *widget,
		       GdkEventButton *event,
		       gpointer user_data)
{
	EogPrintPreview *preview = EOG_PRINT_PREVIEW (user_data);

	preview->priv->cursorx = event->x;
	preview->priv->cursory = event->y;

	switch (event->button) {
	case 1:
		preview->priv->grabbed = press_inside_image_area (preview, event->x, event->y);
		break;
	}

	if (preview->priv->grabbed) {
		gtk_widget_queue_draw (GTK_WIDGET (preview));
	}

	gtk_widget_grab_focus (preview->priv->area);

	return FALSE;
}

static gboolean
button_release_event_cb (GtkWidget *widget,
			 GdkEventButton *event,
			 gpointer user_data)
{
	EogPrintPreview *preview = EOG_PRINT_PREVIEW (user_data);

	switch (event->button) {
	case 1:
		preview->priv->grabbed = FALSE;
		preview->priv->r_dx = 0;
		preview->priv->r_dy = 0;
		gtk_widget_queue_draw (GTK_WIDGET (preview));

	}
	return FALSE;
}

static gboolean
key_press_event_cb (GtkWidget   *widget,
		    GdkEventKey *event,
		    gpointer     user_data)
{
	EogPrintPreviewPrivate *priv;
	gfloat delta, align;
	gboolean stop_emission = FALSE;
	const gchar *property;

	priv = EOG_PRINT_PREVIEW (user_data)->priv;

	delta = 0;

	switch (event->keyval) {
	case GDK_Left:
		property = "image-x-align";
		delta = -0.01;
		break;
	case GDK_Right:
		property = "image-x-align";
		delta = 0.01;
		break;
	case GDK_Up:
		property = "image-y-align";
		delta = -0.01;
		break;
	case GDK_Down:
		property = "image-y-align";
		delta = 0.01;
		break;
	}

	if (delta != 0) {
		g_object_get (G_OBJECT (user_data),
			      property, &align,
			      NULL);

		align += delta;
		align = CLAMP (align, 0, 1);
		g_object_set (G_OBJECT (user_data),
			      property, align,
			      NULL);

		stop_emission = TRUE;
		g_signal_emit (G_OBJECT (user_data),
			       preview_signals
			       [SIGNAL_IMAGE_MOVED], 0);
	}

	return stop_emission;
}

static gboolean
motion_notify_event_cb (GtkWidget      *widget,
			GdkEventMotion *event,
			gpointer        user_data)
{
	EogPrintPreviewPrivate *priv = EOG_PRINT_PREVIEW (user_data)->priv;
	gdouble dx, dy;
	GtkAllocation allocation;

	if (priv->grabbed) {
		dx = event->x - priv->cursorx;
		dy = event->y - priv->cursory;

		gtk_widget_get_allocation (widget, &allocation);

		/* Make sure the image stays inside the margins */

		priv->image_x_align += (dx + priv->r_dx)/(allocation.width  - priv->r_width - priv->l_rmargin - priv->r_rmargin);
		if (priv->image_x_align < 0. || priv->image_x_align > 1.) {
			priv->image_x_align = CLAMP (priv->image_x_align, 0., 1.);
			priv->r_dx += dx;
		}
		else
			priv->r_dx = 0;

		priv->image_y_align += (dy + priv->r_dy)/(allocation.height - priv->r_height - priv->t_rmargin - priv->b_rmargin);
		if (priv->image_y_align < 0. || priv->image_y_align > 1.) {
			priv->image_y_align = CLAMP (priv->image_y_align, 0., 1.);
			priv->r_dy += dy;
		} else
			priv->r_dy = 0;

		/* we do this to correctly change the property values */
		g_object_set (EOG_PRINT_PREVIEW (user_data),
			      "image-x-align", priv->image_x_align,
			      "image-y-align", priv->image_y_align,
			      NULL);

		priv->cursorx = event->x;
		priv->cursory = event->y;

		g_signal_emit (G_OBJECT (user_data),
			       preview_signals
			       [SIGNAL_IMAGE_MOVED], 0);
	} else {
		if (press_inside_image_area (EOG_PRINT_PREVIEW (user_data), event->x, event->y)) {
		  	GdkCursor *cursor;
			cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
							     GDK_FLEUR);
			gdk_window_set_cursor (gtk_widget_get_window (widget),
					       cursor);
			gdk_cursor_unref (cursor);
		} else {
			gdk_window_set_cursor (gtk_widget_get_window (widget),
					       NULL);
		}
	}
	return FALSE;
}

static void
size_allocate_cb (GtkWidget *widget,
		  GtkAllocation *allocation,
		  gpointer user_data)
{
	EogPrintPreview *preview;

	preview = EOG_PRINT_PREVIEW (user_data);
	update_relative_sizes (preview);

	preview->priv->flag_create_surface = TRUE;

	if (preview->priv->image_scaled) {
		g_object_unref (preview->priv->image_scaled);
		preview->priv->image_scaled = NULL;
	}

	g_idle_add ((GSourceFunc) create_surface_when_idle, preview);
}

static void
eog_print_preview_draw (EogPrintPreview *preview, cairo_t *cr)
{
	EogPrintPreviewPrivate *priv;
	GtkWidget *area;
	GtkAllocation allocation;
	gint x0, y0;
	GtkStyle *style;
	gboolean has_focus;

	priv = preview->priv;
	area = priv->area;

	has_focus = gtk_widget_has_focus (area);

	style = gtk_widget_get_style (area);

	gtk_widget_get_allocation (area, &allocation);

	/* draw the page */
 	gdk_cairo_set_source_color (cr, &style->white);
 	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
 	cairo_fill (cr);

	/* draw the page margins */
	gdk_cairo_set_source_color (cr, &style->black);
	cairo_set_line_width (cr, 0.1);
	cairo_rectangle (cr,
			 priv->l_rmargin, priv->t_rmargin,
			 allocation.width - priv->l_rmargin - priv->r_rmargin,
			 allocation.height - priv->t_rmargin - priv->b_rmargin);
	cairo_stroke (cr);

	get_current_image_coordinates (preview, &x0, &y0);

	if (priv->flag_create_surface) {
		create_surface (preview);
	}

	if (priv->surface) {
		cairo_set_source_surface (cr, priv->surface, x0, y0);
		cairo_paint (cr);
	} else if (priv->image_scaled) {
		/* just in the remote case we don't have the surface */

		/* adjust (x0, y0) to the new scale */
		gdouble scale = priv->i_scale * priv->p_scale *
			gdk_pixbuf_get_width (priv->image) / gdk_pixbuf_get_width (priv->image_scaled);
		x0 /= scale;
		y0 /= scale;

		cairo_scale (cr, scale, scale);
		gdk_cairo_set_source_pixbuf (cr, priv->image_scaled, x0, y0);
		cairo_paint (cr);
	} else if (priv->image) {
		/* just in the remote case we don't have the surface */

		/* adjust (x0, y0) to the new scale */
		x0 /=  priv->i_scale * priv->p_scale;
		y0 /=  priv->i_scale * priv->p_scale;

		cairo_scale (cr, priv->i_scale*priv->p_scale, priv->i_scale*priv->p_scale);
		gdk_cairo_set_source_pixbuf (cr, priv->image, x0, y0);
		cairo_paint (cr);
	}

	if (has_focus) {
		gtk_paint_focus (style, gtk_widget_get_window (area),
				 GTK_STATE_NORMAL, NULL, NULL, NULL,
				 0, 0, allocation.width, allocation.height);
	}
}

static void
update_relative_sizes (EogPrintPreview *preview)
{
	EogPrintPreviewPrivate *priv;
	GtkAllocation allocation;
	gint i_width, i_height;

	priv = preview->priv;

	if (priv->image != NULL) {
		i_width = gdk_pixbuf_get_width (priv->image);
		i_height = gdk_pixbuf_get_height (priv->image);
	} else {
		i_width = i_height = 0;
	}

	gtk_widget_get_allocation (priv->area, &allocation);

	priv->p_scale = (gfloat) allocation.width / (priv->p_width * 72.0);

	priv->r_width  = (gint) i_width  * priv->i_scale * priv->p_scale;
	priv->r_height = (gint) i_height * priv->i_scale * priv->p_scale;

	priv->l_rmargin = (gint) (72. * priv->l_margin * priv->p_scale);
	priv->r_rmargin = (gint) (72. * priv->r_margin * priv->p_scale);
	priv->t_rmargin = (gint) (72. * priv->t_margin * priv->p_scale);
	priv->b_rmargin = (gint) (72. * priv->b_margin * priv->p_scale);
}

/**
 * eog_print_preview_set_page_margins:
 * @preview: a #EogPrintPreview
 * @l_margin: Left margin.
 * @r_margin: Right margin.
 * @t_margin: Top margin.
 * @b_margin: Bottom margin.
 *
 * Manually set the margins, in inches.
 **/
void
eog_print_preview_set_page_margins (EogPrintPreview *preview,
				    gfloat l_margin,
				    gfloat r_margin,
				    gfloat t_margin,
				    gfloat b_margin)
{
	g_return_if_fail (EOG_IS_PRINT_PREVIEW (preview));

	g_object_set (G_OBJECT(preview),
		      "page-left-margin",   l_margin,
		      "page-right-margin",  r_margin,
		      "page-top-margin",    t_margin,
		      "page-bottom-margin", r_margin,
		      NULL);
}

/**
 * eog_print_preview_set_from_page_setup:
 * @preview: a #EogPrintPreview
 * @setup: a #GtkPageSetup to set the properties from
 *
 * Sets up the page properties from a #GtkPageSetup. Useful when using the
 * widget with the GtkPrint API.
 **/
void
eog_print_preview_set_from_page_setup (EogPrintPreview *preview,
				       GtkPageSetup *setup)
{
	g_return_if_fail (EOG_IS_PRINT_PREVIEW (preview));
	g_return_if_fail (GTK_IS_PAGE_SETUP (setup));

	g_object_set (G_OBJECT (preview),
		      "page-left-margin", gtk_page_setup_get_left_margin (setup, GTK_UNIT_INCH),
		      "page-right-margin", gtk_page_setup_get_right_margin (setup, GTK_UNIT_INCH),
		      "page-top-margin", gtk_page_setup_get_top_margin (setup, GTK_UNIT_INCH),
		      "page-bottom-margin", gtk_page_setup_get_bottom_margin (setup, GTK_UNIT_INCH),
		      "paper-width", gtk_page_setup_get_paper_width (setup, GTK_UNIT_INCH),
		      "paper-height", gtk_page_setup_get_paper_height (setup, GTK_UNIT_INCH),
		      NULL);

}

/**
 * eog_print_preview_get_image_position:
 * @preview: a #EogPrintPreview
 * @x: a pointer to a #gdouble, or %NULL to ignore it
 * @y: a pointer to a #gdouble, or %NULL to ignore it
 *
 * Gets current image position in inches, relative to the margins. A
 * (0, 0) position is the intersection between the left and top margins.
 **/
void
eog_print_preview_get_image_position (EogPrintPreview *preview,
				      gdouble *x,
				      gdouble *y)
{
	EogPrintPreviewPrivate *priv;
	gdouble width, height;

	g_return_if_fail (EOG_IS_PRINT_PREVIEW (preview));

	priv = preview->priv;

	if (x != NULL) {
		width  = gdk_pixbuf_get_width (priv->image)  * priv->i_scale / 72.;
		*x = priv->image_x_align * (priv->p_width  - priv->l_margin - priv->r_margin - width);
	}
	if (y != NULL) {
		height = gdk_pixbuf_get_height (priv->image) * priv->i_scale / 72.;
		*y = priv->image_y_align * (priv->p_height - priv->t_margin - priv->b_margin - height);
	}
}

/**
 * eog_print_preview_set_image_position:
 * @preview: a #EogPrintPreview
 * @x: The X coordinate, in inches, or -1 to ignore it.
 * @y: The Y coordinate, in inches, or -1 to ignore it.
 *
 * Sets the image position. You can pass -1 to one of the coordinates if you
 * only want to set the other.
 **/
void
eog_print_preview_set_image_position (EogPrintPreview *preview,
				      gdouble x,
				      gdouble y)
{
	EogPrintPreviewPrivate *priv;
	gfloat x_align, y_align;
	gdouble width, height;

	g_return_if_fail (EOG_IS_PRINT_PREVIEW (preview));

	priv = preview->priv;

	if (x != -1) {
		width  = gdk_pixbuf_get_width (priv->image) * priv->i_scale / 72.;
		x_align = CLAMP (x/(priv->p_width - priv->l_margin - priv->r_margin - width), 0, 1);
		g_object_set (preview, "image-x-align", x_align, NULL);
	}

	if (y != -1) {
		height  = gdk_pixbuf_get_height (priv->image) * priv->i_scale / 72.;
		y_align = CLAMP (y/(priv->p_height - priv->t_margin - priv->b_margin - height), 0, 1);
		g_object_set (preview, "image-y-align", y_align, NULL);
	}
}

/**
 * eog_print_preview_set_scale:
 * @preview: a #EogPrintPreview
 * @scale: a scale value, between 0 and 1.
 *
 * Sets the scale for the image.
 **/
void
eog_print_preview_set_scale (EogPrintPreview *preview,
			     gfloat           scale)
{
	g_return_if_fail (EOG_IS_PRINT_PREVIEW (preview));

	g_object_set (preview,
		      "image-scale", scale,
		      NULL);
}
