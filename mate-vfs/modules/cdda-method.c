/* -*- Mode: C; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */

/* cdda-method.c

   Copyright (C) 2000, Eazel Inc.

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

   Author: Gene Z. Ragan <gzr@eazel.com> 
*/

#include <config.h>

#include "cdda-cddb.h"
#include <errno.h>
#include <mateconf/mateconf-client.h>
#include <mateconf/mateconf.h>
#include <libmatevfs/mate-vfs-cancellation.h>
#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cdda_paranoia.h>

typedef struct {
	MateVFSURI *uri;
	MateVFSFileInfo *file_info;
	cdrom_drive *drive;	
	int access_count;
	unsigned int cddb_discid;
	gboolean use_cddb;
	DiscData disc_data;
} CDDAContext;

typedef struct {
	MateVFSURI *uri;
	gboolean inited;
	gboolean wrote_header;
	cdrom_paranoia *paranoia;
	long cursor;
	long first_sector, last_sector;
} ReadHandle;

CDDAContext *global_context = NULL;

/* This is here to work around a broken header file.
 * cdda_interface.h has a statically defined array of
 * chars that is unused. This will break our build
 * due to our strict error checking.
 */
char **broken_header_fix = strerror_tr;

static MateVFSResult do_open	         	(MateVFSMethod              	*method,
						 MateVFSMethodHandle         	**method_handle,
						 MateVFSURI                   	*uri,
					  	 MateVFSOpenMode               mode,
					  	 MateVFSContext               	*context);
static gboolean       do_is_local        	(MateVFSMethod               	*method,
					  	 const MateVFSURI             	*uri);
static MateVFSResult do_open_directory  	(MateVFSMethod               	*method,
					  	 MateVFSMethodHandle         	**method_handle,
					  	 MateVFSURI                   	*uri,
					  	 MateVFSFileInfoOptions        options,
					  	 MateVFSContext               	*context);
static MateVFSResult do_close_directory 	(MateVFSMethod               	*method,
					  	 MateVFSMethodHandle          	*method_handle,
					  	 MateVFSContext               	*context);
static MateVFSResult do_read_directory  	(MateVFSMethod               	*method,
                          			 MateVFSMethodHandle          	*method_handle,
                          			 MateVFSFileInfo              	*file_info,
                          			 MateVFSContext               	*context);
static int		get_data_size 		(cdrom_drive 			*drive, 
						 int 				track);
static gboolean	is_file_is_on_disc 		(CDDAContext 			*context, 
						 const MateVFSURI 		*uri);
static int 		write_wav_header 	(gpointer 			buffer, 
						 long 				bytes);


static const char PROXY_HOST_KEY[] = "/system/http_proxy/host";
static const char PROXY_PORT_KEY[] = "/system/http_proxy/port";
static const char USE_PROXY_KEY[] = "/system/http_proxy/use_http_proxy";

static CDDAContext *
cdda_context_new (cdrom_drive *drive, MateVFSURI *uri)
{
	CDDAContext *context;
	MateConfClient *mateconf_client;
	char *proxy_host;
	gboolean use_proxy;
	ProxyServer proxy_server;
	CDDBServer cddb_server;
				
	context = g_new0 (CDDAContext, 1);
	context->drive = drive;
	context->file_info = mate_vfs_file_info_new ();
	context->uri = mate_vfs_uri_ref (uri);
	context->access_count = 0;
	context->cddb_discid = CDDBDiscid (drive);

	/* Look up CDDB info */
	mateconf_client = mateconf_client_get_default ();
		
	use_proxy = mateconf_client_get_bool (mateconf_client, USE_PROXY_KEY, NULL);
	if (use_proxy) {
		proxy_host = mateconf_client_get_string (mateconf_client, PROXY_HOST_KEY, NULL);

		proxy_server.port = mateconf_client_get_int (mateconf_client, PROXY_PORT_KEY, NULL);

		if (proxy_host != NULL) {	
			strcpy (proxy_server.name, proxy_host);
			g_free (proxy_host);
		} else {
			use_proxy = FALSE;
		}

		if (proxy_server.port == 0) {
			proxy_server.port = 8080;
		}
	}

	strcpy (cddb_server.name, "freedb.freedb.org");
	strcpy (cddb_server.cgi_prog, "~cddb/cddb.cgi");
	cddb_server.port = 80;
	cddb_server.use_proxy = use_proxy;
	cddb_server.proxy = &proxy_server;

	context->use_cddb = CDDBLookupDisc (&cddb_server, drive, &context->disc_data);

	return context;
}

