/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author:  Behdad Esfahbod, Red Hat, Inc.
 */
#ifndef __FONTCONFIG_MONITOR_H
#define __FONTCONFIG_MONITOR_H

#include <glib.h>

G_BEGIN_DECLS

void fontconfig_cache_init (void);
gboolean fontconfig_cache_update (void);

typedef struct _fontconfig_monitor_handle fontconfig_monitor_handle_t;

fontconfig_monitor_handle_t *
fontconfig_monitor_start (GFunc    notify_callback,
                          gpointer notify_data);
void fontconfig_monitor_stop  (fontconfig_monitor_handle_t *handle);

G_END_DECLS

#endif /* __FONTCONFIG_MONITOR_H */
