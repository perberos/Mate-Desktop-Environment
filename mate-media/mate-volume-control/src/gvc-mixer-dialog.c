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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gvc-channel-bar.h"
#include "gvc-balance-bar.h"
#include "gvc-combo-box.h"
#include "gvc-mixer-control.h"
#include "gvc-mixer-card.h"
#include "gvc-mixer-sink.h"
#include "gvc-mixer-source.h"
#include "gvc-mixer-source-output.h"
#include "gvc-mixer-dialog.h"
#include "gvc-sound-theme-chooser.h"
#include "gvc-level-bar.h"
#include "gvc-speaker-test.h"

#define SCALE_SIZE 128

#define GVC_MIXER_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_MIXER_DIALOG, GvcMixerDialogPrivate))

struct GvcMixerDialogPrivate
{
        GvcMixerControl *mixer_control;
        GHashTable      *bars;
        GtkWidget       *notebook;
        GtkWidget       *output_bar;
        GtkWidget       *input_bar;
        GtkWidget       *input_level_bar;
        GtkWidget       *effects_bar;
        GtkWidget       *output_stream_box;
        GtkWidget       *sound_effects_box;
        GtkWidget       *hw_box;
        GtkWidget       *hw_treeview;
        GtkWidget       *hw_settings_box;
        GtkWidget       *hw_profile_combo;
        GtkWidget       *input_box;
        GtkWidget       *output_box;
        GtkWidget       *applications_box;
        GtkWidget       *no_apps_label;
        GtkWidget       *output_treeview;
        GtkWidget       *output_settings_box;
        GtkWidget       *output_balance_bar;
        GtkWidget       *output_fade_bar;
        GtkWidget       *output_lfe_bar;
        GtkWidget       *output_port_combo;
        GtkWidget       *input_treeview;
        GtkWidget       *input_port_combo;
        GtkWidget       *input_settings_box;
        GtkWidget       *sound_theme_chooser;
        GtkWidget       *click_feedback_button;
        GtkWidget       *audible_bell_button;
        GtkSizeGroup    *size_group;
        GtkSizeGroup    *apps_size_group;

        gdouble          last_input_peak;
        guint            num_apps;
};

enum {
        NAME_COLUMN,
        DEVICE_COLUMN,
        ACTIVE_COLUMN,
        ID_COLUMN,
        SPEAKERS_COLUMN,
        NUM_COLUMNS
};

enum {
        HW_ID_COLUMN,
        HW_ICON_COLUMN,
        HW_NAME_COLUMN,
        HW_STATUS_COLUMN,
        HW_PROFILE_COLUMN,
        HW_PROFILE_HUMAN_COLUMN,
        HW_SENSITIVE_COLUMN,
        HW_NUM_COLUMNS
};

enum
{
        PROP_0,
        PROP_MIXER_CONTROL
};

static void     gvc_mixer_dialog_class_init (GvcMixerDialogClass *klass);
static void     gvc_mixer_dialog_init       (GvcMixerDialog      *mixer_dialog);
static void     gvc_mixer_dialog_finalize   (GObject             *object);

static void     bar_set_stream              (GvcMixerDialog      *dialog,
                                             GtkWidget           *bar,
                                             GvcMixerStream      *stream);

static void     on_adjustment_value_changed (GtkAdjustment  *adjustment,
                                             GvcMixerDialog *dialog);

G_DEFINE_TYPE (GvcMixerDialog, gvc_mixer_dialog, GTK_TYPE_DIALOG)

static void
update_default_input (GvcMixerDialog *dialog)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gboolean      ret;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
        ret = gtk_tree_model_get_iter_first (model, &iter);
        if (ret == FALSE) {
                g_debug ("No default input selected or available");
                return;
        }
        do {
                gboolean        toggled;
                gboolean        is_default;
                guint           id;
                GvcMixerStream *stream;

                gtk_tree_model_get (model, &iter,
                                    ID_COLUMN, &id,
                                    ACTIVE_COLUMN, &toggled,
                                    -1);

                stream = gvc_mixer_control_lookup_stream_id (dialog->priv->mixer_control, id);
                if (stream == NULL) {
                        g_warning ("Unable to find stream for id: %u", id);
                        continue;
                }

                is_default = FALSE;
                if (stream == gvc_mixer_control_get_default_source (dialog->priv->mixer_control)) {
                        is_default = TRUE;
                }

                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    ACTIVE_COLUMN, is_default,
                                    -1);
        } while (gtk_tree_model_iter_next (model, &iter));
}

static void
update_description (GvcMixerDialog *dialog,
                    guint column,
                    const char *value,
                    GvcMixerStream *stream)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        guint         id;

        if (GVC_IS_MIXER_SOURCE (stream))
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
        else if (GVC_IS_MIXER_SINK (stream))
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
        else
                g_assert_not_reached ();
        gtk_tree_model_get_iter_first (model, &iter);

        id = gvc_mixer_stream_get_id (stream);
        do {
                guint       current_id;

                gtk_tree_model_get (model, &iter,
                                    ID_COLUMN, &current_id,
                                    -1);
                if (id != current_id)
                        continue;

                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    column, value,
                                    -1);
                break;
        } while (gtk_tree_model_iter_next (model, &iter));
}

static void
port_selection_changed (GvcComboBox *combo_box,
                        const char  *port,
                        GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;

        stream = g_object_get_data (G_OBJECT (combo_box), "stream");
        if (stream == NULL) {
                g_warning ("Could not find stream for port combo box");
                return;
        }
        if (gvc_mixer_stream_change_port (stream, port) == FALSE) {
                g_warning ("Could not change port for stream");
        }
}

