/*
 * Copyright (C) 2007,2008 The MATE Foundation
 * Written by Rodney Dawes <dobey@ximian.com>
 *            Denis Washington <denisw@svn.gnome.org>
 *            Thomas Wood <thos@gnome.org>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "appearance.h"
#include "mate-wp-info.h"
#include "mate-wp-item.h"
#include "mate-wp-xml.h"
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <string.h>
#include <mateconf/mateconf-client.h>
#include <libmateui/mate-desktop-thumbnail.h>
#include <libmateui/mate-bg.h>

enum {
	TARGET_URI_LIST,
	TARGET_BGIMAGE
};

static const GtkTargetEntry drop_types[] = {
	{"text/uri-list", 0, TARGET_URI_LIST},
	{"property/bgimage", 0, TARGET_BGIMAGE}
};

static const GtkTargetEntry drag_types[] = {
	{"text/uri-list", GTK_TARGET_OTHER_WIDGET, TARGET_URI_LIST}
};


static void wp_update_preview(GtkFileChooser* chooser, AppearanceData* data);

static void select_item(AppearanceData* data, MateWPItem* item, gboolean scroll)
{
	GtkTreePath* path;

	g_return_if_fail(data != NULL);

	if (item == NULL)
		return;

	path = gtk_tree_row_reference_get_path(item->rowref);

	gtk_icon_view_select_path(data->wp_view, path);

	if (scroll)
	{
		gtk_icon_view_scroll_to_path(data->wp_view, path, FALSE, 0.5, 0.0);
	}

	gtk_tree_path_free(path);
}

static MateWPItem* get_selected_item(AppearanceData* data, GtkTreeIter* iter)
{
	MateWPItem* item = NULL;
	GList* selected;

	selected = gtk_icon_view_get_selected_items (data->wp_view);

	if (selected != NULL)
	{
		GtkTreeIter sel_iter;

		gtk_tree_model_get_iter(data->wp_model, &sel_iter, selected->data);

		g_list_foreach(selected, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(selected);

		if (iter)
			*iter = sel_iter;

		gtk_tree_model_get(data->wp_model, &sel_iter, 1, &item, -1);
	}

	return item;
}

static gboolean predicate (gpointer key, gpointer value, gpointer data)
{
  MateBG *bg = data;
  MateWPItem *item = value;

  return item->bg == bg;
}

static void on_item_changed (MateBG *bg, AppearanceData *data) {
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;
  MateWPItem *item;

  item = g_hash_table_find (data->wp_hash, predicate, bg);

  if (!item)
    return;

  model = gtk_tree_row_reference_get_model (item->rowref);
  path = gtk_tree_row_reference_get_path (item->rowref);

  if (gtk_tree_model_get_iter (model, &iter, path)) {
    GdkPixbuf *pixbuf;

    g_signal_handlers_block_by_func (bg, G_CALLBACK (on_item_changed), data);

    pixbuf = mate_wp_item_get_thumbnail (item,
                                          data->thumb_factory,
                                          data->thumb_width,
                                          data->thumb_height);
    if (pixbuf) {
      gtk_list_store_set (GTK_LIST_STORE (data->wp_model), &iter, 0, pixbuf, -1);
      g_object_unref (pixbuf);
    }

    g_signal_handlers_unblock_by_func (bg, G_CALLBACK (on_item_changed), data);
  }
}

static void
wp_props_load_wallpaper (gchar *key,
                         MateWPItem *item,
                         AppearanceData *data)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  GdkPixbuf *pixbuf;

  if (item->deleted == TRUE)
    return;

  gtk_list_store_append (GTK_LIST_STORE (data->wp_model), &iter);

  pixbuf = mate_wp_item_get_thumbnail (item, data->thumb_factory,
                                        data->thumb_width,
                                        data->thumb_height);
  mate_wp_item_update_description (item);

  gtk_list_store_set (GTK_LIST_STORE (data->wp_model), &iter,
                      0, pixbuf,
                      1, item,
                      -1);

  if (pixbuf != NULL)
    g_object_unref (pixbuf);

  path = gtk_tree_model_get_path (data->wp_model, &iter);
  item->rowref = gtk_tree_row_reference_new (data->wp_model, path);
  g_signal_connect (item->bg, "changed", G_CALLBACK (on_item_changed), data);
  gtk_tree_path_free (path);
}

static MateWPItem *
wp_add_image (AppearanceData *data,
              const gchar *filename)
{
  MateWPItem *item;

  if (!filename)
    return NULL;

  item = g_hash_table_lookup (data->wp_hash, filename);

  if (item != NULL)
  {
    if (item->deleted)
    {
      item->deleted = FALSE;
      wp_props_load_wallpaper (item->filename, item, data);
    }
  }
  else
  {
    item = mate_wp_item_new (filename, data->wp_hash, data->thumb_factory);

    if (item != NULL)
    {
      wp_props_load_wallpaper (item->filename, item, data);
    }
  }

  return item;
}

static void
wp_add_images (AppearanceData *data,
               GSList *images)
{
  GdkWindow *window;
  GtkWidget *w;
  GdkCursor *cursor;
  MateWPItem *item;

  w = appearance_capplet_get_widget (data, "appearance_window");
  window = gtk_widget_get_window (w);

  item = NULL;
  cursor = gdk_cursor_new_for_display (gdk_display_get_default (),
                                       GDK_WATCH);
  gdk_window_set_cursor (window, cursor);
  gdk_cursor_unref (cursor);

  while (images != NULL)
  {
    gchar *uri = images->data;

    item = wp_add_image (data, uri);
    images = g_slist_remove (images, uri);
    g_free (uri);
  }

  gdk_window_set_cursor (window, NULL);

  if (item != NULL)
  {
    select_item (data, item, TRUE);
  }
}

static void
wp_option_menu_set (AppearanceData *data,
                    int value,
                    gboolean shade_type)
{
  if (shade_type)
  {
    gtk_combo_box_set_active (GTK_COMBO_BOX (data->wp_color_menu),
                              value);

    if (value == MATE_BG_COLOR_SOLID)
      gtk_widget_hide (data->wp_scpicker);
    else
      gtk_widget_show (data->wp_scpicker);
  }
  else
  {
    gtk_combo_box_set_active (GTK_COMBO_BOX (data->wp_style_menu),
                              value);
  }
}

static void
wp_set_sensitivities (AppearanceData *data)
{
  MateWPItem *item;
  gchar *filename = NULL;

  item = get_selected_item (data, NULL);

  if (item != NULL)
    filename = item->filename;

  if (!mateconf_client_key_is_writable (data->client, WP_OPTIONS_KEY, NULL)
      || (filename && !strcmp (filename, "(none)")))
    gtk_widget_set_sensitive (data->wp_style_menu, FALSE);
  else
    gtk_widget_set_sensitive (data->wp_style_menu, TRUE);

  if (!mateconf_client_key_is_writable (data->client, WP_SHADING_KEY, NULL))
    gtk_widget_set_sensitive (data->wp_color_menu, FALSE);
  else
    gtk_widget_set_sensitive (data->wp_color_menu, TRUE);

  if (!mateconf_client_key_is_writable (data->client, WP_PCOLOR_KEY, NULL))
    gtk_widget_set_sensitive (data->wp_pcpicker, FALSE);
  else
    gtk_widget_set_sensitive (data->wp_pcpicker, TRUE);

  if (!mateconf_client_key_is_writable (data->client, WP_SCOLOR_KEY, NULL))
    gtk_widget_set_sensitive (data->wp_scpicker, FALSE);
  else
    gtk_widget_set_sensitive (data->wp_scpicker, TRUE);

  if (!filename || !strcmp (filename, "(none)"))
    gtk_widget_set_sensitive (data->wp_rem_button, FALSE);
  else
    gtk_widget_set_sensitive (data->wp_rem_button, TRUE);
}

static void
wp_scale_type_changed (GtkComboBox *combobox,
                       AppearanceData *data)
{
  MateWPItem *item;
  GtkTreeIter iter;
  GdkPixbuf *pixbuf;

  item = get_selected_item (data, &iter);

  if (item == NULL)
    return;

  item->options = gtk_combo_box_get_active (GTK_COMBO_BOX (data->wp_style_menu));

  pixbuf = mate_wp_item_get_thumbnail (item, data->thumb_factory,
                                        data->thumb_width,
                                        data->thumb_height);
  gtk_list_store_set (GTK_LIST_STORE (data->wp_model), &iter, 0, pixbuf, -1);
  if (pixbuf != NULL)
    g_object_unref (pixbuf);

  if (mateconf_client_key_is_writable (data->client, WP_OPTIONS_KEY, NULL))
    mateconf_client_set_string (data->client, WP_OPTIONS_KEY,
                             wp_item_option_to_string (item->options), NULL);
}

static void
wp_shade_type_changed (GtkWidget *combobox,
                       AppearanceData *data)
{
  MateWPItem *item;
  GtkTreeIter iter;
  GdkPixbuf *pixbuf;

  item = get_selected_item (data, &iter);

  if (item == NULL)
    return;

  item->shade_type = gtk_combo_box_get_active (GTK_COMBO_BOX (data->wp_color_menu));

  pixbuf = mate_wp_item_get_thumbnail (item, data->thumb_factory,
                                        data->thumb_width,
                                        data->thumb_height);
  gtk_list_store_set (GTK_LIST_STORE (data->wp_model), &iter, 0, pixbuf, -1);
  if (pixbuf != NULL)
    g_object_unref (pixbuf);

  if (mateconf_client_key_is_writable (data->client, WP_SHADING_KEY, NULL))
    mateconf_client_set_string (data->client, WP_SHADING_KEY,
                             wp_item_shading_to_string (item->shade_type), NULL);
}

static void
wp_color_changed (AppearanceData *data,
                  gboolean update)
{
  MateWPItem *item;

  item = get_selected_item (data, NULL);

  if (item == NULL)
    return;

  gtk_color_button_get_color (GTK_COLOR_BUTTON (data->wp_pcpicker), item->pcolor);
  gtk_color_button_get_color (GTK_COLOR_BUTTON (data->wp_scpicker), item->scolor);

  if (update)
  {
    gchar *pcolor, *scolor;

    pcolor = gdk_color_to_string (item->pcolor);
    scolor = gdk_color_to_string (item->scolor);
    mateconf_client_set_string (data->client, WP_PCOLOR_KEY, pcolor, NULL);
    mateconf_client_set_string (data->client, WP_SCOLOR_KEY, scolor, NULL);
    g_free (pcolor);
    g_free (scolor);
  }

  wp_shade_type_changed (NULL, data);
}

static void
wp_scolor_changed (GtkWidget *widget,
                   AppearanceData *data)
{
  wp_color_changed (data, TRUE);
}

static void
wp_remove_wallpaper (GtkWidget *widget,
                     AppearanceData *data)
{
  MateWPItem *item;
  GtkTreeIter iter;
  GtkTreePath *path;

  item = get_selected_item (data, &iter);

  if (item)
  {
    item->deleted = TRUE;

    if (gtk_list_store_remove (GTK_LIST_STORE (data->wp_model), &iter))
      path = gtk_tree_model_get_path (data->wp_model, &iter);
    else
      path = gtk_tree_path_new_first ();

    gtk_icon_view_select_path (data->wp_view, path);
    gtk_icon_view_set_cursor (data->wp_view, path, NULL, FALSE);
    gtk_tree_path_free (path);
  }
}

static void
wp_uri_changed (const gchar *uri,
                AppearanceData *data)
{
  MateWPItem *item, *selected;

  item = g_hash_table_lookup (data->wp_hash, uri);
  selected = get_selected_item (data, NULL);

  if (selected != NULL && strcmp (selected->filename, uri) != 0)
  {
    if (item == NULL)
      item = wp_add_image (data, uri);

    if (item)
      select_item (data, item, TRUE);
  }
}

static void
wp_file_changed (MateConfClient *client, guint id,
                 MateConfEntry *entry,
                 AppearanceData *data)
{
  const gchar *uri;
  gchar *wpfile;

  uri = mateconf_value_get_string (entry->value);

  if (g_utf8_validate (uri, -1, NULL) && g_file_test (uri, G_FILE_TEST_EXISTS))
    wpfile = g_strdup (uri);
  else
    wpfile = g_filename_from_utf8 (uri, -1, NULL, NULL, NULL);

  wp_uri_changed (wpfile, data);

  g_free (wpfile);
}

static void
wp_options_changed (MateConfClient *client, guint id,
                    MateConfEntry *entry,
                    AppearanceData *data)
{
  MateWPItem *item;
  const gchar *option;

  option = mateconf_value_get_string (entry->value);

  /* "none" means we don't use a background image */
  if (option == NULL || !strcmp (option, "none"))
  {
    /* temporarily disconnect so we don't override settings when
     * updating the selection */
    data->wp_update_mateconf = FALSE;
    wp_uri_changed ("(none)", data);
    data->wp_update_mateconf = TRUE;
    return;
  }

  item = get_selected_item (data, NULL);

  if (item != NULL)
  {
    item->options = wp_item_string_to_option (option);
    wp_option_menu_set (data, item->options, FALSE);
  }
}

