/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-storage-memory.c: Memory based MateComponent::Storage implementation
 *
 * Author:
 *   ÉRDI Gergõ <cactus@cactus.rulez.org>
 *
 * Copyright 2001 Gergõ Érdi
 *
 * TODO:
 *  * Make it implement PersistStream so you can flatten a
 *    StorageMem into a Stream
 *  * Create a subclass that supports commit/revert
 */
#include <config.h>
#include <string.h>

#include <matecomponent/matecomponent-storage-memory.h>
#include <matecomponent/matecomponent-stream-memory.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-storage.h>

static MateComponentObjectClass *matecomponent_storage_mem_parent_class;

typedef struct {
	gboolean      is_directory;
	MateComponentObject *child;
} MateComponentStorageMemEntry;

struct _MateComponentStorageMemPriv {
	GHashTable *entries;
};

typedef struct {
	GList                    *list;
	MateComponent_StorageInfoFields  mask;
} DirCBData;

static void
matecomponent_storage_mem_entry_free (gpointer data)
{
	MateComponentStorageMemEntry *entry = (MateComponentStorageMemEntry*) data;

	if (!entry)
		return;
	
	matecomponent_object_unref (entry->child);
	
	g_free (entry);
}

static MateComponentStorageMemEntry *
matecomponent_storage_mem_entry_dup (MateComponentStorageMemEntry *entry)
{
	MateComponentStorageMemEntry *ret_val = g_new0 (MateComponentStorageMemEntry, 1);

	ret_val->is_directory = entry->is_directory;
	ret_val->child = entry->child;

	matecomponent_object_ref (ret_val->child);

	return ret_val;
}

static void
split_path (const char  *path,
	    char       **path_head,
	    char       **path_tail)
{
	gchar **path_parts;
	
	if (g_path_is_absolute (path))
		path = g_path_skip_root (path);
	
	path_parts = g_strsplit (path, "/", 2);
	
	*path_head = path_parts[0];
	*path_tail = path_parts[1];

	g_free (path_parts);	
}

static MateComponentStorageMem *
smem_get_parent (MateComponentStorageMem       *storage,
		 const char             *path,
		 char                  **filename,  /* g_free this */
		 MateComponentStorageMemEntry **ret_entry) /* g_free this */
{
	MateComponentStorageMem      *ret;
	MateComponentStorageMemEntry *entry;
	gchar                 *path_head, *path_tail;

	if (!strcmp (path, "/") || !strcmp (path, "")) {
		if (filename)
			*filename = g_strdup ("/");
		if (ret_entry) {
			*ret_entry = g_new0 (MateComponentStorageMemEntry, 1);
			(*ret_entry)->is_directory = TRUE;
			(*ret_entry)->child = MATECOMPONENT_OBJECT (storage);
			matecomponent_object_ref ((*ret_entry)->child);
		}

		return storage;
	}

	split_path (path, &path_head, &path_tail);
	entry = g_hash_table_lookup (storage->priv->entries,
				     path_head);

	/* No child is found */
	if (!entry) {
		g_free (path_head);

		if (filename)
			*filename = path_tail;

		if (ret_entry)
			*ret_entry = NULL;
		
		return NULL;
	}

	/* This is not the immediate parent */
	if (path_tail && entry->is_directory) {
		ret = smem_get_parent (
			MATECOMPONENT_STORAGE_MEM (entry->child),
			path_tail,
			filename,
			ret_entry);
		
		g_free (path_head);
		g_free (path_tail);
		return ret;
	}

	/* This is the immediate parent */
	if (filename)
		*filename = g_strdup (path_head);
	if (ret_entry)
		*ret_entry = matecomponent_storage_mem_entry_dup (entry);
	
	g_free (path_tail);
	g_free (path_head);
	
	return storage;
}

