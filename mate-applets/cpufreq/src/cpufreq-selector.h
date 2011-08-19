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

#ifndef __CPUFREQ_SELECTOR_H__
#define __CPUFREQ_SELECTOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CPUFREQ_TYPE_SELECTOR            (cpufreq_selector_get_type ())
#define CPUFREQ_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_SELECTOR, CPUFreqSelector))
#define CPUFREQ_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_SELECTOR, CPUFreqSelectorClass))
#define CPUFREQ_IS_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_SELECTOR))
#define CPUFREQ_IS_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_SELECTOR))
#define CPUFREQ_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_SELECTOR, CPUFreqSelectorClass))

typedef struct _CPUFreqSelector        CPUFreqSelector;
typedef struct _CPUFreqSelectorClass   CPUFreqSelectorClass;

GType            cpufreq_selector_get_type            (void) G_GNUC_CONST;

CPUFreqSelector *cpufreq_selector_get_default         (void);
void             cpufreq_selector_set_frequency_async (CPUFreqSelector *selector,
						       guint            cpu,
						       guint            frequency,
						       guint32          parent);
void             cpufreq_selector_set_governor_async  (CPUFreqSelector *selector,
						       guint            cpu,
						       const gchar     *governor,
						       guint32          parent);
	
G_END_DECLS

#endif /* __CPUFREQ_SELECTOR_H__ */
