/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user-settings.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>,
 *          Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <oobs/oobs-defines.h>
#include "gst.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <utmp.h>
#include <ctype.h>

#include "users-table.h"
#include "table.h"
#include "callbacks.h"
#include "user-settings.h"
#include "user-password.h"
#include "privileges-table.h"
#include "groups-table.h"
#include "group-settings.h"
#include "test-battery.h"
#include "user-profiles.h"

extern GstTool *tool;


void   on_user_settings_enable_account  (GtkButton *button,
                                         gpointer   user_data);
void   on_user_new_name_changed         (GtkEditable *user_name,
                                         gpointer     user_data);
void   on_user_new_login_changed        (GtkComboBox *login_combo,
                                         gpointer     user_data);
void   on_user_new                      (GtkButton *button,
                                         gpointer   user_data);
void   on_edit_user_name                (GtkButton *button,
                                         gpointer   user_data);
void   on_edit_user_profile             (GtkButton *button,
                                         gpointer   user_data);
void   on_edit_user_advanced            (GtkButton *button,
                                         gpointer   user_data);


static gboolean
check_user_delete (OobsUser *user)
{
	GtkWidget *dialog;
	OobsGroupsConfig *config;
	OobsGroup *admin_group;
	GList *admin_users;
	gint response;

	config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
	admin_group = oobs_groups_config_get_from_name (config, ADMIN_GROUP);
	admin_users = oobs_group_get_users (admin_group);

	if (oobs_user_get_uid (user) == 0) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Administrator account cannot be deleted"));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("This would leave the system unusable."));

	}
	else if (oobs_user_get_active (user)) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_INFO,
					 GTK_BUTTONS_CLOSE,
					 _("%s is currently using this computer"),
					 oobs_user_get_full_name_fallback (user));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("Please ensure the user has logged out before deleting this account."));
	}
	/* don't allow deleting the last admin */
	else if (oobs_user_is_in_group (user, admin_group)
	         && g_list_length (admin_users) < 2)
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
		                                 GTK_DIALOG_MODAL,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_CLOSE,
		                                 _("Can't delete the only administrator account"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		                                          _("%s is the only administrator on this computer. "
		                                            "Deleting this account would lock you out of "
		                                            "administrating the system."),
		                                          oobs_user_get_full_name_fallback (user));
	}
	else {
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
		                                 GTK_DIALOG_MODAL,
		                                 GTK_MESSAGE_QUESTION,
		                                 GTK_BUTTONS_NONE,
		                                 _("Remove home folder for %s?"),
		                                 oobs_user_get_full_name_fallback (user));
		gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
		                                            _("Files owned by user %s in <tt>%s</tt> can be completely "
		                                              "removed if you don't need them anymore. You may want to "
		                                              "back them up before deleting the account, or keep them "
		                                              "so that an administrator can save them later."),
		                                            oobs_user_get_login_name (user),
		                                            oobs_user_get_home_directory (user));

		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
		                        _("Keep Files"), GTK_RESPONSE_NO,
		                        _("Don't Remove Account"), GTK_RESPONSE_CANCEL,
		                        _("Delete Files"), GTK_RESPONSE_YES,
		                        NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
	}

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	gtk_widget_destroy (dialog);
	g_object_unref (admin_group);
	g_list_free (admin_users);

	/* Home flag is used to remove home when deleting a user */
	if (response == GTK_RESPONSE_YES)
		oobs_user_set_home_flags (user, OOBS_USER_REMOVE_HOME);
	else
		oobs_user_set_home_flags (user, 0);

	return (response == GTK_RESPONSE_YES || response == GTK_RESPONSE_NO);
}

gboolean
user_delete (GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;
	OobsUsersConfig *config;
	OobsUser *user;
	OobsResult result;
	gboolean retval = FALSE;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return FALSE;

	gtk_tree_model_get (model, &iter,
			    COL_USER_OBJECT, &user,
			    -1);

	if (check_user_delete (user)) {
		config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);
		result = oobs_users_config_delete_user (config, user);
		if (result == OOBS_RESULT_OK) {
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			retval = TRUE;
		}
		else {
			gst_tool_commit_error (tool, result);
			retval = FALSE;
		}
	}

	g_object_unref (user);

	return retval;
}

void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GList *list, *elem;

	/* No need to prompt if not allowed */
	if (!gst_tool_authenticate (tool, GST_USERS_TOOL (tool)->users_config))
		return;

	list = elem = users_table_get_row_references ();
	model = users_table_get_model ();

	while (elem) {
		path = gtk_tree_row_reference_get_path (elem->data);

		user_delete (model, path);

		gtk_tree_path_free (path);
		elem = elem->next;
	}

	users_table_select_first ();

	g_list_foreach (list, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free (list);
}

static void
set_entry_text (GtkWidget *entry, const gchar *text)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (text) ? text : "");
}

static void
select_main_group (OobsUser *user)
{
	GtkWidget *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsGroup *main_group, *group;
	gboolean valid, found;

	combo = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");

	main_group = oobs_user_get_main_group (user);

	if (!main_group) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), -1);
		return;
	}

	found = FALSE;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid && !found) {
		gtk_tree_model_get (model, &iter,
				    COL_GROUP_OBJECT, &group,
				    -1);

		if (main_group == group) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
			found = TRUE;
		}

		g_object_unref (group);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	if (!found)
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), -1);
}

static void
set_login_length (GtkWidget *entry)
{
	gint max_len;
#ifdef __FreeBSD__
	max_len = UT_NAMESIZE;
#else
	struct utmp ut;

	max_len = sizeof (ut.ut_user);
#endif

	gtk_entry_set_max_length (GTK_ENTRY (entry), max_len);
}

GdkPixbuf *
user_settings_get_user_face (OobsUser *user, int size)
{
	GdkPixbuf *pixbuf;
	const gchar *homedir;
	gchar *face_path;

	homedir = oobs_user_get_home_directory (user);
	face_path = g_strdup_printf ("%s/.face", homedir);
	pixbuf = gdk_pixbuf_new_from_file_at_size (face_path, size, size, NULL);

	if (!pixbuf)
		pixbuf = gtk_icon_theme_load_icon (tool->icon_theme, "stock_person", size, 0, NULL);

	g_free (face_path);

	return pixbuf;
}

