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

#include "mdm-session-option-widget.h"
#include "mdm-sessions.h"

#define MDM_SESSION_OPTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SESSION_OPTION_WIDGET, MdmSessionOptionWidgetPrivate))

#define MDM_SESSION_OPTION_WIDGET_LAST_SESSION "__previous"

struct MdmSessionOptionWidgetPrivate
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

static void     mdm_session_option_widget_class_init  (MdmSessionOptionWidgetClass *klass);
static void     mdm_session_option_widget_init        (MdmSessionOptionWidget      *session_option_widget);
static void     mdm_session_option_widget_finalize    (GObject                     *object);

G_DEFINE_TYPE (MdmSessionOptionWidget, mdm_session_option_widget, MDM_TYPE_OPTION_WIDGET)
static void
mdm_session_option_widget_activated (MdmOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = mdm_option_widget_get_active_item (MDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return;
        }

        g_signal_emit (G_OBJECT (widget), signals[SESSION_ACTIVATED], 0);
}


static void
mdm_session_option_widget_class_init (MdmSessionOptionWidgetClass *klass)
{
        GObjectClass *object_class;
        MdmOptionWidgetClass *option_widget_class;

        object_class = G_OBJECT_CLASS (klass);
        option_widget_class = MDM_OPTION_WIDGET_CLASS (klass);

        object_class->finalize = mdm_session_option_widget_finalize;
        option_widget_class->activated = mdm_session_option_widget_activated;

        signals[SESSION_ACTIVATED] = g_signal_new ("session-activated",
                                                   G_TYPE_FROM_CLASS (object_class),
                                                   G_SIGNAL_RUN_FIRST,
                                                   G_STRUCT_OFFSET (MdmSessionOptionWidgetClass, session_activated),
                                                   NULL,
                                                   NULL,
                                                   g_cclosure_marshal_VOID__VOID,
                                                   G_TYPE_NONE,
                                                   0);

        g_type_class_add_private (klass, sizeof (MdmSessionOptionWidgetPrivate));
}

static void
add_available_sessions (MdmSessionOptionWidget *widget)
{
        char     **session_ids;
        int        i;

        session_ids = mdm_get_all_sessions ();

        for (i = 0; session_ids[i] != NULL; i++) {
                char *name;
                char *comment;

                if (!mdm_get_details_for_session (session_ids[i],
                                                  &name, &comment)) {
                        continue;
                }

                mdm_option_widget_add_item (MDM_OPTION_WIDGET (widget),
                                            session_ids[i], name, comment,
                                            MDM_OPTION_WIDGET_POSITION_MIDDLE);
                g_free (name);
                g_free (comment);
        }

        g_strfreev (session_ids);
}

static void
mdm_session_option_widget_init (MdmSessionOptionWidget *widget)
{
        widget->priv = MDM_SESSION_OPTION_WIDGET_GET_PRIVATE (widget);

        add_available_sessions (widget);
}

static void
mdm_session_option_widget_finalize (GObject *object)
{
        MdmSessionOptionWidget *session_option_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_SESSION_OPTION_WIDGET (object));

        session_option_widget = MDM_SESSION_OPTION_WIDGET (object);

        g_return_if_fail (session_option_widget->priv != NULL);

        G_OBJECT_CLASS (mdm_session_option_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_session_option_widget_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_SESSION_OPTION_WIDGET,
                               "label-text", _("Session"),
                               "icon-name", "session-properties",
                               NULL);

        return GTK_WIDGET (object);
}

char *
mdm_session_option_widget_get_current_session (MdmSessionOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = mdm_option_widget_get_active_item (MDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return NULL;
        }

        return active_item_id;
}

void
mdm_session_option_widget_set_current_session (MdmSessionOptionWidget *widget,
                                               const char             *session)
{
        if (session == NULL) {
                mdm_option_widget_set_active_item (MDM_OPTION_WIDGET (widget),
                                                   MDM_SESSION_OPTION_WIDGET_LAST_SESSION);
        } else if (mdm_option_widget_lookup_item (MDM_OPTION_WIDGET (widget), session,
                                            NULL, NULL, NULL)) {
                mdm_option_widget_set_active_item (MDM_OPTION_WIDGET (widget), session);
        }
}
