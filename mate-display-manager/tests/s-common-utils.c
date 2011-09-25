/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Andrew Ziem <ahz001@gmail.com>
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "s-common-utils.h"
#include "mdm-common.h"

START_TEST (test_mdm_string_hex_encode)
{
        GString *a;
        GString *b;

        a = g_string_new ("foo");
        b = g_string_sized_new (100);
        fail_unless (TRUE == mdm_string_hex_encode (a, 0, b, 0), NULL);
        fail_unless (0 == strncmp (b->str, "666f6f", 7), NULL);

#ifndef NO_INVALID_INPUT
        /* invalid input */
        fail_unless (FALSE == mdm_string_hex_encode (a, -1, b, -1), NULL);
        fail_unless (FALSE == mdm_string_hex_encode (NULL, 0, NULL, 0), NULL);
        fail_unless (FALSE == mdm_string_hex_encode (a, 0, a, 0), NULL);
#endif

        g_string_free (a, TRUE);
        g_string_free (b, TRUE);
}
END_TEST


START_TEST (test_mdm_string_hex_decode)
        GString *a;
        GString *b;

        a = g_string_new ("666f6f");
        b = g_string_sized_new (100);

        fail_unless (TRUE == mdm_string_hex_decode (a, 0, NULL, b, 0), NULL);

        fail_unless (0 == strncmp (b->str, "foo", 7), NULL);

#ifndef NO_INVALID_INPUT
        /* invalid input */
        fail_unless (FALSE == mdm_string_hex_decode (a, -1, NULL, b, -1), NULL);
        fail_unless (FALSE == mdm_string_hex_decode (NULL, 0, NULL, NULL, 0), NULL);
        fail_unless (FALSE == mdm_string_hex_decode (a, 0, NULL, a, 0), NULL);
#endif

        g_string_free (a, TRUE);
        g_string_free (b, TRUE);
END_TEST

Suite *
suite_common_utils (void)
{
        Suite *s;
        TCase *tc_core;

        s = suite_create ("mdm-common");
        tc_core = tcase_create ("core");

        tcase_add_test (tc_core, test_mdm_string_hex_encode);
        tcase_add_test (tc_core, test_mdm_string_hex_decode);

        suite_add_tcase (s, tc_core);

        return s;
}
