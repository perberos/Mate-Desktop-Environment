/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Christian Hammond
 * Copyright (C) 2006 John Palmieri
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include "config.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "notify.h"
#include "internal.h"

#if !defined(G_PARAM_STATIC_NAME) && !defined(G_PARAM_STATIC_NICK) && \
    !defined(G_PARAM_STATIC_BLURB)
# define G_PARAM_STATIC_NAME 0
# define G_PARAM_STATIC_NICK 0
# define G_PARAM_STATIC_BLURB 0
#endif

static void     notify_notification_class_init (NotifyNotificationClass *klass);
static void     notify_notification_init       (NotifyNotification *sp);
static void     notify_notification_finalize   (GObject            *object);
static void     _close_signal_handler          (DBusGProxy         *proxy,
                                                guint32             id,
                                                guint32             reason,
                                                NotifyNotification *notification);

static void     _action_signal_handler         (DBusGProxy         *proxy,
                                                guint32             id,
                                                char               *action,
                                                NotifyNotification *notification);

typedef struct
{
        NotifyActionCallback cb;
        GFreeFunc            free_func;
        gpointer             user_data;

} CallbackPair;

struct _NotifyNotificationPrivate
{
        guint32         id;
        char           *summary;
        char           *body;

        /* NULL to use icon data. Anything else to have server lookup icon */
        char           *icon_name;

        /*
         * -1   = use server default
         *  0   = never timeout
         *  > 0 = Number of milliseconds before we timeout
         */
        gint            timeout;

        GSList         *actions;
        GHashTable     *action_map;
        GHashTable     *hints;

        GtkWidget      *attached_widget;
        GtkStatusIcon  *status_icon;

        gboolean        has_nondefault_actions;
        gboolean        updates_pending;
        gboolean        signals_registered;

        gint            closed_reason;
};

enum
{
        SIGNAL_CLOSED,
        LAST_SIGNAL
};

enum
{
        PROP_0,
        PROP_ID,
        PROP_SUMMARY,
        PROP_BODY,
        PROP_ICON_NAME,
        PROP_ATTACH_WIDGET,
        PROP_STATUS_ICON,
        PROP_CLOSED_REASON
};

