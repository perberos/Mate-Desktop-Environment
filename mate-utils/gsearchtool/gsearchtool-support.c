/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * MATE Search Tool
 *
 *  File:  gsearchtool-support.c
 *
 *  (C) 2002 the Free Software Foundation
 *
 *  Authors:	Dennis Cranston  <dennis_cranston@yahoo.com>
 *		George Lebl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <regex.h>
#include <stdlib.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "gsearchtool.h"
#include "gsearchtool-callbacks.h"
#include "gsearchtool-support.h"

#define C_STANDARD_STRFTIME_CHARACTERS "aAbBcdHIjmMpSUwWxXyYZ"
#define C_STANDARD_NUMERIC_STRFTIME_CHARACTERS "dHIjmMSUwWyY"
#define SUS_EXTENDED_STRFTIME_MODIFIERS "EO"
#define BINARY_EXEC_MIME_TYPE      "application/x-executable"
#define GSEARCH_DATE_FORMAT_LOCALE "locale"
#define GSEARCH_DATE_FORMAT_ISO    "iso"

GtkTreeViewColumn *
gsearchtool_gtk_tree_view_get_column_with_sort_column_id (GtkTreeView * treeview,
                                                          gint id);

/* START OF THE MATECONF FUNCTIONS */

static gboolean
gsearchtool_mateconf_handle_error (GError ** error)
{
	if (error != NULL) {
		if (*error != NULL) {
			g_warning (_("MateConf error:\n  %s"), (*error)->message);
			g_error_free (*error);
			*error = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

static MateConfClient *
gsearchtool_mateconf_client_get_global (void)
{
	static MateConfClient * global_mateconf_client = NULL;

	/* Initialize mateconf if needed */
	if (!mateconf_is_initialized ()) {
		char *argv[] = { "gsearchtool-preferences", NULL };
		GError *error = NULL;

		if (!mateconf_init (1, argv, &error)) {
			if (gsearchtool_mateconf_handle_error (&error)) {
				return NULL;
			}
		}
	}

	if (global_mateconf_client == NULL) {
		global_mateconf_client = mateconf_client_get_default ();
	}

	return global_mateconf_client;
}

char *
gsearchtool_mateconf_get_string (const gchar * key)
{
	MateConfClient * client;
	GError * error = NULL;
	gchar * result;

	g_return_val_if_fail (key != NULL, NULL);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);

	result = mateconf_client_get_string (client, key, &error);

	if (gsearchtool_mateconf_handle_error (&error)) {
		result = g_strdup ("");
	}

	return result;
}

void
gsearchtool_mateconf_set_string (const gchar * key,
                              const gchar * value)
{
	MateConfClient * client;
	GError * error = NULL;

	g_return_if_fail (key != NULL);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_string (client, key, value, &error);

	gsearchtool_mateconf_handle_error (&error);			      
			      
}

GSList *
gsearchtool_mateconf_get_list (const gchar * key,
                            MateConfValueType list_type)
{
	MateConfClient * client;
	GError * error = NULL;
	GSList * result;

	g_return_val_if_fail (key != NULL, FALSE);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);

	result = mateconf_client_get_list (client, key, list_type, &error);

	if (gsearchtool_mateconf_handle_error (&error)) {
		result = NULL;
	}

	return result;
}

void
gsearchtool_mateconf_set_list (const gchar * key,
                            GSList * list,
                            MateConfValueType list_type)
{
	MateConfClient * client;
	GError * error = NULL;

	g_return_if_fail (key != NULL);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_list (client, key, list_type, list, &error);

	gsearchtool_mateconf_handle_error (&error);
}

gint
gsearchtool_mateconf_get_int (const gchar * key)
{
	MateConfClient * client;
	GError * error = NULL;
	gint result;

	g_return_val_if_fail (key != NULL, FALSE);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, FALSE);

	result = mateconf_client_get_int (client, key, &error);

	if (gsearchtool_mateconf_handle_error (&error)) {
		result = 0;
	}

	return result;
}

void
gsearchtool_mateconf_set_int (const gchar * key,
                           const gint value)
{
	MateConfClient * client;
	GError * error = NULL;

	g_return_if_fail (key != NULL);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_int (client, key, value, &error);

	gsearchtool_mateconf_handle_error (&error);
}

gboolean
gsearchtool_mateconf_get_boolean (const gchar * key)
{
	MateConfClient * client;
	GError * error = NULL;
	gboolean result;

	g_return_val_if_fail (key != NULL, FALSE);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, FALSE);

	result = mateconf_client_get_bool (client, key, &error);

	if (gsearchtool_mateconf_handle_error (&error)) {
		result = FALSE;
	}

	return result;
}

void
gsearchtool_mateconf_set_boolean (const gchar * key,
                               const gboolean flag)
{
	MateConfClient * client;
	GError * error = NULL;

	g_return_if_fail (key != NULL);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_bool (client, key, flag, &error);

	gsearchtool_mateconf_handle_error (&error);
}

void
gsearchtool_mateconf_add_dir (const gchar * dir)
{
	MateConfClient * client;
	GError * error = NULL;

	g_return_if_fail (dir != NULL);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_add_dir (client,
	                      dir,
	                      MATECONF_CLIENT_PRELOAD_RECURSIVE,
	                      &error);

	gsearchtool_mateconf_handle_error (&error);
}

void
gsearchtool_mateconf_watch_key (const gchar * dir,
                             const gchar * key,
                             MateConfClientNotifyFunc callback,
                             gpointer user_data)
{
	MateConfClient * client;
	GError * error = NULL;

	g_return_if_fail (key != NULL);
	g_return_if_fail (dir != NULL);

	client = gsearchtool_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_add_dir (client,
	                      dir,
	                      MATECONF_CLIENT_PRELOAD_NONE,
	                      &error);

	gsearchtool_mateconf_handle_error (&error);

	mateconf_client_notify_add (client,
	                         key,
	                         callback,
	                         user_data,
	                         NULL,
	                         &error);

	gsearchtool_mateconf_handle_error (&error);
}

/* START OF GENERIC MATE-SEARCH-TOOL FUNCTIONS */

gboolean
is_path_hidden (const gchar * path)
{
	gint results = FALSE;
	gchar * sub_str;
	gchar * hidden_path_substr = g_strconcat (G_DIR_SEPARATOR_S, ".", NULL);

	sub_str = g_strstr_len (path, strlen (path), hidden_path_substr);

	if (sub_str != NULL) {
		gchar * mate_desktop_str;

		mate_desktop_str = g_strconcat (G_DIR_SEPARATOR_S, ".mate-desktop", G_DIR_SEPARATOR_S, NULL);

		/* exclude the .mate-desktop folder */
		if (strncmp (sub_str, mate_desktop_str, strlen (mate_desktop_str)) == 0) {
			sub_str++;
			results = (g_strstr_len (sub_str, strlen (sub_str), hidden_path_substr) != NULL);
		}
		else {
			results = TRUE;
		}

		g_free (mate_desktop_str);
	}

	g_free (hidden_path_substr);
	return results;
}

