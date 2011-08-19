/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* MATE Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * applet.c: the main applet
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* this is for lrint */
#define _ISOC99_SOURCE
#include <math.h>
#include <string.h>

#include <glib-object.h>
#include <gdk/gdkkeysyms.h>

#include <gtk/gtk.h>

#include <mateconf/mateconf-client.h>

#include "applet.h"
#include "keys.h"
#include "preferences.h"

#define IS_PANEL_HORIZONTAL(o) \
  (o == MATE_PANEL_APPLET_ORIENT_UP || o == MATE_PANEL_APPLET_ORIENT_DOWN)

/* This is defined is load.c, we're doing this instead of creating a load.h file
 * because nothing else is exported. */
GList * mate_volume_applet_create_mixer_collection (void);

static void	mate_volume_applet_class_init	(MateVolumeAppletClass *klass);
static void	mate_volume_applet_init	(MateVolumeApplet *applet);
static void	mate_volume_applet_dispose	(GObject   *object);

static void     mate_volume_applet_size_allocate (GtkWidget     *widget,
						   GtkAllocation *allocation);

static void	mate_volume_applet_popup_dock	(MateVolumeApplet *applet);
static void	mate_volume_applet_popdown_dock (MateVolumeApplet *applet);

/* This function and mate_volume_applet_key are not static so we can
 * inject external events into the applet. Its to work around a GTK+
 * misfeature. See dock.c for details. */
gboolean	mate_volume_applet_scroll	(GtkWidget *widget,
						 GdkEventScroll *event);
static gboolean	mate_volume_applet_button	(GtkWidget *widget,
						 GdkEventButton *event);
gboolean	mate_volume_applet_key		(GtkWidget *widget,
						 GdkEventKey *event);
static gdouble  mate_volume_applet_get_volume  (GstMixer *mixer,
						 GstMixerTrack *track);

static void	mate_volume_applet_background	(MatePanelApplet *mate_panel_applet,
						 MatePanelAppletBackgroundType type,
						 GdkColor  *colour,
						 GdkPixmap *pixmap);
static void	mate_volume_applet_orientation	(MatePanelApplet *applet,
						 MatePanelAppletOrient orient);

static gboolean	mate_volume_applet_refresh	(MateVolumeApplet *applet,
						 gboolean           force_refresh,
						 gdouble            volume,
						 gint               mute);

static void     cb_notify_message (GstBus *bus, GstMessage *message, gpointer data);
static gboolean	cb_check			(gpointer   data);

static void	cb_volume			(GtkAdjustment *adj,
						 gpointer   data);

static void	cb_mateconf			(MateConfClient     *client,
						 guint            connection_id,
						 MateConfEntry      *entry,
						 gpointer         data);

static void	cb_verb				(GtkAction *action,
						 gpointer   data);

static void	cb_theme_change                (GtkIconTheme *icon_theme,
						gpointer      data);
static void	cb_stop_scroll_events		(GtkWidget *widget,
						 GdkEvent  *event);

static MatePanelAppletClass *parent_class = NULL;


G_DEFINE_TYPE (MateVolumeApplet, mate_volume_applet, PANEL_TYPE_APPLET)


static void
init_pixbufs (MateVolumeApplet *applet)
{
  static const gchar *pix_filenames[] = {
    "audio-volume-muted",
    "audio-volume-low",
    "audio-volume-medium",
    "audio-volume-high",
    NULL
  };
  gint n;

  for (n = 0; pix_filenames[n] != NULL; n++) {
    if (applet->pix[n]) {
      g_object_unref (applet->pix[n]);
      applet->pix[n] = NULL; // mate_icon_theme_load_icon can call us
                             // recursively, so we have to be careful.
    }

    applet->pix[n] = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
					       pix_filenames[n],
					       applet->panel_size - 4,
					       0,
					       NULL);
    if (applet->pix[n] != NULL &&
	gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL) {
      GdkPixbuf *temp;

      temp = gdk_pixbuf_flip (applet->pix[n], TRUE);
      g_object_unref (G_OBJECT (applet->pix[n]));
      applet->pix[n] = temp;
    }
  }
}

static void
mate_volume_applet_class_init (MateVolumeAppletClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
  MatePanelAppletClass *matepanelapplet_class = MATE_PANEL_APPLET_CLASS (klass);

  parent_class = g_type_class_ref (PANEL_TYPE_APPLET);

  gobject_class->dispose = mate_volume_applet_dispose;
  gtkwidget_class->key_press_event = mate_volume_applet_key;
  gtkwidget_class->button_press_event = mate_volume_applet_button;
  gtkwidget_class->scroll_event = mate_volume_applet_scroll;
  gtkwidget_class->size_allocate = mate_volume_applet_size_allocate;
  matepanelapplet_class->change_orient = mate_volume_applet_orientation;
  matepanelapplet_class->change_background = mate_volume_applet_background;

  /* FIXME:
   * - style-set.
   */
}

