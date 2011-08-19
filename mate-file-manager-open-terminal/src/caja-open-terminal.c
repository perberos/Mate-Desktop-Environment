/*
 *  caja-open-terminal.c
 * 
 *  Copyright (C) 2004, 2005 Free Software Foundation, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Christian Neumair <chris@mate-de.org>
 * 
 */

#ifdef HAVE_CONFIG_H
 #include <config.h> /* for GETTEXT_PACKAGE */
#endif

#include "caja-open-terminal.h"

#include <libcaja-extension/caja-menu-provider.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkmain.h>
#include <mateconf/mateconf-client.h>
#include <libmate/mate-desktop-item.h>
#include <gio/gio.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h> /* for strcmp, strdup, ... */
#include <unistd.h> /* for chdir */
#include <stdlib.h> /* for atoi */
#include <sys/stat.h>

#define SSH_DEFAULT_PORT 22

static void caja_open_terminal_instance_init (CajaOpenTerminal      *cvs);
static void caja_open_terminal_class_init    (CajaOpenTerminalClass *class);

static GType terminal_type = 0;

typedef enum {
	FILE_INFO_LOCAL,
	FILE_INFO_DESKTOP,
	FILE_INFO_SFTP,
	FILE_INFO_OTHER
} TerminalFileInfo;

static TerminalFileInfo
get_terminal_file_info (CajaFileInfo *file_info)
{
	TerminalFileInfo  ret;
	char             *uri_scheme, *p;

	g_assert (file_info);

	uri_scheme = caja_file_info_get_activation_uri (file_info);
	p = strchr (uri_scheme, ':');
	if (p) {
		*p = 0;
	}

	if (strcmp (uri_scheme, "file") == 0) {
		ret = FILE_INFO_LOCAL;
	} else if (strcmp (uri_scheme, "x-caja-desktop") == 0) {
		ret = FILE_INFO_DESKTOP;
	} else if (strcmp (uri_scheme, "sftp") == 0 ||
		   strcmp (uri_scheme, "ssh") == 0) {
		ret = FILE_INFO_SFTP;
	} else {
		ret = FILE_INFO_OTHER;
	}

	g_free (uri_scheme);

	return ret;
}

char *
lookup_in_data_dir (const char *basename,
                    const char *data_dir)
{
	char *path;

	path = g_build_filename (data_dir, basename, NULL);
	if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
		g_free (path);
		return NULL;
	}

	return path;
}

static char *
lookup_in_data_dirs (const char *basename)
{
	const char * const *system_data_dirs;
	const char          *user_data_dir;
	char                *retval;
	int                  i;

	user_data_dir    = g_get_user_data_dir ();
	system_data_dirs = g_get_system_data_dirs ();

	if ((retval = lookup_in_data_dir (basename, user_data_dir))) {
		return retval;
	}

	for (i = 0; system_data_dirs[i]; i++) {
		if ((retval = lookup_in_data_dir (basename, system_data_dirs[i])))
			return retval;
	}

	return NULL;
}

static inline gboolean
desktop_opens_home_dir (MateConfClient *client)
{
	return mateconf_client_get_bool (client, "/apps/caja-open-terminal/desktop_opens_home_dir", NULL);
}

static inline gboolean
desktop_is_home_dir (MateConfClient *client)
{
	return mateconf_client_get_bool (client, "/apps/caja/preferences/desktop_is_home_dir", NULL);
}

#ifdef HAVE_GLIB_DESKTOP_DIR_API
  #define get_desktop_dir() g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP))
#else
  #define get_desktop_dir() g_build_filename (g_get_home_dir (), "Desktop", NULL)
#endif

