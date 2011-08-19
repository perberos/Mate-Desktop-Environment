/* gdict-utils.h - Utility functions for Gdict
 *
 * Copyright (C) 2005  Emmanuele Bassi <ebassi@gmail.com>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#ifndef __GDICT_UTILS_H__
#define __GDICT_UTILS_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDICT_DEFAULT_DATABASE	"*"
#define GDICT_DEFAULT_STRATEGY	"."

#define GDICT_DEFAULT_FONT_NAME "Sans 10"

/* properties shared between GdictContext implementations */
typedef enum {
  GDICT_CONTEXT_PROP_FIRST      = 0x1000,
  GDICT_CONTEXT_PROP_LOCAL_ONLY = GDICT_CONTEXT_PROP_FIRST,
  GDICT_CONTEXT_PROP_LAST
} GdictContextProp;

/* Status codes as defined by RFC 2229 */
typedef enum {
  GDICT_STATUS_INVALID                   = 0,
    
  GDICT_STATUS_N_DATABASES_PRESENT       = 110,
  GDICT_STATUS_N_STRATEGIES_PRESENT      = 111,
  GDICT_STATUS_DATABASE_INFO             = 112,
  GDICT_STATUS_HELP_TEXT                 = 113,
  GDICT_STATUS_SERVER_INFO               = 114,
  GDICT_STATUS_CHALLENGE                 = 130,
  GDICT_STATUS_N_DEFINITIONS_RETRIEVED   = 150,
  GDICT_STATUS_WORD_DB_NAME              = 151,
  GDICT_STATUS_N_MATCHES_FOUND           = 152,
  GDICT_STATUS_CONNECT                   = 220,
  GDICT_STATUS_QUIT                      = 221,
  GDICT_STATUS_AUTH_OK                   = 230,
  GDICT_STATUS_OK                        = 250,
  GDICT_STATUS_SEND_RESPONSE             = 330,
  /* Connect response codes */
  GDICT_STATUS_SERVER_DOWN               = 420,
  GDICT_STATUS_SHUTDOWN                  = 421,
  /* Error codes */
  GDICT_STATUS_BAD_COMMAND               = 500,
  GDICT_STATUS_BAD_PARAMETERS            = 501,
  GDICT_STATUS_COMMAND_NOT_IMPLEMENTED   = 502,
  GDICT_STATUS_PARAMETER_NOT_IMPLEMENTED = 503,
  GDICT_STATUS_NO_ACCESS                 = 530,
  GDICT_STATUS_USE_SHOW_INFO             = 531,
  GDICT_STATUS_UNKNOWN_MECHANISM         = 532,
  GDICT_STATUS_BAD_DATABASE              = 550,
  GDICT_STATUS_BAD_STRATEGY              = 551,
  GDICT_STATUS_NO_MATCH                  = 552,
  GDICT_STATUS_NO_DATABASES_PRESENT      = 554,
  GDICT_STATUS_NO_STRATEGIES_PRESENT     = 555
} GdictStatusCode;

#define GDICT_IS_VALID_STATUS_CODE(x)	(((x) > GDICT_STATUS_INVALID) && \
                                         ((x) <= GDICT_STATUS_NO_STRATEGIES_PRESENT))

GOptionGroup *gdict_get_option_group (void);
void          gdict_debug_init       (gint    *argc,
                                      gchar ***argv);

G_END_DECLS

#endif /* __GDICT_UTILS_H__ */
