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
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "cpufreq-selector-procfs.h"

static void     cpufreq_selector_procfs_init          (CPUFreqSelectorProcfs      *selector);
static void     cpufreq_selector_procfs_class_init    (CPUFreqSelectorProcfsClass *klass);

static gboolean cpufreq_selector_procfs_set_frequency (CPUFreqSelector            *selector,
						       guint                       frequency,
						       GError                    **error);
static gboolean cpufreq_selector_procfs_set_governor  (CPUFreqSelector            *selector,
						       const gchar                *governor,
						       GError                    **error);

G_DEFINE_TYPE (CPUFreqSelectorProcfs, cpufreq_selector_procfs, CPUFREQ_TYPE_SELECTOR)

static void
cpufreq_selector_procfs_init (CPUFreqSelectorProcfs *selector)
{
}

static void
cpufreq_selector_procfs_class_init (CPUFreqSelectorProcfsClass *klass)
{
        CPUFreqSelectorClass *selector_class = CPUFREQ_SELECTOR_CLASS (klass);

	selector_class->set_frequency = cpufreq_selector_procfs_set_frequency;
        selector_class->set_governor = cpufreq_selector_procfs_set_governor;
}

CPUFreqSelector *
cpufreq_selector_procfs_new (guint cpu)
{
        CPUFreqSelector *selector;

        selector = CPUFREQ_SELECTOR (g_object_new (CPUFREQ_TYPE_SELECTOR_PROCFS,
						   "cpu", cpu,
						   NULL));

        return selector;
}

static gboolean
cpufreq_procfs_read (guint    selector_cpu,
		     guint   *fmax,
		     guint   *pmin,
		     guint   *pmax,
		     guint   *fmin,
		     gchar   *mode,
		     GError **error)
{
	gchar  **lines;
	gchar   *buffer = NULL;
	gint     i;
	guint    cpu;
	gboolean found = FALSE;
	
	if (!g_file_get_contents ("/proc/cpufreq", &buffer, NULL, error)) {
		return FALSE;
	}

	lines = g_strsplit (buffer, "\n", -1);
	for (i = 0; lines[i]; i++) {
		if (g_ascii_strncasecmp (lines[i], "CPU", 3) == 0) {
			/* CPU  0       650000 kHz ( 81 %)  -     800000 kHz (100 %)  -  powersave */
			sscanf (lines[i], "CPU %u %u kHz (%u %%) - %u kHz (%u %%) - %20s",
				&cpu, fmin, pmin, fmax, pmax, mode);

			if (cpu == selector_cpu) {
				found = TRUE;
				break;
			}
		}
	}

	g_strfreev (lines);
	g_free (buffer);

	if (!found) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_ERROR,
			     SELECTOR_ERROR_INVALID_CPU,
			     "Invalid CPU number '%d'",
			     selector_cpu);
		
		return FALSE;
	}

	return TRUE;
}

static gboolean
cpufreq_procfs_write (const gchar *path,
		      const gchar *setting,
		      GError     **error)
{
	FILE *fd;

	fd = g_fopen (path, "w");

	if (!fd) {
		g_set_error (error,
			     G_FILE_ERROR,
			     g_file_error_from_errno (errno),
			     "Failed to open '%s' for writing: "
			     "g_fopen() failed: %s",
			     path,
			     g_strerror (errno));

		return FALSE;
	}

	if (g_fprintf (fd, "%s", setting) < 0) {
		g_set_error (error,
			     G_FILE_ERROR,
			     g_file_error_from_errno (errno),
			     "Failed to write '%s': "
			     "g_fprintf() failed: %s",
			     path,
			     g_strerror (errno));

		fclose (fd);

		return FALSE;
	}

	fclose (fd);

	return TRUE;
}

static gboolean 
cpufreq_selector_procfs_set_frequency (CPUFreqSelector *selector,
				       guint            frequency,
				       GError         **error)
{
        gchar *str;
	gchar *path;
	guint  freq;
	guint  cpu;
	guint  pmin, pmax;
	guint  sc_max, sc_min;
	gchar  mode[21];

	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);

	if (!cpufreq_procfs_read (cpu, &sc_max, &pmin, &pmax, &sc_min, mode, error)) {
		return FALSE;
	}

	if (g_ascii_strcasecmp (mode, "userspace") != 0) {
		if (!cpufreq_selector_procfs_set_governor (selector,
							   "userspace",
							   error)) {
			return FALSE;
		}
	}

	if (frequency != sc_max && frequency != sc_min) {
		if (abs (sc_max - frequency) < abs (frequency - sc_min))
			freq = sc_max;
		else
			freq = sc_min;
	} else {
		freq = frequency;
	}
	
	path = g_strdup_printf ("/proc/sys/cpu/%u/speed", cpu);
	str = g_strdup_printf ("%u", freq);
	if (!cpufreq_procfs_write (path, str, error)) {
		g_free (path);
		g_free (str);

		return FALSE;
	}

	g_free (path);
	g_free (str);
	
	return TRUE;
}
        
static gboolean
cpufreq_selector_procfs_set_governor (CPUFreqSelector *selector,
				      const gchar     *governor,
				      GError         **error)
{
	gchar *str;
	guint  cpu;
	guint  pmin, pmax;
	guint  sc_max, sc_min;
	gchar  mode[21];

	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);

	if (!cpufreq_procfs_read (cpu, &sc_max, &pmin, &pmax, &sc_min, mode, error)) {
		return FALSE;
	}

	if (g_ascii_strcasecmp (governor, mode) == 0)
		return TRUE;

	str = g_strdup_printf ("%u:%u:%u:%s", cpu, sc_min, sc_max, governor);

	if (!cpufreq_procfs_write ("/proc/cpufreq", str, error)) {
		g_free (str);

		return FALSE;
	}

	g_free (str);

	return TRUE;
}
