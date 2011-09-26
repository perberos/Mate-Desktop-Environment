/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Novell, Inc.
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>

#include <libintl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mdm-signal-handler.h"
#include "mdm-log.h"

#include "gsm-consolekit.h"
#include "gsm-mateconf.h"
#include "gsm-util.h"
#include "gsm-manager.h"
#include "gsm-xsmp-server.h"
#include "gsm-store.h"

#define GSM_MATECONF_DEFAULT_SESSION_KEY "/desktop/mate/session/default_session"
#define GSM_MATECONF_REQUIRED_COMPONENTS_DIRECTORY "/desktop/mate/session/required_components"
#define GSM_MATECONF_REQUIRED_COMPONENTS_LIST_KEY "/desktop/mate/session/required_components_list"

#define GSM_DBUS_NAME "org.mate.SessionManager"

#define IS_STRING_EMPTY(x) \
	((x) == NULL || (x)[0] == '\0')

static gboolean failsafe = FALSE;
static gboolean show_version = FALSE;
static gboolean debug = FALSE;

static void on_bus_name_lost(DBusGProxy* bus_proxy, const char* name, gpointer data)
{
	g_warning("Lost name on bus: %s, exiting", name);
	exit(1);
}

static gboolean acquire_name_on_proxy(DBusGProxy* bus_proxy, const char* name)
{
	GError* error;
	guint result;
	gboolean res;
	gboolean ret;

	ret = FALSE;

	if (bus_proxy == NULL)
	{
		goto out;
	}

	error = NULL;
	res = dbus_g_proxy_call(bus_proxy, "RequestName", &error, G_TYPE_STRING, name, G_TYPE_UINT, 0, G_TYPE_INVALID, G_TYPE_UINT, &result, G_TYPE_INVALID);

	if (! res)
	{
		if (error != NULL)
		{
			g_warning("Failed to acquire %s: %s", name, error->message);
			g_error_free(error);
		}
		else
		{
			g_warning ("Failed to acquire %s", name);
		}

		goto out;
	}

	if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	{
		if (error != NULL)
		{
			g_warning("Failed to acquire %s: %s", name, error->message);
			g_error_free(error);
		}
		else
		{
			g_warning("Failed to acquire %s", name);
		}

		goto out;
	}

	/* register for name lost */
	dbus_g_proxy_add_signal(bus_proxy, "NameLost", G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(bus_proxy, "NameLost", G_CALLBACK(on_bus_name_lost), NULL, NULL);

	ret = TRUE;

	out:

	return ret;
}

static gboolean acquire_name(void)
{
	DBusGProxy* bus_proxy;
	GError* error;
	DBusGConnection* connection;

	error = NULL;
	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (connection == NULL)
	{
		gsm_util_init_error(TRUE, "Could not connect to session bus: %s", error->message);
		/* not reached */
	}

	bus_proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	if (!acquire_name_on_proxy(bus_proxy, GSM_DBUS_NAME))
	{
		gsm_util_init_error(TRUE, "%s", "Could not acquire name on session bus");
		/* not reached */
	}

	g_object_unref(bus_proxy);

	return TRUE;
}

/* This doesn't contain the required components, so we need to always
 * call append_required_apps() after a call to append_default_apps(). */
static void append_default_apps(GsmManager* manager, const char* default_session_key, char** autostart_dirs)
{
	GSList* default_apps;
	GSList* a;
	MateConfClient* client;

	g_debug("main: *** Adding default apps");

	g_assert(default_session_key != NULL);
	g_assert(autostart_dirs != NULL);

	client = mateconf_client_get_default();
	default_apps = mateconf_client_get_list(client, default_session_key, MATECONF_VALUE_STRING, NULL);
	g_object_unref(client);

	for (a = default_apps; a; a = a->next)
	{
		char* app_path;

		if (IS_STRING_EMPTY((char*) a->data))
		{
			continue;
		}

		app_path = gsm_util_find_desktop_file_for_app_name(a->data, autostart_dirs);

		if (app_path != NULL)
		{
			gsm_manager_add_autostart_app(manager, app_path, NULL);
			g_free(app_path);
		}
	}

	g_slist_foreach(default_apps, (GFunc) g_free, NULL);
	g_slist_free(default_apps);
}

static void append_required_apps(GsmManager* manager)
{
	GSList* required_components;
	GSList* r;
	MateConfClient* client;

	g_debug("main: *** Adding required apps");

	client = mateconf_client_get_default();
	required_components = mateconf_client_get_list(client, GSM_MATECONF_REQUIRED_COMPONENTS_LIST_KEY, MATECONF_VALUE_STRING, NULL);

	if (required_components == NULL)
	{
		g_warning("No required applications specified");
	}

	for (r = required_components; r != NULL; r = r->next)
	{
			char* path;
			char* default_provider;
			const char* component;

			if (IS_STRING_EMPTY((char*) r->data))
			{
					continue;
			}

			component = r->data;

			path = g_strdup_printf("%s/%s", GSM_MATECONF_REQUIRED_COMPONENTS_DIRECTORY, component);

			default_provider = mateconf_client_get_string(client, path, NULL);

			g_debug ("main: %s looking for component: '%s'", path, default_provider);

			if (default_provider != NULL)
			{
				char* app_path;

				app_path = gsm_util_find_desktop_file_for_app_name(default_provider, NULL);

				if (app_path != NULL)
				{
					gsm_manager_add_autostart_app(manager, app_path, component);
				}
				else
				{
					g_warning("Unable to find provider '%s' of required component '%s'", default_provider, component);
				}

				g_free(app_path);
			}

			g_free(default_provider);
			g_free(path);
	}

	g_debug("main: *** Done adding required apps");

	g_slist_foreach(required_components, (GFunc) g_free, NULL);
	g_slist_free(required_components);

	g_object_unref(client);
}

static void maybe_load_saved_session_apps(GsmManager* manager)
{
	GsmConsolekit* consolekit;
	char* session_type;

	consolekit = gsm_get_consolekit();
	session_type = gsm_consolekit_get_current_session_type(consolekit);

	if (g_strcmp0 (session_type, GSM_CONSOLEKIT_SESSION_TYPE_LOGIN_WINDOW) != 0)
	{
		gsm_manager_add_autostart_apps_from_dir(manager, gsm_util_get_saved_session_dir());
	}

	g_object_unref(consolekit);
	g_free(session_type);
}

static void load_standard_apps (GsmManager* manager, const char* default_session_key)
{
	char** autostart_dirs;
	int i;

	autostart_dirs = gsm_util_get_autostart_dirs();

	if (!failsafe)
	{
		maybe_load_saved_session_apps(manager);

		for (i = 0; autostart_dirs[i]; i++)
		{
			gsm_manager_add_autostart_apps_from_dir(manager, autostart_dirs[i]);
		}
	}

	/* We do this at the end in case a saved session contains an
	 * application that already provides one of the components. */
	append_default_apps(manager, default_session_key, autostart_dirs);
	append_required_apps(manager);

	g_strfreev(autostart_dirs);
}

static void load_override_apps(GsmManager* manager, char** override_autostart_dirs)
{
	int i;

	for (i = 0; override_autostart_dirs[i]; i++)
	{
		gsm_manager_add_autostart_apps_from_dir(manager, override_autostart_dirs[i]);
	}
}

static gboolean signal_cb(int signo, gpointer data)
{
	int ret;
	GsmManager* manager;

	g_debug("Got callback for signal %d", signo);

	ret = TRUE;

	switch (signo)
	{
		case SIGFPE:
		case SIGPIPE:
			/* let the fatal signals interrupt us */
			g_debug ("Caught signal %d, shutting down abnormally.", signo);
			ret = FALSE;
			break;
		case SIGINT:
		case SIGTERM:
			manager = (GsmManager*) data;
			gsm_manager_logout(manager, GSM_MANAGER_LOGOUT_MODE_FORCE, NULL);

			/* let the fatal signals interrupt us */
			g_debug("Caught signal %d, shutting down normally.", signo);
			ret = TRUE;
			break;
		case SIGHUP:
			g_debug("Got HUP signal");
			ret = TRUE;
			break;
		case SIGUSR1:
			g_debug("Got USR1 signal");
			ret = TRUE;
			mdm_log_toggle_debug();
			break;
		default:
			g_debug("Caught unhandled signal %d", signo);
			ret = TRUE;

			break;
	}

	return ret;
}

static void shutdown_cb(gpointer data)
{
	GsmManager* manager = (GsmManager*) data;
	g_debug("Calling shutdown callback function");

	/*
	 * When the signal handler gets a shutdown signal, it calls
	 * this function to inform GsmManager to not restart
	 * applications in the off chance a handler is already queued
	 * to dispatch following the below call to gtk_main_quit.
	 */
	gsm_manager_set_phase(manager, GSM_MANAGER_PHASE_EXIT);

	gtk_main_quit();
}

static gboolean require_dbus_session(int argc, char** argv, GError** error)
{
	char** new_argv;
	int i;

	if (g_getenv("DBUS_SESSION_BUS_ADDRESS"))
	{
		return TRUE;
	}

	/* Just a sanity check to prevent infinite recursion if
	 * dbus-launch fails to set DBUS_SESSION_BUS_ADDRESS
	 */
	g_return_val_if_fail(!g_str_has_prefix(argv[0], "dbus-launch"), TRUE);

	/* +2 for our new arguments, +1 for NULL */
	new_argv = g_malloc(argc + 3 * sizeof (*argv));

	new_argv[0] = "dbus-launch";
	new_argv[1] = "--exit-with-session";

	for (i = 0; i < argc; i++)
	{
		new_argv[i + 2] = argv[i];
	}

	new_argv[i + 2] = NULL;

	if (!execvp("dbus-launch", new_argv))
	{
		g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, "No session bus and could not exec dbus-launch: %s", g_strerror(errno));
		return FALSE;
	}

	/* Should not be reached */
	return TRUE;
}

