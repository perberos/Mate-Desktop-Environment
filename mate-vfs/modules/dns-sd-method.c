/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* dnssd-method.c - dns-sd browsing

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

   Authors:
       Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifdef HAVE_AVAHI
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>
#include <avahi-common/simple-watch.h>
#include <avahi-glib/glib-watch.h>
#endif 

#ifdef HAVE_HOWL
/* Need to work around howl exporting its config file... */
#undef PACKAGE
#undef VERSION
#include <howl.h>
#endif

#include <libmatevfs/mate-vfs-ops.h>
#include <libmatevfs/mate-vfs-directory.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <libmatevfs/mate-vfs-dns-sd.h>
#include <libmatevfs/mate-vfs-monitor-private.h>

#include <glib/gi18n-lib.h>

#define BROWSE_TIMEOUT_MSEC 5000
#define RESOLVE_TIMEOUT_MSEC 5000
#define LOCAL_SYNC_BROWSE_DELAY_MSEC 200

static struct {
	char *type;
	char *method;
	char *icon;
	gpointer handle;
} dns_sd_types[] = {
	{"_ftp._tcp", "ftp", "mate-fs-ftp"},
	{"_webdav._tcp", "dav", "mate-fs-share"},
	{"_webdavs._tcp", "davs", "mate-fs-share"},
	{"_sftp-ssh._tcp", "sftp", "mate-fs-ssh"},
};

#if defined (HAVE_HOWL) || defined (HAVE_AVAHI)
G_LOCK_DEFINE_STATIC (local);
static gboolean started_local = FALSE;
static GList *local_files = NULL;
static GList *local_monitors = NULL;
#endif /* HAVE_HOWL || HAVE_AVAHI */

typedef struct {
	char *data;
	int len;
	int pos;
} FileHandle;

static FileHandle *
file_handle_new (char *data)
{
	FileHandle *result;
	result = g_new (FileHandle, 1);

	result->data = g_strdup (data);
	result->len = strlen (data);
	result->pos = 0;

	return result;
}

static void
file_handle_destroy (FileHandle *handle)
{
	g_free (handle->data);
	g_free (handle);
}

static char *
get_data_for_link (const char *uri, 
		   const char *display_name, 
		   const char *icon)
{
	char *data;

	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=FSDevice\n"
				"Icon=%s\n"
				"URL=%s\n",
				display_name,
				icon,
				uri);
	return data;
}

static gboolean
decode_filename (const char *filename,
		 char **service,
		 char **type,
		 char **domain)
{
	const char *p;
	char *end;
	GString *string;

	*service = NULL;
	*type = NULL;
	*domain = NULL;
	
	string = g_string_new (NULL);

	p = filename;

	while (*p && *p != '.') {
		if (*p == '\\') {
			p++;
			switch (*p) {
			case 's':
				g_string_append_c (string, '/');
				break;
			case '\\':
				g_string_append_c (string, '\\');
				break;
			case '.':
				g_string_append_c (string, '.');
				break;
			default:
				g_string_free (string, TRUE);
				return FALSE;
			}
			
		} else {
			g_string_append_c (string, *p);
		}
		p++;
	}
	
	*service = g_string_free (string, FALSE);

	if (*p == 0)
		goto error;

	p++;

	end = strchr (p, '.');
	if (end == NULL)
		goto error;
	end = strchr (end+1, '.');
	if (end == NULL)
		goto error;
	*end = 0;
	*type = g_strdup (p);
	p = end + 1;
	
	if (*p == 0 || *p == '.')
		goto error;

	*domain = g_strdup (p);

	return TRUE;

 error:
	g_free (*service);
	g_free (*type);
	return FALSE;
}

static char *
encode_filename (const char *service,
		 const char *type,
		 const char *domain)
{
	GString *string;
	const char *p;

	string = g_string_new (NULL);

	p = service;

	while (*p) {
		if (*p == '\\') 
			g_string_append (string, "\\\\");
		else if (*p == '.') 
			g_string_append (string, "\\.");
		else if (*p == '/') 
			g_string_append (string, "\\s");
		else
			g_string_append_c (string, *p);
		p++;
	}
	g_string_append_c (string, '.');
	g_string_append (string, type);
	g_string_append_c (string, '.');
	g_string_append (string, domain);

	return g_string_free (string, FALSE);
}

