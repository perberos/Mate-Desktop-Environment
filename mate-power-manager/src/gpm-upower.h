/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __GPM_DEVICEKIT_H
#define __GPM_DEVICEKIT_H

#include <glib-object.h>
#include <libupower-glib/upower.h>

G_BEGIN_DECLS

const gchar	*gpm_device_kind_to_localised_text	(UpDeviceKind	 kind,
							 guint		 number);
const gchar	*gpm_device_kind_to_icon		(UpDeviceKind	 kind);
const gchar	*gpm_device_technology_to_localised_string (UpDeviceTechnology technology_enum);
const gchar	*gpm_device_state_to_localised_string	(UpDeviceState	 state);
gchar		*gpm_upower_get_device_icon		(UpDevice *device);
gchar		*gpm_upower_get_device_summary		(UpDevice *device);
gchar		*gpm_upower_get_device_description	(UpDevice *device);

G_END_DECLS

#endif	/* __GPM_DEVICEKIT_H */
