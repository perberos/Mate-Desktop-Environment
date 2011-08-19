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

#include <pulse/pulseaudio.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <canberra-gtk.h>

#include "gvc-channel-bar.h"

#define SCALE_SIZE 128
#define ADJUSTMENT_MAX_NORMAL 65536.0 /* PA_VOLUME_NORM */
#define ADJUSTMENT_MAX_AMPLIFIED 98304.0 /* 1.5 * ADJUSTMENT_MAX_NORMAL */
#define ADJUSTMENT_MAX (bar->priv->is_amplified ? ADJUSTMENT_MAX_AMPLIFIED : ADJUSTMENT_MAX_NORMAL)
#define SCROLLSTEP (ADJUSTMENT_MAX / 100.0 * 5.0)

#define GVC_CHANNEL_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_CHANNEL_BAR, GvcChannelBarPrivate))

struct GvcChannelBarPrivate
{
        GtkOrientation orientation;
        GtkWidget     *scale_box;
        GtkWidget     *start_box;
        GtkWidget     *end_box;
        GtkWidget     *image;
        GtkWidget     *label;
        GtkWidget     *low_image;
        GtkWidget     *scale;
        GtkWidget     *high_image;
        GtkWidget     *mute_box;
        GtkWidget     *mute_button;
        GtkAdjustment *adjustment;
        GtkAdjustment *zero_adjustment;
        gboolean       show_mute;
        gboolean       is_muted;
        char          *name;
        char          *icon_name;
        char          *low_icon_name;
        char          *high_icon_name;
        GtkSizeGroup  *size_group;
        gboolean       symmetric;
        gboolean       click_lock;
        gboolean       is_amplified;
        guint32        base_volume;
};

enum
{
        PROP_0,
        PROP_ORIENTATION,
        PROP_SHOW_MUTE,
        PROP_IS_MUTED,
        PROP_ADJUSTMENT,
        PROP_NAME,
        PROP_ICON_NAME,
        PROP_LOW_ICON_NAME,
        PROP_HIGH_ICON_NAME,
        PROP_IS_AMPLIFIED,
};

static void     gvc_channel_bar_class_init    (GvcChannelBarClass *klass);
static void     gvc_channel_bar_init          (GvcChannelBar      *channel_bar);
static void     gvc_channel_bar_finalize      (GObject            *object);

static gboolean on_scale_button_press_event   (GtkWidget      *widget,
                                               GdkEventButton *event,
                                               GvcChannelBar  *bar);
static gboolean on_scale_button_release_event (GtkWidget      *widget,
                                               GdkEventButton *event,
                                               GvcChannelBar  *bar);
static gboolean on_scale_scroll_event         (GtkWidget      *widget,
                                               GdkEventScroll *event,
                                               GvcChannelBar  *bar);

G_DEFINE_TYPE (GvcChannelBar, gvc_channel_bar, GTK_TYPE_HBOX)

