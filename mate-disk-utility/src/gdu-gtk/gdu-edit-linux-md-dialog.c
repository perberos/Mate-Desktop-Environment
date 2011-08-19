/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#define _GNU_SOURCE

#include "config.h"
#include <glib/gi18n-lib.h>

#include <math.h>

#include "gdu-edit-linux-md-dialog.h"
#include "gdu-size-widget.h"

struct GduEditLinuxMdDialogPrivate
{
        gulong changed_id;
        gulong removed_id;

        GtkWidget *components_tree_view;
        GtkTreeStore *components_tree_store;

        GduDetailsElement *component_smart_element;
        GduDetailsElement *component_position_element;
        GduDetailsElement *component_state_element;
        GduDetailsElement *component_device_element;

        GduButtonElement *add_spare_button;
        GduButtonElement *expand_button;
        GduButtonElement *component_attach_button;
        GduButtonElement *component_remove_button;

};

enum {
        ADD_SPARE_BUTTON_CLICKED_SIGNAL,
        EXPAND_BUTTON_CLICKED_SIGNAL,
        ATTACH_BUTTON_CLICKED_SIGNAL,
        REMOVE_BUTTON_CLICKED_SIGNAL,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
        MD_LINUX_SLAVE_VOLUME_COLUMN,
        MD_LINUX_SLAVE_DRIVE_COLUMN,
        MD_LINUX_POSITION_COLUMN,
        MD_LINUX_POSITION_STRING_COLUMN,
        MD_LINUX_ICON_COLUMN,
        MD_LINUX_OBJPATH_COLUMN,
        MD_LINUX_SLAVE_STATE_STRING_COLUMN,
        MD_LINUX_N_COLUMNS,
};

static void gdu_edit_linux_md_dialog_constructed (GObject *object);

static void update_tree (GduEditLinuxMdDialog *dialog);
static void update_details (GduEditLinuxMdDialog *dialog);

static GduDevice *get_selected_component (GduEditLinuxMdDialog *dialog);

G_DEFINE_TYPE (GduEditLinuxMdDialog, gdu_edit_linux_md_dialog, GDU_TYPE_DIALOG)

static void
gdu_edit_linux_md_dialog_finalize (GObject *object)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (object);

        if (dialog->priv->changed_id > 0) {
                g_signal_handler_disconnect (gdu_dialog_get_presentable (GDU_DIALOG (dialog)),
                                             dialog->priv->changed_id);
        }
        if (dialog->priv->removed_id > 0) {
                g_signal_handler_disconnect (gdu_dialog_get_presentable (GDU_DIALOG (dialog)),
                                             dialog->priv->removed_id);
        }

        if (G_OBJECT_CLASS (gdu_edit_linux_md_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_edit_linux_md_dialog_parent_class)->finalize (object);
}

static void
gdu_edit_linux_md_dialog_class_init (GduEditLinuxMdDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduEditLinuxMdDialogPrivate));

        object_class->constructed  = gdu_edit_linux_md_dialog_constructed;
        object_class->finalize     = gdu_edit_linux_md_dialog_finalize;

        signals[ADD_SPARE_BUTTON_CLICKED_SIGNAL] =
                g_signal_new ("add-spare-button-clicked",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduEditLinuxMdDialogClass, add_spare_button_clicked),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        signals[EXPAND_BUTTON_CLICKED_SIGNAL] =
                g_signal_new ("expand-button-clicked",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduEditLinuxMdDialogClass, expand_button_clicked),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        signals[ATTACH_BUTTON_CLICKED_SIGNAL] =
                g_signal_new ("attach-button-clicked",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduEditLinuxMdDialogClass, attach_button_clicked),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE,
                              1,
                              GDU_TYPE_DEVICE);

        signals[REMOVE_BUTTON_CLICKED_SIGNAL] =
                g_signal_new ("remove-button-clicked",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduEditLinuxMdDialogClass, attach_button_clicked),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE,
                              1,
                              GDU_TYPE_DEVICE);
}

static void
gdu_edit_linux_md_dialog_init (GduEditLinuxMdDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                                    GDU_TYPE_EDIT_LINUX_MD_DIALOG,
                                                    GduEditLinuxMdDialogPrivate);
}

