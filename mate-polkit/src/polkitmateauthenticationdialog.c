/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "polkitmateauthenticationdialog.h"

#define RESPONSE_USER_SELECTED 1001

struct _PolkitMateAuthenticationDialogPrivate
{
  GtkWidget *user_combobox;
  GtkWidget *prompt_label;
  GtkWidget *password_entry;
  GtkWidget *auth_button;
  GtkWidget *cancel_button;
  GtkWidget *info_label;
  GtkWidget *table_alignment;

  gchar *message;
  gchar *action_id;
  gchar *vendor;
  gchar *vendor_url;
  gchar *icon_name;
  PolkitDetails *details;

  gchar **users;
  gchar *selected_user;

  gboolean is_running;

  GtkListStore *store;
};

G_DEFINE_TYPE (PolkitMateAuthenticationDialog, polkit_mate_authentication_dialog, GTK_TYPE_DIALOG);

enum {
  PROP_0,
  PROP_ACTION_ID,
  PROP_VENDOR,
  PROP_VENDOR_URL,
  PROP_ICON_NAME,
  PROP_MESSAGE,
  PROP_DETAILS,
  PROP_USERS,
  PROP_SELECTED_USER,
};

enum {
  PIXBUF_COL,
  TEXT_COL,
  USERNAME_COL,
  N_COL
};

static void
user_combobox_set_sensitive (GtkCellLayout   *cell_layout,
                             GtkCellRenderer *cell,
                             GtkTreeModel    *tree_model,
                             GtkTreeIter     *iter,
                             gpointer         user_data)
{
  GtkTreePath *path;
  gint *indices;
  gboolean sensitive;

  path = gtk_tree_model_get_path (tree_model, iter);
  indices = gtk_tree_path_get_indices (path);
  if (indices[0] == 0)
    sensitive = FALSE;
  else
    sensitive = TRUE;
  gtk_tree_path_free (path);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static void
user_combobox_changed (GtkComboBox *widget,
                       gpointer     user_data)
{
  PolkitMateAuthenticationDialog *dialog = POLKIT_MATE_AUTHENTICATION_DIALOG (user_data);
  GtkTreeIter iter;
  gchar *user_name;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter, USERNAME_COL, &user_name, -1);

      g_free (dialog->priv->selected_user);
      dialog->priv->selected_user = user_name;

      g_object_notify (G_OBJECT (dialog), "selected-user");

      gtk_dialog_response (GTK_DIALOG (dialog), RESPONSE_USER_SELECTED);

      /* make the password entry and Authenticate button sensitive again */
      gtk_widget_set_sensitive (dialog->priv->prompt_label, TRUE);
      gtk_widget_set_sensitive (dialog->priv->password_entry, TRUE);
      gtk_widget_set_sensitive (dialog->priv->auth_button, TRUE);
    }
}

