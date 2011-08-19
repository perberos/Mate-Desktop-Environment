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

#include "mateconf-editor-window.h"

#include "mateconf-bookmarks.h"
#include "mateconf-bookmarks-dialog.h"
#include "mateconf-list-model.h"
#include "mateconf-tree-model.h"
#include "mateconf-cell-renderer.h"
#include "mateconf-editor-application.h"
#include "mateconf-key-editor.h"
#include "mateconf-policykit.h"
#include "mateconf-stock-icons.h"
#include "mateconf-util.h"
#include "gedit-output-window.h"
#include "mateconf-search-dialog.h"

#include <mateconf/mateconf.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <string.h>
#include <stdlib.h>

/* Property IDs */
enum
{
  PROP_0,

  PROP_TYPE
};

static void		mateconf_editor_window_set_property	(GObject          *object,
								 guint             param_id,
								 const GValue     *value,
								 GParamSpec       *pspec);
static void		mateconf_editor_window_get_property	(GObject          *object,
								 guint             param_id,
								 GValue           *value,
								 GParamSpec       *pspec);

static MateConfClient* 	mateconf_editor_window_get_client 		(int type);

G_DEFINE_TYPE(MateConfEditorWindow, mateconf_editor_window, GTK_TYPE_WINDOW)

void
mateconf_editor_window_popup_error_dialog (GtkWindow   *parent,
					const gchar *message,
					GError      *error)
{
	GtkWidget *dialog;

	g_return_if_fail (error != NULL);

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 message,
					 error->message);
	g_error_free (error);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
mateconf_editor_window_close_window (GtkAction *action G_GNUC_UNUSED,
				  GtkWidget *window)
{
	gtk_object_destroy (GTK_OBJECT (window));
}

void
mateconf_editor_window_go_to (MateConfEditorWindow *window,
			   const char        *location)
{
	GtkTreePath *path, *child_path;
	gint depth;
	gint i, nchildren;
	char *key;
	
	g_return_if_fail (location != NULL);
	
	child_path = mateconf_tree_model_get_tree_path_from_mateconf_path (
				MATECONF_TREE_MODEL (window->tree_model), location);
	if (!child_path)
		return;

	path = gtk_tree_model_sort_convert_child_path_to_path (
				GTK_TREE_MODEL_SORT (window->sorted_tree_model),
				child_path);

	gtk_tree_path_free (child_path);
	
	/* kind of hackish, but it works! */
	depth = gtk_tree_path_get_depth (path);
	for (i = 0; i < depth; i++) {
		gint j;
		GtkTreePath *cpath = gtk_tree_path_copy (path);

		for (j = 0; j < (depth - i); j++)
			gtk_tree_path_up (cpath);

		gtk_tree_view_expand_row (GTK_TREE_VIEW (window->tree_view), cpath, FALSE);
		gtk_tree_path_free (cpath);
	}

	gtk_tree_view_expand_row (GTK_TREE_VIEW (window->tree_view), path, FALSE);

	gtk_tree_selection_select_path (
			gtk_tree_view_get_selection (GTK_TREE_VIEW (window->tree_view)),
			path);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->tree_view),
				      path, NULL, TRUE, 0.5, 0.5);
	gtk_tree_path_free (path);

	/* check for key name in the list model */
	key = strrchr (location, '/');
	if (!key || strlen (key) < 2)
		return;
	key++;
	nchildren = gtk_tree_model_iter_n_children (window->list_model, NULL);
        for (i = 0; i < nchildren; i++) {
		GtkTreeIter iter;
		char *name;
		gtk_tree_model_iter_nth_child (window->sorted_list_model,
						&iter, NULL, i);

		gtk_tree_model_get (window->sorted_list_model, &iter,
				    MATECONF_LIST_MODEL_KEY_NAME_COLUMN, &name, -1);
		if (strcmp (key, name) == 0) {
			path = gtk_tree_model_get_path (window->sorted_list_model, &iter);
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->list_view)),
                        				path);
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (window->list_view),
						      path, NULL, TRUE, 0.5, 0.5);
			gtk_tree_path_free (path);
			g_free (name);
			return;
		}
		g_free (name);
	}
}

static void
mateconf_editor_window_copy_key_name (GtkAction *action, GtkWidget *callback_data)
{
	MateConfEditorWindow *window = MATECONF_EDITOR_WINDOW (callback_data);
	char *path;
	gchar *key;
	GtkTreeIter iter;
	GtkTreeIter child_iter;
	
	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection
					     (GTK_TREE_VIEW (window->list_view)),
					     NULL, &iter) == TRUE) {
		gtk_tree_model_get (window->sorted_list_model, &iter,
                	            MATECONF_LIST_MODEL_KEY_PATH_COLUMN, &key,
                        	    -1);
		gtk_clipboard_set_text (gtk_clipboard_get
					(GDK_SELECTION_CLIPBOARD), key, -1);
		g_free (key);
		return;
	}
		
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->tree_view)), NULL, &iter);

	gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (window->sorted_tree_model), &child_iter, &iter);
	path = mateconf_tree_model_get_mateconf_path (MATECONF_TREE_MODEL (window->tree_model), &child_iter);

	gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), path, -1);

	g_free (path);
}

static void
mateconf_editor_window_add_bookmark (GtkAction *action, GtkWidget *callback_data)
{
	MateConfEditorWindow *window = MATECONF_EDITOR_WINDOW (callback_data);
	GtkTreeIter iter;
	char *path;
	
	if (!gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->tree_view)), NULL, &iter)) {
		return;
	}
	else {
		GtkTreeIter child_iter;

		gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (window->sorted_tree_model), &child_iter, &iter);
		path = mateconf_tree_model_get_mateconf_path (MATECONF_TREE_MODEL (window->tree_model), &child_iter);
	}

	mateconf_bookmarks_add_bookmark (path);
	
	g_free (path);
}

static void
mateconf_editor_window_recents_init (void)
{
	MateConfClient *client = mateconf_client_get_default ();

	mateconf_client_add_dir (client, "/apps/mateconf-editor/recents",
			      MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	g_object_unref (client);
}

static void
mateconf_add_recent_key (MateConfEditorWindow *window, const char *key)
{
	GSList *list, *tmp;
	gboolean add = TRUE;

	/* Get the old list and then set it */
	list = mateconf_client_get_list (window->client,
				      "/apps/mateconf-editor/recents", MATECONF_VALUE_STRING, NULL);

	/* Check that the recent hasn't been added already */
	for (tmp = list; tmp; tmp = tmp->next) {
		if (strcmp ((gchar*) tmp->data, key) == 0)
			add = FALSE;
	}

	/* Append the new key */
	if (add == TRUE) {
		/* Keep list on a fixed size, remove the last */
		if (g_slist_length (list) > RECENT_LIST_MAX_SIZE - 1) {
			tmp = g_slist_last (list);
			g_free (tmp->data);
			list = g_slist_delete_link (list, tmp);
		}

		list = g_slist_prepend (list, g_strdup (key));

		mateconf_client_set_list (window->client,
				       "/apps/mateconf-editor/recents", MATECONF_VALUE_STRING, list, NULL);

		if (gtk_widget_get_visible (GTK_WIDGET (window->output_window)) &&
	    				window->output_window_type == MATECONF_EDITOR_WINDOW_OUTPUT_WINDOW_RECENTS) 
			gedit_output_window_prepend_line (GEDIT_OUTPUT_WINDOW (window->output_window),
							  (gchar*) key, TRUE);
	}


	for (tmp = list; tmp; tmp = tmp->next) {
		g_free (tmp->data);
	}

	g_slist_free (list);
}


static void
mateconf_editor_window_edit_bookmarks (GtkAction *action, GtkWidget *callback_data)
{
	static GtkWidget *dialog = NULL;

	if (dialog != NULL) {
		gtk_window_present (GTK_WINDOW (dialog));
		return;
	}
	
	dialog = mateconf_bookmarks_dialog_new (GTK_WINDOW (callback_data));
	g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer)&dialog);
	gtk_widget_show (dialog);
}


