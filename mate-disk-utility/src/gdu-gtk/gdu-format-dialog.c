/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  format-window.c
 *
 *  Copyright (C) 2008-2009 Red Hat, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Tomas Bzatek <tbzatek@redhat.com>
 *          David Zeuthen <davidz@redhat.com>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <gdu/gdu.h>
#include <gdu-gtk/gdu-gtk.h>

#include "gdu-format-dialog.h"

struct GduFormatDialogPrivate
{
        GduFormatDialogFlags flags;
        gchar *affirmative_button_mnemonic;

        GtkWidget *table;

        GtkWidget *fs_label_entry;
        GtkWidget *fs_type_combo_box;
        GtkWidget *fs_type_desc_label;
        GtkWidget *take_ownership_check_button;
        GtkWidget *encrypt_check_button;

        gchar *fs_type;
        gchar **fs_options;
        gboolean encrypt;
        gboolean take_ownership;

        gulong removed_signal_handler_id;
};

enum
{
        PROP_0,
        PROP_FLAGS,
        PROP_FS_TYPE,
        PROP_FS_OPTIONS,
        PROP_FS_LABEL,
        PROP_ENCRYPT,
        PROP_TAKE_OWNERSHIP,
        PROP_AFFIRMATIVE_BUTTON_MNEMONIC,
};

static void gdu_format_dialog_constructed (GObject *object);

G_DEFINE_TYPE (GduFormatDialog, gdu_format_dialog, GDU_TYPE_DIALOG)

static void
gdu_format_dialog_finalize (GObject *object)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (object);

        g_free (dialog->priv->fs_type);
        g_strfreev (dialog->priv->fs_options);
        g_free (dialog->priv->affirmative_button_mnemonic);

        if (dialog->priv->removed_signal_handler_id > 0) {
                g_signal_handler_disconnect (gdu_dialog_get_presentable (GDU_DIALOG (dialog)),
                                             dialog->priv->removed_signal_handler_id);
        }

        if (G_OBJECT_CLASS (gdu_format_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_format_dialog_parent_class)->finalize (object);
}

