/*
 *  Authors: Rodney Dawes <dobey@ximian.com>
 *
 *  Copyright 2003-2006 Novell, Inc. (www.novell.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include <config.h>

#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>
#include <string.h>
#include "mate-wp-item.h"

static MateConfEnumStringPair options_lookup[] = {
  { MATE_BG_PLACEMENT_CENTERED, "centered" },
  { MATE_BG_PLACEMENT_FILL_SCREEN, "stretched" },
  { MATE_BG_PLACEMENT_SCALED, "scaled" },
  { MATE_BG_PLACEMENT_ZOOMED, "zoom" },
  { MATE_BG_PLACEMENT_TILED, "wallpaper" },
  { MATE_BG_PLACEMENT_SPANNED, "spanned" },
  { 0, NULL }
};

static MateConfEnumStringPair shade_lookup[] = {
  { MATE_BG_COLOR_SOLID, "solid" },
  { MATE_BG_COLOR_H_GRADIENT, "horizontal-gradient" },
  { MATE_BG_COLOR_V_GRADIENT, "vertical-gradient" },
  { 0, NULL }
};

const gchar *wp_item_option_to_string (MateBGPlacement type)
{
  return mateconf_enum_to_string (options_lookup, type);
}

const gchar *wp_item_shading_to_string (MateBGColorType type)
{
  return mateconf_enum_to_string (shade_lookup, type);
}

MateBGPlacement wp_item_string_to_option (const gchar *option)
{
  int i = MATE_BG_PLACEMENT_SCALED;
  mateconf_string_to_enum (options_lookup, option, &i);
  return i;
}

MateBGColorType wp_item_string_to_shading (const gchar *shade_type)
{
  int i = MATE_BG_COLOR_SOLID;
  mateconf_string_to_enum (shade_lookup, shade_type, &i);
  return i;
}

static void set_bg_properties (MateWPItem *item)
{
  if (item->filename)
    mate_bg_set_filename (item->bg, item->filename);

  mate_bg_set_color (item->bg, item->shade_type, item->pcolor, item->scolor);
  mate_bg_set_placement (item->bg, item->options);
}

void mate_wp_item_ensure_mate_bg (MateWPItem *item)
{
  if (!item->bg) {
    item->bg = mate_bg_new ();

    set_bg_properties (item);
  }
}

void mate_wp_item_update (MateWPItem *item) {
  MateConfClient *client;
  GdkColor color1 = { 0, 0, 0, 0 }, color2 = { 0, 0, 0, 0 };
  gchar *s;

  client = mateconf_client_get_default ();

  s = mateconf_client_get_string (client, WP_OPTIONS_KEY, NULL);
  item->options = wp_item_string_to_option (s);
  g_free (s);

  s = mateconf_client_get_string (client, WP_SHADING_KEY, NULL);
  item->shade_type = wp_item_string_to_shading (s);
  g_free (s);

  s = mateconf_client_get_string (client, WP_PCOLOR_KEY, NULL);
  if (s != NULL) {
    gdk_color_parse (s, &color1);
    g_free (s);
  }

  s = mateconf_client_get_string (client, WP_SCOLOR_KEY, NULL);
  if (s != NULL) {
    gdk_color_parse (s, &color2);
    g_free (s);
  }

  g_object_unref (client);

  if (item->pcolor != NULL)
    gdk_color_free (item->pcolor);

  if (item->scolor != NULL)
    gdk_color_free (item->scolor);

  item->pcolor = gdk_color_copy (&color1);
  item->scolor = gdk_color_copy (&color2);
}

MateWPItem * mate_wp_item_new (const gchar * filename,
				 GHashTable * wallpapers,
				 MateDesktopThumbnailFactory * thumbnails) {
  MateWPItem *item = g_new0 (MateWPItem, 1);

  item->filename = g_strdup (filename);
  item->fileinfo = mate_wp_info_new (filename, thumbnails);

  if (item->fileinfo != NULL && item->fileinfo->mime_type != NULL &&
      (g_str_has_prefix (item->fileinfo->mime_type, "image/") ||
       strcmp (item->fileinfo->mime_type, "application/xml") == 0)) {

    if (g_utf8_validate (item->fileinfo->name, -1, NULL))
      item->name = g_strdup (item->fileinfo->name);
    else
      item->name = g_filename_to_utf8 (item->fileinfo->name, -1, NULL,
				       NULL, NULL);

    mate_wp_item_update (item);
    mate_wp_item_ensure_mate_bg (item);
    mate_wp_item_update_description (item);

    g_hash_table_insert (wallpapers, item->filename, item);
  } else {
    mate_wp_item_free (item);
    item = NULL;
  }

  return item;
}

void mate_wp_item_free (MateWPItem * item) {
  if (item == NULL) {
    return;
  }

  g_free (item->name);
  g_free (item->filename);
  g_free (item->description);

  if (item->pcolor != NULL)
    gdk_color_free (item->pcolor);

  if (item->scolor != NULL)
    gdk_color_free (item->scolor);

  mate_wp_info_free (item->fileinfo);
  if (item->bg)
    g_object_unref (item->bg);

  gtk_tree_row_reference_free (item->rowref);

  g_free (item);
}

static GdkPixbuf *
add_slideshow_frame (GdkPixbuf *pixbuf)
{
  GdkPixbuf *sheet, *sheet2;
  GdkPixbuf *tmp;
  gint w, h;

  w = gdk_pixbuf_get_width (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);

  sheet = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, w, h);
  gdk_pixbuf_fill (sheet, 0x00000000);
  sheet2 = gdk_pixbuf_new_subpixbuf (sheet, 1, 1, w - 2, h - 2);
  gdk_pixbuf_fill (sheet2, 0xffffffff);
  g_object_unref (sheet2);

  tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, w + 6, h + 6);

  gdk_pixbuf_fill (tmp, 0x00000000);
  gdk_pixbuf_composite (sheet, tmp, 6, 6, w, h, 6.0, 6.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
  gdk_pixbuf_composite (sheet, tmp, 3, 3, w, h, 3.0, 3.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
  gdk_pixbuf_composite (pixbuf, tmp, 0, 0, w, h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);

  g_object_unref (sheet);

  return tmp;
}

GdkPixbuf * mate_wp_item_get_frame_thumbnail (MateWPItem * item,
					       MateDesktopThumbnailFactory * thumbs,
                                               int width,
                                               int height,
                                               gint frame) {
  GdkPixbuf *pixbuf = NULL;

  set_bg_properties (item);

  if (frame != -1)
    pixbuf = mate_bg_create_frame_thumbnail (item->bg, thumbs, gdk_screen_get_default (), width, height, frame);
  else
    pixbuf = mate_bg_create_thumbnail (item->bg, thumbs, gdk_screen_get_default(), width, height);

  if (pixbuf && mate_bg_changes_with_time (item->bg))
    {
      GdkPixbuf *tmp;

      tmp = add_slideshow_frame (pixbuf);
      g_object_unref (pixbuf);
      pixbuf = tmp;
    }

  mate_bg_get_image_size (item->bg, thumbs, width, height, &item->width, &item->height);

  return pixbuf;
}


GdkPixbuf * mate_wp_item_get_thumbnail (MateWPItem * item,
					 MateDesktopThumbnailFactory * thumbs,
                                         gint width,
                                         gint height) {
  return mate_wp_item_get_frame_thumbnail (item, thumbs, width, height, -1);
}

void mate_wp_item_update_description (MateWPItem * item) {
  g_free (item->description);

  if (!strcmp (item->filename, "(none)")) {
    item->description = g_strdup (item->name);
  } else {
    const gchar *description;
    gchar *size;
    gchar *dirname = g_path_get_dirname (item->filename);

    description = NULL;
    size = NULL;

    if (strcmp (item->fileinfo->mime_type, "application/xml") == 0)
      {
        if (mate_bg_changes_with_time (item->bg))
          description = _("Slide Show");
        else if (item->width > 0 && item->height > 0)
          description = _("Image");
      }
    else
      description = g_content_type_get_description (item->fileinfo->mime_type);

    if (mate_bg_has_multiple_sizes (item->bg))
      size = g_strdup (_("multiple sizes"));
    else if (item->width > 0 && item->height > 0) {
      /* translators: x pixel(s) by y pixel(s) */
      size = g_strdup_printf (_("%d %s by %d %s"),
                              item->width,
                              ngettext ("pixel", "pixels", item->width),
                              item->height,
                              ngettext ("pixel", "pixels", item->height));
    }

    if (description && size) {
      /* translators: <b>wallpaper name</b>
       * mime type, size
       * Folder: /path/to/file
       */
      item->description = g_markup_printf_escaped (_("<b>%s</b>\n"
                                                     "%s, %s\n"
                                                     "Folder: %s"),
                                                   item->name,
                                                   description,
                                                   size,
                                                   dirname);
    } else {
      /* translators: <b>wallpaper name</b>
       * Image missing
       * Folder: /path/to/file
       */
      item->description = g_markup_printf_escaped (_("<b>%s</b>\n"
                                                     "%s\n"
                                                     "Folder: %s"),
                                                   item->name,
                                                   _("Image missing"),
                                                   dirname);
    }

    g_free (size);
    g_free (dirname);
  }
}
