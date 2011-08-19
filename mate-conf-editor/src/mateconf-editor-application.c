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
#include "mateconf-editor-application.h"

#include "mateconf-util.h"
#include "mateconf-editor-window.h"
#include "mateconf-tree-model.h"
#include "mateconf-list-model.h"
#include <gtk/gtk.h>


static GSList *editor_windows = NULL;

static void
mateconf_editor_application_window_destroyed (GtkObject *window)
{
	editor_windows = g_slist_remove (editor_windows, window);

	if (editor_windows == NULL)
		gtk_main_quit ();
}

GtkWidget *
mateconf_editor_application_create_editor_window (int type)
{
	GtkWidget *window;
	MateConfEditorWindow *mateconfwindow;

	window = g_object_new (MATECONF_TYPE_EDITOR_WINDOW, "type", type, NULL);

	mateconfwindow = MATECONF_EDITOR_WINDOW (window);

	if (mateconfwindow->client == NULL) {
		mateconf_editor_application_window_destroyed (GTK_OBJECT (window));
		return NULL;
	}

	mateconf_tree_model_set_client (MATECONF_TREE_MODEL (mateconfwindow->tree_model), mateconfwindow->client);
	mateconf_list_model_set_client (MATECONF_LIST_MODEL (mateconfwindow->list_model), mateconfwindow->client);

	if (!mateconf_util_can_edit_defaults ())
		gtk_action_set_sensitive (gtk_ui_manager_get_action (mateconfwindow->ui_manager, "/MateConfEditorMenu/FileMenu/NewDefaultsWindow"),
					  FALSE);

	if (!mateconf_util_can_edit_mandatory ())
		gtk_action_set_sensitive (gtk_ui_manager_get_action (mateconfwindow->ui_manager, "/MateConfEditorMenu/FileMenu/NewMandatoryWindow"),
					  FALSE);
	
	g_signal_connect (window, "destroy",
			  G_CALLBACK (mateconf_editor_application_window_destroyed), NULL);
	
	editor_windows = g_slist_prepend (editor_windows, window);

	mateconf_editor_window_expand_first (mateconfwindow);

	return window;
}