static void
gdu_format_dialog_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (object);

        switch (property_id) {
        case PROP_FLAGS:
                g_value_set_flags (value, dialog->priv->flags);
                break;

        case PROP_FS_TYPE:
                g_value_take_string (value, dialog->priv->fs_type);
                break;

        case PROP_FS_LABEL:
                g_value_take_string (value, gdu_format_dialog_get_fs_label (dialog));
                break;

        case PROP_FS_OPTIONS:
                g_value_take_boxed (value, gdu_format_dialog_get_fs_options (dialog));
                break;

        case PROP_ENCRYPT:
                g_value_set_boolean (value, gdu_format_dialog_get_encrypt (dialog));
                break;

        case PROP_TAKE_OWNERSHIP:
                g_value_set_boolean (value, gdu_format_dialog_get_take_ownership (dialog));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_format_dialog_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (object);

        switch (property_id) {
        case PROP_FLAGS:
                dialog->priv->flags = g_value_get_flags (value);
                break;

        case PROP_AFFIRMATIVE_BUTTON_MNEMONIC:
                dialog->priv->affirmative_button_mnemonic = g_value_dup_string (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_format_dialog_class_init (GduFormatDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduFormatDialogPrivate));

        object_class->get_property = gdu_format_dialog_get_property;
        object_class->set_property = gdu_format_dialog_set_property;
        object_class->constructed  = gdu_format_dialog_constructed;
        object_class->finalize     = gdu_format_dialog_finalize;

        g_object_class_install_property (object_class,
                                         PROP_FLAGS,
                                         g_param_spec_flags ("flags",
                                                             _("Flags"),
                                                             _("Flags specifying the behavior of the dialog"),
                                                             GDU_TYPE_FORMAT_DIALOG_FLAGS,
                                                             GDU_FORMAT_DIALOG_FLAGS_NONE,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT_ONLY |
                                                             G_PARAM_STATIC_NAME |
                                                             G_PARAM_STATIC_NICK |
                                                             G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_FS_TYPE,
                                         g_param_spec_string ("fs-type",
                                                              _("Filesystem type"),
                                                              _("The selected filesystem type"),
                                                              NULL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_FS_LABEL,
                                         g_param_spec_string ("fs-label",
                                                              _("Filesystem label"),
                                                              _("The requested filesystem label"),
                                                              NULL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_FS_OPTIONS,
                                         g_param_spec_boxed ("fs-options",
                                                             _("Filesystem options"),
                                                             _("The options to use for creating the filesystem"),
                                                             G_TYPE_STRV,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_STATIC_NAME |
                                                             G_PARAM_STATIC_NICK |
                                                             G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_ENCRYPT,
                                         g_param_spec_boolean ("encrypt",
                                                               _("Encryption"),
                                                               _("Whether the volume should be encrypted"),
                                                               FALSE,
                                                               G_PARAM_READABLE |
                                                               G_PARAM_STATIC_NAME |
                                                               G_PARAM_STATIC_NICK |
                                                               G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_TAKE_OWNERSHIP,
                                         g_param_spec_boolean ("take-ownership",
                                                               _("Take Ownership"),
                                                               _("Whether the filesystem should be owned by the user"),
                                                               FALSE,
                                                               G_PARAM_READABLE |
                                                               G_PARAM_STATIC_NAME |
                                                               G_PARAM_STATIC_NICK |
                                                               G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_AFFIRMATIVE_BUTTON_MNEMONIC,
                                         g_param_spec_string ("affirmative-button-mnemonic",
                                                              _("Affirmative Button Mnemonic"),
                                                              _("The mnemonic label for the affirmative button"),
                                                              /* Translators: Format is used as a verb here */
                                                              _("_Format"),
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));
}

static void
gdu_format_dialog_init (GduFormatDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                                    GDU_TYPE_FORMAT_DIALOG,
                                                    GduFormatDialogPrivate);

        dialog->priv->table = gtk_table_new (2, 2, FALSE);
}

GtkWidget *
gdu_format_dialog_new (GtkWindow            *parent,
                       GduPresentable       *presentable,
                       GduFormatDialogFlags  flags)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_FORMAT_DIALOG,
                                         "transient-for", parent,
                                         "presentable", presentable,
                                         "flags", flags,
                                         NULL));
}

GtkWidget *
gdu_format_dialog_new_for_drive (GtkWindow            *parent,
                                 GduDevice            *device,
                                 GduFormatDialogFlags  flags)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_FORMAT_DIALOG,
                                         "transient-for", parent,
                                         "drive-device", device,
                                         "flags", flags,
                                         NULL));
}

GtkWidget *
gdu_format_dialog_new_for_volume (GtkWindow            *parent,
                                  GduDevice            *device,
                                  GduFormatDialogFlags  flags)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_FORMAT_DIALOG,
                                         "transient-for", parent,
                                         "volume-device", device,
                                         "flags", flags,
                                         NULL));
}

/* ---------------------------------------------------------------------------------------------------- */

gchar *
gdu_format_dialog_get_fs_type  (GduFormatDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_FORMAT_DIALOG (dialog), NULL);
        return g_strdup (dialog->priv->fs_type);
}

gchar *
gdu_format_dialog_get_fs_label (GduFormatDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_FORMAT_DIALOG (dialog), NULL);
        return g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->priv->fs_label_entry)));
}

gchar **
gdu_format_dialog_get_fs_options (GduFormatDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_FORMAT_DIALOG (dialog), NULL);
        return g_strdupv (dialog->priv->fs_options);
}

gboolean
gdu_format_dialog_get_encrypt  (GduFormatDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_FORMAT_DIALOG (dialog), FALSE);
        return dialog->priv->encrypt;
}

gboolean
gdu_format_dialog_get_take_ownership  (GduFormatDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_FORMAT_DIALOG (dialog), FALSE);
        return dialog->priv->take_ownership;
}

GtkWidget *
gdu_format_dialog_get_table (GduFormatDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_FORMAT_DIALOG (dialog), NULL);
        return dialog->priv->table;
}


/* ---------------------------------------------------------------------------------------------------- */

