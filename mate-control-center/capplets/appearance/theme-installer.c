/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 The MATE Foundation
 * Written by Thomas Wood <thos@gnome.org>
 *            Jens Granseuer <jensgr@gmx.net>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "appearance.h"

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <unistd.h>

#include "capplet-util.h"
#include "file-transfer-dialog.h"
#include "theme-installer.h"
#include "theme-util.h"

enum {
	THEME_INVALID,
	THEME_ICON,
	THEME_MATE,
	THEME_GTK,
	THEME_ENGINE,
	THEME_MARCO,
	THEME_CURSOR,
	THEME_ICON_CURSOR
};

enum {
	TARGZ,
	TARBZ,
	DIRECTORY
};

static gboolean
cleanup_tmp_dir (GIOSchedulerJob *job,
		 GCancellable *cancellable,
		 const gchar *tmp_dir)
{
	GFile *directory;

	directory = g_file_new_for_path (tmp_dir);
	capplet_file_delete_recursive (directory, NULL);
	g_object_unref (directory);

	return FALSE;
}

static int
file_theme_type (const gchar *dir)
{
	gchar *filename = NULL;
	gboolean exists;

	if (!dir)
		return THEME_INVALID;

	filename = g_build_filename (dir, "index.theme", NULL);
	exists = g_file_test (filename, G_FILE_TEST_IS_REGULAR);

	if (exists) {
		GPatternSpec *pattern;
		gchar *file_contents = NULL;
		gsize file_size;
		gboolean match;

		g_file_get_contents (filename, &file_contents, &file_size, NULL);
		g_free (filename);

		pattern = g_pattern_spec_new ("*[Icon Theme]*");
		match = g_pattern_match_string (pattern, file_contents);
		g_pattern_spec_free (pattern);

		if (match) {
			pattern = g_pattern_spec_new ("*Directories=*");
			match = g_pattern_match_string (pattern, file_contents);
			g_pattern_spec_free (pattern);
			g_free (file_contents);

			if (match) {
				/* check if we have a cursor, too */
				filename = g_build_filename (dir, "cursors", NULL);
				exists = g_file_test (filename, G_FILE_TEST_IS_DIR);
				g_free (filename);

				if (exists)
					return THEME_ICON_CURSOR;
				else
					return THEME_ICON;
			}
			return THEME_CURSOR;
		}

		pattern = g_pattern_spec_new ("*[X-GNOME-Metatheme]*");
		match = g_pattern_match_string (pattern, file_contents);
		g_pattern_spec_free (pattern);
		g_free (file_contents);

		if (match)
			return THEME_MATE;
	} else {
		g_free (filename);
	}

	filename = g_build_filename (dir, "gtk-2.0", "gtkrc", NULL);
	exists = g_file_test (filename, G_FILE_TEST_IS_REGULAR);
	g_free (filename);

	if (exists)
		return THEME_GTK;

	filename = g_build_filename (dir, "metacity-1", "metacity-theme-1.xml", NULL);
	exists = g_file_test (filename, G_FILE_TEST_IS_REGULAR);
	g_free (filename);

	if (exists)
		return THEME_MARCO;

	/* cursor themes don't necessarily have an index.theme */
	filename = g_build_filename (dir, "cursors", NULL);
	exists = g_file_test (filename, G_FILE_TEST_IS_DIR);
	g_free (filename);

	if (exists)
		return THEME_CURSOR;

	filename = g_build_filename (dir, "configure", NULL);
	exists = g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE);
	g_free (filename);

	if (exists)
		return THEME_ENGINE;

	return THEME_INVALID;
}

static void
transfer_cancel_cb (GtkWidget *dialog,
		    gchar *path)
{
	GFile *todelete;

	todelete = g_file_new_for_path (path);
	capplet_file_delete_recursive (todelete, NULL);

	g_object_unref (todelete);
	g_free (path);
	gtk_widget_destroy (dialog);
}

