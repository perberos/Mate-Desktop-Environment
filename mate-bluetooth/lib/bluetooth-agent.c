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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "bluetooth-agent.h"

#ifdef DEBUG
#define DBG(fmt, arg...) printf("%s:%s() " fmt "\n", __FILE__, __FUNCTION__ , ## arg)
#else
#define DBG(fmt...)
#endif

#define BLUEZ_SERVICE	"org.bluez"

#define BLUEZ_MANAGER_PATH	"/"
#define BLUEZ_MANAGER_INTERFACE	"org.bluez.Manager"
#define BLUEZ_DEVICE_INTERFACE	"org.bluez.Device"

static DBusGConnection *connection = NULL;

#define BLUETOOTH_AGENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
				BLUETOOTH_TYPE_AGENT, BluetoothAgentPrivate))

typedef struct _BluetoothAgentPrivate BluetoothAgentPrivate;

struct _BluetoothAgentPrivate {
	gchar *busname;
	gchar *path;
	DBusGProxy *adapter;

	BluetoothAgentPasskeyFunc pincode_func;
	gpointer pincode_data;

	BluetoothAgentDisplayFunc display_func;
	gpointer display_data;

	BluetoothAgentPasskeyFunc passkey_func;
	gpointer passkey_data;

	BluetoothAgentConfirmFunc confirm_func;
	gpointer confirm_data;

	BluetoothAgentAuthorizeFunc authorize_func;
	gpointer authorize_data;

	BluetoothAgentCancelFunc cancel_func;
	gpointer cancel_data;
};

G_DEFINE_TYPE(BluetoothAgent, bluetooth_agent, G_TYPE_OBJECT)

static gboolean bluetooth_agent_request_pin_code(BluetoothAgent *agent,
			const char *path, DBusGMethodInvocation *context)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	char *sender = dbus_g_method_get_sender(context);
	DBusGProxy *device;
	gboolean result = FALSE;

	DBG("agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE) {
		g_free (sender);
		return FALSE;
	}

	g_free (sender);

	if (priv->pincode_func) {
		if (priv->adapter != NULL)
			device = dbus_g_proxy_new_from_proxy(priv->adapter,
						BLUEZ_DEVICE_INTERFACE, path);
		else
			device = NULL;

		result = priv->pincode_func(context, device,
							priv->pincode_data);

		if (device != NULL)
			g_object_unref(device);
	}

	return result;
}

static gboolean bluetooth_agent_request_passkey(BluetoothAgent *agent,
			const char *path, DBusGMethodInvocation *context)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	char *sender = dbus_g_method_get_sender(context);
	DBusGProxy *device;
	gboolean result = FALSE;

	DBG("agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE) {
		g_free (sender);
		return FALSE;
	}

	g_free (sender);

	if (priv->passkey_func) {
		if (priv->adapter != NULL)
			device = dbus_g_proxy_new_from_proxy(priv->adapter,
						BLUEZ_DEVICE_INTERFACE, path);
		else
			device = NULL;

		result = priv->passkey_func(context, device,
							priv->passkey_data);

		if (device != NULL)
			g_object_unref(device);
	}

	return result;
}

static gboolean bluetooth_agent_display_passkey(BluetoothAgent *agent,
			const char *path, guint passkey, guint8 entered,
						DBusGMethodInvocation *context)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	char *sender = dbus_g_method_get_sender(context);
	DBusGProxy *device;
	gboolean result = FALSE;

	DBG("agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE) {
		g_free (sender);
		return FALSE;
	}

	g_free (sender);

	if (priv->display_func) {
		if (priv->adapter != NULL)
			device = dbus_g_proxy_new_from_proxy(priv->adapter,
						BLUEZ_DEVICE_INTERFACE, path);
		else
			device = NULL;

		result = priv->display_func(context, device, passkey, entered,
							priv->display_data);

		if (device != NULL)
			g_object_unref(device);
	}

	return result;
}

static gboolean bluetooth_agent_request_confirmation(BluetoothAgent *agent,
					const char *path, guint passkey,
						DBusGMethodInvocation *context)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	char *sender = dbus_g_method_get_sender(context);
	DBusGProxy *device;
	gboolean result = FALSE;

	DBG("agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE) {
		g_free (sender);
		return FALSE;
	}

	g_free (sender);

	if (priv->confirm_func) {
		if (priv->adapter != NULL)
			device = dbus_g_proxy_new_from_proxy(priv->adapter,
						BLUEZ_DEVICE_INTERFACE, path);
		else
			device = NULL;

		result = priv->confirm_func(context, device, passkey,
							priv->confirm_data);

		if (device != NULL)
			g_object_unref(device);
	}

	return result;
}