static void
cdda_context_free (CDDAContext *context)
{
	if (context == NULL) {
		return;
	}

	cdda_close (context->drive);
	mate_vfs_file_info_unref (context->file_info);
	mate_vfs_uri_unref (context->uri);
	
	g_free (context);
	context = NULL;
}

static void
cdda_set_file_info_for_root (CDDAContext *context, MateVFSURI *uri)
{
	g_assert (context);

	/* We don't know the io_block size */
	context->file_info->io_block_size = 0;
	context->file_info->valid_fields -= MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;		
	context->file_info->name = mate_vfs_uri_extract_short_path_name (uri);
	/*context->file_info->name = g_strdup (context->disc_data.data_title);*/
	context->file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
	context->file_info->mime_type = g_strdup ("x-directory/normal");
	context->file_info->atime = time (NULL);
	context->file_info->ctime = time (NULL);
	context->file_info->mtime = time (NULL);
	context->file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_TYPE |
										MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
										MATE_VFS_FILE_INFO_FIELDS_ATIME | 
										MATE_VFS_FILE_INFO_FIELDS_MTIME |
										MATE_VFS_FILE_INFO_FIELDS_CTIME;
};

static ReadHandle *
read_handle_new (MateVFSURI *uri)
{
	ReadHandle *result;
	result = g_new (ReadHandle, 1);

	result->uri = mate_vfs_uri_ref (uri);
	result->inited = FALSE;
	result->wrote_header = FALSE;
	result->paranoia = NULL;
	result->cursor = 0;
	result->first_sector = 0;
	result->last_sector = 0;
	
	return result;
}

static void
read_handle_destroy (ReadHandle *handle)
{
	mate_vfs_uri_unref (handle->uri);

	if (handle->paranoia == NULL) {
		paranoia_free (handle->paranoia);
	}
	
	g_free (handle);
}

static int
get_track_index_from_uri (CDDAContext *context, MateVFSURI *uri) {
	char *base_name;
	int index;
	char *escaped_name;
	
	base_name = mate_vfs_uri_extract_short_path_name (uri);
	escaped_name = mate_vfs_unescape_string_for_display (base_name);
	g_free (base_name);

	/* Check and see if filename is in cddb data list */
	for (index = 0; index < context->drive->tracks; index++) {
		if (strcmp (escaped_name, context->disc_data.data_track[index].track_name) == 0) {
			g_free (escaped_name);
			return index + 1;
		}
	}

	g_free (escaped_name);
	return -1;
}

