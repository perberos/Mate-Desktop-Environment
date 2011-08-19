/*
 * MATE CPUFreq Applet
 * Copyright (C) 2008 Carlos Garcia Campos <carlosgc@mate.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <polkit/polkit.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "cpufreq-selector.h"
#include "cpufreq-selector-factory.h"
#include "cpufreq-selector-service.h"

#include "cpufreq-selector-service-glue.h"

#define MAX_CPUS 255

struct _CPUFreqSelectorService {
        GObject parent;

	CPUFreqSelector *selectors[MAX_CPUS];
	gint             selectors_max;

	DBusGConnection *system_bus;
	
	/* PolicyKit */
	PolkitAuthority   *authority;
};

struct _CPUFreqSelectorServiceClass {
        GObjectClass parent_class;
};

G_DEFINE_TYPE (CPUFreqSelectorService, cpufreq_selector_service, G_TYPE_OBJECT)

#define BUS_NAME "org.mate.CPUFreqSelector"

GType
cpufreq_selector_service_error_get_type (void)
{
	static GType etype = 0;

	if (G_UNLIKELY (etype == 0)) {
		static const GEnumValue values[] = {
			{ SERVICE_ERROR_GENERAL,            "SERVICE_ERROR_GENERAL",            "GeneralError" },
			{ SERVICE_ERROR_DBUS,               "SERVICE_ERROR_DBUS",               "DBUSError" },
			{ SERVICE_ERROR_ALREADY_REGISTERED, "SERVICE_ERROR_ALREADY_REGISTERED", "AlreadyRegistered" },
			{ SERVICE_ERROR_NOT_AUTHORIZED,     "SERVICE_ERROR_NOT_AUTHORIZED",     "NotAuthorized"},
			{ 0, NULL, NULL}
		};
		
		etype = g_enum_register_static ("CPUFreqSelectorServiceError", values);
	}

	return etype;
}

GQuark
cpufreq_selector_service_error_quark (void)
{
	static GQuark error_quark = 0;

	if (G_UNLIKELY (error_quark == 0))
		error_quark =
			g_quark_from_static_string ("cpufreq-selector-service-error-quark");
	
	return error_quark;
}

static void
cpufreq_selector_service_finalize (GObject *object)
{
	CPUFreqSelectorService *service = CPUFREQ_SELECTOR_SERVICE (object);
	gint i;

	service->system_bus = NULL;
	
	if (service->selectors_max >= 0) {
		for (i = 0; i < service->selectors_max; i++) {
			if (service->selectors[i]) {
				g_object_unref (service->selectors[i]);
				service->selectors[i] = NULL;
			}
		}

		service->selectors_max = -1;
	}

	if (service->authority) {
		g_object_unref (service->authority);
		service->authority = NULL;
	}

	G_OBJECT_CLASS (cpufreq_selector_service_parent_class)->finalize (object);
}

static void
cpufreq_selector_service_class_init (CPUFreqSelectorServiceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = cpufreq_selector_service_finalize;
}

static void
cpufreq_selector_service_init (CPUFreqSelectorService *service)
{
	service->selectors_max = -1;
}

CPUFreqSelectorService *
cpufreq_selector_service_get_instance (void)
{
	static CPUFreqSelectorService *service = NULL;

	if (!service)
		service = CPUFREQ_SELECTOR_SERVICE (g_object_new (CPUFREQ_TYPE_SELECTOR_SERVICE, NULL));

	return service;
}

static gboolean
service_shutdown (gpointer user_data)
{
	g_object_unref (SELECTOR_SERVICE);

	return FALSE;
}

static void
reset_killtimer (void)
{
	static guint timer_id = 0;

	if (timer_id > 0)
		g_source_remove (timer_id);

	timer_id = g_timeout_add_seconds (30,
					  (GSourceFunc) service_shutdown,
					  NULL);
}

