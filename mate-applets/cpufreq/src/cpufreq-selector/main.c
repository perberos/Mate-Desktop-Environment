/*
 * MATE CPUFreq Applet
 * Copyright (C) 2004 Carlos Garcia Campos <carlosgc@mate.org>
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
 *
 * Authors : Carlos García Campos <carlosgc@mate.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib-object.h>

#ifdef HAVE_POLKIT
#include "cpufreq-selector-service.h"
#endif
#include "cpufreq-selector-factory.h"


static gint    cpu = 0;
static gchar  *governor = NULL;
static gulong  frequency = 0;

static const GOptionEntry options[] = {
	{ "cpu",       'c', 0, G_OPTION_ARG_INT,    &cpu,       "CPU Number",       NULL },
	{ "governor",  'g', 0, G_OPTION_ARG_STRING, &governor,  "Governor",         NULL },
	{ "frequency", 'f', 0, G_OPTION_ARG_INT,    &frequency, "Frequency in KHz", NULL },
	{ NULL }
};

#ifdef HAVE_POLKIT
static void
do_exit (GMainLoop *loop,
	 GObject   *object)
{
	if (g_main_loop_is_running (loop))
		g_main_loop_quit (loop);
}

static void
cpufreq_selector_set_values_dbus (void)
{
	DBusGConnection *connection;
	DBusGProxy      *proxy;
	gboolean         res;
	GError          *error = NULL;

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (!connection) {
		g_printerr ("Couldn't connect to system bus: %s\n",
			    error->message);
		g_error_free (error);

		return;
	}

	proxy = dbus_g_proxy_new_for_name (connection,
					   "org.mate.CPUFreqSelector",
					   "/org/mate/cpufreq_selector/selector",
					   "org.mate.CPUFreqSelector");
	if (!proxy) {
		g_printerr ("Could not construct proxy object\n");

		return;
	}

	if (governor) {
		res = dbus_g_proxy_call (proxy, "SetGovernor", &error,
					 G_TYPE_UINT, cpu,
					 G_TYPE_STRING, governor,
					 G_TYPE_INVALID,
					 G_TYPE_INVALID);
		if (!res) {
			if (error) {
				g_printerr ("Error calling SetGovernor: %s\n", error->message);
				g_error_free (error);
			} else {
				g_printerr ("Error calling SetGovernor\n");
			}
			
			g_object_unref (proxy);
			
			return;
		}
	}

	if (frequency != 0) {
		res = dbus_g_proxy_call (proxy, "SetFrequency", &error,
					 G_TYPE_UINT, cpu,
					 G_TYPE_UINT, frequency,
					 G_TYPE_INVALID,
					 G_TYPE_INVALID);
		if (!res) {
			if (error) {
				g_printerr ("Error calling SetFrequency: %s\n", error->message);
				g_error_free (error);
			} else {
				g_printerr ("Error calling SetFrequency\n");
			}
			
			g_object_unref (proxy);
			
			return;
		}
	}

	g_object_unref (proxy);
}
#endif /* HAVE_POLKIT */

static void
cpufreq_selector_set_values (void)
{
	CPUFreqSelector *selector;
	GError          *error = NULL;

	selector = cpufreq_selector_factory_create_selector (cpu);
	if (!selector) {
		g_printerr ("No cpufreq support\n");

		return;
	}

	if (governor) {
		cpufreq_selector_set_governor (selector, governor, &error);

		if (error) {
			g_printerr ("%s\n", error->message);
			g_error_free (error);
			error = NULL;
		}
	}

	if (frequency != 0) {
		cpufreq_selector_set_frequency (selector, frequency, &error);

		if (error) {
			g_printerr ("%s\n", error->message);
			g_error_free (error);
			error = NULL;
		}
	}

	g_object_unref (selector);
}

gint
main (gint argc, gchar **argv)
{
#ifdef HAVE_POLKIT
	GMainLoop      *loop;
#endif
        GOptionContext *context;
	GError         *error = NULL;

#ifndef HAVE_POLKIT
	if (geteuid () != 0) {
		g_printerr ("You must be root\n");

		return 1;
	}
	
	if (argc < 2) {
		g_printerr ("Missing operand after `cpufreq-selector'\n");
		g_printerr ("Try `cpufreq-selector --help' for more information.\n");

		return 1;
	}
#endif
	
	g_type_init ();

	context = g_option_context_new ("- CPUFreq Selector");
	g_option_context_add_main_entries (context, options, NULL);
	
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		if (error) {
			g_printerr ("%s\n", error->message);
			g_error_free (error);
		} 

		g_option_context_free (context);
		
		return 1;
	}
	
	g_option_context_free (context);
	
#ifdef HAVE_POLKIT
	if (!cpufreq_selector_service_register (SELECTOR_SERVICE, &error)) {
		if (governor || frequency != 0) {
			cpufreq_selector_set_values_dbus ();

			return 0;
		}

		g_printerr ("%s\n", error->message);
		g_error_free (error);

		return 1;
	}

	cpufreq_selector_set_values ();

	loop = g_main_loop_new (NULL, FALSE);
	g_object_weak_ref (G_OBJECT (SELECTOR_SERVICE),
			   (GWeakNotify) do_exit,
			   loop);
		
	g_main_loop_run (loop);

	g_main_loop_unref (loop);
#else /* !HAVE_POLKIT */
	cpufreq_selector_set_values ();
#endif /* HAVE_POLKIT */

        return 0;
}
