/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* test-module-selftest.c - Test program for the MATE Virtual File System.

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2000, 2001 Eazel, Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: 
	Mike Fleming <mfleming@eazel.com>
*/


/*
 * This program executes module self-test code
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-private-utils.h>
#include <gmodule.h>

#include <signal.h>
/* List of modules that have self-test code */
static const char *self_test_modules[] = {
	"http",
	NULL
};

static void
stop_after_log (const char *domain, GLogLevelFlags level, 
	const char *message, gpointer data)
{
	void (* saved_handler) (int);
	
	g_log_default_handler (domain, level, message, data);

	saved_handler = signal (SIGINT, SIG_IGN);
	raise (SIGINT);
	signal (SIGINT, saved_handler);
}

static void
make_asserts_break (const char *domain)
{
	g_log_set_handler
		(domain, 
		 (GLogLevelFlags) (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING),
		 stop_after_log, NULL);
}


typedef gboolean (*TestFunc)(void);

/* Note that, while this initialized mate-vfs, it does
 * not guarentee that mate-vfs actually loads and initializes
 * the modules in question
 */ 
int
main (int argc, char **argv)
{
	int i;
	GModule *module;
	TestFunc test_func;
	gboolean result;
	gboolean running_result;

	make_asserts_break ("GLib");
	make_asserts_break ("MateVFS");

	/* Initialize the libraries we use. */
	mate_vfs_init ();

	running_result = TRUE;
	for (i=0 ; self_test_modules[i] != NULL ; i++) {
		char *module_path;
		char *dummy_uri_string;
		MateVFSURI *uri;
	
		printf ("Module self-test: '%s'\n", self_test_modules[i]);
		module_path = g_module_build_path (MODULES_PATH, self_test_modules[i]);
		module = g_module_open (module_path, G_MODULE_BIND_LAZY);
		g_free (module_path);
		module_path = NULL;

		if (module == NULL) {
			fprintf (stderr, "Couldn't load module '%s'\n", self_test_modules[i]);
			continue;
		}

		g_module_symbol (module, "vfs_module_self_test", (gpointer *) &test_func);

		if (test_func == NULL) {
			fprintf (stderr, "Module had no self-test func '%s'\n", self_test_modules[i]);
			continue;
		}

		dummy_uri_string = g_strdup_printf ("%s:///", self_test_modules[i]);

		/* force normal initializing of the module by creating a URI
		 * for that scheme
		 */
		uri = mate_vfs_uri_new (dummy_uri_string);
		mate_vfs_uri_unref (uri);

		g_free (dummy_uri_string);
		
		result = test_func();

		fprintf (stderr, "%s: %s\n", self_test_modules[i], result ? "PASS" : "FAIL");

		running_result = running_result && result;
	}

	exit (running_result ? 0 : -1);
}

