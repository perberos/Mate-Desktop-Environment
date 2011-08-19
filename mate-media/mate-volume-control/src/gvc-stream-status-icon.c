/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION (2, 21, 8)
#include <gdk/gdkkeysyms-compat.h>
#else
#include <gdk/gdkkeysyms.h>
#endif

#include "gvc-mixer-stream.h"
#include "gvc-channel-bar.h"
#include "gvc-stream-status-icon.h"

#define GVC_STREAM_STATUS_ICON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_STREAM_STATUS_ICON, GvcStreamStatusIconPrivate))

struct GvcStreamStatusIconPrivate
{
        char          **icon_names;
        GvcMixerStream *mixer_stream;
        GtkWidget      *dock;
        GtkWidget      *bar;
        guint           current_icon;
        char           *display_name;
        gboolean        thaw;
};

enum
{
        PROP_0,
        PROP_DISPLAY_NAME,
        PROP_MIXER_STREAM,
        PROP_ICON_NAMES,
};

static void     gvc_stream_status_icon_class_init (GvcStreamStatusIconClass *klass);
static void     gvc_stream_status_icon_init       (GvcStreamStatusIcon      *stream_status_icon);
static void     gvc_stream_status_icon_finalize   (GObject                  *object);

G_DEFINE_TYPE (GvcStreamStatusIcon, gvc_stream_status_icon, GTK_TYPE_STATUS_ICON)

static void
on_adjustment_value_changed (GtkAdjustment *adjustment,
                             GvcStreamStatusIcon     *icon)
{
        gdouble volume;

        if (icon->priv->thaw)
                return;

        volume = gtk_adjustment_get_value (adjustment);

        /* Only push the volume if it's actually changed */
        if (gvc_mixer_stream_set_volume(icon->priv->mixer_stream,
                                    (pa_volume_t) round (volume)) != FALSE) {
                gvc_mixer_stream_push_volume(icon->priv->mixer_stream);
        }
}

static void
update_dock (GvcStreamStatusIcon *icon)
{
        GtkAdjustment *adj;
        gboolean       is_muted;

        g_return_if_fail (icon);

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (icon->priv->bar)));

        icon->priv->thaw = TRUE;
        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (icon->priv->mixer_stream));
        is_muted = gvc_mixer_stream_get_is_muted (icon->priv->mixer_stream);
        gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (icon->priv->bar), is_muted);
        icon->priv->thaw = FALSE;
}

static gboolean
popup_dock (GvcStreamStatusIcon *icon,
            guint                time)
{
        GdkRectangle   area;
        GtkOrientation orientation;
        GdkDisplay    *display;
        GdkScreen     *screen;
        gboolean       res;
        int            x;
        int            y;
        int            monitor_num;
        GdkRectangle   monitor;
        GtkRequisition dock_req;

        update_dock (icon);

        screen = gtk_status_icon_get_screen (GTK_STATUS_ICON (icon));
        res = gtk_status_icon_get_geometry (GTK_STATUS_ICON (icon),
                                            &screen,
                                            &area,
                                            &orientation);
        if (! res) {
                g_warning ("Unable to determine geometry of status icon");
                return FALSE;
        }

        /* position roughly */
        gtk_window_set_screen (GTK_WINDOW (icon->priv->dock), screen);
        gvc_channel_bar_set_orientation (GVC_CHANNEL_BAR (icon->priv->bar),
                                         1 - orientation);

        monitor_num = gdk_screen_get_monitor_at_point (screen, area.x, area.y);
        gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

        gtk_container_foreach (GTK_CONTAINER (icon->priv->dock), 
                               (GtkCallback) gtk_widget_show_all, NULL);
        gtk_widget_size_request (icon->priv->dock, &dock_req);

        if (orientation == GTK_ORIENTATION_VERTICAL) {
                if (area.x + area.width + dock_req.width <= monitor.x + monitor.width) {
                        x = area.x + area.width;
                } else {
                        x = area.x - dock_req.width;
                }
                if (area.y + dock_req.height <= monitor.y + monitor.height) {
                        y = area.y;
                } else {
                        y = monitor.y + monitor.height - dock_req.height;
                }
        } else {
                if (area.y + area.height + dock_req.height <= monitor.y + monitor.height) {
                        y = area.y + area.height;
                } else {
                        y = area.y - dock_req.height;
                }
                if (area.x + dock_req.width <= monitor.x + monitor.width) {
                        x = area.x;
                } else {
                        x = monitor.x + monitor.width - dock_req.width;
                }
        }

        gtk_window_move (GTK_WINDOW (icon->priv->dock), x, y);

        /* FIXME: without this, the popup window appears as a square
         * after changing the orientation
         */
        gtk_window_resize (GTK_WINDOW (icon->priv->dock), 1, 1);

        gtk_widget_show_all (icon->priv->dock);


        /* grab focus */
        gtk_grab_add (icon->priv->dock);

        if (gdk_pointer_grab (gtk_widget_get_window (icon->priv->dock), TRUE,
                              GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                              GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK, NULL, NULL,
                              time)
            != GDK_GRAB_SUCCESS) {
                gtk_grab_remove (icon->priv->dock);
                gtk_widget_hide (icon->priv->dock);
                return FALSE;
        }

        if (gdk_keyboard_grab (gtk_widget_get_window (icon->priv->dock), TRUE, time) != GDK_GRAB_SUCCESS) {
                display = gtk_widget_get_display (icon->priv->dock);
                gdk_display_pointer_ungrab (display, time);
                gtk_grab_remove (icon->priv->dock);
                gtk_widget_hide (icon->priv->dock);
                return FALSE;
        }

        gtk_widget_grab_focus (icon->priv->dock);

        return TRUE;
}

