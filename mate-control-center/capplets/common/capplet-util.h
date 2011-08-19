/* -*- mode: c; style: linux -*- */

/* capplet-util.h
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Written by Bradford Hovinen <hovinen@ximian.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __CAPPLET_UTIL_H
#define __CAPPLET_UTIL_H

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-changeset.h>

/* Macros to make certain repetitive tasks a bit easier */

/* Retrieve a widget from the UI object */

#define WID(s) GTK_WIDGET (gtk_builder_get_object (dialog, s))

/* Some miscellaneous functions useful to all capplets */

void capplet_help (GtkWindow *parent, char const *section);
void capplet_set_icon (GtkWidget *window, char const *icon_file_name);
gboolean capplet_file_delete_recursive (GFile *directory, GError **error);
void capplet_init (GOptionContext *context, int *argc, char ***argv);

#endif /* __CAPPLET_UTIL_H */
