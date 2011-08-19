/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "themed-icon.h"

#include "mate-utils.h"

static void themed_icon_class_init (ThemedIconClass *);
static void themed_icon_init (ThemedIcon *);
static void themed_icon_finalize (GObject *);
static void themed_icon_get_property (GObject *, guint, GValue *, GParamSpec *);
static void themed_icon_set_property (GObject *, guint, const GValue *, GParamSpec *);

static void themed_icon_show (GtkWidget *);
static void themed_icon_style_set (GtkWidget *, GtkStyle *);

enum
{
	PROP_0,
	PROP_ICON_ID,
	PROP_ICON_SIZE
};

typedef struct
{
	gboolean icon_loaded;
} ThemedIconPrivate;

#define THEMED_ICON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), THEMED_ICON_TYPE, ThemedIconPrivate))

G_DEFINE_TYPE (ThemedIcon, themed_icon, GTK_TYPE_IMAGE)

static void themed_icon_class_init (ThemedIconClass * themed_icon_class)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (themed_icon_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (themed_icon_class);

	g_obj_class->get_property = themed_icon_get_property;
	g_obj_class->set_property = themed_icon_set_property;
	g_obj_class->finalize = themed_icon_finalize;

	widget_class->show = themed_icon_show;
	widget_class->style_set = themed_icon_style_set;

	g_type_class_add_private (themed_icon_class, sizeof (ThemedIconPrivate));

	g_object_class_install_property (g_obj_class, PROP_ICON_ID, g_param_spec_string ("icon-id",
			"icon-id", "the identifier of the icon", NULL,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (g_obj_class, PROP_ICON_SIZE,
		g_param_spec_enum ("icon-size", "icon-size", "the size of the icon",
			GTK_TYPE_ICON_SIZE, GTK_ICON_SIZE_BUTTON,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
themed_icon_init (ThemedIcon * icon)
{
	ThemedIconPrivate *priv = THEMED_ICON_GET_PRIVATE (icon);

	priv->icon_loaded = FALSE;
}

GtkWidget *
themed_icon_new (const gchar * id, GtkIconSize size)
{
	GtkWidget *icon = GTK_WIDGET (g_object_new (
		THEMED_ICON_TYPE, "icon-id", id, "icon-size", size, NULL));

	return icon;
}

static void
themed_icon_finalize (GObject * object)
{
	ThemedIcon *icon = THEMED_ICON (object);
	if (icon->id)
		g_free (icon->id);
	(*G_OBJECT_CLASS (themed_icon_parent_class)->finalize) (object);
}

static void
themed_icon_get_property (GObject * g_obj, guint prop_id, GValue * value, GParamSpec * param_spec)
{
	ThemedIcon *icon = THEMED_ICON (g_obj);

	switch (prop_id)
	{
	case PROP_ICON_ID:
		g_value_set_string (value, icon->id);
		break;

	case PROP_ICON_SIZE:
		g_value_set_enum (value, icon->size);
		break;

	default:
		break;
	}
}

static void
themed_icon_set_property (GObject * g_obj, guint prop_id, const GValue * value,
	GParamSpec * param_spec)
{
	ThemedIcon *icon = THEMED_ICON (g_obj);

	switch (prop_id)
	{
	case PROP_ICON_ID:
		icon->id = g_strdup (g_value_get_string (value));

/*			gtk_image_load_by_id (GTK_IMAGE (icon), icon->size, icon->id); */

		break;

	case PROP_ICON_SIZE:
		icon->size = g_value_get_enum (value);

/*			gtk_image_load_by_id (GTK_IMAGE (icon), icon->size, icon->id); */

		break;

	default:
		break;
	}
}

static void
themed_icon_show (GtkWidget * widget)
{
	ThemedIcon *icon = THEMED_ICON (widget);
	ThemedIconPrivate *priv = THEMED_ICON_GET_PRIVATE (icon);

	if (!priv->icon_loaded)
		priv->icon_loaded = load_image_by_id (GTK_IMAGE (icon), icon->size, icon->id);

	(*GTK_WIDGET_CLASS (themed_icon_parent_class)->show) (widget);
}

static void
themed_icon_style_set (GtkWidget * widget, GtkStyle * prev_style)
{
	ThemedIcon *icon = THEMED_ICON (widget);

	load_image_by_id (GTK_IMAGE (icon), icon->size, icon->id);
}