static void
on_status_icon_activate (GtkStatusIcon       *status_icon,
                         GvcStreamStatusIcon *icon)
{
        popup_dock (icon, GDK_CURRENT_TIME);
}

static void
on_menu_mute_toggled (GtkMenuItem         *item,
                      GvcStreamStatusIcon *icon)
{
        gboolean is_muted;
        is_muted = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));
        gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (icon->priv->bar), is_muted);
}

static void
on_menu_activate_open_volume_control (GtkMenuItem *item,
                                      GvcStreamStatusIcon   *icon)
{
        GError *error;

        error = NULL;
        gdk_spawn_command_line_on_screen (gtk_widget_get_screen (icon->priv->dock),
                                          "mate-volume-control",
                                          &error);

        if (error != NULL) {
                GtkWidget *dialog;

                dialog = gtk_message_dialog_new (NULL,
                                                 0,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_CLOSE,
                                                 _("Failed to start Sound Preferences: %s"),
                                                 error->message);
                g_signal_connect (dialog,
                                  "response",
                                  G_CALLBACK (gtk_widget_destroy),
                                  NULL);
                gtk_widget_show (dialog);
                g_error_free (error);
        }
}

static void
on_status_icon_popup_menu (GtkStatusIcon       *status_icon,
                           guint                button,
                           guint                activate_time,
                           GvcStreamStatusIcon *icon)
{
        GtkWidget *menu;
        GtkWidget *item;
        GtkWidget *image;

        menu = gtk_menu_new ();

        item = gtk_check_menu_item_new_with_mnemonic (_("_Mute"));
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                        gvc_mixer_stream_get_is_muted (icon->priv->mixer_stream));
        g_signal_connect (item,
                          "toggled",
                          G_CALLBACK (on_menu_mute_toggled),
                          icon);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        item = gtk_image_menu_item_new_with_mnemonic (_("_Sound Preferences"));
        image = gtk_image_new_from_icon_name ("multimedia-volume-control",
                                              GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
        g_signal_connect (item,
                          "activate",
                          G_CALLBACK (on_menu_activate_open_volume_control),
                          icon);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        gtk_widget_show_all (menu);
        gtk_menu_popup (GTK_MENU (menu),
                        NULL,
                        NULL,
                        gtk_status_icon_position_menu,
                        status_icon,
                        button,
                        activate_time);
}

static gboolean
on_status_icon_scroll_event (GtkStatusIcon       *status_icon,
                             GdkEventScroll      *event,
                             GvcStreamStatusIcon *icon)
{
        return gvc_channel_bar_scroll (GVC_CHANNEL_BAR (icon->priv->bar), event->direction);
}