static void
missing_utility_message_dialog (GtkWindow *parent,
				const gchar *utility)
{
	GtkWidget *dialog = gtk_message_dialog_new (parent,
						    GTK_DIALOG_MODAL,
						    GTK_MESSAGE_ERROR,
						    GTK_BUTTONS_OK,
						    _("Cannot install theme"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("The %s utility is not installed."), utility);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/* this works around problems when doing fork/exec in a threaded app
 * with some locks being held/waited on in different threads.
 *
 * we do the idle callback so that the async xfer has finished and
 * cleaned up its vfs job.  otherwise it seems the slave thread gets
 * woken up and it removes itself from the job queue before it is
 * supposed to.  very strange.
 *
 * see bugzilla.gnome.org #86141 for details
 */
static gboolean
process_local_theme_tgz_tbz (GtkWindow *parent,
			     const gchar *util,
			     const gchar *tmp_dir,
			     const gchar *archive)
{
	gboolean rc;
	int status;
	gchar *command, *filename, *zip, *tar;

	if (!(zip = g_find_program_in_path (util))) {
		missing_utility_message_dialog (parent, util);
		return FALSE;
	}
	if (!(tar = g_find_program_in_path ("tar"))) {
		missing_utility_message_dialog (parent, "tar");
		g_free (zip);
		return FALSE;
	}

	filename = g_shell_quote (archive);

	/* this should be something more clever and nonblocking */
	command = g_strdup_printf ("sh -c 'cd \"%s\"; %s -d -c < \"%s\" | %s xf - '",
				   tmp_dir, zip, filename, tar);
	g_free (zip);
	g_free (tar);
	g_free (filename);

	rc = (g_spawn_command_line_sync (command, NULL, NULL, &status, NULL) && status == 0);
	g_free (command);

	if (rc == FALSE) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (parent,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("Cannot install theme"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("There was a problem while extracting the theme."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}

	return rc;
}

static gboolean
process_local_theme_archive (GtkWindow *parent,
			     gint filetype,
			     const gchar *tmp_dir,
			     const gchar *archive)
{
	if (filetype == TARGZ)
		return process_local_theme_tgz_tbz (parent, "gzip", tmp_dir, archive);
	else if (filetype == TARBZ)
		return process_local_theme_tgz_tbz (parent, "bzip2", tmp_dir, archive);
	else
		return FALSE;
}

static void
invalid_theme_dialog (GtkWindow *parent,
		      const gchar *filename,
		      gboolean maybe_theme_engine)
{
	GtkWidget *dialog;
	const gchar *primary = _("There was an error installing the selected file");
	const gchar *secondary = _("\"%s\" does not appear to be a valid theme.");
	const gchar *engine = _("\"%s\" does not appear to be a valid theme. It may be a theme engine which you need to compile.");

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 "%s", primary);
	if (maybe_theme_engine)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), engine, filename);
	else
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), secondary, filename);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static gboolean
mate_theme_install_real (GtkWindow *parent,
			  gint filetype,
			  const gchar *tmp_dir,
			  const gchar *theme_name,
			  gboolean ask_user)
{
	gboolean success = TRUE;
	GtkWidget *dialog, *apply_button;
	GFile *theme_source_dir, *theme_dest_dir;
	GError *error = NULL;
	gint theme_type;
	gchar *target_dir = NULL;

	/* What type of theme is it? */
	theme_type = file_theme_type (tmp_dir);
	switch (theme_type) {
	case THEME_ICON:
	case THEME_CURSOR:
	case THEME_ICON_CURSOR:
		target_dir = g_build_path (G_DIR_SEPARATOR_S,
					   g_get_home_dir (), ".icons",
					   theme_name, NULL);
		break;
	case THEME_MATE:
		target_dir = g_build_path (G_DIR_SEPARATOR_S,
					   g_get_home_dir (), ".themes",
			 		   theme_name, NULL);
		break;
	case THEME_MARCO:
	case THEME_GTK:
		target_dir = g_build_path (G_DIR_SEPARATOR_S,
					   g_get_home_dir (), ".themes",
					   theme_name, NULL);
		break;
	case THEME_ENGINE:
		invalid_theme_dialog (parent, theme_name, TRUE);
		return FALSE;
	default:
		invalid_theme_dialog (parent, theme_name, FALSE);
		return FALSE;
	}

	/* see if there is an icon theme lurking in this package */
	if (theme_type == THEME_MATE) {
		gchar *path;

		path = g_build_path (G_DIR_SEPARATOR_S,
				     tmp_dir, "icons", NULL);
		if (g_file_test (path, G_FILE_TEST_IS_DIR)
		    && (file_theme_type (path) == THEME_ICON)) {
			gchar *new_path, *update_icon_cache;
			GFile *new_file;
			GFile *src_file;

			src_file = g_file_new_for_path (path);
			new_path = g_build_path (G_DIR_SEPARATOR_S,
						 g_get_home_dir (),
						 ".icons",
						 theme_name, NULL);
			new_file = g_file_new_for_path (new_path);

			if (!g_file_move (src_file, new_file, G_FILE_COPY_NONE,
					  NULL, NULL, NULL, &error)) {
				g_warning ("Error while moving from `%s' to `%s': %s",
					   path, new_path, error->message);
				g_error_free (error);
				error = NULL;
			}
			g_object_unref (new_file);
			g_object_unref (src_file);

			/* update icon cache - shouldn't really matter if this fails */
			update_icon_cache = g_strdup_printf ("gtk-update-icon-cache %s", new_path);
			g_spawn_command_line_async (update_icon_cache, NULL);
			g_free (update_icon_cache);

			g_free (new_path);
		}
		g_free (path);
	}

	/* Move the dir to the target dir */
	theme_source_dir = g_file_new_for_path (tmp_dir);
	theme_dest_dir = g_file_new_for_path (target_dir);

	if (!g_file_move (theme_source_dir, theme_dest_dir,
			  G_FILE_COPY_OVERWRITE, NULL, NULL,
			  NULL, &error)) {
		gchar *str;

		str = g_strdup_printf (_("Installation for theme \"%s\" failed."), theme_name);
		dialog = gtk_message_dialog_new (parent,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 "%s",
						 str);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  "%s", error->message);

		g_free (str);
		g_error_free (error);
		error = NULL;

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		success = FALSE;
	} else {
		if (theme_type == THEME_ICON || theme_type == THEME_ICON_CURSOR)
		{
			gchar *update_icon_cache;

			/* update icon cache - shouldn't really matter if this fails */
			update_icon_cache = g_strdup_printf ("gtk-update-icon-cache %s", target_dir);
			g_spawn_command_line_async (update_icon_cache, NULL);

			g_free (update_icon_cache);
		}

		if (ask_user) {
			/* Ask to apply theme (if we can) */
			if (theme_type == THEME_GTK
			    || theme_type == THEME_MARCO
			    || theme_type == THEME_ICON
			    || theme_type == THEME_CURSOR
			    || theme_type == THEME_ICON_CURSOR) {
				/* TODO: currently cannot apply "mate themes" */
				gchar *str;

				str = g_strdup_printf (_("The theme \"%s\" has been installed."), theme_name);
				dialog = gtk_message_dialog_new_with_markup (parent,
									     GTK_DIALOG_MODAL,
									     GTK_MESSAGE_INFO,
									     GTK_BUTTONS_NONE,
									     "<span weight=\"bold\" size=\"larger\">%s</span>",
									     str);
				g_free (str);

				gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
									  _("Would you like to apply it now, or keep your current theme?"));

				gtk_dialog_add_button (GTK_DIALOG (dialog),
						       _("Keep Current Theme"),
						       GTK_RESPONSE_CLOSE);

				apply_button = gtk_button_new_with_label (_("Apply New Theme"));
				gtk_button_set_image (GTK_BUTTON (apply_button),
						      gtk_image_new_from_stock (GTK_STOCK_APPLY,
										GTK_ICON_SIZE_BUTTON));
				gtk_dialog_add_action_widget (GTK_DIALOG (dialog), apply_button, GTK_RESPONSE_APPLY);
				gtk_widget_set_can_default (apply_button, TRUE);
				gtk_widget_show (apply_button);

				gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY);

				if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_APPLY) {
					/* apply theme here! */
					MateConfClient *mateconf_client;

					mateconf_client = mateconf_client_get_default ();

					switch (theme_type) {
					case THEME_GTK:
						mateconf_client_set_string (mateconf_client, GTK_THEME_KEY, theme_name, NULL);
						break;
					case THEME_MARCO:
						mateconf_client_set_string (mateconf_client, MARCO_THEME_KEY, theme_name, NULL);
						break;
					case THEME_ICON:
						mateconf_client_set_string (mateconf_client, ICON_THEME_KEY, theme_name, NULL);
						break;
					case THEME_CURSOR:
						mateconf_client_set_string (mateconf_client, CURSOR_THEME_KEY, theme_name, NULL);
						break;
					case THEME_ICON_CURSOR:
						mateconf_client_set_string (mateconf_client, ICON_THEME_KEY, theme_name, NULL);
						mateconf_client_set_string (mateconf_client, CURSOR_THEME_KEY, theme_name, NULL);
						break;
					default:
						break;
					}

					g_object_unref (mateconf_client);
				}
			} else {
				dialog = gtk_message_dialog_new (parent,
								 GTK_DIALOG_MODAL,
								 GTK_MESSAGE_INFO,
								 GTK_BUTTONS_OK,
								 _("MATE Theme %s correctly installed"),
								 theme_name);
				gtk_dialog_run (GTK_DIALOG (dialog));
			}
			gtk_widget_destroy (dialog);
		}
	}

	g_free (target_dir);

	return success;
}

