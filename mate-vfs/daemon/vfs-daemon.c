/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2005 Nokia Corporation.
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
 */

#include <config.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <locale.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <libmatevfs/mate-vfs.h>
#include <libmatevfs/mate-vfs-dbus-utils.h>

#include <dbus/dbus-glib-lowlevel.h>

#include "mate-vfs-volume-monitor-daemon.h"
#include "mate-vfs-volume-monitor-private.h"
#include "mate-vfs-private.h"
#include "dbus-utils.h"
#include "daemon-connection.h"

#define d(x) 

static DBusConnection *  daemon_get_connection    (gboolean        create);
static void              daemon_shutdown          (void);
static gboolean          daemon_init              (void);
static void              daemon_unregistered_func (DBusConnection *conn,
						   gpointer        data);
static DBusHandlerResult daemon_message_func      (DBusConnection *conn,
						   DBusMessage    *message,
						   gpointer        data);
static DBusHandlerResult dbus_filter_func         (DBusConnection *connection,
						   DBusMessage    *message,
						   void           *data);


static gboolean replace_daemon = FALSE;

static DBusObjectPathVTable daemon_vtable = {
	daemon_unregistered_func,
	daemon_message_func,
	NULL
};

typedef struct {
	gint32 conn_id;
	char *socket_dir;
} NewConnectionData;

typedef struct {
	char *sender;
	gint32 id;
	MateVFSMonitorHandle *vfs_handle;
} MonitorHandle;

typedef struct {
	char *sender;
	GList *active_monitors;
} MonitorOwner;

static GHashTable *monitors;
static GHashTable *monitor_owners;

static DBusConnection *
daemon_get_connection (gboolean create)
{
	static DBusConnection *conn = NULL;
	DBusError              error;
	gint                   ret;
	int flags;

	if (conn) {
		return conn;
	}

	if (!create) {
		return NULL;
	}

	dbus_error_init (&error);

	conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
	if (!conn) {
		g_printerr ("Failed to connect to the D-BUS daemon: %s\n",
			    error.message);

		dbus_error_free (&error);
		return NULL;
	}

	flags = DBUS_NAME_FLAG_ALLOW_REPLACEMENT;
	if (replace_daemon) {
		flags |= DBUS_NAME_FLAG_REPLACE_EXISTING;
	}
	ret = dbus_bus_request_name (conn, DVD_DAEMON_SERVICE, flags, &error);
	if (dbus_error_is_set (&error)) {
		g_warning ("Failed to acquire vfs-daemon service: %s", error.message);
		dbus_error_free (&error);

		dbus_connection_close (conn);
		/* Remove this, it asserts:
		   dbus_connection_unref (conn); */
		conn = NULL;

		return NULL;
	}
	
	if (ret == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		g_printerr ("VFS daemon already running, exiting.\n");

		dbus_connection_close (conn);
		/* Remove this, it asserts:
		   dbus_connection_unref (conn); */
		conn = NULL;

		return NULL;
	}

	if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		g_printerr ("Not primary owner of the service, exiting.\n");

		dbus_connection_close (conn);
		/* Remove this, it asserts:
		   dbus_connection_unref (conn); */
		conn = NULL;

		return NULL;
	}

	dbus_connection_add_filter (conn,
				    dbus_filter_func,
				    NULL,
				    NULL);
	
	if (!dbus_connection_register_object_path (conn,
						   DVD_DAEMON_OBJECT,
						   &daemon_vtable,
						   NULL)) {
		g_printerr ("Failed to register object with D-BUS.\n");

		dbus_connection_close (conn);
		dbus_connection_unref (conn);
		conn = NULL;

		return NULL;
	}

        dbus_connection_setup_with_g_main (conn, NULL);

	return conn;
}

static gboolean
daemon_init (void)
{
	return daemon_get_connection (TRUE) != NULL;
}

static void
daemon_shutdown (void)
{
	DBusConnection *conn;

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	dbus_connection_close (conn);
	dbus_connection_unref (conn);
}

static void
daemon_unregistered_func (DBusConnection *conn,
			  gpointer        data)
{
}

static void
new_connection_data_free (void *memory)
{
	NewConnectionData *data;

	data = memory;

	g_free (data->socket_dir);
	g_free (data);
}


static void
daemon_new_connection_func (DBusServer     *server,
			    DBusConnection *conn,
			    gpointer        user_data)
{
	gint32            conn_id;
	NewConnectionData *data;

	data = user_data;
	
	conn_id = data->conn_id;

	/* Kill the server, no more need for it */
	dbus_server_disconnect (server);
	dbus_server_unref (server);
	
	/* Remove the socket and dir after connected */
	if (data->socket_dir) {
		rmdir (data->socket_dir);
	}
	
	d(g_print ("Got a new connection, id %d\n", conn_id));

	daemon_connection_setup (conn, conn_id);
}

