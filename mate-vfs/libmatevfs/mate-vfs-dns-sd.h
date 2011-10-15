/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-dns-sd.h - DNS-SD functions

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

#ifndef MATE_VFS_DNS_SD_H
#define MATE_VFS_DNS_SD_H

#include <sys/types.h>
#include <glib.h>
#include <glib-object.h>
#include <libmatevfs/mate-vfs-result.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TODO:
 * a way to get default browse domain for domain
 * async listing of browse domains
 * some way to publish. Using dns update for unicast?
 */


typedef struct {
	char *name;
	char *type;
	char *domain;
} MateVFSDNSSDService;

typedef enum {
	MATE_VFS_DNS_SD_SERVICE_ADDED,
	MATE_VFS_DNS_SD_SERVICE_REMOVED
} MateVFSDNSSDServiceStatus;

GType mate_vfs_dns_sd_service_status_get_type (void);
#define MATE_VFS_TYPE_VFS_DNS_SD_SERVICE_STATUS (mate_vfs_dns_sd_service_status_get_type())

typedef struct MateVFSDNSSDBrowseHandle MateVFSDNSSDBrowseHandle;
typedef struct MateVFSDNSSDResolveHandle MateVFSDNSSDResolveHandle;

/**
 * MateVFSDNSSDBrowseCallback:
 * @handle: handle of the operation generating the callback
 * @status: whether a service addition or removal was detected
 * @service: the service that was discovered or removed
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the mate_vfs_dns_sd_browse() function that informs
 * the user of services that are added or removed.
 **/
typedef void (* MateVFSDNSSDBrowseCallback) (MateVFSDNSSDBrowseHandle *handle,
					      MateVFSDNSSDServiceStatus status,
					      const MateVFSDNSSDService *service,
					      gpointer callback_data);

/**
 * MateVFSDNSSDResolveCallback:
 * @handle: handle of the operation generating the callback
 * @result: whether the resolve succeeded or not
 * @service: the service that was resolved
 * @host: the host name or ip of the host hosting the service
 * @port: the port number to use for the service
 * @text: a hash table giving additional options about the service
 * @text_raw_len: length of @text_raw
 * @text_raw: raw version of the additional options in @text
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the mate_vfs_dns_sd_resolve() function that is called
 * when a service has been resolved.
 *
 * The @host and @port can be used to contact the requested service, and
 * @text contains additional options as defined for the type requested.
 *
 * To check for options being set in @text without any value ("key" as
 * opposed to "key=value") you must use g_hash_table_lookup_extended(), since
 * they are stored in the hash table with a NULL value, and g_hash_table_lookup()
 * can't tell that from the case where the key is not in the hash table.
 **/
typedef void (* MateVFSDNSSDResolveCallback) (MateVFSDNSSDResolveHandle *handle,
					       MateVFSResult result,
					       const MateVFSDNSSDService *service,
					       const char *host,
					       int port,
					       const GHashTable *text,
					       int text_raw_len,
					       const char *text_raw,
					       gpointer callback_data);

/* Async versions */


MateVFSResult
mate_vfs_dns_sd_browse (MateVFSDNSSDBrowseHandle **handle,
			 const char *domain,
			 const char *type,
			 MateVFSDNSSDBrowseCallback callback,
			 gpointer callback_data,
			 GDestroyNotify callback_data_destroy_func);

MateVFSResult
mate_vfs_dns_sd_stop_browse (MateVFSDNSSDBrowseHandle *handle);


MateVFSResult
mate_vfs_dns_sd_resolve (MateVFSDNSSDResolveHandle **handle,
			  const char *name,
			  const char *type,
			  const char *domain,
			  int timeout,
			  MateVFSDNSSDResolveCallback callback,
			  gpointer callback_data,
			  GDestroyNotify callback_data_destroy_func);

MateVFSResult
mate_vfs_dns_sd_cancel_resolve (MateVFSDNSSDResolveHandle *handle);


/* Sync versions */

MateVFSResult
mate_vfs_dns_sd_browse_sync (const char *domain,
			      const char *type,
			      int timeout_msec,
			      int *n_services,
			      MateVFSDNSSDService **services);


MateVFSResult
mate_vfs_dns_sd_resolve_sync (const char *name,
			       const char *type,
			       const char *domain,
			       int timeout_msec,
			       char **host, int *port,
			       GHashTable **text,
			       int *text_raw_len_out,
			       char **text_raw_out);

void
mate_vfs_dns_sd_service_list_free (MateVFSDNSSDService *services,
				    int n_services);

MateVFSResult
mate_vfs_dns_sd_list_browse_domains_sync (const char *domain,
					   int timeout_msec,
					   GList **domains);


GList *
mate_vfs_get_default_browse_domains (void);


#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_DNS_SD_H */
