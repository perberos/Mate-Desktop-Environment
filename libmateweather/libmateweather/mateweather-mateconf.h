/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * mateweather-mateconf.h: MateConf interaction methods for mateweather.
 *
 * Copyright (C) 2005 Philip Langdale, Papadimitriou Spiros
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Philip Langdale <philipl@mail.utexas.edu>
 *     Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 */

#ifndef __MATEWEATHER_MATECONF_WRAPPER_H__
#define __MATEWEATHER_MATECONF_WRAPPER_H__


#ifndef MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#error "libmateweather should only be used if you understand that it's subject to change, and is not supported as a fixed API/ABI or as part of the platform"
#endif


#include <glib.h>
#include <mateconf/mateconf-client.h>
#include <mateconf/mateconf-value.h>

#include <libmateweather/weather.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct		_MateWeatherMateConf			MateWeatherMateConf;

MateWeatherMateConf *		mateweather_mateconf_new		(const char *prefix);
void			mateweather_mateconf_free		(MateWeatherMateConf *ctx);

MateConfClient *		mateweather_mateconf_get_client	(MateWeatherMateConf *ctx);

WeatherLocation *	mateweather_mateconf_get_location	(MateWeatherMateConf *ctx);

gchar *			mateweather_mateconf_get_full_key	(MateWeatherMateConf *ctx,
							 const gchar *key);

void			mateweather_mateconf_set_bool		(MateWeatherMateConf *ctx,
							 const gchar *key,
							 gboolean the_bool,
							 GError **opt_error);
void			mateweather_mateconf_set_int		(MateWeatherMateConf *ctx,
							 const gchar *key,
							 gint the_int,
							 GError **opt_error);
void			mateweather_mateconf_set_string	(MateWeatherMateConf *ctx,
							 const gchar *key,
							 const gchar *the_string,
							 GError **opt_error);

gboolean		mateweather_mateconf_get_bool		(MateWeatherMateConf *ctx,
							 const gchar *key,
							 GError **opt_error);
gint			mateweather_mateconf_get_int		(MateWeatherMateConf *ctx,
							 const gchar *key,
							 GError **opt_error);
gchar *			mateweather_mateconf_get_string	(MateWeatherMateConf *ctx,
							 const gchar *key,
							 GError **opt_error);

#ifdef __cplusplus
}
#endif

#endif /* __MATEWEATHER_MATECONF_WRAPPER_H__ */
