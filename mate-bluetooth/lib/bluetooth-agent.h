/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __BLUETOOTH_AGENT_H
#define __BLUETOOTH_AGENT_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define BLUETOOTH_TYPE_AGENT (bluetooth_agent_get_type())
#define BLUETOOTH_AGENT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					BLUETOOTH_TYPE_AGENT, BluetoothAgent))
#define BLUETOOTH_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					BLUETOOTH_TYPE_AGENT, BluetoothAgentClass))
#define BLUETOOTH_IS_AGENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
							BLUETOOTH_TYPE_AGENT))
#define BLUETOOTH_IS_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
							BLUETOOTH_TYPE_AGENT))
#define BLUETOOTH_GET_AGENT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					BLUETOOTH_TYPE_AGENT, BluetoothAgentClass))

typedef struct _BluetoothAgent BluetoothAgent;
typedef struct _BluetoothAgentClass BluetoothAgentClass;

struct _BluetoothAgent {
	GObject parent;
};

struct _BluetoothAgentClass {
	GObjectClass parent_class;
};

GType bluetooth_agent_get_type(void);

BluetoothAgent *bluetooth_agent_new(void);

gboolean bluetooth_agent_setup(BluetoothAgent *agent, const char *path);

gboolean bluetooth_agent_register(BluetoothAgent *agent, DBusGProxy *adapter);
gboolean bluetooth_agent_unregister(BluetoothAgent *agent);

typedef gboolean (*BluetoothAgentPasskeyFunc) (DBusGMethodInvocation *context,
					DBusGProxy *device, gpointer data);
typedef gboolean (*BluetoothAgentDisplayFunc) (DBusGMethodInvocation *context,
					DBusGProxy *device, guint passkey,
						guint entered, gpointer data);
typedef gboolean (*BluetoothAgentConfirmFunc) (DBusGMethodInvocation *context,
					DBusGProxy *device, guint passkey,
								gpointer data);
typedef gboolean (*BluetoothAgentAuthorizeFunc) (DBusGMethodInvocation *context,
					DBusGProxy *device, const char *uuid,
								gpointer data);
typedef gboolean (*BluetoothAgentCancelFunc) (DBusGMethodInvocation *context,
								gpointer data);

void bluetooth_agent_set_pincode_func(BluetoothAgent *agent,
				BluetoothAgentPasskeyFunc func, gpointer data);
void bluetooth_agent_set_passkey_func(BluetoothAgent *agent,
				BluetoothAgentPasskeyFunc func, gpointer data);
void bluetooth_agent_set_display_func(BluetoothAgent *agent,
				BluetoothAgentDisplayFunc func, gpointer data);
void bluetooth_agent_set_confirm_func(BluetoothAgent *agent,
				BluetoothAgentConfirmFunc func, gpointer data);
void bluetooth_agent_set_authorize_func(BluetoothAgent *agent,
				BluetoothAgentAuthorizeFunc func, gpointer data);
void bluetooth_agent_set_cancel_func(BluetoothAgent *agent,
				BluetoothAgentCancelFunc func, gpointer data);

#define AGENT_ERROR (bluetooth_agent_error_quark())
#define AGENT_ERROR_TYPE (bluetooth_agent_error_get_type()) 

typedef enum {
	AGENT_ERROR_REJECT
} AgentError;

GType bluetooth_agent_error_get_type(void);
GQuark bluetooth_agent_error_quark(void);

G_END_DECLS

#endif /* __BLUETOOTH_AGENT_H */
