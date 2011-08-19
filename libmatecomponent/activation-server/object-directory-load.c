/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  oafd: OAF CORBA dameon.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *           Maciej Stachowiak <mjs@eazel.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include <glib/gi18n.h>
#include <glib.h>

#include "matecomponent-activation/matecomponent-activation-private.h"
#include "server.h"

/* SAX Parser */
typedef enum {
        STATE_START,
        STATE_OAF_INFO,
        STATE_OAF_SERVER,
        STATE_OAF_ATTRIBUTE,
        STATE_ITEM,
        STATE_UNKNOWN,
        STATE_ERROR
} ParseState;

typedef struct {
        char *filename;         /* Filename of .server file, or NULL
                                 * if parsing from memory.
                                 */
        ParseState state;
        ParseState prev_state;
        int unknown_depth;
        
        const char *host;
        GSList **entries;
        
        MateComponent_ServerInfo *cur_server;
        MateComponent_ActivationProperty *cur_prop;
        GList *cur_props;
        GList *cur_items;
        
} ParseInfo;

#define IS_ELEMENT(x) (!strcmp (name, x))

static ParseInfo *
parse_info_new (const char *filename,
                const char *host,
                GSList    **entries)
{
        ParseInfo *info = g_new0 (ParseInfo, 1);

        info->prev_state = STATE_UNKNOWN;
        info->state = STATE_START;

        info->host = host;
        info->entries = entries;
        info->filename = g_strdup (filename);
        
        return info;
}

static void
parse_info_free (ParseInfo *info)
{
        g_free (info->filename);
        g_free (info);
}

static char *
od_validate (const char *iid, const char *type, const char *location)
{
        int i;

        if (iid == NULL) {
                return g_strdup (_("a NULL iid is not valid"));
        }

        if (type == NULL) {
                return g_strdup_printf (_("iid %s has a NULL type"), iid);
        }

        if (location == NULL) {
                return g_strdup_printf (_("iid %s has a NULL location"), iid);
        }

        for (i = 0; iid && iid [i]; i++) {
                char c = iid [i];

                if (c == ',' || c == '[' || c == ']' ||
                    /* Reserved for future expansion */
                    c == '!' || c == '#' || c == '|') {
                        return g_strdup_printf (_("invalid character '%c' in iid '%s'"),
                                                c, iid);
                }
        }

        return NULL;
}

static void
parse_oaf_server_attrs (ParseInfo      *info,
                        const gchar        **attr_names,
                        const gchar        **attr_values)
{
        const char *iid = NULL;
        const char *type = NULL;
        const char *location = NULL;
        const char *att, *val;
        char *error;
        int i = 0;

        info->state = STATE_OAF_SERVER;
        
        if (!attr_names || !attr_values)
                return;

        do {
                att = attr_names[i];
                val = attr_values[i++];

                if (att && val) {
                        if (!iid && !strcmp (att, "iid"))
                                iid = val;
                        else if (!type && !strcmp (att, "type"))
                                type = val;
                        else if (!location && !strcmp (att, "location"))
                                location = val;
                }
                
        } while (att && val);

        error = od_validate (iid, type, location);
        
        if (error != NULL) {
                g_warning ("%s", error);
                
                g_free (error);

                return;
        }

#ifdef G_OS_WIN32
        /* If this data has been read from a .server file, and the
         * path to the exe or dll starts with ../, make it relative to
         * the location of the .server file. Very convenient, means
         * yuo can install some software package that includes a
         * MateComponent component in a freeestanding location, and just need
         * to update your MATECOMPONENT_ACTIVATION_PATH so the .server file
         * is found.
         *
         * In other cases, possibly replace configure-time shlib or
         * exe location with the actual installed one. This is for
         * components that have been built with the same
         * configure-time prefix as libmatecomponent.
         */
        if (!strcmp (type, "exe") || !strcmp (type, "shlib")) {
                if (info->filename != NULL &&
                    !strncmp (location, "..", 2) &&
                    G_IS_DIR_SEPARATOR (location[2])) {
                            gchar *dirname = g_path_get_dirname (info->filename);

                            location = g_build_filename (dirname, location, NULL);
                            g_free (dirname);
                    } else {
                              location = _matecomponent_activation_win32_replace_prefix (_matecomponent_activation_win32_get_prefix (), location);
                    }
        }
#endif

        /* Now create the ServerInfo object */
        info->cur_server = g_new0 (MateComponent_ServerInfo, 1);

        info->cur_server->iid = CORBA_string_dup (iid);
        info->cur_server->server_type = CORBA_string_dup (type);
        info->cur_server->location_info = CORBA_string_dup (location);
        info->cur_server->hostname = CORBA_string_dup (info->host);
        info->cur_server->username = CORBA_string_dup (g_get_user_name ());
        info->cur_server->domain = CORBA_string_dup ("unused");

#ifdef G_OS_WIN32
        if (!strcmp (type, "exe") || !strcmp (type, "shlib"))
                g_free ((char *) location);
#endif
}