static void
mateconf_editor_window_search (GtkAction *action, GtkWidget *callback_data)
{
	static GtkWidget *dialog = NULL;

	if (dialog != NULL) {
		gtk_window_present (GTK_WINDOW (dialog));
		return;
	}

	dialog = mateconf_search_dialog_new (GTK_WINDOW (callback_data));
	g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer)&dialog);
	gtk_widget_show (dialog);
}

static void
mateconf_editor_show_recent_keys (GtkAction *action, GtkWidget *window)
{
	GSList *list, *tmp;
	gedit_output_window_clear (GEDIT_OUTPUT_WINDOW (MATECONF_EDITOR_WINDOW (window)->output_window));
	list = mateconf_client_get_list (MATECONF_EDITOR_WINDOW (window)->client,
                                     "/apps/mateconf-editor/recents", MATECONF_VALUE_STRING, NULL);
	for (tmp = list; tmp; tmp = tmp->next) {
		gedit_output_window_append_line (GEDIT_OUTPUT_WINDOW (MATECONF_EDITOR_WINDOW (window)->output_window),
						 (gchar*) tmp->data, FALSE);
		g_free (tmp->data);
	}

	MATECONF_EDITOR_WINDOW (window)->output_window_type = MATECONF_EDITOR_WINDOW_OUTPUT_WINDOW_RECENTS;
	gtk_widget_show (GTK_WIDGET (MATECONF_EDITOR_WINDOW (window)->output_window));
	
	g_slist_free (list);
}

static void
mateconf_editor_window_new_window (GtkAction *action, GtkWidget *callback_data)
{
	GtkWidget *new_window;

	new_window = mateconf_editor_application_create_editor_window (MATECONF_EDITOR_WINDOW_TYPE_NORMAL);
	gtk_widget_show (new_window);
}

static void
mateconf_editor_window_new_defaults_window (GtkAction *action, GtkWidget *callback_data)
{
	GtkWidget *new_window;

	new_window = mateconf_editor_application_create_editor_window (MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS);
	gtk_widget_show (new_window);
}

static void
mateconf_editor_window_new_mandatory_window (GtkAction *action, GtkWidget *callback_data)
{
	GtkWidget *new_window;

	new_window = mateconf_editor_application_create_editor_window (MATECONF_EDITOR_WINDOW_TYPE_MANDATORY);
	gtk_widget_show (new_window);
}

static void
help_cb (GtkAction *action, GtkWidget *callback_data)
{
        GError *error = NULL;

	gtk_show_uri (gtk_widget_get_screen (callback_data),
		      "ghelp:mateconf-editor", gtk_get_current_event_time (), &error);
                                                                                
        if (error != NULL) {
                mateconf_editor_window_popup_error_dialog (GTK_WINDOW (MATECONF_EDITOR_WINDOW (callback_data)),
                                                        _("Couldn't display help: %s"), error);
        }
}

static void
mateconf_editor_window_close_output_window (GtkWidget *widget, gpointer user_data)
{
	gedit_output_window_clear (GEDIT_OUTPUT_WINDOW (widget));
	gtk_widget_hide (widget);
}

static void
mateconf_editor_window_output_window_changed (GtkWidget *widget, gchar *key, gpointer user_data)
{
	MateConfEditorWindow *window = (MateConfEditorWindow*) user_data;
	mateconf_editor_window_go_to (window, key);
}

static void
mateconf_editor_window_about_window (GtkAction *action, GtkWidget *callback_data)
{
	const gchar *authors[] = {
		"Anders Carlsson <andersca@mate.org>",
		"Fernando Herrera <fherrera@onirica.com>",
		NULL
	};
	const gchar *documenters[] = {
		"Sun Microsystems",
		"Shaun McCance <shaunm@mate.org>",
		NULL
	};

	gtk_show_about_dialog (GTK_WINDOW (callback_data),
#if GTK_CHECK_VERSION (2, 11, 0)
                              "program-name", _("Configuration Editor"),
#else
                              "name", _("Configuration Editor"),
#endif
			      "version", VERSION,
			      "copyright", "Copyright \xc2\xa9 2001-2004 Anders Carlsson",
			      "comments", _("An editor for the MateConf configuration system."),
			      "authors", authors,
			      "documenters", documenters,
			      "translator-credits", _("translator-credits"),
			      "logo-icon-name", "mateconf-editor",
			      NULL);
}	

static void
mateconf_editor_popup_window_unset_key (GtkAction *action, GtkWidget *callback_data)
{
	MateConfEditorWindow *window = MATECONF_EDITOR_WINDOW (callback_data);
	GtkTreeIter iter;
	gchar *key;
	GError *error = NULL;
	
	if (!gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->list_view)),
					 NULL, &iter)) {
		return;
	}

	gtk_tree_model_get (window->sorted_list_model, &iter,
			    MATECONF_LIST_MODEL_KEY_PATH_COLUMN, &key,
			    -1);

	mateconf_client_unset (window->client, key, &error);
	g_free (key);

	if (error != NULL) {
		mateconf_editor_window_popup_error_dialog (GTK_WINDOW (window),
							_("Couldn't unset key. Error was:\n%s"), error);
	}
}

static void
mateconf_editor_new_key_response (GtkDialog *editor,
			       int        response,
			       MateConfEditorWindow *window)
{
	if (response == GTK_RESPONSE_OK) {
		MateConfValue *value;
		char       *full_path;
		char       *why_not_valid = NULL;
		
		full_path = mateconf_key_editor_get_full_key_path (MATECONF_KEY_EDITOR (editor));
		
		if (!mateconf_valid_key (full_path, &why_not_valid)) {
			GtkWidget *message_dialog;

			g_assert (why_not_valid != NULL);

			message_dialog = gtk_message_dialog_new (GTK_WINDOW (editor),
								 GTK_DIALOG_MODAL,
								 GTK_MESSAGE_ERROR,
								 GTK_BUTTONS_OK,
								 _("Could not create key. The error is:\n%s"),
								 why_not_valid);
			gtk_dialog_set_default_response (GTK_DIALOG (message_dialog), GTK_RESPONSE_OK);
			gtk_dialog_run (GTK_DIALOG (message_dialog));
			gtk_widget_destroy (message_dialog);

			g_free (full_path);
			g_free (why_not_valid);

			/* leave the key editor in place */
			return;
		}
			
		/* Create the key */
		value = mateconf_key_editor_get_value (MATECONF_KEY_EDITOR (editor));

		mateconf_client_set (window->client, full_path, value, NULL);
		mateconf_add_recent_key (window, full_path);

		if(value)
		  mateconf_value_free (value);
		g_free (full_path);
	}

	gtk_widget_destroy (GTK_WIDGET (editor));
}

