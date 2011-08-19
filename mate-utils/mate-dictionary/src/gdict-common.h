/* gdict-common.h - shared code between application and applet
 *
 * This file is part of MATE Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __GDICT_COMMON_H__
#define __GDICT_COMMON_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

G_BEGIN_DECLS

gboolean gdict_create_data_dir (void);
gchar *  gdict_get_data_dir    (void) G_GNUC_MALLOC;

void     gdict_show_error_dialog  (GtkWindow   *parent,
				   const gchar *message,
				   const gchar *detail);
void     gdict_show_gerror_dialog (GtkWindow   *parent,
				   const gchar *message,
				   GError      *error);

gchar *  gdict_mateconf_get_string_with_default (MateConfClient *client,
					      const gchar *key,
					      const gchar *def);


G_END_DECLS

#endif /* __GDICT_COMMON_H__ */
