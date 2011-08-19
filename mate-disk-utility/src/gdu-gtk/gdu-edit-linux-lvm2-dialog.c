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

#include "gdu-edit-linux-lvm2-dialog.h"
#include "gdu-size-widget.h"

struct GduEditLinuxLvm2DialogPrivate
{
        gulong changed_id;
        gulong removed_id;

        GtkWidget *pvs_tree_view;
        GtkTreeStore *pvs_tree_store;

        GduDetailsElement *pv_smart_element;
        GduDetailsElement *pv_device_element;

        GduButtonElement *pv_new_button;
        GduButtonElement *pv_remove_button;

};

enum {
        NEW_BUTTON_CLICKED_SIGNAL,
        REMOVE_BUTTON_CLICKED_SIGNAL,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
        LINUX_LVM2_PV_POSITION_COLUMN,
        LINUX_LVM2_PV_UUID_COLUMN,
        LINUX_LVM2_PV_ICON_COLUMN,
        LINUX_LVM2_PV_VOLUME_COLUMN,
        LINUX_LVM2_PV_DRIVE_COLUMN,
        LINUX_LVM2_PV_SIZE_STRING_COLUMN,
        LINUX_LVM2_PV_UNALLOCATED_SIZE_STRING_COLUMN,
        LINUX_LVM2_PV_N_COLUMNS,
};

static void gdu_edit_linux_lvm2_dialog_constructed (GObject *object);

static void update_tree (GduEditLinuxLvm2Dialog *dialog);
static void update_details (GduEditLinuxLvm2Dialog *dialog);

static GduDevice *get_selected_pv (GduEditLinuxLvm2Dialog *dialog);
static gchar *get_selected_pv_uuid (GduEditLinuxLvm2Dialog *dialog);

G_DEFINE_TYPE (GduEditLinuxLvm2Dialog, gdu_edit_linux_lvm2_dialog, GDU_TYPE_DIALOG)

static void
gdu_edit_linux_lvm2_dialog_finalize (GObject *object)
{
        GduEditLinuxLvm2Dialog *dialog = GDU_EDIT_LINUX_LVM2_DIALOG (object);

        if (dialog->priv->changed_id > 0) {
                g_signal_handler_disconnect (gdu_dialog_get_presentable (GDU_DIALOG (dialog)),
                                             dialog->priv->changed_id);
        }
        if (dialog->priv->removed_id > 0) {
                g_signal_handler_disconnect (gdu_dialog_get_presentable (GDU_DIALOG (dialog)),
                                             dialog->priv->removed_id);
        }

        if (G_OBJECT_CLASS (gdu_edit_linux_lvm2_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_edit_linux_lvm2_dialog_parent_class)->finalize (object);
}

static void
gdu_edit_linux_lvm2_dialog_class_init (GduEditLinuxLvm2DialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduEditLinuxLvm2DialogPrivate));

        object_class->constructed  = gdu_edit_linux_lvm2_dialog_constructed;
        object_class->finalize     = gdu_edit_linux_lvm2_dialog_finalize;

        signals[NEW_BUTTON_CLICKED_SIGNAL] =
                g_signal_new ("new-button-clicked",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduEditLinuxLvm2DialogClass, new_button_clicked),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        signals[REMOVE_BUTTON_CLICKED_SIGNAL] =
                g_signal_new ("remove-button-clicked",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduEditLinuxLvm2DialogClass, remove_button_clicked),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
}

static void
gdu_edit_linux_lvm2_dialog_init (GduEditLinuxLvm2Dialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                                    GDU_TYPE_EDIT_LINUX_LVM2_DIALOG,
                                                    GduEditLinuxLvm2DialogPrivate);
}

