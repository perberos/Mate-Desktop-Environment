/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Bastien Nocera
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
#include <canberra.h>
#include <canberra-gtk.h>

#include "gvc-speaker-test.h"
#include "gvc-mixer-stream.h"
#include "gvc-mixer-card.h"

#define GVC_SPEAKER_TEST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_SPEAKER_TEST, GvcSpeakerTestPrivate))

struct GvcSpeakerTestPrivate
{
        GtkWidget       *channel_controls[PA_CHANNEL_POSITION_MAX];
        ca_context      *canberra;
        GvcMixerCard    *card;
        GvcMixerControl *control;
};

enum {
        COL_NAME,
        COL_HUMAN_NAME,
        NUM_COLS
};

enum {
        PROP_0,
        PROP_CARD,
        PROP_CONTROL
};

static void     gvc_speaker_test_class_init (GvcSpeakerTestClass *klass);
static void     gvc_speaker_test_init       (GvcSpeakerTest      *speaker_test);
static void     gvc_speaker_test_finalize   (GObject            *object);
static void     update_channel_map          (GvcSpeakerTest *speaker_test);

G_DEFINE_TYPE (GvcSpeakerTest, gvc_speaker_test, GTK_TYPE_TABLE)

static const int position_table[] = {
        /* Position, X, Y */
        PA_CHANNEL_POSITION_FRONT_LEFT, 0, 0,
        PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER, 1, 0,
        PA_CHANNEL_POSITION_FRONT_CENTER, 2, 0,
        PA_CHANNEL_POSITION_MONO, 2, 0,
        PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER, 3, 0,
        PA_CHANNEL_POSITION_FRONT_RIGHT, 4, 0,
        PA_CHANNEL_POSITION_SIDE_LEFT, 0, 1,
        PA_CHANNEL_POSITION_SIDE_RIGHT, 4, 1,
        PA_CHANNEL_POSITION_REAR_LEFT, 0, 2,
        PA_CHANNEL_POSITION_REAR_CENTER, 2, 2,
        PA_CHANNEL_POSITION_REAR_RIGHT, 4, 2,
        PA_CHANNEL_POSITION_LFE, 3, 2
};

