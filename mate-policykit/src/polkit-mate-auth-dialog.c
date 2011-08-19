/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 *
 * Copyright (C) 2007 David Zeuthen <david@fubar.dk>
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
#include <mateconf/mateconf-client.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "sexy-url-label.h"
#include "polkit-mate-auth-dialog.h"

struct _PolkitMateAuthDialogPrivate
{
	GtkWidget *keep_privilege_check_button;
	GtkWidget *keep_privilege_session_only_check_button;
	GtkWidget *message_label;
	GtkWidget *message_label_secondary;
	GtkWidget *user_combobox;
	GtkWidget *app_desc_label;
	GtkWidget *privilege_desc_label;
	GtkWidget *privilege_vendor_label;
	GtkWidget *prompt_label;
        GtkWidget *password_entry;
	GtkWidget *icon;
	char *message;

	char *vendor;
	char *vendor_url;

	gboolean can_do_always;
	gboolean can_do_session;

	GtkListStore *store;
};

G_DEFINE_TYPE (PolkitMateAuthDialog, polkit_mate_auth_dialog, GTK_TYPE_DIALOG);

enum {
	USER_SELECTED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
	PROP_0,
	PROP_PROGRAM,
	PROP_ACTION_ID,
	PROP_VENDOR,
	PROP_VENDOR_URL,
	PROP_ICON_NAME,
	PROP_MESSAGE,
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


void
polkit_mate_auth_dialog_select_admin_user (PolkitMateAuthDialog *auth_dialog, const char *admin_user)
{
	GtkTreeIter iter;
	gboolean done;
	gboolean valid;

	/* make the password and "Keep.." widgets sensitive again */
	gtk_widget_set_sensitive (auth_dialog->priv->prompt_label, TRUE);
	gtk_widget_set_sensitive (auth_dialog->priv->password_entry, TRUE);
	gtk_widget_set_sensitive (auth_dialog->priv->keep_privilege_check_button, TRUE);
	gtk_widget_set_sensitive (auth_dialog->priv->keep_privilege_session_only_check_button, TRUE);

	/* select appropriate item in combobox */
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (auth_dialog->priv->store), &iter);
	done = FALSE;
	while (valid && !done) {
		char *str_data;
		gtk_tree_model_get (GTK_TREE_MODEL (auth_dialog->priv->store), 
				    &iter, 
				    USERNAME_COL, &str_data,
				    -1);
		if (str_data != NULL && strcmp (admin_user, str_data) == 0) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (auth_dialog->priv->user_combobox),
						       &iter);
			done = TRUE;
		}
		g_free (str_data);
      
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (auth_dialog->priv->store), &iter);
	}
}

static void
user_combobox_changed (GtkComboBox *widget,
		       gpointer     user_data)
{
	char *user_name;
	PolkitMateAuthDialog *auth_dialog = POLKIT_MATE_AUTH_DIALOG (user_data);
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (auth_dialog->priv->store), &iter, USERNAME_COL, &user_name, -1);
		g_signal_emit (auth_dialog, signals[USER_SELECTED], 0, user_name);

		polkit_mate_auth_dialog_select_admin_user (auth_dialog, user_name);

		g_free (user_name);
	}
}

