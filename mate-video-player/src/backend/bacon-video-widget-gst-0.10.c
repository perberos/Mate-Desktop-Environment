/* 
 * Copyright (C) 2003-2007 the GStreamer project
 *      Julien Moutte <julien@moutte.net>
 *      Ronald Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2005-2008 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 * Copyright © 2009 Christian Persch
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission is above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

/**
 * SECTION:bacon-video-widget
 * @short_description: video playing widget and abstraction
 * @stability: Unstable
 * @include: bacon-video-widget.h
 *
 * #BaconVideoWidget is a widget to play audio or video streams, with support for visualisations for audio-only streams. It has a GStreamer
 * backend, and abstracts away the differences to provide a simple interface to the functionality required by Totem. It handles all the low-level
 * audio and video work for Totem (or passes the work off to the backend).
 **/

#include <config.h>

#include <gst/gst.h>

/* GStreamer Interfaces */
#include <gst/interfaces/xoverlay.h>
#include <gst/interfaces/navigation.h>
#include <gst/interfaces/colorbalance.h>
/* for detecting sources of errors */
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>
#include <gst/audio/gstbaseaudiosink.h>
/* for pretty multichannel strings */
#include <gst/audio/multichannel.h>
/* for the volume property */
#include <gst/interfaces/streamvolume.h>

/* for missing decoder/demuxer detection */
#include <gst/pbutils/pbutils.h>

/* for the cover metadata info */
#include <gst/tag/tag.h>

/* system */
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* gtk+/mate */
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <mateconf/mateconf-client.h>

#include "bacon-video-widget.h"
#include "bacon-video-widget-gst-missing-plugins.h"
#include "baconvideowidget-marshal.h"
#include "video-utils.h"
#include "gstscreenshot.h"
#include "bacon-resize.h"

#define FORWARD_RATE 1.0
#define REVERSE_RATE -1.0
#define DEFAULT_HEIGHT 420
#define DEFAULT_WIDTH  315
#define SMALL_STREAM_WIDTH 200
#define SMALL_STREAM_HEIGHT 120
/* Maximum size of the logo */
#define LOGO_SIZE 256
#define NANOSECS_IN_SEC 1000000000
#define SEEK_TIMEOUT NANOSECS_IN_SEC / 10

#define is_error(e, d, c) \
  (e->domain == GST_##d##_ERROR && \
   e->code == GST_##d##_ERROR_##c)

#define I_(string) (g_intern_static_string (string))

G_DEFINE_TYPE (BaconVideoWidget, bacon_video_widget, GTK_TYPE_EVENT_BOX)

/* Signals */
enum
{
  SIGNAL_ERROR,
  SIGNAL_EOS,
  SIGNAL_REDIRECT,
  SIGNAL_CHANNELS_CHANGE,
  SIGNAL_TICK,
  SIGNAL_GOT_METADATA,
  SIGNAL_BUFFERING,
  SIGNAL_MISSING_PLUGINS,
  SIGNAL_DOWNLOAD_BUFFERING,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_LOGO_MODE,
  PROP_POSITION,
  PROP_CURRENT_TIME,
  PROP_STREAM_LENGTH,
  PROP_PLAYING,
  PROP_REFERRER,
  PROP_SEEKABLE,
  PROP_SHOW_CURSOR,
  PROP_SHOW_VISUALS,
  PROP_USER_AGENT,
  PROP_VOLUME,
  PROP_DOWNLOAD_FILENAME
};

static const gchar *video_props_str[4] = {
  MATECONF_PREFIX "/brightness",
  MATECONF_PREFIX "/contrast",
  MATECONF_PREFIX "/saturation",
  MATECONF_PREFIX "/hue"
};

/* GstPlayFlags flags from playbin2 */
typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9)
} GstPlayFlags;

struct BaconVideoWidgetPrivate
{
  char                        *user_agent;

  char                        *referrer;
  char                        *mrl;
  BvwAspectRatio               ratio_type;

  GstElement                  *play;
  GstElement                  *source;
  GstXOverlay                 *xoverlay;      /* protect with lock */
  GstColorBalance             *balance;       /* protect with lock */
  GstNavigation               *navigation;    /* protect with lock */
  guint                        interface_update_id;  /* protect with lock */
  GMutex                      *lock;

  guint                        update_id;
  guint                        fill_id;
  guint                        ready_idle_id;

  GdkPixbuf                   *logo_pixbuf;
  GdkPixbuf                   *cover_pixbuf; /* stream-specific image */

  gboolean                     media_has_video;
  gboolean                     media_has_audio;
  gint                         seekable; /* -1 = don't know, FALSE = no */
  gint64                       stream_length;
  gint64                       current_time;
  gdouble                      current_position;
  gboolean                     is_live;

  GstTagList                  *tagcache;
  GstTagList                  *audiotags;
  GstTagList                  *videotags;

  GAsyncQueue                 *tag_update_queue;
  guint                        tag_update_id;

  gboolean                     got_redirect;

  GdkWindow                   *video_window;
  GdkCursor                   *cursor;

  /* Visual effects */
  GList                       *vis_plugins_list;
  gboolean                     show_vfx;
  gboolean                     vis_changed;
  BvwVisualsQuality            visq;
  gchar                       *vis_element_name;
  GstElement                  *audio_capsfilter;

  /* Other stuff */
  gboolean                     logo_mode;
  gboolean                     cursor_shown;
  gboolean                     fullscreen_mode;
  gboolean                     auto_resize;
  gboolean                     uses_audio_fakesink;
  gdouble                      volume;
  gboolean                     is_menu;
  
  gint                         video_width; /* Movie width */
  gint                         video_height; /* Movie height */
  gboolean                     window_resized; /* Whether the window has already been resized
						  for this media */
  gint                         movie_par_n; /* Movie pixel aspect ratio numerator */
  gint                         movie_par_d; /* Movie pixel aspect ratio denominator */
  gint                         video_width_pixels; /* Scaled movie width */
  gint                         video_height_pixels; /* Scaled movie height */
  gint                         video_fps_n;
  gint                         video_fps_d;

  gdouble                      zoom;
  
  gchar                       *media_device;

  BvwAudioOutType	       speakersetup;
  gint                         connection_speed;

  GstMessageType               ignore_messages_mask;

  MateConfClient                 *gc;

  GstBus                      *bus;
  gulong                       sig_bus_sync;
  gulong                       sig_bus_async;

  BvwUseType                   use_type;

  gint                         eos_id;

  /* When seeking, queue up the seeks if they happen before
   * the previous one finished */
  GMutex                      *seek_mutex;
  GstClock                    *clock;
  GstClockTime                 seek_req_time;
  gint64                       seek_time;
  /* state we want to be in, as opposed to actual pipeline state
   * which may change asynchronously or during buffering */
  GstState                     target_state;
  gboolean                     buffering;
  gboolean                     download_buffering;
  GstElement                  *download_buffering_element;
  char                        *download_filename;
  /* used to compute when the download buffer has gone far
   * enough to start playback */
  gint64                       buffering_left;

  /* for easy codec installation */
  GList                       *missing_plugins;   /* GList of GstMessages */
  gboolean                     plugin_install_in_progress;

  /* for mounting locations if necessary */
  GCancellable                *mount_cancellable;
  gboolean                     mount_in_progress;

  /* for auth */
  GMountOperation             *auth_dialog;
  GMountOperationResult        auth_last_result;
  char                        *user_id, *user_pw;

  /* for stepping */
  float                        rate;

  /* Bacon resize */
  BaconResize                 *bacon_resize;
};

static void bacon_video_widget_set_property (GObject * object,
                                             guint property_id,
                                             const GValue * value,
                                             GParamSpec * pspec);
static void bacon_video_widget_get_property (GObject * object,
                                             guint property_id,
                                             GValue * value,
                                             GParamSpec * pspec);

static void bacon_video_widget_finalize (GObject * object);

static void bvw_update_interface_implementations (BaconVideoWidget *bvw);
static void setup_vis (BaconVideoWidget * bvw);
static GList * get_visualization_features (void);
static gboolean bacon_video_widget_configure_event (GtkWidget *widget,
    GdkEventConfigure *event, BaconVideoWidget *bvw);
static void size_changed_cb (GdkScreen *screen, BaconVideoWidget *bvw);
static void bvw_process_pending_tag_messages (BaconVideoWidget * bvw);
static void bvw_stop_play_pipeline (BaconVideoWidget * bvw);
static GError* bvw_error_from_gst_error (BaconVideoWidget *bvw, GstMessage *m);
static gboolean bvw_check_for_cover_pixbuf (BaconVideoWidget * bvw);
static const GdkPixbuf * bvw_get_logo_pixbuf (BaconVideoWidget * bvw);
static gboolean bvw_set_playback_direction (BaconVideoWidget *bvw, gboolean forward);
static gboolean bacon_video_widget_seek_time_no_lock (BaconVideoWidget *bvw, gint64 _time, GstSeekFlags flag, GError **error);

typedef struct {
  GstTagList *tags;
  const gchar *type;
} UpdateTagsDelayedData;

static void update_tags_delayed_data_destroy (UpdateTagsDelayedData *data);

static GtkWidgetClass *parent_class = NULL;

static int bvw_signals[LAST_SIGNAL] = { 0 };

static GThread *gui_thread;

GST_DEBUG_CATEGORY (_totem_gst_debug_cat);
#define GST_CAT_DEFAULT _totem_gst_debug_cat

typedef gchar * (* MsgToStrFunc) (GstMessage * msg);

static gchar **
bvw_get_missing_plugins_foo (const GList * missing_plugins, MsgToStrFunc func)
{
  GPtrArray *arr = g_ptr_array_new ();

  while (missing_plugins != NULL) {
    g_ptr_array_add (arr, func (GST_MESSAGE (missing_plugins->data)));
    missing_plugins = missing_plugins->next;
  }
  g_ptr_array_add (arr, NULL);
  return (gchar **) g_ptr_array_free (arr, FALSE);
}

static gchar **
bvw_get_missing_plugins_details (const GList * missing_plugins)
{
  return bvw_get_missing_plugins_foo (missing_plugins,
      gst_missing_plugin_message_get_installer_detail);
}

static gchar **
bvw_get_missing_plugins_descriptions (const GList * missing_plugins)
{
  return bvw_get_missing_plugins_foo (missing_plugins,
      gst_missing_plugin_message_get_description);
}

static void
bvw_clear_missing_plugins_messages (BaconVideoWidget * bvw)
{
  g_list_foreach (bvw->priv->missing_plugins,
                  (GFunc) gst_mini_object_unref, NULL);
  g_list_free (bvw->priv->missing_plugins);
  bvw->priv->missing_plugins = NULL;
}

static void
bvw_check_if_video_decoder_is_missing (BaconVideoWidget * bvw)
{
  GList *l;

  if (bvw->priv->media_has_video || bvw->priv->missing_plugins == NULL)
    return;

  for (l = bvw->priv->missing_plugins; l != NULL; l = l->next) {
    GstMessage *msg = GST_MESSAGE (l->data);
    gchar *d, *f;

    if ((d = gst_missing_plugin_message_get_installer_detail (msg))) {
      if ((f = strstr (d, "|decoder-")) && strstr (f, "video")) {
        GError *err;

        /* create a fake GStreamer error so we get a nice warning message */
        err = g_error_new (GST_CORE_ERROR, GST_CORE_ERROR_MISSING_PLUGIN, "x");
        msg = gst_message_new_error (GST_OBJECT (bvw->priv->play), err, NULL);
        g_error_free (err);
        err = bvw_error_from_gst_error (bvw, msg);
        gst_message_unref (msg);
        g_signal_emit (bvw, bvw_signals[SIGNAL_ERROR], 0, err->message, FALSE, FALSE);
        g_error_free (err);
        g_free (d);
        break;
      }
      g_free (d);
    }
  }
}

static void
bvw_error_msg (BaconVideoWidget * bvw, GstMessage * msg)
{
  GError *err = NULL;
  gchar *dbg = NULL;

  GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN_CAST (bvw->priv->play),
      GST_DEBUG_GRAPH_SHOW_ALL ^ GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS,
      "totem-error");

  gst_message_parse_error (msg, &err, &dbg);
  if (err) {
    GST_ERROR ("message = %s", GST_STR_NULL (err->message));
    GST_ERROR ("domain  = %d (%s)", err->domain,
        GST_STR_NULL (g_quark_to_string (err->domain)));
    GST_ERROR ("code    = %d", err->code);
    GST_ERROR ("debug   = %s", GST_STR_NULL (dbg));
    GST_ERROR ("source  = %" GST_PTR_FORMAT, msg->src);
    GST_ERROR ("uri     = %s", GST_STR_NULL (bvw->priv->mrl));

    g_message ("Error: %s\n%s\n", GST_STR_NULL (err->message),
        GST_STR_NULL (dbg));

    g_error_free (err);
  }
  g_free (dbg);
}

static void
get_media_size (BaconVideoWidget *bvw, gint *width, gint *height)
{
  if (bvw->priv->logo_mode) {
    const GdkPixbuf *pixbuf;

    pixbuf = bvw_get_logo_pixbuf (bvw);
    if (pixbuf) {
      *width = gdk_pixbuf_get_width (pixbuf);
      *height = gdk_pixbuf_get_height (pixbuf);
    } else {
      *width = 0;
      *height = 0;
    }
  } else {
    if (bvw->priv->media_has_video) {
      GValue disp_par = {0, };
      guint movie_par_n, movie_par_d, disp_par_n, disp_par_d, num, den;
      
      /* Create and init the fraction value */
      g_value_init (&disp_par, GST_TYPE_FRACTION);

      /* Square pixel is our default */
      gst_value_set_fraction (&disp_par, 1, 1);
    
      /* Now try getting display's pixel aspect ratio */
      if (bvw->priv->xoverlay) {
        GObjectClass *klass;
        GParamSpec *pspec;

        klass = G_OBJECT_GET_CLASS (bvw->priv->xoverlay);
        pspec = g_object_class_find_property (klass, "pixel-aspect-ratio");
      
        if (pspec != NULL) {
          GValue disp_par_prop = { 0, };

          g_value_init (&disp_par_prop, pspec->value_type);
          g_object_get_property (G_OBJECT (bvw->priv->xoverlay),
              "pixel-aspect-ratio", &disp_par_prop);

          if (!g_value_transform (&disp_par_prop, &disp_par)) {
            GST_WARNING ("Transform failed, assuming pixel-aspect-ratio = 1/1");
            gst_value_set_fraction (&disp_par, 1, 1);
          }
        
          g_value_unset (&disp_par_prop);
        }
      }
      
      disp_par_n = gst_value_get_fraction_numerator (&disp_par);
      disp_par_d = gst_value_get_fraction_denominator (&disp_par);
      
      GST_DEBUG ("display PAR is %d/%d", disp_par_n, disp_par_d);
      
      /* If movie pixel aspect ratio is enforced, use that */
      if (bvw->priv->ratio_type != BVW_RATIO_AUTO) {
        switch (bvw->priv->ratio_type) {
          case BVW_RATIO_SQUARE:
            movie_par_n = 1;
            movie_par_d = 1;
            break;
          case BVW_RATIO_FOURBYTHREE:
            movie_par_n = 4 * bvw->priv->video_height;
            movie_par_d = 3 * bvw->priv->video_width;
            break;
          case BVW_RATIO_ANAMORPHIC:
            movie_par_n = 16 * bvw->priv->video_height;
            movie_par_d = 9 * bvw->priv->video_width;
            break;
          case BVW_RATIO_DVB:
            movie_par_n = 20 * bvw->priv->video_height;
            movie_par_d = 9 * bvw->priv->video_width;
            break;
          /* handle these to avoid compiler warnings */
          case BVW_RATIO_AUTO:
          default:
            movie_par_n = 0;
            movie_par_d = 0;
            g_assert_not_reached ();
        }
      }
      else {
        /* Use the movie pixel aspect ratio if any */
        movie_par_n = bvw->priv->movie_par_n;
        movie_par_d = bvw->priv->movie_par_d;
      }
      
      GST_DEBUG ("movie PAR is %d/%d", movie_par_n, movie_par_d);

      if (bvw->priv->video_width == 0 || bvw->priv->video_height == 0) {
        GST_DEBUG ("width and/or height 0, assuming 1/1 ratio");
        num = 1;
        den = 1;
      } else if (!gst_video_calculate_display_ratio (&num, &den,
          bvw->priv->video_width, bvw->priv->video_height,
          movie_par_n, movie_par_d, disp_par_n, disp_par_d)) {
        GST_WARNING ("overflow calculating display aspect ratio!");
        num = 1;   /* FIXME: what values to use here? */
        den = 1;
      }

      GST_DEBUG ("calculated scaling ratio %d/%d for video %dx%d", num, den,
          bvw->priv->video_width, bvw->priv->video_height);
      
      /* now find a width x height that respects this display ratio.
       * prefer those that have one of w/h the same as the incoming video
       * using wd / hd = num / den */
    
      /* start with same height, because of interlaced video */
      /* check hd / den is an integer scale factor, and scale wd with the PAR */
      if (bvw->priv->video_height % den == 0) {
        GST_DEBUG ("keeping video height");
        bvw->priv->video_width_pixels =
            (guint) gst_util_uint64_scale (bvw->priv->video_height, num, den);
        bvw->priv->video_height_pixels = bvw->priv->video_height;
      } else if (bvw->priv->video_width % num == 0) {
        GST_DEBUG ("keeping video width");
        bvw->priv->video_width_pixels = bvw->priv->video_width;
        bvw->priv->video_height_pixels =
            (guint) gst_util_uint64_scale (bvw->priv->video_width, den, num);
      } else {
        GST_DEBUG ("approximating while keeping video height");
        bvw->priv->video_width_pixels =
            (guint) gst_util_uint64_scale (bvw->priv->video_height, num, den);
        bvw->priv->video_height_pixels = bvw->priv->video_height;
      }
      GST_DEBUG ("scaling to %dx%d", bvw->priv->video_width_pixels,
          bvw->priv->video_height_pixels);
      
      *width = bvw->priv->video_width_pixels;
      *height = bvw->priv->video_height_pixels;
      
      /* Free the PAR fraction */
      g_value_unset (&disp_par);
    }
    else {
      *width = 0;
      *height = 0;
    }
  }
}

static void
bacon_video_widget_realize (GtkWidget * widget)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);
  GdkWindowAttr attributes;
  gint attributes_mask, w, h;
  GdkColor colour;
  GdkWindow *window;
  GdkEventMask event_mask;
  GtkAllocation allocation;

  event_mask = gtk_widget_get_events (widget)
    | GDK_POINTER_MOTION_MASK
    | GDK_KEY_PRESS_MASK;
  gtk_widget_set_events (widget, event_mask);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  window = gtk_widget_get_window (widget);
  gdk_window_ensure_native (window);

  /* Creating our video window */
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = 0;
  attributes.y = 0;
  gtk_widget_get_allocation (widget, &allocation);
  attributes.colormap = gdk_screen_get_system_colormap (gtk_widget_get_screen (widget));
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= GDK_EXPOSURE_MASK |
                           GDK_POINTER_MOTION_MASK |
                           GDK_BUTTON_PRESS_MASK |
                           GDK_KEY_PRESS_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_COLORMAP;

  bvw->priv->video_window = gdk_window_new (window,
      &attributes, attributes_mask);
  gdk_window_ensure_native (bvw->priv->video_window);
  gdk_window_set_user_data (bvw->priv->video_window, widget);

  gdk_color_parse ("black", &colour);
  gdk_colormap_alloc_color (gtk_widget_get_colormap (widget),
			    &colour, TRUE, TRUE);
  gdk_window_set_background (window, &colour);
  gtk_widget_set_style (widget,
      gtk_style_attach (gtk_widget_get_style (widget), window));

  gtk_widget_set_realized (widget, TRUE);

  /* Connect to configure event on the top level window */
  g_signal_connect (G_OBJECT (gtk_widget_get_toplevel (widget)),
      "configure-event", G_CALLBACK (bacon_video_widget_configure_event), bvw);

  /* get screen size changes */
  g_signal_connect (G_OBJECT (gtk_widget_get_screen (widget)),
      "size-changed", G_CALLBACK (size_changed_cb), bvw);

  /* nice hack to show the logo fullsize, while still being resizable */
  get_media_size (BACON_VIDEO_WIDGET (widget), &w, &h);
  totem_widget_set_preferred_size (widget, w, h);

  bacon_video_widget_gst_missing_plugins_setup (bvw);

  bvw->priv->bacon_resize = bacon_resize_new (widget);
}

static void
bacon_video_widget_unrealize (GtkWidget *widget)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);

  g_object_unref (bvw->priv->bacon_resize);
  gdk_window_set_user_data (bvw->priv->video_window, NULL);
  gdk_window_destroy (bvw->priv->video_window);
  bvw->priv->video_window = NULL;

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
bacon_video_widget_show (GtkWidget *widget)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);
  GdkWindow *window;

  window = gtk_widget_get_window (widget);
  if (window)
    gdk_window_show (window);
  if (bvw->priv->video_window)
    gdk_window_show (bvw->priv->video_window);

  if (GTK_WIDGET_CLASS (parent_class)->show)
    GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
bacon_video_widget_hide (GtkWidget *widget)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);
  GdkWindow *window;

  window = gtk_widget_get_window (widget);
  if (window)
    gdk_window_hide (window);
  if (bvw->priv->video_window)
    gdk_window_hide (bvw->priv->video_window);

  if (GTK_WIDGET_CLASS (parent_class)->hide)
    GTK_WIDGET_CLASS (parent_class)->hide (widget);
}

static gboolean
bacon_video_widget_configure_event (GtkWidget *widget, GdkEventConfigure *event,
    BaconVideoWidget *bvw)
{
  GstXOverlay *xoverlay = NULL;
  
  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  
  xoverlay = bvw->priv->xoverlay;
  
  if (xoverlay != NULL && GST_IS_X_OVERLAY (xoverlay)) {
    gst_x_overlay_expose (xoverlay);
  }
  
  return FALSE;
}

static void
size_changed_cb (GdkScreen *screen, BaconVideoWidget *bvw)
{
  /* FIXME */
  setup_vis (bvw);
}

static gboolean
bacon_video_widget_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);
  GstXOverlay *xoverlay;
  gboolean draw_logo;
  XID window;
  GdkWindow *win;
  GtkAllocation allocation;
  cairo_t *cr;

  if (event && event->count > 0)
    return TRUE;

  g_mutex_lock (bvw->priv->lock);
  xoverlay = bvw->priv->xoverlay;
  if (xoverlay == NULL) {
    bvw_update_interface_implementations (bvw);
    xoverlay = bvw->priv->xoverlay;
  }
  if (xoverlay != NULL)
    gst_object_ref (xoverlay);

  g_mutex_unlock (bvw->priv->lock);

  window = GDK_WINDOW_XWINDOW (bvw->priv->video_window);

  if (xoverlay != NULL && GST_IS_X_OVERLAY (xoverlay))
    gst_x_overlay_set_xwindow_id (xoverlay, window);

  /* Start with a nice black canvas */
  win = gtk_widget_get_window (widget);
  gtk_widget_get_allocation (widget, &allocation);

  cr = gdk_cairo_create (win);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
  cairo_paint (cr);

  /* If there's only audio and no visualisation, draw the logo as well.
   * If we have a cover image to display, we display it regardless of whether we're
   * doing visualisations. */
  draw_logo = bvw->priv->media_has_audio &&
      !bvw->priv->media_has_video && (!bvw->priv->show_vfx || bvw->priv->cover_pixbuf);

  if (bvw->priv->logo_mode || draw_logo) {
    const GdkPixbuf *pixbuf;

    pixbuf = bvw_get_logo_pixbuf (bvw);
    if (pixbuf != NULL) {
      /* draw logo here */
      GdkPixbuf *logo = NULL;
      gint s_width, s_height, d_width, d_height;
      gfloat ratio;

      s_width = gdk_pixbuf_get_width (pixbuf);
      s_height = gdk_pixbuf_get_height (pixbuf);
      d_width = allocation.width;
      d_height = allocation.height;

      /* Limit the width/height to 256×256 pixels, but only if we're displaying the logo proper */
      if (!bvw->priv->cover_pixbuf && d_width > LOGO_SIZE && d_height > LOGO_SIZE)
        d_width = d_height = LOGO_SIZE;

      if ((gfloat) d_width / s_width > (gfloat) d_height / s_height) {
        ratio = (gfloat) d_height / s_height;
      } else {
        ratio = (gfloat) d_width / s_width;
      }

      s_width *= ratio;
      s_height *= ratio;

      if (s_width <= 1 || s_height <= 1) {
        if (xoverlay != NULL)
	  gst_object_unref (xoverlay);
	cairo_paint (cr);
	cairo_destroy (cr);
	return TRUE;
      }

      /* Only scale the logo if necessary */
      if (d_width != LOGO_SIZE || d_height != LOGO_SIZE)
        logo = gdk_pixbuf_scale_simple (pixbuf, s_width, s_height, GDK_INTERP_BILINEAR);
      else
        logo = g_object_ref (G_OBJECT (pixbuf));

      gdk_cairo_set_source_pixbuf (cr, logo, (allocation.width - s_width) / 2, (allocation.height - s_height) / 2);
      cairo_paint (cr);

      g_object_unref (logo);
    } else if (win) {
      /* No pixbuf, just draw a black background then */
      gdk_window_clear_area (win,
			     0, 0,
			     allocation.width,
			     allocation.height);
    }
  } else {
    /* no logo, pass the expose to gst */
    if (xoverlay != NULL && GST_IS_X_OVERLAY (xoverlay))
      gst_x_overlay_expose (xoverlay);
    else {
      /* No xoverlay to expose yet */
      gdk_window_clear_area (win,
			     0, 0,
			     allocation.width,
			     allocation.height);
    }
  }
  if (xoverlay != NULL)
    gst_object_unref (xoverlay);

  cairo_destroy (cr);

  return TRUE;
}

static GstNavigation *
bvw_get_navigation_iface (BaconVideoWidget *bvw)
{
  GstNavigation *nav = NULL;
  g_mutex_lock (bvw->priv->lock);
  if (bvw->priv->navigation == NULL)
    bvw_update_interface_implementations (bvw);
  if (bvw->priv->navigation)
    nav = gst_object_ref (GST_OBJECT (bvw->priv->navigation));
  g_mutex_unlock (bvw->priv->lock);

  return nav;
}
    
/* need to use gstnavigation interface for these vmethods, to allow for the sink
   to map screen coordinates to video coordinates in the presence of e.g.
   hardware scaling */

static gboolean
bacon_video_widget_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
  gboolean res = FALSE;
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);

  g_return_val_if_fail (bvw->priv->play != NULL, FALSE);

  if (!bvw->priv->logo_mode) {
    GstNavigation *nav = bvw_get_navigation_iface (bvw);
    if (nav) {
      gst_navigation_send_mouse_event (nav, "mouse-move", 0, event->x, event->y);
      gst_object_unref (GST_OBJECT (nav));
    }
  }

  if (GTK_WIDGET_CLASS (parent_class)->motion_notify_event)
    res |= GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);

  return res;
}

static gboolean
bacon_video_widget_button_press (GtkWidget *widget, GdkEventButton *event)
{
  gboolean res = FALSE;
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);

  g_return_val_if_fail (bvw->priv->play != NULL, FALSE);

  if (!bvw->priv->logo_mode) {
    GstNavigation *nav = bvw_get_navigation_iface (bvw);
    if (nav) {
      gst_navigation_send_mouse_event (nav,
          "mouse-button-press", event->button, event->x, event->y);
      gst_object_unref (GST_OBJECT (nav));

      /* FIXME need to check whether the backend will have handled
       * the button press
      res = TRUE; */
    }
  }

  if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
    res |= GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);

  return res;
}

static gboolean
bacon_video_widget_button_release (GtkWidget *widget, GdkEventButton *event)
{
  gboolean res = FALSE;
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);

  g_return_val_if_fail (bvw->priv->play != NULL, FALSE);

  if (!bvw->priv->logo_mode) {
    GstNavigation *nav = bvw_get_navigation_iface (bvw);
    if (nav) {
      gst_navigation_send_mouse_event (nav,
          "mouse-button-release", event->button, event->x, event->y);
      gst_object_unref (GST_OBJECT (nav));

      res = TRUE;
    }
  }

  if (GTK_WIDGET_CLASS (parent_class)->button_release_event)
    res |= GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);

  return res;
}

static void
bacon_video_widget_size_request (GtkWidget * widget,
    GtkRequisition * requisition)
{
  requisition->width = 240;
  requisition->height = 180;
}

