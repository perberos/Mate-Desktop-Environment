#include <config.h>
#include <unistd.h>
#include <string.h>
#include <marco-private/util.h>
#include <marco-private/theme.h>
#include <marco-private/theme-parser.h>
#include <marco-private/preview-widget.h>
#include <signal.h>
#include <errno.h>
#include <math.h>

/* We have to #undef this as marco #defines these. */
#undef _
#undef N_

#include <glib.h>

#include "theme-thumbnail.h"
#include "gtkrc-utils.h"
#include "capplet-util.h"

typedef struct {
	gboolean set;
	gint thumbnail_width;
	gint thumbnail_height;
	GByteArray* data;
	gchar* theme_name;
	ThemeThumbnailFunc func;
	gpointer user_data;
	GDestroyNotify destroy;
	GIOChannel* channel;
	guint watch_id;
} ThemeThumbnailAsyncData;


static ThemeThumbnailAsyncData async_data;

/* Protocol */

/* Our protocol is pretty simple.  The parent process will write several strings
 * (separated by a '\000'). They are the widget theme, the wm theme, the icon
 * theme, etc.  Then, it will wait for the child to write back the data.  The
 * parent expects ICON_SIZE_WIDTH * ICON_SIZE_HEIGHT * 4 bytes of information.
 * After that, the child is ready for the next theme to render.
 */

enum {
	READY_FOR_THEME,
	READING_TYPE,
	READING_CONTROL_THEME_NAME,
	READING_GTK_COLOR_SCHEME,
	READING_WM_THEME_NAME,
	READING_ICON_THEME_NAME,
	READING_APPLICATION_FONT,
	WRITING_PIXBUF_DATA
};

typedef struct {
	gint status;
	GByteArray* type;
	GByteArray* control_theme_name;
	GByteArray* gtk_color_scheme;
	GByteArray* wm_theme_name;
	GByteArray* icon_theme_name;
	GByteArray* application_font;
} ThemeThumbnailData;

typedef struct {
	gchar* thumbnail_type;
	gpointer theme_info;
	ThemeThumbnailFunc func;
	gpointer user_data;
	GDestroyNotify destroy;
} ThemeQueueItem;

static GList* theme_queue = NULL;

static int pipe_to_factory_fd[2];
static int pipe_from_factory_fd[2];

#define THUMBNAIL_TYPE_META     "meta"
#define THUMBNAIL_TYPE_GTK      "gtk"
#define THUMBNAIL_TYPE_MARCO    "marco"
#define THUMBNAIL_TYPE_ICON     "icon"

#define META_THUMBNAIL_SIZE       128
#define GTK_THUMBNAIL_SIZE         96
#define MARCO_THUMBNAIL_WIDTH  120
#define MARCO_THUMBNAIL_HEIGHT  60

/* This draw the thumbnail of gtk
 */
static GdkPixmap* draw_window_on_pixbuf(GtkWidget* widget)
{
	GdkVisual* visual;
	GdkPixmap* pixmap;
	GtkStyle* style;
	GdkScreen* screen = gdk_screen_get_default();
	GdkWindow* window;
	gint width, height;

	gtk_widget_ensure_style(widget);

	style = gtk_widget_get_style(widget);

	g_assert(style);
	g_assert(style->font_desc);

	gtk_window_get_size(GTK_WINDOW(widget), &width, &height);

	visual = gtk_widget_get_visual(widget);
	pixmap = gdk_pixmap_new(NULL, width, height, visual->depth);
	gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), gtk_widget_get_colormap(widget));

	window = gtk_widget_get_window(widget);

	/* This is a hack for the default resize grip on Ubuntu.
	 * Once again, thank you Ubuntu.
	 *
	 * We need to add a --enable-ubuntu for this.
	 */
	#ifdef UBUNTU
		gtk_window_set_has_resize_grip(GTK_WINDOW(widget), FALSE);
	#endif

	gdk_window_redirect_to_drawable(window, pixmap, 0, 0, 0, 0, width, height);
	gdk_window_set_override_redirect(window, TRUE);
	gtk_window_move(GTK_WINDOW(widget), gdk_screen_get_width(screen), gdk_screen_get_height(screen));
	gtk_widget_show(widget);

	gdk_window_process_updates(window, TRUE);
	gtk_widget_hide(widget);

	return pixmap;
}

static void pixbuf_apply_mask_region(GdkPixbuf* pixbuf, GdkRegion* region)
{
  gint nchannels, rowstride, w, h;
  guchar *pixels, *p;

  g_return_if_fail (pixbuf);
  g_return_if_fail (region);

  nchannels = gdk_pixbuf_get_n_channels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);


  /* we need an alpha channel ... */
  if (!gdk_pixbuf_get_has_alpha (pixbuf) || nchannels != 4)
    return;

  for (w = 0; w < gdk_pixbuf_get_width (pixbuf); ++w)
    for (h = 0; h < gdk_pixbuf_get_height (pixbuf); ++h)
    {
      if (!gdk_region_point_in (region, w, h))
      {
        p = pixels + h * rowstride + w * nchannels;
        if (G_BYTE_ORDER == G_BIG_ENDIAN)
          p[0] = 0x0;
        else
          p[3] = 0x0;
      }
    }

}