GtkWidget *
gdu_edit_linux_md_dialog_new (GtkWindow        *parent,
                              GduLinuxMdDrive  *linux_md_drive)
{
        g_return_val_if_fail (GDU_IS_LINUX_MD_DRIVE (linux_md_drive), NULL);
        return GTK_WIDGET (g_object_new (GDU_TYPE_EDIT_LINUX_MD_DIALOG,
                                         "transient-for", parent,
                                         "presentable", linux_md_drive,
                                         NULL));
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_add_spare_button_clicked (GduButtonElement *button_element,
                                 gpointer          user_data)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (user_data);
        g_signal_emit (dialog, signals[ADD_SPARE_BUTTON_CLICKED_SIGNAL], 0);
}

static void
on_expand_button_clicked (GduButtonElement *button_element,
                          gpointer          user_data)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (user_data);
        g_signal_emit (dialog, signals[EXPAND_BUTTON_CLICKED_SIGNAL], 0);
}

static void
on_component_attach_button_clicked (GduButtonElement *button_element,
                                    gpointer          user_data)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (user_data);
        GduDevice *slave;

        slave = get_selected_component (dialog);
        g_signal_emit (dialog, signals[ATTACH_BUTTON_CLICKED_SIGNAL], 0, slave);
        g_object_unref (slave);
}

static void
on_component_remove_button_clicked (GduButtonElement *button_element,
                                    gpointer          user_data)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (user_data);
        GduDevice *slave;

        slave = get_selected_component (dialog);
        g_signal_emit (dialog, signals[REMOVE_BUTTON_CLICKED_SIGNAL], 0, slave);
        g_object_unref (slave);
}



/* ---------------------------------------------------------------------------------------------------- */

static void
format_markup (GtkCellLayout   *cell_layout,
               GtkCellRenderer *renderer,
               GtkTreeModel    *tree_model,
               GtkTreeIter     *iter,
               gpointer         user_data)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (user_data);
        GtkTreeSelection *tree_selection;
        gchar *name;
        gchar *drive_name;
        GduPresentable *volume_for_slave;
        GduPresentable *drive_for_slave;
        gchar *markup;
        gchar color[16];
        GtkStateType state;

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->components_tree_view));

        gtk_tree_model_get (tree_model,
                            iter,
                            MD_LINUX_SLAVE_VOLUME_COLUMN, &volume_for_slave,
                            MD_LINUX_SLAVE_DRIVE_COLUMN, &drive_for_slave,
                            -1);

        if (gtk_tree_selection_iter_is_selected (tree_selection, iter)) {
                if (gtk_widget_has_focus (GTK_WIDGET (dialog->priv->components_tree_view)))
                        state = GTK_STATE_SELECTED;
                else
                        state = GTK_STATE_ACTIVE;
        } else {
                state = GTK_STATE_NORMAL;
        }
        gdu_util_get_mix_color (GTK_WIDGET (dialog->priv->components_tree_view), state, color, sizeof (color));

        name = gdu_presentable_get_vpd_name (volume_for_slave);
        if (drive_for_slave != NULL) {
                drive_name = gdu_presentable_get_vpd_name (drive_for_slave);
        } else {
                drive_name = g_strdup ("");
        }

        markup = g_strdup_printf ("<b>%s</b>\n"
                                  "<span fgcolor=\"%s\"><small>%s</small></span>",
                                  name,
                                  color,
                                  drive_name);

        g_object_set (renderer,
                      "markup", markup,
                      NULL);

        g_free (name);
        g_free (drive_name);
        g_free (markup);
        g_object_unref (volume_for_slave);
        if (drive_for_slave != NULL)
                g_object_unref (drive_for_slave);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_component_tree_selection_changed  (GtkTreeSelection *selection,
                                      gpointer          user_data)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (user_data);

        update_details (dialog);
}

static void
on_linux_md_drive_changed (GduPresentable   *presentable,
                           gpointer          user_data)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (user_data);

        update_tree (dialog);

        /* close the dialog if the RAID array stops */
        if (!gdu_drive_is_active (GDU_DRIVE (gdu_dialog_get_presentable (GDU_DIALOG (dialog))))) {
                gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
        }
}

static void
on_linux_md_drive_removed (GduPresentable   *presentable,
                           gpointer          user_data)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (user_data);

        /* close the dialog if the RAID array disappears */
        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
}

