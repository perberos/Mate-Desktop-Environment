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

#include "gdu-create-linux-md-dialog.h"
#include "gdu-size-widget.h"
#include "gdu-disk-selection-widget.h"

struct GduCreateLinuxMdDialogPrivate
{
        GduPool *pool;

        GtkWidget *level_combo_box;
        GtkWidget *level_desc_label;

        GtkWidget *name_entry;

        GtkWidget *stripe_size_label;
        GtkWidget *stripe_size_combo_box;

        GtkWidget *size_label;
        GtkWidget *size_widget;

        GtkWidget *disk_selection_widget;

        GtkWidget *use_whole_disk_check_button;

        GtkWidget *tip_container;
        GtkWidget *tip_image;
        GtkWidget *tip_label;

        /* represents user selected options */
        gchar *level;
        guint num_disks_needed;
        guint stripe_size;

};

enum
{
        PROP_0,
        PROP_POOL,
        PROP_LEVEL,
        PROP_NAME,
        PROP_SIZE,
        PROP_COMPONENT_SIZE,
        PROP_STRIPE_SIZE,
        PROP_DRIVES,
};

static void gdu_create_linux_md_dialog_constructed (GObject *object);

static void update (GduCreateLinuxMdDialog *dialog);

static void get_sizes (GduCreateLinuxMdDialog *dialog,
                       guint                  *out_num_disks,
                       guint                  *out_num_available_disks,
                       guint64                *out_component_size,
                       guint64                *out_array_size,
                       guint64                *out_max_component_size,
                       guint64                *out_max_array_size);

G_DEFINE_TYPE (GduCreateLinuxMdDialog, gdu_create_linux_md_dialog, GTK_TYPE_DIALOG)

static void
gdu_create_linux_md_dialog_finalize (GObject *object)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (object);

        g_object_unref (dialog->priv->pool);
        g_free (dialog->priv->level);

        if (G_OBJECT_CLASS (gdu_create_linux_md_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_create_linux_md_dialog_parent_class)->finalize (object);
}

