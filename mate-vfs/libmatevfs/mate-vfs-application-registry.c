/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-mime-info.c - MATE mime-information implementation.

   Copyright (C) 1998 Miguel de Icaza
   Copyright (C) 2000 Eazel, Inc
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
   Boston, MA 02111-1307, USA.  */
/*
 * Authors: George Lebl
 * 	Based on original mime-info database code by Miguel de Icaza
 */

#include "config.h"
#include "mate-vfs-application-registry.h"
#include "mate-vfs-configuration.h"
#include "mate-vfs-mime-handlers.h"
#include "mate-vfs-mime-private.h"
#include "mate-vfs-mime.h"
#include "mate-vfs-private-utils.h"
#include "mate-vfs-result.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <glib/gstdio.h>

#if !defined getc_unlocked && !defined HAVE_GETC_UNLOCKED
# define getc_unlocked(fp) getc (fp)
#endif

typedef struct _Application Application;
struct _Application {
	char       *app_id;
	int         ref_count;
	/* The following is true if this was found in the
	 * home directory or if the user changed any keys
	 * here.  It means that it will be saved into a user
	 * file */
	gboolean    user_owned;
	GHashTable *keys;
	MateVFSMimeApplicationArgumentType expects_uris;
	GList      *mime_types;
	GList      *supported_uri_schemes;
	/* The user_owned version of this if this is a system
	 * version */
	Application *user_application;
};

/* Describes the directories we scan for information */
typedef struct {
	char *dirname;
	unsigned int valid : 1;
	unsigned int system_dir : 1;
} ApplicationRegistryDir;

/* These ones are used to automatically reload mime info on demand */
static FileDateTracker *registry_date_tracker;
static ApplicationRegistryDir mate_registry_dir;
static ApplicationRegistryDir user_registry_dir;

/* To initialize the module automatically */
static gboolean mate_vfs_application_registry_initialized = FALSE;

/* we want to replace the previous key if the current key has a lower
   (ie: better) language level */
static char *previous_key = NULL;
static int previous_key_lang_level = -1;

/*
 * A hash table containing application registry record (Application)
 * keyed by application ids.
 */
static GHashTable *global_applications = NULL;
/*
 * A hash table containing GList's of application registry records (Application)
 * keyed by the mime types
 */
/* specific mime_types (e.g. image/png) */
static GHashTable *specific_mime_types = NULL;
/* generic mime_types (e.g. image/<star>) */
static GHashTable *generic_mime_types = NULL;

/*
 * Dirty flag, just to make sure we don't sync needlessly
 */
static gboolean user_file_dirty = FALSE;

/*
 * Some local prototypes
 */
static void mate_vfs_application_registry_init (void);
static void application_clear_mime_types (Application *application);

static Application *
application_ref (Application *application)
{
	g_return_val_if_fail(application != NULL, NULL);

	application->ref_count ++;

	return application;
}

static void
hash_foreach_free_key_value(gpointer key, gpointer value, gpointer user_data)
{
	g_free(key);
	g_free(value);
}

static void
application_unref (Application *application)
{
	g_return_if_fail(application != NULL);

	application->ref_count --;

	if (application->ref_count == 0) {
		application_clear_mime_types (application);

		if (application->keys != NULL) {
			g_hash_table_foreach(application->keys, hash_foreach_free_key_value, NULL);
			g_hash_table_destroy(application->keys);
			application->keys = NULL;
		}

		g_free(application->app_id);
		application->app_id = NULL;

		if (application->user_application != NULL) {
			application_unref (application->user_application);
			application->user_application = NULL;
		}

		g_free(application);
	}
}

static Application *
application_new (const char *app_id, gboolean user_owned)
{
	Application *application;

	application = g_new0 (Application, 1);
	application->app_id = g_strdup(app_id);
	application->ref_count = 1;
	application->expects_uris = MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_PATHS;
	application->user_owned = user_owned;

	return application;
}

static Application *
application_lookup_or_create (const char *app_id, gboolean user_owned)
{
	Application *application;

	g_return_val_if_fail(app_id != NULL, NULL);

	mate_vfs_application_registry_init ();

	application = g_hash_table_lookup (global_applications, app_id);
	if (application != NULL) {
		if ( ! user_owned) {
			/* if we find only a user app, do magic */
			if (application->user_owned) {
				Application *new_application;
				new_application = application_new (app_id, FALSE/*user_owned*/);
				new_application->user_application = application;
				/* override the user application */
				g_hash_table_insert (global_applications, new_application->app_id,
						     new_application);
				return new_application;
			} else {
				return application;
			}
		} else {
			if (application->user_owned) {
				return application;
			} if (application->user_application != NULL) {
				return application->user_application;
			} else {
				Application *new_application;
				new_application = application_new (app_id, TRUE/*user_owned*/);
				application->user_application = new_application;
				return new_application;
			}
		}
	}

	application = application_new (app_id, user_owned);

	g_hash_table_insert (global_applications, application->app_id, application);

	return application;
}

static Application *
application_lookup (const char *app_id)
{
	g_return_val_if_fail(app_id != NULL, NULL);

	if (global_applications == NULL)
		return NULL;

	return g_hash_table_lookup (global_applications, app_id);
}

static const char *
peek_value (const Application *application, const char *key)
{
	g_return_val_if_fail(application != NULL, NULL);
	g_return_val_if_fail(key != NULL, NULL);

	if (application->keys == NULL)
		return NULL;

	return g_hash_table_lookup (application->keys, key);
}