static MateComponentStorageMem *
smem_get_last_storage (MateComponentStorageMem  *storage,
		       const char        *path,
		       char             **last_path)
{
	MateComponentStorageMem      *ret;
	MateComponentStorageMemEntry *entry;
	gchar *path_head, *path_tail;

	if (!strcmp (path, "/") || !strcmp (path, "")) {
		if (last_path)
			*last_path = NULL;
		return storage;
	}
	
	split_path (path, &path_head, &path_tail);
	entry = g_hash_table_lookup (storage->priv->entries,
				     path_head);

	/* No appropriate child is found */
	if (!entry) {
		if (path_tail) {
			g_free (path_head);
			g_free (path_tail);

			if (last_path)
				*last_path = NULL;
			return NULL;
		} else {
			if (last_path)
				*last_path = path_head;
			
			return storage;
		}
	}

	if (!path_tail) {
		if (entry->is_directory) {
			g_free (path_head);

			if (last_path)
				*last_path = NULL;
			
			return MATECOMPONENT_STORAGE_MEM (entry->child);
		} else {
			if (last_path)
				*last_path = path_head;
			
			return storage;
		}
	}

	if (path_tail && entry->is_directory) {
		ret = smem_get_last_storage (
			MATECOMPONENT_STORAGE_MEM (entry->child),
			path_tail, last_path);
		g_free (path_head);
		g_free (path_tail);
		return ret;
	}

	g_free (path_tail);
	g_free (path_head);

	if (last_path)
		*last_path = NULL;
	
	return NULL;
}

static MateComponent_StorageInfo *
smem_get_stream_info (MateComponentObject                   *stream,
		      const MateComponent_StorageInfoFields  mask,
		      CORBA_Environment              *ev)
{
	MateComponent_StorageInfo *ret_val;
	CORBA_Environment   my_ev;
	
	CORBA_exception_init (&my_ev);
	ret_val = MateComponent_Stream_getInfo (matecomponent_object_corba_objref (stream),
					 mask, &my_ev);
	
	if (MATECOMPONENT_EX (&my_ev)) {
		if (MATECOMPONENT_USER_EX (&my_ev, ex_MateComponent_Stream_IOError))
			matecomponent_exception_set (ev, ex_MateComponent_Storage_IOError);
		if (MATECOMPONENT_USER_EX (&my_ev, ex_MateComponent_Stream_NoPermission))
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NoPermission);
		if (MATECOMPONENT_USER_EX (&my_ev, ex_MateComponent_Stream_NotSupported))
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NotSupported);
	}
	
	if (mask & MateComponent_FIELD_TYPE)
		ret_val->type = MateComponent_STORAGE_TYPE_REGULAR;
	
	CORBA_exception_free (&my_ev);

	return ret_val;
}

static void
smem_dir_hash_cb (gpointer key,
		  gpointer value,
		  gpointer user_data)
{
	DirCBData                *cb_data = user_data;
	gchar                    *filename = key;
	MateComponentStorageMemEntry    *entry = value;
	MateComponent_StorageInfo       *info;
	MateComponent_StorageInfoFields  mask = cb_data->mask;
	
	if (entry->is_directory) {
		info = MateComponent_StorageInfo__alloc ();
		info->name = CORBA_string_dup (filename);
		info->type = MateComponent_STORAGE_TYPE_DIRECTORY;
	} else {
		if (mask & MateComponent_FIELD_CONTENT_TYPE ||
		    mask & MateComponent_FIELD_SIZE) {
			CORBA_Environment my_ev;
			
			CORBA_exception_init (&my_ev);
			info = smem_get_stream_info (entry->child, mask, &my_ev);
			CORBA_exception_free (&my_ev);
		}
		else
			info = MateComponent_StorageInfo__alloc ();

		info->name = CORBA_string_dup (filename);
		info->type = MateComponent_STORAGE_TYPE_REGULAR;
	}

	cb_data->list = g_list_prepend (cb_data->list, info);
}

static MateComponent_StorageInfo*
smem_get_info_impl (PortableServer_Servant          servant,
		    const CORBA_char               *path,
		    const MateComponent_StorageInfoFields  mask,
		    CORBA_Environment              *ev)
{
	MateComponentStorageMem       *storage;
	MateComponent_StorageInfo     *ret_val = NULL;
	MateComponentStorageMem       *parent_storage;
	MateComponentStorageMemEntry  *entry = NULL;
	gchar                  *filename = NULL;

	storage = MATECOMPONENT_STORAGE_MEM (matecomponent_object (servant));

	parent_storage = smem_get_parent (storage, path, &filename, &entry);

	if (!parent_storage) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto out;
	}

	if (entry->is_directory) {
		if (mask & MateComponent_FIELD_CONTENT_TYPE ||
		    mask & MateComponent_FIELD_SIZE) {
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NotSupported);
			goto out;
		}
		
		ret_val = MateComponent_StorageInfo__alloc ();

		ret_val->name = CORBA_string_dup (filename);

		if (mask & MateComponent_FIELD_TYPE)
			ret_val->type = MateComponent_STORAGE_TYPE_DIRECTORY;
		
	} else {
		
		if (mask & MateComponent_FIELD_CONTENT_TYPE ||
		    mask & MateComponent_FIELD_SIZE)
			ret_val = smem_get_stream_info (entry->child, mask, ev);
		else
			ret_val = MateComponent_StorageInfo__alloc ();

		ret_val->name = CORBA_string_dup (filename);
		ret_val->type = MateComponent_STORAGE_TYPE_REGULAR;
		
	}


 out:
	matecomponent_storage_mem_entry_free (entry);	
	g_free (filename);
	
	return ret_val;
}

