/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-help.c
 * Copyright (C) 2001 Sid Vicious
 * Copyright (C) 2001 Jonathan Blandford <jrb@alum.mit.edu>
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
 */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include "mate-url.h"
#include "mate-help.h"

static char *
locate_help_file (const char *path, const char *doc_name)
{
	int i, j;
	char *exts[] = { "", ".xml", ".docbook", ".sgml", ".html", NULL };
	const char * const * lang_list = g_get_language_names ();

	for (j = 0;lang_list[j] != NULL; j++) {
		const char *lang = lang_list[j];

		/* This has to be a valid language AND a language with
		 * no encoding postfix.  The language will come up without
		 * encoding next */
		if (lang == NULL ||
		    strchr (lang, '.') != NULL)
			continue;

		for (i = 0; exts[i] != NULL; i++) {
			char *name;
			char *full;

			name = g_strconcat (doc_name, exts[i], NULL);
			full = g_build_filename (path, lang, name, NULL);
			g_free (name);

			if (g_file_test (full, G_FILE_TEST_EXISTS))
				return full;

			g_free (full);
		}
	}

	return NULL;
}

/**
 * mate_help_display:
 * @file_name: The name of the help document to display.
 * @link_id: Can be %NULL. If set, refers to an anchor or section id within the
 * requested document.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Displays the help file specified by @file_name at location @link_id in the
 * preferred help browser of the user.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 **/
gboolean
mate_help_display (const char    *file_name,
		    const char    *link_id,
		    GError       **error)
{
	return mate_help_display_with_doc_id (NULL, NULL, file_name, link_id, error);
}

/**
 * mate_help_display_with_doc_id_and_env:
 * @program: The current application object, or %NULL for the default one.
 * @doc_id: The document identifier, or %NULL to default to empty string.
 * (app_id) of the specified @program.
 * @file_name: The name of the help document to display.
 * @link_id: Can be %NULL. If set, refers to an anchor or section id within the
 * requested document.
 * @envp: child's environment, or %NULL to inherit parent's.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Like mate_display_with_doc_id(), but the the contents of @envp
 * will become the url viewer's environment rather than inheriting
 * from the parents environment.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 *
 * Since: 2.2
 **/