static void
set_value (Application *application, const char *key, const char *value)
{
	char *old_value, *old_key;

	g_return_if_fail(application != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(value != NULL);

	if (application->keys == NULL)
		application->keys = g_hash_table_new (g_str_hash, g_str_equal);

	if (g_hash_table_lookup_extended (application->keys, key,
					  (gpointer *)&old_key,
					  (gpointer *)&old_value)) {
		g_hash_table_insert (application->keys,
				     old_key, g_strdup (value));
		g_free (old_value);
	} else {
		g_hash_table_insert (application->keys,
				     g_strdup (key), g_strdup (value));
	}
}

static void
unset_key (Application *application, const char *key)
{
	char *old_value, *old_key;

	g_return_if_fail(application != NULL);
	g_return_if_fail(key != NULL);

	if (application->keys == NULL)
		return;

	if (g_hash_table_lookup_extended (application->keys, key,
					  (gpointer *)&old_key,
					  (gpointer *)&old_value)) {
		g_hash_table_remove (application->keys, old_key);
		g_free (old_key);
		g_free (old_value);
	}
}

static gboolean
value_looks_true (const char *value)
{
	if (value &&
	    (value[0] == 'T' ||
	     value[0] == 't' ||
	     value[0] == 'Y' ||
	     value[0] == 'y' ||
	     atoi (value) != 0)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean
get_bool_value (const Application *application, const char *key,
		gboolean *got_key)
{
	const char *value = peek_value (application, key);
	if (got_key) {
		if (value != NULL)
			*got_key = TRUE;
		else
			*got_key = FALSE;
	}
	return value_looks_true (value);

}

static void
set_bool_value (Application *application, const char *key,
		gboolean value)
{
	set_value (application, key, value ? "true" : "false");
}

static int
application_compare (Application *application1,
		     Application *application2)
{
	return strcmp (application1->app_id, application2->app_id);
}

static void
add_application_to_mime_type_table (Application *application,
				    const char *mime_type)
{
	GList *application_list;
	GHashTable *table;
	char *old_key;

	if (mate_vfs_mime_type_is_supertype (mime_type))
		table = generic_mime_types;
	else
		table = specific_mime_types;
	
	g_assert (table != NULL);

	if (g_hash_table_lookup_extended (table, mime_type,
					  (gpointer *)&old_key,
					  (gpointer *)&application_list)) {
		/* Sorted order is important as we can then easily
		 * uniquify the results */
		application_list = g_list_insert_sorted
			(application_list,
			 application_ref (application),
			 (GCompareFunc) application_compare);
		g_hash_table_insert (table, old_key, application_list);
	} else {
		application_list = g_list_prepend (NULL,
						   application_ref (application));
		g_hash_table_insert (table, g_strdup (mime_type), application_list);
	}
}

static void
add_mime_type_to_application (Application *application, const char *mime_type)
{
	/* if this exists already, just return */
	if (g_list_find_custom (application->mime_types,
				/*glib is const incorrect*/(gpointer)mime_type,
				(GCompareFunc) strcmp) != NULL)
		return;
	
	application->mime_types =
		g_list_prepend (application->mime_types,
				g_strdup (mime_type));
	
	add_application_to_mime_type_table (application, mime_type);

}

static void
add_supported_uri_scheme_to_application (Application *application,
					 const char *supported_uri_scheme)
{
	if (g_list_find_custom (application->supported_uri_schemes,
				/*glib is const incorrect*/(gpointer) supported_uri_scheme,
				(GCompareFunc) strcmp) != NULL) {
		return;
	}
	
	application->supported_uri_schemes =
		g_list_prepend (application->supported_uri_schemes,
				g_strdup (supported_uri_scheme));

}

static GList *
supported_uri_scheme_list_copy (GList *supported_uri_schemes)
{
	GList *copied_list, *node;

	copied_list = NULL;
	for (node = supported_uri_schemes; node != NULL;
	     node = node->next) {
		copied_list = g_list_prepend (copied_list,
					      g_strdup ((char *) node->data));
	}

	return copied_list;
}

static void
remove_application_from_mime_type_table (Application *application,
					 const char *mime_type)
{
	GHashTable *table;
	char *old_key;
	GList *application_list, *entry;

	if (mate_vfs_mime_type_is_supertype (mime_type))
		table = generic_mime_types;
	else
		table = specific_mime_types;

	g_assert (table != NULL);

	if (g_hash_table_lookup_extended (table, mime_type,
					  (gpointer *)&old_key,
					  (gpointer *)&application_list)) {
		entry = g_list_find (application_list, application);

		/* if this fails we're in deep doodoo I guess */
		g_assert (entry != NULL);

		application_list = g_list_remove_link (application_list, entry);
		entry->data = NULL;
		application_unref (application);

		if (application_list != NULL) {
			g_hash_table_insert (table, old_key, application_list);
		} else {
			g_hash_table_remove (table, old_key);
			g_free(old_key);
		}
	} else
		g_assert_not_reached ();
}

static void
remove_mime_type_for_application (Application *application, const char *mime_type)
{
	GList *entry;

	g_return_if_fail(application != NULL);
	g_return_if_fail(mime_type != NULL);

	entry = g_list_find_custom
		(application->mime_types,
		 /*glib is const incorrect*/(gpointer)mime_type,
		 (GCompareFunc) strcmp);

	/* if this doesn't exist, just return */
	if (entry == NULL) {
		return;
	}

	remove_application_from_mime_type_table (application, mime_type);

	/* Free data last, in case caller passed in mime_type string
	 * that was stored in this table.
	 */
	application->mime_types =
		g_list_remove_link (application->mime_types, entry);
	g_free (entry->data);
	g_list_free_1 (entry);	
}


static void
application_clear_mime_types (Application *application)
{
	g_return_if_fail (application != NULL);

	while (application->mime_types)
		remove_mime_type_for_application (application, application->mime_types->data);
}

static void
application_remove (Application *application)
{
	Application *main_application;

	g_return_if_fail (application != NULL);

	if (global_applications == NULL) {
		return;
	}

	main_application = application_lookup (application->app_id);
	if (main_application == NULL) {
		return;
	}

	/* We make sure the mime types are killed even if the application
	 * entry lives after unreffing it */
	application_clear_mime_types (application);

	if (main_application == application) {
		if (application->user_application)
			application_clear_mime_types (application->user_application);

		g_hash_table_remove (global_applications, application->app_id);
	} else {
		/* This must be a user application */
		g_assert (main_application->user_application == application);

		main_application->user_application = NULL;
	}

	application_unref (application);

}

static void
sync_key (gpointer key, gpointer value, gpointer user_data)
{
	char *key_string = key;
	char *value_string = value;
	FILE *fp = user_data;

	fprintf (fp, "\t%s=%s\n", key_string, value_string);
}

/* write an application to a file */
static void
application_sync (Application *application, FILE *fp)
{
	GList *li;

	g_return_if_fail (application != NULL);
	g_return_if_fail (application->app_id != NULL);
	g_return_if_fail (fp != NULL);

	fprintf (fp, "%s\n", application->app_id);

	if (application->keys != NULL)
		g_hash_table_foreach (application->keys, sync_key, fp);

	if (application->mime_types != NULL) {
		char *separator;
		fprintf (fp, "\tmime_types=");
		separator = "";
		for (li = application->mime_types; li != NULL; li = li->next) {
			char *mime_type = li->data;
			fprintf (fp, "%s%s", separator, mime_type);
			separator = ",";
		}
		fprintf (fp, "\n");
	}
	fprintf (fp, "\n");
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

static void
application_add_key (Application *application, const char *key,
		     const char *lang, const char *value)
{
	int lang_level;

	g_return_if_fail (application != NULL);
	g_return_if_fail (key != NULL);
	g_return_if_fail (value != NULL);

	if (strcmp (key, "mime_types") == 0 ||
	    strcmp (key, "supported_uri_schemes") == 0) {
		char *value_copy = g_strdup (value);
		char *next_value;
		/* FIXME: There used to be a check here for
		   the value of "lang", but spamming
		   the terminal about it is not really
		   the right way to deal with that, nor
		   is "MIME Types can't have languages, bad!"
		   which is what was here before */
		next_value = strtok (value_copy, ", \t");
		while (next_value != NULL) {
			if (strcmp (key, "mime_types") == 0) {
				add_mime_type_to_application (application, next_value);
			}
			else {
				add_supported_uri_scheme_to_application (application, next_value);
			}
			next_value = strtok (NULL, ", \t");
		}
		g_free(value_copy);
		/* fall through so that we can store the values as keys too */
	}
	else if (strcmp (key, "expects_uris") == 0) {
		if (strcmp (value, "non-file") == 0) {
			application->expects_uris = MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS_FOR_NON_FILES;
		}
		else if (value_looks_true (value)) {
			application->expects_uris = MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS;
		}
		else {
			application->expects_uris = MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_PATHS;
		}
	}

	lang_level = language_level (lang);
	/* wrong language completely */
	if (lang_level < 0)
		return;

	/* if we have some language defined and
	   if there was a previous_key */
	if (lang_level > 0 &&
	    previous_key &&
	    /* our language level really sucks and the previous
	       translation was of better language quality so just
	       ignore us */
	    previous_key_lang_level < lang_level) {
		return;
	}

	set_value (application, key, value);

	/* set this as the previous key */
	g_free(previous_key);
	previous_key = g_strdup(key);
	previous_key_lang_level = lang_level;
}

typedef enum {
	STATE_NONE,
	STATE_LANG,
	STATE_LOOKING_FOR_KEY,
	STATE_ON_APPLICATION,
	STATE_ON_KEY,
	STATE_ON_VALUE
} ParserState;

/**
 * strip_trailing_whitespace
 *
 * string
 *
 * strips the white space from a string.
 *
 */

static void
strip_trailing_whitespace (GString *string)
{
	int i;

	for (i = string->len - 1; i >= 0; i--) {
		if (!g_ascii_isspace (string->str[i]))
			break;
	}

	g_string_truncate (string, i + 1);
}

/**
 * load_application_info_from
 *
 * filename:  Target filename to application info from.
 * user_owned: if application is user owned or not.
 *
 * This function will load application info from a file and parse through the
 * application loading the registry with the information contained in the file.
 *
 * 
 **/

static void
load_application_info_from (const char *filename, gboolean user_owned)
{
	FILE *fp;
	gboolean in_comment, app_used;
	GString *line;
	int column, c;
	ParserState state;
	Application *application;
	char *key;
	char *lang;

	fp = g_fopen (filename, "r");
	if (fp == NULL)
		return;

	in_comment = FALSE;
	app_used = FALSE;
	column = -1;
	application = NULL;
	key = NULL;
	lang = NULL;
	line = g_string_sized_new (120);
	state = STATE_NONE;
	
	while ((c = getc_unlocked (fp)) != EOF){
		column++;
		if (c == '\r')
			continue;

		if (c == '#' && column == 0){		
			in_comment = TRUE;
			continue;
		}
		
		if (c == '\n'){
			in_comment = FALSE;
			column = -1;
			if (state == STATE_ON_APPLICATION) {

				/* set previous key to nothing
				   for this mime type */
				g_free(previous_key);
				previous_key = NULL;
				previous_key_lang_level = -1;

				strip_trailing_whitespace (line);
				application = application_lookup_or_create (line->str, user_owned);
				app_used = FALSE;
				g_string_assign (line, "");
				state = STATE_LOOKING_FOR_KEY;
				continue;
			}
			if (state == STATE_ON_VALUE){
				app_used = TRUE;
				application_add_key (application, key, lang, line->str);
				g_string_assign (line, "");
				g_free (key);
				key = NULL;
				g_free (lang);
				lang = NULL;
				state = STATE_LOOKING_FOR_KEY;
				continue;
			}
			continue;
		}

		if (in_comment)
			continue;

		switch (state){
		case STATE_NONE:
			if (c != ' ' && c != '\t')
				state = STATE_ON_APPLICATION;
			else
				break;
			/* fall down */
			
		case STATE_ON_APPLICATION:
			if (c == ':'){
				in_comment = TRUE;
				break;
			}
			g_string_append_c (line, c);
			break;

		case STATE_LOOKING_FOR_KEY:
			if (c == '\t' || c == ' ')
				break;

			if (c == '['){
				state = STATE_LANG;
				break;
			}

			if (column == 0){
				state = STATE_ON_APPLICATION;
				g_string_append_c (line, c);
				break;
			}
			state = STATE_ON_KEY;
			/* falldown */

		case STATE_ON_KEY:
			if (c == '\\'){
				c = getc (fp);
				if (c == EOF)
					break;
			}
			if (c == '='){
				key = g_strdup (line->str);
				g_string_assign (line, "");
				state = STATE_ON_VALUE;
				break;
			}
			g_string_append_c (line, c);
			break;

		case STATE_ON_VALUE:
			g_string_append_c (line, c);
			break;
			
		case STATE_LANG:
			if (c == ']'){
				state = STATE_ON_KEY;      
				if (line->str [0]){
					g_free(lang);
					lang = g_strdup(line->str);
				} else {
					in_comment = TRUE;
					state = STATE_LOOKING_FOR_KEY;
				}
				g_string_assign (line, "");
				break;
			}
			g_string_append_c (line, c);
			break;
		}
	}

	if (application){
		if (key && line->str [0])
			application_add_key (application, key, lang, line->str);
		else
			if ( ! app_used)
				application_remove (application);
	}

	g_string_free (line, TRUE);
	g_free (key);
	g_free (lang);

	/* free the previous_key stuff */
	g_free(previous_key);
	previous_key = NULL;
	previous_key_lang_level = -1;

	fclose (fp);

	_mate_vfs_file_date_tracker_start_tracking_file (registry_date_tracker, filename);
}

/**
 * application_info_load
 *
 * source:
 *
 * 
 */

static void
application_info_load (ApplicationRegistryDir *source)
{
	GDir *dir;
	const char *dent;
	const int extlen = sizeof (".applications") - 1;
	char *filename;
	struct stat s;

	if (g_stat (source->dirname, &s) != -1)
		source->valid = TRUE;
	else
		source->valid = FALSE;
	
	dir = g_dir_open (source->dirname, 0, NULL);
	if (dir == NULL) {
		source->valid = FALSE;
		return;
	}
	if (source->system_dir) {
		filename = g_build_filename (source->dirname,
					     "mate-vfs.applications",
					     NULL);
		load_application_info_from (filename, FALSE /*user_owned*/);
		g_free (filename);
	}

	while ((dent = g_dir_read_name (dir)) != NULL){
		
		int len = strlen (dent);

		if (len <= extlen)
			continue;
		if (strcmp (dent + len - extlen, ".applications"))
			continue;
		if (source->system_dir && strcmp (dent, "mate-vfs.applications") == 0)
			continue;
		if ( ! source->system_dir && strcmp (dent, "user.applications") == 0)
			continue;
		filename = g_build_filename (source->dirname, dent, NULL);
		load_application_info_from (filename, FALSE /*user_owned*/);
		g_free (filename);
	}

	if ( ! source->system_dir) {
		filename = g_build_filename (source->dirname, "user.applications", NULL);
		/* Currently this is the only file that is "user
		 * owned".  It actually makes sense.  Editting of
		 * other files from the API would be too complex
		 */
		load_application_info_from (filename, TRUE /*user_owned*/);
		g_free (filename);
	}
	g_dir_close (dir);

	_mate_vfs_file_date_tracker_start_tracking_file (registry_date_tracker, source->dirname);
}

/**
 * load_application_info
 *
 * This function will load the registry for an application from disk.
 **/

static void
load_application_info (void)
{
	application_info_load (&mate_registry_dir);
	application_info_load (&user_registry_dir);
}

/**
 * mate_vfs_application_registry_init
 *
 * This function initializes the mate-vfs application registry.  
 *
 **/

static void
mate_vfs_application_registry_init (void)
{
	if (mate_vfs_application_registry_initialized)
		return;

	registry_date_tracker = _mate_vfs_file_date_tracker_new ();

	/*
	 * The hash tables that store the mime keys.
	 */
	global_applications = g_hash_table_new (g_str_hash, g_str_equal);
	generic_mime_types  = g_hash_table_new (g_str_hash, g_str_equal);
	specific_mime_types  = g_hash_table_new (g_str_hash, g_str_equal);
	
	/*
	 * Setup the descriptors for the information loading
	 */

	mate_registry_dir.dirname = g_build_filename (MATE_VFS_DATADIR,
						       "application-registry",
						       NULL);
	mate_registry_dir.system_dir = TRUE;
	
	user_registry_dir.dirname = g_build_filename (g_get_home_dir(),
						      ".mate",
						      "application-info",
						      NULL);
	user_registry_dir.system_dir = FALSE;

	/* Make sure user directory exists */
	if (g_mkdir (user_registry_dir.dirname, 0700) &&
	    errno != EEXIST) {
		g_warning("Could not create per-user Mate application-registry directory: %s",
			  user_registry_dir.dirname);
	}

	/* Things have been initialized flag it as ready so that we can load
	 * the applications without attempting to reinitialize
	 */
	mate_vfs_application_registry_initialized = TRUE;

	load_application_info ();
}

/*
 * maybe_reload
 *
 * This function will initialize the registry in memory and then reloads the
 *
 */

static void
maybe_reload (void)
{
	mate_vfs_application_registry_init ();

	if (!_mate_vfs_file_date_tracker_date_has_changed (registry_date_tracker)) {
		return;
	}
	
        mate_vfs_application_registry_reload ();
}

/**
 * remove_apps
 *
 * key: 
 * value:
 * user_data:
 *
 * FIXME: I need a clearer explanation on what this does.
 *
 */

static gboolean
remove_apps (gpointer key, gpointer value, gpointer user_data)
{
	Application *application = value;

	application_clear_mime_types (application);

	application_unref (application);
	
	return TRUE;
}

/**
 * mate_vfs_application_registry_clear:
 *
 * This will wipe the registry clean removing everything from the registry.
 * This is different from mate_vfs_application_registry_shutdown which will
 * actually delete the registry and leave it in an uninitialized state.
 *

 */

static void
mate_vfs_application_registry_clear (void)
{
	if (global_applications != NULL)
		g_hash_table_foreach_remove (global_applications, remove_apps, NULL);
}

/**
 * mate_vfs_application_registry_shutdown
 *
 * Synchronize mate-vfs application registry data to disk, and free
 * resources.
 * 
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 */

void
mate_vfs_application_registry_shutdown (void)
{
	mate_vfs_application_registry_clear ();

	if (global_applications != NULL) {
		g_hash_table_destroy (global_applications);
		global_applications = NULL;
	}

	if(generic_mime_types != NULL) {
		g_hash_table_destroy (generic_mime_types);
		generic_mime_types = NULL;
	}

	if(specific_mime_types != NULL) {
		g_hash_table_destroy (specific_mime_types);
		specific_mime_types = NULL;
	}

	_mate_vfs_file_date_tracker_free (registry_date_tracker);

	g_free(mate_registry_dir.dirname);
	mate_registry_dir.dirname = NULL;
	g_free(user_registry_dir.dirname);
	user_registry_dir.dirname = NULL;

	mate_vfs_application_registry_initialized = FALSE;
}


/**
 * mate_vfs_application_registry_reload
 *
 * If this function is called for the first time it will initialize the
 * registry.  Subsequent calls to the function will clear out the current
 * registry contents and load registry contents from the save file.  Make
 * certain that you've saved your registry before calling this function.  It
 * will destroy unsaved changes.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 */

void
mate_vfs_application_registry_reload (void)
{
	if ( ! mate_vfs_application_registry_initialized) {
		/* If not initialized, initialization will do a "reload" */
		mate_vfs_application_registry_init ();
	} else {
		mate_vfs_application_registry_clear ();
		load_application_info ();
	}
}

/*
 * Existance check
 */

/**
 * mate_vfs_application_registry_exists
 * @app_id: an application ID
 *
 * This function will return TRUE if there is an entry for @app_id in
 * the registry, otherwise FALSE.
 *
 * Returns: TRUE if the application is in the registry, FALSE if not 
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

gboolean
mate_vfs_application_registry_exists (const char *app_id)
{
	g_return_val_if_fail (app_id != NULL, FALSE);

	maybe_reload ();

	if (application_lookup (app_id) != NULL)
		return TRUE;
	else
		return FALSE;
}


/*
 * Getting arbitrary keys
 */


/**
 * get_keys_foreach
 *
 * key:
 * user_data:
 *
 * FIXME: I have no idea what this function does.
 **/

static void
get_keys_foreach(gpointer key, gpointer value, gpointer user_data)
{
	GList **listp = user_data;

	/* make sure we only insert unique keys */
	if ( (*listp) && strcmp ((const char *) (*listp)->data, (const char *) key) == 0)
		return;

	(*listp) = g_list_insert_sorted ((*listp), key,
					 (GCompareFunc) strcmp);
}

/**
 * mate_vfs_application_registry_get_keys:
 * @app_id: the application ID for which to get keys
 * 
 * This function wil return a list of strings which is the list of
 * keys set for @app_id in the application registry.
 *
 * Returns: A list of the keys set for @app_id
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

GList *
mate_vfs_application_registry_get_keys (const char *app_id)
{
	GList *retval;
	Application *application;

	g_return_val_if_fail (app_id != NULL, NULL);

	maybe_reload ();

	application = application_lookup (app_id);
	if (application == NULL)
		return NULL;

	retval = NULL;

	if (application->keys != NULL)
		g_hash_table_foreach (application->keys, get_keys_foreach,
				      &retval);

	if (application->user_application != NULL &&
	    application->user_application->keys)
		g_hash_table_foreach (application->user_application->keys,
				      get_keys_foreach, &retval);

	return retval;
}


/**
 * real_peek_value:
 *
 * @application: registry application
 * @key: target key
 *
 * Returns: the value associated with the key, or NULL if there is no
 * associated value.
 *
 * This function looks and returns the value of the target key in the registry
 * application
 *
 */

static const char *
real_peek_value (const Application *application, const char *key)
{
	const char *retval;

	g_return_val_if_fail (application != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	retval = NULL;

	if (application->user_application)
		retval = peek_value (application->user_application, key);

	if (retval == NULL)
		retval = peek_value (application, key);

	return retval;
}

/**
 * real_get_bool_value
 *
 * application:  Application structure
 * key: taget key
 * got_key: actual key stored in application if key exists.
 *
 * This function will try to determine whether a key exists in the application.
 * It first checks the user applications and then the system applications and
 * then returns whether the key exists and what the value is from the value of
 * got_key.
 *
 * Returns: gboolean
 **/

static gboolean
real_get_bool_value (const Application *application, const char *key, gboolean *got_key)
{
	gboolean sub_got_key, retval;

	g_return_val_if_fail (application != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	sub_got_key = FALSE;
	retval = FALSE;
	if (application->user_application)
		retval = get_bool_value (application->user_application, key,
					 &sub_got_key);

	if ( ! sub_got_key)
		retval = get_bool_value (application, key, &sub_got_key);

	if (got_key != NULL)
		*got_key = sub_got_key;

	return retval;
}


/**
 * mate_vfs_application_registry_peek_value
 * @app_id: the application ID for which to look up a value
 * @key: the key to look up
 *
 * This will return the value associated with @key for @app_id in the
 * application registry. There is no need to free the return value.
 *
 * Returns: the value associated with the key, or NULL if there is no
 * associated value
 *
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 */

const char *
mate_vfs_application_registry_peek_value (const char *app_id, const char *key)
{
	Application *application;

	g_return_val_if_fail (app_id != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	maybe_reload ();

	application = application_lookup (app_id);
	if (application == NULL)
		return NULL;

	return real_peek_value (application, key);
}

/**
 * mate_vfs_application_registry_get_bool_value
 * @app_id:  registry id of the application
 * @key: key to look up
 * @got_key: TRUE if a setting was dound, otherwise FALSE
 *
 * This will look up a key in the structure pointed to by app_id and return the
 * boolean value of that key.  It will return false if there are no
 * applications associated with the app_id.
 *
 * Returns: TRUE if @key is set to "true" or "yes" for @app_id, otherwise FALSE
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

gboolean
mate_vfs_application_registry_get_bool_value (const char *app_id, const char *key,
					       gboolean *got_key)
{
	Application *application;

	g_return_val_if_fail (app_id != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	maybe_reload ();

	application = application_lookup (app_id);
	if (application == NULL)
		return FALSE;

	return real_get_bool_value (application, key, got_key);
}

/*
 * Setting stuff
 */

/**
 * mate_vfs_application_registry_remove_application
 * @app_id:  registry id of the application
 *
 * Given the registry id this function will remove all applications that has
 * been set by the user.  You will need to call
 * mate_vfs_application_registry_sync to save the changes.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

void
mate_vfs_application_registry_remove_application (const char *app_id)
{
	Application *application;

	g_return_if_fail (app_id != NULL);

	maybe_reload ();

	application = application_lookup (app_id);
	if (application == NULL)
		return;

	/* Only remove the user_owned stuff */
	if (application->user_owned) {
		application_remove (application);
		user_file_dirty = TRUE;
	} else if (application->user_application != NULL) {
		application_remove (application->user_application);
		user_file_dirty = TRUE;
	}
}

/**
 * mate_vfs_application_registry_set_value
 * @app_id:  registry id of the application
 * @key: target key
 * @value: value to set the target key to
 *
 * This function will set values pertaining to registry entry pointed to by
 * app_id.  You will need to call mate_vfs_application_registry_sync to
 * realize the changes.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

void
mate_vfs_application_registry_set_value (const char *app_id,
					  const char *key,
					  const char *value)
{
	Application *application;

	g_return_if_fail (app_id != NULL);
	g_return_if_fail (key != NULL);
	g_return_if_fail (value != NULL);

	maybe_reload ();

	application = application_lookup_or_create (app_id, TRUE/*user_owned*/);

	set_value (application, key, value);

	user_file_dirty = TRUE;
}

/**
 * mate_vfs_application_registry_set_bool_value:
 * @app_id:  registry id of the application
 * @key: target key
 * @value: value you want to set the target key to
 *
 * This function will modify those registry values that are of type boolean to
 * a value specified by the user.  You will need to call
 * mate_vfs_application_registry_sync to save your changes.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */
void
mate_vfs_application_registry_set_bool_value (const char *app_id,
					       const char *key,
					       gboolean value)
{
	Application *application;

	g_return_if_fail (app_id != NULL);
	g_return_if_fail (key != NULL);

	maybe_reload ();

	application = application_lookup_or_create (app_id, TRUE/*user_owned*/);

	set_bool_value (application, key, value);

	user_file_dirty = TRUE;
}

/**
 * mate_vfs_application_registry_unset_key:
 * @app_id: registry id of the application
 * @key: search key
 *
 * This function given the application and the target will wipe the current
 * value that the key contains.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 * 
 */

void
mate_vfs_application_registry_unset_key (const char *app_id,
					  const char *key)
{
	Application *application;

	g_return_if_fail (app_id != NULL);
	g_return_if_fail (key != NULL);

	maybe_reload ();

	application = application_lookup_or_create (app_id, TRUE/*user_owned*/);

	unset_key (application, key);

	user_file_dirty = TRUE;
}

/*
 * Query functions
 */


static void
cb_application_collect (gpointer key, gpointer value, gpointer user_data)
{
	Application  *application = value;
	GList       **list = user_data;
	*list = g_list_prepend (*list, application->app_id);
}

/**
 * mate_vfs_application_registry_get_applications
 * @mime_type:  mime type string
 *
 * This will return all applications from the registry that are associated with
 * the given mime type string, if NULL it returns all applications.
 *
 * Returns: a list of the application IDs for all applications which
 * support the given mime type.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

GList *
mate_vfs_application_registry_get_applications (const char *mime_type)
{
	GList *app_list, *app_list2, *retval, *li;
	char *supertype;

	retval = NULL;
	app_list2 = NULL;

	maybe_reload ();

	if (mime_type == NULL) {
		g_hash_table_foreach (global_applications, cb_application_collect, &retval);
		return retval;
	}

	if (mate_vfs_mime_type_is_supertype (mime_type)) {
		app_list = g_hash_table_lookup (generic_mime_types, mime_type);
	} else {
		app_list = g_hash_table_lookup (specific_mime_types, mime_type);

		supertype = mate_vfs_get_supertype_from_mime_type (mime_type);
		if (supertype != NULL) {
			app_list2 = g_hash_table_lookup (generic_mime_types, supertype);
			g_free (supertype);
		}
	}

	for (li = app_list; li != NULL; li = li->next) {
		Application *application = li->data;
		/* Note that this list is sorted so to kill duplicates
		 * in app_list we only need to check the first entry */
		if (retval == NULL ||
		    strcmp ((const char *) retval->data, application->app_id) != 0)
			retval = g_list_prepend (retval, application->app_id);
	}

	for (li = app_list2; li != NULL; li = li->next) {
		Application *application = li->data;
		if (g_list_find_custom (retval, application->app_id,
					(GCompareFunc) strcmp) == NULL)
			retval = g_list_prepend (retval, application->app_id);
	}

	return retval;
}

/**
 * mate_vfs_application_registry_get_mime_types
 * @app_id: registry id of application
 *
 * This function returns a list of strings that represent the mime
 * types that can be handled by an application.
 *
 * Returns: a list of the mime types supported
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

GList *
mate_vfs_application_registry_get_mime_types (const char *app_id)
{
	Application *application;
	GList *retval;

	g_return_val_if_fail (app_id != NULL, NULL);

	maybe_reload ();

	application = application_lookup (app_id);
	if (application == NULL)
		return NULL;

	retval = g_list_copy (application->mime_types);

	/* merge in the mime types from the user_application,
	 * if it exists */
	if (application->user_application) {
		GList *li;
		for (li = application->user_application->mime_types;
		     li != NULL;
		     li = li->next) {
			char *mimetype = li->data;
			if (g_list_find_custom (retval, mimetype,
						(GCompareFunc) strcmp) == NULL)
				retval = g_list_prepend (retval,
							 mimetype);
		}
	}

	return retval;
}

/**
 * mate_vfs_application_registry_supports_uri_scheme
 * @app_id: registry id of application
 * @uri_scheme: uri schme string
 *
 * Given the id of the application this function will determine if the
 * uri scheme will given is supported.
 *
 * Returns: TRUE if @app_id supports @uri_scheme, otherwise FALSE
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

gboolean
mate_vfs_application_registry_supports_uri_scheme (const char *app_id,
						    const char *uri_scheme)
{
	Application *application;
	gboolean uses_matevfs;
	
	g_return_val_if_fail (app_id != NULL, FALSE);
	g_return_val_if_fail (uri_scheme != NULL, FALSE);

	maybe_reload ();

	application = application_lookup (app_id);
	if (application == NULL)
		return FALSE;

	uses_matevfs = real_get_bool_value (application, MATE_VFS_APPLICATION_REGISTRY_USES_MATEVFS, NULL);

	if (strcmp (uri_scheme, "file") == 0 &&
	    uses_matevfs == FALSE &&
	    application->supported_uri_schemes == NULL &&
	    application->user_application->supported_uri_schemes == NULL) {
		return TRUE;
	}

	/* check both the application and the user application
	 * mime_types lists */
	/* FIXME: This method does not allow a user to override and remove
	   uri schemes that an application can handle.  Is this an issue? */

	if ((g_list_find_custom (application->supported_uri_schemes,
				 /*glib is const incorrect*/(gpointer)uri_scheme,
				(GCompareFunc) strcmp) != NULL) ||
	    (application->user_application &&
	     g_list_find_custom (application->user_application->supported_uri_schemes,
				 /*glib is const incorrect*/(gpointer) uri_scheme,
				 (GCompareFunc) strcmp) != NULL)) {
		return TRUE;
	}
	/* check in the list of uris supported by mate-vfs if necessary */
	else if (uses_matevfs) {
		GList *supported_uris;
		gboolean res;
		
		supported_uris = _mate_vfs_configuration_get_methods_list();
		res = (g_list_find_custom(supported_uris,
					  /*glib is const incorrect*/(gpointer) uri_scheme,
					  (GCompareFunc) strcmp) != NULL);

		g_list_foreach(supported_uris, (GFunc) g_free, NULL);
		g_list_free(supported_uris);

		return res;
	}

	return FALSE;
}

