
/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-mime-info.c - MATE mime-information implementation.

   Copyright (C) 1998 Miguel de Icaza
   Copyright (C) 2000, 2001 Eazel, Inc.
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

   Authors:
   Miguel De Icaza <miguel@helixcode.com>
   Mathieu Lacage <mathieu@eazel.com>
*/

#include <config.h>
#include "mate-vfs-mime-info.h"

#include "mate-vfs-mime-monitor.h"
#include "mate-vfs-mime-private.h"
#include "mate-vfs-mime.h"
#include "mate-vfs-private-utils.h"
#include "xdgmime.h"
#include <glib/gi18n-lib.h>

#include <libxml/xmlreader.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct {
	char *description;
	char *parent_classes;
	char *aliases;
} MimeEntry;

typedef struct {
	char *path;
} MimeDirectory;

#if 0
/* These ones are used to automatically reload mime info on demand */
static time_t last_checked;
#endif

/* To initialize the module automatically */
static gboolean mate_vfs_mime_inited = FALSE;

/* we want to replace the previous key if the current key has a higher
   language level */

static GList *mime_directories = NULL;

static GHashTable *mime_entries = NULL;
/* Used to mean "no entry" in cache for negative caching */
static MimeEntry mime_entries_no_entry;

G_LOCK_EXTERN (mate_vfs_mime_mutex);

static gboolean
does_string_contain_caps (const char *string)
{
	const char *temp_c;

	temp_c = string;
	while (*temp_c != '\0') {
		if (g_ascii_isupper (*temp_c)) {
			return TRUE;
		}
		temp_c++;
	}

	return FALSE;
}

static void
mime_entry_free (MimeEntry *entry)
{
	if (!entry) {
		return;
	}
	
	g_free (entry->description);
	g_free (entry->parent_classes);
	g_free (entry->aliases);
	g_free (entry);
}

static void
add_data_dir (const char *dir)
{
	MimeDirectory *directory;

	directory = g_new0 (MimeDirectory, 1);

	directory->path = g_build_filename (dir, "mime", NULL);

	mime_directories = g_list_append (mime_directories, directory);
}

static void
mate_vfs_mime_init (void)
{
	const char *xdg_data_home;
	const char * const *xdg_data_dirs;

	/*
	 * The hash tables that store the mime keys.
	 */
	mime_entries = g_hash_table_new_full (g_str_hash, 
					      g_str_equal,
					      g_free,
					      (GDestroyNotify)mime_entry_free);

	xdg_data_home = g_get_user_data_dir ();
	add_data_dir (xdg_data_home);

	for (xdg_data_dirs = g_get_system_data_dirs (); 
	     *xdg_data_dirs; 
	     xdg_data_dirs++) {
		add_data_dir (*xdg_data_dirs);
	}
#if 0
	last_checked = time (NULL);
#endif
	mate_vfs_mime_inited = TRUE;
}

static void
reload_if_needed (void)
{
#if 0
	time_t now = time (NULL);
	gboolean need_reload = FALSE;
	struct stat s;

	if (mate_vfs_is_frozen > 0)
		return;

	if (mate_mime_dir.force_reload || user_mime_dir.force_reload)
		need_reload = TRUE;
	else if (now > last_checked + 5) {
		if (g_stat (mate_mime_dir.dirname, &s) != -1 &&
		    s.st_mtime != mate_mime_dir.s.st_mtime)
			need_reload = TRUE;
		else if (g_stat (user_mime_dir.dirname, &s) != -1 &&
			 s.st_mtime != user_mime_dir.s.st_mtime)
			need_reload = TRUE;
	}

	last_checked = now;


	if (need_reload) {
	        mate_vfs_mime_info_reload ();
	}
#endif
}

static void
mate_vfs_mime_info_clear (void)
{
}

/**
 * _mate_vfs_mime_info_shutdown:
 * 
 * Remove the MIME database from memory.
 **/
void
_mate_vfs_mime_info_shutdown (void)
{
	mate_vfs_mime_info_clear ();
}