static void
create_user_combobox (PolkitMateAuthenticationDialog *dialog)
{
  int n;
  GtkComboBox *combo;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;

  /* if we've already built the list of admin users once, then avoid
   * doing it again.. (this is mainly used when the user entered the
   * wrong password and the dialog is recycled)
   */
  if (dialog->priv->store != NULL)
    return;

  combo = GTK_COMBO_BOX (dialog->priv->user_combobox);
  dialog->priv->store = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

  gtk_list_store_append (dialog->priv->store, &iter);
  gtk_list_store_set (dialog->priv->store, &iter,
                      PIXBUF_COL, NULL,
                      TEXT_COL, _("Select user..."),
                      USERNAME_COL, NULL,
                      -1);


  /* For each user */
  for (n = 0; dialog->priv->users[n] != NULL; n++)
    {
      gchar *gecos;
      gchar *real_name;
      GdkPixbuf *pixbuf;
      struct passwd *passwd;

      /* we're single threaded so this is fine */
      errno = 0;
      passwd = getpwnam (dialog->priv->users[n]);
      if (passwd == NULL)
        {
          g_warning ("Error doing getpwnam(\"%s\"): %s", dialog->priv->users[n], strerror (errno));
          continue;
        }

      if (passwd->pw_gecos != NULL)
        gecos = g_locale_to_utf8 (passwd->pw_gecos, -1, NULL, NULL, NULL);
      else
        gecos = NULL;

      if (gecos != NULL && strlen (gecos) > 0)
        {
          gchar *first_comma;
          first_comma = strchr (gecos, ',');
          if (first_comma != NULL)
            *first_comma = '\0';
        }
      if (gecos != NULL && strlen (gecos) > 0 && strcmp (gecos, dialog->priv->users[n]) != 0)
        real_name = g_strdup_printf (_("%s (%s)"), gecos, dialog->priv->users[n]);
       else
         real_name = g_strdup (dialog->priv->users[n]);
      g_free (gecos);

      /* Load users face */
      pixbuf = NULL;
      if (passwd->pw_dir != NULL)
        {
          gchar *path;
          path = g_strdup_printf ("%s/.face", passwd->pw_dir);
          /* TODO: we probably shouldn't hard-code the size to 16x16 */
          pixbuf = gdk_pixbuf_new_from_file_at_scale (path, 16, 16, TRUE, NULL);
          g_free (path);
        }

      /* fall back to stock_person icon */
      if (pixbuf == NULL)
        {
          pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                             "stock_person",
                                             GTK_ICON_SIZE_MENU,
                                             0,
                                             NULL);
        }

      gtk_list_store_append (dialog->priv->store, &iter);
      gtk_list_store_set (dialog->priv->store, &iter,
                          PIXBUF_COL, pixbuf,
                          TEXT_COL, real_name,
                          USERNAME_COL, dialog->priv->users[n],
                          -1);

      g_free (real_name);
      g_object_unref (pixbuf);
    }

  gtk_combo_box_set_model (combo, GTK_TREE_MODEL (dialog->priv->store));

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
                                  renderer,
                                  "pixbuf", PIXBUF_COL,
                                  NULL);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo),
                                      renderer,
                                      user_combobox_set_sensitive,
                                      NULL, NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
                                  renderer,
                                  "text", TEXT_COL,
                                  NULL);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo),
                                      renderer,
                                      user_combobox_set_sensitive,
                                      NULL, NULL);

  /* Initially select the "Select user..." ... */
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

  /* Listen when a new user is selected */
  g_signal_connect (GTK_WIDGET (combo),
                    "changed",
                    G_CALLBACK (user_combobox_changed),
                    dialog);
}

static GtkWidget *
get_image (PolkitMateAuthenticationDialog *dialog)
{
  GdkPixbuf *pixbuf;
  GdkPixbuf *copy_pixbuf;
  GdkPixbuf *vendor_pixbuf;
  GtkWidget *image;

  pixbuf = NULL;
  copy_pixbuf = NULL;
  vendor_pixbuf = NULL;

  if (dialog->priv->icon_name == NULL || strlen (dialog->priv->icon_name) == 0)
    {
      image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
      goto out;
    }

  vendor_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                            dialog->priv->icon_name,
                                            48,
                                            0,
                                            NULL);
  if (vendor_pixbuf == NULL)
    {
      g_warning ("No icon for themed icon with name '%s'", dialog->priv->icon_name);
      image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
      goto out;
    }


  pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     GTK_STOCK_DIALOG_AUTHENTICATION,
                                     48,
                                     0,
                                     NULL);
  if (pixbuf == NULL)
    goto out;

  /* need to copy the pixbuf since we're modifying it */
  copy_pixbuf = gdk_pixbuf_copy (pixbuf);
  if (copy_pixbuf == NULL)
    goto out;

  /* blend the vendor icon in the bottom right quarter */
  gdk_pixbuf_composite (vendor_pixbuf,
                        copy_pixbuf,
                        24, 24, 24, 24,
                        24, 24, 0.5, 0.5,
                        GDK_INTERP_BILINEAR,
                        255);

  image = gtk_image_new_from_pixbuf (copy_pixbuf);

