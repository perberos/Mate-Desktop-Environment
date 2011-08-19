/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#define _GNU_SOURCE

#include "config.h"
#include <glib/gi18n-lib.h>

#include <math.h>

#include "gdu-size-widget.h"

typedef enum {
        GDU_UNIT_NOT_SET,
        GDU_UNIT_KB,
        GDU_UNIT_KIB,
        GDU_UNIT_MB,
        GDU_UNIT_MIB,
        GDU_UNIT_GB,
        GDU_UNIT_GIB,
        GDU_UNIT_TB,
        GDU_UNIT_TIB,
} GduUnit;

static gdouble
gdu_unit_get_factor (GduUnit unit)
{
        gdouble ret;

        switch (unit) {
        case GDU_UNIT_KB:
                ret = 1000.0;
                break;
        default:
                g_warning ("Unknown unit %d", unit);
                /* explicit fallthrough */
        case GDU_UNIT_MB:
                ret = 1000.0 * 1000.0;
                break;
        case GDU_UNIT_GB:
                ret = 1000.0 * 1000.0 * 1000.0;
                break;
        case GDU_UNIT_TB:
                ret = 1000.0 * 1000.0 * 1000.0 * 1000.0;
                break;
        case GDU_UNIT_KIB:
                ret = 1024.0;
                break;
        case GDU_UNIT_MIB:
                ret = 1024.0 * 1024.0;
                break;
        case GDU_UNIT_GIB:
                ret = 1024.0 * 1024.0 * 1024.0;
                break;
        case GDU_UNIT_TIB:
                ret = 1024.0 * 1024.0 * 1024.0 * 1024.0;
                break;
        }

        return ret;
}

static const gchar *
gdu_unit_get_name (GduUnit unit)
{
        const gchar *ret;

        switch (unit) {
        case GDU_UNIT_KB:
                ret = _("KB");
                break;
        default:
                g_warning ("Unknown unit %d", unit);
                /* explicit fallthrough */
        case GDU_UNIT_MB:
                ret = _("MB");
                break;
        case GDU_UNIT_GB:
                ret = _("GB");
                break;
        case GDU_UNIT_TB:
                ret = _("TB");
                break;
        case GDU_UNIT_KIB:
                ret = _("KiB");
                break;
        case GDU_UNIT_MIB:
                ret = _("MiB");
                break;
        case GDU_UNIT_GIB:
                ret = _("GiB");
                break;
        case GDU_UNIT_TIB:
                ret = _("TiB");
                break;
        }

        return ret;
}

static GduUnit
gdu_unit_guess (const gchar *str)
{
        GduUnit ret;

        ret = GDU_UNIT_NOT_SET;

        if (strstr (str, "KB") != NULL) {
                ret = GDU_UNIT_KB;
        } else if (strstr (str, "MB") != NULL) {
                ret = GDU_UNIT_MB;
        } else if (strstr (str, "GB") != NULL) {
                ret = GDU_UNIT_GB;
        } else if (strstr (str, "TB") != NULL) {
                ret = GDU_UNIT_TB;
        } else if (strstr (str, "KiB") != NULL) {
                ret = GDU_UNIT_KIB;
        } else if (strstr (str, "MiB") != NULL) {
                ret = GDU_UNIT_MIB;
        } else if (strstr (str, "GiB") != NULL) {
                ret = GDU_UNIT_GIB;
        } else if (strstr (str, "TiB") != NULL) {
                ret = GDU_UNIT_TIB;
        }

        return ret;
}

struct GduSizeWidgetPrivate
{
        guint64 size;
        guint64 min_size;
        guint64 max_size;
        GtkWidget *hscale;
        GtkWidget *spin_button;

        GduUnit unit;
};

enum
{
        PROP_0,
        PROP_SIZE,
        PROP_MIN_SIZE,
        PROP_MAX_SIZE,
};

enum
{
        CHANGED_SIGNAL,
        LAST_SIGNAL,
};

guint signals[LAST_SIGNAL] = {0,};

G_DEFINE_TYPE (GduSizeWidget, gdu_size_widget, GTK_TYPE_HBOX)

static void update_stepping (GduSizeWidget *widget);

