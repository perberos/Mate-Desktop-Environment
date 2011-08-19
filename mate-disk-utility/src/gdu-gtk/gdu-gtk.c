/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-gtk.c
 *
 * Copyright (C) 2007 David Zeuthen
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

#include <glib-object.h>
#include <string.h>
#include <dbus/dbus-glib.h>

#ifdef HAVE_MATE_KEYRING
#include <mate-keyring.h>
#endif

#include <gdu/gdu.h>
#include "gdu-gtk.h"

/* ---------------------------------------------------------------------------------------------------- */

static int
_get_icon_size_for_stock_size (GtkIconSize size)
{
        int w, h;
        if (gtk_icon_size_lookup (size, &w, &h)) {
                return MAX (w, h);
        } else {
                return 24;
        }
}

static GdkPixbuf *
get_pixbuf_for_icon (GIcon *icon)
{
	GdkPixbuf *pixbuf;

	pixbuf = NULL;

	if (G_IS_FILE_ICON (icon)) {
                char *filename;
		filename = g_file_get_path (g_file_icon_get_file (G_FILE_ICON (icon)));
		if (filename != NULL) {
			pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 24, 24, NULL);
                        g_free (filename);
		}

	} else if (G_IS_THEMED_ICON (icon)) {
		const char * const *names;
		char *icon_no_extension;
		char *p;

		names = g_themed_icon_get_names (G_THEMED_ICON (icon));

		if (names != NULL && names[0] != NULL) {
			icon_no_extension = g_strdup (names[0]);
			p = strrchr (icon_no_extension, '.');
			if (p != NULL)
				*p = '\0';
			pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
							   icon_no_extension, 24, 0, NULL);
			g_free (icon_no_extension);
		}
	}

	return pixbuf;
}

typedef struct {
        GduDevice *device;

        GtkWidget *tree_view;
        GtkWidget *dialog;
} ShowBusyData;

static ShowBusyData *
show_busy_data_new (GduDevice *device)
{
        ShowBusyData *data;
        data = g_new0 (ShowBusyData, 1);
        data->device = g_object_ref (device);
        return data;
}

static void
show_busy_data_free (ShowBusyData *data)
{
        g_object_unref (data->device);
        g_free (data);
}

static int
process_compare (GduProcess *a, GduProcess *b)
{
        return gdu_process_get_id (a) - gdu_process_get_id (b);
}

static GtkListStore *
show_busy_get_list_store (ShowBusyData *data, int *num_rows)
{
        GtkListStore *store;
        GtkTreeIter iter;
        GList *processes;
        GError *error;
        GList *l;
        gchar *s;

        if (num_rows != NULL)
                *num_rows = 0;

        store = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDU_TYPE_PROCESS);
        error = NULL;
        processes = gdu_device_filesystem_list_open_files_sync (data->device, &error);
        if (error != NULL) {
                g_warning ("error retrieving list of open files: %s", error->message);
                g_error_free (error);
        }
        processes = g_list_sort (processes, (GCompareFunc) process_compare);
        for (l = processes; l != NULL; l = l->next) {
                GduProcess *process = GDU_PROCESS (l->data);
                char *name;
                char *markup;
                GAppInfo *app_info;
                GdkPixbuf *pixbuf;

                name = NULL;

                app_info = gdu_process_get_app_info (process);
                pixbuf = NULL;
                if (app_info != NULL) {
                        GIcon *icon;

                        name = g_strdup (g_app_info_get_name (app_info));
                        icon = g_app_info_get_icon (app_info);
                        if (icon != NULL) {
                                pixbuf = get_pixbuf_for_icon (icon);
                        }
                } else {
                        const char *command_line;

                        command_line = gdu_process_get_command_line (process);

                        if (command_line != NULL) {
                                char *basename;
                                char *s;
                                int n;

                                for (n = 0; command_line[n] != '\0'; n++) {
                                        if (command_line[n] == ' ')
                                                break;
                                }
                                s = g_strndup (command_line, n);
                                basename = g_path_get_basename (s);
                                g_free (s);

                                /* special handling for common programs without desktop files */
                                if (strcmp (basename, "bash") == 0   ||
                                    strcmp (basename, "-bash") == 0) {
                                        name = g_strdup (C_("application name", "Bourne Again Shell"));
                                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                           "terminal", 24, 0, NULL);
                                } else if (strcmp (basename, "sh") == 0   ||
                                           strcmp (basename, "-sh") == 0) {
                                        name = g_strdup (C_("application name", "Bourne Shell"));
                                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                           "terminal", 24, 0, NULL);
                                } else if (strcmp (basename, "csh") == 0   ||
                                           strcmp (basename, "-csh") == 0) {
                                        name = g_strdup (C_("application name", "C Shell"));
                                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                           "terminal", 24, 0, NULL);
                                } else if (strcmp (basename, "tcsh") == 0   ||
                                           strcmp (basename, "-tcsh") == 0) {
                                        name = g_strdup (C_("application name", "TENEX C Shell"));
                                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                           "terminal", 24, 0, NULL);
                                } else if (strcmp (basename, "zsh") == 0   ||
                                           strcmp (basename, "-zsh") == 0) {
                                        name = g_strdup (C_("application name", "Z Shell"));
                                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                           "terminal", 24, 0, NULL);
                                } else if (strcmp (basename, "ksh") == 0   ||
                                           strcmp (basename, "-ksh") == 0) {
                                        name = g_strdup (C_("application name", "Korn Shell"));
                                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                           "terminal", 24, 0, NULL);
                                } else if (strcmp (basename, "top") == 0) {
                                        name = g_strdup (C_("application name", "Process Viewer (top)"));
                                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                           GTK_STOCK_GOTO_TOP, 24, 0, NULL);
                                } else if (strcmp (basename, "less") == 0) {
                                        name = g_strdup (C_("application name", "Terminal Pager (less)"));
                                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                           GTK_STOCK_FILE, 24, 0, NULL);
                                }

                                if (name != NULL) {
                                } else {
                                        name = g_strdup (basename);
                                }

                                g_free (basename);
                        } else {
                                name = g_strdup (C_("application name", "Unknown"));
                        }
                }

                /* fall back to generic icon */
                if (pixbuf == NULL) {
                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                           "application-x-executable", 24, 0, NULL);
                }

                if (gdu_process_get_owner (process) != getuid ()) {
                        s = g_strdup_printf (_("uid: %d  pid: %d  program: %s"),
                                             gdu_process_get_owner (process),
                                             gdu_process_get_id (process),
                                             gdu_process_get_command_line (process));
                } else {
                        s = g_strdup_printf (_("pid: %d  program: %s"),
                                             gdu_process_get_id (process),
                                             gdu_process_get_command_line (process));
                }
                markup = g_strdup_printf ("<b>%s</b>\n"
                                          "<small>%s</small>",
                                          name,
                                          s);
                g_free (s);

                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter,
                                    0, pixbuf,
                                    1, markup,
                                    2, process,
                                    -1);

                if (num_rows != NULL)
                        *num_rows = (*num_rows) + 1;

                g_free (markup);
                g_free (name);
                if (app_info != NULL)
                        g_object_unref (app_info);
                if (pixbuf != NULL)
                        g_object_unref (pixbuf);
        }
        g_list_foreach (processes, (GFunc) g_object_unref, NULL);
        g_list_free (processes);

        return store;
}

