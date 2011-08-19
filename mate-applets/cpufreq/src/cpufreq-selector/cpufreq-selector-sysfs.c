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
#include <stdlib.h>
#include <errno.h>

#include "cpufreq-selector-sysfs.h"

#define CPUFREQ_SELECTOR_SYSFS_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), CPUFREQ_TYPE_SELECTOR_SYSFS, CPUFreqSelectorSysfsPrivate))

struct _CPUFreqSelectorSysfsPrivate {
	GList *available_freqs;
	GList *available_govs;
};

static void     cpufreq_selector_sysfs_init          (CPUFreqSelectorSysfs      *selector);
static void     cpufreq_selector_sysfs_class_init    (CPUFreqSelectorSysfsClass *klass);
static void     cpufreq_selector_sysfs_finalize      (GObject                   *object);

static gboolean cpufreq_selector_sysfs_set_frequency (CPUFreqSelector           *selector,
						      guint                      frequency,
						      GError                   **error);
static gboolean cpufreq_selector_sysfs_set_governor  (CPUFreqSelector           *selector,
						      const gchar               *governor,
						      GError                   **error);

#define CPUFREQ_SYSFS_BASE_PATH "/sys/devices/system/cpu/cpu%u/cpufreq/%s"

G_DEFINE_TYPE (CPUFreqSelectorSysfs, cpufreq_selector_sysfs, CPUFREQ_TYPE_SELECTOR)

static void
cpufreq_selector_sysfs_init (CPUFreqSelectorSysfs *selector)
{
	selector->priv = CPUFREQ_SELECTOR_SYSFS_GET_PRIVATE (selector);

	selector->priv->available_freqs = NULL;
	selector->priv->available_govs = NULL;
}

static void
cpufreq_selector_sysfs_class_init (CPUFreqSelectorSysfsClass *klass)
{
        GObjectClass         *object_class = G_OBJECT_CLASS (klass);
        CPUFreqSelectorClass *selector_class = CPUFREQ_SELECTOR_CLASS (klass);

        g_type_class_add_private (klass, sizeof (CPUFreqSelectorSysfsPrivate));

	selector_class->set_frequency = cpufreq_selector_sysfs_set_frequency;
        selector_class->set_governor = cpufreq_selector_sysfs_set_governor;

        object_class->finalize = cpufreq_selector_sysfs_finalize;
}

static void
cpufreq_selector_sysfs_finalize (GObject *object)
{
        CPUFreqSelectorSysfs *selector = CPUFREQ_SELECTOR_SYSFS (object);

        if (selector->priv->available_freqs) {
                g_list_foreach (selector->priv->available_freqs,
                                (GFunc) g_free,
				NULL);
                g_list_free (selector->priv->available_freqs);
                selector->priv->available_freqs = NULL;
	}
           
        if (selector->priv->available_govs) {
                g_list_foreach (selector->priv->available_govs,
                                (GFunc) g_free,
				NULL);
                g_list_free (selector->priv->available_govs);
                selector->priv->available_govs = NULL;
	}

	G_OBJECT_CLASS (cpufreq_selector_sysfs_parent_class)->finalize (object);
}

CPUFreqSelector *
cpufreq_selector_sysfs_new (guint cpu)
{
        CPUFreqSelector *selector;

        selector = CPUFREQ_SELECTOR (g_object_new (CPUFREQ_TYPE_SELECTOR_SYSFS,
						   "cpu", cpu, 
						   NULL));

        return selector;
}

static gchar *
cpufreq_sysfs_read (const gchar *path,
		    GError     **error)
{
	gchar  *buffer = NULL;

	if (!g_file_get_contents (path, &buffer, NULL, error)) {
		return NULL;
	}

	return g_strchomp (buffer);
}

static gboolean
cpufreq_sysfs_write (const gchar *path,
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
cpufreq_selector_sysfs_get_freqs (CPUFreqSelectorSysfs *selector)
{
        gchar  *buffer;
        GList  *list = NULL;
        gchar **frequencies = NULL;
        gint    i;
        gchar  *path;
	guint   cpu;
	GError *error = NULL;

	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);
	
        path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH, cpu, 
				"scaling_available_frequencies");

	buffer = cpufreq_sysfs_read (path, &error);
	if (!buffer) {
		g_warning ("%s", error->message);
		g_error_free (error);

		g_free (path);

		return NULL;
	}

	g_free (path);

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