static void
gdu_size_widget_finalize (GObject *object)
{
        //GduSizeWidget *widget = GDU_SIZE_WIDGET (object);

        if (G_OBJECT_CLASS (gdu_size_widget_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_size_widget_parent_class)->finalize (object);
}

static void
gdu_size_widget_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (object);

        switch (property_id) {
        case PROP_SIZE:
                g_value_set_uint64 (value, widget->priv->size);
                break;

        case PROP_MIN_SIZE:
                g_value_set_uint64 (value, widget->priv->min_size);
                break;

        case PROP_MAX_SIZE:
                g_value_set_uint64 (value, widget->priv->max_size);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_size_widget_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (object);

        switch (property_id) {
        case PROP_SIZE:
                gdu_size_widget_set_size (widget, g_value_get_uint64 (value));
                break;

        case PROP_MIN_SIZE:
                gdu_size_widget_set_min_size (widget, g_value_get_uint64 (value));
                break;

        case PROP_MAX_SIZE:
                gdu_size_widget_set_max_size (widget, g_value_get_uint64 (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static gchar *
on_hscale_format_value (GtkScale *scale,
                        gdouble   value,
                        gpointer  user_data)
{
        gchar *ret;

        ret = gdu_util_get_size_for_display ((guint64 ) value, FALSE, FALSE);

        return ret;
}

static void
on_hscale_value_changed (GtkRange  *range,
                         gpointer   user_data)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (user_data);
        guint64 old_size;

        old_size = widget->priv->size;
        widget->priv->size = (guint64) gtk_range_get_value (range);

        if (old_size != widget->priv->size) {

                g_signal_emit (widget,
                               signals[CHANGED_SIGNAL],
                               0);
                g_object_notify (G_OBJECT (widget), "size");
        }
}

static void
on_spin_button_value_changed (GtkSpinButton *spin_button,
                              gpointer   user_data)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (user_data);
        guint64 old_size;

        old_size = widget->priv->size;
        widget->priv->size = (guint64) gtk_spin_button_get_value (spin_button);

        if (old_size != widget->priv->size) {
                g_signal_emit (widget,
                               signals[CHANGED_SIGNAL],
                               0);
                g_object_notify (G_OBJECT (widget), "size");
        }
}

static gboolean
on_query_tooltip (GtkWidget  *w,
                  gint        x,
                  gint        y,
                  gboolean    keyboard_mode,
                  GtkTooltip *tooltip,
                  gpointer    user_data)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (w);
        gchar *s;
        gchar *s1;

        s1 = gdu_util_get_size_for_display (widget->priv->size, FALSE, TRUE);
        /* TODO: handle this use-case
        s = g_strdup_printf ("<b>%s</b>\n"
                             "\n"
                             "%s",
                             s1,
                             _("Right click to specify an exact size."));*/
        s = g_strdup_printf ("%s", s1);
        g_free (s1);
        gtk_tooltip_set_markup (tooltip, s);
        g_free (s);

        return TRUE;
}

static gboolean
on_spin_button_output (GtkSpinButton *spin_button,
                       gpointer       user_data)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (user_data);
        gdouble unit_factor;
        const gchar *unit_name;
        gchar *s;

        unit_factor = gdu_unit_get_factor (widget->priv->unit);
        unit_name = gdu_unit_get_name (widget->priv->unit);

        s = g_strdup_printf ("%.3f %s",
                             widget->priv->size / unit_factor,
                             unit_name);

        gtk_entry_set_text (GTK_ENTRY (widget->priv->spin_button), s);

        g_free (s);

        return TRUE;
}

static void
set_unit (GduSizeWidget *widget,
          GduUnit        unit)
{
        widget->priv->unit = unit;

        update_stepping (widget);
}

static gint
on_spin_button_input (GtkSpinButton *spin_button,
                      gdouble       *out_new_val,
                      gpointer       user_data)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (user_data);
        const gchar *entry_txt;
        gdouble val;
        GduUnit unit;
        gint ret;

        g_assert (out_new_val != NULL);
        ret = GTK_INPUT_ERROR;

        entry_txt = gtk_entry_get_text (GTK_ENTRY (spin_button));
        if (sscanf (entry_txt, "%lf", &val) == 1) {
                unit = gdu_unit_guess (entry_txt);
                if (unit != GDU_UNIT_NOT_SET) {
                        set_unit (widget, unit);
                }

                *out_new_val = val * gdu_unit_get_factor (widget->priv->unit);
                ret = TRUE;
        }

        return ret;
}