static gboolean
show_busy_timeout (gpointer user_data)
{
        ShowBusyData *data = user_data;
        GtkListStore *store;
        int num_rows;

        store = show_busy_get_list_store (data, &num_rows);
        gtk_tree_view_set_model (GTK_TREE_VIEW (data->tree_view), GTK_TREE_MODEL (store));
        gtk_dialog_set_response_sensitive (GTK_DIALOG (data->dialog), 1, num_rows == 0);
        g_object_unref (store);

        return TRUE;
}

gboolean
gdu_util_dialog_show_filesystem_busy (GtkWidget *parent_window,
                                      GduPresentable *presentable)
{
        gboolean ret;
        GduDevice *device;
        char *window_title;
        GIcon *window_icon;
        GtkWidget *dialog;
        GtkWidget *content_area;
        GtkWidget *action_area;
        GtkWidget *hbox;
        GtkWidget *main_vbox;
        GtkWidget *label;
        GtkWidget *image;
        int response;
        GtkListStore *store;
        GtkWidget *tree_view;
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        GtkWidget *scrolled_window;
        GtkWidget *unmount_button;
        GtkWidget *unmount_image;
        GdkPixbuf *pixbuf;
        ShowBusyData *data;
        guint refresh_timer_id;
        char *text;

        ret = FALSE;
        window_title = NULL;
        window_icon = NULL;
        device = NULL;
        dialog = NULL;
        data = NULL;
        refresh_timer_id = 0;

        device = gdu_presentable_get_device (presentable);
        if (device == NULL) {
                g_warning ("%s: presentable has no device", __FUNCTION__);
                goto out;
        }

        data = show_busy_data_new (device);

        if (gdu_device_is_partition (device)) {
                char *s;
                GduPresentable *enclosing_drive;
                enclosing_drive = gdu_presentable_get_enclosing_presentable (presentable);
                s = gdu_presentable_get_name (enclosing_drive);
                /* todo: icon list */
                window_icon = gdu_presentable_get_icon (enclosing_drive);
                g_object_unref (enclosing_drive);
                /* Translators: %d is the partition number, %s the name of the disk */
                window_title = g_strdup_printf (_("Partition %d on %s"),
                                                gdu_device_partition_get_number (device),
                                                s);
                g_free (s);
        } else {
                window_title = gdu_presentable_get_name (presentable);
                window_icon = gdu_presentable_get_icon (presentable);
        }

        dialog = gtk_dialog_new_with_buttons (window_title,
                                              GTK_WINDOW (parent_window),
                                              GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_NO_SEPARATOR,
                                              NULL);
        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

        gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
        gtk_box_set_spacing (GTK_BOX (content_area), 2);
        gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
        gtk_box_set_spacing (GTK_BOX (action_area), 6);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
        // TODO: no support for GIcon in GtkWindow
        //gtk_window_set_icon_name (GTK_WINDOW (dialog), window_icon);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new (FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

        /* main message */
	label = gtk_label_new (NULL);

        text = g_strconcat ("<b><big>", _("Cannot unmount volume"), "</big></b>", NULL);
        gtk_label_set_markup (GTK_LABEL (label), text);
        g_free (text);

	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (label), FALSE, FALSE, 0);

        /* secondary message */
	label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("One or more applications are using the volume. "
                                                   "Quit the applications, and then try unmounting again."));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (label), FALSE, FALSE, 0);


        store = show_busy_get_list_store (data, NULL);
        tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
        gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
                                     GTK_SELECTION_NONE);
        data->dialog = dialog;
        data->tree_view = tree_view;
        g_object_unref (store);

        column = gtk_tree_view_column_new ();
        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "pixbuf", 0,
                                             NULL);
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "markup", 1,
                                             NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

        gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
	gtk_box_pack_start (GTK_BOX (main_vbox), scrolled_window, TRUE, TRUE, 0);

        GtkWidget *cancel_button;
        cancel_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
        gtk_widget_grab_focus (cancel_button);

        unmount_button = gtk_button_new_with_mnemonic (_("_Unmount"));
        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                           "gdu-unmount",
                                           _get_icon_size_for_stock_size (GTK_ICON_SIZE_BUTTON),
                                           0,
                                           NULL);
        unmount_image = gtk_image_new_from_pixbuf (pixbuf);
        g_object_unref (pixbuf);
        gtk_button_set_image (GTK_BUTTON (unmount_button), unmount_image);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
                                      unmount_button,
                                      1);

        /* refresh list of open files every one second... */
        refresh_timer_id = g_timeout_add_seconds (1, show_busy_timeout, data);

        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 1, FALSE);

        gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 280);
        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        if (response == 1)
                ret = TRUE;

out:
        if (refresh_timer_id > 0)
                g_source_remove (refresh_timer_id);

        g_free (window_title);
        if (window_icon != NULL)
                g_object_unref (window_icon);
        if (device != NULL)
                g_object_unref (device);
        if (dialog != NULL)
                gtk_widget_destroy (dialog);
        if (data != NULL)
                show_busy_data_free (data);
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
        gboolean is_new_password;
        GtkWidget *password_entry;
        GtkWidget *password_entry_new;
        GtkWidget *password_entry_verify;
        GtkWidget *warning_hbox;
        GtkWidget *warning_label;
        GtkWidget *button;
} DialogSecretData;

