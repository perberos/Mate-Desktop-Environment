/* 
   2002, 2003, 2004, 2005, 2006 Bastien Nocera
   Copyright (C) 2003 Colin Walters <walters@verbum.org>

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

#ifndef TOTEM_PL_PARSER_H
#define TOTEM_PL_PARSER_H

#include <glib.h>
#include <gio/gio.h>

#include "totem-pl-parser-features.h"
#include "totem-pl-parser-builtins.h"
#include "totem-pl-playlist.h"

G_BEGIN_DECLS

#define TOTEM_TYPE_PL_PARSER            (totem_pl_parser_get_type ())
#define TOTEM_PL_PARSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_PL_PARSER, TotemPlParser))
#define TOTEM_PL_PARSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_PL_PARSER, TotemPlParserClass))
#define TOTEM_IS_PL_PARSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_PL_PARSER))
#define TOTEM_IS_PL_PARSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_PL_PARSER))

/**
 * TotemPlParserResult:
 * @TOTEM_PL_PARSER_RESULT_UNHANDLED: The playlist could not be handled.
 * @TOTEM_PL_PARSER_RESULT_ERROR: There was an error parsing the playlist.
 * @TOTEM_PL_PARSER_RESULT_SUCCESS: The playlist was parsed successfully.
 * @TOTEM_PL_PARSER_RESULT_IGNORED: The playlist was ignored due to its scheme or MIME type (see totem_pl_parser_add_ignored_scheme()
 * and totem_pl_parser_add_ignored_mimetype()).
 * @TOTEM_PL_PARSER_RESULT_CANCELLED: Parsing of the playlist was cancelled part-way through.
 *
 * Gives the result of parsing a playlist.
 **/
typedef enum {
	TOTEM_PL_PARSER_RESULT_UNHANDLED,
	TOTEM_PL_PARSER_RESULT_ERROR,
	TOTEM_PL_PARSER_RESULT_SUCCESS,
	TOTEM_PL_PARSER_RESULT_IGNORED,
	TOTEM_PL_PARSER_RESULT_CANCELLED
} TotemPlParserResult;

/**
 * TotemPlParser:
 *
 * All the fields in the #TotemPlParser structure are private and should never be accessed directly.
 **/
typedef struct _TotemPlParser	       TotemPlParser;
typedef struct TotemPlParserClass      TotemPlParserClass;
typedef struct TotemPlParserPrivate    TotemPlParserPrivate;

struct _TotemPlParser {
	GObject parent;
	TotemPlParserPrivate *priv;
};

/* Known metadata fields */

/**
 * TOTEM_PL_PARSER_FIELD_URI:
 *
 * Metadata field for an entry's URI.
 *
 * Since: 2.26
 **/
#define TOTEM_PL_PARSER_FIELD_URI		"url"
/**
 * TOTEM_PL_PARSER_FIELD_GENRE:
 *
 * Metadata field for an entry's genre.
 **/
#define TOTEM_PL_PARSER_FIELD_GENRE		"genre"
/**
 * TOTEM_PL_PARSER_FIELD_TITLE:
 *
 * Metadata field for an entry's displayable title.
 **/
#define TOTEM_PL_PARSER_FIELD_TITLE		"title"
/**
 * TOTEM_PL_PARSER_FIELD_AUTHOR:
 *
 * Metadata field for an entry's author/composer/director.
 **/
#define TOTEM_PL_PARSER_FIELD_AUTHOR		"author"
/**
 * TOTEM_PL_PARSER_FIELD_ALBUM:
 *
 * Metadata field for an entry's album.
 **/
#define TOTEM_PL_PARSER_FIELD_ALBUM		"album"
/**
 * TOTEM_PL_PARSER_FIELD_BASE:
 *
 * Metadata field for an entry's base path.
 **/
#define TOTEM_PL_PARSER_FIELD_BASE		"base"
/**
 * TOTEM_PL_PARSER_FIELD_SUBTITLE_URI:
 *
 * The URI of the entry's subtitle file.
 **/
#define TOTEM_PL_PARSER_FIELD_SUBTITLE_URI	"subtitle-uri"
/**
 * TOTEM_PL_PARSER_FIELD_VOLUME:
 *
 * Metadata field for an entry's playback volume.
 **/
#define TOTEM_PL_PARSER_FIELD_VOLUME		"volume"
/**
 * TOTEM_PL_PARSER_FIELD_AUTOPLAY:
 *
 * Metadata field for an entry's "autoplay" flag, which is %TRUE if the entry should play automatically.
 **/
#define TOTEM_PL_PARSER_FIELD_AUTOPLAY		"autoplay"
/**
 * TOTEM_PL_PARSER_FIELD_DURATION:
 *
 * Metadata field for an entry's playback duration, which should be parsed using totem_pl_parser_parse_duration().
 **/
