/*
 * Music Applet
 * Copyright (C) 2006 Paul Kuliniewicz <paul@kuliniewicz.org>
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

#include "ma-fancy-tooltips.h"


#define GET_PRIVATE(o)		(G_TYPE_INSTANCE_GET_PRIVATE ((o), MA_TYPE_FANCY_TOOLTIPS, MaFancyTooltipsPrivate))


typedef struct
{
	GtkWidget *original;
	GtkWidget *content;
} MaFancyTooltipsPrivate;

static GtkTooltips *parent_class;


/*********************************************************************
 *
 * Function declarations
 *
 *********************************************************************/

static void ma_fancy_tooltips_class_init (MaFancyTooltipsClass *klass);
static void ma_fancy_tooltips_instance_init (MaFancyTooltips *tips);

static void ma_fancy_tooltips_destroy (GtkObject *object);


/*********************************************************************
 *
 * GType stuff
 *
 *********************************************************************/

GType
ma_fancy_tooltips_get_type (void)
{
	static GType type = 0;

	if (type == 0)
	{
		static const GTypeInfo info = {
			sizeof (MaFancyTooltipsClass),				/* class_size */
			NULL,							/* base_init */
			NULL,							/* base_finalize */
			(GClassInitFunc) ma_fancy_tooltips_class_init,		/* class_init */
			NULL,							/* class_finalize */
			NULL,							/* class_data */
			sizeof (MaFancyTooltips),				/* instance_size */
			0,							/* n_preallocs */
			(GInstanceInitFunc) ma_fancy_tooltips_instance_init,	/* instance_init */
			NULL
		};

		type = g_type_register_static (GTK_TYPE_TOOLTIPS, "MaFancyTooltips", &info, 0);
	}

	return type;
}

static void
ma_fancy_tooltips_class_init (MaFancyTooltipsClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;
	parent_class = g_type_class_peek_parent (klass);

	object_class->destroy = ma_fancy_tooltips_destroy;

	g_type_class_add_private (klass, sizeof (MaFancyTooltipsPrivate));
}

static void
ma_fancy_tooltips_instance_init (MaFancyTooltips *tips)
{
	MaFancyTooltipsPrivate *priv = GET_PRIVATE (tips);

	gtk_tooltips_force_window (GTK_TOOLTIPS (tips));

	priv->original = GTK_TOOLTIPS (tips)->tip_label;
	g_object_ref (priv->original);

	priv->content = NULL;
}


/*********************************************************************
 *
 * Public interface
 *
 *********************************************************************/

GtkTooltips *
ma_fancy_tooltips_new (void)
{
	return g_object_new (MA_TYPE_FANCY_TOOLTIPS, NULL);
}

GtkWidget *
ma_fancy_tooltips_get_content (MaFancyTooltips *tips)
{
	MaFancyTooltipsPrivate *priv;

	g_return_val_if_fail (tips != NULL, NULL);
	g_return_val_if_fail (MA_IS_FANCY_TOOLTIPS (tips), NULL);

	priv = GET_PRIVATE (tips);
	return priv->content;
}

void
ma_fancy_tooltips_set_content (MaFancyTooltips *tips, GtkWidget *content)
{
	MaFancyTooltipsPrivate *priv;

	g_return_if_fail (tips != NULL);
	g_return_if_fail (MA_IS_FANCY_TOOLTIPS (tips));
	g_return_if_fail (content == NULL || GTK_IS_WIDGET (content));

	priv = GET_PRIVATE (tips);
	if (priv->content == content)
		return;

	/* A dirty hack, but it seems to work OK. */

	if (priv->content != NULL)
	{
		gtk_container_remove (GTK_CONTAINER (GTK_TOOLTIPS (tips)->tip_window),
				      priv->content);
		g_object_unref (priv->content);
	}
	else
	{
		gtk_container_remove (GTK_CONTAINER (GTK_TOOLTIPS (tips)->tip_window),
				      GTK_TOOLTIPS (tips)->tip_label);
	}

	if (content != NULL)
	{
		gtk_container_add (GTK_CONTAINER (GTK_TOOLTIPS (tips)->tip_window),
				   content);
		gtk_widget_show (content);
		priv->content = content;
		g_object_ref (content);
	}
	else
	{
		gtk_container_add (GTK_CONTAINER (GTK_TOOLTIPS (tips)->tip_window),
				   GTK_TOOLTIPS (tips)->tip_label);
		priv->content = NULL;
	}
}


/*********************************************************************
 *
 * GtkObject overrides
 *
 *********************************************************************/

static void
ma_fancy_tooltips_destroy (GtkObject *object)
{
	MaFancyTooltipsPrivate *priv = GET_PRIVATE (object);

	if (priv->original != NULL)
	{
		g_object_unref (priv->original);
		priv->original = NULL;
	}

	if (priv->content != NULL)
	{
		g_object_unref (priv->content);
		priv->content = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy != NULL)
		GTK_OBJECT_CLASS (parent_class)->destroy (object);
}