static void
gdu_util_dialog_secret_update (DialogSecretData *data)
{
        const gchar *current, *new, *verify;
        gchar *s;

        if (data->password_entry != NULL)
                current = gtk_entry_get_text (GTK_ENTRY (data->password_entry));
        else
                current = NULL;

        if (data->password_entry_new != NULL)
                new = gtk_entry_get_text (GTK_ENTRY (data->password_entry_new));
        else
                new = NULL;

        if (data->password_entry_verify != NULL)
                verify = gtk_entry_get_text (GTK_ENTRY (data->password_entry_verify));
        else
                verify = NULL;

        if (g_strcmp0 (new, verify) != 0) {
                gtk_widget_show (data->warning_hbox);
                s = g_strconcat ("<i>", _("Passphrases do not match"), "</i>", NULL);
                gtk_label_set_markup (GTK_LABEL (data->warning_label), s);
                g_free (s);
                gtk_widget_set_sensitive (data->button, FALSE);
        } else if (!data->is_new_password &&
                   (g_strcmp0 (current, "") != 0 || g_strcmp0 (new, "") != 0) && g_strcmp0 (current, new) == 0) {
                gtk_widget_show (data->warning_hbox);
                s = g_strconcat ("<i>", _("Passphrases do not differ"), "</i>", NULL);
                gtk_label_set_markup (GTK_LABEL (data->warning_label), s);
                g_free (s);
                gtk_widget_set_sensitive (data->button, FALSE);
        } else if (g_strcmp0 (new, "") == 0) {
                gtk_widget_show (data->warning_hbox);
                s = g_strconcat ("<i>", _("Passphrase can't be empty"), "</i>", NULL);
                gtk_label_set_markup (GTK_LABEL (data->warning_label), s);
                g_free (s);
                gtk_widget_set_sensitive (data->button, FALSE);
        } else {
                if (data->warning_hbox != NULL)
                        gtk_widget_hide (data->warning_hbox);
                gtk_widget_set_sensitive (data->button, g_strcmp0 (new, "") != 0);
        }
}

static void
gdu_util_dialog_secret_entry_changed (GtkWidget *entry, gpointer user_data)
{
        DialogSecretData *data = user_data;
        gdu_util_dialog_secret_update (data);
}

static void
secret_dialog_device_removed (GduDevice *device, gpointer user_data)
{
        GtkDialog *dialog = GTK_DIALOG (user_data);
        gtk_dialog_response (dialog, GTK_RESPONSE_NONE);
}

