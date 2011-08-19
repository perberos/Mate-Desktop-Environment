/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Timezone map.
 *
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 * 
 * Largely based on Michael Fulbright's work on Anaconda.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef _E_TZ_MAP_H
#define _E_TZ_MAP_H

#include "gst-tool.h"

#define TZ_MAP_POINT_NORMAL_RGBA 0xc070a0ff
#define TZ_MAP_POINT_HOVER_RGBA 0xffff60ff
#define TZ_MAP_POINT_SELECTED_1_RGBA 0xff60e0ff
#define TZ_MAP_POINT_SELECTED_2_RGBA 0x000000ff

typedef struct _ETzMap ETzMap;

struct _ETzMap
{
	EMap *map;
	TzDB *tzdb;
	
	EMapPoint *point_selected,
	          *point_hover;

	guint timeout_id;
	GstTool *tool; /* This is not the way to do it, will fix later */
};

/* --- Fake widget --- */

ETzMap     *e_tz_map_new                  (GstTool *tool);
TzDB       *e_tz_map_get_tz_db            (ETzMap *tzmap);
void        e_tz_map_set_tz_from_name     (ETzMap *tzmap, const gchar *name);
gchar      *e_tz_map_get_selected_tz_name (ETzMap *tzmap);
TzLocation *e_tz_map_get_location_by_name (ETzMap *tzmap, const gchar *name);

#endif