static GdkPixbuf *
create_folder_icon (char *icon_theme_name)
{
  GtkIconTheme *icon_theme;
  GdkPixbuf *folder_icon = NULL;
  GtkIconInfo *folder_icon_info;
  gchar *example_icon_name;
  const gchar *icon_names[5];
  gint i;

  icon_theme = gtk_icon_theme_new ();
  gtk_icon_theme_set_custom_theme (icon_theme, icon_theme_name);

  i = 0;
  /* Get the Example icon name in the theme if specified */
  example_icon_name = gtk_icon_theme_get_example_icon_name (icon_theme);
  if (example_icon_name != NULL)
    icon_names[i++] = example_icon_name;
  icon_names[i++] = "x-directory-normal";
  icon_names[i++] = "mate-fs-directory";
  icon_names[i++] = "folder";
  icon_names[i++] = NULL;

  folder_icon_info = gtk_icon_theme_choose_icon (icon_theme, icon_names, 48, GTK_ICON_LOOKUP_FORCE_SIZE);
  if (folder_icon_info != NULL)
  {
    folder_icon = gtk_icon_info_load_icon (folder_icon_info, NULL);
    gtk_icon_info_free (folder_icon_info);
  }

  g_object_unref (icon_theme);
  g_free (example_icon_name);

  /* render the icon to the thumbnail */
  if (folder_icon == NULL)
  {
    GtkWidget *dummy;
    dummy = gtk_label_new ("");

    folder_icon = gtk_widget_render_icon (dummy,
                                          GTK_STOCK_MISSING_IMAGE,
                                          GTK_ICON_SIZE_DIALOG,
                                          NULL);

    gtk_widget_destroy (dummy);
  }

  return folder_icon;
}

static GdkPixbuf *
create_meta_theme_pixbuf (ThemeThumbnailData *theme_thumbnail_data)
{
  GtkWidget *window;
  GtkWidget *preview;
  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *box;
  GtkWidget *stock_button;
  GtkWidget *checkbox;
  GtkWidget *radio;

  GtkRequisition requisition;
  GtkAllocation allocation;
  GtkAllocation vbox_allocation;
  GdkPixmap *pixmap;
  MetaFrameFlags flags;
  MetaTheme *theme;
  GdkPixbuf *pixbuf, *icon;
  int icon_width, icon_height;
  GdkRegion *region;

  g_object_set (gtk_settings_get_default (),
    "gtk-theme-name", (char *) theme_thumbnail_data->control_theme_name->data,
    "gtk-font-name", (char *) theme_thumbnail_data->application_font->data,
    "gtk-icon-theme-name", (char *) theme_thumbnail_data->icon_theme_name->data,
    "gtk-color-scheme", (char *) theme_thumbnail_data->gtk_color_scheme->data,
    NULL);

  theme = meta_theme_load ((char *) theme_thumbnail_data->wm_theme_name->data, NULL);
  if (theme == NULL)
    return NULL;

  /* Represent the icon theme */
  icon = create_folder_icon ((char *) theme_thumbnail_data->icon_theme_name->data);
  icon_width = gdk_pixbuf_get_width (icon);
  icon_height = gdk_pixbuf_get_height (icon);

  /* Create a fake window */
  flags = META_FRAME_ALLOWS_DELETE |
          META_FRAME_ALLOWS_MENU |
          META_FRAME_ALLOWS_MINIMIZE |
          META_FRAME_ALLOWS_MAXIMIZE |
          META_FRAME_ALLOWS_VERTICAL_RESIZE |
          META_FRAME_ALLOWS_HORIZONTAL_RESIZE |
          META_FRAME_HAS_FOCUS |
          META_FRAME_ALLOWS_SHADE |
          META_FRAME_ALLOWS_MOVE;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  preview = meta_preview_new ();
  gtk_container_add (GTK_CONTAINER (window), preview);
  gtk_widget_realize (window);
  gtk_widget_realize (preview);
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (preview), vbox);
  align = gtk_alignment_new (0, 0, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  stock_button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  gtk_container_add (GTK_CONTAINER (align), stock_button);
  box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  checkbox = gtk_check_button_new ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), TRUE);
  gtk_box_pack_start (GTK_BOX (box), checkbox, FALSE, FALSE, 0);
  radio = gtk_radio_button_new (NULL);
  gtk_box_pack_start (GTK_BOX (box), radio, FALSE, FALSE, 0);

  gtk_widget_show_all (preview);

  meta_preview_set_frame_flags (META_PREVIEW (preview), flags);
  meta_preview_set_theme (META_PREVIEW (preview), theme);
  meta_preview_set_title (META_PREVIEW (preview), "");

  gtk_window_set_default_size (GTK_WINDOW (window), META_THUMBNAIL_SIZE, META_THUMBNAIL_SIZE);

  gtk_widget_size_request (window, &requisition);
  allocation.x = 0;
  allocation.y = 0;
  allocation.width = META_THUMBNAIL_SIZE;
  allocation.height = META_THUMBNAIL_SIZE;
  gtk_widget_size_allocate (window, &allocation);
  gtk_widget_size_request (window, &requisition);

  pixmap = draw_window_on_pixbuf (window);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, META_THUMBNAIL_SIZE, META_THUMBNAIL_SIZE);
  gdk_pixbuf_get_from_drawable (pixbuf, pixmap, NULL, 0, 0, 0, 0, META_THUMBNAIL_SIZE, META_THUMBNAIL_SIZE);

  gtk_widget_get_allocation (vbox, &vbox_allocation);

  /* Add the icon theme to the pixbuf */
  gdk_pixbuf_composite (icon, pixbuf,
                        vbox_allocation.x + vbox_allocation.width - icon_width - 5,
                        vbox_allocation.y + vbox_allocation.height - icon_height - 5,
                        icon_width, icon_height,
                        vbox_allocation.x + vbox_allocation.width - icon_width - 5,
                        vbox_allocation.y + vbox_allocation.height - icon_height - 5,
                        1.0, 1.0, GDK_INTERP_BILINEAR, 255);
  region = meta_preview_get_clip_region (META_PREVIEW (preview),
      META_THUMBNAIL_SIZE, META_THUMBNAIL_SIZE);
  pixbuf_apply_mask_region (pixbuf, region);
  gdk_region_destroy (region);

  g_object_unref (icon);
  gtk_widget_destroy (window);
  meta_theme_free (theme);
  g_object_unref (pixmap);

  return pixbuf;
}