#ifdef __linux__
#define USE_ABSTRACT_SOCKETS
#endif

#ifndef USE_ABSTRACT_SOCKETS
static gboolean
test_safe_socket_dir (const char *dirname)
{
	struct stat statbuf;

	if (g_stat (dirname, &statbuf) != 0) {
		return FALSE;
	}
	
#ifndef G_PLATFORM_WIN32
	if (statbuf.st_uid != getuid ()) {
		return FALSE;
	}
	
	if ((statbuf.st_mode & (S_IRWXG|S_IRWXO)) ||
	    !S_ISDIR (statbuf.st_mode)) {
		return FALSE;
	}
#endif

	return TRUE;
}

static char *
create_socket_dir (void)
{
	char *dirname;
	long iteration = 0;
	char *safe_dir;
	gchar tmp[9];
	int i;
	
	safe_dir = NULL;
	do {
		g_free (safe_dir);
		
		for (i = 0; i < 8; i++) {
			if (g_random_int_range (0, 2) == 0) {
				tmp[i] = g_random_int_range ('a', 'z' + 1);
			} else {
				tmp[i] = g_random_int_range ('A', 'Z' + 1);
			}
		}
		tmp[8] = '\0';
		
		dirname = g_strdup_printf ("matevfs-%s-%s",
					   g_get_user_name (), tmp);
		safe_dir = g_build_filename (g_get_tmp_dir (), dirname, NULL);
		g_free (dirname);

		if (g_mkdir (safe_dir, 0700) < 0) {
			switch (errno) {
			case EACCES:
				g_error ("I can't write to '%s', daemon init failed",
					 safe_dir);
				break;
				
			case ENAMETOOLONG:
				g_error ("Name '%s' too long your system is broken",
					 safe_dir);
				break;

			case ENOMEM:
#ifdef ELOOP
			case ELOOP:
#endif
			case ENOSPC:
			case ENOTDIR:
			case ENOENT:
				g_error ("Resource problem creating '%s'", safe_dir);
				break;
				
			default: /* carry on going */
				break;
			}
		}
		/* Possible race - so we re-scan. */

		if (iteration++ == 1000) {
			g_error ("Cannot find a safe socket path in '%s'", g_get_tmp_dir ());
		}
	} while (!test_safe_socket_dir (safe_dir));

	return safe_dir;
}
#endif

static gchar *
generate_address (char **folder)
{
	gint   i;
	gchar  tmp[9];
	gchar *path;

	*folder = NULL;

	for (i = 0; i < 8; i++) {
		if (g_random_int_range (0, 2) == 0) {
			tmp[i] = g_random_int_range ('a', 'z' + 1);
		} else {
			tmp[i] = g_random_int_range ('A', 'Z' + 1);
		}
	}
	tmp[8] = '\0';


#ifdef USE_ABSTRACT_SOCKETS
	path = g_strdup_printf ("unix:abstract=/dbus-vfs-daemon/socket-%s", tmp);
#else
	{
		char *dir;
		
		dir = create_socket_dir ();
		path = g_strdup_printf ("unix:path=%s/socket", dir);
		*folder = dir;
	}
#endif

	return path;
}

static void
daemon_handle_get_connection (DBusConnection *conn, DBusMessage *message)
{
	DBusServer    *server;
	DBusError      error;
	DBusMessage   *reply;
	gchar         *address;
	static gint32  conn_id = 0;
	NewConnectionData *data;
	char *socket_dir;

	address = generate_address (&socket_dir);

	dbus_error_init (&error);

	server = dbus_server_listen (address, &error);
	if (!server) {
		reply = dbus_message_new_error (message,
						DVD_ERROR_SOCKET_FAILED,
						"Failed to create new socket");
		if (!reply) {
			g_error ("Out of memory");
		}

		dbus_connection_send (conn, reply, NULL);
		dbus_message_unref (reply);

		g_free (address);
		return;
	}

	data = g_new (NewConnectionData, 1);
	data->conn_id = ++conn_id;
	data->socket_dir = socket_dir;

	dbus_server_set_new_connection_function (server,
						 daemon_new_connection_func,
						 data, new_connection_data_free);

	dbus_server_setup_with_g_main (server, NULL);

	reply = dbus_message_new_method_return (message);

	dbus_message_append_args (reply,
				  DBUS_TYPE_STRING, &address,
				  DBUS_TYPE_INT32, &conn_id,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);

	g_free (address);
}

