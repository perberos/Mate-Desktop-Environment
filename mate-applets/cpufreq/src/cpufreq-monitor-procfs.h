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

#ifndef __CPUFREQ_MONITOR_PROCFS_H__
#define __CPUFREQ_MONITOR_PROCFS_H__

#include <glib-object.h>

#include "cpufreq-monitor.h"

G_BEGIN_DECLS

#define TYPE_CPUFREQ_MONITOR_PROCFS            (cpufreq_monitor_procfs_get_type ())
#define CPUFREQ_MONITOR_PROCFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CPUFREQ_MONITOR_PROCFS, CPUFreqMonitorProcfs))
#define CPUFREQ_MONITOR_PROCFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_CPUFREQ_MONITOR_PROCFS, CPUFreqMonitorProcfsClass))
#define IS_CPUFREQ_MONITOR_PROCFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CPUFREQ_MONITOR_PROCFS))
#define IS_CPUFREQ_MONITOR_PROCFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CPUFREQ_MONITOR_PROCFS))
#define CPUFREQ_MONITOR_PROCFS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CPUFREQ_MONITOR_PROCFS, CPUFreqMonitorProcfsClass))

typedef struct _CPUFreqMonitorProcfs      CPUFreqMonitorProcfs;
typedef struct _CPUFreqMonitorProcfsClass CPUFreqMonitorProcfsClass;

struct _CPUFreqMonitorProcfs {
	   CPUFreqMonitor parent;
};

struct _CPUFreqMonitorProcfsClass {
	   CPUFreqMonitorClass parent_class;
};

GType           cpufreq_monitor_procfs_get_type (void) G_GNUC_CONST;
CPUFreqMonitor *cpufreq_monitor_procfs_new      (guint cpu);

G_END_DECLS

#endif /* __CPUFREQ_MONITOR_PROCFS_H__ */