static void
resize_video_window (BaconVideoWidget *bvw)
{
  GtkAllocation allocation;
  gfloat width, height, ratio, x, y;
  int w, h;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  gtk_widget_get_allocation (GTK_WIDGET (bvw), &allocation);

  get_media_size (bvw, &w, &h);
  if (!w || !h) {
    w = allocation.width;
    h = allocation.height;
  }
  width = w;
  height = h;

  /* calculate ratio for fitting video into the available space */
  if ((gfloat) allocation.width / width >
      (gfloat) allocation.height / height) {
    ratio = (gfloat) allocation.height / height;
  } else {
    ratio = (gfloat) allocation.width / width;
  }

  /* apply zoom factor */
  ratio = ratio * bvw->priv->zoom;

  width *= ratio;
  height *= ratio;
  x = (allocation.width - width) / 2;
  y = (allocation.height - height) / 2;

  gdk_window_move_resize (bvw->priv->video_window, x, y, width, height);
  gtk_widget_queue_draw (GTK_WIDGET (bvw));
}

static void
bacon_video_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (widget);

  g_return_if_fail (widget != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (widget));

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget)) {

    gdk_window_move_resize (gtk_widget_get_window (widget),
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

    resize_video_window (bvw);
  }
}

static gboolean
bvw_boolean_handled_accumulator (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer foobar)
{
  gboolean continue_emission;
  gboolean signal_handled;
  
  signal_handled = g_value_get_boolean (handler_return);
  g_value_set_boolean (return_accu, signal_handled);
  continue_emission = !signal_handled;
  
  return continue_emission;
}