static void
mate_volume_applet_init (MateVolumeApplet *applet)
{
  GtkWidget *image;
  AtkObject *ao;

  applet->timeout = 0;
  applet->elements = NULL;
  applet->client = mateconf_client_get_default ();
  applet->mixer = NULL;
  applet->tracks = NULL;
  applet->lock = FALSE;
  applet->state = -1;
  applet->prefs = NULL;
  applet->dock = NULL;
  applet->adjustment = NULL;
  applet->panel_size = 24;

  g_set_application_name (_("Volume Applet"));

  /* init pixbufs */
  init_pixbufs (applet);

  /* icon (our main UI) */
  image = gtk_image_new ();
  applet->image = GTK_IMAGE (image);
  gtk_container_add (GTK_CONTAINER (applet), image);
  gtk_widget_show (image);
  gtk_window_set_default_icon_name ("multimedia-volume-control");

  /* dock window (expanded UI) */
  applet->pop = FALSE;

  /* tooltip over applet */
  gtk_widget_set_tooltip_text (GTK_WIDGET (applet), _("Volume Control"));

  /* prevent scroll events from reaching the tooltip */
  g_signal_connect (G_OBJECT (applet),
		    "event-after", G_CALLBACK (cb_stop_scroll_events),
		    NULL);

  /* handle icon theme changes */
  g_signal_connect (gtk_icon_theme_get_default (),
		    "changed", G_CALLBACK (cb_theme_change),
		    applet);

  /* other stuff */
  mate_panel_applet_add_preferences (MATE_PANEL_APPLET (applet),
				"/schemas/apps/mixer_applet/prefs",
				NULL);
  mate_panel_applet_set_flags (MATE_PANEL_APPLET (applet),
			  MATE_PANEL_APPLET_EXPAND_MINOR);

  /* i18n */
  ao = gtk_widget_get_accessible (GTK_WIDGET (applet));
  atk_object_set_name (ao, _("Volume Control"));

  /* Watch for signals from GST. */
  applet->bus = gst_bus_new ();
  gst_bus_add_signal_watch (applet->bus);
  g_signal_connect (G_OBJECT (applet->bus), "message::element",
		    (GCallback) cb_notify_message, applet);

}

/* Parse the list of tracks that are stored in MateConf */

static char **
parse_track_list (const char *track_list)
{
  if (track_list)
    return g_strsplit (track_list, ":", 0);
  else
    return NULL;
}

static GList *
select_tracks (GstElement *element,
	      const char *active_track_names,
	      gboolean    reset_state)
{
  const GList *tracks, *l;
  GstMixerTrack *track_fallback;
  GList *active_tracks;
  char **active_track_name_list;

  active_tracks = NULL;
  track_fallback = NULL;
  active_track_name_list = NULL;

  if (reset_state) {
    gst_element_set_state (element, GST_STATE_READY);
    if (gst_element_get_state(element, NULL, NULL, -1) != GST_STATE_CHANGE_SUCCESS)
      return NULL;
  }

  tracks = gst_mixer_list_tracks (GST_MIXER (element));
  if (active_track_names)
    active_track_name_list = parse_track_list (active_track_names);

  for (l = tracks; l; l = l->next) {
    GstMixerTrack *track = l->data;
    gint i;

    if (!track->num_channels)
      continue;

    if (!track_fallback)
      track_fallback = track;

    if (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_MASTER))
      track_fallback = track;

    if (active_track_name_list) {
      for (i = 0; active_track_name_list[i] != NULL; i++) {
	gchar *track_test = active_track_name_list[i];

	if (!strcmp (track_test, track->label))
	  active_tracks = g_list_append (active_tracks, track);
      }
    }
  }

  /* if the list had no matches and we've got a fallback track,
   * then use it. */

  if (!active_tracks && track_fallback)
    active_tracks = g_list_append (active_tracks, track_fallback);

  if (!active_tracks && reset_state) {
    gst_element_set_state (element, GST_STATE_NULL);
  }

  g_strfreev (active_track_name_list);
  return active_tracks;
}

static gboolean
select_element_and_track (MateVolumeApplet *applet,
			  GList             *elements,
			  const char        *active_element_name,
			  const char        *active_track_names)
{
  GstElement *active_element;
  GList *active_tracks, *l;

  applet->elements = elements;

  active_element = NULL;
  active_tracks = NULL;

  if (active_element_name) {
    for (l = elements; l; l = l->next) {
      GstElement *element = l->data;
      const char *element_name;

      element_name = g_object_get_data (G_OBJECT (element),
				      "mate-volume-applet-name");

      if (!strcmp (element_name, active_element_name)) {
	active_element = element;
	break;
      }
    }
  }

  if (active_element)
    active_tracks = select_tracks (active_element, active_track_names, TRUE);

  if (!active_tracks) {
    active_element = NULL;
    for (l = elements; l; l = l->next) {
      GstElement *element = l->data;

      if ((active_tracks = select_tracks (element, active_track_names, TRUE))) {
	active_element = element;
	break;
      }
    }
  }

  if (!active_element)
    return FALSE;

  applet->mixer = g_object_ref (active_element);
  gst_element_set_bus (GST_ELEMENT (applet->mixer), applet->bus);
  applet->tracks = active_tracks;
  g_list_foreach (applet->tracks, (GFunc) g_object_ref, NULL);

  return TRUE;
}

static void
mate_volume_applet_setup_timeout (MateVolumeApplet *applet)
{
  gboolean need_timeout = TRUE;
  need_timeout = ((gst_mixer_get_mixer_flags (GST_MIXER (applet->mixer)) &
      GST_MIXER_FLAG_AUTO_NOTIFICATIONS) == 0);

  if (need_timeout) {
    if (applet->timeout == 0) {
      applet->timeout = g_timeout_add (100, cb_check, applet);
    }
  }
  else {
    if (applet->timeout != 0) {
      g_source_remove (applet->timeout);
      applet->timeout = 0;
    }
  }
}

