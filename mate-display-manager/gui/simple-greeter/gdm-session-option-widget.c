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

#include "gdm-session-option-widget.h"
#include "gdm-sessions.h"

#define GDM_SESSION_OPTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_SESSION_OPTION_WIDGET, GdmSessionOptionWidgetPrivate))

#define GDM_SESSION_OPTION_WIDGET_LAST_SESSION "__previous"

struct GdmSessionOptionWidgetPrivate
{
        gpointer dummy;
};

enum {
        PROP_0,
};

enum {
        SESSION_ACTIVATED,
        NUMBER_OF_SIGNALS
};

static guint signals [NUMBER_OF_SIGNALS] = { 0, };

static void     gdm_session_option_widget_class_init  (GdmSessionOptionWidgetClass *klass);
static void     gdm_session_option_widget_init        (GdmSessionOptionWidget      *session_option_widget);
static void     gdm_session_option_widget_finalize    (GObject                     *object);

G_DEFINE_TYPE (GdmSessionOptionWidget, gdm_session_option_widget, GDM_TYPE_OPTION_WIDGET)
static void
gdm_session_option_widget_activated (GdmOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = gdm_option_widget_get_active_item (GDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return;
        }

        g_signal_emit (G_OBJECT (widget), signals[SESSION_ACTIVATED], 0);
}


static void
gdm_session_option_widget_class_init (GdmSessionOptionWidgetClass *klass)
{
        GObjectClass *object_class;
        GdmOptionWidgetClass *option_widget_class;

        object_class = G_OBJECT_CLASS (klass);
        option_widget_class = GDM_OPTION_WIDGET_CLASS (klass);

        object_class->finalize = gdm_session_option_widget_finalize;
        option_widget_class->activated = gdm_session_option_widget_activated;

        signals[SESSION_ACTIVATED] = g_signal_new ("session-activated",
                                                   G_TYPE_FROM_CLASS (object_class),
                                                   G_SIGNAL_RUN_FIRST,
                                                   G_STRUCT_OFFSET (GdmSessionOptionWidgetClass, session_activated),
                                                   NULL,
                                                   NULL,
                                                   g_cclosure_marshal_VOID__VOID,
                                                   G_TYPE_NONE,
                                                   0);

        g_type_class_add_private (klass, sizeof (GdmSessionOptionWidgetPrivate));
}

static void
add_available_sessions (GdmSessionOptionWidget *widget)
{
        char     **session_ids;
        int        i;

        session_ids = gdm_get_all_sessions ();

        for (i = 0; session_ids[i] != NULL; i++) {
                char *name;
                char *comment;

                if (!gdm_get_details_for_session (session_ids[i],
                                                  &name, &comment)) {
                        continue;
                }

                gdm_option_widget_add_item (GDM_OPTION_WIDGET (widget),
                                            session_ids[i], name, comment,
                                            GDM_OPTION_WIDGET_POSITION_MIDDLE);
                g_free (name);
                g_free (comment);
        }

        g_strfreev (session_ids);
}

static void
gdm_session_option_widget_init (GdmSessionOptionWidget *widget)
{
        widget->priv = GDM_SESSION_OPTION_WIDGET_GET_PRIVATE (widget);

        add_available_sessions (widget);
}

static void
gdm_session_option_widget_finalize (GObject *object)
{
        GdmSessionOptionWidget *session_option_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_SESSION_OPTION_WIDGET (object));

        session_option_widget = GDM_SESSION_OPTION_WIDGET (object);

        g_return_if_fail (session_option_widget->priv != NULL);

        G_OBJECT_CLASS (gdm_session_option_widget_parent_class)->finalize (object);
}

GtkWidget *
gdm_session_option_widget_new (void)
{
        GObject *object;

        object = g_object_new (GDM_TYPE_SESSION_OPTION_WIDGET,
                               "label-text", _("Session"),
                               "icon-name", "session-properties",
                               NULL);

        return GTK_WIDGET (object);
}

char *
gdm_session_option_widget_get_current_session (GdmSessionOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = gdm_option_widget_get_active_item (GDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return NULL;
        }

        return active_item_id;
}

void
gdm_session_option_widget_set_current_session (GdmSessionOptionWidget *widget,
                                               const char             *session)
{
        if (session == NULL) {
                gdm_option_widget_set_active_item (GDM_OPTION_WIDGET (widget),
                                                   GDM_SESSION_OPTION_WIDGET_LAST_SESSION);
        } else if (gdm_option_widget_lookup_item (GDM_OPTION_WIDGET (widget), session,
                                            NULL, NULL, NULL)) {
                gdm_option_widget_set_active_item (GDM_OPTION_WIDGET (widget), session);
        }
}