/**
 * mate_vfs_mime_info_reload:
 *
 * Reload the MIME database from disk and notify any listeners
 * holding active #MateVFSMIMEMonitor objects.
 */
void
mate_vfs_mime_info_reload (void)
{
	if (!mate_vfs_mime_inited) {
		mate_vfs_mime_init ();
	}

	mate_vfs_mime_info_clear ();

	_mate_vfs_mime_monitor_emit_data_changed (mate_vfs_mime_monitor_get ());
}

/**
 * mate_vfs_mime_freeze:
 *
 * Freezes the mime data so that you can do multiple
 * updates to the data in one batch without needing
 * to back the files to disk or reading them.
 */
void
mate_vfs_mime_freeze (void)
{
	/* noop, get rid of this once all the mime editing stuff is gone */
}



/**
 * mate_vfs_mime_thaw:
 *
 * UnFreezes the mime data so that you can do multiple
 * updates to the data in one batch without needing
 * to back the files to disk or reading them.
 */
void
mate_vfs_mime_thaw (void)
{
	/* noop, get rid of this once all the mime editing stuff is gone */
}

/* this gives us a number of the language in the current language list,
   the lower the number the "better" the translation */
static int
language_level (const char *lang)
{
	const char * const *lang_list;
	int i;

	if (lang == NULL)
		lang = "C";

	/* The returned list is sorted from most desirable to least
            desirable and always contains the default locale "C". */
	lang_list = g_get_language_names();

	for (i = 0; lang_list[i]; i++)
		if (strcmp (lang_list[i], lang) == 0)
			return i;

	return -1;
}

static int
read_next (xmlTextReaderPtr reader) 
{
	int depth;
	int ret;
	
	depth = xmlTextReaderDepth (reader);
	
	ret = xmlTextReaderRead (reader);
	while (ret == 1) {
		if (xmlTextReaderDepth (reader) == depth) {
			return 1;
		} else if (xmlTextReaderDepth (reader) < depth) {
			return 0;
		}
		ret = xmlTextReaderRead (reader);
	}

	return ret;
}

static char *
handle_simple_string (xmlTextReaderPtr reader)
{
	int ret;
	char *text = NULL;

	ret = xmlTextReaderRead (reader);
	while (ret == 1) {
		xmlReaderTypes type;		
		type = xmlTextReaderNodeType (reader);
		if (type == XML_READER_TYPE_TEXT) {
			if (text != NULL) {
				g_free (text);
			}
			text = g_strdup ((char *)xmlTextReaderConstValue (reader));
		}

		ret = read_next (reader);
	}
	return text;
}

static char *
handle_attribute (xmlTextReaderPtr  reader,
		  const char       *attribute)
{
	xmlChar *xml_text = NULL;
	char *text = NULL;

	xml_text = xmlTextReaderGetAttribute (reader, (guchar *)attribute);
	if (xml_text != NULL) {
		text = g_strdup ((char *)xml_text);
		xmlFree (xml_text);
	}
	return text;
}

static MimeEntry *
handle_mime_info (const char *filename, xmlTextReaderPtr reader)
{
	MimeEntry *entry;
	int ret;
	int previous_lang_level = INT_MAX;
	
	entry = g_new0 (MimeEntry, 1);

	ret = xmlTextReaderRead (reader);
	while (ret == 1) {
		xmlReaderTypes type;
		type = xmlTextReaderNodeType (reader);		
		
		if (type == XML_READER_TYPE_ELEMENT) {
			const char *name;
			name = (char *)xmlTextReaderConstName (reader);
			
			if (!strcmp (name, "comment")) {
				const char *lang;
				int lang_level;
				
				lang = (char *)xmlTextReaderConstXmlLang (reader);
				
				lang_level = language_level (lang);
				
				if (lang_level != -1 &&
				    lang_level < previous_lang_level) {
					char *comment;
					comment = handle_simple_string (reader);
					g_free (entry->description);
					entry->description = comment;
					previous_lang_level = lang_level;
				}
			} else if (!strcmp (name, "sub-class-of")) {
				char *mime_type;
				mime_type = handle_attribute (reader, "type");
				if (entry->parent_classes) {
					char *new;
					new = g_strdup_printf ("%s:%s",
							       entry->parent_classes,
							       mime_type);
					g_free (entry->parent_classes);
					entry->parent_classes = new;
				} else {
					entry->parent_classes = g_strdup (mime_type);
				}
				g_free (mime_type);
			} else if (!strcmp (name, "alias")) {
				char *mime_type;
				mime_type = handle_attribute (reader, "type");
				if (entry->aliases) {
					char *new;
					new = g_strdup_printf ("%s:%s",
							       entry->aliases,
							       mime_type);
					g_free (entry->aliases);
					entry->aliases =new;
				} else {
					entry->aliases = g_strdup (mime_type);
				}
				g_free (mime_type);
			}
		}
		ret = read_next (reader);
	}

	if (ret == -1) {
		/* Zero out the mime entry, but put it in the cache anyway
		 * to avoid trying to reread */
		g_free (entry->description);
		g_warning ("couldn't parse %s\n", filename);
	}

	return entry;
}
 
