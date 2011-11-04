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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DRW_TIMER_H__
#define __DRW_TIMER_H__

/*
 * This file defines a timer interface similar to GTimer, but defined in real
 * wall-clock time. A GTimer may stop counting while the computer is suspended
 * or the process is stopped:
 *   https://bugzilla.gnome.org/show_bug.cgi?id=552994
 * but a DrwTimer keeps counting regardless.
 *
 * Currently this only provides second resolution as compared to GTimer's
 * microsecond resolution, but a typing break program doesn't really need
 * microsecond resolution anyway.
 */

typedef struct _DrwTimer DrwTimer;
DrwTimer * drw_timer_new ();
void drw_timer_start (DrwTimer *timer);
double drw_timer_elapsed (DrwTimer *timer);
void drw_timer_destroy (DrwTimer *timer);

#endif /* __DRW_TIMER_H__ */
