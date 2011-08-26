/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 1999 Free Software Foundation, Inc.
 * Copyright (C) 2007 Vincent Untz.
 * Copyright (C) 2008 Lucas Rocha.
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <mateconf/mateconf-client.h>

#include "gsm-properties-dialog.h"
#include "gsm-app-dialog.h"
#include "eggdesktopfile.h"
#include "gsm-util.h"
#include "gsp-app.h"
#include "gsp-app-manager.h"

#define GSM_PROPERTIES_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_PROPERTIES_DIALOG, GsmPropertiesDialogPrivate))

#define IS_STRING_EMPTY(x) ((x)==NULL||(x)[0]=='\0')

#define REALLY_IDENTICAL_STRING(a, b)                   \
        ((a && b && !strcmp (a, b)) || (!a && !b))

#define GTKBUILDER_FILE "session-properties.ui"

#define CAPPLET_TREEVIEW_WIDGET_NAME      "session_properties_treeview"
#define CAPPLET_ADD_WIDGET_NAME           "session_properties_add_button"
#define CAPPLET_DELETE_WIDGET_NAME        "session_properties_delete_button"
#define CAPPLET_EDIT_WIDGET_NAME          "session_properties_edit_button"
#define CAPPLET_SAVE_WIDGET_NAME          "session_properties_save_button"
#define CAPPLET_REMEMBER_WIDGET_NAME      "session_properties_remember_toggle"

#define STARTUP_APP_ICON     "system-run"

#define SPC_MATECONF_CONFIG_PREFIX   "/apps/mate-session/options"
#define SPC_MATECONF_AUTOSAVE_KEY    SPC_MATECONF_CONFIG_PREFIX "/auto_save_session"

struct GsmPropertiesDialogPrivate
{
        GtkBuilder        *xml;
        GtkListStore      *list_store;
        GtkTreeModel      *tree_filter;

        GtkTreeView       *treeview;
        GtkWidget         *add_button;
        GtkWidget         *delete_button;
        GtkWidget         *edit_button;

        GtkWidget         *remember_toggle;

        GspAppManager     *manager;
};

enum {
        STORE_COL_VISIBLE = 0,
        STORE_COL_ENABLED,
        STORE_COL_GICON,
        STORE_COL_DESCRIPTION,
        STORE_COL_APP,
        NUMBER_OF_COLUMNS
};

static void     gsm_properties_dialog_class_init  (GsmPropertiesDialogClass *klass);
static void     gsm_properties_dialog_init        (GsmPropertiesDialog      *properties_dialog);
static void     gsm_properties_dialog_finalize    (GObject                  *object);

G_DEFINE_TYPE (GsmPropertiesDialog, gsm_properties_dialog, GTK_TYPE_DIALOG)

static gboolean
find_by_app (GtkTreeModel *model,
             GtkTreeIter  *iter,
             GspApp       *app)
{
        GspApp *iter_app = NULL;

        if (!gtk_tree_model_get_iter_first (model, iter)) {
                return FALSE;
        }

        do {
                gtk_tree_model_get (model, iter,
                                    STORE_COL_APP, &iter_app,
                                    -1);

                if (iter_app == app) {
                        g_object_unref (iter_app);
                        return TRUE;
                }
        } while (gtk_tree_model_iter_next (model, iter));

        return FALSE;
}

