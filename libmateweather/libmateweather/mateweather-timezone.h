/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mateweather-timezone.c - Timezone handling
 *
 * Copyright 2008, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __MATEWEATHER_TIMEZONE_H__
#define __MATEWEATHER_TIMEZONE_H__

#ifndef MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#error "libmateweather should only be used if you understand that it's subject to change, and is not supported as a fixed API/ABI or as part of the platform"
#endif

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateWeatherTimezone MateWeatherTimezone;

GType mateweather_timezone_get_type (void);
#define MATEWEATHER_TYPE_TIMEZONE (mateweather_timezone_get_type ())

const char       *mateweather_timezone_get_name       (MateWeatherTimezone *zone);
const char       *mateweather_timezone_get_tzid       (MateWeatherTimezone *zone);
int               mateweather_timezone_get_offset     (MateWeatherTimezone *zone);
gboolean          mateweather_timezone_has_dst        (MateWeatherTimezone *zone);
int               mateweather_timezone_get_dst_offset (MateWeatherTimezone *zone);

MateWeatherTimezone *mateweather_timezone_ref            (MateWeatherTimezone *zone);
void              mateweather_timezone_unref          (MateWeatherTimezone *zone);

MateWeatherTimezone *mateweather_timezone_get_utc        (void);

#ifdef __cplusplus
}
#endif

#endif /* __MATEWEATHER_TIMEZONE_H__ */
