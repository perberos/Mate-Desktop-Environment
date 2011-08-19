/*
 * Music Applet
 * Copyright (C) 2007 Paul Kuliniewicz <paul@kuliniewicz.org>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ma-scroller.h"

#include <gdk/gdk.h>
#include <pango/pango.h>

#include <string.h>


#define GET_PRIVATE(obj) 	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), MA_TYPE_SCROLLER, MaScrollerPrivate))

#define VERTICAL(orient)	((orient) == MATE_PANEL_APPLET_ORIENT_LEFT || (orient) == MATE_PANEL_APPLET_ORIENT_RIGHT)

#define DISPLAY_INTERVAL	5000 	/* milliseconds */
#define SCROLL_INTERVAL		50 	/* milliseconds */

typedef enum _MaScrollerField		MaScrollerField;
typedef struct _MaScrollerPrivate	MaScrollerPrivate;
typedef enum _MaScrollerProperty	MaScrollerProperty;

enum _MaScrollerField
{
	FIELD_TITLE,
	FIELD_ARTIST,
	FIELD_ALBUM,
	NUM_FIELDS
};

struct _MaScrollerPrivate
{
	GObject *plugin;
	MatePanelAppletOrient orientation;

	/*
	 * Note that a separate PangoContext is needed since the widget's will
	 * change if the widget gets moved around and reparented.
	 */

	PangoContext *context;
	PangoLayout *layouts[NUM_FIELDS];
	MaScrollerField current;
	MaScrollerField next;
	gint offset;

	gint scaled_width;
	gint scaled_height;

	guint source;
};

enum _MaScrollerProperty
{
	PROP_NONE,
	PROP_PLUGIN,
	PROP_ORIENTATION,
};

static GtkEventBoxClass *parent_class;


/*********************************************************************
 *
 * Function declarations
 *
 *********************************************************************/

static void ma_scroller_class_init (MaScrollerClass *klass);
static void ma_scroller_init (MaScroller *scroller, MaScrollerClass *klass);

static void ma_scroller_get_property (GObject *object,
				      guint prop_id,
				      GValue *value,
				      GParamSpec *pspec);

static void ma_scroller_set_property (GObject *object,
				      guint prop_id,
				      const GValue *value,
				      GParamSpec *pspec);

static void ma_scroller_destroy (GtkObject *object);

static gboolean ma_scroller_button_press_event (GtkWidget *widget,
						GdkEventButton *event);

static gboolean ma_scroller_scroll_event (GtkWidget *widget,
					  GdkEventScroll *event);

static void ma_scroller_style_set (GtkWidget *widget,
				   GtkStyle *previous_style);

static void ma_scroller_direction_changed (GtkWidget *widget,
					   GtkTextDirection previous_direction);

static void ma_scroller_size_request (GtkWidget *widget,
				      GtkRequisition *requisition);

static void ma_scroller_size_allocate (GtkWidget *widget,
				       GtkAllocation *allocation);

static gboolean ma_scroller_expose_event (GtkWidget *widget,
					  GdkEventExpose *event);

static void ma_scroller_update_layout_text (MaScroller *scroller,
					    MaScrollerField field,
					    const gchar *text);

static PangoLayout *ma_scroller_create_layout (MaScroller *scroller, const gchar *text);
static void ma_scroller_recreate_layouts (MaScroller *scroller);

static gboolean ma_scroller_advance (gpointer scroller);
static void ma_scroller_begin_scroll (MaScroller *scroller, MaScrollerField next);
static gboolean ma_scroller_scroll (gpointer scroller);
static MaScrollerField ma_scroller_find_next (MaScroller *scroller);
static MaScrollerField ma_scroller_find_previous (MaScroller *scroller);
static void ma_scroller_jump (MaScroller *scroller, MaScrollerField next);

static void ma_scroller_draw_layout (GtkWidget *widget,
				     PangoLayout *layout,
				     gint offset,
				     GdkGC *gc);