static GHashTable *interesting_locales = NULL;

void
add_initial_locales (void)
{
        const char * const * langs;
        int i;

        if (!interesting_locales)
                interesting_locales = g_hash_table_new (
                        g_str_hash, g_str_equal);

        langs = g_get_language_names ();
	for (i = 0; langs[i] != NULL; i++)
		g_hash_table_insert (interesting_locales,
                                     g_strdup (langs[i]),
                                     GUINT_TO_POINTER (1));
}

gboolean
register_interest_in_locales (const char *locales)
{
        int i;
        char **localev;
        gboolean new_locale = FALSE;

        localev = g_strsplit (locales, ",", 0);

        for (i = 0; localev[i]; i++) {
                if (!g_hash_table_lookup (interesting_locales, localev[i])) {
#ifdef LOCALE_DEBUG
                        g_warning ("New locale '%s' (%d)!",
                                   localev[i], g_list_length (locale_list));
#endif
                        g_hash_table_insert (interesting_locales,
                                             g_strdup (localev[i]),
                                             GUINT_TO_POINTER (1));
                        new_locale = TRUE;
                }
        }
        g_strfreev (localev);

        return new_locale;
}

static gboolean
is_locale_interesting (const char *name_with_locale)
{
        const char *locale;

        if (!name_with_locale)
                return FALSE;

        if (!(locale = strchr (name_with_locale, '-')))
                return TRUE;
        locale++;

        return g_hash_table_lookup (interesting_locales, locale) != NULL;
}

static gboolean 
od_string_to_boolean (const char *str)
{
	if (!g_ascii_strcasecmp (str, "true") ||
            !g_ascii_strcasecmp (str, "yes") ||
	    !strcmp (str, "1"))
		return TRUE;
	else
		return FALSE;
}

static void
parse_oaf_attribute (ParseInfo     *info,
                     const gchar        **attr_names,
                     const gchar        **attr_values)
{
        int i = 0;
        const char *type = NULL;
        const char *name = NULL;
        const char *value = NULL;
        const char *att, *val;
        
        g_assert (info->cur_server);

        info->state = STATE_OAF_ATTRIBUTE;
        
        if (!attr_names || !attr_values)
                return;

        do {
                att = attr_names[i];
                val = attr_values[i++];
                
                if (att && val) {
                        if (!strcmp (att, "type"))
                                type = val;

                        else if (!strcmp (att, "name")) {
                                name = val;
                                if (!is_locale_interesting (name))
                                        return;
                                
                        } else if (!strcmp (att, "value"))
                                value = val;
                }

        } while (att && val);

        if (!type || !name)
                return;
        
        if (name[0] == '_')
                g_critical ("%s is an invalid property name "
                            "- property names beginning with '_' are reserved",
                            name);
        
        info->cur_prop = MateCORBA_small_alloc (TC_MateComponent_ActivationProperty);
        info->cur_prop->name = CORBA_string_dup (name);

        if (g_ascii_strcasecmp (type, "stringv") == 0) {
                info->cur_prop->v._d = MateComponent_ACTIVATION_P_STRINGV;

        } else if (g_ascii_strcasecmp (type, "number") == 0) {
                info->cur_prop->v._d = MateComponent_ACTIVATION_P_NUMBER;
                info->cur_prop->v._u.value_number = atof (value);

        } else if (g_ascii_strcasecmp (type, "boolean") == 0) {
                info->cur_prop->v._d = MateComponent_ACTIVATION_P_BOOLEAN;
                info->cur_prop->v._u.value_boolean = od_string_to_boolean (value);

        } else {
                /* Assume string */
                info->cur_prop->v._d = MateComponent_ACTIVATION_P_STRING;
                if (value != NULL) {
                        info->cur_prop->v._u.value_string = CORBA_string_dup (value);
                } else {
                        g_warning (_("Property '%s' has no value"),
                                   info->cur_prop->name);
                        info->cur_prop->v._u.value_string =
                                CORBA_string_dup ("");
                }
        }
}

static void
parse_stringv_item (ParseInfo     *info,
                    const gchar        **attr_names,
                    const gchar        **attr_values)
{
        const char *value = NULL;
        const char *att, *val;
        int i = 0;

        if (!attr_names || !attr_values)
                return;

        do {
                att = attr_names[i];
                val = attr_values[i++];
                
                if (att && val) {
                        if (!value && !strcmp (att, "value")) {
                                value = val;
                                break;
                        }

                }
                
        } while (att && val);

        if (value) 
                info->cur_items = g_list_prepend (info->cur_items, CORBA_string_dup (value));

        info->state = STATE_ITEM;
        
}