static char *
gdu_util_dialog_secret_internal (GtkWidget   *parent_window,
                                 const char  *window_title,
                                 GIcon       *window_icon,
                                 gboolean     is_new_password,
                                 gboolean     is_change_password,
                                 const char  *old_secret_for_change_password,
                                 char       **old_secret_from_dialog,
                                 gboolean     *save_in_keyring,
                                 gboolean     *save_in_keyring_session,
                                 gboolean      indicate_wrong_passphrase,
                                 GduDevice    *device)
{
        int row;
        int response;
        char *secret;
        GtkWidget *dialog;
        GtkWidget *content_area;
        GtkWidget *action_area;
        GtkWidget *image;
	GtkWidget *hbox;
        GtkWidget *main_vbox;
        GtkWidget *vbox;
        GtkWidget *label;
        GtkWidget *table_alignment;
        GtkWidget *table;
        GtkWidget *never_radio_button;
        GtkWidget *session_radio_button;
        GtkWidget *always_radio_button;
        DialogSecretData *data;
        char *text;

        g_return_val_if_fail (parent_window == NULL || GTK_IS_WINDOW (parent_window), NULL);

        session_radio_button = NULL;
        always_radio_button = NULL;

        secret = NULL;
        data = g_new0 (DialogSecretData, 1);
        data->is_new_password = is_new_password;

        dialog = gtk_dialog_new_with_buttons (window_title,
                                              GTK_WINDOW (parent_window),
                                              GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_NO_SEPARATOR,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              NULL);

        if (is_new_password) {
                data->button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("Cr_eate"), 0);
        } else if (is_change_password) {
                data->button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("Change _Passphrase"), 0);
        } else {
                data->button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Unlock"), 0);
        }
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), 0);

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
        gtk_box_set_spacing (GTK_BOX (content_area), 2);
        gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
        gtk_box_set_spacing (GTK_BOX (action_area), 6);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
        // TODO: no support for GIcon in GtkWindow
        //if (window_icon != NULL)
        //        gtk_window_set_icon_name (GTK_WINDOW (dialog), window_icon_name);
        //else
        gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_DIALOG_AUTHENTICATION);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new (FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

	/* main message */
	label = gtk_label_new (NULL);
        if (is_new_password) {
                text = _("To create an encrypted device, choose a passphrase "
                         "to protect it");
        } else if (is_change_password) {
                text = _("To change the passphrase, enter both the current and "
                         "new passphrase");
        } else {
                text = _("Data on this device is stored in an encrypted form "
                         "protected by a passphrase");
        }
        text = g_strconcat ("<b><big>", text, "</big></b>", NULL);
        gtk_label_set_markup (GTK_LABEL (label), text);
        g_free (text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (label), FALSE, FALSE, 0);

	/* secondary message */
	label = gtk_label_new (NULL);
        if (is_new_password) {
                gtk_label_set_markup (GTK_LABEL (label), _("Data on this device will be stored in an encrypted form "
                                                           "protected by a passphrase."));
        } else if (is_change_password) {
                gtk_label_set_markup (GTK_LABEL (label), _("Data on this device is stored in an encrypted form "
                                                           "protected by a passphrase."));
        } else {
                gtk_label_set_markup (GTK_LABEL (label), _("To make the data available for use, enter the "
                                                           "passphrase for the device."));
        }
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (label), FALSE, FALSE, 0);

        if (indicate_wrong_passphrase) {
                label = gtk_label_new (NULL);
                text = g_strconcat ("<b>", _("Incorrect Passphrase. Try again."), "</b>", NULL);
                gtk_label_set_markup (GTK_LABEL (label), text);
                g_free (text);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
                gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (label), FALSE, FALSE, 0);
        }

	/* password entry */
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

	table_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_box_pack_start (GTK_BOX (vbox), table_alignment, FALSE, FALSE, 0);
	table = gtk_table_new (1, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_container_add (GTK_CONTAINER (table_alignment), table);

        row = 0;

        if (is_change_password || is_new_password) {

                if (is_change_password) {
                        label = gtk_label_new_with_mnemonic (_("C_urrent Passphrase:"));
                        gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
                        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                        gtk_table_attach (GTK_TABLE (table), label,
                                          0, 1, row, row + 1,
                                          GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
                        data->password_entry = gtk_entry_new ();
                        gtk_entry_set_visibility (GTK_ENTRY (data->password_entry), FALSE);
                        gtk_table_attach_defaults (GTK_TABLE (table), data->password_entry, 1, 2, row, row + 1);
                        gtk_label_set_mnemonic_widget (GTK_LABEL (label), data->password_entry);

                        row++;
                }

                label = gtk_label_new_with_mnemonic (_("_New Passphrase:"));
                gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                gtk_table_attach (GTK_TABLE (table), label,
                                  0, 1, row, row + 1,
                                  GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
                data->password_entry_new = gtk_entry_new ();
                gtk_entry_set_visibility (GTK_ENTRY (data->password_entry_new), FALSE);
                gtk_table_attach_defaults (GTK_TABLE (table), data->password_entry_new, 1, 2, row, row + 1);
                gtk_label_set_mnemonic_widget (GTK_LABEL (label), data->password_entry_new);

                row++;

                label = gtk_label_new_with_mnemonic (_("_Verify Passphrase:"));
                gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                gtk_table_attach (GTK_TABLE (table), label,
                                  0, 1, row, row + 1,
                                  GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
                data->password_entry_verify = gtk_entry_new ();
                gtk_entry_set_visibility (GTK_ENTRY (data->password_entry_verify), FALSE);
                gtk_table_attach_defaults (GTK_TABLE (table), data->password_entry_verify, 1, 2, row, row + 1);
                gtk_label_set_mnemonic_widget (GTK_LABEL (label), data->password_entry_verify);

                data->warning_hbox = gtk_hbox_new (FALSE, 12);
                image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU);
                data->warning_label = gtk_label_new (NULL);

                gtk_box_pack_start (GTK_BOX (data->warning_hbox), image, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (data->warning_hbox), data->warning_label, FALSE, FALSE, 0);
                hbox = gtk_hbox_new (FALSE, 0);
                gtk_box_pack_start (GTK_BOX (hbox), data->warning_hbox, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (" "), FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

                if (data->password_entry != NULL)
                        g_signal_connect (data->password_entry, "changed",
                                          (GCallback) gdu_util_dialog_secret_entry_changed, data);
                g_signal_connect (data->password_entry_new, "changed",
                                  (GCallback) gdu_util_dialog_secret_entry_changed, data);
                g_signal_connect (data->password_entry_verify, "changed",
                                  (GCallback) gdu_util_dialog_secret_entry_changed, data);

                /* only the verify entry activates the default action */
                gtk_entry_set_activates_default (GTK_ENTRY (data->password_entry_verify), TRUE);

                /* if the old password is supplied (from e.g. the keyring), set it and focus on the new password */
                if (old_secret_for_change_password != NULL) {
                        gtk_entry_set_text (GTK_ENTRY (data->password_entry), old_secret_for_change_password);
                        gtk_widget_grab_focus (data->password_entry_new);
                } else if (is_new_password) {
                        gtk_widget_grab_focus (data->password_entry_new);
                }

        } else {
                label = gtk_label_new_with_mnemonic (_("_Passphrase:"));
                gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
                gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
                gtk_table_attach (GTK_TABLE (table), label,
                                  0, 1, row, row + 1,
                                  GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
                data->password_entry = gtk_entry_new ();
                gtk_entry_set_visibility (GTK_ENTRY (data->password_entry), FALSE);
                gtk_entry_set_activates_default (GTK_ENTRY (data->password_entry), TRUE);
                gtk_table_attach_defaults (GTK_TABLE (table), data->password_entry, 1, 2, row, row + 1);
                gtk_label_set_mnemonic_widget (GTK_LABEL (label), data->password_entry);
        }

        never_radio_button = gtk_radio_button_new_with_mnemonic (
                NULL,
                _("_Forget passphrase immediately"));
        session_radio_button = gtk_radio_button_new_with_mnemonic_from_widget (
                GTK_RADIO_BUTTON (never_radio_button),
                _("Remember passphrase until you _log out"));
        always_radio_button = gtk_radio_button_new_with_mnemonic_from_widget (
                GTK_RADIO_BUTTON (never_radio_button),
                _("_Remember forever"));

        /* preselect Remember Forever if we've retrieved the existing key from the keyring */
        if (is_change_password && old_secret_for_change_password != NULL) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (always_radio_button), TRUE);
        }

        gtk_box_pack_start (GTK_BOX (vbox), never_radio_button, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), session_radio_button, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), always_radio_button, FALSE, FALSE, 0);

        gtk_widget_show_all (dialog);
        gdu_util_dialog_secret_update (data);

        if (device != NULL)
                g_signal_connect (device, "removed", (GCallback) secret_dialog_device_removed, dialog);

        response = gtk_dialog_run (GTK_DIALOG (dialog));

        if (device != NULL)
                g_signal_handlers_disconnect_by_func (device, secret_dialog_device_removed, dialog);

        if (response != 0)
                goto out;

        if (is_new_password) {
                secret = g_strdup (gtk_entry_get_text (GTK_ENTRY (data->password_entry_new)));
        } else if (is_change_password) {
                *old_secret_from_dialog = g_strdup (gtk_entry_get_text (GTK_ENTRY (data->password_entry)));
                secret = g_strdup (gtk_entry_get_text (GTK_ENTRY (data->password_entry_new)));
        } else {
                secret = g_strdup (gtk_entry_get_text (GTK_ENTRY (data->password_entry)));
        }

        if (save_in_keyring != NULL)
                *save_in_keyring = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (always_radio_button));
        if (save_in_keyring_session != NULL)
                *save_in_keyring_session = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (session_radio_button));

out:
        if (data == NULL)
                g_free (data);
        if (dialog != NULL)
                gtk_widget_destroy (dialog);
        return secret;
}

#ifdef HAVE_MATE_KEYRING
static MateKeyringPasswordSchema encrypted_device_password_schema = {
        MATE_KEYRING_ITEM_GENERIC_SECRET,
        {
                { "luks-device-uuid", MATE_KEYRING_ATTRIBUTE_TYPE_STRING },
                { NULL, 0 }
        }
};
#endif

char *
gdu_util_dialog_ask_for_new_secret (GtkWidget      *parent_window,
                                    gboolean       *save_in_keyring,
                                    gboolean       *save_in_keyring_session)
{
        return gdu_util_dialog_secret_internal (parent_window,
                                                _("Enter Passphrase"),
                                                NULL,
                                                TRUE,
                                                FALSE,
                                                NULL,
                                                NULL,
                                                save_in_keyring,
                                                save_in_keyring_session,
                                                FALSE,
                                                NULL);
}