gboolean
cpufreq_selector_service_register (CPUFreqSelectorService *service,
				   GError                **error)
{
	DBusGConnection *connection;
	DBusGProxy      *bus_proxy;
	gboolean         res;
	guint            result;
	GError          *err = NULL;

	if (service->system_bus) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_SERVICE_ERROR,
			     SERVICE_ERROR_ALREADY_REGISTERED,
			     "Service %s already registered", BUS_NAME);
		return FALSE;
	}

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
	if (!connection) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_SERVICE_ERROR,
			     SERVICE_ERROR_DBUS,
			     "Couldn't connect to system bus: %s",
			     err->message);
		g_error_free (err);

		return FALSE;
	}

	bus_proxy = dbus_g_proxy_new_for_name (connection,
					       DBUS_SERVICE_DBUS,
					       DBUS_PATH_DBUS,
					       DBUS_INTERFACE_DBUS);
	if (!bus_proxy) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_SERVICE_ERROR,
			     SERVICE_ERROR_DBUS,
			     "Could not construct bus_proxy object");
		return FALSE;
	}

	res = dbus_g_proxy_call (bus_proxy,
				 "RequestName",
				 &err,
				 G_TYPE_STRING, BUS_NAME,
				 G_TYPE_UINT, DBUS_NAME_FLAG_DO_NOT_QUEUE,
				 G_TYPE_INVALID,
				 G_TYPE_UINT, &result,
				 G_TYPE_INVALID);
	g_object_unref (bus_proxy);
	
	if (!res) {
		if (err) {
			g_set_error (error,
				     CPUFREQ_SELECTOR_SERVICE_ERROR,
				     SERVICE_ERROR_DBUS,
				     "Failed to acquire %s: %s",
				     BUS_NAME, err->message);
			g_error_free (err);
		} else {
			g_set_error (error,
				     CPUFREQ_SELECTOR_SERVICE_ERROR,
				     SERVICE_ERROR_DBUS,
				     "Failed to acquire %s", BUS_NAME);
		}

		return FALSE;
	}

	if (result == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_SERVICE_ERROR,
			     SERVICE_ERROR_ALREADY_REGISTERED,
			     "Service %s already registered", BUS_NAME);
		return FALSE;
	}

	service->authority = polkit_authority_get ();

	service->system_bus = connection;

	dbus_g_object_type_install_info (CPUFREQ_TYPE_SELECTOR_SERVICE,
					 &dbus_glib_cpufreq_selector_service_object_info);
	dbus_g_connection_register_g_object (connection,
					     "/org/mate/cpufreq_selector/selector",
					     G_OBJECT (service));
	dbus_g_error_domain_register (CPUFREQ_SELECTOR_SERVICE_ERROR, NULL,
				      CPUFREQ_TYPE_SELECTOR_SERVICE_ERROR);

	reset_killtimer ();

	return TRUE;
}

static CPUFreqSelector *
get_selector_for_cpu (CPUFreqSelectorService *service,
		      guint                   cpu)
{
	if (!service->selectors[cpu]) {
		service->selectors[cpu] = cpufreq_selector_factory_create_selector (cpu);
		if (!service->selectors[cpu])
			return NULL;
		
		if (service->selectors_max < cpu)
			service->selectors_max = cpu;
	}
	
	return service->selectors[cpu];
}

/* PolicyKit */
static gboolean
cpufreq_selector_service_check_policy (CPUFreqSelectorService *service,
				       DBusGMethodInvocation  *context,
				       GError                **error)
{
	PolkitSubject             *subject;
	PolkitAuthorizationResult *result;
	gchar                     *sender;
	gboolean                   ret;

	sender = dbus_g_method_get_sender (context);
	subject = polkit_system_bus_name_new (sender);
	g_free (sender);

	result = polkit_authority_check_authorization_sync (service->authority,
                                                            subject,
                                                            "org.mate.cpufreqselector",
                                                            NULL,
                                                            POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
                                                            NULL, error);
        g_object_unref (subject);

        if (*error) {
		g_warning ("Check policy: %s", (*error)->message);
		g_object_unref (result);

		return FALSE;
	}

	ret = polkit_authorization_result_get_is_authorized (result);
	if (!ret) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_SERVICE_ERROR,
			     SERVICE_ERROR_NOT_AUTHORIZED,
			     "Caller is not authorized");
	}

	g_object_unref (result);

	return ret;
}