static void     notify_notification_set_property (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void     notify_notification_get_property (GObject      *object,
                                                  guint         prop_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static guint    signals[LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE (NotifyNotification, notify_notification, G_TYPE_OBJECT)

static GObject *
notify_notification_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_params)
{
        GObject *object;

        object = parent_class->constructor (type,
                                            n_construct_properties,
                                            construct_params);

        _notify_cache_add_notification (NOTIFY_NOTIFICATION (object));

        return object;
}

static void
notify_notification_class_init (NotifyNotificationClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->constructor = notify_notification_constructor;
        object_class->get_property = notify_notification_get_property;
        object_class->set_property = notify_notification_set_property;
        object_class->finalize = notify_notification_finalize;

        /**
	 * NotifyNotification::closed:
	 * @notification: The object which received the signal.
	 *
	 * Emitted when the notification is closed.
	 */
        signals[SIGNAL_CLOSED] =
                g_signal_new ("closed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (NotifyNotificationClass, closed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        g_object_class_install_property (object_class,
                                         PROP_ID,
                                         g_param_spec_int ("id", "ID",
                                                           "The notification ID",
                                                           0,
                                                           G_MAXINT32,
                                                           0,
                                                           G_PARAM_READWRITE
                                                           | G_PARAM_CONSTRUCT
                                                           | G_PARAM_STATIC_NAME
                                                           | G_PARAM_STATIC_NICK
                                                           | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_SUMMARY,
                                         g_param_spec_string ("summary",
                                                              "Summary",
                                                              "The summary text",
                                                              NULL,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_CONSTRUCT
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_BODY,
                                         g_param_spec_string ("body",
                                                              "Message Body",
                                                              "The message body text",
                                                              NULL,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_CONSTRUCT
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_ICON_NAME,
                                         g_param_spec_string ("icon-name",
                                                              "Icon Name",
                                                              "The icon filename or icon theme-compliant name",
                                                              NULL,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_CONSTRUCT
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_ATTACH_WIDGET,
                                         g_param_spec_object ("attach-widget",
                                                              "Attach Widget",
                                                              "The widget to attach the notification to",
                                                              GTK_TYPE_WIDGET,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_CONSTRUCT
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_STATUS_ICON,
                                         g_param_spec_object ("status-icon",
                                                              "Status Icon",
                                                              "The status icon to attach the notification to",
                                                              GTK_TYPE_STATUS_ICON,
                                                              G_PARAM_READWRITE
                                                              | G_PARAM_CONSTRUCT
                                                              | G_PARAM_STATIC_NAME
                                                              | G_PARAM_STATIC_NICK
                                                              | G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_CLOSED_REASON,
                                         g_param_spec_int ("closed-reason",
                                                           "Closed Reason",
                                                           "The reason code for why the notification was closed",
                                                           -1,
                                                           G_MAXINT32,
                                                           -1,
                                                           G_PARAM_READABLE
                                                           | G_PARAM_STATIC_NAME
                                                           | G_PARAM_STATIC_NICK
                                                           | G_PARAM_STATIC_BLURB));
}

static void
notify_notification_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        NotifyNotification        *notification = NOTIFY_NOTIFICATION (object);
        NotifyNotificationPrivate *priv = notification->priv;

        switch (prop_id) {
        case PROP_ID:
                priv->id = g_value_get_int (value);
                break;

        case PROP_SUMMARY:
                notify_notification_update (notification,
                                            g_value_get_string (value),
                                            priv->body,
                                            priv->icon_name);
                break;

        case PROP_BODY:
                notify_notification_update (notification,
                                            priv->summary,
                                            g_value_get_string (value),
                                            priv->icon_name);
                break;

        case PROP_ICON_NAME:
                notify_notification_update (notification,
                                            priv->summary,
                                            priv->body,
                                            g_value_get_string (value));
                break;

        case PROP_ATTACH_WIDGET:
                notify_notification_attach_to_widget (notification,
                                                      g_value_get_object (value));
                break;

        case PROP_STATUS_ICON:
                notify_notification_attach_to_status_icon (notification,
                                                           g_value_get_object (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
notify_notification_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        NotifyNotification        *notification = NOTIFY_NOTIFICATION (object);
        NotifyNotificationPrivate *priv = notification->priv;

        switch (prop_id) {
        case PROP_ID:
                g_value_set_int (value, priv->id);
                break;

        case PROP_SUMMARY:
                g_value_set_string (value, priv->summary);
                break;

        case PROP_BODY:
                g_value_set_string (value, priv->body);
                break;

        case PROP_ICON_NAME:
                g_value_set_string (value, priv->icon_name);
                break;

        case PROP_ATTACH_WIDGET:
                g_value_set_object (value, priv->attached_widget);
                break;

        case PROP_STATUS_ICON:
                g_value_set_object (value, priv->status_icon);
                break;

        case PROP_CLOSED_REASON:
                g_value_set_int (value, priv->closed_reason);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
_g_value_free (GValue *value)
{
        g_value_unset (value);
        g_free (value);
}

static void
destroy_pair (CallbackPair *pair)
{
        if (pair->user_data != NULL && pair->free_func != NULL) {
                pair->free_func (pair->user_data);
        }

        g_free (pair);
}

static void
notify_notification_init (NotifyNotification *obj)
{
        obj->priv = g_new0 (NotifyNotificationPrivate, 1);
        obj->priv->timeout = NOTIFY_EXPIRES_DEFAULT;
        obj->priv->closed_reason = -1;
        obj->priv->hints = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  (GFreeFunc) _g_value_free);

        obj->priv->action_map = g_hash_table_new_full (g_str_hash,
                                                       g_str_equal,
                                                       g_free,
                                                       (GFreeFunc) destroy_pair);
}

static void
on_proxy_destroy (DBusGProxy         *proxy,
                  NotifyNotification *notification)
{
        if (notification->priv->signals_registered) {
                dbus_g_proxy_disconnect_signal (proxy,
                                                "NotificationClosed",
                                                G_CALLBACK (_close_signal_handler),
                                                notification);
                dbus_g_proxy_disconnect_signal (proxy,
                                                "ActionInvoked",
                                                G_CALLBACK (_action_signal_handler),
                                                notification);
                notification->priv->signals_registered = FALSE;
        }
}

static void
notify_notification_finalize (GObject *object)
{
        NotifyNotification        *obj = NOTIFY_NOTIFICATION (object);
        NotifyNotificationPrivate *priv = obj->priv;
        DBusGProxy                *proxy;

        _notify_cache_remove_notification (obj);

        g_free (priv->summary);
        g_free (priv->body);
        g_free (priv->icon_name);

        if (priv->actions != NULL) {
                g_slist_foreach (priv->actions, (GFunc) g_free, NULL);
                g_slist_free (priv->actions);
        }

        if (priv->action_map != NULL)
                g_hash_table_destroy (priv->action_map);

        if (priv->hints != NULL)
                g_hash_table_destroy (priv->hints);

        if (priv->attached_widget != NULL)
                g_object_unref (G_OBJECT (priv->attached_widget));

        if (priv->status_icon != NULL)
                g_object_remove_weak_pointer (G_OBJECT (priv->status_icon),
                                              (gpointer) & priv->status_icon);

        proxy = _notify_get_g_proxy ();
        if (proxy != NULL && priv->signals_registered) {
                g_signal_handlers_disconnect_by_func (proxy,
                                                      G_CALLBACK (on_proxy_destroy),
                                                      object);

                dbus_g_proxy_disconnect_signal (proxy,
                                                "NotificationClosed",
                                                G_CALLBACK (_close_signal_handler),
                                                object);
                dbus_g_proxy_disconnect_signal (proxy,
                                                "ActionInvoked",
                                                G_CALLBACK (_action_signal_handler),
                                                object);
        }

        g_free (obj->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
_notify_notification_update_applet_hints (NotifyNotification *n)
{
        NotifyNotificationPrivate *priv = n->priv;
        GdkScreen                 *screen = NULL;
        gint                       x, y;

        if (priv->status_icon != NULL) {
                GdkRectangle    rect;
                guint32         xid;

                xid = gtk_status_icon_get_x11_window_id (priv->status_icon);
                if (xid > 0) {
                        notify_notification_set_hint_uint32 (n, "window-xid", xid);
                }

                if (!gtk_status_icon_get_geometry (priv->status_icon,
                                                   &screen,
                                                   &rect,
                                                   NULL)) {
                        return;
                }

                x = rect.x + rect.width / 2;
                y = rect.y + rect.height / 2;
        } else if (priv->attached_widget != NULL) {
                GtkWidget    *widget = priv->attached_widget;
                GtkAllocation allocation;

                screen = gtk_widget_get_screen (widget);

                gdk_window_get_origin (gtk_widget_get_window (widget), &x, &y);
                gtk_widget_get_allocation (widget, &allocation);

                if (!gtk_widget_get_has_window (widget)) {
                        x += allocation.x;
                        y += allocation.y;
                }

                x += allocation.width / 2;
                y += allocation.height / 2;
        } else {
                return;
        }

        notify_notification_set_geometry_hints (n, screen, x, y);
}

/**
 * notify_notification_new:
 * @summary: The required summary text.
 * @body: The optional body text.
 * @icon: The optional icon theme icon name or filename.
 * @attach: The optional widget to attach to.
 *
 * Creates a new #NotifyNotification. The summary text is required, but
 * all other parameters are optional.
 *
 * Returns: The new #NotifyNotification.
 */
NotifyNotification *
notify_notification_new (const char *summary,
                         const char *body,
                         const char *icon,
                         GtkWidget  *attach)
{
        g_return_val_if_fail (attach == NULL || GTK_IS_WIDGET (attach), NULL);

        return g_object_new (NOTIFY_TYPE_NOTIFICATION,
                             "summary", summary,
                             "body", body,
                             "icon-name", icon,
                             "attach-widget", attach,
                             NULL);
}

/**
 * notify_notification_new_with_status_icon:
 * @summary: The required summary text.
 * @body: The optional body text.
 * @icon: The optional icon theme icon name or filename.
 * @status_icon: The required #GtkStatusIcon.
 *
 * Creates a new #NotifyNotification and attaches to a #GtkStatusIcon.
 * The summary text and @status_icon is required, but all other parameters
 * are optional.
 *
 * Returns: The new #NotifyNotification.
 *
 * Since: 0.4.1
 */
NotifyNotification *
notify_notification_new_with_status_icon (const char    *summary,
                                          const char    *message,
                                          const char    *icon,
                                          GtkStatusIcon *status_icon)
{
        g_return_val_if_fail (status_icon != NULL, NULL);
        g_return_val_if_fail (GTK_IS_STATUS_ICON (status_icon), NULL);

        return g_object_new (NOTIFY_TYPE_NOTIFICATION,
                             "summary", summary,
                             "body", message,
                             "icon-name", icon,
                             "status-icon", status_icon,
                             NULL);
}

/**
 * notify_notification_update:
 * @notification: The notification to update.
 * @summary: The new required summary text.
 * @body: The optional body text.
 * @icon: The optional icon theme icon name or filename.
 *
 * Updates the notification text and icon. This won't send the update out
 * and display it on the screen. For that, you will need to call
 * notify_notification_show().
 *
 * Returns: %TRUE, unless an invalid parameter was passed.
 */
gboolean
notify_notification_update (NotifyNotification *notification,
                            const char         *summary,
                            const char         *body,
                            const char         *icon)
{
        g_return_val_if_fail (notification != NULL, FALSE);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), FALSE);
        g_return_val_if_fail (summary != NULL && *summary != '\0', FALSE);

        if (notification->priv->summary != summary) {
                g_free (notification->priv->summary);
                notification->priv->summary = g_strdup (summary);
                g_object_notify (G_OBJECT (notification), "summary");
        }

        if (notification->priv->body != body) {
                g_free (notification->priv->body);
                notification->priv->body = (body != NULL
                                            && *body != '\0' ? g_strdup (body) : NULL);
                g_object_notify (G_OBJECT (notification), "body");
        }

        if (notification->priv->icon_name != icon) {
                g_free (notification->priv->icon_name);
                notification->priv->icon_name = (icon != NULL
                                                 && *icon != '\0' ? g_strdup (icon) : NULL);
                g_object_notify (G_OBJECT (notification), "icon-name");
        }

        notification->priv->updates_pending = TRUE;

        return TRUE;
}

/**
 * notify_notification_attach_to_widget:
 * @notification: The notification.
 * @attach: The widget to attach to, or %NULL.
 *
 * Attaches the notification to a widget. This will set hints on the
 * notification requesting that the notification point to the widget's
 * location. If @attach is %NULL, the widget will be unset.
 */
void
notify_notification_attach_to_widget (NotifyNotification *notification,
                                      GtkWidget          *attach)
{
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        if (notification->priv->attached_widget == attach)
                return;

        if (notification->priv->attached_widget != NULL)
                g_object_unref (notification->priv->attached_widget);

        notification->priv->attached_widget =
                (attach != NULL ? g_object_ref (attach) : NULL);

        g_object_notify (G_OBJECT (notification), "attach-widget");
}

/**
 * notify_notification_attach_to_status_icon:
 * @notification: The notification.
 * @status_icon: The #GtkStatusIcon to attach to, or %NULL.
 *
 * Attaches the notification to a #GtkStatusIcon. This will set hints on the
 * notification requesting that the notification point to the status icon's
 * location. If @status_icon is %NULL, the status icon will be unset.
 *
 * Since: 0.4.1
 */
void
notify_notification_attach_to_status_icon (NotifyNotification *notification,
                                           GtkStatusIcon      *status_icon)
{
        NotifyNotificationPrivate *priv;

        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (status_icon == NULL
                          || GTK_IS_STATUS_ICON (status_icon));

        priv = notification->priv;

        if (priv->status_icon == status_icon)
                return;

        if (priv->status_icon != NULL) {
                g_object_remove_weak_pointer (G_OBJECT (priv->status_icon),
                                              (gpointer) & priv->status_icon);
        }

        priv->status_icon = status_icon;

        if (priv->status_icon != NULL) {
                g_object_add_weak_pointer (G_OBJECT (priv->status_icon),
                                           (gpointer) & priv->status_icon);
        }

        g_object_notify (G_OBJECT (notification), "status-icon");
}

/**
 * notify_notification_set_geometry_hints:
 * @notification: The notification.
 * @screen: The #GdkScreen the notification should appear on.
 * @x: The X coordinate to point to.
 * @y: The Y coordinate to point to.
 *
 * Sets the geometry hints on the notification. This sets the screen
 * the notification should appear on and the X, Y coordinates it should
 * point to, if the particular notification supports X, Y hints.
 *
 * Since: 0.4.1
 */
void
notify_notification_set_geometry_hints (NotifyNotification *notification,
                                        GdkScreen          *screen,
                                        gint                x,
                                        gint                y)
{
        char *display_name;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (screen != NULL);
        g_return_if_fail (GDK_IS_SCREEN (screen));

        notify_notification_set_hint_int32 (notification, "x", x);
        notify_notification_set_hint_int32 (notification, "y", y);

        display_name = gdk_screen_make_display_name (screen);
        notify_notification_set_hint_string (notification,
                                             "xdisplay",
                                             display_name);
        g_free (display_name);
}

static void
_close_signal_handler (DBusGProxy         *proxy,
                       guint32             id,
                       guint32             reason,
                       NotifyNotification *notification)
{
        if (id == notification->priv->id) {
                g_object_ref (G_OBJECT (notification));
                notification->priv->closed_reason = reason;
                g_signal_emit (notification, signals[SIGNAL_CLOSED], 0);
                notification->priv->id = 0;
                g_object_unref (G_OBJECT (notification));
        }
}

static void
_action_signal_handler (DBusGProxy         *proxy,
                        guint32             id,
                        char               *action,
                        NotifyNotification *notification)
{
        CallbackPair *pair;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        if (id != notification->priv->id)
                return;

        pair = (CallbackPair *) g_hash_table_lookup (notification->priv->action_map,
                                                     action);

        if (pair == NULL) {
                if (g_ascii_strcasecmp (action, "default")) {
                        g_warning ("Received unknown action %s", action);
                }
        } else {
                pair->cb (notification, action, pair->user_data);
        }
}

static char  **
_gslist_to_string_array (GSList *list)
{
        GSList *l;
        GArray *a;

        a = g_array_sized_new (TRUE,
                               FALSE,
                               sizeof (char *),
                               g_slist_length (list));

        for (l = list; l != NULL; l = l->next) {
                g_array_append_val (a, l->data);
        }

        return (char **) g_array_free (a, FALSE);
}

/**
 * notify_notification_show:
 * @notification: The notification.
 * @error: The returned error information.
 *
 * Tells the notification server to display the notification on the screen.
 *
 * Returns: %TRUE if successful. On error, this will return %FALSE and set
 *          @error.
 */
gboolean
notify_notification_show (NotifyNotification *notification,
                          GError            **error)
{
        NotifyNotificationPrivate *priv;
        GError                    *tmp_error = NULL;
        char                     **action_array;
        DBusGProxy                *proxy;

        g_return_val_if_fail (notification != NULL, FALSE);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), FALSE);
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        priv = notification->priv;
        proxy = _notify_get_g_proxy ();
        if (proxy == NULL) {
                g_set_error (error, 0, 0, "Unable to connect to server");
                return FALSE;
        }

        if (!priv->signals_registered) {
                g_signal_connect (proxy,
                                  "destroy",
                                  G_CALLBACK (on_proxy_destroy),
                                  notification);

                dbus_g_proxy_connect_signal (proxy,
                                             "NotificationClosed",
                                             G_CALLBACK (_close_signal_handler),
                                             notification,
                                             NULL);

                dbus_g_proxy_connect_signal (proxy,
                                             "ActionInvoked",
                                             G_CALLBACK (_action_signal_handler),
                                             notification,
                                             NULL);

                priv->signals_registered = TRUE;
        }

        /* If attached to a widget or status icon, modify x and y in hints */
        _notify_notification_update_applet_hints (notification);

        action_array = _gslist_to_string_array (priv->actions);

        /* TODO: make this nonblocking */
        dbus_g_proxy_call (proxy,
                           "Notify",
                           &tmp_error,
                           G_TYPE_STRING, notify_get_app_name (),
                           G_TYPE_UINT, priv->id,
                           G_TYPE_STRING, priv->icon_name,
                           G_TYPE_STRING, priv->summary,
                           G_TYPE_STRING, priv->body,
                           G_TYPE_STRV, action_array,
                           dbus_g_type_get_map ("GHashTable",
                                                G_TYPE_STRING,
                                                G_TYPE_VALUE),
                           priv->hints,
                           G_TYPE_INT, priv->timeout,
                           G_TYPE_INVALID,
                           G_TYPE_UINT, &priv->id,
                           G_TYPE_INVALID);

        /* Don't free the elements because they are owned by priv->actions */
        g_free (action_array);

        if (tmp_error != NULL) {
                g_propagate_error (error, tmp_error);
                return FALSE;
        }

        return TRUE;
}

/**
 * notify_notification_set_timeout:
 * @notification: The notification.
 * @timeout: The timeout in milliseconds.
 *
 * Sets the timeout of the notification. To set the default time, pass
 * %NOTIFY_EXPIRES_DEFAULT as @timeout. To set the notification to never
 * expire, pass %NOTIFY_EXPIRES_NEVER.
 */
void
notify_notification_set_timeout (NotifyNotification *notification,
                                 gint                timeout)
{
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        notification->priv->timeout = timeout;
}

gint
_notify_notification_get_timeout (const NotifyNotification *notification)
{
        g_return_val_if_fail (notification != NULL, -1);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), -1);

        return notification->priv->timeout;
}

/**
 * notify_notification_set_category:
 * @notification: The notification.
 * @category: The category.
 *
 * Sets the category of this notification. This can be used by the
 * notification server to filter or display the data in a certain way.
 */
void
notify_notification_set_category (NotifyNotification *notification,
                                  const char         *category)
{
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        notify_notification_set_hint_string (notification,
                                             "category",
                                             category);
}

/**
 * notify_notification_set_urgency:
 * @notification: The notification.
 * @urgency: The urgency level.
 *
 * Sets the urgency level of this notification.
 *
 * See: #NotifyUrgency
 */
void
notify_notification_set_urgency (NotifyNotification *notification,
                                 NotifyUrgency       urgency)
{
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        notify_notification_set_hint_byte (notification,
                                           "urgency",
                                           (guchar) urgency);
}

static void
_gvalue_array_append_int (GValueArray *array,
                          gint         i)
{
        GValue value = { 0 };

        g_value_init (&value, G_TYPE_INT);
        g_value_set_int (&value, i);
        g_value_array_append (array, &value);
        g_value_unset (&value);
}

static void
_gvalue_array_append_bool (GValueArray *array, gboolean b)
{
        GValue value = { 0 };

        g_value_init (&value, G_TYPE_BOOLEAN);
        g_value_set_boolean (&value, b);
        g_value_array_append (array, &value);
        g_value_unset (&value);
}

static void
_gvalue_array_append_byte_array (GValueArray *array,
                                 guchar      *bytes,
                                 gsize        len)
{
        GArray *byte_array;
        GValue  value = { 0 };

        byte_array = g_array_sized_new (FALSE, FALSE, sizeof (guchar), len);
        g_assert (byte_array != NULL);
        byte_array = g_array_append_vals (byte_array, bytes, len);

        g_value_init (&value, DBUS_TYPE_G_UCHAR_ARRAY);
        g_value_take_boxed (&value, byte_array);
        g_value_array_append (array, &value);
        g_value_unset (&value);
}

/**
 * notify_notification_set_icon_from_pixbuf:
 * @notification: The notification.
 * @icon: The icon.
 *
 * Sets the icon in the notification from a #GdkPixbuf.
 * Deprecated: use notify_notification_set_image_from_pixbuf() instead.
 *
 * This will only work when libmatenotify is compiled against D-BUS 0.60 or
 * higher.
 */
void
notify_notification_set_icon_from_pixbuf (NotifyNotification *notification,
                                          GdkPixbuf          *icon)
{
        notify_notification_set_image_from_pixbuf (notification, icon);
}

/**
 * notify_notification_set_image_from_pixbuf:
 * @notification: The notification.
 * @pixbuf: The image.
 *
 * Sets the image in the notification from a #GdkPixbuf.
 *
 * This will only work when libmatenotify is compiled against D-BUS 0.60 or
 * higher.
 */
void
notify_notification_set_image_from_pixbuf (NotifyNotification *notification,
                                           GdkPixbuf          *pixbuf)
{
        gint            width;
        gint            height;
        gint            rowstride;
        gint            bits_per_sample;
        gint            n_channels;
        guchar         *image;
        gboolean        has_alpha;
        gsize           image_len;
        GValueArray    *image_struct;
        GValue         *value;
        const char     *hint_name;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        g_object_get (pixbuf,
                      "width", &width,
                      "height", &height,
                      "rowstride", &rowstride,
                      "n-channels", &n_channels,
                      "bits-per-sample", &bits_per_sample,
                      "pixels", &image,
                      "has-alpha", &has_alpha,
                      NULL);
        image_len = (height - 1) * rowstride + width *
                ((n_channels * bits_per_sample + 7) / 8);

        image_struct = g_value_array_new (1);

        _gvalue_array_append_int (image_struct, width);
        _gvalue_array_append_int (image_struct, height);
        _gvalue_array_append_int (image_struct, rowstride);
        _gvalue_array_append_bool (image_struct, has_alpha);
        _gvalue_array_append_int (image_struct, bits_per_sample);
        _gvalue_array_append_int (image_struct, n_channels);
        _gvalue_array_append_byte_array (image_struct, image, image_len);

        value = g_new0 (GValue, 1);
        g_value_init (value, G_TYPE_VALUE_ARRAY);
        g_value_take_boxed (value, image_struct);

        if (_notify_check_spec_version(1, 1)) {
                hint_name = "image_data";
        } else {
                hint_name = "icon_data";
        }

        g_hash_table_insert (notification->priv->hints,
                             g_strdup (hint_name),
                             value);
}

/**
 * notify_notification_set_hint_int32:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a 32-bit integer value.
 */
void
notify_notification_set_hint_int32 (NotifyNotification *notification,
                                    const char         *key,
                                    gint                value)
{
        GValue *hint_value;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (key != NULL && *key != '\0');

        hint_value = g_new0 (GValue, 1);
        g_value_init (hint_value, G_TYPE_INT);
        g_value_set_int (hint_value, value);
        g_hash_table_insert (notification->priv->hints,
                             g_strdup (key),
                             hint_value);
}


/**
 * notify_notification_set_hint_uint32:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with an unsigned 32-bit integer value.
 */
void
notify_notification_set_hint_uint32 (NotifyNotification *notification,
                                     const char         *key,
                                     guint               value)
{
        GValue *hint_value;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (key != NULL && *key != '\0');

        hint_value = g_new0 (GValue, 1);
        g_value_init (hint_value, G_TYPE_UINT);
        g_value_set_uint (hint_value, value);
        g_hash_table_insert (notification->priv->hints,
                             g_strdup (key),
                             hint_value);
}

/**
 * notify_notification_set_hint_double:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a double value.
 */
void
notify_notification_set_hint_double (NotifyNotification *notification,
                                     const char         *key,
                                     gdouble             value)
{
        GValue *hint_value;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (key != NULL && *key != '\0');

        hint_value = g_new0 (GValue, 1);
        g_value_init (hint_value, G_TYPE_FLOAT);
        g_value_set_float (hint_value, value);
        g_hash_table_insert (notification->priv->hints,
                             g_strdup (key),
                             hint_value);
}

/**
 * notify_notification_set_hint_byte:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a byte value.
 */
void
notify_notification_set_hint_byte (NotifyNotification *notification,
                                   const char         *key,
                                   guchar              value)
{
        GValue *hint_value;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (key != NULL && *key != '\0');

        hint_value = g_new0 (GValue, 1);
        g_value_init (hint_value, G_TYPE_UCHAR);
        g_value_set_uchar (hint_value, value);

        g_hash_table_insert (notification->priv->hints,
                             g_strdup (key),
                             hint_value);
}

/**
 * notify_notification_set_hint_byte_array:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 * @len: The length of the byte array.
 *
 * Sets a hint with a byte array value. The length of @value must be passed
 * as @len.
 */
void
notify_notification_set_hint_byte_array (NotifyNotification *notification,
                                         const char         *key,
                                         const guchar       *value,
                                         gsize               len)
{
        GValue *hint_value;
        GArray *byte_array;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (key != NULL && *key != '\0');
        g_return_if_fail (value != NULL);
        g_return_if_fail (len > 0);

        byte_array = g_array_sized_new (FALSE, FALSE, sizeof (guchar), len);
        byte_array = g_array_append_vals (byte_array, value, len);

        hint_value = g_new0 (GValue, 1);
        g_value_init (hint_value, dbus_g_type_get_collection ("GArray",
                                                              G_TYPE_UCHAR));
        g_value_take_boxed (hint_value, byte_array);

        g_hash_table_insert (notification->priv->hints,
                             g_strdup (key),
                             hint_value);
}

/**
 * notify_notification_set_hint_string:
 * @notification: The notification.
 * @key: The hint.
 * @value: The hint's value.
 *
 * Sets a hint with a string value.
 */
void
notify_notification_set_hint_string (NotifyNotification *notification,
                                     const char         *key,
                                     const char         *value)
{
        GValue *hint_value;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (key != NULL && *key != '\0');

        hint_value = g_new0 (GValue, 1);
        g_value_init (hint_value, G_TYPE_STRING);
        g_value_set_string (hint_value, value);
        g_hash_table_insert (notification->priv->hints,
                             g_strdup (key),
                             hint_value);
}

static gboolean
_remove_all (void)
{
        return TRUE;
}

/**
 * notify_notification_clear_hints:
 * @notification: The notification.
 *
 * Clears all hints from the notification.
 */
void
notify_notification_clear_hints (NotifyNotification *notification)
{
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        g_hash_table_foreach_remove (notification->priv->hints,
                                     (GHRFunc) _remove_all,
                                     NULL);
}

/**
 * notify_notification_clear_actions:
 * @notification: The notification.
 *
 * Clears all actions from the notification.
 */
void
notify_notification_clear_actions (NotifyNotification *notification)
{
        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));

        g_hash_table_foreach_remove (notification->priv->action_map,
                                     (GHRFunc) _remove_all,
                                     NULL);

        if (notification->priv->actions != NULL) {
                g_slist_foreach (notification->priv->actions,
                                 (GFunc) g_free,
                                 NULL);
                g_slist_free (notification->priv->actions);
        }

        notification->priv->actions = NULL;
        notification->priv->has_nondefault_actions = FALSE;
}

/**
 * notify_notification_add_action:
 * @notification: The notification.
 * @action: The action ID.
 * @label: The human-readable action label.
 * @callback: The action's callback function.
 * @user_data: Optional custom data to pass to @callback.
 * @free_func: An optional function to free @user_data when the notification
 *             is destroyed.
 *
 * Adds an action to a notification. When the action is invoked, the
 * specified callback function will be called, along with the value passed
 * to @user_data.
 */
void
notify_notification_add_action (NotifyNotification  *notification,
                                const char          *action,
                                const char          *label,
                                NotifyActionCallback callback,
                                gpointer             user_data,
                                GFreeFunc            free_func)
{
        NotifyNotificationPrivate *priv;
        CallbackPair              *pair;

        g_return_if_fail (notification != NULL);
        g_return_if_fail (NOTIFY_IS_NOTIFICATION (notification));
        g_return_if_fail (action != NULL && *action != '\0');
        g_return_if_fail (label != NULL && *label != '\0');
        g_return_if_fail (callback != NULL);

        priv = notification->priv;

        priv->actions = g_slist_append (priv->actions, g_strdup (action));
        priv->actions = g_slist_append (priv->actions, g_strdup (label));

        pair = g_new0 (CallbackPair, 1);
        pair->cb = callback;
        pair->user_data = user_data;
        pair->free_func = free_func;
        g_hash_table_insert (priv->action_map, g_strdup (action), pair);

        if (!notification->priv->has_nondefault_actions &&
            g_ascii_strcasecmp (action, "default") != 0) {
                notification->priv->has_nondefault_actions = TRUE;
        }
}

gboolean
_notify_notification_has_nondefault_actions (const NotifyNotification *n)
{
        g_return_val_if_fail (n != NULL, FALSE);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (n), FALSE);

        return n->priv->has_nondefault_actions;
}

/**
 * notify_notification_close:
 * @notification: The notification.
 * @error: The returned error information.
 *
 * Tells the notification server to hide the notification on the screen.
 *
 * Returns: %TRUE if successful. On error, this will return %FALSE and set
 *          @error.
 */
gboolean
notify_notification_close (NotifyNotification *notification,
                           GError            **error)
{
        NotifyNotificationPrivate *priv;
        GError         *tmp_error = NULL;
        DBusGProxy     *proxy;

        g_return_val_if_fail (notification != NULL, FALSE);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), FALSE);
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        priv = notification->priv;

        proxy = _notify_get_g_proxy ();
        if (proxy == NULL) {
                g_set_error (error, 0, 0, "Unable to connect to server");
                return FALSE;
        }

        dbus_g_proxy_call (proxy,
                           "CloseNotification",
                           &tmp_error,
                           G_TYPE_UINT,
                           priv->id,
                           G_TYPE_INVALID,
                           G_TYPE_INVALID);

        if (tmp_error != NULL) {
                g_propagate_error (error, tmp_error);
                return FALSE;
        }

        return TRUE;
}

/**
 * notify_notification_get_closed_reason:
 * @notification: The notification.
 *
 * Returns the closed reason code for the notification. This is valid only
 * after the "closed" signal is emitted.
 *
 * Returns: The closed reason code.
 */
gint
notify_notification_get_closed_reason (const NotifyNotification *notification)
{
        g_return_val_if_fail (notification != NULL, -1);
        g_return_val_if_fail (NOTIFY_IS_NOTIFICATION (notification), -1);

        return notification->priv->closed_reason;
}