static void
update_output_settings (GvcMixerDialog *dialog)
{
        GvcMixerStream      *stream;
        const GvcChannelMap *map;
        const GList         *ports;

        g_debug ("Updating output settings");
        if (dialog->priv->output_balance_bar != NULL) {
                gtk_container_remove (GTK_CONTAINER (dialog->priv->output_settings_box),
                                      dialog->priv->output_balance_bar);
                dialog->priv->output_balance_bar = NULL;
        }
        if (dialog->priv->output_fade_bar != NULL) {
                gtk_container_remove (GTK_CONTAINER (dialog->priv->output_settings_box),
                                      dialog->priv->output_fade_bar);
                dialog->priv->output_fade_bar = NULL;
        }
        if (dialog->priv->output_lfe_bar != NULL) {
                gtk_container_remove (GTK_CONTAINER (dialog->priv->output_settings_box),
                                      dialog->priv->output_lfe_bar);
                dialog->priv->output_lfe_bar = NULL;
        }
        if (dialog->priv->output_port_combo != NULL) {
                gtk_container_remove (GTK_CONTAINER (dialog->priv->output_settings_box),
                                      dialog->priv->output_port_combo);
                dialog->priv->output_port_combo = NULL;
        }

        stream = gvc_mixer_control_get_default_sink (dialog->priv->mixer_control);
        if (stream == NULL) {
                g_warning ("Default sink stream not found");
                return;
        }

        gvc_channel_bar_set_base_volume (GVC_CHANNEL_BAR (dialog->priv->output_bar),
                                         gvc_mixer_stream_get_base_volume (stream));
        gvc_channel_bar_set_is_amplified (GVC_CHANNEL_BAR (dialog->priv->output_bar),
                                          gvc_mixer_stream_get_can_decibel (stream));

        map = gvc_mixer_stream_get_channel_map (stream);
        if (map == NULL) {
                g_warning ("Default sink stream has no channel map");
                return;
        }

        dialog->priv->output_balance_bar = gvc_balance_bar_new (map, BALANCE_TYPE_RL);
        if (dialog->priv->size_group != NULL) {
                gvc_balance_bar_set_size_group (GVC_BALANCE_BAR (dialog->priv->output_balance_bar),
                                                dialog->priv->size_group,
                                                TRUE);
        }
        gtk_box_pack_start (GTK_BOX (dialog->priv->output_settings_box),
                            dialog->priv->output_balance_bar,
                            FALSE, FALSE, 6);
        gtk_widget_show (dialog->priv->output_balance_bar);

        if (gvc_channel_map_can_fade (map)) {
                dialog->priv->output_fade_bar = gvc_balance_bar_new (map, BALANCE_TYPE_FR);
                if (dialog->priv->size_group != NULL) {
                        gvc_balance_bar_set_size_group (GVC_BALANCE_BAR (dialog->priv->output_fade_bar),
                                                        dialog->priv->size_group,
                                                        TRUE);
                }
                gtk_box_pack_start (GTK_BOX (dialog->priv->output_settings_box),
                                    dialog->priv->output_fade_bar,
                                    FALSE, FALSE, 6);
                gtk_widget_show (dialog->priv->output_fade_bar);
        }

        if (gvc_channel_map_has_lfe (map)) {
                dialog->priv->output_lfe_bar = gvc_balance_bar_new (map, BALANCE_TYPE_LFE);
                if (dialog->priv->size_group != NULL) {
                        gvc_balance_bar_set_size_group (GVC_BALANCE_BAR (dialog->priv->output_lfe_bar),
                                                        dialog->priv->size_group,
                                                        TRUE);
                }
                gtk_box_pack_start (GTK_BOX (dialog->priv->output_settings_box),
                                    dialog->priv->output_lfe_bar,
                                    FALSE, FALSE, 6);
                gtk_widget_show (dialog->priv->output_lfe_bar);
        }

        ports = gvc_mixer_stream_get_ports (stream);
        if (ports != NULL) {
                const GvcMixerStreamPort *port;
                port = gvc_mixer_stream_get_port (stream);

                dialog->priv->output_port_combo = gvc_combo_box_new (_("Co_nnector:"));
                gvc_combo_box_set_ports (GVC_COMBO_BOX (dialog->priv->output_port_combo),
                                         ports);
                gvc_combo_box_set_active (GVC_COMBO_BOX (dialog->priv->output_port_combo), port->port);
                g_object_set_data (G_OBJECT (dialog->priv->output_port_combo), "stream", stream);
                g_signal_connect (G_OBJECT (dialog->priv->output_port_combo), "changed",
                                  G_CALLBACK (port_selection_changed), dialog);

                gtk_box_pack_start (GTK_BOX (dialog->priv->output_settings_box),
                                    dialog->priv->output_port_combo,
                                    TRUE, FALSE, 6);

                gvc_combo_box_set_size_group (GVC_COMBO_BOX (dialog->priv->output_port_combo), dialog->priv->size_group, FALSE);

                gtk_widget_show (dialog->priv->output_port_combo);
        }

        /* FIXME: We could make this into a "No settings" label instead */
        gtk_widget_set_sensitive (dialog->priv->output_balance_bar, gvc_channel_map_can_balance (map));
}

static void
update_default_output (GvcMixerDialog *dialog)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
        gtk_tree_model_get_iter_first (model, &iter);
        do {
                gboolean        toggled;
                gboolean        is_default;
                guint           id;
                GvcMixerStream *stream;

                gtk_tree_model_get (model, &iter,
                                    ID_COLUMN, &id,
                                    ACTIVE_COLUMN, &toggled,
                                    -1);

                stream = gvc_mixer_control_lookup_stream_id (dialog->priv->mixer_control, id);
                if (stream == NULL) {
                        g_warning ("Unable to find stream for id: %u", id);
                        continue;
                }

                is_default = FALSE;
                if (stream == gvc_mixer_control_get_default_sink (dialog->priv->mixer_control)) {
                        is_default = TRUE;
                }

                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    ACTIVE_COLUMN, is_default,
                                    -1);
        } while (gtk_tree_model_iter_next (model, &iter));
}

static void
on_mixer_control_default_sink_changed (GvcMixerControl *control,
                                       guint            id,
                                       GvcMixerDialog  *dialog)
{
        GvcMixerStream *stream;

        g_debug ("GvcMixerDialog: default sink changed: %u", id);

        if (id == PA_INVALID_INDEX)
                stream = NULL;
        else
                stream = gvc_mixer_control_lookup_stream_id (dialog->priv->mixer_control,
                                                             id);
        bar_set_stream (dialog, dialog->priv->output_bar, stream);

        update_output_settings (dialog);

        update_default_output (dialog);
}


#define DECAY_STEP .15

static void
update_input_peak (GvcMixerDialog *dialog,
                   gdouble         v)
{
        GtkAdjustment *adj;

        if (dialog->priv->last_input_peak >= DECAY_STEP) {
                if (v < dialog->priv->last_input_peak - DECAY_STEP) {
                        v = dialog->priv->last_input_peak - DECAY_STEP;
                }
        }

        dialog->priv->last_input_peak = v;

        adj = gvc_level_bar_get_peak_adjustment (GVC_LEVEL_BAR (dialog->priv->input_level_bar));
        if (v >= 0) {
                gtk_adjustment_set_value (adj, v);
        } else {
                gtk_adjustment_set_value (adj, 0.0);
        }
}

static void
update_input_meter (GvcMixerDialog *dialog,
                    uint32_t        source_index,
                    uint32_t        sink_input_idx,
                    double          v)
{
        update_input_peak (dialog, v);
}

static void
on_monitor_suspended_callback (pa_stream *s,
                               void      *userdata)
{
        GvcMixerDialog *dialog;

        dialog = userdata;

        if (pa_stream_is_suspended (s)) {
                g_debug ("Stream suspended");
                update_input_meter (dialog,
                                    pa_stream_get_device_index (s),
                                    PA_INVALID_INDEX,
                                    -1);
        }
}

static void
on_monitor_read_callback (pa_stream *s,
                          size_t     length,
                          void      *userdata)
{
        GvcMixerDialog *dialog;
        const void     *data;
        double          v;

        dialog = userdata;

        if (pa_stream_peek (s, &data, &length) < 0) {
                g_warning ("Failed to read data from stream");
                return;
        }

        assert (length > 0);
        assert (length % sizeof (float) == 0);

        v = ((const float *) data)[length / sizeof (float) -1];

        pa_stream_drop (s);

        if (v < 0) {
                v = 0;
        }
        if (v > 1) {
                v = 1;
        }

        update_input_meter (dialog,
                            pa_stream_get_device_index (s),
                            pa_stream_get_monitor_stream (s),
                            v);
}

static void
create_monitor_stream_for_source (GvcMixerDialog *dialog,
                                  GvcMixerStream *stream)
{
        pa_stream     *s;
        char           t[16];
        pa_buffer_attr attr;
        pa_sample_spec ss;
        pa_context    *context;
        int            res;
        pa_proplist   *proplist;
        gboolean       has_monitor;

        if (stream == NULL) {
                return;
        }
        has_monitor = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (stream), "has-monitor"));
        if (has_monitor != FALSE) {
                return;
        }

        g_debug ("Create monitor for %u",
                 gvc_mixer_stream_get_index (stream));

        context = gvc_mixer_control_get_pa_context (dialog->priv->mixer_control);

        if (pa_context_get_server_protocol_version (context) < 13) {
                return;
        }

        ss.channels = 1;
        ss.format = PA_SAMPLE_FLOAT32;
        ss.rate = 25;

        memset (&attr, 0, sizeof (attr));
        attr.fragsize = sizeof (float);
        attr.maxlength = (uint32_t) -1;

        snprintf (t, sizeof (t), "%u", gvc_mixer_stream_get_index (stream));

        proplist = pa_proplist_new ();
        pa_proplist_sets (proplist, PA_PROP_APPLICATION_ID, "org.mate.VolumeControl");
        s = pa_stream_new_with_proplist (context, _("Peak detect"), &ss, NULL, proplist);
        pa_proplist_free (proplist);
        if (s == NULL) {
                g_warning ("Failed to create monitoring stream");
                return;
        }

        pa_stream_set_read_callback (s, on_monitor_read_callback, dialog);
        pa_stream_set_suspended_callback (s, on_monitor_suspended_callback, dialog);

        res = pa_stream_connect_record (s,
                                        t,
                                        &attr,
                                        (pa_stream_flags_t) (PA_STREAM_DONT_MOVE
                                                             |PA_STREAM_PEAK_DETECT
                                                             |PA_STREAM_ADJUST_LATENCY));
        if (res < 0) {
                g_warning ("Failed to connect monitoring stream");
                pa_stream_unref (s);
        } else {
                g_object_set_data (G_OBJECT (stream), "has-monitor", GINT_TO_POINTER (TRUE));
                g_object_set_data (G_OBJECT (dialog->priv->input_level_bar), "pa_stream", s);
                g_object_set_data (G_OBJECT (dialog->priv->input_level_bar), "stream", stream);
        }
}