static void
daemon_handle_cancel (DBusConnection *conn, DBusMessage *message)
{
	gint32            cancellation_id;
	gint32            conn_id;

	if (!dbus_message_get_args (message, NULL,
				    DBUS_TYPE_INT32, &cancellation_id,
				    DBUS_TYPE_INT32, &conn_id,
				    DBUS_TYPE_INVALID)) {
		return;
	}

	d(g_print ("daemon got Cancel (conn id %d, cancel id %d)\n",
		   conn_id, cancellation_id));

	daemon_connection_cancel (conn_id, cancellation_id);
}

static void
monitor_handle_free (MonitorHandle *handle)
{
	g_free (handle->sender);
	g_free (handle);
}

static void
monitor_owner_free (MonitorOwner *owner)
{
	g_free (owner->sender);
	g_free (owner);
}

/* Called on main loop always */
static void
monitor_callback_func (MateVFSMonitorHandle    *vfs_handle,
		       const gchar              *monitor_uri,
		       const gchar              *info_uri,
		       MateVFSMonitorEventType  event_type,
		       gpointer                  user_data)
{
	DBusConnection *conn;
	DBusMessage *signal;
	MonitorHandle *handle = user_data;
	gint32 event_type32;

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}
	
	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_MONITOR_SIGNAL);
	dbus_message_set_destination  (signal, handle->sender);

	event_type32 = event_type;
	
	if (dbus_message_append_args (signal,
				      DBUS_TYPE_INT32, &handle->id,
				      DBUS_TYPE_STRING, &info_uri,
				      DBUS_TYPE_INT32, &event_type32,
				      DBUS_TYPE_INVALID)) {
		/* In case this gets fired off after we've disconnected. */
		if (dbus_connection_get_is_connected (conn)) {
			dbus_connection_send (conn, signal, NULL);
		}
	}

	dbus_message_unref (signal);
}

static void
daemon_handle_monitor_add (DBusConnection *conn, DBusMessage *message)
{
	char *uri_str;
	gint32 monitor_type;
	MonitorHandle *handle;
	MonitorOwner *owner;
	const char *sender;
	static gint32 next_handle = 0;
	MateVFSResult result;
	
	if (!dbus_message_get_args (message, NULL,
				    DBUS_TYPE_STRING, &uri_str,
				    DBUS_TYPE_INT32, &monitor_type,
				    DBUS_TYPE_INVALID)) {
		dbus_util_reply_result (conn, message,
					MATE_VFS_ERROR_INTERNAL);
		return;
	}

	sender = dbus_message_get_sender (message);
	
	handle = g_new (MonitorHandle, 1);
	handle->sender = g_strdup (sender);
	handle->id = ++next_handle;
	
	result = mate_vfs_monitor_add (&handle->vfs_handle,
					uri_str,
					monitor_type,
					monitor_callback_func,
					handle);

	if (result == MATE_VFS_OK) {
		if (monitors == NULL) {
			monitors = g_hash_table_new_full (g_direct_hash, g_direct_equal,
							  NULL, (GDestroyNotify)monitor_handle_free);
			monitor_owners = g_hash_table_new_full (g_str_hash, g_str_equal,
								NULL, (GDestroyNotify)monitor_owner_free);
		}

		g_hash_table_insert (monitors, GINT_TO_POINTER (handle->id), handle);
		owner = g_hash_table_lookup (monitor_owners, sender);
		if (owner == NULL) {
			owner = g_new (MonitorOwner, 1);
			owner->sender = g_strdup (sender);
			owner->active_monitors = NULL;
			g_hash_table_insert (monitor_owners, owner->sender, owner);

			dbus_util_start_track_name (conn, owner->sender);
			
		}
		owner->active_monitors = g_list_prepend (owner->active_monitors, handle);
		
		dbus_util_reply_id (conn, message, handle->id);
	} else {
		monitor_handle_free (handle);
		dbus_util_reply_result (conn, message, result);
	}
}