static void
wp_shading_changed (MateConfClient *client, guint id,
                    MateConfEntry *entry,
                    AppearanceData *data)
{
  MateWPItem *item;

  wp_set_sensitivities (data);

  item = get_selected_item (data, NULL);

  if (item != NULL)
  {
    item->shade_type = wp_item_string_to_shading (mateconf_value_get_string (entry->value));
    wp_option_menu_set (data, item->shade_type, TRUE);
  }
}

static void
wp_color1_changed (MateConfClient *client, guint id,
                   MateConfEntry *entry,
                   AppearanceData *data)
{
  GdkColor color;
  const gchar *colorhex;

  colorhex = mateconf_value_get_string (entry->value);

  gdk_color_parse (colorhex, &color);

  gtk_color_button_set_color (GTK_COLOR_BUTTON (data->wp_pcpicker), &color);

  wp_color_changed (data, FALSE);
}

static void
wp_color2_changed (MateConfClient *client, guint id,
                   MateConfEntry *entry,
                   AppearanceData *data)
{
  GdkColor color;
  const gchar *colorhex;

  wp_set_sensitivities (data);

  colorhex = mateconf_value_get_string (entry->value);

  gdk_color_parse (colorhex, &color);

  gtk_color_button_set_color (GTK_COLOR_BUTTON (data->wp_scpicker), &color);

  wp_color_changed (data, FALSE);
}