static void
parse_sftp_uri (GFile *file, char **host, guint *port, char **user,
		char **path)
{
	char *uri = g_file_get_uri (file);
	char *u, *h, *s, *p;
	char *h_end;

	g_assert (uri != NULL);

	u = strchr(uri, ':');
	g_assert (u != NULL);
	u+=2;

	p = strchr (u, '/');

	h = strchr(u, '@');

	if (h && (p == NULL || h < p)) {
		*h='\0';
		h++;
	} else {
		h = u;
		u = NULL;
	}

	s = strchr(h, ':');

	if (s && (p == NULL || s < p)) {
		h_end = s-1;
		*s = '\0';
		s++;
	} else {
		h_end = p;
		s = NULL;
	}

	if (h_end == NULL) {
		h_end = h + strlen(h);
	}

	*user = strdup(u);
	*port = s == NULL ? 0 : atoi(s); /* FIXME: getservbyname ? */
	*path = g_uri_unescape_string (p, "/");
	*h_end = '\0';
	*host = strdup(h);

	g_free (uri);
}

static void
append_sftp_info (char **terminal_exec,
		  CajaFileInfo *file_info)
{
	GFile *vfs_uri;
	char *host_name, *path, *user_name;
	char *user_host, *cmd, *quoted_cmd;
	guint host_port;

	g_assert (terminal_exec != NULL);
	g_assert (file_info != NULL);

	
	vfs_uri = g_file_new_for_uri (caja_file_info_get_activation_uri (file_info));
	g_assert (vfs_uri != NULL);

	g_assert (g_file_has_uri_scheme(vfs_uri, "sftp")==TRUE ||
		  g_file_has_uri_scheme(vfs_uri, "ssh")==TRUE);

	parse_sftp_uri (vfs_uri, &host_name, &host_port, &user_name, &path);

	if (host_port == 0) {
		host_port = SSH_DEFAULT_PORT;
	}

	if (user_name != NULL) {
		user_host = g_strdup_printf ("%s@%s", user_name, host_name);
	} else {
		user_host = g_strdup (host_name);
	}

	cmd = g_strdup_printf ("ssh %s -p %d -t \"cd \'%s\' && $SHELL -l\"", user_host, host_port, path);
	quoted_cmd = g_shell_quote (cmd);
	g_free (cmd);

	*terminal_exec = g_realloc (*terminal_exec, strlen (*terminal_exec) + strlen (quoted_cmd) + 4 + 1);
	strcpy (*terminal_exec + strlen (*terminal_exec), " -e ");
	strcpy (*terminal_exec + strlen (*terminal_exec), quoted_cmd);

	g_free (host_name);
	g_free (user_name);
	g_free (path);

	g_free (quoted_cmd);
	g_free (user_host);
	g_object_unref (vfs_uri);
}

