/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "gdm-settings-backend.h"

#include "gdm-marshal.h"

#define GDM_SETTINGS_BACKEND_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_SETTINGS_BACKEND, GdmSettingsBackendPrivate))

struct GdmSettingsBackendPrivate
{
        gpointer dummy;
};

enum {
        VALUE_CHANGED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     gdm_settings_backend_class_init (GdmSettingsBackendClass *klass);
static void     gdm_settings_backend_init       (GdmSettingsBackend      *settings_backend);
static void     gdm_settings_backend_finalize   (GObject                 *object);

G_DEFINE_ABSTRACT_TYPE (GdmSettingsBackend, gdm_settings_backend, G_TYPE_OBJECT)

GQuark
gdm_settings_backend_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("gdm_settings_backend_error");
        }

        return ret;
}

static gboolean
gdm_settings_backend_real_get_value (GdmSettingsBackend *settings_backend,
                                     const char         *key,
                                     char              **value,
                                     GError            **error)
{
        g_return_val_if_fail (GDM_IS_SETTINGS_BACKEND (settings_backend), FALSE);

        return FALSE;
}

static gboolean
gdm_settings_backend_real_set_value (GdmSettingsBackend *settings_backend,
                                     const char         *key,
                                     const char         *value,
                                     GError            **error)
{
        g_return_val_if_fail (GDM_IS_SETTINGS_BACKEND (settings_backend), FALSE);

        return FALSE;
}

gboolean
gdm_settings_backend_get_value (GdmSettingsBackend *settings_backend,
                                const char         *key,
                                char              **value,
                                GError            **error)
{
        gboolean ret;

        g_return_val_if_fail (GDM_IS_SETTINGS_BACKEND (settings_backend), FALSE);

        g_object_ref (settings_backend);
        ret = GDM_SETTINGS_BACKEND_GET_CLASS (settings_backend)->get_value (settings_backend, key, value, error);
        g_object_unref (settings_backend);

        return ret;
}

gboolean
gdm_settings_backend_set_value (GdmSettingsBackend *settings_backend,
                                const char         *key,
                                const char         *value,
                                GError            **error)
{
        gboolean ret;

        g_return_val_if_fail (GDM_IS_SETTINGS_BACKEND (settings_backend), FALSE);

        g_object_ref (settings_backend);
        ret = GDM_SETTINGS_BACKEND_GET_CLASS (settings_backend)->set_value (settings_backend, key, value, error);
        g_object_unref (settings_backend);

        return ret;
}

void
gdm_settings_backend_value_changed (GdmSettingsBackend *settings_backend,
                                    const char         *key,
                                    const char         *old_value,
                                    const char         *new_value)
{
        g_return_if_fail (GDM_IS_SETTINGS_BACKEND (settings_backend));

        g_signal_emit (settings_backend, signals[VALUE_CHANGED], 0, key, old_value, new_value);
}

static void
gdm_settings_backend_class_init (GdmSettingsBackendClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gdm_settings_backend_finalize;

        klass->get_value = gdm_settings_backend_real_get_value;
        klass->set_value = gdm_settings_backend_real_set_value;

        signals [VALUE_CHANGED] =
                g_signal_new ("value-changed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GdmSettingsBackendClass, value_changed),
                              NULL,
                              NULL,
                              gdm_marshal_VOID__STRING_STRING_STRING,
                              G_TYPE_NONE,
                              3,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

        g_type_class_add_private (klass, sizeof (GdmSettingsBackendPrivate));
}

static void
gdm_settings_backend_init (GdmSettingsBackend *settings_backend)
{
        settings_backend->priv = GDM_SETTINGS_BACKEND_GET_PRIVATE (settings_backend);
}

static void
gdm_settings_backend_finalize (GObject *object)
{
        GdmSettingsBackend *settings_backend;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_SETTINGS_BACKEND (object));

        settings_backend = GDM_SETTINGS_BACKEND (object);

        g_return_if_fail (settings_backend->priv != NULL);

        G_OBJECT_CLASS (gdm_settings_backend_parent_class)->finalize (object);
}