static MateVFSResult 
do_open (MateVFSMethod *method, MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri, MateVFSOpenMode mode, MateVFSContext *context) 
{
	MateVFSResult result;
	ReadHandle *read_handle;
	MateVFSFileInfoOptions options;
	char *base_name;
	char *dirname, *schemedir, *sep;
	MateVFSURI *dir_uri;
	
	result = MATE_VFS_ERROR_GENERIC;
	*method_handle = NULL;

	/*g_message ("cdda do_open: %s", mate_vfs_uri_get_path (uri));*/

	/* Load in context for disc if we not yet done so. */
	if (global_context == NULL) {
		base_name = mate_vfs_uri_extract_short_path_name (uri);
		if (base_name[0] == MATE_VFS_URI_PATH_CHR) {
			g_free (base_name);
			return result;
		} else {
			g_free (base_name);
		}

		dirname = mate_vfs_uri_extract_dirname (uri);			
		schemedir = g_strdup_printf ("cdda://%s", dirname);
		
		/* Remove trailing '/' if there is one */
		sep = strrchr (schemedir, '/');
		if (sep != NULL) {
			schemedir [strlen (schemedir) - 1] = '\0';
		}
		
		dir_uri = mate_vfs_uri_new (schemedir);
		options = 0;
		result = do_open_directory (method, method_handle, dir_uri, options, NULL);
		mate_vfs_uri_unref (dir_uri);

		if (result != MATE_VFS_OK) {
			/*g_message ("cdda do_open: Unable to load context");*/
			return result;
		}
	}

	if (mode == MATE_VFS_OPEN_READ) {
		/* Make sure file is present */
		if (is_file_is_on_disc (global_context, uri)) {
			result = MATE_VFS_OK;			
			read_handle = read_handle_new (uri);
			
			/* Set up cdparanoia */
			if (!read_handle->inited) {
				int track;
				int paranoia_mode;
				long offset, sec, off;

				track = get_track_index_from_uri (global_context, uri);
				if (track == -1) {
					return MATE_VFS_ERROR_GENERIC;
				}
				
 				if (!cdda_track_audiop (global_context->drive, track)) {
    					/*g_message ("Error. Selected track is not an audio track.");*/
    					return MATE_VFS_ERROR_GENERIC;
  				}

				/* Calculate sector span and offset */
				sec = cdda_track_firstsector (global_context->drive, track);
				off = cdda_track_lastsector (global_context->drive, track) - sec + 1;
								
				read_handle->first_sector = 0;
				read_handle->last_sector = off - 1;
												
				offset = cdda_track_firstsector (global_context->drive, track);
				read_handle->first_sector += offset;
				read_handle->last_sector += offset;

				read_handle->paranoia = paranoia_init (global_context->drive);
				/*paranoia_mode = PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP;*/
				paranoia_mode = PARANOIA_MODE_DISABLE;
  				paranoia_modeset (read_handle->paranoia, paranoia_mode);
				cdda_verbose_set (global_context->drive, CDDA_MESSAGE_PRINTIT,CDDA_MESSAGE_FORGETIT);

				paranoia_seek (read_handle->paranoia, read_handle->cursor = read_handle->first_sector, SEEK_SET);

				read_handle->inited = TRUE;
			}

			*method_handle = (MateVFSMethodHandle *) read_handle;
		}
	} else if (mode == MATE_VFS_OPEN_WRITE) {
		result = MATE_VFS_ERROR_READ_ONLY;		
	} else {
		result = MATE_VFS_ERROR_INVALID_OPEN_MODE;
	}

	return result;
}

static MateVFSResult 
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context) 
{
	ReadHandle *read_handle;
	
	/*g_message ("cdda do_close");*/

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	read_handle = (ReadHandle *) method_handle;
	read_handle_destroy (read_handle);
	
	return MATE_VFS_OK;
}


/* We have to pass in a callback to paranoia_read, even though we don't use it */
static void 
paranoia_callback (long inpos, int function) {
}

static MateVFSResult 
do_read (MateVFSMethod *method, 
	 MateVFSMethodHandle *method_handle, 
	 gpointer buffer,
	 MateVFSFileSize num_bytes, 
	 MateVFSFileSize *bytes_read, 
	 MateVFSContext *context) 
{
	ReadHandle *read_handle;
	gint16 *readbuf;
	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	if (mate_vfs_context_check_cancellation (context)) {
		return MATE_VFS_ERROR_CANCELLED;
	}

	read_handle = (ReadHandle *) method_handle;
	if (read_handle == NULL) {
		return MATE_VFS_ERROR_INTERNAL;
	}

	readbuf = NULL;

	if (!read_handle->wrote_header) {		
		*bytes_read = write_wav_header (buffer, (read_handle->last_sector - read_handle->first_sector + 1) 
										* CD_FRAMESIZE_RAW);
		read_handle->wrote_header = TRUE;
		return MATE_VFS_OK;
	}

		
	if (read_handle->cursor <= read_handle->last_sector) {
		readbuf = paranoia_read (read_handle->paranoia, paranoia_callback);
	} else {
		return MATE_VFS_ERROR_EOF;
		
	}

	if (readbuf == NULL) {
		*bytes_read = 0;
		return MATE_VFS_ERROR_GENERIC;
	}

	read_handle->cursor++;	

	memcpy (buffer, readbuf, CD_FRAMESIZE_RAW);	
	*bytes_read = CD_FRAMESIZE_RAW;
	       
	return MATE_VFS_OK;
}

