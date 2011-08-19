/* gdict-app.h - main application class
 *
 * This file is part of MATE Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __GDICT_APP_H__
#define __GDICT_APP_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <libgdict/gdict.h>

#include "gdict-window.h"

G_BEGIN_DECLS

#define GDICT_TYPE_APP		(gdict_app_get_type ())
#define GDICT_APP(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_APP, GdictApp))
#define GDICT_IS_APP(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_APP))

typedef struct _GdictApp         GdictApp;
typedef struct _GdictAppClass    GdictAppClass;


struct _GdictApp
{
  GObject parent_instance;

  MateConfClient *mateconf_client;

  GSList *lookup_words;
  GSList *match_words;
  gint remaining_words;
  
  gchar *database;
  gchar *source_name;
  gboolean no_window;
  gboolean list_sources;
  
  GdictSourceLoader *loader;

  GdictWindow *current_window;
  GSList *windows;  
};


GType      gdict_app_get_type    (void) G_GNUC_CONST;

void       gdict_init            (int *argc, char ***argv);
void       gdict_main            (void);
void       gdict_cleanup         (void);

G_END_DECLS

#endif /* __GDICT_APP_H__ */
