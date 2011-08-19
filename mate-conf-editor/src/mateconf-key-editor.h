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

#ifndef __MATECONF_KEY_EDITOR_H__
#define __MATECONF_KEY_EDITOR_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf.h>

#define MATECONF_TYPE_KEY_EDITOR		  (mateconf_key_editor_get_type ())
#define MATECONF_KEY_EDITOR(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECONF_TYPE_KEY_EDITOR, MateConfKeyEditor))
#define MATECONF_KEY_EDITOR_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MATECONF_TYPE_KEY_EDITOR, MateConfKeyEditorClass))
#define MATECONF_IS_KEY_EDITOR(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECONF_TYPE_KEY_EDITOR))
#define MATECONF_IS_KEY_EDITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((obj), MATECONF_TYPE_KEY_EDITOR))
#define MATECONF_KEY_EDITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECONF_TYPE_KEY_EDITOR, MateConfKeyEditorClass))

typedef enum {
	MATECONF_KEY_EDITOR_NEW_KEY,
	MATECONF_KEY_EDITOR_EDIT_KEY
} MateConfKeyEditorAction;

typedef struct _MateConfKeyEditor MateConfKeyEditor;
typedef struct _MateConfKeyEditorClass MateConfKeyEditorClass;

struct _MateConfKeyEditor {
	GtkDialog parent_instance;
	
	int active_type;
	GtkWidget *combobox;
	GtkWidget *bool_box;
	GtkWidget *bool_widget;
	GtkWidget *int_box;	
	GtkWidget *int_widget;
	GtkWidget *string_box;
	GtkWidget *string_widget;
	GtkWidget *float_box;
	GtkWidget *float_widget;

	GtkWidget *non_writable_label;
	
	GtkWidget *path_label;
	GtkWidget *path_box;
	GtkWidget *name_entry;

	/* List editing stuff */
	GtkWidget *list_box;
	GtkWidget *list_widget;
	GtkListStore *list_model;
	GtkWidget *list_type_menu;
	GtkWidget *edit_button, *remove_button, *go_up_button, *go_down_button;

};

struct _MateConfKeyEditorClass {
	GtkDialogClass parent_class;
};

GType       mateconf_key_editor_get_type (void);
GtkWidget  *mateconf_key_editor_new      (MateConfKeyEditorAction action);

void        mateconf_key_editor_set_value (MateConfKeyEditor *editor,
					MateConfValue     *value);
MateConfValue *mateconf_key_editor_get_value (MateConfKeyEditor *editor);

void  mateconf_key_editor_set_key_path      (MateConfKeyEditor *editor,
					  const char     *path);
char *mateconf_key_editor_get_full_key_path (MateConfKeyEditor *editor);

void                 mateconf_key_editor_set_key_name (MateConfKeyEditor *editor,
						    const char     *path);
G_CONST_RETURN char *mateconf_key_editor_get_key_name (MateConfKeyEditor *editor);

void mateconf_key_editor_set_writable	(MateConfKeyEditor *editor,
					 gboolean        writable);

#endif /* __MATECONF_KEY_EDITOR_H__ */
