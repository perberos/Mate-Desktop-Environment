/* MateIconThemes - a loader for icon-themes
 * mate-icon-theme.c Copyright (C) 2002 Red Hat, Inc.
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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "mate-icon-theme.h"

typedef struct _GtkIconThemePrivate MateIconThemePrivate;

struct _GtkIconThemePrivate
{
  GtkIconTheme *gtk_theme;
  gboolean allow_svg;
  MateIconData icon_data;
};

static void   mate_icon_theme_class_init (MateIconThemeClass *klass);
static void   mate_icon_theme_init       (MateIconTheme      *icon_theme);
static void   mate_icon_theme_finalize   (GObject             *object);
static void   set_gtk_theme               (MateIconTheme      *icon_theme,
					   GtkIconTheme        *gtk_theme);


static guint		 signal_changed = 0;
static GObjectClass     *parent_class = NULL;

GType
mate_icon_theme_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    {
      const GTypeInfo info =
	{
	  sizeof (MateIconThemeClass),
	  NULL,           /* base_init */
	  NULL,           /* base_finalize */
	  (GClassInitFunc) mate_icon_theme_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (MateIconTheme),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) mate_icon_theme_init
	};

      type = g_type_register_static (G_TYPE_OBJECT, "MateIconTheme", &info, 0);
    }

  return type;
}

MateIconTheme *
mate_icon_theme_new (void)
{
  return g_object_new (MATE_TYPE_ICON_THEME, NULL);
}