static gboolean bluetooth_agent_authorize(BluetoothAgent *agent,
					const char *path, const char *uuid,
						DBusGMethodInvocation *context)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	char *sender = dbus_g_method_get_sender(context);
	DBusGProxy *device;
	gboolean result = FALSE;

	DBG("agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE) {
		g_free (sender);
		return FALSE;
	}

	g_free (sender);

	if (priv->authorize_func) {
		if (priv->adapter != NULL)
			device = dbus_g_proxy_new_from_proxy(priv->adapter,
						BLUEZ_DEVICE_INTERFACE, path);
		else
			device = NULL;

		result = priv->authorize_func(context, device, uuid,
							priv->authorize_data);

		if (device != NULL)
			g_object_unref(device);
	}

	return result;
}

static gboolean bluetooth_agent_confirm_mode(BluetoothAgent *agent,
			const char *mode, DBusGMethodInvocation *context)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	char *sender = dbus_g_method_get_sender(context);

	DBG("agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE) {
		g_free (sender);
		return FALSE;
	}

	g_free (sender);

	dbus_g_method_return(context);

	return TRUE;
}

static gboolean bluetooth_agent_cancel(BluetoothAgent *agent,
						DBusGMethodInvocation *context)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	char *sender = dbus_g_method_get_sender(context);
	gboolean result = FALSE;

	DBG("agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE) {
		g_free (sender);
		return FALSE;
	}

	g_free (sender);

	if (priv->cancel_func)
		result = priv->cancel_func(context, priv->cancel_data);

	return result;
}

static gboolean bluetooth_agent_release(BluetoothAgent *agent,
						DBusGMethodInvocation *context)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	char *sender = dbus_g_method_get_sender(context);

	DBG("agent %p sender %s", agent, sender);

	if (g_str_equal(sender, priv->busname) == FALSE) {
		g_free (sender);
		return FALSE;
	}

	g_free (sender);

	g_object_unref(agent);

	dbus_g_method_return(context);

	return TRUE;
}

#include "bluetooth-agent-glue.h"

static void bluetooth_agent_init(BluetoothAgent *agent)
{
	DBG("agent %p", agent);
}

static void bluetooth_agent_finalize(GObject *agent)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);

	DBG("agent %p", agent);

	if (priv->adapter != NULL) {
		dbus_g_proxy_call(priv->adapter, "UnregisterAgent", NULL,
					DBUS_TYPE_G_OBJECT_PATH, priv->path,
					G_TYPE_INVALID, G_TYPE_INVALID);

		g_object_unref(priv->adapter);
		priv->adapter = NULL;
	}

	g_free(priv->path);
	g_free(priv->busname);

	G_OBJECT_CLASS(bluetooth_agent_parent_class)->finalize(agent);
}

static void bluetooth_agent_class_init(BluetoothAgentClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GError *error = NULL;

	DBG("class %p", klass);

	g_type_class_add_private(klass, sizeof(BluetoothAgentPrivate));

	object_class->finalize = bluetooth_agent_finalize;

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);

	if (error != NULL) {
		g_printerr("Connecting to system bus failed: %s\n",
							error->message);
		g_error_free(error);
	}

	dbus_g_object_type_install_info(BLUETOOTH_TYPE_AGENT,
				&dbus_glib_bluetooth_agent_object_info);

	//dbus_g_error_domain_register(AGENT_ERROR, "org.bluez.Error",
	//						AGENT_ERROR_TYPE);
}

BluetoothAgent *bluetooth_agent_new(void)
{
	BluetoothAgent *agent;

	agent = BLUETOOTH_AGENT(g_object_new(BLUETOOTH_TYPE_AGENT, NULL));

	DBG("agent %p", agent);

	return agent;
}

gboolean bluetooth_agent_setup(BluetoothAgent *agent, const char *path)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	DBusGProxy *proxy;
	GObject *object;

	DBG("agent %p", agent);

	if (priv->path != NULL)
		return FALSE;

	priv->path = g_strdup(path);

	proxy = dbus_g_proxy_new_for_name_owner(connection, BLUEZ_SERVICE,
			BLUEZ_MANAGER_PATH, BLUEZ_MANAGER_INTERFACE, NULL);

	g_free(priv->busname);

	if (proxy != NULL) {
		priv->busname = g_strdup(dbus_g_proxy_get_bus_name(proxy));
		g_object_unref(proxy);
	} else
		priv->busname = NULL;

	object = dbus_g_connection_lookup_g_object(connection, priv->path);
	if (object != NULL)
		g_object_unref(object);

	dbus_g_connection_register_g_object(connection,
						priv->path, G_OBJECT(agent));

	return TRUE;
}

