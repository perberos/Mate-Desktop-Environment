/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __MATECONF_UTIL_H__
#define __MATECONF_UTIL_H__

#include <glib.h>
#include <glib-object.h>

#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#define MATECONF_TYPE_VALUE (mateconf_value_get_type ())

GType mateconf_value_get_type (void);
gchar *mateconf_get_key_name_from_path (const gchar *path);
gchar *mateconf_value_type_to_string (MateConfValueType value_type);
MateConfSchema *mateconf_client_get_schema_for_key (MateConfClient *client, const char *key);
gboolean mateconf_util_can_edit_defaults (void);
gboolean mateconf_util_can_edit_mandatory (void);

#endif /* __MATECONF_UTIL_H__ */