/* D-BUS interface */
gboolean
cpufreq_selector_service_set_frequency (CPUFreqSelectorService *service,
					guint                   cpu,
					guint                   frequency,
					DBusGMethodInvocation  *context)
{
	CPUFreqSelector *selector;
	GError          *error = NULL;

	reset_killtimer ();

	if (!cpufreq_selector_service_check_policy (service, context, &error)) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return FALSE;
	}

	if (cpu > MAX_CPUS) {
		GError *err;
		
		err = g_error_new (CPUFREQ_SELECTOR_SERVICE_ERROR,
				   SERVICE_ERROR_DBUS,
				   "Error setting frequency on cpu %d: Invalid cpu",
				   cpu);
		dbus_g_method_return_error (context, err);
		g_error_free (err);

		return FALSE;
	}

	selector = get_selector_for_cpu (service, cpu);
	if (!selector) {
		GError *err;

		err = g_error_new (CPUFREQ_SELECTOR_SERVICE_ERROR,
				   SERVICE_ERROR_DBUS,
				   "Error setting frequency on cpu %d: No cpufreq support",
				   cpu);
		dbus_g_method_return_error (context, err);
		g_error_free (err);

		return FALSE;
	}

	cpufreq_selector_set_frequency (selector, frequency, &error);
	if (error) {
		GError *err;

		err = g_error_new (CPUFREQ_SELECTOR_SERVICE_ERROR,
				   SERVICE_ERROR_DBUS,
				   "Error setting frequency %d on cpu %d: %s",
				   frequency, cpu, error->message);
		dbus_g_method_return_error (context, err);
		g_error_free (err);
		g_error_free (error);

		return FALSE;
	}
	
	dbus_g_method_return (context);
	
	return TRUE;
}

gboolean
cpufreq_selector_service_set_governor (CPUFreqSelectorService *service,
				       guint                   cpu,
				       const gchar            *governor,
				       DBusGMethodInvocation  *context)
{
	CPUFreqSelector *selector;
	GError          *error = NULL;

	reset_killtimer ();

	if (!cpufreq_selector_service_check_policy (service, context, &error)) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return FALSE;
	}
	
	if (cpu > MAX_CPUS) {
		GError *err;

		err = g_error_new (CPUFREQ_SELECTOR_SERVICE_ERROR,
				   SERVICE_ERROR_DBUS,
				   "Error setting governor on cpu %d: Invalid cpu",
				   cpu);
		dbus_g_method_return_error (context, err);
		g_error_free (err);

		return FALSE;
	}

	selector = get_selector_for_cpu (service, cpu);
	if (!selector) {
		GError *err;

		err = g_error_new (CPUFREQ_SELECTOR_SERVICE_ERROR,
				   SERVICE_ERROR_DBUS,
				   "Error setting governor on cpu %d: No cpufreq support",
				   cpu);
		dbus_g_method_return_error (context, err);
		g_error_free (err);

		return FALSE;
	}

	cpufreq_selector_set_governor (selector, governor, &error);
	if (error) {
		GError *err;

		err = g_error_new (CPUFREQ_SELECTOR_SERVICE_ERROR,
				   SERVICE_ERROR_DBUS,
				   "Error setting governor %s on cpu %d: %s",
				   governor, cpu, error->message);
		dbus_g_method_return_error (context, err);
		g_error_free (err);
		g_error_free (error);

		return FALSE;
	}

	dbus_g_method_return (context);
	
	return TRUE;
}


gboolean
cpufreq_selector_service_can_set (CPUFreqSelectorService *service,
				  DBusGMethodInvocation  *context)
{
	PolkitSubject             *subject;
	PolkitAuthorizationResult *result;
	gchar                     *sender;
	gboolean                   ret;
        GError                    *error = NULL;

	reset_killtimer ();

	sender = dbus_g_method_get_sender (context);
	subject = polkit_system_bus_name_new (sender);
	g_free (sender);

	result = polkit_authority_check_authorization_sync (service->authority,
                                                            subject,
                                                            "org.mate.cpufreqselector",
                                                            NULL,
                                                            0,
                                                            NULL,
							    &error);
        g_object_unref (subject);

	if (error) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return FALSE;
	}

        if (polkit_authorization_result_get_is_authorized (result)) {
		ret = TRUE;
	} else if (polkit_authorization_result_get_is_challenge (result)) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}

	g_object_unref (result);

        dbus_g_method_return (context, ret);

	return TRUE;
}