static GtkWidget *
_scale_box_new (GvcChannelBar *bar)
{
        GvcChannelBarPrivate *priv = bar->priv;
        GtkWidget            *box;
        GtkWidget            *sbox;
        GtkWidget            *ebox;

        if (priv->orientation == GTK_ORIENTATION_VERTICAL) {
                bar->priv->scale_box = box = gtk_vbox_new (FALSE, 6);

                priv->scale = gtk_vscale_new (priv->adjustment);

                gtk_widget_set_size_request (priv->scale, -1, SCALE_SIZE);
                gtk_range_set_inverted (GTK_RANGE (priv->scale), TRUE);

                bar->priv->start_box = sbox = gtk_vbox_new (FALSE, 6);
                gtk_box_pack_start (GTK_BOX (box), sbox, FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (sbox), priv->image, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (sbox), priv->label, FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (sbox), priv->high_image, FALSE, FALSE, 0);
                gtk_widget_hide (priv->high_image);
                gtk_box_pack_start (GTK_BOX (box), priv->scale, TRUE, TRUE, 0);

                bar->priv->end_box = ebox = gtk_vbox_new (FALSE, 6);
                gtk_box_pack_start (GTK_BOX (box), ebox, FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (ebox), priv->low_image, FALSE, FALSE, 0);
                gtk_widget_hide (priv->low_image);

                gtk_box_pack_start (GTK_BOX (ebox), priv->mute_box, FALSE, FALSE, 0);
        } else {
                bar->priv->scale_box = box = gtk_hbox_new (FALSE, 6);

                priv->scale = gtk_hscale_new (priv->adjustment);

                gtk_widget_set_size_request (priv->scale, SCALE_SIZE, -1);

                bar->priv->start_box = sbox = gtk_hbox_new (FALSE, 6);
                gtk_box_pack_start (GTK_BOX (box), sbox, FALSE, FALSE, 0);

                gtk_box_pack_end (GTK_BOX (sbox), priv->low_image, FALSE, FALSE, 0);
                gtk_widget_show (priv->low_image);

                gtk_box_pack_start (GTK_BOX (sbox), priv->image, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (sbox), priv->label, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (box), priv->scale, TRUE, TRUE, 0);

                bar->priv->end_box = ebox = gtk_hbox_new (FALSE, 6);
                gtk_box_pack_start (GTK_BOX (box), ebox, FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (ebox), priv->high_image, FALSE, FALSE, 0);
                gtk_widget_show (priv->high_image);
                gtk_box_pack_start (GTK_BOX (ebox), priv->mute_box, FALSE, FALSE, 0);
        }

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

static void
update_image (GvcChannelBar *bar)
{
        gtk_image_set_from_icon_name (GTK_IMAGE (bar->priv->image),
                                      bar->priv->icon_name,
                                      GTK_ICON_SIZE_DIALOG);

        if (bar->priv->icon_name != NULL) {
                gtk_widget_show (bar->priv->image);
        } else {
                gtk_widget_hide (bar->priv->image);
        }
}

static void
update_label (GvcChannelBar *bar)
{
        if (bar->priv->name != NULL) {
                gtk_label_set_text_with_mnemonic (GTK_LABEL (bar->priv->label),
                                                  bar->priv->name);
                gtk_label_set_mnemonic_widget (GTK_LABEL (bar->priv->label),
                                               bar->priv->scale);
                gtk_widget_show (bar->priv->label);
        } else {
                gtk_label_set_text (GTK_LABEL (bar->priv->label), NULL);
                gtk_widget_hide (bar->priv->label);
        }
}

static void
update_layout (GvcChannelBar *bar)
{
        GtkWidget *box;
        GtkWidget *frame;

        if (bar->priv->scale == NULL) {
                return;
        }

        box = bar->priv->scale_box;
        frame = gtk_widget_get_parent (box);

        g_object_ref (bar->priv->image);
        g_object_ref (bar->priv->label);
        g_object_ref (bar->priv->mute_box);
        g_object_ref (bar->priv->low_image);
        g_object_ref (bar->priv->high_image);

        gtk_container_remove (GTK_CONTAINER (bar->priv->start_box), bar->priv->image);
        gtk_container_remove (GTK_CONTAINER (bar->priv->start_box), bar->priv->label);
        gtk_container_remove (GTK_CONTAINER (bar->priv->end_box), bar->priv->mute_box);

        if (bar->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                gtk_container_remove (GTK_CONTAINER (bar->priv->start_box), bar->priv->low_image);
                gtk_container_remove (GTK_CONTAINER (bar->priv->end_box), bar->priv->high_image);
        } else {
                gtk_container_remove (GTK_CONTAINER (bar->priv->end_box), bar->priv->low_image);
                gtk_container_remove (GTK_CONTAINER (bar->priv->start_box), bar->priv->high_image);
        }

        gtk_container_remove (GTK_CONTAINER (box), bar->priv->start_box);
        gtk_container_remove (GTK_CONTAINER (box), bar->priv->scale);
        gtk_container_remove (GTK_CONTAINER (box), bar->priv->end_box);
        gtk_container_remove (GTK_CONTAINER (frame), box);

        bar->priv->scale_box = _scale_box_new (bar);
        gtk_container_add (GTK_CONTAINER (frame), bar->priv->scale_box);

        g_object_unref (bar->priv->image);
        g_object_unref (bar->priv->label);
        g_object_unref (bar->priv->mute_box);
        g_object_unref (bar->priv->low_image);
        g_object_unref (bar->priv->high_image);

        gtk_widget_show_all (frame);
}

void
gvc_channel_bar_set_size_group (GvcChannelBar *bar,
                                GtkSizeGroup  *group,
                                gboolean       symmetric)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

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

void
gvc_channel_bar_set_name (GvcChannelBar  *bar,
                          const char     *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        g_free (bar->priv->name);
        bar->priv->name = g_strdup (name);
        update_label (bar);
        g_object_notify (G_OBJECT (bar), "name");
}

void
gvc_channel_bar_set_icon_name (GvcChannelBar  *bar,
                               const char     *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        g_free (bar->priv->icon_name);
        bar->priv->icon_name = g_strdup (name);
        update_image (bar);
        g_object_notify (G_OBJECT (bar), "icon-name");
}

void
gvc_channel_bar_set_low_icon_name   (GvcChannelBar *bar,
                                     const char    *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (name != NULL && strcmp (bar->priv->low_icon_name, name) != 0) {
                g_free (bar->priv->low_icon_name);
                bar->priv->low_icon_name = g_strdup (name);
                gtk_image_set_from_icon_name (GTK_IMAGE (bar->priv->low_image),
                                              bar->priv->low_icon_name,
                                              GTK_ICON_SIZE_BUTTON);
                g_object_notify (G_OBJECT (bar), "low-icon-name");
        }
}

void
gvc_channel_bar_set_high_icon_name  (GvcChannelBar *bar,
                                     const char    *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (name != NULL && strcmp (bar->priv->high_icon_name, name) != 0) {
                g_free (bar->priv->high_icon_name);
                bar->priv->high_icon_name = g_strdup (name);
                gtk_image_set_from_icon_name (GTK_IMAGE (bar->priv->high_image),
                                              bar->priv->high_icon_name,
                                              GTK_ICON_SIZE_BUTTON);
                g_object_notify (G_OBJECT (bar), "high-icon-name");
        }
}

void
gvc_channel_bar_set_orientation (GvcChannelBar  *bar,
                                 GtkOrientation  orientation)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (orientation != bar->priv->orientation) {
                bar->priv->orientation = orientation;
                update_layout (bar);
                g_object_notify (G_OBJECT (bar), "orientation");
        }
}