void
user_settings_show (OobsUser *user)
{
	GtkWidget *name_label;
	GtkWidget *face_image;
	GtkWidget *profile_label;
	GtkWidget *profile_button;
	GtkWidget *passwd_label;
	GtkWidget *disabled_image;
	GtkWidget *disabled_label;
	GtkWidget *enable_button;
	GdkPixbuf *face;
	GstUserProfile *profile;
	OobsGroupsConfig *groups_config;
	OobsGroup *group;

	name_label = gst_dialog_get_widget (tool->main_dialog, "user_settings_real_name");
	gtk_label_set_text (GTK_LABEL (name_label), oobs_user_get_full_name_fallback (user));

	face_image = gst_dialog_get_widget (tool->main_dialog, "user_settings_face");
	face = user_settings_get_user_face (user, 60);
	gtk_image_set_from_pixbuf (GTK_IMAGE (face_image), face);
	g_object_unref (face);

	profile_label = gst_dialog_get_widget (tool->main_dialog, "user_settings_profile");
	profile_button = gst_dialog_get_widget (tool->main_dialog, "edit_user_profile_button");
	profile = gst_user_profiles_get_for_user (GST_USERS_TOOL (tool)->profiles,
	                                          user, FALSE);
	if (oobs_user_is_root (user)) {
		gtk_widget_set_sensitive (profile_button, FALSE);
		gtk_label_set_text (GTK_LABEL (profile_label), _("Superuser"));
	}
	else {
		gtk_widget_set_sensitive (profile_button, TRUE);
		gtk_label_set_text (GTK_LABEL (profile_label),
		                    profile ? profile->name : _("Custom"));
	}

	passwd_label = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd");
	groups_config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
	group = oobs_groups_config_get_from_name (groups_config, NO_PASSWD_LOGIN_GROUP);

	if (oobs_user_get_password_empty (user))
		/* TRANSLATORS: This applies to a password. */
		gtk_label_set_text (GTK_LABEL (passwd_label), _("None"));
	else if (group && oobs_user_is_in_group (user, group))
		/* TRANSLATORS:  This applies to a password. Keep the string short. */
		gtk_label_set_text (GTK_LABEL (passwd_label), _("Not asked on login"));
	else
		gtk_label_set_text (GTK_LABEL (passwd_label), _("Asked on login"));

	/* Show notice if account is locked, this is really useful for new accounts
	 * where user did not choose a password. */
	disabled_image = gst_dialog_get_widget (tool->main_dialog,
	                                        "user_settings_disabled_account_image");
	disabled_label = gst_dialog_get_widget (tool->main_dialog,
	                                        "user_settings_disabled_account");
	enable_button = gst_dialog_get_widget (tool->main_dialog,
	                                       "user_settings_enable_account");

	if (oobs_user_get_password_disabled (user)) {
		gtk_widget_show (disabled_image);
		gtk_widget_show (disabled_label);
		gtk_widget_show (enable_button);
	}
	else {
		gtk_widget_hide (disabled_image);
		gtk_widget_hide (disabled_label);
		gtk_widget_hide (enable_button);
	}

	g_object_unref (group);
}

static void
check_comments (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	GtkWidget *room_number, *wphone, *hphone;
	gchar *comment, *ch;

	room_number = gst_dialog_get_widget (tool->main_dialog, "user_settings_room_number");
	wphone = gst_dialog_get_widget (tool->main_dialog, "user_settings_wphone");
	hphone = gst_dialog_get_widget (tool->main_dialog, "user_settings_hphone");

	comment = g_strjoin (" ",
			     gtk_entry_get_text (GTK_ENTRY (room_number)),
			     gtk_entry_get_text (GTK_ENTRY (wphone)),
			     gtk_entry_get_text (GTK_ENTRY (hphone)),
			     NULL);

	if ((ch = g_utf8_strchr (comment, -1, ',')) ||
	    (ch = g_utf8_strchr (comment, -1, '=')) ||
	    (ch = g_utf8_strchr (comment, -1, ':'))) {
		*primary_text   = g_strdup_printf (_("Invalid character \"%c\" in comment"), *ch);
		*secondary_text = g_strdup (_("Check that this character is not used."));
	}

	g_free (comment);
}