static void
smem_set_info_impl (PortableServer_Servant    servant,
		    const CORBA_char         *path,
		    const MateComponent_StorageInfo *info,
		    MateComponent_StorageInfoFields  mask,
		    CORBA_Environment        *ev)
{
	MateComponentStorageMem      *storage;
	MateComponentStorageMem      *parent_storage;
	MateComponentStorageMemEntry *entry = NULL;
	gchar                 *filename;

	storage = MATECOMPONENT_STORAGE_MEM (matecomponent_object (servant));

	parent_storage = smem_get_parent (storage, path, &filename, &entry);

	if (!parent_storage) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto out;
	}

	if (entry->is_directory)
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotSupported);
	else {
		CORBA_Environment my_ev;

		CORBA_exception_init (&my_ev);
		MateComponent_Stream_setInfo (
			matecomponent_object_corba_objref (entry->child),
			info, mask,
			&my_ev);
		
		if (MATECOMPONENT_EX (&my_ev)) {
			if (MATECOMPONENT_USER_EX (&my_ev, ex_MateComponent_Stream_IOError))
				matecomponent_exception_set (ev, ex_MateComponent_Storage_IOError);
			if (MATECOMPONENT_USER_EX (&my_ev, ex_MateComponent_Stream_NoPermission))
				matecomponent_exception_set (ev, ex_MateComponent_Storage_NoPermission);
			if (MATECOMPONENT_USER_EX (&my_ev, ex_MateComponent_Stream_NotSupported))
				matecomponent_exception_set (ev, ex_MateComponent_Storage_NotSupported);
		}
			
		CORBA_exception_free (&my_ev);
	}
	
 out:
	g_free (filename);
	matecomponent_storage_mem_entry_free (entry);
}

static MateComponent_Stream
smem_open_stream_impl (PortableServer_Servant   servant,
		       const CORBA_char        *path,
		       MateComponent_Storage_OpenMode  mode,
		       CORBA_Environment       *ev)
{
	MateComponentStorageMem       *storage;
	MateComponentStorageMem       *parent;
	MateComponentStorageMemEntry  *entry;
	gchar                  *path_last;
	MateComponentObject           *stream = NULL;

	storage = MATECOMPONENT_STORAGE_MEM (matecomponent_object (servant));
	parent = smem_get_last_storage (storage, path, &path_last);
	
	if (!parent) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto ex_out;
	}

	entry = g_hash_table_lookup (parent->priv->entries, path_last); 

	/* Error cases */
	/* Case 1: Stream not found */
	if (!entry && !(mode & MateComponent_Storage_CREATE)) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto ex_out;
	}

	/* Case 2: A storage by the same name exists */
	if (entry && entry->is_directory) {
		if (mode & MateComponent_Storage_CREATE)
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NameExists);
		else
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NotStream);
		goto ex_out;
	}

	if (!entry) {
		stream = matecomponent_stream_mem_create (NULL, 0,
						   FALSE, TRUE);
		entry = g_new0 (MateComponentStorageMemEntry, 1);
		entry->is_directory = FALSE;
		entry->child = stream;

		g_hash_table_insert (parent->priv->entries,
				     g_strdup (path_last),
				     entry);
		goto ok_out;
	}
	
	stream = entry->child;
	
 ok_out:
	g_free (path_last);
	
	return matecomponent_object_dup_ref (MATECOMPONENT_OBJREF (stream), ev);
	
 ex_out:
	g_free (path_last);
	
	return CORBA_OBJECT_NIL;
}

