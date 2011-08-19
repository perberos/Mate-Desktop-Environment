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


#ifndef __GDM_SETTINGS_H
#define __GDM_SETTINGS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_SETTINGS         (gdm_settings_get_type ())
#define GDM_SETTINGS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SETTINGS, GdmSettings))
#define GDM_SETTINGS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SETTINGS, GdmSettingsClass))
#define GDM_IS_SETTINGS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SETTINGS))
#define GDM_IS_SETTINGS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_SETTINGS))
#define GDM_SETTINGS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_SETTINGS, GdmSettingsClass))

typedef struct GdmSettingsPrivate GdmSettingsPrivate;

typedef struct
{
        GObject             parent;
        GdmSettingsPrivate *priv;
} GdmSettings;

typedef struct
{
        GObjectClass   parent_class;

        void          (* value_changed)    (GdmSettings *settings,
                                            const char  *key,
                                            const char  *old_value,
                                            const char **new_value);
} GdmSettingsClass;

typedef enum
{
        GDM_SETTINGS_ERROR_GENERAL,
        GDM_SETTINGS_ERROR_KEY_NOT_FOUND
} GdmSettingsError;

#define GDM_SETTINGS_ERROR gdm_settings_error_quark ()

GQuark              gdm_settings_error_quark                    (void);
GType               gdm_settings_get_type                       (void);

GdmSettings *       gdm_settings_new                            (void);

/* exported */

gboolean            gdm_settings_get_value                      (GdmSettings *settings,
                                                                 const char  *key,
                                                                 char       **value,
                                                                 GError     **error);
gboolean            gdm_settings_set_value                      (GdmSettings *settings,
                                                                 const char  *key,
                                                                 const char  *value,
                                                                 GError     **error);

G_END_DECLS

#endif /* __GDM_SETTINGS_H */