gboolean
mate_help_display_with_doc_id_and_env (MateProgram  *program,
					const char    *doc_id,
					const char    *file_name,
					const char    *link_id,
					char         **envp,
					GError       **error)
{
	gchar *local_help_path;
	gchar *global_help_path;
	gchar *file;
	struct stat local_help_st;
	struct stat global_help_st;
	gchar *uri;
	gchar *free_doc_id = NULL;
	gboolean retval;

	g_return_val_if_fail (file_name != NULL, FALSE);

	retval = FALSE;

	local_help_path = NULL;
	global_help_path = NULL;
	file = NULL;
	uri = NULL;

	if (program == NULL)
		program = mate_program_get ();

	g_assert (program != NULL);

	if (doc_id == NULL) {
		g_object_get (program, MATE_PARAM_APP_ID, &doc_id, NULL);
		free_doc_id = (char *)doc_id;
	}

	if (doc_id == NULL)
		doc_id = "";

	/* Compute the local and global help paths */

	local_help_path = mate_program_locate_file (program,
						     MATE_FILE_DOMAIN_APP_HELP,
						     doc_id,
						     FALSE /* only_if_exists */,
						     NULL /* ret_locations */);

	if (local_help_path == NULL) {
		g_set_error (error,
			     MATE_HELP_ERROR,
			     MATE_HELP_ERROR_INTERNAL,
			     _("Unable to find the MATE_FILE_DOMAIN_APP_HELP domain"));
		goto out;
	}

	global_help_path = mate_program_locate_file (program,
						      MATE_FILE_DOMAIN_HELP,
						      doc_id,
						      FALSE /* only_if_exists */,
						      NULL /* ret_locations */);
	if (global_help_path == NULL) {
		g_set_error (error,
			     MATE_HELP_ERROR,
			     MATE_HELP_ERROR_INTERNAL,
			     _("Unable to find the MATE_FILE_DOMAIN_HELP domain."));
		goto out;
	}

	/* Try to access the help paths, first the app-specific help path
	 * and then falling back to the global help path if the first one fails.
	 */

	if (g_stat (local_help_path, &local_help_st) == 0) {
		if (!S_ISDIR (local_help_st.st_mode)) {
			g_set_error (error,
				     MATE_HELP_ERROR,
				     MATE_HELP_ERROR_NOT_FOUND,
				     _("Unable to show help as %s is not a directory.  "
				       "Please check your installation."),
				     local_help_path);
			goto out;
		}

		file = locate_help_file (local_help_path, file_name);
	}

	if (file == NULL) {
		if (g_stat (global_help_path, &global_help_st) == 0) {
			if (!S_ISDIR (global_help_st.st_mode)) {
				g_set_error (error,
					     MATE_HELP_ERROR,
					     MATE_HELP_ERROR_NOT_FOUND,
					     _("Unable to show help as %s is not a directory.  "
					       "Please check your installation."),
					     global_help_path);
				goto out;
			}
		} else {
			g_set_error (error,
				     MATE_HELP_ERROR,
				     MATE_HELP_ERROR_NOT_FOUND,
				     _("Unable to find help paths %s or %s. "
				       "Please check your installation"),
				     local_help_path,
				     global_help_path);
			goto out;
		}

		if (!(local_help_st.st_dev == global_help_st.st_dev
		      && local_help_st.st_ino == global_help_st.st_ino))
			file = locate_help_file (global_help_path, file_name);
	}

	if (file == NULL) {
		g_set_error (error,
			     MATE_HELP_ERROR,
			     MATE_HELP_ERROR_NOT_FOUND,
			     _("Unable to find the help files in either %s "
			       "or %s.  Please check your installation"),
			     local_help_path,
			     global_help_path);
		goto out;
	}

	/* Now that we have a file name, try to display it in the help browser */

	if (link_id)
		uri = g_strconcat ("ghelp://", file, "?", link_id, NULL);
	else
		uri = g_strconcat ("ghelp://", file, NULL);

	retval = mate_help_display_uri_with_env (uri, envp, error);

 out:

	g_free (free_doc_id);
	g_free (local_help_path);
	g_free (global_help_path);
	g_free (file);
	g_free (uri);

	return retval;
}

/**
 * mate_help_display_with_doc_id
 * @program: The current application object, or %NULL for the default one.
 * @doc_id: The document identifier, or %NULL to default to the application ID
 * (app_id) of the specified @program.
 * @file_name: The name of the help document to display.
 * @link_id: Can be %NULL. If set, refers to an anchor or section id within the
 * requested document.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Displays the help file specified by @file_name at location @link_id within
 * the @doc_id domain in the preferred help browser of the user.  Most of the
 * time, you want to call mate_help_display() instead.
 *
 * This function will display the help through creating a "ghelp" URI, by
 * looking for @file_name in the applications installed help location (found by
 * #MATE_FILE_DOMAIN_APP_HELP) and its app_id.  The resulting URI is roughly
 * in the form "ghelp:appid/file_name?link_id".  If a matching file cannot be
 * found, %FALSE is returned and @error is set.
 *
 * Please note that this only displays application help.  To display help files
 * from the global MATE domain, you will want to use
 * mate_help_display_desktop().
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 **/
gboolean
mate_help_display_with_doc_id (MateProgram  *program,
				const char    *doc_id,
				const char    *file_name,
				const char    *link_id,
				GError       **error)
{
	return mate_help_display_with_doc_id_and_env (
			program, doc_id, file_name, link_id, NULL, error);
}

/**
 * mate_help_display_desktop_with_env:
 * @program: The current application object, or %NULL for the default one.
 * @doc_id: The name of the help file relative to the system's help domain
 * (#MATE_FILE_DOMAIN_HELP).
 * @file_name: The name of the help document to display.
 * @link_id: Can be %NULL. If set, refers to an anchor or section id within the
 * requested document.
 * @envp: child's environment, or %NULL to inherit parent's.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Like mate_help_display_desktop(), but the the contents of @envp
 * will become the url viewer's environment rather than inheriting
 * from the parents environment.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 *
 * Since: 2.2
 */
