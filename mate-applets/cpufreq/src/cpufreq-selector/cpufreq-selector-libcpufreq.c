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
#include <cpufreq.h>
#include <stdlib.h>

#include "cpufreq-selector-libcpufreq.h"

static void     cpufreq_selector_libcpufreq_init          (CPUFreqSelectorLibcpufreq      *selector);
static void     cpufreq_selector_libcpufreq_class_init    (CPUFreqSelectorLibcpufreqClass *klass);

static gboolean cpufreq_selector_libcpufreq_set_frequency (CPUFreqSelector           *selector,
							   guint                      frequency,
							   GError                   **error);
static gboolean cpufreq_selector_libcpufreq_set_governor  (CPUFreqSelector           *selector,
							   const gchar               *governor,
							   GError                   **error);

G_DEFINE_TYPE (CPUFreqSelectorLibcpufreq, cpufreq_selector_libcpufreq, CPUFREQ_TYPE_SELECTOR)

typedef struct cpufreq_policy                CPUFreqPolicy;
typedef struct cpufreq_available_frequencies CPUFreqFrequencyList;
typedef struct cpufreq_available_governors   CPUFreqGovernorList;

static void
cpufreq_selector_libcpufreq_init (CPUFreqSelectorLibcpufreq *selector)
{
}

static void
cpufreq_selector_libcpufreq_class_init (CPUFreqSelectorLibcpufreqClass *klass)
{
        CPUFreqSelectorClass *selector_class = CPUFREQ_SELECTOR_CLASS (klass);

	selector_class->set_frequency = cpufreq_selector_libcpufreq_set_frequency;
        selector_class->set_governor = cpufreq_selector_libcpufreq_set_governor;
}

CPUFreqSelector *
cpufreq_selector_libcpufreq_new (guint cpu)
{
        CPUFreqSelector *selector;

        selector = CPUFREQ_SELECTOR (g_object_new (CPUFREQ_TYPE_SELECTOR_LIBCPUFREQ,
						   "cpu", cpu, 
						   NULL));

        return selector;
}

static guint 
cpufreq_selector_libcpufreq_get_valid_frequency (CPUFreqSelectorLibcpufreq *selector,
						 guint                      frequency)
{
	guint                 cpu;
	gint                  dist = G_MAXINT;
	guint                 retval = 0;
	CPUFreqFrequencyList *freqs, *freq;

	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);

	freqs = cpufreq_get_available_frequencies (cpu);
	if (!freqs)
		return 0;

	for (freq = freqs; freq; freq = freq->next) {
		guint current_dist;
		
		if (freq->frequency == frequency) {
			cpufreq_put_available_frequencies (freqs);

			return frequency;
		}

		current_dist = abs (freq->frequency - frequency);
		if (current_dist < dist) {
			dist = current_dist;
			retval = freq->frequency;
		}
	}

	return retval;
}

static gboolean
cpufreq_selector_libcpufreq_set_frequency (CPUFreqSelector *selector,
					   guint            frequency,
					   GError         **error)
{
	guint freq;
	guint cpu;

	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);

	freq = cpufreq_selector_libcpufreq_get_valid_frequency (CPUFREQ_SELECTOR_LIBCPUFREQ (selector),
								frequency);
	if (cpufreq_set_frequency (cpu, freq) != 0) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_ERROR,
			     SELECTOR_ERROR_SET_FREQUENCY,
			     "Cannot set frequency '%d'",
			     frequency);
		
		return FALSE;
	}
	
	return TRUE;
}

static gboolean
cpufreq_selector_libcpufreq_validate_governor (CPUFreqSelectorLibcpufreq *selector,
					       const gchar               *governor)
{
	guint                cpu;
	CPUFreqGovernorList *govs, *gov;

	g_object_get (G_OBJECT (selector),
		      "cpu", &cpu,
		      NULL);

	govs = cpufreq_get_available_governors (cpu);
	if (!govs)
		return FALSE;

	for (gov = govs; gov; gov = gov->next) {
		if (g_ascii_strcasecmp (gov->governor, governor) == 0) {
			cpufreq_put_available_governors (govs);

			return TRUE;
		}
	}

	cpufreq_put_available_governors (govs);

	return FALSE;
}

static gboolean
cpufreq_selector_libcpufreq_set_governor (CPUFreqSelector *selector,
					  const gchar     *governor,
					  GError         **error)
{
	CPUFreqSelectorLibcpufreq *selector_libcpufreq;
	guint                      cpu;

	selector_libcpufreq = CPUFREQ_SELECTOR_LIBCPUFREQ (selector);

	if (!cpufreq_selector_libcpufreq_validate_governor (selector_libcpufreq, governor)) {
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

	if (cpufreq_modify_policy_governor (cpu, (gchar *)governor) != 0) {
		g_set_error (error,
			     CPUFREQ_SELECTOR_ERROR,
			     SELECTOR_ERROR_INVALID_GOVERNOR,
			     "Invalid governor '%s'",
			     governor);

		return FALSE;
	}

	return TRUE;
}