static void
_fill_iter_from_app (GtkListStore *list_store,
                     GtkTreeIter  *iter,
                     GspApp       *app)
{
        gboolean    hidden;
        gboolean    enabled;
        GIcon      *icon;
        const char *description;

        hidden      = gsp_app_get_hidden (app);
        enabled     = gsp_app_get_enabled (app);
        icon        = gsp_app_get_icon (app);
        description = gsp_app_get_description (app);

        if (G_IS_THEMED_ICON (icon)) {
                GtkIconTheme       *theme;
                const char * const *icon_names;

                theme = gtk_icon_theme_get_default ();
                icon_names = g_themed_icon_get_names (G_THEMED_ICON (icon));
                if (icon_names[0] == NULL ||
                    !gtk_icon_theme_has_icon (theme, icon_names[0])) {
                        g_object_unref (icon);
                        icon = NULL;
                }
        } else if (G_IS_FILE_ICON (icon)) {
                GFile *iconfile;

                iconfile = g_file_icon_get_file (G_FILE_ICON (icon));
                if (!g_file_query_exists (iconfile, NULL)) {
                        g_object_unref (icon);
                        icon = NULL;
                }
        }

        if (icon == NULL) {
                icon = g_themed_icon_new (STARTUP_APP_ICON);
        }

        gtk_list_store_set (list_store, iter,
                            STORE_COL_VISIBLE, !hidden,
                            STORE_COL_ENABLED, enabled,
                            STORE_COL_GICON, icon,
                            STORE_COL_DESCRIPTION, description,
                            STORE_COL_APP, app,
                            -1);
        g_object_unref (icon);
}

static void
_app_changed (GsmPropertiesDialog *dialog,
              GspApp              *app)
{
        GtkTreeIter iter;

        if (!find_by_app (GTK_TREE_MODEL (dialog->priv->list_store),
                          &iter, app)) {
                return;
        }

        _fill_iter_from_app (dialog->priv->list_store, &iter, app);
}

static void
append_app (GsmPropertiesDialog *dialog,
            GspApp              *app)
{
        GtkTreeIter   iter;

        gtk_list_store_append (dialog->priv->list_store, &iter);
        _fill_iter_from_app (dialog->priv->list_store, &iter, app);

        g_signal_connect_swapped (app, "changed",
                                  G_CALLBACK (_app_changed), dialog);
}

static void
_app_added (GsmPropertiesDialog *dialog,
            GspApp              *app,
            GspAppManager       *manager)
{
        append_app (dialog, app);
}

static void
_app_removed (GsmPropertiesDialog *dialog,
              GspApp              *app,
              GspAppManager       *manager)
{
        GtkTreeIter iter;

        if (!find_by_app (GTK_TREE_MODEL (dialog->priv->list_store),
                          &iter, app)) {
                return;
        }

        g_signal_handlers_disconnect_by_func (app,
                                              _app_changed,
                                              dialog);
        gtk_list_store_remove (dialog->priv->list_store, &iter);
}

static void
populate_model (GsmPropertiesDialog *dialog)
{
        GSList *apps;
        GSList *l;

        apps = gsp_app_manager_get_apps (dialog->priv->manager);
        for (l = apps; l != NULL; l = l->next) {
                append_app (dialog, GSP_APP (l->data));
        }
        g_slist_free (apps);
}

static void
on_selection_changed (GtkTreeSelection    *selection,
                      GsmPropertiesDialog *dialog)
{
        gboolean sel;

        sel = gtk_tree_selection_get_selected (selection, NULL, NULL);

        gtk_widget_set_sensitive (dialog->priv->edit_button, sel);
        gtk_widget_set_sensitive (dialog->priv->delete_button, sel);
}

static void
on_startup_enabled_toggled (GtkCellRendererToggle *cell_renderer,
                            char                  *path,
                            GsmPropertiesDialog   *dialog)
{
        GtkTreeIter iter;
        GspApp     *app;
        gboolean    active;

        if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (dialog->priv->tree_filter),
                                                  &iter, path)) {
                return;
        }

        app = NULL;
        gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->tree_filter),
                            &iter,
                            STORE_COL_APP, &app,
                            -1);

        active = gtk_cell_renderer_toggle_get_active (cell_renderer);
        active = !active;

        if (app) {
                gsp_app_set_enabled (app, active);
                g_object_unref (app);
        }
}