/**
 * mate_vfs_application_registry_supports_mime_type
 * @app_id: registry id of application
 * @mime_type: mime type string
 *
 * Use this function to see if there is an application associated with a given
 * mime type.  The function will return true or false.
 *
 * Returns: TRUE if @app_id supports @mime_type, otherwise FALSE.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

gboolean
mate_vfs_application_registry_supports_mime_type (const char *app_id,
						   const char *mime_type)
{
	Application *application;

	g_return_val_if_fail (app_id != NULL, FALSE);
	g_return_val_if_fail (mime_type != NULL, FALSE);

	maybe_reload ();

	application = application_lookup (app_id);
	if (application == NULL)
		return FALSE;

	/* check both the application and the user application
	 * mime_types lists */
	/* FIXME: This method does not allow a user to override and remove
	   mime types that an application can handle.  Is this an issue? */
	if ((g_list_find_custom (application->mime_types,
				 /*glib is const incorrect*/(gpointer)mime_type,
				(GCompareFunc) strcmp) != NULL) ||
	    (application->user_application &&
	     g_list_find_custom (application->user_application->mime_types,
				 /*glib is const incorrect*/(gpointer)mime_type,
				 (GCompareFunc) strcmp) != NULL))
		return TRUE;
	else
		return FALSE;
}