gboolean
is_quick_search_excluded_path (const gchar * path)
{
	GSList     * exclude_path_list;
	GSList     * tmp_list;
	gchar      * dir;
	gboolean     results = FALSE;

	dir = g_strdup (path);

	/* Remove trailing G_DIR_SEPARATOR. */
	if ((strlen (dir) > 1) && (g_str_has_suffix (dir, G_DIR_SEPARATOR_S) == TRUE)) {
		dir[strlen (dir) - 1] = '\0';
	}

	/* Always exclude a path that is symbolic link. */
	if (g_file_test (dir, G_FILE_TEST_IS_SYMLINK)) {
		g_free (dir);

		return TRUE;
	}
	g_free (dir);

	/* Check path against the Quick-Search-Excluded-Paths list. */
	exclude_path_list = gsearchtool_mateconf_get_list ("/apps/mate-search-tool/quick_search_excluded_paths",
	                                                MATECONF_VALUE_STRING);

	for (tmp_list = exclude_path_list; tmp_list; tmp_list = tmp_list->next) {

		/* Skip empty or null values. */
		if ((tmp_list->data == NULL) || (strlen (tmp_list->data) == 0)) {
			continue;
		}

		dir = g_strdup (tmp_list->data);

		/* Wild-card comparisons. */
		if (g_strstr_len (dir, strlen (dir), "*") != NULL) {

			if (g_pattern_match_simple (dir, path) == TRUE) {

				results = TRUE;
				g_free (dir);
				break;
			}
		}
		/* Non-wild-card comparisons. */
		else {
			/* Add a trailing G_DIR_SEPARATOR. */
			if (g_str_has_suffix (dir, G_DIR_SEPARATOR_S) == FALSE) {

				gchar *tmp;

				tmp = dir;
				dir = g_strconcat (dir, G_DIR_SEPARATOR_S, NULL);
				g_free (tmp);
			}

			if (strcmp (path, dir) == 0) {

				results = TRUE;
				g_free (dir);
				break;
			}
		}
		g_free (dir);
	}

	for (tmp_list = exclude_path_list; tmp_list; tmp_list = tmp_list->next) {
		g_free (tmp_list->data);
	}
	g_slist_free (exclude_path_list);

	return results;
}

gboolean
is_second_scan_excluded_path (const gchar * path)
{
	GSList     * exclude_path_list;
	GSList     * tmp_list;
	gchar      * dir;
	gboolean     results = FALSE;

	dir = g_strdup (path);

	/* Remove trailing G_DIR_SEPARATOR. */
	if ((strlen (dir) > 1) && (g_str_has_suffix (dir, G_DIR_SEPARATOR_S) == TRUE)) {
		dir[strlen (dir) - 1] = '\0';
	}

	/* Always exclude a path that is symbolic link. */
	if (g_file_test (dir, G_FILE_TEST_IS_SYMLINK)) {
		g_free (dir);

		return TRUE;
	}
	g_free (dir);

	/* Check path against the Quick-Search-Excluded-Paths list. */
	exclude_path_list = gsearchtool_mateconf_get_list ("/apps/mate-search-tool/quick_search_second_scan_excluded_paths",
	                                                MATECONF_VALUE_STRING);

	for (tmp_list = exclude_path_list; tmp_list; tmp_list = tmp_list->next) {

		/* Skip empty or null values. */
		if ((tmp_list->data == NULL) || (strlen (tmp_list->data) == 0)) {
			continue;
		}

		dir = g_strdup (tmp_list->data);

		/* Wild-card comparisons. */
		if (g_strstr_len (dir, strlen (dir), "*") != NULL) {

			if (g_pattern_match_simple (dir, path) == TRUE) {

				results = TRUE;
				g_free (dir);
				break;
			}
		}
		/* Non-wild-card comparisons. */
		else {
			/* Add a trailing G_DIR_SEPARATOR. */
			if (g_str_has_suffix (dir, G_DIR_SEPARATOR_S) == FALSE) {

				gchar *tmp;

				tmp = dir;
				dir = g_strconcat (dir, G_DIR_SEPARATOR_S, NULL);
				g_free (tmp);
			}

			if (strcmp (path, dir) == 0) {

				results = TRUE;
				g_free (dir);
				break;
			}
		}
		g_free (dir);
	}

	for (tmp_list = exclude_path_list; tmp_list; tmp_list = tmp_list->next) {
		g_free (tmp_list->data);
	}
	g_slist_free (exclude_path_list);

	return results;
}

gboolean
compare_regex (const gchar * regex,
	       const gchar * string)
{
	regex_t regexec_pattern;

	if (regex == NULL) {
		return TRUE;
	}

	if (!regcomp (&regexec_pattern, regex, REG_EXTENDED|REG_NOSUB)) {
		if (regexec (&regexec_pattern, string, 0, 0, 0) != REG_NOMATCH) {
			regfree (&regexec_pattern);
			return TRUE;
		}
		regfree (&regexec_pattern);
	}
	return FALSE;
}

