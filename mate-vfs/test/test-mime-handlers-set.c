/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-mime.c - Test for the mime handler detection features of the MATE
   Virtual File System Library

   Copyright (C) 2000 Eazel

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

   Author: Maciej Stachowiak <mjs@eazel.com>
*/

#include <config.h>

#include <libmatevfs/mate-vfs-application-registry.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-mime-handlers.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void
usage (const char *name)
{
		fprintf (stderr, "Usage: %s mime_type field value\n", name);
		fprintf (stderr, "Valid field values are: \n");
		fprintf (stderr, "\tdefault_action_type\n");
		fprintf (stderr, "\tdefault_application\n");
		fprintf (stderr, "\tdefault_component\n");
		fprintf (stderr, "\tshort_list_applicationss\n");
		fprintf (stderr, "\tshort_list_components\n");
		fprintf (stderr, "\tadd_to_all_applicationss\n");
		fprintf (stderr, "\tremove_from_all_applications\n");
		fprintf (stderr, "\tdefine_application\n");
		exit (1);
}

static MateVFSMimeActionType
str_to_action_type (const char *str)
{
	if (g_ascii_strcasecmp (str, "component") == 0) {
		return MATE_VFS_MIME_ACTION_TYPE_COMPONENT;
	} else if (g_ascii_strcasecmp (str, "application") == 0) {
		return MATE_VFS_MIME_ACTION_TYPE_APPLICATION;
	} else {
		return MATE_VFS_MIME_ACTION_TYPE_NONE;
	}
}

static char **
strsplit_handle_null (const char *str, const char *delim, int max)
{
	return g_strsplit ((str == NULL ? "" : str), delim, max);
}


static GList *mate_vfs_strsplit_to_list (const char *str, const char *delim, int max)
{
	char **strv;
	GList *retval;
	int i;

	strv = strsplit_handle_null (str, delim, max);

	retval = NULL;

	for (i = 0; strv[i] != NULL; i++) {
		retval = g_list_prepend (retval, strv[i]);
	}

	retval = g_list_reverse (retval);
	/* Don't strfreev, since we didn't copy the individual strings. */
	g_free (strv);

	return retval;
}

static GList *comma_separated_str_to_str_list (const char *str)
{
	return mate_vfs_strsplit_to_list (str, ",", 0);
}


static gboolean
str_to_bool (const char *str)
{
	return ((str != NULL) &&
		((g_ascii_strcasecmp (str, "true") == 0) || 
		 (g_ascii_strcasecmp (str, "yes") == 0)));
}


int
main (int argc, char **argv)
{
        const char *type;  
	const char *field;
	const char *value;

	mate_vfs_init ();

	if (argc < 3) {
		usage (argv[0]);
	}

	type = argv[1];
	field = argv[2];
 	value = argv[3];

	if (strcmp (field, "default_action_type") == 0) {
		puts ("default_action_type");
		mate_vfs_mime_set_default_action_type (type, str_to_action_type (value));
	} else if (strcmp (field, "default_application") == 0) {
		puts ("default_application");
		mate_vfs_mime_set_default_application (type, value);
	} else if (strcmp (field, "default_component") == 0) {
		puts ("default_component");
		mate_vfs_mime_set_default_component (type, value);
	} else if (strcmp (field, "short_list_applicationss") == 0) {
		puts ("short_list_applications");
		mate_vfs_mime_set_short_list_applications (type, 
							    comma_separated_str_to_str_list (value));
	} else if (strcmp (field, "short_list_components") == 0) {
		puts ("short_list_components");
		mate_vfs_mime_set_short_list_components (type, 
							   comma_separated_str_to_str_list (value));
	} else if (strcmp (field, "add_to_all_applicationss") == 0) {
		puts ("add_to_all_applications");
		mate_vfs_mime_extend_all_applications (type, 
							comma_separated_str_to_str_list (value));
	} else if (strcmp (field, "remove_from_all_applications") == 0) {
		puts ("remove_from_all_applications");
		mate_vfs_mime_remove_from_all_applications (type, 
							     comma_separated_str_to_str_list (value));

	} else if (strcmp (field, "define_application") == 0) {
		GList *stuff;
		MateVFSMimeApplication app;

		stuff = comma_separated_str_to_str_list (value);

		app.id = g_list_nth (stuff, 0)->data;
		app.name = g_list_nth (stuff, 1)->data;
		app.command = g_list_nth (stuff, 2)->data;
		app.can_open_multiple_files = str_to_bool (g_list_nth (stuff, 3)->data);
		app.expects_uris = str_to_bool (g_list_nth (stuff, 4)->data);
		app.requires_terminal = str_to_bool (g_list_nth (stuff, 5)->data);
		
		mate_vfs_application_registry_save_mime_application (&app);
		mate_vfs_application_registry_add_mime_type (app.id, type);
		mate_vfs_application_registry_sync ();
	} else {
		usage (argv[0]);
	}

	return 0;
}
