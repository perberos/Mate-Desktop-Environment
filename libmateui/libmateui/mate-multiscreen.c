/*
 * Copyright (C) 2003 Sun Microsystems Inc.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Mark McLoughlin <mark@skynet.ie>
 */

#include <config.h>

#include "mate-url.h"
#include "mate-help.h"

#include <string.h>

#if defined(__APPLE__) && defined(HAVE_NSGETENVIRON) && defined(HAVE_CRT_EXTERNS_H)
  #include <crt_externs.h>
  #define environ (*_NSGetEnviron())
#endif

#ifndef G_OS_WIN32
extern char **environ;

/**
 * make_environment_for_screen:
 * @screen: A #GdkScreen
 *
 * Description: Modifies the current program environment to
 * ensure that $DISPLAY is set such that a launched application
 * inheriting this environment would appear on @screen.
 *
 * Returns: a newly-allocated %NULL-terminated array of strings or
 * %NULL on error. Use g_strfreev() to free it.
 **/
static char **
make_environment_for_screen (GdkScreen *screen)
{
	char **retval = NULL;
	char  *display_name;
	int    i, env_len;
	int    display_index = -1;

	g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

	for (env_len = 0; environ [env_len]; env_len++)
		if (!strncmp (environ [env_len], "DISPLAY", 7))
			display_index = env_len;

	if (display_index == -1)
		display_index = env_len++;

	retval = g_new (char *, env_len + 1);
	retval [env_len] = NULL;

	display_name = gdk_screen_make_display_name (screen);

	for (i = 0; i < env_len; i++)
		if (i == display_index)
			retval [i] = g_strconcat ("DISPLAY=", display_name, NULL);
		else
			retval [i] = g_strdup (environ [i]);

	g_assert (i == env_len);

	g_free (display_name);

	return retval;
}

#else
#define make_environment_for_screen(screen) NULL
#endif

/**
 * mate_url_show_on_screen:
 * @url: The url to display. Should begin with the protocol to use (e.g.
 * "http:", "ghelp:", etc)
 * @screen: a #GdkScreen.
 * @error: Used to store any errors that result from trying to display the @url.
 *
 * Description: Like mate_url_show(), but ensures that the viewer application
 * appears on @screen.
 *
 * Returns: %TRUE if everything went fine, %FALSE otherwise (in which case
 * @error will contain the actual error).
 * 
 * Since: 2.6
 **/
gboolean
mate_url_show_on_screen (const char  *url,
			  GdkScreen   *screen,
			  GError     **error)
{
	char     **env;
	gboolean   retval;

	env = make_environment_for_screen (screen);

	retval = mate_url_show_with_env (url, env, error);

	g_strfreev (env);

	return retval;
}

/**
 * mate_help_display_on_screen:
 * @file_name: The name of the help document to display.
 * @link_id: Can be %NULL. If set, refers to an anchor or section id within the
 * requested document.
 * @screen: a #GdkScreen.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Description: Like mate_help_display(), but ensures that the help viewer
 * application appears on @screen.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 *
 * Since: 2.6
 **/
gboolean
mate_help_display_on_screen (const char  *file_name,
			      const char  *link_id,
			      GdkScreen   *screen,
			      GError     **error)
{
	return mate_help_display_with_doc_id_on_screen (
			NULL, NULL, file_name, link_id, screen, error);
}

/**
 * mate_help_display_with_doc_id_on_screen:
 * @program: The current application object, or %NULL for the default one.
 * @doc_id: The document identifier, or %NULL to default to the application ID
 * (app_id) of the specified @program.
 * @file_name: The name of the help document to display.
 * @link_id: Can be %NULL. If set, refers to an anchor or section id within the
 * requested document.
 * @screen: a #GdkScreen.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Description: Like mate_help_display_with_doc_id(), but ensures that the help
 * viewer application appears on @screen.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 *
 * Since: 2.6
 **/
gboolean
mate_help_display_with_doc_id_on_screen (MateProgram  *program,
					  const char    *doc_id,
					  const char    *file_name,
					  const char    *link_id,
					  GdkScreen     *screen,
					  GError       **error)
{
	gboolean   retval;
	char     **env;

	env = make_environment_for_screen (screen);

	retval = mate_help_display_with_doc_id_and_env (
			program, doc_id, file_name, link_id, env, error);

	g_strfreev (env);

	return retval;
}

/**
 * mate_help_display_desktop_on_screen:
 * @program: The current application object, or %NULL for the default one.
 * @doc_id: The name of the help file relative to the system's help domain
 * (#MATE_FILE_DOMAIN_HELP).
 * @file_name: The name of the help document to display.
 * @link_id: Can be %NULL. If set, refers to an anchor or section id within the
 * requested document.
 * @screen: a #GdkScreen.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Description: Like mate_help_display_desktop(), but ensures that the help
 * viewer application appears on @screen.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 *
 * Since: 2.6
 **/
gboolean
mate_help_display_desktop_on_screen (MateProgram  *program,
				      const char    *doc_id,
				      const char    *file_name,
				      const char    *link_id,
				      GdkScreen     *screen,
				      GError       **error)
{
	gboolean   retval;
	char     **env;

	env = make_environment_for_screen (screen);

	retval = mate_help_display_desktop_with_env (
			program, doc_id, file_name, link_id, env, error);

	g_strfreev (env);

	return retval;
}

/**
 * mate_help_display_uri_on_screen:
 * @help_uri: The URI to display.
 * @screen: a #GdkScreen.
 * @error: A #GError instance that will hold the specifics of any error which
 * occurs during processing, or %NULL
 *
 * Description: Like mate_help_display_uri(), but ensures that the help viewer
 * application appears on @screen.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @error will
 * contain the actual error).
 *
 * Since: 2.6
 **/
gboolean
mate_help_display_uri_on_screen (const char  *help_uri,
				  GdkScreen   *screen,
				  GError     **error)
{
	gboolean   retval;
	char     **env;

	env = make_environment_for_screen (screen);

	retval = mate_help_display_uri_with_env (help_uri, env, error);

	g_strfreev (env);

	return retval;
}