out:
  if (pixbuf != NULL)
    g_object_unref (pixbuf);

  if (copy_pixbuf != NULL)
    g_object_unref (copy_pixbuf);

  if (vendor_pixbuf != NULL)
    g_object_unref (vendor_pixbuf);

  return image;
}

static void
polkit_mate_authentication_dialog_set_property (GObject      *object,
                                                 guint         prop_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec)
{
  PolkitMateAuthenticationDialog *dialog = POLKIT_MATE_AUTHENTICATION_DIALOG (object);

  switch (prop_id)
    {
    case PROP_DETAILS:
      dialog->priv->details = g_value_dup_object (value);
      break;

    case PROP_ACTION_ID:
      dialog->priv->action_id = g_value_dup_string (value);
      break;

    case PROP_VENDOR:
      dialog->priv->vendor = g_value_dup_string (value);
      break;

    case PROP_VENDOR_URL:
      dialog->priv->vendor_url = g_value_dup_string (value);
      break;

    case PROP_ICON_NAME:
      dialog->priv->icon_name = g_value_dup_string (value);
      break;

    case PROP_MESSAGE:
      dialog->priv->message = g_value_dup_string (value);
      break;

    case PROP_USERS:
      dialog->priv->users = g_value_dup_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
polkit_mate_authentication_dialog_get_property (GObject    *object,
                                                 guint       prop_id,
                                                 GValue     *value,
                                                 GParamSpec *pspec)
{
  PolkitMateAuthenticationDialog *dialog = POLKIT_MATE_AUTHENTICATION_DIALOG (object);

  switch (prop_id)
    {
    case PROP_MESSAGE:
      g_value_set_string (value, dialog->priv->message);
      break;

    /* TODO: rest of the properties */

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static GtkWidget *
add_row (GtkWidget *table, int row, const char *label_text, GtkWidget *entry)
{
  GtkWidget *label;

  label = gtk_label_new_with_mnemonic (label_text);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  gtk_table_attach (GTK_TABLE (table), label,
                    0, 1, row, row + 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), entry,
                                   1, 2, row, row + 1);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  return label;
}

static void
action_id_activated (GtkLabel *url_label, gpointer user_data)
{
#if 0
  GError *error;
  DBusGConnection *bus;
  DBusGProxy *manager_proxy;

  error = NULL;
  bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (bus == NULL)
    {
      g_warning ("Couldn't connect to session bus: %s", error->message);
      g_error_free (error);
      goto out;
    }

  manager_proxy = dbus_g_proxy_new_for_name (bus,
                                             "org.mate.PolicyKit.AuthorizationManager",
                                             "/",
                                             "org.mate.PolicyKit.AuthorizationManager.SingleInstance");
  if (manager_proxy == NULL)
    {
      g_warning ("Could not construct manager_proxy object; bailing out");
      goto out;
    }

  if (!dbus_g_proxy_call (manager_proxy,
                          "ShowAction",
                          &error,
                          G_TYPE_STRING, gtk_label_get_current_uri (GTK_LABEL (url_label)),
                          G_TYPE_INVALID,
                          G_TYPE_INVALID))
    {
      if (error != NULL)
        {
          g_warning ("Failed to call into manager: %s", error->message);
          g_error_free (error);
        }
      else
        {
          g_warning ("Failed to call into manager");
        }
      goto out;
    }

out:
        ;
#endif
}

static void
polkit_mate_authentication_dialog_init (PolkitMateAuthenticationDialog *dialog)
{
  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                                   POLKIT_MATE_TYPE_AUTHENTICATION_DIALOG,
                                                   PolkitMateAuthenticationDialogPrivate);
}

static void
polkit_mate_authentication_dialog_finalize (GObject *object)
{
  PolkitMateAuthenticationDialog *dialog;

  dialog = POLKIT_MATE_AUTHENTICATION_DIALOG (object);

  g_free (dialog->priv->message);
  g_free (dialog->priv->action_id);
  g_free (dialog->priv->vendor);
  g_free (dialog->priv->vendor_url);
  g_free (dialog->priv->icon_name);
  if (dialog->priv->details != NULL)
    g_object_unref (dialog->priv->details);

  g_strfreev (dialog->priv->users);
  g_free (dialog->priv->selected_user);

  if (dialog->priv->store != NULL)
    g_object_unref (dialog->priv->store);

  if (G_OBJECT_CLASS (polkit_mate_authentication_dialog_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (polkit_mate_authentication_dialog_parent_class)->finalize (object);
}

static void
polkit_mate_authentication_dialog_constructed (GObject *object)
{
  PolkitMateAuthenticationDialog *dialog;
  GtkWidget *hbox;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *table_alignment;
  GtkWidget *table;
  GtkWidget *details_expander;
  GtkWidget *details_vbox;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *content_area;
  GtkWidget *action_area;
  gboolean have_user_combobox;
  gchar *s;
  guint rows;

  dialog = POLKIT_MATE_AUTHENTICATION_DIALOG (object);

  if (G_OBJECT_CLASS (polkit_mate_authentication_dialog_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (polkit_mate_authentication_dialog_parent_class)->constructed (object);

  have_user_combobox = FALSE;

  dialog->priv->cancel_button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                                            GTK_STOCK_CANCEL,
                                                            GTK_RESPONSE_CANCEL);
  dialog->priv->auth_button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                                          _("_Authenticate"),
                                                          GTK_RESPONSE_OK);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (content_area), 2); /* 2 * 5 + 2 = 12 */
  gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
  gtk_box_set_spacing (GTK_BOX (action_area), 6);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_DIALOG_AUTHENTICATION);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);

  image = get_image (dialog);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  main_vbox = gtk_vbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

  /* main message */
  label = gtk_label_new (NULL);
  s = g_strdup_printf ("<big><b>%s</b></big>", dialog->priv->message);
  gtk_label_set_markup (GTK_LABEL (label), s);
  g_free (s);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);

  /* secondary message */
  label = gtk_label_new (NULL);
  if (g_strv_length (dialog->priv->users) > 1)
    {
          gtk_label_set_markup (GTK_LABEL (label),
                                _("An application is attempting to perform an action that requires privileges. "
                                  "Authentication as one of the users below is required to perform this action."));
    }
  else
    {
      if (strcmp (g_get_user_name (), dialog->priv->users[0]) == 0)
        {
          gtk_label_set_markup (GTK_LABEL (label),
                                _("An application is attempting to perform an action that requires privileges. "
                                  "Authentication is required to perform this action."));
        }
      else
        {
          gtk_label_set_markup (GTK_LABEL (label),
                                _("An application is attempting to perform an action that requires privileges. "
                                  "Authentication as the super user is required to perform this action."));
        }
    }
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);

  /* user combobox */
  if (g_strv_length (dialog->priv->users) > 1)
    {
      dialog->priv->user_combobox = gtk_combo_box_new ();
      gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (dialog->priv->user_combobox), FALSE, FALSE, 0);

      create_user_combobox (dialog);

      have_user_combobox = TRUE;
    }
  else
    {
      dialog->priv->selected_user = g_strdup (dialog->priv->users[0]);
    }

  /* password entry */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

  table_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), table_alignment, FALSE, FALSE, 0);
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (table_alignment), table);
  dialog->priv->password_entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (dialog->priv->password_entry), FALSE);
  dialog->priv->prompt_label = add_row (table, 0, _("_Password:"), dialog->priv->password_entry);

  g_signal_connect_swapped (dialog->priv->password_entry, "activate",
                            G_CALLBACK (gtk_window_activate_default),
                            dialog);

  dialog->priv->table_alignment = table_alignment;
  /* initially never show the password entry stuff; we'll toggle it on/off so it's
   * only shown when prompting for a password */
  gtk_widget_set_no_show_all (dialog->priv->table_alignment, TRUE);

  /* A label for showing PAM_TEXT_INFO and PAM_TEXT_ERROR messages */
  label = gtk_label_new (NULL);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  dialog->priv->info_label = label;

  /* Details */
  details_expander = gtk_expander_new_with_mnemonic (_("<small><b>_Details</b></small>"));
  gtk_expander_set_use_markup (GTK_EXPANDER (details_expander), TRUE);
  gtk_box_pack_start (GTK_BOX (content_area), details_expander, FALSE, FALSE, 0);

  details_vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER (details_expander), details_vbox);

  table_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (details_vbox), table_alignment, FALSE, FALSE, 0);
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (table_alignment), table);

  /* TODO: sort keys? */
  rows = 0;
  if (dialog->priv->details != NULL)
    {
      guint n;
      gchar **keys;

      keys = polkit_details_get_keys (dialog->priv->details);
      for (n = 0; keys[n] != NULL; n++)
        {
          const gchar *key = keys[n];
          const gchar *value;

          value = polkit_details_lookup (dialog->priv->details, key);

          label = gtk_label_new (NULL);
          s = g_strdup_printf ("<small>%s</small>", value);
          gtk_label_set_markup (GTK_LABEL (label), s);
          g_free (s);
          gtk_misc_set_alignment (GTK_MISC (label), 0, 1.0);
          s = g_strdup_printf ("<small><b>%s:</b></small>", key);
          add_row (table, rows, s, label);
          g_free (s);

          rows++;
        }
      g_strfreev (keys);
    }

  /* --- */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  s = g_strdup_printf ("<small><a href=\"%s\">%s</a></small>",
                       dialog->priv->action_id,
                       dialog->priv->action_id);
  gtk_label_set_markup (GTK_LABEL (label), s);
  g_free (s);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 1.0);
  add_row (table, rows++, _("<small><b>Action:</b></small>"), label);
  g_signal_connect (label, "activate-link", G_CALLBACK (action_id_activated), NULL);

  s = g_strdup_printf (_("Click to edit %s"), dialog->priv->action_id);
  gtk_widget_set_tooltip_markup (label, s);
  g_free (s);

  /* --- */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  s = g_strdup_printf ("<small><a href=\"%s\">%s</a></small>",
                       dialog->priv->vendor_url,
                       dialog->priv->vendor);
  gtk_label_set_markup (GTK_LABEL (label), s);
  g_free (s);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 1.0);
  add_row (table, rows++, _("<small><b>Vendor:</b></small>"), label);

  s = g_strdup_printf (_("Click to open %s"), dialog->priv->vendor_url);
  gtk_widget_set_tooltip_markup (label, s);
  g_free (s);

  if (have_user_combobox)
    {
      /* ... and make the password entry and "Authenticate" button insensitive */
      gtk_widget_set_sensitive (dialog->priv->prompt_label, FALSE);
      gtk_widget_set_sensitive (dialog->priv->password_entry, FALSE);
      gtk_widget_set_sensitive (dialog->priv->auth_button, FALSE);
    }
  else
    {
    }

  gtk_widget_realize (GTK_WIDGET (dialog));

}

