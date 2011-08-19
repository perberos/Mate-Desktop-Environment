/*
 * capplet-stock-icons.c
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors:
 *	Rajkumar Sivasamy <rajkumar.siva@wipro.com>
 *	Taken bits of code from panel-stock-icons.c, Thanks Mark <mark@skynet.ie>
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "capplet-stock-icons.h"

static GtkIconSize mouse_capplet_dblclck_icon_size = 0;

GtkIconSize
mouse_capplet_dblclck_icon_get_size (void)
{
	return mouse_capplet_dblclck_icon_size;
}

typedef struct
{
	char *stock_id;
	char *name;
} CappletStockIcon;


static CappletStockIcon items [] = {
	{ MOUSE_DBLCLCK_MAYBE, "double-click-maybe.png"},
	{ MOUSE_DBLCLCK_ON, "double-click-on.png"},
	{ MOUSE_DBLCLCK_OFF, "double-click-off.png"}
};

static void
capplet_register_stock_icons (GtkIconFactory *factory)
{
	gint i;
	GtkIconSource *source;

	source =  gtk_icon_source_new ();

	for (i = 0; i <  G_N_ELEMENTS (items); ++i) {
		GtkIconSet *icon_set;
		char *filename;
		filename = g_build_filename (PIXMAP_DIR, items[i].name, NULL);

		if (!filename) {
		        g_warning (_("Unable to load stock icon '%s'\n"), items[i].name);
			icon_set = gtk_icon_factory_lookup_default (GTK_STOCK_MISSING_IMAGE);
			gtk_icon_factory_add (factory, items[i].stock_id, icon_set);
			continue;
		}

		gtk_icon_source_set_filename (source, filename);
		g_free (filename);

		icon_set = gtk_icon_set_new ();
		gtk_icon_set_add_source (icon_set, source);
		gtk_icon_factory_add (factory, items[i].stock_id, icon_set);
		gtk_icon_set_unref (icon_set);
	}
	gtk_icon_source_free (source);
}

void
capplet_init_stock_icons (void)
{
	GtkIconFactory *factory;
	static gboolean initialized = FALSE;

	if (initialized)
		return;
	initialized = TRUE;

	factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (factory);
	capplet_register_stock_icons (factory);

	mouse_capplet_dblclck_icon_size = gtk_icon_size_register ("mouse-capplet-dblclck-icon",
								   MOUSE_CAPPLET_DBLCLCK_ICON_SIZE,
								   MOUSE_CAPPLET_DBLCLCK_ICON_SIZE);
	g_object_unref (factory);
}
