/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* tar-method.c - The tar method implementation for the MATE Virtual File
   System.

   Copyright (C) 2002 Ximian, Inc

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

   Authors: Rachel Hestilow <hestilow@ximian.com>
   	    Abigail Brady <morwen@evilmagic.org> (tarpet.h) 
*/

#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-mime-utils.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-handle.h>
#include <libmatevfs/mate-vfs-file-info.h>
#include <libmatevfs/mate-vfs.h>
#include <string.h>
#include "tarpet.h"

typedef struct
{
	union TARPET_block* blocks;
	guint num_blocks;
	GNode *info_tree;
	int ref_count;
	gchar *filename;
} TarFile;

typedef struct
{
	TarFile *tar;
	union TARPET_block* start;
	union TARPET_block* current;
	int current_offset;
	int current_index;
	gchar *filename;
	gboolean is_directory;
} FileHandle;

#define TARPET_BLOCKSIZE (sizeof (union TARPET_block))

static GHashTable *tar_cache;
G_LOCK_DEFINE_STATIC (tar_cache);

#define iteration_initialize(condition, i, cond1, cond2) ((i) = (condition) ? (cond1) : (cond2))
#define iteration_check(condition, i, op1, op2, val1, val2) ((condition) ? (i op1 val1) : (i op2 val2))
#define iteration_iterate(condition, i, op1, op2) ((condition) ? (i op1) : (i op2))

#define parse_octal_field(v) (parse_octal ((v), sizeof (v)))
#define IS_OCTAL_DIGIT(c) ((c) >= '0' && (c) <= '8')
#define OCTAL_DIGIT(c) ((c) - '0')

static int parse_octal (const char *str, int len)
{
	int i, ret = 0;
	for (i = 0; i < len; i++)
	{
		if (str[i] == '\0') break;
		else if (!IS_OCTAL_DIGIT (str[i])) return 0;
		ret = ret * 8 + OCTAL_DIGIT (str[i]);
	}
	return ret;
}

static void
split_name_with_level (const gchar *name, gchar **first, gchar **last, int level, gboolean backwards)
{
	int i;
	gchar *found = NULL;
	int num_found = 0;
	if (name[strlen (name) - 1] == '/' && backwards)
		level++;

	for (iteration_initialize (backwards, i, strlen (name) - 1, 0);
	     iteration_check (backwards, i, >=, <, 0, strlen (name));
	     iteration_iterate (backwards, i, --, ++))
	{
		if (name[i] == '/')
			num_found++;
		
		if (num_found >= level)
		{
			found = (gchar*) name + i;
			break;
		}
	}

	if (found)
	{
		*first = g_strndup (name, found - name + 1);
		if (*(found + 1))
			*last = g_strdup (found + 1);
		else
			*last = NULL;
	}
	else
	{
		*first = g_strdup (name);
		*last = NULL;
	}
}

static void
split_name (const gchar *name, gchar **first, gchar **last)
{
	split_name_with_level (name, first, last, 1, TRUE);
}

static GNode*
real_lookup_entry (const GNode *tree, const gchar *name, int level)
{
	GNode *node, *ret = NULL;
	gchar *first, *rest;

	split_name_with_level (name, &first, &rest, level, FALSE);

	for (node = tree->children; node; node = node->next)
	{
		union TARPET_block *b = (union TARPET_block*) node->data;
		if (!strcmp (b->raw.data, first))
		{
			if (rest)
				ret = real_lookup_entry (node, name, level + 1);
			else
				ret = node;
			break;
		}
		else if (!strcmp (b->raw.data, name))
		{
			ret = node;
			break;
		}
	}
	g_free (first);
	g_free (rest);
	
	
	return ret;
}

static GNode*
tree_lookup_entry (const GNode *tree, const gchar *name)
{
	GNode *ret;
	char *root = g_strdup (name);
	char *txt = root;
	
	if (txt[0] == '/')
		txt++;

	ret = real_lookup_entry (tree, txt, 1);
	if (!ret && txt[strlen (txt) - 1] != '/')
	{
		txt = g_strconcat (txt, "/", NULL);
		g_free (root);
		root = txt;
		ret = real_lookup_entry (tree, txt, 1);
	}
	g_free (root);

	if (ret && ret != tree->children)
	{
		union TARPET_block *b = ret->data;
		b--;
		if (b->p.typeflag == TARPET_TYPE_LONGFILEN)
			ret = ret->next;
	}

	return ret;
}