static void
stop_monitor_stream_for_source (GvcMixerDialog *dialog)
{
        pa_stream      *s;
        pa_context     *context;
        int             res;
        GvcMixerStream *stream;

        s = g_object_get_data (G_OBJECT (dialog->priv->input_level_bar), "pa_stream");
        if (s == NULL)
                return;
        stream = g_object_get_data (G_OBJECT (dialog->priv->input_level_bar), "stream");
        g_assert (stream != NULL);

        g_debug ("Stopping monitor for %u", pa_stream_get_index (s));

        context = gvc_mixer_control_get_pa_context (dialog->priv->mixer_control);

        if (pa_context_get_server_protocol_version (context) < 13) {
                return;
        }

        res = pa_stream_disconnect (s);
        if (res == 0)
                g_object_set_data (G_OBJECT (stream), "has-monitor", GINT_TO_POINTER (FALSE));
        g_object_set_data (G_OBJECT (dialog->priv->input_level_bar), "pa_stream", NULL);
        g_object_set_data (G_OBJECT (dialog->priv->input_level_bar), "stream", NULL);
}

static void
update_input_settings (GvcMixerDialog *dialog)
{
        const GList *ports;
        GvcMixerStream *stream;

        g_debug ("Updating input settings");

        stop_monitor_stream_for_source (dialog);

        if (dialog->priv->input_port_combo != NULL) {
                gtk_container_remove (GTK_CONTAINER (dialog->priv->input_settings_box),
                                      dialog->priv->input_port_combo);
                dialog->priv->input_port_combo = NULL;
        }

        stream = gvc_mixer_control_get_default_source (dialog->priv->mixer_control);
        if (stream == NULL) {
                g_debug ("Default source stream not found");
                return;
        }

        gvc_channel_bar_set_base_volume (GVC_CHANNEL_BAR (dialog->priv->input_bar),
                                         gvc_mixer_stream_get_base_volume (stream));
        gvc_channel_bar_set_is_amplified (GVC_CHANNEL_BAR (dialog->priv->input_bar),
                                          gvc_mixer_stream_get_can_decibel (stream));

        ports = gvc_mixer_stream_get_ports (stream);
        if (ports != NULL) {
                const GvcMixerStreamPort *port;
                port = gvc_mixer_stream_get_port (stream);

                dialog->priv->input_port_combo = gvc_combo_box_new (_("Co_nnector:"));
                gvc_combo_box_set_ports (GVC_COMBO_BOX (dialog->priv->input_port_combo),
                                         ports);
                gvc_combo_box_set_active (GVC_COMBO_BOX (dialog->priv->input_port_combo), port->port);
                g_object_set_data (G_OBJECT (dialog->priv->input_port_combo), "stream", stream);
                g_signal_connect (G_OBJECT (dialog->priv->input_port_combo), "changed",
                                  G_CALLBACK (port_selection_changed), dialog);

                gvc_combo_box_set_size_group (GVC_COMBO_BOX (dialog->priv->input_port_combo), dialog->priv->size_group, FALSE);
                gtk_box_pack_start (GTK_BOX (dialog->priv->input_settings_box),
                                    dialog->priv->input_port_combo,
                                    TRUE, TRUE, 0);
                gtk_widget_show (dialog->priv->input_port_combo);
        }

        create_monitor_stream_for_source (dialog, stream);
}

static void
on_mixer_control_default_source_changed (GvcMixerControl *control,
                                         guint            id,
                                         GvcMixerDialog  *dialog)
{
        GvcMixerStream *stream;
        GtkAdjustment *adj;

        g_debug ("GvcMixerDialog: default source changed: %u", id);

        if (id == PA_INVALID_INDEX)
                stream = NULL;
        else
                stream = gvc_mixer_control_lookup_stream_id (dialog->priv->mixer_control, id);

        /* Disconnect the adj, otherwise it might change if is_amplified changes */
        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (dialog->priv->input_bar)));
        g_signal_handlers_disconnect_by_func(adj, on_adjustment_value_changed, dialog);

        bar_set_stream (dialog, dialog->priv->input_bar, stream);
        update_input_settings (dialog);

        g_signal_connect (adj,
                          "value-changed",
                          G_CALLBACK (on_adjustment_value_changed),
                          dialog);

        update_default_input (dialog);
}

static void
gvc_mixer_dialog_set_mixer_control (GvcMixerDialog  *dialog,
                                    GvcMixerControl *control)
{
        g_return_if_fail (GVC_MIXER_DIALOG (dialog));
        g_return_if_fail (GVC_IS_MIXER_CONTROL (control));

        g_object_ref (control);

        if (dialog->priv->mixer_control != NULL) {
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      G_CALLBACK (on_mixer_control_default_sink_changed),
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      G_CALLBACK (on_mixer_control_default_source_changed),
                                                      dialog);
                g_object_unref (dialog->priv->mixer_control);
        }

        dialog->priv->mixer_control = control;

        g_signal_connect (dialog->priv->mixer_control,
                          "default-sink-changed",
                          G_CALLBACK (on_mixer_control_default_sink_changed),
                          dialog);
        g_signal_connect (dialog->priv->mixer_control,
                          "default-source-changed",
                          G_CALLBACK (on_mixer_control_default_source_changed),
                          dialog);

        g_object_notify (G_OBJECT (dialog), "mixer-control");
}

static GvcMixerControl *
gvc_mixer_dialog_get_mixer_control (GvcMixerDialog *dialog)
{
        g_return_val_if_fail (GVC_IS_MIXER_DIALOG (dialog), NULL);

        return dialog->priv->mixer_control;
}

