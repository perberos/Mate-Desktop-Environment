/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2007      William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MATE_SETTINGS_PLUGIN_H__
#define __MATE_SETTINGS_PLUGIN_H__

#include <glib-object.h>
#include <gmodule.h>

#ifdef __cplusplus
extern "C" {
#endif
#define MATE_TYPE_SETTINGS_PLUGIN              (mate_settings_plugin_get_type())
#define MATE_SETTINGS_PLUGIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), MATE_TYPE_SETTINGS_PLUGIN, MateSettingsPlugin))
#define MATE_SETTINGS_PLUGIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  MATE_TYPE_SETTINGS_PLUGIN, MateSettingsPluginClass))
#define MATE_IS_SETTINGS_PLUGIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), MATE_TYPE_SETTINGS_PLUGIN))
#define MATE_IS_SETTINGS_PLUGIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_SETTINGS_PLUGIN))
#define MATE_SETTINGS_PLUGIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),  MATE_TYPE_SETTINGS_PLUGIN, MateSettingsPluginClass))

typedef struct
{
        GObject parent;
} MateSettingsPlugin;

typedef struct
{
        GObjectClass parent_class;

        /* Virtual public methods */
        void            (*activate)                     (MateSettingsPlugin *plugin);
        void            (*deactivate)                   (MateSettingsPlugin *plugin);
} MateSettingsPluginClass;

GType            mate_settings_plugin_get_type           (void) G_GNUC_CONST;

void             mate_settings_plugin_activate           (MateSettingsPlugin *plugin);
void             mate_settings_plugin_deactivate         (MateSettingsPlugin *plugin);

/*
 * Utility macro used to register plugins
 *
 * use: MATE_SETTINGS_PLUGIN_REGISTER (PluginName, plugin_name)
 */
#define MATE_SETTINGS_PLUGIN_REGISTER(PluginName, plugin_name)                   \
                                                                                \
static GType plugin_name##_type = 0;                                            \
static GTypeModule *plugin_module_type = 0;                                     \
                                                                                \
GType                                                                           \
plugin_name##_get_type (void)                                                   \
{                                                                               \
        return plugin_name##_type;                                              \
}                                                                               \
                                                                                \
static void     plugin_name##_init              (PluginName        *self);      \
static void     plugin_name##_class_init        (PluginName##Class *klass);     \
static gpointer plugin_name##_parent_class = NULL;                              \
static void     plugin_name##_class_intern_init (gpointer klass)                \
{                                                                               \
        plugin_name##_parent_class = g_type_class_peek_parent (klass);          \
        plugin_name##_class_init ((PluginName##Class *) klass);                 \
}                                                                               \
                                                                                \
G_MODULE_EXPORT GType                                                           \
register_mate_settings_plugin (GTypeModule *module)                              \
{                                                                               \
        static const GTypeInfo our_info =                                       \
        {                                                                       \
                sizeof (PluginName##Class),                                     \
                NULL, /* base_init */                                           \
                NULL, /* base_finalize */                                       \
                (GClassInitFunc) plugin_name##_class_intern_init,               \
                NULL,                                                           \
                NULL, /* class_data */                                          \
                sizeof (PluginName),                                            \
                0, /* n_preallocs */                                            \
                (GInstanceInitFunc) plugin_name##_init                          \
        };                                                                      \
                                                                                \
        g_debug ("Registering " #PluginName);                                   \
                                                                                \
        /* Initialise the i18n stuff */                                         \
        bindtextdomain (GETTEXT_PACKAGE, MATE_SETTINGS_LOCALEDIR);               \
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");                     \
                                                                                \
        plugin_module_type = module;                                            \
        plugin_name##_type = g_type_module_register_type (module,               \
                                            MATE_TYPE_SETTINGS_PLUGIN,           \
                                            #PluginName,                        \
                                            &our_info,                          \
                                            0);                                 \
                                                                                \
        return plugin_name##_type;                                              \
}

/*
 * Utility macro used to register gobject types in plugins with additional code
 *
 * use: MATE_SETTINGS_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE)
 */
#define MATE_SETTINGS_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE)   \
static void     object_name##_init              (ObjectName        *self);      \
static void     object_name##_class_init        (ObjectName##Class *klass);     \
static gpointer object_name##_parent_class = ((void *)0);                       \
static GType    ojbect_name##_type_id = 0;                                      \
                                                                                \
static void     object_name##_class_intern_init (gpointer klass)                \
{                                                                               \
        object_name##_parent_class = g_type_class_peek_parent (klass);          \
        object_name##_class_init ((ObjectName##Class *) klass);                 \
}                                                                               \
                                                                                \
                                                                                \
GType                                                                           \
object_name##_get_type (void)                                                   \
{                                                                               \
        g_assert (object_name##_type_id != 0);                                  \
                                                                                \
        return object_name##_type_id;                                           \
}                                                                               \
                                                                                \
GType                                                                           \
object_name##_register_type (GTypeModule *module)                               \
{                                                                               \
        if ((object_name##_type_id == 0)) {                                     \
                const GTypeInfo g_define_type_info = {                          \
                        sizeof (ObjectName##Class),                             \
                        (GBaseInitFunc) ((void *)0),                            \
                        (GBaseFinalizeFunc) ((void *)0),                        \
                        (GClassInitFunc) object_name##_class_intern_init,       \
                        (GClassFinalizeFunc) ((void *)0),                       \
                        ((void *)0),                                            \
                        sizeof (ObjectName),                                    \
                        0,                                                      \
                        (GInstanceInitFunc) object_name##_init,                 \
                        ((void *)0)                                             \
                };                                                              \
                object_name##_type_id =                                         \
                        g_type_module_register_type (module,                    \
                                                     PARENT_TYPE,               \
                                                     #ObjectName,               \
                                                     &g_define_type_info,       \
                                                     (GTypeFlags) 0);           \
        }                                                                       \
                                                                                \
        g_debug ("Registering " #ObjectName);                                   \
                                                                                \
        CODE                                                                    \
                                                                                \
        return type_name##_type_id;                                             \
}

/*
 * Utility macro used to register gobject types in plugins
 *
 * use: MATE_SETTINGS_PLUGIN_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)
 */
#define MATE_SETTINGS_PLUGIN_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)           \
        MATE_SETTINGS_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, ;)

#ifdef __cplusplus
}
#endif

#endif  /* __MATE_SETTINGS_PLUGIN_H__ */
