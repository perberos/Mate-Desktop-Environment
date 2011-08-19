/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-dns-ds.c - Test program for the MATE Virtual File System.

   Copyright (C) 2004 Red Hat, Inc

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

   Author: Alexander Larsson <alexl@redhat.com>
*/


#include <stdio.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-dns-sd.h>

static GMainLoop *main_loop;

static void
print_browse_result (MateVFSDNSSDBrowseHandle *handle,
		     MateVFSDNSSDServiceStatus status,
		     const MateVFSDNSSDService *service,
		     gpointer callback_data)
{
	g_print ("status: %s, service: '%s', type: %s, domain: %s\n",
		 status == MATE_VFS_DNS_SD_SERVICE_ADDED? "added" : "removed",
		 service->name, service->type, service->domain);
}

static void
print_resolve_result (MateVFSDNSSDResolveHandle *handle,
		      MateVFSResult result,
		      const MateVFSDNSSDService *service,
		      const char *host,
		      int port,
		      const GHashTable *text,
		      int text_raw_len,
		      const char *text_raw,
		      gpointer callback_data)
{
	if (result == MATE_VFS_OK)
		g_print ("service: '%s', type: %s, domain: %s, host: %s, port: %d\n",
			 service->name, service->type, service->domain,
			 host, port);
	else
		g_print ("error: %d\n", result);
	
	g_main_loop_quit (main_loop);
}


int
main (int argc, char **argv)
{
	int i;
	MateVFSResult res;
	int n_services;
	MateVFSDNSSDService *services;
	MateVFSDNSSDBrowseHandle *browse_handle;
	MateVFSDNSSDResolveHandle *resolve_handle;
	char *domain, *type;
	int port, text_len;
	char *host;
	char *text;
	GList *list, *l, *domains;
	GHashTable *text_hash;
	
	if (argc < 3) {
		domain = "dns-sd.org";
		type = "_ftp._tcp";
	} else {
		domain = argv[1];
		type = argv[2];
	}

	fprintf (stderr, "Initializing mate-vfs...\n");
	mate_vfs_init ();

	fprintf (stderr, "Getting default browse domains:");
	domains = mate_vfs_get_default_browse_domains ();
	for (l = domains; l != NULL; l = l->next) {
		g_print ("%s,", (char *)l->data);
		g_free (l->data);
	}
	g_print ("\n");
	g_list_free (domains);

	
	fprintf (stderr, "Trying sync list\n");
	res = mate_vfs_dns_sd_list_browse_domains_sync ("dns-sd.org",
							 2000,
							 &list);
	if (res == MATE_VFS_OK) {
		for (l = list; l != NULL; l = l->next) {
			g_print ("search domain: %s\n", (char *)l->data);
			g_free (l->data);
		}
		g_list_free (list);
	} else {
		fprintf (stderr, "list error %d\n", res);
	}
	
	fprintf (stderr, "Trying sync resolve\n");
	res = mate_vfs_dns_sd_resolve_sync (
					     "Apple QuickTime Files", "_ftp._tcp", "dns-sd.org",
					     4000,
					     &host, &port,
					     &text_hash,
					     &text_len, &text);
	if (res == MATE_VFS_OK) {
		g_print ("host: %s, port: %d, text: %.*s\n",
			 host, port, text_len, text);
	} else {
		fprintf (stderr, "resolve error %d\n", res);
	}

	fprintf (stderr, "Trying local resolve\n");
	res = mate_vfs_dns_sd_resolve_sync ("Alex other test",
					     "_ftp._tcp",
					     "local",
					     4000,
					     &host, &port,
					     NULL,
					     &text_len, &text);
	if (res == MATE_VFS_OK) {
		g_print ("host: %s, port: %d, text: %.*s\n",
			 host, port, text_len, text);
	} else {
		fprintf (stderr, "resolve error %d\n", res);
	}

	
	fprintf (stderr, "Trying sync browsing\n");
	res = mate_vfs_dns_sd_browse_sync (domain, type,
					    4000,
					    &n_services,
					    &services);
	if (res == MATE_VFS_OK) {
		fprintf (stderr, "Number of hits: %d\n", n_services);
		for (i = 0; i < n_services; i++) {
			fprintf (stderr, "service name: '%s' type: %s, domain: %s\n",
				 services[i].name, services[i].type, services[i].domain);
		}
	} else {
		fprintf (stderr, "error %d\n", res);
	}

	
	main_loop = g_main_loop_new (NULL, TRUE);
	
	fprintf (stderr, "Trying async resolve\n");

	res = mate_vfs_dns_sd_resolve (&resolve_handle,
					"Apple QuickTime Files", "_ftp._tcp", "dns-sd.org",
					4000,
					print_resolve_result,
					NULL, NULL);
	
	fprintf (stderr, "Main loop running.\n");
	g_main_loop_run (main_loop);
	
	fprintf (stderr, "Trying local async resolve\n");

	res = mate_vfs_dns_sd_resolve (&resolve_handle,
					"Alex other test",
					"_ftp._tcp",
					"local",
					4000,
					print_resolve_result,
					NULL, NULL);
	
	fprintf (stderr, "Main loop running.\n");
	g_main_loop_run (main_loop);
	
	
	fprintf (stderr, "Trying async browsing (won't terminate)\n");

	res = mate_vfs_dns_sd_browse (&browse_handle,
				       domain, type,
				       print_browse_result,
				       NULL, NULL);
		
	fprintf (stderr, "Main loop running.\n");
	g_main_loop_run (main_loop);

	fprintf (stderr, "Main loop finished.\n");

	g_main_loop_unref (main_loop);
	
	fprintf (stderr, "All done\n");

	mate_vfs_shutdown ();

	return 0;
}
