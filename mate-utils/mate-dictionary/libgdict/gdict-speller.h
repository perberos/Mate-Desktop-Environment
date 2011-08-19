/* gdict-speller.h - display widget for dictionary matches
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

#ifndef __GDICT_SPELLER_H__
#define __GDICT_SPELLER_H__

#include <gtk/gtk.h>
#include "gdict-context.h"

G_BEGIN_DECLS

#define GDICT_TYPE_SPELLER 		(gdict_speller_get_type ())
#define GDICT_SPELLER(obj) 		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_SPELLER, GdictSpeller))
#define GDICT_SPELLER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_SPELLER, GdictSpellerClass))
#define GDICT_IS_SPELLER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_SPELLER))
#define GDICT_IS_SPELLER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_SPELLER))
#define GDICT_SPELLER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_SPELLER, GdictSpellerClass))

typedef struct _GdictSpeller		GdictSpeller;
typedef struct _GdictSpellerPrivate	GdictSpellerPrivate;
typedef struct _GdictSpellerClass	GdictSpellerClass;

struct _GdictSpeller
{
  GtkVBox parent_instance;
  
  /*< private >*/
  GdictSpellerPrivate *priv;
};

struct _GdictSpellerClass
{
  GtkVBoxClass parent_class;

  void (*word_activated) (GdictSpeller *speller,
		          const gchar  *word,
			  const gchar  *database);

  /* padding for future expansion */
  void (*_gdict_speller_1) (void);
  void (*_gdict_speller_2) (void);
  void (*_gdict_speller_3) (void);
  void (*_gdict_speller_4) (void);  
};

GType                 gdict_speller_get_type         (void) G_GNUC_CONST;

GtkWidget *           gdict_speller_new              (void);
GtkWidget *           gdict_speller_new_with_context (GdictContext *context);

void                  gdict_speller_set_context      (GdictSpeller *speller,
						      GdictContext *context);
GdictContext *        gdict_speller_get_context      (GdictSpeller *speller);
void                  gdict_speller_set_database     (GdictSpeller *speller,
						      const gchar *database);
G_CONST_RETURN gchar *gdict_speller_get_database     (GdictSpeller *speller);
void                  gdict_speller_set_strategy     (GdictSpeller *speller,
						      const gchar  *strategy);
G_CONST_RETURN gchar *gdict_speller_get_strategy     (GdictSpeller *speller);

void                  gdict_speller_clear            (GdictSpeller *speller);
void                  gdict_speller_match            (GdictSpeller *speller,
						      const gchar  *word);

gint                  gdict_speller_count_matches    (GdictSpeller *speller);
gchar **              gdict_speller_get_matches      (GdictSpeller *speller,
						      gsize         length);

G_END_DECLS

#endif /* __GDICT_SPELLER_H__ */
