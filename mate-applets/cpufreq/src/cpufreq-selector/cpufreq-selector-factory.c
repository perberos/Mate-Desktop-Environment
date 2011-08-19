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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cpufreq-selector-factory.h"
#include "cpufreq-selector-sysfs.h"
#include "cpufreq-selector-procfs.h"
#ifdef HAVE_LIBCPUFREQ
#include "cpufreq-selector-libcpufreq.h"
#endif

CPUFreqSelector *
cpufreq_selector_factory_create_selector (guint cpu)
{
	CPUFreqSelector *selector = NULL;
	
#ifdef HAVE_LIBCPUFREQ
	selector = cpufreq_selector_libcpufreq_new (cpu);
#else
	if (g_file_test ("/sys/devices/system/cpu/cpu0/cpufreq", G_FILE_TEST_EXISTS)) { /* 2.6 kernel */
		selector = cpufreq_selector_sysfs_new (cpu);
	} else if (g_file_test ("/proc/cpufreq", G_FILE_TEST_EXISTS)) { /* 2.4 kernel */
		selector = cpufreq_selector_procfs_new (cpu);
	}
#endif /* HAVE_LIBCPUFREQ */

	return selector;
}