static void
create_user_combobox (PolkitMateAuthDialog *auth_dialog, char **admin_users)
{
	int n;
	GtkComboBox *combo;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;

	/* if we've already built the list of admin users once, then avoid
	 * doing it again.. (this is mainly used when the user entered the
	 * wrong password and the dialog is recycled)
	 */
	if (auth_dialog->priv->store != NULL)
		return;

	combo = GTK_COMBO_BOX (auth_dialog->priv->user_combobox);
	auth_dialog->priv->store = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

	gtk_list_store_append (auth_dialog->priv->store, &iter);
	gtk_list_store_set (auth_dialog->priv->store, &iter,
			    PIXBUF_COL, NULL,
			    TEXT_COL, _("Select user..."),
			    USERNAME_COL, NULL,
			    -1);
       	

	/* For each user */
	for (n = 0; admin_users[n] != NULL; n++) {
		char *real_name;
		GdkPixbuf *pixbuf;
		struct passwd *passwd;

		/* we're single threaded so this is fine */
		errno = 0;
		passwd = getpwnam (admin_users[n]);
		if (passwd == NULL) {
			g_warning ("Error doing getpwnam(\"%s\"): %s", admin_users[n], strerror (errno));
			continue;
		}

		/* Real name */
		if (passwd->pw_gecos != NULL && strlen (passwd->pw_gecos) > 0)
			real_name = g_strdup_printf (_("%s (%s)"), passwd->pw_gecos, admin_users[n]);
		else
			real_name = g_strdup (admin_users[n]);

		/* Load users face */
		pixbuf = NULL;
		if (passwd->pw_dir != NULL) {
			char *path;
			path = g_strdup_printf ("%s/.face", passwd->pw_dir);
			/* TODO: we probably shouldn't hard-code the size to 24x24*/
			pixbuf = gdk_pixbuf_new_from_file_at_scale (path, 24, 24, TRUE, NULL);
			g_free (path);
		}

		/* fall back to stock_person icon */
		if (pixbuf == NULL)
			pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
							   "stock_person", GTK_ICON_SIZE_MENU, 0, NULL);

		gtk_list_store_append (auth_dialog->priv->store, &iter);
		gtk_list_store_set (auth_dialog->priv->store, &iter,
				    PIXBUF_COL, pixbuf,
				    TEXT_COL, real_name,
				    USERNAME_COL, admin_users[n],
				    -1);

		g_free (real_name);
		g_object_unref (pixbuf);

	}
	
	gtk_combo_box_set_model (combo, GTK_TREE_MODEL (auth_dialog->priv->store));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
					"pixbuf", PIXBUF_COL,
					NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo),
					    renderer,
					    user_combobox_set_sensitive,
					    NULL, NULL);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
					"text", TEXT_COL,
					NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo),
					    renderer,
					    user_combobox_set_sensitive,
					    NULL, NULL);

	/* Initially select the "Select user..." ... */
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

	/* Listen when a new user is selected */
	g_signal_connect (GTK_WIDGET (combo), "changed",
			  G_CALLBACK (user_combobox_changed), auth_dialog);

	/* ... and make the password and "Keep.." widgets insensitive */
	gtk_widget_set_sensitive (auth_dialog->priv->prompt_label, FALSE);
	gtk_widget_set_sensitive (auth_dialog->priv->password_entry, FALSE);
	gtk_widget_set_sensitive (auth_dialog->priv->keep_privilege_check_button, FALSE);
	gtk_widget_set_sensitive (auth_dialog->priv->keep_privilege_session_only_check_button, FALSE);
}

void
polkit_mate_auth_dialog_set_options (PolkitMateAuthDialog *auth_dialog, 
				      gboolean session, 
				      gboolean always,
				      gboolean requires_admin,
				      char **admin_users)
{
	const char *message_markup_secondary;

	auth_dialog->priv->can_do_always = always;
	auth_dialog->priv->can_do_session = session;

	if (auth_dialog->priv->can_do_session) {
		gtk_button_set_label (GTK_BUTTON (auth_dialog->priv->keep_privilege_check_button), 
				      _("_Remember authorization for this session"));

		gtk_widget_set_no_show_all (auth_dialog->priv->keep_privilege_check_button, FALSE);
		gtk_widget_set_no_show_all (auth_dialog->priv->keep_privilege_session_only_check_button, TRUE);
	} else if (auth_dialog->priv->can_do_always) {
		gtk_button_set_label (GTK_BUTTON (auth_dialog->priv->keep_privilege_check_button), 
				      _("_Remember authorization"));

		gtk_widget_set_no_show_all (auth_dialog->priv->keep_privilege_check_button, FALSE);
		gtk_widget_set_no_show_all (auth_dialog->priv->keep_privilege_session_only_check_button, FALSE);

	} else {
		gtk_widget_set_no_show_all (auth_dialog->priv->keep_privilege_check_button, TRUE);
		gtk_widget_set_no_show_all (auth_dialog->priv->keep_privilege_session_only_check_button, TRUE);
	}

	gtk_widget_set_no_show_all (auth_dialog->priv->user_combobox, TRUE);
		
	if (requires_admin) {
		if (admin_users != NULL) {
			message_markup_secondary =
				_("An application is attempting to perform an action that requires privileges. Authentication as one of the users below is required to perform this action.");

			create_user_combobox (auth_dialog, admin_users);
			gtk_widget_set_no_show_all (auth_dialog->priv->user_combobox, FALSE);

		} else {
			message_markup_secondary =
				_("An application is attempting to perform an action that requires privileges. Authentication as the super user is required to perform this action.");
		}
	} else {
		message_markup_secondary =
			_("An application is attempting to perform an action that requires privileges. Authentication is required to perform this action.");
	}
	gtk_label_set_markup (GTK_LABEL (auth_dialog->priv->message_label_secondary), message_markup_secondary);
}

