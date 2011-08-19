/* MateThemeParser - a parser of icon-theme files
 * mate-theme-parser.h Copyright (C) 2002 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATE_THEME_PARSER_H
#define MATE_THEME_PARSER_H

#ifndef MATE_DISABLE_DEPRECATED

#include <glib.h>

G_BEGIN_DECLS

typedef struct _MateThemeFile MateThemeFile;

typedef void (* MateThemeFileSectionFunc) (MateThemeFile *df,
					    const char       *name,
					    gpointer          data);

/* If @key is %NULL, @value is a comment line. */
/* @value is raw, unescaped data. */
typedef void (* MateThemeFileLineFunc) (MateThemeFile *df,
					 const char       *key,
					 const char       *locale,
					 const char       *value,
					 gpointer          data);

typedef enum 
{
  MATE_THEME_FILE_PARSE_ERROR_INVALID_SYNTAX,
  MATE_THEME_FILE_PARSE_ERROR_INVALID_ESCAPES,
  MATE_THEME_FILE_PARSE_ERROR_INVALID_CHARS
} MateThemeFileParseError;

#define MATE_THEME_FILE_PARSE_ERROR mate_theme_file_parse_error_quark()
GQuark mate_theme_file_parse_error_quark (void);

MateThemeFile *mate_theme_file_new_from_string (char                    *data,
						  GError                 **error);
char *          mate_theme_file_to_string       (MateThemeFile          *df);
void            mate_theme_file_free            (MateThemeFile          *df);


void                   mate_theme_file_foreach_section (MateThemeFile            *df,
							 MateThemeFileSectionFunc  func,
							 gpointer                   user_data);
void                   mate_theme_file_foreach_key     (MateThemeFile            *df,
							 const char                *section,
							 gboolean                   include_localized,
							 MateThemeFileLineFunc     func,
							 gpointer                   user_data);


/* Gets the raw text of the key, unescaped */
gboolean mate_theme_file_get_raw            (MateThemeFile   *df,
					      const char       *section,
					      const char       *keyname,
					      const char       *locale,
					      char            **val);
gboolean mate_theme_file_get_integer        (MateThemeFile   *df,
					      const char       *section,
					      const char       *keyname,
					      int              *val);
gboolean mate_theme_file_get_string         (MateThemeFile   *df,
					      const char       *section,
					      const char       *keyname,
					      char            **val);
gboolean mate_theme_file_get_locale_string  (MateThemeFile   *df,
					      const char       *section,
					      const char       *keyname,
					      char            **val);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* MATE_THEME_PARSER_H */
