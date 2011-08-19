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
#include <canberra-gtk.h>

#include "gvc-balance-bar.h"

#define SCALE_SIZE 128
#define ADJUSTMENT_MAX_NORMAL 65536.0 /* PA_VOLUME_NORM */

#define GVC_BALANCE_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_BALANCE_BAR, GvcBalanceBarPrivate))

struct GvcBalanceBarPrivate
{
        GvcChannelMap *channel_map;
        GvcBalanceType btype;
        GtkWidget     *scale_box;
        GtkWidget     *start_box;
        GtkWidget     *end_box;
        GtkWidget     *label;
        GtkWidget     *scale;
        GtkAdjustment *adjustment;
        GtkSizeGroup  *size_group;
        gboolean       symmetric;
        gboolean       click_lock;
};

enum
{
        PROP_0,
        PROP_CHANNEL_MAP,
        PROP_BALANCE_TYPE,
};

static void     gvc_balance_bar_class_init (GvcBalanceBarClass *klass);
static void     gvc_balance_bar_init       (GvcBalanceBar      *balance_bar);
static void     gvc_balance_bar_finalize   (GObject            *object);

static gboolean on_scale_button_press_event   (GtkWidget      *widget,
                                               GdkEventButton *event,
                                               GvcBalanceBar  *bar);
static gboolean on_scale_button_release_event (GtkWidget      *widget,
                                               GdkEventButton *event,
                                               GvcBalanceBar  *bar);
static gboolean on_scale_scroll_event         (GtkWidget      *widget,
                                               GdkEventScroll *event,
                                               GvcBalanceBar  *bar);
static void on_adjustment_value_changed       (GtkAdjustment *adjustment,
                                               GvcBalanceBar *bar);

G_DEFINE_TYPE (GvcBalanceBar, gvc_balance_bar, GTK_TYPE_HBOX)

static GtkWidget *
_scale_box_new (GvcBalanceBar *bar)
{
        GvcBalanceBarPrivate *priv = bar->priv;
        GtkWidget            *box;
        GtkWidget            *sbox;
        GtkWidget            *ebox;
        GtkAdjustment        *adjustment = bar->priv->adjustment;
        char                 *str_lower, *str_upper;
        gdouble              lower, upper;

        bar->priv->scale_box = box = gtk_hbox_new (FALSE, 6);
        priv->scale = gtk_hscale_new (priv->adjustment);
        gtk_widget_set_size_request (priv->scale, SCALE_SIZE, -1);

        gtk_widget_set_name (priv->scale, "balance-bar-scale");
        gtk_rc_parse_string ("style \"balance-bar-scale-style\" {\n"
                             " GtkScale::trough-side-details = 0\n"
                             "}\n"
                             "widget \"*.balance-bar-scale\" style : rc \"balance-bar-scale-style\"\n");

        bar->priv->start_box = sbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (box), sbox, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (sbox), priv->label, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (box), priv->scale, TRUE, TRUE, 0);

        switch (bar->priv->btype) {
        case BALANCE_TYPE_RL:
                str_lower = g_strdup_printf ("<small>%s</small>", C_("balance", "Left"));
                str_upper = g_strdup_printf ("<small>%s</small>", C_("balance", "Right"));
                break;
        case BALANCE_TYPE_FR:
                str_lower = g_strdup_printf ("<small>%s</small>", C_("balance", "Rear"));
                str_upper = g_strdup_printf ("<small>%s</small>", C_("balance", "Front"));
                break;
        case BALANCE_TYPE_LFE:
                str_lower = g_strdup_printf ("<small>%s</small>", C_("balance", "Minimum"));
                str_upper = g_strdup_printf ("<small>%s</small>", C_("balance", "Maximum"));
                break;
        default:
                g_assert_not_reached ();
        }

        lower = gtk_adjustment_get_lower (adjustment);
        gtk_scale_add_mark (GTK_SCALE (priv->scale), lower,
                            GTK_POS_BOTTOM, str_lower);
        g_free (str_lower);
        upper = gtk_adjustment_get_upper (adjustment);
        gtk_scale_add_mark (GTK_SCALE (priv->scale), upper,
                            GTK_POS_BOTTOM, str_upper);
        g_free (str_upper);

        if (bar->priv->btype != BALANCE_TYPE_LFE) {
                gtk_scale_add_mark (GTK_SCALE (priv->scale),
                                    (upper - lower)/2 + lower,
                                    GTK_POS_BOTTOM, NULL);
        }

        bar->priv->end_box = ebox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (box), ebox, FALSE, FALSE, 0);

        gtk_range_set_update_policy (GTK_RANGE (priv->scale), GTK_UPDATE_CONTINUOUS);
        ca_gtk_widget_disable_sounds (bar->priv->scale, FALSE);
        gtk_widget_add_events (bar->priv->scale, GDK_SCROLL_MASK);

        g_signal_connect (G_OBJECT (bar->priv->scale), "button-press-event",
                          G_CALLBACK (on_scale_button_press_event), bar);
        g_signal_connect (G_OBJECT (bar->priv->scale), "button-release-event",
                          G_CALLBACK (on_scale_button_release_event), bar);
        g_signal_connect (G_OBJECT (bar->priv->scale), "scroll-event",
                          G_CALLBACK (on_scale_scroll_event), bar);

        if (bar->priv->size_group != NULL) {
                gtk_size_group_add_widget (bar->priv->size_group, sbox);

                if (bar->priv->symmetric) {
                        gtk_size_group_add_widget (bar->priv->size_group, ebox);
                }
        }

        gtk_scale_set_draw_value (GTK_SCALE (priv->scale), FALSE);

        return box;
}