static gboolean
wp_props_wp_set (AppearanceData *data, MateWPItem *item)
{
  MateConfChangeSet *cs;
  gchar *pcolor, *scolor;

  cs = mateconf_change_set_new ();

  if (!strcmp (item->filename, "(none)"))
  {
    mateconf_change_set_set_string (cs, WP_OPTIONS_KEY, "none");
    mateconf_change_set_set_string (cs, WP_FILE_KEY, "");
  }
  else
  {
    gchar *uri;

    if (g_utf8_validate (item->filename, -1, NULL))
      uri = g_strdup (item->filename);
    else
      uri = g_filename_to_utf8 (item->filename, -1, NULL, NULL, NULL);

    if (uri == NULL) {
      g_warning ("Failed to convert filename to UTF-8: %s", item->filename);
    } else {
      mateconf_change_set_set_string (cs, WP_FILE_KEY, uri);
      g_free (uri);
    }

    mateconf_change_set_set_string (cs, WP_OPTIONS_KEY,
                                 wp_item_option_to_string (item->options));
  }

  mateconf_change_set_set_string (cs, WP_SHADING_KEY,
                               wp_item_shading_to_string (item->shade_type));

  pcolor = gdk_color_to_string (item->pcolor);
  scolor = gdk_color_to_string (item->scolor);
  mateconf_change_set_set_string (cs, WP_PCOLOR_KEY, pcolor);
  mateconf_change_set_set_string (cs, WP_SCOLOR_KEY, scolor);
  g_free (pcolor);
  g_free (scolor);

  mateconf_client_commit_change_set (data->client, cs, TRUE, NULL);

  mateconf_change_set_unref (cs);

  return FALSE;
}