static void
daemon_handle_monitor_cancel (DBusConnection *conn, DBusMessage *message)
{
	gint32 id;
	MonitorHandle *handle;
	MonitorOwner *owner;
	MateVFSResult result;
	
	if (!dbus_message_get_args (message, NULL,
				    DBUS_TYPE_INT32, &id,
				    DBUS_TYPE_INVALID)) {
		dbus_util_reply_result (conn, message,
					MATE_VFS_ERROR_INTERNAL);
		return;
	}

	handle = g_hash_table_lookup (monitors, GINT_TO_POINTER (id));
	if (handle) {
		result = mate_vfs_monitor_cancel (handle->vfs_handle);
		owner = g_hash_table_lookup (monitor_owners, handle->sender);
		if (owner) {
			owner->active_monitors = g_list_remove (owner->active_monitors, handle);
			if (owner->active_monitors == NULL) {
				dbus_util_stop_track_name (conn, handle->sender);
				g_hash_table_remove (monitor_owners, handle->sender);
			}
		}
		g_hash_table_remove (monitors, GINT_TO_POINTER (handle->id));

		dbus_util_reply_result (conn, message, result);
	} else {
		dbus_util_reply_result (conn, message, MATE_VFS_ERROR_INTERNAL);
	}
}

static void
daemon_handle_get_volumes (DBusConnection *conn, DBusMessage *message)
{
	MateVFSVolumeMonitor *monitor;
	GList                 *volumes;
	DBusMessage           *reply;

	d(g_print ("daemon got GetVolumes\n"));

	monitor = mate_vfs_get_volume_monitor ();
	volumes = mate_vfs_volume_monitor_get_mounted_volumes (monitor);

	reply = dbus_message_new_method_return (message);

	dbus_utils_message_append_volume_list (reply, volumes);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);

	g_list_foreach (volumes, (GFunc) mate_vfs_volume_unref, NULL);
	g_list_free (volumes);
}

static void
daemon_handle_get_drives (DBusConnection *conn, DBusMessage *message)
{
	MateVFSVolumeMonitor *monitor;
	GList                 *drives;
	DBusMessage           *reply;

	d(g_print ("daemon got GetDrives\n"));

	monitor = mate_vfs_get_volume_monitor ();
	drives = mate_vfs_volume_monitor_get_connected_drives (monitor);

	reply = dbus_message_new_method_return (message);

	dbus_utils_message_append_drive_list (reply, drives);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);

	g_list_foreach (drives, (GFunc) mate_vfs_drive_unref, NULL);
	g_list_free (drives);
}

static void
daemon_handle_force_probe (DBusConnection *conn, DBusMessage *message)
{
	MateVFSVolumeMonitor *monitor;
	DBusMessage           *reply;

	d(g_print ("daemon got ForceProbe\n"));

	monitor = mate_vfs_get_volume_monitor ();

	mate_vfs_volume_monitor_daemon_force_probe (monitor);

	reply = dbus_message_new_method_return (message);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
daemon_handle_emit_pre_unmount_volume (DBusConnection *conn, DBusMessage *message)
{
	MateVFSVolumeMonitor *monitor;
	dbus_int32_t           id;
	MateVFSVolume        *volume;
	DBusMessage           *reply;

	d(g_print ("daemon got EmitPreUnmountVolume\n"));

	monitor = mate_vfs_get_volume_monitor ();

	if (dbus_message_get_args (message, NULL,
				   DBUS_TYPE_INT32, &id,
				   DBUS_TYPE_INVALID)) {
		volume = mate_vfs_volume_monitor_get_volume_by_id (monitor, id);
		if (volume != NULL) {
			mate_vfs_volume_monitor_emit_pre_unmount (monitor,
								   volume);
			mate_vfs_volume_unref (volume);
		}
	}

	reply = dbus_message_new_method_return (message);

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);
}

