/*
 * Music Applet
 * Copyright (C) 2008 Paul Kuliniewicz <paul.kuliniewicz@gmail.com>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ma-constrain.h"


void
ma_constrain_applet_size_minimum (MatePanelApplet *applet, gint min_size)
{
	// 9999 as a substitute for "no maximum"
	const gint hints[] = { 9999, min_size };
	mate_panel_applet_set_size_hints (applet, hints, 2, 0);
}

void
ma_constrain_applet_size_clear (MatePanelApplet *applet)
{
	mate_panel_applet_set_size_hints (applet, NULL, 0, 0);
}