static void
polkit_mate_auth_dialog_set_message (PolkitMateAuthDialog *auth_dialog, const char *markup)
{
	char *str;
	str = g_strdup_printf ("<big><b>%s</b></big>", markup);
        gtk_label_set_markup (GTK_LABEL (auth_dialog->priv->message_label), str);
	g_free (str);
}

static void
retain_checkbox_set_defaults (PolkitMateAuthDialog *auth_dialog, const char *action_id)
{
	gboolean retain_authorization;
        MateConfClient *client;
	GError *error;
	GSList *action_list, *l;

        client = mateconf_client_get_default ();
	retain_authorization = TRUE;

	if (client == NULL) {
		g_warning ("Error getting MateConfClient");
		goto out;
	}

	error = NULL;
	retain_authorization = mateconf_client_get_bool (client, KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION, &error);
	if (error != NULL) {
		g_warning ("Error getting key %s: %s",
			   KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION,
			   error->message);
		g_error_free (error);
		goto out;
	}

	/* check the blacklist */
	if (!retain_authorization)
		goto out;

	action_list = mateconf_client_get_list (client,
					     KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION_BLACKLIST,
					     MATECONF_VALUE_STRING,
					     &error);
	if (error != NULL) {
		g_warning ("Error getting key %s: %s",
			   KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION_BLACKLIST,
			   error->message);
		g_error_free (error);
		goto out;
	}

	for (l = action_list; l != NULL; l = l->next) {
		const char *str = l->data;
		if (strcmp (str, action_id) == 0) {
			retain_authorization = FALSE;
			break;
		}
	}
	g_slist_foreach (action_list, (GFunc) g_free, NULL);
	g_slist_free (action_list);

out:
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auth_dialog->priv->keep_privilege_check_button),
				      retain_authorization);
}

static void
polkit_mate_auth_dialog_set_action_id (PolkitMateAuthDialog *auth_dialog, const char *action_id)
{
	char *str;
	str = g_strdup_printf ("<small><a href=\"%s\">%s</a></small>", 
			       action_id, 
			       action_id);
	sexy_url_label_set_markup (SEXY_URL_LABEL (auth_dialog->priv->privilege_desc_label), str);
	g_free (str);

	str = g_strdup_printf (_("Click to edit %s"), action_id);
	gtk_widget_set_tooltip_markup (auth_dialog->priv->privilege_desc_label, str);
	g_free (str);

	retain_checkbox_set_defaults (auth_dialog, action_id);
}

static void
update_vendor (PolkitMateAuthDialog *auth_dialog)
{
	char *str;

	if (auth_dialog->priv->vendor == NULL)
		return;

	if (auth_dialog->priv->vendor_url == NULL) {
		str = g_strdup_printf ("<small>%s</small>", auth_dialog->priv->vendor);
		gtk_widget_set_tooltip_markup (auth_dialog->priv->privilege_vendor_label, NULL);
	} else {
		char *s2;

		str = g_strdup_printf ("<small><a href=\"%s\">%s</a></small>", 
				       auth_dialog->priv->vendor_url, 
				       auth_dialog->priv->vendor);

		s2 = g_strdup_printf (_("Click to open %s"), auth_dialog->priv->vendor_url);
		gtk_widget_set_tooltip_markup (auth_dialog->priv->privilege_vendor_label, s2);
		g_free (s2);
	}
	sexy_url_label_set_markup (SEXY_URL_LABEL (auth_dialog->priv->privilege_vendor_label), str);
	g_free (str);
}