#if defined (HAVE_HOWL) || defined (HAVE_AVAHI)
static void
call_monitors (gboolean add, char *filename)
{
	MateVFSURI *info_uri, *base_uri;
	GList *l;

	if (local_monitors == NULL)
		return;
	
	base_uri = mate_vfs_uri_new ("dns-sd://local/");
	info_uri = mate_vfs_uri_append_file_name (base_uri, filename);
	mate_vfs_uri_unref (base_uri);

	/* This queues an idle, so there are no reentrancy issues */
	for (l = local_monitors; l != NULL; l = l->next) {
		mate_vfs_monitor_callback ((MateVFSMethodHandle *)l->data,
					    info_uri, 
					    add?MATE_VFS_MONITOR_EVENT_CREATED:MATE_VFS_MONITOR_EVENT_DELETED);
	}
	mate_vfs_uri_unref (info_uri);
}

static void
local_browse (gboolean add,
	      const char *name,
	      const char *type_in,
	      const char *domain_in)
{
	char *filename;
	GList *l;
	char *type;
	char *domain;
	int len;

	/* We don't want last dots in the domain or type */
	type = g_strdup (type_in);
	domain = g_strdup (domain_in);
	len = strlen (type);
	if (len > 0 && type[len-1] == '.')
		type[len-1] = 0;
	len = strlen (domain);
	if (len > 0 && domain[len-1] == '.')
		domain[len-1] = 0;
	
	filename = encode_filename (name, type, domain);
	g_free (type);
	g_free (domain);
	if (filename == NULL)
		return;
	
	for (l = local_files; l != NULL; l = l->next) {
		if (strcmp (l->data, filename) == 0) {
			if (!add) {
				
				g_free (l->data);
				local_files =
					g_list_delete_link (local_files, l);

				call_monitors (FALSE, filename);
			}
			g_free (filename);
			return;
		}
	}
	if (add) {
		local_files = g_list_prepend (local_files, filename);
		call_monitors (TRUE, filename);
	} else {
		g_free (filename);
	}
}

static void
local_browse_callback (MateVFSDNSSDBrowseHandle *handle,
		       MateVFSDNSSDServiceStatus status,
		       const MateVFSDNSSDService *service,
		       gpointer callback_data)
{
	G_LOCK (local);

	local_browse (status == MATE_VFS_DNS_SD_SERVICE_ADDED,
		      service->name, service->type, service->domain);
	
	G_UNLOCK (local);
}
#endif /* HAVE_HOWL || HAVE_AVAHI */


#ifdef HAVE_AVAHI
static void
avahi_client_callback (AvahiClient *client, AvahiClientState state, void *user_data)
{
	AvahiSimplePoll *poll;

	poll = user_data;
	if (state == AVAHI_CLIENT_FAILURE) {
		avahi_simple_poll_quit (poll);
	}
}

static void 
local_browse_callback_sync (AvahiServiceBrowser *b,
			    AvahiIfIndex interface,
			    AvahiProtocol protocol,
			    AvahiBrowserEvent event,
			    const char *name,
			    const char *type,
			    const char *domain,
			    AvahiLookupResultFlags flags,
			    void *user_data)
{
	AvahiSimplePoll *poll = user_data;

	if (event == AVAHI_BROWSER_NEW)
		local_browse (TRUE, name, type, domain);
	else if (event == AVAHI_BROWSER_REMOVE)
		local_browse (FALSE, name, type, domain);
	else if (event == AVAHI_BROWSER_ALL_FOR_NOW)
		avahi_simple_poll_quit (poll);
		
}

static void
stop_poll_timeout (AvahiTimeout *timeout, void *user_data)
{
	AvahiSimplePoll *poll = user_data;
	
	avahi_simple_poll_quit (poll);
}