/**
 * gdu_util_dialog_ask_for_secret:
 * @parent_window: Parent window that dialog will be transient for or #NULL.
 * @presentable: A #GduPresentable with a #GduDevice that is encrypted.
 * @bypass_keyring: Set to #TRUE to bypass the keyring.
 * @indicate_wrong_passphrase: Set to #TRUE to display a message that the last
 * entered passphrase was wrong
 * @asked_user: Whether the user was asked for the passphrase
 *
 * Retrieves a secret from the user or the keyring (unless
 * @bypass_keyring is set to #TRUE).
 *
 * Returns: the secret or #NULL if the user cancelled the dialog.
 **/
char *
gdu_util_dialog_ask_for_secret (GtkWidget      *parent_window,
                                GduPresentable *presentable,
                                gboolean        bypass_keyring,
                                gboolean        indicate_wrong_passphrase,
                                gboolean       *asked_user)
{
        char *secret;
        char *password;
        const char *usage;
        const char *uuid;
        gboolean save_in_keyring;
        gboolean save_in_keyring_session;
        GduDevice *device;
        char *window_title;
        GIcon *window_icon;

        window_title = NULL;
        window_icon = NULL;
        device = NULL;
        secret = NULL;
        save_in_keyring = FALSE;
        save_in_keyring_session = FALSE;
        if (asked_user != NULL)
                *asked_user = FALSE;

        device = gdu_presentable_get_device (presentable);
        if (device == NULL) {
                g_warning ("%s: presentable has no device", __FUNCTION__);
                goto out;
        }

        usage = gdu_device_id_get_usage (device);
        uuid = gdu_device_id_get_uuid (device);

        if (strcmp (usage, "crypto") != 0) {
                g_warning ("%s: device is not a crypto device", __FUNCTION__);
                goto out;
        }

        if (uuid == NULL || strlen (uuid) == 0) {
                g_warning ("%s: device has no UUID", __FUNCTION__);
                goto out;
        }

        if (!bypass_keyring) {
#ifdef HAVE_MATE_KEYRING
                password = NULL;
                if (mate_keyring_find_password_sync (&encrypted_device_password_schema,
                                                      &password,
                                                      "luks-device-uuid", uuid,
                                                      NULL) == MATE_KEYRING_RESULT_OK && password != NULL) {
                        /* By contract, the caller is responsible for scrubbing the password
                         * so dupping the string into pageable memory is "fine". Or not?
                         */
                        secret = g_strdup (password);
                        mate_keyring_free_password (password);
                        goto out;
                }
#endif
        }

        if (gdu_device_is_partition (device)) {
                char *s;
                GduPresentable *enclosing_drive;
                enclosing_drive = gdu_presentable_get_enclosing_presentable (presentable);
                s = gdu_presentable_get_name (enclosing_drive);
                /* todo: icon list */
                window_icon = gdu_presentable_get_icon (enclosing_drive);
                g_object_unref (enclosing_drive);
                window_title = g_strdup_printf (_("Partition %d on %s"),
                                                gdu_device_partition_get_number (device),
                                                s);
                g_free (s);
        } else {
                window_title = gdu_presentable_get_name (presentable);
                window_icon = gdu_presentable_get_icon (presentable);
        }

        secret = gdu_util_dialog_secret_internal (parent_window,
                                                  window_title,
                                                  window_icon,
                                                  FALSE,
                                                  FALSE,
                                                  NULL,
                                                  NULL,
                                                  &save_in_keyring,
                                                  &save_in_keyring_session,
                                                  indicate_wrong_passphrase,
                                                  device);

        if (asked_user != NULL)
                *asked_user = TRUE;

#ifdef HAVE_MATE_KEYRING
        if (secret != NULL && (save_in_keyring || save_in_keyring_session)) {
                const char *keyring;
                gchar *name;

                keyring = NULL;
                if (save_in_keyring_session)
                        keyring = MATE_KEYRING_SESSION;

                name = g_strdup_printf (_("LUKS Passphrase for UUID %s"), uuid);

                if (mate_keyring_store_password_sync (&encrypted_device_password_schema,
                                                       keyring,
                                                       name,
                                                       secret,
                                                       "luks-device-uuid", uuid,
                                                       NULL) != MATE_KEYRING_RESULT_OK) {
                        g_warning ("%s: couldn't store passphrase in keyring", __FUNCTION__);
                }

                g_free (name);
        }
#endif

out:
        if (device != NULL)
                g_object_unref (device);
        g_free (window_title);
        if (window_icon != NULL)
                g_object_unref (window_icon);
        return secret;
}

/**
 * gdu_util_dialog_change_secret:
 * @parent_window: Parent window that dialog will be transient for or #NULL.
 * @presentable: A #GduPresentable with a #GduDevice that is encrypted.
 * @old_secret: Return location for old secret.
 * @new_secret: Return location for new secret.
 * @save_in_keyring: Return location for whether the new secret should be saved in the keyring.
 * @save_in_keyring_session: Return location for whether the new secret should be saved in the session keyring.
 * @bypass_keyring: Set to #TRUE to bypass the keyring.
 *
 * Asks the user to change his secret. The secret in the keyring is
 * not updated; that needs to be done manually using the functions
 * gdu_util_delete_secret() and gdu_util_save_secret().
 *
 * Returns: #TRUE if the user agreed to change the secret.
 **/
