/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user-password.c: this file is part of users-admin, a mate-system-tools frontend
 * for user administration.
 *
 * Copyright (C) 2010 Milan Bouchet-Valat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <oobs/oobs.h>
#include <string.h>

#include "gst.h"
#include "user-password.h"
#include "users-tool.h"
#include "users-table.h"
#include "user-settings.h"
#include "run-passwd.h"
#include "passwd.h"


extern GstTool *tool;


/* Callbacks */
void on_user_settings_passwd_random_new   (GtkButton *button,
                                           gpointer   data);

void on_user_passwd_no_check_released     (GtkButton *button,
                                           gpointer   data);

void on_user_settings_passwd_toggled      (GtkToggleButton *toggle,
                                           gpointer         data);

void on_passwd_entry_changed              (GtkWidget *entry,
                                           gpointer   user_data);

gboolean on_user_passwd_current_focus_out (GtkWidget *entry,
                                           GdkEventFocus *event,
                                           gpointer user_data);

void on_user_passwd_dialog_response       (GtkDialog *user_passwd_dialog,
                                           int        response,
                                           gpointer   user_data);

/*
 * Checks that password is valid. Returns it if it's the case,
 * or NULL if it's not (showing an error).
 */
static const char*
check_password (OobsUser *user)
{
	GtkWidget *dialog;
	GtkWidget *widget;
	const gchar *password, *confirmation;
	char *primary_text = NULL;
	char *secondary_text;
	int len;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");

	/* manual password? */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd1");
		password = gtk_entry_get_text (GTK_ENTRY (widget));

		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd2");
		confirmation = gtk_entry_get_text (GTK_ENTRY (widget));
	} else {
		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_random_passwd");
		password = confirmation = gtk_entry_get_text (GTK_ENTRY (widget));
	}

	len = strlen (password);

	/* empty password, accept but don't change it */
	if (len == 0) {
		return password;
	}
	else if (len < 6) {
		primary_text = _("Password is too short");
		secondary_text = _("User passwords must be longer than 5 characters and preferably "
		                   "formed by numbers, letters and special characters.");
	} else if (strcmp (password, confirmation) != 0) {
		primary_text = _("Password confirmation is not correct");
		secondary_text = _("Check that you have provided the same password in both text fields.");
	}

	if (primary_text) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
		                                 GTK_DIALOG_MODAL,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_CLOSE,
		                                 "%s", primary_text);

		gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
		                                            "%s", secondary_text);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return NULL;
	}

	return password;
}

void
on_user_settings_passwd_random_new (GtkButton *button,
                                    gpointer   data)
{
	GtkWidget *widget;
	gchar *passwd;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_random_passwd");

	passwd = passwd_get_random ();
	gtk_entry_set_text (GTK_ENTRY (widget), passwd);
	g_free (passwd);
}

void
on_user_passwd_no_check_released (GtkButton *button,
                                  gpointer   data)
{
	gboolean active;

	/* Prevent modifications if not authenticated */
	if (!gst_tool_authenticate (tool, GST_USERS_TOOL (tool)->users_config)) {
		active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), !active);
	}
}

void
on_user_settings_passwd_toggled (GtkToggleButton *toggle,
                                 gpointer         data)
{
	GtkWidget *user_passwd_random_new = gst_dialog_get_widget (tool->main_dialog, "user_passwd_random_new");
	GtkWidget *user_passwd_random_entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_random_passwd");
	GtkWidget *user_passwd_entry1 = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd1");
	GtkWidget *user_passwd_entry2 = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd2");
	GtkWidget *validate_button = gst_dialog_get_widget (tool->main_dialog, "user_passwd_validate_button");
	GtkToggleButton *pwd_manual = GTK_TOGGLE_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual"));

	if (gtk_toggle_button_get_active (pwd_manual)) {
		gtk_widget_set_sensitive (user_passwd_random_new, FALSE);
		gtk_widget_set_sensitive (user_passwd_random_entry, FALSE);
		gtk_widget_set_sensitive (user_passwd_entry1, TRUE);
		gtk_widget_set_sensitive (user_passwd_entry2, TRUE);
		gtk_entry_set_text (GTK_ENTRY (user_passwd_random_entry), "");
	}
	else {
		gtk_widget_set_sensitive (user_passwd_random_new, TRUE);
		gtk_widget_set_sensitive (user_passwd_random_entry, TRUE);
		gtk_widget_set_sensitive (user_passwd_entry1, FALSE);
		gtk_widget_set_sensitive (user_passwd_entry2, FALSE);
		gtk_entry_set_text (GTK_ENTRY (user_passwd_entry1), "");
		gtk_entry_set_text (GTK_ENTRY (user_passwd_entry2), "");
		on_user_settings_passwd_random_new (NULL, NULL);

		/* We know the random entry can't be empty */
		gtk_widget_set_sensitive (validate_button, TRUE);
	}
}

