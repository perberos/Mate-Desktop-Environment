/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-file-info.c - Handling of file information for the MATE
   Virtual File System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#include <config.h>
#include "mate-vfs-file-info.h"

#include <glib.h>
#include <string.h>

/* Mutex for making MateVFSFileInfo ref's/unref's atomic */
/* Note that an atomic increment function (such as is present in NSPR) is preferable */
/* FIXME: This mutex is probably not needed and might be causing performance issues */
static GStaticMutex file_info_ref_lock = G_STATIC_MUTEX_INIT;

/* Register MateVFSFileInfo in the type system */
GType 
mate_vfs_file_info_get_type (void) {
	static GType fi_type = 0;

	if (fi_type == 0) {
		fi_type = g_boxed_type_register_static ( "MateVFSFileInfo",
				(GBoxedCopyFunc) mate_vfs_file_info_dup,
				(GBoxedFreeFunc) mate_vfs_file_info_unref);
	}

	return fi_type;
}


/**
 * mate_vfs_file_info_new:
 * 
 * Allocate and initialize a new #MateVFSFileInfo struct.
 * 
 * Returns: a pointer to the newly allocated file information struct.
 */
MateVFSFileInfo *
mate_vfs_file_info_new (void)
{
	MateVFSFileInfo *new;

	new = g_new0 (MateVFSFileInfo, 1);

	/* `g_new0()' is enough to initialize everything (we just want
           all the members to be set to zero).  */

	new->refcount = 1;
	
	return new;
}


/**
 * mate_vfs_file_info_ref:
 * @info: pointer to a file information struct.
 * 
 * Increment refcount of @info by 1.
 */
void
mate_vfs_file_info_ref (MateVFSFileInfo *info)
{
	g_return_if_fail (info != NULL);
	g_return_if_fail (info->refcount > 0);

	g_static_mutex_lock (&file_info_ref_lock);
	info->refcount += 1;
	g_static_mutex_unlock (&file_info_ref_lock);
	
}

/**
 * mate_vfs_file_info_unref:
 * @info: pointer to a file information struct.
 * 
 * Decreases the refcount of @info by 1. Frees the struct @info if refcount becomes 0.
 */
void
mate_vfs_file_info_unref (MateVFSFileInfo *info)
{
	g_return_if_fail (info != NULL);
	g_return_if_fail (info->refcount > 0);

	g_static_mutex_lock (&file_info_ref_lock);
	info->refcount -= 1;
	g_static_mutex_unlock (&file_info_ref_lock);

	if (info->refcount == 0) {
		mate_vfs_file_info_clear (info);
		g_free (info);
	}
}


/**
 * mate_vfs_file_info_clear:
 * @info: pointer to a file information struct.
 * 
 * Clear @info so that it's ready to accept new data. This is
 * supposed to be used when @info already contains meaningful information which
 * we want to replace.
 */
void
mate_vfs_file_info_clear (MateVFSFileInfo *info)
{
	guint old_refcount;
	
	g_return_if_fail (info != NULL);

	g_free (info->name);
	g_free (info->symlink_name);
	g_free (info->mime_type);
	g_free (info->selinux_context);

	/* Ensure the ref count is maintained correctly */
	g_static_mutex_lock (&file_info_ref_lock);

	old_refcount = info->refcount;
	memset (info, 0, sizeof (*info));
	info->refcount = old_refcount;

	g_static_mutex_unlock (&file_info_ref_lock);
}


/**
 * mate_vfs_file_info_get_mime_type:
 * @info: a pointer to a file information struct.
 * 
 * Retrieve MIME type from @info. There is no need to free the return
 * value.
 * 
 * Returns: a pointer to a string representing the MIME type.
 */
const gchar *
mate_vfs_file_info_get_mime_type (MateVFSFileInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->mime_type;
}

/**
 * mate_vfs_file_info_copy:
 * @dest: pointer to a struct to copy @src's information into.
 * @src: pointer to the information to be copied into @dest.
 * 
 * Copy information from @src into @dest.
 */