static void
process_local_theme (GtkWindow  *parent,
		     const char *path)
{
	GtkWidget *dialog;
	gint filetype;

	if (g_str_has_suffix (path, ".tar.gz")
	    || g_str_has_suffix (path, ".tgz")
	    || g_str_has_suffix(path, ".gtp")) {
		filetype = TARGZ;
	} else if (g_str_has_suffix (path, ".tar.bz2")) {
		filetype = TARBZ;
	} else if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
		filetype = DIRECTORY;
	} else {
		gchar *filename;
		filename = g_path_get_basename (path);
		invalid_theme_dialog (parent, filename, FALSE);
		g_free (filename);
		return;
	}

	if (filetype == DIRECTORY) {
		gchar *name = g_path_get_basename (path);
		mate_theme_install_real (parent,
					  filetype,
					  path,
					  name,
					  TRUE);
		g_free (name);
	} else {
		/* Create a temp directory and uncompress file there */
		GDir *dir;
		const gchar *name;
		gchar *tmp_dir;
		gboolean ok;
		gint n_themes;
		GFile *todelete;

		tmp_dir = g_strdup_printf ("%s/.themes/.theme-%u",
					   g_get_home_dir (),
					   g_random_int ());

		if ((g_mkdir (tmp_dir, 0700)) != 0) {
			dialog = gtk_message_dialog_new (parent,
							 GTK_DIALOG_MODAL,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 _("Failed to create temporary directory"));
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			g_free (tmp_dir);
			return;
		}

		if (!process_local_theme_archive (parent, filetype, tmp_dir, path)
		    || ((dir = g_dir_open (tmp_dir, 0, NULL)) == NULL)) {
			g_io_scheduler_push_job ((GIOSchedulerJobFunc) cleanup_tmp_dir,
						 g_strdup (tmp_dir),
						 g_free,
						 G_PRIORITY_DEFAULT,
						 NULL);
			g_free (tmp_dir);
			return;
		}

		todelete = g_file_new_for_path (path);
		g_file_delete (todelete, NULL, NULL);
		g_object_unref (todelete);

		/* See whether we have multiple themes to install. If so,
		 * we won't ask the user whether to apply the new theme
		 * after installation. */
		n_themes = 0;
		for (name = g_dir_read_name (dir);
		     name && n_themes <= 1;
		     name = g_dir_read_name (dir)) {
			gchar *theme_dir;

			theme_dir = g_build_filename (tmp_dir, name, NULL);

			if (g_file_test (theme_dir, G_FILE_TEST_IS_DIR))
				++n_themes;

			g_free (theme_dir);
		}
		g_dir_rewind (dir);

		ok = TRUE;
		for (name = g_dir_read_name (dir); name && ok;
		     name = g_dir_read_name (dir)) {
			gchar *theme_dir;

			theme_dir = g_build_filename (tmp_dir, name, NULL);

			if (g_file_test (theme_dir, G_FILE_TEST_IS_DIR))
				ok = mate_theme_install_real (parent,
							       filetype,
							       theme_dir,
							       name,
							       n_themes == 1);

			g_free (theme_dir);
		}
		g_dir_close (dir);

		if (ok && n_themes > 1) {
			dialog = gtk_message_dialog_new (parent,
							 GTK_DIALOG_MODAL,
							 GTK_MESSAGE_INFO,
							 GTK_BUTTONS_OK,
							 _("New themes have been successfully installed."));
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}
		g_io_scheduler_push_job ((GIOSchedulerJobFunc) cleanup_tmp_dir,
					 tmp_dir, g_free,
					 G_PRIORITY_DEFAULT, NULL);
	}
}