gboolean
mate_volume_applet_setup (MateVolumeApplet *applet,
			   GList *elements)
{
  GtkObject *adj;
  static const GtkActionEntry actions[] = {
    { "RunMixer", NULL, N_("_Open Volume Control"),
      NULL, NULL,
      G_CALLBACK (cb_verb) },
    { "Help", GTK_STOCK_HELP, N_("_Help"),
      NULL, NULL,
      G_CALLBACK (cb_verb) },
    { "About", GTK_STOCK_ABOUT, N_("_About"),
      NULL, NULL,
      G_CALLBACK (cb_verb) },
    { "Pref", GTK_STOCK_PROPERTIES, N_("_Preferences"),
      NULL, NULL,
      G_CALLBACK (cb_verb) }
  };
  static const GtkToggleActionEntry toggle_actions[] = {
    { "Mute", NULL, N_("Mu_te"),
      NULL, NULL,
      G_CALLBACK (cb_verb), FALSE }
  };

  gchar *key;
  gchar *active_element_name;
  gchar *active_track_name;
  gchar *ui_path;
  GstMixerTrack *first_track;
  gboolean res;

  active_element_name = mate_panel_applet_mateconf_get_string (MATE_PANEL_APPLET (applet),
						       MATE_VOLUME_APPLET_KEY_ACTIVE_ELEMENT,
						       NULL);

  active_track_name = mate_panel_applet_mateconf_get_string (MATE_PANEL_APPLET (applet),
						     MATE_VOLUME_APPLET_KEY_ACTIVE_TRACK,
						     NULL);

  res = select_element_and_track (applet, elements, active_element_name,
				  active_track_name);
  g_free (active_element_name);
  g_free (active_track_name);

  if (res) {
    first_track = g_list_first (applet->tracks)->data;

    applet->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (50, 0, 100,
							     4, 10, 0));
    /* We want a reference from the applet as well as from the dock it
     * will be attached to. */
    g_object_ref_sink (applet->adjustment);
    g_signal_connect (applet->adjustment, "value-changed",
		      G_CALLBACK (cb_volume), applet);

    gtk_adjustment_set_value (applet->adjustment,
			      mate_volume_applet_get_volume (applet->mixer,
							      first_track));
  }

  mate_volume_applet_orientation (MATE_PANEL_APPLET (applet),
				   mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet)));

  /* menu */
  applet->action_group = gtk_action_group_new ("Mixer Applet Actions");
  gtk_action_group_set_translation_domain (applet->action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (applet->action_group,
				actions,
				G_N_ELEMENTS (actions),
				applet);
  gtk_action_group_add_toggle_actions (applet->action_group,
				       toggle_actions,
				       G_N_ELEMENTS (toggle_actions),
				       applet);
  ui_path = g_build_filename (MIXER_MENU_UI_DIR, "mixer-applet-menu.xml", NULL);
  mate_panel_applet_setup_menu_from_file (MATE_PANEL_APPLET (applet),
				     ui_path, applet->action_group);
  g_free (ui_path);

  mate_volume_applet_refresh (applet, TRUE, -1, -1);
  if (res) {
    mate_volume_applet_setup_timeout (applet);

    /* mateconf */
    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (applet),
				MATE_VOLUME_APPLET_KEY_ACTIVE_ELEMENT);
    mateconf_client_notify_add (applet->client, key,
			     cb_mateconf, applet, NULL, NULL);
    g_free (key);
    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (applet),
				MATE_VOLUME_APPLET_KEY_ACTIVE_TRACK);
    mateconf_client_notify_add (applet->client, key,
			     cb_mateconf, applet, NULL, NULL);
    g_free (key);
  }

  gtk_widget_show (GTK_WIDGET (applet));

  return TRUE;
}

static void
mate_volume_applet_dispose (GObject *object)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (object);
  gint n;

  mate_volume_applet_popdown_dock (applet);

  if (applet->action_group) {
    g_object_unref (applet->action_group);
    applet->action_group = NULL;
  }

  if (applet->elements) {
    GList *item;

    for (item = applet->elements; item != NULL; item = item->next) {
      GstElement *element = GST_ELEMENT (item->data);

      gst_element_set_state (element, GST_STATE_NULL);
      gst_object_unref (GST_OBJECT (element));
    }
    g_list_free (applet->elements);
    applet->elements = NULL;
  }

  if (applet->tracks) {
    g_list_foreach (applet->tracks, (GFunc) g_object_unref, NULL);
    g_list_free (applet->tracks);
    applet->tracks = NULL;
  }

  if (applet->mixer) {
    gst_object_unref (GST_OBJECT (applet->mixer));
    applet->mixer = NULL;
  }

  if (applet->timeout) {
    g_source_remove (applet->timeout);
    applet->timeout = 0;
  }

  if (applet->dock) {
    g_object_unref (applet->dock);
    applet->dock = NULL;
  }

  if (applet->adjustment) {
    g_object_unref (applet->adjustment);
    applet->adjustment = NULL;
  }

  for (n = 0; n < 5; n++) {
    if (applet->pix[n] != NULL) {
      g_object_unref (G_OBJECT (applet->pix[n]));
      applet->pix[n] = NULL;
    }
  }

  if (applet->bus) {
    gst_bus_remove_signal_watch (applet->bus);
    gst_object_unref (applet->bus);
    applet->bus = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Show a dialog (once) when no mixer is available.
 */

static void
show_no_mixer_dialog (MateVolumeApplet *applet)
{
  static gboolean shown = FALSE;
  GtkWidget *dialog;

  if (shown)
    return;
  shown = TRUE;

  dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE, "%s\n\n%s",
		_("The volume control did not find any elements and/or "
		  "devices to control. This means either that you don't "
		  "have the right GStreamer plugins installed, or that you "
		  "don't have a sound card configured."),
		_("You can remove the volume control from the panel by "
		  "right-clicking the speaker icon on the panel and "
		  "selecting \"Remove From Panel\" from the menu."));
  gtk_widget_show (dialog);
  g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
}