void
mate_vfs_file_info_copy (MateVFSFileInfo *dest,
			  const MateVFSFileInfo *src)
{
	guint old_refcount;

	g_return_if_fail (dest != NULL);
	g_return_if_fail (src != NULL);

	/* The primary purpose of this lock is to guarentee that the
	 * refcount is correctly maintained, not to make the copy
	 * atomic.  If you want to make the copy atomic, you probably
	 * want serialize access differently (or perhaps you shouldn't
	 * use copy)
	 */
	g_static_mutex_lock (&file_info_ref_lock);

	old_refcount = dest->refcount;

	/* Copy basic information all at once; we will fix pointers later.  */

	memcpy (dest, src, sizeof (*src));

	/* Duplicate dynamically allocated strings.  */

	dest->name = g_strdup (src->name);
	dest->symlink_name = g_strdup (src->symlink_name);
	dest->mime_type = g_strdup (src->mime_type);
	dest->selinux_context = g_strdup (src->selinux_context);

	dest->refcount = old_refcount;

	g_static_mutex_unlock (&file_info_ref_lock);

}

/**
 * mate_vfs_file_info_dup:
 * @orig: pointer to a file information structure to duplicate.
 * 
 * Duplicates @orig and returns it.
 * 
 * Returns: a new file information struct that duplicates the information in @orig.
 */
MateVFSFileInfo *
mate_vfs_file_info_dup (const MateVFSFileInfo *orig)
{
	MateVFSFileInfo * ret;

	g_return_val_if_fail (orig != NULL, NULL);

	ret = mate_vfs_file_info_new();

	mate_vfs_file_info_copy (ret, orig);

	return ret;
}


static gboolean
mime_matches (char *a, char *b)
{
	if (a == NULL && b == NULL) {
		return TRUE;
	} else if ((a != NULL && b == NULL) ||
		   (a == NULL && b != NULL)) {
		return FALSE;
	} else {
		g_assert (a != NULL && b != NULL);
		return g_ascii_strcasecmp (a, b) == 0;
	}
}

static gboolean
symlink_name_matches (char *a, char *b)
{
	if (a == NULL && b == NULL) {
		return TRUE;
	} else if ((a != NULL && b == NULL) ||
		   (a == NULL && b != NULL)) {
		return FALSE;
	} else {
		g_assert (a != NULL && b != NULL);
		return strcmp (a, b) == 0;
	}
}

static gboolean
selinux_context_matches (char *a, char*b) 
{
	return symlink_name_matches (a, b);
}

/**
 * mate_vfs_file_info_matches:
 * @a: first #MateVFSFileInfo struct to compare.
 * @b: second #MateVFSFileInfo struct to compare.
 *
 * Compare the two file info structs, return %TRUE if they match exactly
 * the same file data.
 *
 * Returns: %TRUE if the two #MateVFSFileInfos match, otherwise return %FALSE.
 */
gboolean
mate_vfs_file_info_matches (const MateVFSFileInfo *a,
			     const MateVFSFileInfo *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);
	g_return_val_if_fail (a->name != NULL, FALSE);
	g_return_val_if_fail (b->name != NULL, FALSE);

	/* This block of code assumes that the MateVfsFileInfo 
	   was correctly allocated with g_new0 which means that each pair
	   of fields are either invalid and equal to NULL or are valid 
	   If both MateVfsFileInfos have only invalid fields, the 
	   function returns TRUE. That is, it says the infos match which is,
	   in a sense, true :)
	*/

	if (a->type != b->type
	    || a->size != b->size
	    || a->block_count != b->block_count
	    || a->atime != b->atime
	    || a->mtime != b->mtime
	    || a->ctime != b->ctime
	    || a->flags != b->flags
	    || a->permissions != b->permissions
	    || a->device != b->device
	    || a->inode != b->inode
	    || a->link_count != b->link_count
	    || a->uid != b->uid
	    || a->gid != b->gid
	    || strcmp (a->name, b->name) != 0
	    || !selinux_context_matches (a->selinux_context, b->selinux_context)
	    || !mime_matches (a->mime_type, b->mime_type)
	    || !symlink_name_matches (a->symlink_name, b->symlink_name)) {
		return FALSE;
	} else {
		return TRUE;
	}
}