static void
bacon_video_widget_class_init (BaconVideoWidgetClass * klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private (object_class, sizeof (BaconVideoWidgetPrivate));

  /* GtkWidget */
  widget_class->size_request = bacon_video_widget_size_request;
  widget_class->size_allocate = bacon_video_widget_size_allocate;
  widget_class->realize = bacon_video_widget_realize;
  widget_class->unrealize = bacon_video_widget_unrealize;
  widget_class->show = bacon_video_widget_show;
  widget_class->hide = bacon_video_widget_hide;
  widget_class->expose_event = bacon_video_widget_expose_event;
  widget_class->motion_notify_event = bacon_video_widget_motion_notify;
  widget_class->button_press_event = bacon_video_widget_button_press;
  widget_class->button_release_event = bacon_video_widget_button_release;

  /* GObject */
  object_class->set_property = bacon_video_widget_set_property;
  object_class->get_property = bacon_video_widget_get_property;
  object_class->finalize = bacon_video_widget_finalize;

  /* Properties */
  /**
   * BaconVideoWidget:logo-mode:
   *
   * Whether the logo should be displayed when no stream is loaded, or the widget
   * should take up no space.
   **/
  g_object_class_install_property (object_class, PROP_LOGO_MODE,
                                   g_param_spec_boolean ("logo-mode", NULL,
                                                         NULL, FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:position:
   *
   * The current position in the stream, as a percentage between %0 and %1.
   **/
  g_object_class_install_property (object_class, PROP_POSITION,
                                   g_param_spec_double ("position", NULL, NULL,
							0, 1.0, 0,
							G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:stream-length:
   *
   * The length of the current stream, in milliseconds.
   **/
  g_object_class_install_property (object_class, PROP_STREAM_LENGTH,
	                           g_param_spec_int64 ("stream-length", NULL,
                                                     NULL, 0, G_MAXINT64, 0,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:playing:
   *
   * Whether a stream is currently playing.
   **/
  g_object_class_install_property (object_class, PROP_PLAYING,
                                   g_param_spec_boolean ("playing", NULL,
                                                         NULL, FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:seekable:
   *
   * Whether the current stream can be seeked.
   **/
  g_object_class_install_property (object_class, PROP_SEEKABLE,
                                   g_param_spec_boolean ("seekable", NULL,
                                                         NULL, FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:volume:
   *
   * The current volume level, as a percentage between %0 and %1.
   **/
  g_object_class_install_property (object_class, PROP_VOLUME,
	                           g_param_spec_double ("volume", NULL, NULL,
	                                                0.0, 1.0, 0.0,
	                                                G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:show-cursor:
   *
   * Whether the cursor should be shown, or should be invisible, when it is over
   * the video widget.
   **/
  g_object_class_install_property (object_class, PROP_SHOW_CURSOR,
                                   g_param_spec_boolean ("show-cursor", NULL,
                                                         NULL, FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:show-visuals:
   *
   * Whether visualisations should be shown for audio-only streams.
   **/
  g_object_class_install_property (object_class, PROP_SHOW_VISUALS,
                                   g_param_spec_boolean ("show-visuals", NULL,
                                                         NULL, FALSE,
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:referrer:
   *
   * The HTTP referrer URI.
   **/
  g_object_class_install_property (object_class, PROP_USER_AGENT,
                                   g_param_spec_string ("referrer", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:user-agent:
   *
   * The HTTP user agent string to use.
   **/
  g_object_class_install_property (object_class, PROP_USER_AGENT,
                                   g_param_spec_string ("user-agent", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * BaconVideoWidget:download-filename:
   *
   * The filename of the fully downloaded stream when using
   * download buffering.
   **/
  g_object_class_install_property (object_class, PROP_DOWNLOAD_FILENAME,
                                   g_param_spec_string ("download-filename", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* Signals */
  /**
   * BaconVideoWidget::error:
   * @message: the error message
   * @playback_stopped: %TRUE if playback has stopped due to the error, %FALSE otherwise
   * @fatal: %TRUE if the error was fatal to playback, %FALSE otherwise
   *
   * Emitted when the backend wishes to asynchronously report an error. If @fatal is %TRUE,
   * playback of this stream cannot be restarted.
   **/
  bvw_signals[SIGNAL_ERROR] =
    g_signal_new (I_("error"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BaconVideoWidgetClass, error),
                  NULL, NULL,
                  baconvideowidget_marshal_VOID__STRING_BOOLEAN_BOOLEAN,
                  G_TYPE_NONE, 3, G_TYPE_STRING,
                  G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

  /**
   * BaconVideoWidget::eos:
   *
   * Emitted when the end of the current stream is reached.
   **/
  bvw_signals[SIGNAL_EOS] =
    g_signal_new (I_("eos"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BaconVideoWidgetClass, eos),
                  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * BaconVideoWidget::got-metadata:
   *
   * Emitted when the widget has updated the metadata of the current stream. This
   * will typically happen just after opening a stream.
   *
   * Call bacon_video_widget_get_metadata() to query the updated metadata.
   **/
  bvw_signals[SIGNAL_GOT_METADATA] =
    g_signal_new (I_("got-metadata"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BaconVideoWidgetClass, got_metadata),
                  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * BaconVideoWidget::got-redirect:
   * @new_mrl: the new MRL
   *
   * Emitted when a redirect response is received from a stream's server.
   **/
  bvw_signals[SIGNAL_REDIRECT] =
    g_signal_new (I_("got-redirect"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BaconVideoWidgetClass, got_redirect),
                  NULL, NULL, g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  /**
   * BaconVideoWidget::channels-change:
   *
   * Emitted when the number of audio languages available changes, or when the
   * selected audio language is changed.
   *
   * Query the new list of audio languages with bacon_video_widget_get_languages().
   **/
  bvw_signals[SIGNAL_CHANNELS_CHANGE] =
    g_signal_new (I_("channels-change"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BaconVideoWidgetClass, channels_change),
                  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * BaconVideoWidget::tick:
   * @current_time: the current position in the stream, in milliseconds since the beginning of the stream
   * @stream_length: the length of the stream, in milliseconds
   * @current_position: the current position in the stream, as a percentage between %0 and %1
   * @seekable: %TRUE if the stream can be seeked, %FALSE otherwise
   *
   * Emitted every time an important time event happens, or at regular intervals when playing a stream.
   **/
  bvw_signals[SIGNAL_TICK] =
    g_signal_new (I_("tick"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BaconVideoWidgetClass, tick),
                  NULL, NULL,
                  baconvideowidget_marshal_VOID__INT64_INT64_DOUBLE_BOOLEAN,
                  G_TYPE_NONE, 4, G_TYPE_INT64, G_TYPE_INT64, G_TYPE_DOUBLE,
                  G_TYPE_BOOLEAN);

  /**
   * BaconVideoWidget::buffering:
   * @percentage: the percentage of buffering completed, between %0 and %1
   *
   * Emitted regularly when a network stream is being buffered, to provide status updates on the buffering
   * progress.
   **/
  bvw_signals[SIGNAL_BUFFERING] =
    g_signal_new (I_("buffering"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BaconVideoWidgetClass, buffering),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * BaconVideoWidget::missing-plugins:
   * @details: a %NULL-terminated array of missing plugin details for use when installing the plugins with libgimme-codec
   * @descriptions: a %NULL-terminated array of missing plugin descriptions for display to the user
   * @playing: %TRUE if the stream could be played even without these plugins, %FALSE otherwise
   *
   * Emitted when plugins required to play the current stream are not found. This allows the application
   * to request the user install them before proceeding to try and play the stream again.
   *
   * Note that this signal is only available for the GStreamer backend.
   *
   * Return value: %TRUE if the signal was handled and some action was taken, %FALSE otherwise
   **/
  bvw_signals[SIGNAL_MISSING_PLUGINS] =
    g_signal_new (I_("missing-plugins"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, /* signal is enough, we don't need a vfunc */
                  bvw_boolean_handled_accumulator, NULL,
                  baconvideowidget_marshal_BOOLEAN__BOXED_BOXED_BOOLEAN,
                  G_TYPE_BOOLEAN, 3, G_TYPE_STRV, G_TYPE_STRV, G_TYPE_BOOLEAN);

  /**
   * BaconVideoWidget::download-buffering:
   * @percentage: the percentage of download buffering completed, between %0 and %1
   *
   * Emitted regularly when a network stream is being cached on disk, to provide status
   *  updates on the buffering level of the stream.
   **/
  bvw_signals[SIGNAL_DOWNLOAD_BUFFERING] =
    g_signal_new ("download-buffering",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BaconVideoWidgetClass, download_buffering),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE, G_TYPE_NONE, 1, G_TYPE_DOUBLE);
}

static void
bacon_video_widget_init (BaconVideoWidget * bvw)
{
  BaconVideoWidgetPrivate *priv;

  gtk_widget_set_can_focus (GTK_WIDGET (bvw), TRUE);
  gtk_widget_set_double_buffered (GTK_WIDGET (bvw), FALSE);

  bvw->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (bvw, BACON_TYPE_VIDEO_WIDGET, BaconVideoWidgetPrivate);

  priv->update_id = 0;
  priv->tagcache = NULL;
  priv->audiotags = NULL;
  priv->videotags = NULL;
  priv->zoom = 1.0;
  priv->volume = -1.0;
  priv->movie_par_n = priv->movie_par_d = 1;
  priv->rate = FORWARD_RATE;

  priv->tag_update_queue = g_async_queue_new_full ((GDestroyNotify) update_tags_delayed_data_destroy);
  priv->tag_update_id = 0;

  priv->lock = g_mutex_new ();

  priv->seek_mutex = g_mutex_new ();
  priv->clock = gst_system_clock_obtain ();
  priv->seek_req_time = GST_CLOCK_TIME_NONE;
  priv->seek_time = -1;

  bvw->priv->missing_plugins = NULL;
  bvw->priv->plugin_install_in_progress = FALSE;

  bvw->priv->mount_cancellable = NULL;
  bvw->priv->mount_in_progress = FALSE;
  bvw->priv->auth_last_result = G_MOUNT_OPERATION_HANDLED;
  bvw->priv->auth_dialog = NULL;

  bacon_video_widget_gst_missing_plugins_blacklist ();
}

static void
shrink_toplevel (BaconVideoWidget * bvw)
{
  GtkWidget *toplevel, *widget;
  widget = GTK_WIDGET (bvw);
  toplevel = gtk_widget_get_toplevel (widget);
  if (toplevel != widget && GTK_IS_WINDOW (toplevel) != FALSE)
    gtk_window_resize (GTK_WINDOW (toplevel), 1, 1);
}

static gboolean bvw_query_timeout (BaconVideoWidget *bvw);
static gboolean bvw_query_buffering_timeout (BaconVideoWidget *bvw);
static void parse_stream_info (BaconVideoWidget *bvw);

static void
bvw_update_stream_info (BaconVideoWidget *bvw)
{
  parse_stream_info (bvw);

  /* if we're not interactive, we want to announce metadata
   * only later when we can be sure we got it all */
  if (bvw->priv->use_type == BVW_USE_TYPE_VIDEO ||
      bvw->priv->use_type == BVW_USE_TYPE_AUDIO) {
    g_signal_emit (bvw, bvw_signals[SIGNAL_GOT_METADATA], 0, NULL);
    g_signal_emit (bvw, bvw_signals[SIGNAL_CHANNELS_CHANGE], 0);
  }
}

static void
bvw_handle_application_message (BaconVideoWidget *bvw, GstMessage *msg)
{
  const gchar *msg_name;
  GdkWindow *window;
  GtkAllocation allocation;

  msg_name = gst_structure_get_name (msg->structure);
  g_return_if_fail (msg_name != NULL);

  GST_DEBUG ("Handling application message: %" GST_PTR_FORMAT, msg->structure);

  if (strcmp (msg_name, "stream-changed") == 0) {
    bvw_update_stream_info (bvw);
  } 
  else if (strcmp (msg_name, "video-size") == 0) {
    /* if we're not interactive, we want to announce metadata
     * only later when we can be sure we got it all */
    if (bvw->priv->use_type == BVW_USE_TYPE_VIDEO ||
        bvw->priv->use_type == BVW_USE_TYPE_AUDIO) {
      g_signal_emit (bvw, bvw_signals[SIGNAL_GOT_METADATA], 0, NULL);
    }

    if (bvw->priv->auto_resize
       	&& !bvw->priv->fullscreen_mode
	&& !bvw->priv->window_resized) {
      bacon_video_widget_set_scale_ratio (bvw, 0.0);
    } else {
      gtk_widget_get_allocation (GTK_WIDGET (bvw), &allocation);
      bacon_video_widget_size_allocate (GTK_WIDGET (bvw),
                                        &allocation);

      /* Uhm, so this ugly hack here makes media loading work for
       * weird laptops with NVIDIA graphics cards... Dunno what the
       * bug is really, but hey, it works. :). */
      window = gtk_widget_get_window (GTK_WIDGET (bvw));
      if (window) {
        gdk_window_hide (window);
        gdk_window_show (window);

        bacon_video_widget_expose_event (GTK_WIDGET (bvw), NULL);
      }
    }
    bvw->priv->window_resized = TRUE;
  } else {
    g_message ("Unhandled application message %s", msg_name);
  }
}

static gboolean
bvw_do_navigation_query (BaconVideoWidget * bvw, GstQuery *query)
{
  GstNavigation *nav = bvw_get_navigation_iface (bvw);
  gboolean res;

  if (G_UNLIKELY (nav == NULL || !GST_IS_ELEMENT (nav)))
    return FALSE;

  res = gst_element_query (GST_ELEMENT_CAST (nav), query);
  gst_object_unref (GST_OBJECT (nav));

  return res;
}

static void
mount_cb (GObject *obj, GAsyncResult *res, gpointer user_data)
{
  BaconVideoWidget * bvw = user_data;
  gboolean ret;
  gchar *uri;
  GError *error = NULL;

  ret = g_file_mount_enclosing_volume_finish (G_FILE (obj), res, &error);

  g_object_unref (bvw->priv->mount_cancellable);
  bvw->priv->mount_cancellable = NULL;
  bvw->priv->mount_in_progress = FALSE;

  g_object_get (G_OBJECT (bvw->priv->play), "uri", &uri, NULL);

  if (ret) {

    GST_DEBUG ("Mounting location '%s' successful", GST_STR_NULL (uri));
    if (bvw->priv->target_state == GST_STATE_PLAYING)
      bacon_video_widget_play (bvw, NULL);
  } else {
    GError *err = NULL;
    GstMessage *msg;

    GST_DEBUG ("Mounting location '%s' failed: %s", GST_STR_NULL (uri), error->message);

    /* create a fake GStreamer error so we get a nice warning message */
    err = g_error_new_literal (GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_OPEN_READ, error->message);
    msg = gst_message_new_error (GST_OBJECT (bvw->priv->play), err, error->message);
    g_error_free (err);
    g_error_free (error);
    err = bvw_error_from_gst_error (bvw, msg);
    gst_message_unref (msg);
    g_signal_emit (bvw, bvw_signals[SIGNAL_ERROR], 0, err->message, FALSE, FALSE);
    g_error_free (err);
  }

  g_free (uri);
}

static void
bvw_handle_element_message (BaconVideoWidget *bvw, GstMessage *msg)
{
  const gchar *type_name = NULL;
  gchar *src_name;

  src_name = gst_object_get_name (msg->src);
  if (msg->structure)
    type_name = gst_structure_get_name (msg->structure);

  GST_DEBUG ("from %s: %" GST_PTR_FORMAT, src_name, msg->structure);

  if (type_name == NULL)
    goto unhandled;

  if (strcmp (type_name, "redirect") == 0) {
    const gchar *new_location;

    new_location = gst_structure_get_string (msg->structure, "new-location");
    GST_DEBUG ("Got redirect to '%s'", GST_STR_NULL (new_location));

    if (new_location && *new_location) {
      g_signal_emit (bvw, bvw_signals[SIGNAL_REDIRECT], 0, new_location);
      goto done;
    }
  } else if (strcmp (type_name, "progress") == 0) {
    /* this is similar to buffering messages, but shouldn't affect pipeline
     * state; qtdemux emits those when headers are after movie data and
     * it is in streaming mode and has to receive all the movie data first */
    if (!bvw->priv->buffering) {
      gint percent = 0;

      if (gst_structure_get_int (msg->structure, "percent", &percent))
        g_signal_emit (bvw, bvw_signals[SIGNAL_BUFFERING], 0, percent);
    }
    goto done;
  } else if (strcmp (type_name, "prepare-xwindow-id") == 0 ||
      strcmp (type_name, "have-xwindow-id") == 0) {
    /* we handle these synchronously or want to ignore them */
    goto done;
  } else if (gst_is_missing_plugin_message (msg)) {
    bvw->priv->missing_plugins =
      g_list_prepend (bvw->priv->missing_plugins, gst_message_ref (msg));
    goto done;
  } else if (strcmp (type_name, "not-mounted") == 0) {
    const GValue *val;
    GFile *file;
    GMountOperation *mount_op;
    GtkWidget *toplevel;
    gchar *uri;

    g_object_get (G_OBJECT (bvw->priv->play), "uri", &uri, NULL);

    if (bvw->priv->mount_in_progress) {
      g_cancellable_cancel (bvw->priv->mount_cancellable);
      g_object_unref (bvw->priv->mount_cancellable);
      bvw->priv->mount_cancellable = NULL;
      bvw->priv->mount_in_progress = FALSE;
    }

    GST_DEBUG ("Trying to mount location '%s'", GST_STR_NULL (uri));
    g_free (uri);
    
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (bvw));
    if (toplevel == GTK_WIDGET (bvw) || !GTK_IS_WINDOW (toplevel))
      toplevel = NULL;

    val = gst_structure_get_value (msg->structure, "file");
    if (val == NULL)
      goto done;
      
    file = G_FILE (g_value_get_object (val));
    if (file == NULL)
      goto done;

    bacon_video_widget_stop (bvw);

    mount_op = gtk_mount_operation_new (toplevel ? GTK_WINDOW (toplevel) : NULL);
    bvw->priv->mount_in_progress = TRUE;
    bvw->priv->mount_cancellable = g_cancellable_new ();
    g_file_mount_enclosing_volume (file, G_MOUNT_MOUNT_NONE,
        mount_op, bvw->priv->mount_cancellable, mount_cb, bvw);

    g_object_unref (mount_op);
    goto done;
  } else {
    GstNavigationMessageType nav_msg_type =
        gst_navigation_message_get_type (msg);

    switch (nav_msg_type) {
      case GST_NAVIGATION_MESSAGE_MOUSE_OVER: {
        gint active;
        if (!gst_navigation_message_parse_mouse_over (msg, &active))
          break;
        if (active) {
          if (bvw->priv->cursor == NULL) {
            bvw->priv->cursor = gdk_cursor_new (GDK_HAND2);
          }
        } else {
          if (bvw->priv->cursor != NULL) {
            gdk_cursor_unref (bvw->priv->cursor);
            bvw->priv->cursor = NULL;
          }
        }
        gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET(bvw)),
            bvw->priv->cursor);
        goto done;
      }
      case GST_NAVIGATION_MESSAGE_COMMANDS_CHANGED: {
        GstQuery *cmds_q = gst_navigation_query_new_commands();
        gboolean res = bvw_do_navigation_query (bvw, cmds_q);

        if (res) {
          gboolean is_menu = FALSE;
          guint i, n;

          if (gst_navigation_query_parse_commands_length (cmds_q, &n)) {
            for (i = 0; i < n; i++) {
              GstNavigationCommand cmd;
              if (!gst_navigation_query_parse_commands_nth (cmds_q, i, &cmd))
                break;
              is_menu |= (cmd == GST_NAVIGATION_COMMAND_ACTIVATE);
              is_menu |= (cmd == GST_NAVIGATION_COMMAND_LEFT);
              is_menu |= (cmd == GST_NAVIGATION_COMMAND_RIGHT);
              is_menu |= (cmd == GST_NAVIGATION_COMMAND_UP);
              is_menu |= (cmd == GST_NAVIGATION_COMMAND_DOWN);
            }
          }
	  /* Are we in a menu now? */
	  if (bvw->priv->is_menu != is_menu) {
	    bvw->priv->is_menu = is_menu;
	    g_object_notify (G_OBJECT (bvw), "seekable");
	  }
        }

        gst_query_unref (cmds_q);
        goto done;
      }
      default:
        break;
    }
  }

unhandled:
  GST_WARNING ("Unhandled element message %s from %s: %" GST_PTR_FORMAT,
      GST_STR_NULL (type_name), GST_STR_NULL (src_name), msg);

done:
  g_free (src_name);
}

/* This is a hack to avoid doing poll_for_state_change() indirectly
 * from the bus message callback (via EOS => totem => close => wait for READY)
 * and deadlocking there. We need something like a
 * gst_bus_set_auto_flushing(bus, FALSE) ... */
static gboolean
bvw_signal_eos_delayed (gpointer user_data)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (user_data);

  g_signal_emit (bvw, bvw_signals[SIGNAL_EOS], 0, NULL);
  bvw->priv->eos_id = 0;
  return FALSE;
}

static void
bvw_reconfigure_tick_timeout (BaconVideoWidget *bvw, guint msecs)
{
  if (bvw->priv->update_id != 0) {
    GST_DEBUG ("removing tick timeout");
    g_source_remove (bvw->priv->update_id);
    bvw->priv->update_id = 0;
  }
  if (msecs > 0) {
    GST_DEBUG ("adding tick timeout (at %ums)", msecs);
    bvw->priv->update_id =
      g_timeout_add (msecs, (GSourceFunc) bvw_query_timeout, bvw);
  }
}

static void
bvw_reconfigure_fill_timeout (BaconVideoWidget *bvw, guint msecs)
{
  if (bvw->priv->fill_id != 0) {
    GST_DEBUG ("removing fill timeout");
    g_source_remove (bvw->priv->fill_id);
    bvw->priv->fill_id = 0;
  }
  if (msecs > 0) {
    GST_DEBUG ("adding fill timeout (at %ums)", msecs);
    bvw->priv->fill_id =
      g_timeout_add (msecs, (GSourceFunc) bvw_query_buffering_timeout, bvw);
  }
}

/* returns TRUE if the error/signal has been handled and should be ignored */
static gboolean
bvw_emit_missing_plugins_signal (BaconVideoWidget * bvw, gboolean prerolled)
{
  gboolean handled = FALSE;
  gchar **descriptions, **details;

  details = bvw_get_missing_plugins_details (bvw->priv->missing_plugins);
  descriptions = bvw_get_missing_plugins_descriptions (bvw->priv->missing_plugins);

  GST_LOG ("emitting missing-plugins signal (prerolled=%d)", prerolled);

  g_signal_emit (bvw, bvw_signals[SIGNAL_MISSING_PLUGINS], 0,
      details, descriptions, prerolled, &handled);
  GST_DEBUG ("missing-plugins signal was %shandled", (handled) ? "" : "not ");

  g_strfreev (descriptions);
  g_strfreev (details);

  if (handled) {
    bvw->priv->plugin_install_in_progress = TRUE;
    bvw_clear_missing_plugins_messages (bvw);
  }

  /* if it wasn't handled, we might need the list of missing messages again
   * later to create a proper error message with details of what's missing */

  return handled;
}

static void
bvw_auth_reply_cb (GMountOperation      *op,
		   GMountOperationResult result,
		   BaconVideoWidget     *bvw)
{
  GST_DEBUG ("Got authentication reply %d", result);
  bvw->priv->auth_last_result = result;

  if (result == G_MOUNT_OPERATION_HANDLED) {
    bvw->priv->user_id = g_strdup (g_mount_operation_get_username (op));
    bvw->priv->user_pw = g_strdup (g_mount_operation_get_password (op));
  }

  g_object_unref (bvw->priv->auth_dialog);
  bvw->priv->auth_dialog = NULL;

  if (bvw->priv->target_state == GST_STATE_PLAYING) {
    GST_DEBUG ("Starting deferred playback after authentication");
    bacon_video_widget_play (bvw, NULL);
  }
}

/* returns TRUE if the error should be ignored */
static gboolean
bvw_check_missing_auth (BaconVideoWidget * bvw, GstMessage * err_msg)
{
  gboolean retval;

  retval = FALSE;

  if (bvw->priv->use_type != BVW_USE_TYPE_VIDEO ||
      gtk_widget_get_realized (GTK_WIDGET (bvw)) == FALSE)
    return retval;

  /* The user already tried, and we aborted */
  if (bvw->priv->auth_last_result == G_MOUNT_OPERATION_ABORTED) {
    GST_DEBUG ("Not authenticating, the user aborted the last auth attempt");
    return retval;
  }
  /* There's already an auth on-going, ignore */
  if (bvw->priv->auth_dialog != NULL) {
    GST_DEBUG ("Ignoring error, we're doing authentication");
    return TRUE;
  }

  /* RTSP or HTTP source with user-id property ? */
  if ((g_strcmp0 ("GstRTSPSrc", G_OBJECT_TYPE_NAME (err_msg->src)) == 0 ||
       g_strcmp0 ("GstSoupHTTPSrc", G_OBJECT_TYPE_NAME (err_msg->src)) == 0) &&
      g_object_class_find_property (G_OBJECT_GET_CLASS (err_msg->src), "user-id") != NULL) {
    GError *err = NULL;
    gchar *dbg = NULL;

    gst_message_parse_error (err_msg, &err, &dbg);

    /* Urgh! Check whether this is an auth error */
    if (err != NULL && dbg != NULL &&
	is_error (err, RESOURCE, READ) &&
	strstr (dbg, "401") != NULL) {
      GtkWidget *toplevel;
      GMountOperationClass *klass;

      GST_DEBUG ("Trying to get auth for location '%s'", GST_STR_NULL (bvw->priv->mrl));

      if (bvw->priv->auth_dialog == NULL) {
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (bvw));
	bvw->priv->auth_dialog = gtk_mount_operation_new (GTK_WINDOW (toplevel));
	g_signal_connect (G_OBJECT (bvw->priv->auth_dialog), "reply",
			  G_CALLBACK (bvw_auth_reply_cb), bvw);
      }

      /* And popup the dialogue! */
      klass = (GMountOperationClass *) G_OBJECT_GET_CLASS (bvw->priv->auth_dialog);
      klass->ask_password (bvw->priv->auth_dialog,
			   _("Password requested for RTSP server"),
			   g_get_user_name (),
			   NULL,
			   G_ASK_PASSWORD_NEED_PASSWORD | G_ASK_PASSWORD_NEED_USERNAME);
      retval = TRUE;
    }
    if (err != NULL)
      g_error_free (err);
    g_free (dbg);
  }

  return retval;
}

/* returns TRUE if the error has been handled and should be ignored */
static gboolean
bvw_check_missing_plugins_error (BaconVideoWidget * bvw, GstMessage * err_msg)
{
  gboolean error_src_is_playbin;
  gboolean ret = FALSE;
  GError *err = NULL;

  if (bvw->priv->missing_plugins == NULL) {
    GST_DEBUG ("no missing-plugin messages");
    return FALSE;
  }

  gst_message_parse_error (err_msg, &err, NULL);

  error_src_is_playbin = (err_msg->src == GST_OBJECT_CAST (bvw->priv->play));

  /* If we get a WRONG_TYPE error from playbin itself it's most likely because
   * there is a subtitle stream we can decode, but no video stream to overlay
   * it on. Since there were missing-plugins messages, we'll assume this is
   * because we cannot decode the video stream (this should probably be fixed
   * in playbin, but for now we'll work around it here) */
  if (is_error (err, CORE, MISSING_PLUGIN) ||
      is_error (err, STREAM, CODEC_NOT_FOUND) ||
      (is_error (err, STREAM, WRONG_TYPE) && error_src_is_playbin)) {
    ret = bvw_emit_missing_plugins_signal (bvw, FALSE);
    if (ret) {
      /* If it was handled, stop playback to make sure we're not processing any
       * other error messages that might also be on the bus */
      bacon_video_widget_stop (bvw);
    }
  } else {
    GST_DEBUG ("not an error code we are looking for, doing nothing");
  }

  g_error_free (err);
  return ret;
}

/* returns TRUE if the error/signal has been handled and should be ignored */
static gboolean
bvw_check_missing_plugins_on_preroll (BaconVideoWidget * bvw)
{
  if (bvw->priv->missing_plugins == NULL) {
    GST_DEBUG ("no missing-plugin messages");
    return FALSE;
  }

  return bvw_emit_missing_plugins_signal (bvw, TRUE); 
}

static void
bvw_update_tags (BaconVideoWidget * bvw, GstTagList *tag_list, const gchar *type)
{
  GstTagList **cache = NULL;
  GstTagList *result;

  GST_DEBUG ("Tags: %" GST_PTR_FORMAT, tag_list);

  /* all tags (replace previous tags, title/artist/etc. might change
   * in the middle of a stream, e.g. with radio streams) */
  result = gst_tag_list_merge (bvw->priv->tagcache, tag_list,
                                   GST_TAG_MERGE_REPLACE);
  if (bvw->priv->tagcache)
    gst_tag_list_free (bvw->priv->tagcache);
  bvw->priv->tagcache = result;

  /* media-type-specific tags */
  if (!strcmp (type, "video")) {
    cache = &bvw->priv->videotags;
  } else if (!strcmp (type, "audio")) {
    cache = &bvw->priv->audiotags;
  }

  if (cache) {
    result = gst_tag_list_merge (*cache, tag_list, GST_TAG_MERGE_REPLACE);
    if (*cache)
      gst_tag_list_free (*cache);
    *cache = result;
  }

  /* clean up */
  if (tag_list)
    gst_tag_list_free (tag_list);

  if (bvw->priv->use_type != BVW_USE_TYPE_METADATA)
    bvw_check_for_cover_pixbuf (bvw);

  /* if we're not interactive, we want to announce metadata
   * only later when we can be sure we got it all */
  if (bvw->priv->use_type == BVW_USE_TYPE_VIDEO ||
      bvw->priv->use_type == BVW_USE_TYPE_AUDIO)
    g_signal_emit (bvw, bvw_signals[SIGNAL_GOT_METADATA], 0);
  else if (bvw->priv->use_type == BVW_USE_TYPE_CAPTURE &&
	   bvw->priv->cover_pixbuf != NULL)
    g_signal_emit (bvw, bvw_signals[SIGNAL_GOT_METADATA], 0);
}

static void
update_tags_delayed_data_destroy (UpdateTagsDelayedData *data)
{
  g_slice_free (UpdateTagsDelayedData, data);
}

static gboolean
bvw_update_tags_dispatcher (BaconVideoWidget *self)
{
  UpdateTagsDelayedData *data;

  /* If we take the queue's lock for the entire function call, we can use it to protect tag_update_id too */
  g_async_queue_lock (self->priv->tag_update_queue);

  while ((data = g_async_queue_try_pop_unlocked (self->priv->tag_update_queue)) != NULL) {
    bvw_update_tags (self, data->tags, data->type);
    update_tags_delayed_data_destroy (data);
  }

  self->priv->tag_update_id = 0;
  g_async_queue_unlock (self->priv->tag_update_queue);

  return FALSE;
}

/* Marshal the changed tags to the main thread for updating the GUI
 * and sending the BVW signals */
static void
bvw_update_tags_delayed (BaconVideoWidget *bvw, GstTagList *tags, const gchar *type) {
  UpdateTagsDelayedData *data = g_slice_new0 (UpdateTagsDelayedData);

  data->tags = tags;
  data->type = type;

  g_async_queue_lock (bvw->priv->tag_update_queue);
  g_async_queue_push_unlocked (bvw->priv->tag_update_queue, data);

  if (bvw->priv->tag_update_id == 0)
    bvw->priv->tag_update_id = g_idle_add ((GSourceFunc) bvw_update_tags_dispatcher, bvw);

  g_async_queue_unlock (bvw->priv->tag_update_queue);
}

static void
video_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data)
{
  BaconVideoWidget *bvw = (BaconVideoWidget *) user_data;
  GstTagList *tags = NULL;
  gint current_stream_id = 0;

  g_object_get (G_OBJECT (bvw->priv->play), "current-video", &current_stream_id, NULL);

  /* Only get the updated tags if it's for our current stream id */
  if (current_stream_id != stream_id)
    return;

  g_signal_emit_by_name (G_OBJECT (bvw->priv->play), "get-video-tags", stream_id, &tags);

  if (tags)
    bvw_update_tags_delayed (bvw, tags, "video");
}

static void
audio_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data)
{
  BaconVideoWidget *bvw = (BaconVideoWidget *) user_data;
  GstTagList *tags = NULL;
  gint current_stream_id = 0;

  g_object_get (G_OBJECT (bvw->priv->play), "current-audio", &current_stream_id, NULL);

  /* Only get the updated tags if it's for our current stream id */
  if (current_stream_id != stream_id)
    return;

  g_signal_emit_by_name (G_OBJECT (bvw->priv->play), "get-audio-tags", stream_id, &tags);

  if (tags)
    bvw_update_tags_delayed (bvw, tags, "audio");
}

static void
text_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data)
{
  BaconVideoWidget *bvw = (BaconVideoWidget *) user_data;
  GstTagList *tags = NULL;
  gint current_stream_id = 0;

  g_object_get (G_OBJECT (bvw->priv->play), "current-text", &current_stream_id, NULL);

  /* Only get the updated tags if it's for our current stream id */
  if (current_stream_id != stream_id)
    return;

  g_signal_emit_by_name (G_OBJECT (bvw->priv->play), "get-text-tags", stream_id, &tags);

  if (tags)
    bvw_update_tags_delayed (bvw, tags, "text");
}

static gboolean
bvw_download_buffering_done (BaconVideoWidget *bvw)
{
  /* When we set buffering left to 0, that means it's ready to play */
  if (bvw->priv->buffering_left == 0) {
    GST_DEBUG ("Buffering left is 0, so buffering done");
    return TRUE;
  }
  if (bvw->priv->stream_length <= 0)
    return FALSE;
  /* When queue2 doesn't implement buffering-left, always think
   * it's ready to go */
  if (bvw->priv->buffering_left < 0) {
    GST_DEBUG ("Buffering left not implemented, so buffering done");
    return TRUE;
  }

  if (bvw->priv->buffering_left * 1.1 < bvw->priv->stream_length) {
    GST_DEBUG ("Buffering left: %lld * 1.1 = %lld < %lld",
	       bvw->priv->buffering_left, bvw->priv->buffering_left * 1.1,
	       bvw->priv->stream_length);
    return TRUE;
  }
  return FALSE;
}

static void
bvw_handle_buffering_message (GstMessage * message, BaconVideoWidget *bvw)
{
  GstBufferingMode mode;
  gint percent = 0;

   gst_message_parse_buffering_stats (message, &mode, NULL, NULL, NULL);
   if (mode == GST_BUFFERING_DOWNLOAD) {
     if (bvw->priv->download_buffering == FALSE) {
       bvw->priv->download_buffering = TRUE;

       /* We're not ready to play yet, so pause the stream */
       GST_DEBUG ("Pausing because we're not ready to play the buffer yet");
       gst_element_set_state (GST_ELEMENT (bvw->priv->play), GST_STATE_PAUSED);

       bvw_reconfigure_fill_timeout (bvw, 200);
     }
     bvw->priv->download_buffering_element = g_object_ref (message->src);

     return;
   }

   /* We switched from download mode to normal buffering */
   if (bvw->priv->download_buffering != FALSE) {
     bvw_reconfigure_fill_timeout (bvw, 0);
     bvw->priv->download_buffering = FALSE;
     g_free (bvw->priv->download_filename);
     bvw->priv->download_filename = NULL;
   }

   /* Live, timeshift and stream buffering modes */
  gst_message_parse_buffering (message, &percent);
  g_signal_emit (bvw, bvw_signals[SIGNAL_BUFFERING], 0, percent);

  if (percent >= 100) {
    /* a 100% message means buffering is done */
    bvw->priv->buffering = FALSE;
    /* if the desired state is playing, go back */
    if (bvw->priv->target_state == GST_STATE_PLAYING) {
      GST_DEBUG ("Buffering done, setting pipeline back to PLAYING");
      bacon_video_widget_play (bvw, NULL);
    } else {
      GST_DEBUG ("Buffering done, keeping pipeline PAUSED");
    }
  } else if (bvw->priv->target_state == GST_STATE_PLAYING) {
    GstState cur_state;

    gst_element_get_state (bvw->priv->play, &cur_state, NULL, 0);
    if (cur_state != GST_STATE_PAUSED) {
      GST_DEBUG ("Buffering ... temporarily pausing playback");
      gst_element_set_state (bvw->priv->play, GST_STATE_PAUSED);
    }
    bvw->priv->buffering = TRUE;
  } else {
    GST_LOG ("Buffering ... %d", percent);
  }
}

static void
bvw_bus_message_cb (GstBus * bus, GstMessage * message, gpointer data)
{
  BaconVideoWidget *bvw = (BaconVideoWidget *) data;
  GstMessageType msg_type;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  msg_type = GST_MESSAGE_TYPE (message);

  /* somebody else is handling the message, probably in poll_for_state_change */
  if (bvw->priv->ignore_messages_mask & msg_type) {
    GST_LOG ("Ignoring %s message from element %" GST_PTR_FORMAT
        " as requested: %" GST_PTR_FORMAT, GST_MESSAGE_TYPE_NAME (message),
        message->src, message);
    return;
  }

  if (msg_type != GST_MESSAGE_STATE_CHANGED) {
    gchar *src_name = gst_object_get_name (message->src);
    GST_LOG ("Handling %s message from element %s",
        gst_message_type_get_name (msg_type), src_name);
    g_free (src_name);
  }

  switch (msg_type) {
    case GST_MESSAGE_ERROR: {
      bvw_error_msg (bvw, message);

      if (!bvw_check_missing_plugins_error (bvw, message) &&
	  !bvw_check_missing_auth (bvw, message)) {
        GError *error;

        error = bvw_error_from_gst_error (bvw, message);

        bvw->priv->target_state = GST_STATE_NULL;
        if (bvw->priv->play)
          gst_element_set_state (bvw->priv->play, GST_STATE_NULL);

        bvw->priv->buffering = FALSE;

        g_signal_emit (bvw, bvw_signals[SIGNAL_ERROR], 0,
                       error->message, TRUE, FALSE);

        g_error_free (error);
      }
      break;
    }
    case GST_MESSAGE_WARNING: {
      GST_WARNING ("Warning message: %" GST_PTR_FORMAT, message);
      break;
    }
    case GST_MESSAGE_TAG: 
      /* Ignore TAG messages, we get updated tags from the
       * {audio,video,text}-tags-changed signals of playbin2
       */
      break;
    case GST_MESSAGE_EOS:
      GST_DEBUG ("EOS message");
      /* update slider one last time */
      bvw_query_timeout (bvw);
      if (bvw->priv->eos_id == 0)
        bvw->priv->eos_id = g_idle_add (bvw_signal_eos_delayed, bvw);
      break;
    case GST_MESSAGE_BUFFERING:
      bvw_handle_buffering_message (message, bvw);
      break;
    case GST_MESSAGE_APPLICATION: {
      bvw_handle_application_message (bvw, message);
      break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state;
      gchar *src_name;

      gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

      if (old_state == new_state)
        break;

      /* we only care about playbin (pipeline) state changes */
      if (GST_MESSAGE_SRC (message) != GST_OBJECT (bvw->priv->play))
        break;

      src_name = gst_object_get_name (message->src);
      GST_DEBUG ("%s changed state from %s to %s", src_name,
          gst_element_state_get_name (old_state),
          gst_element_state_get_name (new_state));
      g_free (src_name);

      /* now do stuff */
      if (new_state <= GST_STATE_PAUSED) {
        bvw_query_timeout (bvw);
        bvw_reconfigure_tick_timeout (bvw, 0);
      } else if (new_state > GST_STATE_PAUSED) {
        bvw_reconfigure_tick_timeout (bvw, 200);
      }

      if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED) {
        GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN_CAST (bvw->priv->play),
            GST_DEBUG_GRAPH_SHOW_ALL ^ GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS,
            "totem-prerolled");
	bacon_video_widget_get_stream_length (bvw);
        bvw_update_stream_info (bvw);
        if (!bvw_check_missing_plugins_on_preroll (bvw)) {
          /* show a non-fatal warning message if we can't decode the video */
          bvw_check_if_video_decoder_is_missing (bvw);
        }
	/* Now that we have the length, check whether we wanted
	 * to pause or to stop the pipeline */
        if (bvw->priv->target_state == GST_STATE_PAUSED)
	  bacon_video_widget_pause (bvw);
      } else if (old_state == GST_STATE_PAUSED && new_state == GST_STATE_READY) {
        bvw->priv->media_has_video = FALSE;
        bvw->priv->media_has_audio = FALSE;

        /* clean metadata cache */
        if (bvw->priv->tagcache) {
          gst_tag_list_free (bvw->priv->tagcache);
          bvw->priv->tagcache = NULL;
        }
        if (bvw->priv->audiotags) {
          gst_tag_list_free (bvw->priv->audiotags);
          bvw->priv->audiotags = NULL;
        }
        if (bvw->priv->videotags) {
          gst_tag_list_free (bvw->priv->videotags);
          bvw->priv->videotags = NULL;
        }

        bvw->priv->video_width = 0;
        bvw->priv->video_height = 0;
      }
      break;
    }
    case GST_MESSAGE_ELEMENT:{
      bvw_handle_element_message (bvw, message);
      break;
    }

    case GST_MESSAGE_DURATION: {
      /* force _get_stream_length() to do new duration query */
      bvw->priv->stream_length = 0;
      if (bacon_video_widget_get_stream_length (bvw) == 0) {
        GST_DEBUG ("Failed to query duration after DURATION message?!");
      }
      break;
    }

    case GST_MESSAGE_ASYNC_DONE: {
	gint64 _time;
	/* When a seek has finished, set the playing state again */
	g_mutex_lock (bvw->priv->seek_mutex);

	bvw->priv->seek_req_time = gst_clock_get_internal_time (bvw->priv->clock);
	_time = bvw->priv->seek_time;
	bvw->priv->seek_time = -1;

	g_mutex_unlock (bvw->priv->seek_mutex);

	if (_time >= 0) {
	  GST_DEBUG ("Have an old seek to schedule, doing it now");
	  bacon_video_widget_seek_time_no_lock (bvw, _time, 0, NULL);
	}
      break;
    }

    /* FIXME: at some point we might want to handle CLOCK_LOST and set the
     * pipeline back to PAUSED and then PLAYING again to select a different
     * clock (this seems to trip up rtspsrc though so has to wait until
     * rtspsrc gets fixed) */
    case GST_MESSAGE_CLOCK_PROVIDE:
    case GST_MESSAGE_CLOCK_LOST:
    case GST_MESSAGE_NEW_CLOCK:
    case GST_MESSAGE_STATE_DIRTY:
    case GST_MESSAGE_STREAM_STATUS:
      break;

    default:
      GST_LOG ("Unhandled message: %" GST_PTR_FORMAT, message);
      break;
  }
}

/* FIXME: how to recognise this in 0.9? */
#if 0
static void
group_switch (GstElement *play, BaconVideoWidget *bvw)
{
  GstMessage *msg;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  if (bvw->priv->tagcache) {
    gst_tag_list_free (bvw->priv->tagcache);
    bvw->priv->tagcache = NULL;
  }
  if (bvw->priv->audiotags) {
    gst_tag_list_free (bvw->priv->audiotags);
    bvw->priv->audiotags = NULL;
  }
  if (bvw->priv->videotags) {
    gst_tag_list_free (bvw->priv->videotags);
    bvw->priv->videotags = NULL;
  }

  msg = gst_message_new_application (GST_OBJECT (bvw->priv->play),
      gst_structure_new ("notify-streaminfo", NULL));
  gst_element_post_message (bvw->priv->play, msg);
}
#endif

static void
got_video_size (BaconVideoWidget * bvw)
{
  GstMessage *msg;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  msg = gst_message_new_application (GST_OBJECT (bvw->priv->play),
      gst_structure_new ("video-size", "width", G_TYPE_INT,
          bvw->priv->video_width, "height", G_TYPE_INT,
          bvw->priv->video_height, NULL));
  gst_element_post_message (bvw->priv->play, msg);
}

static void
got_time_tick (GstElement * play, gint64 time_nanos, BaconVideoWidget * bvw)
{
  gboolean seekable;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  bvw->priv->current_time = (gint64) time_nanos / GST_MSECOND;

  if (bvw->priv->stream_length == 0) {
    bvw->priv->current_position = 0;
  } else {
    bvw->priv->current_position =
      (gdouble) bvw->priv->current_time / bvw->priv->stream_length;
  }

  if (bvw->priv->stream_length == 0) {
    seekable = bacon_video_widget_is_seekable (bvw);
  } else {
    if (bvw->priv->seekable == -1)
      g_object_notify (G_OBJECT (bvw), "seekable");
    seekable = TRUE;
  }

  bvw->priv->is_live = (bvw->priv->stream_length == 0);

/*
  GST_DEBUG ("%" GST_TIME_FORMAT ",%" GST_TIME_FORMAT " %s",
      GST_TIME_ARGS (bvw->priv->current_time),
      GST_TIME_ARGS (bvw->priv->stream_length),
      (seekable) ? "TRUE" : "FALSE");
*/
  
  g_signal_emit (bvw, bvw_signals[SIGNAL_TICK], 0,
                 bvw->priv->current_time, bvw->priv->stream_length,
                 bvw->priv->current_position,
                 seekable);
}

static void
bvw_set_device_on_element (BaconVideoWidget * bvw, GstElement * element)
{
  if (bvw->priv->media_device == NULL)
    return;

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (element), "device")) {
    GST_DEBUG ("Setting device to '%s'", bvw->priv->media_device);
    g_object_set (element, "device", bvw->priv->media_device, NULL);
  }
}

static void
bvw_set_user_agent_on_element (BaconVideoWidget * bvw, GstElement * element)
{
  BaconVideoWidgetPrivate *priv = bvw->priv;

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (element), "user-agent") == NULL)
    return;

  GST_DEBUG ("Setting HTTP user-agent to '%s'", priv->user_agent ? priv->user_agent : "(default)");
  g_object_set (element, "user-agent", priv->user_agent, NULL);
}

static void
bvw_set_auth_on_element (BaconVideoWidget * bvw, GstElement * element)
{
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (element), "user-id") == NULL)
    return;
  if (bvw->priv->auth_last_result != G_MOUNT_OPERATION_HANDLED)
    return;
  if (bvw->priv->user_id == NULL || bvw->priv->user_pw == NULL)
    return;

  GST_DEBUG ("Setting username and password");
  g_object_set (element,
		"user-id", bvw->priv->user_id,
		"user-pw", bvw->priv->user_pw,
		NULL);

  g_free (bvw->priv->user_id);
  bvw->priv->user_id = NULL;
  g_free (bvw->priv->user_pw);
  bvw->priv->user_pw = NULL;
}

static void
bvw_set_referrer_on_element (BaconVideoWidget * bvw, GstElement * element)
{
  BaconVideoWidgetPrivate *priv = bvw->priv;
  GstStructure *extra_headers = NULL;

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (element), "extra-headers") == NULL)
    return;

  GST_DEBUG ("Setting HTTP referrer to '%s'", priv->referrer ? priv->referrer : "none");

  g_object_get (element, "extra-headers", &extra_headers, NULL);
  if (extra_headers == NULL) {
    extra_headers = gst_structure_empty_new ("extra-headers");
  }
  g_assert (GST_IS_STRUCTURE (extra_headers));

  if (priv->referrer != NULL) {
    gst_structure_set (extra_headers,
                       "Referer" /* not a typo! */,
                       G_TYPE_STRING,
                       priv->referrer,
                       NULL);
  } else {
    gst_structure_remove_field (extra_headers,
                                "Referer" /* not a typo! */);
  }

  g_object_set (element, "extra-headers", extra_headers, NULL);
  gst_structure_free (extra_headers);
}

static void
playbin_source_notify_cb (GObject *play, GParamSpec *p, BaconVideoWidget *bvw)
{
  BaconVideoWidgetPrivate *priv = bvw->priv;
  GstElement *source = NULL;

  g_object_get (play, "source", &source, NULL);

  if (priv->source != NULL) {
    g_object_unref (priv->source);
  }

  priv->source = source;
  if (source == NULL)
    return;

  GST_DEBUG ("Got source of type %s", G_OBJECT_TYPE_NAME (source));
  bvw_set_device_on_element (bvw, source);
  bvw_set_user_agent_on_element (bvw, source);
  bvw_set_referrer_on_element (bvw, source);
  bvw_set_auth_on_element (bvw, source);
}

static void
playbin_deep_notify_cb (GstObject  *gstobject,
			GstObject  *prop_object,
			GParamSpec *prop,
			BaconVideoWidget *bvw)
{
  if (g_str_equal (prop->name, "temp-location") == FALSE)
    return;

  g_free (bvw->priv->download_filename);
  bvw->priv->download_filename = NULL;
  g_object_get (G_OBJECT (prop_object),
		"temp-location", &bvw->priv->download_filename,
		NULL);
}

static gboolean
bvw_query_timeout (BaconVideoWidget *bvw)
{
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 prev_len = -1;
  gint64 pos = -1, len = -1;
  
  /* check length/pos of stream */
  prev_len = bvw->priv->stream_length;
  if (gst_element_query_duration (bvw->priv->play, &fmt, &len)) {
    if (len != -1 && fmt == GST_FORMAT_TIME) {
      bvw->priv->stream_length = len / GST_MSECOND;
      if (bvw->priv->stream_length != prev_len) {
	 if (bvw->priv->use_type == BVW_USE_TYPE_AUDIO ||
	     bvw->priv->use_type == BVW_USE_TYPE_VIDEO) {
	   g_signal_emit (bvw, bvw_signals[SIGNAL_GOT_METADATA], 0, NULL);
	 }
      }
    }
  } else {
    GST_DEBUG ("could not get duration");
  }

  if (gst_element_query_position (bvw->priv->play, &fmt, &pos)) {
    if (pos != -1 && fmt == GST_FORMAT_TIME) {
      got_time_tick (GST_ELEMENT (bvw->priv->play), pos, bvw);
    }
  } else {
    GST_DEBUG ("could not get position");
  }

  return TRUE;
}

static gboolean
bvw_query_buffering_timeout (BaconVideoWidget *bvw)
{
  GstQuery *query;
  GstElement *element;

  element = bvw->priv->download_buffering_element;
  if (element == NULL)
    element = bvw->priv->play;

  query = gst_query_new_buffering (GST_FORMAT_PERCENT);
  if (gst_element_query (element, query)) {
    gint64 start, stop;
    GstFormat format;
    gdouble fill;
    gboolean busy;
    gint percent;

    gst_query_parse_buffering_stats (query, NULL, NULL, NULL, &bvw->priv->buffering_left);
    gst_query_parse_buffering_percent (query, &busy, &percent);
    gst_query_parse_buffering_range (query, &format, &start, &stop, NULL);

    GST_DEBUG ("start %" G_GINT64_FORMAT ", stop %" G_GINT64_FORMAT
	       ", buffering left %" G_GINT64_FORMAT ", percent %d%%",
	       start, stop, bvw->priv->buffering_left, percent);

    if (stop != -1)
      fill = (gdouble) stop / GST_FORMAT_PERCENT_MAX;
    else
      fill = -1.0;

    GST_DEBUG ("download buffer filled up to %f%% (element: %s)", fill * 100.0,
	       G_OBJECT_TYPE_NAME (element));

    g_signal_emit (bvw, bvw_signals[SIGNAL_DOWNLOAD_BUFFERING], 0, fill);

    /* Nothing left to buffer when fill is 100% */
    if (fill == 1.0)
      bvw->priv->buffering_left = 0;

    /* Start playing when we've downloaded enough */
    if (bvw_download_buffering_done (bvw) != FALSE &&
	bvw->priv->target_state == GST_STATE_PLAYING) {
      GST_DEBUG ("Starting playback because the download buffer is filled enough");
      bacon_video_widget_play (bvw, NULL);
    }

    /* Finished buffering, so don't run the timeout anymore */
    if (fill == 1.0) {
      bvw->priv->fill_id = 0;
      gst_query_unref (query);
      if (bvw->priv->download_buffering_element != NULL) {
	g_object_unref (bvw->priv->download_buffering_element);
	bvw->priv->download_buffering_element = NULL;
      }

      /* Tell the front-end about the downloaded file */
      g_object_notify (G_OBJECT (bvw), "download-filename");

      return FALSE;
    }
  }
  gst_query_unref (query);

  return TRUE;
}

static void
caps_set (GObject * obj,
    GParamSpec * pspec, BaconVideoWidget * bvw)
{
  GstPad *pad = GST_PAD (obj);
  GstStructure *s;
  GstCaps *caps;

  if (!(caps = gst_pad_get_negotiated_caps (pad)))
    return;

  /* Get video decoder caps */
  s = gst_caps_get_structure (caps, 0);
  if (s) {
    const GValue *movie_par;

    /* We need at least width/height and framerate */
    if (!(gst_structure_get_fraction (s, "framerate", &bvw->priv->video_fps_n, 
          &bvw->priv->video_fps_d) &&
          gst_structure_get_int (s, "width", &bvw->priv->video_width) &&
          gst_structure_get_int (s, "height", &bvw->priv->video_height)))
      return;
    
    /* Get the movie PAR if available */
    movie_par = gst_structure_get_value (s, "pixel-aspect-ratio");
    if (movie_par) {
      bvw->priv->movie_par_n = gst_value_get_fraction_numerator (movie_par);
      bvw->priv->movie_par_d = gst_value_get_fraction_denominator (movie_par);
    }
    else {
      /* Square pixels */
      bvw->priv->movie_par_n = 1;
      bvw->priv->movie_par_d = 1;
    }
    
    /* Now set for real */
    bacon_video_widget_set_aspect_ratio (bvw, bvw->priv->ratio_type);
  }

  gst_caps_unref (caps);
}

static void get_visualization_size (BaconVideoWidget *bvw,
                                    int *w, int *h, gint *fps_n, gint *fps_d);

static void
parse_stream_info (BaconVideoWidget *bvw)
{
  GstPad *videopad = NULL;
  gint n_audio, n_video;

  g_object_get (G_OBJECT (bvw->priv->play), "n-audio", &n_audio,
      "n-video", &n_video, NULL);

  bvw->priv->media_has_video = FALSE;
  if (n_video > 0) {
    gint i;

    bvw->priv->media_has_video = TRUE;
    if (bvw->priv->video_window)
      gdk_window_show (bvw->priv->video_window);

    for (i = 0; i < n_video && videopad == NULL; i++)
      g_signal_emit_by_name (bvw->priv->play, "get-video-pad", i, &videopad);
  }

  bvw->priv->media_has_audio = FALSE;
  if (n_audio > 0) {
    bvw->priv->media_has_audio = TRUE;
    if (!bvw->priv->media_has_video && bvw->priv->video_window) {
      gint flags;

      g_object_get (bvw->priv->play, "flags", &flags, NULL);
      if (bvw->priv->show_vfx) {
        gdk_window_show (bvw->priv->video_window);
	gtk_widget_set_double_buffered (GTK_WIDGET (bvw), FALSE);
	flags |= GST_PLAY_FLAG_VIS;
      } else {
        gdk_window_hide (bvw->priv->video_window);
	gtk_widget_set_double_buffered (GTK_WIDGET (bvw), TRUE);
	flags &= ~GST_PLAY_FLAG_VIS;
      }
      g_object_set (bvw->priv->play, "flags", flags, NULL);
    }
  }

  if (videopad) {
    GstCaps *caps;

    if ((caps = gst_pad_get_negotiated_caps (videopad))) {
      caps_set (G_OBJECT (videopad), NULL, bvw);
      gst_caps_unref (caps);
    }
    g_signal_connect (videopad, "notify::caps",
        G_CALLBACK (caps_set), bvw);
    gst_object_unref (videopad);
  } else if (bvw->priv->show_vfx) {
    get_visualization_size (bvw, &bvw->priv->video_width,
        &bvw->priv->video_height, NULL, NULL);
  }
}

static void
playbin_stream_changed_cb (GstElement * obj, gpointer data)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (data);
  GstMessage *msg;

  /* we're being called from the streaming thread, so don't do anything here */
  GST_LOG ("streams have changed");
  msg = gst_message_new_application (GST_OBJECT (bvw->priv->play),
      gst_structure_new ("stream-changed", NULL));
  gst_element_post_message (bvw->priv->play, msg);
}

static void
bacon_video_widget_finalize (GObject * object)
{
  BaconVideoWidget *bvw = (BaconVideoWidget *) object;

  GST_DEBUG ("finalizing");

  if (bvw->priv->bus) {
    /* make bus drop all messages to make sure none of our callbacks is ever
     * called again (main loop might be run again to display error dialog) */
    gst_bus_set_flushing (bvw->priv->bus, TRUE);

    if (bvw->priv->sig_bus_sync)
      g_signal_handler_disconnect (bvw->priv->bus, bvw->priv->sig_bus_sync);

    if (bvw->priv->sig_bus_async)
      g_signal_handler_disconnect (bvw->priv->bus, bvw->priv->sig_bus_async);

    gst_object_unref (bvw->priv->bus);
    bvw->priv->bus = NULL;
  }

  g_free (bvw->priv->user_agent);
  bvw->priv->user_agent = NULL;

  g_free (bvw->priv->media_device);
  bvw->priv->media_device = NULL;

  g_free (bvw->priv->referrer);
  bvw->priv->referrer = NULL;

  g_free (bvw->priv->mrl);
  bvw->priv->mrl = NULL;

  g_free (bvw->priv->vis_element_name);
  bvw->priv->vis_element_name = NULL;

  if (bvw->priv->clock) {
    g_object_unref (bvw->priv->clock);
    bvw->priv->clock = NULL;
  }

  if (bvw->priv->vis_plugins_list) {
    g_list_free (bvw->priv->vis_plugins_list);
    bvw->priv->vis_plugins_list = NULL;
  }

  if (bvw->priv->source != NULL) {
    g_object_unref (bvw->priv->source);
    bvw->priv->source = NULL;
  }

  if (bvw->priv->ready_idle_id) {
    g_source_remove (bvw->priv->ready_idle_id);
    bvw->priv->ready_idle_id = 0;
  }

  if (bvw->priv->play != NULL && GST_IS_ELEMENT (bvw->priv->play)) {
    gst_element_set_state (bvw->priv->play, GST_STATE_NULL);
    gst_object_unref (bvw->priv->play);
    bvw->priv->play = NULL;
  }

  if (bvw->priv->update_id) {
    g_source_remove (bvw->priv->update_id);
    bvw->priv->update_id = 0;
  }

  if (bvw->priv->interface_update_id) {
    g_source_remove (bvw->priv->interface_update_id);
    bvw->priv->interface_update_id = 0;
  }

  if (bvw->priv->tagcache) {
    gst_tag_list_free (bvw->priv->tagcache);
    bvw->priv->tagcache = NULL;
  }
  if (bvw->priv->audiotags) {
    gst_tag_list_free (bvw->priv->audiotags);
    bvw->priv->audiotags = NULL;
  }
  if (bvw->priv->videotags) {
    gst_tag_list_free (bvw->priv->videotags);
    bvw->priv->videotags = NULL;
  }

  if (bvw->priv->tag_update_id != 0)
    g_source_remove (bvw->priv->tag_update_id);
  g_async_queue_unref (bvw->priv->tag_update_queue);

  if (bvw->priv->eos_id != 0)
    g_source_remove (bvw->priv->eos_id);

  if (bvw->priv->cursor != NULL) {
    gdk_cursor_unref (bvw->priv->cursor);
    bvw->priv->cursor = NULL;
  }

  if (bvw->priv->mount_cancellable) {
    g_cancellable_cancel (bvw->priv->mount_cancellable);
    g_object_unref (bvw->priv->mount_cancellable);
    bvw->priv->mount_cancellable = NULL;
  }

  g_mutex_free (bvw->priv->lock);
  g_mutex_free (bvw->priv->seek_mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bacon_video_widget_set_property (GObject * object, guint property_id,
                                 const GValue * value, GParamSpec * pspec)
{
  BaconVideoWidget *bvw;

  bvw = BACON_VIDEO_WIDGET (object);

  switch (property_id) {
    case PROP_LOGO_MODE:
      bacon_video_widget_set_logo_mode (bvw,
      g_value_get_boolean (value));
      break;
    case PROP_REFERRER:
      bacon_video_widget_set_referrer (bvw,
      g_value_get_string (value));
      break;
    case PROP_SHOW_CURSOR:
      bacon_video_widget_set_show_cursor (bvw,
      g_value_get_boolean (value));
      break;
    case PROP_SHOW_VISUALS:
      bacon_video_widget_set_show_visuals (bvw,
      g_value_get_boolean (value));
      break;
    case PROP_USER_AGENT:
      bacon_video_widget_set_user_agent (bvw,
      g_value_get_string (value));
      break;
    case PROP_VOLUME:
      bacon_video_widget_set_volume (bvw, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bacon_video_widget_get_property (GObject * object, guint property_id,
                                 GValue * value, GParamSpec * pspec)
{
  BaconVideoWidget *bvw;

  bvw = BACON_VIDEO_WIDGET (object);

  switch (property_id) {
    case PROP_LOGO_MODE:
      g_value_set_boolean (value,
      bacon_video_widget_get_logo_mode (bvw));
      break;
    case PROP_POSITION:
      g_value_set_double (value, bacon_video_widget_get_position (bvw));
      break;
    case PROP_STREAM_LENGTH:
      g_value_set_int64 (value,
      bacon_video_widget_get_stream_length (bvw));
      break;
    case PROP_PLAYING:
      g_value_set_boolean (value,
      bacon_video_widget_is_playing (bvw));
      break;
    case PROP_REFERRER:
      g_value_set_string (value, bvw->priv->referrer);
      break;
    case PROP_SEEKABLE:
      g_value_set_boolean (value,
      bacon_video_widget_is_seekable (bvw));
      break;
    case PROP_SHOW_CURSOR:
      g_value_set_boolean (value,
      bacon_video_widget_get_show_cursor (bvw));
      break;
    case PROP_USER_AGENT:
      g_value_set_string (value, bvw->priv->user_agent);
      break;
    case PROP_VOLUME:
      g_value_set_double (value, bvw->priv->volume);
      break;
    case PROP_DOWNLOAD_FILENAME:
      g_value_set_string (value, bvw->priv->download_filename);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

/* ============================================================= */
/*                                                               */
/*                       Public Methods                          */
/*                                                               */
/* ============================================================= */

/**
 * bacon_video_widget_get_backend_name:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the name string for @bvw. For the GStreamer backend, it is the output
 * of gst_version_string(). *
 * Return value: the backend's name; free with g_free()
 **/
char *
bacon_video_widget_get_backend_name (BaconVideoWidget * bvw)
{
  return gst_version_string ();
}

/**
 * bacon_video_widget_get_subtitle:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the index of the current subtitles.
 *
 * If the widget is not playing, %-2 will be returned. If no subtitles are
 * being used, %-1 is returned.
 *
 * Return value: the subtitle index
 **/
int
bacon_video_widget_get_subtitle (BaconVideoWidget * bvw)
{
  int subtitle = -1;
  gint flags;

  g_return_val_if_fail (bvw != NULL, -2);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), -2);
  g_return_val_if_fail (bvw->priv->play != NULL, -2);

  g_object_get (bvw->priv->play, "flags", &flags, NULL);

  if ((flags & GST_PLAY_FLAG_TEXT) == 0)
    return -2;

  g_object_get (G_OBJECT (bvw->priv->play), "current-text", &subtitle, NULL);

  return subtitle;
}

/**
 * bacon_video_widget_set_subtitle:
 * @bvw: a #BaconVideoWidget
 * @subtitle: a subtitle index
 *
 * Sets the subtitle index for @bvw. If @subtitle is %-1, no subtitles will
 * be used.
 **/
void
bacon_video_widget_set_subtitle (BaconVideoWidget * bvw, int subtitle)
{
  GstTagList *tags;
  gint flags;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (bvw->priv->play != NULL);

  g_object_get (bvw->priv->play, "flags", &flags, NULL);

  if (subtitle == -2) {
    flags &= ~GST_PLAY_FLAG_TEXT;
    subtitle = -1;
  } else {
    flags |= GST_PLAY_FLAG_TEXT;
  }
  
  g_object_set (bvw->priv->play, "flags", flags, "current-text", subtitle, NULL);
  
  if (flags & GST_PLAY_FLAG_TEXT) {
    g_object_get (bvw->priv->play, "current-text", &subtitle, NULL);

    g_signal_emit_by_name (G_OBJECT (bvw->priv->play), "get-text-tags", subtitle, &tags);
    bvw_update_tags (bvw, tags, "text");
  }
}

/**
 * bacon_video_widget_has_next_track:
 * @bvw: a #BaconVideoWidget
 *
 * Determines whether there is another track after the current one, typically
 * as a chapter on a DVD.
 *
 * Return value: %TRUE if there is another track, %FALSE otherwise
 **/
gboolean
bacon_video_widget_has_next_track (BaconVideoWidget *bvw)
{
  //FIXME
  return TRUE;
}

/**
 * bacon_video_widget_has_previous_track:
 * @bvw: a #BaconVideoWidget
 *
 * Determines whether there is another track before the current one, typically
 * as a chapter on a DVD.
 *
 * Return value: %TRUE if there is another track, %FALSE otherwise
 **/
gboolean
bacon_video_widget_has_previous_track (BaconVideoWidget *bvw)
{
  //FIXME
  return TRUE;
}

static GList *
get_lang_list_for_type (BaconVideoWidget * bvw, const gchar * type_name)
{
  GList *ret = NULL;
  gint num = 1;

  if (g_str_equal (type_name, "AUDIO")) {
    gint i, n;

    g_object_get (G_OBJECT (bvw->priv->play), "n-audio", &n, NULL);
    if (n == 0)
      return NULL;

    for (i = 0; i < n; i++) {
      GstTagList *tags = NULL;

      g_signal_emit_by_name (G_OBJECT (bvw->priv->play), "get-audio-tags",
          i, &tags);

      if (tags) {
        gchar *lc = NULL, *cd = NULL;

	gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &lc);
	gst_tag_list_get_string (tags, GST_TAG_CODEC, &lc);

        if (lc) {
	  ret = g_list_prepend (ret, lc);
	  g_free (cd);
	} else if (cd) {
	  ret = g_list_prepend (ret, cd);
	} else {
	  ret = g_list_prepend (ret, g_strdup_printf (_("Audio Track #%d"), num++));
	}
	gst_tag_list_free (tags);
      } else {
	ret = g_list_prepend (ret, g_strdup_printf (_("Audio Track #%d"), num++));
      }
    }
  } else if (g_str_equal (type_name, "TEXT")) {
    gint i, n = 0;

    g_object_get (G_OBJECT (bvw->priv->play), "n-text", &n, NULL);
    if (n == 0)
      return NULL;

    for (i = 0; i < n; i++) {
      GstTagList *tags = NULL;

      g_signal_emit_by_name (G_OBJECT (bvw->priv->play), "get-text-tags",
          i, &tags);

      if (tags) {
        gchar *lc = NULL, *cd = NULL;

	gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &lc);
	gst_tag_list_get_string (tags, GST_TAG_CODEC, &lc);

        if (lc) {
	  ret = g_list_prepend (ret, lc);
	  g_free (cd);
	} else if (cd) {
	  ret = g_list_prepend (ret, cd);
	} else {
	  ret = g_list_prepend (ret, g_strdup_printf (_("Subtitle #%d"), num++));
	}
	gst_tag_list_free (tags);
      } else {
	ret = g_list_prepend (ret, g_strdup_printf (_("Subtitle #%d"), num++));
      }
    }
  } else {
    g_critical ("Invalid stream type '%s'", type_name);
    return NULL;
  }

  return g_list_reverse (ret);
}

/**
 * bacon_video_widget_get_subtitles:
 * @bvw: a #BaconVideoWidget
 *
 * Returns a list of subtitle tags, each in the form <literal>TEXT <replaceable>x</replaceable></literal>,
 * where <replaceable>x</replaceable> is the subtitle index.
 *
 * Return value: a #GList of subtitle tags, or %NULL; free each element with g_free() and the list with g_list_free()
 **/
GList *
bacon_video_widget_get_subtitles (BaconVideoWidget * bvw)
{
  GList *list;

  g_return_val_if_fail (bvw != NULL, NULL);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), NULL);
  g_return_val_if_fail (bvw->priv->play != NULL, NULL);

  list = get_lang_list_for_type (bvw, "TEXT");

  return list;
}

/**
 * bacon_video_widget_get_languages:
 * @bvw: a #BaconVideoWidget
 *
 * Returns a list of audio language tags, each in the form <literal>AUDIO <replaceable>x</replaceable></literal>,
 * where <replaceable>x</replaceable> is the language index.
 *
 * Return value: a #GList of audio language tags, or %NULL; free each element with g_free() and the list with g_list_free()
 **/
GList *
bacon_video_widget_get_languages (BaconVideoWidget * bvw)
{
  GList *list;

  g_return_val_if_fail (bvw != NULL, NULL);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), NULL);
  g_return_val_if_fail (bvw->priv->play != NULL, NULL);

  list = get_lang_list_for_type (bvw, "AUDIO");

  /* When we have only one language, we don't need to show
   * any languages, we default to the only track */
  if (g_list_length (list) == 1) {
    g_free (list->data);
    g_list_free (list);
    list = NULL;
  }

  return list;
}

/**
 * bacon_video_widget_get_language:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the index of the current audio language.
 *
 * If the widget is not playing, or the default language is in use, %-1 will be returned.
 *
 * Return value: the audio language index
 **/
int
bacon_video_widget_get_language (BaconVideoWidget * bvw)
{
  int language = -1;

  g_return_val_if_fail (bvw != NULL, -1);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), -1);
  g_return_val_if_fail (bvw->priv->play != NULL, -1);

  g_object_get (G_OBJECT (bvw->priv->play), "current-audio", &language, NULL);

  return language;
}

/**
 * bacon_video_widget_set_language:
 * @bvw: a #BaconVideoWidget
 * @language: an audio language index
 *
 * Sets the audio language index for @bvw. If @language is %-1, the default language will
 * be used.
 **/
void
bacon_video_widget_set_language (BaconVideoWidget * bvw, int language)
{
  GstTagList *tags;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (bvw->priv->play != NULL);

  if (language == -1)
    language = 0;
  else if (language == -2)
    language = -1;

  GST_DEBUG ("setting language to %d", language);

  g_object_set (bvw->priv->play, "current-audio", language, NULL);

  g_object_get (bvw->priv->play, "current-audio", &language, NULL);
  GST_DEBUG ("current-audio now: %d", language);

  g_signal_emit_by_name (G_OBJECT (bvw->priv->play), "get-audio-tags", language, &tags);
  bvw_update_tags (bvw, tags, "audio");

  /* so it updates its metadata for the newly-selected stream */
  g_signal_emit (bvw, bvw_signals[SIGNAL_GOT_METADATA], 0, NULL);
  g_signal_emit (bvw, bvw_signals[SIGNAL_CHANNELS_CHANGE], 0);
}

static guint
connection_speed_enum_to_kbps (gint speed)
{
  static const guint conv_table[] = { 14400, 19200, 28800, 33600, 34400, 56000,
      112000, 256000, 384000, 512000, 1536000, 10752000 };

  g_return_val_if_fail (speed >= 0 && (guint) speed < G_N_ELEMENTS (conv_table), 0);

  /* must round up so that the correct streams are chosen and not ignored
   * due to rounding errors when doing kbps <=> bps */
  return (conv_table[speed] / 1000) +
    (((conv_table[speed] % 1000) != 0) ? 1 : 0);
}

/**
 * bacon_video_widget_get_connection_speed:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the current connection speed, where %0 is the lowest speed
 * and %11 is the highest.
 *
 * Return value: the connection speed index
 **/
int
bacon_video_widget_get_connection_speed (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (bvw != NULL, 0);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), 0);

  return bvw->priv->connection_speed;
}

/**
 * bacon_video_widget_set_connection_speed:
 * @bvw: a #BaconVideoWidget
 * @speed: the connection speed index
 *
 * Sets the connection speed from the given @speed index, where %0 is the lowest speed
 * and %11 is the highest.
 **/
void
bacon_video_widget_set_connection_speed (BaconVideoWidget * bvw, int speed)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  if (bvw->priv->connection_speed != speed) {
    bvw->priv->connection_speed = speed;
    mateconf_client_set_int (bvw->priv->gc,
         MATECONF_PREFIX"/connection_speed", speed, NULL);
  }

  if (bvw->priv->play != NULL &&
      g_object_class_find_property (G_OBJECT_GET_CLASS (bvw->priv->play), "connection-speed")) {
    guint kbps = connection_speed_enum_to_kbps (speed);

    GST_LOG ("Setting connection speed %d (= %d kbps)", speed, kbps);
    g_object_set (bvw->priv->play, "connection-speed", kbps, NULL);
  }
}

/**
 * bacon_video_widget_set_deinterlacing:
 * @bvw: a #BaconVideoWidget
 * @deinterlace: %TRUE if videos should be automatically deinterlaced, %FALSE otherwise
 *
 * Sets whether the widget should deinterlace videos.
 **/
void
bacon_video_widget_set_deinterlacing (BaconVideoWidget * bvw,
                                      gboolean deinterlace)
{
  gint flags;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  g_object_get (bvw->priv->play, "flags", &flags, NULL);
  if (deinterlace)
    flags |= GST_PLAY_FLAG_DEINTERLACE;
  else
    flags &= ~GST_PLAY_FLAG_DEINTERLACE;
  g_object_set (bvw->priv->play, "flags", flags, NULL);
}

/**
 * bacon_video_widget_get_deinterlacing:
 * @bvw: a #BaconVideoWidget
 *
 * Returns whether deinterlacing of videos is enabled for this widget.
 *
 * Return value: %TRUE if automatic deinterlacing is enabled, %FALSE otherwise
 **/
gboolean
bacon_video_widget_get_deinterlacing (BaconVideoWidget * bvw)
{
  gint flags;

  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);

  g_object_get (bvw->priv->play, "flags", &flags, NULL);

  return !!(flags & GST_PLAY_FLAG_DEINTERLACE);
}

static gint
get_num_audio_channels (BaconVideoWidget * bvw)
{
  gint channels;

  switch (bvw->priv->speakersetup) {
    case BVW_AUDIO_SOUND_STEREO:
      channels = 2;
      break;
    case BVW_AUDIO_SOUND_4CHANNEL:
      channels = 4;
      break;
    case BVW_AUDIO_SOUND_5CHANNEL:
      channels = 5;
      break;
    case BVW_AUDIO_SOUND_41CHANNEL:
      /* so alsa has this as 5.1, but empty center speaker. We don't really
       * do that yet. ;-). So we'll take the placebo approach. */
    case BVW_AUDIO_SOUND_51CHANNEL:
      channels = 6;
      break;
    case BVW_AUDIO_SOUND_AC3PASSTHRU:
    default:
      g_return_val_if_reached (-1);
  }

  return channels;
}

static GstCaps *
fixate_to_num (const GstCaps * in_caps, gint channels)
{
  gint n, count;
  GstStructure *s;
  const GValue *v;
  GstCaps *out_caps;

  out_caps = gst_caps_copy (in_caps);

  count = gst_caps_get_size (out_caps);
  for (n = 0; n < count; n++) {
    s = gst_caps_get_structure (out_caps, n);
    v = gst_structure_get_value (s, "channels");
    if (!v)
      continue;

    /* get channel count (or list of ~) */
    gst_structure_fixate_field_nearest_int (s, "channels", channels);
  }

  return out_caps;
}

static void
set_audio_filter (BaconVideoWidget *bvw)
{
  gint channels;
  GstCaps *caps, *res;
  GstPad *pad;

  /* reset old */
  g_object_set (bvw->priv->audio_capsfilter, "caps", NULL, NULL);

  /* construct possible caps to filter down to our chosen caps */
  /* Start with what the audio sink supports, but limit the allowed
   * channel count to our speaker output configuration */
  pad = gst_element_get_static_pad (bvw->priv->audio_capsfilter, "src");
  caps = gst_pad_peer_get_caps (pad);        
  gst_object_unref (pad);

  if ((channels = get_num_audio_channels (bvw)) == -1)
    return;

  res = fixate_to_num (caps, channels);
  gst_caps_unref (caps);

  /* set */
  if (res && gst_caps_is_empty (res)) {
    gst_caps_unref (res);
    res = NULL;
  }
  g_object_set (bvw->priv->audio_capsfilter, "caps", res, NULL);

  if (res) {
    gst_caps_unref (res);
  }

  /* reset */
  pad = gst_element_get_static_pad (bvw->priv->audio_capsfilter, "src");
  gst_pad_set_caps (pad, NULL);
  gst_object_unref (pad);
}

/**
 * bacon_video_widget_get_audio_out_type:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the current audio output type (e.g. how many speaker channels)
 * from #BaconVideoWidgetAudioOutType.
 *
 * Return value: the audio output type, or %-1
 **/
BvwAudioOutType
bacon_video_widget_get_audio_out_type (BaconVideoWidget *bvw)
{
  g_return_val_if_fail (bvw != NULL, -1);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), -1);

  return bvw->priv->speakersetup;
}

