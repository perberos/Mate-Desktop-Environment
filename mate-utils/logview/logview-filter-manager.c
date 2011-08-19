/*-*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * mate-utils
 * Copyright (C) Johannes Schmid 2009 <jhs@mate.org>
 *
 * mate-utils is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mate-utils is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "logview-filter-manager.h"
#include "logview-prefs.h"
#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>

#define UI_FILE LOGVIEW_DATADIR "/logview-filter.ui"

struct _LogviewFilterManagerPrivate {
  GtkWidget *tree;

  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkWidget *edit_button;

  GtkTreeModel *model;
  GtkBuilder* builder;

  LogviewPrefs* prefs;
};

enum {
  COLUMN_NAME = 0,
  COLUMN_FILTER,
  N_COLUMNS
};

#define LOGVIEW_FILTER_MANAGER_GET_PRIVATE(o)  \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_FILTER_MANAGER, LogviewFilterManagerPrivate))

G_DEFINE_TYPE (LogviewFilterManager, logview_filter_manager, GTK_TYPE_DIALOG);

static void
logview_filter_manager_update_model (LogviewFilterManager *manager)
{
  GList *filters;
  GList *filter;
  gchar *name;
  GtkTreeIter iter;

  gtk_list_store_clear (GTK_LIST_STORE (manager->priv->model));

  filters = logview_prefs_get_filters (manager->priv->prefs);  

  for (filter = filters; filter != NULL; filter = g_list_next (filter)) {
    g_object_get (filter->data, "name", &name, NULL);

    gtk_list_store_append (GTK_LIST_STORE(manager->priv->model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (manager->priv->model),
                        &iter,
                        COLUMN_NAME, name,
                        COLUMN_FILTER, filter->data,
                        -1);

    g_free (name);
  }

  g_list_free (filters);  
}

static gboolean
check_name (LogviewFilterManager *manager, const gchar *name)
{
  GtkWidget *dialog;

  if (!strlen (name)) {
    dialog = gtk_message_dialog_new (GTK_WINDOW (manager),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", _("Filter name is empty!"));
    gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);

    return FALSE;
  }

  if (strstr (name, ":") != NULL) {
    dialog = gtk_message_dialog_new (GTK_WINDOW(manager),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", _("Filter name may not contain the ':' character"));
    gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);

    return FALSE;
  }

  return TRUE;
}

static gboolean
check_regex (LogviewFilterManager *manager, const gchar *regex)
{
  GtkWidget *dialog;
  GError *error = NULL;
  GRegex *reg;

  if (!strlen (regex)) {
    dialog = gtk_message_dialog_new (GTK_WINDOW(manager),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", _("Regular expression is empty!"));

    gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);

    return FALSE;
  }

  reg = g_regex_new (regex,
                     0, 0, &error);
  if (error) {
    dialog = gtk_message_dialog_new (GTK_WINDOW (manager),
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Regular expression is invalid: %s"),
                                     error->message);
    gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);
    g_error_free (error);

    return FALSE;
  }

  g_regex_unref (reg);

  return TRUE;
}

static void
on_check_toggled (GtkToggleButton *button, GtkWidget *widget)
{
  gtk_widget_set_sensitive (widget, 
                            gtk_toggle_button_get_active (button));
}

static void
on_dialog_add_edit_reponse (GtkWidget *dialog, int response_id,
                            LogviewFilterManager *manager)
{
  GtkWidget *entry_name, *entry_regex;
  GtkWidget *radio_color, *radio_visible;
  GtkWidget *check_foreground, *check_background;
  GtkWidget *color_foreground, *color_background;
  gchar *old_name;
  const gchar *name;
  const gchar *regex;
  LogviewFilter *filter;
  GtkTextTag *tag;
  GtkBuilder *builder;

  old_name = g_object_get_data (G_OBJECT (manager), "old_name");
  builder = manager->priv->builder;

  entry_name = GTK_WIDGET (gtk_builder_get_object (builder,
                                                   "entry_name"));
  entry_regex = GTK_WIDGET (gtk_builder_get_object (builder,
                                                    "entry_regex"));
  radio_color = GTK_WIDGET (gtk_builder_get_object (builder,
                                                    "radio_color"));
  radio_visible = GTK_WIDGET (gtk_builder_get_object (builder,
                                                      "radio_visible"));
  check_foreground = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "check_foreground"));
  check_background = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "check_background"));
  color_foreground = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "color_foreground"));
  color_background = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "color_background"));

  if (response_id == GTK_RESPONSE_APPLY) {
    name = gtk_entry_get_text (GTK_ENTRY (entry_name));
    regex = gtk_entry_get_text (GTK_ENTRY (entry_regex));

    if (!check_name (manager, name) || !check_regex (manager, regex)) {
      return;
    }

    filter = logview_filter_new (name, regex);
    tag = gtk_text_tag_new (name);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_color))) {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_foreground))) {
        GdkColor foreground_color;
        gtk_color_button_get_color (GTK_COLOR_BUTTON (color_foreground),
                                    &foreground_color);
        g_object_set (G_OBJECT (tag), 
                      "foreground-gdk", &foreground_color,
                      "foreground-set", TRUE, NULL);
      }

      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_background))) {
        GdkColor background_color;
        gtk_color_button_get_color (GTK_COLOR_BUTTON (color_background),
                                    &background_color);
        g_object_set (tag, 
                      "paragraph-background-gdk", &background_color,
                      "paragraph-background-set", TRUE, NULL);
      }
      
      if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_foreground))
          && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_background))) {
          GtkWidget *error_dialog;

          error_dialog = gtk_message_dialog_new (GTK_WINDOW (manager),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_CLOSE,
                                                 "%s",
                                                 _("Please specify either foreground or background color!"));
          gtk_dialog_run (GTK_DIALOG (error_dialog));
          gtk_widget_destroy (error_dialog);
          g_object_unref (tag);
          g_object_unref (filter);

          return;
      }
    } else { /* !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_color)) */
      g_object_set (tag, "invisible", TRUE, NULL);
    }

    if (old_name && !g_str_equal (old_name, name)) {
      logview_prefs_remove_filter (manager->priv->prefs, old_name);
    }

    g_object_set (G_OBJECT (filter), "texttag", tag, NULL);
    g_object_unref (tag);

    logview_prefs_add_filter (manager->priv->prefs, filter);
    g_object_unref (filter);

    logview_filter_manager_update_model (manager);
  }

  gtk_widget_destroy (dialog);
}