static void ma_scroller_info_notify_cb (GObject *plugin,
					GParamSpec *pspec,
					MaScroller *scroller);


/*********************************************************************
 *
 * GType stuff
 *
 *********************************************************************/

GType
ma_scroller_get_type (void)
{
	static GType type = 0;

	if (type == 0)
	{
		static const GTypeInfo info = {
			sizeof (MaScrollerClass),			/* class_size */
			NULL,						/* base_init */
			NULL,						/* base_finalize */
			(GClassInitFunc) ma_scroller_class_init,	/* class_init */
			NULL,						/* class_finalize */
			NULL,						/* class_data */
			sizeof (MaScroller),				/* instance_size */
			0,						/* n_preallocs */
			(GInstanceInitFunc) ma_scroller_init,		/* instance_init */
			NULL
		};

		type = g_type_register_static (GTK_TYPE_EVENT_BOX, "MaScroller", &info, 0);
	}

	return type;
}

static void
ma_scroller_class_init (MaScrollerClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GtkObjectClass *gtk_object_class = (GtkObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
	parent_class = g_type_class_peek_parent (klass);

	object_class->get_property = ma_scroller_get_property;
	object_class->set_property = ma_scroller_set_property;

	gtk_object_class->destroy = ma_scroller_destroy;

	widget_class->button_press_event = ma_scroller_button_press_event;
	widget_class->scroll_event = ma_scroller_scroll_event;
	widget_class->style_set = ma_scroller_style_set;
	widget_class->direction_changed = ma_scroller_direction_changed;
	widget_class->size_request = ma_scroller_size_request;
	widget_class->size_allocate = ma_scroller_size_allocate;
	widget_class->expose_event = ma_scroller_expose_event;

	g_object_class_install_property (object_class,
					 PROP_PLUGIN,
					 g_param_spec_object ("plugin",
							      "plugin",
							      "Plugin object to get song information from",
							      G_TYPE_OBJECT,	/* actual type is in Python code */
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_ORIENTATION,
					 g_param_spec_uint ("orientation",
							    "orientation",
							    "Orientation of the applet the scroller is in",
							    0, 3,
							    MATE_PANEL_APPLET_ORIENT_DOWN,
							    G_PARAM_READWRITE));

	g_type_class_add_private (klass, sizeof (MaScrollerPrivate));
}

static void
ma_scroller_init (MaScroller *scroller, MaScrollerClass *klass)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);
	guint i;

	priv->plugin = NULL;
	priv->orientation = MATE_PANEL_APPLET_ORIENT_DOWN;

	priv->context = gtk_widget_create_pango_context (GTK_WIDGET (scroller));
	for (i = 0; i < NUM_FIELDS; i++)
		priv->layouts[i] = NULL;
	priv->current = FIELD_TITLE;
	priv->next = FIELD_TITLE;
	priv->offset = 0;

	priv->scaled_width = -1;
	priv->scaled_height = -1;

	priv->source = 0;
}


/*********************************************************************
 *
 * Object lifetime
 *
 *********************************************************************/

GtkWidget *
ma_scroller_new (void)
{
	return g_object_new (MA_TYPE_SCROLLER, NULL);
}

