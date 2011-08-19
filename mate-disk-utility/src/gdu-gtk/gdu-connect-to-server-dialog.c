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
#include <avahi-ui/avahi-ui.h>

#include "gdu-connect-to-server-dialog.h"

struct GduConnectToServerDialogPrivate
{
        GtkWidget *hostname_entry;
        GtkWidget *username_entry;
};

enum
{
        PROP_0,
        PROP_USER_NAME,
        PROP_ADDRESS,
};

G_DEFINE_TYPE (GduConnectToServerDialog, gdu_connect_to_server_dialog, GTK_TYPE_DIALOG);

static void gdu_connect_to_server_dialog_constructed (GObject *object);

static void
gdu_connect_to_server_dialog_finalize (GObject *object)
{
        //GduConnectToServerDialog *dialog = GDU_CONNECT_TO_SERVER_DIALOG (object);

        if (G_OBJECT_CLASS (gdu_connect_to_server_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_connect_to_server_dialog_parent_class)->finalize (object);
}

static void
gdu_connect_to_server_dialog_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
        GduConnectToServerDialog *dialog = GDU_CONNECT_TO_SERVER_DIALOG (object);

        switch (property_id) {
        case PROP_USER_NAME:
                g_value_take_string (value, gdu_connect_to_server_dialog_get_user_name (dialog));
                break;

        case PROP_ADDRESS:
                g_value_take_string (value, gdu_connect_to_server_dialog_get_address (dialog));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_connect_to_server_dialog_class_init (GduConnectToServerDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduConnectToServerDialogPrivate));

        object_class->get_property = gdu_connect_to_server_dialog_get_property;
        object_class->constructed  = gdu_connect_to_server_dialog_constructed;
        object_class->finalize     = gdu_connect_to_server_dialog_finalize;

        g_object_class_install_property (object_class,
                                         PROP_USER_NAME,
                                         g_param_spec_string ("user-name",
                                                              _("User Name"),
                                                              _("The chosen user name"),
                                                              NULL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_ADDRESS,
                                         g_param_spec_string ("address",
                                                              _("Address"),
                                                              _("The chosen address"),
                                                              NULL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));
}

static void
gdu_connect_to_server_dialog_init (GduConnectToServerDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, GDU_TYPE_CONNECT_TO_SERVER_DIALOG, GduConnectToServerDialogPrivate);
}

GtkWidget *
gdu_connect_to_server_dialog_new (GtkWindow *parent)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_CONNECT_TO_SERVER_DIALOG,
                                         "transient-for", parent,
                                         NULL));
}

/* ---------------------------------------------------------------------------------------------------- */

gchar *
gdu_connect_to_server_dialog_get_user_name (GduConnectToServerDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_CONNECT_TO_SERVER_DIALOG (dialog), NULL);
        return g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->priv->username_entry)));
}

gchar *
gdu_connect_to_server_dialog_get_address  (GduConnectToServerDialog *dialog)
{
        g_return_val_if_fail (GDU_IS_CONNECT_TO_SERVER_DIALOG (dialog), NULL);
        return g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->priv->hostname_entry)));
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_dns_sd_clicked (GtkButton *button,
                   gpointer   user_data)
{
        GduConnectToServerDialog *dialog = GDU_CONNECT_TO_SERVER_DIALOG (user_data);
        GtkWidget *service_dialog;
        gint response;

        service_dialog = aui_service_dialog_new (_("Choose Server"),
                                                 gtk_window_get_transient_for (GTK_WINDOW (dialog)),
                                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                 GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                 NULL);

        aui_service_dialog_set_browse_service_types (AUI_SERVICE_DIALOG (service_dialog),
                                                     "_udisks-ssh._tcp",
                                                     NULL);
        gtk_widget_show_all (service_dialog);
        response = gtk_dialog_run (GTK_DIALOG (service_dialog));

        if (response == GTK_RESPONSE_OK) {
                const gchar *hostname;
                hostname = aui_service_dialog_get_host_name (AUI_SERVICE_DIALOG (service_dialog));
                gtk_entry_set_text (GTK_ENTRY (dialog->priv->hostname_entry), hostname);
        }

        gtk_widget_destroy (service_dialog);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_hostname_entry_changed (GtkEditable *editable,
                           gpointer     user_data)
{
        GduConnectToServerDialog *dialog = GDU_CONNECT_TO_SERVER_DIALOG (user_data);
        gboolean sensitive;
        const gchar *text;

        /* TODO: potentially validate the host name? */

        sensitive = FALSE;
        text = gtk_entry_get_text (GTK_ENTRY (dialog->priv->hostname_entry));
        if (text != NULL && strlen (text) > 0)
                sensitive = TRUE;

        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           sensitive);
}