gboolean
mate_help_display_desktop_with_env (MateProgram  *program,
				     const char    *doc_id,
				     const char    *file_name,
				     const char    *link_id,
				     char         **envp,
				     GError       **error)
{
	GSList *ret_locations, *li;
	char *file;
	gboolean retval;
	char *url;

	g_return_val_if_fail (doc_id != NULL, FALSE);
	g_return_val_if_fail (file_name != NULL, FALSE);

	if (program == NULL)
		program = mate_program_get ();

	ret_locations = NULL;
	mate_program_locate_file (program,
				   MATE_FILE_DOMAIN_HELP,
				   doc_id,
				   FALSE /* only_if_exists */,
				   &ret_locations);

	if (ret_locations == NULL) {
		g_set_error (error,
			     MATE_HELP_ERROR,
			     MATE_HELP_ERROR_NOT_FOUND,
			     _("Unable to find doc_id %s in the help path"),
			     doc_id);
		return FALSE;
	}

	file = NULL;
	for (li = ret_locations; li != NULL; li = li->next) {
		char *path = li->data;

		file = locate_help_file (path, file_name);
		if (file != NULL)
			break;
	}

	g_slist_foreach (ret_locations, (GFunc)g_free, NULL);
	g_slist_free (ret_locations);

	if (file == NULL) {
		g_set_error (error,
			     MATE_HELP_ERROR,
			     MATE_HELP_ERROR_NOT_FOUND,
			     _("Help document %s/%s not found"),
			     doc_id,
			     file_name);
		return FALSE;
	}

	if (link_id != NULL) {
		url = g_strconcat ("ghelp://", file, "?", link_id, NULL);
	} else {
		url = g_strconcat ("ghelp://", file, NULL);
	}

	retval = mate_help_display_uri_with_env (url, envp, error);

	g_free (file);
	g_free (url);

	return retval;
}

/**
 * mate_help_display_desktop
 * @program: The current application object, or %NULL for the default one.
 * @doc_id: The name of the help file relative to the system's help domain
 * (#MATE_FILE_DOMAIN_HELP).
 * @file_name: The name of the help document to display.
 * @link_id: Can be %NULL. If set, refers to an anchor or section id within the
 * requested document.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Displays the MATE system help file specified by @file_name at location
 * @link_id in the preferred help browser of the user.  This is done by creating
 * a "ghelp" URI, by looking for @file_name in the system help domain
 * (#MATE_FILE_DOMAIN_HELP) and it's app_id.  This domain is determined when
 * the library is compiled.  If a matching file cannot be found, %FALSE is
 * returned and @error is set.
 *
 * Please note that this only displays system help.  To display help files
 * for an application, you will want to use mate_help_display().
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 */
gboolean
mate_help_display_desktop (MateProgram  *program,
			    const char    *doc_id,
			    const char    *file_name,
			    const char    *link_id,
			    GError       **error)
{
	return mate_help_display_desktop_with_env (
			program, doc_id, file_name, link_id, NULL, error);
}

/**
 * mate_help_display_uri_with_env:
 * @help_uri: The URI to display.
 * @envp: child's environment, or %NULL to inherit parent's.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Like mate_help_display_uri(), but the the contents of @envp
 * will become the help viewer's environment rather than inheriting
 * from the parents environment.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 *
 * Since: 2.2
 */
gboolean
mate_help_display_uri_with_env (const char  *help_uri,
				 char       **envp,
				 GError     **error)
{
	GError *real_error;
	gboolean retval;

	real_error = NULL;
	retval = mate_url_show_with_env (help_uri, envp, &real_error);

	if (real_error != NULL)
		g_propagate_error (error, real_error);

	return retval;
}

/**
 * mate_help_display_uri:
 * @help_uri: The URI to display.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Displays @help_uri in the user's preferred viewer. You should never need to
 * call this function directly in code, since it is just a wrapper for
 * mate_url_show() and consequently the viewer used to display the results
 * depends upon the scheme of the URI (so it is not strictly a help-only
 * function).
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 */
gboolean
mate_help_display_uri (const char  *help_uri,
			GError     **error)
{
	return mate_help_display_uri_with_env (help_uri, NULL, error);
}

/**
 * mate_help_error_quark:
 *
 * Returns: A #GQuark representing the domain of mate-help errors.
 */
GQuark
mate_help_error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0)
		error_quark =
			g_quark_from_static_string ("mate-help-error-quark");

	return error_quark;
}