static void
polkit_mate_authentication_dialog_class_init (PolkitMateAuthenticationDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PolkitMateAuthenticationDialogPrivate));

  gobject_class->finalize = polkit_mate_authentication_dialog_finalize;
  gobject_class->get_property = polkit_mate_authentication_dialog_get_property;
  gobject_class->set_property = polkit_mate_authentication_dialog_set_property;
  gobject_class->constructed  = polkit_mate_authentication_dialog_constructed;

  g_object_class_install_property (gobject_class,
                                   PROP_DETAILS,
                                   g_param_spec_object ("details",
                                                        NULL,
                                                        NULL,
                                                        POLKIT_TYPE_DETAILS,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (gobject_class,
                                   PROP_ACTION_ID,
                                   g_param_spec_string ("action-id",
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (gobject_class,
                                   PROP_VENDOR,
                                   g_param_spec_string ("vendor",
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (gobject_class,
                                   PROP_VENDOR_URL,
                                   g_param_spec_string ("vendor-url",
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (gobject_class,
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


  g_object_class_install_property (gobject_class,
                                   PROP_MESSAGE,
                                   g_param_spec_string ("message",
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (gobject_class,
                                   PROP_USERS,
                                   g_param_spec_boxed ("users",
                                                       NULL,
                                                       NULL,
                                                       G_TYPE_STRV,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

  g_object_class_install_property (gobject_class,
                                   PROP_SELECTED_USER,
                                   g_param_spec_string ("selected-user",
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
}

/**
 * polkit_mate_authentication_dialog_new:
 *
 * Yada yada yada...
 *
 * Returns: A new password dialog.
 **/
GtkWidget *
polkit_mate_authentication_dialog_new (const gchar    *action_id,
                                        const gchar    *vendor,
                                        const gchar    *vendor_url,
                                        const gchar    *icon_name,
                                        const gchar    *message_markup,
                                        PolkitDetails  *details,
                                        gchar         **users)
{
  PolkitMateAuthenticationDialog *dialog;
  GtkWindow *window;

  dialog = g_object_new (POLKIT_MATE_TYPE_AUTHENTICATION_DIALOG,
                         "action-id", action_id,
                         "vendor", vendor,
                         "vendor-url", vendor_url,
                         "icon-name", icon_name,
                         "message", message_markup,
                         "details", details,
                         "users", users,
                         NULL);

  window = GTK_WINDOW (dialog);

  gtk_window_set_position (window, GTK_WIN_POS_CENTER);
  gtk_window_set_modal (window, TRUE);
  gtk_window_set_resizable (window, FALSE);
  gtk_window_set_keep_above (window, TRUE);
  gtk_window_set_title (window, _("Authenticate"));
  g_signal_connect (dialog, "close", G_CALLBACK (gtk_widget_hide), NULL);

  return GTK_WIDGET (dialog);
}

/**
 * polkit_mate_authentication_dialog_indicate_error:
 * @dialog: the auth dialog
 *
 * Call this function to indicate an authentication error; typically shakes the window
 **/
void
polkit_mate_authentication_dialog_indicate_error (PolkitMateAuthenticationDialog *dialog)
{
  int x, y;
  int n;
  int diff;

  /* TODO: detect compositing manager and do fancy stuff here */

  gtk_window_get_position (GTK_WINDOW (dialog), &x, &y);

  for (n = 0; n < 10; n++)
    {
      if (n % 2 == 0)
        diff = -15;
      else
        diff = 15;

      gtk_window_move (GTK_WINDOW (dialog), x + diff, y);

      while (gtk_events_pending ())
        {
          gtk_main_iteration ();
        }

      g_usleep (10000);
    }

  gtk_window_move (GTK_WINDOW (dialog), x, y);
}

/**
 * polkit_mate_authentication_dialog_run_until_user_is_selected:
 * @dialog: A #PolkitMateAuthenticationDialog.
 *
 * Runs @dialog in a recursive main loop until a user have been selected.
 *
 * If there is only one element in the the users array (which is set upon construction) or
 * an user has already been selected, this function returns immediately with the return
 * value %TRUE.
 *
 * Returns: %TRUE if a user is selected (use polkit_mate_dialog_get_selected_user() to obtain the user) or
 *          %FALSE if the dialog was cancelled.
 **/
gboolean
polkit_mate_authentication_dialog_run_until_user_is_selected (PolkitMateAuthenticationDialog *dialog)
{
  gboolean ret;
  gint response;

  ret = FALSE;

  if (dialog->priv->selected_user != NULL)
    {
      ret = TRUE;
      goto out;
    }

  dialog->priv->is_running = TRUE;

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  dialog->priv->is_running = FALSE;

  if (response == RESPONSE_USER_SELECTED)
    ret = TRUE;

 out:
  return ret;
}

/**
 * polkit_mate_authentication_dialog_run_until_response_for_prompt:
 * @dialog: A #PolkitMateAuthenticationDialog.
 * @prompt: The prompt to present the user with.
 * @echo_chars: Whether characters should be echoed in the password entry box.
 * @was_cancelled: Set to %TRUE if the dialog was cancelled.
 * @new_user_selected: Set to %TRUE if another user was selected.
 *
 * Runs @dialog in a recursive main loop until a response to @prompt have been obtained from the user.
 *
 * Returns: The response (free with g_free()) or %NULL if one of @was_cancelled or @new_user_selected
 *          has been set to %TRUE.
 **/
gchar *
polkit_mate_authentication_dialog_run_until_response_for_prompt (PolkitMateAuthenticationDialog *dialog,
                                                                  const gchar           *prompt,
                                                                  gboolean               echo_chars,
                                                                  gboolean              *was_cancelled,
                                                                  gboolean              *new_user_selected)
{
  gint response;
  gchar *ret;

  gtk_label_set_text_with_mnemonic (GTK_LABEL (dialog->priv->prompt_label), prompt);
  gtk_entry_set_visibility (GTK_ENTRY (dialog->priv->password_entry), echo_chars);
  gtk_entry_set_text (GTK_ENTRY (dialog->priv->password_entry), "");
  gtk_widget_grab_focus (dialog->priv->password_entry);

  ret = NULL;

  if (was_cancelled != NULL)
    *was_cancelled = FALSE;

  if (new_user_selected != NULL)
    *new_user_selected = FALSE;

  dialog->priv->is_running = TRUE;

  gtk_widget_set_no_show_all (dialog->priv->table_alignment, FALSE);
  gtk_widget_show_all (dialog->priv->table_alignment);

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_hide_all (dialog->priv->table_alignment);
  gtk_widget_set_no_show_all (dialog->priv->table_alignment, TRUE);

  dialog->priv->is_running = FALSE;

  if (response == GTK_RESPONSE_OK)
    {
      ret = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->priv->password_entry)));
    }
  else if (response == RESPONSE_USER_SELECTED)
    {
      if (new_user_selected != NULL)
        *new_user_selected = TRUE;
    }
  else
    {
      if (was_cancelled != NULL)
        *was_cancelled = TRUE;
    }

  return ret;
}

/**
 * polkit_mate_authentication_dialog_get_selected_user:
 * @dialog: A #PolkitMateAuthenticationDialog.
 *
 * Gets the currently selected user.
 *
 * Returns: The currently selected user (free with g_free()) or %NULL if no user is currently selected.
 **/
gchar *
polkit_mate_authentication_dialog_get_selected_user (PolkitMateAuthenticationDialog *dialog)
{
  return g_strdup (dialog->priv->selected_user);
}

void
polkit_mate_authentication_dialog_set_info_message (PolkitMateAuthenticationDialog *dialog,
                                                     const gchar                     *info_markup)
{
  gtk_label_set_markup (GTK_LABEL (dialog->priv->info_label), info_markup);
}


/**
 * polkit_mate_authentication_dialog_cancel:
 * @dialog: A #PolkitMateAuthenticationDialog.
 *
 * Cancels the dialog if it is currenlty running.
 *
 * Returns: %TRUE if the dialog was running.
 **/
gboolean
polkit_mate_authentication_dialog_cancel (PolkitMateAuthenticationDialog *dialog)
{
  if (!dialog->priv->is_running)
    return FALSE;

  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

  return TRUE;
}