/*
 * get position that the dock should get based on applet position.
 */

static void
mate_volume_applet_get_dock_position (MateVolumeApplet *applet,
				       gint *_x, gint *_y)
{
  GtkWidget *widget = GTK_WIDGET (applet);
  GtkAllocation widget_allocation, dock_allocation;
  gint x, y;

  gtk_widget_get_allocation (GTK_WIDGET (applet->dock), &dock_allocation);
  gtk_widget_get_allocation (widget, &widget_allocation);

  gdk_window_get_origin (gtk_widget_get_window (widget), &x, &y);
  switch (mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet))) {
    case MATE_PANEL_APPLET_ORIENT_DOWN:
      x += widget_allocation.x;
      x -= (dock_allocation.width -
          widget_allocation.width) / 2;
      y += widget_allocation.height + widget_allocation.y;
      break;
    case MATE_PANEL_APPLET_ORIENT_UP:
      x += widget_allocation.x;
      x -= (dock_allocation.width -
          widget_allocation.width) / 2;
      y += widget_allocation.y;
      y -= dock_allocation.height;
      break;
    case MATE_PANEL_APPLET_ORIENT_RIGHT:
      x += widget_allocation.width + widget_allocation.x;
      y += widget_allocation.y;
      y -= (dock_allocation.height -
          widget_allocation.height) / 2;
      break;
    case MATE_PANEL_APPLET_ORIENT_LEFT:
      x += widget_allocation.x;
      x -= dock_allocation.width;
      y += widget_allocation.y;
      y -= (dock_allocation.height -
          widget_allocation.height) / 2;
      break;
    default:
      g_assert_not_reached ();
  }

  *_x = x;
  *_y = y;
}

/*
 * popup (show) or popdown (hide) the dock.
 */

static void
mate_volume_applet_popup_dock (MateVolumeApplet *applet)
{
  GtkWidget *widget = GTK_WIDGET (applet);
  gint x, y;

  /* Get it in just about the right position so that it
   * doesn't flicker to obviously when we reposition it. */
  mate_volume_applet_get_dock_position (applet, &x, &y);
  gtk_window_move (GTK_WINDOW (applet->dock), x, y);

  gtk_widget_show_all (GTK_WIDGET (applet->dock));

  /* Reposition the window now that we know its actual size
   * and can center it. */
  mate_volume_applet_get_dock_position (applet, &x, &y);
  gtk_window_move (GTK_WINDOW (applet->dock), x, y);

  /* Set the keyboard focus in the correct place. */
  mate_volume_applet_dock_set_focus (applet->dock);

  /* set menu item as active */
  gtk_widget_set_state (GTK_WIDGET (applet), GTK_STATE_SELECTED);

  /* keep state */
  applet->pop = TRUE;
}

static void
mate_volume_applet_popdown_dock (MateVolumeApplet *applet)
{
  GtkWidget *widget = GTK_WIDGET (applet);

  if (!applet->pop)
    return;

  /* hide */
  gtk_widget_hide_all (GTK_WIDGET (applet->dock));

  /* set menu item as active */
  gtk_widget_set_state (GTK_WIDGET (applet), GTK_STATE_NORMAL);

  /* keep state */
  applet->pop = FALSE;
}

static void
mate_volume_applet_pop_dock (MateVolumeApplet *applet)
{
  if (applet->pop) {
    mate_volume_applet_popdown_dock (applet);
  } else {
    mate_volume_applet_popup_dock (applet);
  }
}

static void
mate_volume_applet_update_mute_action (MateVolumeApplet *applet,
					gboolean           newmute)
{
  GtkAction *action;

  if (!applet->action_group)
    return;

  action = gtk_action_group_get_action (applet->action_group, "Mute");
  if (newmute == gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    return;

  gtk_action_block_activate (action);
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), newmute);
  gtk_action_unblock_activate (action);
}

gboolean
mixer_is_muted (MateVolumeApplet *applet)
{
  return applet->state & 1;
}

/*
 * Toggle mute.
 */

void
mate_volume_applet_toggle_mute (MateVolumeApplet *applet)
{
  gboolean mute = mixer_is_muted (applet);
  gboolean newmute = !mute;
  GList *tracks;

  for (tracks = g_list_first (applet->tracks); tracks; tracks = tracks->next)
    gst_mixer_set_mute (applet->mixer, tracks->data, !mute);

  if (mute) {
    /* sync back actual volume */
    cb_volume (applet->adjustment, applet);
  }

  /* update menu */
  mate_volume_applet_update_mute_action (applet, newmute);

  /* update graphic - this should happen automagically after the next
   * idle call, but apparently doesn't for some people... */
  mate_volume_applet_refresh (applet, TRUE, -1, newmute);
}

/*
 * Run g-v-c.
 */

void
mate_volume_applet_run_mixer (MateVolumeApplet *applet)
{
  GError *error = NULL;

  gdk_spawn_command_line_on_screen (gtk_widget_get_screen (GTK_WIDGET (applet)),
				    "mate-volume-control", &error);

  if (error) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				     GTK_BUTTONS_CLOSE,
				     _("Failed to start Volume Control: %s"),
				     error->message);
    g_signal_connect (dialog, "response",
		      G_CALLBACK (gtk_widget_destroy), NULL);
    gtk_widget_show (dialog);
    g_error_free (error);
  }
}