static void
gdu_size_widget_constructed (GObject *object)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (object);

        gtk_widget_show (widget->priv->hscale);
        gtk_box_pack_start (GTK_BOX (widget),
                            widget->priv->hscale,
                            TRUE,
                            TRUE,
                            0);

        g_signal_connect (widget->priv->hscale,
                          "format-value",
                          G_CALLBACK (on_hscale_format_value),
                          widget);
        g_signal_connect (widget->priv->hscale,
                          "value-changed",
                          G_CALLBACK (on_hscale_value_changed),
                          widget);

        gtk_widget_set_has_tooltip (GTK_WIDGET (widget),
                                    TRUE);
        g_signal_connect (widget,
                          "query-tooltip",
                          G_CALLBACK (on_query_tooltip),
                          widget);

        /* ---------------------------------------------------------------------------------------------------- */

        gtk_widget_show (widget->priv->spin_button);
        gtk_box_pack_start (GTK_BOX (widget),
                            widget->priv->spin_button,
                            FALSE,
                            FALSE,
                            0);
        g_signal_connect (widget->priv->spin_button,
                          "output",
                          G_CALLBACK (on_spin_button_output),
                          widget);
        g_signal_connect (widget->priv->spin_button,
                          "input",
                          G_CALLBACK (on_spin_button_input),
                          widget);
        g_signal_connect (widget->priv->spin_button,
                          "value-changed",
                          G_CALLBACK (on_spin_button_value_changed),
                          widget);

        /* ---------------------------------------------------------------------------------------------------- */

        update_stepping (widget);

        if (G_OBJECT_CLASS (gdu_size_widget_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_size_widget_parent_class)->constructed (object);
}

static void
gdu_size_widget_init (GduSizeWidget *widget)
{
        GtkAdjustment *adjustment;

        widget->priv = G_TYPE_INSTANCE_GET_PRIVATE (widget,
                                                    GDU_TYPE_SIZE_WIDGET,
                                                    GduSizeWidgetPrivate);

        widget->priv->hscale = gtk_hscale_new_with_range (0,
                                                          10,
                                                          1);
        gtk_scale_set_draw_value (GTK_SCALE (widget->priv->hscale), TRUE);

        adjustment = gtk_range_get_adjustment (GTK_RANGE (widget->priv->hscale));
        widget->priv->spin_button = gtk_spin_button_new (adjustment,
                                                         0,
                                                         0);
        gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (widget->priv->spin_button),
                                           GTK_UPDATE_IF_VALID);

        set_unit (widget, GDU_UNIT_GB);
}

static gboolean
gdu_size_widget_mnemonic_activate (GtkWidget *_widget,
                                   gboolean   group_cycling)
{
        GduSizeWidget *widget = GDU_SIZE_WIDGET (_widget);
        return gtk_widget_mnemonic_activate (widget->priv->hscale, group_cycling);
}

