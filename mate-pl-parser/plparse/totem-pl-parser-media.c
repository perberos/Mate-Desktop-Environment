/* 
   Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007 Bastien Nocera
   Copyright (C) 2003, 2004 Colin Walters <walters@rhythmbox.org>

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
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"

#ifndef TOTEM_PL_PARSER_MINI
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "totem-pl-parser.h"
#include "totemplparser-marshal.h"
#include "totem-disc.h"
#endif /* !TOTEM_PL_PARSER_MINI */

#include "totem-pl-parser-mini.h"
#include "totem-pl-parser-media.h"
#include "totem-pl-parser-private.h"

/* Files that start with these characters sort after files that don't. */
#define SORT_LAST_CHAR1 '.'
#define SORT_LAST_CHAR2 '#'

#ifndef TOTEM_PL_PARSER_MINI
/* Returns NULL if we don't have an ISO image,
 * or an empty string if it's non-UTF-8 data */
static char *
totem_pl_parser_iso_get_title (GFile *_file)
{
	char *fname;
	FILE  *file;
#define BUFFER_SIZE 128
	char buf [BUFFER_SIZE+1];
	int res;
	char *str;

	fname = g_file_get_path (_file);
	if (fname == NULL)
		return NULL;

	file = fopen (fname, "rb");
	g_free (fname);
	if (file == NULL)
		return NULL;

	/* Verify we have an ISO image */
	/* This check is for the raw sector images */
	res = fseek (file, 37633L, SEEK_SET);
	if (res != 0) {
		fclose (file);
		return NULL;
	}

	res = fread (buf, sizeof (char), 5, file);
	if (res != 5 || strncmp (buf, "CD001", 5) != 0) {
		/* Standard ISO images */
		res = fseek (file, 32769L, SEEK_SET);
		if (res != 0) {
			fclose (file);
			return NULL;
		}
		res = fread (buf, sizeof (char), 5, file);
		if (res != 5 || strncmp (buf, "CD001", 5) != 0) {
			/* High Sierra images */
			res = fseek (file, 32776L, SEEK_SET);
			if (res != 0) {
				fclose (file);
				return NULL;
			}
			res = fread (buf, sizeof (char), 5, file);
			if (res != 5 || strncmp (buf, "CDROM", 5) != 0) {
				fclose (file);
				return NULL;
			}
		}
	}
	/* Extract the volume label from the image */
	res = fseek (file, 32808L, SEEK_SET);
	if (res != 0) {
		fclose (file);
		return NULL;
	}
	res = fread (buf, sizeof(char), BUFFER_SIZE, file);
	fclose (file);
	if (res != BUFFER_SIZE)
		return NULL;

	buf [BUFFER_SIZE] = '\0';
	str = g_strdup (g_strstrip (buf));
	if (!g_utf8_validate (str, -1, NULL)) {
		g_free (str);
		return g_strdup ("");
	}

	return str;
}

TotemPlParserResult
totem_pl_parser_add_iso (TotemPlParser *parser,
			 GFile *file,
			 GFile *base_file,
			 TotemPlParseData *parse_data,
			 gpointer data)
{
	TotemDiscMediaType type;
	char *uri, *retval;

	uri = g_file_get_uri (file);
	type = totem_cd_detect_type_with_url (uri, &retval, NULL);
	g_free (uri);
	if (type == MEDIA_TYPE_DVD || type == MEDIA_TYPE_VCD) {
		char *label;

		label = totem_pl_parser_iso_get_title (file);
		totem_pl_parser_add_one_uri (parser, retval, label);
		g_free (label);
		g_free (retval);
		return TOTEM_PL_PARSER_RESULT_SUCCESS;
	}

	return TOTEM_PL_PARSER_RESULT_IGNORED;
}

TotemPlParserResult
totem_pl_parser_add_cue (TotemPlParser *parser,
			 GFile *file,
			 GFile *base_file,
			 TotemPlParseData *parse_data,
			 gpointer data)
{
	char *vcduri, *path;

	path = g_file_get_path (file);
	if (path == NULL)
		return TOTEM_PL_PARSER_RESULT_IGNORED;

	vcduri = totem_cd_mrl_from_type ("vcd", path);
	g_free (path);
	totem_pl_parser_add_one_uri (parser, vcduri, NULL);
	g_free (vcduri);

	return TOTEM_PL_PARSER_RESULT_SUCCESS;
}