static void
run_add_edit_dialog (LogviewFilterManager *manager, LogviewFilter *filter)
{
  GError *error;
  gchar *name, *regex;
  const gchar *title;
  GtkWidget *dialog, *entry_name, *entry_regex, *radio_color;
  GtkWidget *radio_visible, *check_foreground, *check_background;
  GtkWidget *color_foreground, *color_background, *vbox_color;
  gboolean foreground_set, background_set, invisible;
  GtkTextTag *tag;
  GtkBuilder* builder;

  builder = manager->priv->builder;

  error = NULL;
  name = NULL;
  
  gtk_builder_add_from_file (builder, UI_FILE, &error);

  if (error) {
    g_warning ("Could not load filter ui: %s", error->message);
    g_error_free (error);
    return;
  }

  title = (filter != NULL ? _("Edit filter") : _("Add new filter"));

  dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                               "dialog_filter"));

  entry_name = GTK_WIDGET (gtk_builder_get_object (builder,
                                                   "entry_name"));
  entry_regex = GTK_WIDGET (gtk_builder_get_object (builder,
                                                    "entry_regex"));
  radio_color = GTK_WIDGET (gtk_builder_get_object (builder,
                                                    "radio_color"));
  radio_visible = GTK_WIDGET (gtk_builder_get_object (builder,
                                                      "radio_visible"));

  gtk_radio_button_set_group (GTK_RADIO_BUTTON (radio_color),
                              gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio_visible)));

  check_foreground = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "check_foreground"));
  check_background = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "check_background"));
  color_foreground = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "color_foreground"));
  color_background = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "color_background"));
  g_signal_connect (check_foreground, "toggled", G_CALLBACK (on_check_toggled),
                    color_foreground);
  g_signal_connect (check_background, "toggled", G_CALLBACK (on_check_toggled),
                    color_background);

  on_check_toggled (GTK_TOGGLE_BUTTON (check_foreground),
                    color_foreground);
  on_check_toggled (GTK_TOGGLE_BUTTON (check_background),
                    color_background);

  vbox_color = GTK_WIDGET (gtk_builder_get_object (builder, "vbox_color"));
  g_signal_connect (radio_color, "toggled", G_CALLBACK (on_check_toggled),
                    vbox_color);
  on_check_toggled (GTK_TOGGLE_BUTTON (radio_color),
                    vbox_color);

  if (filter) {
    g_object_get (filter,
                  "name", &name,
                  "regex", &regex,
                  "texttag", &tag,
                  NULL);
    g_object_get (tag,
                  "foreground-set", &foreground_set,
                  "paragraph-background-set", &background_set,
                  "invisible", &invisible, NULL);
    gtk_entry_set_text (GTK_ENTRY(entry_name), name);
    gtk_entry_set_text (GTK_ENTRY(entry_regex), regex);

    if (foreground_set) {
      GdkColor *foreground;

      g_object_get (tag, "foreground-gdk", &foreground, NULL);
      gtk_color_button_set_color (GTK_COLOR_BUTTON (color_foreground),
                                  foreground);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_foreground),
                                    TRUE);

      gdk_color_free (foreground);
    }

    if (background_set) {
      GdkColor *background;

      g_object_get (tag, "paragraph-background-gdk", &background, NULL);
      gtk_color_button_set_color (GTK_COLOR_BUTTON (color_background),
                                  background);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_background),
                                    TRUE);

      gdk_color_free (background);
    }

    if (background_set || foreground_set) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_color), TRUE);
    } else if (invisible) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_visible), TRUE);
    }

    g_free (regex);
    g_object_unref (tag);
  }

  g_object_set_data_full (G_OBJECT (manager), "old_name", name, g_free);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (on_dialog_add_edit_reponse), manager);
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                GTK_WINDOW (manager));
  gtk_window_set_modal (GTK_WINDOW (dialog),
                        TRUE);

  gtk_widget_show (GTK_WIDGET (dialog));
}