void
gvc_balance_bar_set_size_group (GvcBalanceBar *bar,
                                GtkSizeGroup  *group,
                                gboolean       symmetric)
{
        g_return_if_fail (GVC_IS_BALANCE_BAR (bar));

        bar->priv->size_group = group;
        bar->priv->symmetric = symmetric;

        if (bar->priv->size_group != NULL) {
                gtk_size_group_add_widget (bar->priv->size_group,
                                           bar->priv->start_box);

                if (bar->priv->symmetric) {
                        gtk_size_group_add_widget (bar->priv->size_group,
                                                   bar->priv->end_box);
                }
        }
        gtk_widget_queue_draw (GTK_WIDGET (bar));
}

static const char *
btype_to_string (guint btype)
{
        switch (btype) {
        case BALANCE_TYPE_RL:
                return "Balance";
        case BALANCE_TYPE_FR:
                return "Fade";
                break;
        case BALANCE_TYPE_LFE:
                return "LFE";
        default:
                g_assert_not_reached ();
        }
        return NULL;
}

static void
update_level_from_map (GvcBalanceBar *bar,
                       GvcChannelMap *map)
{
        const gdouble *volumes;
        gdouble val;

        g_debug ("Volume changed (for %s bar)", btype_to_string (bar->priv->btype));

        volumes = gvc_channel_map_get_volume (map);
        switch (bar->priv->btype) {
        case BALANCE_TYPE_RL:
                val = volumes[BALANCE];
                break;
        case BALANCE_TYPE_FR:
                val = volumes[FADE];
                break;
        case BALANCE_TYPE_LFE:
                val = volumes[LFE];
                break;
        default:
                g_assert_not_reached ();
        }

        gtk_adjustment_set_value (bar->priv->adjustment, val);
}

static void
on_channel_map_volume_changed (GvcChannelMap  *map,
                               gboolean        set,
                               GvcBalanceBar  *bar)
{
        update_level_from_map (bar, map);
}

static void
gvc_balance_bar_set_channel_map (GvcBalanceBar *bar,
                                 GvcChannelMap *map)
{
        g_return_if_fail (GVC_BALANCE_BAR (bar));

        if (bar->priv->channel_map != NULL) {
                g_signal_handlers_disconnect_by_func (G_OBJECT (bar->priv->channel_map),
                                                      on_channel_map_volume_changed, bar);
                g_object_unref (bar->priv->channel_map);
        }
        bar->priv->channel_map = g_object_ref (map);

        update_level_from_map (bar, map);

        g_signal_connect (G_OBJECT (map), "volume-changed",
                          G_CALLBACK (on_channel_map_volume_changed), bar);

        g_object_notify (G_OBJECT (bar), "channel-map");
}