gboolean
gdu_util_dialog_change_secret (GtkWidget       *parent_window,
                               GduPresentable  *presentable,
                               char           **old_secret,
                               char           **new_secret,
                               gboolean        *save_in_keyring,
                               gboolean        *save_in_keyring_session,
                               gboolean         bypass_keyring,
                               gboolean         indicate_wrong_passphrase)
{
        char *password;
        const char *usage;
        const char *uuid;
        gboolean ret;
        char *old_secret_from_keyring;
        char *window_title;
        GIcon *window_icon;
        GduDevice *device;

        window_title = NULL;
        window_icon = NULL;
        device = NULL;
        *old_secret = NULL;
        *new_secret = NULL;
        old_secret_from_keyring = NULL;
        ret = FALSE;

        device = gdu_presentable_get_device (presentable);
        if (device == NULL) {
                g_warning ("%s: presentable has no device", __FUNCTION__);
                goto out;
        }

        usage = gdu_device_id_get_usage (device);
        uuid = gdu_device_id_get_uuid (device);

        if (strcmp (usage, "crypto") != 0) {
                g_warning ("%s: device is not a crypto device", __FUNCTION__);
                goto out;
        }

        if (uuid == NULL || strlen (uuid) == 0) {
                g_warning ("%s: device has no UUID", __FUNCTION__);
                goto out;
        }

#ifdef HAVE_MATE_KEYRING
        if (!bypass_keyring) {
                password = NULL;
                if (mate_keyring_find_password_sync (&encrypted_device_password_schema,
                                                      &password,
                                                      "luks-device-uuid", uuid,
                                                      NULL) == MATE_KEYRING_RESULT_OK && password != NULL) {
                        /* By contract, the caller is responsible for scrubbing the password
                         * so dupping the string into pageable memory "fine". Or not?
                         */
                        old_secret_from_keyring = g_strdup (password);
                        mate_keyring_free_password (password);
                }
        }
#endif

        if (gdu_device_is_partition (device)) {
                char *s;
                GduPresentable *enclosing_drive;
                enclosing_drive = gdu_presentable_get_enclosing_presentable (presentable);
                s = gdu_presentable_get_name (enclosing_drive);
                /* todo: icon list */
                window_icon = gdu_presentable_get_icon (enclosing_drive);
                g_object_unref (enclosing_drive);
                window_title = g_strdup_printf (_("Partition %d on %s"),
                                                gdu_device_partition_get_number (device),
                                                s);
                g_free (s);
        } else {
                window_title = gdu_presentable_get_name (presentable);
                window_icon = gdu_presentable_get_icon (presentable);
        }

        *new_secret = gdu_util_dialog_secret_internal (parent_window,
                                                       window_title,
                                                       window_icon,
                                                       FALSE,
                                                       TRUE,
                                                       old_secret_from_keyring,
                                                       old_secret,
                                                       save_in_keyring,
                                                       save_in_keyring_session,
                                                       indicate_wrong_passphrase,
                                                       device);

        if (old_secret_from_keyring != NULL) {
                memset (old_secret_from_keyring, '\0', strlen (old_secret_from_keyring));
                g_free (old_secret_from_keyring);
        }

        if (*new_secret == NULL)
                goto out;

        ret = TRUE;

out:
        if (!ret) {
                if (*old_secret != NULL) {
                        memset (*old_secret, '\0', strlen (*old_secret));
                        g_free (*old_secret);
                        *old_secret = NULL;
                }
                if (*new_secret != NULL) {
                        memset (*new_secret, '\0', strlen (*new_secret));
                        g_free (*new_secret);
                        *new_secret = NULL;
                }
        }

        g_free (window_title);
        if (window_icon != NULL)
                g_object_unref (window_icon);
        if (device != NULL)
                g_object_unref (device);
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * gdu_util_delete_confirmation_dialog:
 * @parent_window: parent window for transient dialog
 * @title: the title of the dialog
 * @primary_text: primary text
 * @secondary_text: secondary text
 * @affirmative_action_button_mnemonic: text to use on the affirmative action button
 *
 * Utility to show a confirmation dialog for deletion.
 *
 * Returns: %FALSE if the user canceled, otherwise %TRUE.
 **/
gboolean
gdu_util_delete_confirmation_dialog (GtkWidget   *parent_window,
                                     const char  *title,
                                     const char  *primary_text,
                                     const char  *secondary_text,
                                     const char  *affirmative_action_button_mnemonic)
{
        gboolean ret;
        gint response;
        GtkWidget *dialog;
        GtkWidget *content_area;
        GtkWidget *action_area;
        GtkWidget *hbox;
        GtkWidget *image;
        GtkWidget *main_vbox;
        GtkWidget *label;

        ret = FALSE;

        dialog = gtk_dialog_new_with_buttons (title,
                                              GTK_WINDOW (parent_window),
                                              GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_NO_SEPARATOR,
                                              NULL);
        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
        action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
        gtk_box_set_spacing (GTK_BOX (content_area), 2);
        gtk_container_set_border_width (GTK_CONTAINER (action_area), 5);
        gtk_box_set_spacing (GTK_BOX (action_area), 6);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new (FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

	label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), primary_text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (label), FALSE, FALSE, 0);

	label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), secondary_text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (label), FALSE, FALSE, 0);

        gtk_widget_grab_focus (gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL));
        gtk_dialog_add_button (GTK_DIALOG (dialog), affirmative_action_button_mnemonic, 0);

        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));

        gtk_widget_destroy (dialog);
        if (response != 0) {
                goto out;
        }

        ret = TRUE;

out:
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_util_fstype_combo_box_update_desc_label (GtkWidget *combo_box)
{
        GtkWidget *desc_label;

        desc_label = g_object_get_data (G_OBJECT (combo_box), "gdu-desc-label");
        if (desc_label != NULL) {
                char *s;
                char *fstype;
                char *desc;

                fstype = gdu_util_fstype_combo_box_get_selected (combo_box);
                if (fstype != NULL) {
                        desc = gdu_util_fstype_get_description (fstype);
                        if (desc != NULL) {
                                s = g_strdup_printf ("<small><i>%s</i></small>", desc);
                                gtk_label_set_markup (GTK_LABEL (desc_label), s);
                                g_free (s);
                                g_free (desc);
                        } else {
                                gtk_label_set_markup (GTK_LABEL (desc_label), "");
                        }
                } else {
                        gtk_label_set_markup (GTK_LABEL (desc_label), "");
                }
                g_free (fstype);
        }
}