static void
on_add_clicked (GtkWidget *button, LogviewFilterManager *manager)
{                                                   
  run_add_edit_dialog (manager, NULL);
}

static void
on_edit_clicked (GtkWidget *button, LogviewFilterManager *manager)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  LogviewFilter *filter;
  GtkTreeSelection *selection;

  selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (manager->priv->tree));

  gtk_tree_selection_get_selected (selection, &model, &iter);
  gtk_tree_model_get (model, &iter, COLUMN_FILTER, &filter, -1);

  run_add_edit_dialog (manager, filter);

  g_object_unref (filter);
}

static void
on_remove_clicked (GtkWidget *button, LogviewFilterManager *manager)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *name;

  selection  = 
    gtk_tree_view_get_selection (GTK_TREE_VIEW (manager->priv->tree));

  gtk_tree_selection_get_selected (selection, &model, &iter);
  gtk_tree_model_get (model, &iter, COLUMN_NAME, &name, -1);

  logview_prefs_remove_filter (manager->priv->prefs, name);
  logview_filter_manager_update_model (manager);

  g_free(name);
}

static void
on_tree_selection_changed (GtkTreeSelection *selection, LogviewFilterManager *manager)
{
  gboolean status;

  status = gtk_tree_selection_get_selected (selection, NULL, NULL);

  gtk_widget_set_sensitive (manager->priv->edit_button, status);
  gtk_widget_set_sensitive (manager->priv->remove_button, status);	
}

