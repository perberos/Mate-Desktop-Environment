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
 * Written by: Ray Strode <rstrode@redhat.com>
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

#include <mateconf/mateconf-client.h>

#include "mdm-recent-option-widget.h"

#define MDM_RECENT_OPTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_RECENT_OPTION_WIDGET, MdmRecentOptionWidgetPrivate))

struct MdmRecentOptionWidgetPrivate
{
        MateConfClient                   *mateconf_client;
        char                          *mateconf_key;
        guint                          notification_id;
        MdmRecentOptionLookupItemFunc  lookup_item_func;

        int                            max_item_count;
};

enum {
        PROP_0,
        PROP_MAX_ITEM_COUNT,
};

enum {
        NUMBER_OF_SIGNALS
};

static void     mdm_recent_option_widget_class_init  (MdmRecentOptionWidgetClass *klass);
static void     mdm_recent_option_widget_init        (MdmRecentOptionWidget      *option_widget);
static void     mdm_recent_option_widget_finalize    (GObject              *object);

G_DEFINE_TYPE (MdmRecentOptionWidget, mdm_recent_option_widget, MDM_TYPE_OPTION_WIDGET)

static GSList *
mdm_recent_option_widget_get_list_from_mateconf (MdmRecentOptionWidget *widget)
{
        MateConfValue *value;
        GSList     *value_list;
        GSList     *list;
        GSList     *tmp;
        GError     *lookup_error;

        lookup_error = NULL;
        value = mateconf_client_get (widget->priv->mateconf_client,
                                  widget->priv->mateconf_key,
                                  &lookup_error);

        if (lookup_error != NULL) {
                g_warning ("could not get mateconf key '%s': %s",
                           widget->priv->mateconf_key,
                           lookup_error->message);
                g_error_free (lookup_error);
                return NULL;
        }

        if (value == NULL) {
                g_warning ("mateconf key '%s' is unset",
                           widget->priv->mateconf_key);
                return NULL;
        }

        if (value->type != MATECONF_VALUE_LIST ||
            mateconf_value_get_list_type (value) != MATECONF_VALUE_STRING) {
                g_warning ("mateconf key is not a list of strings");
                return NULL;
        }

        value_list = mateconf_value_get_list (value);

        list = NULL;
        for (tmp = value_list; tmp != NULL; tmp = tmp->next) {
                const char *id;

                id = mateconf_value_get_string ((MateConfValue *)tmp->data);

                list = g_slist_prepend (list, g_strdup (id));
        }
        list = g_slist_reverse (list);

        mateconf_value_free (value);

        return list;
}

static GSList *
truncate_list (GSList *list,
               int     size)
{
    GSList     *tmp;
    GSList     *next_node;

    tmp = g_slist_nth (list, size);
    do {
        next_node = tmp->next;

        g_free (tmp->data);
        list = g_slist_delete_link (list, tmp);
        tmp = next_node;
    } while (tmp != NULL);

    return list;
}

static void
mdm_recent_option_widget_sync_items_from_mateconf (MdmRecentOptionWidget *widget)
{
        GSList     *list;
        GSList     *tmp;
        gboolean    default_is_set;

        list = mdm_recent_option_widget_get_list_from_mateconf (widget);

        if (widget->priv->max_item_count > 0 && g_slist_length (list) > widget->priv->max_item_count) {
                list = truncate_list (list, widget->priv->max_item_count);
                mateconf_client_set_list (widget->priv->mateconf_client,
                                       widget->priv->mateconf_key,
                                       MATECONF_VALUE_STRING, list, NULL);
                g_slist_foreach (list, (GFunc) g_free, NULL);
                g_slist_free (list);

                list = mdm_recent_option_widget_get_list_from_mateconf (widget);
        }

        mdm_option_widget_remove_all_items (MDM_OPTION_WIDGET (widget));
        default_is_set = FALSE;

        for (tmp = list; tmp != NULL; tmp = tmp->next) {
                const char *key;
                char       *id;
                char       *name;
                char       *comment;

                key = (char *) tmp->data;

                id = widget->priv->lookup_item_func (widget, key, &name, &comment);

                if (id != NULL) {
                        gboolean item_exists;

                        item_exists = mdm_option_widget_lookup_item (MDM_OPTION_WIDGET (widget), id, NULL, NULL, NULL);

                        if (item_exists) {
                                continue;
                        }

                        mdm_option_widget_add_item (MDM_OPTION_WIDGET (widget),
                                                    id, name, comment,
                                                    MDM_OPTION_WIDGET_POSITION_MIDDLE);
                        if (!default_is_set) {
                                mdm_option_widget_set_active_item (MDM_OPTION_WIDGET (widget),
                                        id);
                                default_is_set = TRUE;
                        }

                        g_free (name);
                        g_free (comment);
                        g_free (id);
                }
        }

        g_slist_foreach (list, (GFunc) g_free, NULL);
        g_slist_free (list);
}