static void
update (GduFormatDialog *dialog)
{
        GduKnownFilesystem *kfs;
        gint max_label_len;

        /* keep in sync with where combo box is constructed in constructed() */
        if (dialog->priv->flags & GDU_FORMAT_DIALOG_FLAGS_SIMPLE) {
                g_free (dialog->priv->fs_type);
                switch (gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->fs_type_combo_box))) {
                case 0:
                        dialog->priv->fs_type = g_strdup ("vfat");
                        dialog->priv->encrypt = FALSE;
                        dialog->priv->take_ownership = FALSE;
                        break;
                case 1:
                        dialog->priv->fs_type = g_strdup ("ext2");
                        dialog->priv->encrypt = FALSE;
                        dialog->priv->take_ownership = TRUE;
                        break;
                case 2:
                        dialog->priv->fs_type = g_strdup ("ext4");
                        dialog->priv->encrypt = FALSE;
                        dialog->priv->take_ownership = TRUE;
                        break;
                case 3:
                        dialog->priv->fs_type = g_strdup ("vfat");
                        dialog->priv->encrypt = TRUE;
                        dialog->priv->take_ownership = FALSE;
                        break;
                default:
                        g_assert_not_reached ();
                        break;
                }
        } else {
                dialog->priv->fs_type = gdu_util_fstype_combo_box_get_selected (dialog->priv->fs_type_combo_box);
                dialog->priv->encrypt = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->encrypt_check_button));
                dialog->priv->take_ownership = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->take_ownership_check_button));
        }

        max_label_len = 0;
        kfs = gdu_pool_get_known_filesystem_by_id (gdu_dialog_get_pool (GDU_DIALOG (dialog)), dialog->priv->fs_type);
        if (kfs != NULL) {
                max_label_len = gdu_known_filesystem_get_max_label_len (kfs);
                g_object_unref (kfs);

                if (! (dialog->priv->flags & GDU_FORMAT_DIALOG_FLAGS_SIMPLE)) {
                        if (gdu_known_filesystem_get_supports_unix_owners (kfs)) {
                                gtk_widget_show (dialog->priv->take_ownership_check_button);
                        } else {
                                gtk_widget_hide (dialog->priv->take_ownership_check_button);
                                dialog->priv->take_ownership = FALSE;
                        }
                }
        } else {
                gtk_widget_hide (dialog->priv->take_ownership_check_button);
                dialog->priv->take_ownership = FALSE;
        }
        gtk_entry_set_max_length (GTK_ENTRY (dialog->priv->fs_label_entry), max_label_len);
        if (max_label_len == 0) {
                gtk_widget_set_sensitive (dialog->priv->fs_label_entry, FALSE);
                gtk_entry_set_text (GTK_ENTRY (dialog->priv->fs_label_entry), "");
        } else {
                gtk_widget_set_sensitive (dialog->priv->fs_label_entry, TRUE);
        }

        if (! (dialog->priv->flags & GDU_FORMAT_DIALOG_FLAGS_SIMPLE)) {
                if (g_strcmp0 (dialog->priv->fs_type, "empty") == 0 ||
                    g_strcmp0 (dialog->priv->fs_type, "msdos_extended_partition") == 0) {
                        gtk_widget_hide (dialog->priv->encrypt_check_button);
                        dialog->priv->encrypt = FALSE;
                } else {
                        gtk_widget_show (dialog->priv->encrypt_check_button);
                }
        }
}

static void
on_combo_box_changed (GtkWidget *combo_box,
                      gpointer   user_data)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (user_data);
        update (dialog);
}

static void
on_take_ownership_check_button_toggled (GtkToggleButton *toggle_button,
                                        gpointer         user_data)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (user_data);
        update (dialog);
}

static void
on_encrypt_check_button_toggled (GtkToggleButton *toggle_button,
                                 gpointer         user_data)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (user_data);
        update (dialog);
}