static MimeEntry *
load_mime_entry (const char *mime_type, const char *filename)
{	
	MimeEntry *entry;
	xmlTextReaderPtr reader;
	int ret;

	reader = xmlNewTextReaderFilename (filename);

	if (!reader) {
		return NULL;
	}
	
	ret = xmlTextReaderRead (reader);
	
	entry = NULL;
	while (ret == 1) {
		if (xmlTextReaderNodeType (reader) == XML_READER_TYPE_ELEMENT) {
			if (entry) {
				g_warning ("two mime-info elements in %s", filename);
			} else {
				entry = handle_mime_info (filename, reader);
			}
		}
		ret = read_next (reader);
	}
	xmlFreeTextReader (reader);

	if (ret != 0 || entry == NULL) {
		mime_entry_free (entry);
		return NULL;
	}
		
	return entry;
}

static char *
get_mime_entry_path (const char *mime_type)
{
	GList *l;
	char *path;

	path = g_strdup_printf ("%s.xml", mime_type);
	
	if (G_DIR_SEPARATOR != '/') {
		char *p;
		for (p = path; *p != '\0'; p++) {
			if (*p == '/') {
				*p = G_DIR_SEPARATOR;
				break;
			}
		}
	}

	for (l = mime_directories; l != NULL; l = l->next) {
		char *full_path;
		MimeDirectory *dir = l->data;
		
		full_path = g_build_filename (dir->path, path, NULL);
		
		if (g_file_test (full_path, G_FILE_TEST_EXISTS)) {
			g_free (path);
			return full_path;
		}
		
		g_free (full_path);
	}

	g_free (path);
	
	return NULL;
}

static MimeEntry *
get_entry (const char *mime_type)
{
	const char *umime;
	MimeEntry *entry;
	char *path;

	G_LOCK (mate_vfs_mime_mutex);
	umime =	xdg_mime_unalias_mime_type (mime_type);
	G_UNLOCK (mate_vfs_mime_mutex);

	entry = g_hash_table_lookup (mime_entries, umime);
	
	if (entry) {
		if (entry == &mime_entries_no_entry)
			return NULL;
		return entry;
	}
	
	path = get_mime_entry_path (umime);

	if (path) {
		entry = load_mime_entry (umime, path);
		g_hash_table_insert (mime_entries, 
				     g_strdup (umime), 
				     entry);
		g_free (path);
		return entry;
	} else {
		g_hash_table_insert (mime_entries, 
				     g_strdup (umime), 
				     &mime_entries_no_entry);
		return NULL;
	}
}

/**
 * mate_vfs_mime_set_value:
 * @mime_type: a mime type.
 * @key: a key to store the value in.
 * @value: the value to store in the @key.
 *
 * This function will set the @value for the @key and it will save it
 * to the user' file if necessary.
 * You should not free the @key/@value passed to this function.
 * They are used internally.
 *
 * Returns: %MATE_VFS_OK if the operation succeeded, otherwise an error code.
 */