static void
open_terminal_callback (CajaMenuItem *item,
			CajaFileInfo *file_info)
{
	gchar *display_str;
	const gchar *old_display_str;
	gchar *uri;
	gchar **argv, *terminal_exec;
	gchar *working_directory;
	gchar *dfile;
	MateDesktopItem *ditem;
	GdkScreen *screen;
	static MateConfClient *client;

	client = mateconf_client_get_default ();

	terminal_exec = mateconf_client_get_string (client,
						 "/desktop/mate/applications/terminal/"
						 "exec",
						 NULL);

	if (terminal_exec == NULL || strlen (terminal_exec) == 0) {
		g_free (terminal_exec);
		terminal_exec = g_strdup ("mate-terminal");
	}

	switch (get_terminal_file_info (file_info)) {
		case FILE_INFO_LOCAL:
			uri = caja_file_info_get_activation_uri (file_info);
			if (uri != NULL) {
				working_directory = g_filename_from_uri (uri, NULL, NULL);
			} else {
				working_directory = g_strdup (g_get_home_dir ());
			}
			g_free (uri);
			break;

		case FILE_INFO_DESKTOP:
			if (desktop_is_home_dir (client) || desktop_opens_home_dir (client)) {
				working_directory = g_strdup (g_get_home_dir ());
			} else {
				working_directory = get_desktop_dir ();
			}
			break;

		case FILE_INFO_SFTP:
			working_directory = NULL;
			append_sftp_info (&terminal_exec, file_info);
			break;

		case FILE_INFO_OTHER:
		default:
			g_assert_not_reached ();
	}

	if (g_str_has_prefix (terminal_exec, "mate-terminal")) {
		dfile = lookup_in_data_dirs ("applications/mate-terminal.desktop");
	} else {
		dfile = NULL;
	}

	g_shell_parse_argv (terminal_exec, NULL, &argv, NULL);

	display_str = NULL;
	old_display_str = g_getenv ("DISPLAY");

	screen = g_object_get_data (G_OBJECT (item), "CajaOpenTerminal::screen");
	if (screen != NULL) {
		display_str = gdk_screen_make_display_name (screen);
		g_setenv ("DISPLAY", display_str, TRUE);
	}

	if (dfile != NULL) {
		int orig_cwd = -1;

		do {
			orig_cwd = open (".", O_RDONLY);
		} while (orig_cwd == -1 && errno == EINTR);

		if (orig_cwd == -1) {
			g_message ("CajaOpenTerminal: Failed to open current Caja working directory.");
		} else if (working_directory != NULL) {

			if (chdir (working_directory) == -1) {
				int ret;

				g_message ("CajaOpenTerminal: Failed to change Caja working directory to \"%s\".",
					   working_directory);

				do {
					ret = close (orig_cwd);
				} while (ret == -1 && errno == EINTR);

				if (ret == -1) {
					g_message ("CajaOpenTerminal: Failed to close() current Caja working directory.");
				}

				orig_cwd = -1;
			}
		}

		ditem = mate_desktop_item_new_from_file (dfile, 0, NULL);

		mate_desktop_item_set_string (ditem, "Exec", terminal_exec);
		if (gtk_get_current_event_time () > 0) {
			mate_desktop_item_set_launch_time (ditem, gtk_get_current_event_time ());
		}
		mate_desktop_item_launch (ditem, NULL, MATE_DESKTOP_ITEM_LAUNCH_USE_CURRENT_DIR, NULL);
		mate_desktop_item_unref (ditem);
		g_free (dfile);

		if (orig_cwd != -1) {
			int ret;

			ret = fchdir (orig_cwd);
			if (ret == -1) {
				g_message ("CajaOpenTerminal: Failed to change back Caja working directory to original location after changing it to \"%s\".",
					   working_directory);
			}

			do {
				ret = close (orig_cwd);
			} while (ret == -1 && errno == EINTR);

			if (ret == -1) {
				g_message ("CajaOpenTerminal: Failed to close Caja working directory.");
			}
		}
	} else {	
		g_spawn_async (working_directory,
			       argv,
			       NULL,
			       G_SPAWN_SEARCH_PATH,
			       NULL,
			       NULL,
			       NULL,
			       NULL);
	}

	g_setenv ("DISPLAY", old_display_str, TRUE);

	g_strfreev (argv);
	g_free (terminal_exec);
	g_free (working_directory);
	g_free (display_str);
}

static CajaMenuItem *
open_terminal_menu_item_new (TerminalFileInfo  terminal_file_info,
			     GdkScreen        *screen,
			     gboolean          is_file_item)
{
	CajaMenuItem *ret;
	const char *name;
	const char *tooltip;

	switch (terminal_file_info) {
		case FILE_INFO_LOCAL:
		case FILE_INFO_SFTP:
			name = _("Open In _Terminal");
			if (is_file_item) {
				tooltip = _("Open the currently selected folder in a terminal");
			} else {
				tooltip = _("Open the currently open folder in a terminal");
			}
			break;

		case FILE_INFO_DESKTOP:
			if (desktop_opens_home_dir (mateconf_client_get_default ())) {
				name = _("Open _Terminal");
				tooltip = _("Open a terminal");
			} else {
				name = _("Open In _Terminal");
				tooltip = _("Open the currently open folder in a terminal");
			}
			break;

		case FILE_INFO_OTHER:
		default:
			g_assert_not_reached ();
	}

	ret = caja_menu_item_new ("CajaOpenTerminal::open_terminal",
				      name, tooltip, "terminal");
	g_object_set_data (G_OBJECT (ret),
			   "CajaOpenTerminal::screen",
			   screen);

	return ret;
}

