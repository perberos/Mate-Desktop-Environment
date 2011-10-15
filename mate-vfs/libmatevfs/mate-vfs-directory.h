/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-directory.h - Directory handling for the MATE Virtual
   File System.

   Copyright (C) 1999 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#ifndef MATE_VFS_DIRECTORY_H
#define MATE_VFS_DIRECTORY_H

#include <glib.h>
#include <libmatevfs/mate-vfs-file-info.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MateVFSDirectoryHandle MateVFSDirectoryHandle;

/**
 * MateVFSDirectoryVisitOptions:
 * @MATE_VFS_DIRECTORY_VISIT_DEFAULT: Default options, i.e. call
 * the specified #MateVFSDirectoryVisitFunc for each file.
 * @MATE_VFS_DIRECTORY_VISIT_SAMEFS: Visit only directories on the same
 * file system as the parent
 * @MATE_VFS_DIRECTORY_VISIT_LOOPCHECK: Loop prevention. If this
 * is set, and a file is found to be a directory referencing a prefiously
 * found directory inode (i.e. already used for one of it's parents), this
 * is considered a recursion loop, and #MateVFSDirectoryVisitFunc will
 * be notified using its @recursing_will_loop parameter. If this is not
 * set, the #MateVFSDirectoryVisitFunc's @recursing_will_loop parameter
 * will always be set to %FALSE.
 *
 * These options control the way in which directories are visited. They are
 * passed to mate_vfs_directory_visit(), mate_vfs_directory_visit_uri()
 * mate_vfs_directory_visit_files() and
 * mate_vfs_directory_visit_files_at_uri().
 **/
typedef enum {
	MATE_VFS_DIRECTORY_VISIT_DEFAULT = 0,
	MATE_VFS_DIRECTORY_VISIT_SAMEFS = 1 << 0,
	MATE_VFS_DIRECTORY_VISIT_LOOPCHECK = 1 << 1,
	MATE_VFS_DIRECTORY_VISIT_IGNORE_RECURSE_ERROR = 1 << 2
} MateVFSDirectoryVisitOptions;

/**
 * MateVFSDirectoryVisitFunc:
 * @rel_path: A char * specifying the path of the currently visited
 * 	      file relative to the base directory for the visit.
 * @info: The #MateVFSFileInfo of the currently visited file.
 * @recursing_will_loop: Whether setting *@recurse to %TRUE will cause
 * 			 a loop, i.e. whether this is a link
 * 			 pointing to a parent directory.
 * @user_data: The user data passed to mate_vfs_directory_visit(),
 * 	       mate_vfs_directory_visit_uri(), mate_vfs_directory_visit_files() or
 * 	       mate_vfs_directory_visit_files_at_uri().
 * @recurse: A valid pointer to a #gboolean, which points to %FALSE by
 * 	     default and can be modified to point to %TRUE, which indicates that
 * 	     the currently considered file should be visited recursively.
 * 	     The recursive visit will only be actually done if @info refers to a directory,
 * 	     *@recurse is %TRUE and the return value of the #MateVFSDirectoryVisitFunc is %TRUE.
 * 	     *@recurse should usually not be set to %TRUE if @recursing_will_loop is %TRUE.
 *
 * This function is passed to mate_vfs_directory_visit(),
 * mate_vfs_directory_visit_uri(), mate_vfs_directory_visit_files() and
 * mate_vfs_directory_visit_files_at_uri(), and is called for each
 * file in the specified directory.
 *
 * <note>
 *  When a recursive visit was requested for a particular directory, errors
 *  during the child visit will lead to a cancellation of the overall visit.
 *  Thus, you must ensure that the user has sufficient access rights to visit
 *  a directory before requesting a recursion.
 * </note>
 *
 * Returns: %TRUE if visit should be continued, %FALSE otherwise.
 **/
typedef gboolean (* MateVFSDirectoryVisitFunc)	 (const gchar *rel_path,
						  MateVFSFileInfo *info,
						  gboolean recursing_will_loop,
						  gpointer user_data,
						  gboolean *recurse);

MateVFSResult	mate_vfs_directory_open
					(MateVFSDirectoryHandle **handle,
					 const gchar              *text_uri,
					 MateVFSFileInfoOptions   options);
MateVFSResult	mate_vfs_directory_open_from_uri
					(MateVFSDirectoryHandle **handle,
					 MateVFSURI              *uri,
					 MateVFSFileInfoOptions   options);
MateVFSResult	mate_vfs_directory_read_next
					(MateVFSDirectoryHandle  *handle,
					 MateVFSFileInfo         *file_info);
MateVFSResult	mate_vfs_directory_close
					(MateVFSDirectoryHandle  *handle);


MateVFSResult  mate_vfs_directory_visit
					(const gchar *text_uri,
					 MateVFSFileInfoOptions info_options,
					 MateVFSDirectoryVisitOptions visit_options,
					 MateVFSDirectoryVisitFunc callback,
					 gpointer data);

MateVFSResult  mate_vfs_directory_visit_uri
					(MateVFSURI *uri,
					 MateVFSFileInfoOptions info_options,
					 MateVFSDirectoryVisitOptions visit_options,
					 MateVFSDirectoryVisitFunc callback,
					 gpointer data);

MateVFSResult	mate_vfs_directory_visit_files
					(const gchar *text_uri,
					 GList *file_list,
					 MateVFSFileInfoOptions info_options,
					 MateVFSDirectoryVisitOptions visit_options,
					 MateVFSDirectoryVisitFunc callback,
					 gpointer data);

MateVFSResult	mate_vfs_directory_visit_files_at_uri
					(MateVFSURI *uri,
					 GList *file_list,
					 MateVFSFileInfoOptions info_options,
					 MateVFSDirectoryVisitOptions visit_options,
					 MateVFSDirectoryVisitFunc callback,
					 gpointer data);

MateVFSResult mate_vfs_directory_list_load
					(GList **list,
				         const gchar *text_uri,
				         MateVFSFileInfoOptions options);

#ifdef __cplusplus
}
#endif

#endif