/**
 * mate_vfs_file_info_list_ref:
 * @list: list of #MateVFSFileInfo elements.
 *
 * Increments the refcount of the items in @list by one.
 *
 * Return value: @list.
 */
GList *
mate_vfs_file_info_list_ref (GList *list)
{
	g_list_foreach (list, (GFunc) mate_vfs_file_info_ref, NULL);
	return list;
}

/**
 * mate_vfs_file_info_list_unref:
 * @list: list of #MateVFSFileInfo elements.
 *
 * Decrements the refcount of the items in @list by one.
 * Note that the list is *not freed* even if each member of the list
 * is freed.
 * Return value: @list.
 */
GList *
mate_vfs_file_info_list_unref (GList *list)
{
	g_list_foreach (list, (GFunc) mate_vfs_file_info_unref, NULL);
	return list;
}

/**
 * mate_vfs_file_info_list_copy:
 * @list: list of #MateVFSFileInfo elements.
 *
 * Creates a duplicate of @list, and references each member of
 * that list.
 *
 * Return value: a newly referenced duplicate of @list.
 */
GList *
mate_vfs_file_info_list_copy (GList *list)
{
	return g_list_copy (mate_vfs_file_info_list_ref (list));
}

/**
 * mate_vfs_file_info_list_free:
 * @list: list of #MateVFSFileInfo elements.
 *
 * Decrements the refcount of each member of @list by one,
 * and frees the list itself.
 */
void
mate_vfs_file_info_list_free (GList *list)
{
	g_list_free (mate_vfs_file_info_list_unref (list));
}


/* Register MateVfsGetFileInfoResult into the GType system  */
GType
mate_vfs_get_file_info_result_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0) {
	    our_type = g_boxed_type_register_static ("MateVfsGetFileInfoResult",
	        (GBoxedCopyFunc) mate_vfs_get_file_info_result_dup,
	        (GBoxedFreeFunc) mate_vfs_get_file_info_result_free);
	}
	    
	return our_type;
}
/**
 * mate_vfs_get_file_info_result_dup:
 * @result: a #MateVFSGetFileInfoResult.
 * 
 * Duplicate @result.
 *
 * Note: The internal uri and fileinfo objects are not duplicated
 * but their refcount is incremented by 1.
 *
 * Return value: a duplicated version of @result.
 *
 * Since: 2.12
 */
MateVFSGetFileInfoResult*
mate_vfs_get_file_info_result_dup (MateVFSGetFileInfoResult *result)
{
	MateVFSGetFileInfoResult* copy;
	
	g_return_val_if_fail (result != NULL, NULL);

	copy = g_new0 (MateVFSGetFileInfoResult, 1);

	/* "Copy" and ref the objects: */
	copy->uri = result->uri;
	mate_vfs_uri_ref (copy->uri);

	copy->result = result->result;

	copy->file_info = result->file_info;
	mate_vfs_file_info_ref (copy->file_info);

	return copy;
}

/**
 * mate_vfs_get_file_info_result_free:
 * @result: a #MateVFSGetFileInfoResult.
 *
 * Unrefs the internal uri and fileinfo objects and frees the
 * memory allocated for @result.
 * 
 * Since: 2.12
 */
void
mate_vfs_get_file_info_result_free (MateVFSGetFileInfoResult *result)
{
	g_return_if_fail (result != NULL);
 
	mate_vfs_uri_unref (result->uri);
	result->uri = NULL;

	mate_vfs_file_info_unref (result->file_info);
	result->file_info = NULL;

	g_free (result);
}