static void
gvc_mixer_dialog_set_property (GObject       *object,
                               guint          prop_id,
                               const GValue  *value,
                               GParamSpec    *pspec)
{
        GvcMixerDialog *self = GVC_MIXER_DIALOG (object);

        switch (prop_id) {
        case PROP_MIXER_CONTROL:
                gvc_mixer_dialog_set_mixer_control (self, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_mixer_dialog_get_property (GObject     *object,
                               guint        prop_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
        GvcMixerDialog *self = GVC_MIXER_DIALOG (object);

        switch (prop_id) {
        case PROP_MIXER_CONTROL:
                g_value_set_object (value, gvc_mixer_dialog_get_mixer_control (self));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
on_adjustment_value_changed (GtkAdjustment  *adjustment,
                             GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;

        stream = g_object_get_data (G_OBJECT (adjustment), "gvc-mixer-dialog-stream");
        if (stream != NULL) {
                GObject *bar;
                gdouble volume, rounded;
                char *name;

                volume = gtk_adjustment_get_value (adjustment);
                rounded = round (volume);

                bar = g_object_get_data (G_OBJECT (adjustment), "gvc-mixer-dialog-bar");
                g_object_get (bar, "name", &name, NULL);
                g_debug ("Setting stream volume %lf (rounded: %lf) for bar '%s'", volume, rounded, name);
                g_free (name);

                /* FIXME would need to do that in the balance bar really... */
                /* Make sure we do not unmute muted streams, there's a button for that */
                if (volume == 0.0)
                        gvc_mixer_stream_set_is_muted (stream, TRUE);
                /* Only push the volume if it's actually changed */
                if (gvc_mixer_stream_set_volume(stream, (pa_volume_t) rounded) != FALSE)
                        gvc_mixer_stream_push_volume (stream);
        }
}

static void
on_bar_is_muted_notify (GObject        *object,
                        GParamSpec     *pspec,
                        GvcMixerDialog *dialog)
{
        gboolean        is_muted;
        GvcMixerStream *stream;

        is_muted = gvc_channel_bar_get_is_muted (GVC_CHANNEL_BAR (object));

        stream = g_object_get_data (object, "gvc-mixer-dialog-stream");
        if (stream != NULL) {
                gvc_mixer_stream_change_is_muted (stream, is_muted);
        } else {
                char *name;
                g_object_get (object, "name", &name, NULL);
                g_warning ("Unable to find stream for bar '%s'", name);
                g_free (name);
        }
}

static GtkWidget *
lookup_bar_for_stream (GvcMixerDialog *dialog,
                       GvcMixerStream *stream)
{
        GtkWidget *bar;

        bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (gvc_mixer_stream_get_id (stream)));

        return bar;
}

static GtkWidget *
lookup_combo_box_for_stream (GvcMixerDialog *dialog,
                             GvcMixerStream *stream)
{
        GvcMixerStream *combo_stream;
        guint id;

        id = gvc_mixer_stream_get_id (stream);

        if (dialog->priv->output_port_combo != NULL) {
                combo_stream = g_object_get_data (G_OBJECT (dialog->priv->output_port_combo),
                                                  "stream");
                if (combo_stream != NULL) {
                        if (id == gvc_mixer_stream_get_id (combo_stream))
                                return dialog->priv->output_port_combo;
                }
        }

        if (dialog->priv->input_port_combo != NULL) {
                combo_stream = g_object_get_data (G_OBJECT (dialog->priv->input_port_combo),
                                                  "stream");
                if (combo_stream != NULL) {
                        if (id == gvc_mixer_stream_get_id (combo_stream))
                                return dialog->priv->input_port_combo;
                }
        }

        return NULL;
}

static void
on_stream_description_notify (GvcMixerStream *stream,
                              GParamSpec     *pspec,
                              GvcMixerDialog *dialog)
{
        update_description (dialog, NAME_COLUMN,
                            gvc_mixer_stream_get_description (stream),
                            stream);
}

static void
on_stream_port_notify (GObject        *object,
                       GParamSpec     *pspec,
                       GvcMixerDialog *dialog)
{
        GvcComboBox *combo_box;
        char *port;

        combo_box = GVC_COMBO_BOX (lookup_combo_box_for_stream (dialog, GVC_MIXER_STREAM (object)));
        if (combo_box == NULL)
                return;

        g_signal_handlers_block_by_func (G_OBJECT (combo_box),
                                         port_selection_changed,
                                         dialog);

        g_object_get (object, "port", &port, NULL);
        gvc_combo_box_set_active (GVC_COMBO_BOX (combo_box), port);

        g_signal_handlers_unblock_by_func (G_OBJECT (combo_box),
                                         port_selection_changed,
                                         dialog);
}

static void
on_stream_volume_notify (GObject        *object,
                         GParamSpec     *pspec,
                         GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;
        GtkWidget      *bar;
        GtkAdjustment  *adj;

        stream = GVC_MIXER_STREAM (object);

        bar = lookup_bar_for_stream (dialog, stream);

        if (bar == NULL) {
                g_warning ("Unable to find bar for stream %s in on_stream_volume_notify()",
                           gvc_mixer_stream_get_name (stream));
                return;
        }

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));

        g_signal_handlers_block_by_func (adj,
                                         on_adjustment_value_changed,
                                         dialog);

        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (stream));

        g_signal_handlers_unblock_by_func (adj,
                                           on_adjustment_value_changed,
                                           dialog);
}

static void
on_stream_is_muted_notify (GObject        *object,
                           GParamSpec     *pspec,
                           GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;
        GtkWidget      *bar;
        gboolean        is_muted;

        stream = GVC_MIXER_STREAM (object);
        bar = lookup_bar_for_stream (dialog, stream);

        if (bar == NULL) {
                g_warning ("Unable to find bar for stream %s in on_stream_is_muted_notify()",
                           gvc_mixer_stream_get_name (stream));
                return;
        }

        is_muted = gvc_mixer_stream_get_is_muted (stream);
        gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (bar),
                                      is_muted);

        if (stream == gvc_mixer_control_get_default_sink (dialog->priv->mixer_control)) {
                gtk_widget_set_sensitive (dialog->priv->applications_box,
                                          !is_muted);
        }

}

static void
save_bar_for_stream (GvcMixerDialog *dialog,
                     GvcMixerStream *stream,
                     GtkWidget      *bar)
{
        g_hash_table_insert (dialog->priv->bars,
                             GUINT_TO_POINTER (gvc_mixer_stream_get_id (stream)),
                             bar);
}

static GtkWidget *
create_bar (GvcMixerDialog *dialog,
            GtkSizeGroup   *size_group,
            gboolean        symmetric)
{
        GtkWidget *bar;

        bar = gvc_channel_bar_new ();
        gtk_widget_set_sensitive (bar, FALSE);
        if (size_group != NULL) {
                gvc_channel_bar_set_size_group (GVC_CHANNEL_BAR (bar),
                                                size_group,
                                                symmetric);
        }
        gvc_channel_bar_set_orientation (GVC_CHANNEL_BAR (bar),
                                         GTK_ORIENTATION_HORIZONTAL);
        gvc_channel_bar_set_show_mute (GVC_CHANNEL_BAR (bar),
                                       TRUE);
        g_signal_connect (bar,
                          "notify::is-muted",
                          G_CALLBACK (on_bar_is_muted_notify),
                          dialog);
        return bar;
}

static void
bar_set_stream (GvcMixerDialog *dialog,
                GtkWidget      *bar,
                GvcMixerStream *stream)
{
        GtkAdjustment  *adj;
        GvcMixerStream *old_stream;

        g_assert (bar != NULL);

        old_stream = g_object_get_data (G_OBJECT (bar), "gvc-mixer-dialog-stream");
        if (old_stream != NULL) {
                char *name;

                g_object_get (bar, "name", &name, NULL);
                g_debug ("Disconnecting old stream '%s' from bar '%s'",
                         gvc_mixer_stream_get_name (old_stream), name);
                g_free (name);

                g_signal_handlers_disconnect_by_func (old_stream, on_stream_is_muted_notify, dialog);
                g_signal_handlers_disconnect_by_func (old_stream, on_stream_volume_notify, dialog);
                g_signal_handlers_disconnect_by_func (old_stream, on_stream_port_notify, dialog);
                g_hash_table_remove (dialog->priv->bars, GUINT_TO_POINTER (gvc_mixer_stream_get_id (old_stream)));
        }

        gtk_widget_set_sensitive (bar, (stream != NULL));

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));

        g_signal_handlers_disconnect_by_func (adj, on_adjustment_value_changed, dialog);

        g_object_set_data (G_OBJECT (bar), "gvc-mixer-dialog-stream", stream);
        g_object_set_data (G_OBJECT (adj), "gvc-mixer-dialog-stream", stream);
        g_object_set_data (G_OBJECT (adj), "gvc-mixer-dialog-bar", bar);

        if (stream != NULL) {
                gboolean is_muted;

                is_muted = gvc_mixer_stream_get_is_muted (stream);
                gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (bar), is_muted);

                save_bar_for_stream (dialog, stream, bar);

                gtk_adjustment_set_value (adj,
                                          gvc_mixer_stream_get_volume (stream));

                g_signal_connect (stream,
                                  "notify::is-muted",
                                  G_CALLBACK (on_stream_is_muted_notify),
                                  dialog);
                g_signal_connect (stream,
                                  "notify::volume",
                                  G_CALLBACK (on_stream_volume_notify),
                                  dialog);
                g_signal_connect (stream,
                                  "notify::port",
                                  G_CALLBACK (on_stream_port_notify),
                                  dialog);
                g_signal_connect (adj,
                                  "value-changed",
                                  G_CALLBACK (on_adjustment_value_changed),
                                  dialog);
        }
}

