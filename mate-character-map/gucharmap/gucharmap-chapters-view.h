/*
 * Copyright © 2004 Noah Levitt
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

/* block means unicode block */

#if !defined (__GUCHARMAP_GUCHARMAP_H_INSIDE__) && !defined (GUCHARMAP_COMPILATION)
#error "Only <gucharmap/gucharmap.h> can be included directly."
#endif

#ifndef GUCHARMAP_CHAPTERS_VIEW_H
#define GUCHARMAP_CHAPTERS_VIEW_H

#include <gtk/gtk.h>

#include <gucharmap/gucharmap-chapters-model.h>

G_BEGIN_DECLS

#define GUCHARMAP_TYPE_CHAPTERS_VIEW             (gucharmap_chapters_view_get_type ())
#define GUCHARMAP_CHAPTERS_VIEW(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), GUCHARMAP_TYPE_CHAPTERS_VIEW, GucharmapChaptersView))
#define GUCHARMAP_CHAPTERS_VIEW_CLASS(k)         (G_TYPE_CHECK_CLASS_CAST((k), GUCHARMAP_TYPE_CHAPTERS_VIEW, GucharmapChaptersViewClass))
#define GUCHARMAP_IS_CHAPTERS_VIEW(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), GUCHARMAP_TYPE_CHAPTERS_VIEW))
#define GUCHARMAP_IS_CHAPTERS_VIEW_CLASS(k)      (G_TYPE_CHECK_CLASS_TYPE ((k), GUCHARMAP_TYPE_CHAPTERS_VIEW))
#define GUCHARMAP_CHAPTERS_VIEW_GET_CLASS(o)     (G_TYPE_INSTANCE_GET_CLASS ((o), GUCHARMAP_TYPE_CHAPTERS_VIEW, GucharmapChaptersViewClass))

typedef struct _GucharmapChaptersView         GucharmapChaptersView;
typedef struct _GucharmapChaptersViewPrivate  GucharmapChaptersViewPrivate;
typedef struct _GucharmapChaptersViewClass    GucharmapChaptersViewClass;

struct _GucharmapChaptersView
{
  GtkTreeView parent_instance;

  /*< private >*/
  GucharmapChaptersViewPrivate *priv;
};

struct _GucharmapChaptersViewClass
{
  GtkTreeViewClass parent_class;
};

GType       gucharmap_chapters_view_get_type (void);

GtkWidget * gucharmap_chapters_view_new      (void);

void                    gucharmap_chapters_view_set_model (GucharmapChaptersView *view,
                                                           GucharmapChaptersModel *model);
GucharmapChaptersModel *gucharmap_chapters_view_get_model (GucharmapChaptersView *view);

gboolean           gucharmap_chapters_view_select_character (GucharmapChaptersView *view,
                                                             gunichar           wc);
GucharmapCodepointList * gucharmap_chapters_view_get_codepoint_list      (GucharmapChaptersView *view);
GucharmapCodepointList * gucharmap_chapters_view_get_book_codepoint_list (GucharmapChaptersView *view);

void               gucharmap_chapters_view_next         (GucharmapChaptersView *view);
void               gucharmap_chapters_view_previous     (GucharmapChaptersView *view);

gchar *            gucharmap_chapters_view_get_selected  (GucharmapChaptersView *view);
gboolean           gucharmap_chapters_view_set_selected  (GucharmapChaptersView *view,
                                                          const gchar       *name);

gboolean           gucharmap_chapters_view_select_locale (GucharmapChaptersView *view);

G_END_DECLS

#endif /* #ifndef GUCHARMAP_CHAPTERS_VIEW_H */
