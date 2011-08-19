/*
 * Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include "mateconf-stock-icons.h"

#include <gtk/gtk.h>


static const char *icon_theme_items[] =
{
	STOCK_BOOKMARK,
        STOCK_EDIT_BOOKMARK
};

void
mateconf_stock_icons_register (void)
{
	GtkIconFactory *icon_factory;
	GtkIconSet *set;
	GtkIconSource *icon_source;
	int i;
	static gboolean initialized = FALSE;

	if (initialized == TRUE) {
		return;
	}

	icon_factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (icon_factory);

	for (i = 0; i < (int) G_N_ELEMENTS (icon_theme_items); i++)
	{
		set = gtk_icon_set_new ();
		icon_source = gtk_icon_source_new ();
		gtk_icon_source_set_icon_name (icon_source, icon_theme_items[i]);
		gtk_icon_set_add_source (set, icon_source);
		gtk_icon_factory_add (icon_factory, icon_theme_items[i], set);
		gtk_icon_set_unref (set);
		gtk_icon_source_free (icon_source);
	}

	g_object_unref (G_OBJECT (icon_factory));

	initialized = TRUE;
}