static void
init_local (void)
{
	int i;
	MateVFSResult res;
	
	if (!started_local) {
		AvahiSimplePoll *simple_poll;
		const AvahiPoll *poll;
		AvahiClient *client = NULL;
		AvahiServiceBrowser **sb;
		struct timeval tv;
		int error;
		
		started_local = TRUE;
		
		for (i = 0; i < G_N_ELEMENTS (dns_sd_types); i++) {
			MateVFSDNSSDBrowseHandle *handle;
			res = mate_vfs_dns_sd_browse (&handle,
						       "local",
						       dns_sd_types[i].type,
						       local_browse_callback,
						       NULL, NULL);
			if (res == MATE_VFS_OK) {
				dns_sd_types[i].handle = handle;
			}
		}

		simple_poll = avahi_simple_poll_new ();
		if (simple_poll == NULL) {
			g_warning ("Failed to create simple poll object");
			return;
		}

		poll = avahi_simple_poll_get (simple_poll);
		client = avahi_client_new (poll, 0, 
					   avahi_client_callback, simple_poll, &error);
		
		/* Check wether creating the client object succeeded */
		if (client == NULL) {
			g_warning ("Failed to create client: %s\n", avahi_strerror (error));
			avahi_simple_poll_free (simple_poll);
			return;
		}

		sb = g_new0 (AvahiServiceBrowser *, G_N_ELEMENTS (dns_sd_types));

		for (i = 0; i < G_N_ELEMENTS (dns_sd_types); i++) {
			sb[i] = avahi_service_browser_new (client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 
							   dns_sd_types[i].type, "local",
							   AVAHI_LOOKUP_USE_MULTICAST,
							   local_browse_callback_sync, simple_poll);
		}
		

		avahi_elapse_time (&tv, LOCAL_SYNC_BROWSE_DELAY_MSEC, 0);
		poll->timeout_new (poll, &tv, stop_poll_timeout,
				   (void *)simple_poll);

		/* Run the main loop util reply or timeout */
		for (;;)
			if (avahi_simple_poll_iterate (simple_poll, -1) != 0)
				break;

		for (i = 0; i < G_N_ELEMENTS (dns_sd_types); i++) {
			if (sb[i] != NULL) {
				avahi_service_browser_free (sb[i]);
			}
		}
		avahi_client_free (client);
		avahi_simple_poll_free (simple_poll);

					  
	}
}
#endif /* HAVE_AVAHI */


#ifdef HAVE_HOWL
static sw_result
local_browse_callback_sync (sw_discovery                 discovery,
			    sw_discovery_oid             id,
			    sw_discovery_browse_status   status,
			    sw_uint32			 interface_index,
			    sw_const_string              name,
			    sw_const_string              type,
			    sw_const_string              domain,
			    sw_opaque                    extra)
{
	if (status == SW_DISCOVERY_BROWSE_ADD_SERVICE)
		local_browse (TRUE, name, type, domain);
	else if (status == SW_DISCOVERY_BROWSE_REMOVE_SERVICE)
		local_browse (FALSE, name, type, domain);

	return SW_OKAY;
}
			    

