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

#ifndef CPUFREQ_APPLET_H
#define CPUFREQ_APPLET_H

#include <glib-object.h>

G_BEGIN_DECLS

#define CPUFREQ_TYPE_APPLET            (cpufreq_applet_get_type ())
#define CPUFREQ_APPLET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_APPLET, CPUFreqApplet))
#define CPUFREQ_APPLET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_APPLET, CPUFreqAppletClass))
#define CPUFREQ_IS_APPLET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_APPLET))
#define CPUFREQ_IS_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_APPLET))
#define CPUFREQ_APPLET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_APPLET, CPUFreqAppletClass))

#define CPUFREQ_TYPE_SHOW_MODE      (cpufreq_applet_show_mode_get_type ())
#define CPUFREQ_TYPE_SHOW_TEXT_MODE (cpufreq_applet_show_text_mode_get_type ())

typedef struct _CPUFreqApplet      CPUFreqApplet;
typedef struct _CPUFreqAppletClass CPUFreqAppletClass;

typedef enum {
        CPUFREQ_MODE_GRAPHIC,
        CPUFREQ_MODE_TEXT,
        CPUFREQ_MODE_BOTH
} CPUFreqShowMode;

typedef enum {
        CPUFREQ_MODE_TEXT_FREQUENCY,
        CPUFREQ_MODE_TEXT_FREQUENCY_UNIT,
        CPUFREQ_MODE_TEXT_PERCENTAGE
} CPUFreqShowTextMode;

GType    cpufreq_applet_get_type                (void) G_GNUC_CONST;

GType    cpufreq_applet_show_mode_get_type      (void) G_GNUC_CONST;
GType    cpufreq_applet_show_text_mode_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* CPUFREQ_APPLET_H */