static void
gdu_size_widget_class_init (GduSizeWidgetClass *klass)
{
        GObjectClass *gobject_class;
        GtkWidgetClass *widget_class;

        gobject_class = G_OBJECT_CLASS (klass);
        widget_class = GTK_WIDGET_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduSizeWidgetPrivate));

        gobject_class->get_property        = gdu_size_widget_get_property;
        gobject_class->set_property        = gdu_size_widget_set_property;
        gobject_class->constructed         = gdu_size_widget_constructed;
        gobject_class->finalize            = gdu_size_widget_finalize;

        widget_class->mnemonic_activate    = gdu_size_widget_mnemonic_activate;

        g_object_class_install_property (gobject_class,
                                         PROP_SIZE,
                                         g_param_spec_uint64 ("size",
                                                              _("Size"),
                                                              _("The currently selected size"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (gobject_class,
                                         PROP_MIN_SIZE,
                                         g_param_spec_uint64 ("min-size",
                                                              _("Minimum Size"),
                                                              _("The minimum size that can be selected"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (gobject_class,
                                         PROP_MAX_SIZE,
                                         g_param_spec_uint64 ("max-size",
                                                              _("Maximum Size"),
                                                              _("The maximum size that can be selected"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        signals[CHANGED_SIGNAL] = g_signal_new ("changed",
                                                G_TYPE_FROM_CLASS (klass),
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (GduSizeWidgetClass, changed),
                                                NULL,
                                                NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE,
                                                0);
}

GtkWidget *
gdu_size_widget_new (guint64 size,
                     guint64 min_size,
                     guint64 max_size)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_SIZE_WIDGET,
                                         "size", size,
                                         "min-size", min_size,
                                         "max-size", max_size,
                                         NULL));
}

static void
update_stepping (GduSizeWidget *widget)
{
        gdouble extent;

        extent = widget->priv->max_size - widget->priv->min_size;

        /* set steps in hscale according to magnitude of extent und so weiter */
        if (extent > 0) {
                gdouble increment;
                gdouble shown_extent;
                gdouble unit_factor;

                unit_factor = gdu_unit_get_factor (widget->priv->unit);

                shown_extent = extent / unit_factor;

                increment = (exp10 (floor (log10 (shown_extent))) / 10.0) * unit_factor;

                gtk_range_set_increments (GTK_RANGE (widget->priv->hscale),
                                          increment,
                                          increment * 10.0);
        }


        /* add markers at 0, 25%, 50%, 75% and 100% */
        gtk_scale_clear_marks (GTK_SCALE (widget->priv->hscale));
        gtk_scale_add_mark (GTK_SCALE (widget->priv->hscale),
                            widget->priv->min_size,
                            GTK_POS_BOTTOM,
                            NULL);
        gtk_scale_add_mark (GTK_SCALE (widget->priv->hscale),
                            widget->priv->min_size + extent * 0.25,
                            GTK_POS_BOTTOM,
                            NULL);
        gtk_scale_add_mark (GTK_SCALE (widget->priv->hscale),
                            widget->priv->min_size + extent * 0.50,
                            GTK_POS_BOTTOM,
                            NULL);
        gtk_scale_add_mark (GTK_SCALE (widget->priv->hscale),
                            widget->priv->min_size + extent * 0.75,
                            GTK_POS_BOTTOM,
                            NULL);
        gtk_scale_add_mark (GTK_SCALE (widget->priv->hscale),
                            widget->priv->min_size + extent * 1.0,
                            GTK_POS_BOTTOM,
                            NULL);
}

void
gdu_size_widget_set_size (GduSizeWidget *widget,
                          guint64        size)
{
        g_return_if_fail (GDU_IS_SIZE_WIDGET (widget));
        if (widget->priv->size != size) {
                gtk_range_set_value (GTK_RANGE (widget->priv->hscale),
                                     size);
        }
}

void
gdu_size_widget_set_min_size (GduSizeWidget *widget,
                              guint64        min_size)
{
        g_return_if_fail (GDU_IS_SIZE_WIDGET (widget));
        if (widget->priv->min_size != min_size) {
                widget->priv->min_size = min_size;
                gtk_range_set_range (GTK_RANGE (widget->priv->hscale),
                                     widget->priv->min_size,
                                     widget->priv->max_size);
                update_stepping (widget);
                g_signal_emit (widget,
                               signals[CHANGED_SIGNAL],
                               0);
                g_object_notify (G_OBJECT (widget), "min-size");
        }
}

void
gdu_size_widget_set_max_size (GduSizeWidget *widget,
                              guint64        max_size)
{
        g_return_if_fail (GDU_IS_SIZE_WIDGET (widget));
        if (widget->priv->max_size != max_size) {
                widget->priv->max_size = max_size;
                gtk_range_set_range (GTK_RANGE (widget->priv->hscale),
                                     widget->priv->min_size,
                                     widget->priv->max_size);
                update_stepping (widget);
                g_signal_emit (widget,
                               signals[CHANGED_SIGNAL],
                               0);
                g_object_notify (G_OBJECT (widget), "max-size");
        }
}

guint64
gdu_size_widget_get_size     (GduSizeWidget *widget)
{
        g_return_val_if_fail (GDU_IS_SIZE_WIDGET (widget), 0);
        return widget->priv->size;
}

guint64
gdu_size_widget_get_min_size (GduSizeWidget *widget)
{
        g_return_val_if_fail (GDU_IS_SIZE_WIDGET (widget), 0);
        return widget->priv->min_size;
}

guint64
gdu_size_widget_get_max_size (GduSizeWidget *widget)
{
        g_return_val_if_fail (GDU_IS_SIZE_WIDGET (widget), 0);
        return widget->priv->max_size;
}