static void
add_stream (GvcMixerDialog *dialog,
            GvcMixerStream *stream)
{
        GtkWidget     *bar;
        gboolean       is_muted;
        gboolean       is_default;
        GtkAdjustment *adj;
        const char    *id;

        g_assert (stream != NULL);

        if (gvc_mixer_stream_is_event_stream (stream) != FALSE)
                return;

        bar = NULL;
        is_default = FALSE;
        id = gvc_mixer_stream_get_application_id (stream);

        if (stream == gvc_mixer_control_get_default_sink (dialog->priv->mixer_control)) {
                bar = dialog->priv->output_bar;
                is_muted = gvc_mixer_stream_get_is_muted (stream);
                is_default = TRUE;
                gtk_widget_set_sensitive (dialog->priv->applications_box,
                                          !is_muted);
                adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));
                g_signal_handlers_disconnect_by_func(adj, on_adjustment_value_changed, dialog);
                update_output_settings (dialog);
        } else if (stream == gvc_mixer_control_get_default_source (dialog->priv->mixer_control)) {
                bar = dialog->priv->input_bar;
                adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));
                g_signal_handlers_disconnect_by_func(adj, on_adjustment_value_changed, dialog);
                update_input_settings (dialog);
                is_default = TRUE;
        } else if (stream == gvc_mixer_control_get_event_sink_input (dialog->priv->mixer_control)) {
                bar = dialog->priv->effects_bar;
                g_debug ("Adding effects stream");
        } else if (! GVC_IS_MIXER_SOURCE (stream)
                   && !GVC_IS_MIXER_SINK (stream)
                   && !gvc_mixer_stream_is_virtual (stream)
                   && g_strcmp0 (id, "org.mate.VolumeControl") != 0
                   && g_strcmp0 (id, "org.PulseAudio.pavucontrol") != 0) {
                const char *name;

                bar = create_bar (dialog, dialog->priv->apps_size_group, FALSE);

                name = gvc_mixer_stream_get_name (stream);
                if (name == NULL || strchr (name, '_') == NULL) {
                        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar), name);
                } else {
                        char **tokens, *escaped;

                        tokens = g_strsplit (name, "_", -1); 
                        escaped = g_strjoinv ("__", tokens);
                        g_strfreev (tokens);
                        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar), escaped);
                        g_free (escaped);
                }

                gvc_channel_bar_set_icon_name (GVC_CHANNEL_BAR (bar),
                                               gvc_mixer_stream_get_icon_name (stream));

                gtk_box_pack_start (GTK_BOX (dialog->priv->applications_box), bar, FALSE, FALSE, 12);
                dialog->priv->num_apps++;
                gtk_widget_hide (dialog->priv->no_apps_label);
        }

        if (GVC_IS_MIXER_SOURCE (stream)) {
                GtkTreeModel *model;
                GtkTreeIter   iter;

                model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
                gtk_list_store_append (GTK_LIST_STORE (model), &iter);
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    NAME_COLUMN, gvc_mixer_stream_get_description (stream),
                                    DEVICE_COLUMN, "",
                                    ACTIVE_COLUMN, is_default,
                                    ID_COLUMN, gvc_mixer_stream_get_id (stream),
                                    -1);
                g_signal_connect (stream,
                                  "notify::description",
                                  G_CALLBACK (on_stream_description_notify),
                                  dialog);
        } else if (GVC_IS_MIXER_SINK (stream)) {
                GtkTreeModel        *model;
                GtkTreeIter          iter;
                const GvcChannelMap *map;

                model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
                gtk_list_store_append (GTK_LIST_STORE (model), &iter);
                map = gvc_mixer_stream_get_channel_map (stream);
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    NAME_COLUMN, gvc_mixer_stream_get_description (stream),
                                    DEVICE_COLUMN, "",
                                    ACTIVE_COLUMN, is_default,
                                    ID_COLUMN, gvc_mixer_stream_get_id (stream),
                                    SPEAKERS_COLUMN, gvc_channel_map_get_mapping (map),
                                    -1);
                g_signal_connect (stream,
                                  "notify::description",
                                  G_CALLBACK (on_stream_description_notify),
                                  dialog);
        }

        if (bar != NULL) {
                bar_set_stream (dialog, bar, stream);
                gtk_widget_show (bar);
        }
}

static void
on_control_stream_added (GvcMixerControl *control,
                         guint            id,
                         GvcMixerDialog  *dialog)
{
        GvcMixerStream *stream;
        GtkWidget      *bar;

        bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (id));
        if (bar != NULL) {
                g_debug ("GvcMixerDialog: Stream %u already added", id);
                return;
        }

        stream = gvc_mixer_control_lookup_stream_id (control, id);
        if (stream != NULL) {
                add_stream (dialog, stream);
        }
}

static gboolean
find_item_by_id (GtkTreeModel *model,
                   guint         id,
                   guint         column,
                   GtkTreeIter  *iter)
{
        gboolean found_item;

        found_item = FALSE;

        if (!gtk_tree_model_get_iter_first (model, iter)) {
                return FALSE;
        }

        do {
                guint t_id;

                gtk_tree_model_get (model, iter,
                                    column, &t_id, -1);

                if (id == t_id) {
                        found_item = TRUE;
                }
        } while (!found_item && gtk_tree_model_iter_next (model, iter));

        return found_item;
}

static void
remove_stream (GvcMixerDialog  *dialog,
               guint            id)
{
        GtkWidget    *bar;
        gboolean      found;
        GtkTreeIter   iter;
        GtkTreeModel *model;

        /* remove bars for applications and reset fixed bars */
        bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (id));
        if (bar == dialog->priv->output_bar
            || bar == dialog->priv->input_bar
            || bar == dialog->priv->effects_bar) {
                char *name;
                g_object_get (bar, "name", &name, NULL);
                g_debug ("Removing stream for bar '%s'", name);
                g_free (name);
                bar_set_stream (dialog, bar, NULL);
        } else if (bar != NULL) {
                g_hash_table_remove (dialog->priv->bars, GUINT_TO_POINTER (id));
                gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (bar)),
                                      bar);
                dialog->priv->num_apps--;
                if (dialog->priv->num_apps == 0) {
                        gtk_widget_show (dialog->priv->no_apps_label);
                }
        }

        /* remove from any models */
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
        found = find_item_by_id (GTK_TREE_MODEL (model), id, ID_COLUMN, &iter);
        if (found) {
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        }
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
        found = find_item_by_id (GTK_TREE_MODEL (model), id, ID_COLUMN, &iter);
        if (found) {
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        }
}

static void
on_control_stream_removed (GvcMixerControl *control,
                           guint            id,
                           GvcMixerDialog  *dialog)
{
        remove_stream (dialog, id);
}

static void
add_card (GvcMixerDialog *dialog,
          GvcMixerCard   *card)
{
        GtkTreeModel        *model;
        GtkTreeIter          iter;
        GtkTreeSelection    *selection;
        GvcMixerCardProfile *profile;
        GIcon               *icon;
        guint                index;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->hw_treeview));
        index = gvc_mixer_card_get_index (card);
        if (find_item_by_id (GTK_TREE_MODEL (model), index, HW_ID_COLUMN, &iter) == FALSE)
                gtk_list_store_append (GTK_LIST_STORE (model), &iter);
        profile = gvc_mixer_card_get_profile (card);
        g_assert (profile != NULL);
        icon = g_themed_icon_new_with_default_fallbacks (gvc_mixer_card_get_icon_name (card));
        //FIXME we need the status (default for a profile?) here
        gtk_list_store_set (GTK_LIST_STORE (model),
                            &iter,
                            HW_NAME_COLUMN, gvc_mixer_card_get_name (card),
                            HW_ID_COLUMN, index,
                            HW_ICON_COLUMN, icon,
                            HW_PROFILE_COLUMN, profile->profile,
                            HW_PROFILE_HUMAN_COLUMN, profile->human_profile,
                            HW_STATUS_COLUMN, profile->status,
                            HW_SENSITIVE_COLUMN, g_strcmp0 (profile->profile, "off") != 0,
                            -1);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->hw_treeview));
        if (gtk_tree_selection_get_selected (selection, NULL, NULL) == FALSE) {
                gtk_tree_selection_select_iter (selection, &iter);
        } else if (dialog->priv->hw_profile_combo != NULL) {
                GvcMixerCard *selected;

                /* Set the current profile if it changed for the selected card */
                selected = g_object_get_data (G_OBJECT (dialog->priv->hw_profile_combo), "card");
                if (gvc_mixer_card_get_index (selected) == gvc_mixer_card_get_index (card)) {
                        gvc_combo_box_set_active (GVC_COMBO_BOX (dialog->priv->hw_profile_combo),
                                                  profile->profile);
                        g_object_set (G_OBJECT (dialog->priv->hw_profile_combo),
                                      "show-button", profile->n_sinks == 1,
                                      NULL);
                }
        }
}