static GdkPixbuf *
create_gtk_theme_pixbuf (ThemeThumbnailData *theme_thumbnail_data)
{
  GtkSettings *settings;
  GtkWidget *window, *vbox, *box, *stock_button, *checkbox, *radio;
  GtkRequisition requisition;
  GtkAllocation allocation;
  GdkPixmap *pixmap;
  GdkPixbuf *pixbuf, *retval;
  gint width, height;

  settings = gtk_settings_get_default ();
  g_object_set (settings, "gtk-theme-name", (char *) theme_thumbnail_data->control_theme_name->data,
			  "gtk-color-scheme", (char *) theme_thumbnail_data->gtk_color_scheme->data,
 			  NULL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  box = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  stock_button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  gtk_box_pack_start (GTK_BOX (box), stock_button, FALSE, FALSE, 0);
  checkbox = gtk_check_button_new ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox), TRUE);
  gtk_box_pack_start (GTK_BOX (box), checkbox, FALSE, FALSE, 0);
  radio = gtk_radio_button_new_from_widget (NULL);
  gtk_box_pack_start (GTK_BOX (box), radio, FALSE, FALSE, 0);

  gtk_widget_show_all (vbox);
  gtk_widget_realize (stock_button);
  gtk_widget_realize (gtk_bin_get_child (GTK_BIN (stock_button)));
  gtk_widget_realize (checkbox);
  gtk_widget_realize (radio);
  gtk_widget_map (stock_button);
  gtk_widget_map (gtk_bin_get_child (GTK_BIN (stock_button)));
  gtk_widget_map (checkbox);
  gtk_widget_map (radio);

  gtk_widget_size_request (window, &requisition);
  allocation.x = 0;
  allocation.y = 0;
  allocation.width = requisition.width;
  allocation.height = requisition.height;
  gtk_widget_size_allocate (window, &allocation);
  gtk_widget_size_request (window, &requisition);

  gtk_window_get_size (GTK_WINDOW (window), &width, &height);

  pixmap = draw_window_on_pixbuf (window);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  gdk_pixbuf_get_from_drawable (pixbuf, pixmap, NULL, 0, 0, 0, 0, width, height);

  retval = gdk_pixbuf_scale_simple (pixbuf,
                                    GTK_THUMBNAIL_SIZE,
                                    (int) GTK_THUMBNAIL_SIZE * (((double) height) / ((double) width)),
                                    GDK_INTERP_BILINEAR);
  g_object_unref (pixbuf);
  gtk_widget_destroy (window);
  g_object_unref (pixmap);

  return retval;
}