/*
 * Mime type functions
 * Note that mime_type can be a specific (image/png) or generic (image/<star>) type
 */


/**
 * mate_vfs_application_registry_clear_mime_types
 * @app_id: Application id
 *
 * This function will remove the mime types associated with the application.
 * Changes are not realized until the  mate_vfs_application_registry_sync
 * function is called to save the changes to the file.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

void
mate_vfs_application_registry_clear_mime_types (const char *app_id)
{
	Application *application;

	g_return_if_fail (app_id != NULL);

	maybe_reload ();

	application = application_lookup_or_create (app_id, TRUE/*user_owned*/);

	application_clear_mime_types (application);

	user_file_dirty = TRUE;
}

/**
 * mate_vfs_application_registry_add_mime_type
 * @app_id: registry id of application
 * @mime_type: mime type string
 *
 * This function will associate a mime type with an application given the   
 * application registry id and the mime type.  Changes are not realized until
 * the mate_vfs_application_registry_sync function is called to save the
 * changes to the file.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

void
mate_vfs_application_registry_add_mime_type (const char *app_id,
					      const char *mime_type)
{
	Application *application;

	g_return_if_fail (app_id != NULL);
	g_return_if_fail (mime_type != NULL);

	maybe_reload ();

	application = application_lookup_or_create (app_id, TRUE/*user_owned*/);

	add_mime_type_to_application (application, mime_type);

	user_file_dirty = TRUE;
}