static void
gvc_icon_release_grab (GvcStreamStatusIcon *icon,
                         GdkEventButton    *event)
{
        GdkDisplay     *display;

        /* ungrab focus */
        display = gtk_widget_get_display (GTK_WIDGET (icon->priv->dock));
        gdk_display_keyboard_ungrab (display, event->time);
        gdk_display_pointer_ungrab (display, event->time);
        gtk_grab_remove (icon->priv->dock);

        /* hide again */
        gtk_widget_hide (icon->priv->dock);
}

static gboolean
on_dock_button_press (GtkWidget      *widget,
                      GdkEventButton *event,
                      GvcStreamStatusIcon      *icon)
{
        if (event->type == GDK_BUTTON_PRESS) {
                gvc_icon_release_grab (icon, event);
                return TRUE;
        }

        return FALSE;
}

static void
popdown_dock (GvcStreamStatusIcon *icon)
{
        GdkDisplay *display;

        /* ungrab focus */
        display = gtk_widget_get_display (icon->priv->dock);
        gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
        gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
        gtk_grab_remove (icon->priv->dock);

        /* hide again */
        gtk_widget_hide (icon->priv->dock);
}

/* This is called when the grab is broken for
 * either the dock, or the scale itself */
static void
gvc_icon_grab_notify (GvcStreamStatusIcon *icon,
                      gboolean             was_grabbed)
{
        if (was_grabbed != FALSE) {
                return;
        }

        if (!gtk_widget_has_grab (icon->priv->dock)) {
                return;
        }

        if (gtk_widget_is_ancestor (gtk_grab_get_current (), icon->priv->dock)) {
                return;
        }

        popdown_dock (icon);
}

static void
on_dock_grab_notify (GtkWidget           *widget,
                     gboolean             was_grabbed,
                     GvcStreamStatusIcon *icon)
{
        gvc_icon_grab_notify (icon, was_grabbed);
}

static gboolean
on_dock_grab_broken_event (GtkWidget           *widget,
                           gboolean             was_grabbed,
                           GvcStreamStatusIcon *icon)
{
        gvc_icon_grab_notify (icon, FALSE);

        return FALSE;
}

static gboolean
on_dock_key_release (GtkWidget           *widget,
                     GdkEventKey         *event,
                     GvcStreamStatusIcon *icon)
{
        if (event->keyval == GDK_Escape) {
                popdown_dock (icon);
                return TRUE;
        }

#if 0
        if (!gtk_bindings_activate_event (GTK_OBJECT (widget), event)) {
                /* The popup hasn't managed the event, pass onto the button */
                gtk_bindings_activate_event (GTK_OBJECT (user_data), event);
        }
#endif
        return TRUE;
}

static gboolean
on_dock_scroll_event (GtkWidget           *widget,
                      GdkEventScroll      *event,
                      GvcStreamStatusIcon *icon)
{
        /* Forward event to the status icon */
        on_status_icon_scroll_event (NULL, event, icon);
        return TRUE;
}

