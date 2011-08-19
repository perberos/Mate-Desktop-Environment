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

#include <glib.h>
#include <glib/gi18n.h>

#include <string.h>
#include <stdlib.h>

#include "cpufreq-monitor-sysfs.h"
#include "cpufreq-utils.h"

enum {
        SCALING_MAX,
        SCALING_MIN,
        GOVERNOR,
        CPUINFO_MAX,
        SCALING_SETSPEED,
        SCALING_CUR_FREQ,
        N_FILES
};

static void     cpufreq_monitor_sysfs_class_init                (CPUFreqMonitorSysfsClass *klass);

static gboolean cpufreq_monitor_sysfs_run                       (CPUFreqMonitor *monitor);
static GList   *cpufreq_monitor_sysfs_get_available_frequencies (CPUFreqMonitor *monitor);
static GList   *cpufreq_monitor_sysfs_get_available_governors   (CPUFreqMonitor *monitor);

static gchar   *cpufreq_sysfs_read                              (const gchar    *path,
								 GError        **error);

/* /sys/devices/system/cpu/cpu[0]/cpufreq/scaling_max_freq
 * /sys/devices/system/cpu/cpu[0]/cpufreq/scaling_min_freq
 * /sys/devices/system/cpu/cpu[0]/cpufreq/scaling_governor
 * /sys/devices/system/cpu/cpu[0]/cpufreq/cpuinfo_max_freq
 * /sys/devices/system/cpu/cpu[0]/cpufreq/scaling_setspeed (userspace)
 * /sys/devices/system/cpu/cpu[0]/cpufreq/scaling_cur_freq (new governors)
 */
const gchar *monitor_sysfs_files[] = {
        "scaling_max_freq",
        "scaling_min_freq",
        "scaling_governor",
        "cpuinfo_max_freq",
        "scaling_setspeed",
        "scaling_cur_freq",
        NULL
};

#define CPUFREQ_SYSFS_BASE_PATH "/sys/devices/system/cpu/cpu%u/cpufreq/%s"

G_DEFINE_TYPE (CPUFreqMonitorSysfs, cpufreq_monitor_sysfs, CPUFREQ_TYPE_MONITOR)

static void
cpufreq_monitor_sysfs_init (CPUFreqMonitorSysfs *monitor)
{
}

static GObject *
cpufreq_monitor_sysfs_constructor (GType                  type,
				   guint                  n_construct_properties,
				   GObjectConstructParam *construct_params)
{
	GObject *object;
	gchar   *path;
	gchar   *frequency;
	gint     max_freq;
	guint    cpu;
	GError  *error = NULL;

	object = G_OBJECT_CLASS (
		cpufreq_monitor_sysfs_parent_class)->constructor (type,
								  n_construct_properties,
								  construct_params);
	g_object_get (G_OBJECT (object),
		      "cpu", &cpu,
		      NULL);
	
	path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH,
				cpu, monitor_sysfs_files[CPUINFO_MAX]);
	
	frequency = cpufreq_sysfs_read (path, &error);
	if (!frequency) {
		g_warning ("%s", error->message);
		g_error_free (error);
		max_freq = -1;
	} else {
		max_freq = atoi (frequency);
	}

	g_free (path);
	g_free (frequency);

	g_object_set (G_OBJECT (object),
		      "max-frequency", max_freq,
		      NULL);

	return object;
}

static void
cpufreq_monitor_sysfs_class_init (CPUFreqMonitorSysfsClass *klass)
{
	GObjectClass        *object_class = G_OBJECT_CLASS (klass);
        CPUFreqMonitorClass *monitor_class = CPUFREQ_MONITOR_CLASS (klass);

	object_class->constructor = cpufreq_monitor_sysfs_constructor;
	
        monitor_class->run = cpufreq_monitor_sysfs_run;
        monitor_class->get_available_frequencies = cpufreq_monitor_sysfs_get_available_frequencies;
        monitor_class->get_available_governors = cpufreq_monitor_sysfs_get_available_governors;
}

CPUFreqMonitor *
cpufreq_monitor_sysfs_new (guint cpu)
{
        CPUFreqMonitorSysfs *monitor;

        monitor = g_object_new (CPUFREQ_TYPE_MONITOR_SYSFS,
                                "cpu", cpu, NULL);

        return CPUFREQ_MONITOR (monitor);
}

static gchar *
cpufreq_sysfs_read (const gchar *path,
		    GError     **error)
{
	gchar *buffer = NULL;

	if (!cpufreq_file_get_contents (path, &buffer, NULL, error)) {
		return NULL;
	}
	
	return g_strchomp (buffer);
}

