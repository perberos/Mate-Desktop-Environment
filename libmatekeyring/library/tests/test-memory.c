/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* unit-test-memory.c: Test memory allocation functionality

   Copyright (C) 2007 Stefan Walter

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "run-auto-test.h"

#include "library/mate-keyring-memory.h"

#define IS_ZERO -1

static int
find_non_zero (gpointer mem, gsize len)
{
	guchar *b, *e;
	gsize sz = 0;
	for (b = (guchar*)mem, e = ((guchar*)mem) + len; b != e; ++b, ++sz) {
		if (*b != 0x00)
			return (int)sz;
	}

	return -1;
}

DEFINE_TEST(alloc_free)
{
	gpointer p;
	gboolean ret;

	p = mate_keyring_memory_alloc (512);
	g_assert (p != NULL);
	g_assert_cmpint (IS_ZERO, ==, find_non_zero (p, 512));

	memset (p, 0x67, 512);

	ret = mate_keyring_memory_is_secure (p);
	g_assert (ret == TRUE);

	mate_keyring_memory_free (p);
}

DEFINE_TEST(alloc_two)
{
	gpointer p, p2;
	gboolean ret;

	p2 = mate_keyring_memory_alloc (4);
	g_assert(p2 != NULL);
	g_assert_cmpint (IS_ZERO, ==, find_non_zero (p2, 4));

	memset (p2, 0x67, 4);

	p = mate_keyring_memory_alloc (16200);
	g_assert (p != NULL);
	g_assert_cmpint (IS_ZERO, ==, find_non_zero (p, 16200));

	memset (p, 0x67, 16200);

	ret = mate_keyring_memory_is_secure (p);
	g_assert (ret == TRUE);

	mate_keyring_memory_free (p2);
	mate_keyring_memory_free (p);
}

DEFINE_TEST(realloc)
{
	gchar *str = "a test string to see if realloc works properly";
	gpointer p, p2;
	gsize len;

	len = strlen (str) + 1;

	p = mate_keyring_memory_realloc (NULL, len);
	g_assert (p != NULL);
	g_assert_cmpint (IS_ZERO, ==, find_non_zero (p, len));

	strcpy ((gchar*)p, str);

	p2 = mate_keyring_memory_realloc (p, 512);
	g_assert (p2 != NULL);

	/* "strings not equal after realloc" */
	g_assert_cmpstr (p2, ==, str);

	p = mate_keyring_memory_realloc (p2, 0);
	/* "should have freed memory" */
	g_assert (p == NULL);
}

DEFINE_TEST(realloc_across)
{
	gpointer p, p2;

	/* Tiny allocation */
	p = mate_keyring_memory_realloc (NULL, 1088);
	g_assert (p != NULL);
	g_assert_cmpint (IS_ZERO, ==, find_non_zero (p, 1088));

	/* Reallocate to a large one, will have to have changed blocks */
	p2 = mate_keyring_memory_realloc (p, 16200);
	g_assert (p2 != NULL);
	g_assert_cmpint (IS_ZERO, ==, find_non_zero (p2, 16200));

	mate_keyring_memory_free (p2);
}