static GdkPixbuf *
create_marco_theme_pixbuf (ThemeThumbnailData *theme_thumbnail_data)
{
  GtkWidget *window, *preview, *dummy;
  MetaFrameFlags flags;
  MetaTheme *theme;
  GtkRequisition requisition;
  GtkAllocation allocation;
  GdkPixmap *pixmap;
  GdkPixbuf *pixbuf, *retval;
  GdkRegion *region;

  theme = meta_theme_load ((char *) theme_thumbnail_data->wm_theme_name->data, NULL);
  if (theme == NULL)
    return NULL;

  flags = META_FRAME_ALLOWS_DELETE |
          META_FRAME_ALLOWS_MENU |
          META_FRAME_ALLOWS_MINIMIZE |
          META_FRAME_ALLOWS_MAXIMIZE |
          META_FRAME_ALLOWS_VERTICAL_RESIZE |
          META_FRAME_ALLOWS_HORIZONTAL_RESIZE |
          META_FRAME_HAS_FOCUS |
          META_FRAME_ALLOWS_SHADE |
          META_FRAME_ALLOWS_MOVE;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), (int) MARCO_THUMBNAIL_WIDTH * 1.2, (int) MARCO_THUMBNAIL_HEIGHT * 1.2);

  preview = meta_preview_new ();
  meta_preview_set_frame_flags (META_PREVIEW (preview), flags);
  meta_preview_set_theme (META_PREVIEW (preview), theme);
  meta_preview_set_title (META_PREVIEW (preview), "");
  gtk_container_add (GTK_CONTAINER (window), preview);

  dummy = gtk_label_new ("");
  gtk_container_add (GTK_CONTAINER (preview), dummy);

  gtk_widget_realize (window);
  gtk_widget_realize (preview);
  gtk_widget_realize (dummy);
  gtk_widget_show_all (preview);
  gtk_widget_map (dummy);

  gtk_widget_size_request (window, &requisition);
  allocation.x = 0;
  allocation.y = 0;
  allocation.width = (int) MARCO_THUMBNAIL_WIDTH * 1.2;
  allocation.height = (int) MARCO_THUMBNAIL_HEIGHT * 1.2;
  gtk_widget_size_allocate (window, &allocation);
  gtk_widget_size_request (window, &requisition);

  pixmap = draw_window_on_pixbuf (window);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, (int) MARCO_THUMBNAIL_WIDTH * 1.2, (int) MARCO_THUMBNAIL_HEIGHT * 1.2);
  gdk_pixbuf_get_from_drawable (pixbuf, pixmap, NULL, 0, 0, 0, 0, (int) MARCO_THUMBNAIL_WIDTH * 1.2, (int) MARCO_THUMBNAIL_HEIGHT * 1.2);

  region = meta_preview_get_clip_region (META_PREVIEW (preview),
      MARCO_THUMBNAIL_WIDTH * 1.2, MARCO_THUMBNAIL_HEIGHT * 1.2);
  pixbuf_apply_mask_region (pixbuf, region);
  gdk_region_destroy (region);


  retval = gdk_pixbuf_scale_simple (pixbuf,
                                    MARCO_THUMBNAIL_WIDTH,
                                    MARCO_THUMBNAIL_HEIGHT,
                                    GDK_INTERP_BILINEAR);
  g_object_unref (pixbuf);

  gtk_widget_destroy (window);
  meta_theme_free (theme);
  g_object_unref (pixmap);

  return retval;
}

static GdkPixbuf *
create_icon_theme_pixbuf (ThemeThumbnailData *theme_thumbnail_data)
{
  return create_folder_icon ((char *) theme_thumbnail_data->icon_theme_name->data);
}


static void
handle_bytes (const gchar        *buffer,
              gint                bytes_read,
              ThemeThumbnailData *theme_thumbnail_data)
{
  const gchar *ptr;
  ptr = buffer;

  while (bytes_read > 0)
  {
    char *nil;

    switch (theme_thumbnail_data->status)
    {
      case READY_FOR_THEME:
        theme_thumbnail_data->status = READING_TYPE;
        /* fall through */
      case READING_TYPE:
        nil = memchr (ptr, '\000', bytes_read);
        if (nil == NULL)
        {
          g_byte_array_append (theme_thumbnail_data->type, ptr, bytes_read);
          bytes_read = 0;
        }
        else
        {
          g_byte_array_append (theme_thumbnail_data->type, ptr, nil - ptr + 1);
          bytes_read -= (nil - ptr + 1);
          ptr = nil + 1;
          theme_thumbnail_data->status = READING_CONTROL_THEME_NAME;
        }
        break;

      case READING_CONTROL_THEME_NAME:
        nil = memchr (ptr, '\000', bytes_read);
        if (nil == NULL)
        {
          g_byte_array_append (theme_thumbnail_data->control_theme_name, ptr, bytes_read);
          bytes_read = 0;
        }
        else
        {
          g_byte_array_append (theme_thumbnail_data->control_theme_name, ptr, nil - ptr + 1);
          bytes_read -= (nil - ptr + 1);
          ptr = nil + 1;
          theme_thumbnail_data->status = READING_GTK_COLOR_SCHEME;
        }
        break;

      case READING_GTK_COLOR_SCHEME:
        nil = memchr (ptr, '\000', bytes_read);
        if (nil == NULL)
        {
          g_byte_array_append (theme_thumbnail_data->gtk_color_scheme, ptr, bytes_read);
          bytes_read = 0;
        }
        else
        {
          g_byte_array_append (theme_thumbnail_data->gtk_color_scheme, ptr, nil - ptr + 1);
          bytes_read -= (nil - ptr + 1);
          ptr = nil + 1;
          theme_thumbnail_data->status = READING_WM_THEME_NAME;
        }
        break;

      case READING_WM_THEME_NAME:
        nil = memchr (ptr, '\000', bytes_read);
        if (nil == NULL)
        {
          g_byte_array_append (theme_thumbnail_data->wm_theme_name, ptr, bytes_read);
          bytes_read = 0;
        }
        else
        {
          g_byte_array_append (theme_thumbnail_data->wm_theme_name, ptr, nil - ptr + 1);
          bytes_read -= (nil - ptr + 1);
          ptr = nil + 1;
          theme_thumbnail_data->status = READING_ICON_THEME_NAME;
        }
        break;

      case READING_ICON_THEME_NAME:
        nil = memchr (ptr, '\000', bytes_read);
        if (nil == NULL)
        {
          g_byte_array_append (theme_thumbnail_data->icon_theme_name, ptr, bytes_read);
          bytes_read = 0;
        }
        else
        {
          g_byte_array_append (theme_thumbnail_data->icon_theme_name, ptr, nil - ptr + 1);
          bytes_read -= (nil - ptr + 1);
          ptr = nil + 1;
          theme_thumbnail_data->status = READING_APPLICATION_FONT;
        }
        break;

      case READING_APPLICATION_FONT:
        nil = memchr (ptr, '\000', bytes_read);
        if (nil == NULL)
        {
          g_byte_array_append (theme_thumbnail_data->application_font, ptr, bytes_read);
          bytes_read = 0;
        }
        else
        {
          g_byte_array_append (theme_thumbnail_data->application_font, ptr, nil - ptr + 1);
          bytes_read -= (nil - ptr + 1);
          ptr = nil + 1;
          theme_thumbnail_data->status = WRITING_PIXBUF_DATA;
        }
        break;

      default:
        g_assert_not_reached ();
    }
  }
}