MateVFSResult
mate_vfs_mime_set_value (const char *mime_type, const char *key, const char *value)
{
	/* Remove once */
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

/**
 * mate_vfs_mime_get_value:
 * @mime_type: a mime type.
 * @key: a key to lookup for the given @mime_type.
 *
 * This function retrieves the value associated with @key in
 * the given #MateMimeContext.  The string is private, you
 * should not free the result.
 *
 * Returns: %MATE_VFS_OK if the operation succeeded, otherwise an error code.
 */
const char *
mate_vfs_mime_get_value (const char *mime_type, const char *key)
{
	MimeEntry *entry;
	
	if (!mate_vfs_mime_inited)
		mate_vfs_mime_init ();

	entry = get_entry (mime_type);

	if (!entry) {
		return NULL;
	}
	
	if (!strcmp (key, "description")) {
		return entry->description;
	} else if (!strcmp (key, "parent_classes")) {
		return entry->parent_classes;
	} else if (!strcmp (key, "aliases")) {
		return entry->aliases;
	} else if (!strcmp (key, "can_be_executable")) {
		if (mate_vfs_mime_type_get_equivalence (mime_type, "application/x-executable") != MATE_VFS_MIME_UNRELATED ||
		    mate_vfs_mime_type_get_equivalence (mime_type, "text/plain") != MATE_VFS_MIME_UNRELATED)
			return "TRUE";
	}

	return NULL;
}

/**
 * mate_vfs_mime_type_is_known:
 * @mime_type: a mime type.
 *
 * This function returns %TRUE if @mime_type is in the MIME database at all.
 *
 * Returns: %TRUE if anything is known about @mime_type, otherwise %FALSE.
 */
gboolean
mate_vfs_mime_type_is_known (const char *mime_type)
{
	MimeEntry *entry;
	
	if (mime_type == NULL) {
		return FALSE;
	}

	g_return_val_if_fail (!does_string_contain_caps (mime_type),
			      FALSE);

	if (!mate_vfs_mime_inited)
		mate_vfs_mime_init ();

	reload_if_needed ();

	entry = get_entry (mime_type);
	
	return entry != NULL;
}

/**
 * mate_vfs_mime_get_extensions_list:
 * @mime_type: mime type to get the extensions of.
 *
 * Get the file extensions associated with mime type @mime_type.
 *
 * Return value: a #GList of char *s.
 */
GList *
mate_vfs_mime_get_extensions_list (const char *mime_type)
{
	return NULL;
}


/**
 * mate_vfs_mime_extensions_list_free:
 * @list: the extensions list.
 *
 * Call this function on the list returned by mate_vfs_mime_get_extensions_list()
 * to free the @list and all of its elements.
 */
void
mate_vfs_mime_extensions_list_free (GList *list)
{
	if (list == NULL) {
		return;
	}
	g_list_foreach (list, (GFunc) g_free, NULL);
	g_list_free (list);
}

void
_mate_vfs_mime_info_mark_user_mime_dir_dirty (void)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
}


void
_mate_vfs_mime_info_mark_mate_mime_dir_dirty (void)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
}

MateVFSResult
mate_vfs_mime_set_registered_type_key (const char *mime_type, 
					const char *key,
					const char *value)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

GList *
mate_vfs_get_registered_mime_types (void)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return NULL;
}


void
mate_vfs_mime_registered_mime_type_delete (const char *mime_type)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
}

void
mate_vfs_mime_registered_mime_type_list_free (GList *list)
{
	if (list == NULL) {
		return;
	}

	g_list_foreach (list, (GFunc) g_free, NULL);
	g_list_free (list);
}

char *
mate_vfs_mime_get_extensions_pretty_string (const char *mime_type)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return NULL;
}

char *
mate_vfs_mime_get_extensions_string (const char *mime_type)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return NULL;
}

GList *
mate_vfs_mime_get_key_list (const char *mime_type)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return NULL;
}

void
mate_vfs_mime_keys_list_free (GList *mime_type_list)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	g_list_free (mime_type_list);
}


void
mate_vfs_mime_reset (void)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
}

MateVFSResult
mate_vfs_mime_set_extensions_list (const char *mime_type,
				    const char *extensions_list)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}