GtkWidget *
gdu_edit_linux_lvm2_dialog_new (GtkWindow                *parent,
                                GduLinuxLvm2VolumeGroup  *vg)
{
        g_return_val_if_fail (GDU_IS_LINUX_LVM2_VOLUME_GROUP (vg), NULL);
        return GTK_WIDGET (g_object_new (GDU_TYPE_EDIT_LINUX_LVM2_DIALOG,
                                         "transient-for", parent,
                                         "presentable", vg,
                                         NULL));
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_pv_new_button_clicked (GduButtonElement *button_element,
                          gpointer          user_data)
{
        GduEditLinuxLvm2Dialog *dialog = GDU_EDIT_LINUX_LVM2_DIALOG (user_data);
        g_signal_emit (dialog, signals[NEW_BUTTON_CLICKED_SIGNAL], 0);
}

static void
on_pv_remove_button_clicked (GduButtonElement *button_element,
                             gpointer          user_data)
{
        GduEditLinuxLvm2Dialog *dialog = GDU_EDIT_LINUX_LVM2_DIALOG (user_data);
        gchar *pv_uuid;

        pv_uuid = get_selected_pv_uuid (dialog);
        if (pv_uuid != NULL) {
                g_signal_emit (dialog, signals[REMOVE_BUTTON_CLICKED_SIGNAL], 0, pv_uuid);
                g_free (pv_uuid);
        }
}


/* ---------------------------------------------------------------------------------------------------- */

static void
format_markup (GtkCellLayout   *cell_layout,
               GtkCellRenderer *renderer,
               GtkTreeModel    *tree_model,
               GtkTreeIter     *iter,
               gpointer         user_data)
{
        GduEditLinuxLvm2Dialog *dialog = GDU_EDIT_LINUX_LVM2_DIALOG (user_data);
        GtkTreeSelection *tree_selection;
        gchar *name;
        gchar *drive_name;
        GduPresentable *volume_for_pv;
        GduPresentable *drive_for_pv;
        gchar *pv_uuid;
        gchar *markup;
        gchar color[16];
        GtkStateType state;

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->pvs_tree_view));

        gtk_tree_model_get (tree_model,
                            iter,
                            LINUX_LVM2_PV_VOLUME_COLUMN, &volume_for_pv,
                            LINUX_LVM2_PV_DRIVE_COLUMN, &drive_for_pv,
                            LINUX_LVM2_PV_UUID_COLUMN, &pv_uuid,
                            -1);

        if (gtk_tree_selection_iter_is_selected (tree_selection, iter)) {
                if (gtk_widget_has_focus (GTK_WIDGET (dialog->priv->pvs_tree_view)))
                        state = GTK_STATE_SELECTED;
                else
                        state = GTK_STATE_ACTIVE;
        } else {
                state = GTK_STATE_NORMAL;
        }
        gdu_util_get_mix_color (GTK_WIDGET (dialog->priv->pvs_tree_view), state, color, sizeof (color));

        if (volume_for_pv != NULL) {
                name = gdu_presentable_get_vpd_name (volume_for_pv);
                if (drive_for_pv != NULL) {
                        drive_name = gdu_presentable_get_vpd_name (drive_for_pv);
                } else {
                        drive_name = g_strdup ("");
                }
        } else {
                name = g_strdup (_("Missing Physical Volume"));
                drive_name = g_strdup_printf (_("UUID: %s"), pv_uuid);
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
        if (volume_for_pv != NULL)
                g_object_unref (volume_for_pv);
        if (drive_for_pv != NULL)
                g_object_unref (drive_for_pv);
        g_free (pv_uuid);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_pv_tree_selection_changed  (GtkTreeSelection *selection,
                                      gpointer          user_data)
{
        GduEditLinuxLvm2Dialog *dialog = GDU_EDIT_LINUX_LVM2_DIALOG (user_data);

        update_details (dialog);
}

static void
on_linux_lvm2_vg_changed (GduPresentable   *presentable,
                          gpointer          user_data)
{
        GduEditLinuxLvm2Dialog *dialog = GDU_EDIT_LINUX_LVM2_DIALOG (user_data);

        update_tree (dialog);

        /* close the dialog if the RAID array stops */
        if (!gdu_drive_is_active (GDU_DRIVE (gdu_dialog_get_presentable (GDU_DIALOG (dialog))))) {
                gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
        }
}

static void
on_linux_lvm2_vg_removed (GduPresentable   *presentable,
                          gpointer          user_data)
{
        GduEditLinuxLvm2Dialog *dialog = GDU_EDIT_LINUX_LVM2_DIALOG (user_data);

        /* close the dialog if the RAID array disappears */
        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
}

static void
gdu_edit_linux_lvm2_dialog_constructed (GObject *object)
{
        GduEditLinuxLvm2Dialog *dialog = GDU_EDIT_LINUX_LVM2_DIALOG (object);
        GduLinuxLvm2VolumeGroup *vg;
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

        vg = GDU_LINUX_LVM2_VOLUME_GROUP (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));

        icon = gdu_presentable_get_icon (GDU_PRESENTABLE (vg));

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

        name = gdu_presentable_get_name (GDU_PRESENTABLE (vg));
        vpd_name = gdu_presentable_get_vpd_name (GDU_PRESENTABLE (vg));
        s = g_strdup_printf (_("Edit PVs on %s (%s)"), name, vpd_name);
        gtk_window_set_title (GTK_WINDOW (dialog), s);
        g_free (s);

        gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

        /* -------------------------------------------------------------------------------- */

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        s = g_strconcat ("<b>", _("Physical _Volumes"), "</b>", NULL);
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
        dialog->priv->pvs_tree_view = tree_view;
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
        g_signal_connect (selection, "changed", G_CALLBACK (on_pv_tree_selection_changed), dialog);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_title (column, _("Physical Volume"));
        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", LINUX_LVM2_PV_ICON_COLUMN,
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
        gtk_tree_view_column_set_title (column, _("Capacity"));
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "markup", LINUX_LVM2_PV_SIZE_STRING_COLUMN,
                                             NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_title (column, _("Unallocated"));
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "markup", LINUX_LVM2_PV_UNALLOCATED_SIZE_STRING_COLUMN,
                                             NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), TRUE);

        /* -------------------------------------------------------------------------------- */

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        element = gdu_details_element_new (_("SMART Status:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->pv_smart_element = element;

        element = gdu_details_element_new (_("Device:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->pv_device_element = element;

        table = gdu_details_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

        /* -------------------------------------------------------------------------------- */

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        button_element = gdu_button_element_new (GTK_STOCK_NEW,
                                                 _("_New Physical Volume"),
                                                 _("Add a new PV to the VG"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_pv_new_button_clicked),
                          dialog);
        g_ptr_array_add (elements, button_element);
        dialog->priv->pv_new_button = button_element;

        button_element = gdu_button_element_new (GTK_STOCK_DELETE,
                                                 _("_Remove Physical Volume"),
                                                 _("Remove the PV from the VG"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_pv_remove_button_clicked),
                          dialog);
        g_ptr_array_add (elements, button_element);
        dialog->priv->pv_remove_button = button_element;

        table = gdu_button_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

        /* -------------------------------------------------------------------------------- */

        dialog->priv->changed_id = g_signal_connect (vg,
                                                     "changed",
                                                     G_CALLBACK (on_linux_lvm2_vg_changed),
                                                     dialog);
        dialog->priv->removed_id = g_signal_connect (vg,
                                                     "removed",
                                                     G_CALLBACK (on_linux_lvm2_vg_removed),
                                                     dialog);
        update_tree (dialog);
        update_details (dialog);

        g_free (name);
        g_free (vpd_name);

        /* select a sane size for the dialog and allow resizing */
        gtk_widget_set_size_request (GTK_WIDGET (dialog), 650, 450);
        gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

        if (G_OBJECT_CLASS (gdu_edit_linux_lvm2_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_edit_linux_lvm2_dialog_parent_class)->constructed (object);
}

/* ---------------------------------------------------------------------------------------------------- */

static GduDevice *
find_pv_by_uuid (GduPool     *pool,
                 const gchar *uuid)
{
        GduDevice *ret;
        GList *devices;
        GList *l;

        ret = NULL;

        devices = gdu_pool_get_devices (pool);
        for (l = devices; l != NULL; l = l->next) {
                GduDevice *d = GDU_DEVICE (l->data);

                if (gdu_device_should_ignore (d))
                        continue;

                if (gdu_device_is_linux_lvm2_pv (d) && g_strcmp0 (gdu_device_linux_lvm2_pv_get_uuid (d), uuid) == 0) {
                        ret = g_object_ref (d);
                        goto out;
                }
        }

 out:
        g_list_foreach (devices, (GFunc) g_object_unref, NULL);
        g_list_free (devices);
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static GduDevice *
get_selected_pv (GduEditLinuxLvm2Dialog *dialog)
{
        gchar *pv_uuid;
        GduPool *pool;
        GduDevice *pv;
        GtkTreeSelection *tree_selection;
        GtkTreeIter iter;

        pv_uuid = NULL;
        pv = NULL;
        pool = NULL;

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->pvs_tree_view));
        if (gtk_tree_selection_get_selected (tree_selection, NULL, &iter)) {
                gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->pvs_tree_store), &iter,
                                    LINUX_LVM2_PV_UUID_COLUMN,
                                    &pv_uuid,
                                    -1);
        }

        if (pv_uuid == NULL) {
                goto out;
        }

        pool = gdu_presentable_get_pool (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        pv = find_pv_by_uuid (pool, pv_uuid);

 out:
        g_free (pv_uuid);
        if (pool != NULL)
                g_object_unref (pool);

        return pv;
}

static gchar *
get_selected_pv_uuid (GduEditLinuxLvm2Dialog *dialog)
{
        gchar *pv_uuid;
         GtkTreeSelection *tree_selection;
        GtkTreeIter iter;

        pv_uuid = NULL;

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->pvs_tree_view));
        if (gtk_tree_selection_get_selected (tree_selection, NULL, &iter)) {
                gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->pvs_tree_store), &iter,
                                    LINUX_LVM2_PV_UUID_COLUMN,
                                    &pv_uuid,
                                    -1);
        }

         return pv_uuid;
}

