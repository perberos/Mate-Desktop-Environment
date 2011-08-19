/* 
   Copyright (C) 2007 Jonathan Matthew

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

   Author: Jonathan Matthew <jonathan@d14n.org>
 */

#include "config.h"

#ifndef TOTEM_PL_PARSER_MINI
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "totem-pl-parser.h"
#include "totemplparser-marshal.h"
#endif /* !TOTEM_PL_PARSER_MINI */

#include "totem-pl-parser-mini.h"
#include "totem-pl-parser-pla.h"
#include "totem-pl-parser-private.h"

/* things we know */
#define PATH_OFFSET		2
#define FORMAT_ID_OFFSET	4
#define RECORD_SIZE		512

/* things we guessed */
#define TITLE_OFFSET		32
#define TITLE_SIZE		64

#ifndef TOTEM_PL_PARSER_MINI
gboolean
totem_pl_parser_save_pla (TotemPlParser    *parser,
                          TotemPlPlaylist  *playlist,
                          GFile            *output,
                          const char       *title,
                          GError          **error)
{
        TotemPlPlaylistIter iter;
	GFileOutputStream *stream;
        gint num_entries_total, i;
	char *buffer;
	gboolean valid, ret;

	stream = g_file_replace (output, NULL, FALSE, G_FILE_CREATE_NONE, NULL, error);
	if (stream == NULL)
		return FALSE;

        num_entries_total = totem_pl_playlist_size (playlist);

	/* write the header */
	buffer = g_malloc0 (RECORD_SIZE);
	*((gint32 *)buffer) = GINT32_TO_BE (num_entries_total);
	strcpy (buffer + FORMAT_ID_OFFSET, "iriver UMS PLA");

	/* the player doesn't display this, but it stores
	 * the 'quick list' name there.
	 */
	strncpy (buffer + TITLE_OFFSET, title, TITLE_SIZE);
	if (totem_pl_parser_write_buffer (G_OUTPUT_STREAM (stream), buffer, RECORD_SIZE, error) == FALSE)
	{
		DEBUG(output, g_print ("Couldn't write header block for '%s'", uri));
		g_free (buffer);
		return FALSE;
	}

	ret = TRUE;
        valid = totem_pl_playlist_iter_first (playlist, &iter);
        i = 0;

        while (valid)
        {
		char *euri, *path, *converted, *filename;
		gsize written;

                totem_pl_playlist_get (playlist, &iter,
                                       TOTEM_PL_PARSER_FIELD_URI, &euri,
                                       NULL);

                valid = totem_pl_playlist_iter_next (playlist, &iter);

                if (!euri) {
                        continue;
                }

                memset (buffer, 0, RECORD_SIZE);
		path = g_filename_from_uri (euri, NULL, error);
                i++;

		if (path == NULL)
		{
			DEBUG(NULL, g_print ("Couldn't convert URI '%s' to a filename: %s\n", euri, (*error)->message));
			g_free (euri);
			ret = FALSE;
			break;
		}
		g_free (euri);

		/* the first two bytes of the record give the offset of the first character in the
		 * filename.  this is used to display just the filename when viewing the playlist
		 * on the device.
		 */
		filename = g_utf8_strrchr (path, -1, '/');
		if (filename == NULL)
		{
			/* should never happen, but this would be the right value */
			buffer[1] = 0x01;
		}
                else
                {
			/* we want the char after the slash, and it's one-based */
			guint name_offset = (filename - path) + 2;

			buffer[0] = (name_offset >> 8) & 0xff;
			buffer[1] = name_offset & 0xff;
		}

		/* replace slashes */
		g_strdelimit (path, "/", '\\');

		/* convert to big-endian utf16 and write it into the buffer */
		converted = g_convert (path, -1, "UTF-16BE", "UTF-8", NULL, &written, error);
		if (converted == NULL)
		{
			DEBUG(NULL, g_print ("Couldn't convert filename '%s' to UTF-16BE\n", path));
			g_free (path);
			ret = FALSE;
			break;
		}
		g_free (path);

		if (written > RECORD_SIZE - PATH_OFFSET)
			written = RECORD_SIZE - PATH_OFFSET;

		memcpy (buffer + PATH_OFFSET, converted, written);
		g_free (converted);

		if (totem_pl_parser_write_buffer (G_OUTPUT_STREAM (stream), buffer, RECORD_SIZE, error) == FALSE)
		{
			DEBUG(NULL, g_print ("Couldn't write entry %d to the file\n", i));
			ret = FALSE;
			break;
		}
	}

	g_free (buffer);
	g_object_unref (stream);

	return ret;
}

TotemPlParserResult
totem_pl_parser_add_pla (TotemPlParser *parser,
			 GFile *file,
			 GFile *base_file,
			 TotemPlParseData *parse_data,
			 gpointer data)
{
	TotemPlParserResult retval = TOTEM_PL_PARSER_RESULT_UNHANDLED;
	char *contents, *title;
	guint offset, max_entries, entry;
	gsize size;

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	if (size < RECORD_SIZE)
	{
		g_free (contents);
		DEBUG(file, g_print ("playlist '%s' is too short: %d\n", uri, (unsigned int) size));
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}

	/* read header block */
	max_entries = GINT32_FROM_BE (*((gint32 *)contents));
	if (strcmp (contents + FORMAT_ID_OFFSET, "iriver UMS PLA") != 0)
	{
		DEBUG(file, g_print ("playlist '%s' signature doesn't match: %s\n", uri, contents + 4));
		g_free (contents);
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}

	/* read playlist title starting at offset 32 */
	title = NULL;
	if (contents[TITLE_OFFSET] != '\0')
	{
		title = contents + TITLE_OFFSET;
		totem_pl_parser_add_uri (parser,
					 TOTEM_PL_PARSER_FIELD_IS_PLAYLIST, TRUE,
					 TOTEM_PL_PARSER_FIELD_FILE, file,
					 TOTEM_PL_PARSER_FIELD_TITLE, title,
					 NULL);
	}

	offset = RECORD_SIZE;
	entry = 0;
	while (offset + RECORD_SIZE <= size && entry < max_entries) {
		char *path, *uri;
		GError *error = NULL;

		/* path starts at +2, is at most 500 bytes, in big-endian utf16 .. */
		path = g_convert (contents + offset + PATH_OFFSET,
				  RECORD_SIZE - PATH_OFFSET,
				  "UTF-8", "UTF-16BE",
				  NULL, NULL, &error);
		if (path == NULL)
		{
			DEBUG(NULL, g_print ("error converting entry %d to UTF-8: %s\n", entry, error->message));
			g_error_free (error);
			retval = TOTEM_PL_PARSER_RESULT_ERROR;
			break;
		}

		/* .. with backslashes.. */
		g_strdelimit (path, "\\", '/');

		/* and that's all we get. */
		uri = g_filename_to_uri (path, NULL, NULL);
		if (uri == NULL)
		{
			DEBUG(file, g_print ("error converting path %s to URI: %s\n", path, error->message));
			g_error_free (error);
			retval = TOTEM_PL_PARSER_RESULT_ERROR;
			break;
		}

		totem_pl_parser_add_uri (parser, TOTEM_PL_PARSER_FIELD_URI, uri, NULL);

		g_free (uri);
		g_free (path);
		offset += RECORD_SIZE;
		entry++;
	}

	if (title != NULL)
		totem_pl_parser_playlist_end (parser, title);

	g_free (contents);

	return retval;
}

#endif /* !TOTEM_PL_PARSER_MINI */