/*
 * Alternative to run_edit_dialog() used by on_edit_user_passwd,
 * which needs more flexibility than gtk_dialog_run() provides.
 */
static void
prepare_edit_dialog (GtkDialog *dialog,
                     GtkImage  *face_image2,
                     GtkLabel  *name_label)
{
	OobsUser *user;
	GtkWidget *face_image1;
	GdkPixbuf *face;
	const char *name;

	/* Set user face from the main dialog image */
	face_image1 = gst_dialog_get_widget (tool->main_dialog, "user_settings_face");
	face = gtk_image_get_pixbuf (GTK_IMAGE (face_image1));
	gtk_image_set_from_pixbuf (face_image2, face);

	/* Set user name */
	user = users_table_get_current ();
	name = oobs_user_get_full_name_fallback (user);
	gtk_label_set_text (name_label, name);
	g_object_unref (user);

	/* Run dialog with correct settings */
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
	                              GTK_WINDOW (tool->main_dialog));
	gst_dialog_add_edit_dialog (tool->main_dialog, GTK_WIDGET (dialog));
	gtk_widget_show (GTK_WIDGET (dialog));
}

/*
 * Callback for edit_user_name_button: run the dialog to change the user's
 * password and apply changes if needed.
 */
void
on_edit_user_passwd (GtkButton *button,
                     gpointer   user_data)
{
	static gboolean once = TRUE;
	GtkWidget *user_passwd_dialog;
	GtkWidget *face_image;
	GtkWidget *name_label;
	GtkWidget *manual_toggle;
	GtkWidget *passwd_entry;
	GtkWidget *nocheck_toggle;
	GtkWidget *user_passwd_current;
	GtkWidget *current_label;
	OobsUser *user;
	OobsGroup *no_passwd_login_group;
	PasswdHandler *passwd_handler = NULL;
	gboolean is_self;

	user = users_table_get_current ();
	is_self = oobs_self_config_is_user_self (OOBS_SELF_CONFIG (GST_USERS_TOOL (tool)->self_config),
	                                         user);

	if (!is_self && !gst_tool_authenticate (tool, GST_USERS_TOOL (tool)->users_config))
		return;

	user_passwd_dialog = gst_dialog_get_widget (tool->main_dialog, "user_passwd_dialog");
	face_image = gst_dialog_get_widget (tool->main_dialog, "user_passwd_face");
	name_label = gst_dialog_get_widget (tool->main_dialog, "user_passwd_name");
	nocheck_toggle = gst_dialog_get_widget (tool->main_dialog, "user_passwd_no_check");
	manual_toggle = gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");
	user_passwd_current = gst_dialog_get_widget (tool->main_dialog, "user_passwd_current");
	current_label = gst_dialog_get_widget (tool->main_dialog, "user_passwd_current_label");

	if (once) {
		g_signal_connect (G_OBJECT (user_passwd_dialog),
		                  "delete-event",
		                  G_CALLBACK (gtk_widget_hide_on_delete),
		                  NULL);
	}

	if (is_self)
		passwd_handler = passwd_init ();

	g_object_set_data (G_OBJECT (user_passwd_current), "passwd_handler", passwd_handler);

	/* set manual password */
	manual_toggle = gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (manual_toggle), TRUE);

	/* clear entries */
	passwd_entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd1");
	gtk_entry_set_text (GTK_ENTRY (passwd_entry), "");
	gtk_widget_grab_focus (passwd_entry);

	passwd_entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd2");
	gtk_entry_set_text (GTK_ENTRY (passwd_entry), "");

	gtk_entry_set_text (GTK_ENTRY (user_passwd_current), "");
	gtk_widget_modify_base (user_passwd_current, GTK_STATE_NORMAL, NULL);

	/* Reset current password widgets */
	if (is_self) {
		gtk_widget_show (user_passwd_current);
		gtk_widget_show (current_label);

		gtk_widget_grab_focus (user_passwd_current);
	}
	else {
		gtk_widget_hide (user_passwd_current);
		gtk_widget_hide (current_label);
	}

	/* set option to skip password check at login */
	no_passwd_login_group =
		oobs_groups_config_get_from_name (OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config),
		                                  NO_PASSWD_LOGIN_GROUP);
	/* root should not be allowed to login without password,
	 * and we disable the feature if the group does not exist */
	if (oobs_user_is_root (user) || no_passwd_login_group == NULL) {
		gtk_widget_set_sensitive (nocheck_toggle, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nocheck_toggle),
		                              FALSE);
	}
	else {
		gtk_widget_set_sensitive (nocheck_toggle, TRUE);
		if (oobs_user_is_in_group (user, no_passwd_login_group))
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nocheck_toggle),
			                              TRUE);
		else
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nocheck_toggle),
			                              FALSE);
	}

	prepare_edit_dialog (GTK_DIALOG (user_passwd_dialog),
	                     GTK_IMAGE (face_image), GTK_LABEL (name_label));

	g_object_unref (user);

	if (no_passwd_login_group)
		g_object_unref (no_passwd_login_group);
}

