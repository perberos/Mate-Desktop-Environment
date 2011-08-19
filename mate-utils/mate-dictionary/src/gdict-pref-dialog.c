/* gdict-pref-dialog.c - preferences dialog
 *
 * This file is part of MATE Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>

#include "gdict-source-dialog.h"
#include "gdict-pref-dialog.h"
#include "gdict-common.h"

#define GDICT_PREFERENCES_UI 	PKGDATADIR "/mate-dictionary-preferences.ui"

#define DEFAULT_MIN_WIDTH 	220
#define DEFAULT_MIN_HEIGHT 	330

/*******************
 * GdictPrefDialog *
 *******************/

static GtkWidget *global_dialog = NULL;

enum
{
  SOURCES_ACTIVE_COLUMN = 0,
  SOURCES_NAME_COLUMN,
  SOURCES_DESCRIPTION_COLUMN,
  
  SOURCES_N_COLUMNS
};

struct _GdictPrefDialog
{
  GtkDialog parent_instance;

  GtkBuilder *builder;

  MateConfClient *mateconf_client;
  guint notify_id;
  
  gchar *active_source;
  GdictSourceLoader *loader;
  GtkListStore *sources_list;
  
  /* direct pointers to widgets */
  GtkWidget *notebook;
  
  GtkWidget *sources_view;
  GtkWidget *sources_add;
  GtkWidget *sources_remove;
  
  gchar *print_font;
  GtkWidget *font_button;
  
  GtkWidget *help_button;
  GtkWidget *close_button;
};

struct _GdictPrefDialogClass
{
  GtkDialogClass parent_class;
};

enum
{
  PROP_0,

  PROP_SOURCE_LOADER
};


G_DEFINE_TYPE (GdictPrefDialog, gdict_pref_dialog, GTK_TYPE_DIALOG);


static gboolean
select_active_source_name (GtkTreeModel *model,
			   GtkTreePath  *path,
			   GtkTreeIter  *iter,
			   gpointer      data)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (data);
  gboolean is_active;
  
  gtk_tree_model_get (model, iter,
      		      SOURCES_ACTIVE_COLUMN, &is_active,
      		      -1);
  if (is_active)
    {
      GtkTreeSelection *selection;
      
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->sources_view));
      
      gtk_tree_selection_select_iter (selection, iter);
      
      return TRUE;
    }
  
  return FALSE;
}

static void
update_sources_view (GdictPrefDialog *dialog)
{
  const GSList *sources, *l;
  
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view), NULL);
  
  gtk_list_store_clear (dialog->sources_list);
  
  /* force update of the sources list */
  gdict_source_loader_update (dialog->loader);
  
  sources = gdict_source_loader_get_sources (dialog->loader);
  for (l = sources; l != NULL; l = l->next)
    {
      GdictSource *source = GDICT_SOURCE (l->data);
      GtkTreeIter iter;
      const gchar *name, *description;
      gboolean is_active = FALSE;
      
      name = gdict_source_get_name (source);
      description = gdict_source_get_description (source);
      if (!description)
	description = name;

      if (strcmp (name, dialog->active_source) == 0)
        is_active = TRUE;

      gtk_list_store_append (dialog->sources_list, &iter);
      gtk_list_store_set (dialog->sources_list, &iter,
      			  SOURCES_ACTIVE_COLUMN, is_active,
      			  SOURCES_NAME_COLUMN, name,
      			  SOURCES_DESCRIPTION_COLUMN, description,
      			  -1);
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view),
  			   GTK_TREE_MODEL (dialog->sources_list));
  
  /* select the currently active source name */
  gtk_tree_model_foreach (GTK_TREE_MODEL (dialog->sources_list),
  			  select_active_source_name,
  			  dialog);
}

