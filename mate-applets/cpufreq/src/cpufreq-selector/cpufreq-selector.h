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

#ifndef __CPUFREQ_SELECTOR_H__
#define __CPUFREQ_SELECTOR_H__

#include <glib-object.h>

#define CPUFREQ_TYPE_SELECTOR            (cpufreq_selector_get_type ())
#define CPUFREQ_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_SELECTOR, CPUFreqSelector))
#define CPUFREQ_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_SELECTOR, CPUFreqSelectorClass))
#define CPUFREQ_IS_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_SELECTOR))
#define CPUFREQ_IS_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_SELECTOR))
#define CPUFREQ_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_SELECTOR, CPUFreqSelectorClass))

#define CPUFREQ_SELECTOR_ERROR (cpufreq_selector_error_quark ())

enum {
	SELECTOR_ERROR_INVALID_CPU,
	SELECTOR_ERROR_INVALID_GOVERNOR,
	SELECTOR_ERROR_SET_FREQUENCY
};

typedef struct _CPUFreqSelector        CPUFreqSelector;
typedef struct _CPUFreqSelectorClass   CPUFreqSelectorClass;
typedef struct _CPUFreqSelectorPrivate CPUFreqSelectorPrivate;

struct _CPUFreqSelector {
        GObject parent;

	CPUFreqSelectorPrivate *priv;
};

struct _CPUFreqSelectorClass {
        GObjectClass parent_class;

	gboolean  (* set_frequency) (CPUFreqSelector *selector,
				     guint            frequency,
				     GError         **error);
        gboolean  (* set_governor)  (CPUFreqSelector *selector,
				     const gchar     *governor,
				     GError         **error);
};


GType    cpufreq_selector_get_type      (void) G_GNUC_CONST;
GQuark   cpufreq_selector_error_quark   (void) G_GNUC_CONST;

gboolean cpufreq_selector_set_frequency (CPUFreqSelector *selector,
					 guint            frequency,
					 GError         **error);
gboolean cpufreq_selector_set_governor  (CPUFreqSelector *selector,
					 const gchar     *governor,
					 GError         **error);

#endif /* __CPUFREQ_SELECTOR_H__ */