static void
ma_scroller_destroy (GtkObject *object)
{
	MaScrollerPrivate *priv = GET_PRIVATE (object);
	guint i;

	if (priv->source != 0)
	{
		g_source_remove (priv->source);
		priv->source = 0;
	}

	if (priv->plugin != NULL)
	{
		g_object_unref (priv->plugin);
		priv->plugin = NULL;
	}

	if (priv->context != NULL)
	{
		g_object_unref (priv->context);
		priv->context = NULL;
	}

	for (i = 0; i < NUM_FIELDS; i++)
	{
		if (priv->layouts[i] != NULL)
		{
			g_object_unref (priv->layouts[i]);
			priv->layouts[i] = NULL;
		}
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy != NULL)
		GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


/*********************************************************************
 *
 * Properties
 *
 *********************************************************************/

GObject *
ma_scroller_get_plugin (MaScroller *scroller)
{
	g_return_val_if_fail (scroller != NULL, NULL);
	g_return_val_if_fail (MA_IS_SCROLLER (scroller), NULL);

	return GET_PRIVATE (scroller)->plugin;
}

void
ma_scroller_set_plugin (MaScroller *scroller, GObject *plugin)
{
	MaScrollerPrivate *priv;
	guint i;

	g_return_if_fail (scroller != NULL);
	g_return_if_fail (MA_IS_SCROLLER (scroller));
	g_return_if_fail (plugin == NULL || G_IS_OBJECT (plugin));

	priv = GET_PRIVATE (scroller);

	if (priv->plugin != NULL)
	{
		g_signal_handlers_disconnect_by_func (priv->plugin, ma_scroller_info_notify_cb, scroller);
		g_object_unref (priv->plugin);
		priv->plugin = NULL;

		for (i = 0; i < NUM_FIELDS; i++)
			ma_scroller_update_layout_text (scroller, i, NULL);
	}

	if (plugin != NULL)
	{
		gchar *title, *artist, *album;

		priv->plugin = plugin;
		g_object_ref (plugin);

		g_object_get (plugin,
			      "title", &title,
			      "artist", &artist,
			      "album", &album,
			      NULL);

		ma_scroller_update_layout_text (scroller, FIELD_TITLE, title);
		ma_scroller_update_layout_text (scroller, FIELD_ARTIST, artist);
		ma_scroller_update_layout_text (scroller, FIELD_ALBUM, album);

		g_free (title);
		g_free (artist);
		g_free (album);

		g_signal_connect (plugin, "notify", G_CALLBACK (ma_scroller_info_notify_cb), scroller);
	}
}

MatePanelAppletOrient
ma_scroller_get_orientation (MaScroller *scroller)
{
	g_return_val_if_fail (scroller != NULL, MATE_PANEL_APPLET_ORIENT_DOWN);
	g_return_val_if_fail (MA_IS_SCROLLER (scroller), MATE_PANEL_APPLET_ORIENT_DOWN);

	return GET_PRIVATE (scroller)->orientation;
}

void
ma_scroller_set_orientation (MaScroller *scroller, MatePanelAppletOrient orientation)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);

	g_return_if_fail (scroller != NULL);
	g_return_if_fail (MA_IS_SCROLLER (scroller));
	g_return_if_fail (orientation <= 3);

	if (priv->orientation != orientation)
	{
		priv->orientation = orientation;
		ma_scroller_recreate_layouts (scroller);
	}
}

