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

#ifndef _NOTIFY_NOTIFICATION_H_
#define _NOTIFY_NOTIFICATION_H_

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NOTIFY_EXPIRES_DEFAULT -1
#define NOTIFY_EXPIRES_NEVER    0

#define NOTIFY_TYPE_NOTIFICATION         (notify_notification_get_type ())
#define NOTIFY_NOTIFICATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NOTIFY_TYPE_NOTIFICATION, NotifyNotification))
#define NOTIFY_NOTIFICATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), NOTIFY_TYPE_NOTIFICATION, NotifyNotificationClass))
#define NOTIFY_IS_NOTIFICATION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NOTIFY_TYPE_NOTIFICATION))
#define NOTIFY_IS_NOTIFICATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), NOTIFY_TYPE_NOTIFICATION))
#define NOTIFY_NOTIFICATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NOTIFY_TYPE_NOTIFICATION, NotifyNotificationClass))

typedef struct _NotifyNotification NotifyNotification;
typedef struct _NotifyNotificationClass NotifyNotificationClass;
typedef struct _NotifyNotificationPrivate NotifyNotificationPrivate;

struct _NotifyNotification
{
        GObject                    parent_object;
        NotifyNotificationPrivate *priv;
};

struct _NotifyNotificationClass
{
        GObjectClass    parent_class;

        /* Signals */
        void            (*closed) (NotifyNotification *notification);
};

/*
 * Notification urgency levels.
 */
typedef enum
{
        NOTIFY_URGENCY_LOW,
        NOTIFY_URGENCY_NORMAL,
        NOTIFY_URGENCY_CRITICAL,

} NotifyUrgency;

typedef void    (*NotifyActionCallback) (NotifyNotification *notification,
                                         char               *action,
                                         gpointer            user_data);

#define NOTIFY_ACTION_CALLBACK(func) ((NotifyActionCallback)(func))

GType               notify_notification_get_type             (void);

NotifyNotification *notify_notification_new                  (const char         *summary,
                                                              const char         *body,
                                                              const char         *icon,
                                                              GtkWidget          *attach);

NotifyNotification *notify_notification_new_with_status_icon  (const char         *summary,
                                                               const char         *body,
                                                               const char         *icon,
                                                               GtkStatusIcon      *status_icon);

gboolean            notify_notification_update                (NotifyNotification *notification,
                                                               const char         *summary,
                                                               const char         *body,
                                                               const char         *icon);

void                notify_notification_attach_to_widget      (NotifyNotification *notification,
                                                               GtkWidget          *attach);

void                notify_notification_attach_to_status_icon (NotifyNotification *notification,
                                                               GtkStatusIcon      *status_icon);

void                notify_notification_set_geometry_hints    (NotifyNotification *notification,
                                                               GdkScreen          *screen,
                                                               gint                x,
                                                               gint                y);

gboolean            notify_notification_show                  (NotifyNotification *notification,
                                                               GError            **error);

void                notify_notification_set_timeout           (NotifyNotification *notification,
                                                               gint                timeout);

void                notify_notification_set_category          (NotifyNotification *notification,
                                                               const char         *category);

void                notify_notification_set_urgency           (NotifyNotification *notification,
                                                               NotifyUrgency       urgency);

void                notify_notification_set_icon_from_pixbuf  (NotifyNotification *notification,
                                                               GdkPixbuf          *icon);
void                notify_notification_set_image_from_pixbuf (NotifyNotification *notification,
                                                               GdkPixbuf          *image);

void                notify_notification_set_hint_int32        (NotifyNotification *notification,
                                                               const char         *key,
                                                               gint                value);
void                notify_notification_set_hint_uint32       (NotifyNotification *notification,
                                                               const char         *key,
                                                               guint               value);

void                notify_notification_set_hint_double       (NotifyNotification *notification,
                                                               const char         *key,
                                                               gdouble             value);

void                notify_notification_set_hint_string       (NotifyNotification *notification,
                                                               const char         *key,
                                                               const char         *value);

void                notify_notification_set_hint_byte         (NotifyNotification *notification,
                                                               const char         *key,
                                                               guchar              value);

void                notify_notification_set_hint_byte_array   (NotifyNotification *notification,
                                                               const char         *key,
                                                               const guchar       *value,
                                                               gsize               len);

void                notify_notification_clear_hints           (NotifyNotification *notification);

void                notify_notification_add_action            (NotifyNotification *notification,
                                                               const char         *action,
                                                               const char         *label,
                                                               NotifyActionCallback callback,
                                                               gpointer            user_data,
                                                               GFreeFunc           free_func);

void                notify_notification_clear_actions         (NotifyNotification *notification);
gboolean            notify_notification_close                 (NotifyNotification *notification,
                                                               GError            **error);

gint                notify_notification_get_closed_reason     (const NotifyNotification *notification);

G_END_DECLS
#endif /* NOTIFY_NOTIFICATION_H */
