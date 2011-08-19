/*
 * MATE CPUFreq Applet
 * Copyright (C) 2006 Carlos Garcia Campos <carlosgc@mate.org>
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

#ifndef CPUFREQ_UTILS_H
#define CPUFREQ_UTILS_H

#include <glib.h>

G_BEGIN_DECLS

/* Useful global methods */
guint    cpufreq_utils_get_n_cpus            (void);
void     cpufreq_utils_display_error         (const gchar *message,
					      const gchar *secondary);
gboolean cpufreq_utils_selector_is_available (void);
gchar   *cpufreq_utils_get_frequency_label   (guint        freq);
gchar   *cpufreq_utils_get_frequency_unit    (guint        freq);
gboolean cpufreq_utils_governor_is_automatic (const gchar *governor);
gboolean cpufreq_file_get_contents           (const gchar *filename,
					      gchar      **contents,
					      gsize       *length,
					      GError     **error);

G_END_DECLS

#endif /* CPUFREQ_UTILS_H */
