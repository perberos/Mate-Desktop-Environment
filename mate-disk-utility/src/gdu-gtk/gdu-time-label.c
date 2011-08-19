/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-time-label.c
 *
 * Copyright (C) 2007 David Zeuthen
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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <string.h>

#include "gdu-time-label.h"

struct GduTimeLabelPrivate
{
        GTimeVal time;
        guint update_timer_id;
};

static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE (GduTimeLabel, gdu_time_label, GTK_TYPE_LABEL)

enum {
        PROP_0,
        PROP_TIME,
};

static void do_update (GduTimeLabel *time_label);

static gboolean
timeout_func (GduTimeLabel *time_label)
{
        time_label->priv->update_timer_id = 0;
        do_update (time_label);
        return FALSE;
}

static void
do_update (GduTimeLabel *time_label)
{
        char *s;
        GTimeVal now;
        int age;
        int next_update;

        next_update = -1;

        g_get_current_time (&now);

        age = (int) (now.tv_sec - time_label->priv->time.tv_sec);

        //g_warning ("updating; age=%d was: %s", age, gtk_label_get_label (GTK_LABEL (time_label)));

        if (age < 60) {
                s = g_strdup_printf (_("Less than a minute ago"));
                next_update = 60 - age;
        } else if (age < 60 * 60) {
                s = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                                "%d minute ago",
                                                "%d minutes ago",
                                                age / 60),
                                     age / 60);
                next_update = 60*(age/60 + 1) - age;
        } else {
                s = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                                "%d hour ago",
                                                "%d hours ago",
                                                age / 60 / 60),
                                     age / 60 / 60);
                next_update = 60*60*(age/(60*60) + 1) - age;
        }
        gtk_label_set_text (GTK_LABEL (time_label), s);
        g_free (s);

        //g_warning ("updating; next=%d now: %s", next_update, gtk_label_get_label (GTK_LABEL (time_label)));

        if (next_update > 0) {
                if (time_label->priv->update_timer_id > 0)
                        g_source_remove (time_label->priv->update_timer_id);
                time_label->priv->update_timer_id =
                        g_timeout_add_seconds (next_update, (GSourceFunc) timeout_func, time_label);
        }
}

static void
gdu_time_label_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
        GduTimeLabel *time_label = GDU_TIME_LABEL (object);
        GTimeVal *time_val;

        switch (prop_id) {
        case PROP_TIME:
                time_val = (GTimeVal *) g_value_get_boxed (value);
                if (time_val != NULL) {
                        time_label->priv->time = * time_val;
                        do_update (time_label);
                } else {
                        if (time_label->priv->update_timer_id > 0) {
                                g_source_remove (time_label->priv->update_timer_id);
                                time_label->priv->update_timer_id = 0;
                        }
                }
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gdu_time_label_get_property (GObject     *object,
                             guint        prop_id,
                             GValue      *value,
                             GParamSpec  *pspec)
{
        GduTimeLabel *time_label = GDU_TIME_LABEL (object);

        switch (prop_id) {
        case PROP_TIME:
                g_value_set_boxed (value, &time_label->priv->time);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
    }
}

static void
gdu_time_label_finalize (GduTimeLabel *time_label)
{
        if (time_label->priv->update_timer_id > 0)
                g_source_remove (time_label->priv->update_timer_id);
        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (time_label));
}

static GTimeVal *
time_val_copy (GTimeVal *t)
{
        GTimeVal *copy;

        copy = NULL;

        if (t == NULL)
                goto out;

        copy = g_new0 (GTimeVal, 1);
        memcpy (copy, t, sizeof (GTimeVal));

out:
        return copy;
}

static void
time_val_free (GTimeVal *t)
{
        g_free (t);
}

static void
gdu_time_label_class_init (GduTimeLabelClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;
        GType boxed_type;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_time_label_finalize;
        obj_class->set_property = gdu_time_label_set_property;
        obj_class->get_property = gdu_time_label_get_property;

        g_type_class_add_private (klass, sizeof (GduTimeLabelPrivate));

        boxed_type = g_boxed_type_register_static ("GduTimeLabelBoxedGTimeVal",
                                                   (GBoxedCopyFunc) time_val_copy,
                                                   (GBoxedFreeFunc) time_val_free);

        /**
         * GduTimeLabel:time:
         *
         * The time (a #GTimeVal) used for computing the label of the string.
         */
        g_object_class_install_property (obj_class,
                                         PROP_TIME,
                                         g_param_spec_boxed ("time",
                                                             NULL,
                                                             NULL,
                                                             boxed_type,
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_READABLE));
}

static void
gdu_time_label_init (GduTimeLabel *time_label)
{
        time_label->priv = G_TYPE_INSTANCE_GET_PRIVATE (time_label, GDU_TYPE_TIME_LABEL, GduTimeLabelPrivate);
}

GtkWidget *
gdu_time_label_new (GTimeVal *time)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_TIME_LABEL, "time", time, NULL));
}

void
gdu_time_label_set_time (GduTimeLabel *time_label, GTimeVal     *time)
{
        g_object_set (time_label, "time", time, NULL);
}