static gboolean
cpufreq_sysfs_cpu_is_online (guint cpu)
{
	gchar   *path;
	gboolean retval;
	
	path = g_strdup_printf ("/sys/devices/system/cpu/cpu%u/", cpu);
	retval = g_file_test (path, G_FILE_TEST_IS_DIR);
	g_free (path);

	return retval;
}

static gboolean
cpufreq_monitor_sysfs_run (CPUFreqMonitor *monitor)
{
        guint   cpu;
	gchar  *frequency;
        gchar  *governor;
	gchar  *path;
	GError *error = NULL;

        g_object_get (G_OBJECT (monitor),
		      "cpu", &cpu,
		      NULL);

	path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH,
				cpu, monitor_sysfs_files[GOVERNOR]);
	governor = cpufreq_sysfs_read (path, &error);
	if (!governor) {
		gboolean retval = FALSE;
		
		/* Check whether it failed because
		 * cpu is not online.
		 */
		if (!cpufreq_sysfs_cpu_is_online (cpu)) {
			g_object_set (G_OBJECT (monitor), "online", FALSE, NULL);
			retval = TRUE;
		} else {
			g_warning ("%s", error->message);
		}
		
		g_error_free (error);
		g_free (path);

		return retval;
	}
	
	g_free (path);

	if (g_ascii_strcasecmp (governor, "userspace") == 0) {
		path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH,
					cpu, monitor_sysfs_files[SCALING_SETSPEED]);
	} else if (g_ascii_strcasecmp (governor, "powersave") == 0) {
		path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH,
					cpu, monitor_sysfs_files[SCALING_MIN]);
	} else if (g_ascii_strcasecmp (governor, "performance") == 0) {
		path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH,
					cpu, monitor_sysfs_files[SCALING_MAX]);
	} else { /* Ondemand, Conservative, ... */
		path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH,
					cpu, monitor_sysfs_files[SCALING_CUR_FREQ]);
	}

	frequency = cpufreq_sysfs_read (path, &error);
	if (!frequency) {
		g_warning ("%s", error->message);
		g_error_free (error);
		g_free (path);
		g_free (governor);

		return FALSE;
	}

	g_free (path);

	g_object_set (G_OBJECT (monitor),
		      "online", TRUE,
		      "governor", governor,
		      "frequency", atoi (frequency),
		      NULL);

	g_free (governor);
	g_free (frequency);
	
        return TRUE;
}

static gint
compare (gconstpointer a, gconstpointer b)
{
        gint aa, bb;

        aa = atoi ((gchar *) a);
        bb = atoi ((gchar *) b);

        if (aa == bb)
                return 0;
        else if (aa > bb)
                return -1;
        else
                return 1;
}

static GList *
cpufreq_monitor_sysfs_get_available_frequencies (CPUFreqMonitor *monitor)
{
        gchar  *path;
        GList  *list = NULL;
        gchar **frequencies = NULL;
        gint    i;
        guint   cpu;
        gchar  *buffer = NULL;
        GError *error = NULL;

        g_object_get (G_OBJECT (monitor),
                      "cpu", &cpu, NULL);

        path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH, cpu, 
                                "scaling_available_frequencies");

        if (!cpufreq_file_get_contents (path, &buffer, NULL, &error)) {
                g_warning ("%s", error->message);
                g_error_free (error);

                g_free (path);

                return NULL;
        }

        g_free (path);
        
        buffer = g_strchomp (buffer);
        frequencies = g_strsplit (buffer, " ", -1);

        i = 0;
        while (frequencies[i]) {
                if (!g_list_find_custom (list, frequencies[i], compare))
                        list = g_list_prepend (list, g_strdup (frequencies[i]));
                i++;
        }
           
        g_strfreev (frequencies);
        g_free (buffer);

        return g_list_sort (list, compare);
}

static GList *
cpufreq_monitor_sysfs_get_available_governors (CPUFreqMonitor *monitor)
{
        gchar   *path;
        GList   *list = NULL;
        gchar  **governors = NULL;
        gint     i;
        guint    cpu;
        gchar   *buffer = NULL;
        GError  *error = NULL;

        g_object_get (G_OBJECT (monitor),
                      "cpu", &cpu, NULL);
        
        path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH, cpu,
                                "scaling_available_governors");

        if (!cpufreq_file_get_contents (path, &buffer, NULL, &error)) {
                g_warning ("%s", error->message);
                g_error_free (error);

                g_free (path);

                return NULL;
        }

        g_free (path);
        
        buffer = g_strchomp (buffer);

        governors = g_strsplit (buffer, " ", -1);

        i = 0;
        while (governors[i] != NULL) {
                list = g_list_prepend (list, g_strdup (governors[i]));
                i++;
        }
           
        g_strfreev (governors);
        g_free (buffer);

        return list;
}