static DBusHandlerResult
daemon_message_func (DBusConnection *conn,
		     DBusMessage    *message,
		     gpointer        data)
{
	if (dbus_message_is_method_call (message,
					 DVD_DAEMON_INTERFACE,
					 DVD_DAEMON_METHOD_GET_CONNECTION)) {
		daemon_handle_get_connection (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_CANCEL)) {
		daemon_handle_cancel (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_MONITOR_ADD)) {
		daemon_handle_monitor_add (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_MONITOR_CANCEL)) {
		daemon_handle_monitor_cancel (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_GET_VOLUMES)) {
		daemon_handle_get_volumes (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_GET_DRIVES)) {
		daemon_handle_get_drives (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_FORCE_PROBE)) {
		daemon_handle_force_probe (conn, message);
	}
	else if (dbus_message_is_method_call (message,
					      DVD_DAEMON_INTERFACE,
					      DVD_DAEMON_METHOD_EMIT_PRE_UNMOUNT_VOLUME)) {
		daemon_handle_emit_pre_unmount_volume (conn, message);
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return DBUS_HANDLER_RESULT_HANDLED;
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
		MonitorOwner *owner;
		GList *l;

		dbus_message_get_args (message,
				       NULL,
				       DBUS_TYPE_STRING, &service,
				       DBUS_TYPE_STRING, &old_owner,
				       DBUS_TYPE_STRING, &new_owner,
				       DBUS_TYPE_INVALID);

		/* Handle monitor owner going away */
		if (*new_owner == 0) {
			owner = g_hash_table_lookup (monitor_owners, service);

			if (owner) {
				for (l = owner->active_monitors; l != NULL; l = l->next) {
					MonitorHandle *handle = l->data;

					mate_vfs_monitor_cancel (handle->vfs_handle);
					g_hash_table_remove (monitors, GINT_TO_POINTER (handle->id));
				}
				owner->active_monitors = NULL;
				dbus_util_stop_track_name (connection, service);
				g_hash_table_remove (monitor_owners, service);
			}
		}
	}
	
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static void
monitor_volume_mounted_cb (MateVFSVolumeMonitor *monitor,
			   MateVFSVolume        *volume,
			   gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;

	d(g_print ("daemon got volume_mounted\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_VOLUME_MOUNTED_SIGNAL);

	dbus_utils_message_append_volume (signal, volume);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

static void
monitor_volume_unmounted_cb (MateVFSVolumeMonitor *monitor,
			     MateVFSVolume        *volume,
			     gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;
	gint32          id;

	d(g_print ("daemon got volume_unmounted\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_VOLUME_UNMOUNTED_SIGNAL);

	id = mate_vfs_volume_get_id (volume);

	dbus_message_append_args (signal,
				  DBUS_TYPE_INT32, &id,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

static void
monitor_volume_pre_unmount_cb (MateVFSVolumeMonitor *monitor,
			       MateVFSVolume        *volume,
			       gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;
	gint32          id;

	d(g_print ("daemon got volume_pre_unmount\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_VOLUME_PRE_UNMOUNT_SIGNAL);

	id = mate_vfs_volume_get_id (volume);

	dbus_message_append_args (signal,
				  DBUS_TYPE_INT32, &id,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

static void
monitor_drive_connected_cb (MateVFSVolumeMonitor *monitor,
			    MateVFSDrive         *drive,
			    gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;

	d(g_print ("daemon got drive_connected\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_DRIVE_CONNECTED_SIGNAL);

	dbus_utils_message_append_drive (signal, drive);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

static void
monitor_drive_disconnected_cb (MateVFSVolumeMonitor *monitor,
			       MateVFSDrive         *drive,
			       gpointer               user_data)
{
	DBusConnection *conn;
	DBusMessage    *signal;
	gint32          id;

	d(g_print ("daemon got drive_disconnected\n"));

	conn = daemon_get_connection (FALSE);
	if (!conn) {
		return;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_DRIVE_DISCONNECTED_SIGNAL);

	id = mate_vfs_drive_get_id (drive);
	dbus_message_append_args (signal,
				  DBUS_TYPE_INT32, &id,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (conn, signal, NULL);
	dbus_message_unref (signal);
}

int
main (int argc, char *argv[])
{
	GMainLoop             *loop;
	MateVFSVolumeMonitor *monitor;
	int i;

	d(g_print ("Starting daemon.\n"));

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if (strcmp (argv[i], "--replace") == 0) {
				replace_daemon = TRUE;
			}
		}
	}

	setlocale(LC_ALL, "");
	
	g_type_init ();

	mate_vfs_set_is_daemon (MATE_VFS_TYPE_VOLUME_MONITOR_DAEMON,
				 mate_vfs_volume_monitor_daemon_force_probe);

	if (!mate_vfs_init ()) {
		g_printerr ("Could not initialize mate vfs");
		return 1;
	}

	loop = g_main_loop_new (NULL, FALSE);

	if (!daemon_init ()) {
		d(g_print ("Couldn't init daemon, exiting.\n"));
		return 1;
	}

	/* Init the volume monitor. */
	monitor = mate_vfs_get_volume_monitor ();

	g_signal_connect (monitor,
			  "volume_mounted",
			  G_CALLBACK (monitor_volume_mounted_cb),
			  NULL);
	g_signal_connect (monitor,
			  "volume_unmounted",
			  G_CALLBACK (monitor_volume_unmounted_cb),
			  NULL);
	g_signal_connect (monitor,
			  "volume_pre_unmount",
			  G_CALLBACK (monitor_volume_pre_unmount_cb),
			  NULL);

	g_signal_connect (monitor,
			  "drive_connected",
			  G_CALLBACK (monitor_drive_connected_cb),
			  NULL);
	g_signal_connect (monitor,
			  "drive_disconnected",
			  G_CALLBACK (monitor_drive_disconnected_cb),
			  NULL);

	g_main_loop_run (loop);

	d(g_print ("Shutting down.\n"));

	daemon_shutdown ();

	return 0;
}