static void
mateconf_editor_popup_window_new_key (GtkAction *action, GtkWidget *callback_data)
{
	MateConfEditorWindow *window = MATECONF_EDITOR_WINDOW (callback_data);
	GtkTreeIter iter;
	GtkWidget *editor;
	char *path;
	
	editor = mateconf_key_editor_new (MATECONF_KEY_EDITOR_NEW_KEY);

	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->tree_view)), NULL, &iter)) {
		GtkTreeIter child_iter;

		gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (window->sorted_tree_model), &child_iter, &iter);
		path = mateconf_tree_model_get_mateconf_path (MATECONF_TREE_MODEL (window->tree_model), &child_iter);
	}
	else {
		path = g_strdup ("/");
	}
	
	mateconf_key_editor_set_key_path (MATECONF_KEY_EDITOR (editor), path);
	mateconf_key_editor_set_writable (MATECONF_KEY_EDITOR (editor), TRUE);
	g_free (path);

	g_signal_connect (editor, "response",
			  G_CALLBACK (mateconf_editor_new_key_response), window);

	gtk_widget_show (editor);
}

static void
mateconf_editor_edit_key_response (GtkDialog *editor,
				int        response,
				MateConfEditorWindow *window)
{
	if (response == GTK_RESPONSE_OK) {
		MateConfValue *value;
		GError     *error = NULL;
		const char *path;

		value = mateconf_key_editor_get_value (MATECONF_KEY_EDITOR (editor));

		path = mateconf_key_editor_get_key_name (MATECONF_KEY_EDITOR (editor));
		g_assert (mateconf_valid_key (path, NULL));

		/* if not writable we weren't allowed to change things anyway */
		if (mateconf_client_key_is_writable (window->client, path, &error)) {
			mateconf_client_set (window->client, path, value, &error);
			mateconf_add_recent_key (window, path);
		}

		mateconf_value_free (value);

		if (error != NULL) {
			mateconf_editor_window_popup_error_dialog (GTK_WINDOW (editor),
								_("Could not change key value. Error message:\n%s"),
								error);
			return;
		}
	}

	gtk_widget_destroy (GTK_WIDGET (editor));
}

static void
mateconf_editor_popup_window_edit_key (GtkAction *action, GtkWidget *callback_data)
{
	MateConfEditorWindow *window = MATECONF_EDITOR_WINDOW (callback_data);
	GtkTreeIter iter;
	GtkWidget *editor, *dialog;
	MateConfValue *value; 
	char *path = NULL;
	
	if (!gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->list_view)),
					      NULL, &iter))
		return;
	
	gtk_tree_model_get (window->sorted_list_model, &iter,
			    MATECONF_LIST_MODEL_KEY_PATH_COLUMN, &path,
			    MATECONF_LIST_MODEL_VALUE_COLUMN, &value,
			    -1);

	if (value && (value->type == MATECONF_VALUE_SCHEMA ||
		      value->type == MATECONF_VALUE_PAIR)) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (window), 0,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_OK,
						 _("Currently pairs and schemas can't "
						   "be edited. This will be changed in a later "
						   "version."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		mateconf_value_free (value);
		g_free (path);
		return;
	}
	
	editor = mateconf_key_editor_new (MATECONF_KEY_EDITOR_EDIT_KEY);

	if(value) {
	  mateconf_key_editor_set_value (MATECONF_KEY_EDITOR (editor), value);
	  mateconf_value_free (value);
	}

	mateconf_key_editor_set_key_name (MATECONF_KEY_EDITOR (editor), path);

	mateconf_key_editor_set_writable (MATECONF_KEY_EDITOR (editor),
				       mateconf_client_key_is_writable (window->client, path, NULL));

	g_free (path);

	g_signal_connect (editor, "response",
			  G_CALLBACK (mateconf_editor_edit_key_response), window);

	gtk_widget_show (editor);
}

static void
mateconf_editor_window_list_view_row_activated (GtkTreeView       *tree_view,
					     GtkTreePath       *path,
					     GtkTreeViewColumn *column,
					     MateConfEditorWindow *window)
{
	mateconf_editor_popup_window_edit_key (NULL, GTK_WIDGET (window));
}

static gboolean
mateconf_editor_window_test_expand_row (GtkTreeView       *tree_view,
				     GtkTreeIter       *iter,
				     GtkTreePath       *path,
				     gpointer           data)
{
  MateConfEditorWindow *mateconfwindow = MATECONF_EDITOR_WINDOW (data);
  GdkCursor *cursor;

  if (!gtk_widget_get_realized (GTK_WIDGET (mateconfwindow)))
    return FALSE;

  cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (mateconfwindow)),
                         cursor);
  gdk_cursor_unref (cursor);

  gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (mateconfwindow)));

  return FALSE;
}

static void
mateconf_editor_window_row_expanded (GtkTreeView       *tree_view,
				  GtkTreeIter       *iter,
				  GtkTreePath       *path,
				  gpointer           data)
{
  MateConfEditorWindow *mateconfwindow = MATECONF_EDITOR_WINDOW (data);

  if (!gtk_widget_get_realized (GTK_WIDGET (mateconfwindow)))
    return;

  gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (mateconfwindow)), NULL);
  gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (mateconfwindow)));
}

static void
mateconf_editor_popup_policykit_callback (GtkWindow *window, GError *error)
{
  if (error)
    mateconf_editor_window_popup_error_dialog (window, _("Could not set value. Error was:\n%s"), error);
}

static void
mateconf_editor_popup_window_set_as_default (GtkAction *action, GtkWidget *callback_data)
{
  MateConfEditorWindow *mateconfwindow = MATECONF_EDITOR_WINDOW (callback_data);

  GtkTreeIter iter;
  MateConfValue *value;
  char *path = NULL;
  GtkWindow *window = GTK_WINDOW (callback_data);
  gboolean can_use_pk;
  
  can_use_pk = (mateconfwindow->type == MATECONF_EDITOR_WINDOW_TYPE_NORMAL);
  gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (mateconfwindow->list_view)),
                                   NULL, &iter);
  gtk_tree_model_get (mateconfwindow->sorted_list_model, &iter,
                      MATECONF_LIST_MODEL_KEY_PATH_COLUMN, &path,
                      MATECONF_LIST_MODEL_VALUE_COLUMN, &value,
                      -1);

  if (mateconf_util_can_edit_defaults ()) {
    MateConfClient *client;

    client = mateconf_editor_window_get_client (MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS);
    if (client != NULL) {
      GError *error = NULL;

      mateconf_client_set (client, path, value, NULL);
      mateconf_client_suggest_sync (client, &error);
      g_object_unref (client);

      if (!error)
        return;
      if (!can_use_pk)
        mateconf_editor_window_popup_error_dialog (window, _("Could not sync value. Error was:\n%s"), error);
    }
  }
  if (can_use_pk) {
    mateconf_client_suggest_sync (mateconfwindow->client, NULL);
    mateconf_pk_set_default_async (path,
                                (GFunc) mateconf_editor_popup_policykit_callback,
                                g_object_ref (window), g_object_unref);
  }
}

