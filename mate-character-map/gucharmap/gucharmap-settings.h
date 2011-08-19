/*
 * Copyright © 2005 Jason Allen
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA
 */

#ifndef GUCHARMAP_SETTINGS_H
#define GUCHARMAP_SETTINGS_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gucharmap/gucharmap.h>

G_BEGIN_DECLS

typedef enum
{
  GUCHARMAP_CHAPTERS_SCRIPT = 0,
  GUCHARMAP_CHAPTERS_BLOCK  = 1
} GucharmapChaptersMode;

void         gucharmap_settings_initialize           (void);
void         gucharmap_settings_shutdown             (void);
GucharmapChaptersMode gucharmap_settings_get_chapters_mode    (void);
void         gucharmap_settings_set_chapters_mode    (GucharmapChaptersMode mode);
gchar *      gucharmap_settings_get_font             (void);
void         gucharmap_settings_set_font             (gchar *font);
gunichar     gucharmap_settings_get_last_char        (void);
void         gucharmap_settings_set_last_char        (gunichar wc);
gboolean     gucharmap_settings_get_snap_pow2        (void);
void         gucharmap_settings_set_snap_pow2        (gboolean snap_pow2);
void         gucharmap_settings_add_window           (GtkWindow *window);

G_END_DECLS

#endif  /* #ifndef GUCHARMAP_SETTINGS_H */