#if 0
static void 
display_toc (cdrom_drive *d)
{
	long audiolen = 0;
  	int i;

  	printf ("\nTable of contents (audio tracks only):\n"
	 "track        length               begin        copy pre ch\n"
	 "===========================================================\n");
  
	for (i = 1; i <= d->tracks; i++) {	
		if (cdda_track_audiop (d, i)) {
      		char buffer[256];

      		long sec = cdda_track_firstsector (d, i);
      		long off = cdda_track_lastsector (d, i) - sec + 1;
      
      		sprintf(buffer,
	      		"%3d.  %7ld [%02d:%02d.%02d]  %7ld [%02d:%02d.%02d]  %s %s %s",
	      		i,
	      		off,(int)(off/(60*75)),(int)((off/75)%60),(int)(off%75),
	      		sec,(int)(sec/(60*75)),(int)((sec/75)%60),(int)(sec%75),
	      		cdda_track_copyp (d,i)?"  OK":"  no",
	      		cdda_track_preemp (d,i)?" yes":"  no",
	      		cdda_track_channels (d,i)==2?" 2":" 4");
      			printf ("%s\n", buffer);
      			audiolen+=off;
		}
	}
	
	{
		char buffer[256];
		sprintf (buffer, "TOTAL %7ld [%02d:%02d.%02d]    (audio only)",
	    		 audiolen, (int)(audiolen / (60 * 75)),(int)((audiolen / 75) % 60),
	    		 (int)(audiolen % 75));
      	printf ("%s\n", buffer);
  	}

  	printf ("\n");
}
#endif

static cdrom_drive *
open_cdda_device (MateVFSURI *uri)
{
	const char *device_name;
	cdrom_drive *drive;
	
	device_name = mate_vfs_uri_get_path (uri);

	drive = cdda_identify (device_name, FALSE, NULL);
	if (drive == NULL) {
		return NULL;
	} 

	/* Turn off verbosity */
	cdda_verbose_set (drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_FORGETIT);

	/* Open drive */
	switch (cdda_open (drive)) {
  		case -2:
  		case -3:
  		case -4:
  		case -5:
    		g_message ("Unable to open disc.  Is there an audio CD in the drive?");
    		return NULL;

  		case -6:
    		g_message ("CDDA method could not find a way to read audio from this drive.");
    		return NULL;
    			
  		case 0:
    		break;

  		default:
    		g_message ("Unable to open disc.");
    		return NULL;
	}

	return drive;
}

static gboolean
is_file_is_on_disc (CDDAContext *context, const MateVFSURI *uri)
{
	int index;
	char *base_name;
	char *escaped_name;
	
	if (context == NULL) {
		return FALSE;
	}

	base_name = mate_vfs_uri_extract_short_path_name (uri);
	escaped_name = mate_vfs_unescape_string_for_display (base_name);
	g_free (base_name);

	for (index = 0; index < context->drive->tracks; index++) {
		if (strcmp (escaped_name, context->disc_data.data_track[index].track_name) == 0) {
			g_free (escaped_name);
			return TRUE;
		}
	}

	g_free (escaped_name);
	return FALSE;
}

