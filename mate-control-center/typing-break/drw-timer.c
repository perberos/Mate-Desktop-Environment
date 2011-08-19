/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Nathaniel Smith <njs@pobox.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include "drw-timer.h"

struct _DrwTimer 
{
	GTimeVal start_time;
};

DrwTimer * drw_timer_new () 
{
	DrwTimer * timer = g_new0 (DrwTimer, 1);
	drw_timer_start (timer);
	return timer;
}

void drw_timer_start (DrwTimer *timer) 
{
	g_get_current_time (&timer->start_time);
}

double drw_timer_elapsed (DrwTimer *timer) 
{
	GTimeVal now;
	g_get_current_time (&now);
	return now.tv_sec - timer->start_time.tv_sec;
}

void drw_timer_destroy (DrwTimer *timer)
{
	g_free (timer);
}