static void
ma_scroller_get_property (GObject *object,
			  guint prop_id,
			  GValue *value,
			  GParamSpec *pspec)
{
	MaScroller *scroller = MA_SCROLLER (object);

	switch (prop_id)
	{
	case PROP_PLUGIN:
		g_value_set_object (value, ma_scroller_get_plugin (scroller));
		break;

	case PROP_ORIENTATION:
		g_value_set_uint (value, ma_scroller_get_orientation (scroller));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
ma_scroller_set_property (GObject *object,
			  guint prop_id,
			  const GValue *value,
			  GParamSpec *pspec)
{
	MaScroller *scroller = MA_SCROLLER (object);

	switch (prop_id)
	{
	case PROP_PLUGIN:
		ma_scroller_set_plugin (scroller, g_value_get_object (value));
		break;

	case PROP_ORIENTATION:
		ma_scroller_set_orientation (scroller, g_value_get_uint (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


/*********************************************************************
 *
 * Content management
 *
 *********************************************************************/

static void
ma_scroller_info_notify_cb (GObject *plugin, GParamSpec *pspec, MaScroller *scroller)
{
	MaScrollerField field;
	gchar *text;

	if (strcmp (pspec->name, "title") == 0)
		field = FIELD_TITLE;
	else if (strcmp (pspec->name, "artist") == 0)
		field = FIELD_ARTIST;
	else if (strcmp (pspec->name, "album") == 0)
		field = FIELD_ALBUM;
	else
		return;

	g_object_get (plugin, pspec->name, &text, NULL);
	ma_scroller_update_layout_text (scroller, field, text);
	g_free (text);
}

static void
ma_scroller_update_layout_text (MaScroller *scroller, MaScrollerField field, const gchar *text)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);

	g_return_if_fail (field < NUM_FIELDS);

	if (text != NULL)
	{
		if (priv->layouts[field] == NULL)
			priv->layouts[field] = ma_scroller_create_layout (scroller, text);
		else
			pango_layout_set_text (priv->layouts[field], text, -1);
	}
	else if (priv->layouts[field] != NULL)
	{
		g_object_unref (priv->layouts[field]);
		priv->layouts[field] = NULL;
	}

	if (field == FIELD_TITLE)
		ma_scroller_jump (scroller, FIELD_TITLE);

	gtk_widget_queue_resize (GTK_WIDGET (scroller));
}

static PangoLayout *
ma_scroller_create_layout (MaScroller *scroller, const gchar *text)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);
	PangoLayout *layout;

	layout = pango_layout_new (priv->context);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_width (layout, priv->scaled_width);
	pango_layout_set_height (layout, priv->scaled_height);
	pango_layout_set_text (layout, text, -1);

	return layout;
}

static void
ma_scroller_recreate_layouts (MaScroller *scroller)
{
	/* Propogate the widget's current PangoContext to the layouts. */

	/*
	 * This is the whole reason the scroller is implemented in C and not
	 * Python: PyGTK provides no way to actually create a PangoMatrix, which
	 * is needed to draw rotated text.
	 */

	MaScrollerPrivate *priv = GET_PRIVATE (scroller);
	PangoMatrix matrix = PANGO_MATRIX_INIT;
	guint i;

	g_object_unref (priv->context);
	priv->context = gtk_widget_create_pango_context (GTK_WIDGET (scroller));

	switch (priv->orientation)
	{
	case MATE_PANEL_APPLET_ORIENT_LEFT:
		pango_matrix_rotate (&matrix, 270.0);
		pango_context_set_matrix (priv->context, &matrix);
		pango_context_set_base_gravity (priv->context, PANGO_GRAVITY_WEST);
		break;

	case MATE_PANEL_APPLET_ORIENT_RIGHT:
		pango_matrix_rotate (&matrix, 90.0);
		pango_context_set_matrix (priv->context, &matrix);
		pango_context_set_base_gravity (priv->context, PANGO_GRAVITY_EAST);
		break;

	default:
		pango_context_set_matrix (priv->context, NULL);
		pango_context_set_base_gravity (priv->context, PANGO_GRAVITY_SOUTH);
		break;
	}

	for (i = 0; i < NUM_FIELDS; i++)
	{
		if (priv->layouts[i] != NULL)
		{
			const gchar *text = pango_layout_get_text (priv->layouts[i]);
			PangoLayout *layout = ma_scroller_create_layout (scroller, text);
			g_object_unref (priv->layouts[i]);
			priv->layouts[i] = layout;
		}
	}

	gtk_widget_queue_resize (GTK_WIDGET (scroller));
}

static gboolean
ma_scroller_advance (gpointer scroller)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);
	MaScrollerField next;

	next = ma_scroller_find_next (MA_SCROLLER (scroller));
	if (next != priv->current)
		ma_scroller_begin_scroll (MA_SCROLLER (scroller), next);
	return FALSE;
}

static void
ma_scroller_begin_scroll (MaScroller *scroller, MaScrollerField next)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);

	if (priv->source != 0)
		g_source_remove (priv->source);
	priv->next = next;
	priv->offset = 0;
	ma_scroller_scroll (scroller);
	priv->source = g_timeout_add (SCROLL_INTERVAL, ma_scroller_scroll, scroller);
}