/**
 * bacon_video_widget_set_audio_out_type:
 * @bvw: a #BaconVideoWidget
 * @type: the new audio output type
 *
 * Sets the audio output type (number of speaker channels) in the video widget,
 * and stores it in MateConf.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
gboolean
bacon_video_widget_set_audio_out_type (BaconVideoWidget *bvw,
                                       BvwAudioOutType type)
{
  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);

  if (type == bvw->priv->speakersetup)
    return FALSE;
  else if (type == BVW_AUDIO_SOUND_AC3PASSTHRU)
    return FALSE;

  bvw->priv->speakersetup = type;
  mateconf_client_set_int (bvw->priv->gc,
      MATECONF_PREFIX"/audio_output_type", type, NULL);

  set_audio_filter (bvw);

  return FALSE;
}

/* =========================================== */
/*                                             */
/*               Play/Pause, Stop              */
/*                                             */
/* =========================================== */

static GError*
bvw_error_from_gst_error (BaconVideoWidget *bvw, GstMessage * err_msg)
{
  const gchar *src_typename;
  GError *ret = NULL;
  GError *e = NULL;

  GST_LOG ("resolving error message %" GST_PTR_FORMAT, err_msg);

  src_typename = (err_msg->src) ? G_OBJECT_TYPE_NAME (err_msg->src) : NULL;

  gst_message_parse_error (err_msg, &e, NULL);

  if (is_error (e, RESOURCE, NOT_FOUND) ||
      is_error (e, RESOURCE, OPEN_READ)) {
#if 0
    if (strchr (mrl, ':') &&
        (g_str_has_prefix (mrl, "dvd") ||
         g_str_has_prefix (mrl, "cd") ||
         g_str_has_prefix (mrl, "vcd"))) {
      ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_INVALID_DEVICE,
                                 e->message);
    } else {
#endif
      if (e->code == GST_RESOURCE_ERROR_NOT_FOUND) {
        if (GST_IS_BASE_AUDIO_SINK (err_msg->src)) {
          ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_AUDIO_PLUGIN,
              _("The requested audio output was not found. "
                "Please select another audio output in the Multimedia "
                "Systems Selector."));
        } else {
          ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_FILE_NOT_FOUND,
                                     _("Location not found."));
        }
      } else {
        ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_FILE_PERMISSION,
            _("Could not open location; "
              "you might not have permission to open the file."));
      }
#if 0
    }
#endif
  } else if (is_error (e, RESOURCE, BUSY)) {
    if (GST_IS_VIDEO_SINK (err_msg->src)) {
      /* a somewhat evil check, but hey.. */
      ret = g_error_new_literal (BVW_ERROR,
          BVW_ERROR_VIDEO_PLUGIN,
          _("The video output is in use by another application. "
            "Please close other video applications, or select "
            "another video output in the Multimedia Systems Selector."));
    } else if (GST_IS_BASE_AUDIO_SINK (err_msg->src)) {
      ret = g_error_new_literal (BVW_ERROR,
          BVW_ERROR_AUDIO_BUSY,
           _("The audio output is in use by another application. "
             "Please select another audio output in the Multimedia Systems Selector. "
             "You may want to consider using a sound server."));
    }
  } else if (e->domain == GST_RESOURCE_ERROR) {
    ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_FILE_GENERIC,
                               e->message);
  } else if (is_error (e, CORE, MISSING_PLUGIN) ||
             is_error (e, STREAM, CODEC_NOT_FOUND)) {
    if (bvw->priv->missing_plugins != NULL) {
      gchar **descs, *msg = NULL;
      guint num;

      descs = bvw_get_missing_plugins_descriptions (bvw->priv->missing_plugins);
      num = g_list_length (bvw->priv->missing_plugins);

      if (is_error (e, CORE, MISSING_PLUGIN)) {
        /* should be exactly one missing thing (source or converter) */
        msg = g_strdup_printf (_("The playback of this movie requires a %s "
          "plugin which is not installed."), descs[0]);
      } else {
        gchar *desc_list;

        desc_list = g_strjoinv ("\n", descs);
        msg = g_strdup_printf (ngettext (_("The playback of this movie "
            "requires a %s plugin which is not installed."), _("The playback "
            "of this movie requires the following decoders which are not "
            "installed:\n\n%s"), num), (num == 1) ? descs[0] : desc_list);
        g_free (desc_list);
      }
      ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_CODEC_NOT_HANDLED, msg);
      g_free (msg);
      g_strfreev (descs);
    } else {
      GST_LOG ("no missing plugin messages, posting generic error");
      ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_CODEC_NOT_HANDLED,
          e->message);
    }
  } else if (is_error (e, STREAM, WRONG_TYPE) ||
             is_error (e, STREAM, NOT_IMPLEMENTED)) {
    if (src_typename) {
      ret = g_error_new (BVW_ERROR, BVW_ERROR_CODEC_NOT_HANDLED, "%s: %s",
          src_typename, e->message);
    } else {
      ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_CODEC_NOT_HANDLED,
          e->message);
    }
  } else if (is_error (e, STREAM, FAILED) &&
             src_typename && strncmp (src_typename, "GstTypeFind", 11) == 0) {
    ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_READ_ERROR,
        _("Cannot play this file over the network. "
          "Try downloading it to disk first."));
  } else {
    /* generic error, no code; take message */
    ret = g_error_new_literal (BVW_ERROR, BVW_ERROR_GENERIC,
                               e->message);
  }
  g_error_free (e);
  bvw_clear_missing_plugins_messages (bvw);

  return ret;
}

static gboolean
poll_for_state_change_full (BaconVideoWidget *bvw, GstElement *element,
    GstState state, GstMessage ** err_msg, gint64 timeout)
{
  GstBus *bus;
  GstMessageType events, saved_events;

  g_assert (err_msg != NULL);

  bus = gst_element_get_bus (element);

  events = GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS;

  saved_events = bvw->priv->ignore_messages_mask;

  if (element != NULL && element == bvw->priv->play) {
    /* we do want the main handler to process state changed messages for
     * playbin as well, otherwise it won't hook up the timeout etc. */
    bvw->priv->ignore_messages_mask |= (events ^ GST_MESSAGE_STATE_CHANGED);
  } else {
    bvw->priv->ignore_messages_mask |= events;
  }

  while (TRUE) {
    GstMessage *message;
    GstElement *src;

    message = gst_bus_poll (bus, events, timeout);
    
    if (!message)
      goto timed_out;
    
    src = (GstElement*)GST_MESSAGE_SRC (message);

    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old, new, pending;

      if (src == element) {
        gst_message_parse_state_changed (message, &old, &new, &pending);
        if (new == state) {
          gst_message_unref (message);
          goto success;
        }
      }
      break;
    }
    case GST_MESSAGE_ERROR: {
      bvw_error_msg (bvw, message);
      *err_msg = message;
      message = NULL;
      goto error;
      break;
    }
    case GST_MESSAGE_EOS: {
      GError *e = NULL;

      gst_message_unref (message);
      e = g_error_new_literal (BVW_ERROR, BVW_ERROR_FILE_GENERIC,
          _("Media file could not be played."));
      *err_msg = gst_message_new_error (GST_OBJECT (bvw->priv->play), e, NULL);
      g_error_free (e);
      goto error;
      break;
    }
    default:
      g_assert_not_reached ();
      break;
    }

    gst_message_unref (message);
  }
    
  g_assert_not_reached ();

success:
  /* state change succeeded */
  GST_DEBUG ("state change to %s succeeded", gst_element_state_get_name (state));
  bvw->priv->ignore_messages_mask = saved_events;
  return TRUE;