static void
init_local (void)
{
	int i;
	MateVFSResult res;
	
	if (!started_local) {
		sw_discovery session;
		sw_salt salt;
		sw_ulong timeout;
		struct timeval end_tv, tv;
		sw_discovery_oid *sync_handles;
		int timeout_msec;
		sw_result swres;
		
		started_local = TRUE;
		
		for (i = 0; i < G_N_ELEMENTS (dns_sd_types); i++) {
			MateVFSDNSSDBrowseHandle *handle;
			res = mate_vfs_dns_sd_browse (&handle,
						       "local",
						       dns_sd_types[i].type,
						       local_browse_callback,
						       NULL, NULL);
			if (res == MATE_VFS_OK) {
				dns_sd_types[i].handle = handle;
			}
		}

		if ((swres = sw_discovery_init (&session)) != SW_OKAY) {
			g_warning ("dns-sd: howl init failed: %d\n", (int)swres);
			return;
		}

		if (sw_discovery_salt (session, &salt) != SW_OKAY) {
			g_warning ("dns-sd: couldn't get salt\n");
			sw_discovery_fina (session);
			return;
		}

		sync_handles = g_new0 (sw_discovery_oid, G_N_ELEMENTS (dns_sd_types));

		for (i = 0; i < G_N_ELEMENTS (dns_sd_types); i++) {
			sw_discovery_browse (session,
					     0, 
					     dns_sd_types[i].type, "local",
					     local_browse_callback_sync,
					     NULL,
					     &(sync_handles[i]));
		}
		
		gettimeofday (&end_tv, NULL);
		tv = end_tv;

		timeout_msec = LOCAL_SYNC_BROWSE_DELAY_MSEC;
		
		end_tv.tv_sec += timeout_msec / 1000;
		end_tv.tv_usec += (timeout_msec % 1000) * 1000;
		end_tv.tv_sec += end_tv.tv_usec / 1000000;
		end_tv.tv_usec %= 1000000;
		
		do {
			timeout = timeout_msec;
			sw_salt_step (salt, &timeout);

			gettimeofday (&tv, NULL);
			timeout_msec = (end_tv.tv_sec - tv.tv_sec) * 1000 + 
				(end_tv.tv_usec - tv.tv_usec) / 1000;
		} while (timeout_msec > 0);
		
		for (i = 0; i < G_N_ELEMENTS (dns_sd_types); i++) {
			if (sync_handles[i] != 0) {
				sw_discovery_cancel (session, sync_handles[i]);
			}
		}
					  
		sw_discovery_fina (session);
	}
}
#endif /* HAVE_HOWL */




static MateVFSResult
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode mode,
	 MateVFSContext *context)
{
	FileHandle *file_handle;
	char *name;
	char *type;
	char *domain;
	char *filename;
	char *link_uri;
	char *host;
	char *data;
	char *path, *s, *user, *pwd, *user_and_pwd;
	int port;
	int i;
	MateVFSResult res;
	GHashTable *text;
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);

	if (mode & MATE_VFS_OPEN_WRITE) {
		return MATE_VFS_ERROR_NOT_PERMITTED;
	}

	if (strcmp (uri->text, "/") == 0) {
		return MATE_VFS_ERROR_IS_DIRECTORY;
	}

	if (mate_vfs_uri_get_host_name (uri) == NULL) {
		return MATE_VFS_ERROR_INVALID_HOST_NAME;
	}

	filename = mate_vfs_unescape_string (uri->text, 
					      G_DIR_SEPARATOR_S);
	
	if (filename[0] != '/' ||
	    !decode_filename (filename+1, &name, &type, &domain)) {
		g_free (filename);
		return MATE_VFS_ERROR_NOT_FOUND;
	}
	g_free (filename);

	for (i=0; i < G_N_ELEMENTS (dns_sd_types); i++) {
		if (strcmp (type, dns_sd_types[i].type) == 0) {
			break;
		}
	}
	if (i == G_N_ELEMENTS (dns_sd_types)) {
		g_free (name);
		g_free (type);
		g_free (domain);
		return MATE_VFS_ERROR_NOT_FOUND;
	}
	
	res = mate_vfs_dns_sd_resolve_sync (name, type, domain,
					     RESOLVE_TIMEOUT_MSEC,
					     &host, &port,
					     &text, NULL, NULL);
	g_free (type);
	g_free (domain);
	
	if (res != MATE_VFS_OK) {
		g_free (name);
		return MATE_VFS_ERROR_NOT_FOUND;
	}

	
	path = "/";
	user_and_pwd = NULL;
	if (text != NULL) {
		s = g_hash_table_lookup (text, "path");
		if (s != NULL)
			path = s;

		user = g_hash_table_lookup (text, "u");
		pwd = g_hash_table_lookup (text, "p");

		if (user != NULL) {
			if (pwd != NULL) {
				user_and_pwd = g_strdup_printf ("%s:%s@",
								user,
								pwd);
			} else {
				user_and_pwd = g_strdup_printf ("%s@",
								user);
			}
		}
		
		
	}
		    
	if (strchr (host, ':')) 
		/* must be an ipv6 address, follow rfc2732 */
		link_uri = g_strdup_printf ("%s://%s[%s]:%d%s",
					    dns_sd_types[i].method,
					    user_and_pwd?user_and_pwd:"",
					    host, port, path);
	else
		link_uri = g_strdup_printf ("%s://%s%s:%d%s",
					    dns_sd_types[i].method,
					    user_and_pwd?user_and_pwd:"",
					    host, port, path);
	g_free (user_and_pwd);

	/* TODO: Escape / in name */
	data = get_data_for_link (link_uri,
				  name,
				  dns_sd_types[i].icon);
	g_free (name);
	if (text)
		g_hash_table_destroy (text);
	
	file_handle = file_handle_new (data);
	
	g_free (data);
		
	*method_handle = (MateVFSMethodHandle *) file_handle;

	return MATE_VFS_OK;
}