int main(int argc, char** argv)
{
	struct sigaction sa;
	GError* error;
	char* display_str;
	GsmManager* manager;
	GsmStore* client_store;
	GsmXsmpServer* xsmp_server;
	MdmSignalHandler* signal_handler;
	static char** override_autostart_dirs = NULL;
	static char* default_session_key = NULL;

	static GOptionEntry entries[] = {
		{"autostart", 'a', 0, G_OPTION_ARG_STRING_ARRAY, &override_autostart_dirs, N_("Override standard autostart directories"), NULL},
		{"default-session-key", 0, 0, G_OPTION_ARG_STRING, &default_session_key, N_("MateConf key used to look up default session"), NULL},
		{"debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debugging code"), NULL},
		{"failsafe", 'f', 0, G_OPTION_ARG_NONE, &failsafe, N_("Do not load user-specified applications"), NULL},
		{"version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Version of this application"), NULL},
		{NULL, 0, 0, 0, NULL, NULL, NULL }
	};

	/* Make sure that we have a session bus */
	if (!require_dbus_session(argc, argv, &error))
	{
		gsm_util_init_error(TRUE, "%s", error->message);
	}

	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);

	error = NULL;
	gtk_init_with_args(&argc, &argv, (char*) _(" - the MATE session manager"), entries, GETTEXT_PACKAGE, &error);

	if (error != NULL)
	{
		g_warning("%s", error->message);
		exit(1);
	}

	if (show_version)
	{
		g_print("%s %s\n", argv [0], VERSION);
		exit(1);
	}

	mdm_log_init();
	mdm_log_set_debug(debug);

	/* Set DISPLAY explicitly for all our children, in case --display
	 * was specified on the command line.
	 */
	display_str = gdk_get_display();
	gsm_util_setenv("DISPLAY", display_str);
	g_free(display_str);

	/* Some third-party programs rely on MATE_DESKTOP_SESSION_ID to
	 * detect if MATE is running. We keep this for compatibility reasons.
	 */
	gsm_util_setenv("MATE_DESKTOP_SESSION_ID", "this-is-deprecated");

	client_store = gsm_store_new();


	/* Start up mateconfd if not already running. */
	gsm_mateconf_init();

	xsmp_server = gsm_xsmp_server_new(client_store);

	/* Now make sure they succeeded. (They'll call
	 * gsm_util_init_error() if they failed.)
	 */
	gsm_mateconf_check();
	acquire_name();

	manager = gsm_manager_new(client_store, failsafe);

	signal_handler = mdm_signal_handler_new();
	mdm_signal_handler_add_fatal(signal_handler);
	mdm_signal_handler_add(signal_handler, SIGFPE, signal_cb, NULL);
	mdm_signal_handler_add(signal_handler, SIGHUP, signal_cb, NULL);
	mdm_signal_handler_add(signal_handler, SIGUSR1, signal_cb, NULL);
	mdm_signal_handler_add(signal_handler, SIGTERM, signal_cb, manager);
	mdm_signal_handler_add(signal_handler, SIGINT, signal_cb, manager);
	mdm_signal_handler_set_fatal_func(signal_handler, shutdown_cb, manager);

	if (override_autostart_dirs != NULL)
	{
		load_override_apps(manager, override_autostart_dirs);
	}
	else
	{
		if (!IS_STRING_EMPTY(default_session_key))
		{
			load_standard_apps(manager, default_session_key);
		}
		else
		{
			load_standard_apps(manager, GSM_MATECONF_DEFAULT_SESSION_KEY);
		}
	}

	gsm_xsmp_server_start(xsmp_server);
	gsm_manager_start(manager);

	gtk_main();

	if (xsmp_server != NULL)
	{
		g_object_unref(xsmp_server);
	}

	if (manager != NULL)
	{
		g_debug("Unreffing manager");
		g_object_unref(manager);
	}

	if (client_store != NULL)
	{
		g_object_unref(client_store);
	}

	gsm_mateconf_shutdown();

	mdm_log_shutdown();

	return 0;
}