static void
mateconf_editor_popup_window_set_as_mandatory (GtkAction *action, GtkWidget *callback_data)
{
  MateConfEditorWindow *mateconfwindow = MATECONF_EDITOR_WINDOW (callback_data);

  GtkTreeIter iter;
  MateConfValue *value;
  char *path = NULL;
  GtkWindow *window = GTK_WINDOW (callback_data);
  gboolean can_use_pk;
  
  can_use_pk = (mateconfwindow->type == MATECONF_EDITOR_WINDOW_TYPE_NORMAL);
  gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (mateconfwindow->list_view)),
                                   NULL, &iter);
  gtk_tree_model_get (mateconfwindow->sorted_list_model, &iter,
                      MATECONF_LIST_MODEL_KEY_PATH_COLUMN, &path,
                      MATECONF_LIST_MODEL_VALUE_COLUMN, &value,
                      -1);

  if (mateconf_util_can_edit_mandatory ()) {
    MateConfClient *client;

    client = mateconf_editor_window_get_client (MATECONF_EDITOR_WINDOW_TYPE_MANDATORY);
    if (client != NULL) {
      GError *error = NULL;

      mateconf_client_set (client, path, value, NULL);
      mateconf_client_suggest_sync (client, &error);
      g_object_unref (client);

      if (!error)
        return;
      if (!can_use_pk)
        mateconf_editor_window_popup_error_dialog (window, _("Could not sync value. Error was:\n%s"), error);
    }
  }
  if (can_use_pk) {
    mateconf_client_suggest_sync (mateconfwindow->client, NULL);
    mateconf_pk_set_mandatory_async (path,
                                  (GFunc) mateconf_editor_popup_policykit_callback,
                                  g_object_ref (window), g_object_unref);
  }
}

static GtkActionEntry entries[] = {
	{ "FileMenu", NULL, N_("_File"), NULL, NULL, NULL },
	{ "EditMenu", NULL, N_("_Edit"), NULL, NULL, NULL },
	{ "SearchMenu", NULL, N_("_Search"), NULL, NULL, NULL },
	{ "BookmarksMenu", NULL, N_("_Bookmarks"), NULL, NULL, NULL },
	{ "HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL },

	{ "NewWindow", GTK_STOCK_NEW, N_("New _Settings Window"), "<control>S", 
	  N_("Open a new Configuration Editor window editing current settings"), 
	  G_CALLBACK (mateconf_editor_window_new_window) },
	{ "NewDefaultsWindow", GTK_STOCK_NEW, N_("New _Defaults Window"), "<control>D", 
          N_("Open a new Configuration Editor window editing system default settings"), 
	  G_CALLBACK (mateconf_editor_window_new_defaults_window) },
	{ "NewMandatoryWindow", GTK_STOCK_NEW, N_("New _Mandatory Window"), "<control>M", 
          N_("Open a new Configuration Editor window editing system mandatory settings"), 
	  G_CALLBACK (mateconf_editor_window_new_mandatory_window) },
	{ "CloseWindow", GTK_STOCK_CLOSE, N_("_Close Window"), "<control>W", N_("Close this window"), 
	  G_CALLBACK (mateconf_editor_window_close_window) },
	{ "QuitMateConfEditor", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", N_("Quit the Configuration Editor"), 
	  G_CALLBACK (gtk_main_quit) },
	
	{ "CopyKeyName", GTK_STOCK_COPY, N_("_Copy Key Name"), "<control>C", N_("Copy the name of the selected key"), 
	  G_CALLBACK (mateconf_editor_window_copy_key_name) },
	{ "Find", GTK_STOCK_FIND, N_("_Find..."), "<control>F", N_("Find patterns in keys and values"), 
	  G_CALLBACK (mateconf_editor_window_search) },
	{ "RecentKeys", NULL, N_("_List Recent Keys"), "<control>R", N_("Show recently modified keys"), 
	  G_CALLBACK (mateconf_editor_show_recent_keys) },

	{ "AddBookmark", STOCK_ADD_BOOKMARK, N_("_Add Bookmark"), NULL, N_("Add a bookmark to the selected directory"), 
	  G_CALLBACK (mateconf_editor_window_add_bookmark) },
	{ "EditBookmarks", STOCK_EDIT_BOOKMARK, N_("_Edit Bookmarks"), NULL, N_("Edit the bookmarks"), 
	G_CALLBACK (mateconf_editor_window_edit_bookmarks) },
	
	{ "HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1", N_("Open the help contents for the Configuration Editor"), 
	  G_CALLBACK (help_cb) },
	{ "AboutAction", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("Show the about dialog for the Configuration Editor"), 
	  G_CALLBACK (mateconf_editor_window_about_window) },

	{ "NewKey", GTK_STOCK_NEW, N_("_New Key..."), "<control>N", N_("Create a new key"),
	  G_CALLBACK (mateconf_editor_popup_window_new_key) },
	{ "EditKey", NULL, N_("_Edit Key..."), NULL, N_("Edit the selected key"),
	  G_CALLBACK (mateconf_editor_popup_window_edit_key) },
	{ "UnsetKey", GTK_STOCK_DELETE, N_("_Unset Key"), NULL, N_("Unset the selected key"),
	  G_CALLBACK (mateconf_editor_popup_window_unset_key) },
	{ "DefaultKey", NULL, N_("Set as _Default"), NULL, N_("Set the selected key to be the default"),
	  G_CALLBACK (mateconf_editor_popup_window_set_as_default) },
	{ "MandatoryKey", NULL, N_("Set as _Mandatory"), NULL, N_("Set the selected key to the mandatory"),
	  G_CALLBACK (mateconf_editor_popup_window_set_as_mandatory) }	
};

static const char *ui_description = 
	"<ui>"
	"	<menubar name='MateConfEditorMenu'>"
	"		<menu action='FileMenu'>"
	"			<menuitem action='NewWindow'/>"
	"			<menuitem action='NewDefaultsWindow'/>"
	"			<menuitem action='NewMandatoryWindow'/>"
	"			<menuitem action='CloseWindow'/>"
	"			<separator/>"
	"			<menuitem action='QuitMateConfEditor'/>"
	"		</menu>"
	"		<menu action='EditMenu'>"
	"			<menuitem action='CopyKeyName'/>"
	"			<separator/>"
	"			<menuitem action='RecentKeys'/>"
	"			<separator/>"
	"			<menuitem action='Find'/>"
	"		</menu>"	
	"		<menu action='BookmarksMenu'>"
	"			<menuitem action='AddBookmark'/>"
	"			<menuitem action='EditBookmarks'/>"
	"		</menu>"
	"		<menu action='HelpMenu'>"
	"			<menuitem action='HelpContents'/>"
	"			<menuitem action='AboutAction'/>"
	"		</menu>"
	"	</menubar>"
	"	<popup name='MateConfKeyPopupMenu'>"
	"		<menuitem action='NewKey'/>"
	"		<menuitem action='EditKey'/>"
	"		<separator/>"
	"		<menuitem action='UnsetKey'/>"
	"		<separator/>"
	"		<menuitem action='DefaultKey'/>"	
	"		<separator/>"
	"		<menuitem action='MandatoryKey'/>"	
	"	</popup>"
	"</ui>";

static gchar *image_menu_items_paths[] = {
	"/MateConfEditorMenu/FileMenu/NewWindow",
	"/MateConfEditorMenu/FileMenu/NewDefaultsWindow",
	"/MateConfEditorMenu/FileMenu/NewMandatoryWindow",
	"/MateConfEditorMenu/FileMenu/CloseWindow",
	"/MateConfEditorMenu/FileMenu/QuitMateConfEditor",

	"/MateConfEditorMenu/EditMenu/CopyKeyName",
	"/MateConfEditorMenu/EditMenu/RecentKeys",
	"/MateConfEditorMenu/EditMenu/Find",

	"/MateConfEditorMenu/HelpMenu/HelpContents",
	"/MateConfEditorMenu/HelpMenu/AboutAction",

	"/MateConfKeyPopupMenu/NewKey",
	"/MateConfKeyPopupMenu/UnsetKey"
};

static void
mateconf_editor_window_row_activated (GtkTreeView *treeview, 
				   GtkTreePath *path,
				   GtkTreeViewColumn *column,
				   MateConfEditorWindow *window)
{
	if (gtk_tree_view_row_expanded (treeview, path)) {
		gtk_tree_view_collapse_row (treeview, path);
	} else {
		gtk_tree_view_expand_row (treeview, path, FALSE);
	}
}

static void
mateconf_editor_window_selection_changed (GtkTreeSelection *selection, MateConfEditorWindow *window)
{
	GtkTreeIter iter;
	gchar *base_title;

	switch (window->type) {
	case MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS:
		base_title = _("Configuration Editor (Default settings)");
		break;
	case MATECONF_EDITOR_WINDOW_TYPE_MANDATORY:
		base_title = _("Configuration Editor (Mandatory settings)");
		break;
	default:
		base_title = _("Configuration Editor");
	}
		
	if (selection == NULL)
		gtk_window_set_title (GTK_WINDOW (window), base_title);
	else {
		gchar *name, *title, *path;
		GtkTreeIter child_iter;

		if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
			gtk_window_set_title (GTK_WINDOW (window), base_title);
			return;
		}

		gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (window->sorted_tree_model), &child_iter, &iter);

		name = mateconf_tree_model_get_mateconf_name (MATECONF_TREE_MODEL (window->tree_model), &child_iter);

		if (strlen (name) > 0) {
			title = g_strdup_printf ("%s - %s", base_title, name);
		}
		else {
			title = g_strdup (base_title);
		}
		
		gtk_window_set_title (GTK_WINDOW (window), title);
		g_free (title);
		g_free (name);

		path = mateconf_tree_model_get_mateconf_path (MATECONF_TREE_MODEL (window->tree_model), &child_iter);
		gtk_statusbar_pop (GTK_STATUSBAR (window->statusbar), 0);
		gtk_statusbar_push (GTK_STATUSBAR (window->statusbar), 0, path);

		mateconf_list_model_set_root_path (MATECONF_LIST_MODEL (window->list_model), path);

		g_free (path);
	}
}

