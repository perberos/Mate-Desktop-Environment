/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* vi: set ts=8 sts=0 sw=8 tw=80 noexpandtab: */
/* mate-vfs-mime-info.c - MATE xdg mime information implementation.

 Copyright (C) 2004 Red Hat, Inc
 All rights reserved.
 
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
*/

#include <config.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "mate-vfs-mime.h"
#include "mate-vfs-mime-info-cache.h"
#include "mate-vfs-mime-info.h"
#include "mate-vfs-mime-monitor.h"
#include "mate-vfs-monitor.h"
#include "mate-vfs-ops.h"
#include "mate-vfs-utils.h"
#include "xdgmime.h"

typedef struct {
	char *path;
	GHashTable *mime_info_cache_map;
	GHashTable *defaults_list_map;
	MateVFSMonitorHandle  *cache_monitor_handle;
	MateVFSMonitorHandle  *defaults_monitor_handle;

	time_t mime_info_cache_timestamp;
	time_t defaults_list_timestamp;
} MateVFSMimeInfoCacheDir;

typedef struct {
	GList *dirs;               /* mimeinfo.cache and defaults.list */
	GHashTable *global_defaults_cache; /* global results of defaults.list lookup and validation */
        time_t last_stat_time;
	guint should_ping_mime_monitor : 1;
} MateVFSMimeInfoCache;

G_LOCK_EXTERN (mate_vfs_mime_mutex);

extern void _mate_vfs_mime_monitor_emit_data_changed (MateVFSMIMEMonitor *monitor); 
extern void _mate_vfs_mime_info_cache_init (void);

static void mate_vfs_mime_info_cache_dir_init (MateVFSMimeInfoCacheDir *dir);
static void mate_vfs_mime_info_cache_dir_init_defaults_list (MateVFSMimeInfoCacheDir *dir);
static MateVFSMimeInfoCacheDir *mate_vfs_mime_info_cache_dir_new (const char *path);
static void mate_vfs_mime_info_cache_dir_free (MateVFSMimeInfoCacheDir *dir);
static char **mate_vfs_mime_info_cache_get_search_path (void);

static gboolean mate_vfs_mime_info_cache_dir_desktop_entry_is_valid (MateVFSMimeInfoCacheDir *dir,
								      const char *desktop_entry);
static void mate_vfs_mime_info_cache_dir_add_desktop_entries (MateVFSMimeInfoCacheDir *dir,
							       const char *mime_type,
							       char **new_desktop_file_ids);
static MateVFSMimeInfoCache *mate_vfs_mime_info_cache_new (void);
static void mate_vfs_mime_info_cache_free (MateVFSMimeInfoCache *cache);

static MateVFSMimeInfoCache *mime_info_cache = NULL;
G_LOCK_DEFINE_STATIC (mime_info_cache);


static void
destroy_info_cache_value (gpointer key, GList *value, gpointer data)
{
	mate_vfs_list_deep_free (value);
}

static void
destroy_info_cache_map (GHashTable *info_cache_map)
{
	g_hash_table_foreach (info_cache_map, (GHFunc)destroy_info_cache_value,
			      NULL);
	g_hash_table_destroy (info_cache_map);
}

static gboolean
mate_vfs_mime_info_cache_dir_out_of_date (MateVFSMimeInfoCacheDir *dir, const char *cache_file,
		                           time_t *timestamp)
{
	struct stat buf;
	char *filename;

	filename = g_build_filename (dir->path, cache_file, NULL);

	if (g_stat (filename, &buf) < 0) {
		g_free (filename);
		return TRUE;
	}
	g_free (filename);

	if (buf.st_mtime != *timestamp) 
		return TRUE;

	return FALSE;
}

/* Call with lock held */
static gboolean remove_all (gpointer  key,
			    gpointer  value,
			    gpointer  user_data)
{
	return TRUE;
}


static void
mate_vfs_mime_info_cache_blow_global_cache (void)
{
	g_hash_table_foreach_remove (mime_info_cache->global_defaults_cache,
				     remove_all, NULL);
}