static void
wp_props_wp_selected (GtkTreeSelection *selection,
                      AppearanceData *data)
{
  MateWPItem *item;

  item = get_selected_item (data, NULL);

  if (item != NULL)
  {
    wp_set_sensitivities (data);

    if (strcmp (item->filename, "(none)") != 0)
      wp_option_menu_set (data, item->options, FALSE);

    wp_option_menu_set (data, item->shade_type, TRUE);

    gtk_color_button_set_color (GTK_COLOR_BUTTON (data->wp_pcpicker),
                                item->pcolor);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (data->wp_scpicker),
                                item->scolor);

    if (data->wp_update_mateconf)
      wp_props_wp_set (data, item);
  }
  else
  {
    gtk_widget_set_sensitive (data->wp_rem_button, FALSE);
  }
}

void
wp_create_filechooser (AppearanceData *data)
{
  const char *start_dir, *pictures = NULL;
  GtkFileFilter *filter;

  data->wp_filesel = GTK_FILE_CHOOSER (
                     gtk_file_chooser_dialog_new (_("Add Wallpaper"),
                     GTK_WINDOW (appearance_capplet_get_widget (data, "appearance_window")),
                     GTK_FILE_CHOOSER_ACTION_OPEN,
                     GTK_STOCK_CANCEL,
                     GTK_RESPONSE_CANCEL,
                     GTK_STOCK_OPEN,
                     GTK_RESPONSE_OK,
                     NULL));

  gtk_dialog_set_default_response (GTK_DIALOG (data->wp_filesel), GTK_RESPONSE_OK);
  gtk_file_chooser_set_select_multiple (data->wp_filesel, TRUE);
  gtk_file_chooser_set_use_preview_label (data->wp_filesel, FALSE);

  start_dir = g_get_home_dir ();

  if (g_file_test ("/usr/share/backgrounds", G_FILE_TEST_IS_DIR)) {
    gtk_file_chooser_add_shortcut_folder (data->wp_filesel,
                                          "/usr/share/backgrounds", NULL);
    start_dir = "/usr/share/backgrounds";
  }

  pictures = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  if (pictures != NULL && g_file_test (pictures, G_FILE_TEST_IS_DIR)) {
    gtk_file_chooser_add_shortcut_folder (data->wp_filesel, pictures, NULL);
    start_dir = pictures;
  }

  gtk_file_chooser_set_current_folder (data->wp_filesel, start_dir);

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_pixbuf_formats (filter);
  gtk_file_filter_set_name (filter, _("Images"));
  gtk_file_chooser_add_filter (data->wp_filesel, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (data->wp_filesel, filter);

  data->wp_image = gtk_image_new ();
  gtk_file_chooser_set_preview_widget (data->wp_filesel, data->wp_image);
  gtk_widget_set_size_request (data->wp_image, 128, -1);

  gtk_widget_show (data->wp_image);

  g_signal_connect (data->wp_filesel, "update-preview",
                    (GCallback) wp_update_preview, data);
}

static void
wp_file_open_dialog (GtkWidget *widget,
                     AppearanceData *data)
{
  GSList *files;

  if (!data->wp_filesel)
    wp_create_filechooser (data);

  switch (gtk_dialog_run (GTK_DIALOG (data->wp_filesel)))
  {
  case GTK_RESPONSE_OK:
    files = gtk_file_chooser_get_filenames (data->wp_filesel);
    wp_add_images (data, files);
  case GTK_RESPONSE_CANCEL:
  default:
    gtk_widget_hide (GTK_WIDGET (data->wp_filesel));
    break;
  }
}

static void
wp_drag_received (GtkWidget *widget,
                  GdkDragContext *context,
                  gint x, gint y,
                  GtkSelectionData *selection_data,
                  guint info, guint time,
                  AppearanceData *data)
{
  if (info == TARGET_URI_LIST || info == TARGET_BGIMAGE)
  {
    GSList *realuris = NULL;
    gchar **uris;

    uris = g_uri_list_extract_uris ((gchar *) gtk_selection_data_get_data (selection_data));
    if (uris != NULL)
    {
      GtkWidget *w;
      GdkWindow *window;
      GdkCursor *cursor;
      gchar **uri;

      w = appearance_capplet_get_widget (data, "appearance_window");
      window = gtk_widget_get_window (w);

      cursor = gdk_cursor_new_for_display (gdk_display_get_default (),
             GDK_WATCH);
      gdk_window_set_cursor (window, cursor);
      gdk_cursor_unref (cursor);

      for (uri = uris; *uri; ++uri)
      {
        GFile *f;

        f = g_file_new_for_uri (*uri);
        realuris = g_slist_append (realuris, g_file_get_path (f));
        g_object_unref (f);
      }

      wp_add_images (data, realuris);
      gdk_window_set_cursor (window, NULL);

      g_strfreev (uris);
    }
  }
}

static void
wp_drag_get_data (GtkWidget *widget,
		  GdkDragContext *context,
		  GtkSelectionData *selection_data,
		  guint type, guint time,
		  AppearanceData *data)
{
  if (type == TARGET_URI_LIST) {
    MateWPItem *item = get_selected_item (data, NULL);

    if (item != NULL) {
      char *uris[2];

      uris[0] = g_filename_to_uri (item->filename, NULL, NULL);
      uris[1] = NULL;

      gtk_selection_data_set_uris (selection_data, uris);

      g_free (uris[0]);
    }
  }
}

static gboolean
wp_view_tooltip_cb (GtkWidget  *widget,
                    gint x,
                    gint y,
                    gboolean keyboard_mode,
                    GtkTooltip *tooltip,
                    AppearanceData *data)
{
  GtkTreeIter iter;
  MateWPItem *item;

  if (gtk_icon_view_get_tooltip_context (data->wp_view,
                                         &x, &y,
                                         keyboard_mode,
                                         NULL,
                                         NULL,
                                         &iter))
    {
      gtk_tree_model_get (data->wp_model, &iter, 1, &item, -1);
      gtk_tooltip_set_markup (tooltip, item->description);

      return TRUE;
    }

  return FALSE;
}

static gint
wp_list_sort (GtkTreeModel *model,
              GtkTreeIter *a, GtkTreeIter *b,
              AppearanceData *data)
{
  MateWPItem *itema, *itemb;
  gint retval;

  gtk_tree_model_get (model, a, 1, &itema, -1);
  gtk_tree_model_get (model, b, 1, &itemb, -1);

  if (!strcmp (itema->filename, "(none)"))
  {
    retval =  -1;
  }
  else if (!strcmp (itemb->filename, "(none)"))
  {
    retval =  1;
  }
  else
  {
    retval = g_utf8_collate (itema->description, itemb->description);
  }

  return retval;
}

static void
wp_update_preview (GtkFileChooser *chooser,
                   AppearanceData *data)
{
  gchar *uri;

  uri = gtk_file_chooser_get_preview_uri (chooser);

  if (uri)
  {
    GdkPixbuf *pixbuf = NULL;
    const gchar *mime_type = NULL;
    GFile *file;
    GFileInfo *file_info;

    file = g_file_new_for_uri (uri);
    file_info = g_file_query_info (file,
                                   G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                   G_FILE_QUERY_INFO_NONE,
                                   NULL, NULL);
    g_object_unref (file);

    if (file_info != NULL)
    {
      mime_type = g_file_info_get_content_type (file_info);
      g_object_unref (file_info);
    }

    if (mime_type)
    {
      pixbuf = mate_desktop_thumbnail_factory_generate_thumbnail (data->thumb_factory,
								   uri,
								   mime_type);
    }

    if (pixbuf != NULL)
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (data->wp_image), pixbuf);
      g_object_unref (pixbuf);
    }
    else
    {
      gtk_image_set_from_stock (GTK_IMAGE (data->wp_image),
                                "gtk-dialog-question",
                                GTK_ICON_SIZE_DIALOG);
    }
  }

  gtk_file_chooser_set_preview_widget_active (chooser, TRUE);
}

