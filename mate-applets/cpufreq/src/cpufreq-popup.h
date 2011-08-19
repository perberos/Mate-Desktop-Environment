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

#ifndef CPUFREQ_POPUP_H
#define CPUFREQ_POPUP_H

#include <glib-object.h>

#include "cpufreq-monitor.h"
#include "cpufreq-prefs.h"

G_BEGIN_DECLS

#define CPUFREQ_TYPE_POPUP            (cpufreq_popup_get_type ())
#define CPUFREQ_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_POPUP, CPUFreqPopup))
#define CPUFREQ_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_POPUP, CPUFreqPopupClass))
#define CPUFREQ_IS_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_POPUP))
#define CPUFREQ_IS_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_POPUP))
#define CPUFREQ_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_POPUP, CPUFreqPopupClass))

typedef struct _CPUFreqPopup        CPUFreqPopup;
typedef struct _CPUFreqPopupClass   CPUFreqPopupClass;
typedef struct _CPUFreqPopupPrivate CPUFreqPopupPrivate;

struct _CPUFreqPopup {
	GObject              base;

	CPUFreqPopupPrivate *priv;
};

struct _CPUFreqPopupClass {
	GObjectClass         parent_class;
};

GType         cpufreq_popup_get_type        (void) G_GNUC_CONST;
CPUFreqPopup *cpufreq_popup_new             (void);

void          cpufreq_popup_set_preferences (CPUFreqPopup   *popup,
					     CPUFreqPrefs   *prefs);
void          cpufreq_popup_set_monitor     (CPUFreqPopup   *popup,
					     CPUFreqMonitor *monitor);
void          cpufreq_popup_set_parent      (CPUFreqPopup   *popup,
					     GtkWidget      *parent);
GtkWidget    *cpufreq_popup_get_menu        (CPUFreqPopup   *popup);

G_END_DECLS

#endif /* CPUFREQ_POPUP_H */