static void
mate_vfs_mime_info_cache_dir_init (MateVFSMimeInfoCacheDir *dir)
{
	GError *load_error;
	GKeyFile *key_file;
	gchar *filename, **mime_types;
	int i;
	struct stat buf;

	load_error = NULL;
	mime_types = NULL;

	if (dir->mime_info_cache_map != NULL &&
	    dir->cache_monitor_handle == NULL &&
  	    !mate_vfs_mime_info_cache_dir_out_of_date (dir, "mimeinfo.cache",
		                                        &dir->mime_info_cache_timestamp))
		return;

	if (dir->mime_info_cache_map != NULL) {
		destroy_info_cache_map (dir->mime_info_cache_map);
	}

	dir->mime_info_cache_map = g_hash_table_new_full (g_str_hash, g_str_equal,
							  (GDestroyNotify) g_free,
							  NULL);

	key_file = g_key_file_new ();

	filename = g_build_filename (dir->path, "mimeinfo.cache", NULL);

	if (g_stat (filename, &buf) < 0)
		goto error;

	if (dir->mime_info_cache_timestamp > 0) 
		mime_info_cache->should_ping_mime_monitor = TRUE;

	dir->mime_info_cache_timestamp = buf.st_mtime;

	g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &load_error);

	g_free (filename);
	filename = NULL;

	if (load_error != NULL)
		goto error;

	mime_types = g_key_file_get_keys (key_file, "MIME Cache",
					  NULL, &load_error);

	if (load_error != NULL)
		goto error;

	G_LOCK (mate_vfs_mime_mutex);

	for (i = 0; mime_types[i] != NULL; i++) {
		gchar **desktop_file_ids;
		desktop_file_ids = g_key_file_get_string_list (key_file,
							       "MIME Cache",
							       mime_types[i],
							       NULL,
							       &load_error);

		if (load_error != NULL) {
			g_error_free (load_error);
			load_error = NULL;
			continue;
		}

		mate_vfs_mime_info_cache_dir_add_desktop_entries (dir,
								   xdg_mime_unalias_mime_type (mime_types[i]),
								   desktop_file_ids);

		g_strfreev (desktop_file_ids);
	}

	G_UNLOCK (mate_vfs_mime_mutex);

	g_strfreev (mime_types);
	g_key_file_free (key_file);

	return;
error:
	if (filename)
		g_free (filename);

	g_key_file_free (key_file);

	if (mime_types != NULL)
		g_strfreev (mime_types);

	if (load_error)
		g_error_free (load_error);
}

static void
mate_vfs_mime_info_cache_dir_init_defaults_list (MateVFSMimeInfoCacheDir *dir)
{
	GKeyFile *key_file;
	GError *load_error;
	gchar *filename, **mime_types;
	char **desktop_file_ids;
	int i;
	struct stat buf;

	load_error = NULL;
	mime_types = NULL;

	if (dir->defaults_list_map != NULL &&
	    dir->defaults_monitor_handle == NULL &&
  	    !mate_vfs_mime_info_cache_dir_out_of_date (dir, "defaults.list",
		                                        &dir->defaults_list_timestamp))
		return;

	if (dir->defaults_list_map != NULL) {
		g_hash_table_destroy (dir->defaults_list_map);
	}

	dir->defaults_list_map = g_hash_table_new_full (g_str_hash, g_str_equal,
							g_free, (GDestroyNotify)g_strfreev);

	key_file = g_key_file_new ();

	filename = g_build_filename (dir->path, "defaults.list", NULL);

	if (g_stat (filename, &buf) < 0)
		goto error;

	if (dir->defaults_list_timestamp > 0) 
		mime_info_cache->should_ping_mime_monitor = TRUE;

	dir->defaults_list_timestamp = buf.st_mtime;

	g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &load_error);

	g_free (filename);
	filename = NULL;

	if (load_error != NULL)
		goto error;

	mime_types = g_key_file_get_keys (key_file, "Default Applications",
					  NULL, &load_error);

	if (load_error != NULL)
		goto error;

	G_LOCK (mate_vfs_mime_mutex);

	for (i = 0; mime_types[i] != NULL; i++) {
		desktop_file_ids = g_key_file_get_string_list (key_file,
							       "Default Applications",
							       mime_types[i],
							       NULL,
							       &load_error);
		if (load_error != NULL) {
			g_error_free (load_error);
			load_error = NULL;
			continue;
		}

		g_hash_table_replace (dir->defaults_list_map,
				      g_strdup (xdg_mime_unalias_mime_type (mime_types[i])),
				      desktop_file_ids);
	}

	G_UNLOCK (mate_vfs_mime_mutex);

	g_strfreev (mime_types);
	g_key_file_free (key_file);

	return;