static gboolean
reload_item (GtkTreeModel *model,
             GtkTreePath *path,
             GtkTreeIter *iter,
             AppearanceData *data)
{
  MateWPItem *item;
  GdkPixbuf *pixbuf;

  gtk_tree_model_get (model, iter, 1, &item, -1);

  pixbuf = mate_wp_item_get_thumbnail (item,
                                        data->thumb_factory,
                                        data->thumb_width,
                                        data->thumb_height);
  if (pixbuf) {
    gtk_list_store_set (GTK_LIST_STORE (data->wp_model), iter, 0, pixbuf, -1);
    g_object_unref (pixbuf);
  }

  return FALSE;
}

static gdouble
get_monitor_aspect_ratio_for_widget (GtkWidget *widget)
{
  gdouble aspect;
  gint monitor;
  GdkRectangle rect;

  monitor = gdk_screen_get_monitor_at_window (gtk_widget_get_screen (widget), gtk_widget_get_window (widget));
  gdk_screen_get_monitor_geometry (gtk_widget_get_screen (widget), monitor, &rect);
  aspect = rect.height / (gdouble)rect.width;

  return aspect;
}

#define LIST_IMAGE_SIZE 108

static void
compute_thumbnail_sizes (AppearanceData *data)
{
  gdouble aspect;

  aspect = get_monitor_aspect_ratio_for_widget (GTK_WIDGET (data->wp_view));
  if (aspect > 1) {
    /* portrait */
    data->thumb_width = LIST_IMAGE_SIZE / aspect;
    data->thumb_height = LIST_IMAGE_SIZE;
  } else {
    data->thumb_width = LIST_IMAGE_SIZE;
    data->thumb_height = LIST_IMAGE_SIZE * aspect;
  }
}