static gboolean
message_from_capplet (GIOChannel   *source,
                      GIOCondition  condition,
                      gpointer      data)
{
  gchar buffer[1024];
  GIOStatus status;
  gsize bytes_read;
  ThemeThumbnailData *theme_thumbnail_data;

  theme_thumbnail_data = (ThemeThumbnailData *) data;
  status = g_io_channel_read_chars (source,
                                    buffer,
                                    1024,
                                    &bytes_read,
                                    NULL);

  switch (status)
  {
    case G_IO_STATUS_NORMAL:
      handle_bytes (buffer, bytes_read, theme_thumbnail_data);

      if (theme_thumbnail_data->status == WRITING_PIXBUF_DATA)
      {
        GdkPixbuf *pixbuf = NULL;
        gint i, rowstride;
        guchar *pixels;
        gint width, height;
        const gchar *type = (const gchar *) theme_thumbnail_data->type->data;

        if (!strcmp (type, THUMBNAIL_TYPE_META))
          pixbuf = create_meta_theme_pixbuf (theme_thumbnail_data);
        else if (!strcmp (type, THUMBNAIL_TYPE_GTK))
          pixbuf = create_gtk_theme_pixbuf (theme_thumbnail_data);
        else if (!strcmp (type, THUMBNAIL_TYPE_MARCO))
          pixbuf = create_marco_theme_pixbuf (theme_thumbnail_data);
        else if (!strcmp (type, THUMBNAIL_TYPE_ICON))
          pixbuf = create_icon_theme_pixbuf (theme_thumbnail_data);
        else
          g_assert_not_reached ();

        if (pixbuf == NULL) {
          width = height = rowstride = 0;
          pixels = NULL;
        } else {
          width = gdk_pixbuf_get_width (pixbuf);
          height = gdk_pixbuf_get_height (pixbuf);
          rowstride = gdk_pixbuf_get_rowstride (pixbuf);
          pixels = gdk_pixbuf_get_pixels (pixbuf);
        }

        /* Write the pixbuf's size */
        write (pipe_from_factory_fd[1], &width, sizeof (width));
        write (pipe_from_factory_fd[1], &height, sizeof (height));

        for (i = 0; i < height; i++)
        {
          write (pipe_from_factory_fd[1], pixels + rowstride * i, width * gdk_pixbuf_get_n_channels (pixbuf));
        }

        if (pixbuf)
          g_object_unref (pixbuf);
        g_byte_array_set_size (theme_thumbnail_data->type, 0);
        g_byte_array_set_size (theme_thumbnail_data->control_theme_name, 0);
        g_byte_array_set_size (theme_thumbnail_data->gtk_color_scheme, 0);
        g_byte_array_set_size (theme_thumbnail_data->wm_theme_name, 0);
        g_byte_array_set_size (theme_thumbnail_data->icon_theme_name, 0);
        g_byte_array_set_size (theme_thumbnail_data->application_font, 0);
        theme_thumbnail_data->status = READY_FOR_THEME;
      }
      return TRUE;

    case G_IO_STATUS_AGAIN:
      return TRUE;

    case G_IO_STATUS_EOF:
    case G_IO_STATUS_ERROR:
      _exit (0);

    default:
      g_assert_not_reached ();
    }

  return TRUE;
}