static TarFile* read_tar_file (MateVFSHandle *handle)
{
	GArray *arr = g_array_new (TRUE, TRUE, sizeof (union TARPET_block));
	MateVFSResult res;
	TarFile* ret;
	MateVFSFileSize bytes_read;
	int i;
	
	do
	{
		union TARPET_block b;
		res = mate_vfs_read (handle, b.raw.data,
				      TARPET_BLOCKSIZE, &bytes_read);
		if (res == MATE_VFS_OK)
			g_array_append_val (arr, b);
	} while (res == MATE_VFS_OK && bytes_read > 0);

	ret = g_new0 (TarFile, 1);
	ret->blocks = (union TARPET_block*) arr->data;
	ret->num_blocks = arr->len;
	ret->info_tree = g_node_new (NULL); 

	for (i = 0; i < ret->num_blocks;)
	{
		gchar *dir;
		gchar *rest;
		GNode *node;
		int size = 0, maxsize;
		int orig;

		if (!(*ret->blocks[i].p.name))
		{
			i++;
			continue;
		}

		if (ret->blocks[i].p.typeflag == TARPET_TYPE_LONGFILEN)
		{
			i++;
			continue;
		}
		
		split_name (ret->blocks[i].p.name, &dir, &rest);
		node = tree_lookup_entry (ret->info_tree, dir);
		
		if (!node)
		{
			node = ret->info_tree;
		}
		g_node_append (node, g_node_new (&(ret->blocks[i])));
		
		g_free (dir);
		g_free (rest);
	
		maxsize = parse_octal_field (ret->blocks[i].p.size);
		if (maxsize)
		{
			for (orig = i; i < ret->num_blocks && size < maxsize; i++)
			{
				int wsize = TARPET_BLOCKSIZE;
				if ((maxsize - size) < TARPET_BLOCKSIZE)
					wsize = maxsize - size;
				size += wsize;
			}
			i++;
		}
		else
		{
			i++;
		}
	}
	
	g_array_free (arr, FALSE);

	return ret;
}

static TarFile* 
ensure_tarfile (MateVFSURI *uri)
{
	TarFile *tar;
	MateVFSHandle *handle;
	gchar *parent_string;

	parent_string = mate_vfs_uri_to_string (uri->parent, MATE_VFS_URI_HIDE_NONE);
	G_LOCK (tar_cache);
	tar = g_hash_table_lookup (tar_cache, parent_string);
	if (!tar)
	{
		if (mate_vfs_open_uri (&handle, uri->parent, MATE_VFS_OPEN_READ) != MATE_VFS_OK)
			return NULL;
		tar = read_tar_file (handle);
		tar->filename = parent_string;
		mate_vfs_close (handle);
		g_hash_table_insert (tar_cache, parent_string, tar);
	}
	G_UNLOCK (tar_cache);

	tar->ref_count++;
	return tar;
}

static void
tar_file_unref (TarFile *tar)
{
	tar->ref_count--;
	if (tar->ref_count < 0)
	{
		G_LOCK (tar_cache);
		g_hash_table_remove (tar_cache, tar->filename);
		G_UNLOCK (tar_cache);
		g_free (tar->blocks);
		g_node_destroy (tar->info_tree);
		g_free (tar->filename);
		g_free (tar);
	}
}

static MateVFSResult
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode mode,
	 MateVFSContext *context)
{	
	TarFile *tar;
	FileHandle *new_handle;
	GNode *node;
	union TARPET_block *start;
	int i;
		
	if (!uri->parent)
		return MATE_VFS_ERROR_INVALID_URI;
	
	tar = ensure_tarfile (uri);
	if (!tar)
		return MATE_VFS_ERROR_BAD_FILE;
	node = tree_lookup_entry (tar->info_tree, uri->text);
	if (!node)
	{
		tar_file_unref (tar);
		return MATE_VFS_ERROR_NOT_FOUND;
	}
	start = node->data;

	if (start->p.name[strlen (start->p.name) - 1] == '/')
		return MATE_VFS_ERROR_IS_DIRECTORY;
	new_handle = g_new0 (FileHandle, 1);
	new_handle->tar = tar;
	new_handle->filename = g_strdup (uri->text);
	new_handle->start = start;
	new_handle->current = new_handle->start;
	new_handle->current_offset = 0;
	for (i = 0; i < tar->num_blocks; i++)
		if (start == &(tar->blocks[i]))
			break;
	new_handle->current_index = i;
	new_handle->is_directory = FALSE;
	
	*method_handle = (MateVFSMethodHandle*) new_handle;
	
	return MATE_VFS_OK;
}

static void
file_handle_unref (FileHandle *handle)
{
	tar_file_unref (handle->tar);
	g_free (handle->filename);
	g_free (handle);
}

