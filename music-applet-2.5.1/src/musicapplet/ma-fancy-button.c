/*
 * Music Applet
 * Copyright (C) 2006 Paul Kuliniewicz <paul.kuliniewicz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02111-1301, USA.
 */


#include <config.h>

#include "ma-fancy-button.h"

#include <gtk/gtkicontheme.h>
#include <gtk/gtkstock.h>


#define GET_PRIVATE(o) 		(G_TYPE_INSTANCE_GET_PRIVATE ((o), MA_TYPE_FANCY_BUTTON, MaFancyButtonPrivate))

#define HIGHLIGHT_SHIFT		30
#define DISPLACEMENT		2

#define MIN_SIZE		24
#define PREFERRED_SIZE		24
#define MAX_SIZE		48


typedef struct
{
	gchar *stock_id;
	gchar *icon_name;

	GdkPixbuf *pixbuf;
	GdkPixbuf *pixbuf_hi;

	gint size;
	MatePanelAppletOrient orientation;

	GtkIconTheme *theme;
} MaFancyButtonPrivate;

typedef enum
{
	PROP_NONE,
	PROP_STOCK_ID,
	PROP_ICON_NAME,
	PROP_ORIENTATION,
} MaFancyButtonProperty;

static GtkButtonClass *parent_class;


/*********************************************************************
 *
 * Function declarations
 *
 *********************************************************************/

static void ma_fancy_button_class_init (MaFancyButtonClass *klass);
static void ma_fancy_button_init (MaFancyButton *fb);

static void ma_fancy_button_get_property (GObject *object,
					  guint prop_id,
					  GValue *value,
					  GParamSpec *pspec);

static void ma_fancy_button_set_property (GObject *object,
					  guint prop_id,
					  const GValue *value,
					  GParamSpec *pspec);

static void ma_fancy_button_destroy (GtkObject *object);

static void ma_fancy_button_size_request (GtkWidget *widget,
					  GtkRequisition *requisition);

static void ma_fancy_button_size_allocate (GtkWidget *widget,
					   GtkAllocation *allocation);

static gboolean ma_fancy_button_button_press_event (GtkWidget *widget,
						    GdkEventButton *event);

static gboolean ma_fancy_button_expose_event (GtkWidget *widget,
					      GdkEventExpose *event);

static void recompute_size (MaFancyButton *fb);

static void reload_pixbufs (MaFancyButton *fb);

static void shift_colors (GdkPixbuf *pixbuf, gint shift);

static void theme_changed_cb (GtkIconTheme *theme, MaFancyButton *fb);


/*********************************************************************
 *
 * GType stuff
 *
 *********************************************************************/

GType
ma_fancy_button_get_type (void)
{
	static GType type = 0;

	if (type == 0)
	{
		static const GTypeInfo info = {
			sizeof (MaFancyButtonClass),			/* class_size */
			NULL,						/* base_init */
			NULL,						/* base_finalize */
			(GClassInitFunc) ma_fancy_button_class_init,	/* class_init */
			NULL,						/* class_finalize */
			NULL,						/* class_data */
			sizeof (MaFancyButton),				/* instance_size */
			0,						/* n_preallocs */
			(GInstanceInitFunc) ma_fancy_button_init,	/* instance_init */
			NULL
		};

		type = g_type_register_static (GTK_TYPE_BUTTON, "MaFancyButton", &info, 0);
	}

	return type;
}

