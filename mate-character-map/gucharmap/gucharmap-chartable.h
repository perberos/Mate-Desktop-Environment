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

#ifndef GUCHARMAP_CHARTABLE_H
#define GUCHARMAP_CHARTABLE_H

#include <gtk/gtk.h>

#include <gucharmap/gucharmap-codepoint-list.h>

G_BEGIN_DECLS

#define GUCHARMAP_TYPE_CHARTABLE             (gucharmap_chartable_get_type ())
#define GUCHARMAP_CHARTABLE(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), GUCHARMAP_TYPE_CHARTABLE, GucharmapChartable))
#define GUCHARMAP_CHARTABLE_CLASS(k)         (G_TYPE_CHECK_CLASS_CAST((k), GUCHARMAP_TYPE_CHARTABLE, GucharmapChartableClass))
#define GUCHARMAP_IS_CHARTABLE(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), GUCHARMAP_TYPE_CHARTABLE))
#define GUCHARMAP_IS_CHARTABLE_CLASS(k)      (G_TYPE_CHECK_CLASS_TYPE ((k), GUCHARMAP_TYPE_CHARTABLE))
#define GUCHARMAP_CHARTABLE_GET_CLASS(o)     (G_TYPE_INSTANCE_GET_CLASS ((o), GUCHARMAP_TYPE_CHARTABLE, GucharmapChartableClass))

typedef struct _GucharmapChartable        GucharmapChartable;
typedef struct _GucharmapChartablePrivate GucharmapChartablePrivate;
typedef struct _GucharmapChartableClass   GucharmapChartableClass;

struct _GucharmapChartable
{
  GtkDrawingArea parent_instance;

  /*< private >*/
  GucharmapChartablePrivate *priv;
};

struct _GucharmapChartableClass
{
  GtkDrawingAreaClass parent_class;

  void    (* set_scroll_adjustments) (GucharmapChartable *chartable,
                                      GtkAdjustment      *hadjustment,
                                      GtkAdjustment      *vadjustment);
  gboolean (* move_cursor)           (GucharmapChartable *chartable,
                                      GtkMovementStep     step,
                                      gint                count);
  void (* activate) (GucharmapChartable *chartable);
  void (* copy_clipboard) (GucharmapChartable *chartable);
  void (* paste_clipboard) (GucharmapChartable *chartable);

  void (* set_active_char) (GucharmapChartable *chartable, guint ch);
  void (* status_message) (GucharmapChartable *chartable, const gchar *message);
};

GType gucharmap_chartable_get_type (void);
GtkWidget * gucharmap_chartable_new (void);
void gucharmap_chartable_set_font_desc (GucharmapChartable *chartable,
                                        PangoFontDescription *font_desc);
PangoFontDescription * gucharmap_chartable_get_font_desc (GucharmapChartable *chartable);
gunichar gucharmap_chartable_get_active_character (GucharmapChartable *chartable);
void gucharmap_chartable_set_active_character (GucharmapChartable *chartable, 
                                               gunichar uc);
void gucharmap_chartable_set_zoom_enabled (GucharmapChartable *chartable,
                                           gboolean enabled);
gboolean gucharmap_chartable_get_zoom_enabled (GucharmapChartable *chartable);
void gucharmap_chartable_set_snap_pow2 (GucharmapChartable *chartable,
                                        gboolean snap);
gboolean gucharmap_chartable_get_snap_pow2 (GucharmapChartable *chartable);
void gucharmap_chartable_set_codepoint_list (GucharmapChartable         *chartable,
                                             GucharmapCodepointList *list);
GucharmapCodepointList * gucharmap_chartable_get_codepoint_list (GucharmapChartable *chartable);

G_END_DECLS

#endif  /* #ifndef GUCHARMAP_CHARTABLE_H */
