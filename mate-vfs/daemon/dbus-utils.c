/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Nokia Corporation.
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
#include <string.h>
#include <glib.h>

#include <dbus/dbus.h>

#include "mate-vfs-volume-monitor-private.h"
#include "mate-vfs-dbus-utils.h"
#include "dbus-utils.h"

/*
 * Volume messages
 */
void
dbus_utils_message_append_volume_list (DBusMessage *message, GList *volumes)
{
	DBusMessageIter  iter, array_iter;
	GList           *l;
	MateVFSVolume  *volume;
	
	g_return_if_fail (message != NULL);

	if (!volumes) {
		return;
	}

	dbus_message_iter_init_append (message, &iter);

	dbus_message_iter_open_container (&iter,
					  DBUS_TYPE_ARRAY,
					  MATE_VFS_VOLUME_DBUS_TYPE,
					  &array_iter);
	
	for (l = volumes; l; l = l->next) {
		volume = l->data;

		mate_vfs_volume_to_dbus (&array_iter, volume);
	}

	dbus_message_iter_close_container (&iter, &array_iter);
}

void
dbus_utils_message_append_drive_list (DBusMessage *message, GList *drives)
{
	DBusMessageIter  iter, array_iter;
	GList           *l;
	MateVFSDrive   *drive;
	
	g_return_if_fail (message != NULL);

	if (!drives) {
		return;
	}

	dbus_message_iter_init_append (message, &iter);
	
	dbus_message_iter_open_container (&iter,
					  DBUS_TYPE_ARRAY,
					  MATE_VFS_DRIVE_DBUS_TYPE,
					  &array_iter);
	
	for (l = drives; l; l = l->next) {
		drive = l->data;

		mate_vfs_drive_to_dbus (&array_iter, drive);
	}

	dbus_message_iter_close_container (&iter, &array_iter);
}

void
dbus_utils_message_append_volume (DBusMessage *message, MateVFSVolume *volume)
{
	DBusMessageIter  iter;
	
	g_return_if_fail (message != NULL);
	g_return_if_fail (volume != NULL);

	dbus_message_iter_init_append (message, &iter);
	mate_vfs_volume_to_dbus (&iter, volume);
}

void
dbus_utils_message_append_drive (DBusMessage *message, MateVFSDrive *drive)
{
	DBusMessageIter  iter;
	
	g_return_if_fail (message != NULL);
	g_return_if_fail (drive != NULL);

	dbus_message_iter_init_append (message, &iter);
	mate_vfs_drive_to_dbus (&iter, drive);
}

/*
 * Reply functions.
 */

static DBusMessage *
create_reply_helper (DBusMessage    *message,
		     MateVFSResult  result)
{
	DBusMessage     *reply;
	DBusMessageIter  iter;
	gint32           i;

	reply = dbus_message_new_method_return (message);
	if (!reply) {
		g_error ("Out of memory");
	}

	i = result;

	dbus_message_iter_init_append (reply, &iter);
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_INT32,
					     &i)) {
		g_error ("Out of memory");
	}

	return reply;
}

void
dbus_util_reply_result (DBusConnection *conn,
			DBusMessage      *message,
			MateVFSResult    result)
{
	DBusMessage *reply;
	reply = create_reply_helper (message, result);
	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);
}

void
dbus_util_reply_id (DBusConnection *conn,
		    DBusMessage      *message,
		    gint32            id)
{
	DBusMessage     *reply;
	DBusMessageIter  iter;

	reply = create_reply_helper (message, MATE_VFS_OK);

	dbus_message_iter_init_append (reply, &iter);
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_INT32,
					     &id)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn, reply, NULL);
	dbus_message_unref (reply);
}

void
dbus_util_start_track_name (DBusConnection *conn, const char *name)
{
	char *rule;
			
	rule = g_strdup_printf ("type='signal',sender='" DBUS_SERVICE_DBUS "',interface='" DBUS_INTERFACE_DBUS "',member='NameOwnerChanged',arg0='%s'", name);
	dbus_bus_add_match (conn, rule, NULL);
	g_free (rule);
}

void
dbus_util_stop_track_name (DBusConnection *conn, const char *name)
{
	char *rule;
			
	rule = g_strdup_printf ("type='signal',sender='" DBUS_SERVICE_DBUS "',interface='" DBUS_INTERFACE_DBUS "',member='NameOwnerChanged',arg0='%s'", name);
	dbus_bus_remove_match (conn, rule, NULL);
	g_free (rule);
}
