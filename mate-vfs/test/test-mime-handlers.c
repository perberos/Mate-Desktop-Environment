/* 
 * Copyright (C) 2004  Red Hat, Inc., Marco Pesenti Gritti <mpg@redhat.com>
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

#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-mime-handlers.h>

#ifdef TEXT_EXEC_MACRO_EXPANSION
void test_exec_macro_expansion (void);
#endif

static void
print_application_info (MateVFSMimeApplication *app)
{
	g_print ("Desktop ID %s\n",
		 mate_vfs_mime_application_get_desktop_id (app));
	g_print ("Desktop file path %s\n",
		 mate_vfs_mime_application_get_desktop_file_path (app));
	g_print ("Name %s\n",
		 mate_vfs_mime_application_get_name (app));
	g_print ("Generic Name %s\n",
		 mate_vfs_mime_application_get_generic_name (app));
	g_print ("Icon %s\n",
		 mate_vfs_mime_application_get_icon (app));
	g_print ("Exec %s\n",
		 mate_vfs_mime_application_get_exec (app));
	g_print ("Supports uris %d\n",
		 mate_vfs_mime_application_supports_uris (app));
	g_print ("Requires terminal %d\n",
		 mate_vfs_mime_application_requires_terminal (app));
	g_print ("Supports startup notification %d\n",
		 mate_vfs_mime_application_supports_startup_notification (app));
	g_print ("Startup WM class %s\n",
		 mate_vfs_mime_application_get_startup_wm_class (app));
	g_print ("Binary name %s\n",
		 mate_vfs_mime_application_get_binary_name (app));
}

int
main (int argc, char **argv)
{
	MateVFSMimeApplication *application;
	GList *applications, *l;
	const char *type, *uri = NULL;

	mate_vfs_init ();
        if (argc < 2) {
               g_print ("Usage: %s mime_type\n", *argv);
               return 1;
	}

	type = argv[1];

	if (argc > 2) uri = argv[2];

	g_print ("----- MIME type -----\n\n");
	
	g_print ("Description: %s\n",
		 mate_vfs_mime_get_description (type));

	g_print ("Icon: %s\n",
		 mate_vfs_mime_get_icon (type));

	g_print ("Can be executable: %d\n",
		 mate_vfs_mime_can_be_executable (type));

	g_print ("----- Default application -----\n\n");

	if (uri) {
		application = mate_vfs_mime_get_default_application_for_uri (uri, type);
	} else {
		application = mate_vfs_mime_get_default_application (type);
	}

	if (application == NULL) {
		g_print ("No default application.\n");
	} else {	
		print_application_info (application);
		mate_vfs_mime_application_free (application);
	}

	g_print ("----- All applications -----\n\n");

	if (uri) {
		applications = mate_vfs_mime_get_all_applications_for_uri (uri, type);
	} else {
		applications = mate_vfs_mime_get_all_applications (type);
	}

	if (applications == NULL) {
		g_print ("No applications.\n");	
	}

	for (l = applications; l != NULL; l = l->next)
	{
		print_application_info (l->data);
		g_print ("-----------------------------\n");
	}
	mate_vfs_mime_application_list_free (applications);	

#ifdef TEXT_EXEC_MACRO_EXPANSION
	g_print ("Test exec macro expansion\n");
	test_exec_macro_expansion ();
#endif

	mate_vfs_shutdown ();
	return 0;
}