static void
gdu_create_linux_md_dialog_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (object);

        switch (property_id) {
        case PROP_POOL:
                g_value_set_object (value, dialog->priv->pool);
                break;

        case PROP_LEVEL:
                g_value_set_string (value, dialog->priv->level);
                break;

        case PROP_NAME:
                g_value_set_string (value, gdu_create_linux_md_dialog_get_name (dialog));
                break;

        case PROP_SIZE:
                g_value_set_uint64 (value, gdu_create_linux_md_dialog_get_size (dialog));
                break;

        case PROP_COMPONENT_SIZE:
                g_value_set_uint64 (value, gdu_create_linux_md_dialog_get_component_size (dialog));
                break;

        case PROP_STRIPE_SIZE:
                g_value_set_uint64 (value, gdu_create_linux_md_dialog_get_stripe_size (dialog));
                break;

        case PROP_DRIVES:
                g_value_set_boxed (value, gdu_create_linux_md_dialog_get_drives (dialog));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_create_linux_md_dialog_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (object);

        switch (property_id) {
        case PROP_POOL:
                dialog->priv->pool = g_value_dup_object (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_create_linux_md_dialog_class_init (GduCreateLinuxMdDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduCreateLinuxMdDialogPrivate));

        object_class->get_property = gdu_create_linux_md_dialog_get_property;
        object_class->set_property = gdu_create_linux_md_dialog_set_property;
        object_class->constructed  = gdu_create_linux_md_dialog_constructed;
        object_class->finalize     = gdu_create_linux_md_dialog_finalize;

        g_object_class_install_property (object_class,
                                         PROP_POOL,
                                         g_param_spec_object ("pool",
                                                              _("Pool"),
                                                              _("The pool of devices"),
                                                              GDU_TYPE_POOL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_LEVEL,
                                         g_param_spec_string ("level",
                                                              _("RAID Level"),
                                                              _("The selected RAID level"),
                                                              NULL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_NAME,
                                         g_param_spec_string ("name",
                                                              _("Name"),
                                                              _("The requested name for the array"),
                                                              NULL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_SIZE,
                                         g_param_spec_uint64 ("size",
                                                              _("Size"),
                                                              _("The requested size of the array"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_COMPONENT_SIZE,
                                         g_param_spec_uint64 ("component-size",
                                                              _("Component Size"),
                                                              _("The size of each component"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_STRIPE_SIZE,
                                         g_param_spec_uint64 ("stripe-size",
                                                              _("Stripe Size"),
                                                              _("The requested stripe size of the array"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_DRIVES,
                                         g_param_spec_boxed ("drives",
                                                             _("Drives"),
                                                             _("Array of drives to use for the array"),
                                                             G_TYPE_PTR_ARRAY,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_STATIC_NAME |
                                                             G_PARAM_STATIC_NICK |
                                                             G_PARAM_STATIC_BLURB));
}

static void
gdu_create_linux_md_dialog_init (GduCreateLinuxMdDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, GDU_TYPE_CREATE_LINUX_MD_DIALOG, GduCreateLinuxMdDialogPrivate);
}

GtkWidget *
gdu_create_linux_md_dialog_new (GtkWindow *parent,
                                GduPool   *pool)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_CREATE_LINUX_MD_DIALOG,
                                         "transient-for", parent,
                                         "pool", pool,
                                         NULL));
}

/* ---------------------------------------------------------------------------------------------------- */

gchar *
gdu_create_linux_md_dialog_get_level  (GduCreateLinuxMdDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_CREATE_LINUX_MD_DIALOG (dialog), NULL);
        return g_strdup (dialog->priv->level);
}

gchar *
gdu_create_linux_md_dialog_get_name (GduCreateLinuxMdDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_CREATE_LINUX_MD_DIALOG (dialog), NULL);
        return g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->priv->name_entry)));
}

guint64
gdu_create_linux_md_dialog_get_size (GduCreateLinuxMdDialog  *dialog)
{
        guint64 array_size;

        g_return_val_if_fail (GDU_IS_CREATE_LINUX_MD_DIALOG (dialog), 0);

        get_sizes (dialog,
                   NULL,  /* num_disks */
                   NULL,  /* num_available_disks */
                   NULL,  /* component_size */
                   &array_size,
                   NULL,  /* max_component_size */
                   NULL); /* max_array_size */

        return array_size;
}

guint64
gdu_create_linux_md_dialog_get_component_size (GduCreateLinuxMdDialog  *dialog)
{
        guint64 component_size;

        g_return_val_if_fail (GDU_IS_CREATE_LINUX_MD_DIALOG (dialog), 0);

        get_sizes (dialog,
                   NULL,  /* num_disks */
                   NULL,  /* num_available_disks */
                   &component_size,
                   NULL,  /* array_size */
                   NULL,  /* max_component_size */
                   NULL); /* max_array_size */

        return component_size;
}

guint64
gdu_create_linux_md_dialog_get_stripe_size (GduCreateLinuxMdDialog  *dialog)
{
        g_return_val_if_fail (GDU_IS_CREATE_LINUX_MD_DIALOG (dialog), 0);

        if (g_strcmp0 (dialog->priv->level, "raid1") == 0)
                return 0;
        else
                return dialog->priv->stripe_size;
}

GPtrArray *
gdu_create_linux_md_dialog_get_drives (GduCreateLinuxMdDialog  *dialog)
{
        g_return_val_if_fail (GDU_IS_CREATE_LINUX_MD_DIALOG (dialog), NULL);

        return gdu_disk_selection_widget_get_selected_drives (GDU_DISK_SELECTION_WIDGET (dialog->priv->disk_selection_widget));
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_level_combo_box_changed (GtkWidget *combo_box,
                            gpointer   user_data)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (user_data);
        gchar *s;
        gchar *markup;

        /* keep in sync with where combo box is constructed in constructed() */
        g_free (dialog->priv->level);
        switch (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box))) {
        case 0:
                dialog->priv->level = g_strdup ("raid0");
                dialog->priv->num_disks_needed = 2;
                break;
        case 1:
                dialog->priv->level = g_strdup ("raid1");
                dialog->priv->num_disks_needed = 2;
                break;
        case 2:
                dialog->priv->level = g_strdup ("raid5");
                dialog->priv->num_disks_needed = 3;
                break;
        case 3:
                dialog->priv->level = g_strdup ("raid6");
                dialog->priv->num_disks_needed = 4;
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        s = gdu_linux_md_get_raid_level_description (dialog->priv->level);
        markup = g_strconcat ("<small><i>",
                              s,
                              "</i></small>",
                              NULL);
        g_free (s);
        gtk_label_set_markup (GTK_LABEL (dialog->priv->level_desc_label), markup);
        g_free (markup);

        update (dialog);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_stripe_size_combo_box_changed (GtkWidget *combo_box,
                                  gpointer   user_data)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (user_data);

        /* keep in sync with where combo box is constructed in constructed() */
        switch (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box))) {
        case 0:
                dialog->priv->stripe_size = 4 * 1024;
                break;
        case 1:
                dialog->priv->stripe_size = 8 * 1024;
                break;
        case 2:
                dialog->priv->stripe_size = 16 * 1024;
                break;
        case 3:
                dialog->priv->stripe_size = 32 * 1024;
                break;
        case 4:
                dialog->priv->stripe_size = 64 * 1024;
                break;
        case 5:
                dialog->priv->stripe_size = 128 * 1024;
                break;
        case 6:
                dialog->priv->stripe_size = 256 * 1024;
                break;
        case 7:
                dialog->priv->stripe_size = 512 * 1024;
                break;
        case 8:
                dialog->priv->stripe_size = 1024 * 1024;
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        update (dialog);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_name_entry_activated (GtkWidget *combo_box,
                         gpointer   user_data)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (user_data);

        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_size_widget_changed (GduSizeWidget *size_widget,
                        gpointer       user_data)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (user_data);
        guint64 component_size;

        get_sizes (dialog,
                   NULL,
                   NULL,
                   &component_size,
                   NULL,
                   NULL,
                   NULL);

        gdu_disk_selection_widget_set_component_size (GDU_DISK_SELECTION_WIDGET (dialog->priv->disk_selection_widget),
                                                      component_size);

        update (dialog);
}

