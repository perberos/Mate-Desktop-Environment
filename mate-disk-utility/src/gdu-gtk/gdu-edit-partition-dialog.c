/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-edit-partition-dialog.c
 *
 * Copyright (C) 2009 David Zeuthen
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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <atasmart.h>
#include <glib/gstdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "gdu-gtk.h"
#include "gdu-edit-partition-dialog.h"

/* ---------------------------------------------------------------------------------------------------- */

struct GduEditPartitionDialogPrivate
{
        GtkWidget *part_label_entry;
        GtkWidget *part_type_combo_box;
        GtkWidget *part_flag_boot_check_button;
        GtkWidget *part_flag_required_check_button;
};

enum {
        PROP_0,
        PROP_PARTITION_LABEL,
        PROP_PARTITION_TYPE,
        PROP_PARTITION_FLAGS,
};

G_DEFINE_TYPE (GduEditPartitionDialog, gdu_edit_partition_dialog, GDU_TYPE_DIALOG)

static void
gdu_edit_partition_dialog_finalize (GObject *object)
{
        //GduEditPartitionDialog *dialog = GDU_EDIT_PARTITION_DIALOG (object);

        if (G_OBJECT_CLASS (gdu_edit_partition_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_edit_partition_dialog_parent_class)->finalize (object);
}

static void
gdu_edit_partition_dialog_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
        GduEditPartitionDialog *dialog = GDU_EDIT_PARTITION_DIALOG (object);

        switch (property_id) {
        case PROP_PARTITION_LABEL:
                g_value_take_string (value, gdu_edit_partition_dialog_get_partition_label (dialog));
                break;

        case PROP_PARTITION_TYPE:
                g_value_take_string (value, gdu_edit_partition_dialog_get_partition_type (dialog));
                break;

        case PROP_PARTITION_FLAGS:
                g_value_take_boxed (value, gdu_edit_partition_dialog_get_partition_flags (dialog));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
has_flag (char **flags, const char *flag)
{
        int n;

        n = 0;
        while (flags != NULL && flags[n] != NULL) {
                if (strcmp (flags[n], flag) == 0)
                        return TRUE;
                n++;
        }
        return FALSE;
}

static void
update_apply_sensitivity (GduEditPartitionDialog *dialog)
{
        gboolean label_differ;
        gboolean type_differ;
        gboolean flags_differ;
        char *selected_type;
        GduDevice *device;
        char **flags;

        device = gdu_dialog_get_device (GDU_DIALOG (dialog));

        label_differ = FALSE;
        type_differ = FALSE;
        flags_differ = FALSE;

        if (strcmp (gdu_device_partition_get_scheme (device), "gpt") == 0 ||
            strcmp (gdu_device_partition_get_scheme (device), "apm") == 0) {
                if (strcmp (gdu_device_partition_get_label (device),
                            gtk_entry_get_text (GTK_ENTRY (dialog->priv->part_label_entry))) != 0) {
                        label_differ = TRUE;
                }
        }

        flags = gdu_device_partition_get_flags (device);
        if (has_flag (flags, "boot") !=
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->part_flag_boot_check_button)))
                flags_differ = TRUE;
        if (has_flag (flags, "required") !=
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->part_flag_required_check_button)))
                flags_differ = TRUE;

        selected_type = gdu_util_part_type_combo_box_get_selected (dialog->priv->part_type_combo_box);
        if (selected_type != NULL && strcmp (gdu_device_partition_get_type (device), selected_type) != 0) {
                type_differ = TRUE;
        }
        g_free (selected_type);

        if (label_differ || type_differ || flags_differ) {
                gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY, TRUE);
        } else {
                gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY, FALSE);
        }
}