static GtkListStore *
gdu_util_fstype_combo_box_create_store (GduPool *pool, const char *include_extended_partitions_for_scheme)
{
        GList *l;
        GtkListStore *store;
        GList *known_filesystems;
        GtkTreeIter iter;

        store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

        if (pool != NULL)
                known_filesystems = gdu_pool_get_known_filesystems (pool);
        else
                known_filesystems = NULL;
        for (l = known_filesystems; l != NULL; l = l->next) {
                GduKnownFilesystem *kfs = l->data;
                const char *fstype;
                char *fstype_name;

                fstype = gdu_known_filesystem_get_id (kfs);

                if (strcmp (fstype, "empty") == 0) {
                        fstype_name = g_strdup (_("Empty (don't create a file system)"));
                } else {
                        fstype_name = gdu_util_get_fstype_for_display (fstype, NULL, TRUE);
                        /* TODO */
                }

                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter,
                                    0, fstype,
                                    1, fstype_name,
                                    -1);

                g_free (fstype_name);
        }
        g_list_foreach (known_filesystems, (GFunc) g_object_unref, NULL);
        g_list_free (known_filesystems);

        gtk_list_store_append (store, &iter);
        /* Translators: Used as an option in a combobox of filesystem types, meaning 'do not create a filesystem' */
        gtk_list_store_set (store, &iter,
                            0, "empty",
                            1, _("Empty"),
                            -1);

        if (include_extended_partitions_for_scheme != NULL &&
            strcmp  (include_extended_partitions_for_scheme, "mbr") == 0) {
                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter,
                                    0, "msdos_extended_partition",
                                    1, _("Extended Partition"),
                                    -1);
        }

        return store;
}

void
gdu_util_fstype_combo_box_set_desc_label (GtkWidget *combo_box, GtkWidget *desc_label)
{
        g_object_set_data_full (G_OBJECT (combo_box),
                                "gdu-desc-label",
                                g_object_ref (desc_label),
                                g_object_unref);

        gdu_util_fstype_combo_box_update_desc_label (combo_box);
}

void
gdu_util_fstype_combo_box_rebuild (GtkWidget  *combo_box,
                                   GduPool *pool,
                                   const char *include_extended_partitions_for_scheme)
{
        GtkListStore *store;
        store = gdu_util_fstype_combo_box_create_store (pool, include_extended_partitions_for_scheme);
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
        g_object_unref (store);
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);
}

static void
gdu_util_fstype_combo_box_changed (GtkWidget *combo_box, gpointer user_data)
{
        gdu_util_fstype_combo_box_update_desc_label (combo_box);
}

/**
 * gdu_util_fstype_combo_box_create:
 * @pool: A #GduPool object
 * @include_extended_partitions_for_scheme: if not #NULL, includes
 * extended partition types. This is currently only relevant for
 * Master Boot Record ("mbr") where a single item "Extended Partition"
 * will be returned.
 *
 * Get a combo box with the file system types that the DeviceKit-disks
 * daemon can create.
 *
 * Returns: A #GtkComboBox widget
 **/
GtkWidget *
gdu_util_fstype_combo_box_create (GduPool *pool, const char *include_extended_partitions_for_scheme)
{
        GtkListStore *store;
	GtkCellRenderer *renderer;
        GtkWidget *combo_box;


        combo_box = gtk_combo_box_new ();
        store = gdu_util_fstype_combo_box_create_store (pool, include_extended_partitions_for_scheme);
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
        g_object_unref (store);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"text", 1,
					NULL);

        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);

        g_signal_connect (combo_box, "changed", (GCallback) gdu_util_fstype_combo_box_changed, NULL);

        return combo_box;
}

gboolean
gdu_util_fstype_combo_box_select (GtkWidget *combo_box, const char *fstype)
{
        GtkTreeModel *model;
        GtkTreeIter iter;
        gboolean ret;

        ret = FALSE;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
        gtk_tree_model_get_iter_first (model, &iter);
        do {
                char *iter_fstype;

                gtk_tree_model_get (model, &iter, 0, &iter_fstype, -1);
                if (iter_fstype != NULL && strcmp (fstype, iter_fstype) == 0) {
                        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
                        ret = TRUE;
                }
                g_free (iter_fstype);
        } while (!ret && gtk_tree_model_iter_next (model, &iter));

        return ret;
}

char *
gdu_util_fstype_combo_box_get_selected (GtkWidget *combo_box)
{
        GtkTreeModel *model;
        GtkTreeIter iter;
        char *fstype;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
        fstype = NULL;
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
                gtk_tree_model_get (model, &iter, 0, &fstype, -1);

        return fstype;
}


/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        const char *part_scheme;
        GtkListStore *store;
} PartTypeData;

static void
part_type_foreach_cb (const char *scheme,
                      const char *type,
                      const char *name,
                      gpointer user_data)
{
        PartTypeData *data = (PartTypeData *) user_data;
        GtkTreeIter iter;

        if (strcmp (scheme, data->part_scheme) != 0)
                return;

        gtk_list_store_append (data->store, &iter);
        gtk_list_store_set (data->store, &iter,
                            0, type,
                            1, _(name),
                            -1);
}

static GtkListStore *
gdu_util_part_type_combo_box_create_store (const char *part_scheme)
{
        GtkListStore *store;
        PartTypeData data;

        store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        if (part_scheme == NULL)
                goto out;

        data.part_scheme = part_scheme;
        data.store = store;

        gdu_util_part_type_foreach (part_type_foreach_cb, &data);

out:
        return store;
}

void
gdu_util_part_type_combo_box_rebuild (GtkWidget  *combo_box, const char *part_scheme)
{
        GtkListStore *store;
        store = gdu_util_part_type_combo_box_create_store (part_scheme);
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
        g_object_unref (store);
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);
}


/**
 * gdu_util_part_type_combo_box_create:
 * @part_scheme: Partitioning scheme to get partitions types for.
 *
 * Get a combo box with the partition types for a given scheme.
 *
 * Returns: A #GtkComboBox widget
 **/
GtkWidget *
gdu_util_part_type_combo_box_create (const char *part_scheme)
{
        GtkListStore *store;
	GtkCellRenderer *renderer;
        GtkWidget *combo_box;

        combo_box = gtk_combo_box_new ();
        store = gdu_util_part_type_combo_box_create_store (part_scheme);
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
        g_object_unref (store);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"text", 1,
					NULL);

        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);

        return combo_box;
}


gboolean
gdu_util_part_type_combo_box_select (GtkWidget *combo_box, const char *part_type)
{
        GtkTreeModel *model;
        GtkTreeIter iter;
        gboolean ret;

        ret = FALSE;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
        if (gtk_tree_model_get_iter_first (model, &iter)) {
                do {
                        char *iter_part_type;

                        gtk_tree_model_get (model, &iter, 0, &iter_part_type, -1);
                        if (iter_part_type != NULL && strcmp (part_type, iter_part_type) == 0) {
                                gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
                                ret = TRUE;
                        }
                        g_free (iter_part_type);
                } while (!ret && gtk_tree_model_iter_next (model, &iter));
        }

        return ret;
}

