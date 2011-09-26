/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 * save-session.c - Small program to talk to session manager.

   Copyright (C) 1998 Tom Tromey
   Copyright (C) 2008 Red Hat, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.
*/

#include <config.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#define GSM_SERVICE_DBUS "org.mate.SessionManager"
#define GSM_PATH_DBUS "/org/mate/SessionManager"
#define GSM_INTERFACE_DBUS "org.mate.SessionManager"

enum {
	GSM_LOGOUT_MODE_NORMAL = 0,
	GSM_LOGOUT_MODE_NO_CONFIRMATION,
	GSM_LOGOUT_MODE_FORCE
};

/* True if killing. This is deprecated, but we keep it for compatibility
 * reasons. */
static gboolean kill_session = FALSE;

/* The real options that should be used now. They are not ambiguous. */
static gboolean logout = FALSE;
static gboolean force_logout = FALSE;
static gboolean logout_dialog = FALSE;
static gboolean shutdown_dialog = FALSE;

/* True if we should use dialog boxes */
static gboolean show_error_dialogs = FALSE;

/* True if we should do the requested action without confirmation */
static gboolean no_interaction = FALSE;

static char* session_name = NULL;

static GOptionEntry options[] = {
	{"logout", '\0', 0, G_OPTION_ARG_NONE, &logout, N_("Log out"), NULL},
	{"force-logout", '\0', 0, G_OPTION_ARG_NONE, &force_logout, N_("Log out, ignoring any existing inhibitors"), NULL},
	{"logout-dialog", '\0', 0, G_OPTION_ARG_NONE, &logout_dialog, N_("Show logout dialog"), NULL},
	{"shutdown-dialog", '\0', 0, G_OPTION_ARG_NONE, &shutdown_dialog, N_("Show shutdown dialog"), NULL},
	{"gui",  '\0', 0, G_OPTION_ARG_NONE, &show_error_dialogs, N_("Use dialog boxes for errors"), NULL},
	/* deprecated options */
	{"session-name", 's', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &session_name, N_("Set the current session name"), N_("NAME")},
	{"kill", '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &kill_session, N_("Kill session"), NULL},
	{"silent", '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &no_interaction, N_("Do not require confirmation"), NULL},
	{NULL}
};

static void display_error(const char* message)
{
	if (show_error_dialogs && !no_interaction)
	{
		GtkWidget* dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);

		/*gtk_window_set_default_icon_name (GTK_STOCK_SAVE);*/

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	else
	{
		g_printerr("%s\n", message);
	}
}

static DBusGConnection* get_session_bus(void)
{
	GError* error = NULL;
	DBusGConnection* bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (bus == NULL)
	{
		g_warning("Couldn't connect to session bus: %s", error->message);
		g_error_free(error);
	}

	return bus;
}

static DBusGProxy* get_sm_proxy(void)
{
	DBusGConnection* connection;
	DBusGProxy* sm_proxy;

	connection = get_session_bus();

	if (connection == NULL)
	{
		display_error(_("Could not connect to the session manager"));
		return NULL;
	}

	sm_proxy = dbus_g_proxy_new_for_name(connection, GSM_SERVICE_DBUS, GSM_PATH_DBUS, GSM_INTERFACE_DBUS);

	if (sm_proxy == NULL)
	{
		display_error(_("Could not connect to the session manager"));
		return NULL;
	}

	return sm_proxy;
}

#if 0
static void set_session_name(const char* session_name)
{
	DBusGProxy* sm_proxy;
	GError* error;
	gboolean res;

	sm_proxy = get_sm_proxy();

	if (sm_proxy == NULL)
	{
		return;
	}

	error = NULL;
	res = dbus_g_proxy_call(sm_proxy, "SetName", &error, G_TYPE_STRING, session_name, G_TYPE_INVALID, G_TYPE_INVALID);

	if (!res)
	{
		if (error != NULL)
		{
			g_warning("Failed to set session name '%s': %s", session_name, error->message);
			g_error_free(error);
		}
		else
		{
			g_warning("Failed to set session name '%s'", session_name);
		}
	}

	if (sm_proxy != NULL)
	{
		g_object_unref(sm_proxy);
	}
}
#endif

static void do_logout(unsigned int mode)
{
	DBusGProxy* sm_proxy;
	GError* error;
	gboolean res;

	sm_proxy = get_sm_proxy();

	if (sm_proxy == NULL)
	{
		return;
	}

	error = NULL;
	res = dbus_g_proxy_call(sm_proxy, "Logout", &error, G_TYPE_UINT, mode, G_TYPE_INVALID, G_TYPE_INVALID);

	if (!res)
	{
		if (error != NULL)
		{
			g_warning("Failed to call logout: %s", error->message);
			g_error_free(error);
		}
		else
		{
			g_warning("Failed to call logout");
		}
	}

	if (sm_proxy != NULL)
	{
		g_object_unref(sm_proxy);
	}
}

static void do_shutdown_dialog(void)
{
	DBusGProxy* sm_proxy;
	GError* error;
	gboolean res;

	sm_proxy = get_sm_proxy();

	if (sm_proxy == NULL)
	{
		return;
	}

	error = NULL;
	res = dbus_g_proxy_call(sm_proxy, "Shutdown", &error, G_TYPE_INVALID, G_TYPE_INVALID);

	if (!res)
	{
		if (error != NULL)
		{
			g_warning("Failed to call shutdown: %s", error->message);
			g_error_free(error);
		}
		else
		{
			g_warning("Failed to call shutdown");
		}
	}

	if (sm_proxy != NULL)
	{
		g_object_unref(sm_proxy);
	}
}

int main(int argc, char* argv[])
{
	GError* error;
	int conflicting_options;

	/* Initialize the i18n stuff */
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	error = NULL;

	if (!gtk_init_with_args(&argc, &argv, NULL, options, NULL, &error))
	{
		g_warning("Unable to start: %s", error->message);
		g_error_free(error);
		exit(1);
	}

	conflicting_options = 0;

	if (kill_session)
	{
		conflicting_options++;
	}

	if (logout)
	{
		conflicting_options++;
	}

	if (force_logout)
	{
		conflicting_options++;
	}

	if (logout_dialog)
	{
		conflicting_options++;
	}

	if (shutdown_dialog)
	{
		conflicting_options++;
	}

	if (conflicting_options > 1)
	{
		display_error(_("Program called with conflicting options"));
	}

	if (kill_session)
	{
		if (no_interaction)
		{
			force_logout = TRUE;
		}
		else
		{
			logout_dialog = TRUE;
		}
	}

	if (logout)
	{
		do_logout(GSM_LOGOUT_MODE_NO_CONFIRMATION);
	}
	else if (force_logout)
	{
		do_logout(GSM_LOGOUT_MODE_FORCE);
	}
	else if (logout_dialog)
	{
		do_logout(GSM_LOGOUT_MODE_NORMAL);
	}
	else if (shutdown_dialog)
	{
		do_shutdown_dialog();
	}

	return 0;
}
