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

#ifndef __MATECONF_EDITOR_WINDOW_H__
#define __MATECONF_EDITOR_WINDOW_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#define MATECONF_TYPE_EDITOR_WINDOW		  (mateconf_editor_window_get_type ())
#define MATECONF_EDITOR_WINDOW(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECONF_TYPE_EDITOR_WINDOW, MateConfEditorWindow))
#define MATECONF_EDITOR_WINDOW_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MATECONF_TYPE_EDITOR_WINDOW, MateConfEditorWindowClass))
#define MATECONF_IS_EDITOR_WINDOW(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECONF_TYPE_EDITOR_WINDOW))
#define MATECONF_IS_EDITOR_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((obj), MATECONF_TYPE_EDITOR_WINDOW))
#define MATECONF_EDITOR_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECONF_TYPE_EDITOR_WINDOW, MateConfEditorWindowClass))

#define RECENT_LIST_MAX_SIZE 20

enum {
        MATECONF_EDITOR_WINDOW_TYPE_NORMAL,
        MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS,
        MATECONF_EDITOR_WINDOW_TYPE_MANDATORY
};

enum {
	MATECONF_EDITOR_WINDOW_OUTPUT_WINDOW_NONE,
	MATECONF_EDITOR_WINDOW_OUTPUT_WINDOW_SEARCH,
	MATECONF_EDITOR_WINDOW_OUTPUT_WINDOW_RECENTS
};


typedef struct _MateConfEditorWindow MateConfEditorWindow;
typedef struct _MateConfEditorWindowClass MateConfEditorWindowClass;

struct _MateConfEditorWindow {
	GtkWindow parent_instance;

	int type;
	MateConfClient *client;

	GtkWidget *tree_view;
	GtkTreeModel *tree_model;
	GtkTreeModel *sorted_tree_model;
	
	GtkWidget *list_view;
	GtkTreeModel *list_model;
	GtkTreeModel *sorted_list_model;
	
	GtkWidget *output_window;
	int output_window_type;

	GtkWidget *statusbar;

	GtkUIManager *ui_manager;
	GtkWidget *popup_menu;
	GtkTreeViewColumn *value_column;

	GtkWidget *non_writable_label;
	GtkWidget *key_name_label;
	GtkWidget *short_desc_label;
	GtkTextBuffer *long_desc_buffer;
	GtkWidget *owner_label;
	GtkWidget *no_schema_label;

	guint tearoffs_notify_id;
	guint icons_notify_id;
};

struct _MateConfEditorWindowClass {
	GtkWindowClass parent_class;
};

GType mateconf_editor_window_get_type (void);
GtkWidget *mateconf_editor_window_new (void);

void mateconf_editor_window_go_to (MateConfEditorWindow *window,
				const char        *location);
void mateconf_editor_window_expand_first (MateConfEditorWindow *window);

void mateconf_editor_window_popup_error_dialog (GtkWindow   *parent,
					     const gchar *message,
					     GError      *error);


#endif /* __MATECONF_EDITOR_WINDOW_H__ */