timed_out:
  /* it's taking a long time to open -- just tell totem it was ok, this allows
   * the user to stop the loading process with the normal stop button */
  GST_DEBUG ("state change to %s timed out, returning success and handling "
      "errors asynchronously", gst_element_state_get_name (state));
  bvw->priv->ignore_messages_mask = saved_events;
  return TRUE;

error:
  GST_DEBUG ("error while waiting for state change to %s: %" GST_PTR_FORMAT,
      gst_element_state_get_name (state), *err_msg);
  /* already set *err_msg */
  bvw->priv->ignore_messages_mask = saved_events;
  return FALSE;
}

/**
 * bacon_video_widget_open:
 * @bvw: a #BaconVideoWidget
 * @mrl: an MRL
 * @subtitle_uri: the URI of a subtitle file, or %NULL
 * @error: a #GError, or %NULL
 *
 * Opens the given @mrl in @bvw for playing. If @subtitle_uri is not %NULL, the given
 * subtitle file is also loaded. Alternatively, the subtitle URI can be passed in @mrl
 * by adding it after <literal>#subtitle:</literal>. For example:
 * <literal>http://example.com/video.mpg#subtitle:/home/user/subtitle.ass</literal>.
 *
 * If there was a filesystem error, a %BVW_ERROR_GENERIC error will be returned. Otherwise,
 * more specific #BvwError errors will be returned.
 *
 * On success, the MRL is loaded and waiting to be played with bacon_video_widget_play().
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
gboolean
bacon_video_widget_open (BaconVideoWidget * bvw,
                         const gchar * mrl, const gchar *subtitle_uri, GError ** error)
{
  GtkAllocation allocation;
  GstMessage *err_msg = NULL;
  GFile *file;
  gboolean ret;
  char *path;
  GstBus *bus;

  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (mrl != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (bvw->priv->play != NULL, FALSE);
  
  /* So we aren't closed yet... */
  if (bvw->priv->mrl) {
    bacon_video_widget_close (bvw);
  }
  
  GST_DEBUG ("mrl = %s", GST_STR_NULL (mrl));
  GST_DEBUG ("subtitle_uri = %s", GST_STR_NULL (subtitle_uri));
  
  /* this allows non-URI type of files in the thumbnailer and so on */
  file = g_file_new_for_commandline_arg (mrl);

  /* Only use the URI when FUSE isn't available for a file
   * or we're trying to read from an archive or ObexFTP */
  if (g_file_has_uri_scheme (file, "archive") != FALSE ||
      g_file_has_uri_scheme (file, "obex") != FALSE ||
      g_file_has_uri_scheme (file, "ftp") != FALSE)
    path = NULL;
  else
    path = g_file_get_path (file);
  if (path) {
    bvw->priv->mrl = g_filename_to_uri (path, NULL, NULL);
    g_free (path);
  } else {
    bvw->priv->mrl = g_strdup (mrl);
  }

  g_object_unref (file);

  if (g_str_has_prefix (mrl, "icy:") != FALSE) {
    /* Handle "icy://" URLs from QuickTime */
    g_free (bvw->priv->mrl);
    bvw->priv->mrl = g_strdup_printf ("http:%s", mrl + 4);
  } else if (g_str_has_prefix (mrl, "icyx:") != FALSE) {
    /* Handle "icyx://" URLs from Orban/Coding Technologies AAC/aacPlus Player */
    g_free (bvw->priv->mrl);
    bvw->priv->mrl = g_strdup_printf ("http:%s", mrl + 5);
  } else if (g_str_has_prefix (mrl, "dvd:///")) {
    /* this allows to play backups of dvds */
    g_free (bvw->priv->mrl);
    bvw->priv->mrl = g_strdup ("dvd://");
    g_free (bvw->priv->media_device);
    bvw->priv->media_device = g_strdup (mrl + strlen ("dvd://"));
  } else if (g_str_has_prefix (mrl, "vcd:///")) {
    /* this allows to play backups of vcds */
    g_free (bvw->priv->mrl);
    bvw->priv->mrl = g_strdup ("vcd://");
    g_free (bvw->priv->media_device);
    bvw->priv->media_device = g_strdup (mrl + strlen ("vcd://"));
  }

  bvw->priv->got_redirect = FALSE;
  bvw->priv->media_has_video = FALSE;
  bvw->priv->media_has_audio = FALSE;
  bvw->priv->stream_length = 0;
  bvw->priv->ignore_messages_mask = 0;

  /* We hide the video window for now. Will show when video of vfx comes up */
  if (bvw->priv->video_window) {
    gdk_window_hide (bvw->priv->video_window);
    /* We also take the whole widget until we know video size */
    gtk_widget_get_allocation (GTK_WIDGET (bvw), &allocation);
    gdk_window_move_resize (bvw->priv->video_window, 0, 0,
                            allocation.width,
                            allocation.height);
  }

  /* Visualization settings changed */
  if (bvw->priv->vis_changed) {
    setup_vis (bvw);
  }

  if (bvw->priv->ready_idle_id) {
    g_source_remove (bvw->priv->ready_idle_id);
    bvw->priv->ready_idle_id = 0;
  }

  /* Flush the bus to make sure we don't get any messages
   * from the previous URI, see bug #607224.
   */
  bus = gst_element_get_bus (bvw->priv->play);
  gst_bus_set_flushing (bus, TRUE);

  bvw->priv->target_state = GST_STATE_READY;
  gst_element_set_state (bvw->priv->play, GST_STATE_READY);

  gst_bus_set_flushing (bus, FALSE);
  gst_object_unref (bus);

  g_object_set (bvw->priv->play, "uri", bvw->priv->mrl,
                "suburi", subtitle_uri, NULL);

  bvw->priv->seekable = -1;
  bvw->priv->target_state = GST_STATE_PAUSED;
  bvw_clear_missing_plugins_messages (bvw);

  gst_element_set_state (bvw->priv->play, GST_STATE_PAUSED);

  if (bvw->priv->use_type == BVW_USE_TYPE_AUDIO ||
      bvw->priv->use_type == BVW_USE_TYPE_VIDEO) {
    GST_DEBUG ("normal playback, handling all errors asynchroneously");
    ret = TRUE;
  } else {
    /* used as thumbnailer or metadata extractor for properties dialog. In
     * this case, wait for any state change to really finish and process any
     * pending tag messages, so that the information is available right away */
    GST_DEBUG ("waiting for state changed to PAUSED to complete");
    ret = poll_for_state_change_full (bvw, bvw->priv->play,
        GST_STATE_PAUSED, &err_msg, -1);

    if (bvw->priv->use_type == BVW_USE_TYPE_METADATA) {
      bvw_process_pending_tag_messages (bvw);
      bacon_video_widget_get_stream_length (bvw);
      GST_DEBUG ("stream length = %u", bvw->priv->stream_length);

      /* even in case of an error (e.g. no decoders installed) we might still
       * have useful metadata (like codec types, duration, etc.) */
      g_signal_emit (bvw, bvw_signals[SIGNAL_GOT_METADATA], 0, NULL);
    }
  }
  
  if (ret) {
    g_signal_emit (bvw, bvw_signals[SIGNAL_CHANNELS_CHANGE], 0);
  } else {
    GST_DEBUG ("Error on open: %" GST_PTR_FORMAT, err_msg);
    if (bvw_check_missing_plugins_error (bvw, err_msg)) {
      /* totem will try to start playing, so ignore all messages on the bus */
      bvw->priv->ignore_messages_mask |= GST_MESSAGE_ERROR;
      GST_LOG ("missing plugins handled, ignoring error and returning TRUE");
      gst_message_unref (err_msg);
      err_msg = NULL;
      ret = TRUE;
    } else {
      bvw->priv->ignore_messages_mask |= GST_MESSAGE_ERROR;
      bvw_stop_play_pipeline (bvw);
      g_free (bvw->priv->mrl);
      bvw->priv->mrl = NULL;
    }
  }
  
  /* When opening a new media we want to redraw ourselves */
  gtk_widget_queue_draw (GTK_WIDGET (bvw));

  if (err_msg != NULL) {
    if (error) {
      *error = bvw_error_from_gst_error (bvw, err_msg);
    } else {
      GST_WARNING ("Got error, but caller is not collecting error details!");
    }
    gst_message_unref (err_msg);
  }

  return ret;
}

/**
 * bacon_video_widget_play:
 * @bvw: a #BaconVideoWidget
 * @error: a #GError, or %NULL
 *
 * Plays the currently-loaded video in @bvw.
 *
 * Errors from the GStreamer backend will be returned asynchronously via the
 * #BaconVideoWidget::error signal, even if this function returns %TRUE.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
gboolean
bacon_video_widget_play (BaconVideoWidget * bvw, GError ** error)
{
  GstState cur_state;

  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);
  g_return_val_if_fail (bvw->priv->mrl != NULL, FALSE);

  if (bvw->priv->ready_idle_id) {
    g_source_remove (bvw->priv->ready_idle_id);
    bvw->priv->ready_idle_id = 0;
  }

  bvw->priv->target_state = GST_STATE_PLAYING;

  /* no need to actually go into PLAYING in capture/metadata mode (esp.
   * not with sinks that don't sync to the clock), we'll get everything
   * we need by prerolling the pipeline, and that is done in _open() */
  if (bvw->priv->use_type == BVW_USE_TYPE_CAPTURE ||
      bvw->priv->use_type == BVW_USE_TYPE_METADATA) {
    return TRUE;
  }

  /* Don't try to play if we're already doing that */
  gst_element_get_state (bvw->priv->play, &cur_state, NULL, 0);
  if (cur_state == GST_STATE_PLAYING)
    return TRUE;

  /* Lie when trying to play a file whilst we're download buffering */
  if (bvw->priv->download_buffering != FALSE &&
      bvw_download_buffering_done (bvw) == FALSE) {
    GST_DEBUG ("download buffering in progress, not playing");
    return TRUE;
  }

  /* Or when we're buffering */
  if (bvw->priv->buffering != FALSE) {
    GST_DEBUG ("buffering in progress, not playing");
    return TRUE;
  }

  /* just lie and do nothing in this case */
  if (bvw->priv->plugin_install_in_progress && cur_state != GST_STATE_PAUSED) {
    GST_DEBUG ("plugin install in progress and nothing to play, not playing");
    return TRUE;
  } else if (bvw->priv->mount_in_progress) {
    GST_DEBUG ("Mounting in progress, not playing");
    return TRUE;
  } else if (bvw->priv->auth_dialog != NULL) {
    GST_DEBUG ("Authentication in progress, not playing");
    return TRUE;
  }

  /* Set direction to forward */
  if (bvw_set_playback_direction (bvw, TRUE) == FALSE) {
    GST_DEBUG ("Failed to reset direction back to forward to play");
    return FALSE;
  }

  GST_DEBUG ("play");
  gst_element_set_state (bvw->priv->play, GST_STATE_PLAYING);

  /* will handle all errors asynchroneously */
  return TRUE;
}

/**
 * bacon_video_widget_can_direct_seek:
 * @bvw: a #BaconVideoWidget
 *
 * Determines whether direct seeking is possible for the current stream.
 *
 * Return value: %TRUE if direct seeking is possible, %FALSE otherwise
 **/
gboolean
bacon_video_widget_can_direct_seek (BaconVideoWidget *bvw)
{
  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);

  if (bvw->priv->mrl == NULL)
    return FALSE;

  if (bvw->priv->download_buffering != FALSE)
    return TRUE;

  /* (instant seeking only make sense with video,
   * hence no cdda:// here) */
  if (g_str_has_prefix (bvw->priv->mrl, "file://") ||
      g_str_has_prefix (bvw->priv->mrl, "dvd:/") ||
      g_str_has_prefix (bvw->priv->mrl, "vcd:/"))
    return TRUE;

  return FALSE;
}

static gboolean
bacon_video_widget_seek_time_no_lock (BaconVideoWidget *bvw,
				      gint64 _time,
				      GstSeekFlags flag,
				      GError **error)
{
  if (bvw_set_playback_direction (bvw, TRUE) == FALSE)
    return FALSE;

  bvw->priv->seek_time = -1;
  bvw->priv->rate = FORWARD_RATE;

  gst_element_seek (bvw->priv->play, FORWARD_RATE,
      GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | flag,
      GST_SEEK_TYPE_SET, _time * GST_MSECOND,
      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  return TRUE;
}

/**
 * bacon_video_widget_seek_time:
 * @bvw: a #BaconVideoWidget
 * @_time: the time to which to seek, in milliseconds
 * @accurate: whether to use accurate seek, an accurate seek might be slower for some formats (see GStreamer docs)
 * @error: a #GError, or %NULL
 *
 * Seeks the currently-playing stream to the absolute position @time, in milliseconds.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
gboolean
bacon_video_widget_seek_time (BaconVideoWidget *bvw, gint64 _time, gboolean accurate, GError **error)
{
  GstClockTime cur_time;
  GstSeekFlags  flag;

  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);

  GST_LOG ("Seeking to %" GST_TIME_FORMAT, GST_TIME_ARGS (_time * GST_MSECOND));

  if (_time > bvw->priv->stream_length
      && bvw->priv->stream_length > 0
      && !g_str_has_prefix (bvw->priv->mrl, "dvd:")
      && !g_str_has_prefix (bvw->priv->mrl, "vcd:")) {
    if (bvw->priv->eos_id == 0)
      bvw->priv->eos_id = g_idle_add (bvw_signal_eos_delayed, bvw);
    return TRUE;
  }

  /* Emit a time tick of where we are going, we are paused */
  got_time_tick (bvw->priv->play, _time * GST_MSECOND, bvw);

  /* Is there a pending seek? */
  g_mutex_lock (bvw->priv->seek_mutex);
  /* If there's no pending seek, or
   * it's been too long since the seek,
   * or we don't have an accurate seek requested */
  cur_time = gst_clock_get_internal_time (bvw->priv->clock);
  if (bvw->priv->seek_req_time == GST_CLOCK_TIME_NONE ||
      cur_time > bvw->priv->seek_req_time + SEEK_TIMEOUT ||
      accurate) {
    bvw->priv->seek_time = -1;
    bvw->priv->seek_req_time = cur_time;
    g_mutex_unlock (bvw->priv->seek_mutex);
  } else {
    GST_LOG ("Not long enough since last seek, queuing it");
    bvw->priv->seek_time = _time;
    g_mutex_unlock (bvw->priv->seek_mutex);
    return TRUE;
  }

  if (bvw_set_playback_direction (bvw, TRUE) == FALSE)
    return FALSE;

  flag = (accurate ? GST_SEEK_FLAG_ACCURATE : GST_SEEK_FLAG_KEY_UNIT);
  bacon_video_widget_seek_time_no_lock (bvw, _time, flag, error);

  return TRUE;
}

/**
 * bacon_video_widget_seek:
 * @bvw: a #BaconVideoWidget
 * @position: the percentage of the way through the stream to which to seek
 * @error: a #GError, or %NULL
 *
 * Seeks the currently-playing stream to @position as a percentage of the total
 * stream length.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
gboolean
bacon_video_widget_seek (BaconVideoWidget *bvw, double position, GError **error)
{
  gint64 seek_time, length_nanos;

  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);

  length_nanos = (gint64) (bvw->priv->stream_length * GST_MSECOND);
  seek_time = (gint64) (length_nanos * position);

  GST_LOG ("Seeking to %3.2f%% %" GST_TIME_FORMAT, position,
      GST_TIME_ARGS (seek_time));

  return bacon_video_widget_seek_time (bvw, seek_time / GST_MSECOND, FALSE, error);
}

/**
 * bacon_video_widget_step:
 * @bvw: a #BaconVideoWidget
 * @forward: the direction of the frame step
 * @error: a #GError, or %NULL
 *
 * Step one frame forward, if @forward is %TRUE, or backwards, if @forward is %FALSE
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
gboolean
bacon_video_widget_step (BaconVideoWidget *bvw, gboolean forward, GError **error)
{
  GstEvent *event;
  gboolean retval;

  if (bvw_set_playback_direction (bvw, forward) == FALSE)
    return FALSE;

  event = gst_event_new_step (GST_FORMAT_BUFFERS, 1, 1.0, TRUE, FALSE);

  retval = gst_element_send_event (bvw->priv->play, event);

  if (retval != FALSE)
    bvw_query_timeout (bvw);

  return retval;
}

static gboolean
_ready_idle_cb (gpointer data)
{
  GstElement *playbin = GST_ELEMENT (data);

  GST_DEBUG ("setting playbin to NULL");
  gst_element_set_state (playbin, GST_STATE_NULL);

  return FALSE;
}

static void
bvw_stop_play_pipeline (BaconVideoWidget * bvw)
{
  GstState cur_state;
  GstBus *bus;

  bus = gst_element_get_bus (bvw->priv->play);
  gst_element_get_state (bvw->priv->play, &cur_state, NULL, 0);
  if (cur_state > GST_STATE_READY) {
    GstMessage *msg;

    GST_DEBUG ("stopping");
    gst_element_set_state (bvw->priv->play, GST_STATE_READY);

    /* process all remaining state-change messages so everything gets
     * cleaned up properly (before the state change to NULL flushes them) */
    GST_DEBUG ("processing pending state-change messages");
    while ((msg = gst_bus_poll (bus, GST_MESSAGE_STATE_CHANGED, 0))) {
      gst_bus_async_signal_func (bus, msg, NULL);
      gst_message_unref (msg);
    }
  }

  /* and now drop all following messages until we start again. The
   * bus is set to flush=false again in bacon_video_widget_open()
   */
  gst_bus_set_flushing (bus, TRUE);
  gst_object_unref (bus);

  /* Now in READY or lower */
  if (bvw->priv->ready_idle_id) {
    g_source_remove (bvw->priv->ready_idle_id);
    bvw->priv->ready_idle_id = 0;
  }
  bvw->priv->ready_idle_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, 10, _ready_idle_cb, gst_object_ref (bvw->priv->play), (GDestroyNotify) gst_object_unref);

  bvw->priv->target_state = GST_STATE_READY;
  bvw->priv->buffering = FALSE;
  bvw->priv->plugin_install_in_progress = FALSE;
  bvw->priv->download_buffering = FALSE;
  g_free (bvw->priv->download_filename);
  bvw->priv->download_filename = NULL;
  bvw->priv->buffering_left = -1;
  if (bvw->priv->download_buffering_element != NULL) {
    g_object_unref (bvw->priv->download_buffering_element);
    bvw->priv->download_buffering_element = NULL;
  }
  bvw->priv->ignore_messages_mask = 0;
  bvw_reconfigure_fill_timeout (bvw, 0);
  bvw->priv->movie_par_n = bvw->priv->movie_par_d = 1;
  if (bvw->priv->cover_pixbuf) {
    g_object_unref (bvw->priv->cover_pixbuf);
    bvw->priv->cover_pixbuf = NULL;
  }
  GST_DEBUG ("stopped");
}

/**
 * bacon_video_widget_stop:
 * @bvw: a #BaconVideoWidget
 *
 * Stops playing the current stream and resets to the first position in the stream.
 **/
void
bacon_video_widget_stop (BaconVideoWidget * bvw)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  GST_LOG ("Stopping");
  bvw_stop_play_pipeline (bvw);

  /* Reset position to 0 when stopping */
  got_time_tick (GST_ELEMENT (bvw->priv->play), 0, bvw);
}

/**
 * bacon_video_widget_close:
 * @bvw: a #BaconVideoWidget
 *
 * Closes the current stream and frees the resources associated with it.
 **/
void
bacon_video_widget_close (BaconVideoWidget * bvw)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));
  
  GST_LOG ("Closing");
  bvw_stop_play_pipeline (bvw);

  g_free (bvw->priv->mrl);
  bvw->priv->mrl = NULL;
  g_free (bvw->priv->user_id);
  bvw->priv->user_id = NULL;
  g_free (bvw->priv->user_pw);
  bvw->priv->user_pw = NULL;

  bvw->priv->is_live = FALSE;
  bvw->priv->window_resized = FALSE;
  bvw->priv->rate = FORWARD_RATE;

  bvw->priv->seek_req_time = GST_CLOCK_TIME_NONE;
  bvw->priv->seek_time = -1;

  if (bvw->priv->tagcache) {
    gst_tag_list_free (bvw->priv->tagcache);
    bvw->priv->tagcache = NULL;
  }
  if (bvw->priv->audiotags) {
    gst_tag_list_free (bvw->priv->audiotags);
    bvw->priv->audiotags = NULL;
  }
  if (bvw->priv->videotags) {
    gst_tag_list_free (bvw->priv->videotags);
    bvw->priv->videotags = NULL;
  }

  g_object_notify (G_OBJECT (bvw), "seekable");
  g_signal_emit (bvw, bvw_signals[SIGNAL_CHANNELS_CHANGE], 0);
  got_time_tick (GST_ELEMENT (bvw->priv->play), 0, bvw);
}

static void
bvw_do_navigation_command (BaconVideoWidget * bvw, GstNavigationCommand command)
{
  GstNavigation *nav = bvw_get_navigation_iface (bvw);
  if (nav == NULL)
    return;

  gst_navigation_send_command (nav, command);
  gst_object_unref (GST_OBJECT (nav));
}

/**
 * bacon_video_widget_dvd_event:
 * @bvw: a #BaconVideoWidget
 * @type: the type of DVD event to issue
 *
 * Issues a DVD navigation event to the video widget, such as one to skip to the
 * next chapter, or navigate to the DVD title menu.
 *
 * This is a no-op if the current stream is not navigable.
 **/
void
bacon_video_widget_dvd_event (BaconVideoWidget * bvw,
                              BvwDVDEvent type)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  switch (type) {
    case BVW_DVD_ROOT_MENU:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_DVD_MENU);
      break;
    case BVW_DVD_TITLE_MENU:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_DVD_TITLE_MENU);
      break;
    case BVW_DVD_SUBPICTURE_MENU:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_DVD_SUBPICTURE_MENU);
      break;
    case BVW_DVD_AUDIO_MENU:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_DVD_AUDIO_MENU);
      break;
    case BVW_DVD_ANGLE_MENU:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_DVD_ANGLE_MENU);
      break;
    case BVW_DVD_CHAPTER_MENU:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_DVD_CHAPTER_MENU);
      break;
    case BVW_DVD_NEXT_ANGLE:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_NEXT_ANGLE);
      break;
    case BVW_DVD_PREV_ANGLE:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_PREV_ANGLE);
      break;
    case BVW_DVD_ROOT_MENU_UP:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_UP);
      break;
    case BVW_DVD_ROOT_MENU_DOWN:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_DOWN);
      break;
    case BVW_DVD_ROOT_MENU_LEFT:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_LEFT);
      break;
    case BVW_DVD_ROOT_MENU_RIGHT:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_RIGHT);
      break;
    case BVW_DVD_ROOT_MENU_SELECT:
      bvw_do_navigation_command (bvw, GST_NAVIGATION_COMMAND_ACTIVATE);
      break;
    case BVW_DVD_NEXT_CHAPTER:
    case BVW_DVD_PREV_CHAPTER:
    case BVW_DVD_NEXT_TITLE:
    case BVW_DVD_PREV_TITLE: {
      const gchar *fmt_name;
      GstFormat fmt;
      gint64 val;
      gint dir;

      if (type == BVW_DVD_NEXT_CHAPTER || type == BVW_DVD_NEXT_TITLE)
        dir = 1;
      else
        dir = -1;

      if (type == BVW_DVD_NEXT_CHAPTER || type == BVW_DVD_PREV_CHAPTER)
        fmt_name = "chapter"; 
      else if (type == BVW_DVD_NEXT_TITLE || type == BVW_DVD_PREV_TITLE)
        fmt_name = "title";
      else
        fmt_name = "angle";

      bvw_set_playback_direction (bvw, TRUE);

      fmt = gst_format_get_by_nick (fmt_name);
      if (gst_element_query_position (bvw->priv->play, &fmt, &val)) {
        GST_DEBUG ("current %s is: %" G_GINT64_FORMAT, fmt_name, val);
        val += dir;
        GST_DEBUG ("seeking to %s: %" G_GINT64_FORMAT, val);
        gst_element_seek (bvw->priv->play, FORWARD_RATE, fmt, GST_SEEK_FLAG_FLUSH,
            GST_SEEK_TYPE_SET, val, GST_SEEK_TYPE_NONE, 0);
	bvw->priv->rate = FORWARD_RATE;
      } else {
        GST_DEBUG ("failed to query position (%s)", fmt_name);
      }
      break;
    }
    default:
      GST_WARNING ("unhandled type %d", type);
      break;
  }
}

/**
 * bacon_video_widget_set_logo:
 * @bvw: a #BaconVideoWidget
 * @name: the icon name of the logo
 *
 * Sets the logo displayed on the video widget when no stream is loaded.
 **/
void
bacon_video_widget_set_logo (BaconVideoWidget *bvw, const gchar *name)
{
  GtkIconTheme *theme;
  GError *error = NULL;

  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (name != NULL);

  if (bvw->priv->logo_pixbuf != NULL)
    g_object_unref (bvw->priv->logo_pixbuf);

  theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (bvw)));
  bvw->priv->logo_pixbuf = gtk_icon_theme_load_icon (theme, name, LOGO_SIZE, 0, &error);

  if (error) {
    g_warning ("An error occurred trying to open logo %s: %s", name, error->message);
    g_error_free (error);
  }
}

/**
 * bacon_video_widget_set_logo_mode:
 * @bvw: a #BaconVideoWidget
 * @logo_mode: %TRUE to display the logo, %FALSE otherwise
 *
 * Sets whether to display a logo set with @bacon_video_widget_set_logo when
 * no stream is loaded. If @logo_mode is %FALSE, nothing will be displayed
 * and the video widget will take up no space. Otherwise, the logo will be
 * displayed and will requisition a corresponding amount of space.
 **/
void
bacon_video_widget_set_logo_mode (BaconVideoWidget * bvw, gboolean logo_mode)
{
  BaconVideoWidgetPrivate *priv;

  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  priv = bvw->priv;

  logo_mode = logo_mode != FALSE;

  if (priv->logo_mode != logo_mode) {
    priv->logo_mode = logo_mode;

    if (priv->video_window) {
      if (logo_mode) {
        gdk_window_hide (priv->video_window);
	gtk_widget_set_double_buffered (GTK_WIDGET (bvw), TRUE);
      } else {
        gdk_window_show (priv->video_window);
	gtk_widget_set_double_buffered (GTK_WIDGET (bvw), FALSE);
      }
    }

    g_object_notify (G_OBJECT (bvw), "logo_mode");
    g_object_notify (G_OBJECT (bvw), "seekable");

    /* Queue a redraw of the widget */
    gtk_widget_queue_draw (GTK_WIDGET (bvw));
  }
}

/**
 * bacon_video_widget_get_logo_mode
 * @bvw: a #BaconVideoWidget
 *
 * Gets whether the logo is displayed when no stream is loaded.
 *
 * Return value: %TRUE if the logo is displayed, %FALSE otherwise
 **/
gboolean
bacon_video_widget_get_logo_mode (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);

  return bvw->priv->logo_mode;
}

static gboolean
bvw_check_for_cover_pixbuf (BaconVideoWidget * bvw)
{
  GValue value = { 0, };

  /* for efficiency reasons (decoding of encoded image into pixbuf) we assume
   * that all potential images come in the same taglist, so once we've
   * determined the best image/cover, we assume that's really the best one
   * for this stream, even if more tag messages come in later (this should
   * not be a problem in practice) */
  if (bvw->priv->cover_pixbuf)
    return TRUE;

  bacon_video_widget_get_metadata (bvw, BVW_INFO_COVER, &value);
  if (G_VALUE_HOLDS_OBJECT (&value)) {
    bvw->priv->cover_pixbuf = g_value_dup_object (&value);
    g_value_unset (&value);
  }

  if (bvw->priv->cover_pixbuf)
    setup_vis (bvw);

  return (bvw->priv->cover_pixbuf != NULL);
}

static const GdkPixbuf *
bvw_get_logo_pixbuf (BaconVideoWidget * bvw)
{
  if (bvw_check_for_cover_pixbuf (bvw))
    return bvw->priv->cover_pixbuf;
  else
    return bvw->priv->logo_pixbuf;
}

/**
 * bacon_video_widget_pause:
 * @bvw: a #BaconVideoWidget
 *
 * Pauses the current stream in the video widget.
 *
 * If a live stream is being played, playback is stopped entirely.
 **/