static void
reload_wallpapers (AppearanceData *data)
{
  compute_thumbnail_sizes (data);
  gtk_tree_model_foreach (data->wp_model, (GtkTreeModelForeachFunc)reload_item, data);
}

static gboolean
wp_load_stuffs (void *user_data)
{
  AppearanceData *data;
  gchar *imagepath, *uri, *style;
  MateWPItem *item;

  data = (AppearanceData *) user_data;

  compute_thumbnail_sizes (data);

  mate_wp_xml_load_list (data);
  g_hash_table_foreach (data->wp_hash, (GHFunc) wp_props_load_wallpaper,
                        data);

  style = mateconf_client_get_string (data->client,
                                   WP_OPTIONS_KEY,
                                   NULL);
  if (style == NULL)
    style = g_strdup ("none");

  uri = mateconf_client_get_string (data->client,
                                 WP_FILE_KEY,
                                 NULL);

  if (uri && *uri == '\0')
  {
    g_free (uri);
    uri = NULL;
  }

  if (uri == NULL)
    uri = g_strdup ("(none)");

  if (g_utf8_validate (uri, -1, NULL) && g_file_test (uri, G_FILE_TEST_EXISTS))
    imagepath = g_strdup (uri);
  else
    imagepath = g_filename_from_utf8 (uri, -1, NULL, NULL, NULL);

  g_free (uri);

  item = g_hash_table_lookup (data->wp_hash, imagepath);

  if (item != NULL)
  {
    /* update with the current mateconf settings */
    mate_wp_item_update (item);

    if (strcmp (style, "none") != 0)
    {
      if (item->deleted == TRUE)
      {
        item->deleted = FALSE;
        wp_props_load_wallpaper (item->filename, item, data);
      }

      select_item (data, item, FALSE);
    }
  }
  else if (strcmp (style, "none") != 0)
  {
    item = wp_add_image (data, imagepath);
    if (item)
      select_item (data, item, FALSE);
  }

  item = g_hash_table_lookup (data->wp_hash, "(none)");
  if (item == NULL)
  {
    item = mate_wp_item_new ("(none)", data->wp_hash, data->thumb_factory);
    if (item != NULL)
    {
      wp_props_load_wallpaper (item->filename, item, data);
    }
  }
  else
  {
    if (item->deleted == TRUE)
    {
      item->deleted = FALSE;
      wp_props_load_wallpaper (item->filename, item, data);
    }

    if (!strcmp (style, "none"))
    {
      select_item (data, item, FALSE);
      wp_option_menu_set (data, MATE_BG_PLACEMENT_SCALED, FALSE);
    }
  }
  g_free (imagepath);
  g_free (style);

  if (data->wp_uris) {
    wp_add_images (data, data->wp_uris);
    data->wp_uris = NULL;
  }

  return FALSE;
}

static void
wp_select_after_realize (GtkWidget *widget,
                         AppearanceData *data)
{
  MateWPItem *item;

  g_idle_add (wp_load_stuffs, data);

  item = get_selected_item (data, NULL);
  if (item == NULL)
    item = g_hash_table_lookup (data->wp_hash, "(none)");

  select_item (data, item, TRUE);
}

static GdkPixbuf *buttons[3];

static void
create_button_images (AppearanceData  *data)
{
  GtkWidget *widget = (GtkWidget*)data->wp_view;
  GtkStyle *style = gtk_widget_get_style (widget);
  GtkIconSet *icon_set;
  GdkPixbuf *pixbuf, *pb, *pb2;
  gint i, w, h;

  icon_set = gtk_style_lookup_icon_set (style, "gtk-media-play");
  pb = gtk_icon_set_render_icon (icon_set,
                                 style,
                                 GTK_TEXT_DIR_RTL,
                                 GTK_STATE_NORMAL,
                                 GTK_ICON_SIZE_MENU,
                                 widget,
                                 NULL);
  pb2 = gtk_icon_set_render_icon (icon_set,
                                  style,
                                  GTK_TEXT_DIR_LTR,
                                  GTK_STATE_NORMAL,
                                  GTK_ICON_SIZE_MENU,
                                  widget,
                                  NULL);
  w = gdk_pixbuf_get_width (pb);
  h = gdk_pixbuf_get_height (pb);

  for (i = 0; i < 3; i++) {
    pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 2 * w, h);
    gdk_pixbuf_fill (pixbuf, 0);
    if (i > 0)
      gdk_pixbuf_composite (pb, pixbuf, 0, 0, w, h, 0, 0, 1, 1, GDK_INTERP_NEAREST, 255);
    if (i < 2)
      gdk_pixbuf_composite (pb2, pixbuf, w, 0, w, h, w, 0, 1, 1, GDK_INTERP_NEAREST, 255);

    buttons[i] = pixbuf;
  }

  g_object_unref (pb);
  g_object_unref (pb2);
}