static gboolean
check_home (OobsUser *user)
{
	GtkWidget *dialog;
	GtkWidget *home_entry;
	GtkWidget *content_area;
	GtkWidget *chown_home_checkbutton;
	GtkWidget *delete_old_checkbutton;
	const char *home;
	gboolean old_home_exists, new_home_exists;
	gboolean chown_home, delete_old;
	char *message;
	int response;
	int home_flags;

	/* Better be sure there's no remnant from aborted changes */
	oobs_user_set_home_flags (user, 0);

	home_entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	home = gtk_entry_get_text (GTK_ENTRY (home_entry));

	if (strlen (home) < 1 || !g_path_is_absolute (home)) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
		                                 GTK_DIALOG_MODAL,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_CLOSE,
		                                 "%s", _("Incomplete path in home directory"));
		message = _("Please enter full path for home directory\n"
		            "<span size=\"smaller\">i.e.: /home/john</span>.");
		gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
		                                            "%s", message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		return FALSE;
	}
	else if (strcmp (oobs_user_get_home_directory (user), home) != 0) {
		old_home_exists = g_file_test (oobs_user_get_home_directory (user), G_FILE_TEST_EXISTS);
		new_home_exists = g_file_test (home, G_FILE_TEST_EXISTS);

		chown_home = FALSE;
		delete_old = FALSE;
		home_flags = 0;

		if (old_home_exists && new_home_exists) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog), GTK_DIALOG_MODAL,
			                                 GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
			                                 _("New home directory already exists, use it?"));
			gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
			                                            _("The home directory for %s has been set "
			                                              "to <tt>%s</tt>, which already exists. "
			                                              "Do you want to use files from this directory, "
			                                              "or copy the contents of <tt>%s</tt> "
			                                              "to the new home, overwriting it?\n\n"
			                                              "In doubt, use the new directory to avoid "
			                                              "losing data, and copy files from the old "
			                                              "directory later."),
			                                            oobs_user_get_full_name_fallback (user),
			                                            home,
			                                            oobs_user_get_home_directory (user));
			gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			                        _("_Replace With Old Files"), GTK_RESPONSE_NO,
			                        _("_Cancel Change"), GTK_RESPONSE_CANCEL,
			                        /* TRANSLATORS: This means "use the files from the new location",
			                         * as opposed to those from the old location. */
			                        _("_Use New Files"), GTK_RESPONSE_YES, NULL);
			gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

			content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
			chown_home_checkbutton =
			    gtk_check_button_new_with_mnemonic (_("Make user the _owner of the new home directory"));
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chown_home_checkbutton), TRUE);
			delete_old_checkbutton =
			    gtk_check_button_new_with_mnemonic (_("_Delete old home directory"));
			gtk_box_pack_start (GTK_BOX (content_area), chown_home_checkbutton, FALSE, FALSE, 0);
			gtk_box_pack_start (GTK_BOX (content_area), delete_old_checkbutton, FALSE, FALSE, 0);
			gtk_widget_show (chown_home_checkbutton);
			gtk_widget_show (delete_old_checkbutton);

			response = gtk_dialog_run (GTK_DIALOG (dialog));
			chown_home = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chown_home_checkbutton));
			delete_old = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (delete_old_checkbutton));

			if (response == GTK_RESPONSE_NO)
			  home_flags = OOBS_USER_COPY_HOME;

			gtk_widget_destroy (dialog);
		}
		else if (new_home_exists) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog), GTK_DIALOG_MODAL,
			                                 GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
			                                 _("New home directory already exists, use it?"));
			gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
			                                            _("The home directory for %s has been set "
			                                              "to <tt>%s</tt>, which already exists. "
			                                              "Do you want to use files from this directory, "
			                                              "or delete all its contents and use a "
			                                              "completely empty home directory?\n\n"
			                                              "In doubt, keep the files, and remove them "
			                                              "later if needed."),
			                                            oobs_user_get_full_name_fallback (user),
			                                            home);
			gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			                        _("_Delete Files"), GTK_RESPONSE_NO,
			                        _("_Cancel Change"), GTK_RESPONSE_CANCEL,
			                        _("_Use Existing Files"), GTK_RESPONSE_YES, NULL);
			gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

			content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
			chown_home_checkbutton =
			    gtk_check_button_new_with_mnemonic (_("Make user the _owner of the new home directory"));
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chown_home_checkbutton), TRUE);
			gtk_box_pack_start (GTK_BOX (content_area), chown_home_checkbutton, FALSE, FALSE, 0);
			gtk_widget_show (chown_home_checkbutton);

			response = gtk_dialog_run (GTK_DIALOG (dialog));
			chown_home = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chown_home_checkbutton));

			if (response == GTK_RESPONSE_NO)
			  home_flags = OOBS_USER_ERASE_HOME;

			gtk_widget_destroy (dialog);
		}
		else if (old_home_exists) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog), GTK_DIALOG_MODAL,
			                                 GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
			                                 _("Copy old home directory to new location?"));
			gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
			                                            _("The home directory for %s has been set "
			                                              "to <tt>%s</tt>, which doesn't exist. "
			                                              "Do you want to copy the contents of the "
			                                              "old home directory (<tt>%s</tt>), or use "
			                                              "a completely empty home directory?\n\n"
			                                              "If you choose to copy the files to the new "
			                                              "location, it's safe to delete the old directory."),
			                                            oobs_user_get_full_name_fallback (user),
			                                            home,
			                                            oobs_user_get_home_directory (user));
			gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			                        _("_Use Empty Directory"), GTK_RESPONSE_NO,
			                        _("_Cancel Change"), GTK_RESPONSE_CANCEL,
			                        /* TRANSLATORS: This means "copy files from the old home directory". */
			                        _("Co_py Old Files"), GTK_RESPONSE_YES, NULL);
			gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

			content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
			delete_old_checkbutton =
			    gtk_check_button_new_with_mnemonic (_("_Delete old home directory"));
			gtk_box_pack_start (GTK_BOX (content_area), delete_old_checkbutton, FALSE, FALSE, 0);
			gtk_widget_show (delete_old_checkbutton);

			response = gtk_dialog_run (GTK_DIALOG (dialog));
			delete_old = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (delete_old_checkbutton));

			if (response == GTK_RESPONSE_YES)
			  home_flags = OOBS_USER_COPY_HOME;

			gtk_widget_destroy (dialog);
		}
		else {
			/* Backends will create an empty dir owned by the user */
			return TRUE;
		}

		if (chown_home && delete_old)
			oobs_user_set_home_flags (user, home_flags | OOBS_USER_CHOWN_HOME
			                                           | OOBS_USER_REMOVE_HOME);
		else if (chown_home)
			oobs_user_set_home_flags (user, home_flags | OOBS_USER_CHOWN_HOME);
		else if (delete_old)
			oobs_user_set_home_flags (user, home_flags | OOBS_USER_REMOVE_HOME);
		else
			oobs_user_set_home_flags (user, home_flags);

		return (response != GTK_RESPONSE_CANCEL);
	}

	return TRUE;
}