/* ---------------------------------------------------------------------------------------------------- */

static gint
pv_tree_sort_func (GtkTreeModel *model,
                          GtkTreeIter  *a,
                          GtkTreeIter  *b,
                          gpointer      user_data)
{
        gint ret;
        gint a_position;
        gint b_position;
        gchar *a_uuid;
        gchar *b_uuid;

        gtk_tree_model_get (model,
                            a,
                            LINUX_LVM2_PV_UUID_COLUMN, &a_uuid,
                            LINUX_LVM2_PV_POSITION_COLUMN, &a_position,
                            -1);

        gtk_tree_model_get (model,
                            b,
                            LINUX_LVM2_PV_UUID_COLUMN, &b_uuid,
                            LINUX_LVM2_PV_POSITION_COLUMN, &b_position,
                            -1);

        if (a_position != b_position)
                ret = a_position - b_position;
        else
                ret = g_strcmp0 (a_uuid, b_uuid);

        g_free (a_uuid);
        g_free (b_uuid);
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
update_tree (GduEditLinuxLvm2Dialog *dialog)
{
        GduLinuxLvm2VolumeGroup *vg;
        GtkTreeStore *store;
        gchar *selected_pv_uuid;
        GtkTreeSelection *tree_selection;
        GtkTreeIter *iter_to_select;
        GduDevice *pv_device;
        GduPool *pool;
        gchar **pvs;
        guint n;

        pv_device = NULL;
        pool = NULL;

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->pvs_tree_view));

        iter_to_select = NULL;
        selected_pv_uuid = NULL;
        if (dialog->priv->pvs_tree_store != NULL) {
                GtkTreeIter iter;
                if (gtk_tree_selection_get_selected (tree_selection, NULL, &iter)) {
                        gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->pvs_tree_store), &iter,
                                            LINUX_LVM2_PV_UUID_COLUMN,
                                            &selected_pv_uuid,
                                            -1);
                }
                g_object_unref (dialog->priv->pvs_tree_store);
        }

        store = gtk_tree_store_new (LINUX_LVM2_PV_N_COLUMNS,
                                    G_TYPE_INT,
                                    G_TYPE_STRING,
                                    GDK_TYPE_PIXBUF,
                                    GDU_TYPE_VOLUME,
                                    GDU_TYPE_DRIVE,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING);
        dialog->priv->pvs_tree_store = store;

        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
                                         LINUX_LVM2_PV_UUID_COLUMN,
                                         pv_tree_sort_func,
                                         NULL,
                                         NULL);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                              LINUX_LVM2_PV_UUID_COLUMN,
                                              GTK_SORT_ASCENDING);


        vg = GDU_LINUX_LVM2_VOLUME_GROUP (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        if (vg == NULL)
                goto out;

        pv_device = gdu_linux_lvm2_volume_group_get_pv_device (vg);
        if (pv_device == NULL)
                goto out;

        pool = gdu_presentable_get_pool (GDU_PRESENTABLE (vg));

        pvs = gdu_device_linux_lvm2_pv_get_group_physical_volumes (pv_device);
        for (n = 0; pvs != NULL && pvs[n] != NULL; n++) {
                const gchar *pv_info = (const gchar *) pvs[n];
                gchar *pv_uuid;
                GduDevice *pv;
                GdkPixbuf *pixbuf;
                GduPresentable *volume_for_pv;
                GduPresentable *drive_for_pv;
                GtkTreeIter iter;
                guint64 allocated_size;
                guint64 size;
                gchar *size_str;
                gchar *unallocated_size_str;
                gchar *s;

                pv_uuid = NULL;
                pixbuf = NULL;
                volume_for_pv = NULL;
                drive_for_pv = NULL;
                size = 0;

                /* TODO: this is kind of a hack */
                pv_uuid = g_strdup (pv_info + 5); /* skip leading 'uuid=' */
                s = strstr (pv_uuid, ";");
                if (s != NULL)
                        *s = '\0';

                if (!gdu_linux_lvm2_volume_group_get_pv_info (vg,
                                                              pv_uuid,
                                                              NULL,
                                                              &size,
                                                              &allocated_size)) {
                        g_warning ("Unable to get information about PV with uuid `%s'", pv_uuid);
                        continue;
                }

                pv = find_pv_by_uuid (pool, pv_uuid);
                if (pv != NULL) {

                        volume_for_pv = gdu_pool_get_volume_by_device (pool, pv);

                        if (volume_for_pv == NULL) {
                                g_warning ("Cannot find volume for device `%s'", gdu_device_get_object_path (pv));
                                g_object_unref (pv);
                                continue;
                        }

                        if (gdu_device_is_partition (pv)) {
                                GduDevice *drive_device;
                                drive_device = gdu_pool_get_by_object_path (pool, gdu_device_partition_get_slave (pv));
                                if (drive_device != NULL) {
                                        drive_for_pv = gdu_pool_get_drive_by_device (pool, drive_device);
                                        g_object_unref (drive_device);
                                } else {
                                        g_warning ("No device with objpath %s", gdu_device_partition_get_slave (pv));
                                }
                        } else {
                                drive_for_pv = gdu_pool_get_drive_by_device (pool, pv);
                        }

                        pixbuf = gdu_util_get_pixbuf_for_presentable (volume_for_pv, GTK_ICON_SIZE_SMALL_TOOLBAR);

                        /* TODO: we can remove this once we export SIZE and ALLOCATED_SIZE */
                        if (size == G_MAXUINT64)
                                size = gdu_device_get_size (pv);

                        g_object_unref (pv);
                }

                if (size != G_MAXUINT64)
                        size_str = gdu_util_get_size_for_display (size, FALSE, FALSE);
                else
                        size_str = g_strdup ("?");

                if (size != G_MAXUINT64 && allocated_size != G_MAXUINT64)
                        unallocated_size_str = gdu_util_get_size_for_display (size - allocated_size, FALSE, FALSE);
                else
                        unallocated_size_str = g_strdup ("?");

                gtk_tree_store_append (store, &iter, NULL);
                gtk_tree_store_set (store,
                                    &iter,
                                    LINUX_LVM2_PV_POSITION_COLUMN, n,
                                    LINUX_LVM2_PV_ICON_COLUMN, pixbuf,
                                    LINUX_LVM2_PV_UUID_COLUMN, pv_uuid,
                                    LINUX_LVM2_PV_VOLUME_COLUMN, volume_for_pv,
                                    LINUX_LVM2_PV_DRIVE_COLUMN, drive_for_pv,
                                    LINUX_LVM2_PV_SIZE_STRING_COLUMN, size_str,
                                    LINUX_LVM2_PV_UNALLOCATED_SIZE_STRING_COLUMN, unallocated_size_str,
                                    -1);

                if (g_strcmp0 (pv_uuid, selected_pv_uuid) == 0) {
                        g_assert (iter_to_select == NULL);
                        iter_to_select = gtk_tree_iter_copy (&iter);
                }

                g_free (size_str);
                g_free (unallocated_size_str);
                if (pixbuf != NULL)
                        g_object_unref (pixbuf);
                if (volume_for_pv != NULL)
                        g_object_unref (volume_for_pv);
                if (drive_for_pv != NULL)
                        g_object_unref (drive_for_pv);

                g_free (pv_uuid);
        }

#if 0
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

                if (gdu_drive_is_active (GDU_DRIVE (linux_lvm2_drive))) {
                        position = gdu_device_linux_lvm2_pv_get_position (sd);
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

                slave_state_str = gdu_linux_lvm2_drive_get_slave_state_markup (linux_lvm2_drive, sd);
                if (slave_state_str == NULL)
                        slave_state_str = g_strdup ("–");

                gtk_tree_store_append (store, &iter, NULL);
                gtk_tree_store_set (store,
                                    &iter,
                                    LINUX_LVM2_SLAVE_VOLUME_COLUMN, volume_for_slave,
                                    LINUX_LVM2_SLAVE_DRIVE_COLUMN, drive_for_slave,
                                    LINUX_LVM2_POSITION_COLUMN, position,
                                    LINUX_LVM2_POSITION_STRING_COLUMN, position_str,
                                    LINUX_LVM2_PV_ICON_COLUMN, pixbuf,
                                    LINUX_LVM2_OBJPATH_COLUMN, object_path,
                                    LINUX_LVM2_SLAVE_STATE_STRING_COLUMN, slave_state_str,
                                    -1);

                if (g_strcmp0 (object_path, selected_pv_uuid) == 0) {
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
#endif

 out:
        if (pool != NULL)
                g_object_unref (pool);
        if (pv_device != NULL)
                g_object_unref (pv_device);

        gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->priv->pvs_tree_view), GTK_TREE_MODEL (store));

        /* Select the first iter if nothing is currently selected */
        if (iter_to_select == NULL) {
                GtkTreeIter iter;
                if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
                        iter_to_select = gtk_tree_iter_copy (&iter);
        }
        /* (Re-)select the pv */
        if (iter_to_select != NULL) {
                gtk_tree_selection_select_iter (tree_selection, iter_to_select);
                gtk_tree_iter_free (iter_to_select);
        }

        g_free (selected_pv_uuid);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
update_details (GduEditLinuxLvm2Dialog *dialog)
{
        GduLinuxLvm2VolumeGroup *vg;
        GduDevice *pv;
        GduDevice *slave_drive_device;
        const gchar *pv_device_file;
        gchar *s;
        gchar *s2;
        GIcon *icon;
        gboolean show_pv_new_button;
        gboolean show_pv_remove_button;
        //GduLinuxLvm2DriveSlaveFlags slave_flags;

        show_pv_new_button = TRUE; /* It's always possible to add a PV */
        show_pv_remove_button = TRUE; /* .. and it's always possible to remove a PV */

        slave_drive_device = NULL;

        pv = get_selected_pv (dialog);
        if (pv == NULL) {
                gdu_details_element_set_text (dialog->priv->pv_smart_element, "–");
                gdu_details_element_set_icon (dialog->priv->pv_smart_element, NULL);
                gdu_details_element_set_text (dialog->priv->pv_device_element, "–");
                goto out;
        }

        show_pv_remove_button = TRUE;

        vg = GDU_LINUX_LVM2_VOLUME_GROUP (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));

        if (gdu_device_is_partition (pv)) {
                GduPool *pool;
                pool = gdu_device_get_pool (pv);
                slave_drive_device = gdu_pool_get_by_object_path (pool, gdu_device_partition_get_slave (pv));
                g_object_unref (pool);
        } else {
                slave_drive_device = g_object_ref (pv);
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

                gdu_details_element_set_text (dialog->priv->pv_smart_element, s);
                gdu_details_element_set_icon (dialog->priv->pv_smart_element, icon);

                g_free (s);
                g_object_unref (icon);
        } else {
                /* Translators: Used when SMART is not supported on the RAID pv */
                gdu_details_element_set_text (dialog->priv->pv_smart_element, _("Not Supported"));
                icon = g_themed_icon_new ("gdu-smart-unknown");
                gdu_details_element_set_icon (dialog->priv->pv_smart_element, icon);
                g_object_unref (icon);
        }

        pv_device_file = gdu_device_get_device_file_presentation (pv);
        if (pv_device_file == NULL || strlen (pv_device_file) == 0)
                pv_device_file = gdu_device_get_device_file (pv);
        gdu_details_element_set_text (dialog->priv->pv_device_element, pv_device_file);

 out:
        gdu_button_element_set_visible (dialog->priv->pv_new_button, show_pv_new_button);
        gdu_button_element_set_visible (dialog->priv->pv_remove_button, show_pv_remove_button);

        if (pv != NULL)
                g_object_unref (pv);
        if (slave_drive_device != NULL)
                g_object_unref (slave_drive_device);
}