char *
gdu_util_part_type_combo_box_get_selected (GtkWidget *combo_box)
{
        GtkTreeModel *model;
        GtkTreeIter iter;
        char *part_type;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
        part_type = NULL;
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
                gtk_tree_model_get (model, &iter, 0, &part_type, -1);

        return part_type;
}

/* ---------------------------------------------------------------------------------------------------- */

static GtkListStore *
gdu_util_part_table_type_combo_box_create_store (void)
{
        GtkListStore *store;
        GtkTreeIter iter;

        store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

        /* TODO: get from daemon */
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            0, "mbr",
                            1, _("Master Boot Record"),
                            -1);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            0, "gpt",
                            1, _("GUID Partition Table"),
                            -1);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            0, "none",
                            1, _("Don't partition"),
                            -1);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            0, "apm",
                            1, _("Apple Partition Map"),
                            -1);

        return store;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_util_part_table_type_combo_box_update_desc_label (GtkWidget *combo_box)
{
        GtkWidget *desc_label;

        desc_label = g_object_get_data (G_OBJECT (combo_box), "gdu-desc-label");
        if (desc_label != NULL) {
                char *s;
                char *fstype;
                char *desc;

                fstype = gdu_util_part_table_type_combo_box_get_selected (combo_box);
                if (fstype != NULL) {
                        desc = gdu_util_part_table_type_get_description (fstype);
                        if (desc != NULL) {
                                s = g_strdup_printf ("<small><i>%s</i></small>", desc);
                                gtk_label_set_markup (GTK_LABEL (desc_label), s);
                                g_free (s);
                                g_free (desc);
                        } else {
                                gtk_label_set_markup (GTK_LABEL (desc_label), "");
                        }
                } else {
                        gtk_label_set_markup (GTK_LABEL (desc_label), "");
                }
                g_free (fstype);
        }
}

static void
gdu_util_part_table_type_combo_box_changed (GtkWidget *combo_box, gpointer user_data)
{
        gdu_util_part_table_type_combo_box_update_desc_label (combo_box);
}

void
gdu_util_part_table_type_combo_box_set_desc_label (GtkWidget *combo_box, GtkWidget *desc_label)
{
        g_object_set_data_full (G_OBJECT (combo_box),
                                "gdu-desc-label",
                                g_object_ref (desc_label),
                                g_object_unref);

        gdu_util_part_table_type_combo_box_update_desc_label (combo_box);
}

/**
 * gdu_util_part_table_type_combo_box_create:
 *
 * Get a combo box with the partition tables types we can create.
 *
 * Returns: A #GtkComboBox widget
 **/
GtkWidget *
gdu_util_part_table_type_combo_box_create (void)
{
        GtkListStore *store;
	GtkCellRenderer *renderer;
        GtkWidget *combo_box;

        combo_box = gtk_combo_box_new ();
        store = gdu_util_part_table_type_combo_box_create_store ();
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
        g_object_unref (store);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), renderer,
					"text", 1,
					NULL);

        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);

        g_signal_connect (combo_box, "changed", (GCallback) gdu_util_part_table_type_combo_box_changed, NULL);

        return combo_box;
}

gboolean
gdu_util_part_table_type_combo_box_select (GtkWidget *combo_box, const char *part_table_type)
{
        GtkTreeModel *model;
        GtkTreeIter iter;
        gboolean ret;

        ret = FALSE;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
        gtk_tree_model_get_iter_first (model, &iter);
        do {
                char *iter_part_table_type;

                gtk_tree_model_get (model, &iter, 0, &iter_part_table_type, -1);
                if (iter_part_table_type != NULL && strcmp (part_table_type, iter_part_table_type) == 0) {
                        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
                        ret = TRUE;
                }
                g_free (iter_part_table_type);
        } while (!ret && gtk_tree_model_iter_next (model, &iter));

        return ret;
}

char *
gdu_util_part_table_type_combo_box_get_selected (GtkWidget *combo_box)
{
        GtkTreeModel *model;
        GtkTreeIter iter;
        char *part_table_type;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
        part_table_type = NULL;
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
                gtk_tree_model_get (model, &iter, 0, &part_table_type, -1);

        return part_table_type;
}

/* ---------------------------------------------------------------------------------------------------- */

GdkPixbuf *
gdu_util_get_pixbuf_for_presentable (GduPresentable *presentable, GtkIconSize size)
{
        gint icon_width, icon_height;

        if (!gtk_icon_size_lookup (size, &icon_width, &icon_height))
                icon_height = 48;

        return gdu_util_get_pixbuf_for_presentable_at_pixel_size (presentable, icon_height);
}

GdkPixbuf *
gdu_util_get_pixbuf_for_presentable_at_pixel_size (GduPresentable *presentable, gint pixel_size)
{
        GIcon *icon;
        GdkPixbuf *pixbuf;

        icon = gdu_presentable_get_icon (presentable);

        pixbuf = NULL;
        if (icon != NULL) {
                GtkIconInfo *icon_info;

                icon_info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
                                                            icon,
                                                            pixel_size,
                                                            GTK_ICON_LOOKUP_GENERIC_FALLBACK);
                if (icon_info == NULL)
                        goto out;

                pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
                gtk_icon_info_free (icon_info);

                if (pixbuf == NULL)
                        goto out;

        }

 out:
        if (icon != NULL)
                g_object_unref (icon);

        return pixbuf;
}

/* ---------------------------------------------------------------------------------------------------- */

void
gdu_util_get_mix_color (GtkWidget    *widget,
                        GtkStateType  state,
                        gchar        *color_buf,
                        gsize         color_buf_size)
{
        GtkStyle *style;
        GdkColor color = {0};

        g_return_if_fail (GTK_IS_WIDGET (widget));
        g_return_if_fail (color_buf != NULL);

        /* This color business shouldn't be this hard... */
        style = gtk_widget_get_style (widget);
#define BLEND_FACTOR 0.7
        color.red   = style->text[state].red   * BLEND_FACTOR +
                      style->base[state].red   * (1.0 - BLEND_FACTOR);
        color.green = style->text[state].green * BLEND_FACTOR +
                      style->base[state].green * (1.0 - BLEND_FACTOR);
        color.blue  = style->text[state].blue  * BLEND_FACTOR +
                      style->base[state].blue  * (1.0 - BLEND_FACTOR);
#undef BLEND_FACTOR
        snprintf (color_buf,
                  color_buf_size, "#%02x%02x%02x",
                  (color.red >> 8),
                  (color.green >> 8),
                  (color.blue >> 8));
}
