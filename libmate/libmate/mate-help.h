/* mate-help.h
 * Copyright (C) 2001 Sid Vicious
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
 */

#ifndef MATE_HELP_H
#define MATE_HELP_H

#include <glib.h>
#include <libmate/mate-program.h>

G_BEGIN_DECLS

typedef enum {
  MATE_HELP_ERROR_INTERNAL,
  MATE_HELP_ERROR_NOT_FOUND
} MateHelpError;

#define MATE_HELP_ERROR (mate_help_error_quark ())
GQuark mate_help_error_quark (void) G_GNUC_CONST;

/* Errors that could be returned can be from mate-url
 * and g-spawn */

gboolean mate_help_display             (const char    *file_name,
					 const char    *link_id,
					 GError       **error);
gboolean mate_help_display_with_doc_id (MateProgram  *program,
					 const char    *doc_id,
					 const char    *file_name,
					 const char    *link_id,
					 GError       **error);
gboolean mate_help_display_desktop     (MateProgram  *program,
					 const char    *doc_id,
					 const char    *file_name,
					 const char    *link_id,
					 GError       **error);
gboolean mate_help_display_uri         (const char    *help_uri,
					 GError       **error);

gboolean mate_help_display_uri_with_env        (const char    *help_uri,
						 char         **envp,
						 GError       **error);
gboolean mate_help_display_with_doc_id_and_env (MateProgram  *program,
						 const char    *doc_id,
						 const char    *file_name,
						 const char    *link_id,
						 char         **envp,
						 GError       **error);
gboolean mate_help_display_desktop_with_env    (MateProgram  *program,
						 const char    *doc_id,
						 const char    *file_name,
						 const char    *link_id,
						 char         **envp,
						 GError       **error);

G_END_DECLS

#endif /* MATE_HELP_H */
