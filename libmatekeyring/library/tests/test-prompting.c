/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* unit-test-other.c: Test some prompts

   Copyright (C) 2010 Stefan Walter

   The Mate Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Stef Walter <stef@memberwebs.com>
*/

#include "config.h"
#include "mate-keyring.h"

#include <glib.h>

static int
test_create_item (void)
{
	MateKeyringResult res;
	guint32 item_id;

	res = mate_keyring_item_create_sync (NULL, MATE_KEYRING_ITEM_GENERIC_SECRET,
	                                      "Test Create Prompt Item", NULL, "secret", FALSE, &item_id);

	if (res == MATE_KEYRING_RESULT_CANCELLED)
		g_printerr ("creating item cancelled\n");
	else if (res != MATE_KEYRING_RESULT_OK)
		g_printerr ("creating item failed: %s", mate_keyring_result_to_message (res));
	else
		g_print ("item id: %u", (guint)item_id);

	return 0;
}

int
main (int argc, char *argv[])
{
	if (argc == 2) {
		if (g_str_equal (argv[1], "--create-item")) {
			return test_create_item ();
		}
	}

	g_printerr ("usage: test-prompting --create-item\n");
	return 2;
}
