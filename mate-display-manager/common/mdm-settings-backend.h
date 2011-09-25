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


#ifndef __MDM_SETTINGS_BACKEND_H
#define __MDM_SETTINGS_BACKEND_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_SETTINGS_BACKEND         (mdm_settings_backend_get_type ())
#define MDM_SETTINGS_BACKEND(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SETTINGS_BACKEND, MdmSettingsBackend))
#define MDM_SETTINGS_BACKEND_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SETTINGS_BACKEND, MdmSettingsBackendClass))
#define MDM_IS_SETTINGS_BACKEND(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SETTINGS_BACKEND))
#define MDM_IS_SETTINGS_BACKEND_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SETTINGS_BACKEND))
#define MDM_SETTINGS_BACKEND_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SETTINGS_BACKEND, MdmSettingsBackendClass))

typedef struct MdmSettingsBackendPrivate MdmSettingsBackendPrivate;

typedef struct
{
        GObject             parent;
        MdmSettingsBackendPrivate *priv;
} MdmSettingsBackend;

typedef struct
{
        GObjectClass   parent_class;

        /* methods */
        gboolean (*get_value)              (MdmSettingsBackend *settings_backend,
                                            const char         *key,
                                            char              **value,
                                            GError            **error);
        gboolean (*set_value)              (MdmSettingsBackend *settings_backend,
                                            const char         *key,
                                            const char         *value,
                                            GError            **error);

        /* signals */
        void          (* value_changed)    (MdmSettingsBackend *settings_backend,
                                            const char  *key,
                                            const char  *old_value,
                                            const char **new_value);
} MdmSettingsBackendClass;

typedef enum
{
        MDM_SETTINGS_BACKEND_ERROR_GENERAL,
        MDM_SETTINGS_BACKEND_ERROR_KEY_NOT_FOUND
} MdmSettingsBackendError;

#define MDM_SETTINGS_BACKEND_ERROR mdm_settings_backend_error_quark ()

GQuark              mdm_settings_backend_error_quark            (void);
GType               mdm_settings_backend_get_type               (void);

gboolean            mdm_settings_backend_get_value              (MdmSettingsBackend *settings_backend,
                                                                 const char  *key,
                                                                 char       **value,
                                                                 GError     **error);
gboolean            mdm_settings_backend_set_value              (MdmSettingsBackend *settings_backend,
                                                                 const char  *key,
                                                                 const char  *value,
                                                                 GError     **error);

void                mdm_settings_backend_value_changed          (MdmSettingsBackend *settings_backend,
                                                                 const char  *key,
                                                                 const char  *old_value,
                                                                 const char  *new_value);

G_END_DECLS

#endif /* __MDM_SETTINGS_BACKEND_H */