static int
totem_pl_parser_dir_compare (GFileInfo *a, GFileInfo *b)
{
	const char *name_1, *name_2;
	char *key_1, *key_2;
	gboolean sort_last_1, sort_last_2;
	int compare;

	name_1 = g_file_info_get_name (a);
	name_2 = g_file_info_get_name (b);

	if (name_1 == NULL) {
		if (name_2 == NULL)
			compare = 0;
		else
			compare = -1;
	} else {	
		sort_last_1 = name_1[0] == SORT_LAST_CHAR1 || name_1[0] == SORT_LAST_CHAR2;
		sort_last_2 = name_2[0] == SORT_LAST_CHAR1 || name_2[0] == SORT_LAST_CHAR2;
		
		if (sort_last_1 && !sort_last_2) {
			compare = +1;
		} else if (!sort_last_1 && sort_last_2) {
			compare = -1;
		} else {
			key_1 = g_utf8_collate_key_for_filename (name_1, -1);
			key_2 = g_utf8_collate_key_for_filename (name_2, -1);
			compare = strcmp (key_1, key_2);
			g_free (key_1);
			g_free (key_2);
		}
	}

	return compare;
}

static gboolean
totem_pl_parser_load_directory (GFile *file, GList **list, gboolean *unhandled)
{
	GFileEnumerator *e;
	GFileInfo *info;
	GError *err = NULL;

	*list = NULL;
	*unhandled = FALSE;

	e = g_file_enumerate_children (file,
				       G_FILE_ATTRIBUTE_STANDARD_NAME,
				       G_FILE_QUERY_INFO_NONE,
				       NULL, &err);
	if (e == NULL) {
		if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED) != FALSE)
			*unhandled = TRUE;
		g_file_enumerator_close (e, NULL, NULL);
		g_object_unref (e);
		g_error_free (err);
		return FALSE;
	}

	while ((info = g_file_enumerator_next_file (e, NULL, NULL)) != NULL)
		*list = g_list_prepend (*list, info);

	g_file_enumerator_close (e, NULL, NULL);
	g_object_unref (e);
	return TRUE;
}

TotemPlParserResult
totem_pl_parser_add_directory (TotemPlParser *parser,
			       GFile *file,
			       GFile *base_file,
			       TotemPlParseData *parse_data,
			       gpointer data)
{
	TotemDiscMediaType type;
	GList *list, *l;
	char *media_uri, *uri;
	gboolean unhandled;

	uri = g_file_get_uri (file);
	type = totem_cd_detect_type_from_dir (uri, &media_uri, NULL);
	g_free (uri);

	if (type != MEDIA_TYPE_DATA && type != MEDIA_TYPE_ERROR && media_uri != NULL) {
		char *basename = NULL, *fname;

		fname = g_file_get_path (file);
		if (fname != NULL) {
			basename = g_filename_display_basename (fname);
			g_free (fname);
		}
		totem_pl_parser_add_one_uri (parser, media_uri, basename);
		g_free (basename);
		g_free (media_uri);
		return TOTEM_PL_PARSER_RESULT_SUCCESS;
	}

	if (totem_pl_parser_load_directory (file, &list, &unhandled) == FALSE) {
		if (unhandled != FALSE)
			return TOTEM_PL_PARSER_RESULT_UNHANDLED;
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}

	list = g_list_sort (list, (GCompareFunc) totem_pl_parser_dir_compare);
	l = list;

	while (l != NULL) {
		GFileInfo *info = l->data;
		GFile *item;
		TotemPlParserResult ret;

		item = g_file_get_child (file, g_file_info_get_name (info));

		ret = totem_pl_parser_parse_internal (parser, item, NULL, parse_data);
		if (ret != TOTEM_PL_PARSER_RESULT_SUCCESS && ret != TOTEM_PL_PARSER_RESULT_IGNORED) {
			char *item_uri;

			item_uri = g_file_get_uri (item);
			totem_pl_parser_add_one_uri (parser, item_uri, NULL);
			g_free (item_uri);
		}

		g_object_unref (item);
		g_object_unref (info);

		l = l->next;
	}

	g_list_free (list);

	return TOTEM_PL_PARSER_RESULT_SUCCESS;
}

TotemPlParserResult
totem_pl_parser_add_block (TotemPlParser *parser,
			   GFile *file,
			   GFile *base_file,
			   TotemPlParseData *parse_data,
			   gpointer data)
{
	TotemDiscMediaType type;
	char *media_uri, *path;
	GError *err = NULL;

	path = g_file_get_path (file);
	if (path == NULL)
		return TOTEM_PL_PARSER_RESULT_UNHANDLED;

	type = totem_cd_detect_type_with_url (path, &media_uri, &err);
	g_free (path);
	if (err != NULL) {
		DEBUG(file, g_print ("Couldn't get CD type for URI '%s': %s\n", uri, err->message));
		g_error_free (err);
	}
	if (type == MEDIA_TYPE_DATA || media_uri == NULL)
		return TOTEM_PL_PARSER_RESULT_UNHANDLED;
	else if (type == MEDIA_TYPE_ERROR)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	totem_pl_parser_add_one_uri (parser, media_uri, NULL);
	g_free (media_uri);
	return TOTEM_PL_PARSER_RESULT_SUCCESS;
}

#endif /* !TOTEM_PL_PARSER_MINI */