static gboolean
list_view_button_press_event (GtkTreeView *tree_view, GdkEventButton *event, MateConfEditorWindow *window)
{
	GtkTreePath *path;

	if (event->button == 3) {
		gtk_widget_grab_focus (GTK_WIDGET (tree_view));

		/* Select our row */
		if (gtk_tree_view_get_path_at_pos (tree_view, event->x, event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (tree_view), path);

			gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/EditKey"), 
						  TRUE);
			gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/UnsetKey"), 
						  window->type != MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS);
			gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/DefaultKey"), 
						  (mateconf_util_can_edit_defaults () && window->type != MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS) ||
						  (mateconf_pk_can_set_default ("/") && window->type == MATECONF_EDITOR_WINDOW_TYPE_NORMAL));
			gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/MandatoryKey"), 
						  (mateconf_util_can_edit_mandatory () && window->type != MATECONF_EDITOR_WINDOW_TYPE_MANDATORY) ||
						  (mateconf_pk_can_set_mandatory ("/") && window->type == MATECONF_EDITOR_WINDOW_TYPE_NORMAL));
			
			gtk_tree_path_free (path);
		}
		else {
			gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/EditKey"), 
						  FALSE);
			gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/UnsetKey"), 
						  FALSE);
			gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/DefaultKey"), 
						  FALSE);
			gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/MandatoryKey"), 
						  FALSE);

		}
		
		gtk_menu_popup (GTK_MENU (window->popup_menu), NULL, NULL, NULL, NULL,
				event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

static void
mateconf_editor_mateconf_value_changed (MateConfCellRenderer *cell, const gchar *path_str, MateConfValue *new_value, MateConfEditorWindow *window)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *key;
	MateConfValue *value;
	
	path = gtk_tree_path_new_from_string (path_str);

	gtk_tree_model_get_iter (window->sorted_list_model, &iter, path);

	gtk_tree_model_get (window->sorted_list_model, &iter,
			    MATECONF_LIST_MODEL_KEY_PATH_COLUMN, &key,
			    MATECONF_LIST_MODEL_VALUE_COLUMN, &value,
			    -1);

	/* We need to check this because the changed signal could come from an old
	 * cell in another list_model */ 
	if (value->type == new_value->type) {
		mateconf_client_set (window->client, key, new_value, NULL);
		mateconf_add_recent_key (window, key);
	}

	mateconf_value_free (value);
	g_free (key);
	gtk_tree_path_free (path);
}

static gboolean
mateconf_editor_check_writable (MateConfCellRenderer *cell, const gchar *path_str, MateConfEditorWindow *window)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *key;
	gboolean ret;
	
	path = gtk_tree_path_new_from_string (path_str);

	gtk_tree_model_get_iter (window->sorted_list_model, &iter, path);

	gtk_tree_model_get (window->sorted_list_model, &iter,
			    MATECONF_LIST_MODEL_KEY_PATH_COLUMN, &key,
			    -1);

	ret = mateconf_client_key_is_writable (window->client, key, NULL);

	g_free (key);

	return ret;
}

static void
mateconf_editor_window_list_view_popup_position (GtkMenu   *menu G_GNUC_UNUSED,
					      gint      *x,
					      gint      *y,
					      gboolean  *push_in G_GNUC_UNUSED,
					      GdkWindow *window)
{
	gdk_window_get_origin (window, x, y);
}

static void
mateconf_editor_window_list_view_popup_menu (GtkWidget *widget, MateConfEditorWindow *window)
{
	GtkTreeIter iter;

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->list_view));
	/* Select our row */
	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/EditKey"), 
					  TRUE);
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/UnsetKey"), 
					  window->type != MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS);
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/DefaultKey"), 
					  (mateconf_util_can_edit_defaults () && window->type != MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS) ||
					  (mateconf_pk_can_set_default ("/") && window->type == MATECONF_EDITOR_WINDOW_TYPE_NORMAL));
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/MandatoryKey"), 
					  (mateconf_util_can_edit_mandatory () && window->type != MATECONF_EDITOR_WINDOW_TYPE_MANDATORY) ||
					  (mateconf_pk_can_set_mandatory ("/") && window->type == MATECONF_EDITOR_WINDOW_TYPE_NORMAL));
	}
	else {
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/EditKey"), 
					  FALSE);
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/UnsetKey"), 
					  FALSE);
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/DefaultKey"), 
					  FALSE);
		gtk_widget_set_sensitive (gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu/MandatoryKey"), 
					  FALSE);

	}
	
	gtk_menu_popup (GTK_MENU (window->popup_menu), NULL, NULL,
			(GtkMenuPositionFunc) mateconf_editor_window_list_view_popup_position, 
			gtk_widget_get_window (widget),
			0, gtk_get_current_event_time ());
}

static char *
strip_whitespace (const char *text)
{
	const char *p;
	const char *end;
	GString *str;

	p = text;
	end = text + strlen (text);

	/* First skip the leading whitespace */
	while (p != end && g_unichar_isspace (g_utf8_get_char (p))) {
	  p = g_utf8_next_char (p);
	}
	  
	str = g_string_new (NULL);
	
	while (p != end) {
		gunichar ch;

		ch = g_utf8_get_char (p);

		if (g_unichar_isspace (ch)) {
			while (p != end && g_unichar_isspace (ch)) {
				p = g_utf8_next_char (p);
				ch = g_utf8_get_char (p);
			}

			p = g_utf8_prev_char (p);
			g_string_append_unichar (str, ' ');
		}
		else {
			g_string_append_unichar (str, ch);
		}

		p = g_utf8_next_char (p);
	}

	return g_string_free (str, FALSE);
}