static void
check_uid (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsUsersConfig *config;
	OobsUser *user;
	OobsUser *uid_user;
	GtkWidget *widget;
	uid_t uid;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	/* we know the value is positive because the range is limited */
	uid = (unsigned) gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
	user = users_table_get_current ();

	/* don't warn if nothing has changed */
	if (uid == oobs_user_get_uid (user)) {
		g_object_unref (user);
		return;
	}

	config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);
	uid_user = oobs_users_config_get_from_uid (config, uid);

	if (oobs_user_is_root (user) && uid != 0) {
		*primary_text   = g_strdup (_("ID for the root user should not be modified"));
		*secondary_text = g_strdup (_("This would leave the system unusable."));
	}
	else if (uid_user) { /* check that UID is free */
		*primary_text   = g_strdup_printf (_("User ID %d is already used by user %s"),
		                                   uid, oobs_user_get_login_name (uid_user));
		*secondary_text = g_strdup_printf (_("Please choose a different numeric identifier for %s."),
		                                   oobs_user_get_login_name (user));
		g_object_unref (uid_user);
	}

	g_object_unref (user);
}

static void
check_shell (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	GtkWidget *widget;
	const gchar *path;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	path = gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (widget))));

	if (strlen (path) < 1 || !g_path_is_absolute (path)) {
		*primary_text = g_strdup (_("Incomplete path in shell"));
		*secondary_text = g_strdup (_("Please enter full path for shell\n"
					      "<span size=\"smaller\">i.e.: /bin/bash</span>."));
	}
}

/*
 * Check that current user is not removing himself from the admin group: warn if it's the case.
 * Also, if selected user is the only admin left, we directly show an error dialog.
 * (This function assumes selected user is an administrator, so check this before calling.)
 */
gboolean
user_settings_check_revoke_admin_rights ()
{
	OobsGroupsConfig *groups_config;
	OobsSelfConfig *self_config;
	OobsUser *user;
	OobsGroup *admin_group;
	GList *members;
	GtkWidget *dialog;
	int response;

	groups_config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
	self_config = OOBS_SELF_CONFIG (GST_USERS_TOOL (tool)->self_config);

	user = users_table_get_current ();
	admin_group = oobs_groups_config_get_from_name (groups_config, ADMIN_GROUP);
	members = oobs_group_get_users (admin_group);

	/* check that user is no alone in the admin group */
	if (g_list_length (members) < 2) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
		                                 GTK_DIALOG_MODAL,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_CLOSE,
		                                 _("Can't revoke administration rights"));

		gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
		                                            _("%s is the only administrator on this computer. "
		                                              "Revoking administration rights for this account "
		                                              "would lock you out of administrating the system."),
		                                            oobs_user_get_full_name_fallback (user));

		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return FALSE;
	}
	/* If there are other admins, ask for confirmation only when dealing
	 * with the current account. This does not catch the case where PolicyKit
	 * authentication was done with another admin account, but this is not critical. */
	else if (user == oobs_self_config_get_user (self_config)) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
		                                 GTK_DIALOG_MODAL,
		                                 GTK_MESSAGE_WARNING,
		                                 GTK_BUTTONS_NONE,
		                                 _("You are about to revoke your own administration rights"));

		gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
		                                            _("%s will no longer be able to perform administrative tasks. "
		                                              "This account won't be allowed to get administration rights back on its own."),
		                                            oobs_user_get_full_name_fallback (user));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
		                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		                        _("Give up administration rights"), GTK_RESPONSE_OK, NULL);

		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return (response == GTK_RESPONSE_OK);
	}

	g_object_unref (admin_group);

	return TRUE;
}

static gboolean
check_profile (OobsUser *user, GstUserProfile *profile)
{
	OobsGroupsConfig *groups_config;
	OobsGroup *admin_group;
	char **l;
	gboolean is_admin_profile;

	groups_config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
	admin_group = oobs_groups_config_get_from_name (groups_config, ADMIN_GROUP);

	if (oobs_user_is_in_group (user, admin_group)) {
		is_admin_profile = FALSE;

		for (l = profile->groups; *l; l++)
			  if (strcmp (ADMIN_GROUP, *l) == 0) {
				  is_admin_profile = TRUE;
				  break;
			  }

		  if (!is_admin_profile)
			  return user_settings_check_revoke_admin_rights (user);
	  }

	g_object_unref (admin_group);

	return TRUE;
}

/*
 * Callback for user_new_name entry: on every change, fill the combo entry
 * with proposed logins. Also update validate button's sensitivity if name is empty.
 */
