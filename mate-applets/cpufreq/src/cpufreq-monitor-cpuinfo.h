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

#ifndef __CPUFREQ_MONITOR_CPUINFO_H__
#define __CPUFREQ_MONITOR_CPUINFO_H__

#include <glib-object.h>

#include "cpufreq-monitor.h"

#define CPUFREQ_TYPE_MONITOR_CPUINFO            (cpufreq_monitor_cpuinfo_get_type ())
#define CPUFREQ_MONITOR_CPUINFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_MONITOR_CPUINFO, CPUFreqMonitorCPUInfo))
#define CPUFREQ_MONITOR_CPUINFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_MONITOR_CPUINFO, CPUFreqMonitorCPUInfoClass))
#define CPUFREQ_IS_MONITOR_CPUINFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_MONITOR_CPUINFO))
#define CPUFREQ_IS_MONITOR_CPUINFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_MONITOR_CPUINFO))
#define CPUFREQ_MONITOR_CPUINFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_MONITOR_CPUINFO, CPUFreqMonitorCPUInfoClass))

typedef struct _CPUFreqMonitorCPUInfo      CPUFreqMonitorCPUInfo;
typedef struct _CPUFreqMonitorCPUInfoClass CPUFreqMonitorCPUInfoClass;

struct _CPUFreqMonitorCPUInfo {
        CPUFreqMonitor parent;
};

struct _CPUFreqMonitorCPUInfoClass {
        CPUFreqMonitorClass parent_class;
};

GType           cpufreq_monitor_cpuinfo_get_type (void) G_GNUC_CONST;
CPUFreqMonitor *cpufreq_monitor_cpuinfo_new      (guint cpu);

#endif /* __CPUFREQ_MONITOR_CPUINFO_H__ */