static MateVFSResult
do_create (MateVFSMethod *method,
	   MateVFSMethodHandle **method_handle,
	   MateVFSURI *uri,
	   MateVFSOpenMode mode,
	   gboolean exclusive,
	   guint perm,
	   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)
{
	FileHandle *file_handle;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	file_handle_destroy (file_handle);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 gpointer buffer,
	 MateVFSFileSize num_bytes,
	 MateVFSFileSize *bytes_read,
	 MateVFSContext *context)
{
	FileHandle *file_handle;
	int read_len;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;
	*bytes_read = 0;
	
	if (file_handle->pos >= file_handle->len) {
		return MATE_VFS_ERROR_EOF;
	}

	read_len = MIN (num_bytes, file_handle->len - file_handle->pos);

	memcpy (buffer, file_handle->data + file_handle->pos, read_len);
	*bytes_read = read_len;
	file_handle->pos += read_len;

	
	return MATE_VFS_OK;
}

static MateVFSResult
do_write (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  MateVFSFileSize num_bytes,
	  MateVFSFileSize *bytes_written,
	  MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}


static MateVFSResult
do_seek (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition whence,
	 MateVFSFileOffset offset,
	 MateVFSContext *context)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	switch (whence) {
	case MATE_VFS_SEEK_START:
		file_handle->pos = offset;
		break;
	case MATE_VFS_SEEK_CURRENT:
		file_handle->pos += offset;
		break;
	case MATE_VFS_SEEK_END:
		file_handle->pos = file_handle->len + offset;
		break;
	}

	if (file_handle->pos < 0) {
		file_handle->pos = 0;
	}
	
	if (file_handle->pos > file_handle->len) {
		file_handle->pos = file_handle->len;
	}
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_tell (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize *offset_return)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	*offset_return = file_handle->pos;
	return MATE_VFS_OK;
}