static gboolean
ma_scroller_scroll (gpointer scroller)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);
	gint max_offset;

	if (VERTICAL (priv->orientation))
		max_offset = GTK_WIDGET (scroller)->allocation.width;
	else
		max_offset = GTK_WIDGET (scroller)->allocation.height;

	priv->offset++;
	if (priv->offset >= max_offset)
	{
		priv->current = priv->next;
		priv->offset = 0;
		priv->source = g_timeout_add (DISPLAY_INTERVAL, ma_scroller_advance, scroller);
		gtk_widget_queue_draw (GTK_WIDGET (scroller));
		return FALSE;
	}
	else
	{
		gtk_widget_queue_draw (GTK_WIDGET (scroller));
		return TRUE;
	}
}

static gboolean
ma_scroller_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	MaScrollerPrivate *priv = GET_PRIVATE (widget);

	/* Left clicks immediately advance to the next item. */

	if (event->button == 1)
	{
		MaScrollerField next = ma_scroller_find_next (MA_SCROLLER (widget));

		if (next != priv->current)
		{
			ma_scroller_jump (MA_SCROLLER (widget), next);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
ma_scroller_scroll_event (GtkWidget *widget, GdkEventScroll *event)
{
	MaScrollerPrivate *priv = GET_PRIVATE (widget);
	MaScrollerField next;

	/* Mouse scrolls go to the next or previous item */

	if (event->direction == GDK_SCROLL_UP ||
	    (priv->orientation == MATE_PANEL_APPLET_ORIENT_LEFT && event->direction == GDK_SCROLL_RIGHT) ||
	    (priv->orientation == MATE_PANEL_APPLET_ORIENT_RIGHT && event->direction == GDK_SCROLL_LEFT))
	{
		next = ma_scroller_find_previous (MA_SCROLLER (widget));
	}
	else if (event->direction == GDK_SCROLL_DOWN ||
		 (priv->orientation == MATE_PANEL_APPLET_ORIENT_LEFT && event->direction == GDK_SCROLL_LEFT) ||
		 (priv->orientation == MATE_PANEL_APPLET_ORIENT_RIGHT && event->direction == GDK_SCROLL_RIGHT))
	{
		next = ma_scroller_find_next (MA_SCROLLER (widget));
	}
	else
		return FALSE;

	if (next != priv->current)
	{
		ma_scroller_jump (MA_SCROLLER (widget), next);
		return TRUE;
	}

	return FALSE;
}

static MaScrollerField
ma_scroller_find_next (MaScroller *scroller)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);
	MaScrollerField next;
	
	next = (priv->current + 1) % NUM_FIELDS;
	while (next != priv->current && priv->layouts[next] == NULL)
		next = (next + 1) % NUM_FIELDS;
	return next;
}

static MaScrollerField
ma_scroller_find_previous (MaScroller *scroller)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);
	MaScrollerField previous;

	previous = (priv->current + NUM_FIELDS - 1) % NUM_FIELDS;
	while (previous != priv->current && priv->layouts[previous] == NULL)
		previous = (previous + NUM_FIELDS - 1) % NUM_FIELDS;
	return previous;
}

static void
ma_scroller_jump (MaScroller *scroller, MaScrollerField next)
{
	MaScrollerPrivate *priv = GET_PRIVATE (scroller);

	if (priv->source != 0)
		g_source_remove (priv->source);
	priv->current = next;
	priv->next = next;
	priv->offset = 0;
	priv->source = g_timeout_add (DISPLAY_INTERVAL, ma_scroller_advance, scroller);
	gtk_widget_queue_draw (GTK_WIDGET (scroller));
}


/*********************************************************************
 *
 * Layout and rendering
 *
 *********************************************************************/

static void
ma_scroller_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	ma_scroller_recreate_layouts (MA_SCROLLER (widget));
}