static void
polkit_mate_auth_dialog_set_icon_name (PolkitMateAuthDialog *auth_dialog, const char *icon_name)
{
	GdkPixbuf *pixbuf;
	GdkPixbuf *copy_pixbuf;
	GdkPixbuf *vendor_pixbuf;

	pixbuf = NULL;
	copy_pixbuf = NULL;
	vendor_pixbuf = NULL;

	vendor_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
						  icon_name,
						  48,
						  0,
						  NULL);
	if (vendor_pixbuf == NULL)
		goto out;

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

	gtk_image_set_from_pixbuf (GTK_IMAGE (auth_dialog->priv->icon),
				   copy_pixbuf);

out:
	if (pixbuf != NULL)
		g_object_unref (pixbuf);
	if (copy_pixbuf != NULL)
		g_object_unref (copy_pixbuf);
	if (vendor_pixbuf != NULL)
		g_object_unref (vendor_pixbuf);
}

static void
polkit_mate_auth_dialog_set_program (PolkitMateAuthDialog *auth_dialog, const char *program)
{
	char *str;
	str = g_strdup_printf ("<small>%s</small>", program);
	gtk_label_set_markup (GTK_LABEL (auth_dialog->priv->app_desc_label), str);
	g_free (str);
}


void
polkit_mate_auth_dialog_set_prompt (PolkitMateAuthDialog *auth_dialog, 
					const char *prompt, 
					gboolean show_chars)
{
	gtk_label_set_text_with_mnemonic (GTK_LABEL (auth_dialog->priv->prompt_label), prompt);
	gtk_entry_set_visibility (GTK_ENTRY (auth_dialog->priv->password_entry), show_chars);
	gtk_entry_set_text (GTK_ENTRY (auth_dialog->priv->password_entry), "");
	gtk_widget_grab_focus (auth_dialog->priv->password_entry);
}