static MateVFSResult
do_truncate_handle (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSFileSize where,
		    MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_truncate (MateVFSMethod *method,
	     MateVFSURI *uri,
	     MateVFSFileSize where,
	     MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

typedef struct {
	MateVFSFileInfoOptions options;
	GList *filenames;
} DirectoryHandle;

static DirectoryHandle *
directory_handle_new (MateVFSFileInfoOptions options)
{
	DirectoryHandle *result;

	result = g_new (DirectoryHandle, 1);
	result->options = options;
	result->filenames = NULL;

	return result;
}

static void
directory_handle_destroy (DirectoryHandle *dir_handle)
{
	g_list_foreach (dir_handle->filenames, (GFunc)g_free, NULL);
	g_list_free (dir_handle->filenames);
	g_free (dir_handle);
}

static void
directory_handle_add_filename (DirectoryHandle *dir_handle, char *file)
{
	if (file != NULL) {
		dir_handle->filenames = g_list_prepend (dir_handle->filenames, g_strdup (file));
	}
}

#if defined (HAVE_HOWL) || defined (HAVE_AVAHI)
static void
directory_handle_add_filenames (DirectoryHandle *dir_handle, GList *files)
{
	while (files != NULL) {
		directory_handle_add_filename (dir_handle, files->data);
		files = files->next;
	}
} 
#endif /* HAVE_HOWL || HAVE_AVAHI */

static MateVFSResult
do_open_directory (MateVFSMethod *method,
		   MateVFSMethodHandle **method_handle,
		   MateVFSURI *uri,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context)
{
	DirectoryHandle *dir_handle;
	const char *domain;
	int i;
	MateVFSResult res;
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);

	if (strcmp (uri->text, "") != 0 &&
	    strcmp (uri->text, "/") != 0) {
		return MATE_VFS_ERROR_NOT_A_DIRECTORY;
	}

	domain = mate_vfs_uri_get_host_name (uri);
	if (domain == NULL) {
		return MATE_VFS_ERROR_INVALID_HOST_NAME;
	}

	dir_handle = directory_handle_new (options);
	
	if (strcmp (domain, "local") == 0) {
#if defined (HAVE_HOWL) || defined (HAVE_AVAHI)
		G_LOCK (local);
		init_local ();

		directory_handle_add_filenames (dir_handle, local_files);
		
		G_UNLOCK (local);
#endif /* HAVE_HOWL || HAVE_AVAHI */
	} else 	{
		for (i=0; i < G_N_ELEMENTS (dns_sd_types); i++) {
			int n_services;
			MateVFSDNSSDService *services;
			int j;
			char *filename;
			
			res = mate_vfs_dns_sd_browse_sync (domain, dns_sd_types[i].type,
							    BROWSE_TIMEOUT_MSEC,
							    &n_services,
							    &services);
			
			if (res == MATE_VFS_OK) {
				for (j = 0; j < n_services; j++) {
					filename = encode_filename (services[j].name,
								    services[j].type,
								    services[j].domain);
					if (filename)
						directory_handle_add_filename (dir_handle, filename);
					
					g_free (services[j].name);
					g_free (services[j].type);
					g_free (services[j].domain);
				}
				g_free (services);
			}
		}
	}

	*method_handle = (MateVFSMethodHandle *) dir_handle;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context)
{
	DirectoryHandle *dir_handle;

	dir_handle = (DirectoryHandle *) method_handle;

	directory_handle_destroy (dir_handle);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{
	DirectoryHandle *handle;
	GList *entry;

	handle = (DirectoryHandle *) method_handle;

	if (handle->filenames == NULL) {
		return MATE_VFS_ERROR_EOF;
	}

	entry = handle->filenames;
	handle->filenames = g_list_remove_link (handle->filenames, entry);
	
	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;
	file_info->name = g_strdup (entry->data);
	g_free (entry->data);
	g_list_free_1 (entry);

	file_info->mime_type = g_strdup ("application/x-desktop");
	file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
	file_info->valid_fields |=
		MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
		MATE_VFS_FILE_INFO_FIELDS_TYPE;

	file_info->permissions =
		MATE_VFS_PERM_USER_READ |
		MATE_VFS_PERM_OTHER_READ |
		MATE_VFS_PERM_GROUP_READ;
	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context)
{
	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;
	
	if (strcmp (uri->text, "") == 0 ||
	    strcmp (uri->text, "/") == 0) {
		file_info->name = g_strdup ("/");
		
		file_info->mime_type = g_strdup ("x-directory/normal");
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		file_info->valid_fields |=
			MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_TYPE;
	} else {
		file_info->name = mate_vfs_uri_extract_short_name (uri);
		
		file_info->mime_type = g_strdup ("application/x-desktop");
		file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
		file_info->valid_fields |=
			MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_TYPE;
	}
	file_info->permissions =
		MATE_VFS_PERM_USER_READ |
		MATE_VFS_PERM_OTHER_READ |
		MATE_VFS_PERM_GROUP_READ;
	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
			      MateVFSMethodHandle *method_handle,
			      MateVFSFileInfo *file_info,
			      MateVFSFileInfoOptions options,
			      MateVFSContext *context)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;
	
	file_info->mime_type = g_strdup ("application/x-desktop");
	file_info->size = file_handle->len;
	file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
	file_info->valid_fields |=
		MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
		MATE_VFS_FILE_INFO_FIELDS_SIZE |
		MATE_VFS_FILE_INFO_FIELDS_TYPE;

	return MATE_VFS_OK;
}

static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	return TRUE;
}