static MateVFSResult
get_file_info_for_basename (CDDAContext *context, const char *base_name)
{
	int index;

	g_assert (context);
	
	if (!context->use_cddb) {
		return MATE_VFS_ERROR_GENERIC;
	}
	
	/* Check and see if filename is in cddb data list */
	for (index = 0; index < context->drive->tracks; index++) {
		if (strcmp (base_name, context->disc_data.data_track[index].track_name) == 0) {
			/* Populate file info structure */
			context->file_info->io_block_size = CD_FRAMESIZE_RAW;			
			context->file_info->name = g_strdup (base_name);
			context->file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
			context->file_info->mime_type = g_strdup ("audio/x-wav");
			context->file_info->atime = time (NULL);
			context->file_info->ctime = time (NULL);
			context->file_info->mtime = time (NULL);
			context->file_info->size = get_data_size (context->drive, index + 1);
			context->file_info->valid_fields  = MATE_VFS_FILE_INFO_FIELDS_TYPE |
												MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
												MATE_VFS_FILE_INFO_FIELDS_SIZE |
												MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE |
												MATE_VFS_FILE_INFO_FIELDS_ATIME | 
												MATE_VFS_FILE_INFO_FIELDS_MTIME |
												MATE_VFS_FILE_INFO_FIELDS_CTIME;

			return MATE_VFS_OK;
		}
	}	
	return MATE_VFS_ERROR_GENERIC;
}

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context) 
{		
	cdrom_drive *drive;
	char *base_name;
	gboolean use_base, use_cache;
	MateVFSResult result;
	char *escaped_name;

	/*g_message ("do_get_file_info: %s", mate_vfs_uri_get_path (uri));*/
	
	use_base = FALSE;
	use_cache = FALSE;

	result = MATE_VFS_OK;
	
	/* Get basename */
	base_name = mate_vfs_uri_extract_short_path_name (uri);
	escaped_name = mate_vfs_unescape_string_for_display (base_name);
	g_free (base_name);

	/* Extract path and attempt to open */
	drive = open_cdda_device (uri);
	if (drive == NULL) {
			/* OK. We failed to open. Let's try the parent... */
			gchar *dirname, *schemedir, *sep;
			MateVFSURI *dir_uri;
			
			dirname = mate_vfs_uri_extract_dirname (uri);			
			schemedir = g_strdup_printf ("cdda://%s", dirname);
			
			/* Remove trailing '/' if there is one  */
			sep = strrchr (schemedir, '/');
			if (sep != NULL) {
				schemedir [strlen (schemedir) - 1] = '\0';
			}
			
			dir_uri = mate_vfs_uri_new (schemedir);
			drive = open_cdda_device (dir_uri);

			g_free (dirname);
			g_free (schemedir);
			mate_vfs_uri_unref (dir_uri);

			if (drive == NULL) {
				g_free (escaped_name);
				return MATE_VFS_ERROR_GENERIC;
			}

			use_base = TRUE;
	}

	/* Check and see if we already have opened and stashed this drive */
	if (!use_base) {
		if (global_context != NULL) {
#if defined(HAVE_CDDA_WITH_LIBCAM)
			if (strcmp (drive->dev->device_path, global_context->drive->dev->device_path) == 0) {
#else
			if (strcmp (drive->cdda_device_name, global_context->drive->cdda_device_name) == 0) {
#endif
				use_cache = TRUE;
				cdda_close (drive);
				mate_vfs_file_info_copy (file_info, global_context->file_info);
			} else {
				/* We have a new drive. */
				cdda_context_free (global_context);
				global_context = cdda_context_new (drive, uri);
				cdda_set_file_info_for_root (global_context, uri);
				mate_vfs_file_info_copy (file_info, global_context->file_info);
			}			
		} else {
			/* Create a new context */
			global_context = cdda_context_new (drive, uri);
			cdda_set_file_info_for_root (global_context, uri);
			mate_vfs_file_info_copy (file_info, global_context->file_info);
		}
	} else {
		cdda_context_free (global_context);
		global_context = cdda_context_new (drive, uri);
		result = get_file_info_for_basename (global_context, escaped_name);
		if (result == MATE_VFS_OK) {
			mate_vfs_file_info_copy (file_info, global_context->file_info);
		} else {
			cdda_context_free (global_context);
			global_context = NULL;
		}
	}

	g_free (escaped_name);
	return result;
}

static MateVFSResult 
do_open_directory (MateVFSMethod *method, MateVFSMethodHandle **method_handle,
		   MateVFSURI *uri, MateVFSFileInfoOptions options,
		   MateVFSContext *context)
{
	cdrom_drive *drive;
	gboolean use_base, use_cache;
	char *base_name;
	char *escaped_name;
	
	g_print ("do_open_directory () in uri: %s\n", mate_vfs_uri_get_path (uri));

	use_base = FALSE;
	use_cache = FALSE;
	
	/* Get basename */
	base_name = mate_vfs_uri_extract_short_path_name (uri);
	escaped_name = mate_vfs_unescape_string_for_display (base_name);
	g_free (base_name);

	/* Make sure we can open URI */
	drive = open_cdda_device (uri);
	if (drive == NULL) {								
			/* OK. We failed to open. Let's try the parent... */
			gchar *dirname, *schemedir, *sep;
			MateVFSURI *dir_uri;

			dirname = mate_vfs_uri_extract_dirname (uri);			
			schemedir = g_strdup_printf ("cdda://%s", dirname);
			
			/* Remove trailing '/' if there is one */
			sep = strrchr (schemedir, '/');
			if (sep != NULL) {
				schemedir [strlen (schemedir) - 1] = '\0';
			}
			
			dir_uri = mate_vfs_uri_new (schemedir);
			drive = open_cdda_device (dir_uri);

			g_free (dirname);
			g_free (schemedir);
			mate_vfs_uri_unref (dir_uri);

			if (drive == NULL) {
				g_free (escaped_name);
				return MATE_VFS_ERROR_GENERIC;
			}
			use_base = TRUE;
	}

	if (!use_base) {
		/* Check for cache */
		if (global_context != NULL) {
#if defined(HAVE_CDDA_WITH_LIBCAM)
				if (strcmp (drive->dev->device_path, global_context->drive->dev->device_path) != 0) {
#else
				if (strcmp (drive->cdda_device_name, global_context->drive->cdda_device_name) != 0) {
#endif
					/*	Clear old cache */
					cdda_context_free (global_context);
					global_context = cdda_context_new (drive, uri);
					cdda_set_file_info_for_root (global_context, uri);
				} else {
					/* g_message ("Using cache"); */
					cdda_close (drive);
				}
		} else {
			/* Allocate new context */
			global_context = cdda_context_new (drive, uri);
			cdda_set_file_info_for_root (global_context, uri);
		}
	} else {
		/* This is a file. Blast cache.*/
		/* g_message ("Use base: %s", escaped_name); */
		cdda_context_free (global_context);
		global_context = NULL;
		*method_handle = NULL;
		cdda_close (drive);
		g_free (escaped_name);
		return MATE_VFS_ERROR_GENERIC;
	}
	
	*method_handle = (MateVFSMethodHandle *) global_context;
	g_free (escaped_name);
	
	return MATE_VFS_OK;
}


static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context) 
{
	CDDAContext *cdda_context = (CDDAContext *)method_handle;

	/*g_message ("cdda do_close_directory");*/

	if (cdda_context == NULL) {
		/*g_message ("cdda do_close_directory: NULL cdda context");*/
		return MATE_VFS_ERROR_GENERIC;
	}
	
	cdda_context->access_count = 0;
		
	return MATE_VFS_OK;
}

static int
get_data_size (cdrom_drive *drive, int track)
{
	int minutes, seconds, total_seconds, size;

	size = 0;
	
	if (cdda_track_audiop (drive, track)) {
		long sec = cdda_track_firstsector (drive, track);
		long off = cdda_track_lastsector (drive, track) - sec + 1;

      		minutes = off / (60 * 75);
      		seconds = (off / 75) % 60;

		total_seconds = (minutes * 60) + seconds;
		size = ((total_seconds * 44) * 2 * 2) * 1024;
	}

	/*g_message ("get_data_size: %d", size);*/
	
	return size;
}

#if 0
static int
get_data_size_from_uri (MateVFSURI *uri, CDDAContext *context)
{
	int minutes, seconds, total_seconds, size, index;
	const char *base_name;
	
	size = -1;
	
	if (context == NULL) {
		return size;
	}

	base_name = mate_vfs_uri_extract_short_path_name (uri);
	
	/* Check and see if filename is in cddb data list */
	for (index = 0; index < context->drive->tracks; index++) {
		if (strcmp (base_name, context->disc_data.data_track[index].track_name) == 0) {	
			if (cdda_track_audiop (context->drive, index+1)) {
				long sec = cdda_track_firstsector (context->drive, index+1);
				long off = cdda_track_lastsector (context->drive, index+1) - sec + 1;

		      	minutes = off / (60 * 75);
		      	seconds = (off / 75) % 60;

				total_seconds = (minutes * 60) + seconds;
				size = ((total_seconds * 44) * 2 * 2) * 1024;
			}
			g_free (base_name);
			return size;
		}		
	}
	g_free (base_name);
	return size;
}
#endif
 
static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{

	CDDAContext *cdda_context = (CDDAContext *) method_handle;

	/*g_message ("cdda do_read_directory");*/
	
	if (cdda_context == NULL) {
		g_warning ("do_read_directory: NULL context");
		return MATE_VFS_ERROR_GENERIC;
	}

	if (cdda_context->access_count >= cdda_context->drive->tracks) {
		/*g_message ("do_read_directory: over access count");*/
		return MATE_VFS_ERROR_EOF;
	}

	cdda_context->access_count++;

	/* Populate file info */
	file_info->io_block_size = CD_FRAMESIZE_RAW;
	file_info->size = get_data_size (cdda_context->drive, cdda_context->access_count);		
	file_info->atime = time (NULL);
	file_info->ctime = time (NULL);
	file_info->mtime = time (NULL);
	if (cdda_context->use_cddb) {
		file_info->name = g_strdup (cdda_context->disc_data.data_track[cdda_context->access_count-1].track_name);
	} else {
		file_info->name = g_strdup_printf ("Untitled %d", cdda_context->access_count);
	}
	file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
	file_info->mime_type = g_strdup ("audio/x-wav");
	file_info->valid_fields = 	MATE_VFS_FILE_INFO_FIELDS_TYPE |
								MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE | 
								MATE_VFS_FILE_INFO_FIELDS_SIZE |
								MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE |
								MATE_VFS_FILE_INFO_FIELDS_ATIME | 
								MATE_VFS_FILE_INFO_FIELDS_MTIME |
	  							MATE_VFS_FILE_INFO_FIELDS_CTIME;

	return MATE_VFS_OK;
}

static MateVFSResult
do_check_same_fs (MateVFSMethod *method,
      MateVFSURI *a,
      MateVFSURI *b,
      gboolean *same_fs_return,
      MateVFSContext *context)
{
	*same_fs_return = TRUE;

	return MATE_VFS_OK;
}

static gboolean       
do_is_local (MateVFSMethod *method, const MateVFSURI *uri)
{
	return TRUE;
}


static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	do_open,
	NULL, 	/* do_create */
	do_close,
	do_read, /* do_read */
	NULL, 	/* do_write */
	NULL, 	/* seek */
	NULL, 	/* tell */
	NULL, 	/* truncate */
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	NULL,
	do_is_local,
	NULL, 	/* make directory */
	NULL, 	/* remove directory */
	NULL, 	/* rename */
	NULL, 	/* unlink */
	do_check_same_fs,
	NULL, 	/* do_set_file_info */
	NULL, 	/* do_truncate */
	NULL, 	/* do_find_directory */
	NULL 	/* do_create_symbolic_link */
};