static void
next_frame (AppearanceData  *data,
            GtkCellRenderer *cr,
            gint             direction)
{
  MateWPItem *item;
  GtkTreeIter iter;
  GdkPixbuf *pixbuf, *pb;
  gint frame;

  pixbuf = NULL;

  frame = data->frame + direction;
  item = get_selected_item (data, &iter);

  if (frame >= 0)
    pixbuf = mate_wp_item_get_frame_thumbnail (item,
                                                data->thumb_factory,
                                                data->thumb_width,
                                                data->thumb_height,
                                                frame);
  if (pixbuf) {
    gtk_list_store_set (GTK_LIST_STORE (data->wp_model), &iter, 0, pixbuf, -1);
    g_object_unref (pixbuf);
    data->frame = frame;
  }

  pb = buttons[1];
  if (direction < 0) {
    if (frame == 0)
      pb = buttons[0];
  }
  else {
    pixbuf = mate_wp_item_get_frame_thumbnail (item,
                                                data->thumb_factory,
                                                data->thumb_width,
                                                data->thumb_height,
                                                frame + 1);
    if (pixbuf)
      g_object_unref (pixbuf);
    else
      pb = buttons[2];
  }
  g_object_set (cr, "pixbuf", pb, NULL);
}

static gboolean
wp_button_press_cb (GtkWidget      *widget,
                    GdkEventButton *event,
                    AppearanceData *data)
{
  GtkCellRenderer *cell;
  GdkEventButton *button_event = (GdkEventButton *) event;

  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  if (gtk_icon_view_get_item_at_pos (GTK_ICON_VIEW (widget),
                                     button_event->x, button_event->y,
                                     NULL, &cell)) {
    if (g_object_get_data (G_OBJECT (cell), "buttons")) {
      gint w, h;
      GtkCellRenderer *cell2 = NULL;
      gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
      if (gtk_icon_view_get_item_at_pos (GTK_ICON_VIEW (widget),
                                         button_event->x + w, button_event->y,
                                         NULL, &cell2) && cell == cell2)
        next_frame (data, cell, -1);
      else
        next_frame (data, cell, 1);
      return TRUE;
    }
  }

  return FALSE;
}

static void
wp_selected_changed_cb (GtkIconView    *view,
                        AppearanceData *data)
{
  GtkCellRenderer *cr;
  GList *cells, *l;

  data->frame = -1;

  cells = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (data->wp_view));
  for (l = cells; l; l = l->next) {
    cr = l->data;
    if (g_object_get_data (G_OBJECT (cr), "buttons"))
      g_object_set (cr, "pixbuf", buttons[0], NULL);
  }
  g_list_free (cells);
}

static void
buttons_cell_data_func (GtkCellLayout   *layout,
                        GtkCellRenderer *cell,
                        GtkTreeModel    *model,
                        GtkTreeIter     *iter,
                        gpointer         user_data)
{
  AppearanceData *data = user_data;
  GtkTreePath *path;
  MateWPItem *item;
  gboolean visible;

  path = gtk_tree_model_get_path (model, iter);

  if (gtk_icon_view_path_is_selected (GTK_ICON_VIEW (layout), path)) {
    item = get_selected_item (data, NULL);
    visible = mate_bg_changes_with_time (item->bg);
  }
  else
    visible = FALSE;

  g_object_set (G_OBJECT (cell), "visible", visible, NULL);

  gtk_tree_path_free (path);
}

static void
screen_monitors_changed (GdkScreen *screen,
                         AppearanceData *data)
{
  reload_wallpapers (data);
}