static void
set_label_and_strip_whitespace (GtkLabel *label, const char *text)
{
	char *stripped_text;

	stripped_text = strip_whitespace (text);
	gtk_label_set_text (GTK_LABEL (label), stripped_text);
	g_free (stripped_text);
}

static void
mateconf_editor_window_update_list_selection (GtkTreeSelection *selection, MateConfEditorWindow *window)
{
	GtkTreeIter iter;
	MateConfSchema *schema;
	char *path;
	
	if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gtk_label_set_text (GTK_LABEL (window->key_name_label), _("(None)"));
		gtk_label_set_text (GTK_LABEL (window->owner_label), _("(None)"));
		gtk_label_set_text (GTK_LABEL (window->short_desc_label), _("(None)"));
		gtk_text_buffer_set_text (window->long_desc_buffer, _("(None)"), -1);
		gtk_widget_hide (window->non_writable_label);
		gtk_widget_hide (window->no_schema_label);
		
		return;
	}
	
	gtk_tree_model_get (window->sorted_list_model, &iter,
			    MATECONF_LIST_MODEL_KEY_PATH_COLUMN, &path,
			    -1);
	
	gtk_label_set_text (GTK_LABEL (window->key_name_label), path);

	if (mateconf_client_key_is_writable (window->client, path, NULL))
		gtk_widget_hide (window->non_writable_label);
	else
		gtk_widget_show (window->non_writable_label);

	schema = mateconf_client_get_schema_for_key (window->client, path);

	if (schema != NULL && mateconf_schema_get_long_desc (schema) != NULL) {
		char *long_desc;

		long_desc = strip_whitespace (mateconf_schema_get_long_desc (schema));
		
		gtk_text_buffer_set_text (window->long_desc_buffer, long_desc, -1);
		g_free (long_desc);
	}
	else {
		gtk_text_buffer_set_text (window->long_desc_buffer, _("(None)"), -1);
	}

	if (schema != NULL && mateconf_schema_get_short_desc (schema) != NULL) {
		set_label_and_strip_whitespace (GTK_LABEL (window->short_desc_label),
						mateconf_schema_get_short_desc (schema));
	}
	else {
		gtk_label_set_text (GTK_LABEL (window->short_desc_label), _("(None)"));

	}

	if (schema != NULL && mateconf_schema_get_owner (schema) != NULL) {
		set_label_and_strip_whitespace (GTK_LABEL (window->owner_label),
						mateconf_schema_get_owner (schema));
	}
	else {
		gtk_label_set_text (GTK_LABEL (window->owner_label), _("(None)"));

	}

	if (schema == NULL) {
		MateConfValue *value = mateconf_client_get (window->client, path, NULL);
		if (value != NULL) {
			if (value->type == MATECONF_VALUE_SCHEMA) {
				gtk_widget_hide (window->no_schema_label);
			}
			else {
				gtk_widget_show (window->no_schema_label);
			}
			mateconf_value_free (value);
		}
	}
	else {
		gtk_widget_hide (window->no_schema_label);
	}

	if (schema != NULL)
		mateconf_schema_free (schema);

	gtk_statusbar_pop (GTK_STATUSBAR (window->statusbar), 0);
	gtk_statusbar_push (GTK_STATUSBAR (window->statusbar), 0, path);

	g_free (path);
}

static void
mateconf_editor_window_have_tearoffs_notify (MateConfClient       *client,
					  guint              cnxn_id,
					  MateConfEntry        *entry,
					  MateConfEditorWindow *window)
{
	gboolean have_tearoffs;

	if (entry->value->type != MATECONF_VALUE_BOOL)
		return;

	have_tearoffs = mateconf_value_get_bool (entry->value);

	gtk_ui_manager_set_add_tearoffs (window->ui_manager, have_tearoffs);
}

static void
mateconf_editor_window_set_item_has_icon (GtkUIManager *ui_manager,
				       const char     *path,
				       gboolean        have_icons)
{
	GtkWidget *item;
	GtkWidget *image;

	item = gtk_ui_manager_get_widget (ui_manager, path);

	image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (item));
	if (image && !g_object_get_data (G_OBJECT (item), "mateconf-editor-icon"))
		g_object_set_data_full (G_OBJECT (item), "mateconf-editor-icon",
					g_object_ref (image), g_object_unref);

	if (!image)
		image = g_object_get_data (G_OBJECT (item), "mateconf-editor-icon");

	if (!image)
		return;

	if (have_icons)
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	else
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), NULL);
}

static void
mateconf_editor_window_set_have_icons (MateConfEditorWindow *window,
				    gboolean           have_icons)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (image_menu_items_paths); i++)
		mateconf_editor_window_set_item_has_icon (window->ui_manager, image_menu_items_paths [i], have_icons);
}

static void
mateconf_editor_window_have_icons_notify (MateConfClient       *client,
				       guint              cnxn_id,
				       MateConfEntry        *entry,
				       MateConfEditorWindow *window)
{
	gboolean have_icons;

	if (entry->value->type != MATECONF_VALUE_BOOL)
		return;

	have_icons = mateconf_value_get_bool (entry->value);

	mateconf_editor_window_set_have_icons (window, have_icons);
}

static void
mateconf_editor_window_setup_ui_prefs_handler (MateConfEditorWindow *window)
{
	MateConfClient *client;

	client = mateconf_client_get_default ();

	gtk_ui_manager_set_add_tearoffs (
		window->ui_manager, mateconf_client_get_bool (client, "/desktop/mate/interface/menus_have_tearoff", 
							   NULL));

	window->tearoffs_notify_id = mateconf_client_notify_add (
						client,
						"/desktop/mate/interface/menus_have_tearoff",
						(MateConfClientNotifyFunc) mateconf_editor_window_have_tearoffs_notify,
						window, NULL, NULL);

	mateconf_editor_window_set_have_icons (
		window, mateconf_client_get_bool (client, "/desktop/mate/interface/menus_have_icons", NULL));

	window->icons_notify_id = mateconf_client_notify_add (
						client,
						"/desktop/mate/interface/menus_have_icons",
						(MateConfClientNotifyFunc) mateconf_editor_window_have_icons_notify,
						window, NULL, NULL);

	g_object_unref (client);
}

static void
mateconf_editor_window_style_set (GtkWidget *widget, GtkStyle *prev_style, GtkWidget *text_view)
{
	GtkStyle *style;

	style = gtk_widget_get_style (widget);
	gtk_widget_modify_base (text_view, GTK_STATE_NORMAL,
				&style->bg[GTK_STATE_NORMAL]);
	gtk_widget_modify_text (text_view, GTK_STATE_NORMAL,
				&style->fg[GTK_STATE_NORMAL]);
}

void
mateconf_editor_window_expand_first (MateConfEditorWindow *window)
{
	GtkTreePath *path;

	path = gtk_tree_path_new_first ();
	gtk_tree_view_expand_row (GTK_TREE_VIEW (window->tree_view), path, FALSE);
	gtk_tree_path_free (path);
}