static void
on_control_card_added (GvcMixerControl *control,
                       guint            id,
                       GvcMixerDialog  *dialog)
{
        GvcMixerCard *card;

        card = gvc_mixer_control_lookup_card_id (control, id);
        if (card != NULL) {
                add_card (dialog, card);
        }
}

static void
remove_card (GvcMixerDialog  *dialog,
             guint            id)
{
        gboolean      found;
        GtkTreeIter   iter;
        GtkTreeModel *model;

        /* remove from any models */
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->hw_treeview));
        found = find_item_by_id (GTK_TREE_MODEL (model), id, HW_ID_COLUMN, &iter);
        if (found) {
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        }
}
static void
on_control_card_removed (GvcMixerControl *control,
                         guint            id,
                         GvcMixerDialog  *dialog)
{
        remove_card (dialog, id);
}

static void
_gtk_label_make_bold (GtkLabel *label)
{
        PangoFontDescription *font_desc;

        font_desc = pango_font_description_new ();

        pango_font_description_set_weight (font_desc,
                                           PANGO_WEIGHT_BOLD);

        /* This will only affect the weight of the font, the rest is
         * from the current state of the widget, which comes from the
         * theme or user prefs, since the font desc only has the
         * weight flag turned on.
         */
        gtk_widget_modify_font (GTK_WIDGET (label), font_desc);

        pango_font_description_free (font_desc);
}

static void
on_input_radio_toggled (GtkCellRendererToggle *renderer,
                        char                  *path_str,
                        GvcMixerDialog        *dialog)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        GtkTreePath  *path;
        gboolean      toggled;
        guint         id;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));

        path = gtk_tree_path_new_from_string (path_str);
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_path_free (path);

        gtk_tree_model_get (model, &iter,
                            ID_COLUMN, &id,
                            ACTIVE_COLUMN, &toggled,
                            -1);

        toggled ^= 1;
        if (toggled) {
                GvcMixerStream *stream;

                g_debug ("Default input selected: %u", id);
                stream = gvc_mixer_control_lookup_stream_id (dialog->priv->mixer_control, id);
                if (stream == NULL) {
                        g_warning ("Unable to find stream for id: %u", id);
                        return;
                }

                gvc_mixer_control_set_default_source (dialog->priv->mixer_control, stream);
        }
}

static void
on_output_radio_toggled (GtkCellRendererToggle *renderer,
                         char                  *path_str,
                         GvcMixerDialog        *dialog)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        GtkTreePath  *path;
        gboolean      toggled;
        guint         id;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));

        path = gtk_tree_path_new_from_string (path_str);
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_path_free (path);

        gtk_tree_model_get (model, &iter,
                            ID_COLUMN, &id,
                            ACTIVE_COLUMN, &toggled,
                            -1);

        toggled ^= 1;
        if (toggled) {
                GvcMixerStream *stream;

                g_debug ("Default output selected: %u", id);
                stream = gvc_mixer_control_lookup_stream_id (dialog->priv->mixer_control, id);
                if (stream == NULL) {
                        g_warning ("Unable to find stream for id: %u", id);
                        return;
                }

                gvc_mixer_control_set_default_sink (dialog->priv->mixer_control, stream);
        }
}

static void
name_to_text (GtkTreeViewColumn *column,
              GtkCellRenderer *cell,
              GtkTreeModel *model,
              GtkTreeIter *iter,
              gpointer user_data)
{
        char *name, *mapping;

        gtk_tree_model_get(model, iter,
                           NAME_COLUMN, &name,
                           SPEAKERS_COLUMN, &mapping,
                           -1);

        if (mapping == NULL) {
                g_object_set (cell, "text", name, NULL);
        } else {
                char *str;

                str = g_strdup_printf ("%s\n<i>%s</i>",
                                       name, mapping);
                g_object_set (cell, "markup", str, NULL);
                g_free (str);
        }

        g_free (name);
        g_free (mapping);
}

static GtkWidget *
create_stream_treeview (GvcMixerDialog *dialog,
                        GCallback       on_toggled)
{
        GtkWidget         *treeview;
        GtkListStore      *store;
        GtkCellRenderer   *renderer;
        GtkTreeViewColumn *column;

        treeview = gtk_tree_view_new ();
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

        store = gtk_list_store_new (NUM_COLUMNS,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_BOOLEAN,
                                    G_TYPE_UINT,
                                    G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (store));

        renderer = gtk_cell_renderer_toggle_new ();
        gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer),
                                            TRUE);
        column = gtk_tree_view_column_new_with_attributes (NULL,
                                                           renderer,
                                                           "active", ACTIVE_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        g_signal_connect (renderer,
                          "toggled",
                          G_CALLBACK (on_toggled),
                          dialog);

        gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (treeview), -1,
                                                    _("Name"), gtk_cell_renderer_text_new (),
                                                    name_to_text, NULL, NULL);

#if 0
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Device"),
                                                           renderer,
                                                           "text", DEVICE_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
#endif
        return treeview;
}

static void
on_profile_changed (GvcComboBox *widget,
                    const char  *profile,
                    gpointer     user_data)
{
        GvcMixerCard        *card;

        card = g_object_get_data (G_OBJECT (widget), "card");
        if (card == NULL) {
                g_warning ("Could not find card for combobox");
                return;
        }

        g_debug ("Profile changed to %s for card %s", profile,
                 gvc_mixer_card_get_name (card));

        gvc_mixer_card_change_profile (card, profile);
}

static void
on_test_speakers_clicked (GvcComboBox *widget,
                          gpointer     user_data)
{
        GvcMixerDialog      *dialog = GVC_MIXER_DIALOG (user_data);
        GvcMixerCard        *card;
        GvcMixerCardProfile *profile;
        GtkWidget           *d, *speaker_test, *container;
        char                *title;

        card = g_object_get_data (G_OBJECT (widget), "card");
        if (card == NULL) {
                g_warning ("Could not find card for combobox");
                return;
        }
        profile = gvc_mixer_card_get_profile (card);

        g_debug ("XXX Start speaker testing for profile '%s', card %s XXX",
                 profile->profile, gvc_mixer_card_get_name (card));

        title = g_strdup_printf (_("Speaker Testing for %s"), gvc_mixer_card_get_name (card));
        d = gtk_dialog_new_with_buttons (title,
                                         GTK_WINDOW (dialog),
                                         GTK_DIALOG_MODAL |
#if !GTK_CHECK_VERSION (2, 21, 8)
                                         GTK_DIALOG_NO_SEPARATOR |
#endif
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         NULL);
        g_free (title);
        speaker_test = gvc_speaker_test_new (dialog->priv->mixer_control,
                                             card);
        gtk_widget_show (speaker_test);
        container = gtk_dialog_get_content_area (GTK_DIALOG (d));
        gtk_container_add (GTK_CONTAINER (container), speaker_test);

        gtk_dialog_run (GTK_DIALOG (d));
        gtk_widget_destroy (d);
}