gboolean bluetooth_agent_register(BluetoothAgent *agent, DBusGProxy *adapter)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	DBusGProxy *proxy;
	GObject *object;
	GError *error = NULL;
	gchar *path;

	g_return_val_if_fail (BLUETOOTH_IS_AGENT (agent), FALSE);
	g_return_val_if_fail (DBUS_IS_G_PROXY (adapter), FALSE);

	DBG("agent %p", agent);

	if (priv->adapter != NULL)
		return FALSE;

	priv->adapter = g_object_ref(adapter);

	path = g_path_get_basename(dbus_g_proxy_get_path(adapter));

	priv->path = g_strdup_printf("/org/bluez/agent/%s", path);

	g_free(path);

	proxy = dbus_g_proxy_new_for_name_owner(connection,
			dbus_g_proxy_get_bus_name(priv->adapter),
			dbus_g_proxy_get_path(priv->adapter),
			dbus_g_proxy_get_interface(priv->adapter), NULL);

	g_free(priv->busname);

	if (proxy != NULL) {
		priv->busname = g_strdup(dbus_g_proxy_get_bus_name(proxy));
		g_object_unref(proxy);
	} else
		priv->busname = g_strdup(dbus_g_proxy_get_bus_name(adapter));

	object = dbus_g_connection_lookup_g_object(connection, priv->path);
	if (object != NULL)
		g_object_unref(object);

	dbus_g_connection_register_g_object(connection,
						priv->path, G_OBJECT(agent));

	dbus_g_proxy_call(priv->adapter, "RegisterAgent", &error,
					DBUS_TYPE_G_OBJECT_PATH, priv->path,
					G_TYPE_STRING, "DisplayYesNo",
					G_TYPE_INVALID, G_TYPE_INVALID);

	if (error != NULL) {
		g_printerr("Agent registration failed: %s\n",
							error->message);
		g_error_free(error);
	}

	return TRUE;
}

gboolean bluetooth_agent_unregister(BluetoothAgent *agent)
{
	BluetoothAgentPrivate *priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);
	GError *error = NULL;

	g_return_val_if_fail (BLUETOOTH_IS_AGENT (agent), FALSE);

	DBG("agent %p", agent);

	if (priv->adapter == NULL)
		return FALSE;

	DBG("unregistering agent for path '%s'", priv->path);

	dbus_g_proxy_call (priv->adapter, "UnregisterAgent", &error,
			   DBUS_TYPE_G_OBJECT_PATH, priv->path,
			   G_TYPE_INVALID, G_TYPE_INVALID);

	if (error != NULL) {
		/* Ignore errors if the adapter is gone */
		if (g_error_matches (error, DBUS_GERROR, DBUS_GERROR_UNKNOWN_METHOD) == FALSE) {
			g_printerr ("Agent unregistration failed: %s '%s'\n",
				    error->message,
				    g_quark_to_string (error->domain));
		}
		g_error_free(error);
	}

	g_object_unref(priv->adapter);
	priv->adapter = NULL;

	g_free(priv->path);
	priv->path = NULL;

	return TRUE;
}

void bluetooth_agent_set_pincode_func(BluetoothAgent *agent,
				BluetoothAgentPasskeyFunc func, gpointer data)
{
	BluetoothAgentPrivate *priv;

	g_return_if_fail (BLUETOOTH_IS_AGENT (agent));

	priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);

	DBG("agent %p", agent);

	priv->pincode_func = func;
	priv->pincode_data = data;
}

void bluetooth_agent_set_passkey_func(BluetoothAgent *agent,
				BluetoothAgentPasskeyFunc func, gpointer data)
{
	BluetoothAgentPrivate *priv;

	g_return_if_fail (BLUETOOTH_IS_AGENT (agent));

	priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);

	DBG("agent %p", agent);

	priv->passkey_func = func;
	priv->passkey_data = data;
}

void bluetooth_agent_set_display_func(BluetoothAgent *agent,
				BluetoothAgentDisplayFunc func, gpointer data)
{
	BluetoothAgentPrivate *priv;

	g_return_if_fail (BLUETOOTH_IS_AGENT (agent));

	priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);

	DBG("agent %p", agent);

	priv->display_func = func;
	priv->display_data = data;
}

void bluetooth_agent_set_confirm_func(BluetoothAgent *agent,
				BluetoothAgentConfirmFunc func, gpointer data)
{
	BluetoothAgentPrivate *priv;

	g_return_if_fail (BLUETOOTH_IS_AGENT (agent));

	priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);

	DBG("agent %p", agent);

	priv->confirm_func = func;
	priv->confirm_data = data;
}

void bluetooth_agent_set_authorize_func(BluetoothAgent *agent,
				BluetoothAgentAuthorizeFunc func, gpointer data)
{
	BluetoothAgentPrivate *priv;

	g_return_if_fail (BLUETOOTH_IS_AGENT (agent));

	priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);

	DBG("agent %p", agent);

	priv->authorize_func = func;
	priv->authorize_data = data;
}

void bluetooth_agent_set_cancel_func(BluetoothAgent *agent,
				BluetoothAgentCancelFunc func, gpointer data)
{
	BluetoothAgentPrivate *priv;

	g_return_if_fail (BLUETOOTH_IS_AGENT (agent));

	priv = BLUETOOTH_AGENT_GET_PRIVATE(agent);

	DBG("agent %p", agent);

	priv->cancel_func = func;
	priv->cancel_data = data;
}

GQuark bluetooth_agent_error_quark(void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string("agent");

	return quark;
}

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType bluetooth_agent_error_get_type(void)
{
	static GType etype = 0;
	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY(AGENT_ERROR_REJECT, "Rejected"),
			{ 0, 0, 0 }
		};

		etype = g_enum_register_static("agent", values);
	}

	return etype;
}

