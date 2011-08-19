/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gvc-applet.h"
#include "gvc-mixer-control.h"
#include "gvc-stream-status-icon.h"

#define GVC_APPLET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_APPLET, GvcAppletPrivate))

#define SCALE_SIZE 128

static const char *output_icon_names[] = {
        "audio-volume-muted",
        "audio-volume-low",
        "audio-volume-medium",
        "audio-volume-high",
        NULL
};

static const char *input_icon_names[] = {
        "audio-input-microphone-muted",
        "audio-input-microphone-low",
        "audio-input-microphone-medium",
        "audio-input-microphone-high",
        NULL
};

struct GvcAppletPrivate
{
        GvcStreamStatusIcon *input_status_icon;
        GvcStreamStatusIcon *output_status_icon;
        GvcMixerControl     *control;
};

static void     gvc_applet_class_init (GvcAppletClass *klass);
static void     gvc_applet_init       (GvcApplet      *applet);
static void     gvc_applet_finalize   (GObject        *object);

G_DEFINE_TYPE (GvcApplet, gvc_applet, G_TYPE_OBJECT)

static void
maybe_show_status_icons (GvcApplet *applet)
{
        gboolean        show;
        GvcMixerStream *stream;
        GSList         *source_outputs, *l;

        show = TRUE;
        stream = gvc_mixer_control_get_default_sink (applet->priv->control);
        if (stream == NULL) {
                show = FALSE;
        }
        gtk_status_icon_set_visible (GTK_STATUS_ICON (applet->priv->output_status_icon), show);


        show = FALSE;
        stream = gvc_mixer_control_get_default_source (applet->priv->control);
        source_outputs = gvc_mixer_control_get_source_outputs (applet->priv->control);
        if (stream != NULL && source_outputs != NULL) {
                /* Check that we're not trying to add the peak detector
                 * as an application doing recording */
                for (l = source_outputs ; l ; l = l->next) {
                        GvcMixerStream *s = l->data;
                        const char *id;

                        id = gvc_mixer_stream_get_application_id (s);
                        if (id == NULL) {
                                show = TRUE;
                                break;
                        }

                        if (!g_str_equal (id, "org.mate.VolumeControl") &&
                            !g_str_equal (id, "org.PulseAudio.pavucontrol")) {
                                show = TRUE;
                                break;
                        }
                }
        }
        gtk_status_icon_set_visible (GTK_STATUS_ICON (applet->priv->input_status_icon), show);

        g_slist_free (source_outputs);
}

void
gvc_applet_start (GvcApplet *applet)
{
        g_return_if_fail (GVC_IS_APPLET (applet));

        maybe_show_status_icons (applet);
}

static void
gvc_applet_dispose (GObject *object)
{
        GvcApplet *applet = GVC_APPLET (object);

        if (applet->priv->control != NULL) {
                g_object_unref (applet->priv->control);
                applet->priv->control = NULL;
        }

        G_OBJECT_CLASS (gvc_applet_parent_class)->dispose (object);
}

static void
update_default_source (GvcApplet *applet)
{
        GvcMixerStream *stream;

        stream = gvc_mixer_control_get_default_source (applet->priv->control);
        if (stream != NULL) {
                gvc_stream_status_icon_set_mixer_stream (applet->priv->input_status_icon,
                                                         stream);
                maybe_show_status_icons(applet);
        } else {
                g_debug ("Unable to get default source, or no source available");
        }
}

static void
update_default_sink (GvcApplet *applet)
{
        GvcMixerStream *stream;

        stream = gvc_mixer_control_get_default_sink (applet->priv->control);
        if (stream != NULL) {
                gvc_stream_status_icon_set_mixer_stream (applet->priv->output_status_icon,
                                                         stream);
                maybe_show_status_icons(applet);
        } else {
                g_warning ("Unable to get default sink");
        }
}

static void
on_control_ready (GvcMixerControl *control,
                  GvcApplet       *applet)
{
        update_default_sink (applet);
        update_default_source (applet);
}

static void
on_control_connecting (GvcMixerControl *control,
                       GvcApplet       *applet)
{
        g_debug ("Connecting..");
}

static void
on_control_default_sink_changed (GvcMixerControl *control,
                                 guint            id,
                                 GvcApplet       *applet)
{
        update_default_sink (applet);
}

static void
on_control_default_source_changed (GvcMixerControl *control,
                                   guint            id,
                                   GvcApplet       *applet)
{
        update_default_source (applet);
}

static void
on_control_stream_removed (GvcMixerControl *control,
                           guint            id,
                           GvcApplet       *applet)
{
        maybe_show_status_icons (applet);
}

static void
on_control_stream_added (GvcMixerControl *control,
                         guint            id,
                         GvcApplet       *applet)
{
        maybe_show_status_icons (applet);
}

static GObject *
gvc_applet_constructor (GType                  type,
                        guint                  n_construct_properties,
                        GObjectConstructParam *construct_params)
{
        GObject   *object;
        GvcApplet *self;

        object = G_OBJECT_CLASS (gvc_applet_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_APPLET (object);

        self->priv->control = gvc_mixer_control_new ("MATE Volume Control Applet");
        g_signal_connect (self->priv->control,
                          "ready",
                          G_CALLBACK (on_control_ready),
                          self);
        g_signal_connect (self->priv->control,
                          "connecting",
                          G_CALLBACK (on_control_connecting),
                          self);
        g_signal_connect (self->priv->control,
                          "default-sink-changed",
                          G_CALLBACK (on_control_default_sink_changed),
                          self);
        g_signal_connect (self->priv->control,
                          "default-source-changed",
                          G_CALLBACK (on_control_default_source_changed),
                          self);
        g_signal_connect (self->priv->control,
                          "stream-added",
                          G_CALLBACK (on_control_stream_added),
                          self);
        g_signal_connect (self->priv->control,
                          "stream-removed",
                          G_CALLBACK (on_control_stream_removed),
                          self);

        gvc_mixer_control_open (self->priv->control);

        return object;
}

static void
gvc_applet_class_init (GvcAppletClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gvc_applet_finalize;
        object_class->dispose = gvc_applet_dispose;
        object_class->constructor = gvc_applet_constructor;

        g_type_class_add_private (klass, sizeof (GvcAppletPrivate));
}

static void
gvc_applet_init (GvcApplet *applet)
{
        applet->priv = GVC_APPLET_GET_PRIVATE (applet);

        applet->priv->output_status_icon = gvc_stream_status_icon_new (NULL,
                                                                       output_icon_names);
        gvc_stream_status_icon_set_display_name (applet->priv->output_status_icon,
                                                 _("Output"));
        gtk_status_icon_set_title (GTK_STATUS_ICON (applet->priv->output_status_icon),
                                   _("Sound Output Volume"));
        applet->priv->input_status_icon = gvc_stream_status_icon_new (NULL,
                                                                      input_icon_names);
        gvc_stream_status_icon_set_display_name (applet->priv->input_status_icon,
                                                 _("Input"));
        gtk_status_icon_set_title (GTK_STATUS_ICON (applet->priv->input_status_icon),
                                   _("Microphone Volume"));
}

static void
gvc_applet_finalize (GObject *object)
{
        GvcApplet *applet;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_APPLET (object));

        applet = GVC_APPLET (object);

        g_return_if_fail (applet->priv != NULL);


        G_OBJECT_CLASS (gvc_applet_parent_class)->finalize (object);
}

GvcApplet *
gvc_applet_new (void)
{
        GObject *applet;

        applet = g_object_new (GVC_TYPE_APPLET, NULL);

        return GVC_APPLET (applet);
}