static void
gvc_channel_bar_set_adjustment (GvcChannelBar *bar,
                                GtkAdjustment *adjustment)
{
        g_return_if_fail (GVC_CHANNEL_BAR (bar));
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

        if (bar->priv->adjustment != NULL) {
                g_object_unref (bar->priv->adjustment);
        }
        bar->priv->adjustment = g_object_ref_sink (adjustment);

        if (bar->priv->scale != NULL) {
                gtk_range_set_adjustment (GTK_RANGE (bar->priv->scale), adjustment);
        }

        g_object_notify (G_OBJECT (bar), "adjustment");
}

GtkAdjustment *
gvc_channel_bar_get_adjustment (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), NULL);

        return bar->priv->adjustment;
}

static gboolean
on_scale_button_press_event (GtkWidget      *widget,
                             GdkEventButton *event,
                             GvcChannelBar  *bar)
{
        /* HACK: we want the behaviour you get with the middle button, so we
         * mangle the event.  clicking with other buttons moves the slider in
         * step increments, clicking with the middle button moves the slider to
         * the location of the click.
         */
        if (event->button == 1)
                event->button = 2;

        bar->priv->click_lock = TRUE;

        return FALSE;
}

static gboolean
on_scale_button_release_event (GtkWidget      *widget,
                               GdkEventButton *event,
                               GvcChannelBar  *bar)
{
        GtkAdjustment *adj;
        gdouble value;

        /* HACK: see on_scale_button_press_event() */
        if (event->button == 1)
                event->button = 2;

        bar->priv->click_lock = FALSE;

        adj = gtk_range_get_adjustment (GTK_RANGE (widget));

        value = gtk_adjustment_get_value (adj);

        /* this means the adjustment moved away from zero and
         * therefore we should unmute and set the volume. */
        gvc_channel_bar_set_is_muted (bar, (value == 0.0));

        /* Play a sound! */
        ca_gtk_play_for_widget (GTK_WIDGET (bar), 0,
                                CA_PROP_EVENT_ID, "audio-volume-change",
                                CA_PROP_EVENT_DESCRIPTION, "foobar event happened",
                                CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
                                NULL);

        return FALSE;
}

