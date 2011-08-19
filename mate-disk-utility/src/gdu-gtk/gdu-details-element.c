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

struct GduDetailsElementPrivate
{
        gchar *heading;
        gchar *text;
        guint64 time;
        gdouble progress;
        gchar *tooltip;
        GIcon *icon;
        gchar *action_text;
        gchar *action_uri;
        gchar *action_tooltip;
        gboolean is_spinning;
        GtkWidget *widget;
};

enum
{
        PROP_0,
        PROP_HEADING,
        PROP_TEXT,
        PROP_TIME,
        PROP_PROGRESS,
        PROP_TOOLTIP,
        PROP_ICON,
        PROP_ACTION_TEXT,
        PROP_ACTION_URI,
        PROP_ACTION_TOOLTIP,
        PROP_IS_SPINNING,
        PROP_WIDGET
};

enum
{
        CHANGED_SIGNAL,
        ACTIVATED_SIGNAL,
        LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = {0,};

G_DEFINE_TYPE (GduDetailsElement, gdu_details_element, G_TYPE_OBJECT)

static void
gdu_details_element_finalize (GObject *object)
{
        GduDetailsElement *element = GDU_DETAILS_ELEMENT (object);

        g_free (element->priv->heading);
        g_free (element->priv->text);
        g_free (element->priv->tooltip);
        if (element->priv->icon != NULL)
                g_object_unref (element->priv->icon);
        g_free (element->priv->action_text);
        g_free (element->priv->action_uri);
        g_free (element->priv->action_tooltip);

        if (G_OBJECT_CLASS (gdu_details_element_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_details_element_parent_class)->finalize (object);
}

static void
gdu_details_element_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        GduDetailsElement *element = GDU_DETAILS_ELEMENT (object);

        switch (property_id) {
        case PROP_HEADING:
                g_value_set_string (value, gdu_details_element_get_heading (element));
                break;

        case PROP_TEXT:
                g_value_set_string (value, gdu_details_element_get_text (element));
                break;

        case PROP_TIME:
                g_value_set_uint64 (value, gdu_details_element_get_time (element));
                break;

        case PROP_PROGRESS:
                g_value_set_double (value, gdu_details_element_get_progress (element));
                break;

        case PROP_TOOLTIP:
                g_value_set_string (value, gdu_details_element_get_tooltip (element));
                break;

        case PROP_ICON:
                g_value_take_object (value, gdu_details_element_get_icon (element));
                break;

        case PROP_ACTION_TEXT:
                g_value_set_string (value, gdu_details_element_get_action_text (element));
                break;

        case PROP_ACTION_URI:
                g_value_set_string (value, gdu_details_element_get_action_uri (element));
                break;

        case PROP_ACTION_TOOLTIP:
                g_value_set_string (value, gdu_details_element_get_action_tooltip (element));
                break;

        case PROP_IS_SPINNING:
                g_value_set_boolean (value, gdu_details_element_get_is_spinning (element));
                break;

        case PROP_WIDGET:
                g_value_set_object (value, gdu_details_element_get_widget (element));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_details_element_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        GduDetailsElement *element = GDU_DETAILS_ELEMENT (object);

        switch (property_id) {

        case PROP_HEADING:
                gdu_details_element_set_heading (element, g_value_get_string (value));
                break;

        case PROP_TEXT:
                gdu_details_element_set_text (element, g_value_get_string (value));
                break;

        case PROP_TIME:
                gdu_details_element_set_time (element, g_value_get_uint64 (value));
                break;

        case PROP_PROGRESS:
                gdu_details_element_set_progress (element, g_value_get_double (value));
                break;

        case PROP_TOOLTIP:
                gdu_details_element_set_tooltip (element, g_value_get_string (value));
                break;

        case PROP_ICON:
                gdu_details_element_set_icon (element, g_value_get_object (value));
                break;

        case PROP_ACTION_TEXT:
                gdu_details_element_set_action_text (element, g_value_get_string (value));
                break;

        case PROP_ACTION_URI:
                gdu_details_element_set_action_uri (element, g_value_get_string (value));
                break;

        case PROP_ACTION_TOOLTIP:
                gdu_details_element_set_action_tooltip (element, g_value_get_string (value));
                break;

        case PROP_IS_SPINNING:
                gdu_details_element_set_is_spinning (element, g_value_get_boolean (value));
                break;

        case PROP_WIDGET:
                gdu_details_element_set_widget (element, g_value_get_object (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_details_element_init (GduDetailsElement *element)
{
        element->priv = G_TYPE_INSTANCE_GET_PRIVATE (element,
                                                     GDU_TYPE_DETAILS_ELEMENT,
                                                     GduDetailsElementPrivate);

}

static void
gdu_details_element_class_init (GduDetailsElementClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduDetailsElementPrivate));

        gobject_class->get_property        = gdu_details_element_get_property;
        gobject_class->set_property        = gdu_details_element_set_property;
        gobject_class->finalize            = gdu_details_element_finalize;

        g_object_class_install_property (gobject_class,
                                         PROP_HEADING,
                                         g_param_spec_string ("heading",
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_TEXT,
                                         g_param_spec_string ("text",
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_TIME,
                                         g_param_spec_uint64 ("time",
                                                              NULL,
                                                              NULL,
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_PROGRESS,
                                         g_param_spec_double ("progress",
                                                              NULL,
                                                              NULL,
                                                              -G_MAXDOUBLE,
                                                              G_MAXDOUBLE,
                                                              -1.0,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_TOOLTIP,
                                         g_param_spec_string ("tooltip",
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_ICON,
                                         g_param_spec_object ("icon",
                                                              NULL,
                                                              NULL,
                                                              G_TYPE_ICON,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_ACTION_TEXT,
                                         g_param_spec_string ("action-text",
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_ACTION_URI,
                                         g_param_spec_string ("action-uri",
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_ACTION_TOOLTIP,
                                         g_param_spec_string ("action-tooltip",
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT));


        g_object_class_install_property (gobject_class,
                                         PROP_IS_SPINNING,
                                         g_param_spec_boolean ("is-spinning",
                                                               NULL,
                                                               NULL,
                                                               FALSE,
                                                               G_PARAM_READABLE |
                                                               G_PARAM_WRITABLE |
                                                               G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_WIDGET,
                                         g_param_spec_object ("widget",
                                                              NULL,
                                                              NULL,
                                                              GTK_TYPE_WIDGET,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT));

        signals[CHANGED_SIGNAL] = g_signal_new ("changed",
                                                GDU_TYPE_DETAILS_ELEMENT,
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (GduDetailsElementClass, changed),
                                                NULL,
                                                NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE,
                                                0);

        signals[ACTIVATED_SIGNAL] = g_signal_new ("activated",
                                                  GDU_TYPE_DETAILS_ELEMENT,
                                                  G_SIGNAL_RUN_LAST,
                                                  G_STRUCT_OFFSET (GduDetailsElementClass, activated),
                                                  NULL,
                                                  NULL,
                                                  g_cclosure_marshal_VOID__STRING,
                                                  G_TYPE_NONE,
                                                  1,
                                                  G_TYPE_STRING);
}

GduDetailsElement *
gdu_details_element_new (const gchar       *heading,
                         const gchar       *text,
                         const gchar       *tooltip)
{
        return GDU_DETAILS_ELEMENT (g_object_new (GDU_TYPE_DETAILS_ELEMENT,
                                                  "heading", heading,
                                                  "text", text,
                                                  "tooltip", tooltip,
                                                  NULL));
}

const gchar *
gdu_details_element_get_heading (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), NULL);
        return element->priv->heading;
}

const gchar *
gdu_details_element_get_text (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), NULL);
        return element->priv->text;
}

guint64
gdu_details_element_get_time (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), 0);
        return element->priv->time;
}

gdouble
gdu_details_element_get_progress (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), -1);
        return element->priv->progress;
}

const gchar *
gdu_details_element_get_tooltip (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), NULL);
        return element->priv->tooltip;
}

GIcon *
gdu_details_element_get_icon (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), NULL);
        return element->priv->icon != NULL ? g_object_ref (element->priv->icon) : NULL;
}

const gchar *
gdu_details_element_get_action_text (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), NULL);
        return element->priv->action_text;
}