static void
update (GduEditPartitionDialog *dialog)
{
        gboolean show_flag_boot;
        gboolean show_flag_required;
        gboolean can_edit_part_label;
        GduDevice *device;
        const char *scheme;
        guint64 size;
        char **flags;

        device = gdu_dialog_get_device (GDU_DIALOG (dialog));

        size = gdu_device_partition_get_size (device);
        scheme = gdu_device_partition_get_scheme (device);

        gdu_util_part_type_combo_box_rebuild (dialog->priv->part_type_combo_box, scheme);
        gdu_util_part_type_combo_box_select (dialog->priv->part_type_combo_box,
                                             gdu_device_partition_get_type (device));

        can_edit_part_label = FALSE;
        show_flag_boot = FALSE;
        show_flag_required = FALSE;

        if (strcmp (scheme, "mbr") == 0) {
                show_flag_boot = TRUE;
        }

        if (strcmp (scheme, "gpt") == 0) {
                can_edit_part_label = TRUE;
                show_flag_required = TRUE;
        }

        if (strcmp (scheme, "apm") == 0) {
                can_edit_part_label = TRUE;
                show_flag_boot = TRUE;
        }

        if (show_flag_boot)
                gtk_widget_show (dialog->priv->part_flag_boot_check_button);
        else
                gtk_widget_hide (dialog->priv->part_flag_boot_check_button);

        if (show_flag_required)
                gtk_widget_show (dialog->priv->part_flag_required_check_button);
        else
                gtk_widget_hide (dialog->priv->part_flag_required_check_button);

        flags = gdu_device_partition_get_flags (device);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->part_flag_boot_check_button),
                                      has_flag (flags, "boot"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->part_flag_required_check_button),
                                      has_flag (flags, "required"));

        gtk_widget_set_sensitive (dialog->priv->part_label_entry, can_edit_part_label);
        if (can_edit_part_label) {
                gtk_entry_set_text (GTK_ENTRY (dialog->priv->part_label_entry),
                                    gdu_device_partition_get_label (device));
                /* TODO: check real max length */
                gtk_entry_set_max_length (GTK_ENTRY (dialog->priv->part_label_entry), 31);
        } else {
                gtk_entry_set_text (GTK_ENTRY (dialog->priv->part_label_entry), "");
        }

        update_apply_sensitivity (dialog);
}


/* ---------------------------------------------------------------------------------------------------- */

static void
part_type_combo_box_changed (GtkWidget *combo_box,
                             gpointer   user_data)
{
        GduEditPartitionDialog *dialog = GDU_EDIT_PARTITION_DIALOG (user_data);
        update_apply_sensitivity (dialog);
}

static void
part_label_entry_changed (GtkWidget *combo_box,
                          gpointer   user_data)
{
        GduEditPartitionDialog *dialog = GDU_EDIT_PARTITION_DIALOG (user_data);
        update_apply_sensitivity (dialog);
}