error:
	if (filename)
		g_free (filename);

	g_key_file_free (key_file);

	if (mime_types != NULL)
		g_strfreev (mime_types);

	if (load_error)
		g_error_free (load_error);
}

static gboolean
emit_mime_changed (gpointer data)
{
	_mate_vfs_mime_monitor_emit_data_changed (mate_vfs_mime_monitor_get ());
	return FALSE;
}

static void
mate_vfs_mime_info_cache_dir_changed (MateVFSMonitorHandle    *handle,
		                       const gchar              *monitor_uri,
		                       const gchar              *info_uri,
		                       MateVFSMonitorEventType  event_type,
	                               MateVFSMimeInfoCacheDir *dir)
{
	G_LOCK (mime_info_cache);
	mate_vfs_mime_info_cache_blow_global_cache ();
	mate_vfs_mime_info_cache_dir_init (dir);
	mime_info_cache->should_ping_mime_monitor = FALSE;
	G_UNLOCK (mime_info_cache);
	g_idle_add (emit_mime_changed, NULL);
}

static void
mate_vfs_mime_info_cache_dir_defaults_changed (MateVFSMonitorHandle    *handle,
					 const gchar              *monitor_uri,
					 const gchar              *info_uri,
					 MateVFSMonitorEventType  event_type,
					 MateVFSMimeInfoCacheDir *dir)
{
	G_LOCK (mime_info_cache);
	mate_vfs_mime_info_cache_blow_global_cache ();
	mate_vfs_mime_info_cache_dir_init_defaults_list (dir);
	mime_info_cache->should_ping_mime_monitor = FALSE;
	G_UNLOCK (mime_info_cache);
	g_idle_add (emit_mime_changed, NULL);
}

static MateVFSMimeInfoCacheDir *
mate_vfs_mime_info_cache_dir_new (const char *path)
{
	MateVFSMimeInfoCacheDir *dir;
	char *filename;

	dir = g_new0 (MateVFSMimeInfoCacheDir, 1);
	dir->path = g_strdup (path);

	filename = g_build_filename (dir->path, "mimeinfo.cache", NULL);

	mate_vfs_monitor_add (&dir->cache_monitor_handle,
			       filename,
			       MATE_VFS_MONITOR_FILE,
			       (MateVFSMonitorCallback) 
			       mate_vfs_mime_info_cache_dir_changed,
			       dir);
	g_free (filename);

	filename = g_build_filename (dir->path, "defaults.list", NULL);

	mate_vfs_monitor_add (&dir->defaults_monitor_handle,
			       filename,
			       MATE_VFS_MONITOR_FILE,
			       (MateVFSMonitorCallback) 
			       mate_vfs_mime_info_cache_dir_defaults_changed,
			       dir);
	g_free (filename);

	return dir;
}

static void
mate_vfs_mime_info_cache_dir_free (MateVFSMimeInfoCacheDir *dir)
{
	if (dir == NULL)
		return;
	if (dir->mime_info_cache_map != NULL) {
		destroy_info_cache_map (dir->mime_info_cache_map);
		dir->mime_info_cache_map = NULL;

	}

	if (dir->defaults_list_map != NULL) {
		g_hash_table_destroy (dir->defaults_list_map);
		dir->defaults_list_map = NULL;
	}

	if (dir->defaults_monitor_handle) {
		mate_vfs_monitor_cancel (dir->defaults_monitor_handle);
		dir->defaults_monitor_handle = NULL;
	}

	if (dir->cache_monitor_handle){
		mate_vfs_monitor_cancel (dir->cache_monitor_handle);
		dir->cache_monitor_handle = NULL;
	}

	g_free (dir);
}

static char **
mate_vfs_mime_info_cache_get_search_path (void)
{
	char **args = NULL;
	const char * const *data_dirs;
	const char *user_data_dir;
	int i, length, j;

	data_dirs = g_get_system_data_dirs ();

	for (length = 0; data_dirs[length] != NULL; length++);

	args = g_new (char *, length + 2);

	j = 0;
	user_data_dir = g_get_user_data_dir ();
	args[j++] = g_build_filename (user_data_dir, "applications", NULL);

	for (i = 0; i < length; i++) {
		args[j++] = g_build_filename (data_dirs[i],
					      "applications", NULL);
	}
	args[j++] = NULL;

	return args;
}