/*
 * Control events, change volume and so on.
 */

/* This is not static so we can inject external events
 * into the applet. Its to work around a GTK+ misfeature. See dock.c
 * for details. */

gboolean
mate_volume_applet_scroll (GtkWidget      *widget,
			    GdkEventScroll *event)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (widget);

  if (!applet->mixer) {
    show_no_mixer_dialog (applet);
    return TRUE;
  }

  if (event->type == GDK_SCROLL) {
    switch (event->direction) {
      case GDK_SCROLL_UP:
      case GDK_SCROLL_DOWN: {
        gdouble volume = gtk_adjustment_get_value (applet->adjustment);

        if (event->direction == GDK_SCROLL_UP) {
          volume += gtk_adjustment_get_step_increment (applet->adjustment);
          if (volume > gtk_adjustment_get_upper (applet->adjustment))
            volume = gtk_adjustment_get_upper (applet->adjustment);
        } else {
          volume -= gtk_adjustment_get_step_increment (applet->adjustment);
          if (volume < gtk_adjustment_get_lower (applet->adjustment))
            volume = gtk_adjustment_get_lower (applet->adjustment);
        }

        gtk_adjustment_set_value (applet->adjustment, volume);
        return TRUE;
      }
      default:
        break;
    }
  }

  if (GTK_WIDGET_CLASS (parent_class)->scroll_event)
    return GTK_WIDGET_CLASS (parent_class)->scroll_event (widget, event);
  else
    return FALSE;
}

static gboolean
mate_volume_applet_button (GtkWidget      *widget,
			    GdkEventButton *event)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (widget);

  if (event->window != gtk_widget_get_window (GTK_WIDGET (applet)) &&
      event->type == GDK_BUTTON_PRESS) {
    mate_volume_applet_popdown_dock (applet);
    return TRUE;
  } else if (event->window == gtk_widget_get_window (GTK_WIDGET (applet))) {
    switch (event->button) {
      case 1:
        switch (event->type) {
          case GDK_BUTTON_PRESS:
            if (!applet->mixer) {
              show_no_mixer_dialog (applet);
            } else {
              mate_volume_applet_pop_dock (applet);
            }
            return TRUE;
          case GDK_2BUTTON_PRESS:
            if (applet->mixer) {
              mate_volume_applet_popdown_dock (applet);
            }
            mate_volume_applet_toggle_mute (applet);
            return TRUE;
          default:
            break;
        }
        break;
      case 2: /* move */
      case 3: /* menu */
        if (applet->pop) {
          mate_volume_applet_popdown_dock (applet);
          return TRUE;
        }
        break;
      default:
        break;
    }
  }

  if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
    return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);

  return FALSE;
}

/* This is not static so we can inject external events
 * into the applet. Its to work around a GTK+ misfeature. See dock.c
 * for details. */

gboolean
mate_volume_applet_key (GtkWidget   *widget,
			 GdkEventKey *event)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (widget);

  if (!applet->mixer) {
    show_no_mixer_dialog (applet);
  } else switch (event->keyval) {
    case GDK_KP_Enter:
    case GDK_ISO_Enter:
    case GDK_3270_Enter:
    case GDK_Return:
    case GDK_space:
    case GDK_KP_Space:
      mate_volume_applet_pop_dock (applet);
      return TRUE;
    case GDK_m:
      if (event->state == GDK_CONTROL_MASK) {
        mate_volume_applet_toggle_mute (applet);
        return TRUE;
      }
      break;
    case GDK_o:
      if (event->state == GDK_CONTROL_MASK) {
        mate_volume_applet_run_mixer (applet);
        return TRUE;
      }
      break;
    case GDK_Escape:
      mate_volume_applet_popdown_dock (applet);
      return TRUE;
    case GDK_Page_Up:
    case GDK_Page_Down:
    case GDK_Left:
    case GDK_Right:
    case GDK_Up:
    case GDK_Down: {
      gdouble volume = gtk_adjustment_get_value (applet->adjustment);
      gdouble increment;

      if (event->state != 0)
        break;

      if (event->keyval == GDK_Up || event->keyval == GDK_Down
         ||event->keyval == GDK_Left)
        increment = gtk_adjustment_get_step_increment (applet->adjustment);
      else
        increment = gtk_adjustment_get_page_increment (applet->adjustment);

      if (event->keyval == GDK_Page_Up || event->keyval == GDK_Up
         ||event->keyval == GDK_Right) {
        volume += increment;
        if (volume > gtk_adjustment_get_upper (applet->adjustment))
          volume = gtk_adjustment_get_upper (applet->adjustment);
      } else {
        volume -= increment;
        if (volume < gtk_adjustment_get_lower (applet->adjustment))
          volume = gtk_adjustment_get_lower (applet->adjustment);
      }

      gtk_adjustment_set_value (applet->adjustment, volume);
      return TRUE;
    }
    default:
      break;
  }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

static gboolean
mate_volume_applet_dock_focus_out (GtkWidget *dock, GdkEventFocus *event,
				    MateVolumeApplet *applet)
{
  mate_volume_applet_popdown_dock (applet);

  return FALSE;
}

/*
 * Change orientation or size of panel.
 */