static void
polkit_mate_auth_dialog_set_property (GObject *object,
					  guint prop_id,
					  const GValue *value,
					  GParamSpec *pspec)
{
	PolkitMateAuthDialog *auth_dialog = POLKIT_MATE_AUTH_DIALOG (object);
	switch (prop_id)
	{
	case PROP_PROGRAM:
		polkit_mate_auth_dialog_set_program (auth_dialog, g_value_get_string (value));
		break;

	case PROP_ACTION_ID:
		polkit_mate_auth_dialog_set_action_id (auth_dialog, g_value_get_string (value));
		break;

	case PROP_VENDOR:
		g_free (auth_dialog->priv->vendor);
		auth_dialog->priv->vendor = g_strdup (g_value_get_string (value));
		update_vendor (auth_dialog);
		break;

	case PROP_VENDOR_URL:
		g_free (auth_dialog->priv->vendor_url);
		auth_dialog->priv->vendor_url = g_strdup (g_value_get_string (value));
		update_vendor (auth_dialog);
		break;

	case PROP_ICON_NAME:
		polkit_mate_auth_dialog_set_icon_name (auth_dialog, g_value_get_string (value));
		break;

	case PROP_MESSAGE:
		if (auth_dialog->priv->message)
			g_free (auth_dialog->priv->message);
		auth_dialog->priv->message = g_strdup (g_value_get_string (value));
		polkit_mate_auth_dialog_set_message (auth_dialog, auth_dialog->priv->message);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
polkit_mate_auth_dialog_get_property (GObject *object,
					  guint prop_id,
					  GValue *value,
					  GParamSpec *pspec)
{
	PolkitMateAuthDialog *auth_dialog = POLKIT_MATE_AUTH_DIALOG (object);

	switch (prop_id)
	{
	case PROP_MESSAGE:
		g_value_set_string (value, auth_dialog->priv->message);
		break;
		
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
remember_check_button_toggled (GtkWidget *widget, PolkitMateAuthDialog *auth_dialog)
{
	gboolean toggled;

	toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	if (auth_dialog->priv->keep_privilege_session_only_check_button != NULL) {
		gtk_widget_set_sensitive (auth_dialog->priv->keep_privilege_session_only_check_button, toggled);
	}
}

static void
vendor_url_activated (SexyUrlLabel *url_label, char *url, gpointer user_data)
{
        if (url != NULL) {
                gtk_show_uri (NULL, url, GDK_CURRENT_TIME, NULL);
        }
}

static void
action_id_activated (SexyUrlLabel *url_label, char *url, gpointer user_data)
{
        GError *error;
        DBusGConnection *bus;
        DBusGProxy *manager_proxy;

        error = NULL;
        bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (bus == NULL) {
                g_warning ("Couldn't connect to session bus: %s", error->message);
                g_error_free (error);
                goto out;
        }

	manager_proxy = dbus_g_proxy_new_for_name (bus,
                                               "org.mate.PolicyKit.AuthorizationManager",
                                               "/",
                                               "org.mate.PolicyKit.AuthorizationManager.SingleInstance");
        if (manager_proxy == NULL) {
                g_warning ("Could not construct manager_proxy object; bailing out");
                goto out;
        }

	if (!dbus_g_proxy_call (manager_proxy,
                                "ShowAction",
                                &error,
                                G_TYPE_STRING, url,
                                G_TYPE_INVALID,
                                G_TYPE_INVALID)) {
                if (error != NULL) {
                        g_warning ("Failed to call into manager: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to call into manager");
                }
                goto out;
	}
out:
	;
}

static void
polkit_mate_auth_dialog_init (PolkitMateAuthDialog *auth_dialog)
{
	GtkDialog *dialog = GTK_DIALOG (auth_dialog);
	PolkitMateAuthDialogPrivate *priv;

	priv = auth_dialog->priv = g_new0 (PolkitMateAuthDialogPrivate, 1);

        gtk_dialog_add_button (dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (dialog, _("_Authenticate"), GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);

	gtk_dialog_set_has_separator (dialog, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
	gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area), 5);
	gtk_box_set_spacing (GTK_BOX (dialog->action_area), 6);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_DIALOG_AUTHENTICATION);

	GtkWidget *hbox, *main_vbox, *vbox;

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), hbox, TRUE, TRUE, 0);

	priv->icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (priv->icon), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->icon, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new (FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

	/* main message */
	priv->message_label = gtk_label_new (NULL);
	polkit_mate_auth_dialog_set_message (auth_dialog, "");
	gtk_misc_set_alignment (GTK_MISC (priv->message_label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (priv->message_label), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (priv->message_label),
			    FALSE, FALSE, 0);


	/* secondary message */
	priv->message_label_secondary = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (priv->message_label_secondary), "");
	gtk_misc_set_alignment (GTK_MISC (priv->message_label_secondary), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (priv->message_label_secondary), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (priv->message_label_secondary),
			    FALSE, FALSE, 0);

	/* user combobox */
	priv->user_combobox = gtk_combo_box_new ();
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (priv->user_combobox), FALSE, FALSE, 0);

	/* password entry */
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

        GtkWidget *table_alignment;
        GtkWidget *table;
	table_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_box_pack_start (GTK_BOX (vbox), table_alignment, FALSE, FALSE, 0);
	table = gtk_table_new (1, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_container_add (GTK_CONTAINER (table_alignment), table);
	priv->password_entry = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (priv->password_entry), FALSE);
        priv->prompt_label = add_row (table, 0, _("_Password:"), priv->password_entry);

	g_signal_connect_swapped (priv->password_entry, "activate",
				  G_CALLBACK (gtk_window_activate_default),
				  dialog);

	priv->keep_privilege_check_button = gtk_check_button_new_with_mnemonic ("");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->keep_privilege_check_button), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), priv->keep_privilege_check_button, FALSE, FALSE, 0);
	g_signal_connect (priv->keep_privilege_check_button, "toggled",
			  G_CALLBACK (remember_check_button_toggled), auth_dialog);

	GtkWidget *keep_alignment;
	keep_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (keep_alignment), 0, 0, 10, 0);
	
	gtk_box_pack_start (GTK_BOX (vbox), keep_alignment, FALSE, FALSE, 0);
	GtkWidget *vbox3;
	vbox3 = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (keep_alignment), vbox3);
	
	priv->keep_privilege_session_only_check_button = 
		gtk_check_button_new_with_mnemonic (_("For this _session only"));
	gtk_box_pack_start (GTK_BOX (vbox3), priv->keep_privilege_session_only_check_button, FALSE, FALSE, 0);

	gtk_widget_set_no_show_all (auth_dialog->priv->keep_privilege_check_button, TRUE);
	gtk_widget_set_no_show_all (auth_dialog->priv->keep_privilege_session_only_check_button, TRUE);


	GtkWidget *details_expander;
	details_expander = gtk_expander_new_with_mnemonic (_("<small><b>_Details</b></small>"));
	gtk_expander_set_use_markup (GTK_EXPANDER (details_expander), TRUE);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), details_expander, FALSE, FALSE, 0);
	//gtk_box_pack_start (GTK_BOX (vbox), details_expander, FALSE, FALSE, 0);

	GtkWidget *details_vbox;
	details_vbox = gtk_vbox_new (FALSE, 10);
	gtk_container_add (GTK_CONTAINER (details_expander), details_vbox);

        //GtkWidget *table_alignment;
        //GtkWidget *table;
	table_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_box_pack_start (GTK_BOX (details_vbox), table_alignment, FALSE, FALSE, 0);
	table = gtk_table_new (1, 3, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_container_add (GTK_CONTAINER (table_alignment), table);


	priv->app_desc_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->app_desc_label), 0, 1.0);
        add_row (table, 0, _("<small><b>Application:</b></small>"), priv->app_desc_label);

	priv->privilege_desc_label = sexy_url_label_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->privilege_desc_label), 0, 1.0);
        add_row (table, 1, _("<small><b>Action:</b></small>"), priv->privilege_desc_label);
        g_signal_connect (priv->privilege_desc_label, "url-activated", (GCallback) action_id_activated, NULL);

	priv->privilege_vendor_label = sexy_url_label_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->privilege_vendor_label), 0, 1.0);
        add_row (table, 2, _("<small><b>Vendor:</b></small>"), priv->privilege_vendor_label);
        g_signal_connect (priv->privilege_vendor_label, "url-activated", (GCallback) vendor_url_activated, NULL);
}