#define TOTEM_PL_PARSER_FIELD_DURATION		"duration"
/**
 * TOTEM_PL_PARSER_FIELD_DURATION_MS:
 *
 * Metadata field for an entry's playback duration, in milliseconds. It's only used when an entry's
 * duration is available in that format, so one would get either the %TOTEM_PL_PARSER_FIELD_DURATION
 * or %TOTEM_PL_PARSER_FIELD_DURATION_MS as metadata.
 **/
#define TOTEM_PL_PARSER_FIELD_DURATION_MS	"duration-ms"
/**
 * TOTEM_PL_PARSER_FIELD_STARTTIME:
 *
 * Metadata field for an entry's playback start time, which should be parsed using totem_pl_parser_parse_duration().
 **/
#define TOTEM_PL_PARSER_FIELD_STARTTIME		"starttime"
/**
 * TOTEM_PL_PARSER_FIELD_ENDTIME:
 *
 * Metadata field for an entry's playback end time.
 **/
#define TOTEM_PL_PARSER_FIELD_ENDTIME		"endtime"
/**
 * TOTEM_PL_PARSER_FIELD_COPYRIGHT:
 *
 * Metadata field for an entry's copyright line.
 **/
#define TOTEM_PL_PARSER_FIELD_COPYRIGHT		"copyright"
/**
 * TOTEM_PL_PARSER_FIELD_ABSTRACT:
 *
 * Metadata field for an entry's abstract text.
 **/
#define TOTEM_PL_PARSER_FIELD_ABSTRACT		"abstract"
/**
 * TOTEM_PL_PARSER_FIELD_DESCRIPTION:
 *
 * Metadata field for an entry's description.
 **/
#define TOTEM_PL_PARSER_FIELD_DESCRIPTION	"description"
/**
 * TOTEM_PL_PARSER_FIELD_SUMMARY:
 *
 * Metadata field for an entry's summary. (In practice, identical to %TOTEM_PL_PARSER_FIELD_DESCRIPTION.)
 **/
#define TOTEM_PL_PARSER_FIELD_SUMMARY		TOTEM_PL_PARSER_FIELD_DESCRIPTION
/**
 * TOTEM_PL_PARSER_FIELD_MOREINFO:
 *
 * Metadata field for an entry's "more info" URI.
 **/
#define TOTEM_PL_PARSER_FIELD_MOREINFO		"moreinfo"
/**
 * TOTEM_PL_PARSER_FIELD_SCREENSIZE:
 *
 * Metadata field for an entry's preferred screen size.
 **/
#define TOTEM_PL_PARSER_FIELD_SCREENSIZE	"screensize"
/**
 * TOTEM_PL_PARSER_FIELD_UI_MODE:
 *
 * Metadata field for an entry's preferred UI mode.
 **/
#define TOTEM_PL_PARSER_FIELD_UI_MODE		"ui-mode"
/**
 * TOTEM_PL_PARSER_FIELD_PUB_DATE:
 *
 * Metadata field for an entry's publication date, which should be parsed using totem_pl_parser_parse_date().
 **/
#define TOTEM_PL_PARSER_FIELD_PUB_DATE		"publication-date"
/**
 * TOTEM_PL_PARSER_FIELD_FILESIZE:
 *
 * Metadata field for an entry's filesize in bytes. This is only advisory, and can sometimes not match the actual filesize of the stream.
 **/
#define TOTEM_PL_PARSER_FIELD_FILESIZE		"filesize"
/**
 * TOTEM_PL_PARSER_FIELD_LANGUAGE:
 *
 * Metadata field for an entry's audio language.
 **/
#define TOTEM_PL_PARSER_FIELD_LANGUAGE		"language"
/**
 * TOTEM_PL_PARSER_FIELD_CONTACT:
 *
 * Metadata field for an entry's contact details for the webmaster.
 **/
#define TOTEM_PL_PARSER_FIELD_CONTACT		"contact"
/**
 * TOTEM_PL_PARSER_FIELD_IMAGE_URI:
 *
 * Metadata field for an entry's thumbnail image URI.
 *
 * Since: 2.26
 **/
#define TOTEM_PL_PARSER_FIELD_IMAGE_URI		"image-url"
/**
 * TOTEM_PL_PARSER_FIELD_DOWNLOAD_URI:
 *
 * Metadata field for an entry's download URI. Only used if an alternate download
 * location is available for the entry.
 *
 * Since: 2.26
 **/
#define TOTEM_PL_PARSER_FIELD_DOWNLOAD_URI	"download-url"
/**
 * TOTEM_PL_PARSER_FIELD_ID:
 *
 * Metadata field for an entry's identifier. Its use is dependent on the format
 * of the playlist parsed, and its origin.
 **/
#define TOTEM_PL_PARSER_FIELD_ID		"id"
/**
 * TOTEM_PL_PARSER_FIELD_IS_PLAYLIST:
 *
 * Metadata field used to tell the calling code that the parsing of a playlist
 * started. It is only %TRUE for the metadata passed to #TotemPlParser::playlist-started or
 * #TotemPlParser::playlist-ended signal handlers.
 **/