static void
generate_next_in_queue (void)
{
  ThemeQueueItem *item;

  if (theme_queue == NULL)
    return;

  item = theme_queue->data;
  theme_queue = g_list_delete_link (theme_queue, g_list_first (theme_queue));

  if (!strcmp (item->thumbnail_type, THUMBNAIL_TYPE_META))
    generate_meta_theme_thumbnail_async ((MateThemeMetaInfo *) item->theme_info,
                                         item->func,
                                         item->user_data,
                                         item->destroy);
  else if (!strcmp (item->thumbnail_type, THUMBNAIL_TYPE_GTK))
    generate_gtk_theme_thumbnail_async ((MateThemeInfo *) item->theme_info,
                                        item->func,
                                        item->user_data,
                                        item->destroy);
  else if (!strcmp (item->thumbnail_type, THUMBNAIL_TYPE_MARCO))
    generate_marco_theme_thumbnail_async ((MateThemeInfo *) item->theme_info,
                                             item->func,
                                             item->user_data,
                                             item->destroy);
  else if (!strcmp (item->thumbnail_type, THUMBNAIL_TYPE_ICON))
    generate_icon_theme_thumbnail_async ((MateThemeIconInfo *) item->theme_info,
                                         item->func,
                                         item->user_data,
                                         item->destroy);

  g_free (item);
}

static gboolean
message_from_child (GIOChannel   *source,
                    GIOCondition  condition,
                    gpointer      data)
{
  gchar buffer[1024];
  GIOStatus status;
  gsize bytes_read;

  if (async_data.set == FALSE)
    return TRUE;

  if (condition == G_IO_HUP)
    return FALSE;

  status = g_io_channel_read_chars (source,
                                    buffer,
                                    1024,
                                    &bytes_read,
                                    NULL);
  switch (status)
  {
    case G_IO_STATUS_NORMAL:
      g_byte_array_append (async_data.data, (guchar *) buffer, bytes_read);

      if (async_data.thumbnail_width == -1 && async_data.data->len >= 2 * sizeof (gint))
      {
        async_data.thumbnail_width = *((gint *) async_data.data->data);
        async_data.thumbnail_height = *(((gint *) async_data.data->data) + 1);
        g_byte_array_remove_range (async_data.data, 0, 2 * sizeof (gint));
      }

      if (async_data.thumbnail_width >= 0 && async_data.data->len == async_data.thumbnail_width * async_data.thumbnail_height * 4)
      {
        GdkPixbuf *pixbuf = NULL;

        if (async_data.thumbnail_width > 0) {
          gchar *pixels;
          gint i, rowstride;

          pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, async_data.thumbnail_width, async_data.thumbnail_height);
          pixels = (gchar *) gdk_pixbuf_get_pixels (pixbuf);
          rowstride = gdk_pixbuf_get_rowstride (pixbuf);

          for (i = 0; i < async_data.thumbnail_height; ++i)
            memcpy (pixels + rowstride * i, async_data.data->data + 4 * async_data.thumbnail_width * i, async_data.thumbnail_width * 4);
        }

        /* callback function needs to ref the pixbuf if it wants to keep it */
        (* async_data.func) (pixbuf, async_data.theme_name, async_data.user_data);

        if (async_data.destroy)
          (* async_data.destroy) (async_data.user_data);

        if (pixbuf)
          g_object_unref (pixbuf);

        /* Clean up async_data */
        g_free (async_data.theme_name);
        g_source_remove (async_data.watch_id);
        g_io_channel_unref (async_data.channel);

        /* reset async_data */
        async_data.thumbnail_width = -1;
        async_data.thumbnail_height = -1;
        async_data.theme_name = NULL;
        async_data.channel = NULL;
        async_data.func = NULL;
        async_data.user_data = NULL;
        async_data.destroy = NULL;
        async_data.set = FALSE;
        g_byte_array_set_size (async_data.data, 0);

        generate_next_in_queue ();
      }
      return TRUE;

    case G_IO_STATUS_AGAIN:
      return TRUE;

    case G_IO_STATUS_EOF:
    case G_IO_STATUS_ERROR:
      return FALSE;

    default:
      g_assert_not_reached ();
  }

  return TRUE;
}

static void
send_thumbnail_request (gchar *thumbnail_type,
                        gchar *gtk_theme_name,
                        gchar *gtk_color_scheme,
                        gchar *marco_theme_name,
                        gchar *icon_theme_name,
                        gchar *application_font)
{
  write (pipe_to_factory_fd[1], thumbnail_type, strlen (thumbnail_type) + 1);

  if (gtk_theme_name)
    write (pipe_to_factory_fd[1], gtk_theme_name, strlen (gtk_theme_name) + 1);
  else
    write (pipe_to_factory_fd[1], "", 1);

  if (gtk_color_scheme)
    write (pipe_to_factory_fd[1], gtk_color_scheme, strlen (gtk_color_scheme) + 1);
  else
    write (pipe_to_factory_fd[1], "", 1);

  if (marco_theme_name)
    write (pipe_to_factory_fd[1], marco_theme_name, strlen (marco_theme_name) + 1);
  else
    write (pipe_to_factory_fd[1], "", 1);

  if (icon_theme_name)
    write (pipe_to_factory_fd[1], icon_theme_name, strlen (icon_theme_name) + 1);
  else
    write (pipe_to_factory_fd[1], "", 1);

  if (application_font)
    write (pipe_to_factory_fd[1], application_font, strlen (application_font) + 1);
  else
     write (pipe_to_factory_fd[1], "Sans 10", strlen ("Sans 10") + 1);

}