static void
logview_filter_manager_init (LogviewFilterManager *manager)
{
  GtkWidget *table;
  GtkWidget *scrolled_window;
  GtkTreeViewColumn *column;
  GtkCellRenderer *text_renderer;
  LogviewFilterManagerPrivate *priv;

  manager->priv = LOGVIEW_FILTER_MANAGER_GET_PRIVATE (manager);
  priv = manager->priv;

  priv->builder = gtk_builder_new ();
  g_object_ref (priv->builder);
  priv->prefs = logview_prefs_get ();

  gtk_dialog_add_button (GTK_DIALOG(manager),
                         GTK_STOCK_CLOSE,
                         GTK_RESPONSE_CLOSE);
  gtk_window_set_modal (GTK_WINDOW (manager),
                        TRUE);

  priv->model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS,
                                                    G_TYPE_STRING,
                                                    G_TYPE_OBJECT));
  logview_filter_manager_update_model (manager);

  table = gtk_table_new (3, 2, FALSE);
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_ETCHED_IN);
  priv->tree = gtk_tree_view_new_with_model (priv->model);
  gtk_widget_set_size_request (priv->tree, 150, 200);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->tree), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->tree);

  text_renderer = gtk_cell_renderer_text_new ();

  column = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start (column, text_renderer, TRUE);
  gtk_tree_view_column_set_attributes (column,
                                       text_renderer,
                                       "text", COLUMN_NAME,
                                       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree),
                               column);

  priv->add_button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  priv->edit_button = gtk_button_new_from_stock (GTK_STOCK_PROPERTIES);
  priv->remove_button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);

  gtk_window_set_title (GTK_WINDOW (manager),
                        _("Filters"));

  g_signal_connect (priv->add_button, "clicked", 
                    G_CALLBACK (on_add_clicked), manager);
  g_signal_connect (priv->edit_button, "clicked", 
                    G_CALLBACK (on_edit_clicked), manager);
  g_signal_connect (priv->remove_button, "clicked", 
                    G_CALLBACK (on_remove_clicked), manager);

  gtk_widget_set_sensitive (priv->edit_button, FALSE);
  gtk_widget_set_sensitive (priv->remove_button, FALSE);

  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree)),
                    "changed", G_CALLBACK (on_tree_selection_changed),
                    manager);

  gtk_table_attach_defaults (GTK_TABLE (table),
                             scrolled_window,
                             0, 1, 0, 3);
  gtk_table_attach (GTK_TABLE (table),
                    priv->add_button,
                    1, 2, 0, 1, GTK_FILL, 0, 5, 5);
  gtk_table_attach (GTK_TABLE (table),
                    priv->edit_button,
                    1, 2, 1, 2, GTK_FILL, 0, 5, 5);
  gtk_table_attach (GTK_TABLE (table),
                    priv->remove_button,
                    1, 2, 2, 3, GTK_FILL, 0, 5, 5);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (manager))),
                      table, TRUE, TRUE, 5);
  gtk_widget_show_all (GTK_WIDGET (manager));
}

static void
logview_filter_manager_dispose (GObject *object)
{
  LogviewFilterManager* manager;

  manager = LOGVIEW_FILTER_MANAGER (object);

  g_object_unref (manager->priv->builder);

  G_OBJECT_CLASS (logview_filter_manager_parent_class)->dispose (object);
}

static void
logview_filter_manager_response (GtkDialog* dialog, gint response_id)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
logview_filter_manager_class_init (LogviewFilterManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *parent_class = GTK_DIALOG_CLASS (klass);

  g_type_class_add_private (klass, sizeof (LogviewFilterManagerPrivate));

  object_class->dispose = logview_filter_manager_dispose;
  parent_class->response = logview_filter_manager_response;
}

GtkWidget *
logview_filter_manager_new (void)
{
  return g_object_new (LOGVIEW_TYPE_FILTER_MANAGER, NULL);
}