/**
 * mate_vfs_application_registry_remove_mime_type
 * @app_id: registry id of the application
 * @mime_type: mime type string
 *
 * This function will de-associate a mime type from an application registry.  
 * Given the application registry id and the mime type.  Changes are not
 * realized until the mate_vfs_application_registry_sync function is called to
 * save the changes to the file.
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

void
mate_vfs_application_registry_remove_mime_type (const char *app_id,
						 const char *mime_type)
{
	Application *application;

	g_return_if_fail (app_id != NULL);

	maybe_reload ();

	application = application_lookup_or_create (app_id, TRUE/*user_owned*/);

	remove_mime_type_for_application (application, mime_type);

	user_file_dirty = TRUE;
}

/*
 * Syncing to disk
 */

static void
application_sync_foreach (gpointer key, gpointer value, gpointer user_data)
{
	Application *application = value;
	FILE *fp = user_data;

	/* Only sync things that are user owned */
	if (application->user_owned)
		application_sync (application, fp);
	else if (application->user_application)
		application_sync (application->user_application, fp);
}

/**
 * mate_vfs_application_registry_sync
 *
 * This function will sync the registry.  Typically you would use this function
 * after a modification of the registry.  When you modify the registry a dirty
 * flag is set.  Calling this function will save your modifications to disk and
 * reset the flag.
 *
 * If successful, will return MATE_VFS_OK
 *
 * Returns: MateVFSResult
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

MateVFSResult
mate_vfs_application_registry_sync (void)
{
	FILE *fp;
	char *file;
	time_t curtime;

	if ( ! user_file_dirty)
		return MATE_VFS_OK;

	maybe_reload ();

	file = g_build_filename (user_registry_dir.dirname, "user.applications", NULL);
	fp = g_fopen (file, "w");

	if ( ! fp) {
		g_warning ("Cannot open '%s' for writing", file);
		g_free (file);
		return mate_vfs_result_from_errno ();
	}

	g_free (file);

	time(&curtime);

	fprintf (fp, "# This file is automatically generated by mate-vfs "
		 "application registry\n"
		 "# Do NOT edit by hand\n# Generated: %s\n",
		 ctime (&curtime));

	if (global_applications != NULL)
		g_hash_table_foreach (global_applications, application_sync_foreach, fp);

	fclose (fp);

	user_file_dirty = FALSE;

	return MATE_VFS_OK;
}


/**
 * mate_vfs_application_registry_get_mime_application
 * @app_id:  registry id of the application
 *
 * Returns a structure that contains the application that handles
 * the mime type associated by the application referred by app_id.
 *
 * Returns: MateVFSMimeApplication
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 *
 */