static void
source_renderer_toggled_cb (GtkCellRendererToggle *renderer,
			    const gchar           *path,
			    GdictPrefDialog       *dialog)
{
  GtkTreePath *treepath;
  GtkTreeIter iter;
  gboolean res;
  gboolean is_active;
  gchar *name;
  
  treepath = gtk_tree_path_new_from_string (path);
  res = gtk_tree_model_get_iter (GTK_TREE_MODEL (dialog->sources_list),
                                 &iter,
                                 treepath);
  if (!res)
    {
      gtk_tree_path_free (treepath);
      
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (dialog->sources_list), &iter,
      		      SOURCES_NAME_COLUMN, &name,
      		      SOURCES_ACTIVE_COLUMN, &is_active,
      		      -1);
  if (!is_active && name != NULL)
    {
      g_free (dialog->active_source);
      dialog->active_source = g_strdup (name);
      
      mateconf_client_set_string (dialog->mateconf_client,
      			       GDICT_MATECONF_SOURCE_KEY,
      			       dialog->active_source,
      			       NULL);
      
      update_sources_view (dialog);
      
      g_free (name);
    }
  
  gtk_tree_path_free (treepath);
}

static void
sources_view_row_activated_cb (GtkTreeView       *tree_view,
			       GtkTreePath       *tree_path,
			       GtkTreeViewColumn *tree_iter,
			       GdictPrefDialog   *dialog)
{
  GtkWidget *edit_dialog;
  gchar *source_name;
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (tree_view);
  if (!model)
    return;
  
  if (!gtk_tree_model_get_iter (model, &iter, tree_path))
    return;
  
  gtk_tree_model_get (model, &iter, SOURCES_NAME_COLUMN, &source_name, -1);
  if (!source_name)
    return;
  
  edit_dialog = gdict_source_dialog_new (GTK_WINDOW (dialog),
  					 _("Edit Dictionary Source"),
  					 GDICT_SOURCE_DIALOG_EDIT,
  					 dialog->loader,
  					 source_name);
  gtk_dialog_run (GTK_DIALOG (edit_dialog));

  gtk_widget_destroy (edit_dialog);
  g_free (source_name);

  update_sources_view (dialog);
}

static void
build_sources_view (GdictPrefDialog *dialog)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  
  if (dialog->sources_list)
    return;
    
  dialog->sources_list = gtk_list_store_new (SOURCES_N_COLUMNS,
  					     G_TYPE_BOOLEAN,  /* active */
  					     G_TYPE_STRING,   /* name */
  					     G_TYPE_STRING    /* description */);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dialog->sources_list),
  					SOURCES_DESCRIPTION_COLUMN,
  					GTK_SORT_ASCENDING);
  
  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
  g_signal_connect (renderer, "toggled",
  		    G_CALLBACK (source_renderer_toggled_cb),
  		    dialog);
  
  column = gtk_tree_view_column_new_with_attributes ("active",
  						     renderer,
  						     "active", SOURCES_ACTIVE_COLUMN,
  						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->sources_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("description",
  						     renderer,
  						     "text", SOURCES_DESCRIPTION_COLUMN,
  						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->sources_view), column);
  
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->sources_view), FALSE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->sources_view),
  			   GTK_TREE_MODEL (dialog->sources_list));

  g_signal_connect (dialog->sources_view, "row-activated",
		    G_CALLBACK (sources_view_row_activated_cb),
		    dialog);
}

static void
source_add_clicked_cb (GtkWidget       *widget,
		       GdictPrefDialog *dialog)
{
  GtkWidget *add_dialog;
  
  add_dialog = gdict_source_dialog_new (GTK_WINDOW (dialog),
  					_("Add Dictionary Source"),
  					GDICT_SOURCE_DIALOG_CREATE,
  					dialog->loader,
  					NULL);

  gtk_dialog_run (GTK_DIALOG (add_dialog));

  gtk_widget_destroy (add_dialog);

  update_sources_view (dialog);
}