static void
mate_volume_applet_orientation	(MatePanelApplet *_applet,
				 MatePanelAppletOrient orientation)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (_applet);
  GtkWidget *dock;

  if (applet->dock) {
    gtk_widget_destroy (GTK_WIDGET (applet->dock));
  }
  dock = mate_volume_applet_dock_new (GTK_ORIENTATION_VERTICAL,
				       applet);
  g_object_ref_sink (dock); /* It isn't a child, but we do own it. */
  gtk_widget_add_events (dock, GDK_FOCUS_CHANGE_MASK);
  g_signal_connect (G_OBJECT (dock), "focus-out-event",
		    G_CALLBACK (mate_volume_applet_dock_focus_out),
		    applet);
  applet->dock = MATE_VOLUME_APPLET_DOCK (dock);
  mate_volume_applet_dock_change (applet->dock, applet->adjustment);

  if (MATE_PANEL_APPLET_CLASS (parent_class)->change_orient)
    MATE_PANEL_APPLET_CLASS (parent_class)->change_orient (_applet, orientation);
}

void mate_volume_applet_size_allocate (GtkWidget     *widget,
					GtkAllocation *allocation)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (widget);
  MatePanelAppletOrient orient;

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  orient = mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet));

  if (orient == MATE_PANEL_APPLET_ORIENT_UP || orient == MATE_PANEL_APPLET_ORIENT_DOWN) {
    if (applet->panel_size == allocation->height)
      return;
    applet->panel_size = allocation->height;
  }
  else {
    if (applet->panel_size == allocation->width)
      return;
    applet->panel_size = allocation->width;
  }

  init_pixbufs (applet);
  mate_volume_applet_refresh (applet, TRUE, -1, -1);
}

static void
mate_volume_applet_background (MatePanelApplet *_applet,
				MatePanelAppletBackgroundType type,
				GdkColor  *colour,
				GdkPixmap *pixmap)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (_applet);
  GtkRcStyle *rc_style;
  GtkStyle *style;

  /* reset style */
  gtk_widget_set_style (GTK_WIDGET (applet), NULL);
  rc_style = gtk_rc_style_new ();
  gtk_widget_modify_style (GTK_WIDGET (applet), rc_style);
  g_object_unref (rc_style);

  switch (type) {
    case PANEL_NO_BACKGROUND:
      break;
    case PANEL_COLOR_BACKGROUND:
      gtk_widget_modify_bg (GTK_WIDGET (applet),
			    GTK_STATE_NORMAL, colour);
      break;
    case PANEL_PIXMAP_BACKGROUND:
      style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (applet)));
      if (style->bg_pixmap[GTK_STATE_NORMAL])
        g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);
      style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
      gtk_widget_set_style (GTK_WIDGET (applet), style);
      g_object_unref (style);
      break;
  }
}

/*
 * This needs to be here because not all tracks have the same volume range,
 * so you can send this function the track and a new volume and it will be
 * scaled according to the volume range of the track in question.
 */

void
mate_volume_applet_adjust_volume (GstMixer *mixer,
				   GstMixerTrack *track,
				   gdouble volume)
{
  int range = track->max_volume - track->min_volume;
  gdouble scale = ((gdouble) range) / 100;
  int *volumes, n, volint;

  if (volume == 1.0) {
    volint = track->max_volume;
  } else if (volume == 0.0) {
    volint = track->min_volume;
  } else {
    volume *= scale;
    volume += track->min_volume;
    volint = lrint (volume);
  }

  volumes = g_new (gint, track->num_channels);
  for (n = 0; n < track->num_channels; n++)
    volumes[n] = volint;
  gst_mixer_set_volume (mixer, track, volumes);

  g_free (volumes);
}

static gdouble
mate_volume_applet_get_volume (GstMixer *mixer,
				GstMixerTrack *track)
{
  int *volumes, n;
  gdouble j;

  if (!track || !mixer)
    return -1;

  volumes = g_new (gint, track->num_channels);
  gst_mixer_get_volume (mixer, track, volumes);

  j = 0;
  for (n = 0; n < track->num_channels; n++)
    j += volumes[n];
  g_free (volumes);
  j /= track->num_channels;

  return 100 * (j - track->min_volume) / (track->max_volume - track->min_volume);
}

/*
 * Volume changed.
 */

static void
cb_volume (GtkAdjustment *adj,
	   gpointer data)
{
  MateVolumeApplet *applet = data;
  gdouble v;
  GList *iter;

  if (applet->lock)
    return;
  applet->lock = TRUE;

  v = gtk_adjustment_get_value (adj);

  for (iter = g_list_first (applet->tracks); iter; iter = iter->next) {
    GstMixerTrack *track = iter->data;
    mate_volume_applet_adjust_volume (applet->mixer, track, v);
  }

  applet->lock = FALSE;
  applet->force_next_update = TRUE;
  mate_volume_applet_refresh (MATE_VOLUME_APPLET (data), FALSE, v, -1);
}

/*
 * Automatic timer. Check for changes.
 */

#define STATE(vol,m) (((gint) vol << 1) | (m ? 1 : 0))