static void
gvc_speaker_test_set_property (GObject       *object,
                               guint          prop_id,
                               const GValue  *value,
                               GParamSpec    *pspec)
{
        GvcSpeakerTest *self = GVC_SPEAKER_TEST (object);

        switch (prop_id) {
        case PROP_CARD:
                self->priv->card = g_value_dup_object (value);
                if (self->priv->control != NULL)
                        update_channel_map (self);
                break;
        case PROP_CONTROL:
                self->priv->control = g_value_dup_object (value);
                if (self->priv->card != NULL)
                        update_channel_map (self);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_speaker_test_get_property (GObject     *object,
                               guint        prop_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
        GvcSpeakerTest *self = GVC_SPEAKER_TEST (object);

        switch (prop_id) {
        case PROP_CARD:
                g_value_set_object (value, self->priv->card);
                break;
        case PROP_CONTROL:
                g_value_set_object (value, self->priv->control);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_speaker_test_class_init (GvcSpeakerTestClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gvc_speaker_test_finalize;
        object_class->set_property = gvc_speaker_test_set_property;
        object_class->get_property = gvc_speaker_test_get_property;

        g_object_class_install_property (object_class,
                                         PROP_CARD,
                                         g_param_spec_object ("card",
                                                              "card",
                                                              "The card",
                                                              GVC_TYPE_MIXER_CARD,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_CONTROL,
                                         g_param_spec_object ("control",
                                                              "control",
                                                              "The mixer controller",
                                                              GVC_TYPE_MIXER_CONTROL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
        g_type_class_add_private (klass, sizeof (GvcSpeakerTestPrivate));
}

static const char *
sound_name (pa_channel_position_t position)
{
        switch (position) {
        case PA_CHANNEL_POSITION_FRONT_LEFT:
                return "audio-channel-front-left";
        case PA_CHANNEL_POSITION_FRONT_RIGHT:
                return "audio-channel-front-right";
        case PA_CHANNEL_POSITION_FRONT_CENTER:
                return "audio-channel-front-center";
        case PA_CHANNEL_POSITION_REAR_LEFT:
                return "audio-channel-rear-left";
        case PA_CHANNEL_POSITION_REAR_RIGHT:
                return "audio-channel-rear-right";
        case PA_CHANNEL_POSITION_REAR_CENTER:
                return "audio-channel-rear-center";
        case PA_CHANNEL_POSITION_LFE:
                return "audio-channel-lfe";
        case PA_CHANNEL_POSITION_SIDE_LEFT:
                return "audio-channel-side-left";
        case PA_CHANNEL_POSITION_SIDE_RIGHT:
                return "audio-channel-side-right";
        default:
                return NULL;
        }
}

static const char *
icon_name (pa_channel_position_t position, gboolean playing)
{
        switch (position) {
        case PA_CHANNEL_POSITION_FRONT_LEFT:
                return playing ? "audio-speaker-left-testing" : "audio-speaker-left";
        case PA_CHANNEL_POSITION_FRONT_RIGHT:
                return playing ? "audio-speaker-right-testing" : "audio-speaker-right";
        case PA_CHANNEL_POSITION_FRONT_CENTER:
                return playing ? "audio-speaker-center-testing" : "audio-speaker-center";
        case PA_CHANNEL_POSITION_REAR_LEFT:
                return playing ? "audio-speaker-left-back-testing" : "audio-speaker-left-back";
        case PA_CHANNEL_POSITION_REAR_RIGHT:
                return playing ? "audio-speaker-right-back-testing" : "audio-speaker-right-back";
        case PA_CHANNEL_POSITION_REAR_CENTER:
                return playing ? "audio-speaker-center-back-testing" : "audio-speaker-center-back";
        case PA_CHANNEL_POSITION_LFE:
                return playing ? "audio-subwoofer-testing" : "audio-subwoofer";
        case PA_CHANNEL_POSITION_SIDE_LEFT:
                return playing ? "audio-speaker-left-side-testing" : "audio-speaker-left-side";
        case PA_CHANNEL_POSITION_SIDE_RIGHT:
                return playing ? "audio-speaker-right-side-testing" : "audio-speaker-right-side";
        default:
                return NULL;
        }
}

static void
update_button (GtkWidget *control)
{
        GtkWidget *button;
        GtkWidget *image;
        pa_channel_position_t position;
        gboolean playing;

        button = g_object_get_data (G_OBJECT (control), "button");
        image = g_object_get_data (G_OBJECT (control), "image");
        position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (control), "position"));
        playing = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (control), "playing"));
        gtk_button_set_label (GTK_BUTTON (button), playing ? _("Stop") : _("Test"));
        gtk_image_set_from_icon_name (GTK_IMAGE (image), icon_name (position, playing), GTK_ICON_SIZE_DIALOG);
}

static const char *
pretty_position (pa_channel_position_t position)
{
        if (position == PA_CHANNEL_POSITION_LFE)
                return N_("Subwoofer");

        return pa_channel_position_to_pretty_string (position);
}

static gboolean
idle_cb (GtkWidget *control)
{
        if (control == NULL)
                return FALSE;

        /* This is called in the background thread, hence
         * forward to main thread via idle callback */
        g_object_set_data (G_OBJECT (control), "playing", GINT_TO_POINTER(FALSE));
        update_button (control);

        return FALSE;
}

static void
finish_cb (ca_context *c, uint32_t id, int error_code, void *userdata)
{
        GtkWidget *control = (GtkWidget *) userdata;

        if (error_code == CA_ERROR_DESTROYED || control == NULL)
                return;
        g_idle_add ((GSourceFunc) idle_cb, control);
}

static void
on_test_button_clicked (GtkButton *button,
                        GtkWidget *control)
{
        gboolean playing;
        ca_context *canberra;

        canberra = g_object_get_data (G_OBJECT (control), "canberra");

        ca_context_cancel (canberra, 1);

        playing = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (control), "playing"));

        if (playing) {
                g_object_set_data (G_OBJECT (control), "playing", GINT_TO_POINTER(FALSE));
        } else {
                pa_channel_position_t position;
                const char *name;
                ca_proplist *proplist;

                position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (control), "position"));

                ca_proplist_create (&proplist);
                ca_proplist_sets (proplist, CA_PROP_MEDIA_ROLE, "test");
                ca_proplist_sets (proplist, CA_PROP_MEDIA_NAME, pretty_position (position));
                ca_proplist_sets (proplist, CA_PROP_CANBERRA_FORCE_CHANNEL,
                                  pa_channel_position_to_string (position));
                ca_proplist_sets (proplist, CA_PROP_CANBERRA_ENABLE, "1");

                name = sound_name (position);
                if (name != NULL) {
                        ca_proplist_sets (proplist, CA_PROP_EVENT_ID, name);
                        playing = ca_context_play_full (canberra, 1, proplist, finish_cb, control) >= 0;
                }

                if (!playing) {
                        ca_proplist_sets (proplist, CA_PROP_EVENT_ID, "audio-test-signal");
                        playing = ca_context_play_full (canberra, 1, proplist, finish_cb, control) >= 0;
                }

                if (!playing) {
                        ca_proplist_sets(proplist, CA_PROP_EVENT_ID, "bell-window-system");
                        playing = ca_context_play_full (canberra, 1, proplist, finish_cb, control) >= 0;
                }
                g_object_set_data (G_OBJECT (control), "playing", GINT_TO_POINTER(playing));
        }

        update_button (control);
}