static void
on_drag_data_received (GtkWidget           *widget,
                       GdkDragContext      *drag_context,
                       gint                 x,
                       gint                 y,
                       GtkSelectionData    *data,
                       guint                info,
                       guint                time,
                       GsmPropertiesDialog *dialog)
{
        gboolean dnd_success;

        dnd_success = FALSE;

        if (data != NULL) {
                char **filenames;
                int    i;

                filenames = gtk_selection_data_get_uris (data);

                for (i = 0; filenames[i] && filenames[i][0]; i++) {
                        /* Return success if at least one file succeeded */
                        gboolean file_success;
                        file_success = gsp_app_copy_desktop_file (filenames[i]);
                        dnd_success = dnd_success || file_success;
                }

                g_strfreev (filenames);
        }

        gtk_drag_finish (drag_context, dnd_success, FALSE, time);
        g_signal_stop_emission_by_name (widget, "drag_data_received");
}

static void
on_drag_begin (GtkWidget           *widget,
               GdkDragContext      *context,
               GsmPropertiesDialog *dialog)
{
        GtkTreePath *path;
        GtkTreeIter  iter;
        GspApp      *app;

        gtk_tree_view_get_cursor (GTK_TREE_VIEW (widget), &path, NULL);
        gtk_tree_model_get_iter (GTK_TREE_MODEL (dialog->priv->tree_filter),
                                 &iter, path);
        gtk_tree_path_free (path);

        gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->tree_filter),
                            &iter,
                            STORE_COL_APP, &app,
                            -1);

        if (app) {
                g_object_set_data_full (G_OBJECT (context), "gsp-app",
                                        g_object_ref (app), g_object_unref);
                g_object_unref (app);
        }

}

static void
on_drag_data_get (GtkWidget           *widget,
                  GdkDragContext      *context,
                  GtkSelectionData    *selection_data,
                  guint                info,
                  guint                time,
                  GsmPropertiesDialog *dialog)
{
        GspApp *app;

        app = g_object_get_data (G_OBJECT (context), "gsp-app");
        if (app) {
                const char *uris[2];
                char       *uri;

                uri = g_filename_to_uri (gsp_app_get_path (app), NULL, NULL);

                uris[0] = uri;
                uris[1] = NULL;
                gtk_selection_data_set_uris (selection_data, (char **) uris);

                g_free (uri);
        }
}

static void
on_add_app_clicked (GtkWidget           *widget,
                    GsmPropertiesDialog *dialog)
{
        GtkWidget  *add_dialog;
        char       *name;
        char       *exec;
        char       *comment;

        add_dialog = gsm_app_dialog_new (NULL, NULL, NULL);
        gtk_window_set_transient_for (GTK_WINDOW (add_dialog),
                                      GTK_WINDOW (dialog));

        if (gsm_app_dialog_run (GSM_APP_DIALOG (add_dialog),
                                &name, &exec, &comment)) {
                gsp_app_create (name, comment, exec);
                g_free (name);
                g_free (exec);
                g_free (comment);
        }
}

static void
on_delete_app_clicked (GtkWidget           *widget,
                       GsmPropertiesDialog *dialog)
{
        GtkTreeSelection *selection;
        GtkTreeIter       iter;
        GspApp           *app;

        selection = gtk_tree_view_get_selection (dialog->priv->treeview);

        if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
                return;
        }

        app = NULL;
        gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->tree_filter),
                            &iter,
                            STORE_COL_APP, &app,
                            -1);

        if (app) {
                gsp_app_delete (app);
                g_object_unref (app);
        }
}

static void
on_edit_app_clicked (GtkWidget           *widget,
                     GsmPropertiesDialog *dialog)
{
        GtkTreeSelection *selection;
        GtkTreeIter       iter;
        GspApp           *app;

        selection = gtk_tree_view_get_selection (dialog->priv->treeview);

        if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
                return;
        }

        app = NULL;
        gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->tree_filter),
                            &iter,
                            STORE_COL_APP, &app,
                            -1);

        if (app) {
                GtkWidget  *edit_dialog;
                char       *name;
                char       *exec;
                char       *comment;

                edit_dialog = gsm_app_dialog_new (gsp_app_get_name (app),
                                                  gsp_app_get_exec (app),
                                                  gsp_app_get_comment (app));
                gtk_window_set_transient_for (GTK_WINDOW (edit_dialog),
                                              GTK_WINDOW (dialog));

                if (gsm_app_dialog_run (GSM_APP_DIALOG (edit_dialog),
                                        &name, &exec, &comment)) {
                        gsp_app_update (app, name, comment, exec);
                        g_free (name);
                        g_free (exec);
                        g_free (comment);
                }

                g_object_unref (app);
        }
}