#if 0
static void 
put_num (long num, int f, int endianness, int bytes)
{
	int i;
	unsigned char c;

	if (!endianness) {
		i = 0;
	} else {
		i = bytes - 1;
	}
		
	while (bytes--){
		c = (num >> (i << 3)) & 0xff;
		if (write (f, &c, 1) == -1) {
			perror ("Could not write to output.");
			exit (1);
		}

		if (endianness) {
			i--;
		} else {
			i++;
		}
	}
}
#endif


/* Write WAV header information into memory buffer */
#if 0
static int
write_wav_header (gpointer buffer, long bytes)
{
		char *ptr;

		memset (buffer, 0, CD_FRAMESIZE_RAW);
		
		ptr = buffer;
		
		*ptr = "RIFF"; ptr += 4;
		*ptr = bytes + 44 - 8; ptr += 4;
		*ptr = "WAVEfmt "; ptr += 8;
		*ptr = 16; ptr += 4;
		*ptr = 1; ptr += 2;
		*ptr = 2; ptr += 2;
		*ptr = 44100; ptr += 4;
		*ptr = 44100 * 2 * 2; ptr += 4;
		*ptr = 4; ptr += 2;
		*ptr = 16; ptr += 2;		
		*ptr = "data"; ptr += 4;
		*ptr = bytes; ptr += 4;

		return 44;
}
#endif

