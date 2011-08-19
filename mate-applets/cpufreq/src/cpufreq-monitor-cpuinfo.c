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
#include <stdio.h>

#include "cpufreq-monitor-cpuinfo.h"
#include "cpufreq-utils.h"

static void     cpufreq_monitor_cpuinfo_class_init (CPUFreqMonitorCPUInfoClass *klass);

static gboolean cpufreq_monitor_cpuinfo_run        (CPUFreqMonitor *monitor);

G_DEFINE_TYPE (CPUFreqMonitorCPUInfo, cpufreq_monitor_cpuinfo, CPUFREQ_TYPE_MONITOR)

static void
cpufreq_monitor_cpuinfo_init (CPUFreqMonitorCPUInfo *monitor)
{
}

static void
cpufreq_monitor_cpuinfo_class_init (CPUFreqMonitorCPUInfoClass *klass)
{
        CPUFreqMonitorClass *monitor_class = CPUFREQ_MONITOR_CLASS (klass);

        monitor_class->run = cpufreq_monitor_cpuinfo_run;
}

CPUFreqMonitor *
cpufreq_monitor_cpuinfo_new (guint cpu)
{
        CPUFreqMonitorCPUInfo *monitor;

        monitor = g_object_new (CPUFREQ_TYPE_MONITOR_CPUINFO, "cpu", cpu, NULL);

        return CPUFREQ_MONITOR (monitor);
}

static gboolean
cpufreq_monitor_cpuinfo_run (CPUFreqMonitor *monitor)
{
        gchar *file;
        gchar                   **lines;
        gchar *buffer = NULL;
        gchar                    *p;
        gint                      cpu, i;
        gint cur_freq, max_freq;
        gchar *governor;
        GError *error = NULL;

        file = g_strdup ("/proc/cpuinfo");
        if (!cpufreq_file_get_contents (file, &buffer, NULL, &error)) {
                g_warning ("%s", error->message);
                g_error_free (error);

                g_free (file);

                return FALSE;
        }
        g_free (file);
                
        /* TODO: SMP support */
        lines = g_strsplit (buffer, "\n", -1);
        for (i = 0; lines[i]; i++) {
                if (g_ascii_strncasecmp ("cpu MHz", lines[i], strlen ("cpu MHz")) == 0) {
                        p = g_strrstr (lines[i], ":");

                        if (p == NULL) {
                                g_strfreev (lines);
                                g_free (buffer);
                                                  
                                return FALSE;
                        }

                        if (strlen (lines[i]) < (size_t)(p - lines[i])) {
                                g_strfreev (lines);
                                g_free (buffer);
                                                  
                                return FALSE;
                        }

                        if ((sscanf (p + 1, "%d.", &cpu)) != 1) {
                                g_strfreev (lines);
                                g_free (buffer);
                                                  
                                return FALSE;
                        }

                        break;
                }
        }

        g_strfreev (lines);
        g_free (buffer);
           
        governor = g_strdup (_("Frequency Scaling Unsupported"));
        cur_freq = cpu * 1000;
        max_freq = cur_freq;

        g_object_set (G_OBJECT (monitor),
                      "governor", governor,
                      "frequency", cur_freq,
                      "max-frequency", max_freq,
                      NULL);

        g_free (governor);

        return TRUE;
}