void
on_user_new_name_changed (GtkEditable *user_name, gpointer user_data)
{
	OobsUsersConfig *config;
	GtkWidget *validate_button;
	GtkWidget *user_login;
	GtkWidget *login_entry;
	GtkTreeModel *model;
	gboolean valid_login;
	gboolean valid_name;
	gboolean used_login;
	const char *name;
	char *lc_name, *ascii_name, *stripped_name;
	char **words1;
	char **words2 = NULL;
	char **w1, **w2;
	char *c;
	char *unicode_fallback = "?";
	GString *first_word, *last_word;
	GString *item1, *item2, *item3, *item4;
	int len;
	int nwords1, nwords2, i;

	validate_button = gst_dialog_get_widget (tool->main_dialog,
	                                         "user_new_validate_button");

	user_login = gst_dialog_get_widget (tool->main_dialog, "user_new_login");
	login_entry = gtk_bin_get_child (GTK_BIN (user_login));
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (user_login));
	gtk_list_store_clear (GTK_LIST_STORE (model));

	name = gtk_entry_get_text (GTK_ENTRY (user_name));

	/* make validate button insensitive if name is empty */
	valid_name = (strlen (name) > 0);

	/* take into account sensitivity set from on_user_new_login_changed() */
	valid_login = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (validate_button),
	                                                  "valid-login"));
	gtk_widget_set_sensitive (validate_button, valid_name && valid_login);
	g_object_set_data (G_OBJECT (validate_button),
	                   "valid-name", GINT_TO_POINTER (valid_name));

	if (!valid_name) {
		gtk_entry_set_text (GTK_ENTRY (login_entry), "");
		return;
	}


	ascii_name = g_convert_with_fallback (name, -1, "ASCII//TRANSLIT", "UTF-8",
	                                      unicode_fallback, NULL, NULL, NULL);

	lc_name = g_ascii_strdown (ascii_name, -1);

	/* remove all non ASCII alphanumeric chars from the name,
	 * apart from the few allowed symbols */
	stripped_name = g_strnfill (strlen (lc_name) + 1, '\0');
	i = 0;
	for (c = lc_name; *c; c++) {
		if ( !(g_ascii_isdigit (*c) || g_ascii_islower (*c)
		       || *c == ' ' || *c == '-' || *c == '.' || *c == '_'
		       /* used to track invalid words, removed below */
		       || *c == '?') )
			continue;

		    stripped_name[i] = *c;
		    i++;
	}

	if (strlen (stripped_name) <= 0) {
		g_free (ascii_name);
		g_free (lc_name);
		g_free (stripped_name);
		return;
	}

	/* we split name on spaces, and then on dashes, so that we can treat words
	 * linked with dashes the same way, i.e. both fully shown, or both abbreviated */
	words1 = g_strsplit_set (stripped_name, " ", -1);
	len = g_strv_length (words1);

	g_free (ascii_name);
	g_free (lc_name);
	g_free (stripped_name);


	/* Concatenate the whole first word with the first letter of each word (item1),
	 * and the last word with the first letter of each word (item2). item3 and item4
	 * are symmetrical respectively to item1 and item2.
	 *
	 * Constant 5 is the max reasonable number of words we may get when
	 * splitting on dashes, since we can't guess it at this point,
	 * and reallocating would be too bad. */
	item1 = g_string_sized_new (strlen (words1[0]) + len - 1 + 5);
	item3 = g_string_sized_new (strlen (words1[0]) + len - 1 + 5);

	item2 = g_string_sized_new (strlen (words1[len - 1]) + len - 1 + 5);
	item4 = g_string_sized_new (strlen (words1[len - 1]) + len - 1 + 5);

	/* again, guess at the max size of names */
	first_word = g_string_sized_new (20);
	last_word = g_string_sized_new (20);

	nwords1 = 0;
	nwords2 = 0;
	for (w1 = words1; *w1; w1++) {
		if (strlen (*w1) == 0)
			continue;

		/* skip words with string '?', most likely resulting
		 * from failed transliteration to ASCII */
		if (strstr (*w1, unicode_fallback) != NULL)
			continue;

		nwords1++; /* count real words, excluding empty string */

		words2 = g_strsplit_set (*w1, "-", -1);

		/* reset last word if a new non-empty word has been found */
		if (strlen (*words2) > 0)
			last_word = g_string_set_size (last_word, 0);

		for (w2 = words2; *w2; w2++) {
			if (strlen (*w2) == 0)
				continue;

			nwords2++;

			/* part of the first "toplevel" real word */
			if (nwords1 == 1) {
				item1 = g_string_append (item1, *w2);
				first_word = g_string_append (first_word, *w2);
			}
			else {
				item1 = g_string_append_unichar (item1,
				                                 g_utf8_get_char (*w2));
				item3 = g_string_append_unichar (item3,
				                                 g_utf8_get_char (*w2));
			}

			/* not part of the last "toplevel" word */
			if (w1 != words1 + len - 1) {
				item2 = g_string_append_unichar (item2,
				                                 g_utf8_get_char (*w2));
				item4 = g_string_append_unichar (item4,
				                                 g_utf8_get_char (*w2));
			}

			/* always save current word so that we have it if last one reveals empty */
			last_word = g_string_append (last_word, *w2);
		}

		g_strfreev (words2);
	}

	item2 = g_string_append (item2, last_word->str);
	item3 = g_string_append (item3, first_word->str);
	item4 = g_string_prepend (item4, last_word->str);

	config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);

	used_login = oobs_users_config_is_login_used (config, item1->str);
	if (nwords2 > 0 && !used_login && !isdigit(item1->str[0]))
		gtk_combo_box_append_text (GTK_COMBO_BOX (user_login), item1->str);

	/* if there's only one word, would be the same as item1 */
	if (nwords2 > 1) {
		/* add other items */
		used_login = oobs_users_config_is_login_used (config, item2->str);
		if (!used_login && !isdigit(item2->str[0]))
			gtk_combo_box_append_text (GTK_COMBO_BOX (user_login), item2->str);

		used_login = oobs_users_config_is_login_used (config, item3->str);
		if (!used_login && !isdigit(item3->str[0]))
			gtk_combo_box_append_text (GTK_COMBO_BOX (user_login), item3->str);

		used_login = oobs_users_config_is_login_used (config, item4->str);
		if (!used_login && !isdigit(item4->str[0]))
			gtk_combo_box_append_text (GTK_COMBO_BOX (user_login), item4->str);

		/* add the last word */
		used_login = oobs_users_config_is_login_used (config, last_word->str);
		if (!used_login && !isdigit(last_word->str[0]))
			gtk_combo_box_append_text (GTK_COMBO_BOX (user_login), last_word->str);

		/* ...and the first one */
		used_login = oobs_users_config_is_login_used (config, first_word->str);
		if (!used_login && !isdigit(first_word->str[0]))
			gtk_combo_box_append_text (GTK_COMBO_BOX (user_login), first_word->str);
	}


	gtk_combo_box_set_active (GTK_COMBO_BOX (user_login), 0);

	g_strfreev (words1);
	g_string_free (first_word, TRUE);
	g_string_free (last_word, TRUE);
	g_string_free (item1, TRUE);
	g_string_free (item2, TRUE);
	g_string_free (item3, TRUE);
	g_string_free (item4, TRUE);
}

/*
 * Callback for user_new login combo box: prevent inserting invalid chars,
 * update notice, icons and validate button sensitivity.
 */
