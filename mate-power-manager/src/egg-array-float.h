/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
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

#ifndef __EGG_ARRAY_FLOAT_H
#define __EGG_ARRAY_FLOAT_H

#include <glib.h>

G_BEGIN_DECLS

/* at the moment just use a GArray as it's quick */
typedef GArray EggArrayFloat;

EggArrayFloat	*egg_array_float_new			(guint		 length);
void		 egg_array_float_free			(EggArrayFloat	*array);
gfloat		 egg_array_float_sum			(EggArrayFloat	*array);
EggArrayFloat	*egg_array_float_compute_gaussian	(guint		 length,
							 gfloat		 sigma);
gfloat		 egg_array_float_compute_integral	(EggArrayFloat	*array,
							 guint		 x1,
							 guint		 x2);
gfloat		 egg_array_float_get_average		(EggArrayFloat	*array);
gboolean	 egg_array_float_print			(EggArrayFloat	*array);
EggArrayFloat	*egg_array_float_convolve		(EggArrayFloat	*data,
							 EggArrayFloat	*kernel);
gfloat		 egg_array_float_get			(EggArrayFloat	*array,
							 guint		 i);
void		 egg_array_float_set			(EggArrayFloat	*array,
							 guint		 i,
							 gfloat		 value);
EggArrayFloat	*egg_array_float_remove_outliers	(EggArrayFloat *data, guint length, gfloat sigma);
#ifdef EGG_TEST
void		 egg_array_float_test			(gpointer	 data);
#endif

G_END_DECLS

#endif /* __EGG_ARRAY_FLOAT_H */