static void
polkit_mate_auth_dialog_finalize (GObject *object)
{
	PolkitMateAuthDialog *auth_dialog = POLKIT_MATE_AUTH_DIALOG (object);

	g_free (auth_dialog->priv);

	G_OBJECT_CLASS (polkit_mate_auth_dialog_parent_class)->finalize (object);
}

#define PARAM_STATIC G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB

static void
polkit_mate_auth_dialog_class_init (PolkitMateAuthDialogClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	polkit_mate_auth_dialog_parent_class = (GObjectClass *) g_type_class_peek_parent (klass);

	gobject_class->finalize = polkit_mate_auth_dialog_finalize;
	gobject_class->get_property = polkit_mate_auth_dialog_get_property;
	gobject_class->set_property = polkit_mate_auth_dialog_set_property;

	g_object_class_install_property
		(gobject_class,
		 PROP_PROGRAM,
		 g_param_spec_string ("program",
				      "program",
				      "program",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));

	g_object_class_install_property
		(gobject_class,
		 PROP_ACTION_ID,
		 g_param_spec_string ("action-id",
				      "action-id",
				      "action-id",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));

	g_object_class_install_property
		(gobject_class,
		 PROP_VENDOR,
		 g_param_spec_string ("vendor",
				      "vendor",
				      "vendor",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));

	g_object_class_install_property
		(gobject_class,
		 PROP_VENDOR_URL,
		 g_param_spec_string ("vendor-url",
				      "vendor-url",
				      "vendor-url",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));

	g_object_class_install_property
		(gobject_class,
		 PROP_ICON_NAME,
		 g_param_spec_string ("icon-name",
				      "icon-name",
				      "icon-name",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));


	g_object_class_install_property
		(gobject_class,
		 PROP_MESSAGE,
		 g_param_spec_string ("message",
				      "message",
				      "message",
				      NULL,
				      G_PARAM_READWRITE | PARAM_STATIC));


	signals[USER_SELECTED] =
		g_signal_new ("user-selected",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (PolkitMateAuthDialogClass,
					       user_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);

}

/**
 * polkit_mate_auth_dialog_new:
 * 
 * Yada yada yada...
 * 
 * Returns: A new password dialog.
 **/
GtkWidget *
polkit_mate_auth_dialog_new (const char *path_to_program,
			      const char *action_id,
			      const char *vendor,
			      const char *vendor_url,
			      const char *icon_name,
			      const char *message_markup)
{
	PolkitMateAuthDialog *auth_dialog;
	GtkWindow *window;

	auth_dialog = g_object_new (POLKIT_MATE_TYPE_AUTH_DIALOG, 
				    "program", path_to_program,
				    "action-id", action_id,
				    "vendor", vendor,
				    "vendor-url", vendor_url,
				    "icon-name", icon_name,
				    "message", message_markup,
				    NULL);

	window = GTK_WINDOW (auth_dialog);

 	gtk_window_set_position (window, GTK_WIN_POS_CENTER);
	gtk_window_set_modal (window, TRUE);
	gtk_window_set_resizable (window, FALSE);
	gtk_window_set_keep_above (window, TRUE);
	gtk_window_set_title (window, _("Authenticate"));
	g_signal_connect (auth_dialog, "close",
			  G_CALLBACK (gtk_widget_hide), NULL);
	
	return GTK_WIDGET (auth_dialog);
}

const char *
polkit_mate_auth_dialog_get_password (PolkitMateAuthDialog *auth_dialog)
{
	return gtk_entry_get_text (GTK_ENTRY (auth_dialog->priv->password_entry));
}

gboolean
polkit_mate_auth_dialog_get_remember_session (PolkitMateAuthDialog *auth_dialog)
{
	gboolean ret;
	gboolean remember;

	remember = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (auth_dialog->priv->keep_privilege_check_button));

	if (auth_dialog->priv->can_do_always) {
		gboolean session_only;
		session_only = gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (auth_dialog->priv->keep_privilege_session_only_check_button));

		ret = remember && session_only;
	} else {
		ret = remember;
	}
	return ret;
}