static void
ma_scroller_direction_changed (GtkWidget *widget, GtkTextDirection previous_direction)
{
	ma_scroller_recreate_layouts (MA_SCROLLER (widget));
}

static void
ma_scroller_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	MaScrollerPrivate *priv = GET_PRIVATE (widget);
	guint i;

	requisition->width = 0;
	requisition->height = 0;

	for (i = 0; i < NUM_FIELDS; i++)
	{
		if (priv->layouts[i] != NULL)
		{
			gint width, height;
			pango_layout_get_pixel_size (priv->layouts[i], &width, &height);
			if (requisition->width < width)
				requisition->width = width;
			if (requisition->height < height)
				requisition->height = height;
		}
	}

	/* pango_layout_get_pixel_size doesn't consider the context's matrix */
	if (priv->orientation == MATE_PANEL_APPLET_ORIENT_LEFT || priv->orientation == MATE_PANEL_APPLET_ORIENT_RIGHT)
	{
		gint temp = requisition->width;
		requisition->width = requisition->height;
		requisition->height = temp;
	}
}

static void
ma_scroller_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	MaScrollerPrivate *priv = GET_PRIVATE (widget);
	guint i;

	if (GTK_WIDGET_CLASS (parent_class)->size_allocate != NULL)
		GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

	/*
	 * pango_layout_set_(width|height) doesn't consider the context's
	 * matrix, so swap width and height if vertical.
	 */

	if (VERTICAL (priv->orientation))
	{
		priv->scaled_width = allocation->height * PANGO_SCALE;
		priv->scaled_height = allocation->width * PANGO_SCALE;
	}
	else
	{
		priv->scaled_width = allocation->width * PANGO_SCALE;
		priv->scaled_height = allocation->height * PANGO_SCALE;
	}

	for (i = 0; i < NUM_FIELDS; ++i)
	{
		if (priv->layouts[i] != NULL)
		{
			pango_layout_set_width (priv->layouts[i], priv->scaled_width);
			pango_layout_set_height (priv->layouts[i], priv->scaled_height);
		}
	}
}

static gboolean
ma_scroller_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	MaScrollerPrivate *priv = GET_PRIVATE (widget);
	GdkGC *gc = gdk_gc_new (GDK_DRAWABLE (widget->window));

	gdk_gc_set_foreground (gc, &widget->style->fg[GTK_STATE_NORMAL]);

	if (priv->current != priv->next)
	{
		gint displacement = VERTICAL (priv->orientation) ? widget->allocation.width : widget->allocation.height;
		ma_scroller_draw_layout (widget, priv->layouts[priv->current], -priv->offset, gc);
		ma_scroller_draw_layout (widget, priv->layouts[priv->next], displacement - priv->offset, gc);
	}
	else
		ma_scroller_draw_layout (widget, priv->layouts[priv->current], 0, gc);

	g_object_unref (gc);
	return TRUE;
}

static void
ma_scroller_draw_layout (GtkWidget *widget, PangoLayout *layout, gint offset, GdkGC *gc)
{
	MaScrollerPrivate *priv = GET_PRIVATE (widget);
	gint width, height;
	gint x, y;

	if (layout == NULL)
		return;

	/* again, width and height here are prior to rotation */
	pango_layout_get_pixel_size (layout, &width, &height);

	if (VERTICAL (priv->orientation))
	{
		x = (widget->allocation.width - height) / 2;
		y = (widget->allocation.height - width) / 2;
	}
	else
	{
		x = (widget->allocation.width - width) / 2;
		y = (widget->allocation.height - height) / 2;
	}

	switch (priv->orientation)
	{
	case MATE_PANEL_APPLET_ORIENT_LEFT:
		x -= offset;
		break;

	case MATE_PANEL_APPLET_ORIENT_RIGHT:
		x += offset;
		break;

	default:
		y += offset;
		break;
	}

	gdk_draw_layout (GDK_DRAWABLE (widget->window), gc, x, y, layout);
}
