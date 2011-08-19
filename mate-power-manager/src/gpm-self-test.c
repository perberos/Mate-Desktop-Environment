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

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "egg-test.h"
#include "egg-debug.h"

#include "gpm-screensaver.h"

/* prototypes */
void egg_precision_test (EggTest *test);
void egg_discrete_test (EggTest *test);
void egg_color_test (EggTest *test);
void egg_array_float_test (EggTest *test);
void egg_idletime_test (EggTest *test);

void gpm_common_test (EggTest *test);
void gpm_idle_test (EggTest *test);
void gpm_phone_test (EggTest *test);
void gpm_dpms_test (EggTest *test);
void gpm_graph_widget_test (EggTest *test);
void gpm_proxy_test (EggTest *test);
void gpm_hal_manager_test (EggTest *test);
void gpm_device_test (EggTest *test);
void gpm_device_teststore (EggTest *test);

int
main (int argc, char **argv)
{
	EggTest *test;

	g_type_init ();
	test = egg_test_init ();
	egg_debug_init (TRUE);

	/* needed for DPMS checks */
	gtk_init (&argc, &argv);

	/* tests go here */
	egg_precision_test (test);
	egg_discrete_test (test);
	egg_color_test (test);
	egg_array_float_test (test);
//	egg_idletime_test (test);

	gpm_common_test (test);
	gpm_idle_test (test);
	gpm_phone_test (test);
//	gpm_dpms_test (test);
//	gpm_graph_widget_test (test);
//	gpm_screensaver_test (test);

#if 0
	gpm_proxy_test (test);
	gpm_hal_manager_test (test);
	gpm_device_test (test);
	gpm_device_teststore (test);
#endif

	return (egg_test_finish (test));
}

