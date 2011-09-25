/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 * Written by: William Jon McCann <mccann@jhu.edu>
 *             Ray Strode <rstrode@redhat.com>
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "mdm-clock-widget.h"

#define MDM_CLOCK_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_CLOCK_WIDGET, MdmClockWidgetPrivate))

struct MdmClockWidgetPrivate
{
        GtkWidget *label;
        char      *time_format;
        char      *tooltip_format;
        guint      update_clock_id;
        guint      should_show_seconds : 1;
        guint      should_show_date : 1;
};

static void     mdm_clock_widget_class_init  (MdmClockWidgetClass *klass);
static void     mdm_clock_widget_init        (MdmClockWidget      *clock_widget);
static void     mdm_clock_widget_finalize    (GObject             *object);
static gboolean update_timeout_cb            (MdmClockWidget      *clock);

G_DEFINE_TYPE (MdmClockWidget, mdm_clock_widget, GTK_TYPE_ALIGNMENT)

static void
update_time_format (MdmClockWidget *clock)
{
        char       *clock_format;
        char       *tooltip_format;

        if (clock->priv->should_show_date && clock->priv->should_show_seconds) {
                /* translators: This is the time format to use when both
                 * the date and time with seconds are being shown together.
                 */
                clock_format = _("%a %b %e, %l:%M:%S %p");
                tooltip_format = NULL;
        } else if (clock->priv->should_show_date && !clock->priv->should_show_seconds) {
                /* translators: This is the time format to use when both
                 * the date and time without seconds are being shown together.
                 */
                clock_format = _("%a %b %e, %l:%M %p");

                tooltip_format = NULL;
        } else if (!clock->priv->should_show_date && clock->priv->should_show_seconds) {
                /* translators: This is the time format to use when there is
                 * no date, just weekday and time with seconds.
                 */
                clock_format = _("%a %l:%M:%S %p");

                /* translators: This is the time format to use for the date
                 */
                tooltip_format = "%x";
        } else {
                /* translators: This is the time format to use when there is
                 * no date, just weekday and time without seconds.
                 */
                clock_format = _("%a %l:%M %p");

                tooltip_format = "%x";
        }

        g_free (clock->priv->time_format);
        clock->priv->time_format = g_locale_from_utf8 (clock_format, -1, NULL, NULL, NULL);

        g_free (clock->priv->tooltip_format);

        if (tooltip_format != NULL) {
                clock->priv->tooltip_format = g_locale_from_utf8 (tooltip_format, -1, NULL, NULL, NULL);
        } else {
                clock->priv->tooltip_format = NULL;
        }
}

static void
update_clock (GtkLabel   *label,
              const char *clock_format,
              const char *tooltip_format)
{
        time_t     t;
        struct tm *tm;
        char       buf[256];
        char      *utf8;

        time (&t);
        tm = localtime (&t);
        if (tm == NULL) {
                g_warning ("Unable to get broken down local time");
                return;
        }
        if (strftime (buf, sizeof (buf), clock_format, tm) == 0) {
                g_warning ("Couldn't format time: %s", clock_format);
                strcpy (buf, "???");
        }
        utf8 = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
        gtk_label_set_text (label, utf8);
        g_free (utf8);

        if (tooltip_format != NULL) {
                if (strftime (buf, sizeof (buf), tooltip_format, tm) == 0) {
                        g_warning ("Couldn't format tooltip date: %s", tooltip_format);
                        strcpy (buf, "???");
                }
                utf8 = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
                gtk_widget_set_tooltip_text (GTK_WIDGET (label), utf8);
                g_free (utf8);
        } else {
                gtk_widget_set_has_tooltip (GTK_WIDGET (label), FALSE);
        }
}

static void
set_clock_timeout (MdmClockWidget *clock,
                   time_t          now)
{
        GTimeVal       tv;
        int            timeouttime;

        if (clock->priv->update_clock_id > 0) {
                g_source_remove (clock->priv->update_clock_id);
                clock->priv->update_clock_id = 0;
        }

        g_get_current_time (&tv);
        timeouttime = (G_USEC_PER_SEC - tv.tv_usec) / 1000 + 1;

        /* timeout of one minute if we don't care about the seconds */
        if (! clock->priv->should_show_seconds) {
                timeouttime += 1000 * (59 - now % 60);
        }

        clock->priv->update_clock_id = g_timeout_add (timeouttime,
                                                      (GSourceFunc)update_timeout_cb,
                                                      clock);
}

static gboolean
update_timeout_cb (MdmClockWidget *clock)
{
        time_t     new_time;

        time (&new_time);

        if (clock->priv->label != NULL) {
                update_clock (GTK_LABEL (clock->priv->label),
                              clock->priv->time_format,
                              clock->priv->tooltip_format);
        }

        set_clock_timeout (clock, new_time);

        return FALSE;
}

static void
remove_timeout (MdmClockWidget *clock)
{
        if (clock->priv->update_clock_id > 0) {
                g_source_remove (clock->priv->update_clock_id);
                clock->priv->update_clock_id = 0;
        }
}

static void
mdm_clock_widget_size_request (GtkWidget      *widget,
                               GtkRequisition *requisition)
{
        PangoFontMetrics *metrics;
        PangoContext     *context;
        int               ascent;
        int               descent;
        int               padding;

        if (GTK_WIDGET_CLASS (mdm_clock_widget_parent_class)->size_request) {
                GTK_WIDGET_CLASS (mdm_clock_widget_parent_class)->size_request (widget, requisition);
        }

        gtk_widget_ensure_style (widget);
        context = gtk_widget_get_pango_context (widget);
        metrics = pango_context_get_metrics (context,
                                             gtk_widget_get_style (widget)->font_desc,
                                             pango_context_get_language (context));

        ascent = pango_font_metrics_get_ascent (metrics);
        descent = pango_font_metrics_get_descent (metrics);
        padding = PANGO_PIXELS (ascent + descent) / 2.0;
        requisition->height += padding;

        pango_font_metrics_unref (metrics);
}

static void
mdm_clock_widget_class_init (MdmClockWidgetClass *klass)
{
        GObjectClass *object_class;
        GtkWidgetClass *widget_class;

        object_class = G_OBJECT_CLASS (klass);
        widget_class = GTK_WIDGET_CLASS (klass);

        object_class->finalize = mdm_clock_widget_finalize;
        widget_class->size_request = mdm_clock_widget_size_request;

        g_type_class_add_private (klass, sizeof (MdmClockWidgetPrivate));
}

static void
mdm_clock_widget_init (MdmClockWidget *widget)
{
        GtkWidget *box;

        widget->priv = MDM_CLOCK_WIDGET_GET_PRIVATE (widget);

        box = gtk_hbox_new (FALSE, 6);
        gtk_widget_show (box);
        gtk_container_add (GTK_CONTAINER (widget), box);

        widget->priv->label = gtk_label_new ("");

        gtk_widget_show (widget->priv->label);
        gtk_box_pack_start (GTK_BOX (box), widget->priv->label, FALSE, FALSE, 0);

        update_time_format (widget);
        update_timeout_cb (widget);
}

static void
mdm_clock_widget_finalize (GObject *object)
{
        MdmClockWidget *clock_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_CLOCK_WIDGET (object));

        clock_widget = MDM_CLOCK_WIDGET (object);

        g_return_if_fail (clock_widget->priv != NULL);

        remove_timeout (clock_widget);

        G_OBJECT_CLASS (mdm_clock_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_clock_widget_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_CLOCK_WIDGET,
                               NULL);

        return GTK_WIDGET (object);
}
