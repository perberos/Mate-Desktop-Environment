/*
 * Music Applet
 * Copyright (C) 2007 Paul Kuliniewicz <paul.kuliniewicz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include <config.h>

#include "ma-deserialize.h"

#include <gdk-pixbuf/gdk-pixdata.h>


GdkPixbuf *
ma_deserialize_pixdata (const guint8 *bytes, gint size)
{
	GdkPixdata pixdata;

	if (gdk_pixdata_deserialize (&pixdata, size, bytes, NULL))
	{
		return gdk_pixbuf_from_pixdata (&pixdata, TRUE, NULL);
	}
	else
	{
		return NULL;
	}
}