static int 
write_wav_header (gpointer buffer, long bytes)
{
		char *ptr;
		int var;

		memset (buffer, 0, CD_FRAMESIZE_RAW);
		
		ptr = buffer;
		
		memcpy (ptr, "RIFF", 4); ptr += 4;

		var = bytes + 44 - 8;
		memcpy (ptr, &var, 4); ptr += 4;
		
		memcpy (ptr, "WAVEfmt ", 8); ptr += 8;

		var = 16;
		memcpy (ptr, &var, 4); ptr += 4;

		var = 1;
		memcpy (ptr, &var, 2); ptr += 2;
		
		var = 2;
		memcpy (ptr, &var, 2); ptr += 2;

		var = 44100;
		memcpy (ptr, &var, 4); ptr += 4;

		var = 44100 * 2 * 2;
		memcpy (ptr, &var, 4); ptr += 4;

		var = 4;
		memcpy (ptr, &var, 2); ptr += 2;

		var = 16;
		memcpy (ptr, &var, 2); ptr += 2;
		
		memcpy (ptr, "data", 4); ptr += 4;
		
		memcpy (ptr, &bytes, 4); ptr += 4;

		return 44;
}

MateVFSMethod *
vfs_module_init (const char *method_name, 
		 const char *args)
{
	char *argv[] = { "mate-vfs-cdda-module", NULL };
	
	if (!mateconf_is_initialized ()) {
		mateconf_init (1, argv, NULL);
	}

	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
}