static void
gdu_edit_linux_md_dialog_constructed (GObject *object)
{
        GduEditLinuxMdDialog *dialog = GDU_EDIT_LINUX_MD_DIALOG (object);
        GduLinuxMdDrive *linux_md_drive;
        GtkWidget *content_area;
        GtkWidget *action_area;
        GtkWidget *vbox;
        GtkWidget *vbox2;
        GtkWidget *hbox;
        GtkWidget *align;
        GIcon *icon;
        GtkWidget *image;
        GtkWidget *table;
        GtkWidget *label;
        gchar *s;
        gchar *name;
        gchar *vpd_name;
        GPtrArray *elements;
        GduDetailsElement *element;
        GtkWidget *tree_view;
        GtkWidget *scrolled_window;
        GtkTreeSelection *selection;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        GduButtonElement *button_element;

        linux_md_drive = GDU_LINUX_MD_DRIVE (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));

        icon = gdu_presentable_get_icon (GDU_PRESENTABLE (linux_md_drive));

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        gtk_container_set_border_width (GTK_CONTAINER (content_area), 10);

        hbox = gtk_hbox_new (FALSE, 12);
        gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);

        image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
        gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

        vbox = gtk_vbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

        action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
        gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
        gtk_box_set_spacing (GTK_BOX (content_area), 0);
        gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
        gtk_box_set_spacing (GTK_BOX (action_area), 6);

        name = gdu_presentable_get_name (GDU_PRESENTABLE (linux_md_drive));
        vpd_name = gdu_presentable_get_vpd_name (GDU_PRESENTABLE (linux_md_drive));
        s = g_strdup_printf (_("Edit components on %s (%s)"), name, vpd_name);
        gtk_window_set_title (GTK_WINDOW (dialog), s);
        g_free (s);

        gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

        /* -------------------------------------------------------------------------------- */

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        s = g_strconcat ("<b>", _("C_omponents"), "</b>", NULL);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), s);
        g_free (s);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        vbox2 = gtk_vbox_new (FALSE, 12);
        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
        gtk_container_add (GTK_CONTAINER (align), vbox2);

        /* -------------------------------------------------------------------------------- */

        tree_view = gtk_tree_view_new ();
        dialog->priv->components_tree_view = tree_view;
        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
        gtk_box_pack_start (GTK_BOX (vbox2), scrolled_window, TRUE, TRUE, 0);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), tree_view);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
        g_signal_connect (selection, "changed", G_CALLBACK (on_component_tree_selection_changed), dialog);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_title (column, _("Position"));
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "markup", MD_LINUX_POSITION_STRING_COLUMN,
                                             NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_title (column, _("Component"));
        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", MD_LINUX_ICON_COLUMN,
                                             NULL);
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
                                            renderer,
                                            format_markup,
                                            dialog,
                                            NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_title (column, _("State"));
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "markup", MD_LINUX_SLAVE_STATE_STRING_COLUMN,
                                             NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), TRUE);

        /* -------------------------------------------------------------------------------- */

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        element = gdu_details_element_new (_("SMART Status:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->component_smart_element = element;

        element = gdu_details_element_new (_("Position:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->component_position_element = element;

        element = gdu_details_element_new (_("State:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->component_state_element = element;

        element = gdu_details_element_new (_("Device:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->component_device_element = element;

        table = gdu_details_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

        /* -------------------------------------------------------------------------------- */

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        button_element = gdu_button_element_new (GTK_STOCK_NEW,
                                                 _("Add _Spare"),
                                                 _("Add a spare to the array"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_add_spare_button_clicked),
                          dialog);
        g_ptr_array_add (elements, button_element);
        dialog->priv->add_spare_button = button_element;

        button_element = gdu_button_element_new (GTK_STOCK_NEW,
                                                 _("_Expand Array"),
                                                 _("Increase the capacity of the array"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_expand_button_clicked),
                          dialog);
        g_ptr_array_add (elements, button_element);
        dialog->priv->expand_button = button_element;

        button_element = gdu_button_element_new (GTK_STOCK_ADD,
                                                 _("_Attach Component"),
                                                 _("Attach the component to the array"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_component_attach_button_clicked),
                          dialog);
        g_ptr_array_add (elements, button_element);
        dialog->priv->component_attach_button = button_element;

        button_element = gdu_button_element_new (GTK_STOCK_DELETE,
                                                 _("_Remove Component"),
                                                 _("Remove the component from the array"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_component_remove_button_clicked),
                          dialog);
        g_ptr_array_add (elements, button_element);
        dialog->priv->component_remove_button = button_element;

        table = gdu_button_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

        /* -------------------------------------------------------------------------------- */

        dialog->priv->changed_id = g_signal_connect (linux_md_drive,
                                                     "changed",
                                                     G_CALLBACK (on_linux_md_drive_changed),
                                                     dialog);
        dialog->priv->removed_id = g_signal_connect (linux_md_drive,
                                                     "removed",
                                                     G_CALLBACK (on_linux_md_drive_removed),
                                                     dialog);
        update_tree (dialog);
        update_details (dialog);

        /* close the dialog if the RAID array is not running */
        if (!gdu_drive_is_active (GDU_DRIVE (gdu_dialog_get_presentable (GDU_DIALOG (dialog))))) {
                g_warning ("Tried to construct a dialog for a non-running array");
                gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
        }

        g_free (name);
        g_free (vpd_name);

        /* select a sane size for the dialog and allow resizing */
        gtk_widget_set_size_request (GTK_WIDGET (dialog), 650, 450);
        gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

        if (G_OBJECT_CLASS (gdu_edit_linux_md_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_edit_linux_md_dialog_parent_class)->constructed (object);
}

/* ---------------------------------------------------------------------------------------------------- */

static GduDevice *
get_selected_component (GduEditLinuxMdDialog *dialog)
{
        gchar *component_objpath;
        GduPool *pool;
        GduDevice *slave_device;
        GtkTreeSelection *tree_selection;
        GtkTreeIter iter;

        component_objpath = NULL;
        slave_device = NULL;
        pool = NULL;

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->components_tree_view));
        if (gtk_tree_selection_get_selected (tree_selection, NULL, &iter)) {
                gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->components_tree_store), &iter,
                                    MD_LINUX_OBJPATH_COLUMN,
                                    &component_objpath,
                                    -1);
        }

        if (component_objpath == NULL) {
                goto out;
        }

        pool = gdu_presentable_get_pool (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        slave_device = gdu_pool_get_by_object_path (pool, component_objpath);

 out:
        g_free (component_objpath);
        if (pool != NULL)
                g_object_unref (pool);

        return slave_device;
}

/* ---------------------------------------------------------------------------------------------------- */

static gint
component_tree_sort_func (GtkTreeModel *model,
                          GtkTreeIter  *a,
                          GtkTreeIter  *b,
                          gpointer      user_data)
{
        gint ret;
        gint a_position;
        gint b_position;
        gchar *a_objpath;
        gchar *b_objpath;

        gtk_tree_model_get (model,
                            a,
                            MD_LINUX_OBJPATH_COLUMN, &a_objpath,
                            MD_LINUX_POSITION_COLUMN, &a_position,
                            -1);

        gtk_tree_model_get (model,
                            b,
                            MD_LINUX_OBJPATH_COLUMN, &b_objpath,
                            MD_LINUX_POSITION_COLUMN, &b_position,
                            -1);

        if (a_position != b_position)
                ret = a_position - b_position;
        else
                ret = g_strcmp0 (a_objpath, b_objpath);

        g_free (a_objpath);
        g_free (b_objpath);
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
update_tree (GduEditLinuxMdDialog *dialog)
{
        GduLinuxMdDrive *linux_md_drive;
        GtkTreeStore *store;
        GList *slaves;
        GList *l;
        gchar *selected_component_objpath;
        GtkTreeSelection *tree_selection;
        GtkTreeIter *iter_to_select;

        linux_md_drive = GDU_LINUX_MD_DRIVE (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        slaves = gdu_linux_md_drive_get_slaves (linux_md_drive);

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->components_tree_view));

        iter_to_select = NULL;
        selected_component_objpath = NULL;
        if (dialog->priv->components_tree_store != NULL) {
                GtkTreeIter iter;
                if (gtk_tree_selection_get_selected (tree_selection, NULL, &iter)) {
                        gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->components_tree_store), &iter,
                                            MD_LINUX_OBJPATH_COLUMN,
                                            &selected_component_objpath,
                                            -1);
                }
                g_object_unref (dialog->priv->components_tree_store);
        }

        store = gtk_tree_store_new (MD_LINUX_N_COLUMNS,
                                    GDU_TYPE_VOLUME,
                                    GDU_TYPE_DRIVE,
                                    G_TYPE_INT,
                                    G_TYPE_STRING,
                                    GDK_TYPE_PIXBUF,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING);
        dialog->priv->components_tree_store = store;

        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
                                         MD_LINUX_OBJPATH_COLUMN,
                                         component_tree_sort_func,
                                         NULL,
                                         NULL);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                              MD_LINUX_OBJPATH_COLUMN,
                                              GTK_SORT_ASCENDING);

        /* add all slaves */
        for (l = slaves; l != NULL; l = l->next) {
                GduDevice *sd = GDU_DEVICE (l->data);
                GdkPixbuf *pixbuf;
                GtkTreeIter iter;
                GduPool *pool;
                GduPresentable *volume_for_slave;
                GduPresentable *drive_for_slave;
                gchar *position_str;
                const gchar *object_path;
                gint position;
                gchar *slave_state_str;

                if (gdu_device_should_ignore (sd))
                        continue;

                pool = gdu_device_get_pool (sd);
                volume_for_slave = gdu_pool_get_volume_by_device (pool, sd);
                g_object_unref (pool);

                if (volume_for_slave == NULL) {
                        g_warning ("Cannot find volume for device `%s'", gdu_device_get_object_path (sd));
                        continue;
                }

                if (gdu_device_is_partition (sd)) {
                        GduDevice *drive_device;
                        drive_device = gdu_pool_get_by_object_path (pool, gdu_device_partition_get_slave (sd));
                        if (drive_device != NULL) {
                                drive_for_slave = gdu_pool_get_drive_by_device (pool, drive_device);
                                g_object_unref (drive_device);
                        } else {
                                g_warning ("No slave with objpath %s", gdu_device_partition_get_slave (sd));
                        }
                } else {
                        drive_for_slave = gdu_pool_get_drive_by_device (pool, sd);
                 }

                if (gdu_drive_is_active (GDU_DRIVE (linux_md_drive))) {
                        position = gdu_device_linux_md_component_get_position (sd);
                        if (position >= 0) {
                                position_str = g_strdup_printf ("%d", position);
                        } else {
                                position = G_MAXINT; /* to appear last in list */
                                position_str = g_strdup ("–");
                        }
                } else {
                        position = G_MAXINT; /* to appear last in list */
                        position_str = g_strdup ("–");
                }

                pixbuf = gdu_util_get_pixbuf_for_presentable (volume_for_slave, GTK_ICON_SIZE_SMALL_TOOLBAR);
                object_path = gdu_device_get_object_path (sd);

                slave_state_str = gdu_linux_md_drive_get_slave_state_markup (linux_md_drive, sd);
                if (slave_state_str == NULL)
                        slave_state_str = g_strdup ("–");

                gtk_tree_store_append (store, &iter, NULL);
                gtk_tree_store_set (store,
                                    &iter,
                                    MD_LINUX_SLAVE_VOLUME_COLUMN, volume_for_slave,
                                    MD_LINUX_SLAVE_DRIVE_COLUMN, drive_for_slave,
                                    MD_LINUX_POSITION_COLUMN, position,
                                    MD_LINUX_POSITION_STRING_COLUMN, position_str,
                                    MD_LINUX_ICON_COLUMN, pixbuf,
                                    MD_LINUX_OBJPATH_COLUMN, object_path,
                                    MD_LINUX_SLAVE_STATE_STRING_COLUMN, slave_state_str,
                                    -1);

                if (g_strcmp0 (object_path, selected_component_objpath) == 0) {
                        g_assert (iter_to_select == NULL);
                        iter_to_select = gtk_tree_iter_copy (&iter);
                }

                g_free (position_str);
                g_free (slave_state_str);
                if (pixbuf != NULL)
                        g_object_unref (pixbuf);

                g_object_unref (volume_for_slave);
                if (drive_for_slave != NULL)
                        g_object_unref (drive_for_slave);
        }

        gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->priv->components_tree_view), GTK_TREE_MODEL (store));

        /* Select the first iter if nothing is currently selected */
        if (iter_to_select == NULL) {
                GtkTreeIter iter;
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
                        iter_to_select = gtk_tree_iter_copy (&iter);
        }
        /* (Re-)select the component */
        if (iter_to_select != NULL) {
                gtk_tree_selection_select_iter (tree_selection, iter_to_select);
                gtk_tree_iter_free (iter_to_select);
        }

        g_list_foreach (slaves, (GFunc) g_object_unref, NULL);
        g_list_free (slaves);
        g_free (selected_component_objpath);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