#define TOTEM_PL_PARSER_FIELD_IS_PLAYLIST	"is-playlist"

/**
 * TotemPlParserClass:
 * @parent_class: the parent class
 * @entry_parsed: the generic signal handler for the #TotemPlParser::entry-parsed signal,
 * which can be overridden by inheriting classes
 * @playlist_started: the generic signal handler for the #TotemPlParser::playlist-started signal,
 * which can be overridden by inheriting classes
 * @playlist_ended: the generic signal handler for the #TotemPlParser::playlist-ended signal,
 * which can be overridden by inheriting classes
 *
 * The class structure for the #TotemPlParser type.
 **/
struct TotemPlParserClass {
	GObjectClass parent_class;

	/* signals */
	void (*entry_parsed) (TotemPlParser *parser,
			      const char *uri,
			      GHashTable *metadata);
	void (*playlist_started) (TotemPlParser *parser,
				  const char *uri,
				  GHashTable *metadata);
	void (*playlist_ended) (TotemPlParser *parser,
				const char *uri);
};

/**
 * TotemPlParserType:
 * @TOTEM_PL_PARSER_PLS: PLS parser
 * @TOTEM_PL_PARSER_M3U: M3U parser
 * @TOTEM_PL_PARSER_M3U_DOS: M3U (DOS linebreaks) parser
 * @TOTEM_PL_PARSER_XSPF: XSPF parser
 * @TOTEM_PL_PARSER_IRIVER_PLA: iRiver PLA parser
 *
 * The type of playlist a #TotemPlParser will parse.
 **/
typedef enum {
	TOTEM_PL_PARSER_PLS,
	TOTEM_PL_PARSER_M3U,
	TOTEM_PL_PARSER_M3U_DOS,
	TOTEM_PL_PARSER_XSPF,
	TOTEM_PL_PARSER_IRIVER_PLA,
} TotemPlParserType;

/**
 * TotemPlParserError:
 * @TOTEM_PL_PARSER_ERROR_NO_DISC: Error attempting to open a disc device when no disc is present
 * @TOTEM_PL_PARSER_ERROR_MOUNT_FAILED: An attempted mount operation failed
 * @TOTEM_PL_PARSER_ERROR_EMPTY_PLAYLIST: Playlist to be saved is empty
 *
 * Allows you to differentiate between different
 * errors occurring during file operations in a #TotemPlParser.
 **/
typedef enum {
	TOTEM_PL_PARSER_ERROR_NO_DISC,
	TOTEM_PL_PARSER_ERROR_MOUNT_FAILED,
	TOTEM_PL_PARSER_ERROR_EMPTY_PLAYLIST
} TotemPlParserError;

#define TOTEM_PL_PARSER_ERROR (totem_pl_parser_error_quark ())

GQuark totem_pl_parser_error_quark (void);

GType    totem_pl_parser_get_type (void);

gint64  totem_pl_parser_parse_duration (const char *duration, gboolean debug);
guint64 totem_pl_parser_parse_date     (const char *date_str, gboolean debug);

gboolean totem_pl_parser_save (TotemPlParser      *parser,
			       TotemPlPlaylist    *playlist,
			       GFile              *dest,
			       const gchar        *title,
			       TotemPlParserType   type,
			       GError            **error);

void	   totem_pl_parser_add_ignored_scheme (TotemPlParser *parser,
					       const char *scheme);
void       totem_pl_parser_add_ignored_mimetype (TotemPlParser *parser,
						 const char *mimetype);

TotemPlParserResult totem_pl_parser_parse (TotemPlParser *parser,
					   const char *uri, gboolean fallback);
void totem_pl_parser_parse_async (TotemPlParser *parser, const char *uri,
				  gboolean fallback, GCancellable *cancellable,
				  GAsyncReadyCallback callback,
                                  gpointer user_data);
TotemPlParserResult totem_pl_parser_parse_finish (TotemPlParser *parser,
						  GAsyncResult *async_result,
						  GError **error);

TotemPlParserResult totem_pl_parser_parse_with_base (TotemPlParser *parser,
						     const char *uri,
						     const char *base,
						     gboolean fallback);
void totem_pl_parser_parse_with_base_async (TotemPlParser *parser,
					    const char *uri, const char *base,
					    gboolean fallback,
					    GCancellable *cancellable,
					    GAsyncReadyCallback callback,
                    			    gpointer user_data);

TotemPlParser *totem_pl_parser_new (void);

GType totem_pl_parser_metadata_get_type (void) G_GNUC_CONST;
#define TOTEM_TYPE_PL_PARSER_METADATA (totem_pl_parser_metadata_get_type())

G_END_DECLS

#endif /* TOTEM_PL_PARSER_H */