static gboolean
mate_volume_applet_refresh (MateVolumeApplet *applet,
			     gboolean           force_refresh,
			     gdouble            volume,
			     gint               mute)
{
  GdkPixbuf *pixbuf;
  gint n;
  gboolean show_mute, did_change;
  gchar *tooltip_str;
  GstMixerTrack *first_track;
  GString *track_names;
  GList *iter;

  show_mute = 0;

  if (!applet->mixer) {
    n = 0;
    show_mute = 0;
    mute = 0;
  } else if (!applet->tracks) {
    return FALSE;
  } else {
    first_track = g_list_first (applet->tracks)->data;
    if (volume == -1) {
      /* only first track */
      volume = mate_volume_applet_get_volume (applet->mixer, first_track);
    }
    if (mute == -1) {
      mute = GST_MIXER_TRACK_HAS_FLAG (first_track,
				       GST_MIXER_TRACK_MUTE) ? 1 : 0;
    }
    if (volume <= 0 || mute) {
	show_mute = 1;
	n = 0;
    }
    else {
      /* select image */
      n = 3 * volume / 100 + 1;
      if (n < 1)
	n = 1;
      if (n > 3)
	n = 3;
    }
  }

  did_change = (force_refresh || (STATE (volume, mute) != applet->state) ||
      applet->force_next_update);
  applet->force_next_update = FALSE;

  if (did_change) {
    if (show_mute) {
      pixbuf = applet->pix[0];
    } else {
      pixbuf = applet->pix[n];
    }

    gtk_image_set_from_pixbuf (applet->image, pixbuf);
    applet->state = STATE (volume, mute);

    if (applet->dock) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (applet->dock->mute),
				    mute);
    }
  }

  if (!did_change || !applet->mixer)
    return did_change;

  /* build names of selecter tracks */
  track_names = g_string_new ("");
  for (iter = g_list_first (applet->tracks); iter; iter = iter->next) {
    GstMixerTrack *track = iter->data;

    if (iter->next != NULL)
      g_string_append_printf (track_names, "%s / ", track->label);
    else
      track_names = g_string_append (track_names, track->label);
  }

  if (show_mute) {
    tooltip_str = g_strdup_printf (_("%s: muted"), track_names->str);
  } else {
    /* Translator comment: I'm not all too sure if this makes sense
     * to mark as a translation, but anyway. The string is a list of
     * selected tracks, the number is the volume in percent. You
     * most likely want to keep this as-is. */
    tooltip_str = g_strdup_printf (_("%s: %d%%"), track_names->str,
        (int) volume);
  }
  g_string_free (track_names, TRUE);

  gtk_widget_set_tooltip_text (GTK_WIDGET (applet), tooltip_str);
  g_free (tooltip_str);

  applet->lock = TRUE;
  if (volume != 0) {
    gtk_adjustment_set_value (applet->adjustment, volume);
  }
  applet->lock = FALSE;

  /* update mute status */
  mate_volume_applet_update_mute_action (applet, mute);

  return did_change;
}

static void
cb_notify_message (GstBus *bus, GstMessage *message, gpointer data)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (data);
  GstMixerMessageType type;
  GstMixerTrack *first_track;
  GstMixerTrack *track = NULL;
  gint mute;
  gdouble volume;

  if (applet->tracks == NULL ||
      GST_MESSAGE_SRC (message) != GST_OBJECT (applet->mixer)) {
    /* No tracks, or not from our mixer - can't update anything anyway */
    return;
  }

  volume = mute = -1;

  first_track = g_list_first (applet->tracks)->data;

  /* This code only calls refresh if the first_track changes, because the
   * refresh code only retrieves the current value from that track anyway */
  type = gst_mixer_message_get_type (message);
  if (type == GST_MIXER_MESSAGE_MUTE_TOGGLED) {
    gboolean muted;
    gst_mixer_message_parse_mute_toggled (message, &track, &muted);
    mute = muted ? 1 : 0;
  }
  else if (type == GST_MIXER_MESSAGE_VOLUME_CHANGED) {
    gint n, num_channels, *vols;
    volume = 0.0;

    gst_mixer_message_parse_volume_changed (message, &track, &vols, &num_channels);
    for (n = 0; n < num_channels; n++)
      volume += vols[n];
    volume /= track->num_channels;
    volume = 100 * volume / (track->max_volume - track->min_volume);
  } else
  {
    return;
  }

  if (first_track == track)
    mate_volume_applet_refresh (MATE_VOLUME_APPLET (data), FALSE, volume, mute);
}

static gboolean
cb_check (gpointer data)
{
  static int      time_counter  = -1;
  static int      timeout       = 15;
  static gboolean recent_change = FALSE;
  gboolean        did_change;

  time_counter++;

  /*
   * This timeout is called 10 times per second.  Only do the update every
   * 15 times this function is called (every 1.5 seconds), unless the value
   * actually changed.
   */
  if (time_counter % timeout == 0 || recent_change) {
     did_change = mate_volume_applet_refresh (MATE_VOLUME_APPLET (data),
                                               FALSE, -1, -1);

     /*
      * If a change was done, set recent_change so that the update is
      * done 10 times a second for 10 seconds and reset the counter to 0.
      * This way we update frequently for 10 seconds after the last time
      * the value is actually changed.
      */
     if (did_change) {
        recent_change = TRUE;
        time_counter = 0;
        timeout      = 100;
     } else if (time_counter % timeout == 0) {
        /*
         * When the counter gets to the timeout, reset recent_change and
         * time_counter so we go back to the behavior where we only check
         * every 15 times the function is called.
         */
        recent_change = FALSE;
        time_counter  = 0;
        timeout       = 15;
     }
  }

  return TRUE;
}

/*
 * MateConf callback.
 */

