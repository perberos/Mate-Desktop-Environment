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

#ifndef CPUFREQ_PREFS_H
#define CPUFREQ_PREFS_H

#include <gdk/gdk.h>
#include <glib-object.h>
#include "cpufreq-applet.h"

G_BEGIN_DECLS

#define CPUFREQ_TYPE_PREFS            (cpufreq_prefs_get_type ())
#define CPUFREQ_PREFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_PREFS, CPUFreqPrefs))
#define CPUFREQ_PREFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_PREFS, CPUFreqPrefsClass))
#define CPUFREQ_IS_PREFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_PREFS))
#define CPUFREQ_IS_PREFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_PREFS))
#define CPUFREQ_PREFS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_PREFS, CPUFreqPrefsClass))

typedef struct _CPUFreqPrefs        CPUFreqPrefs;
typedef struct _CPUFreqPrefsClass   CPUFreqPrefsClass;
typedef struct _CPUFreqPrefsPrivate CPUFreqPrefsPrivate;

struct _CPUFreqPrefs {
	GObject              base;

	CPUFreqPrefsPrivate *priv;
};

struct _CPUFreqPrefsClass {
	GObjectClass         parent_class;
};

GType               cpufreq_prefs_get_type           (void) G_GNUC_CONST;

CPUFreqPrefs       *cpufreq_prefs_new                (const gchar  *mateconf_key);

guint               cpufreq_prefs_get_cpu            (CPUFreqPrefs *prefs);
CPUFreqShowMode     cpufreq_prefs_get_show_mode      (CPUFreqPrefs *prefs);
CPUFreqShowTextMode cpufreq_prefs_get_show_text_mode (CPUFreqPrefs *prefs);

/* Properties dialog */
void                cpufreq_preferences_dialog_run   (CPUFreqPrefs *prefs,
						      GdkScreen    *screen);

G_END_DECLS

#endif /* CPUFREQ_PREFS_H */