static const gchar *
cpufreq_selector_sysfs_get_valid_frequency (CPUFreqSelectorSysfs *selector,
					    guint                 frequency)
{
	GList       *list = NULL;
	GList       *l;
	gint         dist = G_MAXINT;
	const gchar *retval = NULL;
	
	if (!selector->priv->available_freqs) {
		list = cpufreq_selector_sysfs_get_freqs (selector);
		selector->priv->available_freqs = list;
	} else {
		list = selector->priv->available_freqs;
	}

	if (!list)
		return NULL;

	for (l = list; l && l->data; l = g_list_next (l)) {
		const gchar *freq;
		guint        f;
		guint        current_dist;

		freq = (const gchar *) l->data;
		f = atoi (freq);

		if (f == frequency)
			return freq;

		current_dist = abs (frequency - f);
		if (current_dist < dist) {
			dist = current_dist;
			retval = freq;
		}
	}

	return retval;
}

static gboolean
cpufreq_selector_sysfs_set_frequency (CPUFreqSelector *selector,
				      guint            frequency,
				      GError         **error)
{
	gchar       *governor;
	gchar       *path;
	const gchar *frequency_text;
	guint        cpu;

	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);
	
	path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH, cpu,
				"scaling_governor");
	
	governor = cpufreq_sysfs_read (path, error);
	g_free (path);
	
	if (!governor)
		return FALSE;

	if (g_ascii_strcasecmp (governor, "userspace") != 0) {
		if (!cpufreq_selector_sysfs_set_governor (selector,
							  "userspace",
							  error)) {
			g_free (governor);

			return FALSE;
		}
	}

	g_free (governor);

	frequency_text =
		cpufreq_selector_sysfs_get_valid_frequency (CPUFREQ_SELECTOR_SYSFS (selector),
							    frequency);
	if (!frequency_text) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_ERROR,
			     SELECTOR_ERROR_SET_FREQUENCY,
			     "Cannot set frequency '%d'",
			     frequency);

		return FALSE;
	}

	path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH, cpu,
				"scaling_setspeed");
	if (!cpufreq_sysfs_write (path, frequency_text, error)) {
		g_free (path);
		
		return FALSE;
	}
	
	g_free (path);
	
	return TRUE;
}

static GList *
cpufreq_selector_sysfs_get_govs (CPUFreqSelectorSysfs *selector)
{
	gchar  *buffer;
	GList  *list = NULL;
	gchar **governors = NULL;
	gint    i;
	gchar  *path;
	guint   cpu;
	GError *error = NULL;

	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);

	path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH, cpu, 
				"scaling_available_governors");

	buffer = cpufreq_sysfs_read (path, &error);
	if (!buffer) {
		g_warning ("%s", error->message);
		g_error_free (error);

		g_free (path);

		return NULL;
	}

	g_free (path);

	governors = g_strsplit (buffer, " ", -1);

	i = 0;
	while (governors[i]) {
		list = g_list_prepend (list, g_strdup (governors[i]));
		i++;
	}

	g_strfreev (governors);
	g_free (buffer);
	
	return list;
}

static gboolean
cpufreq_selector_sysfs_validate_governor (CPUFreqSelectorSysfs *selector,
					  const gchar          *governor)
{
	GList *list = NULL;
	
	if (!selector->priv->available_govs) {
		list = cpufreq_selector_sysfs_get_govs (selector);
		selector->priv->available_govs = list;
	} else {
		list = selector->priv->available_govs;
	}

	if (!list)
		return FALSE;
	
	list = g_list_find_custom (selector->priv->available_govs,
				   governor,
				   (GCompareFunc) g_ascii_strcasecmp);
	
	return (list != NULL);
}

static gboolean
cpufreq_selector_sysfs_set_governor (CPUFreqSelector *selector,
				     const gchar     *governor,
				     GError         **error)
{
	CPUFreqSelectorSysfs *selector_sysfs;
        gchar                *path;
	guint                 cpu;

	selector_sysfs = CPUFREQ_SELECTOR_SYSFS (selector);

	if (!cpufreq_selector_sysfs_validate_governor (selector_sysfs, governor)) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_ERROR,
			     SELECTOR_ERROR_INVALID_GOVERNOR,
			     "Invalid governor '%s'",
			     governor);
		
		return FALSE;
	}
	
	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);

        path = g_strdup_printf (CPUFREQ_SYSFS_BASE_PATH, cpu,
				"scaling_governor");

	if (!cpufreq_sysfs_write (path, governor, error)) {
		g_free (path);
		
		return FALSE;
	}

        g_free (path);

	return TRUE;
}
