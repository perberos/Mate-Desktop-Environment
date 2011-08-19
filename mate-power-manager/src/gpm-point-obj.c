/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <glib.h>

#include "egg-debug.h"
#include "gpm-point-obj.h"

/**
 * gpm_point_obj_copy:
 **/
GpmPointObj *
gpm_point_obj_copy (const GpmPointObj *cobj)
{
	GpmPointObj *obj;
	obj = g_new0 (GpmPointObj, 1);
	obj->x = cobj->x;
	obj->y = cobj->y;
	obj->color = cobj->color;
	return obj;
}

/**
 * gpm_point_obj_new:
 **/
GpmPointObj *
gpm_point_obj_new (void)
{
	GpmPointObj *obj;
	obj = g_new0 (GpmPointObj, 1);
	obj->x = 0.0f;
	obj->y = 0.0f;
	obj->color = 0x0;
	return obj;
}

/**
 * gpm_point_obj_free:
 **/
void
gpm_point_obj_free (GpmPointObj *obj)
{
	if (obj == NULL)
		return;
	g_free (obj);
}