/*
 * Called after the Validate button has been clicked, either directly,
 * or asynchronously via chpasswd_cb(). Checks that changes to the password or
 * to the password-less option should be committed, and close dialog.
 */
static void
finish_password_change ()
{
	GtkWidget *user_passwd_dialog;
	GtkWidget *passwd_entry;
	GtkWidget *nocheck_toggle;
	OobsUser  *user;
	OobsGroup *no_passwd_login_group;
	gboolean   no_passwd_login_changed;
	gboolean   is_self;

	user_passwd_dialog = gst_dialog_get_widget (tool->main_dialog, "user_passwd_dialog");
	passwd_entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd1");
	nocheck_toggle = gst_dialog_get_widget (tool->main_dialog, "user_passwd_no_check");
	user = users_table_get_current ();
	is_self = oobs_self_config_is_user_self (OOBS_SELF_CONFIG (GST_USERS_TOOL (tool)->self_config),
	                                         user);

	no_passwd_login_group =
		oobs_groups_config_get_from_name (OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config),
		                                  NO_PASSWD_LOGIN_GROUP);

	/* FIXME: this is a hack; proper handling of this mess would involve
	 * displaying an error message when people enable password-less login
	 * without providing a non-empty password.
	 *
	 * Only unlock account if a new, non-empty password was provided.
	 * Else, we would believe the account is enabled, while it's not,
	 * and we would allow password-less login, which is unexpected. */
	if (oobs_user_get_password_disabled (user)
	    && strlen (gtk_entry_get_text (GTK_ENTRY (passwd_entry))) == 0)
	  {
		  /* Force removing user from this group, since results are unexpected */
		  if (no_passwd_login_group)
			  oobs_group_remove_user (no_passwd_login_group, user);
	  }
	else
	  {
		  /* check whether user is allowed to login without password */
		  no_passwd_login_changed =
			  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nocheck_toggle))
			  != oobs_user_is_in_group (user, no_passwd_login_group);

		  if (no_passwd_login_changed) {
			  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nocheck_toggle)))
				  oobs_group_add_user (no_passwd_login_group, user);
			  else
				  oobs_group_remove_user (no_passwd_login_group, user);
		  }

		  /* Unlock account, this may not be what is wanted, but that's the only solution
		   * since 'passwd' doesn't differentiate accounts with no passwords yet from disabled ones */
		  oobs_user_set_password_disabled (user, FALSE);

		  /* Only commit if password-less login option has changed, or
		   * if user is not self (changing password via the backends). */
		  if (no_passwd_login_changed || !is_self) {
			  /* commit both user and groups config
			   * because of the no_passwd_login_group membership */
			  if (gst_tool_commit (tool, OOBS_OBJECT (user)) == OOBS_RESULT_OK) {
				  gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);

				  /* Update settings shown in the main dialog */
				  user_settings_show (user);
			  }
		  }

		  /* We know we've set a non-empty password */
		  oobs_user_set_password_empty (user, FALSE);
	  }

	user_settings_show (user);

	gst_dialog_remove_edit_dialog (tool->main_dialog, GTK_WIDGET (user_passwd_dialog));
	gtk_widget_hide (GTK_WIDGET (user_passwd_dialog));

	g_object_unref (user);
	if (no_passwd_login_group)
		g_object_unref (no_passwd_login_group);
}

static void
auth_cb (PasswdHandler *passwd_handler,
         GError        *error,
         gpointer       user_data)
{
	GtkWidget *entry = GTK_WIDGET (user_data);
	GdkColor color;

	gdk_color_parse ("red", &color);
	gtk_widget_modify_base (entry, GTK_STATE_NORMAL, error ? &color : NULL);
}