static MateComponent_Storage
smem_open_storage_impl (PortableServer_Servant   servant,
			const CORBA_char        *path,
			MateComponent_Storage_OpenMode  mode,
			CORBA_Environment       *ev)
{
	MateComponentStorageMem       *storage;
	MateComponentStorageMem       *parent_storage;
	MateComponentStorageMemEntry  *entry;
	MateComponentObject           *ret = NULL;
	gchar                  *path_last = NULL;
	
	storage = MATECOMPONENT_STORAGE_MEM (matecomponent_object (servant));

	parent_storage = smem_get_last_storage (storage, path, &path_last);
	
	if (!parent_storage) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto ex_out;
	}

	entry = g_hash_table_lookup (parent_storage->priv->entries, path_last);
	
	/* Error cases */
	/* Case 1: Storage not found */
	if (!entry && !(mode & MateComponent_Storage_CREATE)) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto ex_out;
	}

	/* Case 2: A stream by the same name exists */
	if (entry && !entry->is_directory) {
		if (mode & MateComponent_Storage_CREATE)
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NameExists);
		else
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NotStorage);
		goto ex_out;
	}

	if (!entry) {
		ret = matecomponent_storage_mem_create ();
		entry = g_new0 (MateComponentStorageMemEntry, 1);
		entry->is_directory = TRUE;
		entry->child = ret;

		g_hash_table_insert (parent_storage->priv->entries,
				     g_strdup (path_last),
				     entry);
		goto ok_out;
	}

	ret = entry->child;
	
 ok_out:
	g_free (path_last);
	
	return matecomponent_object_dup_ref (MATECOMPONENT_OBJREF (ret), ev);
	
 ex_out:
	g_free (path_last);
	
	return CORBA_OBJECT_NIL;
}

static void
smem_copy_to_impl (PortableServer_Servant  servant,
		   const MateComponent_Storage    target,
		   CORBA_Environment      *ev)
{
	MateComponentStorageMem *storage;

	storage = MATECOMPONENT_STORAGE_MEM (matecomponent_object (servant));

	matecomponent_storage_copy_to (
		matecomponent_object_corba_objref (MATECOMPONENT_OBJECT (storage)),
		target, ev);
}

static MateComponent_Storage_DirectoryList *
smem_list_contents_impl (PortableServer_Servant          servant,
			 const CORBA_char               *path,
			 const MateComponent_StorageInfoFields  mask,
			 CORBA_Environment              *ev)
{
	MateComponentStorageMem             *storage;
	MateComponent_Storage_DirectoryList *ret_val = NULL;
	MateComponent_StorageInfo           *info;
	MateComponentStorageMem             *last_storage;
	gchar                        *path_last;
	GList                        *list;
	DirCBData                     cb_data;
	int                           i;

	storage = MATECOMPONENT_STORAGE_MEM (matecomponent_object (servant));
	
	last_storage = smem_get_last_storage (storage, path, &path_last);

	if (!last_storage) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto out;
	}

	if (path_last) { /* The requested entry is a stream or does not
			    exist */
		if (g_hash_table_lookup (last_storage->priv->entries, path_last))
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NotStorage);
		else
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto out;
	}

	cb_data.list = NULL;
	cb_data.mask = mask;

	g_hash_table_foreach (last_storage->priv->entries,
			      smem_dir_hash_cb, &cb_data);

	ret_val = MateComponent_Storage_DirectoryList__alloc ();
	list = cb_data.list;
	ret_val->_length = g_list_length (list);
	ret_val->_buffer = MateComponent_Storage_DirectoryList_allocbuf (ret_val->_length);

	for (i = 0; list != NULL; list = list->next, i++) {
		info = list->data;

		ret_val->_buffer[i].name = CORBA_string_dup (info->name);
		ret_val->_buffer[i].type = info->type;
		ret_val->_buffer[i].content_type = CORBA_string_dup (info->content_type);
		ret_val->_buffer[i].size = info->size;

		CORBA_free (info);
	}
	g_list_free (cb_data.list);
	
 out:
	g_free (path_last);
	return ret_val;
}

static void
smem_erase_impl (PortableServer_Servant  servant,
		 const CORBA_char       *path,
		 CORBA_Environment      *ev)
{
	MateComponentStorageMem       *storage;
	MateComponentStorageMemEntry  *entry = NULL;
	MateComponentStorageMem       *parent_storage;
	gchar                  *filename = NULL;

	storage = MATECOMPONENT_STORAGE_MEM (matecomponent_object (servant));

	parent_storage = smem_get_parent (storage, path, &filename, &entry);
	if (!parent_storage) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto out;
	}

	if (entry->is_directory) {
		MateComponentStorageMem *storage_to_remove =
			MATECOMPONENT_STORAGE_MEM (entry->child);
		
		/* You can't remove the root item */
		if (!strcmp (path, "/") || !strcmp (path, "")) {
			matecomponent_exception_set (ev, ex_MateComponent_Storage_IOError);
			goto out;
		}

		/* Is the storage empty? */
		if (g_hash_table_size (storage_to_remove->priv->entries)) {
			matecomponent_exception_set (ev, ex_MateComponent_Storage_NotEmpty);
			goto out;
		}
		g_hash_table_remove (parent_storage->priv->entries, filename);
		
	} else
		g_hash_table_remove (parent_storage->priv->entries,
				     filename);

 out:
	matecomponent_storage_mem_entry_free (entry);
	g_free (filename);
}