void
bacon_video_widget_pause (BaconVideoWidget * bvw)
{
  GstStateChangeReturn ret;
  GstState state;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));
  g_return_if_fail (bvw->priv->mrl != NULL);

  /* Get the current state */
  ret = gst_element_get_state (GST_ELEMENT (bvw->priv->play), &state, NULL, 0);

  if (bvw->priv->is_live != FALSE &&
      ret != GST_STATE_CHANGE_NO_PREROLL &&
      ret != GST_STATE_CHANGE_SUCCESS &&
      state > GST_STATE_READY) {
    GST_LOG ("Stopping because we have a live stream");
    bacon_video_widget_stop (bvw);
    return;
  }

  if (bvw->priv->ready_idle_id) {
    g_source_remove (bvw->priv->ready_idle_id);
    bvw->priv->ready_idle_id = 0;
  }

  GST_LOG ("Pausing");
  bvw->priv->target_state = GST_STATE_PAUSED;
  gst_element_set_state (GST_ELEMENT (bvw->priv->play), GST_STATE_PAUSED);
}

/**
 * bacon_video_widget_set_subtitle_font:
 * @bvw: a #BaconVideoWidget
 * @font: a font description string
 *
 * Sets the font size and style in which to display subtitles.
 *
 * @font is a Pango font description string, as understood by
 * pango_font_description_from_string().
 **/
void
bacon_video_widget_set_subtitle_font (BaconVideoWidget * bvw,
                                          const gchar * font)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  if (!g_object_class_find_property (G_OBJECT_GET_CLASS (bvw->priv->play), "subtitle-font-desc"))
    return;
  g_object_set (bvw->priv->play, "subtitle-font-desc", font, NULL);
}

/**
 * bacon_video_widget_set_subtitle_encoding:
 * @bvw: a #BaconVideoWidget
 * @encoding: an encoding system
 *
 * Sets the encoding system for the subtitles, so that they can be decoded
 * properly.
 **/
void
bacon_video_widget_set_subtitle_encoding (BaconVideoWidget *bvw,
                                          const char *encoding)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  if (!g_object_class_find_property (G_OBJECT_GET_CLASS (bvw->priv->play), "subtitle-encoding"))
    return;
  g_object_set (bvw->priv->play, "subtitle-encoding", encoding, NULL);
}

/**
 * bacon_video_widget_set_user_agent:
 * @bvw: a #BaconVideoWidget
 * @user_agent: a HTTP user agent string, or %NULL to use the default
 *
 * Sets the HTTP user agent string to use when fetching HTTP ressources.
 **/
void
bacon_video_widget_set_user_agent (BaconVideoWidget *bvw,
                                   const char *user_agent)
{
  BaconVideoWidgetPrivate *priv;

  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  priv = bvw->priv;

  if (g_strcmp0 (user_agent, priv->user_agent) == 0)
    return;

  g_free (priv->user_agent);
  priv->user_agent = g_strdup (user_agent);

  if (priv->source) {
    bvw_set_user_agent_on_element (bvw, priv->source);
  }

  g_object_notify (G_OBJECT (bvw), "user-agent");
}

/**
 * bacon_video_widget_set_referrer:
 * @bvw: a #BaconVideoWidget
 * @referrer: a HTTP referrer URI, or %NULL
 *
 * Sets the HTTP referrer URI to use when fetching HTTP ressources.
 **/
void
bacon_video_widget_set_referrer (BaconVideoWidget *bvw,
                                 const char *referrer)
{
  BaconVideoWidgetPrivate *priv;
  char *frag;

  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  priv = bvw->priv;

  if (g_strcmp0 (referrer, priv->referrer) == 0)
    return;

  g_free (priv->referrer);
  priv->referrer = g_strdup (referrer);

  /* Referrer URIs must not have a fragment */
  if ((frag = strchr (priv->referrer, '#')) != NULL)
    *frag = '\0';

  if (priv->source) {
    bvw_set_referrer_on_element (bvw, priv->source);
  }

  g_object_notify (G_OBJECT (bvw), "referrer");
}

/**
 * bacon_video_widget_can_set_volume:
 * @bvw: a #BaconVideoWidget
 *
 * Returns whether the volume level can be set, given the current settings.
 *
 * The volume cannot be set if the audio output type is set to
 * %BVW_AUDIO_SOUND_AC3PASSTHRU.
 *
 * Return value: %TRUE if the volume can be set, %FALSE otherwise
 **/
gboolean
bacon_video_widget_can_set_volume (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);

  if (bvw->priv->speakersetup == BVW_AUDIO_SOUND_AC3PASSTHRU)
    return FALSE;

  return !bvw->priv->uses_audio_fakesink;
}

/**
 * bacon_video_widget_set_volume:
 * @bvw: a #BaconVideoWidget
 * @volume: the new volume level, as a percentage between %0 and %1
 *
 * Sets the volume level of the stream as a percentage between %0 and %1.
 *
 * If bacon_video_widget_can_set_volume() returns %FALSE, this is a no-op.
 **/
void
bacon_video_widget_set_volume (BaconVideoWidget * bvw, double volume)
{
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  if (bacon_video_widget_can_set_volume (bvw) != FALSE) {
    volume = CLAMP (volume, 0.0, 1.0);
    gst_stream_volume_set_volume (GST_STREAM_VOLUME (bvw->priv->play),
                                  GST_STREAM_VOLUME_FORMAT_CUBIC,
                                  volume);

    bvw->priv->volume = volume;
    g_object_notify (G_OBJECT (bvw), "volume");
  }
}

/**
 * bacon_video_widget_get_volume:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the current volume level, as a percentage between %0 and %1.
 *
 * Return value: the volume as a percentage between %0 and %1
 **/
double
bacon_video_widget_get_volume (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), 0.0);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), 0.0);

  return bvw->priv->volume;
}

/**
 * bacon_video_widget_set_fullscreen:
 * @bvw: a #BaconVideoWidget
 * @fullscreen: %TRUE to go fullscreen, %FALSE otherwise
 *
 * Sets whether the widget renders the stream in fullscreen mode.
 *
 * Fullscreen rendering is done only when possible, as xvidmode is required.
 **/
void
bacon_video_widget_set_fullscreen (BaconVideoWidget * bvw,
                                   gboolean fullscreen)
{
  gboolean have_xvidmode;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  g_object_get (G_OBJECT (bvw->priv->bacon_resize),
        "have-xvidmode", &have_xvidmode,
        NULL);

  if (have_xvidmode == FALSE)
    return;

  bvw->priv->fullscreen_mode = fullscreen;

  if (fullscreen == FALSE)
  {
    bacon_resize_restore (bvw->priv->bacon_resize);
    /* Turn fullscreen on when we have xvidmode */
  } else if (have_xvidmode != FALSE) {
    bacon_resize_resize (bvw->priv->bacon_resize);
  }
}

/**
 * bacon_video_widget_set_show_cursor:
 * @bvw: a #BaconVideoWidget
 * @show_cursor: %TRUE to show the cursor, %FALSE otherwise
 *
 * Sets whether the cursor should be shown when it is over the video
 * widget. If @show_cursor is %FALSE, the cursor will be invisible
 * when it is moved over the video widget.
 **/
void
bacon_video_widget_set_show_cursor (BaconVideoWidget * bvw,
                                    gboolean show_cursor)
{
  GdkWindow *window;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  bvw->priv->cursor_shown = show_cursor;
  window = gtk_widget_get_window (GTK_WIDGET (bvw));

  if (!window) {
    return;
  }

  if (show_cursor == FALSE) {
    totem_gdk_window_set_invisible_cursor (window);
  } else {
    gdk_window_set_cursor (window, bvw->priv->cursor);
  }
}

/**
 * bacon_video_widget_get_show_cursor:
 * @bvw: a #BaconVideoWidget
 *
 * Returns whether the cursor is shown when it is over the video widget.
 *
 * Return value: %TRUE if the cursor is shown, %FALSE otherwise
 **/
gboolean
bacon_video_widget_get_show_cursor (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);

  return bvw->priv->cursor_shown;
}

static struct {
	int height;
	int fps;
} const vis_qualities[] = {
	{ 240, 15 }, /* VISUAL_SMALL */
	{ 320, 25 }, /* VISUAL_NORMAL */
	{ 480, 25 }, /* VISUAL_LARGE */
	{ 600, 30 }  /* VISUAL_EXTRA_LARGE */
};

static void
get_visualization_size (BaconVideoWidget *bvw,
                        int *w, int *h, gint *fps_n, gint *fps_d)
{
  GdkScreen *screen;
  int new_fps_n;

  g_return_if_fail (h != NULL);
  g_return_if_fail (bvw->priv->visq < G_N_ELEMENTS (vis_qualities));

  if (!bvw->priv->video_window)
    return;

  *h = vis_qualities[bvw->priv->visq].height;
  new_fps_n = vis_qualities[bvw->priv->visq].fps;

  screen = gtk_widget_get_screen (GTK_WIDGET (bvw));
  *w = *h * gdk_screen_get_width (screen) / gdk_screen_get_height (screen);

  if (fps_n) 
    *fps_n = new_fps_n;
  if (fps_d) 
    *fps_d = 1;
}

static GstElementFactory *
setup_vis_find_factory (BaconVideoWidget * bvw, const gchar * vis_name)
{
  GstElementFactory *fac = NULL;
  GList *l, *features;

  features = get_visualization_features ();

  /* find element factory using long name */
  for (l = features; l != NULL; l = l->next) {
    GstElementFactory *f = GST_ELEMENT_FACTORY (l->data);
       
    if (f && strcmp (vis_name, gst_element_factory_get_longname (f)) == 0) {
      fac = f;
      goto done;
    }
  }

  /* if nothing was found, try the short name (the default schema uses this) */
  for (l = features; l != NULL; l = l->next) {
    GstElementFactory *f = GST_ELEMENT_FACTORY (l->data);

    /* set to long name as key so that the preferences dialog gets it right */
    if (f && strcmp (vis_name, GST_PLUGIN_FEATURE_NAME (f)) == 0) {
      mateconf_client_set_string (bvw->priv->gc, MATECONF_PREFIX "/visual",
          gst_element_factory_get_longname (f), NULL);
      fac = f;
      goto done;
    }
  }

done:
  g_list_free (features);
  return fac;
}

static void
setup_vis (BaconVideoWidget * bvw)
{
  GstElement *vis_bin = NULL;

  GST_DEBUG ("setup_vis called, show_vfx %d, vis element %s",
      bvw->priv->show_vfx, bvw->priv->vis_element_name);
  
  if (bvw->priv->show_vfx && !bvw->priv->cover_pixbuf && bvw->priv->vis_element_name) {
    GstElement *vis_element = NULL, *vis_capsfilter = NULL;
    GstPad *pad = NULL;
    GstCaps *caps = NULL;
    GstElementFactory *fac = NULL;
    
    fac = setup_vis_find_factory (bvw, bvw->priv->vis_element_name);
    if (!fac) {
      GST_DEBUG ("Could not find element factory for visualisation '%s'",
          GST_STR_NULL (bvw->priv->vis_element_name));
      /* use goom as fallback, better than nothing */
      fac = setup_vis_find_factory (bvw, "goom");
      if (fac == NULL) {
        goto beach;
      } else {
        GST_DEBUG ("Falling back on 'goom' for visualisation");
      }     
    }
    
    vis_element = gst_element_factory_create (fac, "vis_element");
    if (!GST_IS_ELEMENT (vis_element)) {
      GST_DEBUG ("failed creating visualisation element");
      goto beach;
    }
    
    vis_capsfilter = gst_element_factory_make ("capsfilter",
        "vis_capsfilter");
    if (!GST_IS_ELEMENT (vis_capsfilter)) {
      GST_DEBUG ("failed creating visualisation capsfilter element");
      gst_object_unref (vis_element);
      goto beach;
    }
    
    vis_bin = gst_bin_new ("vis_bin");
    if (!GST_IS_ELEMENT (vis_bin)) {
      GST_DEBUG ("failed creating visualisation bin");
      gst_object_unref (vis_element);
      gst_object_unref (vis_capsfilter);
      goto beach;
    }
    /* We created the bin, now ref and sink to make sure we own it */
    gst_object_ref (vis_bin);
    gst_object_sink (vis_bin);
    
    gst_bin_add_many (GST_BIN (vis_bin), vis_element, vis_capsfilter, NULL);
    
    /* Sink ghostpad */
    pad = gst_element_get_static_pad (vis_element, "sink");
    gst_element_add_pad (vis_bin, gst_ghost_pad_new ("sink", pad));
    gst_object_unref (pad);

    /* Source ghostpad, link with vis_element */
    pad = gst_element_get_static_pad (vis_capsfilter, "src");
    gst_element_add_pad (vis_bin, gst_ghost_pad_new ("src", pad));
    gst_element_link_pads (vis_element, "src", vis_capsfilter, "sink");
    gst_object_unref (pad);

    /* Get allowed output caps from visualisation element */
    pad = gst_element_get_static_pad (vis_element, "src");
    caps = gst_pad_get_allowed_caps (pad);
    gst_object_unref (pad);
    
    GST_DEBUG ("allowed caps: %" GST_PTR_FORMAT, caps);
    
    /* Can we fixate ? */
    if (caps && !gst_caps_is_fixed (caps)) {
      guint i;
      gint w, h, fps_n, fps_d;

      caps = gst_caps_make_writable (caps);

      /* Get visualization size */
      get_visualization_size (bvw, &w, &h, &fps_n, &fps_d);

      for (i = 0; i < gst_caps_get_size (caps); ++i) {
        GstStructure *s = gst_caps_get_structure (caps, i);
      
        /* Fixate */
        gst_structure_fixate_field_nearest_int (s, "width", w);
        gst_structure_fixate_field_nearest_int (s, "height", h);
        gst_structure_fixate_field_nearest_fraction (s, "framerate", fps_n,
            fps_d);
      }

      /* set this */
      g_object_set (vis_capsfilter, "caps", caps, NULL);
    }

    GST_DEBUG ("visualisation caps: %" GST_PTR_FORMAT, caps);
    
    if (GST_IS_CAPS (caps)) {
      gst_caps_unref (caps);
    }
  }

  /* Check to see if we have an embedded cover image. If we do, don't show visualisations. */
  bvw_check_for_cover_pixbuf (bvw);

  if (bvw->priv->media_has_audio &&
      !bvw->priv->media_has_video && bvw->priv->video_window) {
    gint flags;

    g_object_get (bvw->priv->play, "flags", &flags, NULL);
    if (bvw->priv->show_vfx && !bvw->priv->cover_pixbuf) {
      gdk_window_show (bvw->priv->video_window);
      gtk_widget_set_double_buffered (GTK_WIDGET (bvw), FALSE);
      flags |= GST_PLAY_FLAG_VIS;
    } else {
      gdk_window_hide (bvw->priv->video_window);
      gtk_widget_set_double_buffered (GTK_WIDGET (bvw), TRUE);
      flags &= ~GST_PLAY_FLAG_VIS;
    }
    g_object_set (bvw->priv->play, "flags", flags, NULL);

    gtk_widget_queue_draw (GTK_WIDGET (bvw));
  }
  
  bvw->priv->vis_changed = FALSE;
  
beach:
  g_object_set (bvw->priv->play, "vis-plugin", vis_bin, NULL);
  if (vis_bin)
    gst_object_unref (vis_bin);
  
  return;
}

/**
 * bacon_video_widget_set_show_visuals:
 * @bvw: a #BaconVideoWidget
 * @show_visuals: %TRUE to show visualisations, %FALSE otherwise
 *
 * Sets whether to show visualisations when playing audio-only streams.
 **/
void
bacon_video_widget_set_show_visuals (BaconVideoWidget * bvw,
                                     gboolean show_visuals)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  if (show_visuals == bvw->priv->show_vfx)
    return;

  bvw->priv->show_vfx = show_visuals;
  setup_vis (bvw);
}

static gboolean
filter_features (GstPluginFeature * feature, gpointer data)
{
  GstElementFactory *f;

  if (!GST_IS_ELEMENT_FACTORY (feature))
    return FALSE;
  f = GST_ELEMENT_FACTORY (feature);
  if (!g_strrstr (gst_element_factory_get_klass (f), "Visualization"))
    return FALSE;

  return TRUE;
}

static GList *
get_visualization_features (void)
{
  return gst_registry_feature_filter (gst_registry_get_default (),
      filter_features, FALSE, NULL);
}

static void
add_longname (GstElementFactory *f, GList ** to)
{
  *to = g_list_append (*to, (gchar *) gst_element_factory_get_longname (f));
}

/**
 * bacon_video_widget_get_visuals_list:
 * @bvw: a #BaconVideoWidget
 *
 * Returns a list of the visualisations available when playing audio-only streams.
 *
 * Return value: a #GList of visualisation names; owned by @bvw
 **/
GList *
bacon_video_widget_get_visuals_list (BaconVideoWidget * bvw)
{
  GList *features, *names = NULL;

  g_return_val_if_fail (bvw != NULL, NULL);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), NULL);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), NULL);

  if (bvw->priv->vis_plugins_list) {
    return bvw->priv->vis_plugins_list;
  }

  features = get_visualization_features ();
  g_list_foreach (features, (GFunc) add_longname, &names);
  g_list_free (features);
  bvw->priv->vis_plugins_list = names;

  return names;
}

/**
 * bacon_video_widget_set_visuals:
 * @bvw: a #BaconVideoWidget
 * @name: the visualisation's name, or %NULL
 *
 * Sets the visualisation to display when playing audio-only streams.
 *
 * If @name is %NULL, visualisations will be disabled. Otherwise, @name
 * should be from the list returned by bacon_video_widget_get_visuals_list().
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
gboolean
bacon_video_widget_set_visuals (BaconVideoWidget * bvw, const char *name)
{
  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);
  
  if (bvw->priv->vis_element_name) {
    if (strcmp (bvw->priv->vis_element_name, name) == 0) {
      return FALSE;
    }
    else {
      g_free (bvw->priv->vis_element_name);
    }
  }
  
  bvw->priv->vis_element_name = g_strdup (name);

  GST_DEBUG ("new visualisation element name = '%s'", GST_STR_NULL (name));
  
  setup_vis (bvw);
  
  return FALSE;
}

/**
 * bacon_video_widget_set_visuals_quality:
 * @bvw: a #BaconVideoWidget
 * @quality: the visualisation quality
 *
 * Sets the quality/size of displayed visualisations.
 **/
void
bacon_video_widget_set_visuals_quality (BaconVideoWidget * bvw,
                                        BvwVisualsQuality quality)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  if (bvw->priv->visq == quality)
    return;

  bvw->priv->visq = quality;
  
  setup_vis (bvw);
}

/**
 * bacon_video_widget_get_auto_resize:
 * @bvw: a #BaconVideoWidget
 *
 * Returns whether the widget will automatically resize to fit videos.
 *
 * Return value: %TRUE if the widget will resize, %FALSE otherwise
 **/
gboolean
bacon_video_widget_get_auto_resize (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);

  return bvw->priv->auto_resize;
}

/**
 * bacon_video_widget_set_auto_resize:
 * @bvw: a #BaconVideoWidget
 * @auto_resize: %TRUE to automatically resize for new videos, %FALSE otherwise
 *
 * Sets whether the widget should automatically resize to fit to new videos when
 * they are loaded. Changes to this will take effect when the next media file is
 * loaded.
 **/
void
bacon_video_widget_set_auto_resize (BaconVideoWidget * bvw,
                                    gboolean auto_resize)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  bvw->priv->auto_resize = auto_resize;

  /* this will take effect when the next media file loads */
}

/**
 * bacon_video_widget_set_aspect_ratio:
 * @bvw: a #BaconVideoWidget
 * @ratio: the new aspect ratio
 *
 * Sets the aspect ratio used by the widget, from #BaconVideoWidgetAspectRatio.
 *
 * Changes to this take effect immediately.
 **/
void
bacon_video_widget_set_aspect_ratio (BaconVideoWidget *bvw,
                                BvwAspectRatio ratio)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  bvw->priv->ratio_type = ratio;
  got_video_size (bvw);
}

/**
 * bacon_video_widget_get_aspect_ratio:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the current aspect ratio used by the widget, from
 * #BaconVideoWidgetAspectRatio.
 *
 * Return value: the aspect ratio
 **/
BvwAspectRatio
bacon_video_widget_get_aspect_ratio (BaconVideoWidget *bvw)
{
  g_return_val_if_fail (bvw != NULL, 0);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), 0);

  return bvw->priv->ratio_type;
}

/**
 * bacon_video_widget_set_scale_ratio:
 * @bvw: a #BaconVideoWidget
 * @ratio: the new scale ratio
 *
 * Sets the ratio by which the widget will scale videos when they are
 * displayed. If @ratio is set to %0, the highest ratio possible will
 * be chosen.
 **/
void
bacon_video_widget_set_scale_ratio (BaconVideoWidget * bvw, gfloat ratio)
{
  gint w, h;

  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  GST_DEBUG ("ratio = %.2f", ratio);

  if (bvw->priv->video_window == NULL)
    return;

  if (!bvw->priv->media_has_video && bvw->priv->show_vfx) {
    get_visualization_size (bvw, &w, &h, NULL, NULL);
  } else {
    get_media_size (bvw, &w, &h);
  }

  if (ratio == 0.0) {
    if (totem_ratio_fits_screen (bvw->priv->video_window, w, h, 2.0))
      ratio = 2.0;
    else if (totem_ratio_fits_screen (bvw->priv->video_window, w, h, 1.0))
      ratio = 1.0;
    else if (totem_ratio_fits_screen (bvw->priv->video_window, w, h, 0.5))
      ratio = 0.5;
    else
      return;
  } else {
    if (!totem_ratio_fits_screen (bvw->priv->video_window, w, h, ratio)) {
      GST_DEBUG ("movie doesn't fit on screen @ %.1fx (%dx%d)", w, h, ratio);
      return;
    }
  }
  w = (gfloat) w * ratio;
  h = (gfloat) h * ratio;

  shrink_toplevel (bvw);

  GST_DEBUG ("setting preferred size %dx%d", w, h);
  totem_widget_set_preferred_size (GTK_WIDGET (bvw), w, h);
}

/**
 * bacon_video_widget_set_zoom:
 * @bvw: a #BaconVideoWidget
 * @zoom: a percentage zoom factor
 *
 * Sets the zoom factor applied to the video when it is displayed,
 * as an integeric percentage between %0 and %1
 * (e.g. set @zoom to %1 to not zoom at all).
 **/
void
bacon_video_widget_set_zoom (BaconVideoWidget *bvw,
                             double            zoom)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));

  bvw->priv->zoom = zoom;
  if (bvw->priv->video_window != NULL)
    resize_video_window (bvw);
}

/**
 * bacon_video_widget_get_zoom:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the zoom factor applied to videos displayed by the widget,
 * as an integeric percentage between %0 and %1
 * (e.g. %1 means no zooming at all).
 *
 * Return value: the zoom factor
 **/
double
bacon_video_widget_get_zoom (BaconVideoWidget *bvw)
{
  g_return_val_if_fail (bvw != NULL, 1.0);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), 1.0);

  return bvw->priv->zoom;
}

/* Search for the color balance channel corresponding to type and return it. */
static GstColorBalanceChannel *
bvw_get_color_balance_channel (GstColorBalance * color_balance,
    BvwVideoProperty type)
{
  const GList *channels;

  channels = gst_color_balance_list_channels (color_balance);

  for (; channels != NULL; channels = channels->next) {
    GstColorBalanceChannel *c = channels->data;

    if (type == BVW_VIDEO_BRIGHTNESS && g_strrstr (c->label, "BRIGHTNESS"))
      return g_object_ref (c);
    else if (type == BVW_VIDEO_CONTRAST && g_strrstr (c->label, "CONTRAST"))
      return g_object_ref (c);
    else if (type == BVW_VIDEO_SATURATION && g_strrstr (c->label, "SATURATION"))
      return g_object_ref (c);
    else if (type == BVW_VIDEO_HUE && g_strrstr (c->label, "HUE"))
      return g_object_ref (c);
  }

  return NULL;
}

/**
 * bacon_video_widget_get_video_property:
 * @bvw: a #BaconVideoWidget
 * @type: the type of property
 *
 * Returns the given property of the video, such as its brightness or saturation.
 *
 * It is returned as a percentage in the full range of integer values; from %0
 * to %G_MAXINT, where %G_MAXINT/2 is the default.
 *
 * Return value: the property's value, in the range %0 to %G_MAXINT
 **/
int
bacon_video_widget_get_video_property (BaconVideoWidget *bvw,
                                       BvwVideoProperty type)
{
  int ret;

  g_return_val_if_fail (bvw != NULL, 65535/2);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), 65535/2);
  
  g_mutex_lock (bvw->priv->lock);

  ret = 0;
  
  if (bvw->priv->balance && GST_IS_COLOR_BALANCE (bvw->priv->balance))
    {
      GstColorBalanceChannel *found_channel = NULL;
      
      found_channel = bvw_get_color_balance_channel (bvw->priv->balance, type);
      
      if (found_channel && GST_IS_COLOR_BALANCE_CHANNEL (found_channel)) {
        gint cur;

        cur = gst_color_balance_get_value (bvw->priv->balance,
                                           found_channel);

        GST_DEBUG ("channel %s: cur=%d, min=%d, max=%d", found_channel->label,
            cur, found_channel->min_value, found_channel->max_value);

        ret = floor (0.5 +
            ((double) cur - found_channel->min_value) * 65535 /
            ((double) found_channel->max_value - found_channel->min_value));

        GST_DEBUG ("channel %s: returning value %d", found_channel->label, ret);
        g_object_unref (found_channel);
        goto done;
      } else {
	ret = -1;
      }
    }

  /* value wasn't found, get from mateconf */
  if (ret == 0)
    ret = mateconf_client_get_int (bvw->priv->gc, video_props_str[type], NULL);

  GST_DEBUG ("nothing found for type %d, returning value %d from mateconf key %s",
      type, ret, video_props_str[type]);

done:

  g_mutex_unlock (bvw->priv->lock);
  return ret;
}

/**
 * bacon_video_widget_has_menus:
 * @bvw: a #BaconVideoWidget
 *
 * Returns whether the widget is currently displaying a menu,
 * such as a DVD menu.
 *
 * Return value: %TRUE if a menu is displyed, %FALSE otherwise
 **/
gboolean
bacon_video_widget_has_menus (BaconVideoWidget *bvw)
{
    g_return_val_if_fail (bvw != NULL, FALSE);
    g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);

    if (bacon_video_widget_is_playing (bvw) == FALSE)
        return FALSE;

    return bvw->priv->is_menu;
}

static gboolean
notify_volume_idle_cb (BaconVideoWidget *bvw)
{
  gdouble vol;

  vol = gst_stream_volume_get_volume (GST_STREAM_VOLUME (bvw->priv->play),
                                      GST_STREAM_VOLUME_FORMAT_CUBIC);

  bvw->priv->volume = vol;

  g_object_notify (G_OBJECT (bvw), "volume");

  return FALSE;
}

static void
notify_volume_cb (GObject             *object,
		  GParamSpec          *pspec,
		  BaconVideoWidget    *bvw)
{
  g_idle_add ((GSourceFunc) notify_volume_idle_cb, bvw);
}

/**
 * bacon_video_widget_set_video_property:
 * @bvw: a #BaconVideoWidget
 * @type: the type of property
 * @value: the property's value, in the range %0 to %G_MAXINT
 *
 * Sets the given property of the video, such as its brightness or saturation.
 *
 * It should be given as a percentage in the full range of integer values; from %0
 * to %G_MAXINT, where %G_MAXINT/2 is the default.
 **/
void
bacon_video_widget_set_video_property (BaconVideoWidget *bvw,
                                       BvwVideoProperty type,
                                       int value)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  
  GST_DEBUG ("set video property type %d to value %d", type, value);
  
  if ( !(value <= 65535 && value >= 0) )
    return;

  if (bvw->priv->balance && GST_IS_COLOR_BALANCE (bvw->priv->balance))
    {
      GstColorBalanceChannel *found_channel = NULL;
      
      found_channel = bvw_get_color_balance_channel (bvw->priv->balance, type);

      if (found_channel && GST_IS_COLOR_BALANCE_CHANNEL (found_channel))
        {
          int i_value;
          
          i_value = floor (0.5 + value * ((double) found_channel->max_value -
              found_channel->min_value) / 65535 + found_channel->min_value);

          GST_DEBUG ("channel %s: set to %d/65535", found_channel->label, value);

          gst_color_balance_set_value (bvw->priv->balance, found_channel,
                                       i_value);

          GST_DEBUG ("channel %s: val=%d, min=%d, max=%d", found_channel->label,
              i_value, found_channel->min_value, found_channel->max_value);

          g_object_unref (found_channel);
        }
    }

  /* save in mateconf */
  mateconf_client_set_int (bvw->priv->gc, video_props_str[type], value, NULL);

  GST_DEBUG ("setting value %d on mateconf key %s", value, video_props_str[type]);
}

/**
 * bacon_video_widget_get_position:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the current position in the stream, as a value between
 * %0 and %1.
 *
 * Return value: the current position, or %-1
 **/
double
bacon_video_widget_get_position (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (bvw != NULL, -1);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), -1);
  return bvw->priv->current_position;
}

