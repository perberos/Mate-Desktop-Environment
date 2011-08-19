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
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <check.h>

#include "gdm-address.h"
#include "s-common-address.h"

static GdmAddress             *ga;
static GdmAddress             *ga192;
static struct sockaddr         sa;
static struct sockaddr_in     *s_in;

#ifdef ENABLE_IPV6
static GdmAddress             *ga6;
static struct sockaddr_storage sa6;
static struct sockaddr_in6    *s_in6;
#endif

static void
setup (void)
{
        s_in = (struct sockaddr_in *) &sa;
        s_in->sin_family = AF_INET;
        s_in->sin_port = htons (25);
        s_in->sin_addr.s_addr = htonl (INADDR_LOOPBACK);

        ga = gdm_address_new_from_sockaddr (&sa, sizeof (sa));
        fail_unless (NULL != ga);

        s_in->sin_addr.s_addr = htonl (0xc0a80001); /* 192.168.0.1 */
        ga192 = gdm_address_new_from_sockaddr (&sa, sizeof (sa));
        fail_unless (NULL != (ga192));

#ifdef ENABLE_IPV6
        s_in6 = (struct sockaddr_in6 *) &sa6;
        s_in6->sin6_family = AF_INET6;
        s_in6->sin6_port = htons (25);
        s_in6->sin6_addr = in6addr_loopback;
        ga6 = gdm_address_new_from_sockaddr ((struct sockaddr*)&sa6, sizeof(sa6));
        fail_unless (NULL != ga6);
#endif
}

static void
teardown (void)
{
        gdm_address_free (ga);
        gdm_address_free (ga192);
#ifdef ENABLE_IPV6
        gdm_address_free (ga6);
#endif
}


/*
 * GType                    gdm_address_get_type                  (void);
 */
START_TEST (test_gdm_address_get_type)
{
        gdm_address_get_type ();
        /* it did not crash! :) */

}
END_TEST


START_TEST (test_gdm_address_new_from_sockaddr)
{

        GdmAddress *_ga;
#ifdef ENABLE_IPV6
        GdmAddress *_ga6;
#endif

        _ga = gdm_address_new_from_sockaddr ((struct sockaddr *) &sa, sizeof (sa));
        fail_unless (NULL != _ga);
        gdm_address_free (_ga);

#ifdef ENABLE_IPV6
        _ga6 = gdm_address_new_from_sockaddr((struct sockaddr *) &sa6, sizeof (sa6));
        fail_unless (NULL != _ga6);
        gdm_address_free (_ga6);
#endif

#ifndef NO_INVALID_INPUT
        /* invalid input */
        fail_unless (NULL == gdm_address_new_from_sockaddr ((struct sockaddr *) &sa, 1), NULL );
        fail_unless (NULL == gdm_address_new_from_sockaddr (NULL, 0), NULL);
#endif
}
END_TEST


START_TEST (test_gdm_address_get_family_type)
{
        fail_unless (AF_INET == gdm_address_get_family_type (ga), NULL);

#ifdef ENABLE_IPV6
        fail_unless (AF_INET6 == gdm_address_get_family_type (ga6), NULL);
#endif

#ifndef NO_INVALID_INPUT
        /* invalid input */
        fail_unless (-1 == gdm_address_get_family_type (NULL), NULL);
#endif

}
END_TEST


START_TEST (test_gdm_address_is_loopback)
{
        fail_unless (TRUE == gdm_address_is_loopback (ga));
        fail_unless (FALSE == gdm_address_is_loopback (ga192));

#ifdef ENABLE_IPV6
        fail_unless (TRUE == gdm_address_is_loopback (ga6));
        /* FIXME: add more addresses */
#endif

#ifndef NO_INVALID_INPUT
        /* invalid input */
        fail_unless (FALSE == gdm_address_is_loopback (NULL));
#endif
}
END_TEST


START_TEST (test_gdm_address_equal)
{
        GdmAddress         *gdm1;
        struct sockaddr     sa1;
        struct sockaddr_in *sin1;

        /* should be inequal */
        sin1 = (struct sockaddr_in *) &sa1;
        sin1->sin_family = AF_INET;
        sin1->sin_addr.s_addr = htonl (0xc0a80001); /* 192.168.0.1 */
        gdm1 = gdm_address_new_from_sockaddr (&sa1, sizeof (sa1));
        fail_unless (gdm_address_equal (ga, ga192) == FALSE, NULL);

        /* should be equal */
        fail_unless (TRUE == gdm_address_equal (ga192, gdm1), NULL);

        gdm_address_free (gdm1);

#ifdef ENABLE_IPV6
        /* should be inequal */
        fail_unless (FALSE == gdm_address_equal (ga6, ga), NULL);
        fail_unless (FALSE == gdm_address_equal (ga6, ga192), NULL);
        fail_unless (FALSE == gdm_address_equal (ga6, gdm1), NULL);

        /* should be equal */
        /* FIXME: ipv6 version too */
#endif

#ifndef NO_INVALID_INPUT
        /* invalid input */
        fail_unless (FALSE == gdm_address_equal (NULL, NULL), NULL);
        fail_unless (FALSE == gdm_address_equal (ga, NULL), NULL);
        fail_unless (FALSE == gdm_address_equal (NULL, ga), NULL);
#endif
}
END_TEST


Suite *
suite_common_address (void)
{
        Suite *s;
        TCase *tc_core;

        s = suite_create ("gdm-address");
        tc_core = tcase_create ("core");

        tcase_add_checked_fixture (tc_core, setup, teardown);
        tcase_add_test (tc_core, test_gdm_address_get_type);
        tcase_add_test (tc_core, test_gdm_address_new_from_sockaddr);
        tcase_add_test (tc_core, test_gdm_address_get_family_type);
        tcase_add_test (tc_core, test_gdm_address_is_loopback);
        tcase_add_test (tc_core, test_gdm_address_equal);
        suite_add_tcase (s, tc_core);

        return s;
}
