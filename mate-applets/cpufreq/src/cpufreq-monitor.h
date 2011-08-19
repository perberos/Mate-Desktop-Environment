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

#ifndef __CPUFREQ_MONITOR_H__
#define __CPUFREQ_MONITOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CPUFREQ_TYPE_MONITOR            (cpufreq_monitor_get_type ())
#define CPUFREQ_MONITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_MONITOR, CPUFreqMonitor))
#define CPUFREQ_MONITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_MONITOR, CPUFreqMonitorClass))
#define CPUFREQ_IS_MONITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_MONITOR))
#define CPUFREQ_IS_MONITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_MONITOR))
#define CPUFREQ_MONITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_MONITOR, CPUFreqMonitorClass))

typedef struct _CPUFreqMonitor        CPUFreqMonitor;
typedef struct _CPUFreqMonitorClass   CPUFreqMonitorClass;
typedef struct _CPUFreqMonitorPrivate CPUFreqMonitorPrivate;

struct _CPUFreqMonitor {
        GObject parent;

        CPUFreqMonitorPrivate *priv;
};

struct _CPUFreqMonitorClass {
        GObjectClass parent_class;

        gboolean  (* run)                       (CPUFreqMonitor *monitor);
        GList    *(* get_available_frequencies) (CPUFreqMonitor *monitor);
        GList    *(* get_available_governors)   (CPUFreqMonitor *monitor);

        /*< signals >*/
        void      (* changed)                   (CPUFreqMonitor *monitor);
};

GType        cpufreq_monitor_get_type                  (void) G_GNUC_CONST;

void         cpufreq_monitor_run                       (CPUFreqMonitor *monitor);
GList       *cpufreq_monitor_get_available_frequencies (CPUFreqMonitor *monitor);
GList       *cpufreq_monitor_get_available_governors   (CPUFreqMonitor *monitor);

guint        cpufreq_monitor_get_cpu                   (CPUFreqMonitor *monitor);
void         cpufreq_monitor_set_cpu                   (CPUFreqMonitor *monitor,
							guint           cpu);
const gchar *cpufreq_monitor_get_governor              (CPUFreqMonitor *monitor);
gint         cpufreq_monitor_get_frequency             (CPUFreqMonitor *monitor);
gint         cpufreq_monitor_get_percentage            (CPUFreqMonitor *monitor);

G_END_DECLS

#endif /* __CPUFREQ_MONITOR_H__ */