static GtkWidget *
channel_control_new (ca_context *canberra, pa_channel_position_t position)
{
        GtkWidget *control;
        GtkWidget *box;
        GtkWidget *label;
        GtkWidget *image;
        GtkWidget *test_button;
        const char *name;

        control = gtk_vbox_new (FALSE, 6);
        g_object_set_data (G_OBJECT (control), "playing", GINT_TO_POINTER(FALSE));
        g_object_set_data (G_OBJECT (control), "position", GINT_TO_POINTER(position));
        g_object_set_data (G_OBJECT (control), "canberra", canberra);

        name = icon_name (position, FALSE);
        if (name == NULL)
                name = "audio-volume-medium";
        image = gtk_image_new_from_icon_name (name, GTK_ICON_SIZE_DIALOG);
        g_object_set_data (G_OBJECT (control), "image", image);
        gtk_box_pack_start (GTK_BOX (control), image, FALSE, FALSE, 0);

        label = gtk_label_new (pretty_position (position));
        gtk_box_pack_start (GTK_BOX (control), label, FALSE, FALSE, 0);

        test_button = gtk_button_new_with_label (_("Test"));
        g_signal_connect (G_OBJECT (test_button), "clicked",
                          G_CALLBACK (on_test_button_clicked), control);
        g_object_set_data (G_OBJECT (control), "button", test_button);

        box = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (box), test_button, TRUE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (control), box, FALSE, FALSE, 0);

        gtk_widget_show_all (control);

        return control;
}

static void
create_channel_controls (GvcSpeakerTest *speaker_test)
{
        guint i;

        for (i = 0; i < G_N_ELEMENTS (position_table); i += 3) {
                speaker_test->priv->channel_controls[position_table[i]] = channel_control_new (speaker_test->priv->canberra, (pa_channel_position_t) position_table[i]);
                gtk_table_attach (GTK_TABLE (speaker_test),
                                  speaker_test->priv->channel_controls[position_table[i]],
                                  position_table[i+1],
                                  position_table[i+1]+1,
                                  position_table[i+2],
                                  position_table[i+2]+1,
                                  GTK_EXPAND, GTK_EXPAND, 0, 0);
        }
}

