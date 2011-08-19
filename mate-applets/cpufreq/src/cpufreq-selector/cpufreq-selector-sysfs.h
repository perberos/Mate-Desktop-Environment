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

#ifndef __CPUFREQ_SELECTOR_SYSFS_H__
#define __CPUFREQ_SELECTOR_SYSFS_H__

#include <glib-object.h>

#include "cpufreq-selector.h"

G_BEGIN_DECLS

#define CPUFREQ_TYPE_SELECTOR_SYSFS            (cpufreq_selector_sysfs_get_type ())
#define CPUFREQ_SELECTOR_SYSFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_SELECTOR_SYSFS, CPUFreqSelectorSysfs))
#define CPUFREQ_SELECTOR_SYSFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_SELECTOR_SYSFS, CPUFreqSelectorSysfsClass))
#define CPUFREQ_IS_SELECTOR_SYSFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_SELECTOR_SYSFS))
#define CPUFREQ_IS_SELECTOR_SYSFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_SELECTOR_SYSFS))
#define CPUFREQ_SELECTOR_SYSFS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_SELECTOR_SYSFS, CPUFreqSelectorSysfsClass))

typedef struct _CPUFreqSelectorSysfs        CPUFreqSelectorSysfs;
typedef struct _CPUFreqSelectorSysfsClass   CPUFreqSelectorSysfsClass;
typedef struct _CPUFreqSelectorSysfsPrivate CPUFreqSelectorSysfsPrivate;

struct _CPUFreqSelectorSysfs {
        CPUFreqSelector parent;

	CPUFreqSelectorSysfsPrivate *priv;
};

struct _CPUFreqSelectorSysfsClass {
        CPUFreqSelectorClass parent_class;
};


GType            cpufreq_selector_sysfs_get_type (void) G_GNUC_CONST;
CPUFreqSelector *cpufreq_selector_sysfs_new      (guint cpu);

G_END_DECLS

#endif /* __CPUFREQ_SELECTOR_SYSFS_H__ */