gboolean
limit_string_to_x_lines (GString * string,
			 gint x)
{
	int i;
	int count = 0;
	for (i = 0; string->str[i] != '\0'; i++) {
		if (string->str[i] == '\n') {
			count++;
			if (count == x) {
				g_string_truncate (string, i);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static gint
count_of_char_in_string (const gchar * string,
			 const gchar c)
{
	int cnt = 0;
	for(; *string; string++) {
		if (*string == c) cnt++;
	}
	return cnt;
}

gchar *
escape_single_quotes (const gchar * string)
{
	GString * gs;

	if (string == NULL) {
		return NULL;
	}

	if (count_of_char_in_string (string, '\'') == 0) {
		return g_strdup(string);
	}
	gs = g_string_new ("");
	for(; *string; string++) {
		if (*string == '\'') {
			g_string_append(gs, "'\\''");
		}
		else {
			g_string_append_c(gs, *string);
		}
	}
	return g_string_free (gs, FALSE);
}

gchar *
escape_double_quotes (const gchar * string)
{
	GString * gs;

	if (string == NULL) {
		return NULL;
	}

	if (count_of_char_in_string (string, '\"') == 0) {
		return g_strdup(string);
	}
	gs = g_string_new ("");
	for(; *string; string++) {
		if (*string == '\"') {
			g_string_append(gs, "\\\"");
		}
		else {
			g_string_append_c(gs, *string);
		}
	}
	return g_string_free (gs, FALSE);
}

gchar *
backslash_backslash_characters (const gchar * string)
{
	GString * gs;

	if (string == NULL) {
		return NULL;
	}

	if (count_of_char_in_string (string, '\\') == 0){
		return g_strdup(string);
	}
	gs = g_string_new ("");
	for(; *string; string++) {
		if (*string == '\\') {
			g_string_append(gs, "\\\\");
		}
		else {
			g_string_append_c(gs, *string);
		}
	}
	return g_string_free (gs, FALSE);
}

gchar *
backslash_special_characters (const gchar * string)
{
	GString * gs;

	if (string == NULL) {
		return NULL;
	}

	if ((count_of_char_in_string (string, '\\') == 0) && 
	    (count_of_char_in_string (string, '-') == 0)) {
		return g_strdup(string);
	}
	gs = g_string_new ("");
	for(; *string; string++) {
		if (*string == '\\') {
			g_string_append(gs, "\\\\");
		}
		if (*string == '-') {
			g_string_append(gs, "\\-");
		}
		else {
			g_string_append_c(gs, *string);
		}
	}
	return g_string_free (gs, FALSE);
}

gchar *
remove_mnemonic_character (const gchar * string)
{
	GString * gs;
	gboolean first_mnemonic = TRUE;

	if (string == NULL) {
		return NULL;
	}

	gs = g_string_new ("");
	for(; *string; string++) {
		if ((first_mnemonic) && (*string == '_')) {
			first_mnemonic = FALSE;
			continue;
		}
		g_string_append_c(gs, *string);
	}
	return g_string_free (gs, FALSE);
}

gchar *
get_readable_date (const gchar * format_string,
                   const time_t file_time_raw)
{
	struct tm * file_time;
	gchar * format;
	GDate * today;
	GDate * file_date;
	guint32 file_date_age;
	gchar * readable_date;

	file_time = localtime (&file_time_raw);

	/* Base format of date column on caja date_format key */
	if (format_string != NULL) {
		if (strcmp(format_string, GSEARCH_DATE_FORMAT_LOCALE) == 0) {
			return gsearchtool_strdup_strftime ("%c", file_time);
		} else if (strcmp (format_string, GSEARCH_DATE_FORMAT_ISO) == 0) {
			return gsearchtool_strdup_strftime ("%Y-%m-%d %H:%M:%S", file_time);
		}
	}

	file_date = g_date_new_dmy (file_time->tm_mday,
			       file_time->tm_mon + 1,
			       file_time->tm_year + 1900);

	today = g_date_new ();
	g_date_set_time_t (today, time (NULL));

	file_date_age = g_date_get_julian (today) - g_date_get_julian (file_date);

	g_date_free (today);
	g_date_free (file_date);

	if (file_date_age == 0)	{
	/* Translators:  Below are the strings displayed in the 'Date Modified'
	   column of the list view.  The format of this string can vary depending
	   on age of a file.  Please modify the format of the timestamp to match
	   your locale.  For example, to display 24 hour time replace the '%-I'
	   with '%-H' and remove the '%p'.  (See bugzilla report #120434.) */
		format = g_strdup(_("today at %-I:%M %p"));
	} else if (file_date_age == 1) {
		format = g_strdup(_("yesterday at %-I:%M %p"));
	} else if (file_date_age < 7) {
		format = g_strdup(_("%A, %B %-d %Y at %-I:%M:%S %p"));
	} else {
		format = g_strdup(_("%A, %B %-d %Y at %-I:%M:%S %p"));
	}

	readable_date = gsearchtool_strdup_strftime (format, file_time);
	g_free (format);

	return readable_date;
}

gchar *
gsearchtool_strdup_strftime (const gchar * format,
                             struct tm * time_pieces)
{
	/* This function is borrowed from eel's eel_strdup_strftime() */
	GString * string;
	const char * remainder, * percent;
	char code[4], buffer[512];
	char * piece, * result, * converted;
	size_t string_length;
	gboolean strip_leading_zeros, turn_leading_zeros_to_spaces;
	char modifier;
	int i;

	/* Format could be translated, and contain UTF-8 chars,
	 * so convert to locale encoding which strftime uses */
	converted = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
	g_return_val_if_fail (converted != NULL, NULL);

	string = g_string_new ("");
	remainder = converted;

	/* Walk from % character to % character. */
	for (;;) {
		percent = strchr (remainder, '%');
		if (percent == NULL) {
			g_string_append (string, remainder);
			break;
		}
		g_string_append_len (string, remainder,
				     percent - remainder);

		/* Handle the "%" character. */
		remainder = percent + 1;
		switch (*remainder) {
		case '-':
			strip_leading_zeros = TRUE;
			turn_leading_zeros_to_spaces = FALSE;
			remainder++;
			break;
		case '_':
			strip_leading_zeros = FALSE;
			turn_leading_zeros_to_spaces = TRUE;
			remainder++;
			break;
		case '%':
			g_string_append_c (string, '%');
			remainder++;
			continue;
		case '\0':
			g_warning ("Trailing %% passed to gsearchtool_strdup_strftime");
			g_string_append_c (string, '%');
			continue;
		default:
			strip_leading_zeros = FALSE;
			turn_leading_zeros_to_spaces = FALSE;
			break;
		}

		modifier = 0;
		if (strchr (SUS_EXTENDED_STRFTIME_MODIFIERS, *remainder) != NULL) {
			modifier = *remainder;
			remainder++;

			if (*remainder == 0) {
				g_warning ("Unfinished %%%c modifier passed to gsearchtool_strdup_strftime", modifier);
				break;
			}
		}

		if (strchr (C_STANDARD_STRFTIME_CHARACTERS, *remainder) == NULL) {
			g_warning ("gsearchtool_strdup_strftime does not support "
				   "non-standard escape code %%%c",
				   *remainder);
		}

		/* Convert code to strftime format. We have a fixed
		 * limit here that each code can expand to a maximum
		 * of 512 bytes, which is probably OK. There's no
		 * limit on the total size of the result string.
		 */
		i = 0;
		code[i++] = '%';
		if (modifier != 0) {
#ifdef HAVE_STRFTIME_EXTENSION
			code[i++] = modifier;
#endif
		}
		code[i++] = *remainder;
		code[i++] = '\0';
		string_length = strftime (buffer, sizeof (buffer),
					  code, time_pieces);
		if (string_length == 0) {
			/* We could put a warning here, but there's no
			 * way to tell a successful conversion to
			 * empty string from a failure.
			 */
			buffer[0] = '\0';
		}

		/* Strip leading zeros if requested. */
		piece = buffer;
		if (strip_leading_zeros || turn_leading_zeros_to_spaces) {
			if (strchr (C_STANDARD_NUMERIC_STRFTIME_CHARACTERS, *remainder) == NULL) {
				g_warning ("gsearchtool_strdup_strftime does not support "
					   "modifier for non-numeric escape code %%%c%c",
					   remainder[-1],
					   *remainder);
			}
			if (*piece == '0') {
				do {
					piece++;
				} while (*piece == '0');
				if (!g_ascii_isdigit (*piece)) {
				    piece--;
				}
			}
			if (turn_leading_zeros_to_spaces) {
				memset (buffer, ' ', piece - buffer);
				piece = buffer;
			}
		}
		remainder++;

		/* Add this piece. */
		g_string_append (string, piece);
	}

	/* Convert the string back into utf-8. */
	result = g_locale_to_utf8 (string->str, -1, NULL, NULL, NULL);

	g_string_free (string, TRUE);
	g_free (converted);

	return result;
}

gchar *
get_file_type_description (const gchar * file,
                           GFileInfo * file_info)
{
	const char * content_type = NULL;
	gchar * desc;

	if (file != NULL) {
		content_type = g_file_info_get_content_type (file_info);
	}

	if (content_type == NULL || g_content_type_is_unknown (content_type) == TRUE) {
		return g_strdup (g_content_type_get_description ("application/octet-stream"));
	}	

	desc = g_strdup (g_content_type_get_description (content_type));

	if (g_file_info_get_is_symlink (file_info) == TRUE) {

		const gchar * symlink_target;
		gchar * absolute_symlink = NULL;
		gchar * str = NULL;

		symlink_target = g_file_info_get_symlink_target (file_info);

		if (g_path_is_absolute (symlink_target) != TRUE) {
			gchar *dirname;

			dirname = g_path_get_dirname (file);
			absolute_symlink = g_strconcat (dirname, G_DIR_SEPARATOR_S, symlink_target, NULL);
			g_free (dirname);
		}
		else {
			absolute_symlink = g_strdup (symlink_target);
		}

		if (g_file_test (absolute_symlink, G_FILE_TEST_EXISTS) != TRUE) {
                       if ((g_ascii_strcasecmp (content_type, "x-special/socket") != 0) &&
                           (g_ascii_strcasecmp (content_type, "x-special/fifo") != 0)) {
				g_free (absolute_symlink);
				g_free (desc);
				return g_strdup (_("link (broken)"));
			}
		}

		str = g_strdup_printf (_("link to %s"), (desc != NULL) ? desc : content_type);
		g_free (absolute_symlink);
		g_free (desc);
		return str;
	}
	return desc;
}

static gchar *
gsearchtool_pixmap_file (const gchar * partial_path)
{
	gchar * path;

	path = g_build_filename (DATADIR "/pixmaps/gsearchtool", partial_path, NULL);
	if (g_file_test (path, G_FILE_TEST_EXISTS)) {
		return path;
	}
	g_free (path);
	return NULL;
}

static GdkPixbuf *
gsearchtool_load_thumbnail_frame (void)
{
	GdkPixbuf * pixbuf = NULL;
	gchar * image_path;

	image_path = gsearchtool_pixmap_file ("thumbnail_frame.png");

	if (image_path != NULL) {
		pixbuf = gdk_pixbuf_new_from_file (image_path, NULL);
	}
	g_free (image_path);
	return pixbuf;
}

static void
gsearchtool_draw_frame_row (GdkPixbuf * frame_image,
                            gint target_width,
                            gint source_width,
                            gint source_v_position,
                            gint dest_v_position,
                            GdkPixbuf * result_pixbuf,
                            gint left_offset,
                            gint height)
{
	gint remaining_width;
	gint h_offset;
	gint slab_width;

	remaining_width = target_width;
	h_offset = 0;
	while (remaining_width > 0) {
		slab_width = remaining_width > source_width ? source_width : remaining_width;
		gdk_pixbuf_copy_area (frame_image, left_offset, source_v_position, slab_width,
		                      height, result_pixbuf, left_offset + h_offset, dest_v_position);
		remaining_width -= slab_width;
		h_offset += slab_width;
	}
}

static void
gsearchtool_draw_frame_column (GdkPixbuf * frame_image,
                               gint target_height,
                               gint source_height,
                               gint source_h_position,
                               gint dest_h_position,
                               GdkPixbuf * result_pixbuf,
                               gint top_offset,
                               gint width)
{
	gint remaining_height;
	gint v_offset;
	gint slab_height;

	remaining_height = target_height;
	v_offset = 0;
	while (remaining_height > 0) {
		slab_height = remaining_height > source_height ? source_height : remaining_height;
		gdk_pixbuf_copy_area (frame_image, source_h_position, top_offset, width, slab_height,
		                      result_pixbuf, dest_h_position, top_offset + v_offset);
		remaining_height -= slab_height;
		v_offset += slab_height;
	}
}

static GdkPixbuf *
gsearchtool_stretch_frame_image (GdkPixbuf *frame_image,
                                 gint left_offset,
                                 gint top_offset,
                                 gint right_offset,
                                 gint bottom_offset,
                                 gint dest_width,
                                 gint dest_height,
                                 gboolean fill_flag)
{
	GdkPixbuf * result_pixbuf;
	gint frame_width, frame_height;
	gint target_width, target_frame_width;
	gint target_height, target_frame_height;

	frame_width = gdk_pixbuf_get_width (frame_image);
	frame_height = gdk_pixbuf_get_height (frame_image);

	if (fill_flag) {
		result_pixbuf = gdk_pixbuf_scale_simple (frame_image, dest_width, dest_height, GDK_INTERP_NEAREST);
	} else {
		result_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, dest_width, dest_height);
	}

	/* clear the new pixbuf */
	if (fill_flag == FALSE) {
		gdk_pixbuf_fill (result_pixbuf, 0xffffffff);
	}

	target_width  = dest_width - left_offset - right_offset;
	target_frame_width = frame_width - left_offset - right_offset;

	target_height  = dest_height - top_offset - bottom_offset;
	target_frame_height = frame_height - top_offset - bottom_offset;

	/* Draw the left top corner  and top row */
	gdk_pixbuf_copy_area (frame_image, 0, 0, left_offset, top_offset, result_pixbuf, 0,  0);
	gsearchtool_draw_frame_row (frame_image, target_width, target_frame_width, 0, 0,
	                            result_pixbuf, left_offset, top_offset);

	/* Draw the right top corner and left column */
	gdk_pixbuf_copy_area (frame_image, frame_width - right_offset, 0, right_offset, top_offset,
	                      result_pixbuf, dest_width - right_offset,  0);
	gsearchtool_draw_frame_column (frame_image, target_height, target_frame_height, 0, 0,
	                               result_pixbuf, top_offset, left_offset);

	/* Draw the bottom right corner and bottom row */
	gdk_pixbuf_copy_area (frame_image, frame_width - right_offset, frame_height - bottom_offset,
	                      right_offset, bottom_offset, result_pixbuf, dest_width - right_offset,
			      dest_height - bottom_offset);
	gsearchtool_draw_frame_row (frame_image, target_width, target_frame_width,
	                            frame_height - bottom_offset, dest_height - bottom_offset,
				    result_pixbuf, left_offset, bottom_offset);

	/* Draw the bottom left corner and the right column */
	gdk_pixbuf_copy_area (frame_image, 0, frame_height - bottom_offset, left_offset, bottom_offset,
	                      result_pixbuf, 0,  dest_height - bottom_offset);
	gsearchtool_draw_frame_column (frame_image, target_height, target_frame_height,
	                               frame_width - right_offset, dest_width - right_offset,
				       result_pixbuf, top_offset, right_offset);
	return result_pixbuf;
}

static GdkPixbuf *
gsearchtool_embed_image_in_frame (GdkPixbuf * source_image,
                                  GdkPixbuf * frame_image,
                                  gint left_offset,
                                  gint top_offset,
                                  gint right_offset,
                                  gint bottom_offset)
{
	GdkPixbuf * result_pixbuf;
	gint source_width, source_height;
	gint dest_width, dest_height;

	source_width = gdk_pixbuf_get_width (source_image);
	source_height = gdk_pixbuf_get_height (source_image);

	dest_width = source_width + left_offset + right_offset;
	dest_height = source_height + top_offset + bottom_offset;

	result_pixbuf = gsearchtool_stretch_frame_image (frame_image, left_offset, top_offset, right_offset, bottom_offset,
						         dest_width, dest_height, FALSE);

	gdk_pixbuf_copy_area (source_image, 0, 0, source_width, source_height, result_pixbuf, left_offset, top_offset);

	return result_pixbuf;
}

static void
gsearchtool_thumbnail_frame_image (GdkPixbuf ** pixbuf)
{
	GdkPixbuf * pixbuf_with_frame;
	GdkPixbuf * frame;

	frame = gsearchtool_load_thumbnail_frame ();
	if (frame == NULL) {
		return;
	}

	pixbuf_with_frame = gsearchtool_embed_image_in_frame (*pixbuf, frame, 3, 3, 6, 6);
	g_object_unref (*pixbuf);
	g_object_unref (frame);

	*pixbuf = pixbuf_with_frame;
}

static GdkPixbuf *
gsearchtool_get_thumbnail_image (const gchar * thumbnail)
{
	GdkPixbuf * pixbuf = NULL;

	if (thumbnail != NULL) {
		if (g_file_test (thumbnail, G_FILE_TEST_EXISTS)) {

			GdkPixbuf * thumbnail_pixbuf = NULL;
			gfloat scale_factor_x = 1.0;
			gfloat scale_factor_y = 1.0;
			gint scale_x;
			gint scale_y;

			thumbnail_pixbuf = gdk_pixbuf_new_from_file (thumbnail, NULL);
			gsearchtool_thumbnail_frame_image (&thumbnail_pixbuf);

			if (gdk_pixbuf_get_width (thumbnail_pixbuf) > ICON_SIZE) {
				scale_factor_x = (gfloat) ICON_SIZE / (gfloat) gdk_pixbuf_get_width (thumbnail_pixbuf);
			}
			if (gdk_pixbuf_get_height (thumbnail_pixbuf) > ICON_SIZE) {
				scale_factor_y = (gfloat) ICON_SIZE / (gfloat) gdk_pixbuf_get_height (thumbnail_pixbuf);
			}

			if (gdk_pixbuf_get_width (thumbnail_pixbuf) > gdk_pixbuf_get_height (thumbnail_pixbuf)) {
				scale_x = ICON_SIZE;
				scale_y = (gint) (gdk_pixbuf_get_height (thumbnail_pixbuf) * scale_factor_x);
			}
			else {
				scale_x = (gint) (gdk_pixbuf_get_width (thumbnail_pixbuf) * scale_factor_y);
				scale_y = ICON_SIZE;
			}

			pixbuf = gdk_pixbuf_scale_simple (thumbnail_pixbuf, scale_x, scale_y, GDK_INTERP_BILINEAR);
			g_object_unref (thumbnail_pixbuf);
		}
	}
	return pixbuf;
}

static GdkPixbuf *
get_themed_icon_pixbuf (GThemedIcon * icon,
                        int size,
                        GtkIconTheme * icon_theme)
{
	char ** icon_names;
	GtkIconInfo * icon_info;
	GdkPixbuf * pixbuf;
	GError * error = NULL;

	g_object_get (icon, "names", &icon_names, NULL);

	icon_info = gtk_icon_theme_choose_icon (icon_theme, (const char **)icon_names, size, 0);
	if (icon_info == NULL) {
		icon_info = gtk_icon_theme_lookup_icon (icon_theme, "text-x-generic", size, GTK_ICON_LOOKUP_USE_BUILTIN);
	}
	pixbuf = gtk_icon_info_load_icon (icon_info, &error);
	if (pixbuf == NULL) {
		g_warning ("Could not load icon pixbuf: %s\n", error->message);
		g_clear_error (&error);
	}

	gtk_icon_info_free (icon_info);
	g_strfreev (icon_names);

	return pixbuf;
}



GdkPixbuf *
get_file_pixbuf (GSearchWindow * gsearch,
                 GFileInfo * file_info)
{
	GdkPixbuf * pixbuf;
	GIcon * icon = NULL;
	const gchar * thumbnail_path = NULL;

	if (file_info == NULL) {
		return NULL;
	}

	icon = g_file_info_get_icon (file_info);

	if (gsearch->show_thumbnails == TRUE) {
		thumbnail_path = g_file_info_get_attribute_byte_string (file_info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH);
	}

	if (thumbnail_path != NULL) {
		pixbuf = gsearchtool_get_thumbnail_image (thumbnail_path);
	}
	else {
		gchar * icon_string;

		icon_string = g_icon_to_string (icon);		
		pixbuf = (GdkPixbuf *) g_hash_table_lookup (gsearch->search_results_filename_hash_table, icon_string);

		if (pixbuf == NULL) {
			pixbuf = get_themed_icon_pixbuf (G_THEMED_ICON (icon), ICON_SIZE, gtk_icon_theme_get_default ());
			g_hash_table_insert (gsearch->search_results_filename_hash_table, g_strdup (icon_string), pixbuf);
		}
		g_free (icon_string);
	}
	return pixbuf;
}

gboolean
open_file_with_filemanager (GtkWidget * window,
                            const gchar * file)
{
	GDesktopAppInfo * d_app_info;
	GKeyFile * key_file;
	GdkAppLaunchContext * ctx = NULL;
	GList * list = NULL;
	GAppInfo * g_app_info;
	GFile * g_file;
	gchar * command;
	gchar * contents;
	gchar * uri;
	gboolean result = TRUE;

	uri = g_filename_to_uri (file, NULL, NULL);
	list = g_list_prepend (list, uri);

	g_file = g_file_new_for_path (file);
	g_app_info = g_file_query_default_handler (g_file, NULL, NULL);

	if (strcmp (g_app_info_get_executable (g_app_info), "caja") == 0) {
		command = g_strconcat ("caja ",
		                       "--sm-disable ",
		                       "--no-desktop ",
		                       "--no-default-window ",
		                       NULL);
	}
	else {
		command = g_strconcat (g_app_info_get_executable (g_app_info),
		                       " ", NULL);
	}

	contents = g_strdup_printf ("[Desktop Entry]\n"
				    "Name=Caja\n"
				    "Icon=file-manager\n"
				    "Exec=%s\n"
				    "Terminal=false\n"
				    "StartupNotify=true\n"
				    "Type=Application\n",
				    command);
	key_file = g_key_file_new ();
	g_key_file_load_from_data (key_file, contents, strlen(contents), G_KEY_FILE_NONE, NULL);
	d_app_info = g_desktop_app_info_new_from_keyfile (key_file);
	
	if (d_app_info != NULL) {
		ctx = gdk_app_launch_context_new ();
		gdk_app_launch_context_set_screen (ctx, gtk_widget_get_screen (window));

		result = g_app_info_launch_uris (G_APP_INFO (d_app_info), list,  G_APP_LAUNCH_CONTEXT (ctx), NULL);
	}
	else {
		result = FALSE;
	}

	g_object_unref (g_app_info);
	g_object_unref (d_app_info);
	g_object_unref (g_file);
	g_object_unref (ctx);
	g_key_file_free (key_file);
	g_list_free (list);
	g_free (contents);
	g_free (command);
	g_free (uri);

	return result;
}

gboolean
open_file_with_application (GtkWidget * window,
                            const gchar * file,
                            GAppInfo * app)
{
	GdkAppLaunchContext * context;
	GdkScreen * screen;
	gboolean result;

	if (g_file_test (file, G_FILE_TEST_IS_DIR) == TRUE) {
		return FALSE;
	}

	context = gdk_app_launch_context_new ();
	screen = gtk_widget_get_screen (window);
	gdk_app_launch_context_set_screen (context, screen);

	if (app == NULL) {
		gchar * uri;

		uri = g_filename_to_uri (file, NULL, NULL);
		result = g_app_info_launch_default_for_uri (uri, (GAppLaunchContext *) context, NULL);
		g_free (uri);
	}
	else {
		GList * g_file_list = NULL;
		GFile * g_file = NULL;

		g_file = g_file_new_for_path (file);

		if (g_file == NULL) {
			result = FALSE;
		}
		else {
			g_file_list = g_list_prepend (g_file_list, g_file);

			result = g_app_info_launch (app, g_file_list, (GAppLaunchContext *) context, NULL);
			g_list_free (g_file_list);
			g_object_unref (g_file);
		}
	}
	return result;
}

gboolean
launch_file (const gchar * file)
{
	const char * content_type = g_content_type_guess (file, NULL, 0, NULL);
	gboolean result = FALSE;

	if ((g_file_test (file, G_FILE_TEST_IS_EXECUTABLE)) &&
	    (g_ascii_strcasecmp (content_type, BINARY_EXEC_MIME_TYPE) == 0)) {
		result = g_spawn_command_line_async (file, NULL);
	}

	return result;
}

gchar *
gsearchtool_get_unique_filename (const gchar * path,
                                 const gchar * suffix)
{
	const gint num_of_words = 12;
	gchar     * words[] = {
		    "foo",
		    "bar",
		    "blah",
	   	    "cranston",
		    "frobate",
		    "hadjaha",
		    "greasy",
		    "hammer",
		    "eek",
		    "larry",
		    "curly",
		    "moe",
		    NULL};
	gchar * retval = NULL;
	gboolean exists = TRUE;

	while (exists) {
		gchar * file;
		gint rnd;
		gint word;

		rnd = rand ();
		word = rand () % num_of_words;

		file = g_strdup_printf ("%s-%010x%s",
					    words [word],
					    (guint) rnd,
					    suffix);

		g_free (retval);
		retval = g_strconcat (path, G_DIR_SEPARATOR_S, file, NULL);
		exists = g_file_test (retval, G_FILE_TEST_EXISTS);
		g_free (file);
	}
	return retval;
}

GtkWidget *
gsearchtool_button_new_with_stock_icon (const gchar * string,
                                        const gchar * stock_id)
{
	GtkWidget * align;
	GtkWidget * button;
	GtkWidget * hbox;
	GtkWidget * image;
	GtkWidget * label;

	button = gtk_button_new ();
	label = gtk_label_new_with_mnemonic (string);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
	hbox = gtk_hbox_new (FALSE, 2);
	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), align);
	gtk_container_add (GTK_CONTAINER (align), hbox);
	gtk_widget_show_all (align);

	return button;
}

GSList *
gsearchtool_get_columns_order (GtkTreeView * treeview)
{
	GSList *order = NULL;
	GList * columns;
	GList * col;

	columns = gtk_tree_view_get_columns (treeview);

	for (col = columns; col; col = col->next) {
		gint id;

		id = gtk_tree_view_column_get_sort_column_id (col->data);
		order = g_slist_prepend (order, GINT_TO_POINTER (id));
	}
	g_list_free (columns);

	order = g_slist_reverse (order);
	return order;
}

GtkTreeViewColumn *
gsearchtool_gtk_tree_view_get_column_with_sort_column_id (GtkTreeView * treeview,
                                                          gint id)
{
	GtkTreeViewColumn * col = NULL;
	GList * columns;
	GList * it;

	columns = gtk_tree_view_get_columns (treeview);

	for (it = columns; it; it = it->next) {
		if (gtk_tree_view_column_get_sort_column_id (it->data) == id) {
			col = it->data;
			break;
		}
	}
	g_list_free (columns);
	return col;
}

void
gsearchtool_set_columns_order (GtkTreeView * treeview)
{
	GtkTreeViewColumn * last = NULL;
	GSList * order;
	GSList * it;

	order = gsearchtool_mateconf_get_list ("/apps/mate-search-tool/columns_order", MATECONF_VALUE_INT);

	for (it = order; it; it = it->next) {

		GtkTreeViewColumn * cur;
		gint id;

		id = GPOINTER_TO_INT (it->data);

		if (id >= 0 && id < NUM_COLUMNS) {

			cur = gsearchtool_gtk_tree_view_get_column_with_sort_column_id (treeview, id);

			if (cur && cur != last) {
				gtk_tree_view_move_column_after (treeview, cur, last);
				last = cur;
			}
		}
	}
	g_slist_free (order);
}

void
gsearchtool_get_stored_window_geometry (gint * width,
                                        gint * height)
{
	gint saved_width;
	gint saved_height;

	if (width == NULL || height == NULL) {
		return;
	}

	saved_width = gsearchtool_mateconf_get_int ("/apps/mate-search-tool/default_window_width");
	saved_height = gsearchtool_mateconf_get_int ("/apps/mate-search-tool/default_window_height");

	if (saved_width == -1) {
		saved_width = DEFAULT_WINDOW_WIDTH;
	}

	if (saved_height == -1) {
		saved_height = DEFAULT_WINDOW_HEIGHT;
	}

	*width = MAX (saved_width, MINIMUM_WINDOW_WIDTH);
	*height = MAX (saved_height, MINIMUM_WINDOW_HEIGHT);
}

/* START OF CAJA/EEL FUNCTIONS: USED FOR HANDLING OF DUPLICATE FILENAMES */

/* Localizers: 
 * Feel free to leave out the st, nd, rd and th suffix or
 * make some or all of them match.
 */

/* localizers: tag used to detect the first copy of a file */
static const char untranslated_copy_duplicate_tag[] = N_(" (copy)");
/* localizers: tag used to detect the second copy of a file */
static const char untranslated_another_copy_duplicate_tag[] = N_(" (another copy)");

/* localizers: tag used to detect the x11th copy of a file */
static const char untranslated_x11th_copy_duplicate_tag[] = N_("th copy)");
/* localizers: tag used to detect the x12th copy of a file */
static const char untranslated_x12th_copy_duplicate_tag[] = N_("th copy)");
/* localizers: tag used to detect the x13th copy of a file */
static const char untranslated_x13th_copy_duplicate_tag[] = N_("th copy)");

/* localizers: tag used to detect the x1st copy of a file */
static const char untranslated_st_copy_duplicate_tag[] = N_("st copy)");
/* localizers: tag used to detect the x2nd copy of a file */
static const char untranslated_nd_copy_duplicate_tag[] = N_("nd copy)");
/* localizers: tag used to detect the x3rd copy of a file */
static const char untranslated_rd_copy_duplicate_tag[] = N_("rd copy)");

/* localizers: tag used to detect the xxth copy of a file */
static const char untranslated_th_copy_duplicate_tag[] = N_("th copy)");

#define COPY_DUPLICATE_TAG _(untranslated_copy_duplicate_tag)
#define ANOTHER_COPY_DUPLICATE_TAG _(untranslated_another_copy_duplicate_tag)
#define X11TH_COPY_DUPLICATE_TAG _(untranslated_x11th_copy_duplicate_tag)
#define X12TH_COPY_DUPLICATE_TAG _(untranslated_x12th_copy_duplicate_tag)
#define X13TH_COPY_DUPLICATE_TAG _(untranslated_x13th_copy_duplicate_tag)

#define ST_COPY_DUPLICATE_TAG _(untranslated_st_copy_duplicate_tag)
#define ND_COPY_DUPLICATE_TAG _(untranslated_nd_copy_duplicate_tag)
#define RD_COPY_DUPLICATE_TAG _(untranslated_rd_copy_duplicate_tag)
#define TH_COPY_DUPLICATE_TAG _(untranslated_th_copy_duplicate_tag)

/* localizers: appended to first file copy */
static const char untranslated_first_copy_duplicate_format[] = N_("%s (copy)%s");
/* localizers: appended to second file copy */
static const char untranslated_second_copy_duplicate_format[] = N_("%s (another copy)%s");

/* localizers: appended to x11th file copy */
static const char untranslated_x11th_copy_duplicate_format[] = N_("%s (%dth copy)%s");
/* localizers: appended to x12th file copy */
static const char untranslated_x12th_copy_duplicate_format[] = N_("%s (%dth copy)%s");
/* localizers: appended to x13th file copy */
static const char untranslated_x13th_copy_duplicate_format[] = N_("%s (%dth copy)%s");

/* localizers: appended to x1st file copy */
static const char untranslated_st_copy_duplicate_format[] = N_("%s (%dst copy)%s");
/* localizers: appended to x2nd file copy */
static const char untranslated_nd_copy_duplicate_format[] = N_("%s (%dnd copy)%s");
/* localizers: appended to x3rd file copy */
static const char untranslated_rd_copy_duplicate_format[] = N_("%s (%drd copy)%s");
/* localizers: appended to xxth file copy */
static const char untranslated_th_copy_duplicate_format[] = N_("%s (%dth copy)%s");

#define FIRST_COPY_DUPLICATE_FORMAT _(untranslated_first_copy_duplicate_format)
#define SECOND_COPY_DUPLICATE_FORMAT _(untranslated_second_copy_duplicate_format)
#define X11TH_COPY_DUPLICATE_FORMAT _(untranslated_x11th_copy_duplicate_format)
#define X12TH_COPY_DUPLICATE_FORMAT _(untranslated_x12th_copy_duplicate_format)
#define X13TH_COPY_DUPLICATE_FORMAT _(untranslated_x13th_copy_duplicate_format)

#define ST_COPY_DUPLICATE_FORMAT _(untranslated_st_copy_duplicate_format)
#define ND_COPY_DUPLICATE_FORMAT _(untranslated_nd_copy_duplicate_format)
#define RD_COPY_DUPLICATE_FORMAT _(untranslated_rd_copy_duplicate_format)
#define TH_COPY_DUPLICATE_FORMAT _(untranslated_th_copy_duplicate_format)

static gchar *
make_valid_utf8 (const gchar * name)
{
	GString *string;
	const char *remainder, *invalid;
	int remaining_bytes, valid_bytes;

	string = NULL;
	remainder = name;
	remaining_bytes = strlen (name);

	while (remaining_bytes != 0) {
		if (g_utf8_validate (remainder, remaining_bytes, &invalid)) {
			break;
		}
		valid_bytes = invalid - remainder;

		if (string == NULL) {
			string = g_string_sized_new (remaining_bytes);
		}
		g_string_append_len (string, remainder, valid_bytes);
		g_string_append_c (string, '?');

		remaining_bytes -= valid_bytes + 1;
		remainder = invalid + 1;
	}

	if (string == NULL) {
		return g_strdup (name);
	}

	g_string_append (string, remainder);
	g_string_append (string, _(" (invalid Unicode)"));
	g_assert (g_utf8_validate (string->str, -1, NULL));

	return g_string_free (string, FALSE);
}

static gchar *
extract_string_until (const gchar * original, 
                      const gchar * until_substring)
{
	gchar * result;
	
	g_assert ((gint) strlen (original) >= until_substring - original);
	g_assert (until_substring - original >= 0);

	result = g_malloc (until_substring - original + 1);
	strncpy (result, original, until_substring - original);
	result[until_substring - original] = '\0';
	
	return result;
}

/* Dismantle a file name, separating the base name, the file suffix and removing any
 * (xxxcopy), etc. string. Figure out the count that corresponds to the given
 * (xxxcopy) substring.
 */
static void
parse_previous_duplicate_name (const gchar * name,
                               gchar ** name_base,
                               const gchar ** suffix,
                               gint * count)
{
	const gchar * tag;

	g_assert (name[0] != '\0');
	
	*suffix = strchr (name + 1, '.');
	if (*suffix == NULL || (*suffix)[1] == '\0') {
		/* no suffix */
		*suffix = "";
	}

	tag = strstr (name, COPY_DUPLICATE_TAG);
	if (tag != NULL) {
		if (tag > *suffix) {
			/* handle case "foo. (copy)" */
			*suffix = "";
		}
		*name_base = extract_string_until (name, tag);
		*count = 1;
		return;
	}

	tag = strstr (name, ANOTHER_COPY_DUPLICATE_TAG);
	if (tag != NULL) {
		if (tag > *suffix) {
			/* handle case "foo. (another copy)" */
			*suffix = "";
		}
		*name_base = extract_string_until (name, tag);
		*count = 2;
		return;
	}

	/* Check to see if we got one of st, nd, rd, th. */
	tag = strstr (name, X11TH_COPY_DUPLICATE_TAG);

	if (tag == NULL) {
		tag = strstr (name, X12TH_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, X13TH_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, ST_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, ND_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, RD_COPY_DUPLICATE_TAG);
	}
	if (tag == NULL) {
		tag = strstr (name, TH_COPY_DUPLICATE_TAG);
	}

	/* If we got one of st, nd, rd, th, fish out the duplicate number. */
	if (tag != NULL) {
		/* localizers: opening parentheses to match the "th copy)" string */
		tag = strstr (name, _(" ("));
		if (tag != NULL) {
			if (tag > *suffix) {
				/* handle case "foo. (22nd copy)" */
				*suffix = "";
			}
			*name_base = extract_string_until (name, tag);
			/* localizers: opening parentheses of the "th copy)" string */
			if (sscanf (tag, _(" (%d"), count) == 1) {
				if (*count < 1 || *count > 1000000) {
					/* keep the count within a reasonable range */
					*count = 0;
				}
				return;
			}
			*count = 0;
			return;
		}
	}

	*count = 0;
	if (**suffix != '\0') {
		*name_base = extract_string_until (name, *suffix);
	} else {
		*name_base = g_strdup (name);
	}
}

static gchar *
make_next_duplicate_name (const gchar *base, 
                          const gchar *suffix,
                          gint count)
{
	const gchar * format;
	gchar * result;

	if (count < 1) {
		g_warning ("bad count %d in make_next_duplicate_name()", count);
		count = 1;
	}

	if (count <= 2) {

		/* Handle special cases for low numbers.
		 * Perhaps for some locales we will need to add more.
		 */
		switch (count) {
		default:
			g_assert_not_reached ();
			/* fall through */
		case 1:
			format = FIRST_COPY_DUPLICATE_FORMAT;
			break;
		case 2:
			format = SECOND_COPY_DUPLICATE_FORMAT;
			break;

		}
		result = g_strdup_printf (format, base, suffix);
	} else {

		/* Handle special cases for the first few numbers of each ten.
		 * For locales where getting this exactly right is difficult,
		 * these can just be made all the same as the general case below.
		 */

		/* Handle special cases for x11th - x20th.
		 */
		switch (count % 100) {
		case 11:
			format = X11TH_COPY_DUPLICATE_FORMAT;
			break;
		case 12:
			format = X12TH_COPY_DUPLICATE_FORMAT;
			break;
		case 13:
			format = X13TH_COPY_DUPLICATE_FORMAT;
			break;
		default:
			format = NULL;
			break;
		}

		if (format == NULL) {
			switch (count % 10) {
			case 1:
				format = ST_COPY_DUPLICATE_FORMAT;
				break;
			case 2:
				format = ND_COPY_DUPLICATE_FORMAT;
				break;
			case 3:
				format = RD_COPY_DUPLICATE_FORMAT;
				break;
			default:
				/* The general case. */
				format = TH_COPY_DUPLICATE_FORMAT;
				break;
			}
		}
		result = g_strdup_printf (format, base, count, suffix);
	}
	return result;
}

static gchar *
get_duplicate_name (const gchar *name)
{
	const gchar * suffix;
	gchar * name_base;
	gchar * result;
	gint count;

	parse_previous_duplicate_name (name, &name_base, &suffix, &count);
	result = make_next_duplicate_name (name_base, suffix, count + 1);
	g_free (name_base);

	return result;
}

gchar *
gsearchtool_get_next_duplicate_name (const gchar * basename)
{
	gchar * utf8_name;
	gchar * utf8_result;
	gchar * result;

	utf8_name = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
	
	if (utf8_name == NULL) {
		/* Couldn't convert to utf8 - probably
		 * G_BROKEN_FILENAMES not set when it should be.
		 * Try converting from the locale */
		utf8_name = g_locale_to_utf8 (basename, -1, NULL, NULL, NULL);	

		if (utf8_name == NULL) {
			utf8_name = make_valid_utf8 (basename);
		}
	}

	utf8_result = get_duplicate_name (utf8_name);
	g_free (utf8_name);

	result = g_filename_from_utf8 (utf8_result, -1, NULL, NULL, NULL);
	g_free (utf8_result);
	return result;
}
