/*
 * Copyright (C) 2005 by Ryan Lortie <desrt@desrt.ca>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#ifndef _battstat_hal_h_
#define _battstat_hal_h_

char *battstat_hal_initialise (void (*) (void));
void battstat_hal_cleanup (void);

#include "battstat.h"
void battstat_hal_get_battery_info (BatteryStatus *status);

#endif /* _battstat_hal_h_ */