static MateVFSResult
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)
{
	FileHandle *handle = (FileHandle*) method_handle;

	file_handle_unref (handle);

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
	FileHandle *handle = (FileHandle*) method_handle;
	int i, size = 0, maxsize;
	
	if (handle->is_directory)
		return MATE_VFS_ERROR_IS_DIRECTORY;

	maxsize = parse_octal_field (handle->start->p.size);
	if (handle->current == handle->start)
	{
		handle->current_index++;
		handle->current_offset = TARPET_BLOCKSIZE;
	}

	for (i = handle->current_index; i < handle->tar->num_blocks && handle->current_offset < (maxsize + TARPET_BLOCKSIZE) && size < num_bytes; i++)
	{
		int wsize = TARPET_BLOCKSIZE;
		gpointer target_buf = (gchar*)buffer + size;
		if ((maxsize - (handle->current_offset - TARPET_BLOCKSIZE)) < TARPET_BLOCKSIZE
		   && (maxsize - (handle->current_offset - TARPET_BLOCKSIZE)) > 0)
			wsize = maxsize - handle->current_offset + TARPET_BLOCKSIZE;
		else if (num_bytes < (size + wsize))
			wsize = num_bytes - size;
		else
			handle->current_index = i + 1;
		
		memcpy (target_buf, handle->start->raw.data + handle->current_offset, wsize);
		
		size += wsize;
		handle->current_offset += wsize;
	}
	if (handle->current_index < handle->tar->num_blocks)
		handle->current = &handle->tar->blocks[handle->current_index]; 
	else
		handle->current = NULL;
	
	*bytes_read = size;
	return MATE_VFS_OK;
}

static MateVFSResult
do_seek (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition whence,
	 MateVFSFileOffset offset,
	 MateVFSContext *context)
{
	FileHandle *handle = (FileHandle*) method_handle;
	MateVFSFileOffset current_offset;

	switch (whence)
	{
	case MATE_VFS_SEEK_START:
		current_offset = 0;
		break;
	case MATE_VFS_SEEK_CURRENT:
		current_offset = handle->current_offset;
		break;
	case MATE_VFS_SEEK_END:
		current_offset = parse_octal_field (handle->start->p.size);
		break;
	default:
		current_offset = handle->current_offset;
		break;
	}

	handle->current_offset = current_offset + offset;
	return MATE_VFS_OK;
}

static MateVFSResult
do_tell (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize *offset_return)
{
	FileHandle *handle = (FileHandle*) method_handle;
	*offset_return = handle->current_offset;
	return MATE_VFS_OK;
}

static MateVFSResult
do_open_directory (MateVFSMethod *method,
  	 	   MateVFSMethodHandle **method_handle,
	 	   MateVFSURI *uri,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context)
{
	TarFile *tar;
	FileHandle *new_handle;
	union TARPET_block *start, *current;
	GNode *node;
	int i;

	if (!uri->parent)
		return MATE_VFS_ERROR_INVALID_URI;
	tar = ensure_tarfile (uri);
	if (uri->text)
	{
		node = tree_lookup_entry (tar->info_tree, uri->text);
		if (!node)
		{
			tar_file_unref (tar);
			return MATE_VFS_ERROR_NOT_FOUND;
		}
		start = node->data;
		
		if (start->p.name[strlen (start->p.name) - 1] != '/')
			return MATE_VFS_ERROR_NOT_A_DIRECTORY;

		if (node->children)
			current = node->children->data; 
		else
			current = NULL;
	}
	else
	{
		node = tar->info_tree;
		if (!node)
		{
			tar_file_unref (tar);
			return MATE_VFS_ERROR_NOT_FOUND;
		}

		if (node->children)
			start = node->children->data;
		else
			start = NULL;
		current = start;
	}
	
	new_handle = g_new0 (FileHandle, 1);
	new_handle->tar = tar;
	new_handle->filename = g_strdup (tar->filename);
	new_handle->start = start;
	new_handle->current = current;
	for (i = 0; i < tar->num_blocks; i++)
		if (start == &(tar->blocks[i]))
			break;
	new_handle->current_index = i;
	new_handle->is_directory = TRUE;

	*method_handle = (MateVFSMethodHandle*) new_handle;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context)
{
	FileHandle *handle = (FileHandle*) method_handle;

	file_handle_unref (handle);

	return MATE_VFS_OK;
}