static void
od_start_element (GMarkupParseContext *context,
                  const gchar         *name,
                  const gchar        **attribute_names,
                  const gchar        **attribute_values,
                  gpointer             user_data,
                  GError             **error)
{
        ParseInfo *info = user_data;

        switch (info->state) {
        case STATE_START:
                if (IS_ELEMENT ("oaf_info")) 
                        info->state = STATE_OAF_INFO;
                else {
                        info->prev_state = info->state;
                        info->state = STATE_UNKNOWN;
                        info->unknown_depth++;
                }
                break;
        case STATE_OAF_INFO:
                if (IS_ELEMENT ("oaf_server")) 
                        parse_oaf_server_attrs (info, attribute_names, attribute_values);
                else {
                        info->prev_state = info->state;
                        info->state = STATE_UNKNOWN;
                        info->unknown_depth++;
                }
                break;
        case STATE_OAF_SERVER:
                if (IS_ELEMENT ("oaf_attribute")) 
                        parse_oaf_attribute (info, attribute_names, attribute_values);
                else {
                        info->prev_state = info->state;
                        info->state = STATE_UNKNOWN;
                        info->unknown_depth++;
                }
                break;
        case STATE_OAF_ATTRIBUTE:
                if (IS_ELEMENT ("item"))
                        parse_stringv_item (info, attribute_names, attribute_values);
                else {
                        info->prev_state = info->state;
                        info->state = STATE_UNKNOWN;
                        info->unknown_depth++;
                }
                break;
        case STATE_UNKNOWN:
		info->unknown_depth++;
		break;
        case STATE_ERROR:
                break;
                break;
        default:
                g_error ("start element, unknown state: %d", info->state);
        }
}

static void
add_entry (ParseInfo *info)
{
        GSList *l;

        for (l = *(info->entries); l; l = l->next) {
                MateComponent_ServerInfo *si = l->data;

                if (!strcmp (si->iid, info->cur_server->iid))
                        return;
        }

        *(info->entries) = g_slist_prepend (*(info->entries), info->cur_server);
}

static void
od_end_element (GMarkupParseContext *context,
                const gchar         *name,
                gpointer             user_data,
                GError             **error)
{
        ParseInfo *info = user_data;

        switch (info->state) {
        case STATE_ITEM:
                info->state = STATE_OAF_ATTRIBUTE;
                break;
        case STATE_OAF_ATTRIBUTE: {
                if (info->cur_prop && info->cur_prop->v._d == MateComponent_ACTIVATION_P_STRINGV) {
                        gint i, len;
                        GList *p;
                        
                        len = g_list_length (info->cur_items);

                        info->cur_prop->v._u.value_stringv._length = len;
                        info->cur_prop->v._u.value_stringv._buffer =
                                CORBA_sequence_CORBA_string_allocbuf (len);
                        
			info->cur_items = g_list_reverse (info->cur_items);
                        for (i = 0, p = info->cur_items; p; p = p->next, i++)
                                info->cur_prop->v._u.
                                        value_stringv._buffer[i] = p->data;
                        g_list_free (info->cur_items);
                        info->cur_items = NULL;
                }

                if (info->cur_prop) {
                        info->cur_props = g_list_prepend (info->cur_props, info->cur_prop);
                        info->cur_prop = NULL;
                }
                
                info->state = STATE_OAF_SERVER;
                break;
        }
        case STATE_OAF_SERVER: {
                if (info->cur_server) {
                        GList *p;
                        gint len, i;

                        len = g_list_length (info->cur_props);

                        info->cur_server->props._length = len;
                        info->cur_server->props._buffer =
                                CORBA_sequence_MateComponent_ActivationProperty_allocbuf (len);

			info->cur_props = g_list_reverse (info->cur_props);
                        for (i = 0, p = info->cur_props; p; p = p->next, i++) {
                                MateComponent_ActivationProperty_copy (&info->cur_server->props._buffer[i],
                                                                (MateComponent_ActivationProperty *) p->data);
                                CORBA_free (p->data);
                        }
                        g_list_free (info->cur_props);
                        info->cur_props = NULL;

                        add_entry (info);
                        info->cur_server = NULL;
                }
                info->state = STATE_OAF_INFO;
                break;
        }
        case STATE_OAF_INFO: {
                info->state = STATE_START;
                break;
        }
        case STATE_UNKNOWN:
		info->unknown_depth--;
		if (info->unknown_depth == 0)
			info->state = info->prev_state;
		break;
        case STATE_START:
                break;
        default:
                g_error ("end element, unknown state: %d", info->state);
        }
}