static void
on_row_activated (GtkTreeView         *tree_view,
                  GtkTreePath         *path,
                  GtkTreeViewColumn   *column,
                  GsmPropertiesDialog *dialog)
{
        on_edit_app_clicked (NULL, dialog);
}

static void
on_autosave_value_notify (MateConfClient         *client,
                          guint                id,
                          MateConfEntry          *entry,
                          GsmPropertiesDialog *dialog)
{
        gboolean   gval;
        gboolean   bval;

        gval = mateconf_value_get_bool (entry->value);
        bval = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->remember_toggle));

        if (bval != gval) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->remember_toggle), gval);
        }
}

static void
on_autosave_value_toggled (GtkToggleButton     *button,
                           GsmPropertiesDialog *dialog)
{
        MateConfClient *client;
        gboolean     gval;
        gboolean     bval;

        client = mateconf_client_get_default ();
        gval = mateconf_client_get_bool (client, SPC_MATECONF_AUTOSAVE_KEY, NULL);
        bval = gtk_toggle_button_get_active (button);

        if (gval != bval) {
                mateconf_client_set_bool (client, SPC_MATECONF_AUTOSAVE_KEY, bval, NULL);
        }
        g_object_unref (client);
}

static void
on_save_session_clicked (GtkWidget           *widget,
                         GsmPropertiesDialog *dialog)
{
        g_debug ("Session saving is not implemented yet!");
}