static void
gvc_balance_bar_set_balance_type (GvcBalanceBar *bar,
                                  GvcBalanceType btype)
{
        GtkWidget *frame;

        g_return_if_fail (GVC_BALANCE_BAR (bar));

        bar->priv->btype = btype;
        if (bar->priv->btype != BALANCE_TYPE_LFE) {
                bar->priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                            -1.0,
                                                                            1.0,
                                                                            0.5,
                                                                            0.5,
                                                                            0.0));
        } else {
                bar->priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                            0.0,
                                                                            ADJUSTMENT_MAX_NORMAL,
                                                                            ADJUSTMENT_MAX_NORMAL/100.0,
                                                                            ADJUSTMENT_MAX_NORMAL/10.0,
                                                                            0.0));
        }

        g_object_ref_sink (bar->priv->adjustment);
        g_signal_connect (bar->priv->adjustment,
                          "value-changed",
                          G_CALLBACK (on_adjustment_value_changed),
                          bar);

        switch (btype) {
        case BALANCE_TYPE_RL:
                bar->priv->label = gtk_label_new_with_mnemonic (_("_Balance:"));
                break;
        case BALANCE_TYPE_FR:
                bar->priv->label = gtk_label_new_with_mnemonic (_("_Fade:"));
                break;
        case BALANCE_TYPE_LFE:
                bar->priv->label = gtk_label_new_with_mnemonic (_("_Subwoofer:"));
                break;
        default:
                g_assert_not_reached ();
        }
        gtk_misc_set_alignment (GTK_MISC (bar->priv->label),
                                0.0,
                                0.0);
        /* frame */
        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
        gtk_container_add (GTK_CONTAINER (bar), frame);

        /* box with scale */
        bar->priv->scale_box = _scale_box_new (bar);
        gtk_container_add (GTK_CONTAINER (frame), bar->priv->scale_box);
        gtk_widget_show_all (frame);

        gtk_widget_set_direction (bar->priv->scale, GTK_TEXT_DIR_LTR);
        gtk_label_set_mnemonic_widget (GTK_LABEL (bar->priv->label),
                                       bar->priv->scale);

        g_object_notify (G_OBJECT (bar), "balance-type");
}