static void
source_remove_clicked_cb (GtkWidget       *widget,
			  GdictPrefDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean is_selected;
  gchar *name, *description;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->sources_view));
  if (!selection)
    return;
  
  is_selected = gtk_tree_selection_get_selected (selection, &model, &iter);
  if (!is_selected)
    return;
    
  gtk_tree_model_get (model, &iter,
  		      SOURCES_NAME_COLUMN, &name,
  		      SOURCES_DESCRIPTION_COLUMN, &description,
  		      -1);
  if (!name) 
    return;
  else
    {
      GtkWidget *confirm_dialog;
      gint response;
      
      confirm_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
      					       GTK_DIALOG_DESTROY_WITH_PARENT,
      					       GTK_MESSAGE_WARNING,
      					       GTK_BUTTONS_NONE,
      					       _("Remove \"%s\"?"), description);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (confirm_dialog),
      						_("This will permanently remove the "
      						  "dictionary source from the list."));
      
      gtk_dialog_add_button (GTK_DIALOG (confirm_dialog),
      			     GTK_STOCK_CANCEL,
      			     GTK_RESPONSE_CANCEL);
      gtk_dialog_add_button (GTK_DIALOG (confirm_dialog),
      			     GTK_STOCK_REMOVE,
      			     GTK_RESPONSE_OK);
      
      gtk_window_set_title (GTK_WINDOW (confirm_dialog), "");
      
      response = gtk_dialog_run (GTK_DIALOG (confirm_dialog));
      if (response == GTK_RESPONSE_CANCEL)
        {
          gtk_widget_destroy (confirm_dialog);
          
          goto out;
        }
      
      gtk_widget_destroy (confirm_dialog);
    }
  
  if (gdict_source_loader_remove_source (dialog->loader, name))
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  else
    {
      GtkWidget *error_dialog;
      gchar *message;
      
      message = g_strdup_printf (_("Unable to remove source '%s'"),
      				 description);
      
      error_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
      					     GTK_DIALOG_DESTROY_WITH_PARENT,
      					     GTK_MESSAGE_ERROR,
      					     GTK_BUTTONS_OK,
      					     "%s", message);
      gtk_window_set_title (GTK_WINDOW (error_dialog), "");
      
      gtk_dialog_run (GTK_DIALOG (error_dialog));
      
      gtk_widget_destroy (error_dialog);
    }

out:
  g_free (name);
  g_free (description);
  
  update_sources_view (dialog);
}

static void
set_source_loader (GdictPrefDialog   *dialog,
		   GdictSourceLoader *loader)
{
  if (!dialog->sources_list)
    return;
  
  if (dialog->loader)
    g_object_unref (dialog->loader);
  
  dialog->loader = g_object_ref (loader);
  
  update_sources_view (dialog);
}

static void
font_button_font_set_cb (GtkWidget       *font_button,
			 GdictPrefDialog *dialog)
{
  const gchar *font;
  
  font = gtk_font_button_get_font_name (GTK_FONT_BUTTON (font_button));
  if (!font || font[0] == '\0')
    return;

  if (dialog->print_font && (strcmp (dialog->print_font, font) == 0))
    return;
  
  g_free (dialog->print_font);
  dialog->print_font = g_strdup (font);
      
  mateconf_client_set_string (dialog->mateconf_client,
  			   GDICT_MATECONF_PRINT_FONT_KEY,
  			   dialog->print_font,
  			   NULL);
}

static void
gdict_pref_dialog_mateconf_notify_cb (MateConfClient *client,
				   guint        cnxn_id,
				   MateConfEntry  *entry,
				   gpointer     user_data)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (user_data);
  
  if (strcmp (entry->key, GDICT_MATECONF_SOURCE_KEY) == 0)
    {
      if (entry->value && entry->value->type == MATECONF_VALUE_STRING)
        {
          g_free (dialog->active_source);
          dialog->active_source = g_strdup (mateconf_value_get_string (entry->value));
        }
      else
        {
          g_free (dialog->active_source);
          dialog->active_source = g_strdup (GDICT_DEFAULT_SOURCE_NAME);
        }
      
      update_sources_view (dialog);
    }
  else if (strcmp (entry->key, GDICT_MATECONF_PRINT_FONT_KEY) == 0)
    {
      if (entry->value && entry->value->type == MATECONF_VALUE_STRING)
        {
          g_free (dialog->print_font);
          dialog->print_font = g_strdup (mateconf_value_get_string (entry->value));
        }
      else
        {
          g_free (dialog->print_font);
          dialog->print_font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
        }

      gtk_font_button_set_font_name (GTK_FONT_BUTTON (dialog->font_button),
		      		     dialog->print_font);
    }
}