static gboolean
mate_vfs_mime_info_cache_dir_desktop_entry_is_valid (MateVFSMimeInfoCacheDir *dir,
                                                      const char *desktop_entry)
{
	GKeyFile *key_file;
	GError *load_error;
	int i;
	gboolean can_show_in;
	char *filename;

	load_error = NULL;
	can_show_in = TRUE;

	filename = g_build_filename ("applications", desktop_entry, NULL);

	key_file = g_key_file_new ();
	g_key_file_load_from_data_dirs (key_file, filename, NULL,
					G_KEY_FILE_NONE, &load_error);
	g_free (filename);

	if (load_error != NULL) {
		g_error_free (load_error);
		g_key_file_free (key_file);
		return FALSE;
	}

	if (g_key_file_has_key (key_file, DESKTOP_ENTRY_GROUP,
				"OnlyShowIn", NULL)) {

		char **only_show_in_list;
		only_show_in_list = g_key_file_get_string_list (key_file,
								DESKTOP_ENTRY_GROUP,
								"OnlyShowIn",
								NULL,
								&load_error);

		if (load_error != NULL) {
			g_error_free (load_error);
			g_strfreev (only_show_in_list);
			g_key_file_free (key_file);
			return FALSE;
		}

		can_show_in = FALSE;
		for (i = 0; only_show_in_list[i] != NULL; i++) {
			if (strcmp (only_show_in_list[i], "MATE") == 0) {
				can_show_in = TRUE;
				break;
			}
		}

		g_strfreev (only_show_in_list);
	}

	if (g_key_file_has_key (key_file,
				DESKTOP_ENTRY_GROUP,
				"NotShowIn", NULL)) {
		char **not_show_in_list;
		not_show_in_list = g_key_file_get_string_list (key_file,
							       DESKTOP_ENTRY_GROUP,
							       "NotShowIn",
							       NULL,
							       &load_error);

		if (load_error != NULL) {
			g_error_free (load_error);
			g_strfreev (not_show_in_list);
			g_key_file_free (key_file);
			return FALSE;
		}

		for (i = 0; not_show_in_list[i] != NULL; i++) {
			if (strcmp (not_show_in_list[i], "MATE") == 0) {
				can_show_in = FALSE;
				break;
			}
		}

		g_strfreev (not_show_in_list);
	}

	g_key_file_free (key_file);
	return can_show_in;
}

static gboolean
mate_vfs_mime_info_desktop_entry_is_valid (const char *desktop_entry)
{
	GList *l;
	
	for (l = mime_info_cache->dirs; l != NULL; l = l->next) {
		MateVFSMimeInfoCacheDir *app_dir;
		
		app_dir = (MateVFSMimeInfoCacheDir *) l->data;
		if (mate_vfs_mime_info_cache_dir_desktop_entry_is_valid (app_dir,
									  desktop_entry)) {
			return TRUE;
		}
	}
	return FALSE;
}


static void
mate_vfs_mime_info_cache_dir_add_desktop_entries (MateVFSMimeInfoCacheDir *dir,
                                                   const char *mime_type,
                                                   char **new_desktop_file_ids)
{
	GList *desktop_file_ids;
	int i;

	desktop_file_ids = g_hash_table_lookup (dir->mime_info_cache_map,
						mime_type);

	for (i = 0; new_desktop_file_ids[i] != NULL; i++) {
		if (!g_list_find (desktop_file_ids, new_desktop_file_ids[i]))
			desktop_file_ids = g_list_append (desktop_file_ids,
							  g_strdup (new_desktop_file_ids[i]));
	}

	g_hash_table_insert (dir->mime_info_cache_map, g_strdup (mime_type), desktop_file_ids);
}

static void
mate_vfs_mime_info_cache_init_dir_lists (void)
{
	char **dirs;
	int i;

	mime_info_cache = mate_vfs_mime_info_cache_new ();

	dirs = mate_vfs_mime_info_cache_get_search_path ();

	for (i = 0; dirs[i] != NULL; i++) {
		MateVFSMimeInfoCacheDir *dir;

		dir = mate_vfs_mime_info_cache_dir_new (dirs[i]);

		if (dir != NULL) {
			mate_vfs_mime_info_cache_dir_init (dir);
			mate_vfs_mime_info_cache_dir_init_defaults_list (dir);

			mime_info_cache->dirs = g_list_append (mime_info_cache->dirs,
			 		                       dir);
		}
	}
	g_strfreev (dirs);
}