gboolean
polkit_mate_auth_dialog_get_remember_always (PolkitMateAuthDialog *auth_dialog)
{
	gboolean ret;
	gboolean remember;

	remember = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (auth_dialog->priv->keep_privilege_check_button));

	if (auth_dialog->priv->can_do_always) {
		gboolean session_only;
		session_only = gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (auth_dialog->priv->keep_privilege_session_only_check_button));

		ret = remember && (!session_only);
	} else {
		ret = FALSE;
	}
	return ret;
}

/**
 * polkit_mate_auth_dialog_indicate_auth_error:
 * @auth_dialog: the auth dialog
 * 
 * Call this function to indicate an authentication error; typically shakes the window
 **/
void
polkit_mate_auth_dialog_indicate_auth_error (PolkitMateAuthDialog *auth_dialog)
{
	int x, y;
	int n;
	int diff;

	/* TODO: detect compositing manager and do fancy stuff here */

	gtk_window_get_position (GTK_WINDOW (auth_dialog), &x, &y);
	for (n = 0; n < 10; n++) {
		if (n % 2 == 0)
			diff = -15;
		else
			diff = 15;

		gtk_window_move (GTK_WINDOW (auth_dialog), x + diff, y);

                while (gtk_events_pending ()) {
                        gtk_main_iteration ();
                }

                g_usleep (10000);
	}
	gtk_window_move (GTK_WINDOW (auth_dialog), x, y);
}