gboolean
gvc_channel_bar_scroll (GvcChannelBar *bar, GdkScrollDirection direction)
{
        GtkAdjustment *adj;
        gdouble value;

        g_return_val_if_fail (bar != NULL, FALSE);
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);

        if (bar->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                if (direction != GDK_SCROLL_UP && direction != GDK_SCROLL_DOWN)
                        return FALSE;
        } else {
                /* Switch direction for RTL */
                if (gtk_widget_get_direction (GTK_WIDGET (bar)) == GTK_TEXT_DIR_RTL) {
                        if (direction == GDK_SCROLL_RIGHT)
                                direction = GDK_SCROLL_LEFT;
                        else if (direction == GDK_SCROLL_LEFT)
                                direction = GDK_SCROLL_RIGHT;
                }
                /* Switch side scroll to vertical */
                if (direction == GDK_SCROLL_RIGHT)
                        direction = GDK_SCROLL_UP;
                else if (GDK_SCROLL_LEFT)
                        direction = GDK_SCROLL_DOWN;
        }

        adj = gtk_range_get_adjustment (GTK_RANGE (bar->priv->scale));
        if (adj == bar->priv->zero_adjustment) {
                if (direction == GDK_SCROLL_UP)
                        gvc_channel_bar_set_is_muted (bar, FALSE);
                return TRUE;
        }

        value = gtk_adjustment_get_value (adj);

        if (direction == GDK_SCROLL_UP) {
                if (value + SCROLLSTEP > ADJUSTMENT_MAX)
                        value = ADJUSTMENT_MAX;
                else
                        value = value + SCROLLSTEP;
        } else if (direction == GDK_SCROLL_DOWN) {
                if (value - SCROLLSTEP < 0)
                        value = 0.0;
                else
                        value = value - SCROLLSTEP;
        }

        gvc_channel_bar_set_is_muted (bar, (value == 0.0));
        adj = gtk_range_get_adjustment (GTK_RANGE (bar->priv->scale));
        gtk_adjustment_set_value (adj, value);

        return TRUE;
}

static gboolean
on_scale_scroll_event (GtkWidget      *widget,
                       GdkEventScroll *event,
                       GvcChannelBar  *bar)
{
        return gvc_channel_bar_scroll (bar, event->direction);
}

static void
on_zero_adjustment_value_changed (GtkAdjustment *adjustment,
                                  GvcChannelBar *bar)
{
        gdouble value;

        if (bar->priv->click_lock != FALSE) {
                return;
        }

        value = gtk_adjustment_get_value (bar->priv->zero_adjustment);
        gtk_adjustment_set_value (bar->priv->adjustment, value);


        if (bar->priv->show_mute == FALSE) {
                /* this means the adjustment moved away from zero and
                 * therefore we should unmute and set the volume. */
                gvc_channel_bar_set_is_muted (bar, value > 0.0);
        }
}

static void
update_mute_button (GvcChannelBar *bar)
{
        if (bar->priv->show_mute) {
                gtk_widget_show (bar->priv->mute_button);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bar->priv->mute_button),
                                              bar->priv->is_muted);
        } else {
                gtk_widget_hide (bar->priv->mute_button);

                if (bar->priv->is_muted) {
                        /* If we aren't showing the mute button then
                         * move slider to the zero.  But we don't want to
                         * change the adjustment.  */
                        g_signal_handlers_block_by_func (bar->priv->zero_adjustment,
                                                         on_zero_adjustment_value_changed,
                                                         bar);
                        gtk_adjustment_set_value (bar->priv->zero_adjustment, 0);
                        g_signal_handlers_unblock_by_func (bar->priv->zero_adjustment,
                                                           on_zero_adjustment_value_changed,
                                                           bar);
                        gtk_range_set_adjustment (GTK_RANGE (bar->priv->scale),
                                                  bar->priv->zero_adjustment);
                } else {
                        /* no longer muted so restore the original adjustment
                         * and tell the front-end that the value changed */
                        gtk_range_set_adjustment (GTK_RANGE (bar->priv->scale),
                                                  bar->priv->adjustment);
                        gtk_adjustment_value_changed (bar->priv->adjustment);
                }
        }
}

