/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtest-helpers.h: Declarations for common functions called from gtest unit tests

   Copyright (C) 2008 Stefan Walter

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

#ifndef GTEST_HELPERS_H_
#define GTEST_HELPERS_H_

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_mainloop_quit (void);
void test_mainloop_run (int timeout);
GMainLoop* test_mainloop_get (void);

const gchar* test_dir_testdata (void);
guchar* test_read_testdata (const gchar *basename, gsize *n_data);
gchar* test_build_filename (const gchar *basename);

#define DECLARE_SETUP(x) \
	void setup_##x(int *v, gconstpointer d)
#define DEFINE_SETUP(x) \
	void setup_##x(int *__unused G_GNUC_UNUSED, gconstpointer __data G_GNUC_UNUSED)

#define DECLARE_TEARDOWN(x) \
	void teardown_##x(int *v, gconstpointer d)
#define DEFINE_TEARDOWN(x) \
	void teardown_##x(int *__unused G_GNUC_UNUSED, gconstpointer __data G_GNUC_UNUSED)

#define DECLARE_TEST(x) \
	void test_##x(int *v, gconstpointer d)
#define DEFINE_TEST(x) \
	void test_##x(int *__unused G_GNUC_UNUSED, gconstpointer __data G_GNUC_UNUSED)

#define DECLARE_START(x) \
	void start_##x(void)
#define DEFINE_START(x) \
	void start_##x(void)

#define DECLARE_STOP(x) \
	void stop_##x(void)
#define DEFINE_STOP(x) \
	void stop_##x(void)

#ifndef g_assert_cmpsize
#define g_assert_cmpsize(a, o, b) \
	g_assert_cmpuint ((guint)(a), o, (guint)(b))
#endif

#endif /* GTEST_HELPERS_H_ */
