/* -*- mode: C; c-basic-offset: 4 -*-
 * themus - utilities for MATE themes
 * Copyright (C) 2002 Jonathan Blandford <aes@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <mateconf/mateconf-client.h>
#include <mate-wm-manager.h>
#include "mate-theme-apply.h"
#include "gtkrc-utils.h"

#define GTK_THEME_KEY      "/desktop/mate/interface/gtk_theme"
#define COLOR_SCHEME_KEY   "/desktop/mate/interface/gtk_color_scheme"
#define ICON_THEME_KEY     "/desktop/mate/interface/icon_theme"
#define FONT_KEY	         "/desktop/mate/interface/font_name"
#define CURSOR_FONT_KEY   "/desktop/mate/peripherals/mouse/cursor_font"
#define CURSOR_THEME_KEY   "/desktop/mate/peripherals/mouse/cursor_theme"
#define CURSOR_SIZE_KEY    "/desktop/mate/peripherals/mouse/cursor_size"
#define NOTIFICATION_THEME_KEY    "/apps/notification-daemon/theme"

#define compare(x,y) (!x && y) || (x && !y) || (x && y && strcmp (x, y))

void
mate_meta_theme_set (MateThemeMetaInfo *meta_theme_info)
{
  MateConfClient *client;
  gchar *old_key;
  gint old_key_int;
  MateWindowManager *window_manager;
  MateWMSettings wm_settings;

  mate_wm_manager_init ();

  window_manager = mate_wm_manager_get_current (gdk_display_get_default_screen (gdk_display_get_default ()));

  client = mateconf_client_get_default ();

  /* Set the gtk+ key */
  old_key = mateconf_client_get_string (client, GTK_THEME_KEY, NULL);
  if (compare (old_key, meta_theme_info->gtk_theme_name))
    {
      mateconf_client_set_string (client, GTK_THEME_KEY, meta_theme_info->gtk_theme_name, NULL);
    }
  g_free (old_key);

  /* Set the color scheme key */
  old_key = mateconf_client_get_string (client, COLOR_SCHEME_KEY, NULL);
  if (compare (old_key, meta_theme_info->gtk_color_scheme))
    {
      /* only save the color scheme if it differs from the default
         scheme for the selected gtk theme */
      gchar *newval, *gtkcols;

      newval = meta_theme_info->gtk_color_scheme;
      gtkcols = gtkrc_get_color_scheme_for_theme (meta_theme_info->gtk_theme_name);

      if (newval == NULL || !strcmp (newval, "") ||
          mate_theme_color_scheme_equal (newval, gtkcols))
        {
          mateconf_client_unset (client, COLOR_SCHEME_KEY, NULL);
        }
      else
        {
          mateconf_client_set_string (client, COLOR_SCHEME_KEY, newval, NULL);
        }
      g_free (gtkcols);
    }
  g_free (old_key);

  /* Set the wm key */
  wm_settings.flags = MATE_WM_SETTING_THEME;
  wm_settings.theme = meta_theme_info->marco_theme_name;
  if (window_manager)
    mate_window_manager_change_settings (window_manager, &wm_settings);

  /* set the icon theme */
  old_key = mateconf_client_get_string (client, ICON_THEME_KEY, NULL);
  if (compare (old_key, meta_theme_info->icon_theme_name))
    {
      mateconf_client_set_string (client, ICON_THEME_KEY, meta_theme_info->icon_theme_name, NULL);
    }
  g_free (old_key);

  /* set the notification theme */
  if (meta_theme_info->notification_theme_name != NULL)
    {
      old_key = mateconf_client_get_string (client, NOTIFICATION_THEME_KEY, NULL);
      if (compare (old_key, meta_theme_info->notification_theme_name))
        {
          mateconf_client_set_string (client, NOTIFICATION_THEME_KEY, meta_theme_info->notification_theme_name, NULL);
        }
      g_free (old_key);
    }

  /* Set the cursor theme key */
#ifdef HAVE_XCURSOR
  old_key = mateconf_client_get_string (client, CURSOR_THEME_KEY, NULL);
  if (compare (old_key, meta_theme_info->cursor_theme_name))
    {
      mateconf_client_set_string (client, CURSOR_THEME_KEY, meta_theme_info->cursor_theme_name, NULL);
    }

  old_key_int = mateconf_client_get_int (client, CURSOR_SIZE_KEY, NULL);
  if (old_key_int != meta_theme_info->cursor_size)
    {
      mateconf_client_set_int (client, CURSOR_SIZE_KEY, meta_theme_info->cursor_size, NULL);
    }
#else
  old_key = mateconf_client_get_string (client, CURSOR_FONT_KEY, NULL);
  if (compare (old_key, meta_theme_info->cursor_theme_name))
    {
      mateconf_client_set_string (client, CURSOR_FONT_KEY, meta_theme_info->cursor_theme_name, NULL);
    }
#endif

  g_free (old_key);
  g_object_unref (client);
}