static void
on_disk_selection_widget_changed (GduDiskSelectionWidget *widget,
                                  gpointer                user_data)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (user_data);
        update (dialog);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_create_linux_md_dialog_constructed (GObject *object)
{
        GduCreateLinuxMdDialog *dialog = GDU_CREATE_LINUX_MD_DIALOG (object);
        GtkWidget *content_area;
        GtkWidget *action_area;
        GtkWidget *button;
        GtkWidget *label;
        GtkWidget *hbox;
        GtkWidget *image;
        GtkWidget *table;
        GtkWidget *entry;
        GtkWidget *combo_box;
        GtkWidget *vbox;
        GtkWidget *vbox2;
        GtkWidget *size_widget;
        gint row;
        gboolean ret;
        GtkWidget *align;
        gchar *s;
        GtkWidget *disk_selection_widget;
        GtkWidget *check_button;

        ret = FALSE;

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

        gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
        gtk_box_set_spacing (GTK_BOX (content_area), 0);
        gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
        gtk_box_set_spacing (GTK_BOX (action_area), 6);

        gtk_window_set_title (GTK_WINDOW (dialog), _("Create RAID Array"));
        gtk_window_set_icon_name (GTK_WINDOW (dialog), "gdu-raid-array");

        gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
        button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                        _("C_reate"),
                                        GTK_RESPONSE_OK);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

        gtk_container_set_border_width (GTK_CONTAINER (content_area), 10);

        vbox = content_area;
        gtk_box_set_spacing (GTK_BOX (vbox), 6);

        /* -------------------------------------------------------------------------------- */

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        s = g_strconcat ("<b>", _("General"), "</b>", NULL);
        gtk_label_set_markup (GTK_LABEL (label), s);
        g_free (s);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        vbox2 = gtk_vbox_new (FALSE, 12);
        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
        gtk_container_add (GTK_CONTAINER (align), vbox2);

        row = 0;

        table = gtk_table_new (2, 2, FALSE);
        gtk_table_set_col_spacings (GTK_TABLE (table), 12);
        gtk_table_set_row_spacings (GTK_TABLE (table), 6);
        gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

        /*  RAID level  */
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("RAID _Level:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        combo_box = gtk_combo_box_new_text ();
        dialog->priv->level_combo_box = combo_box;
        /* keep in sync with on_level_combo_box_changed() */
        s = gdu_linux_md_get_raid_level_for_display ("raid0", TRUE);
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), s);
        g_free (s);
        s = gdu_linux_md_get_raid_level_for_display ("raid1", TRUE);
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), s);
        g_free (s);
        s = gdu_linux_md_get_raid_level_for_display ("raid5", TRUE);
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), s);
        g_free (s);
        s = gdu_linux_md_get_raid_level_for_display ("raid6", TRUE);
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), s);
        g_free (s);
        g_signal_connect (combo_box,
                          "changed",
                          G_CALLBACK (on_level_combo_box_changed),
                          dialog);

        gtk_table_attach (GTK_TABLE (table), combo_box, 1, 2, row, row +1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo_box);
        row++;

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_table_attach (GTK_TABLE (table), label, 1, 2, row, row +1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        dialog->priv->level_desc_label = label;
        row++;

        /*  Array name  */
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("Array _Name:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        entry = gtk_entry_new ();
        /* Translators: This is the default name to use for the new array.
         * Keep length of UTF-8 representation of the translation of "New RAID Array" to less than
         * 32 bytes since that's the on-disk-format limit.
         */
        gtk_entry_set_text (GTK_ENTRY (entry), _("New RAID Array"));
        gtk_entry_set_max_length (GTK_ENTRY (entry), 32); /* on-disk-format restriction */
        g_signal_connect (entry,
                          "activate",
                          G_CALLBACK (on_name_entry_activated),
                          dialog);

        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, row, row + 1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
        dialog->priv->name_entry = entry;
        row++;

        /*  Stripe Size  */
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("Stripe S_ize:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        /* keep in sync with on_stripe_size_combo_box_changed() */
        combo_box = gtk_combo_box_new_text ();
        /* Translators: The following strings (4 KiB, 8 Kib, ..., 1 MiB) are for choosing the RAID stripe size.
         * Since the rest of mate-disk-utility use the sane 1k=1000 conventions and RAID needs the 1k=1024
         * convenention (this is because disk block sizes are powers of two) we resort to the nerdy units.
         */
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("4 KiB"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("8 KiB"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("16 KiB"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("32 KiB"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("64 KiB"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("128 KiB"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("256 KiB"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("512 KiB"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                   _("1 MiB"));
        /* keep in sync with gtk_combo_box_set_active() on stripe_size_combo_box below */
        dialog->priv->stripe_size = 512 * 1024;
        g_signal_connect (combo_box,
                          "changed",
                          G_CALLBACK (on_stripe_size_combo_box_changed),
                          dialog);

        gtk_table_attach (GTK_TABLE (table), combo_box, 1, 2, row, row +1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo_box);
        dialog->priv->stripe_size_label = label;
        dialog->priv->stripe_size_combo_box = combo_box;
        row++;

        /*  Array size  */
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("Array _Size:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);

        size_widget = gdu_size_widget_new (0,
                                           0,
                                           0);
        g_signal_connect (size_widget,
                          "changed",
                          G_CALLBACK (on_size_widget_changed),
                          dialog);
        gtk_table_attach (GTK_TABLE (table), size_widget, 1, 2, row, row + 1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        dialog->priv->size_label = label;
        dialog->priv->size_widget = size_widget;
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), size_widget);
        row++;

        /* -------------------------------------------------------------------------------- */

        disk_selection_widget = gdu_disk_selection_widget_new (dialog->priv->pool,
                                                               GDU_DISK_SELECTION_WIDGET_FLAGS_ALLOW_MULTIPLE |
                                                               GDU_DISK_SELECTION_WIDGET_FLAGS_ALLOW_DISKS_WITH_INSUFFICIENT_SPACE);
        dialog->priv->disk_selection_widget = disk_selection_widget;

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        s = g_strconcat ("<b>", _("_Disks"), "</b>", NULL);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), s);
        g_free (s);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), disk_selection_widget);

        vbox2 = gtk_vbox_new (FALSE, 12);
        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
        gtk_container_add (GTK_CONTAINER (align), vbox2);
        gtk_box_pack_start (GTK_BOX (vbox2), disk_selection_widget, TRUE, TRUE, 0);

        /* -------------------------------------------------------------------------------- */

        /* TODO: Actually make this work */
        check_button = gtk_check_button_new_with_mnemonic (_("Use entire disks instead of _partitions"));
        gtk_widget_set_tooltip_text (check_button, _("If checked, the entirety of each selected disk will be used for the RAID array. Otherwise partitions will be created."));
        dialog->priv->use_whole_disk_check_button = check_button;
#if 0
        gtk_box_pack_start (GTK_BOX (vbox2), check_button, FALSE, FALSE, 0);
#endif

        /* -------------------------------------------------------------------------------- */

        gtk_widget_grab_focus (dialog->priv->name_entry);
        gtk_editable_select_region (GTK_EDITABLE (dialog->priv->name_entry), 0, 1000);

        /* -------------------------------------------------------------------------------- */

        hbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

        image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU);
        gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

        dialog->priv->tip_container = hbox;
        dialog->priv->tip_image = image;
        dialog->priv->tip_label = label;
        gtk_widget_set_no_show_all (hbox, TRUE);

        /* Calls on_level_combo_box_changed() which calls update() */
        gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->level_combo_box), 0);
        /* keep in sync with "..stripe_size = 512 * 1024;" above */
        gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->stripe_size_combo_box), 7);

        /* select a sane size for the dialog and allow resizing */
        gtk_widget_set_size_request (GTK_WIDGET (dialog), 500, 550);
        gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

        g_signal_connect (disk_selection_widget,
                          "changed",
                          G_CALLBACK (on_disk_selection_widget_changed),
                          dialog);

        if (G_OBJECT_CLASS (gdu_create_linux_md_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_create_linux_md_dialog_parent_class)->constructed (object);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
get_sizes (GduCreateLinuxMdDialog *dialog,
           guint                  *out_num_disks,
           guint                  *out_num_available_disks,
           guint64                *out_component_size,
           guint64                *out_array_size,
           guint64                *out_max_component_size,
           guint64                *out_max_array_size)
{
        guint num_disks;
        guint num_available_disks;
        guint64 component_size;
        guint64 array_size;
        guint64 max_component_size;
        guint64 max_array_size;
        gdouble factor;
        GPtrArray *selected_disks;

        selected_disks = gdu_disk_selection_widget_get_selected_drives (GDU_DISK_SELECTION_WIDGET (dialog->priv->disk_selection_widget));

        num_disks = selected_disks->len;
        num_available_disks = gdu_disk_selection_widget_get_num_available_disks (GDU_DISK_SELECTION_WIDGET (dialog->priv->disk_selection_widget));
        max_component_size = gdu_disk_selection_widget_get_largest_segment_for_selected (GDU_DISK_SELECTION_WIDGET (dialog->priv->disk_selection_widget));

        //g_print ("num selected disks:           %d\n", num_disks);
        //g_print ("num available disks:          %d\n", num_available_disks);
        //g_print ("largest segment for selected: %" G_GUINT64_FORMAT "\n", max_component_size);

        factor = 0.0;
        if (num_disks > 1) {
                if (g_strcmp0 (dialog->priv->level, "raid0") == 0) {
                        factor = num_disks;
                } else if (g_strcmp0 (dialog->priv->level, "raid1") == 0) {
                        factor = 1.0;
                } else if (g_strcmp0 (dialog->priv->level, "raid5") == 0) {
                        if (num_disks > 1)
                                factor = num_disks - 1;
                } else if (g_strcmp0 (dialog->priv->level, "raid6") == 0) {
                        if (num_disks > 2)
                                factor = num_disks - 2;
                } else {
                        g_assert_not_reached ();
                }
        }
        max_array_size = max_component_size * factor;
        array_size = gdu_size_widget_get_size (GDU_SIZE_WIDGET (dialog->priv->size_widget));
        if (max_array_size > 0)
                component_size = max_component_size * ( ((gdouble) array_size) / ((gdouble) max_array_size) );
        else
                component_size = 0;

        if (out_num_disks != NULL)
                *out_num_disks = num_disks;

        if (out_num_available_disks != NULL)
                *out_num_available_disks = num_available_disks;

        if (out_component_size != NULL)
                *out_component_size = component_size;

        if (out_array_size != NULL)
                *out_array_size = array_size;

        if (out_max_component_size != NULL)
                *out_max_component_size = max_component_size;

        if (out_max_array_size != NULL)
                *out_max_array_size = max_array_size;

        g_ptr_array_unref (selected_disks);
}

static void
update (GduCreateLinuxMdDialog *dialog)
{
        gchar *tip_text;
        const gchar *tip_stock_icon;
        guint num_disks;
        guint num_available_disks;
        guint64 max_component_size;
        guint64 max_array_size;
        gboolean can_create;
        gchar *level_str;
        guint64 array_size;
        guint64 old_size;
        guint64 old_max_size;
        gboolean was_at_max;
        gboolean was_at_zero;

        tip_text = NULL;
        tip_stock_icon = NULL;
        can_create = FALSE;

        get_sizes (dialog,
                   &num_disks,
                   &num_available_disks,
                   NULL,  /* component_size */
                   &array_size,
                   &max_component_size,
                   &max_array_size);

        level_str = gdu_linux_md_get_raid_level_for_display (dialog->priv->level, FALSE);

        old_size = gdu_size_widget_get_size (GDU_SIZE_WIDGET (dialog->priv->size_widget));
        old_max_size = gdu_size_widget_get_max_size (GDU_SIZE_WIDGET (dialog->priv->size_widget));
        was_at_zero = (old_size == 0);
        was_at_max = (old_size == old_max_size);

        if (num_available_disks < dialog->priv->num_disks_needed) {
                gtk_widget_set_sensitive (dialog->priv->size_label, FALSE);
                gtk_widget_set_sensitive (dialog->priv->size_widget, FALSE);
                gdu_size_widget_set_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), 0);
                gdu_size_widget_set_min_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), 0);
                gdu_size_widget_set_max_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), 100);

                if (tip_text == NULL) {
                        /* Translators: This is for the tip text shown in the dialog.
                         * First %s is the short localized name for the RAID level, e.g. "RAID-1".
                         */
                        tip_text = g_strdup_printf (_("Insufficient number of disks to create a %s array."),
                                                    level_str);
                        tip_stock_icon = GTK_STOCK_DIALOG_ERROR;
                }

        } else if (num_disks < dialog->priv->num_disks_needed) {

                gtk_widget_set_sensitive (dialog->priv->size_label, FALSE);
                gtk_widget_set_sensitive (dialog->priv->size_widget, FALSE);
                gdu_size_widget_set_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), 0);
                gdu_size_widget_set_min_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), 0);
                gdu_size_widget_set_max_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), 100);

                if (tip_text == NULL) {
                        if (num_disks == 0) {
                                tip_text = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                                                       "To create a %s array, select a disk.",
                                                                       "To create a %s array, select %d disks.",
                                                                       dialog->priv->num_disks_needed - num_disks),
                                                            level_str,
                                                            dialog->priv->num_disks_needed - num_disks);
                        } else {
                                tip_text = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                                                       "To create a %s array, select one more disk.",
                                                                       "To create a %s array, select %d more disks.",
                                                                       dialog->priv->num_disks_needed - num_disks),
                                                            level_str,
                                                            dialog->priv->num_disks_needed - num_disks);
                        }
                        tip_stock_icon = GTK_STOCK_DIALOG_INFO;
                }

        } else {
                gtk_widget_set_sensitive (dialog->priv->size_label, TRUE);
                gtk_widget_set_sensitive (dialog->priv->size_widget, TRUE);
                gdu_size_widget_set_min_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), 0);
                gdu_size_widget_set_max_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), max_array_size);

                if (tip_text == NULL) {
                        gchar *strsize;

                        strsize = gdu_util_get_size_for_display (array_size, FALSE, FALSE);
                        /* Translators: This is for the tip text shown in the dialog.
                         * First %s is the size e.g. '42 GB'.
                         * Second %s is the short localized name for the RAID level, e.g. "RAID-1".
                         * Third parameter is the number of disks to use (always greater than 1).
                         */
                        tip_text = g_strdup_printf (_("To create a %s %s array on %d disks, press \"Create\""),
                                                    strsize,
                                                    level_str,
                                                    num_disks);
                        tip_stock_icon = GTK_STOCK_DIALOG_INFO;

                        can_create = TRUE;

                        g_free (strsize);
                }
        }

        /* Always go to max size if
         *
         *  - we were at size 0
         *  - we were at max and e.g. a disk was added
         */
        if (max_array_size > 0 &&
            (max_array_size > old_max_size && (was_at_zero || was_at_max)))
                gdu_size_widget_set_size (GDU_SIZE_WIDGET (dialog->priv->size_widget), max_array_size);

        if (g_strcmp0 (dialog->priv->level, "raid0") == 0 ||
            g_strcmp0 (dialog->priv->level, "raid5") == 0 ||
            g_strcmp0 (dialog->priv->level, "raid6") == 0) {
                gtk_widget_set_sensitive (dialog->priv->stripe_size_label, TRUE);
                gtk_widget_set_sensitive (dialog->priv->stripe_size_combo_box, TRUE);
        } else {
                gtk_widget_set_sensitive (dialog->priv->stripe_size_label, FALSE);
                gtk_widget_set_sensitive (dialog->priv->stripe_size_combo_box, FALSE);
        }


        if (tip_text != NULL) {
                gchar *s;
                s = g_strconcat ("<i>",
                                 tip_text,
                                 "</i>",
                                 NULL);
                gtk_label_set_markup (GTK_LABEL (dialog->priv->tip_label), s);
                g_free (s);
                gtk_image_set_from_stock (GTK_IMAGE (dialog->priv->tip_image),
                                          tip_stock_icon,
                                          GTK_ICON_SIZE_MENU);
                gtk_widget_show (dialog->priv->tip_container);
                gtk_widget_show (dialog->priv->tip_image);
                gtk_widget_show (dialog->priv->tip_label);
        } else {
                gtk_widget_hide (dialog->priv->tip_container);
                gtk_widget_hide (dialog->priv->tip_image);
                gtk_widget_hide (dialog->priv->tip_label);
        }

        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           can_create);

        g_free (level_str);
}