void
on_user_new_login_changed (GtkComboBox *login_combo, gpointer user_data)
{
	GtkWidget *login_entry;
	GtkWidget *validate_button;
	GtkWidget *notice_image;
	GtkWidget *used_notice;
	GtkWidget *login_notice;
	GtkWidget *letter_notice;
	GdkColor color;
	gboolean used_login;
	gboolean empty_login;
	gboolean valid_login;
	gboolean valid_name;
	char *login;
	char *c;

	login = gtk_combo_box_get_active_text (login_combo);
	login_entry = gtk_bin_get_child (GTK_BIN (login_combo));
	validate_button = gst_dialog_get_widget (tool->main_dialog, "user_new_validate_button");
	notice_image = gst_dialog_get_widget (tool->main_dialog, "user_new_notice_image");
	used_notice = gst_dialog_get_widget (tool->main_dialog, "user_new_login_used_notice");
	login_notice = gst_dialog_get_widget (tool->main_dialog, "user_new_login_notice");
	letter_notice = gst_dialog_get_widget (tool->main_dialog, "user_new_login_letter_notice");

	used_login = oobs_users_config_is_login_used (OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config),
	                                              login);
	empty_login = (strlen (login) <= 0);
	valid_login = TRUE;

	if (!used_login && !empty_login) {
		/* first char of a login must be a letter, and it must only composed
		 * of ASCII letters, digits, and a '.', '-', '_' */
		for (c = login; *c; c++) {
			if ( (c == login && !islower (*c))
			    || !(isdigit (*c) || islower (*c)
			         || *c == '.' || *c == '-' || *c == '_' ) )
				valid_login = FALSE;
		}
	}

	if (!empty_login && !isdigit (login[0])) {
		gtk_widget_set_visible (letter_notice, FALSE);
		gtk_widget_set_visible (used_notice, used_login);
		gtk_widget_set_visible (login_notice, !used_login);
	}
	else {
		gtk_widget_set_visible (letter_notice, !empty_login);
		gtk_widget_set_visible (used_notice, FALSE);
		gtk_widget_set_visible (login_notice, empty_login);
	}

	/* don't override sensitivity set from on_user_new_name_changed() */
	valid_name = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (validate_button),
	                                                 "valid-name"));
	gtk_widget_set_sensitive (validate_button,
	                          !used_login && valid_login && valid_name && !empty_login);
	g_object_set_data (G_OBJECT (validate_button), "valid-login",
	                   GINT_TO_POINTER (!used_login && valid_login && !empty_login));

	gtk_image_set_from_stock (GTK_IMAGE (notice_image),
	                          (!used_login && valid_login) || empty_login ? GTK_STOCK_INFO :
		                                                                GTK_STOCK_DIALOG_WARNING,
	                          GTK_ICON_SIZE_LARGE_TOOLBAR);

	gdk_color_parse ("red", &color);
	gtk_widget_modify_base (GTK_WIDGET (login_entry), GTK_STATE_NORMAL,
	                        (!used_login && valid_login) || empty_login ? NULL : &color);
}

/*
 * Callback for user_new button: run the dialog to enter the user's
 * real name and login, and create the user with default settings.
 */
void
on_user_new (GtkButton *button, gpointer user_data)
{
	int response;
	GtkWidget *user_new_dialog;
	GtkWidget *user_name;
	GtkWidget *user_login;
	GtkWidget *login_entry;
	GtkWidget *notice_image;
	GtkWidget *encrypted_home;
	GtkTreeModel *model;
	GtkTreePath *user_path;
	OobsUser *user;
	OobsGroup *main_group;
	const char *fullname, *login;
	GstUserProfile *profile;
	OobsUsersConfig *users_config;
	OobsGroupsConfig *groups_config;
	OobsResult result;
	gboolean encrypt;

	/* Before going further, check for authorizations, authenticating if needed */
	if (!gst_tool_authenticate (tool, GST_USERS_TOOL (tool)->users_config))
		return;

	user_new_dialog = gst_dialog_get_widget (tool->main_dialog, "user_new_dialog");
	user_name = gst_dialog_get_widget (tool->main_dialog, "user_new_name");
	user_login = gst_dialog_get_widget (tool->main_dialog, "user_new_login");
	login_entry = gtk_bin_get_child (GTK_BIN (user_login));
	notice_image = gst_dialog_get_widget (tool->main_dialog, "user_new_notice_image");
	encrypted_home = gst_dialog_get_widget (tool->main_dialog, "user_new_encrypted_home");

	users_config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);
	groups_config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);

	/* clear any text, colors or icon left */
	gtk_entry_set_text (GTK_ENTRY (user_name), "");
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (user_login));
	gtk_list_store_clear (GTK_LIST_STORE (model));
	gtk_combo_box_set_active (GTK_COMBO_BOX (user_login), -1);
	on_user_new_login_changed (GTK_COMBO_BOX (user_login), NULL);

	/* Show the encrypt home check box if supported */
	if (oobs_users_config_get_encrypted_home_support (users_config))
	    gtk_widget_show (encrypted_home);
	else
	    gtk_widget_hide (encrypted_home);

	gtk_window_set_focus (GTK_WINDOW (user_new_dialog), user_name);

	set_login_length (login_entry);

	/* run dialog with correct settings */
	gtk_window_set_transient_for (GTK_WINDOW (user_new_dialog),
	                              GTK_WINDOW (tool->main_dialog));
	gst_dialog_add_edit_dialog (tool->main_dialog, GTK_WIDGET (user_new_dialog));
	response = gtk_dialog_run (GTK_DIALOG (user_new_dialog));
	gst_dialog_remove_edit_dialog (tool->main_dialog, GTK_WIDGET (user_new_dialog));
	gtk_widget_hide (GTK_WIDGET (user_new_dialog));

	if (response != GTK_RESPONSE_OK)
		return;

	/* Create user with base data entered by the user.
	   Main group will be automatically managed by the backends,
	   following the system configuration. */
	login = gtk_combo_box_get_active_text (GTK_COMBO_BOX (user_login));
	fullname = gtk_entry_get_text (GTK_ENTRY (user_name));
	encrypt = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (encrypted_home));
	user = oobs_user_new (login);
	oobs_user_set_full_name (user, fullname);
	oobs_user_set_encrypted_home (user, encrypt);
	oobs_user_set_home_flags (user, OOBS_USER_CHOWN_HOME);

	/* Emptying password and disabling account allows enabling it more easily if asked */
	oobs_user_set_password_empty (user, TRUE);
	oobs_user_set_password_disabled (user, TRUE);

	/* If the user group already exists and /etc/adduser.conf USERGROUPS option is yes,
	 * then adduser will fail, and the backends don't allow reporting that error.
	 * So assume we want the user to be in that group. */
	main_group = oobs_groups_config_get_from_name (groups_config, login);
	if (main_group) {
		oobs_user_set_main_group (user, main_group);
		g_object_unref (main_group);
	}

	g_return_if_fail (user != NULL);

	/* fill settings with values from default profile */
	profile = gst_user_profiles_get_default_profile (GST_USERS_TOOL (tool)->profiles);
	gst_user_profiles_apply (GST_USERS_TOOL (tool)->profiles, profile, user, TRUE);

	/* We need to know about this user before adding it, else we won't be aware
	 * that we triggered the commit, and we will show a "Reload config?" dialog. */
	gst_tool_add_configuration_object (GST_TOOL (tool), OOBS_OBJECT (user), FALSE);

	/* Commit both user and groups config because of possible memberships
	 * added by the profile. Avoid showing the new user or trying to commit
	 * group changes if the user has not been created. */
	result = oobs_users_config_add_user (users_config, user);
	if (result == OOBS_RESULT_OK) {
		gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);

		user_path = users_table_add_user (user);
		users_table_select_path (user_path);
		gtk_tree_path_free (user_path);

		/* Finally, run the password edit dialog.
		 * User can hit cancel, leaving the account disabled */
		on_edit_user_passwd (NULL, NULL);
	}
	else {
		gst_tool_commit_error (tool, result);
	}

	g_object_unref (user);
}