static void
chpasswd_cb (PasswdHandler *passwd_handler,
             GError        *error,
             gpointer       user_data)
{
	GtkWidget *user_passwd_dialog;
	GtkWidget *dialog;
	GtkWidget *entry;
	char *primary_text;
	char *secondary_text;

	user_passwd_dialog = gst_dialog_get_widget (tool->main_dialog, "user_passwd_dialog");

	/* Restore dialog sensitivity and reset cursor */
	gtk_widget_set_sensitive (GTK_WIDGET (user_passwd_dialog), TRUE);
	gdk_window_set_cursor (gtk_widget_get_window (user_passwd_dialog), NULL);


	if (!error) {
		finish_password_change ();

		passwd_destroy (passwd_handler);
		return;
	}

	if (error->code == PASSWD_ERROR_REJECTED) {
		primary_text = error->message;
		secondary_text = _("Please choose another password.");

		entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd1");
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		gtk_widget_grab_focus (entry);

		entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd2");
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	}
	else if (error->code == PASSWD_ERROR_AUTH_FAILED) {
		primary_text = error->message;
		secondary_text = _("Please type again your current password.");

		entry = gst_dialog_get_widget (tool->main_dialog, "user_passwd_current");
		gtk_widget_grab_focus (entry);
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	}
	else {
		primary_text = _("Password could not be changed");
		secondary_text = error->message;
	}

	dialog = gtk_message_dialog_new (GTK_WINDOW (user_passwd_dialog),
	                                 GTK_DIALOG_MODAL,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_CLOSE,
	                                 "%s", primary_text);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          "%s", secondary_text);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

gboolean
on_user_passwd_current_focus_out (GtkWidget     *entry,
                                  GdkEventFocus *event,
                                  gpointer       user_data)
{
	PasswdHandler *passwd_handler;
	const char *password;

	password = gtk_entry_get_text (GTK_ENTRY (entry));

	if (strlen (password) > 0) {
		passwd_handler = g_object_get_data (G_OBJECT (entry), "passwd_handler");
		passwd_authenticate (passwd_handler, password, auth_cb, entry);
	}

	return FALSE;
}

/*
 * Callback for user_passwd_dialog: (try to) change password after performing checks.
 */
void
on_user_passwd_dialog_response (GtkDialog *user_passwd_dialog,
                                int        response,
                                gpointer   user_data)
{
	GtkWidget *manual_toggle;
	GtkWidget *nocheck_toggle;
	GtkWidget *user_passwd_current;
	OobsUser *user;
	const char *passwd;
	PasswdHandler *passwd_handler = NULL;
	gboolean is_self;

	nocheck_toggle = gst_dialog_get_widget (tool->main_dialog, "user_passwd_no_check");
	manual_toggle = gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");
	user_passwd_current = gst_dialog_get_widget (tool->main_dialog, "user_passwd_current");

	user = users_table_get_current ();
	is_self = oobs_self_config_is_user_self (OOBS_SELF_CONFIG (GST_USERS_TOOL (tool)->self_config),
	                                         user);

	passwd_handler = g_object_get_data (G_OBJECT (user_passwd_current), "passwd_handler");

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_hide (GTK_WIDGET (user_passwd_dialog));

		if (passwd_handler)
			passwd_destroy (passwd_handler);

		g_object_unref (user);
		return;
	}

	/* Shows an error if password is not valid */
	passwd = check_password (user);
	if (!passwd) {
		g_object_unref (user);
		return;
	}
	/* If empty, directly handle password-less option, don't change password */
	else if (strlen (passwd) == 0) {
		finish_password_change ();
		g_object_unref (user);
		return;
	}


	/* For current user, change password via PAM */
	if (is_self) {
		passwd_change_password (passwd_handler, passwd, chpasswd_cb, NULL);
		/* chpasswd_cb() will run finish_passwd_change() when done */

		gtk_widget_set_sensitive (GTK_WIDGET (user_passwd_dialog), FALSE);
		GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (user_passwd_dialog));
		GdkCursor *cursor;
		cursor = gdk_cursor_new_for_display (display, GDK_WATCH);

		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (user_passwd_dialog)),
		                       cursor);
		gdk_display_flush (display);

		gdk_cursor_unref (cursor);
	}
	/* For other users, set password via the backends */
	else {
		oobs_user_set_password (user, passwd);
		finish_password_change ();
	}

	g_object_unref (user);
}