typedef struct
{
	GtkWindow *parent;
	char      *path;
} TransferData;

static void
transfer_done_cb (GtkWidget *dialog,
		  TransferData *tdata)
{
	gdk_threads_enter ();
	/* XXX: path should be on the local filesystem by now? */

	if (dialog != NULL) {
		gtk_widget_destroy (dialog);
	}

	process_local_theme (tdata->parent, tdata->path);

	g_free (tdata->path);
	g_free (tdata);

	gdk_threads_leave ();
}

void
mate_theme_install (GFile *file,
		     GtkWindow *parent)
{
	GtkWidget *dialog;
	gchar *path, *base;
	GList *src, *target;
	const gchar *template;
	TransferData *tdata;

	if (file == NULL) {
		dialog = gtk_message_dialog_new (parent,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("No theme file location specified to install"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	/* see if someone dropped a local directory */
	if (g_file_is_native (file)
	    && g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY) {
		path = g_file_get_path (file);
		process_local_theme (parent, path);
		g_free (path);
		return;
	}

	/* we can't tell if this is an icon theme yet, so just make a
	 * temporary copy in .themes */
	path = g_build_filename (g_get_home_dir (), ".themes", NULL);

	if (access (path, X_OK | W_OK) != 0) {
		dialog = gtk_message_dialog_new (parent,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("Insufficient permissions to install the theme in:\n%s"), path);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_free (path);
		return;
	}

	base = g_file_get_basename (file);

	if (g_str_has_suffix (base, ".tar.gz")
	    || g_str_has_suffix (base, ".tgz")
	    || g_str_has_suffix (base, ".gtp"))
		template = "mate-theme-%d.gtp";
	else if (g_str_has_suffix (base, ".tar.bz2"))
		template = "mate-theme-%d.tar.bz2";
	else {
		invalid_theme_dialog (parent, base, FALSE);
		g_free (base);
		return;
	}
	g_free (base);

	src = g_list_append (NULL, g_object_ref (file));

	path = NULL;
	do {
	  	gchar *file_tmp;

		g_free (path);
    		file_tmp = g_strdup_printf (template, g_random_int ());
	    	path = g_build_filename (g_get_home_dir (), ".themes", file_tmp, NULL);
	  	g_free (file_tmp);
	} while (g_file_test (path, G_FILE_TEST_EXISTS));

	tdata = g_new0 (TransferData, 1);
	tdata->parent = parent;
	tdata->path = path;

	dialog = file_transfer_dialog_new_with_parent (parent);
	g_signal_connect (dialog,
			  "cancel",
			  (GCallback) transfer_cancel_cb, path);
	g_signal_connect (dialog,
			  "done",
			  (GCallback) transfer_done_cb, tdata);

	target = g_list_append (NULL, g_file_new_for_path (path));
	file_transfer_dialog_copy_async (FILE_TRANSFER_DIALOG (dialog),
					 src,
					 target,
					 FILE_TRANSFER_DIALOG_DEFAULT,
					 G_PRIORITY_DEFAULT);
	gtk_widget_show (dialog);

	/* don't free the path since we're using that for the signals */
	g_list_foreach (src, (GFunc) g_object_unref, NULL);
	g_list_free (src);
	g_list_foreach (target, (GFunc) g_object_unref, NULL);
	g_list_free (target);
}

void
mate_theme_installer_run (GtkWindow *parent,
			   const gchar *filename)
{
	static gboolean running_theme_install = FALSE;
	static gchar old_folder[512] = "";
	GtkWidget *dialog;
	GtkFileFilter *filter;

	if (running_theme_install)
		return;

	running_theme_install = TRUE;

	if (filename == NULL)
		filename = old_folder;

	dialog = gtk_file_chooser_dialog_new (_("Select Theme"),
					      parent,
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OPEN,
					      GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Theme Packages"));
	gtk_file_filter_add_mime_type (filter, "application/x-bzip-compressed-tar");
	gtk_file_filter_add_mime_type (filter, "application/x-compressed-tar");
	gtk_file_filter_add_mime_type (filter, "application/x-mate-theme-package");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	if (strcmp (old_folder, ""))
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), old_folder);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *uri_selected, *folder;

		uri_selected = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));

		folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
		g_strlcpy (old_folder, folder, 255);
		g_free (folder);

		gtk_widget_destroy (dialog);

		if (uri_selected != NULL) {
			GFile *file = g_file_new_for_uri (uri_selected);
			g_free (uri_selected);

			mate_theme_install (file, parent);
			g_object_unref (file);
		}
	} else {
		gtk_widget_destroy (dialog);
	}

	/*
	 * we're relying on the mate theme info module to pick up changes
	 * to the themes so we don't need to update the model here
	 */

	running_theme_install = FALSE;
}
