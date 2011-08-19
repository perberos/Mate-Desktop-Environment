/* gdict-database-chooser.h - display widget for database names
 *
 * Copyright (C) 2006  Emmanuele Bassi <ebassi@gmail.com>
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

#ifndef __GDICT_DATABASE_CHOOSER_H__
#define __GDICT_DATABASE_CHOOSER_H__

#include <gtk/gtk.h>
#include "gdict-context.h"

G_BEGIN_DECLS

#define GDICT_TYPE_DATABASE_CHOOSER		(gdict_database_chooser_get_type ())
#define GDICT_DATABASE_CHOOSER(obj) \
(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_DATABASE_CHOOSER, GdictDatabaseChooser))
#define GDICT_IS_DATABASE_CHOOSER(obj) \
(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_DATABASE_CHOOSER))
#define GDICT_DATABASE_CHOOSER_CLASS(klass) \
(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_DATABASE_CHOOSER, GdictDatabaseChooserClass))
#define GDICT_IS_DATABASE_CHOOSER_CLASS(klass) \
(G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_DATABASE_CHOOSER))
#define GDICT_DATABASE_CHOOSER_GET_CLASS(obj) \
(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_DATABASE_CHOOSER, GdictDatabaseChooserClass))

typedef struct _GdictDatabaseChooser		GdictDatabaseChooser;
typedef struct _GdictDatabaseChooserPrivate	GdictDatabaseChooserPrivate;
typedef struct _GdictDatabaseChooserClass	GdictDatabaseChooserClass;

struct _GdictDatabaseChooser
{
  /*< private >*/
  GtkVBox parent_instance;
  
  GdictDatabaseChooserPrivate *priv;
};

struct _GdictDatabaseChooserClass
{
  /*< private >*/
  GtkVBoxClass parent_class;

  /*< public >*/
  void (*database_activated) (GdictDatabaseChooser *chooser,
		  	      const gchar          *name,
			      const gchar          *description);
  void (*selection_changed)  (GdictDatabaseChooser *chooser);

  /*< private >*/
  /* padding for future expansion */
  void (*_gdict_padding2) (void);
  void (*_gdict_padding3) (void);
  void (*_gdict_padding4) (void);
  void (*_gdict_padding5) (void);
  void (*_gdict_padding6) (void);
};

GType         gdict_database_chooser_get_type             (void) G_GNUC_CONST;

GtkWidget *   gdict_database_chooser_new                  (void);
GtkWidget *   gdict_database_chooser_new_with_context     (GdictContext         *context);

GdictContext *gdict_database_chooser_get_context          (GdictDatabaseChooser *chooser);
void          gdict_database_chooser_set_context          (GdictDatabaseChooser *chooser,
						           GdictContext         *context);
gboolean      gdict_database_chooser_select_database      (GdictDatabaseChooser *chooser,
                                                           const gchar          *db_name);
gboolean      gdict_database_chooser_unselect_database    (GdictDatabaseChooser *chooser,
                                                           const gchar          *db_name);
gboolean      gdict_database_chooser_set_current_database (GdictDatabaseChooser *chooser,
                                                           const gchar          *db_name);
gchar *       gdict_database_chooser_get_current_database (GdictDatabaseChooser *chooser) G_GNUC_MALLOC;
gchar **      gdict_database_chooser_get_databases        (GdictDatabaseChooser *chooser,
						           gsize                *length) G_GNUC_MALLOC;
gint          gdict_database_chooser_count_databases      (GdictDatabaseChooser *chooser);
gboolean      gdict_database_chooser_has_database         (GdictDatabaseChooser *chooser,
						           const gchar          *database);
void          gdict_database_chooser_refresh              (GdictDatabaseChooser *chooser);
void          gdict_database_chooser_clear                (GdictDatabaseChooser *chooser);
GtkWidget *   gdict_database_chooser_add_button           (GdictDatabaseChooser *chooser,
                                                           const gchar          *button_text);

G_END_DECLS

#endif /* __GDICT_DATABASE_CHOOSER_H__ */