static GdkPixbuf *
read_pixbuf (void)
{
  gint bytes_read, i, j = 0;
  gint size[2];
  GdkPixbuf *pixbuf;
  gint rowstride;
  guchar *pixels;

  do
  {
    bytes_read = read (pipe_from_factory_fd[0], ((guint8*) size) + j, 2 * sizeof (gint));
    if (bytes_read == 0)
      goto eof;
    j += bytes_read;
  }
  while (j < 2 * sizeof (gint));

  if (size[0] <= 0 || size[1] <= 0)
    return NULL;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size[0], size[1]);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  for (i = 0; i < size[1]; i++)
  {
    j = 0;

    do
    {
      bytes_read = read (pipe_from_factory_fd[0], pixels + rowstride * i + j, size[0] * gdk_pixbuf_get_n_channels (pixbuf) - j);

      if (bytes_read > 0)
        j += bytes_read;
      else if (bytes_read == 0)
      {
        g_object_unref (pixbuf);
        goto eof;
      }
    }
    while (j < size[0] * gdk_pixbuf_get_n_channels (pixbuf));
  }

  return pixbuf;

eof:
  g_warning ("Received EOF while reading thumbnail");
  close (pipe_to_factory_fd[1]);
  pipe_to_factory_fd[1] = 0;
  close (pipe_from_factory_fd[0]);
  pipe_from_factory_fd[0] = 0;
  return NULL;
}

static GdkPixbuf *
generate_theme_thumbnail (gchar *thumbnail_type,
                          gchar *gtk_theme_name,
                          gchar *gtk_color_scheme,
                          gchar *marco_theme_name,
                          gchar *icon_theme_name,
                          gchar *application_font)
{
  if (async_data.set || !pipe_to_factory_fd[1] || !pipe_from_factory_fd[0])
    return NULL;

  send_thumbnail_request (thumbnail_type,
                          gtk_theme_name,
                          gtk_color_scheme,
                          marco_theme_name,
                          icon_theme_name,
                          application_font);

  return read_pixbuf ();
}

GdkPixbuf *
generate_meta_theme_thumbnail (MateThemeMetaInfo *theme_info)
{
  return generate_theme_thumbnail (THUMBNAIL_TYPE_META,
                                   theme_info->gtk_theme_name,
                                   theme_info->gtk_color_scheme,
                                   theme_info->marco_theme_name,
                                   theme_info->icon_theme_name,
                                   theme_info->application_font);
}

GdkPixbuf *
generate_gtk_theme_thumbnail (MateThemeInfo *theme_info)
{
  gchar *scheme;

  scheme = gtkrc_get_color_scheme_for_theme (theme_info->name);

  return generate_theme_thumbnail (THUMBNAIL_TYPE_GTK,
                                   theme_info->name,
                                   scheme,
                                   NULL,
                                   NULL,
                                   NULL);
  g_free (scheme);
}

GdkPixbuf *
generate_marco_theme_thumbnail (MateThemeInfo *theme_info)
{
  return generate_theme_thumbnail (THUMBNAIL_TYPE_MARCO,
                                   NULL,
                                   NULL,
                                   theme_info->name,
                                   NULL,
                                   NULL);
}

GdkPixbuf *
generate_icon_theme_thumbnail (MateThemeIconInfo *theme_info)
{
  return generate_theme_thumbnail (THUMBNAIL_TYPE_ICON,
                                   NULL,
                                   NULL,
                                   NULL,
                                   theme_info->name,
                                   NULL);
}

static void generate_theme_thumbnail_async(gpointer theme_info, gchar* theme_name, gchar* thumbnail_type, gchar* gtk_theme_name, gchar* gtk_color_scheme, gchar* marco_theme_name, gchar* icon_theme_name, gchar* application_font, ThemeThumbnailFunc func, gpointer user_data, GDestroyNotify destroy)
{
	if (async_data.set)
	{
		ThemeQueueItem* item = g_new0 (ThemeQueueItem, 1);

		item->thumbnail_type = thumbnail_type;
		item->theme_info = theme_info;
		item->func = func;
		item->user_data = user_data;
		item->destroy = destroy;

		theme_queue = g_list_append(theme_queue, item);

		return;
	}

	if (!pipe_to_factory_fd[1] || !pipe_from_factory_fd[0])
	{
		(*func)(NULL, theme_name, user_data);

		if (destroy)
		{
			(*destroy)(user_data);
		}

		return;
	}

	if (async_data.channel == NULL)
	{
		async_data.channel = g_io_channel_unix_new(pipe_from_factory_fd[0]);

		g_io_channel_set_flags(async_data.channel, g_io_channel_get_flags (async_data.channel) | G_IO_FLAG_NONBLOCK, NULL);
		g_io_channel_set_encoding(async_data.channel, NULL, NULL);

		async_data.watch_id = g_io_add_watch(async_data.channel, G_IO_IN | G_IO_HUP, message_from_child, NULL);
	}

	async_data.set = TRUE;
	async_data.thumbnail_width = -1;
	async_data.thumbnail_height = -1;
	async_data.theme_name = g_strdup(theme_name);
	async_data.func = func;
	async_data.user_data = user_data;
	async_data.destroy = destroy;

	send_thumbnail_request(thumbnail_type, gtk_theme_name, gtk_color_scheme, marco_theme_name, icon_theme_name, application_font);
}

