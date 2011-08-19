/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
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

#ifndef __MATECONF_BOOKMARKS_DIALOG_H__
#define __MATECONF_BOOKMARKS_DIALOG_H__

#include <gtk/gtk.h>

#define MATECONF_TYPE_BOOKMARKS_DIALOG		  (mateconf_bookmarks_dialog_get_type ())
#define MATECONF_BOOKMARKS_DIALOG(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECONF_TYPE_BOOKMARKS_DIALOG, MateConfBookmarksDialog))
#define MATECONF_BOOKMARKS_DIALOG_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MATECONF_TYPE_BOOKMARKS_DIALOG, MateConfBookmarksDialogClass))
#define MATECONF_IS_BOOKMARKS_DIALOG(obj)	          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECONF_TYPE_BOOKMARKS_DIALOG))
#define MATECONF_IS_BOOKMARKS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((obj), MATECONF_TYPE_BOOKMARKS_DIALOG))
#define MATECONF_BOOKMARKS_DIALOG_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECONF_TYPE_BOOKMARKS_DIALOG, MateConfBookmarksDialogClass))

typedef struct _MateConfBookmarksDialog MateConfBookmarksDialog;
typedef struct _MateConfBookmarksDialogClass MateConfBookmarksDialogClass;

struct _MateConfBookmarksDialog {
	GtkDialog parent_instance;

	GtkWidget *tree_view;
	GtkListStore *list_store;
	
	GtkWidget *delete_button;

	gboolean changing_model;
	gboolean changing_key;
	guint notify_id;
};

struct _MateConfBookmarksDialogClass {
	GtkDialogClass parent_class;
};

GType mateconf_bookmarks_dialog_get_type (void);
GtkWidget *mateconf_bookmarks_dialog_new (GtkWindow *parent);

#endif /* __MATECONF_BOOKMARKS_DIALOG_H__ */