MateVFSMimeApplication *
mate_vfs_application_registry_get_mime_application (const char *app_id)
{
	Application *i_application;
	MateVFSMimeApplication *application;
	gboolean uses_matevfs = FALSE;

	g_return_val_if_fail (app_id != NULL, NULL);

	maybe_reload ();

	i_application = application_lookup (app_id);

	if (i_application == NULL)
		return NULL;

	application = g_new0 (MateVFSMimeApplication, 1);

	application->id = g_strdup (app_id);

	application->name =
		g_strdup (real_peek_value
			  (i_application,
			   MATE_VFS_APPLICATION_REGISTRY_NAME));
	application->command =
		g_strdup (real_peek_value
			  (i_application,
			   MATE_VFS_APPLICATION_REGISTRY_COMMAND));

	application->can_open_multiple_files =
		real_get_bool_value
			(i_application,
			 MATE_VFS_APPLICATION_REGISTRY_CAN_OPEN_MULTIPLE_FILES,
			 NULL);
	application->expects_uris = i_application->expects_uris;
	application->supported_uri_schemes = 
		supported_uri_scheme_list_copy (i_application->supported_uri_schemes);

	application->requires_terminal =
		real_get_bool_value
			(i_application,
			 MATE_VFS_APPLICATION_REGISTRY_REQUIRES_TERMINAL,
			 NULL);

	uses_matevfs = real_get_bool_value (i_application, MATE_VFS_APPLICATION_REGISTRY_USES_MATEVFS, NULL);

	if (uses_matevfs) {
		GList *methods_list = 
			_mate_vfs_configuration_get_methods_list();
		GList *l;
		if (application->expects_uris == MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_PATHS) {
			application->expects_uris = MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS;
		}
		for (l = methods_list; l != NULL; l = l->next) {
			if (g_list_find_custom (application->supported_uri_schemes,
						/*glib is const incorrect*/(gpointer) l->data,
						(GCompareFunc) strcmp) == NULL) {
				application->supported_uri_schemes = 
					g_list_prepend(application->supported_uri_schemes, 
						       l->data);
			} else {
				g_free(l->data);
			}			
		}
		g_list_free(methods_list);
	}	

	return application;
}