static void
response_cb (GtkDialog *dialog,
	     gint       response_id,
	     gpointer   user_data)
{
  GError *err = NULL;
  
  switch (response_id)
    {
    case GTK_RESPONSE_HELP:
      gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
                    "ghelp:mate-dictionary#mate-dictionary-preferences",
                    gtk_get_current_event_time (), &err);
      if (err)
	{
          GtkWidget *error_dialog;
	  gchar *message;

	  message = g_strdup_printf (_("There was an error while displaying help"));
	  error_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
      					         GTK_DIALOG_DESTROY_WITH_PARENT,
      					         GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 "%s", message);
	  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog),
			  			    "%s", err->message);
	  gtk_window_set_title (GTK_WINDOW (error_dialog), "");
	  
	  gtk_dialog_run (GTK_DIALOG (error_dialog));
      
          gtk_widget_destroy (error_dialog);
	  g_error_free (err);
        }
      
      /* we don't want the dialog to close itself */
      g_signal_stop_emission_by_name (dialog, "response");
      break;
    case GTK_RESPONSE_ACCEPT:
    default:
      gtk_widget_hide (GTK_WIDGET (dialog));
      break;
    }
}

static void
gdict_pref_dialog_finalize (GObject *object)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  if (dialog->notify_id);
    mateconf_client_notify_remove (dialog->mateconf_client, dialog->notify_id);
  
  if (dialog->mateconf_client)
    g_object_unref (dialog->mateconf_client);
  
  if (dialog->builder)
    g_object_unref (dialog->builder);

  if (dialog->active_source)
    g_free (dialog->active_source);
  
  if (dialog->loader)
    g_object_unref (dialog->loader);
  
  G_OBJECT_CLASS (gdict_pref_dialog_parent_class)->finalize (object);
}

static void
gdict_pref_dialog_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      set_source_loader (dialog, g_value_get_object (value));
      break;
    default:
      break;
    }
}

static void
gdict_pref_dialog_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
  GdictPrefDialog *dialog = GDICT_PREF_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      g_value_set_object (value, dialog->loader);
      break;
    default:
      break;
    }
}