static void
update_icon (GvcStreamStatusIcon *icon)
{
        guint    volume;
        gboolean is_muted;
        guint    n;
        char    *markup;
        gboolean can_decibel;
        gdouble  db;

        if (icon->priv->mixer_stream == NULL) {
                return;
        }

        volume = gvc_mixer_stream_get_volume (icon->priv->mixer_stream);
        is_muted = gvc_mixer_stream_get_is_muted (icon->priv->mixer_stream);
        db = gvc_mixer_stream_get_decibel (icon->priv->mixer_stream);
        can_decibel = gvc_mixer_stream_get_can_decibel (icon->priv->mixer_stream);

        /* select image */
        if (volume <= 0 || is_muted) {
                n = 0;
        } else {
                n = 3 * volume / PA_VOLUME_NORM + 1;
                if (n < 1) {
                        n = 1;
                } else if (n > 3) {
                        n = 3;
                }
        }

        /* apparently status icon will reset icon even if
         * if doesn't change */
        if (icon->priv->current_icon != n) {
                gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (icon),
                                                    icon->priv->icon_names [n]);
                icon->priv->current_icon = n;
        }


        if (is_muted) {
                markup = g_strdup_printf (
                                          "<b>%s: %s</b>\n<small>%s</small>",
                                          icon->priv->display_name,
                                          _("Muted"),
                                          gvc_mixer_stream_get_description (icon->priv->mixer_stream));
        } else if (can_decibel && (db > PA_DECIBEL_MININFTY)) {
                markup = g_strdup_printf (
                                          "<b>%s: %.0f%%</b>\n<small>%0.2f dB\n%s</small>",
                                          icon->priv->display_name,
                                          100 * (float)volume / PA_VOLUME_NORM,
                                          db,
                                          gvc_mixer_stream_get_description (icon->priv->mixer_stream));
        } else if (can_decibel) {
                markup = g_strdup_printf (
                                          "<b>%s: %.0f%%</b>\n<small>-&#8734; dB\n%s</small>",
                                          icon->priv->display_name,
                                          100 * (float)volume / PA_VOLUME_NORM,
                                          gvc_mixer_stream_get_description (icon->priv->mixer_stream));
        } else {
                markup = g_strdup_printf (
                                          "<b>%s: %.0f%%</b>\n<small>%s</small>",
                                          icon->priv->display_name,
                                          100 * (float)volume / PA_VOLUME_NORM,
                                          gvc_mixer_stream_get_description (icon->priv->mixer_stream));
        }
        gtk_status_icon_set_tooltip_markup (GTK_STATUS_ICON (icon), markup);
        g_free (markup);
}

void
gvc_stream_status_icon_set_icon_names (GvcStreamStatusIcon  *icon,
                                       const char          **names)
{
        g_return_if_fail (GVC_IS_STREAM_STATUS_ICON (icon));

        g_strfreev (icon->priv->icon_names);
        icon->priv->icon_names = g_strdupv ((char **)names);
        update_icon (icon);
        g_object_notify (G_OBJECT (icon), "icon-names");
}

static void
on_stream_volume_notify (GObject             *object,
                         GParamSpec          *pspec,
                         GvcStreamStatusIcon *icon)
{
        update_icon (icon);
        update_dock (icon);
}

static void
on_stream_is_muted_notify (GObject             *object,
                           GParamSpec          *pspec,
                           GvcStreamStatusIcon *icon)
{
        update_icon (icon);
        update_dock (icon);
}

void
gvc_stream_status_icon_set_display_name (GvcStreamStatusIcon *icon,
                                         const char          *name)
{
        g_return_if_fail (GVC_STREAM_STATUS_ICON (icon));

        g_free (icon->priv->display_name);
        icon->priv->display_name = g_strdup (name);
        update_icon (icon);
        g_object_notify (G_OBJECT (icon), "display-name");
}

void
gvc_stream_status_icon_set_mixer_stream (GvcStreamStatusIcon *icon,
                                         GvcMixerStream      *stream)
{
        g_return_if_fail (GVC_STREAM_STATUS_ICON (icon));

        if (stream != NULL) {
                g_object_ref (stream);
        }

        if (icon->priv->mixer_stream != NULL) {
                g_signal_handlers_disconnect_by_func (icon->priv->mixer_stream,
                                                      G_CALLBACK (on_stream_volume_notify),
                                                      icon);
                g_signal_handlers_disconnect_by_func (icon->priv->mixer_stream,
                                                      G_CALLBACK (on_stream_is_muted_notify),
                                                      icon);
                g_object_unref (icon->priv->mixer_stream);
                icon->priv->mixer_stream = NULL;
        }

        icon->priv->mixer_stream = stream;

        if (icon->priv->mixer_stream != NULL) {
                GtkAdjustment *adj;

                g_object_ref (icon->priv->mixer_stream);

                icon->priv->thaw = TRUE;
                adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (icon->priv->bar)));
                gtk_adjustment_set_value (adj,
                                          gvc_mixer_stream_get_volume (icon->priv->mixer_stream));
                icon->priv->thaw = FALSE;

                g_signal_connect (icon->priv->mixer_stream,
                                  "notify::volume",
                                  G_CALLBACK (on_stream_volume_notify),
                                  icon);
                g_signal_connect (icon->priv->mixer_stream,
                                  "notify::is-muted",
                                  G_CALLBACK (on_stream_is_muted_notify),
                                  icon);
        }

        update_icon (icon);

        g_object_notify (G_OBJECT (icon), "mixer-stream");
}