static void
ma_fancy_button_class_init (MaFancyButtonClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GtkObjectClass *gtk_object_class = (GtkObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
	parent_class = g_type_class_peek_parent (klass);

	object_class->get_property = ma_fancy_button_get_property;
	object_class->set_property = ma_fancy_button_set_property;

	gtk_object_class->destroy = ma_fancy_button_destroy;

	widget_class->size_request = ma_fancy_button_size_request;
	widget_class->size_allocate = ma_fancy_button_size_allocate;
	widget_class->button_press_event = ma_fancy_button_button_press_event;
	widget_class->expose_event = ma_fancy_button_expose_event;

	g_object_class_install_property (object_class,
					 PROP_STOCK_ID,
					 g_param_spec_string ("stock-id",
						 	      "stock-id",
							      "Stock ID of the image to display",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_ICON_NAME,
					 g_param_spec_string ("icon-name",
						 	      "icon-name",
							      "Name of the icon to display",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_ORIENTATION,
					 g_param_spec_uint ("orientation",
						 	    "orientation",
							    "Orientation of the applet the button is in",
							    0, 3,
							    MATE_PANEL_APPLET_ORIENT_DOWN,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_type_class_add_private (klass, sizeof (MaFancyButtonPrivate));
}

static void
ma_fancy_button_init (MaFancyButton *fb)
{
	MaFancyButtonPrivate *priv = GET_PRIVATE (fb);

	priv->stock_id = NULL;
	priv->icon_name = NULL;

	priv->pixbuf = NULL;
	priv->pixbuf_hi = NULL;

	priv->size = PREFERRED_SIZE;
	priv->orientation = MATE_PANEL_APPLET_ORIENT_DOWN;

	priv->theme = gtk_icon_theme_get_default ();
	g_signal_connect (priv->theme, "changed", G_CALLBACK (theme_changed_cb), fb);
}


/*********************************************************************
 *
 * Public interface
 *
 *********************************************************************/

GtkWidget *
ma_fancy_button_new (void)
{
	return g_object_new (MA_TYPE_FANCY_BUTTON,
			     "stock-id", GTK_STOCK_MISSING_IMAGE,
			     NULL);
}

const gchar *
ma_fancy_button_get_stock_id (MaFancyButton *fb)
{
	g_return_val_if_fail (fb != NULL, NULL);
	g_return_val_if_fail (MA_IS_FANCY_BUTTON (fb), NULL);

	return GET_PRIVATE (fb)->stock_id;
}

void
ma_fancy_button_set_stock_id (MaFancyButton *fb, const gchar *stock_id)
{
	MaFancyButtonPrivate *priv;

	g_return_if_fail (fb != NULL);
	g_return_if_fail (MA_IS_FANCY_BUTTON (fb));

	priv = GET_PRIVATE (fb);

	if (stock_id == NULL && priv->stock_id == NULL)
		return;

	g_free (priv->stock_id);
	priv->stock_id = g_strdup (stock_id);

	g_free (priv->icon_name);
	priv->icon_name = NULL;

	reload_pixbufs (fb);
}

const gchar *
ma_fancy_button_get_icon_name (MaFancyButton *fb)
{
	g_return_val_if_fail (fb != NULL, NULL);
	g_return_val_if_fail (MA_IS_FANCY_BUTTON (fb), NULL);

	return GET_PRIVATE (fb)->icon_name;
}

void
ma_fancy_button_set_icon_name (MaFancyButton *fb, const gchar *icon_name)
{
	MaFancyButtonPrivate *priv;

	g_return_if_fail (fb != NULL);
	g_return_if_fail (MA_IS_FANCY_BUTTON (fb));

	priv = GET_PRIVATE (fb);

	if (icon_name == NULL && priv->icon_name == NULL)
		return;

	g_free (priv->stock_id);
	priv->stock_id = NULL;

	g_free (priv->icon_name);
	priv->icon_name = g_strdup (icon_name);

	reload_pixbufs (fb);
}

MatePanelAppletOrient
ma_fancy_button_get_orientation (MaFancyButton *fb)
{
	g_return_val_if_fail (fb != NULL, MATE_PANEL_APPLET_ORIENT_DOWN);
	g_return_val_if_fail (MA_IS_FANCY_BUTTON (fb), MATE_PANEL_APPLET_ORIENT_DOWN);

	return GET_PRIVATE (fb)->orientation;
}

void
ma_fancy_button_set_orientation (MaFancyButton *fb, MatePanelAppletOrient orientation)
{
	g_return_if_fail (fb != NULL);
	g_return_if_fail (MA_IS_FANCY_BUTTON (fb));
	g_return_if_fail (orientation <= 3);

	GET_PRIVATE (fb)->orientation = orientation;
	recompute_size (fb);
}


/*********************************************************************
 *
 * GObject overrides
 *
 *********************************************************************/

static void
ma_fancy_button_get_property (GObject *object,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *pspec)
{
	MaFancyButton *fb = MA_FANCY_BUTTON (object);

	switch (prop_id)
	{
	case PROP_STOCK_ID:
		g_value_set_string (value, ma_fancy_button_get_stock_id (fb));
		break;

	case PROP_ICON_NAME:
		g_value_set_string (value, ma_fancy_button_get_icon_name (fb));
		break;

	case PROP_ORIENTATION:
		g_value_set_uint (value, ma_fancy_button_get_orientation (fb));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
ma_fancy_button_set_property (GObject *object,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *pspec)
{
	MaFancyButton *fb = MA_FANCY_BUTTON (object);

	switch (prop_id)
	{
	case PROP_STOCK_ID:
		ma_fancy_button_set_stock_id (fb, g_value_get_string (value));
		break;

	case PROP_ICON_NAME:
		ma_fancy_button_set_icon_name (fb, g_value_get_string (value));
		break;

	case PROP_ORIENTATION:
		ma_fancy_button_set_orientation (fb, g_value_get_uint (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


/*********************************************************************
 *
 * GtkObject overrides
 *
 *********************************************************************/

static void
ma_fancy_button_destroy (GtkObject *object)
{
	MaFancyButton *fb = MA_FANCY_BUTTON (object);
	MaFancyButtonPrivate *priv = GET_PRIVATE (fb);

	g_signal_handlers_disconnect_by_func (priv->theme, G_CALLBACK (theme_changed_cb), fb);

	g_free (priv->stock_id);
	priv->stock_id = NULL;

	g_free (priv->icon_name);
	priv->icon_name = NULL;

	if (priv->pixbuf != NULL)
	{
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (priv->pixbuf_hi != NULL)
	{
		g_object_unref (priv->pixbuf_hi);
		priv->pixbuf_hi = NULL;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


/*********************************************************************
 *
 * GtkWidget overrides
 *
 *********************************************************************/

static void
ma_fancy_button_size_request (GtkWidget *widget,
				 GtkRequisition *requisition)
{
	MaFancyButton *fb = MA_FANCY_BUTTON (widget);
	MaFancyButtonPrivate *priv = GET_PRIVATE (widget);

	if (priv->pixbuf != NULL)
	{
		requisition->width = gdk_pixbuf_get_width (priv->pixbuf);
		requisition->height = gdk_pixbuf_get_height (priv->pixbuf);
	}
}

static void
ma_fancy_button_size_allocate (GtkWidget *widget,
				  GtkAllocation *allocation)
{
	MaFancyButton *fb = MA_FANCY_BUTTON (widget);
	MaFancyButtonPrivate *priv = GET_PRIVATE (widget);

	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
	recompute_size (fb);
}

static gboolean
ma_fancy_button_button_press_event (GtkWidget *widget,
				       GdkEventButton *event)
{
	/* Make sure non-left-clicks propagate up to the applet. */

	if (event->button != 1)
		return FALSE;
	else
		return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
}

static gboolean
ma_fancy_button_expose_event (GtkWidget *widget,
				 GdkEventExpose *event)
{
	GtkButton *button = GTK_BUTTON (widget);
	MaFancyButton *fb = MA_FANCY_BUTTON (widget);
	MaFancyButtonPrivate *priv = GET_PRIVATE (fb);

	GdkPixbuf *pixbuf;
	gint offset;

	gint x;
	gint y;
	gint width;
	gint height;

	GdkRectangle target_area;
	GdkRectangle draw_area;

	/* Highlight the button image on mouseover */

	if (button->in_button || GTK_WIDGET_HAS_FOCUS (widget))
		pixbuf = priv->pixbuf_hi;
	else
		pixbuf = priv->pixbuf;

	if (pixbuf == NULL)
		return TRUE;

	/* Offset the image if the button is being pressed */

	if (button->in_button && button->button_down)
	{
		offset = DISPLACEMENT * widget->allocation.height / 48.0;
		if (offset < 1)
			offset = 1;
	}
	else
		offset = 0;

	/* Center the image in the button's area */

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	x = widget->allocation.x + offset + (widget->allocation.width - width) / 2;
	y = widget->allocation.y + offset + (widget->allocation.height - height) / 2;

	/* Clip using the visible area and the area that needs to be redrawn */

	target_area.x = x;
	target_area.y = y;
	target_area.width = width;
	target_area.height = height;

	if (gdk_rectangle_intersect (&event->area, &widget->allocation, &draw_area) &&
	    gdk_rectangle_intersect (&draw_area, &target_area, &draw_area))
	{
		gdk_draw_pixbuf (widget->window, NULL, pixbuf,
				 target_area.x - x, target_area.y - y,
				 target_area.x, target_area.y,
				 target_area.width, target_area.height,
				 GDK_RGB_DITHER_NORMAL,
				 0, 0);
	}

	return FALSE;
}


/*********************************************************************
 *
 * Internal functions
 *
 *********************************************************************/

static void
recompute_size (MaFancyButton *fb)
{
	GtkWidget *widget = GTK_WIDGET (fb);
	MaFancyButtonPrivate *priv = GET_PRIVATE (fb);
	gint size;

	switch (priv->orientation)
	{
	case MATE_PANEL_APPLET_ORIENT_DOWN:
	case MATE_PANEL_APPLET_ORIENT_UP:
		size = widget->allocation.height;
		break;

	case MATE_PANEL_APPLET_ORIENT_LEFT:
	case MATE_PANEL_APPLET_ORIENT_RIGHT:
		size = widget->allocation.width;
		break;

	default:
		g_return_if_reached ();
	}

	size = CLAMP (size, MIN_SIZE, MAX_SIZE);

	if (size != priv->size)
	{
		priv->size = size;
		reload_pixbufs (fb);
	}
}

static void
reload_pixbufs (MaFancyButton *fb)
{
	MaFancyButtonPrivate *priv = GET_PRIVATE (fb);

	if (priv->pixbuf != NULL)
		g_object_unref (priv->pixbuf);
	if (priv->pixbuf_hi != NULL)
		g_object_unref (priv->pixbuf_hi);

	if (priv->stock_id != NULL)
	{
		priv->pixbuf = gtk_widget_render_icon (GTK_WIDGET (fb),
						       priv->stock_id,
						       GTK_ICON_SIZE_LARGE_TOOLBAR,
						       NULL);
		if (priv->pixbuf == NULL)
			g_warning ("Unable to render stock icon '%s'", priv->stock_id);
	}
	else if (priv->icon_name != NULL)
	{
		GError *error = NULL;

		priv->pixbuf = gtk_icon_theme_load_icon (priv->theme,
							 priv->icon_name,
							 priv->size,
							 0,
							 &error);
		if (priv->pixbuf == NULL)
		{
			if (error != NULL)
			{
				g_warning ("Unable to load icon '%s': %s", priv->icon_name, error->message);
				g_error_free (error);
			}
			else
				g_warning ("Unable to load icon '%s'", priv->icon_name);
		}
	}
	else
		priv->pixbuf = NULL;

	if (priv->pixbuf == NULL)
	{
		/* Fall back to a safe default if the requested icon isn't found. */
		priv->pixbuf = gtk_widget_render_icon (GTK_WIDGET (fb),
						       "music-applet",
						       GTK_ICON_SIZE_LARGE_TOOLBAR,
						       NULL);
		if (priv->pixbuf == NULL)
			g_critical ("Unable to render fallback stock icon 'music-applet'");
	}

	if (priv->pixbuf != NULL)
	{
		priv->pixbuf_hi = gdk_pixbuf_copy (priv->pixbuf);
		shift_colors (priv->pixbuf_hi, HIGHLIGHT_SHIFT);
	}
	else
		priv->pixbuf_hi = NULL;

	gtk_widget_queue_resize (GTK_WIDGET (fb));
}

static void
shift_colors (GdkPixbuf *pixbuf, gint shift)
{
	gint width = gdk_pixbuf_get_width (pixbuf);
	gint height = gdk_pixbuf_get_height (pixbuf);
	gint rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	gboolean has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
	guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

	guchar *p;
	gint row;
	gint col;
	gint i;

	g_return_if_fail (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
	g_return_if_fail (gdk_pixbuf_get_n_channels (pixbuf) == (has_alpha ? 4 : 3));

	for (row = 0; row < height; row++)
	{
		p = pixels + row * rowstride;
		for (col = 0; col < width; col++)
		{
			for (i = 0; i < 3; i++) 		/* red, green, blue */
			{
				*p = CLAMP (*p + shift, 0, 255);
				p++;
			}

			if (has_alpha)
				p++;
		}
	}
}


/*********************************************************************
 *
 * Callbacks
 *
 *********************************************************************/

static void
theme_changed_cb (GtkIconTheme *theme, MaFancyButton *fb)
{
	reload_pixbufs (fb);
}