static void
gdu_connect_to_server_dialog_constructed (GObject *object)
{
        GduConnectToServerDialog *dialog = GDU_CONNECT_TO_SERVER_DIALOG (object);
        GtkWidget *content_area;
        GtkWidget *button;
        GtkWidget *label;
        GtkWidget *table;
        GtkWidget *entry;
        GtkWidget *vbox;
        GtkWidget *image;
        gint row;
        gchar *s;

        gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);

        gtk_window_set_title (GTK_WINDOW (dialog), _("Connect to Server"));

        gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
        button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                        GTK_STOCK_CONNECT,
                                        GTK_RESPONSE_OK);

        button = gtk_button_new_with_mnemonic (_("_Browse..."));
        image = gtk_image_new_from_stock (GTK_STOCK_NETWORK, GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image (GTK_BUTTON (button), image);
        gtk_box_pack_start (GTK_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dialog))),
                            button,
                            FALSE,
                            FALSE,
                            0);
        gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dialog))),
                                            button,
                                            TRUE);
        /* Translators: this is the tooltip for the "Browse..." button */
        gtk_widget_set_tooltip_text (button, _("Browse servers discovered via the DNS-SD protocol"));
        g_signal_connect (button,
                          "clicked",
                          G_CALLBACK (on_dns_sd_clicked),
                          dialog);

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

        vbox = content_area;
        gtk_box_set_spacing (GTK_BOX (vbox), 12);

        /* -------------------------------------------------------------------------------- */

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        /* Translators: This is the label explaining the dialog
         */
        gtk_label_set_markup (GTK_LABEL (label), _("To manage storage devices on another machine, enter the address and press “Connect”. The connection will be made using the <i>Secure Shell</i> protocol."));
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        /* -------------------------------------------------------------------------------- */

        table = gtk_table_new (3, 1, FALSE);
        gtk_table_set_col_spacings (GTK_TABLE (table), 12);
        gtk_table_set_row_spacings (GTK_TABLE (table), 6);
        gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

        row = 0;

        /* Translators: This is the tooltip for the "Server Address" entry
         */
        s = g_strdup (_("The hostname or address to connect to"));

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("Server _Address:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_widget_set_tooltip_text (label, s);

        entry = gtk_entry_new ();
        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, row, row + 1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
        dialog->priv->hostname_entry = entry;
        gtk_widget_set_tooltip_text (entry, s);
        gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
        g_free (s);

        row++;

        /* Translators: This is the tooltip for the "User Name" entry
         * First %s is the UNIX user name e.g. 'davidz'.
         */
        s = g_strdup (_("The user name to connect as"));

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), _("_User Name:"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_widget_set_tooltip_text (label, s);

        entry = gtk_entry_new ();
        gtk_entry_set_text (GTK_ENTRY (entry), g_get_user_name ());
        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, row, row + 1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 2);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
        dialog->priv->username_entry = entry;
        gtk_widget_set_tooltip_text (entry, s);
        gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
        g_free (s);

        row++;

        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           FALSE);
        g_signal_connect (dialog->priv->hostname_entry,
                          "changed",
                          G_CALLBACK (on_hostname_entry_changed),
                          dialog);

        if (G_OBJECT_CLASS (gdu_connect_to_server_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_connect_to_server_dialog_parent_class)->constructed (object);
}