static void
gvc_balance_bar_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
        GvcBalanceBar *self = GVC_BALANCE_BAR (object);

        switch (prop_id) {
        case PROP_CHANNEL_MAP:
                gvc_balance_bar_set_channel_map (self, g_value_get_object (value));
                break;
        case PROP_BALANCE_TYPE:
                gvc_balance_bar_set_balance_type (self, g_value_get_int (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_balance_bar_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
        GvcBalanceBar *self = GVC_BALANCE_BAR (object);

        switch (prop_id) {
        case PROP_CHANNEL_MAP:
                g_value_set_object (value, self->priv->channel_map);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gvc_balance_bar_constructor (GType                  type,
                             guint                  n_construct_properties,
                             GObjectConstructParam *construct_params)
{
        return G_OBJECT_CLASS (gvc_balance_bar_parent_class)->constructor (type, n_construct_properties, construct_params);
}

static void
gvc_balance_bar_class_init (GvcBalanceBarClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_balance_bar_constructor;
        object_class->finalize = gvc_balance_bar_finalize;
        object_class->set_property = gvc_balance_bar_set_property;
        object_class->get_property = gvc_balance_bar_get_property;

        g_object_class_install_property (object_class,
                                         PROP_CHANNEL_MAP,
                                         g_param_spec_object ("channel-map",
                                                              "channel map",
                                                              "The channel map",
                                                              GVC_TYPE_CHANNEL_MAP,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_BALANCE_TYPE,
                                         g_param_spec_int ("balance-type",
                                                           "balance type",
                                                           "Whether the balance is right-left or front-rear",
                                                           BALANCE_TYPE_RL, NUM_BALANCE_TYPES - 1, BALANCE_TYPE_RL,
                                                           G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

        g_type_class_add_private (klass, sizeof (GvcBalanceBarPrivate));
}


static gboolean
on_scale_button_press_event (GtkWidget      *widget,
                             GdkEventButton *event,
                             GvcBalanceBar  *bar)
{
        bar->priv->click_lock = TRUE;

        return FALSE;
}

static gboolean
on_scale_button_release_event (GtkWidget      *widget,
                               GdkEventButton *event,
                               GvcBalanceBar  *bar)
{
        bar->priv->click_lock = FALSE;

        return FALSE;
}

static gboolean
on_scale_scroll_event (GtkWidget      *widget,
                       GdkEventScroll *event,
                       GvcBalanceBar  *bar)
{
        gdouble value;

        value = gtk_adjustment_get_value (bar->priv->adjustment);

        if (bar->priv->btype == BALANCE_TYPE_LFE) {
                if (event->direction == GDK_SCROLL_UP) {
                        if (value + ADJUSTMENT_MAX_NORMAL/100.0 > ADJUSTMENT_MAX_NORMAL)
                                value = ADJUSTMENT_MAX_NORMAL;
                        else
                                value = value + ADJUSTMENT_MAX_NORMAL/100.0;
                } else if (event->direction == GDK_SCROLL_DOWN) {
                        if (value - ADJUSTMENT_MAX_NORMAL/100.0 < 0)
                                value = 0.0;
                        else
                                value = value - ADJUSTMENT_MAX_NORMAL/100.0;
                }
        } else {
                if (event->direction == GDK_SCROLL_UP) {
                        if (value + 0.01 > 1.0)
                                value = 1.0;
                        else
                                value = value + 0.01;
                } else if (event->direction == GDK_SCROLL_DOWN) {
                        if (value - 0.01 < 0)
                                value = 0.0;
                        else
                                value = value - 0.01;
                }
        }
        gtk_adjustment_set_value (bar->priv->adjustment, value);

        return TRUE;
}

/* FIXME remove when we depend on a newer PA */
static pa_cvolume *
gvc_pa_cvolume_set_position (pa_cvolume *cv, const pa_channel_map *map, pa_channel_position_t t, pa_volume_t v) {
        unsigned c;
        gboolean good = FALSE;

        g_assert(cv);
        g_assert(map);

        g_return_val_if_fail(pa_cvolume_compatible_with_channel_map(cv, map), NULL);
        g_return_val_if_fail(t < PA_CHANNEL_POSITION_MAX, NULL);

        for (c = 0; c < map->channels; c++)
                if (map->map[c] == t) {
                        cv->values[c] = v;
                        good = TRUE;
                }

        return good ? cv : NULL;
}

static void
on_adjustment_value_changed (GtkAdjustment *adjustment,
                             GvcBalanceBar *bar)
{
        gdouble                val;
        pa_cvolume             cv;
        const pa_channel_map  *pa_map;

        if (bar->priv->channel_map == NULL)
                return;

        cv = *gvc_channel_map_get_cvolume (bar->priv->channel_map);
        val = gtk_adjustment_get_value (adjustment);

        pa_map = gvc_channel_map_get_pa_channel_map (bar->priv->channel_map);

        switch (bar->priv->btype) {
        case BALANCE_TYPE_RL:
                pa_cvolume_set_balance (&cv, pa_map, val);
                break;
        case BALANCE_TYPE_FR:
                pa_cvolume_set_fade (&cv, pa_map, val);
                break;
        case BALANCE_TYPE_LFE:
                gvc_pa_cvolume_set_position (&cv, pa_map, PA_CHANNEL_POSITION_LFE, val);
                break;
        }

        gvc_channel_map_volume_changed (bar->priv->channel_map, &cv, TRUE);
}

static void
gvc_balance_bar_init (GvcBalanceBar *bar)
{
        bar->priv = GVC_BALANCE_BAR_GET_PRIVATE (bar);
}

static void
gvc_balance_bar_finalize (GObject *object)
{
        GvcBalanceBar *bar;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_BALANCE_BAR (object));

        bar = GVC_BALANCE_BAR (object);

        g_return_if_fail (bar->priv != NULL);

        if (bar->priv->channel_map != NULL) {
                g_signal_handlers_disconnect_by_func (G_OBJECT (bar->priv->channel_map),
                                                      on_channel_map_volume_changed, bar);
                g_object_unref (bar->priv->channel_map);
        }

        G_OBJECT_CLASS (gvc_balance_bar_parent_class)->finalize (object);
}

GtkWidget *
gvc_balance_bar_new (const GvcChannelMap *channel_map, GvcBalanceType btype)
{
        GObject *bar;
        bar = g_object_new (GVC_TYPE_BALANCE_BAR,
                            "channel-map", channel_map,
                            "balance-type", btype,
                            NULL);
        return GTK_WIDGET (bar);
}