update_details (GduEditLinuxMdDialog *dialog)
{
        GduLinuxMdDrive *linux_md_drive;
        GduDevice *slave_device;
        GduDevice *slave_drive_device;
        const gchar *slave_device_file;
        gchar *s;
        gchar *s2;
        GIcon *icon;
        gint position;
        gboolean show_component_attach_button;
        gboolean show_component_remove_button;
        GduLinuxMdDriveSlaveFlags slave_flags;

        show_component_attach_button = FALSE;
        show_component_remove_button = FALSE;

        slave_drive_device = NULL;

        slave_device = get_selected_component (dialog);
        if (slave_device == NULL) {
                gdu_details_element_set_text (dialog->priv->component_smart_element, "–");
                gdu_details_element_set_icon (dialog->priv->component_smart_element, NULL);
                gdu_details_element_set_text (dialog->priv->component_position_element, "–");
                gdu_details_element_set_text (dialog->priv->component_state_element, "–");
                gdu_details_element_set_text (dialog->priv->component_device_element, "–");
                goto out;
        }

        linux_md_drive = GDU_LINUX_MD_DRIVE (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));

        s = gdu_linux_md_drive_get_slave_state_markup (linux_md_drive, slave_device);
        if (s == NULL)
                s = g_strdup ("–");
        gdu_details_element_set_text (dialog->priv->component_state_element, s);
        g_free (s);

        if (gdu_device_is_partition (slave_device)) {
                GduPool *pool;
                pool = gdu_device_get_pool (slave_device);
                slave_drive_device = gdu_pool_get_by_object_path (pool, gdu_device_partition_get_slave (slave_device));
                g_object_unref (pool);
        } else {
                slave_drive_device = g_object_ref (slave_device);
        }

        if (gdu_device_drive_ata_smart_get_is_available (slave_drive_device) &&
            gdu_device_drive_ata_smart_get_time_collected (slave_drive_device) > 0) {
                gboolean highlight;

                s = gdu_util_ata_smart_status_to_desc (gdu_device_drive_ata_smart_get_status (slave_drive_device),
                                                       &highlight,
                                                       NULL,
                                                       &icon);
                if (highlight) {
                        s2 = g_strdup_printf ("<span fgcolor=\"red\"><b>%s</b></span>", s);
                        g_free (s);
                        s = s2;
                }

                gdu_details_element_set_text (dialog->priv->component_smart_element, s);
                gdu_details_element_set_icon (dialog->priv->component_smart_element, icon);

                g_free (s);
                g_object_unref (icon);
        } else {
                /* Translators: Used when SMART is not supported on the RAID component */
                gdu_details_element_set_text (dialog->priv->component_smart_element, _("Not Supported"));
                icon = g_themed_icon_new ("gdu-smart-unknown");
                gdu_details_element_set_icon (dialog->priv->component_smart_element, icon);
                g_object_unref (icon);
        }

        if (gdu_drive_is_active (GDU_DRIVE (linux_md_drive))) {
                position = gdu_device_linux_md_component_get_position (slave_device);
                if (position >= 0) {
                        s = g_strdup_printf ("%d", position);
                } else {
                        s = g_strdup ("–");
                }
        } else {
                s = g_strdup ("–");
        }
        gdu_details_element_set_text (dialog->priv->component_position_element, s);
        g_free (s);

        slave_device_file = gdu_device_get_device_file_presentation (slave_device);
        if (slave_device_file == NULL || strlen (slave_device_file) == 0)
                slave_device_file = gdu_device_get_device_file (slave_device);
        gdu_details_element_set_text (dialog->priv->component_device_element, slave_device_file);

        if (gdu_drive_is_active (GDU_DRIVE (linux_md_drive))) {
                slave_flags = gdu_linux_md_drive_get_slave_flags (linux_md_drive, slave_device);
                if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NOT_ATTACHED)
                        show_component_attach_button = TRUE;
                else
                        show_component_remove_button = TRUE;
        }

 out:
        gdu_button_element_set_visible (dialog->priv->component_attach_button, show_component_attach_button);
        gdu_button_element_set_visible (dialog->priv->component_remove_button, show_component_remove_button);

        if (slave_device != NULL)
                g_object_unref (slave_device);
        if (slave_drive_device != NULL)
                g_object_unref (slave_drive_device);
}