void
gvc_channel_bar_set_is_muted (GvcChannelBar *bar,
                              gboolean       is_muted)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (is_muted != bar->priv->is_muted) {
                /* Update our internal state before telling the
                 * front-end about our changes */
                bar->priv->is_muted = is_muted;
                update_mute_button (bar);
                g_object_notify (G_OBJECT (bar), "is-muted");
        }
}

gboolean
gvc_channel_bar_get_is_muted  (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);
        return bar->priv->is_muted;
}

void
gvc_channel_bar_set_show_mute (GvcChannelBar *bar,
                               gboolean       show_mute)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (show_mute != bar->priv->show_mute) {
                bar->priv->show_mute = show_mute;
                g_object_notify (G_OBJECT (bar), "show-mute");
                update_mute_button (bar);
        }
}

gboolean
gvc_channel_bar_get_show_mute (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);
        return bar->priv->show_mute;
}

void
gvc_channel_bar_set_is_amplified (GvcChannelBar *bar, gboolean amplified)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        bar->priv->is_amplified = amplified;
        gtk_adjustment_set_upper (bar->priv->adjustment, ADJUSTMENT_MAX);
        gtk_adjustment_set_upper (bar->priv->zero_adjustment, ADJUSTMENT_MAX);
        gtk_scale_clear_marks (GTK_SCALE (bar->priv->scale));

        if (amplified) {
                char *str;

                if (bar->priv->base_volume == ADJUSTMENT_MAX_NORMAL) {
                        str = g_strdup_printf ("<small>%s</small>", C_("volume", "100%"));
                        gtk_scale_add_mark (GTK_SCALE (bar->priv->scale), ADJUSTMENT_MAX_NORMAL,
                                            GTK_POS_BOTTOM, str);
                } else {
                        str = g_strdup_printf ("<small>%s</small>", C_("volume", "Unamplified"));
                        gtk_scale_add_mark (GTK_SCALE (bar->priv->scale), bar->priv->base_volume,
                                            GTK_POS_BOTTOM, str);
                        /* Only show 100% if it's higher than the base volume */
                        if (bar->priv->base_volume < ADJUSTMENT_MAX_NORMAL) {
                                str = g_strdup_printf ("<small>%s</small>", C_("volume", "100%"));
                                gtk_scale_add_mark (GTK_SCALE (bar->priv->scale), ADJUSTMENT_MAX_NORMAL,
                                                    GTK_POS_BOTTOM, str);
                        }
                }

                g_free (str);
                gtk_alignment_set (GTK_ALIGNMENT (bar->priv->mute_box), 0.5, 0, 0, 0);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->low_image), 0.5, 0);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->high_image), 0.5, 0);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->label), 0, 0);
        } else {
                gtk_alignment_set (GTK_ALIGNMENT (bar->priv->mute_box), 0.5, 0.5, 0, 0);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->low_image), 0.5, 0.5);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->high_image), 0.5, 0.5);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->label), 0, 0.5);
        }
}

void
gvc_channel_bar_set_base_volume (GvcChannelBar *bar,
                                 pa_volume_t    base_volume)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (base_volume == 0) {
                bar->priv->base_volume = ADJUSTMENT_MAX_NORMAL;
                return;
        }

        /* Note that you need to call _is_amplified() afterwards to update the marks */
        bar->priv->base_volume = base_volume;
}