static MateVFSResult
do_make_directory (MateVFSMethod *method,
		   MateVFSURI *uri,
		   guint perm,
		   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_remove_directory (MateVFSMethod *method,
		     MateVFSURI *uri,
		     MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}


static MateVFSResult
do_find_directory (MateVFSMethod *method,
		   MateVFSURI *near_uri,
		   MateVFSFindDirectoryKind kind,
		   MateVFSURI **result_uri,
		   gboolean create_if_needed,
		   gboolean find_if_needed,
		   guint permissions,
		   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_move (MateVFSMethod *method,
	 MateVFSURI *old_uri,
	 MateVFSURI *new_uri,
	 gboolean force_replace,
	 MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_unlink (MateVFSMethod *method,
	   MateVFSURI *uri,
	   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_create_symbolic_link (MateVFSMethod *method,
			 MateVFSURI *uri,
			 const char *target_reference,
			 MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

/* When checking whether two locations are on the same file system, we are
   doing this to determine whether we can recursively move or do other
   sorts of transfers.  When a symbolic link is the "source", its
   location is the location of the link file, because we want to
   know about transferring the link, whereas for symbolic links that
   are "targets", we use the location of the object being pointed to,
   because that is where we will be moving/copying to. */
static MateVFSResult
do_check_same_fs (MateVFSMethod *method,
		  MateVFSURI *source_uri,
		  MateVFSURI *target_uri,
		  gboolean *same_fs_return,
		  MateVFSContext *context)
{
	return TRUE;
}
static MateVFSResult
do_set_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  const MateVFSFileInfo *info,
		  MateVFSSetFileInfoMask mask,
		  MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}


static MateVFSResult
do_file_control (MateVFSMethod *method,
		 MateVFSMethodHandle *method_handle,
		 const char *operation,
		 gpointer operation_data,
		 MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_monitor_add (MateVFSMethod *method,
		MateVFSMethodHandle **method_handle_return,
		MateVFSURI *uri,
		MateVFSMonitorType monitor_type)
{
	const char *domain;
	
	domain = mate_vfs_uri_get_host_name (uri);
	if (domain == NULL) {
		return MATE_VFS_ERROR_INVALID_HOST_NAME;
	}

	if (strcmp (domain, "local") != 0) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
#if defined (HAVE_HOWL) || defined (HAVE_AVAHI)
	if (strcmp (uri->text, "") == 0 ||
	    strcmp (uri->text, "/") == 0) {
		int *handle;
		
		G_LOCK (local);
		init_local ();
		
		handle = g_new0 (int, 1);

		local_monitors = g_list_prepend (local_monitors, handle);
		
		G_UNLOCK (local);

		*method_handle_return = (MateVFSMethodHandle *)handle;

		return MATE_VFS_OK;
	} else 
#endif /* HAVE_HOWL || HAVE_AVAHI */
		return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_monitor_cancel (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle)
{
#if defined (HAVE_HOWL) || defined (HAVE_AVAHI)
	G_LOCK (local);
	
	local_monitors = g_list_remove (local_monitors, method_handle);
	g_free (method_handle);
	
	G_UNLOCK (local);

	return MATE_VFS_OK;
#else
	return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif /* HAVE_HOWL || HAVE_AVAHI */
}


static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	do_seek,
	do_tell,
	do_truncate_handle,
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	do_get_file_info_from_handle,
	do_is_local,
	do_make_directory,
	do_remove_directory,
	do_move,
	do_unlink,
	do_check_same_fs,
	do_set_file_info,
	do_truncate,
	do_find_directory,
	do_create_symbolic_link,
	do_monitor_add,
	do_monitor_cancel,
	do_file_control
};


MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
}