static void
mate_vfs_mime_info_cache_update_dir_lists (void)
{
	GList *tmp;
	tmp = mime_info_cache->dirs;
	while (tmp != NULL) {
		MateVFSMimeInfoCacheDir *dir;

		dir = (MateVFSMimeInfoCacheDir *) tmp->data;

		if (dir->cache_monitor_handle == NULL) {
			mate_vfs_mime_info_cache_blow_global_cache ();
			mate_vfs_mime_info_cache_dir_init (dir);
		}
		
		if (dir->defaults_monitor_handle == NULL) {
			mate_vfs_mime_info_cache_blow_global_cache ();
			mate_vfs_mime_info_cache_dir_init_defaults_list (dir);
		}

		tmp = tmp->next;
	}
}

void
_mate_vfs_mime_info_cache_init (void)
{
	G_LOCK (mime_info_cache);
	if (mime_info_cache == NULL) {
		mate_vfs_mime_info_cache_init_dir_lists ();
	} else {
		time_t now;

		time (&now);

		if (now >= mime_info_cache->last_stat_time + 5) {
			mate_vfs_mime_info_cache_update_dir_lists ();
			mime_info_cache->last_stat_time = now;
		}
	}

	if (mime_info_cache->should_ping_mime_monitor) {
		g_idle_add (emit_mime_changed, NULL);
		mime_info_cache->should_ping_mime_monitor = FALSE;
	}
	G_UNLOCK (mime_info_cache);
}


static MateVFSMimeInfoCache *
mate_vfs_mime_info_cache_new (void)
{
	MateVFSMimeInfoCache *cache;

	cache = g_new0 (MateVFSMimeInfoCache, 1);

	cache->global_defaults_cache = g_hash_table_new_full (g_str_hash, g_str_equal,
							      (GDestroyNotify) g_free,
							      (GDestroyNotify) g_free);
	return cache;
}

static void
mate_vfs_mime_info_cache_free (MateVFSMimeInfoCache *cache)
{
	if (cache == NULL)
		return;

	g_list_foreach (cache->dirs,
			(GFunc) mate_vfs_mime_info_cache_dir_free,
			NULL);
	g_list_free (cache->dirs);
	g_hash_table_destroy (cache->global_defaults_cache);
	g_free (cache);
}

/**
 * mate_vfs_mime_info_cache_reload:
 * @dir: directory path which needs reloading.
 * 
 * Reload the mime information for the @dir.
 */
void
mate_vfs_mime_info_cache_reload (const char *dir)
{
	/* FIXME: just reload the dir that needs reloading,
	 * don't blow the whole cache
	 */
	if (mime_info_cache != NULL) {
		G_LOCK (mime_info_cache);
		mate_vfs_mime_info_cache_free (mime_info_cache);
		mime_info_cache = NULL;
		G_UNLOCK (mime_info_cache);
	}
}

static GList *
get_all_parent_types (const char *mime_type)
{
	const char *umime;
	const char **parents;
	GList *l = NULL;
	int i;

	G_LOCK (mate_vfs_mime_mutex);
	
	umime = xdg_mime_unalias_mime_type (mime_type);
	l = g_list_prepend (l, g_strdup (umime));

	parents = xdg_mime_get_mime_parents (mime_type);

	for (i = 0; parents && parents[i] != NULL; i++) {
		l = g_list_prepend (l, g_strdup (parents[i]));
	}
	
	G_UNLOCK (mate_vfs_mime_mutex);

	return g_list_reverse (l);
}

static GList *
append_desktop_entry (GList *list, const char *desktop_entry)
{
	/* Add if not already in list, and valid */
	/* TODO: Should we cache here to avoid desktop file validation? */
	if (!g_list_find_custom (list, desktop_entry, (GCompareFunc) strcmp) &&
	    mate_vfs_mime_info_desktop_entry_is_valid (desktop_entry)) {
		list = g_list_prepend (list, g_strdup (desktop_entry));
	}
	return list;
}

