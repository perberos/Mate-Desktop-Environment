/* gdict-source-chooser.h - display widget for dictionary sources
 *
 * Copyright (C) 2007  Emmanuele Bassi <ebassi@mate.org>
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

#ifndef __GDICT_SOURCE_CHOOSER_H__
#define __GDICT_SOURCE_CHOOSER_H__

#include <gtk/gtk.h>
#include "gdict-source-loader.h"

G_BEGIN_DECLS

#define GDICT_TYPE_SOURCE_CHOOSER               (gdict_source_chooser_get_type ())
#define GDICT_SOURCE_CHOOSER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_SOURCE_CHOOSER, GdictSourceChooser))
#define GDICT_IS_SOURCE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_SOURCE_CHOOSER))
#define GDICT_SOURCE_CHOOSER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_SOURCE_CHOOSER, GdictSourceChooserClass))
#define GDICT_IS_SOURCE_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_SOURCE_CHOOSER))
#define GDICT_SOURCE_CHOOSER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_SOURCE_CHOOSER, GdictSourceChooserClass))

typedef struct _GdictSourceChooser              GdictSourceChooser;
typedef struct _GdictSourceChooserPrivate       GdictSourceChooserPrivate;
typedef struct _GdictSourceChooserClass         GdictSourceChooserClass;

struct _GdictSourceChooser
{
  /*< private >*/
  GtkVBox parent_instance;

  GdictSourceChooserPrivate *priv;
};

struct _GdictSourceChooserClass
{
  /*< private >*/
  GtkVBoxClass parent_class;

  /*< public >*/
  void (*source_activated)  (GdictSourceChooser *chooser,
                             const gchar        *source_name,
                             GdictSource        *source);
  void (*selection_changed) (GdictSourceChooser *chooser);

  /*< private >*/
  /* padding for future expansion */
  void (*_gdict_padding1) (void);
  void (*_gdict_padding2) (void);
  void (*_gdict_padding3) (void);
  void (*_gdict_padding4) (void);
  void (*_gdict_padding5) (void);
  void (*_gdict_padding6) (void);
};

GType gdict_source_chooser_get_type (void) G_GNUC_CONST;

GtkWidget *        gdict_source_chooser_new                (void);
GtkWidget *        gdict_source_chooser_new_with_loader    (GdictSourceLoader *loader);
void               gdict_source_chooser_set_loader         (GdictSourceChooser *chooser,
                                                            GdictSourceLoader  *loader);
GdictSourceLoader *gdict_source_chooser_get_loader         (GdictSourceChooser *chooser);
gboolean           gdict_source_chooser_select_source      (GdictSourceChooser *chooser,
                                                            const gchar        *source_name);
gboolean           gdict_source_chooser_unselect_source    (GdictSourceChooser *chooser,
                                                            const gchar        *source_name);
gboolean           gdict_source_chooser_set_current_source (GdictSourceChooser *chooser,
                                                            const gchar        *source_name);
gchar *            gdict_source_chooser_get_current_source (GdictSourceChooser *chooser) G_GNUC_MALLOC;
gchar **           gdict_source_chooser_get_sources        (GdictSourceChooser *chooser,
                                                            gsize              *length) G_GNUC_MALLOC;
gint               gdict_source_chooser_count_sources      (GdictSourceChooser *chooser);
gboolean           gdict_source_chooser_has_source         (GdictSourceChooser *chooser,
                                                            const gchar        *source_name);
void               gdict_source_chooser_refresh            (GdictSourceChooser *chooser);
GtkWidget *        gdict_source_chooser_add_button         (GdictSourceChooser *chooser,
                                                            const gchar        *button_text);

G_END_DECLS

#endif /* __GDICT_SOURCE_CHOOSER_H__ */