static void
on_card_selection_changed (GtkTreeSelection *selection,
                           gpointer          user_data)
{
        GvcMixerDialog      *dialog = GVC_MIXER_DIALOG (user_data);
        GtkTreeModel        *model;
        GtkTreeIter          iter;
        const GList         *profiles;
        guint                id;
        GvcMixerCard        *card;
        GvcMixerCardProfile *current_profile;

        g_debug ("Card selection changed");

        if (dialog->priv->hw_profile_combo != NULL) {
                gtk_container_remove (GTK_CONTAINER (dialog->priv->hw_settings_box),
                                      dialog->priv->hw_profile_combo);
                dialog->priv->hw_profile_combo = NULL;
        }

        if (gtk_tree_selection_get_selected (selection,
                                             NULL,
                                             &iter) == FALSE) {
                return;
        }

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->hw_treeview));
        gtk_tree_model_get (model, &iter,
                            HW_ID_COLUMN, &id,
                            -1);
        card = gvc_mixer_control_lookup_card_id (dialog->priv->mixer_control, id);
        if (card == NULL) {
                g_warning ("Unable to find card for id: %u", id);
                return;
        }

        current_profile = gvc_mixer_card_get_profile (card);
        profiles = gvc_mixer_card_get_profiles (card);
        dialog->priv->hw_profile_combo = gvc_combo_box_new (_("_Profile:"));
        g_object_set (G_OBJECT (dialog->priv->hw_profile_combo), "button-label", _("Test Speakers"), NULL);
        gvc_combo_box_set_profiles (GVC_COMBO_BOX (dialog->priv->hw_profile_combo), profiles);
        gvc_combo_box_set_active (GVC_COMBO_BOX (dialog->priv->hw_profile_combo), current_profile->profile);

        gtk_box_pack_start (GTK_BOX (dialog->priv->hw_settings_box),
                            dialog->priv->hw_profile_combo,
                            TRUE, TRUE, 6);
        g_object_set (G_OBJECT (dialog->priv->hw_profile_combo),
                      "show-button", current_profile->n_sinks == 1,
                      NULL);
        gtk_widget_show (dialog->priv->hw_profile_combo);

        g_object_set_data (G_OBJECT (dialog->priv->hw_profile_combo), "card", card);
        g_signal_connect (G_OBJECT (dialog->priv->hw_profile_combo), "changed",
                          G_CALLBACK (on_profile_changed), dialog);
        g_signal_connect (G_OBJECT (dialog->priv->hw_profile_combo), "button-clicked",
                          G_CALLBACK (on_test_speakers_clicked), dialog);
}

static void
card_to_text (GtkTreeViewColumn *column,
              GtkCellRenderer *cell,
              GtkTreeModel *model,
              GtkTreeIter *iter,
              gpointer user_data)
{
        char *name, *status, *profile, *str;
        gboolean sensitive;

        gtk_tree_model_get(model, iter,
                           HW_NAME_COLUMN, &name,
                           HW_STATUS_COLUMN, &status,
                           HW_PROFILE_HUMAN_COLUMN, &profile,
                           HW_SENSITIVE_COLUMN, &sensitive,
                           -1);

        str = g_strdup_printf ("%s\n<i>%s</i>\n<i>%s</i>",
                               name, status, profile);
        g_object_set (cell,
                      "markup", str,
                      "sensitive", sensitive,
                      NULL);
        g_free (str);

        g_free (name);
        g_free (status);
        g_free (profile);
}

static GtkWidget *
create_cards_treeview (GvcMixerDialog *dialog,
                       GCallback       on_changed)
{
        GtkWidget         *treeview;
        GtkListStore      *store;
        GtkCellRenderer   *renderer;
        GtkTreeViewColumn *column;
        GtkTreeSelection  *selection;

        treeview = gtk_tree_view_new ();
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_signal_connect (G_OBJECT (selection), "changed",
                          on_changed, dialog);

        store = gtk_list_store_new (HW_NUM_COLUMNS,
                                    G_TYPE_UINT,
                                    G_TYPE_ICON,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_BOOLEAN);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (store));

        renderer = gtk_cell_renderer_pixbuf_new ();
        g_object_set (G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_DIALOG, NULL);
        column = gtk_tree_view_column_new_with_attributes (NULL,
                                                           renderer,
                                                           "gicon", HW_ICON_COLUMN,
                                                           "sensitive", HW_SENSITIVE_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (treeview), -1,
                                                    _("Name"), gtk_cell_renderer_text_new (),
                                                    card_to_text, NULL, NULL);

        return treeview;
}

static const guint tab_accel_keys[] = {
        GDK_1, GDK_2, GDK_3, GDK_4, GDK_5
};

static void
dialog_accel_cb (GtkAccelGroup    *accelgroup,
                 GObject          *object,
                 guint             key,
                 GdkModifierType   mod,
                 GvcMixerDialog   *self)
{
        gint num = -1;
        gint i;

        for (i = 0; i < G_N_ELEMENTS (tab_accel_keys); i++) {
                if (tab_accel_keys[i] == key) {
                        num = i;
                        break;
                }
        }

        if (num != -1) {
                gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), num);
        }
}