static void
smem_rename_impl (PortableServer_Servant  servant,
		  const CORBA_char       *path,
		  const CORBA_char       *new_path,
		  CORBA_Environment      *ev)
{
	MateComponentStorageMem      *storage;
	MateComponentStorageMem      *parent_storage, *target_storage;
	MateComponentStorageMemEntry *entry;
	gchar                 *filename = NULL, *new_filename;

	if (!strcmp (path, "/") || !strcmp (path, "")) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_IOError);
		goto out;
	}

	storage = MATECOMPONENT_STORAGE_MEM (matecomponent_object (servant));

	parent_storage = smem_get_parent (storage, path, &filename, &entry);
	target_storage = smem_get_last_storage (storage, new_path, &new_filename);

	/* Source or target does not exists */
	if (!parent_storage || !target_storage) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NotFound);
		goto out;
	}

	/* Target exists and is not a storage */
	if (new_filename && g_hash_table_lookup (target_storage->priv->entries,
						 new_filename)) {
		matecomponent_exception_set (ev, ex_MateComponent_Storage_NameExists);
		goto out;
	}

	g_hash_table_remove (parent_storage->priv->entries, filename);
	
	/* If target does not exists, new_filename will be non-NULL */
	if (new_filename)
		g_hash_table_insert (target_storage->priv->entries,
				     new_filename, entry);
	else
		g_hash_table_insert (target_storage->priv->entries,
				     g_strdup (filename), entry);

 out:
	g_free (filename);
}

static void
smem_commit_impl (PortableServer_Servant  servant,
		  CORBA_Environment      *ev)
{
	matecomponent_exception_set (ev, ex_MateComponent_Storage_NotSupported);
}

static void
smem_revert_impl (PortableServer_Servant  servant,
		  CORBA_Environment      *ev)
{
	matecomponent_exception_set (ev, ex_MateComponent_Storage_NotSupported);
}

static void
matecomponent_storage_mem_finalize (GObject *object)
{
	MateComponentStorageMem *smem = MATECOMPONENT_STORAGE_MEM (object);

	if (smem->priv) {
		g_hash_table_destroy (smem->priv->entries);
		g_free (smem->priv);
		smem->priv = NULL;
	}
	
	G_OBJECT_CLASS (matecomponent_storage_mem_parent_class)->finalize (object);
}

static void
matecomponent_storage_mem_init (MateComponentStorageMem *smem)
{
	smem->priv = g_new0 (MateComponentStorageMemPriv, 1);

	smem->priv->entries = g_hash_table_new_full (
		g_str_hash, g_str_equal, g_free,
		matecomponent_storage_mem_entry_free);
}

static void
matecomponent_storage_mem_class_init (MateComponentStorageMemClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_Storage__epv *epv = &klass->epv;
	
	matecomponent_storage_mem_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = matecomponent_storage_mem_finalize;

	epv->getInfo       = smem_get_info_impl;
	epv->setInfo       = smem_set_info_impl;
	epv->listContents  = smem_list_contents_impl;
	epv->openStream    = smem_open_stream_impl;
	epv->openStorage   = smem_open_storage_impl;
	epv->copyTo = smem_copy_to_impl;
	epv->erase  = smem_erase_impl;
	epv->rename = smem_rename_impl;
	epv->commit = smem_commit_impl;
	epv->revert = smem_revert_impl;
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentStorageMem, 
		       MateComponent_Storage,
		       MATECOMPONENT_TYPE_OBJECT,
		       matecomponent_storage_mem)

MateComponentObject *
matecomponent_storage_mem_create (void)
{
	MateComponentStorageMem *smem;

	smem = g_object_new (matecomponent_storage_mem_get_type (), NULL);
	
	if (!smem)
		return NULL;
	
	return MATECOMPONENT_STORAGE (smem);
}

