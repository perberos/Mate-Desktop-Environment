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

#ifndef __CPUFREQ_MONITOR_LIBCPUFREQ_H__
#define __CPUFREQ_MONITOR_LIBCPUFREQ_H__

#include <glib-object.h>

#include "cpufreq-monitor.h"

#define CPUFREQ_TYPE_MONITOR_LIBCPUFREQ            \
	(cpufreq_monitor_libcpufreq_get_type ())
#define CPUFREQ_MONITOR_LIBCPUFREQ(obj)            \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_MONITOR_LIBCPUFREQ, CPUFreqMonitorLibcpufreq))
#define CPUFREQ_MONITOR_LIBCPUFREQ_CLASS(klass)    \
	(G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_MONITOR_LIBCPUFREQ, CPUFreqMonitorLibcpufreqClass))
#define CPUFREQ_IS_MONITOR_LIBCPUFREQ(obj)         \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_MONITOR_LIBCPUFREQ))
#define CPUFREQ_IS_MONITOR_LIBCPUFREQ_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_MONITOR_LIBCPUFREQ))
#define CPUFREQ_MONITOR_LIBCPUFREQ_GET_CLASS(obj)  \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_MONITOR_LIBCPUFREQ, CPUFreqMonitorLibcpufreqClass))

typedef struct _CPUFreqMonitorLibcpufreq      CPUFreqMonitorLibcpufreq;
typedef struct _CPUFreqMonitorLibcpufreqClass CPUFreqMonitorLibcpufreqClass;

struct _CPUFreqMonitorLibcpufreq {
        CPUFreqMonitor parent;
};

struct _CPUFreqMonitorLibcpufreqClass {
        CPUFreqMonitorClass parent_class;
};

GType           cpufreq_monitor_libcpufreq_get_type (void) G_GNUC_CONST;
CPUFreqMonitor *cpufreq_monitor_libcpufreq_new      (guint cpu);

#endif /* __CPUFREQ_MONITOR_LIBCPUFREQ_H__ */