static void
on_fs_label_entry_activated (GtkWidget *combo_box,
                             gpointer   user_data)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (user_data);

        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_presentable_removed (GduPresentable *presentable,
                        gpointer        user_data)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (user_data);

        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_format_dialog_constructed (GObject *object)
{
        GduFormatDialog *dialog = GDU_FORMAT_DIALOG (object);
        GtkWidget *content_area;
        GtkWidget *action_area;
        GtkWidget *button;
        GtkWidget *icon;
        GtkWidget *label;
        GtkWidget *hbox;
        GtkWidget *image;
        GtkWidget *table;
        GtkWidget *entry;
        GtkWidget *combo_box;
        GtkWidget *vbox2;
        GdkPixbuf *pixbuf;
        gint row;
        gboolean ret;
        GtkWidget *align;
        gchar *s;
        gchar *s2;
        GtkWidget *check_button;

        ret = FALSE;

        pixbuf = gdu_util_get_pixbuf_for_presentable (gdu_dialog_get_presentable (GDU_DIALOG (dialog)),
                                                      GTK_ICON_SIZE_DIALOG);

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

        gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
        gtk_box_set_spacing (GTK_BOX (content_area), 0);
        gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
        gtk_box_set_spacing (GTK_BOX (action_area), 6);

        gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

        s2 = gdu_presentable_get_vpd_name (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        /* Translators: Format is a verb here. The %s is the name of the volume */
        s = g_strdup_printf (_("Format %s"), s2);
        gtk_window_set_title (GTK_WINDOW (dialog), s);
        g_free (s);
        g_free (s2);

        gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               dialog->priv->affirmative_button_mnemonic,
                               GTK_RESPONSE_OK);

        if (dialog->priv->flags & GDU_FORMAT_DIALOG_FLAGS_DISK_UTILITY_BUTTON) {
                button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("Disk _Utility"), GTK_RESPONSE_ACCEPT);
                icon = gtk_image_new_from_icon_name ("palimpsest", GTK_ICON_SIZE_BUTTON);
                gtk_button_set_image (GTK_BUTTON (button), icon);
                gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dialog))),
                                                    button,
                                                    TRUE);
                /* Translators: this is the tooltip for the "Disk Utility" button */
                gtk_widget_set_tooltip_text (button, _("Use Disk Utility to format volume"));
        }
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

        gtk_container_set_border_width (GTK_CONTAINER (content_area), 10);

        /*  icon and text labels  */
        hbox = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, TRUE, 0);

        image = gtk_image_new_from_pixbuf (pixbuf);
        gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 12);

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 12, 0, 0);
        gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);

        vbox2 = gtk_vbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (align), vbox2);

        table = dialog->priv->table;
        g_object_get (table,
                      "n-rows", &row,
                      NULL);
        gtk_table_set_col_spacings (GTK_TABLE (table), 12);
        gtk_table_set_row_spacings (GTK_TABLE (table), 6);
        gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

        /*  filesystem type  */
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        /* Translators: 'type' means 'filesystem type' here. */
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("_Type:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        if (dialog->priv->flags & GDU_FORMAT_DIALOG_FLAGS_SIMPLE) {
                /* keep in sync with on_combo_box_changed() */
                combo_box = gtk_combo_box_new_text ();
                gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                           _("Compatible with all systems (FAT)"));
                gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                           _("Compatible with Linux (ext2)"));
                gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                           _("Compatible with Linux (ext4)"));
                gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box),
                                           _("Encrypted, compatible with Linux (FAT)"));
                gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);
                dialog->priv->fs_type = g_strdup ("vfat");
                dialog->priv->fs_options = NULL;
                dialog->priv->encrypt = FALSE;
                dialog->priv->take_ownership = FALSE;
        } else {
                const gchar *include_extended_partitions_for_scheme;

                include_extended_partitions_for_scheme = NULL;
                if (dialog->priv->flags & GDU_FORMAT_DIALOG_FLAGS_ALLOW_MSDOS_EXTENDED)
                        include_extended_partitions_for_scheme = "mbr";
                combo_box = gdu_util_fstype_combo_box_create (gdu_dialog_get_pool (GDU_DIALOG (dialog)),
                                                              include_extended_partitions_for_scheme);
                gdu_util_fstype_combo_box_select (combo_box, "ext4");
                dialog->priv->fs_type = g_strdup ("ext4");
                dialog->priv->fs_options = NULL;
                dialog->priv->encrypt = FALSE;
                dialog->priv->take_ownership = TRUE;
        }
        gtk_table_attach (GTK_TABLE (table), combo_box, 1, 2, row, row +1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo_box);
        dialog->priv->fs_type_combo_box = combo_box;
        row++;

        if (! (dialog->priv->flags & GDU_FORMAT_DIALOG_FLAGS_SIMPLE)) {
                label = gtk_label_new (NULL);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
                gtk_table_attach (GTK_TABLE (table), label, 1, 2, row, row +1,
                                  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
                dialog->priv->fs_type_desc_label = label;
                row++;
                gdu_util_fstype_combo_box_set_desc_label (dialog->priv->fs_type_combo_box,
                                                          dialog->priv->fs_type_desc_label);
        }


        /*  filesystem label  */
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        /* Translators: 'name' means 'filesystem label' here. */
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("_Name:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        entry = gtk_entry_new ();
        /* Translators: Default name to use for the formatted volume. Keep length of
         * translation of "New Volume" to less than 16 characters.
         */
        gtk_entry_set_text (GTK_ENTRY (entry), _("New Volume"));
        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, row, row + 1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
        dialog->priv->fs_label_entry = entry;
        row++;

        g_signal_connect (dialog->priv->fs_type_combo_box,
                          "changed",
                          G_CALLBACK (on_combo_box_changed),
                          dialog);

        g_signal_connect (dialog->priv->fs_label_entry,
                          "activate",
                          G_CALLBACK (on_fs_label_entry_activated),
                          dialog);

        if (! (dialog->priv->flags & GDU_FORMAT_DIALOG_FLAGS_SIMPLE)) {
                check_button = gtk_check_button_new_with_mnemonic (_("T_ake ownership of filesystem"));
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
                gtk_widget_set_tooltip_text (check_button,
                                             _("The selected file system has a concept of file ownership. "
                                               "If checked, the created file system will be owned by you. "
                                               "If not checked, only the super user can access the file system."));
                gtk_box_pack_start (GTK_BOX (vbox2), check_button, FALSE, FALSE, 0);
                dialog->priv->take_ownership_check_button = check_button;

                check_button = gtk_check_button_new_with_mnemonic (_("_Encrypt underlying device"));
                gtk_widget_set_tooltip_text (check_button,
                                             _("Encryption protects your data, requiring a "
                                               "passphrase to be entered before the file system can be "
                                               "used. May decrease performance and may not be compatible if "
                                               "you use the media on other operating systems."));
                gtk_box_pack_start (GTK_BOX (vbox2), check_button, FALSE, FALSE, 0);
                dialog->priv->encrypt_check_button = check_button;

                /* visibility is controlled in update() */
                gtk_widget_set_no_show_all (dialog->priv->take_ownership_check_button, TRUE);
                gtk_widget_set_no_show_all (dialog->priv->encrypt_check_button, TRUE);

                g_signal_connect (dialog->priv->take_ownership_check_button,
                                  "toggled",
                                  G_CALLBACK (on_take_ownership_check_button_toggled),
                                  dialog);

                g_signal_connect (dialog->priv->encrypt_check_button,
                                  "toggled",
                                  G_CALLBACK (on_encrypt_check_button_toggled),
                                  dialog);

                /* TODO: we want to also expose fs create options */
        }

        update (dialog);

#if 0
        hbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
        image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	label = gtk_label_new (NULL);
        s = g_strconcat ("<i>",
                         _("Warning: All data on the volume will be irrevocably lost."),
                         "</i>",
                         NULL);
        gtk_label_set_markup (GTK_LABEL (label), s);
        g_free (s);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
#endif

        /* nuke dialog if device is yanked */
        dialog->priv->removed_signal_handler_id = g_signal_connect (gdu_dialog_get_presentable (GDU_DIALOG (dialog)),
                                                                    "removed",
                                                                    G_CALLBACK (on_presentable_removed),
                                                                    dialog);

        gtk_widget_grab_focus (dialog->priv->fs_label_entry);
        gtk_editable_select_region (GTK_EDITABLE (dialog->priv->fs_label_entry), 0, 1000);

        if (G_OBJECT_CLASS (gdu_format_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_format_dialog_parent_class)->constructed (object);
}

/* ---------------------------------------------------------------------------------------------------- */
