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
#include <stdio.h>

#include "cpufreq-monitor-procfs.h"
#include "cpufreq-utils.h"

static void     cpufreq_monitor_procfs_class_init                (CPUFreqMonitorProcfsClass *klass);

static gboolean cpufreq_monitor_procfs_run                       (CPUFreqMonitor       *monitor);
static GList   *cpufreq_monitor_procfs_get_available_frequencies (CPUFreqMonitor       *monitor);

G_DEFINE_TYPE (CPUFreqMonitorProcfs, cpufreq_monitor_procfs, CPUFREQ_TYPE_MONITOR)

static void
cpufreq_monitor_procfs_init (CPUFreqMonitorProcfs *monitor)
{
}

static void
cpufreq_monitor_procfs_class_init (CPUFreqMonitorProcfsClass *klass)
{
	CPUFreqMonitorClass *monitor_class = CPUFREQ_MONITOR_CLASS (klass);
	
	monitor_class->run = cpufreq_monitor_procfs_run;
	monitor_class->get_available_frequencies = cpufreq_monitor_procfs_get_available_frequencies;
}

CPUFreqMonitor *
cpufreq_monitor_procfs_new (guint cpu)
{
	   CPUFreqMonitorProcfs *monitor;

	   monitor = g_object_new (TYPE_CPUFREQ_MONITOR_PROCFS, "cpu", cpu, NULL);

	   return CPUFREQ_MONITOR (monitor);
}

static gint
cpufreq_monitor_procfs_get_freq_from_userspace (guint cpu)
{
	gchar  *buffer = NULL;
	gchar  *path;
	gchar  *p;
	gchar  *frequency;
	gint    freq;
	gint    len;
	GError *error = NULL;
	
	path = g_strdup_printf ("/proc/sys/cpu/%u/speed", cpu);

	if (!cpufreq_file_get_contents (path, &buffer, NULL, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);

		g_free (path);

		return -1;
	}

	g_free (path);
	
	/* Try to remove the '\n' */
	p = g_strrstr (buffer, "\n");
	len = strlen (buffer);
	if (p)
		len -= strlen (p);

	frequency = g_strndup (buffer, len);
	g_free (buffer);
	
	freq = atoi (frequency);
	g_free (frequency);
	
	return freq;
}

static gboolean
cpufreq_monitor_procfs_parse (CPUFreqMonitorProcfs *monitor,
			      gint                 *cpu,
			      gint                 *fmax,
			      gint                 *pmin,
			      gint                 *pmax,
			      gint                 *fmin,
			      gchar                *mode)
{
	gchar **lines;
	gchar  *buffer = NULL;
	gint    i, count;
	guint   mon_cpu;
	GError *error = NULL;

	if (!cpufreq_file_get_contents ("/proc/cpufreq", &buffer, NULL, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);

		return FALSE;
	}
	
	g_object_get (G_OBJECT (monitor),
		      "cpu", &mon_cpu, NULL);
	
	count = 0;
	lines = g_strsplit (buffer, "\n", -1);
	for (i = 0; lines[i]; i++) {
		if (g_ascii_strncasecmp (lines[i], "CPU", 3) == 0) {
			/* CPU  0       650000 kHz ( 81 %)  -     800000 kHz (100 %)  -  powersave */
			count = sscanf (lines[i], "CPU %d %d kHz (%d %%) - %d kHz (%d %%) - %20s",
					cpu, fmin, pmin, fmax, pmax, mode);
			
			if ((guint)(*cpu) == mon_cpu)
				break;
		}
	}
	
	g_strfreev (lines);
	g_free (buffer);

	return (count == 6);
}	   

static gboolean
cpufreq_procfs_cpu_is_online (void)
{
	return g_file_test ("/proc/cpufreq",
			    G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);
}

static gboolean
cpufreq_monitor_procfs_run (CPUFreqMonitor *monitor)
{
	gint   fmax, fmin, cpu;
	gint   pmin, pmax;
	gchar  mode[21];
	gint   cur_freq, max_freq;
	gchar *governor;

	if (!cpufreq_monitor_procfs_parse (CPUFREQ_MONITOR_PROCFS (monitor),
					   &cpu, &fmax, &pmin, &pmax, &fmin, mode)) {
		/* Check whether it failed because
		 * cpu is not online.
		 */
		if (!cpufreq_procfs_cpu_is_online ()) {
			g_object_set (G_OBJECT (monitor), "online", FALSE, NULL);
			return TRUE;
		}
		return FALSE;
	}
	
	governor = mode;
	max_freq = fmax;
	
	if (g_ascii_strcasecmp (governor, "powersave") == 0) {
		cur_freq = fmin;
	} else if (g_ascii_strcasecmp (governor, "performance") == 0) {
		cur_freq = fmax;
	} else if (g_ascii_strcasecmp (governor, "userspace") == 0) {
		cur_freq = cpufreq_monitor_procfs_get_freq_from_userspace (cpu);
	} else {
		cur_freq = fmax;
	}

	g_object_set (G_OBJECT (monitor),
		      "online", TRUE,
		      "governor", governor,
		      "frequency", cur_freq,
		      "max-frequency", max_freq,
		      NULL);

	return TRUE;
}

static GList *
cpufreq_monitor_procfs_get_available_frequencies (CPUFreqMonitor *monitor)
{
	gint   fmax, fmin, cpu, freq;
	gint   pmin, pmax;
	gchar  mode[21];
	GList *list = NULL;
	
	if (!cpufreq_monitor_procfs_parse (CPUFREQ_MONITOR_PROCFS (monitor), &cpu,
					   &fmax, &pmin, &pmax, &fmin, mode)) {
		return NULL;
	}
	
	if ((pmax > 0) && (pmax != 100)) {
		freq = (fmax * 100) / pmax;
		list = g_list_prepend (list, g_strdup_printf ("%d", freq));
	}
	
	list = g_list_prepend (list, g_strdup_printf ("%d", fmax));
	if (fmax != fmin)
		list = g_list_prepend (list, g_strdup_printf ("%d", fmin));

	return g_list_reverse (list);
}