/*
 * Common to all modular edit dialogs: run the dialog after filling
 * the user's name and face and handling window settings.
 */
static int
run_edit_dialog (GtkDialog *dialog, GtkImage *face_image2, GtkLabel *name_label)
{
	OobsUser *user;
	GtkWidget *face_image1;
	GdkPixbuf *face;
	const char *name;
	int response;

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
	response = gtk_dialog_run (dialog);
	gst_dialog_remove_edit_dialog (tool->main_dialog, GTK_WIDGET (dialog));
	gtk_widget_hide (GTK_WIDGET (dialog));

	return response;
}

/*
 * Callback for edit_user_name_button: run the dialog to change the user's
 * real name and apply changes if needed.
 */
void
on_edit_user_name (GtkButton *button, gpointer user_data)
{
	int response;
	GtkWidget *user_name_dialog;
	GtkWidget *user_name_entry;
	GtkWidget *face_image;
	GtkWidget *name_label;
	GtkWidget *user_settings_real_name;
	OobsUser *user;
	const char *fullname;
	gboolean is_self;
	OobsResult result;

	user = users_table_get_current ();

	/* Before going further, check for authorizations, authenticating if needed */
	is_self = oobs_self_config_is_user_self (OOBS_SELF_CONFIG (GST_USERS_TOOL (tool)->self_config),
	                                         user);

	/* Authenticate as self when changing self user, as admin for all other users */
	if (is_self) {
		if (!gst_tool_authenticate (tool, GST_USERS_TOOL (tool)->self_config))
			return;
	}
	else if (!gst_tool_authenticate (tool, OOBS_OBJECT (user)))
		return;

	user_name_dialog = gst_dialog_get_widget (tool->main_dialog, "user_name_dialog");
	user_name_entry = gst_dialog_get_widget (tool->main_dialog, "user_name_entry");
	face_image = gst_dialog_get_widget (tool->main_dialog, "user_name_face");
	name_label = gst_dialog_get_widget (tool->main_dialog, "user_name_name");
	user_settings_real_name = gst_dialog_get_widget (tool->main_dialog,
	                                                 "user_settings_real_name");

	fullname = oobs_user_get_full_name (user);
	gtk_entry_set_text (GTK_ENTRY (user_name_entry), fullname ? fullname : "");
	gtk_editable_select_region (GTK_EDITABLE (user_name_entry), 0, -1);

	response = run_edit_dialog (GTK_DIALOG (user_name_dialog),
	                            GTK_IMAGE (face_image), GTK_LABEL (name_label));

	if (response == GTK_RESPONSE_OK) {
		user_name_entry = gst_dialog_get_widget (tool->main_dialog,
		                                         "user_name_entry");

		fullname = gtk_entry_get_text (GTK_ENTRY (user_name_entry));
		oobs_user_set_full_name (user, fullname);

		if (is_self)
			result = gst_tool_commit (tool, GST_USERS_TOOL (tool)->self_config);
		else
			result = gst_tool_commit (tool, OOBS_OBJECT (user));

		if (result == OOBS_RESULT_OK) {
			gtk_label_set_text (GTK_LABEL (user_settings_real_name), fullname);
			users_table_update_current ();
		}
	}

	g_object_unref (user);
}

/*
 * Callback for edit_user_profile_button: run the dialog to change the user's
 * account type and apply changes if needed.
 */