static void
gdict_pref_dialog_class_init (GdictPrefDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->set_property = gdict_pref_dialog_set_property;
  gobject_class->get_property = gdict_pref_dialog_get_property;
  gobject_class->finalize = gdict_pref_dialog_finalize;
  
  g_object_class_install_property (gobject_class,
  				   PROP_SOURCE_LOADER,
  				   g_param_spec_object ("source-loader",
  				   			"Source Loader",
  				   			"The GdictSourceLoader used by the application",
  				   			GDICT_TYPE_SOURCE_LOADER,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
}

static void
gdict_pref_dialog_init (GdictPrefDialog *dialog)
{
  gchar *font;
  GError *error = NULL;

  gtk_window_set_default_size (GTK_WINDOW (dialog),
  			       DEFAULT_MIN_WIDTH,
  			       DEFAULT_MIN_HEIGHT);
    
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  
  /* add buttons */
  gtk_dialog_add_button (GTK_DIALOG (dialog),
  			 "gtk-help",
  			 GTK_RESPONSE_HELP);
  gtk_dialog_add_button (GTK_DIALOG (dialog),
  			 "gtk-close",
  			 GTK_RESPONSE_ACCEPT);

  dialog->mateconf_client = mateconf_client_get_default ();
  mateconf_client_add_dir (dialog->mateconf_client,
  			GDICT_MATECONF_DIR,
  			MATECONF_CLIENT_PRELOAD_ONELEVEL,
  			NULL);
  dialog->notify_id = mateconf_client_notify_add (dialog->mateconf_client,
  					       GDICT_MATECONF_DIR,
		  			       gdict_pref_dialog_mateconf_notify_cb,
  					       dialog,
  					       NULL,
  					       NULL);

  /* get the UI from the GtkBuilder file */
  dialog->builder = gtk_builder_new ();
  gtk_builder_add_from_file (dialog->builder, GDICT_PREFERENCES_UI, &error);

  if (error) {
    g_critical ("Unable to load the preferences user interface: %s", error->message);
    g_error_free (error);
    g_assert_not_reached ();
  }

  /* the main widget */
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                     GTK_WIDGET (gtk_builder_get_object (dialog->builder, "preferences_root")));

  /* keep all the interesting widgets around */  
  dialog->notebook = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "preferences_notebook"));
  
  dialog->sources_view = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "sources_treeview"));
  build_sources_view (dialog);

  dialog->active_source = gdict_mateconf_get_string_with_default (dialog->mateconf_client,
							       GDICT_MATECONF_SOURCE_KEY,
							       GDICT_DEFAULT_SOURCE_NAME);

  dialog->sources_add = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "add_button"));
  gtk_widget_set_tooltip_text (dialog->sources_add,
                               _("Add a new dictionary source"));
  g_signal_connect (dialog->sources_add, "clicked",
  		    G_CALLBACK (source_add_clicked_cb), dialog);
  		    
  dialog->sources_remove = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "remove_button"));
  gtk_widget_set_tooltip_text (dialog->sources_remove,
                               _("Remove the currently selected dictionary source"));
  g_signal_connect (dialog->sources_remove, "clicked",
  		    G_CALLBACK (source_remove_clicked_cb), dialog);
  
  font = mateconf_client_get_string (dialog->mateconf_client,
  				  GDICT_MATECONF_PRINT_FONT_KEY,
  				  NULL);
  if (!font)
    font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
  
  dialog->font_button = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "print_font_button"));
  gtk_font_button_set_font_name (GTK_FONT_BUTTON (dialog->font_button), font);
  gtk_widget_set_tooltip_text (dialog->font_button,
                               _("Set the font used for printing the definitions"));
  g_signal_connect (dialog->font_button, "font-set",
  		    G_CALLBACK (font_button_font_set_cb), dialog);
  g_free (font);
  
  gtk_widget_show_all (dialog->notebook);

  /* we want to intercept the response signal before any other
   * callbacks might be attached by the users of the
   * GdictPrefDialog widget.
   */
  g_signal_connect (dialog, "response",
		    G_CALLBACK (response_cb),
		    NULL);
}

void
gdict_show_pref_dialog (GtkWidget         *parent,
			const gchar       *title,
			GdictSourceLoader *loader)
{
  GtkWidget *dialog;
  
  g_return_if_fail (GTK_IS_WIDGET (parent));
  g_return_if_fail (GDICT_IS_SOURCE_LOADER (loader));
  
  if (parent)
    dialog = g_object_get_data (G_OBJECT (parent), "gdict-pref-dialog");
  else
    dialog = global_dialog;
  
  if (!dialog)
    {
      dialog = g_object_new (GDICT_TYPE_PREF_DIALOG,
                             "source-loader", loader,
                             "title", title,
                             NULL);
      
      g_object_ref_sink (dialog);
      
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_widget_hide_on_delete),
                        NULL);
      
      if (parent && GTK_IS_WINDOW (parent))
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
          gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
          g_object_set_data_full (G_OBJECT (parent), "gdict-pref-dialog",
                                  dialog,
                                  g_object_unref);
        }
      else
        global_dialog = dialog;
    }

  gtk_window_set_screen (GTK_WINDOW (dialog),
 			 gtk_widget_get_screen (parent));
  gtk_window_present (GTK_WINDOW (dialog));  
}