static GObject *
gvc_mixer_dialog_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_params)
{
        GObject          *object;
        GvcMixerDialog   *self;
        GtkWidget        *main_vbox;
        GtkWidget        *label;
        GtkWidget        *alignment;
        GtkWidget        *box;
        GtkWidget        *sbox;
        GtkWidget        *ebox;
        GSList           *streams;
        GSList           *cards;
        GSList           *l;
        GvcMixerStream   *stream;
        GvcMixerCard     *card;
        GtkTreeSelection *selection;
        GtkAccelGroup    *accel_group;
        GClosure         *closure;
        gint             i;

        object = G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_MIXER_DIALOG (object);
        gtk_dialog_add_button (GTK_DIALOG (self), "gtk-close", GTK_RESPONSE_OK);

        main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
        gtk_box_set_spacing (GTK_BOX (main_vbox), 2);

        gtk_container_set_border_width (GTK_CONTAINER (self), 6);

        self->priv->output_stream_box = gtk_hbox_new (FALSE, 12);
        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 0, 0, 0);
        gtk_container_add (GTK_CONTAINER (alignment), self->priv->output_stream_box);
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            alignment,
                            FALSE, FALSE, 0);
        self->priv->output_bar = create_bar (self, self->priv->size_group, TRUE);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->output_bar),
                                  _("_Output volume: "));
        gtk_widget_set_sensitive (self->priv->output_bar, FALSE);
        gtk_box_pack_start (GTK_BOX (self->priv->output_stream_box),
                            self->priv->output_bar, TRUE, TRUE, 12);

        self->priv->notebook = gtk_notebook_new ();
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            self->priv->notebook,
                            TRUE, TRUE, 0);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->notebook), 5);

        /* Set up accels (borrowed from Empathy) */
        accel_group = gtk_accel_group_new ();
        gtk_window_add_accel_group (GTK_WINDOW (self), accel_group);

        for (i = 0; i < G_N_ELEMENTS (tab_accel_keys); i++) {
                closure =  g_cclosure_new (G_CALLBACK (dialog_accel_cb),
                                           self,
                                           NULL);
                gtk_accel_group_connect (accel_group,
                                         tab_accel_keys[i],
                                         GDK_MOD1_MASK,
                                         0,
                                         closure);
        }

        g_object_unref (accel_group);

        /* Effects page */
        self->priv->sound_effects_box = gtk_vbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->sound_effects_box), 12);
        label = gtk_label_new (_("Sound Effects"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->sound_effects_box,
                                  label);

        self->priv->effects_bar = create_bar (self, self->priv->size_group, TRUE);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->effects_bar),
                                  _("_Alert volume: "));
        gtk_widget_set_sensitive (self->priv->effects_bar, FALSE);
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            self->priv->effects_bar, FALSE, FALSE, 0);

        self->priv->sound_theme_chooser = gvc_sound_theme_chooser_new ();
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            self->priv->sound_theme_chooser,
                            TRUE, TRUE, 6);

        /* Hardware page */
        self->priv->hw_box = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->hw_box), 12);
        label = gtk_label_new (_("Hardware"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->hw_box,
                                  label);

        box = gtk_frame_new (_("C_hoose a device to configure:"));
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->hw_box), box, TRUE, TRUE, 0);

        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_container_add (GTK_CONTAINER (box), alignment);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 0, 0);

        self->priv->hw_treeview = create_cards_treeview (self,
                                                         G_CALLBACK (on_card_selection_changed));
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), self->priv->hw_treeview);

        box = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (box), self->priv->hw_treeview);
        gtk_container_add (GTK_CONTAINER (alignment), box);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->hw_treeview));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

        box = gtk_frame_new (_("Settings for the selected device:"));
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->hw_box), box, FALSE, TRUE, 12);
        self->priv->hw_settings_box = gtk_vbox_new (FALSE, 12);
        gtk_container_add (GTK_CONTAINER (box), self->priv->hw_settings_box);

        /* Input page */
        self->priv->input_box = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->input_box), 12);
        label = gtk_label_new (_("Input"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->input_box,
                                  label);

        self->priv->input_bar = create_bar (self, self->priv->size_group, TRUE);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->input_bar),
                                  _("_Input volume: "));
        gvc_channel_bar_set_low_icon_name (GVC_CHANNEL_BAR (self->priv->input_bar),
                                           "audio-input-microphone-low");
        gvc_channel_bar_set_high_icon_name (GVC_CHANNEL_BAR (self->priv->input_bar),
                                            "audio-input-microphone-high");
        gtk_widget_set_sensitive (self->priv->input_bar, FALSE);
        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 0, 0);
        gtk_container_add (GTK_CONTAINER (alignment), self->priv->input_bar);
        gtk_box_pack_start (GTK_BOX (self->priv->input_box),
                            alignment,
                            FALSE, FALSE, 0);

        box = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (self->priv->input_box),
                            box,
                            FALSE, FALSE, 6);

        sbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (box),
                            sbox,
                            FALSE, FALSE, 0);

        label = gtk_label_new (_("Input level:"));
        gtk_box_pack_start (GTK_BOX (sbox),
                            label,
                            FALSE, FALSE, 0);
        gtk_size_group_add_widget (self->priv->size_group, sbox);

        self->priv->input_level_bar = gvc_level_bar_new ();
        gvc_level_bar_set_orientation (GVC_LEVEL_BAR (self->priv->input_level_bar),
                                       GTK_ORIENTATION_HORIZONTAL);
        gvc_level_bar_set_scale (GVC_LEVEL_BAR (self->priv->input_level_bar),
                                 GVC_LEVEL_SCALE_LINEAR);
        gtk_box_pack_start (GTK_BOX (box),
                            self->priv->input_level_bar,
                            TRUE, TRUE, 6);

        ebox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (box),
                            ebox,
                            FALSE, FALSE, 0);
        gtk_size_group_add_widget (self->priv->size_group, ebox);

        self->priv->input_settings_box = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (self->priv->input_box),
                            self->priv->input_settings_box,
                            FALSE, FALSE, 0);

        box = gtk_frame_new (_("C_hoose a device for sound input:"));
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->input_box), box, TRUE, TRUE, 0);

        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_container_add (GTK_CONTAINER (box), alignment);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 0, 0);

        self->priv->input_treeview = create_stream_treeview (self,
                                                             G_CALLBACK (on_input_radio_toggled));
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), self->priv->input_treeview);

        box = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (box), self->priv->input_treeview);
        gtk_container_add (GTK_CONTAINER (alignment), box);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->input_treeview));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

        /* Output page */
        self->priv->output_box = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->output_box), 12);
        label = gtk_label_new (_("Output"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->output_box,
                                  label);

        box = gtk_frame_new (_("C_hoose a device for sound output:"));
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->output_box), box, TRUE, TRUE, 0);

        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_container_add (GTK_CONTAINER (box), alignment);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 0, 0);

        self->priv->output_treeview = create_stream_treeview (self,
                                                              G_CALLBACK (on_output_radio_toggled));
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), self->priv->output_treeview);

        box = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (box), self->priv->output_treeview);
        gtk_container_add (GTK_CONTAINER (alignment), box);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->output_treeview));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

        box = gtk_frame_new (_("Settings for the selected device:"));
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->output_box), box, FALSE, FALSE, 12);
        self->priv->output_settings_box = gtk_vbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (box), self->priv->output_settings_box);

        /* Applications */
        self->priv->applications_box = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->applications_box), 12);
        label = gtk_label_new (_("Applications"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->applications_box,
                                  label);
        self->priv->no_apps_label = gtk_label_new (_("No application is currently playing or recording audio."));
        gtk_box_pack_start (GTK_BOX (self->priv->applications_box),
                            self->priv->no_apps_label,
                            TRUE, TRUE, 0);

        g_signal_connect (self->priv->mixer_control,
                          "stream-added",
                          G_CALLBACK (on_control_stream_added),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "stream-removed",
                          G_CALLBACK (on_control_stream_removed),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "card-added",
                          G_CALLBACK (on_control_card_added),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "card-removed",
                          G_CALLBACK (on_control_card_removed),
                          self);

        gtk_widget_show_all (main_vbox);

        streams = gvc_mixer_control_get_streams (self->priv->mixer_control);
        for (l = streams; l != NULL; l = l->next) {
                stream = l->data;
                add_stream (self, stream);
        }
        g_slist_free (streams);

        cards = gvc_mixer_control_get_cards (self->priv->mixer_control);
        for (l = cards; l != NULL; l = l->next) {
                card = l->data;
                add_card (self, card);
        }
        g_slist_free (cards);

        return object;
}

static void
gvc_mixer_dialog_dispose (GObject *object)
{
        GvcMixerDialog *dialog = GVC_MIXER_DIALOG (object);

        if (dialog->priv->mixer_control != NULL) {
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_stream_added,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_stream_removed,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_card_added,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_card_removed,
                                                      dialog);

                g_object_unref (dialog->priv->mixer_control);
                dialog->priv->mixer_control = NULL;
        }

        if (dialog->priv->bars != NULL) {
                g_hash_table_destroy (dialog->priv->bars);
                dialog->priv->bars = NULL;
        }

        G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->dispose (object);
}

static void
gvc_mixer_dialog_class_init (GvcMixerDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_mixer_dialog_constructor;
        object_class->dispose = gvc_mixer_dialog_dispose;
        object_class->finalize = gvc_mixer_dialog_finalize;
        object_class->set_property = gvc_mixer_dialog_set_property;
        object_class->get_property = gvc_mixer_dialog_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MIXER_CONTROL,
                                         g_param_spec_object ("mixer-control",
                                                              "mixer control",
                                                              "mixer control",
                                                              GVC_TYPE_MIXER_CONTROL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GvcMixerDialogPrivate));
}


static void
gvc_mixer_dialog_init (GvcMixerDialog *dialog)
{
        dialog->priv = GVC_MIXER_DIALOG_GET_PRIVATE (dialog);
        dialog->priv->bars = g_hash_table_new (NULL, NULL);
        dialog->priv->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
        dialog->priv->apps_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
}

static void
gvc_mixer_dialog_finalize (GObject *object)
{
        GvcMixerDialog *mixer_dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_MIXER_DIALOG (object));

        mixer_dialog = GVC_MIXER_DIALOG (object);

        g_return_if_fail (mixer_dialog->priv != NULL);
        G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->finalize (object);
}

GvcMixerDialog *
gvc_mixer_dialog_new (GvcMixerControl *control)
{
        GObject *dialog;
        dialog = g_object_new (GVC_TYPE_MIXER_DIALOG,
                               "icon-name", "multimedia-volume-control",
                               "title", _("Sound Preferences"),
                               "has-separator", FALSE,
                               "mixer-control", control,
                               NULL);
        return GVC_MIXER_DIALOG (dialog);
}

enum {
        PAGE_EVENTS,
        PAGE_HARDWARE,
        PAGE_INPUT,
        PAGE_OUTPUT,
        PAGE_APPLICATIONS
};

gboolean
gvc_mixer_dialog_set_page (GvcMixerDialog *self,
                           const char     *page)
{
        guint num;

        g_return_val_if_fail (self != NULL, FALSE);

        if (page == NULL)
                num = 0;
        else if (g_str_equal (page, "effects"))
                num = PAGE_EVENTS;
        else if (g_str_equal (page, "hardware"))
                num = PAGE_HARDWARE;
        else if (g_str_equal (page, "input"))
                num = PAGE_INPUT;
        else if (g_str_equal (page, "output"))
                num = PAGE_OUTPUT;
        else if (g_str_equal (page, "applications"))
                num = PAGE_APPLICATIONS;
        else
                num = 0;

        gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), num);

        return TRUE;
}