static GList *
caja_open_terminal_get_background_items (CajaMenuProvider *provider,
					     GtkWidget		  *window,
					     CajaFileInfo	  *file_info)
{
	CajaMenuItem *item;
	TerminalFileInfo  terminal_file_info;

	terminal_file_info = get_terminal_file_info (file_info);
	switch (terminal_file_info) {
		case FILE_INFO_LOCAL:
		case FILE_INFO_DESKTOP:
		case FILE_INFO_SFTP:
			item = open_terminal_menu_item_new (terminal_file_info, gtk_widget_get_screen (window), FALSE);
			g_object_set_data_full (G_OBJECT (item), "file-info",
						g_object_ref (file_info),
						(GDestroyNotify) g_object_unref);
			g_signal_connect (item, "activate",
					  G_CALLBACK (open_terminal_callback),
					  file_info);

			return g_list_append (NULL, item);

		case FILE_INFO_OTHER:
			return NULL;

		default:
			g_assert_not_reached ();
	}
}

GList *
caja_open_terminal_get_file_items (CajaMenuProvider *provider,
				       GtkWidget            *window,
				       GList                *files)
{
	CajaMenuItem *item;
	TerminalFileInfo  terminal_file_info;

	if (g_list_length (files) != 1 ||
	    (!caja_file_info_is_directory (files->data) &&
	     caja_file_info_get_file_type (files->data) != G_FILE_TYPE_SHORTCUT &&
	     caja_file_info_get_file_type (files->data) != G_FILE_TYPE_MOUNTABLE)) {
		return NULL;
	}

	terminal_file_info = get_terminal_file_info (files->data);
	switch (terminal_file_info) {
		case FILE_INFO_LOCAL:
		case FILE_INFO_SFTP:
			item = open_terminal_menu_item_new (terminal_file_info, gtk_widget_get_screen (window), TRUE);
			g_object_set_data_full (G_OBJECT (item), "file-info",
						g_object_ref (files->data),
						(GDestroyNotify) g_object_unref);
			g_signal_connect (item, "activate",
					  G_CALLBACK (open_terminal_callback),
					  files->data);

			return g_list_append (NULL, item);

		case FILE_INFO_DESKTOP:
		case FILE_INFO_OTHER:
			return NULL;

		default:
			g_assert_not_reached ();
	}
}

static void
caja_open_terminal_menu_provider_iface_init (CajaMenuProviderIface *iface)
{
	iface->get_background_items = caja_open_terminal_get_background_items;
	iface->get_file_items = caja_open_terminal_get_file_items;
}

static void 
caja_open_terminal_instance_init (CajaOpenTerminal *cvs)
{
}

static void
caja_open_terminal_class_init (CajaOpenTerminalClass *class)
{
}

GType
caja_open_terminal_get_type (void) 
{
	return terminal_type;
}

void
caja_open_terminal_register_type (GTypeModule *module)
{
	static const GTypeInfo info = {
		sizeof (CajaOpenTerminalClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) caja_open_terminal_class_init,
		NULL, 
		NULL,
		sizeof (CajaOpenTerminal),
		0,
		(GInstanceInitFunc) caja_open_terminal_instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) caja_open_terminal_menu_provider_iface_init,
		NULL,
		NULL
	};

	terminal_type = g_type_module_register_type (module,
						     G_TYPE_OBJECT,
						     "CajaOpenTerminal",
						     &info, 0);

	g_type_module_add_interface (module,
				     terminal_type,
				     CAJA_TYPE_MENU_PROVIDER,
				     &menu_provider_iface_info);
}