static void
setup_dialog (GsmPropertiesDialog *dialog)
{
        GtkTreeView       *treeview;
        GtkWidget         *button;
        GtkTreeModel      *tree_filter;
        GtkTreeViewColumn *column;
        GtkCellRenderer   *renderer;
        GtkTreeSelection  *selection;
        MateConfClient       *client;
        GtkTargetList     *targetlist;

        gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                NULL);

        dialog->priv->list_store = gtk_list_store_new (NUMBER_OF_COLUMNS,
                                                       G_TYPE_BOOLEAN,
                                                       G_TYPE_BOOLEAN,
                                                       G_TYPE_ICON,
                                                       G_TYPE_STRING,
                                                       G_TYPE_OBJECT);
        tree_filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (dialog->priv->list_store),
                                                 NULL);
        g_object_unref (dialog->priv->list_store);
        dialog->priv->tree_filter = tree_filter;

        gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (tree_filter),
                                                  STORE_COL_VISIBLE);

        treeview = GTK_TREE_VIEW (gtk_builder_get_object (dialog->priv->xml,
                                                          CAPPLET_TREEVIEW_WIDGET_NAME));
        dialog->priv->treeview = treeview;

        gtk_tree_view_set_model (treeview, tree_filter);
        g_object_unref (tree_filter);

        gtk_tree_view_set_headers_visible (treeview, FALSE);
        g_signal_connect (treeview,
                          "row-activated",
                          G_CALLBACK (on_row_activated),
                          dialog);

        selection = gtk_tree_view_get_selection (treeview);
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
        g_signal_connect (selection,
                          "changed",
                          G_CALLBACK (on_selection_changed),
                          dialog);

        /* CHECKBOX COLUMN */
        renderer = gtk_cell_renderer_toggle_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Enabled"),
                                                           renderer,
                                                           "active", STORE_COL_ENABLED,
                                                           NULL);
        gtk_tree_view_append_column (treeview, column);
        g_signal_connect (renderer,
                          "toggled",
                          G_CALLBACK (on_startup_enabled_toggled),
                          dialog);

        /* ICON COLUMN */
        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Icon"),
                                                           renderer,
                                                           "gicon", STORE_COL_GICON,
                                                           "sensitive", STORE_COL_ENABLED,
                                                           NULL);
        g_object_set (renderer,
                      "stock-size", GSM_PROPERTIES_ICON_SIZE,
                      NULL);
        gtk_tree_view_append_column (treeview, column);

        /* NAME COLUMN */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Program"),
                                                           renderer,
                                                           "markup", STORE_COL_DESCRIPTION,
                                                           "sensitive", STORE_COL_ENABLED,
                                                           NULL);
        g_object_set (renderer,
                      "ellipsize", PANGO_ELLIPSIZE_END,
                      NULL);
        gtk_tree_view_append_column (treeview, column);


        gtk_tree_view_column_set_sort_column_id (column, STORE_COL_DESCRIPTION);
        gtk_tree_view_set_search_column (treeview, STORE_COL_DESCRIPTION);
        gtk_tree_view_set_rules_hint (treeview, TRUE);

        gtk_tree_view_enable_model_drag_source (treeview,
                                                GDK_BUTTON1_MASK|GDK_BUTTON2_MASK,
                                                NULL, 0,
                                                GDK_ACTION_COPY);
        gtk_drag_source_add_uri_targets (GTK_WIDGET (treeview));

        gtk_drag_dest_set (GTK_WIDGET (treeview),
                           GTK_DEST_DEFAULT_ALL,
                           NULL, 0,
                           GDK_ACTION_COPY);
        gtk_drag_dest_add_uri_targets (GTK_WIDGET (treeview));

        /* we don't want to accept drags coming from this widget */
        targetlist = gtk_drag_dest_get_target_list (GTK_WIDGET (treeview));
        if (targetlist != NULL) {
                GList *list;
                list = targetlist->list;
                while (list != NULL) {
                        GtkTargetPair *targetpair;
                        targetpair = list->data;
                        targetpair->flags = GTK_TARGET_OTHER_WIDGET;
                        list = list->next;
                }
        }

        g_signal_connect (treeview, "drag_begin",
                          G_CALLBACK (on_drag_begin),
                          dialog);
        g_signal_connect (treeview, "drag_data_get",
                          G_CALLBACK (on_drag_data_get),
                          dialog);
        g_signal_connect (treeview, "drag_data_received",
                          G_CALLBACK (on_drag_data_received),
                          dialog);

        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dialog->priv->list_store),
                                              STORE_COL_DESCRIPTION,
                                              GTK_SORT_ASCENDING);


        button = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     CAPPLET_ADD_WIDGET_NAME));
        dialog->priv->add_button = button;
        g_signal_connect (button,
                          "clicked",
                          G_CALLBACK (on_add_app_clicked),
                          dialog);

        button = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     CAPPLET_DELETE_WIDGET_NAME));
        dialog->priv->delete_button = button;
        g_signal_connect (button,
                          "clicked",
                          G_CALLBACK (on_delete_app_clicked),
                          dialog);

        button = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     CAPPLET_EDIT_WIDGET_NAME));
        dialog->priv->edit_button = button;
        g_signal_connect (button,
                          "clicked",
                          G_CALLBACK (on_edit_app_clicked),
                          dialog);

        client = mateconf_client_get_default ();
        button = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     CAPPLET_REMEMBER_WIDGET_NAME));
        dialog->priv->remember_toggle = button;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                      mateconf_client_get_bool (client, SPC_MATECONF_AUTOSAVE_KEY, NULL));
        mateconf_client_notify_add (client,
                                 SPC_MATECONF_AUTOSAVE_KEY,
                                 (MateConfClientNotifyFunc)on_autosave_value_notify,
                                 dialog,
                                 NULL,
                                 NULL);
        g_object_unref (client);

        g_signal_connect (button,
                          "toggled",
                          G_CALLBACK (on_autosave_value_toggled),
                          dialog);

        button = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     CAPPLET_SAVE_WIDGET_NAME));
        g_signal_connect (button,
                          "clicked",
                          G_CALLBACK (on_save_session_clicked),
                          dialog);

        dialog->priv->manager = gsp_app_manager_get ();
        gsp_app_manager_fill (dialog->priv->manager);
        g_signal_connect_swapped (dialog->priv->manager, "added",
                                  G_CALLBACK (_app_added), dialog);
        g_signal_connect_swapped (dialog->priv->manager, "removed",
                                  G_CALLBACK (_app_removed), dialog);

        populate_model (dialog);
}

