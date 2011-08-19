/*
 * Copyright © 2004 Noah Levitt
 * Copyright © 2007 Christian Persch
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA
 */

#if !defined (__GUCHARMAP_GUCHARMAP_H_INSIDE__) && !defined (GUCHARMAP_COMPILATION)
#error "Only <gucharmap/gucharmap.h> can be included directly."
#endif

#ifndef GUCHARMAP_CHAPTERS_MODEL_H
#define GUCHARMAP_CHAPTERS_MODEL_H

#include <gtk/gtk.h>

#include <gucharmap/gucharmap-codepoint-list.h>

G_BEGIN_DECLS

#define GUCHARMAP_TYPE_CHAPTERS_MODEL (gucharmap_chapters_model_get_type ())

#define GUCHARMAP_TYPE_CHAPTERS_MODEL             (gucharmap_chapters_model_get_type ())
#define GUCHARMAP_CHAPTERS_MODEL(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), GUCHARMAP_TYPE_CHAPTERS_MODEL, GucharmapChaptersModel))
#define GUCHARMAP_CHAPTERS_MODEL_CLASS(k)         (G_TYPE_CHECK_CLASS_CAST((k), GUCHARMAP_TYPE_CHAPTERS_MODEL, GucharmapChaptersModelClass))
#define GUCHARMAP_IS_CHAPTERS_MODEL(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), GUCHARMAP_TYPE_CHAPTERS_MODEL))
#define GUCHARMAP_IS_CHAPTERS_MODEL_CLASS(k)      (G_TYPE_CHECK_CLASS_TYPE ((k), GUCHARMAP_TYPE_CHAPTERS_MODEL))
#define GUCHARMAP_CHAPTERS_MODEL_GET_CLASS(o)     (G_TYPE_INSTANCE_GET_CLASS ((o), GUCHARMAP_TYPE_CHAPTERS_MODEL, GucharmapChaptersModelClass))

typedef struct _GucharmapChaptersModel        GucharmapChaptersModel;
typedef struct _GucharmapChaptersModelPrivate GucharmapChaptersModelPrivate;
typedef struct _GucharmapChaptersModelClass   GucharmapChaptersModelClass;

struct _GucharmapChaptersModel
{
  GtkListStore parent_instance;

  /*< private >*/
  GucharmapChaptersModelPrivate *priv;
};

struct _GucharmapChaptersModelClass
{
  GtkListStoreClass parent_class;

  const char *title;
  gboolean (* character_to_iter) (GucharmapChaptersModel *chapters,
                                  gunichar wc,
                                  GtkTreeIter *iter);
  GucharmapCodepointList * (* get_codepoint_list) (GucharmapChaptersModel *chapters,
                                                   GtkTreeIter *iter);
  GucharmapCodepointList * (* get_book_codepoint_list) (GucharmapChaptersModel *chapters);
};


enum {
  GUCHARMAP_CHAPTERS_MODEL_COLUMN_ID    = 0,
  GUCHARMAP_CHAPTERS_MODEL_COLUMN_LABEL = 1
};

GType                    gucharmap_chapters_model_get_type                (void);
GucharmapCodepointList * gucharmap_chapters_model_get_codepoint_list      (GucharmapChaptersModel *chapters,
                                                                                          GtkTreeIter            *iter);
const char *             gucharmap_chapters_model_get_title               (GucharmapChaptersModel *chapters);
GucharmapCodepointList * gucharmap_chapters_model_get_book_codepoint_list (GucharmapChaptersModel *chapters);
gboolean                 gucharmap_chapters_model_character_to_iter       (GucharmapChaptersModel *chapters,
                                                                           gunichar                wc,
                                                                           GtkTreeIter            *iter);
gboolean                 gucharmap_chapters_model_id_to_iter              (GucharmapChaptersModel *model,
                                                                           const char             *id,
                                                                           GtkTreeIter            *iter);

G_END_DECLS

#endif /* #ifndef GUCHARMAP_CHAPTERS_MODEL_H */