static MateConfClient*
mateconf_editor_window_get_client (int type)
{
	MateConfEngine *engine;
	MateConfClient *client = NULL;
	GError *error = NULL;

	switch (type) {
	case MATECONF_EDITOR_WINDOW_TYPE_NORMAL:
		client = mateconf_client_get_default ();
		break;
	case MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS:
		engine = mateconf_engine_get_for_address (MATECONF_DEFAULTS_SOURCE, &error);
		if (error) {
			mateconf_editor_window_popup_error_dialog (NULL,
			_("Cannot create MateConf engine. Error was:\n%s"), error);
			return NULL;
		}
		client = mateconf_client_get_for_engine (engine);
		mateconf_engine_unref (engine);
		break;
	case MATECONF_EDITOR_WINDOW_TYPE_MANDATORY:
		engine = mateconf_engine_get_for_address (MATECONF_MANDATORY_SOURCE, &error);
		if (error) {
			mateconf_editor_window_popup_error_dialog (NULL,
			_("Cannot create MateConf engine. Error was:\n%s"), error);
			return NULL;
		}
		client = mateconf_client_get_for_engine (engine);
		mateconf_engine_unref (engine);
		break;
	default:
		g_assert_not_reached ();
	}

	return client;
}

static void
mateconf_editor_window_set_property (GObject      *object,
				  guint         param_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	MateConfEditorWindow *mateconfwindow = (MateConfEditorWindow *) object;

	switch (param_id) {
	case PROP_TYPE:
		/* Construct-only */
		mateconfwindow->type = g_value_get_int (value);
		mateconfwindow->client = mateconf_editor_window_get_client (mateconfwindow->type);
		switch (mateconfwindow->type) {
			case MATECONF_EDITOR_WINDOW_TYPE_DEFAULTS:
				gtk_window_set_title (GTK_WINDOW (mateconfwindow), _("Configuration Editor (Defaults settings)"));
				break;
			case MATECONF_EDITOR_WINDOW_TYPE_MANDATORY:
				gtk_window_set_title (GTK_WINDOW (mateconfwindow), _("Configuration Editor (Mandatory settings)"));
				break;
			default:
				gtk_window_set_title (GTK_WINDOW (mateconfwindow), _("Configuration Editor"));
			}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
mateconf_editor_window_get_property (GObject      *object,
				  guint         param_id,
				  GValue       *value,
				  GParamSpec   *pspec)
{

	MateConfEditorWindow *mateconfwindow = (MateConfEditorWindow *) object;

	switch (param_id) {
	case PROP_TYPE:
		g_value_set_int (value, mateconfwindow->type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
mateconf_editor_window_finalize (GObject *object)
{
	MateConfEditorWindow *window = MATECONF_EDITOR_WINDOW(object);
	MateConfClient       *client;

	client = mateconf_client_get_default ();

	g_object_unref (window->ui_manager);

	if (window->tearoffs_notify_id)
		mateconf_client_notify_remove (client, window->tearoffs_notify_id);

	if (window->icons_notify_id)
		mateconf_client_notify_remove (client, window->icons_notify_id);

	g_object_unref (client);
	g_object_unref (window->client);

	G_OBJECT_CLASS (mateconf_editor_window_parent_class)->finalize (object);
    
}

static void
mateconf_editor_window_class_init (MateConfEditorWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->set_property = mateconf_editor_window_set_property;
	object_class->get_property = mateconf_editor_window_get_property;
	object_class->finalize = mateconf_editor_window_finalize;

	g_object_class_install_property (object_class, PROP_TYPE,
					 g_param_spec_int ("type",
					 "Type",
					 "The Configuration Editor window type.",
					 MATECONF_EDITOR_WINDOW_TYPE_NORMAL, MATECONF_EDITOR_WINDOW_TYPE_MANDATORY,
					 MATECONF_EDITOR_WINDOW_TYPE_NORMAL, 
					 (G_PARAM_WRITABLE |
					 G_PARAM_CONSTRUCT_ONLY)));
}

static void
tree_view_style_set_cb (GtkWidget *widget,
                        GtkStyle *previous_style,
                        GObject *cell)
{
        GdkPixbuf *folder_open = NULL, *folder_closed = NULL;
        int w, h;

        if (gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
                                               GTK_ICON_SIZE_MENU,
                                               &w, &h)) {
                GtkIconTheme *theme;

                theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
                
                folder_open = gtk_icon_theme_load_icon (theme, "folder-open", w, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
                folder_closed = gtk_icon_theme_load_icon (theme, "folder", w, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
        }

        g_object_set (cell,
                      "pixbuf-expander-open", folder_open,
                      "pixbuf-expander-closed", folder_closed,
                      "pixbuf", folder_closed,
                      NULL);
        if (folder_open) {
                g_object_unref (folder_open);
        }
        if (folder_closed) {
                g_object_unref (folder_closed);
        }
}

static void
mateconf_editor_window_init (MateConfEditorWindow *window)
{
	GtkWidget *hpaned, *vpaned, *scrolled_window, *vbox;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkAccelGroup *accel_group;
	GtkActionGroup *action_group;
	GtkWidget *menubar;
	GtkWidget *details_frame, *alignment, *table, *label, *text_view, *image;
	gchar *markup;
	GError *error;
	
	gtk_window_set_default_size (GTK_WINDOW (window), 700, 550);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	action_group = gtk_action_group_new ("MateConfEditorMenuActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), window);
	
	window->ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_set_add_tearoffs (window->ui_manager, TRUE);
	gtk_ui_manager_insert_action_group (window->ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string (window->ui_manager, ui_description, -1, &error)) {
      		g_message ("Building menus failed: %s", error->message);
	      	g_error_free (error);
      		exit (EXIT_FAILURE);
    	}

	menubar = gtk_ui_manager_get_widget (window->ui_manager, "/MateConfEditorMenu");
	gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

	window->popup_menu = gtk_ui_manager_get_widget (window->ui_manager, "/MateConfKeyPopupMenu");

	/* Hook up bookmarks */
	mateconf_bookmarks_hook_up_menu (window,
				      gtk_menu_item_get_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (window->ui_manager,
				      						     "/MateConfEditorMenu/BookmarksMenu/"))),
				      gtk_ui_manager_get_widget (window->ui_manager, "/MateConfEditorMenu/BookmarksMenu/AddBookmark"),
				      gtk_ui_manager_get_widget (window->ui_manager, "/MateConfEditorMenu/BookmarksMenu/EditBookmarks"));

	/* Create content area */
	vpaned = gtk_vpaned_new ();
	gtk_paned_set_position (GTK_PANED (vpaned), 400);
	gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);

	hpaned = gtk_hpaned_new ();
	gtk_paned_set_position (GTK_PANED (hpaned), 250);

	gtk_paned_pack1 (GTK_PANED (vpaned), hpaned, FALSE, FALSE);

	/* Create ouput window */
	window->output_window = gedit_output_window_new ();
	window->output_window_type = MATECONF_EDITOR_WINDOW_OUTPUT_WINDOW_NONE;
	gedit_output_window_set_select_multiple (GEDIT_OUTPUT_WINDOW (window->output_window),
						 GTK_SELECTION_SINGLE);
	gtk_paned_pack2 (GTK_PANED (vpaned), window->output_window, FALSE, FALSE);
	g_signal_connect (G_OBJECT (window->output_window), "close_requested",
			  G_CALLBACK (mateconf_editor_window_close_output_window), window);

	g_signal_connect (G_OBJECT (window->output_window), "selection_changed",
			  G_CALLBACK (mateconf_editor_window_output_window_changed), window);

	mateconf_editor_window_recents_init ();
	
	/* Create status bar */
	window->statusbar = gtk_statusbar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), window->statusbar, FALSE, FALSE, 0);
	
	/* Create tree model and tree view */
	window->tree_model = mateconf_tree_model_new ();
	window->sorted_tree_model = gtk_tree_model_sort_new_with_model (window->tree_model);
	window->tree_view = gtk_tree_view_new_with_model (window->sorted_tree_model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (window->tree_view), FALSE);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->tree_view))), "changed",
			  G_CALLBACK (mateconf_editor_window_selection_changed), window);
	g_signal_connect (G_OBJECT (window->tree_view), "row-activated", 
			  G_CALLBACK (mateconf_editor_window_row_activated),
			  window);
	g_signal_connect (window->tree_view, "test-expand-row",
			  G_CALLBACK (mateconf_editor_window_test_expand_row),
			  window);
	g_signal_connect (window->tree_view, "row-expanded",
			  G_CALLBACK (mateconf_editor_window_row_expanded),
			  window);
	g_object_unref (G_OBJECT (window->tree_model));

	column = gtk_tree_view_column_new ();

	cell = gtk_cell_renderer_pixbuf_new ();
        g_signal_connect (window->tree_view, "style-set", G_CALLBACK (tree_view_style_set_cb), cell);
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_set_attributes (column, cell,
					     "text", MATECONF_TREE_MODEL_NAME_COLUMN,
					     NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (window->tree_view), column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (window->sorted_tree_model), 0, GTK_SORT_ASCENDING);
	
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled_window), window->tree_view);
	gtk_paned_add1 (GTK_PANED (hpaned), scrolled_window);

	/* Create list model and list view */
	window->list_model = mateconf_list_model_new ();
	window->sorted_list_model = gtk_tree_model_sort_new_with_model (window->list_model);
	window->list_view = gtk_tree_view_new_with_model (window->sorted_list_model);
	g_object_unref (G_OBJECT (window->list_model));
	g_object_unref (G_OBJECT (window->sorted_list_model));

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (window->sorted_list_model),
					      MATECONF_LIST_MODEL_KEY_NAME_COLUMN, GTK_SORT_ASCENDING);

	g_signal_connect (window->list_view, "popup_menu",
			  G_CALLBACK (mateconf_editor_window_list_view_popup_menu), window);
	g_signal_connect (window->list_view, "row_activated",
			  G_CALLBACK (mateconf_editor_window_list_view_row_activated), window);

	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->list_view)), "changed",
			  G_CALLBACK (mateconf_editor_window_update_list_selection), window);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_reorderable (column, TRUE);
	gtk_tree_view_column_set_title (column, _("Name"));

	cell = gtk_cell_renderer_pixbuf_new ();
        g_object_set (cell, "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_set_attributes (column, cell,
					     "icon-name", MATECONF_LIST_MODEL_ICON_NAME_COLUMN,
					     NULL);

	gtk_tree_view_column_set_resizable (column, TRUE);
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_set_attributes (column, cell,
					     "text", MATECONF_LIST_MODEL_KEY_NAME_COLUMN,
					     NULL);
	gtk_tree_view_column_set_sort_column_id (column, MATECONF_LIST_MODEL_KEY_NAME_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (window->list_view), column);

	cell = mateconf_cell_renderer_new ();
	g_signal_connect (cell, "changed",
			  G_CALLBACK (mateconf_editor_mateconf_value_changed), window);
	g_signal_connect (cell, "check_writable",
			  G_CALLBACK (mateconf_editor_check_writable), window);
	
	window->value_column = column = gtk_tree_view_column_new_with_attributes (_("Value"),
										  cell,
										  "value", MATECONF_LIST_MODEL_VALUE_COLUMN,
										  NULL);
	g_signal_connect (window->list_view, "button_press_event",
			  G_CALLBACK (list_view_button_press_event), window);

	gtk_tree_view_column_set_reorderable (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);

	gtk_tree_view_append_column (GTK_TREE_VIEW (window->list_view), column);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled_window), window->list_view);

	vpaned = gtk_vpaned_new ();
	gtk_paned_add2 (GTK_PANED (hpaned), vpaned);

	gtk_paned_pack1 (GTK_PANED (vpaned), scrolled_window, TRUE, FALSE);

	/* Create details area */
	label = gtk_label_new (NULL);
	markup = g_strdup_printf ("<span weight=\"bold\">%s</span>", _("Key Documentation"));
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);

	details_frame = gtk_frame_new (NULL);
	gtk_frame_set_label_widget (GTK_FRAME (details_frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (details_frame), GTK_SHADOW_NONE);
	gtk_paned_pack2 (GTK_PANED (vpaned), details_frame, FALSE, FALSE);

	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 0, 24, 0);
	gtk_container_add (GTK_CONTAINER (details_frame), alignment);

	table = gtk_table_new (6, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	
	gtk_container_add (GTK_CONTAINER (alignment), table);


	window->non_writable_label = gtk_hbox_new (FALSE, 6);
	gtk_table_attach (GTK_TABLE (table), window->non_writable_label,
			  0, 2, 0, 1,
			  GTK_FILL, 0, 0, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (window->non_writable_label), image, FALSE, FALSE, 0);

	label = gtk_label_new (_("This key is not writable"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (window->non_writable_label), label, FALSE, FALSE, 0);
	

	window->no_schema_label = gtk_hbox_new (FALSE, 6);
	gtk_table_attach (GTK_TABLE (table), window->no_schema_label,
			  0, 2, 1, 2,
			  GTK_FILL, 0, 0, 0);
	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (window->no_schema_label), image, FALSE, FALSE, 0);

	label = gtk_label_new (_("This key has no schema"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (window->no_schema_label), label, FALSE, FALSE, 0);


	label = gtk_label_new (_("Key name:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 2, 3,
			  GTK_FILL, 0, 0, 0);
	window->key_name_label = gtk_label_new (_("(None)"));
	gtk_label_set_selectable (GTK_LABEL (window->key_name_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (window->key_name_label), 0.0, 0.5);	
	gtk_table_attach (GTK_TABLE (table), window->key_name_label,
			  1, 2, 2, 3,
			  GTK_FILL, 0, 0, 0);
	
	label = gtk_label_new (_("Key owner:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);	
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 3, 4,
			  GTK_FILL, 0, 0, 0);
	window->owner_label= gtk_label_new (_("(None)"));
	gtk_label_set_selectable (GTK_LABEL (window->owner_label), TRUE);	
	gtk_misc_set_alignment (GTK_MISC (window->owner_label), 0.0, 0.5);		
	gtk_table_attach (GTK_TABLE (table), window->owner_label,
			  1, 2, 3, 4,
			  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Short description:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);	
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 4, 5,
			  GTK_FILL, 0, 0, 0);
	window->short_desc_label= gtk_label_new (_("(None)"));
	gtk_label_set_line_wrap (GTK_LABEL (window->short_desc_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (window->short_desc_label), TRUE);	
	gtk_misc_set_alignment (GTK_MISC (window->short_desc_label), 0.0, 0.5);		
	gtk_table_attach (GTK_TABLE (table), window->short_desc_label,
			  1, 2, 4, 5,
			  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Long description:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);	
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 5, 6,
			  GTK_FILL, GTK_FILL, 0, 0);

	window->long_desc_buffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_set_text (window->long_desc_buffer, _("(None)"), -1);	
	text_view = gtk_text_view_new_with_buffer (window->long_desc_buffer);
	g_signal_connect (window, "style_set",
			  G_CALLBACK (mateconf_editor_window_style_set),
			  text_view);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
	
	gtk_table_attach (GTK_TABLE (table), scrolled_window, 
			  1, 2, 5, 6,
			  GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

	gtk_widget_show_all (vbox);
	gtk_widget_hide (window->non_writable_label);
	gtk_widget_hide (window->no_schema_label);
	gtk_widget_hide (window->output_window);
	
	gtk_window_set_default_icon_name ("mateconf-editor");

	mateconf_editor_window_setup_ui_prefs_handler (window);
}

