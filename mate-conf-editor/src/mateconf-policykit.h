/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Vincent Untz <vuntz@mate.org>
 *
 * Based on set-timezone.h from mate-panel which was GPLv2+ with this
 * copyright:
 *    Copyright (C) 2007 David Zeuthen <david@fubar.dk>
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
 */

#ifndef  __MATECONF_POLICYKIT_H__
#define  __MATECONF_POLICYKIT_H__

#include <glib.h>

gint     mateconf_pk_can_set_default    (const gchar *key);

gint     mateconf_pk_can_set_mandatory  (const gchar *key);

void     mateconf_pk_set_default_async  (const gchar    *key,
                                      GFunc           callback,
                                      gpointer        data,
                                      GDestroyNotify  notify);

void     mateconf_pk_set_mandatory_async (const gchar    *key,
                                       GFunc           callback,
                                       gpointer        data,
                                       GDestroyNotify  notify);

#endif /* __MATECONF_POLICYKIT_H__ */