/**
 * bacon_video_widget_get_current_time:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the current position in the stream, as the time (in milliseconds)
 * since the beginning of the stream.
 *
 * Return value: time since the beginning of the stream, in milliseconds, or %-1
 **/
gint64
bacon_video_widget_get_current_time (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (bvw != NULL, -1);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), -1);
  return bvw->priv->current_time;
}

/**
 * bacon_video_widget_get_stream_length:
 * @bvw: a #BaconVideoWidget
 *
 * Returns the total length of the stream, in milliseconds.
 *
 * Return value: the stream length, in milliseconds, or %-1
 **/
gint64
bacon_video_widget_get_stream_length (BaconVideoWidget * bvw)
{
  g_return_val_if_fail (bvw != NULL, -1);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), -1);

  if (bvw->priv->stream_length == 0 && bvw->priv->play != NULL) {
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 len = -1;

    if (gst_element_query_duration (bvw->priv->play, &fmt, &len) && len != -1) {
      bvw->priv->stream_length = len / GST_MSECOND;
    }
  }

  return bvw->priv->stream_length;
}

/**
 * bacon_video_widget_is_playing:
 * @bvw: a #BaconVideoWidget
 *
 * Returns whether the widget is currently playing a stream.
 *
 * Return value: %TRUE if a stream is playing, %FALSE otherwise
 **/
gboolean
bacon_video_widget_is_playing (BaconVideoWidget * bvw)
{
  gboolean ret;

  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);

  ret = (bvw->priv->target_state == GST_STATE_PLAYING);
  GST_LOG ("%splaying", (ret) ? "" : "not ");

  return ret;
}

/**
 * bacon_video_widget_is_seekable:
 * @bvw: a #BaconVideoWidget
 *
 * Returns whether seeking is possible in the current stream.
 *
 * If no stream is loaded, %FALSE is returned.
 *
 * Return value: %TRUE if the stream is seekable, %FALSE otherwise
 **/
gboolean
bacon_video_widget_is_seekable (BaconVideoWidget * bvw)
{
  gboolean res;
  gint old_seekable;

  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);

  if (bvw->priv->mrl == NULL)
    return FALSE;

  old_seekable = bvw->priv->seekable;

  if (bvw->priv->is_menu != FALSE)
    return FALSE;

  if (bvw->priv->seekable == -1) {
    GstQuery *query;

    query = gst_query_new_seeking (GST_FORMAT_TIME);
    if (gst_element_query (bvw->priv->play, query)) {
      gst_query_parse_seeking (query, NULL, &res, NULL, NULL);
      GST_DEBUG ("seeking query says the stream is%s seekable", (res) ? "" : " not");
      bvw->priv->seekable = (res) ? 1 : 0;
    } else {
      GST_DEBUG ("seeking query failed");
    }
    gst_query_unref (query);
  }

  if (bvw->priv->seekable != -1) {
    res = (bvw->priv->seekable != 0);
    goto done;
  }

  /* try to guess from duration (this is very unreliable though) */
  if (bvw->priv->stream_length == 0) {
    res = (bacon_video_widget_get_stream_length (bvw) > 0);
  } else {
    res = (bvw->priv->stream_length > 0);
  }

done:

  if (old_seekable != bvw->priv->seekable)
    g_object_notify (G_OBJECT (bvw), "seekable");

  GST_DEBUG ("stream is%s seekable", (res) ? "" : " not");
  return res;
}

/**
 * bacon_video_widget_get_mrls:
 * @bvw: a #BaconVideoWidget
 * @type: the media type
 * @device: the device name
 * @error: a #GError, or %NULL
 *
 * Returns an array of MRLs available for the given @device and media @type.
 *
 * @device should typically be the number of the device (e.g. %0 for the first
 * DVD drive.
 *
 * @type can be any value from #TotemDiscMediaType, but a %BVW_ERROR_INVALID_LOCATION error
 * will be returned if @type is %MEDIA_TYPE_CDDA, as CDDA support has been removed from
 * Totem (and hence #BaconVideoWidget).
 *
 * A %BVW_ERROR_NO_PLUGIN_FOR_FILE error will be returned if the required GStreamer elements do
 * not exist for the given @type (for the GStreamer backend). *
 * If @device does not exist, a %BVW_ERROR_INVALID_DEVICE error will be returned.
 *
 * Return value: a %NULL-terminated array of MRLs, or %NULL; free with g_strfreev()
 **/
gchar **
bacon_video_widget_get_mrls (BaconVideoWidget * bvw,
			     TotemDiscMediaType type,
			     const char *device,
			     GError **error)
{
  gchar **mrls;

  g_return_val_if_fail (bvw != NULL, NULL);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), NULL);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), NULL);

  GST_DEBUG ("type = %d", type);
  GST_DEBUG ("device = %s", GST_STR_NULL (device));

  switch (type) {
    case MEDIA_TYPE_VCD: {
	gchar *uri[] = { NULL, NULL };
	uri[0] = g_strdup_printf ("vcd://%s", device);
	mrls = g_strdupv (uri);
	g_free (uri[0]);
	break;
      }
    case MEDIA_TYPE_DVD: {
      if (!gst_default_registry_check_feature_version ("rsndvdbin", 0, 10, 0)) {
        GST_DEBUG ("Missing rsndvdbin");
	g_set_error_literal (error, BVW_ERROR, BVW_ERROR_NO_PLUGIN_FOR_FILE,
                             "XXX Do not use XXX");
        return NULL;
      } else if (!gst_default_registry_check_feature_version ("mpegpsdemux", 0, 10, 0) &&
		 gst_default_registry_check_feature_version ("flupsdemux", 0, 10, 0) &&
      		 !gst_default_registry_check_feature_version ("flupsdemux", 0, 10, 15)) {
        GST_DEBUG ("flupsdemux not new enough for DVD playback");
	g_set_error_literal (error, BVW_ERROR, BVW_ERROR_NO_PLUGIN_FOR_FILE,
                             "XXX Do not use XXX");
        return NULL;
      } else {
	gchar *uri[] = { NULL, NULL };
	uri[0] = g_strdup_printf ("dvd://%s", device);
	mrls = g_strdupv (uri);
	g_free (uri[0]);
      }
      break;
    }
    case MEDIA_TYPE_CDDA:
      g_set_error_literal (error, BVW_ERROR, BVW_ERROR_INVALID_LOCATION,
                           "XXX Do not use XXX");
      return NULL;
    default:
      g_assert_not_reached();
  }

  if (mrls == NULL)
    return NULL;

  g_free (bvw->priv->media_device);
  bvw->priv->media_device = g_strdup (device);

  return mrls;
}

static struct _metadata_map_info {
  BvwMetadataType type;
  const gchar *str;
} metadata_str_map[] = {
  { BVW_INFO_TITLE, "title" },
  { BVW_INFO_ARTIST, "artist" },
  { BVW_INFO_YEAR, "year" },
  { BVW_INFO_COMMENT, "comment" },
  { BVW_INFO_ALBUM, "album" },
  { BVW_INFO_DURATION, "duration" },
  { BVW_INFO_TRACK_NUMBER, "track-number" },
  { BVW_INFO_HAS_VIDEO, "has-video" },
  { BVW_INFO_DIMENSION_X, "dimension-x" },
  { BVW_INFO_DIMENSION_Y, "dimension-y" },
  { BVW_INFO_VIDEO_BITRATE, "video-bitrate" },
  { BVW_INFO_VIDEO_CODEC, "video-codec" },
  { BVW_INFO_FPS, "fps" },
  { BVW_INFO_HAS_AUDIO, "has-audio" },
  { BVW_INFO_AUDIO_BITRATE, "audio-bitrate" },
  { BVW_INFO_AUDIO_CODEC, "audio-codec" },
  { BVW_INFO_AUDIO_SAMPLE_RATE, "samplerate" },
  { BVW_INFO_AUDIO_CHANNELS, "channels" }
};

static const gchar *
get_metadata_type_name (BvwMetadataType type)
{
  guint i;
  for (i = 0; i < G_N_ELEMENTS (metadata_str_map); ++i) {
    if (metadata_str_map[i].type == type)
      return metadata_str_map[i].str;
  }
  return "unknown";
}

static gint
bvw_get_current_stream_num (BaconVideoWidget * bvw,
    const gchar *stream_type)
{
  gchar *lower, *cur_prop_str;
  gint stream_num = -1;

  if (bvw->priv->play == NULL)
    return stream_num;

  lower = g_ascii_strdown (stream_type, -1);
  cur_prop_str = g_strconcat ("current-", lower, NULL);
  g_object_get (bvw->priv->play, cur_prop_str, &stream_num, NULL);
  g_free (cur_prop_str);
  g_free (lower);

  GST_LOG ("current %s stream: %d", stream_type, stream_num);
  return stream_num;
}

static GstTagList *
bvw_get_tags_of_current_stream (BaconVideoWidget * bvw,
    const gchar *stream_type)
{
  GstTagList *tags = NULL;
  gint stream_num = -1;
  gchar *lower, *cur_sig_str;

  stream_num = bvw_get_current_stream_num (bvw, stream_type);
  if (stream_num < 0)
    return NULL;

  lower = g_ascii_strdown (stream_type, -1);
  cur_sig_str = g_strconcat ("get-", lower, "-tags", NULL);
  g_signal_emit_by_name (bvw->priv->play, cur_sig_str, stream_num, &tags);
  g_free (cur_sig_str);
  g_free (lower);

  GST_LOG ("current %s stream tags %" GST_PTR_FORMAT, stream_type, tags);
  return tags;
}

static GstCaps *
bvw_get_caps_of_current_stream (BaconVideoWidget * bvw,
    const gchar *stream_type)
{
  GstCaps *caps = NULL;
  gint stream_num = -1;
  GstPad *current;
  gchar *lower, *cur_sig_str;

  stream_num = bvw_get_current_stream_num (bvw, stream_type);
  if (stream_num < 0)
    return NULL;

  lower = g_ascii_strdown (stream_type, -1);
  cur_sig_str = g_strconcat ("get-", lower, "-pad", NULL);
  g_signal_emit_by_name (bvw->priv->play, cur_sig_str, stream_num, &current);
  g_free (cur_sig_str);
  g_free (lower);

  if (current != NULL) {
    caps = gst_pad_get_negotiated_caps (current);
    gst_object_unref (current);
  }
  GST_LOG ("current %s stream caps: %" GST_PTR_FORMAT, stream_type, caps);
  return caps;
}

static gboolean
audio_caps_have_LFE (GstStructure * s)
{
  GstAudioChannelPosition *positions;
  gint i, channels;

  if (!gst_structure_get_value (s, "channel-positions") ||
      !gst_structure_get_int (s, "channels", &channels)) {
    return FALSE;
  }

  positions = gst_audio_get_channel_positions (s);
  if (positions == NULL)
    return FALSE;

  for (i = 0; i < channels; ++i) {
    if (positions[i] == GST_AUDIO_CHANNEL_POSITION_LFE) {
      g_free (positions);
      return TRUE;
    }
  }

  g_free (positions);
  return FALSE;
}

static void
bacon_video_widget_get_metadata_string (BaconVideoWidget * bvw,
                                        BvwMetadataType type,
                                        GValue * value)
{
  char *string = NULL;
  gboolean res = FALSE;

  g_value_init (value, G_TYPE_STRING);

  if (bvw->priv->play == NULL) {
    g_value_set_string (value, NULL);
    return;
  }

  switch (type) {
    case BVW_INFO_TITLE:
      if (bvw->priv->tagcache != NULL) {
        res = gst_tag_list_get_string_index (bvw->priv->tagcache,
                                             GST_TAG_TITLE, 0, &string);
      }
      break;
    case BVW_INFO_ARTIST:
      if (bvw->priv->tagcache != NULL) {
        res = gst_tag_list_get_string_index (bvw->priv->tagcache,
                                             GST_TAG_ARTIST, 0, &string);
      }
      break;
    case BVW_INFO_YEAR:
      if (bvw->priv->tagcache != NULL) {
        GDate *date;

        if ((res = gst_tag_list_get_date (bvw->priv->tagcache,
                                          GST_TAG_DATE, &date))) {
          string = g_strdup_printf ("%d", g_date_get_year (date));
          g_date_free (date);
        }
      }
      break;
    case BVW_INFO_COMMENT:
      if (bvw->priv->tagcache != NULL) {
        res = gst_tag_list_get_string_index (bvw->priv->tagcache,
                                             GST_TAG_COMMENT, 0, &string);
      }
      break;
    case BVW_INFO_ALBUM:
      if (bvw->priv->tagcache != NULL) {
        res = gst_tag_list_get_string_index (bvw->priv->tagcache,
                                             GST_TAG_ALBUM, 0, &string);
      }
      break;
    case BVW_INFO_VIDEO_CODEC: {
      GstTagList *tags;

      /* try to get this from the stream info first */
      if ((tags = bvw_get_tags_of_current_stream (bvw, "video"))) {
        res = gst_tag_list_get_string (tags, GST_TAG_CODEC, &string);
	gst_tag_list_free (tags);
      }

      /* if that didn't work, try the aggregated tags */
      if (!res && bvw->priv->tagcache != NULL) {
        res = gst_tag_list_get_string (bvw->priv->tagcache,
            GST_TAG_VIDEO_CODEC, &string);
      }
      break;
    }
    case BVW_INFO_AUDIO_CODEC: {
      GstTagList *tags;

      /* try to get this from the stream info first */
      if ((tags = bvw_get_tags_of_current_stream (bvw, "audio"))) {
        res = gst_tag_list_get_string (tags, GST_TAG_CODEC, &string);
	gst_tag_list_free (tags);
      }

      /* if that didn't work, try the aggregated tags */
      if (!res && bvw->priv->tagcache != NULL) {
        res = gst_tag_list_get_string (bvw->priv->tagcache,
            GST_TAG_AUDIO_CODEC, &string);
      }
      break;
    }
    case BVW_INFO_AUDIO_CHANNELS: {
      GstStructure *s;
      GstCaps *caps;

      caps = bvw_get_caps_of_current_stream (bvw, "audio");
      if (caps) {
        gint channels = 0;

        s = gst_caps_get_structure (caps, 0);
        if ((res = gst_structure_get_int (s, "channels", &channels))) {
          /* FIXME: do something more sophisticated - but what? */
          if (channels > 2 && audio_caps_have_LFE (s)) {
            string = g_strdup_printf ("%s %d.1", _("Surround"), channels - 1);
          } else if (channels == 1) {
            string = g_strdup (_("Mono"));
          } else if (channels == 2) {
            string = g_strdup (_("Stereo"));
          } else {
            string = g_strdup_printf ("%d", channels);
          }
        }
        gst_caps_unref (caps);
      }
      break;
    }
    default:
      g_assert_not_reached ();
    }

  /* Remove line feeds */
  if (string && strstr (string, "\n") != NULL)
    g_strdelimit (string, "\n", ' ');

  if (res && string && g_utf8_validate (string, -1, NULL)) {
    g_value_take_string (value, string);
    GST_DEBUG ("%s = '%s'", get_metadata_type_name (type), string);
  } else {
    g_value_set_string (value, NULL);
    g_free (string);
  }

  return;
}

static void
bacon_video_widget_get_metadata_int (BaconVideoWidget * bvw,
                                     BvwMetadataType type,
                                     GValue * value)
{
  int integer = 0;

  g_value_init (value, G_TYPE_INT);

  if (bvw->priv->play == NULL) {
    g_value_set_int (value, 0);
    return;
  }

  switch (type) {
    case BVW_INFO_DURATION:
      integer = bacon_video_widget_get_stream_length (bvw) / 1000;
      break;
    case BVW_INFO_TRACK_NUMBER:
      if (bvw->priv->tagcache == NULL)
        break;
      if (!gst_tag_list_get_uint (bvw->priv->tagcache,
                                  GST_TAG_TRACK_NUMBER, (guint *) &integer))
        integer = 0;
      break;
    case BVW_INFO_DIMENSION_X:
      integer = bvw->priv->video_width;
      break;
    case BVW_INFO_DIMENSION_Y:
      integer = bvw->priv->video_height;
      break;
    case BVW_INFO_FPS:
      if (bvw->priv->video_fps_d > 0) {
        /* Round up/down to the nearest integer framerate */
        integer = (bvw->priv->video_fps_n + bvw->priv->video_fps_d/2) /
                  bvw->priv->video_fps_d;
      }
      else
        integer = 0;
      break;
    case BVW_INFO_AUDIO_BITRATE:
      if (bvw->priv->audiotags == NULL)
        break;
      if (gst_tag_list_get_uint (bvw->priv->audiotags, GST_TAG_BITRATE,
          (guint *)&integer) ||
          gst_tag_list_get_uint (bvw->priv->audiotags, GST_TAG_NOMINAL_BITRATE,
          (guint *)&integer)) {
        integer /= 1000;
      }
      break;
    case BVW_INFO_VIDEO_BITRATE:
      if (bvw->priv->videotags == NULL)
        break;
      if (gst_tag_list_get_uint (bvw->priv->videotags, GST_TAG_BITRATE,
          (guint *)&integer) ||
          gst_tag_list_get_uint (bvw->priv->videotags, GST_TAG_NOMINAL_BITRATE,
          (guint *)&integer)) {
        integer /= 1000;
      }
      break;
    case BVW_INFO_AUDIO_SAMPLE_RATE: {
      GstStructure *s;
      GstCaps *caps;

      caps = bvw_get_caps_of_current_stream (bvw, "audio");
      if (caps) {
        s = gst_caps_get_structure (caps, 0);
        gst_structure_get_int (s, "rate", &integer);
        gst_caps_unref (caps);
      }
      break;
    }
    default:
      g_assert_not_reached ();
    }

  g_value_set_int (value, integer);
  GST_DEBUG ("%s = %d", get_metadata_type_name (type), integer);

  return;
}

static void
bacon_video_widget_get_metadata_bool (BaconVideoWidget * bvw,
                                      BvwMetadataType type,
                                      GValue * value)
{
  gboolean boolean = FALSE;

  g_value_init (value, G_TYPE_BOOLEAN);

  if (bvw->priv->play == NULL) {
    g_value_set_boolean (value, FALSE);
    return;
  }

  GST_DEBUG ("tagcache  = %" GST_PTR_FORMAT, bvw->priv->tagcache);
  GST_DEBUG ("videotags = %" GST_PTR_FORMAT, bvw->priv->videotags);
  GST_DEBUG ("audiotags = %" GST_PTR_FORMAT, bvw->priv->audiotags);

  switch (type)
  {
    case BVW_INFO_HAS_VIDEO:
      boolean = bvw->priv->media_has_video;
      /* if properties dialog, show the metadata we
       * have even if we cannot decode the stream */
      if (!boolean && bvw->priv->use_type == BVW_USE_TYPE_METADATA &&
          bvw->priv->tagcache != NULL &&
          gst_structure_has_field ((GstStructure *) bvw->priv->tagcache,
                                   GST_TAG_VIDEO_CODEC)) {
        boolean = TRUE;
      }
      break;
    case BVW_INFO_HAS_AUDIO:
      boolean = bvw->priv->media_has_audio;
      /* if properties dialog, show the metadata we
       * have even if we cannot decode the stream */
      if (!boolean && bvw->priv->use_type == BVW_USE_TYPE_METADATA &&
          bvw->priv->tagcache != NULL &&
          gst_structure_has_field ((GstStructure *) bvw->priv->tagcache,
                                   GST_TAG_AUDIO_CODEC)) {
        boolean = TRUE;
      }
      break;
    default:
      g_assert_not_reached ();
  }

  g_value_set_boolean (value, boolean);
  GST_DEBUG ("%s = %s", get_metadata_type_name (type), (boolean) ? "yes" : "no");

  return;
}

static void
bvw_process_pending_tag_messages (BaconVideoWidget * bvw)
{
  GstMessageType events;
  GstMessage *msg;
  GstBus *bus;
    
  /* process any pending tag messages on the bus NOW, so we can get to
   * the information without/before giving control back to the main loop */

  /* application message is for stream-info */
  events = GST_MESSAGE_TAG | GST_MESSAGE_DURATION | GST_MESSAGE_APPLICATION;
  bus = gst_element_get_bus (bvw->priv->play);
  while ((msg = gst_bus_poll (bus, events, 0))) {
    gst_bus_async_signal_func (bus, msg, NULL);
  }
  gst_object_unref (bus);
}

static GdkPixbuf *
bacon_video_widget_get_metadata_pixbuf (BaconVideoWidget * bvw,
					GstBuffer *buffer)
{
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf = NULL;
  GError *err = NULL;

  loader = gdk_pixbuf_loader_new ();

  if (gdk_pixbuf_loader_write (loader, buffer->data, buffer->size, &err) &&
      gdk_pixbuf_loader_close (loader, &err)) {
    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    if (pixbuf)
      g_object_ref (pixbuf);
  } else {
    GST_WARNING("could not convert tag image to pixbuf: %s", err->message);
    g_error_free (err);
  }

  g_object_unref (loader);
  return pixbuf;
}

static const GValue *
bacon_video_widget_get_best_image (BaconVideoWidget *bvw)
{
  const GValue *cover_value = NULL;
  guint i;

  for (i = 0; ; i++) {
    const GValue *value;
    GstBuffer *buffer;
    GstStructure *caps_struct;
    int type;

    value = gst_tag_list_get_value_index (bvw->priv->tagcache,
					  GST_TAG_IMAGE,
					  i);
    if (value == NULL)
      break;

    buffer = gst_value_get_buffer (value);

    caps_struct = gst_caps_get_structure (buffer->caps, 0);
    gst_structure_get_enum (caps_struct,
			    "image-type",
			    GST_TYPE_TAG_IMAGE_TYPE,
			    &type);
    if (type == GST_TAG_IMAGE_TYPE_UNDEFINED) {
      if (cover_value == NULL)
        cover_value = value;
    } else if (type == GST_TAG_IMAGE_TYPE_FRONT_COVER) {
      cover_value = value;
      break;
    }
  }

  return cover_value;
}

/**
 * bacon_video_widget_get_metadata:
 * @bvw: a #BaconVideoWidget
 * @type: the type of metadata to return
 * @value: a #GValue
 *
 * Provides metadata of the given @type about the current stream in @value.
 *
 * Free the #GValue with g_value_unset().
 **/
void
bacon_video_widget_get_metadata (BaconVideoWidget * bvw,
                                 BvwMetadataType type,
                                 GValue * value)
{
  g_return_if_fail (bvw != NULL);
  g_return_if_fail (BACON_IS_VIDEO_WIDGET (bvw));
  g_return_if_fail (GST_IS_ELEMENT (bvw->priv->play));

  switch (type)
    {
    case BVW_INFO_TITLE:
    case BVW_INFO_ARTIST:
    case BVW_INFO_YEAR:
    case BVW_INFO_COMMENT:
    case BVW_INFO_ALBUM:
    case BVW_INFO_VIDEO_CODEC:
    case BVW_INFO_AUDIO_CODEC:
    case BVW_INFO_AUDIO_CHANNELS:
      bacon_video_widget_get_metadata_string (bvw, type, value);
      break;
    case BVW_INFO_DURATION:
    case BVW_INFO_DIMENSION_X:
    case BVW_INFO_DIMENSION_Y:
    case BVW_INFO_FPS:
    case BVW_INFO_AUDIO_BITRATE:
    case BVW_INFO_VIDEO_BITRATE:
    case BVW_INFO_TRACK_NUMBER:
    case BVW_INFO_AUDIO_SAMPLE_RATE:
      bacon_video_widget_get_metadata_int (bvw, type, value);
      break;
    case BVW_INFO_HAS_VIDEO:
    case BVW_INFO_HAS_AUDIO:
      bacon_video_widget_get_metadata_bool (bvw, type, value);
      break;
    case BVW_INFO_COVER:
      {
        const GValue *cover_value;

	g_value_init (value, G_TYPE_OBJECT);

        if (bvw->priv->tagcache == NULL)
          break;
        cover_value = bacon_video_widget_get_best_image (bvw);
	if (!cover_value) {
	  cover_value = gst_tag_list_get_value_index (bvw->priv->tagcache,
						      GST_TAG_PREVIEW_IMAGE,
						      0);
	}
	if (cover_value) {
	  GstBuffer *buffer;
	  GdkPixbuf *pixbuf;

	  buffer = gst_value_get_buffer (cover_value);
	  pixbuf = bacon_video_widget_get_metadata_pixbuf (bvw, buffer);
	  if (pixbuf)
	    g_value_take_object (value, pixbuf);
	}
      }
      break;
    default:
      g_return_if_reached ();
    }

  return;
}

/* Screenshot functions */

/**
 * bacon_video_widget_can_get_frames:
 * @bvw: a #BaconVideoWidget
 * @error: a #GError, or %NULL
 *
 * Determines whether individual frames from the current stream can
 * be returned using bacon_video_widget_get_current_frame().
 *
 * Frames cannot be returned for audio-only streams, unless visualisations
 * are enabled.
 *
 * Return value: %TRUE if frames can be captured, %FALSE otherwise
 **/
gboolean
bacon_video_widget_can_get_frames (BaconVideoWidget * bvw, GError ** error)
{
  g_return_val_if_fail (bvw != NULL, FALSE);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), FALSE);

  /* check for version */
  if (!g_object_class_find_property (
           G_OBJECT_GET_CLASS (bvw->priv->play), "frame")) {
    g_set_error_literal (error, BVW_ERROR, BVW_ERROR_GENERIC,
                         _("Too old version of GStreamer installed."));
    return FALSE;
  }

  /* check for video */
  if (!bvw->priv->media_has_video && !bvw->priv->show_vfx) {
    g_set_error_literal (error, BVW_ERROR, BVW_ERROR_GENERIC,
        _("Media contains no supported video streams."));
    return FALSE;
  }

  return TRUE;
}

static void
destroy_pixbuf (guchar *pix, gpointer data)
{
  gst_buffer_unref (GST_BUFFER (data));
}

/**
 * bacon_video_widget_get_current_frame:
 * @bvw: a #BaconVideoWidget
 *
 * Returns a #GdkPixbuf containing the current frame from the playing
 * stream. This will wait for any pending seeks to complete before
 * capturing the frame.
 *
 * Return value: the current frame, or %NULL; unref with g_object_unref()
 **/
GdkPixbuf *
bacon_video_widget_get_current_frame (BaconVideoWidget * bvw)
{
  GstStructure *s;
  GstBuffer *buf = NULL;
  GdkPixbuf *pixbuf;
  GstCaps *to_caps;
  gint outwidth = 0;
  gint outheight = 0;

  g_return_val_if_fail (bvw != NULL, NULL);
  g_return_val_if_fail (BACON_IS_VIDEO_WIDGET (bvw), NULL);
  g_return_val_if_fail (GST_IS_ELEMENT (bvw->priv->play), NULL);

  /* when used as thumbnailer, wait for pending seeks to complete */
  if (bvw->priv->use_type == BVW_USE_TYPE_CAPTURE) {
    gst_element_get_state (bvw->priv->play, NULL, NULL, GST_CLOCK_TIME_NONE);
  }

  /* no video info */
  if (!bvw->priv->video_width || !bvw->priv->video_height) {
    GST_DEBUG ("Could not take screenshot: %s", "no video info");
    g_warning ("Could not take screenshot: %s", "no video info");
    return NULL;
  }

  /* get frame */
  g_object_get (bvw->priv->play, "frame", &buf, NULL);

  if (!buf) {
    GST_DEBUG ("Could not take screenshot: %s", "no last video frame");
    g_warning ("Could not take screenshot: %s", "no last video frame");
    return NULL;
  }

  if (GST_BUFFER_CAPS (buf) == NULL) {
    GST_DEBUG ("Could not take screenshot: %s", "no caps on buffer");
    g_warning ("Could not take screenshot: %s", "no caps on buffer");
    return NULL;
  }

  /* convert to our desired format (RGB24) */
  to_caps = gst_caps_new_simple ("video/x-raw-rgb",
      "bpp", G_TYPE_INT, 24,
      "depth", G_TYPE_INT, 24,
      /* Note: we don't ask for a specific width/height here, so that
       * videoscale can adjust dimensions from a non-1/1 pixel aspect
       * ratio to a 1/1 pixel-aspect-ratio */
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
      "endianness", G_TYPE_INT, G_BIG_ENDIAN,
      "red_mask", G_TYPE_INT, 0xff0000,
      "green_mask", G_TYPE_INT, 0x00ff00,
      "blue_mask", G_TYPE_INT, 0x0000ff,
      NULL);

  if (bvw->priv->video_fps_n > 0 && bvw->priv->video_fps_d > 0) {
    gst_caps_set_simple (to_caps, "framerate", GST_TYPE_FRACTION, 
      bvw->priv->video_fps_n, bvw->priv->video_fps_d, NULL);
  }

  GST_DEBUG ("frame caps: %" GST_PTR_FORMAT, GST_BUFFER_CAPS (buf));
  GST_DEBUG ("pixbuf caps: %" GST_PTR_FORMAT, to_caps);

  /* bvw_frame_conv_convert () takes ownership of the buffer passed */
  buf = bvw_frame_conv_convert (buf, to_caps);

  gst_caps_unref (to_caps);

  if (!buf) {
    GST_DEBUG ("Could not take screenshot: %s", "conversion failed");
    g_warning ("Could not take screenshot: %s", "conversion failed");
    return NULL;
  }

  if (!GST_BUFFER_CAPS (buf)) {
    GST_DEBUG ("Could not take screenshot: %s", "no caps on output buffer");
    g_warning ("Could not take screenshot: %s", "no caps on output buffer");
    return NULL;
  }

  s = gst_caps_get_structure (GST_BUFFER_CAPS (buf), 0);
  gst_structure_get_int (s, "width", &outwidth);
  gst_structure_get_int (s, "height", &outheight);
  g_return_val_if_fail (outwidth > 0 && outheight > 0, NULL);

  /* create pixbuf from that - use our own destroy function */
  pixbuf = gdk_pixbuf_new_from_data (GST_BUFFER_DATA (buf),
      GDK_COLORSPACE_RGB, FALSE, 8, outwidth, outheight,
      GST_ROUND_UP_4 (outwidth * 3), destroy_pixbuf, buf);

  if (!pixbuf) {
    GST_DEBUG ("Could not take screenshot: %s", "could not create pixbuf");
    g_warning ("Could not take screenshot: %s", "could not create pixbuf");
    gst_buffer_unref (buf);
  }

  return pixbuf;
}