static void
part_flag_check_button_clicked (GtkWidget *check_button,
                                gpointer   user_data)
{
        GduEditPartitionDialog *dialog = GDU_EDIT_PARTITION_DIALOG (user_data);
        update_apply_sensitivity (dialog);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_edit_partition_dialog_constructed (GObject *object)
{
        GduEditPartitionDialog *dialog = GDU_EDIT_PARTITION_DIALOG (object);
        GtkWidget *content_area;
        GtkWidget *hbox;
        GtkWidget *vbox;
        GtkWidget *image;
        GtkWidget *label;
        gchar *s;
        gchar *s2;
        GIcon *icon;
        GduPresentable *p;
        GduDevice *d;
        GduPool *pool;
        GduPresentable *drive;
        GduDevice *drive_device;

        gtk_container_set_border_width (GTK_CONTAINER (dialog), 12);

        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               GTK_STOCK_CANCEL,
                               GTK_RESPONSE_CANCEL);

        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               GTK_STOCK_APPLY,
                               GTK_RESPONSE_APPLY);

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

        icon = gdu_presentable_get_icon (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));

        hbox = gtk_hbox_new (FALSE, 12);
        gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);

        image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
        gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

        vbox = gtk_vbox_new (FALSE, 12);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

        p = gdu_dialog_get_presentable (GDU_DIALOG (dialog));
        d = gdu_presentable_get_device (p);
        pool = gdu_presentable_get_pool (p);
        drive_device = gdu_pool_get_by_object_path (pool, gdu_device_partition_get_slave (d));
        drive = gdu_pool_get_drive_by_device (pool, drive_device);

        s2 = gdu_presentable_get_vpd_name (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        /* Translators: title of the dialog - first %s is the name of the volume
         * e.g. "Partition 1 of ATA INTEL SSDSA2MH080G1GC"
         */
        s = g_strdup_printf (_("Edit %s"), s2);
        gtk_window_set_title (GTK_WINDOW (dialog), s);
        g_free (s);
        g_free (s2);

        /* --- */

        GtkWidget *table;
        GtkWidget *entry;
        GtkWidget *combo_box;
        GtkWidget *check_button;
        gint row;

        table = gtk_table_new (2, 2, FALSE);
        gtk_table_set_col_spacings (GTK_TABLE (table), 12);
        gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

        row = 0;

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("Part_ition Label:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        entry = gtk_entry_new ();
        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
        dialog->priv->part_label_entry = entry;

        row++;

        /* --- */

        /* partition type */
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        /* Translators: 'Type' means partition type here */
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("Ty_pe:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        combo_box = gdu_util_part_type_combo_box_create (NULL);
        gtk_table_attach (GTK_TABLE (table), combo_box, 1, 2, row, row +1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo_box);
        dialog->priv->part_type_combo_box = combo_box;

        row++;

        /* --- */

        /* flag used by mbr, apm */
        check_button = gtk_check_button_new_with_mnemonic (_("_Bootable"));
        gtk_table_attach (GTK_TABLE (table), check_button, 0, 2, row, row +1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        dialog->priv->part_flag_boot_check_button = check_button;

        row++;

        /* flag used by gpt */
        check_button = gtk_check_button_new_with_mnemonic (_("Required / Firm_ware"));
        gtk_table_attach (GTK_TABLE (table), check_button, 0, 2, row, row +1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        dialog->priv->part_flag_required_check_button = check_button;

        g_signal_connect (dialog->priv->part_type_combo_box,
                          "changed",
                          G_CALLBACK (part_type_combo_box_changed),
                          dialog);
        g_signal_connect (dialog->priv->part_label_entry,
                          "changed",
                          G_CALLBACK (part_label_entry_changed),
                          dialog);
        g_signal_connect (dialog->priv->part_flag_boot_check_button,
                          "toggled",
                          G_CALLBACK (part_flag_check_button_clicked),
                          dialog);
        g_signal_connect (dialog->priv->part_flag_required_check_button,
                          "toggled",
                          G_CALLBACK (part_flag_check_button_clicked),
                          dialog);

        /* control visibility of check buttons */
        gtk_widget_set_no_show_all (dialog->priv->part_flag_boot_check_button, TRUE);
        gtk_widget_set_no_show_all (dialog->priv->part_flag_required_check_button, TRUE);

        g_object_unref (icon);
        g_object_unref (d);
        g_object_unref (drive_device);
        g_object_unref (drive);
        g_object_unref (pool);

        update (dialog);

        if (G_OBJECT_CLASS (gdu_edit_partition_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_edit_partition_dialog_parent_class)->constructed (object);
}

static void
gdu_edit_partition_dialog_class_init (GduEditPartitionDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduEditPartitionDialogPrivate));

        object_class->get_property = gdu_edit_partition_dialog_get_property;
        object_class->constructed  = gdu_edit_partition_dialog_constructed;
        object_class->finalize     = gdu_edit_partition_dialog_finalize;

        g_object_class_install_property (object_class,
                                         PROP_PARTITION_LABEL,
                                         g_param_spec_string ("partition-label",
                                                              NULL,
                                                              NULL,
                                                              NULL,
                                                              G_PARAM_READABLE));

        g_object_class_install_property (object_class,
                                         PROP_PARTITION_TYPE,
                                         g_param_spec_string ("partition-type",
                                                              NULL,
                                                              NULL,
                                                              NULL,
                                                              G_PARAM_READABLE));


        g_object_class_install_property (object_class,
                                         PROP_PARTITION_FLAGS,
                                         g_param_spec_boxed ("partition-flags",
                                                             NULL,
                                                             NULL,
                                                             G_TYPE_STRV,
                                                             G_PARAM_READABLE));

}

static void
gdu_edit_partition_dialog_init (GduEditPartitionDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, GDU_TYPE_EDIT_PARTITION_DIALOG, GduEditPartitionDialogPrivate);
}

GtkWidget *
gdu_edit_partition_dialog_new (GtkWindow      *parent,
                               GduPresentable *presentable)
{
        g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);
        return GTK_WIDGET (g_object_new (GDU_TYPE_EDIT_PARTITION_DIALOG,
                                         "transient-for", parent,
                                         "presentable", presentable,
                                         NULL));
}

gchar *
gdu_edit_partition_dialog_get_partition_type  (GduEditPartitionDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_EDIT_PARTITION_DIALOG (dialog), NULL);
        return gdu_util_part_type_combo_box_get_selected (dialog->priv->part_type_combo_box);
}

gchar *
gdu_edit_partition_dialog_get_partition_label (GduEditPartitionDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_EDIT_PARTITION_DIALOG (dialog), NULL);
        return g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->priv->part_label_entry)));
}

gchar **
gdu_edit_partition_dialog_get_partition_flags (GduEditPartitionDialog *dialog)
{
        GPtrArray *p;

        g_return_val_if_fail (GDU_IS_EDIT_PARTITION_DIALOG (dialog), NULL);

        p = g_ptr_array_new ();
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->part_flag_boot_check_button)))
                g_ptr_array_add (p, g_strdup ("boot"));
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->part_flag_required_check_button)))
                g_ptr_array_add (p, g_strdup ("required"));
        g_ptr_array_add (p, NULL);

        return (gchar **) g_ptr_array_free (p, FALSE);
}


/* ---------------------------------------------------------------------------------------------------- */