void
on_edit_user_profile (GtkButton *button, gpointer user_data)
{
	int response;
	GtkWidget *user_profile_dialog;
	GtkWidget *face_image;
	GtkWidget *name_label;
	GtkWidget *table;
	GtkWidget *radio;
	GtkWidget *custom_radio;
	GtkWidget *custom_label;
	GHashTable *radios;
	GHashTableIter iter;
	gpointer key;
	gpointer value;
	OobsUser *user;
	GstUserProfile *profile;

	user = users_table_get_current ();

	/* Before going further, check for authorizations, authenticating if needed */
	if (!gst_tool_authenticate (tool, OOBS_OBJECT (user)))
		return;

	user_profile_dialog = gst_dialog_get_widget (tool->main_dialog, "user_profile_dialog");
	face_image = gst_dialog_get_widget (tool->main_dialog, "user_profile_face");
	name_label = gst_dialog_get_widget (tool->main_dialog, "user_profile_name");
	table = gst_dialog_get_widget (tool->main_dialog, "user_profile_table");
	custom_radio = gst_dialog_get_widget (tool->main_dialog, "user_profile_custom");
	custom_label = gst_dialog_get_widget (tool->main_dialog, "user_profile_custom_label");

	/* select the profile that matches the user settings, if any */
	profile = gst_user_profiles_get_for_user (GST_USERS_TOOL (tool)->profiles,
	                                          user, FALSE);
	radios = (GHashTable *) g_object_get_data (G_OBJECT (table), "radio_buttons");

	if (profile) {
		radio = g_hash_table_lookup (radios, profile->name);
		gtk_widget_set_sensitive (custom_radio, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
		gtk_widget_set_sensitive (custom_label, FALSE);
	}
	else {
		gtk_widget_set_sensitive (custom_radio, TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (custom_radio), TRUE);
		gtk_widget_set_sensitive (custom_label, FALSE);
	}

	response = run_edit_dialog (GTK_DIALOG (user_profile_dialog),
	                            GTK_IMAGE (face_image), GTK_LABEL (name_label));

	/* If a profile is selected, apply it (the "custom" radio button does nothing).
	 * For an existing user, this only means changing shell and privilege groups,
	 * since other settings would break many things. */
	g_hash_table_iter_init (&iter, radios);
	while (g_hash_table_iter_next (&iter, &key, &value))
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (value))) {
			profile = gst_user_profiles_get_from_name (GST_USERS_TOOL (tool)->profiles,
			                                           (char *) key);
			break;
		}

	/* apply if conditions were met, else a message has been displayed in check_profile */
	if (profile && check_profile (user, profile))
	  {
		  gst_user_profiles_apply (GST_USERS_TOOL (tool)->profiles,
		                           profile, user, FALSE);

		  if (response == GTK_RESPONSE_OK) {
			  if (gst_tool_commit (tool, OOBS_OBJECT (user)) == OOBS_RESULT_OK)
				  gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);

			  user_settings_show (user);
		  }
	  }

	g_object_unref (user);
}

static void
on_commit_finish (OobsObject *object, OobsResult result, gpointer user)
{
	if (result == OOBS_RESULT_OK) {
		gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);
		user_settings_show (OOBS_USER (user));
	}
}

void
on_edit_user_advanced (GtkButton *button, gpointer user_data)
{
	GtkWidget *user_advanced_dialog;
	GtkWidget *face_image;
	GtkWidget *name_label;
	GtkWidget *uid_notice;
	GtkWidget *widget;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsUser *user;
	OobsGroup *main_group;
	gboolean password_disabled;
	OobsGroup *no_passwd_login_group;
	int response;

	TestBattery battery[] = {
		check_comments,
		check_uid,
		check_shell,
		NULL
	};

	user = users_table_get_current ();

	/* Before going further, check for authorizations, authenticating if needed */
	if (!gst_tool_authenticate (tool, OOBS_OBJECT (user)))
		return;

	user_advanced_dialog = gst_dialog_get_widget (tool->main_dialog, "user_advanced_dialog");
	face_image = gst_dialog_get_widget (tool->main_dialog, "user_advanced_face");
	name_label = gst_dialog_get_widget (tool->main_dialog, "user_advanced_name");


	/* set various settings */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_room_number");
	set_entry_text (widget, (user) ? oobs_user_get_room_number (user) : NULL);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_wphone");
	set_entry_text (widget, (user) ? oobs_user_get_work_phone_number (user) : NULL);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_hphone");
	set_entry_text (widget, (user) ? oobs_user_get_home_phone_number (user) : NULL);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	set_entry_text (gtk_bin_get_child (GTK_BIN (widget)), oobs_user_get_shell (user));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	set_entry_text (widget, oobs_user_get_home_directory (user));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), oobs_user_get_uid (user));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_locked_account");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), oobs_user_get_password_disabled (user));


	/* show a notice if the user is logged in */
	uid_notice = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid_disabled");

	if (oobs_user_is_root (user)) {
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_widget_hide (uid_notice);
	}
	else if (oobs_user_get_active (user)) {
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_widget_show (uid_notice);
	}
	else {
		gtk_widget_set_sensitive (widget, TRUE);
		gtk_widget_hide (uid_notice);
	}

	privileges_table_set_from_user (user);
	select_main_group (user);


	/* run dialog */
	do {
		response = run_edit_dialog (GTK_DIALOG (user_advanced_dialog),
		                            GTK_IMAGE (face_image), GTK_LABEL (name_label));

		if (response != GTK_RESPONSE_OK) {
			g_object_unref (user);
			return;
		}

	} while (!test_battery_run (battery, GTK_WINDOW (user_advanced_dialog), user_advanced_dialog)
	         || !check_home (user));


	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_room_number");
	oobs_user_set_room_number (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_wphone");
	oobs_user_set_work_phone_number (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_hphone");
	oobs_user_set_home_phone_number (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	oobs_user_set_shell (user, gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (widget)))));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	oobs_user_set_home_directory (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	oobs_user_set_uid (user, gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_locked_account");
	password_disabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	oobs_user_set_password_disabled (user, password_disabled);
	/* Leaving user in the password-less login group would still allow him to login,
	 * which defeats the purpose of disabling account */
	no_passwd_login_group =
		oobs_groups_config_get_from_name (GST_USERS_TOOL (tool)->groups_config,
		                                  NO_PASSWD_LOGIN_GROUP);
	if (password_disabled && no_passwd_login_group)
		oobs_group_remove_user (no_passwd_login_group, user);

	privileges_table_save (user);

	/* Get main group */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
		gtk_tree_model_get (model, &iter,
				    COL_GROUP_OBJECT, &main_group,
				    -1);
		oobs_user_set_main_group (user, main_group);
		g_object_unref (main_group);
	}
	else
		oobs_user_set_main_group (user, NULL);


	/* Need to run async since copying home dir could be slow */
	gst_tool_commit_async (tool, OOBS_OBJECT (user),
	                       _("Applying changes to user settings..."),
	                       on_commit_finish, user);

	g_object_unref (user);
	if (no_passwd_login_group)
		g_object_unref (no_passwd_login_group);
}
