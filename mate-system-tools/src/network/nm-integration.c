/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlosg@mate.org>
 */

#include <dbus/dbus.h>
#include "nm-integration.h"

#define NM_SERVICE "org.freedesktop.NetworkManager"
#define NM_PATH "/org/freedesktop/NetworkManager"
#define NM_INTERFACE "org.freedesktop.NetworkManager"

static DBusMessage*
create_nm_message (const gchar *method)
{
  return dbus_message_new_method_call (NM_SERVICE,
				       NM_PATH,
				       NM_INTERFACE,
				       method);
}

NMState
nm_integration_get_state (GstNetworkTool *tool)
{
  DBusMessage *message, *reply;
  DBusMessageIter iter;
  DBusError error;
  NMState state = NM_STATE_UNKNOWN;

  g_return_val_if_fail (tool->bus_connection != NULL, NM_STATE_UNKNOWN);

  dbus_error_init (&error);

  message = create_nm_message ("state");
  reply = dbus_connection_send_with_reply_and_block (tool->bus_connection, message, -1, &error);

  if (reply)
    {
      dbus_message_iter_init (reply, &iter);
      dbus_message_iter_get_basic (&iter, &state);
      dbus_message_unref (reply);
    }

  dbus_message_unref (message);

  return state;
}

void
nm_integration_sleep (GstNetworkTool *tool)
{
  DBusMessage *message;

  g_return_if_fail (tool->bus_connection != NULL);

  message = create_nm_message ("sleep");
  dbus_connection_send (tool->bus_connection, message, NULL);
  dbus_message_unref (message);
}

void
nm_integration_wake (GstNetworkTool *tool)
{
  DBusMessage *message;

  g_return_if_fail (tool->bus_connection != NULL);

  message = create_nm_message ("wake");
  dbus_connection_send (tool->bus_connection, message, NULL);
  dbus_message_unref (message);
}

gboolean
nm_integration_iface_supported (OobsIface *iface)
{
  return (OOBS_IS_IFACE_ETHERNET (iface) ||
	  OOBS_IS_IFACE_WIRELESS (iface));
}