static void
gvc_channel_bar_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
        GvcChannelBar *self = GVC_CHANNEL_BAR (object);

        switch (prop_id) {
        case PROP_ORIENTATION:
                gvc_channel_bar_set_orientation (self, g_value_get_enum (value));
                break;
        case PROP_IS_MUTED:
                gvc_channel_bar_set_is_muted (self, g_value_get_boolean (value));
                break;
        case PROP_SHOW_MUTE:
                gvc_channel_bar_set_show_mute (self, g_value_get_boolean (value));
                break;
        case PROP_NAME:
                gvc_channel_bar_set_name (self, g_value_get_string (value));
                break;
        case PROP_ICON_NAME:
                gvc_channel_bar_set_icon_name (self, g_value_get_string (value));
                break;
        case PROP_LOW_ICON_NAME:
                gvc_channel_bar_set_low_icon_name (self, g_value_get_string (value));
                break;
        case PROP_HIGH_ICON_NAME:
                gvc_channel_bar_set_high_icon_name (self, g_value_get_string (value));
                break;
        case PROP_ADJUSTMENT:
                gvc_channel_bar_set_adjustment (self, g_value_get_object (value));
                break;
        case PROP_IS_AMPLIFIED:
                gvc_channel_bar_set_is_amplified (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_channel_bar_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
        GvcChannelBar *self = GVC_CHANNEL_BAR (object);
        GvcChannelBarPrivate *priv = self->priv;

        switch (prop_id) {
        case PROP_ORIENTATION:
                g_value_set_enum (value, priv->orientation);
                break;
        case PROP_IS_MUTED:
                g_value_set_boolean (value, priv->is_muted);
                break;
        case PROP_SHOW_MUTE:
                g_value_set_boolean (value, priv->show_mute);
                break;
        case PROP_NAME:
                g_value_set_string (value, priv->name);
                break;
        case PROP_ICON_NAME:
                g_value_set_string (value, priv->icon_name);
                break;
        case PROP_LOW_ICON_NAME:
                g_value_set_string (value, priv->low_icon_name);
                break;
        case PROP_HIGH_ICON_NAME:
                g_value_set_string (value, priv->high_icon_name);
                break;
        case PROP_ADJUSTMENT:
                g_value_set_object (value, gvc_channel_bar_get_adjustment (self));
                break;
        case PROP_IS_AMPLIFIED:
                g_value_set_boolean (value, priv->is_amplified);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gvc_channel_bar_constructor (GType                  type,
                             guint                  n_construct_properties,
                             GObjectConstructParam *construct_params)
{
        GObject       *object;
        GvcChannelBar *self;

        object = G_OBJECT_CLASS (gvc_channel_bar_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_CHANNEL_BAR (object);

        update_mute_button (self);

        return object;
}

static void
gvc_channel_bar_class_init (GvcChannelBarClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_channel_bar_constructor;
        object_class->finalize = gvc_channel_bar_finalize;
        object_class->set_property = gvc_channel_bar_set_property;
        object_class->get_property = gvc_channel_bar_get_property;

        g_object_class_install_property (object_class,
                                         PROP_ORIENTATION,
                                         g_param_spec_enum ("orientation",
                                                            "Orientation",
                                                            "The orientation of the scale",
                                                            GTK_TYPE_ORIENTATION,
                                                            GTK_ORIENTATION_VERTICAL,
                                                            G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_IS_MUTED,
                                         g_param_spec_boolean ("is-muted",
                                                               "is muted",
                                                               "Whether stream is muted",
                                                               FALSE,
                                                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_SHOW_MUTE,
                                         g_param_spec_boolean ("show-mute",
                                                               "show mute",
                                                               "Whether stream is muted",
                                                               FALSE,
                                                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_object_class_install_property (object_class,
                                         PROP_ADJUSTMENT,
                                         g_param_spec_object ("adjustment",
                                                              "Adjustment",
                                                              "The GtkAdjustment that contains the current value of this scale button object",
                                                              GTK_TYPE_ADJUSTMENT,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_NAME,
                                         g_param_spec_string ("name",
                                                              "Name",
                                                              "Name to display for this stream",
                                                              NULL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_ICON_NAME,
                                         g_param_spec_string ("icon-name",
                                                              "Icon Name",
                                                              "Name of icon to display for this stream",
                                                              NULL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_LOW_ICON_NAME,
                                         g_param_spec_string ("low-icon-name",
                                                              "Icon Name",
                                                              "Name of icon to display for this stream",
                                                              "audio-volume-low",
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_HIGH_ICON_NAME,
                                         g_param_spec_string ("high-icon-name",
                                                              "Icon Name",
                                                              "Name of icon to display for this stream",
                                                              "audio-volume-high",
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_IS_AMPLIFIED,
                                         g_param_spec_boolean ("is-amplified",
                                                               "is amplified",
                                                               "Whether the stream is digitally amplified",
                                                               FALSE,
                                                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GvcChannelBarPrivate));
}

static void
on_mute_button_toggled (GtkToggleButton *button,
                        GvcChannelBar   *bar)
{
        gboolean is_muted;
        is_muted = gtk_toggle_button_get_active (button);
        gvc_channel_bar_set_is_muted (bar, is_muted);
}

static void
gvc_channel_bar_init (GvcChannelBar *bar)
{
        GtkWidget *frame;

        bar->priv = GVC_CHANNEL_BAR_GET_PRIVATE (bar);

        bar->priv->base_volume = ADJUSTMENT_MAX_NORMAL;
        bar->priv->low_icon_name = g_strdup ("audio-volume-low");
        bar->priv->high_icon_name = g_strdup ("audio-volume-high");

        bar->priv->orientation = GTK_ORIENTATION_VERTICAL;
        bar->priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                    0.0,
                                                                    ADJUSTMENT_MAX_NORMAL,
                                                                    ADJUSTMENT_MAX_NORMAL/100.0,
                                                                    ADJUSTMENT_MAX_NORMAL/10.0,
                                                                    0.0));
        g_object_ref_sink (bar->priv->adjustment);

        bar->priv->zero_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                         0.0,
                                                                         ADJUSTMENT_MAX_NORMAL,
                                                                         ADJUSTMENT_MAX_NORMAL/100.0,
                                                                         ADJUSTMENT_MAX_NORMAL/10.0,
                                                                         0.0));
        g_object_ref_sink (bar->priv->zero_adjustment);

        g_signal_connect (bar->priv->zero_adjustment,
                          "value-changed",
                          G_CALLBACK (on_zero_adjustment_value_changed),
                          bar);

        bar->priv->mute_button = gtk_check_button_new_with_label (_("Mute"));
        gtk_widget_set_no_show_all (bar->priv->mute_button, TRUE);
        g_signal_connect (bar->priv->mute_button,
                          "toggled",
                          G_CALLBACK (on_mute_button_toggled),
                          bar);
        bar->priv->mute_box = gtk_alignment_new (0.5, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (bar->priv->mute_box), bar->priv->mute_button);

        bar->priv->low_image = gtk_image_new_from_icon_name ("audio-volume-low",
                                                             GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_no_show_all (bar->priv->low_image, TRUE);
        bar->priv->high_image = gtk_image_new_from_icon_name ("audio-volume-high",
                                                              GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_no_show_all (bar->priv->high_image, TRUE);

        bar->priv->image = gtk_image_new ();
        gtk_widget_set_no_show_all (bar->priv->image, TRUE);

        bar->priv->label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (bar->priv->label), 0.0, 0.5);
        gtk_widget_set_no_show_all (bar->priv->label, TRUE);

        /* frame */
        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
        gtk_container_add (GTK_CONTAINER (bar), frame);
        gtk_widget_show_all (frame);

        /* box with scale */
        bar->priv->scale_box = _scale_box_new (bar);

        gtk_container_add (GTK_CONTAINER (frame), bar->priv->scale_box);
}

static void
gvc_channel_bar_finalize (GObject *object)
{
        GvcChannelBar *channel_bar;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_CHANNEL_BAR (object));

        channel_bar = GVC_CHANNEL_BAR (object);

        g_return_if_fail (channel_bar->priv != NULL);

        g_free (channel_bar->priv->name);
        g_free (channel_bar->priv->icon_name);
        g_free (channel_bar->priv->low_icon_name);
        g_free (channel_bar->priv->high_icon_name);

        G_OBJECT_CLASS (gvc_channel_bar_parent_class)->finalize (object);
}

GtkWidget *
gvc_channel_bar_new (void)
{
        GObject *bar;
        bar = g_object_new (GVC_TYPE_CHANNEL_BAR,
                            NULL);
        return GTK_WIDGET (bar);
}