static void
gvc_stream_status_icon_set_property (GObject       *object,
                                     guint          prop_id,
                                     const GValue  *value,
                                     GParamSpec    *pspec)
{
        GvcStreamStatusIcon *self = GVC_STREAM_STATUS_ICON (object);

        switch (prop_id) {
        case PROP_MIXER_STREAM:
                gvc_stream_status_icon_set_mixer_stream (self, g_value_get_object (value));
                break;
        case PROP_DISPLAY_NAME:
                gvc_stream_status_icon_set_display_name (self, g_value_get_string (value));
                break;
        case PROP_ICON_NAMES:
                gvc_stream_status_icon_set_icon_names (self, g_value_get_boxed (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_stream_status_icon_get_property (GObject     *object,
                                     guint        prop_id,
                                     GValue      *value,
                                     GParamSpec  *pspec)
{
        GvcStreamStatusIcon *self = GVC_STREAM_STATUS_ICON (object);
        GvcStreamStatusIconPrivate *priv = self->priv;

        switch (prop_id) {
        case PROP_MIXER_STREAM:
                g_value_set_object (value, priv->mixer_stream);
                break;
        case PROP_DISPLAY_NAME:
                g_value_set_string (value, priv->display_name);
                break;
        case PROP_ICON_NAMES:
                g_value_set_boxed (value, priv->icon_names);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
on_bar_is_muted_notify (GObject             *object,
                        GParamSpec          *pspec,
                        GvcStreamStatusIcon *icon)
{
        gboolean is_muted;

        is_muted = gvc_channel_bar_get_is_muted (GVC_CHANNEL_BAR (object));

        if (gvc_mixer_stream_get_is_muted (icon->priv->mixer_stream) != is_muted) {
                /* Update the stream before pushing the change */
                gvc_mixer_stream_set_is_muted (icon->priv->mixer_stream, is_muted);
                gvc_mixer_stream_change_is_muted (icon->priv->mixer_stream,
                                                  is_muted);
        }
}

static GObject *
gvc_stream_status_icon_constructor (GType                  type,
                                    guint                  n_construct_properties,
                                    GObjectConstructParam *construct_params)
{
        GObject             *object;
        GvcStreamStatusIcon *icon;
        GtkWidget           *frame;
        GtkWidget           *box;
        GtkAdjustment       *adj;

        object = G_OBJECT_CLASS (gvc_stream_status_icon_parent_class)->constructor (type, n_construct_properties, construct_params);

        icon = GVC_STREAM_STATUS_ICON (object);

        gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (icon),
                                            icon->priv->icon_names[0]);

        /* window */
        icon->priv->dock = gtk_window_new (GTK_WINDOW_POPUP);
        gtk_widget_set_name (icon->priv->dock, "gvc-stream-status-icon-popup-window");
        g_signal_connect (icon->priv->dock,
                          "button-press-event",
                          G_CALLBACK (on_dock_button_press),
                          icon);
        g_signal_connect (icon->priv->dock,
                          "key-release-event",
                          G_CALLBACK (on_dock_key_release),
                          icon);
        g_signal_connect (icon->priv->dock,
                          "scroll-event",
                          G_CALLBACK (on_dock_scroll_event),
                          icon);
        g_signal_connect (icon->priv->dock,
                          "grab-notify",
                          G_CALLBACK (on_dock_grab_notify),
                          icon);
        g_signal_connect (icon->priv->dock,
                          "grab-broken-event",
                          G_CALLBACK (on_dock_grab_broken_event),
                          icon);

        gtk_window_set_decorated (GTK_WINDOW (icon->priv->dock), FALSE);

        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
        gtk_container_add (GTK_CONTAINER (icon->priv->dock), frame);

        box = gtk_vbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (box), 2);
        gtk_container_add (GTK_CONTAINER (frame), box);

        icon->priv->bar = gvc_channel_bar_new ();
        gvc_channel_bar_set_orientation (GVC_CHANNEL_BAR (icon->priv->bar),
                                         GTK_ORIENTATION_VERTICAL);

        gtk_box_pack_start (GTK_BOX (box), icon->priv->bar, TRUE, FALSE, 0);
        g_signal_connect (icon->priv->bar,
                          "notify::is-muted",
                          G_CALLBACK (on_bar_is_muted_notify),
                          icon);

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (icon->priv->bar)));
        g_signal_connect (adj,
                          "value-changed",
                          G_CALLBACK (on_adjustment_value_changed),
                          icon);

        return object;
}

static void
gvc_stream_status_icon_dispose (GObject *object)
{
        GvcStreamStatusIcon *icon = GVC_STREAM_STATUS_ICON (object);

        if (icon->priv->dock != NULL) {
                gtk_widget_destroy (icon->priv->dock);
                icon->priv->dock = NULL;
        }

        if (icon->priv->mixer_stream != NULL) {
                g_object_unref (icon->priv->mixer_stream);
                icon->priv->mixer_stream = NULL;
        }

        G_OBJECT_CLASS (gvc_stream_status_icon_parent_class)->dispose (object);
}

static void
gvc_stream_status_icon_class_init (GvcStreamStatusIconClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_stream_status_icon_constructor;
        object_class->finalize = gvc_stream_status_icon_finalize;
        object_class->dispose = gvc_stream_status_icon_dispose;
        object_class->set_property = gvc_stream_status_icon_set_property;
        object_class->get_property = gvc_stream_status_icon_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MIXER_STREAM,
                                         g_param_spec_object ("mixer-stream",
                                                              "mixer stream",
                                                              "mixer stream",
                                                              GVC_TYPE_MIXER_STREAM,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_NAME,
                                         g_param_spec_string ("display-name",
                                                              "Display Name",
                                                              "Name to display for this stream",
                                                              NULL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_ICON_NAMES,
                                         g_param_spec_boxed ("icon-names",
                                                             "Icon Names",
                                                             "Name of icon to display for this stream",
                                                              G_TYPE_STRV,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GvcStreamStatusIconPrivate));
}

static void
on_status_icon_visible_notify (GvcStreamStatusIcon *icon)
{
        gboolean visible;

        g_object_get (icon, "visible", &visible, NULL);
        if (! visible) {
                if (icon->priv->dock != NULL) {
                        gtk_widget_hide (icon->priv->dock);
                }
        }
}

static void
gvc_stream_status_icon_init (GvcStreamStatusIcon *icon)
{
        icon->priv = GVC_STREAM_STATUS_ICON_GET_PRIVATE (icon);

        g_signal_connect (icon,
                          "activate",
                          G_CALLBACK (on_status_icon_activate),
                          icon);
        g_signal_connect (icon,
                          "popup-menu",
                          G_CALLBACK (on_status_icon_popup_menu),
                          icon);
        g_signal_connect (icon,
                          "scroll-event",
                          G_CALLBACK (on_status_icon_scroll_event),
                          icon);
        g_signal_connect (icon,
                          "notify::visible",
                          G_CALLBACK (on_status_icon_visible_notify),
                          NULL);

        icon->priv->thaw = FALSE;
}

static void
gvc_stream_status_icon_finalize (GObject *object)
{
        GvcStreamStatusIcon *stream_status_icon;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_STREAM_STATUS_ICON (object));

        stream_status_icon = GVC_STREAM_STATUS_ICON (object);

        g_return_if_fail (stream_status_icon->priv != NULL);

        g_strfreev (stream_status_icon->priv->icon_names);

        G_OBJECT_CLASS (gvc_stream_status_icon_parent_class)->finalize (object);
}

GvcStreamStatusIcon *
gvc_stream_status_icon_new (GvcMixerStream *stream,
                            const char    **icon_names)
{
        GObject *icon;
        icon = g_object_new (GVC_TYPE_STREAM_STATUS_ICON,
                             "mixer-stream", stream,
                             "icon-names", icon_names,
                             NULL);
        return GVC_STREAM_STATUS_ICON (icon);
}