static void
od_error (GMarkupParseContext *context,
          GError              *error,
          gpointer             user_data)
{
        ParseInfo *info = user_data;

        g_error ("Failed to parse: '%s' in file '%s'",
                 error ? error->message : "<nomsg>",
                 info->filename ? info->filename : "<memory>");
}

static GMarkupParser od_gmarkup_parser = {
        od_start_element,
        od_end_element,
        NULL,
        NULL,
        od_error
};

static void
od_load_file (const char *file,
              GSList    **entries,
              const char *host)
{
        gsize length;
        gchar *contents = NULL;
        ParseInfo *info;
        GMarkupParseContext *ctxt = NULL;

        if (!g_file_get_contents (file, &contents, &length, NULL))
                goto err;

        info = parse_info_new (file, host, entries);
        ctxt = g_markup_parse_context_new (&od_gmarkup_parser, 0, info,
                                           (GDestroyNotify) parse_info_free);

        if (!g_markup_parse_context_parse (ctxt, contents, length, NULL)) {
  err:
                g_warning (_("Could not parse badly formed XML document %s"), file);
        }
        if (ctxt)
                g_markup_parse_context_free (ctxt);

	g_free (contents);
}

void
matecomponent_parse_server_info_memory (const char *server_info,
                                 GSList    **entries,
                                 const char *host)
{
        ParseInfo *info;
        GMarkupParseContext *ctxt;

        info = parse_info_new (NULL, host, entries);

        ctxt = g_markup_parse_context_new (&od_gmarkup_parser, 0, info,
                                           (GDestroyNotify) parse_info_free);

        if (!g_markup_parse_context_parse (ctxt, server_info, strlen (server_info), NULL))
                g_warning ("Failed to parse serverinfo from memory");
        g_markup_parse_context_free (ctxt);
}

static gboolean
od_filename_has_extension (const char *filename,
                           const char *extension)
{
        char *last_dot;
        
        last_dot = strrchr (filename, '.');

        return last_dot != NULL && strcmp (last_dot, extension) == 0;
}

static void
od_load_directory (const char *directory,
                   GSList    **entries,
                   const char *host)
{
	GDir *directory_handle;
	const char *directory_entry;
        char *pathname;

        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, _("Trying dir %s"), directory);

        directory_handle = g_dir_open (directory, 0, NULL);

        if (directory_handle == NULL) {
                /* FIXME */
                return;
        }
        
        for (directory_entry = g_dir_read_name (directory_handle);
             directory_entry != NULL;
             directory_entry = g_dir_read_name (directory_handle)) {
                pathname = g_build_filename (directory, directory_entry, NULL);

                if (od_filename_has_extension (pathname, ".server")) {
                        od_load_file (pathname, entries, host);
                }

		g_free (pathname);
        }

        g_dir_close (directory_handle);
}


void
matecomponent_server_info_load (char                  **directories,
                         MateComponent_ServerInfoList  *servers,
                         GPtrArray const        *runtime_servers,
                         GHashTable            **iid_to_server_info_map,
                         const char             *host)
{
	GSList *entries;
        int length;
        GSList *p;
	int i, j; 
        
	g_return_if_fail (directories);
	g_return_if_fail (iid_to_server_info_map);

        entries = NULL;

	if (*iid_to_server_info_map != NULL) {
		g_hash_table_destroy (*iid_to_server_info_map);
        }

	*iid_to_server_info_map = g_hash_table_new (g_str_hash, g_str_equal);

        /* Load each directory */
	for (i = 0; directories[i] != NULL; i++)
                od_load_directory (directories[i], &entries, host);

	/* Now convert 'entries' into something that the server can store and pass back */
	length = g_slist_length (entries);

	servers->_buffer = CORBA_sequence_MateComponent_ServerInfo_allocbuf
                (length + runtime_servers->len);
	servers->_length = length + runtime_servers->len;
        servers->_maximum = servers->_length;

	for (j = 0, p = entries; j < length; j++, p = p->next) {
		memcpy (&servers->_buffer[j], p->data, sizeof (MateComponent_ServerInfo));
		g_hash_table_insert (*iid_to_server_info_map,
                                     servers->_buffer[j].iid,
                                     &servers->_buffer[j]);
	}
          /* append information of runtime-defined servers  */
	for (j = 0; j < runtime_servers->len; j++) {
                servers->_buffer[length + j] = *(MateComponent_ServerInfo *)
                        g_ptr_array_index (runtime_servers, j);
		g_hash_table_insert (*iid_to_server_info_map,
                                     servers->_buffer[length + j].iid,
                                     &servers->_buffer[length + j]);
	}

        g_slist_foreach (entries, (GFunc) g_free, NULL);
        g_slist_free (entries);
}