const gchar *
gdu_details_element_get_action_uri (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), NULL);
        return element->priv->action_uri;
}

const gchar *
gdu_details_element_get_action_tooltip (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), NULL);
        return element->priv->action_tooltip;
}

gboolean
gdu_details_element_get_is_spinning (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), FALSE);
        return element->priv->is_spinning;
}

void
gdu_details_element_set_heading (GduDetailsElement *element,
                                 const gchar       *heading)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (g_strcmp0 (element->priv->heading, heading) != 0) {
                g_free (element->priv->heading);
                element->priv->heading = g_strdup (heading);
                g_object_notify (G_OBJECT (element), "heading");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_text (GduDetailsElement *element,
                              const gchar       *text)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (g_strcmp0 (element->priv->text, text) != 0) {
                g_free (element->priv->text);
                element->priv->text = g_strdup (text);
                g_object_notify (G_OBJECT (element), "text");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_time (GduDetailsElement *element,
                              guint64            time)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (element->priv->time != time) {
                element->priv->time = time;
                g_object_notify (G_OBJECT (element), "time");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_progress (GduDetailsElement *element,
                                  gdouble            progress)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (element->priv->progress != progress) {
                element->priv->progress = progress;
                g_object_notify (G_OBJECT (element), "progress");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_tooltip (GduDetailsElement *element,
                                 const gchar       *tooltip)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (g_strcmp0 (element->priv->tooltip, tooltip) != 0) {
                g_free (element->priv->tooltip);
                element->priv->tooltip = g_strdup (tooltip);
                g_object_notify (G_OBJECT (element), "tooltip");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_icon (GduDetailsElement *element,
                              GIcon             *icon)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (!g_icon_equal (element->priv->icon, icon)) {
                if (element->priv->icon != NULL)
                        g_object_unref (element->priv->icon);
                element->priv->icon = icon != NULL ? g_object_ref (icon) : NULL;
                g_object_notify (G_OBJECT (element), "icon");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_action_text (GduDetailsElement *element,
                                     const gchar       *action_text)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (g_strcmp0 (element->priv->action_text, action_text) != 0) {
                g_free (element->priv->action_text);
                element->priv->action_text = g_strdup (action_text);
                g_object_notify (G_OBJECT (element), "action-text");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_action_uri (GduDetailsElement *element,
                                    const gchar       *action_uri)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (g_strcmp0 (element->priv->action_uri, action_uri) != 0) {
                g_free (element->priv->action_uri);
                element->priv->action_uri = g_strdup (action_uri);
                g_object_notify (G_OBJECT (element), "action-uri");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_action_tooltip (GduDetailsElement *element,
                                        const gchar       *action_tooltip)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (g_strcmp0 (element->priv->action_tooltip, action_tooltip) != 0) {
                g_free (element->priv->action_tooltip);
                element->priv->action_tooltip = g_strdup (action_tooltip);
                g_object_notify (G_OBJECT (element), "action-tooltip");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}

void
gdu_details_element_set_is_spinning (GduDetailsElement *element,
                                     gboolean           is_spinning)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (element->priv->is_spinning != is_spinning) {
                element->priv->is_spinning = is_spinning;
                g_object_notify (G_OBJECT (element), "is-spinning");
                g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
        }
}


/* widget overrides anything else */

GtkWidget *
gdu_details_element_get_widget (GduDetailsElement *element)
{
        g_return_val_if_fail (GDU_IS_DETAILS_ELEMENT (element), NULL);
        return element->priv->widget;
}

void
gdu_details_element_set_widget (GduDetailsElement *element,
                                GtkWidget         *widget)
{
        g_return_if_fail (GDU_IS_DETAILS_ELEMENT (element));
        if (element->priv->widget != NULL)
                g_object_unref (element->priv->widget);
        element->priv->widget = widget != NULL ? g_object_ref_sink (widget) : NULL;
        g_object_notify (G_OBJECT (element), "widget");
        g_signal_emit (element, signals[CHANGED_SIGNAL], 0);
}