static MateVFSResult 
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context)
{
	TarFile *tar;
	GNode *node;
	union TARPET_block *current;
	gchar *name;
	gchar *path;
	char *mime_type;
	int i;

	if (!uri->parent)
		return MATE_VFS_ERROR_INVALID_URI;

	tar = ensure_tarfile (uri);
	
	if (uri->text)
		node = tree_lookup_entry (tar->info_tree, uri->text);
	else
		node = tar->info_tree->children;
	
	if (!node)
	{
		tar_file_unref (tar);
		return MATE_VFS_ERROR_NOT_FOUND;
	}

	current = node->data;
	for (i = 0; i < tar->num_blocks; i++)
		if (&(tar->blocks[i]) == current)
			break;
	if (i && tar->blocks[i - 2].p.typeflag == TARPET_TYPE_LONGFILEN)
		name = g_strdup (tar->blocks[i - 1].raw.data);
	else
		name = g_strdup (current->p.name);

	file_info->name = g_path_get_basename (name);
	if (name[strlen (name) - 1] == '/')
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
	else if (current->p.typeflag == TARPET_TYPE_SYMLINK)
	{
		file_info->type = MATE_VFS_FILE_TYPE_SYMBOLIC_LINK;
		file_info->symlink_name = g_strdup (current->p.linkname);
	}
	else
		file_info->type = MATE_VFS_FILE_TYPE_REGULAR;

	file_info->permissions = parse_octal_field (current->p.mode);
	file_info->uid = parse_octal_field (current->p.uid);
	file_info->gid = parse_octal_field (current->p.gid);
	file_info->size = parse_octal_field (current->p.size);
	file_info->mtime = parse_octal_field (current->p.mtime);
	file_info->atime = parse_octal_field (current->gnu.atime);
	file_info->ctime = parse_octal_field (current->gnu.ctime);

	if (file_info->type == MATE_VFS_FILE_TYPE_DIRECTORY)
		mime_type = "x-directory/normal";
	else if (!(options & MATE_VFS_FILE_INFO_FOLLOW_LINKS) && file_info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK)
		mime_type = "x-special/symlink";
	else if (!file_info->size || (options & MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE))
	{
		path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);
		mime_type = (char*) mate_vfs_get_file_mime_type (path, NULL, TRUE);
		g_free (path);
	}
	else
	{
		mime_type = (char*) mate_vfs_get_mime_type_for_data ((current + 1)->raw.data, MIN (TARPET_BLOCKSIZE, file_info->size));
		if (!mime_type)
		{
			path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);
			mime_type = (char*) mate_vfs_get_file_mime_type (path, NULL, TRUE);
			g_free (path);
		}
	}

	file_info->mime_type = g_strdup (mime_type);

	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_TYPE |
			   	  MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS |
			   	  MATE_VFS_FILE_INFO_FIELDS_SIZE |
			   	  MATE_VFS_FILE_INFO_FIELDS_ATIME |
			   	  MATE_VFS_FILE_INFO_FIELDS_MTIME |
			   	  MATE_VFS_FILE_INFO_FIELDS_CTIME |
			   	  MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
				  MATE_VFS_FILE_INFO_FIELDS_IDS;

	g_free (name);
	tar_file_unref (tar);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{
	FileHandle *handle = (FileHandle*) method_handle;
	MateVFSURI *uri;
	gchar *str;
	GNode *node;
	
	if (!handle->current)
		return MATE_VFS_ERROR_EOF;

	str = g_strconcat (handle->filename, "#tar:", handle->current->p.name, NULL);
	uri = mate_vfs_uri_new (str);
	do_get_file_info (method, uri, file_info, 0, context);
	node = tree_lookup_entry (handle->tar->info_tree, uri->text);
	if (!node)
	{
		mate_vfs_uri_unref (uri);
		return MATE_VFS_ERROR_NOT_FOUND;
	}
	
	if (node->next)
		handle->current = node->next->data;
	else
		handle->current = NULL;
	mate_vfs_uri_unref (uri);

	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
			      MateVFSMethodHandle *method_handle,
			      MateVFSFileInfo *file_info,
			      MateVFSFileInfoOptions options,
			      MateVFSContext *context)
{
	FileHandle *handle = (FileHandle*) method_handle;
	MateVFSURI *uri;
	
	uri = mate_vfs_uri_new (handle->start->p.name);
	do_get_file_info (method, uri, file_info, options, context);
	mate_vfs_uri_unref (uri);

	return MATE_VFS_OK;
}

static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	return mate_vfs_uri_is_local (uri->parent);
}

static MateVFSMethod method =
{
	sizeof (MateVFSMethod),
	do_open,
	NULL,
	do_close,
	do_read,
	NULL,
	do_seek,
	do_tell,
	NULL,
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	do_get_file_info_from_handle,
	do_is_local,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	G_LOCK (tar_cache);
	tar_cache = g_hash_table_new (g_str_hash, g_str_equal);
	G_UNLOCK (tar_cache);
	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
	G_LOCK (tar_cache);
	g_hash_table_destroy (tar_cache);
	G_UNLOCK (tar_cache);
}
