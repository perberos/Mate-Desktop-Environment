/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Richard Hult <richard@imendio.com>
 *         Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>
#include <string.h>
#include <glib.h>

#include "mate-vfs-volume.h"
#include "mate-vfs-drive.h"
#include "mate-vfs-volume-monitor-private.h"
#include "mate-vfs-volume-monitor-client.h"
#include "mate-vfs-dbus-utils.h"

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE 1
#endif
#include <dbus/dbus-glib-lowlevel.h>

#define d(x)

static DBusConnection *main_dbus;

#define DAEMON_SIGNAL_RULE \
  "type='signal',sender='org.mate.MateVFS.Daemon',interface='org.mate.MateVFS.Daemon'"

#define NAME_OWNER_CHANGED_SIGNAL_RULE \
  "type='signal',sender='" DBUS_SERVICE_DBUS "',interface='" DBUS_INTERFACE_DBUS "',member='NameOwnerChanged',arg0='org.mate.MateVFS.Daemon'"

static guint retry_timeout_id = 0;

/* Should only be called from the main thread. */
static gboolean
dbus_try_activate_daemon_helper (void)
{
	DBusError error;

	d(g_print ("Try activating daemon.\n"));

	dbus_error_init (&error);
	if (!dbus_bus_start_service_by_name (main_dbus,
					     DVD_DAEMON_SERVICE,
					     0,
					     NULL,
					     &error)) {
		g_warning ("Failed to re-activate daemon: %s", error.message);
		dbus_error_free (&error);
	} else {
		/* Succeeded, reload drives/volumes. */
		_mate_vfs_volume_monitor_client_daemon_died (mate_vfs_get_volume_monitor ());

		return TRUE;
	}

	return FALSE;
}

static gboolean 
dbus_try_activate_daemon_timeout_func (gpointer data)
{

	if (dbus_try_activate_daemon_helper ()) {
		retry_timeout_id = 0;
		return FALSE;
	}

	/* Try again. */
	return TRUE;
}

/* Will re-try every 5 seconds until succeeding. */
static void
dbus_try_activate_daemon (void)
{
	if (retry_timeout_id != 0) {
		return;
	}
	
	if (dbus_try_activate_daemon_helper ()) {
		return;
	}

	/* We failed to activate the daemon. This should only happen if the
	 * daemon has been explicitly killed by the user or some OOM
	 * functionality just after we tried to activate it. We try again in 5
	 * seconds.
	 */
	retry_timeout_id = g_timeout_add (5000, dbus_try_activate_daemon_timeout_func, NULL);
}

static DBusHandlerResult
dbus_filter_func (DBusConnection *connection,
		  DBusMessage    *message,
		  void           *data)
{
	if (dbus_message_is_signal (message,
				    DBUS_INTERFACE_DBUS,
				    "NameOwnerChanged")) {
		gchar *service, *old_owner, *new_owner;

		dbus_message_get_args (message,
				       NULL,
				       DBUS_TYPE_STRING, &service,
				       DBUS_TYPE_STRING, &old_owner,
				       DBUS_TYPE_STRING, &new_owner,
				       DBUS_TYPE_INVALID);

		d(g_print ("NameOwnerChanged %s %s->%s\n", service, old_owner, new_owner));
		
		if (strcmp (service, DVD_DAEMON_SERVICE) == 0) {
			if (strcmp (old_owner, "") != 0 &&
			    strcmp (new_owner, "") == 0) {
				/* No new owner, try to restart it. */
				dbus_try_activate_daemon ();
			}
		}
	}
	
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* Gets the main client dbus connection. Do not use in server */
DBusConnection *
_mate_vfs_get_main_dbus_connection (void)
{
	DBusError error;

	if (main_dbus != NULL)
		return main_dbus;
	
	dbus_error_init (&error);
	main_dbus = dbus_bus_get (DBUS_BUS_SESSION, &error);
	if (dbus_error_is_set (&error)) {
		/*g_warning ("Failed to open session DBUS connection: %s\n"
			   "Volume monitoring will not work.", error.message);*/
		dbus_error_free (&error);
		main_dbus = NULL;
		return NULL;
	}
	
	/* We pass an error here to make this block (otherwise it just
	 * sends off the match rule when flushing the connection. This
	 * way we are sure to receive signals as soon as possible).
	 */
	dbus_bus_add_match (main_dbus, DAEMON_SIGNAL_RULE, NULL);
	dbus_bus_add_match (main_dbus, NAME_OWNER_CHANGED_SIGNAL_RULE, &error);
	if (dbus_error_is_set (&error)) {
		g_warning ("Couldn't add match rule.");
		dbus_error_free (&error);
	}
	
	if (!dbus_bus_start_service_by_name (main_dbus,
					     DVD_DAEMON_SERVICE,
					     0,
					     NULL,
					     &error)) {
		g_warning ("Failed to activate daemon: %s", error.message);
		dbus_error_free (&error);
	}
	
	dbus_connection_setup_with_g_main (main_dbus, NULL);
	
	dbus_connection_add_filter (main_dbus,
				    dbus_filter_func,
				    NULL,
				    NULL);

	return main_dbus;
}

static GStaticPrivate  daemon_connection_private = G_STATIC_PRIVATE_INIT;

DBusConnection *
_mate_vfs_daemon_get_current_connection (void)
{
	return g_static_private_get (&daemon_connection_private);
}

void
mate_vfs_daemon_set_current_connection (DBusConnection *conn)
{
	g_static_private_set (&daemon_connection_private, conn, NULL);
}
