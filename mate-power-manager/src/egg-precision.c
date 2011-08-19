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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "egg-debug.h"
#include "egg-precision.h"

/**
 * egg_precision_round_up:
 * @value: The input value
 * @smallest: The smallest increment allowed
 *
 * 101, 10	110
 * 95,  10	100
 * 0,   10	0
 * 112, 10	120
 * 100, 10	100
 **/
gint
egg_precision_round_up (gfloat value, gint smallest)
{
	gfloat division;
	if (fabs (value) < 0.01)
		return 0;
	if (smallest == 0) {
		egg_warning ("divisor zero");
		return 0;
	}
	division = (gfloat) value / (gfloat) smallest;
	division = ceilf (division);
	division *= smallest;
	return (gint) division;
}

/**
 * egg_precision_round_down:
 * @value: The input value
 * @smallest: The smallest increment allowed
 *
 * 101, 10	100
 * 95,  10	90
 * 0,   10	0
 * 112, 10	110
 * 100, 10	100
 **/
gint
egg_precision_round_down (gfloat value, gint smallest)
{
	gfloat division;
	if (fabs (value) < 0.01)
		return 0;
	if (smallest == 0) {
		egg_warning ("divisor zero");
		return 0;
	}
	division = (gfloat) value / (gfloat) smallest;
	division = floorf (division);
	division *= smallest;
	return (gint) division;
}

/***************************************************************************
 ***                          MAKE CHECK TESTS                           ***
 ***************************************************************************/
#ifdef EGG_TEST
#include "egg-test.h"

void
egg_precision_test (gpointer data)
{
	guint value;
	EggTest *test = (EggTest *) data;

	if (!egg_test_start (test, "EggPrecision"))
		return;

	/************************************************************/
	egg_test_title (test, "limit precision down 0,10");
	value = egg_precision_round_down (0, 10);
	if (value == 0) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision down 4,10");
	value = egg_precision_round_down (4, 10);
	if (value == 0) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision down 11,10");
	value = egg_precision_round_down (11, 10);
	if (value == 10) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision down 201,2");
	value = egg_precision_round_down (201, 2);
	if (value == 200) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision down 100,10");
	value = egg_precision_round_down (100, 10);
	if (value == 100) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision up 0,10");
	value = egg_precision_round_up (0, 10);
	if (value == 0) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision up 4,10");
	value = egg_precision_round_up (4, 10);
	if (value == 10) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision up 11,10");
	value = egg_precision_round_up (11, 10);
	if (value == 20) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision up 201,2");
	value = egg_precision_round_up (201, 2);
	if (value == 202) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	/************************************************************/
	egg_test_title (test, "limit precision up 100,10");
	value = egg_precision_round_up (100, 10);
	if (value == 100) {
		egg_test_success (test, "got %i", value);
	} else {
		egg_test_failed (test, "precision incorrect (%i)", value);
	}

	egg_test_end (test);
}

#endif