void
generate_meta_theme_thumbnail_async (MateThemeMetaInfo *theme_info,
                                     ThemeThumbnailFunc  func,
                                     gpointer            user_data,
                                     GDestroyNotify      destroy)
{
  generate_theme_thumbnail_async (theme_info,
                                         theme_info->name,
                                         THUMBNAIL_TYPE_META,
                                         theme_info->gtk_theme_name,
                                         theme_info->gtk_color_scheme,
                                         theme_info->marco_theme_name,
                                         theme_info->icon_theme_name,
                                         theme_info->application_font,
                                         func, user_data, destroy);
}

void generate_gtk_theme_thumbnail_async (MateThemeInfo* theme_info, ThemeThumbnailFunc  func, gpointer user_data, GDestroyNotify destroy)
{
	gchar* scheme = gtkrc_get_color_scheme_for_theme(theme_info->name);

	generate_theme_thumbnail_async(theme_info, theme_info->name, THUMBNAIL_TYPE_GTK, theme_info->name, scheme,  NULL, NULL, NULL, func, user_data, destroy);

	g_free(scheme);
}

void
generate_marco_theme_thumbnail_async (MateThemeInfo *theme_info,
                                         ThemeThumbnailFunc  func,
                                         gpointer            user_data,
                                         GDestroyNotify      destroy)
{
  generate_theme_thumbnail_async (theme_info,
                                         theme_info->name,
                                         THUMBNAIL_TYPE_MARCO,
                                         NULL,
                                         NULL,
                                         theme_info->name,
                                         NULL,
                                         NULL,
                                         func, user_data, destroy);
}

void
generate_icon_theme_thumbnail_async (MateThemeIconInfo *theme_info,
                                     ThemeThumbnailFunc  func,
                                     gpointer            user_data,
                                     GDestroyNotify      destroy)
{
  generate_theme_thumbnail_async (theme_info,
                                         theme_info->name,
                                         THUMBNAIL_TYPE_ICON,
                                         NULL,
                                         NULL,
                                         NULL,
                                         theme_info->name,
                                         NULL,
                                         func, user_data, destroy);
}

void
theme_thumbnail_factory_init (int argc, char *argv[])
{
#ifndef __APPLE__
  gint child_pid;
#endif

  pipe (pipe_to_factory_fd);
  pipe (pipe_from_factory_fd);

/* Apple's CoreFoundation classes must not be used from forked
 * processes. Since freetype (and thus GTK) uses them, we simply
 * disable the thumbnailer on MacOS for now. That means no thumbs
 * until the thumbnailing process is rewritten, but at least we won't
 * make apps crash. */
#ifndef __APPLE__
  child_pid = fork ();
  if (child_pid == 0)
  {
    ThemeThumbnailData data;
    GIOChannel *channel;

    /* Child */
    gtk_init (&argc, &argv);

    close (pipe_to_factory_fd[1]);
    pipe_to_factory_fd[1] = 0;
    close (pipe_from_factory_fd[0]);
    pipe_from_factory_fd[0] = 0;

    data.status = READY_FOR_THEME;
    data.type = g_byte_array_new ();
    data.control_theme_name = g_byte_array_new ();
    data.gtk_color_scheme = g_byte_array_new ();
    data.wm_theme_name = g_byte_array_new ();
    data.icon_theme_name = g_byte_array_new ();
    data.application_font = g_byte_array_new ();

    channel = g_io_channel_unix_new (pipe_to_factory_fd[0]);
    g_io_channel_set_flags (channel, g_io_channel_get_flags (channel) |
          G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_encoding (channel, NULL, NULL);
    g_io_add_watch (channel, G_IO_IN | G_IO_HUP, message_from_capplet, &data);
    g_io_channel_unref (channel);

    gtk_main ();
    _exit (0);
  }

  g_assert (child_pid > 0);

  /* Parent */
  close (pipe_to_factory_fd[0]);
  close (pipe_from_factory_fd[1]);
#endif /* __APPLE__ */

  async_data.set = FALSE;
  async_data.theme_name = NULL;
  async_data.data = g_byte_array_new ();
}