/* =========================================== */
/*                                             */
/*          Widget typing & Creation           */
/*                                             */
/* =========================================== */

/* applications must use exactly one of bacon_video_widget_get_option_group()
 * OR bacon_video_widget_init_backend(), but not both */

/**
 * bacon_video_widget_get_option_group:
 *
 * Returns the #GOptionGroup containing command-line options for
 * #BaconVideoWidget.
 *
 * Applications must call either this or bacon_video_widget_init_backend() exactly
 * once; but not both.
 *
 * Return value: a #GOptionGroup giving command-line options for #BaconVideoWidget
 **/
GOptionGroup*
bacon_video_widget_get_option_group (void)
{
  return gst_init_get_option_group ();
}

/**
 * bacon_video_widget_init_backend:
 * @argc: pointer to application's argc
 * @argv: pointer to application's argv
 *
 * Initialises #BaconVideoWidget's GStreamer backend. If this fails
 * for the GStreamer backend, your application will be terminated.
 *
 * Applications must call either this or bacon_video_widget_get_option_group() exactly
 * once; but not both.
 **/
void
bacon_video_widget_init_backend (int *argc, char ***argv)
{
  gst_init (argc, argv);
}

GQuark
bacon_video_widget_error_quark (void)
{
  static GQuark q; /* 0 */

  if (G_UNLIKELY (q == 0)) {
    q = g_quark_from_static_string ("bvw-error-quark");
  }
  return q;
}

/* fold function to pick the best colorspace element */
static gboolean
find_colorbalance_element (GstElement *element, GValue * ret, GstElement **cb)
{
  GstColorBalanceClass *cb_class;

  GST_DEBUG ("Checking element %s ...", GST_OBJECT_NAME (element));

  if (!GST_IS_COLOR_BALANCE (element))
    return TRUE;

  GST_DEBUG ("Element %s is a color balance", GST_OBJECT_NAME (element));

  cb_class = GST_COLOR_BALANCE_GET_CLASS (element);
  if (GST_COLOR_BALANCE_TYPE (cb_class) == GST_COLOR_BALANCE_HARDWARE) {
    gst_object_replace ((GstObject **) cb, (GstObject *) element);
    /* shortcuts the fold */
    return FALSE;
  } else if (*cb == NULL) {
    gst_object_replace ((GstObject **) cb, (GstObject *) element);
    return TRUE;
  } else {
    return TRUE;
  }
}

static void
bvw_update_brightness_and_contrast_from_mateconf (BaconVideoWidget * bvw)
{
  MateConfValue *confvalue;
  guint i;

  g_return_if_fail (g_thread_self() == gui_thread);

  /* Setup brightness and contrast */
  GST_LOG ("updating brightness and contrast from MateConf settings");
  for (i = 0; i < G_N_ELEMENTS (video_props_str); i++) {
    confvalue = mateconf_client_get_without_default (bvw->priv->gc,
        video_props_str[i], NULL);
    if (confvalue != NULL) {
      bacon_video_widget_set_video_property (bvw, i,
        mateconf_value_get_int (confvalue));
      mateconf_value_free (confvalue);
    }
  }
}

static gboolean
bvw_update_interfaces_delayed (BaconVideoWidget *bvw)
{
  GST_DEBUG ("Delayed updating interface implementations");
  g_mutex_lock (bvw->priv->lock);
  bvw_update_interface_implementations (bvw);
  bvw->priv->interface_update_id = 0;
  g_mutex_unlock (bvw->priv->lock);

  return FALSE;
}

/* Must be called with bvw->priv->lock held */
static void
bvw_update_interface_implementations (BaconVideoWidget *bvw)
{
  GstColorBalance *old_balance = bvw->priv->balance;
  GstXOverlay *old_xoverlay = bvw->priv->xoverlay;
  GstElement *video_sink = NULL;
  GstElement *element = NULL;
  GstIterator *iter;
  GstElement *play;

  if (g_thread_self() != gui_thread) {
    if (bvw->priv->balance)
      gst_object_unref (bvw->priv->balance);
    bvw->priv->balance = NULL;
    if (bvw->priv->xoverlay)
      gst_object_unref (bvw->priv->xoverlay);
    bvw->priv->xoverlay = NULL;
    if (bvw->priv->navigation)
      gst_object_unref (bvw->priv->navigation);
    bvw->priv->navigation = NULL;

    if (bvw->priv->interface_update_id)
       g_source_remove (bvw->priv->interface_update_id);
    bvw->priv->interface_update_id =
        g_idle_add ((GSourceFunc) bvw_update_interfaces_delayed, bvw);
    return;
  }

  play = gst_object_ref(bvw->priv->play);

  g_mutex_unlock (bvw->priv->lock);
  g_object_get (bvw->priv->play, "video-sink", &video_sink, NULL);
  g_assert (video_sink != NULL);
  g_mutex_lock (bvw->priv->lock);

  gst_object_unref(play);

  /* We try to get an element supporting XOverlay interface */
  if (GST_IS_BIN (video_sink)) {
    GST_DEBUG ("Retrieving xoverlay from bin ...");
    element = gst_bin_get_by_interface (GST_BIN (video_sink),
                                        GST_TYPE_X_OVERLAY);
  } else {
    element = gst_object_ref(video_sink);
  }

  if (GST_IS_X_OVERLAY (element)) {
    GST_DEBUG ("Found xoverlay: %s", GST_OBJECT_NAME (element));
    bvw->priv->xoverlay = GST_X_OVERLAY (element);
  } else {
    GST_DEBUG ("No xoverlay found");
    if (element)
      gst_object_unref (element);
    bvw->priv->xoverlay = NULL;
  }

  /* Try to find the navigation interface */
  if (GST_IS_BIN (video_sink)) {
    GST_DEBUG ("Retrieving navigation from bin ...");
    element = gst_bin_get_by_interface (GST_BIN (video_sink),
                                        GST_TYPE_NAVIGATION);
  } else {
    element = gst_object_ref(video_sink);
  }

  if (GST_IS_NAVIGATION (element)) {
    GST_DEBUG ("Found navigation: %s", GST_OBJECT_NAME (element));
    bvw->priv->navigation = GST_NAVIGATION (element);
  } else {
    GST_DEBUG ("No navigation found");
    if (element)
      gst_object_unref (element);
    bvw->priv->navigation = NULL;
  }

  /* Find best color balance element (using custom iterator so
   * we can prefer hardware implementations to software ones) */

  /* FIXME: this doesn't work reliably yet, most of the time
   * the fold function doesn't even get called, while sometimes
   * it does ... */
  iter = gst_bin_iterate_all_by_interface (GST_BIN (bvw->priv->play),
                                           GST_TYPE_COLOR_BALANCE);
  /* naively assume no resync */
  element = NULL;
  gst_iterator_fold (iter,
      (GstIteratorFoldFunction) find_colorbalance_element, NULL, &element);
  gst_iterator_free (iter);

  if (element) {
    bvw->priv->balance = GST_COLOR_BALANCE (element);
    GST_DEBUG ("Best colorbalance found: %s",
        GST_OBJECT_NAME (bvw->priv->balance));
  } else if (GST_IS_COLOR_BALANCE (bvw->priv->xoverlay)) {
    bvw->priv->balance = GST_COLOR_BALANCE (bvw->priv->xoverlay);
    gst_object_ref (bvw->priv->balance);
    GST_DEBUG ("Colorbalance backup found: %s",
        GST_OBJECT_NAME (bvw->priv->balance));
  } else {
    GST_DEBUG ("No colorbalance found");
    bvw->priv->balance = NULL;
  }

  /* Setup brightness and contrast from configured values (do it delayed if
   * we're within a streaming thread, otherwise mateconf/matecorba/whatever may
   * iterate or otherwise mess with the default main context and cause all
   * kinds of nasty issues) */
  bvw_update_brightness_and_contrast_from_mateconf (bvw);

  if (old_xoverlay)
    gst_object_unref (GST_OBJECT (old_xoverlay));

  if (old_balance)
    gst_object_unref (GST_OBJECT (old_balance));

  gst_object_unref (video_sink);
}

static void
bvw_element_msg_sync (GstBus *bus, GstMessage *msg, gpointer data)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (data);

  g_assert (msg->type == GST_MESSAGE_ELEMENT);

  if (msg->structure == NULL)
    return;

  /* This only gets sent if we haven't set an ID yet. This is our last
   * chance to set it before the video sink will create its own window */
  if (gst_structure_has_name (msg->structure, "prepare-xwindow-id")) {
    XID window;

    GST_DEBUG ("Handling sync prepare-xwindow-id message");

    g_mutex_lock (bvw->priv->lock);
    bvw_update_interface_implementations (bvw);
    if (bvw->priv->xoverlay == NULL) {
      GstObject *sender = GST_MESSAGE_SRC (msg);
      if (sender && GST_IS_X_OVERLAY (sender))
        bvw->priv->xoverlay = GST_X_OVERLAY (gst_object_ref (sender));
    }
    g_mutex_unlock (bvw->priv->lock);

    g_return_if_fail (bvw->priv->xoverlay != NULL);
    g_return_if_fail (bvw->priv->video_window != NULL);

    window = GDK_WINDOW_XWINDOW (bvw->priv->video_window);
    gst_x_overlay_set_xwindow_id (bvw->priv->xoverlay, window);
  }
}

static gboolean
bvw_set_playback_direction (BaconVideoWidget *bvw, gboolean forward)
{
  gboolean is_forward;
  gboolean retval;

  is_forward = (bvw->priv->rate > 0.0);
  if (forward == is_forward)
    return TRUE;

  retval = FALSE;

  if (forward == FALSE) {
    GstEvent *event;
    GstFormat fmt;
    gint64 cur = 0;

    fmt = GST_FORMAT_TIME;
    if (gst_element_query_position (bvw->priv->play, &fmt, &cur)) {
      GST_DEBUG ("Setting playback direction to reverse at %"G_GINT64_FORMAT"", cur);
      event = gst_event_new_seek (REVERSE_RATE,
				  fmt, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
				  GST_SEEK_TYPE_SET, G_GINT64_CONSTANT (0),
				  GST_SEEK_TYPE_SET, cur);
      if (gst_element_send_event (bvw->priv->play, event) == FALSE) {
	GST_WARNING ("Failed to set playback direction to reverse");
      } else {
	gst_element_get_state (bvw->priv->play, NULL, NULL, GST_CLOCK_TIME_NONE);
	bvw->priv->rate = REVERSE_RATE;
	retval = TRUE;
      }
    } else {
      GST_LOG ("Failed to query position to set playback to reverse");
    }
  } else {
    GstEvent *event;
    GstFormat fmt;
    gint64 cur = 0;

    fmt = GST_FORMAT_TIME;
    cur = 0;
    if (gst_element_query_position (bvw->priv->play, &fmt, &cur)) {
      GST_DEBUG ("Setting playback direction to forward at %"G_GINT64_FORMAT"", cur);
      event = gst_event_new_seek (FORWARD_RATE,
				  fmt, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
				  GST_SEEK_TYPE_SET, cur,
				  GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
      if (gst_element_send_event (bvw->priv->play, event) == FALSE) {
	GST_WARNING ("Failed to set playback direction to forward");
      } else {
	gst_element_get_state (bvw->priv->play, NULL, NULL, GST_CLOCK_TIME_NONE);
	bvw->priv->rate = FORWARD_RATE;
	retval = TRUE;
      }
    } else {
      GST_LOG ("Failed to query position to set playback to forward");
    }
  }

  return retval;
}

static void
got_new_video_sink_bin_element (GstBin *video_sink, GstElement *element,
                                gpointer data)
{
  BaconVideoWidget *bvw = BACON_VIDEO_WIDGET (data);

  g_mutex_lock (bvw->priv->lock);
  bvw_update_interface_implementations (bvw);
  g_mutex_unlock (bvw->priv->lock);
}

/**
 * bacon_video_widget_new:
 * @width: initial or expected video width, in pixels, or %-1
 * @height: initial or expected video height, in pixels, or %-1
 * @type: the widget's use type
 * @error: a #GError, or %NULL
 *
 * Creates a new #BaconVideoWidget for the purpose specified in @type.
 *
 * If @type is %BVW_USE_TYPE_VIDEO, the #BaconVideoWidget will be fully-featured; other
 * values of @type will enable less functionality on the widget, which will come with
 * corresponding decreases in the size of its memory footprint.
 *
 * @width and @height give the initial or expected video height. Set them to %-1 if the
 * video size is unknown. For small videos, #BaconVideoWidget will be configured differently.
 *
 * A #BvwError will be returned on error.
 *
 * Return value: a new #BaconVideoWidget, or %NULL; destroy with gtk_widget_destroy()
 **/
GtkWidget *
bacon_video_widget_new (int width, int height,
                        BvwUseType type, GError ** error)
{
  MateConfValue *confvalue;
  BaconVideoWidget *bvw;
  GstElement *audio_sink = NULL, *video_sink = NULL;
  gchar *version_str;
  GstPlayFlags flags;

#ifndef GST_DISABLE_GST_DEBUG
  if (_totem_gst_debug_cat == NULL) {
    GST_DEBUG_CATEGORY_INIT (_totem_gst_debug_cat, "totem", 0,
        "Totem GStreamer Backend");
  }
#endif

  version_str = gst_version_string ();
  GST_DEBUG ("Initialised %s", version_str);
  g_free (version_str);

  gst_pb_utils_init ();

  bvw = BACON_VIDEO_WIDGET (g_object_new
                            (bacon_video_widget_get_type (), NULL));

  bvw->priv->use_type = type;
  GST_DEBUG ("use_type = %d", type);

  bvw->priv->play = gst_element_factory_make ("playbin2", "play");
  if (!bvw->priv->play) {
    g_set_error_literal (error, BVW_ERROR, BVW_ERROR_PLUGIN_LOAD,
                 _("Failed to create a GStreamer play object. "
                   "Please check your GStreamer installation."));
    g_object_ref_sink (bvw);
    g_object_unref (bvw);
    return NULL;
  }

  bvw->priv->bus = gst_element_get_bus (bvw->priv->play);

  /* Add the download flag, for streaming buffering,
   * and the deinterlace flag, for video only */
  if (type == BVW_USE_TYPE_VIDEO) {
    g_object_get (bvw->priv->play, "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_DOWNLOAD;
    g_object_set (bvw->priv->play, "flags", flags, NULL);
    flags |= GST_PLAY_FLAG_DEINTERLACE;
    g_object_set (bvw->priv->play, "flags", flags, NULL);
  }

  /* Disable video decoding in audio mode */
  if (type == BVW_USE_TYPE_AUDIO) {
    g_object_get (bvw->priv->play, "flags", &flags, NULL);
    flags &= ~GST_PLAY_FLAG_VIDEO;
    g_object_set (bvw->priv->play, "flags", flags, NULL);
  }

  gst_bus_add_signal_watch (bvw->priv->bus);

  bvw->priv->sig_bus_async = 
      g_signal_connect (bvw->priv->bus, "message", 
                        G_CALLBACK (bvw_bus_message_cb),
                        bvw);

  bvw->priv->speakersetup = BVW_AUDIO_SOUND_STEREO;
  bvw->priv->media_device = g_strdup ("/dev/dvd");
  bvw->priv->visq = VISUAL_SMALL;
  bvw->priv->show_vfx = FALSE;
  bvw->priv->vis_element_name = g_strdup ("goom");
  bvw->priv->connection_speed = 11;
  bvw->priv->ratio_type = BVW_RATIO_AUTO;

  bvw->priv->cursor_shown = TRUE;
  bvw->priv->logo_mode = FALSE;
  bvw->priv->auto_resize = FALSE;

  /* mateconf setting in backend */
  bvw->priv->gc = mateconf_client_get_default ();

  if (type == BVW_USE_TYPE_VIDEO || type == BVW_USE_TYPE_AUDIO) {
    audio_sink = gst_element_factory_make ("gconfaudiosink", "audio-sink");
    if (audio_sink == NULL) {
      g_warning ("Could not create element 'gconfaudiosink'");
      /* Try to fallback on autoaudiosink */
      audio_sink = gst_element_factory_make ("autoaudiosink", "audio-sink");
    } else {
      /* set the profile property on the gconfaudiosink to "music and movies" */
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (audio_sink), "profile"))
        g_object_set (G_OBJECT (audio_sink), "profile", 1, NULL);
    }
  } else {
    audio_sink = gst_element_factory_make ("fakesink", "audio-fake-sink");
  }

  if (type == BVW_USE_TYPE_VIDEO) {
    if (width > 0 && width < SMALL_STREAM_WIDTH &&
        height > 0 && height < SMALL_STREAM_HEIGHT) {
      GST_INFO ("forcing ximagesink, image size only %dx%d", width, height);
      video_sink = gst_element_factory_make ("ximagesink", "video-sink");
    } else {
      video_sink = gst_element_factory_make ("gconfvideosink", "video-sink");
      if (video_sink == NULL) {
        g_warning ("Could not create element 'gconfvideosink'");
        /* Try to fallback on ximagesink */
        video_sink = gst_element_factory_make ("ximagesink", "video-sink");
      }
    }

    /* Set the display if the video sink supports it */
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (video_sink), "display")) {
      const char *display;

      display = gdk_get_display_arg_name();
      if (display != NULL)
	g_object_set (G_OBJECT (video_sink), "display", display, NULL);
    }
  } else {
    video_sink = gst_element_factory_make ("fakesink", "video-fake-sink");
    if (video_sink)
      g_object_set (video_sink, "sync", TRUE, NULL);
  }

  if (video_sink) {
    GstStateChangeReturn ret;

    /* need to set bus explicitly as it's not in a bin yet and
     * poll_for_state_change() needs one to catch error messages */
    gst_element_set_bus (video_sink, bvw->priv->bus);
    /* state change NULL => READY should always be synchronous */
    ret = gst_element_set_state (video_sink, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      /* Drop this video sink */
      gst_element_set_state (video_sink, GST_STATE_NULL);
      gst_object_unref (video_sink);
      /* Try again with ximagesink */
      video_sink = gst_element_factory_make ("ximagesink", "video-sink");
      gst_element_set_bus (video_sink, bvw->priv->bus);
      ret = gst_element_set_state (video_sink, GST_STATE_READY);
      if (ret == GST_STATE_CHANGE_FAILURE) {
        GstMessage *err_msg;

        err_msg = gst_bus_poll (bvw->priv->bus, GST_MESSAGE_ERROR, 0);
        if (err_msg == NULL) {
          g_warning ("Should have gotten an error message, please file a bug.");
          g_set_error_literal (error, BVW_ERROR, BVW_ERROR_VIDEO_PLUGIN,
               _("Failed to open video output. It may not be available. "
                 "Please select another video output in the Multimedia "
                 "Systems Selector."));
        } else if (error) {
          *error = bvw_error_from_gst_error (bvw, err_msg);
          gst_message_unref (err_msg);
        }
        goto sink_error;
      }
    }
  } else {
    g_set_error_literal (error, BVW_ERROR, BVW_ERROR_VIDEO_PLUGIN,
                 _("Could not find the video output. "
                   "You may need to install additional GStreamer plugins, "
                   "or select another video output in the Multimedia Systems "
                   "Selector."));
    goto sink_error;
  }

  if (audio_sink) {
    GstStateChangeReturn ret;
    GstBus *bus;

    /* need to set bus explicitly as it's not in a bin yet and
     * we need one to catch error messages */
    bus = gst_bus_new ();
    gst_element_set_bus (audio_sink, bus);

    /* state change NULL => READY should always be synchronous */
    ret = gst_element_set_state (audio_sink, GST_STATE_READY);
    gst_element_set_bus (audio_sink, NULL);

    if (ret == GST_STATE_CHANGE_FAILURE) {
      /* doesn't work, drop this audio sink */
      gst_element_set_state (audio_sink, GST_STATE_NULL);
      gst_object_unref (audio_sink);
      audio_sink = NULL;
      /* Hopefully, fakesink should always work */
      if (type != BVW_USE_TYPE_AUDIO)
        audio_sink = gst_element_factory_make ("fakesink", "audio-sink");
      if (audio_sink == NULL) {
        GstMessage *err_msg;

        err_msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0);
        if (err_msg == NULL) {
          g_warning ("Should have gotten an error message, please file a bug.");
          g_set_error_literal (error, BVW_ERROR, BVW_ERROR_AUDIO_PLUGIN,
                       _("Failed to open audio output. You may not have "
                         "permission to open the sound device, or the sound "
                         "server may not be running. "
                         "Please select another audio output in the Multimedia "
                         "Systems Selector."));
        } else if (error) {
          *error = bvw_error_from_gst_error (bvw, err_msg);
          gst_message_unref (err_msg);
        }
        gst_object_unref (bus);
        goto sink_error;
      }
      /* make fakesink sync to the clock like a real sink */
      g_object_set (audio_sink, "sync", TRUE, NULL);
      GST_DEBUG ("audio sink doesn't work, using fakesink instead");
      bvw->priv->uses_audio_fakesink = TRUE;
    }
    gst_object_unref (bus);
  } else {
    g_set_error_literal (error, BVW_ERROR, BVW_ERROR_AUDIO_PLUGIN,
                 _("Could not find the audio output. "
                   "You may need to install additional GStreamer plugins, or "
                   "select another audio output in the Multimedia Systems "
                   "Selector."));
    goto sink_error;
  }

  /* set back to NULL to close device again in order to avoid interrupts
   * being generated after startup while there's nothing to play yet */
  gst_element_set_state (audio_sink, GST_STATE_NULL);

  do {
    GstElement *bin;
    GstPad *pad;

    bvw->priv->audio_capsfilter =
        gst_element_factory_make ("capsfilter", "audiofilter");
    bin = gst_bin_new ("audiosinkbin");
    gst_bin_add_many (GST_BIN (bin), bvw->priv->audio_capsfilter,
        audio_sink, NULL);
    gst_element_link_pads (bvw->priv->audio_capsfilter, "src",
        audio_sink, "sink");

    pad = gst_element_get_static_pad (bvw->priv->audio_capsfilter, "sink");
    gst_element_add_pad (bin, gst_ghost_pad_new ("sink", pad));
    gst_object_unref (pad);

    audio_sink = bin;
  } while (0);

  /* now tell playbin */
  g_object_set (bvw->priv->play, "video-sink", video_sink, NULL);
  g_object_set (bvw->priv->play, "audio-sink", audio_sink, NULL);

  bvw->priv->vis_plugins_list = NULL;

  g_signal_connect (G_OBJECT (bvw->priv->play), "notify::volume",
      G_CALLBACK (notify_volume_cb), bvw);
  g_signal_connect (bvw->priv->play, "notify::source",
      G_CALLBACK (playbin_source_notify_cb), bvw);
  g_signal_connect (bvw->priv->play, "video-changed",
      G_CALLBACK (playbin_stream_changed_cb), bvw);
  g_signal_connect (bvw->priv->play, "audio-changed",
      G_CALLBACK (playbin_stream_changed_cb), bvw);
  g_signal_connect (bvw->priv->play, "text-changed",
      G_CALLBACK (playbin_stream_changed_cb), bvw);
  g_signal_connect (bvw->priv->play, "deep-notify::temp-location",
      G_CALLBACK (playbin_deep_notify_cb), bvw);

  g_signal_connect (bvw->priv->play, "video-tags-changed",
      G_CALLBACK (video_tags_changed_cb), bvw);
  g_signal_connect (bvw->priv->play, "audio-tags-changed",
      G_CALLBACK (audio_tags_changed_cb), bvw);
  g_signal_connect (bvw->priv->play, "text-tags-changed",
      G_CALLBACK (text_tags_changed_cb), bvw);

  /* assume we're always called from the main Gtk+ GUI thread */
  gui_thread = g_thread_self();

  if (type == BVW_USE_TYPE_VIDEO) {
    GstStateChangeReturn ret;

    /* wait for video sink to finish changing to READY state, 
     * otherwise we won't be able to detect the colorbalance interface */
    ret = gst_element_get_state (video_sink, NULL, NULL, 5 * GST_SECOND);
    if (ret != GST_STATE_CHANGE_SUCCESS) {
      GST_WARNING ("Timeout setting videosink to READY");
      g_set_error_literal (error, BVW_ERROR, BVW_ERROR_VIDEO_PLUGIN,
          _("Failed to open video output. It may not be available. "
          "Please select another video output in the Multimedia Systems Selector."));
      return NULL;
    }
    g_mutex_lock (bvw->priv->lock);
    bvw_update_interface_implementations (bvw);
    g_mutex_unlock (bvw->priv->lock);
  }

  /* we want to catch "prepare-xwindow-id" element messages synchronously */
  gst_bus_set_sync_handler (bvw->priv->bus, gst_bus_sync_signal_handler, bvw);

  bvw->priv->sig_bus_sync = 
      g_signal_connect (bvw->priv->bus, "sync-message::element",
                        G_CALLBACK (bvw_element_msg_sync), bvw);

  if (GST_IS_BIN (video_sink)) {
    /* video sink bins like gconfvideosink might remove their children and
     * create new ones when set to NULL state, and they are currently set
     * to NULL state whenever playbin re-creates its internal video bin
     * (it sets all elements to NULL state before gst_bin_remove()ing them) */
    g_signal_connect (video_sink, "element-added",
                      G_CALLBACK (got_new_video_sink_bin_element), bvw);
  }

  /* audio out, if any */
  confvalue = mateconf_client_get_without_default (bvw->priv->gc,
      MATECONF_PREFIX"/audio_output_type", NULL);
  if (confvalue != NULL &&
      (type != BVW_USE_TYPE_METADATA && type != BVW_USE_TYPE_CAPTURE)) {
    bvw->priv->speakersetup = mateconf_value_get_int (confvalue);
    bacon_video_widget_set_audio_out_type (bvw, bvw->priv->speakersetup);
    mateconf_value_free (confvalue);
  } else if (type == BVW_USE_TYPE_METADATA || type == BVW_USE_TYPE_CAPTURE) {
    bvw->priv->speakersetup = -1;
    /* don't set up a filter for the speaker setup, anything is fine */
  } else {
    bvw->priv->speakersetup = -1;
    bacon_video_widget_set_audio_out_type (bvw, BVW_AUDIO_SOUND_STEREO);
  }

  /* tv/conn (not used yet) */
  confvalue = mateconf_client_get_without_default (bvw->priv->gc,
      MATECONF_PREFIX "/connection_speed", NULL);
  if (confvalue != NULL) {
    bacon_video_widget_set_connection_speed (bvw,
        mateconf_value_get_int (confvalue)); 
    mateconf_value_free (confvalue);
  } else {
    bacon_video_widget_set_connection_speed (bvw,
    	bvw->priv->connection_speed);
  }

  return GTK_WIDGET (bvw);

  /* errors */
sink_error:
  {
    if (video_sink) {
      gst_element_set_state (video_sink, GST_STATE_NULL);
      gst_object_unref (video_sink);
    }
    if (audio_sink) {
      gst_element_set_state (audio_sink, GST_STATE_NULL);
      gst_object_unref (audio_sink);
    }

    g_object_ref (bvw);
    g_object_ref_sink (G_OBJECT (bvw));
    g_object_unref (bvw);

    return NULL;
  }
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