static void
mdm_recent_option_widget_notify_func (MateConfClient *client,
                                      guint        connection_id,
                                      MateConfEntry  *entry,
                                      gpointer     user_data)
{
        mdm_recent_option_widget_sync_items_from_mateconf (MDM_RECENT_OPTION_WIDGET (user_data));
}

gboolean
mdm_recent_option_widget_set_mateconf_key (MdmRecentOptionWidget          *widget,
                                        const char                     *mateconf_key,
                                        MdmRecentOptionLookupItemFunc   lookup_item_func,
                                        GError                        **error)
{
        GError *notify_error;

        if (widget->priv->mateconf_key != NULL &&
            strcmp (widget->priv->mateconf_key, mateconf_key) == 0) {
                return TRUE;
        }

        if (widget->priv->notification_id != 0) {
                mateconf_client_notify_remove (widget->priv->mateconf_client,
                                            widget->priv->notification_id);

                widget->priv->notification_id = 0;
        }

        g_free (widget->priv->mateconf_key);
        widget->priv->mateconf_key = g_strdup (mateconf_key);

        widget->priv->lookup_item_func = lookup_item_func;

        mdm_recent_option_widget_sync_items_from_mateconf (widget);

        notify_error = NULL;
        widget->priv->notification_id = mateconf_client_notify_add (widget->priv->mateconf_client,
                                                                 mateconf_key,
                                                                 (MateConfClientNotifyFunc)
                                                                 mdm_recent_option_widget_notify_func,
                                                                 widget, NULL, &notify_error);
        if (notify_error != NULL) {
                g_propagate_error (error, notify_error);
                return FALSE;
        }

        return TRUE;
}

static void
mdm_recent_option_widget_dispose (GObject *object)
{
        G_OBJECT_CLASS (mdm_recent_option_widget_parent_class)->dispose (object);
}

static void
mdm_recent_option_widget_set_property (GObject        *object,
                                       guint           prop_id,
                                       const GValue   *value,
                                       GParamSpec     *pspec)
{
        MdmRecentOptionWidget *self;

        self = MDM_RECENT_OPTION_WIDGET (object);

        switch (prop_id) {
        case PROP_MAX_ITEM_COUNT:
                self->priv->max_item_count = g_value_get_int (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_recent_option_widget_get_property (GObject        *object,
                                       guint           prop_id,
                                       GValue         *value,
                                       GParamSpec     *pspec)
{
        MdmRecentOptionWidget *self;

        self = MDM_RECENT_OPTION_WIDGET (object);

        switch (prop_id) {
        case PROP_MAX_ITEM_COUNT:
                g_value_set_int (value,
                                 self->priv->max_item_count);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_recent_option_widget_class_init (MdmRecentOptionWidgetClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_recent_option_widget_get_property;
        object_class->set_property = mdm_recent_option_widget_set_property;
        object_class->dispose = mdm_recent_option_widget_dispose;
        object_class->finalize = mdm_recent_option_widget_finalize;

        g_object_class_install_property (object_class,
                                         PROP_MAX_ITEM_COUNT,
                                         g_param_spec_int ("max-item-count",
                                                              _("Max Item Count"),
                                                              _("The maximum number of items to keep around in the list"),
                                                              0, G_MAXINT, 5,
                                                              (G_PARAM_READWRITE |
                                                               G_PARAM_CONSTRUCT)));


        g_type_class_add_private (klass, sizeof (MdmRecentOptionWidgetPrivate));
}

static void
mdm_recent_option_widget_init (MdmRecentOptionWidget *widget)
{
        widget->priv = MDM_RECENT_OPTION_WIDGET_GET_PRIVATE (widget);

        widget->priv->mateconf_client = mateconf_client_get_default ();
        widget->priv->max_item_count = 5;
}

static void
mdm_recent_option_widget_finalize (GObject *object)
{
        MdmRecentOptionWidget *widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_OPTION_WIDGET (object));

        widget = MDM_RECENT_OPTION_WIDGET (object);

        g_return_if_fail (widget->priv != NULL);

        g_object_unref (widget->priv->mateconf_client);

        G_OBJECT_CLASS (mdm_recent_option_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_recent_option_widget_new (const char *label_text,
                              int         max_item_count)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_RECENT_OPTION_WIDGET,
                               "label-text", label_text,
                               "max-item-count", max_item_count,
                               NULL);

        return GTK_WIDGET (object);
}

void
mdm_recent_option_widget_add_item (MdmRecentOptionWidget *widget,
                                   const char            *id)
{
        GSList *list;

        list = mdm_recent_option_widget_get_list_from_mateconf (widget);

        list = g_slist_prepend (list, g_strdup (id));

        mateconf_client_set_list (widget->priv->mateconf_client,
                          widget->priv->mateconf_key,
                          MATECONF_VALUE_STRING, list, NULL);

        g_slist_foreach (list, (GFunc) g_free, NULL);
        g_slist_free (list);

        mdm_recent_option_widget_sync_items_from_mateconf (widget);
}