static GObject *
gsm_properties_dialog_constructor (GType                  type,
                                guint                  n_construct_properties,
                                GObjectConstructParam *construct_properties)
{
        GsmPropertiesDialog *dialog;

        dialog = GSM_PROPERTIES_DIALOG (G_OBJECT_CLASS (gsm_properties_dialog_parent_class)->constructor (type,
                                                                                                                  n_construct_properties,
                                                                                                                  construct_properties));

        setup_dialog (dialog);

        gtk_widget_show (GTK_WIDGET (dialog));

        return G_OBJECT (dialog);
}

static void
gsm_properties_dialog_dispose (GObject *object)
{
        GsmPropertiesDialog *dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_PROPERTIES_DIALOG (object));

        dialog = GSM_PROPERTIES_DIALOG (object);

        if (dialog->priv->xml != NULL) {
                g_object_unref (dialog->priv->xml);
                dialog->priv->xml = NULL;
        }

        G_OBJECT_CLASS (gsm_properties_dialog_parent_class)->dispose (object);

        /* it's important to do this after chaining to the parent dispose
         * method because we want to make sure the treeview has been disposed
         * and removed all its references to GspApp objects */
        if (dialog->priv->manager != NULL) {
                g_object_unref (dialog->priv->manager);
                dialog->priv->manager = NULL;
        }
}

static void
gsm_properties_dialog_class_init (GsmPropertiesDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gsm_properties_dialog_constructor;
        object_class->dispose = gsm_properties_dialog_dispose;
        object_class->finalize = gsm_properties_dialog_finalize;

        g_type_class_add_private (klass, sizeof (GsmPropertiesDialogPrivate));
}

static void
gsm_properties_dialog_init (GsmPropertiesDialog *dialog)
{
        GtkWidget   *content_area;
        GtkWidget   *widget;
        MateConfClient *client;
        GError      *error;

        dialog->priv = GSM_PROPERTIES_DIALOG_GET_PRIVATE (dialog);

        client = mateconf_client_get_default ();
        mateconf_client_add_dir (client,
                              SPC_MATECONF_CONFIG_PREFIX,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);

        dialog->priv->xml = gtk_builder_new ();
        gtk_builder_set_translation_domain (dialog->priv->xml, GETTEXT_PACKAGE);

        error = NULL;
        if (!gtk_builder_add_from_file (dialog->priv->xml,
                                        GTKBUILDER_DIR "/" GTKBUILDER_FILE,
                                        &error)) {
                if (error) {
                        g_warning ("Could not load capplet UI file: %s",
                                   error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Could not load capplet UI file.");
                }
        }

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        widget = GTK_WIDGET (gtk_builder_get_object (dialog->priv->xml,
                                                     "main-notebook"));
        gtk_container_add (GTK_CONTAINER (content_area), widget);

        gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
        gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
        gtk_box_set_spacing (GTK_BOX (content_area), 2);
        gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
        gtk_window_set_icon_name (GTK_WINDOW (dialog), "mate-session-properties");
        gtk_window_set_title (GTK_WINDOW (dialog), _("Startup Applications Preferences"));
}

static void
gsm_properties_dialog_finalize (GObject *object)
{
        GsmPropertiesDialog *dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_PROPERTIES_DIALOG (object));

        dialog = GSM_PROPERTIES_DIALOG (object);

        g_return_if_fail (dialog->priv != NULL);

        G_OBJECT_CLASS (gsm_properties_dialog_parent_class)->finalize (object);
}

GtkWidget *
gsm_properties_dialog_new (void)
{
        GObject *object;

        object = g_object_new (GSM_TYPE_PROPERTIES_DIALOG,
                               NULL);

        return GTK_WIDGET (object);
}