static const GvcChannelMap *
get_channel_map_for_card (GvcMixerControl *control,
                          GvcMixerCard    *card,
                          char           **output_name)
{
        int card_index;
        GSList *sinks, *l;
        GvcMixerStream *stream;
        const GvcChannelMap *map;

        /* This gets the channel map for the only
         * output for the card */

        card_index = gvc_mixer_card_get_index (card);
        if (card_index == PA_INVALID_INDEX)
                return NULL;
        sinks = gvc_mixer_control_get_sinks (control);
        stream = NULL;
        for (l = sinks; l != NULL; l = l->next) {
                GvcMixerStream *s = l->data;
                if (gvc_mixer_stream_get_card_index (s) == card_index) {
                        stream = g_object_ref (s);
                        break;
                }
        }
        g_slist_free (sinks);

        g_assert (stream);

        g_debug ("Found stream '%s' for card '%s'",
                 gvc_mixer_stream_get_name (stream),
                 gvc_mixer_card_get_name (card));

        *output_name = g_strdup (gvc_mixer_stream_get_name (stream));
        map = gvc_mixer_stream_get_channel_map (stream);

        g_debug ("Got channel map '%s' for port '%s'",
                 gvc_channel_map_get_mapping (map), *output_name);

        return map;
}

static void
update_channel_map (GvcSpeakerTest *speaker_test)
{
        guint i;
        const GvcChannelMap *map;
        char *output_name;

        g_return_if_fail (speaker_test->priv->control != NULL);
        g_return_if_fail (speaker_test->priv->card != NULL);

        g_debug ("XXX update_channel_map called XXX");

        map = get_channel_map_for_card (speaker_test->priv->control,
                                        speaker_test->priv->card,
                                        &output_name);

        g_return_if_fail (map != NULL);

        ca_context_change_device (speaker_test->priv->canberra, output_name);
        g_free (output_name);

        for (i = 0; i < G_N_ELEMENTS (position_table); i += 3) {
                gtk_widget_set_visible (speaker_test->priv->channel_controls[position_table[i]],
                                        gvc_channel_map_has_position(map, position_table[i]));
        }
}

static void
gvc_speaker_test_init (GvcSpeakerTest *speaker_test)
{
        GtkWidget *face;

        speaker_test->priv = GVC_SPEAKER_TEST_GET_PRIVATE (speaker_test);

        ca_context_create (&speaker_test->priv->canberra);
        ca_context_set_driver (speaker_test->priv->canberra, "pulse");
        ca_context_change_props (speaker_test->priv->canberra,
                                 CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
                                 NULL);

        gtk_table_resize (GTK_TABLE (speaker_test), 3, 5);
        gtk_container_set_border_width (GTK_CONTAINER (speaker_test), 12);
        gtk_table_set_homogeneous (GTK_TABLE (speaker_test), TRUE);
        gtk_table_set_row_spacings (GTK_TABLE (speaker_test), 12);
        gtk_table_set_col_spacings (GTK_TABLE (speaker_test), 12);

        create_channel_controls (speaker_test);

        face = gtk_image_new_from_icon_name ("face-smile", GTK_ICON_SIZE_DIALOG);
        gtk_table_attach (GTK_TABLE (speaker_test), face,
                          2, 3, 1, 2, GTK_EXPAND, GTK_EXPAND, 0, 0);
        gtk_widget_show (face);
}

static void
gvc_speaker_test_finalize (GObject *object)
{
        GvcSpeakerTest *speaker_test;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_SPEAKER_TEST (object));

        speaker_test = GVC_SPEAKER_TEST (object);

        g_return_if_fail (speaker_test->priv != NULL);

        g_object_unref (speaker_test->priv->card);
        speaker_test->priv->card = NULL;

        g_object_unref (speaker_test->priv->control);
        speaker_test->priv->control = NULL;

        ca_context_destroy (speaker_test->priv->canberra);
        speaker_test->priv->canberra = NULL;

        G_OBJECT_CLASS (gvc_speaker_test_parent_class)->finalize (object);
}

GtkWidget *
gvc_speaker_test_new (GvcMixerControl *control,
                      GvcMixerCard *card)
{
        GObject *speaker_test;

        g_return_val_if_fail (card != NULL, NULL);
        g_return_val_if_fail (control != NULL, NULL);

        speaker_test = g_object_new (GVC_TYPE_SPEAKER_TEST,
                                  "card", card,
                                  "control", control,
                                  NULL);

        return GTK_WIDGET (speaker_test);
}

