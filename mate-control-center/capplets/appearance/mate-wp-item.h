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

#include <glib.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <libmateui/mate-desktop-thumbnail.h>
#include <libmateui/mate-bg.h>

#include "mate-wp-info.h"

#ifndef _MATE_WP_ITEM_H_
#define _MATE_WP_ITEM_H_

#define WP_PATH_KEY "/desktop/mate/background"
#define WP_FILE_KEY WP_PATH_KEY "/picture_filename"
#define WP_OPTIONS_KEY WP_PATH_KEY "/picture_options"
#define WP_SHADING_KEY WP_PATH_KEY "/color_shading_type"
#define WP_PCOLOR_KEY WP_PATH_KEY "/primary_color"
#define WP_SCOLOR_KEY WP_PATH_KEY "/secondary_color"

typedef struct _MateWPItem MateWPItem;

struct _MateWPItem {
  MateBG *bg;

  gchar * name;
  gchar * filename;
  gchar * description;
  MateBGPlacement options;
  MateBGColorType shade_type;

  /* Where the Item is in the List */
  GtkTreeRowReference * rowref;

  /* Real colors */
  GdkColor * pcolor;
  GdkColor * scolor;

  MateWPInfo * fileinfo;

  /* Did the user remove us? */
  gboolean deleted;

  /* Width and Height of the original image */
  gint width;
  gint height;
};

MateWPItem * mate_wp_item_new (const gchar *filename,
				 GHashTable *wallpapers,
				 MateDesktopThumbnailFactory *thumbnails);

void mate_wp_item_free (MateWPItem *item);
GdkPixbuf * mate_wp_item_get_thumbnail (MateWPItem *item,
					 MateDesktopThumbnailFactory *thumbs,
                                         gint width,
                                         gint height);
GdkPixbuf * mate_wp_item_get_frame_thumbnail (MateWPItem *item,
                                               MateDesktopThumbnailFactory *thumbs,
                                               gint width,
                                               gint height,
                                               gint frame);
void mate_wp_item_update (MateWPItem *item);
void mate_wp_item_update_description (MateWPItem *item);
void mate_wp_item_ensure_mate_bg (MateWPItem *item);

const gchar *wp_item_option_to_string (MateBGPlacement type);
const gchar *wp_item_shading_to_string (MateBGColorType type);
MateBGPlacement wp_item_string_to_option (const gchar *option);
MateBGColorType wp_item_string_to_shading (const gchar *shade_type);

#endif