/**
 * mate_vfs_application_registry_save_mime_application
 * @application: application associated with the mime type
 * 
 * This will save to the registry the application that will be associated with
 * a defined mime type.  The defined mime type is located within the
 * MateVFSMimeApplication structure.  Changes are not realized until the
 * mate_vfs_application_registry_sync function is called.
 *
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 */

void
mate_vfs_application_registry_save_mime_application (const MateVFSMimeApplication *application)
{
	Application *i_application;

	g_return_if_fail (application != NULL);

	/* make us a new user application */
	i_application = application_lookup_or_create (application->id, TRUE);

	application_ref (i_application);

	set_value (i_application, MATE_VFS_APPLICATION_REGISTRY_NAME,
		   application->name);
	set_value (i_application, MATE_VFS_APPLICATION_REGISTRY_COMMAND,
		   application->command);
	set_bool_value (i_application, MATE_VFS_APPLICATION_REGISTRY_CAN_OPEN_MULTIPLE_FILES,
			application->can_open_multiple_files);
	i_application->expects_uris = application->expects_uris;
	set_bool_value (i_application, MATE_VFS_APPLICATION_REGISTRY_REQUIRES_TERMINAL,
			application->requires_terminal);
	/* FIXME: Need to save supported_uri_schemes information */
	user_file_dirty = TRUE;
}

/**
 * mate_vfs_application_is_user_owned_application
 * @application:  data structure of the mime application
 *
 * This function will determine if a mime application is user owned or not.  By
 * user ownered this means that the application is not a system application
 * located in the prerequisite /usr area but rather in the user's area.
 *
 * Returns: gboolean
 *
 * Deprecated: All application registry functions have been
 * deprecated. Use the functions available in mate-mime-handlers
 * instead.
 */

gboolean
mate_vfs_application_is_user_owned_application (const MateVFSMimeApplication *application)
{
	Application *i_application;

	g_return_val_if_fail (application != NULL, FALSE);

	/* make us a new user application */
	i_application = g_hash_table_lookup (global_applications, application->id);
	if (i_application != NULL) {
		return i_application->user_owned;
	}
	
	return FALSE;
}