static void
cb_mateconf (MateConfClient *client,
	  guint        connection_id,
	  MateConfEntry  *entry,
	  gpointer     data)
{
  MateVolumeApplet *applet = data;
  MateConfValue *value;
  const gchar *str, *key;
  const GList *item;
  gboolean newdevice = FALSE;
  gchar *keyroot;
  GList *active_tracks;

  active_tracks = NULL;

  keyroot = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (applet), "");
  key = mateconf_entry_get_key (entry);
  if (strncmp (key, keyroot, strlen (keyroot))) {
    g_free (keyroot);
    return;
  }
  key += strlen (keyroot);
  g_free (keyroot);

  g_list_free(applet->elements);
  applet->elements = mate_volume_applet_create_mixer_collection ();

  if ((value = mateconf_entry_get_value (entry)) != NULL &&
      (value->type == MATECONF_VALUE_STRING) &&
      (str = mateconf_value_get_string (value)) != NULL) {
    if (!strcmp (key, MATE_VOLUME_APPLET_KEY_ACTIVE_ELEMENT)) {
      for (item = applet->elements; item != NULL; item = item->next) {
        gchar *cur_el_str = g_object_get_data (item->data,
					       "mate-volume-applet-name");

        if (!strcmp (cur_el_str, str)) {
          GstElement *old_element = GST_ELEMENT (applet->mixer),
		     *new_element = item->data;

          if (new_element != old_element) {
            /* change element */
            gst_element_set_state (item->data, GST_STATE_READY);
            if (gst_element_get_state (item->data, NULL, NULL, -1) != GST_STATE_CHANGE_SUCCESS)
              continue;

            /* save */
            gst_object_replace ((GstObject **) &applet->mixer, item->data);
            gst_element_set_state (old_element, GST_STATE_NULL);
	    mate_volume_applet_setup_timeout (applet);
            newdevice = TRUE;
          }
          break;
        }
      }
    }

    if (!strcmp (key, MATE_VOLUME_APPLET_KEY_ACTIVE_TRACK) || newdevice) {
      if (!active_tracks) {
        active_tracks = select_tracks (GST_ELEMENT (applet->mixer), str, FALSE);
      }

      if (active_tracks) {
	GstMixerTrack *first_track;

	/* copy the newly created track list over to the main list */
	g_list_free (applet->tracks);
	applet->tracks = g_list_copy (active_tracks);

	first_track = g_list_first (active_tracks)->data;

        /* dock */
	gtk_adjustment_set_value (applet->adjustment,
				  mate_volume_applet_get_volume (applet->mixer,
								  first_track));

        /* if preferences window is open, update */
	if (applet->prefs) {
          mate_volume_applet_preferences_change (
	      MATE_VOLUME_APPLET_PREFERENCES (applet->prefs),
	      applet->mixer, applet->tracks);
	}

        applet->force_next_update = TRUE;
      }
    }
  }
}

/*
 * verb callback.
 */

static void
cb_prefs_destroy (GtkWidget *widget,
		  gpointer   data)
{
  MATE_VOLUME_APPLET (data)->prefs = NULL;
}

static void
cb_verb (GtkAction   *action,
	 gpointer     data)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (data);
  const gchar       *verbname = gtk_action_get_name (action);

  if (!strcmp (verbname, "RunMixer")) {
    mate_volume_applet_run_mixer (applet);
  } else if (!strcmp (verbname, "Help")) {
    GError *error = NULL;

    gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (applet)),
		  "ghelp:mixer_applet2",
		  gtk_get_current_event_time (),
		  &error);

    if (error) {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				       GTK_BUTTONS_CLOSE,
				       _("Failed to display help: %s"),
				       error->message);
      g_signal_connect (dialog, "response",
			G_CALLBACK (gtk_widget_destroy), NULL);
      gtk_widget_show (dialog);
      g_error_free (error);
    }
  } else if (!strcmp (verbname, "About")) {

    const gchar *authors[] = { "Ronald Bultje <rbultje@ronald.bitfreak.net>",
			     NULL };

    char *comments = g_strdup_printf ("%s\n\n%s",
		    _("Volume control for your MATE Panel."),
		    _("Using GStreamer 0.10.")
		    );

    gtk_show_about_dialog (NULL,
		"version",	VERSION,
		"copyright",	"\xC2\xA9 2004 Ronald Bultje",
		"comments",	comments,
		"authors",	authors,
		"translator-credits",	_("translator-credits"),
		"logo-icon-name",	"multimedia-volume-control",
		NULL);

    g_free (comments);

  } else if (!strcmp (verbname, "Pref")) {
    if (!applet->mixer) {
      show_no_mixer_dialog (applet);
    } else {
      if (applet->prefs)
        return;

      g_list_free(applet->elements);
      applet->elements = mate_volume_applet_create_mixer_collection ();

      applet->prefs = mate_volume_applet_preferences_new (MATE_PANEL_APPLET (applet),
							   applet->elements,
							   applet->mixer,
							   applet->tracks);
      g_signal_connect (applet->prefs, "destroy",
		        G_CALLBACK (cb_prefs_destroy), applet);
      gtk_widget_show (applet->prefs);
    }
  } else if (!strcmp (verbname, "Mute")) {
    if (!applet->mixer) {
      show_no_mixer_dialog (applet);
    } else {
      gboolean mute = applet->state & 1,
	       want_mute = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
      if (mute != want_mute)
	mate_volume_applet_toggle_mute (applet);
    }
  } else {
    g_warning ("Unknown menu action '%s'", verbname);
  }
}

static void
cb_theme_change (GtkIconTheme *icon_theme,
		 gpointer data)
{
  MateVolumeApplet *applet = MATE_VOLUME_APPLET (data);

  init_pixbufs (applet);
  mate_volume_applet_refresh (applet, TRUE, -1, -1);
}

/*
 * Block the tooltips event-after handler on scroll events.
 */

static void
cb_stop_scroll_events (GtkWidget *widget,
		       GdkEvent  *event)
{
  if (event->type == GDK_SCROLL)
    g_signal_stop_emission_by_name (widget, "event-after");
}