static void
gtk_theme_changed_callback (GtkIconTheme *gtk_theme,
			    gpointer user_data)
{
  MateIconTheme *icon_theme;

  icon_theme = user_data;
  
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

static void
mate_icon_theme_class_init (MateIconThemeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = mate_icon_theme_finalize;

  signal_changed = g_signal_new ("changed",
				 G_TYPE_FROM_CLASS (klass),
				 G_SIGNAL_RUN_LAST,
				 G_STRUCT_OFFSET (MateIconThemeClass, changed),
				 NULL, NULL,
				 g_cclosure_marshal_VOID__VOID,
				 G_TYPE_NONE, 0);
}

static void
set_gtk_theme (MateIconTheme *icon_theme,
	       GtkIconTheme *gtk_theme)
{
  icon_theme->priv->gtk_theme = gtk_theme;
  g_signal_connect_object (gtk_theme,
			   "changed",
			   G_CALLBACK (gtk_theme_changed_callback),
			   icon_theme, 0);
}

static GtkIconTheme *
get_gtk_theme (MateIconTheme *icon_theme)
{
  MateIconThemePrivate *priv;
  GtkIconTheme *gtk_theme;
  
  priv = icon_theme->priv;

  if (priv->gtk_theme == NULL)
    {
      gtk_theme = gtk_icon_theme_new ();
      gtk_icon_theme_set_screen (gtk_theme, gdk_screen_get_default ());
      set_gtk_theme (icon_theme, gtk_theme);
    }
  
  return priv->gtk_theme;
}

GtkIconTheme  *
_mate_icon_theme_get_gtk_icon_theme (MateIconTheme *icon_theme)
{
  return get_gtk_theme (icon_theme);
}


static void
mate_icon_theme_init (MateIconTheme *icon_theme)
{
  MateIconThemePrivate *priv;

  priv = g_new0 (MateIconThemePrivate, 1);
  
  icon_theme->priv = priv;

  priv->gtk_theme = NULL;
  priv->allow_svg = FALSE;
}

static void
mate_icon_theme_finalize (GObject *object)
{
  MateIconTheme *icon_theme;
  MateIconThemePrivate *priv;

  icon_theme = MATE_ICON_THEME (object);
  priv = icon_theme->priv;

  if (priv->gtk_theme)
    g_object_unref (priv->gtk_theme);
  
  g_free (priv);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
mate_icon_theme_set_allow_svg (MateIconTheme      *icon_theme,
				 gboolean              allow_svg)
{
  allow_svg = allow_svg != FALSE;

  if (allow_svg == icon_theme->priv->allow_svg)
    return;
  
  icon_theme->priv->allow_svg = allow_svg;
  
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

gboolean
mate_icon_theme_get_allow_svg (MateIconTheme *icon_theme)
{
  return icon_theme->priv->allow_svg;
}

void
mate_icon_theme_set_search_path (MateIconTheme *icon_theme,
				  const char *path[],
				  int         n_elements)
{
  gtk_icon_theme_set_search_path (get_gtk_theme (icon_theme),
				  path, n_elements);
}


void
mate_icon_theme_get_search_path (MateIconTheme      *icon_theme,
				  char                 **path[],
				  int                   *n_elements)
{
  gtk_icon_theme_get_search_path (get_gtk_theme (icon_theme),
				  path, n_elements);
}

void
mate_icon_theme_append_search_path (MateIconTheme      *icon_theme,
				     const char          *path)
{
  gtk_icon_theme_append_search_path (get_gtk_theme (icon_theme), path);
}

void
mate_icon_theme_prepend_search_path (MateIconTheme      *icon_theme,
				       const char           *path)
{
  gtk_icon_theme_prepend_search_path (get_gtk_theme (icon_theme), path);
}

void
mate_icon_theme_set_custom_theme (MateIconTheme *icon_theme,
				   const char *theme_name)
{
  gtk_icon_theme_set_custom_theme (get_gtk_theme (icon_theme), theme_name);
}

char *
mate_icon_theme_lookup_icon (MateIconTheme      *icon_theme,
			      const char           *icon_name,
			      int                   size,
			      const MateIconData **icon_data_out,
			      int                  *base_size)
{
  MateIconThemePrivate *priv;
  int i;
  char *icon;
  GtkIconInfo *info;
  GtkIconLookupFlags flags;
  GdkRectangle rectangle;
  GdkPoint *points;
  int n_points;
  MateIconData *icon_data;
  priv = icon_theme->priv;

  if (icon_data_out)
    *icon_data_out = NULL;
  
  if (priv->allow_svg)
      flags = GTK_ICON_LOOKUP_FORCE_SVG;
  else
      flags = GTK_ICON_LOOKUP_NO_SVG;
  
  info = gtk_icon_theme_lookup_icon (get_gtk_theme (icon_theme),
				     icon_name, size, flags);

  if (info == NULL)
    return NULL;

  icon = g_strdup (gtk_icon_info_get_filename (info));
  
  if (base_size != NULL)
    *base_size = gtk_icon_info_get_base_size (info);

  /* Free the old info */

  icon_data = &priv->icon_data;
  g_free (icon_data->display_name);
  g_free (icon_data->attach_points);
  memset (icon_data, 0, sizeof (MateIconData));
  
  icon_data->display_name = g_strdup (gtk_icon_info_get_display_name (info));

  gtk_icon_info_set_raw_coordinates (info, TRUE);
  icon_data->has_embedded_rect = gtk_icon_info_get_embedded_rect (info, &rectangle);
  if (icon_data->has_embedded_rect)
    {
      icon_data->x0 = rectangle.x;
      icon_data->y0 = rectangle.y;
      icon_data->x1 = rectangle.x + rectangle.width;
      icon_data->y1 = rectangle.y + rectangle.height;
    }
  
  if (gtk_icon_info_get_attach_points (info, &points, &n_points))
    {
      icon_data->n_attach_points = n_points;
      icon_data->attach_points = g_new (MateIconDataPoint, n_points);
      for (i = 0; i < n_points; i++)
	{
	  icon_data->attach_points[i].x = points[i].x;
	  icon_data->attach_points[i].y = points[i].y;
	}
      g_free (points);
    }

  if (icon_data_out != NULL &&
      (icon_data->has_embedded_rect ||
       icon_data->attach_points != NULL ||
       icon_data->display_name != NULL))
    *icon_data_out = icon_data;
  
   
  gtk_icon_info_free (info);
    
  return icon;
}

gboolean 
mate_icon_theme_has_icon (MateIconTheme      *icon_theme,
			    const char           *icon_name)
{
  return gtk_icon_theme_has_icon (get_gtk_theme (icon_theme), icon_name);
}



GList *
mate_icon_theme_list_icons (MateIconTheme *icon_theme,
			     const char *context)
{
  return gtk_icon_theme_list_icons (get_gtk_theme (icon_theme), context);
}

char *
mate_icon_theme_get_example_icon_name (MateIconTheme *icon_theme)
{
  return gtk_icon_theme_get_example_icon_name (get_gtk_theme (icon_theme));
}

gboolean
mate_icon_theme_rescan_if_needed (MateIconTheme *icon_theme)
{
  return gtk_icon_theme_rescan_if_needed (get_gtk_theme (icon_theme));
}

void
mate_icon_data_free (MateIconData *icon_data)
{
  g_free (icon_data->attach_points);
  g_free (icon_data->display_name);
  g_free (icon_data);
}

MateIconData *
mate_icon_data_dup (const MateIconData *icon_data)
{
  MateIconData *copy;

  copy = g_memdup (icon_data, sizeof (MateIconData));
  
  copy->display_name = g_strdup (copy->display_name);
  
  if (copy->attach_points)
    copy->attach_points = g_memdup (copy->attach_points,
				    copy->n_attach_points * sizeof (MateIconDataPoint));
  return copy;
}