static gchar *
get_default_desktop_entry (const char *mime_type)
{
	gchar *desktop_entry;
	char **desktop_entries;
	GList *dir_list, *l;
	GList *folder_handlers;
	MateVFSMimeInfoCacheDir *dir;
	int i;

	desktop_entry = g_hash_table_lookup (mime_info_cache->global_defaults_cache,
					     mime_type);
	if (desktop_entry) {
		return g_strdup (desktop_entry);
	}

	if (g_str_has_prefix (mime_type, "x-directory/")) {
		/* We special case folder handler defaults so that
		 * we always can get a working handler for them
		 * even though we don't control the defaults.list,
		 * or the list of availible directory handlers.
		 */

		for (dir_list = mime_info_cache->dirs; dir_list != NULL; dir_list = dir_list->next) {
			dir = dir_list->data;
		
			folder_handlers = g_hash_table_lookup (dir->mime_info_cache_map, "x-directory/mate-default-handler");
			for (l = folder_handlers; l != NULL; l = l->next) {
				desktop_entry = l->data;
			
				if (desktop_entry != NULL &&
				    mate_vfs_mime_info_desktop_entry_is_valid (desktop_entry)) {
					g_hash_table_insert (mime_info_cache->global_defaults_cache,
							     g_strdup (mime_type), g_strdup (desktop_entry));
					return g_strdup (desktop_entry);
				}
			}
		}
	}
	
	desktop_entry = NULL;
	for (dir_list = mime_info_cache->dirs; dir_list != NULL; dir_list = dir_list->next) {
		dir = dir_list->data;
		desktop_entries = g_hash_table_lookup (dir->defaults_list_map,
						       mime_type);
		for (i = 0; desktop_entries != NULL && desktop_entries[i] != NULL; i++) {
			desktop_entry = desktop_entries[i];
			if (desktop_entry != NULL &&
			    mate_vfs_mime_info_desktop_entry_is_valid (desktop_entry)) {
				g_hash_table_insert (mime_info_cache->global_defaults_cache,
						     g_strdup (mime_type), g_strdup (desktop_entry));
				return g_strdup (desktop_entry);
			}
		}
	}

	return NULL;
}

/**
 * mate_vfs_mime_get_all_desktop_entries:
 * @mime_type: a mime type.
 *
 * Returns all the desktop filenames for @mime_type. The desktop files
 * are listed in an order so that default applications are listed before
 * non-default ones, and handlers for inherited mimetypes are listed
 * after the base ones.
 *
 * Return value: a #GList containing the desktop filenames containing the
 * @mime_type.
 */
GList *
mate_vfs_mime_get_all_desktop_entries (const char *base_mime_type)
{
	GList *desktop_entries, *list, *dir_list, *tmp;
	GList *mime_types, *m_list;
	MateVFSMimeInfoCacheDir *dir;
	char *mime_type;
	char *default_desktop_entry;

	_mate_vfs_mime_info_cache_init ();

	G_LOCK (mime_info_cache);
	mime_types = get_all_parent_types (base_mime_type);

	desktop_entries = NULL;
	for (m_list = mime_types; m_list != NULL; m_list = m_list->next) {
		mime_type = m_list->data;

		default_desktop_entry = get_default_desktop_entry (mime_type);
		if (default_desktop_entry) {
			desktop_entries = append_desktop_entry (desktop_entries, default_desktop_entry);
			g_free (default_desktop_entry);
		}
		
		for (dir_list = mime_info_cache->dirs;
		     dir_list != NULL;
		     dir_list = dir_list->next) {
			dir = (MateVFSMimeInfoCacheDir *) dir_list->data;
			
			list = g_hash_table_lookup (dir->mime_info_cache_map, mime_type);
			
			for (tmp = list; tmp != NULL; tmp = tmp->next) {
				desktop_entries = append_desktop_entry (desktop_entries, tmp->data);
			}
		}
	}

	G_UNLOCK (mime_info_cache);

	g_list_foreach (mime_types, (GFunc)g_free, NULL);
	g_list_free (mime_types);

	desktop_entries = g_list_reverse (desktop_entries);

	return desktop_entries;
}


/**
 * mate_vfs_mime_get_default_desktop_entry:
 * @mime_type: a mime type.
 *
 * Returns the default desktop filename for @mime_type.
 *
 * Return value: the default desktop filename for @mime_type.
 */
gchar *
mate_vfs_mime_get_default_desktop_entry (const char *mime_type)
{
	char *desktop_entry;
	GList *desktop_files;

	desktop_entry = NULL;
	
	desktop_files =	mate_vfs_mime_get_all_desktop_entries (mime_type);
	if (desktop_files != NULL) {
		desktop_entry = g_strdup (desktop_files->data);
	}
		
	g_list_foreach (desktop_files, (GFunc)g_free, NULL);
	g_list_free (desktop_files);

	return desktop_entry;
}

