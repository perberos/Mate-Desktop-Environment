/* MateIconTheme - a loader for icon-themes
 * mate-icon-loader.h Copyright (C) 2002 Red Hat, Inc.
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

#ifndef MATE_ICON_THEME_H
#define MATE_ICON_THEME_H

#ifndef MATE_DISABLE_DEPRECATED

#include <glib-object.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_ICON_THEME             (mate_icon_theme_get_type ())
#define MATE_ICON_THEME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_ICON_THEME, MateIconTheme))
#define MATE_ICON_THEME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_ICON_THEME, MateIconThemeClass))
#define MATE_IS_ICON_THEME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_ICON_THEME))
#define MATE_IS_ICON_THEME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_ICON_THEME))
#define MATE_ICON_THEME_GET_CLASS(obj)   (G_TYPE_CHECK_GET_CLASS ((obj), MATE_TYPE_ICON_THEME, MateIconThemeClass))

typedef GtkIconTheme MateIconTheme;
typedef struct _MateIconThemeClass MateIconThemeClass;

struct _MateIconThemeClass
{
  GObjectClass parent_class;

  void (* changed)  (MateIconTheme *icon_theme);
};

typedef struct
{
  int x;
  int y;
} MateIconDataPoint;

typedef struct
{
  gboolean has_embedded_rect;
  int x0, y0, x1, y1;

  MateIconDataPoint *attach_points;
  int n_attach_points;

  char *display_name;
} MateIconData;

GType            mate_icon_theme_get_type              (void) G_GNUC_CONST;

MateIconTheme *mate_icon_theme_new                   (void);
void            mate_icon_theme_set_search_path       (MateIconTheme       *theme,
							const char           *path[],
							int                   n_elements);
void            mate_icon_theme_get_search_path       (MateIconTheme       *theme,
							char                **path[],
							int                  *n_elements);
void            mate_icon_theme_set_allow_svg         (MateIconTheme       *theme,
							gboolean              allow_svg);
gboolean        mate_icon_theme_get_allow_svg         (MateIconTheme       *theme);
void            mate_icon_theme_append_search_path    (MateIconTheme       *theme,
							const char           *path);
void            mate_icon_theme_prepend_search_path   (MateIconTheme       *theme,
							const char           *path);
void            mate_icon_theme_set_custom_theme      (MateIconTheme       *theme,
							const char           *theme_name);
char *          mate_icon_theme_lookup_icon           (MateIconTheme       *theme,
							const char           *icon_name,
							int                   size,
							const MateIconData **icon_data,
							int                  *base_size);
gboolean        mate_icon_theme_has_icon              (MateIconTheme       *theme,
							const char           *icon_name);
GList *         mate_icon_theme_list_icons            (MateIconTheme       *theme,
							const char           *context);
char *          mate_icon_theme_get_example_icon_name (MateIconTheme       *theme);
gboolean        mate_icon_theme_rescan_if_needed      (MateIconTheme       *theme);

MateIconData * mate_icon_data_dup                    (const MateIconData  *icon_data);
void            mate_icon_data_free                   (MateIconData        *icon_data);

GtkIconTheme   *_mate_icon_theme_get_gtk_icon_theme    (MateIconTheme       *icon_theme);

G_END_DECLS

#endif  /* MATE_DISABLE_DEPRECATED */


#endif /* MATE_ICON_THEME_H */