void
desktop_init (AppearanceData *data,
	      const gchar **uris)
{
  GtkWidget *add_button, *w;
  GtkCellRenderer *cr;
  char *url;

  data->wp_update_mateconf = TRUE;

  data->wp_uris = NULL;
  if (uris != NULL) {
    while (*uris != NULL) {
      data->wp_uris = g_slist_append (data->wp_uris, g_strdup (*uris));
      uris++;
    }
  }

  w = appearance_capplet_get_widget (data, "more_backgrounds_linkbutton");
  url = mateconf_client_get_string (data->client, MORE_BACKGROUNDS_URL_KEY, NULL);
  if (url != NULL && url[0] != '\0') {
    gtk_link_button_set_uri (GTK_LINK_BUTTON (w), url);
    gtk_widget_show (w);
  } else {
    gtk_widget_hide (w);
  }
  g_free (url);

  data->wp_hash = g_hash_table_new (g_str_hash, g_str_equal);

  mateconf_client_add_dir (data->client, WP_PATH_KEY,
      MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);

  mateconf_client_notify_add (data->client,
                           WP_FILE_KEY,
                           (MateConfClientNotifyFunc) wp_file_changed,
                           data, NULL, NULL);
  mateconf_client_notify_add (data->client,
                           WP_OPTIONS_KEY,
                           (MateConfClientNotifyFunc) wp_options_changed,
                           data, NULL, NULL);
  mateconf_client_notify_add (data->client,
                           WP_SHADING_KEY,
                           (MateConfClientNotifyFunc) wp_shading_changed,
                           data, NULL, NULL);
  mateconf_client_notify_add (data->client,
                           WP_PCOLOR_KEY,
                           (MateConfClientNotifyFunc) wp_color1_changed,
                           data, NULL, NULL);
  mateconf_client_notify_add (data->client,
                           WP_SCOLOR_KEY,
                           (MateConfClientNotifyFunc) wp_color2_changed,
                           data, NULL, NULL);

  data->wp_model = GTK_TREE_MODEL (gtk_list_store_new (2, GDK_TYPE_PIXBUF,
                                                       G_TYPE_POINTER));

  data->wp_view = GTK_ICON_VIEW (appearance_capplet_get_widget (data, "wp_view"));
  gtk_icon_view_set_model (data->wp_view, GTK_TREE_MODEL (data->wp_model));

  g_signal_connect_after (data->wp_view, "realize",
                          (GCallback) wp_select_after_realize, data);

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (data->wp_view));

  cr = gtk_cell_renderer_pixbuf_new ();
  g_object_set (cr, "xpad", 5, "ypad", 5, NULL);

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->wp_view), cr, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (data->wp_view), cr,
                                  "pixbuf", 0,
                                  NULL);

  cr = gtk_cell_renderer_pixbuf_new ();
  create_button_images (data);
  g_object_set (cr,
                "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
                "pixbuf", buttons[0],
                NULL);
  g_object_set_data (G_OBJECT (cr), "buttons", GINT_TO_POINTER (TRUE));

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->wp_view), cr, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (data->wp_view), cr,
                                      buttons_cell_data_func, data, NULL);
  g_signal_connect (data->wp_view, "selection-changed",
                    (GCallback) wp_selected_changed_cb, data);
  g_signal_connect (data->wp_view, "button-press-event",
                    G_CALLBACK (wp_button_press_cb), data);

  data->frame = -1;

  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (data->wp_model), 1,
                                   (GtkTreeIterCompareFunc) wp_list_sort,
                                   data, NULL);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->wp_model),
                                        1, GTK_SORT_ASCENDING);

  gtk_drag_dest_set (GTK_WIDGET (data->wp_view), GTK_DEST_DEFAULT_ALL, drop_types,
                     G_N_ELEMENTS (drop_types), GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect (data->wp_view, "drag_data_received",
                    (GCallback) wp_drag_received, data);

  gtk_drag_source_set (GTK_WIDGET (data->wp_view), GDK_BUTTON1_MASK,
                       drag_types, G_N_ELEMENTS (drag_types), GDK_ACTION_COPY);
  g_signal_connect (data->wp_view, "drag-data-get",
		    (GCallback) wp_drag_get_data, data);

  data->wp_style_menu = appearance_capplet_get_widget (data, "wp_style_menu");

  g_signal_connect (data->wp_style_menu, "changed",
                    (GCallback) wp_scale_type_changed, data);

  data->wp_color_menu = appearance_capplet_get_widget (data, "wp_color_menu");

  g_signal_connect (data->wp_color_menu, "changed",
                    (GCallback) wp_shade_type_changed, data);

  data->wp_scpicker = appearance_capplet_get_widget (data, "wp_scpicker");

  g_signal_connect (data->wp_scpicker, "color-set",
                    (GCallback) wp_scolor_changed, data);

  data->wp_pcpicker = appearance_capplet_get_widget (data, "wp_pcpicker");

  g_signal_connect (data->wp_pcpicker, "color-set",
                    (GCallback) wp_scolor_changed, data);

  add_button = appearance_capplet_get_widget (data, "wp_add_button");
  gtk_button_set_image (GTK_BUTTON (add_button),
                        gtk_image_new_from_stock ("gtk-add", GTK_ICON_SIZE_BUTTON));

  g_signal_connect (add_button, "clicked",
                    (GCallback) wp_file_open_dialog, data);

  data->wp_rem_button = appearance_capplet_get_widget (data, "wp_rem_button");

  g_signal_connect (data->wp_rem_button, "clicked",
                    (GCallback) wp_remove_wallpaper, data);
  data->screen_monitors_handler = g_signal_connect (gtk_widget_get_screen (GTK_WIDGET (data->wp_view)),
                                                    "monitors-changed",
                                                    G_CALLBACK (screen_monitors_changed),
                                                    data);
  data->screen_size_handler = g_signal_connect (gtk_widget_get_screen (GTK_WIDGET (data->wp_view)),
                                                    "size-changed",
                                                    G_CALLBACK (screen_monitors_changed),
                                                    data);

  g_signal_connect (data->wp_view, "selection-changed",
                    (GCallback) wp_props_wp_selected, data);
  g_signal_connect (data->wp_view, "query-tooltip",
                    (GCallback) wp_view_tooltip_cb, data);
  gtk_widget_set_has_tooltip (GTK_WIDGET (data->wp_view), TRUE);

  wp_set_sensitivities (data);

  /* create the file selector later to save time on startup */
  data->wp_filesel = NULL;

}

void
desktop_shutdown (AppearanceData *data)
{
  mate_wp_xml_save_list (data);

  if (data->screen_monitors_handler > 0) {
    g_signal_handler_disconnect (gtk_widget_get_screen (GTK_WIDGET (data->wp_view)),
                                 data->screen_monitors_handler);
    data->screen_monitors_handler = 0;
  }
  if (data->screen_size_handler > 0) {
    g_signal_handler_disconnect (gtk_widget_get_screen (GTK_WIDGET (data->wp_view)),
                                 data->screen_size_handler);
    data->screen_size_handler = 0;
  }

  g_slist_foreach (data->wp_uris, (GFunc) g_free, NULL);
  g_slist_free (data->wp_uris);
  if (data->wp_filesel)
  {
    g_object_ref_sink (data->wp_filesel);
    g_object_unref (data->wp_filesel);
  }
}
