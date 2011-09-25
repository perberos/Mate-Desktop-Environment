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


#ifndef __MDM_SETTINGS_DESKTOP_BACKEND_H
#define __MDM_SETTINGS_DESKTOP_BACKEND_H

#include <glib-object.h>
#include "mdm-settings-backend.h"

G_BEGIN_DECLS

#define MDM_TYPE_SETTINGS_DESKTOP_BACKEND         (mdm_settings_desktop_backend_get_type ())
#define MDM_SETTINGS_DESKTOP_BACKEND(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SETTINGS_DESKTOP_BACKEND, MdmSettingsDesktopBackend))
#define MDM_SETTINGS_DESKTOP_BACKEND_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SETTINGS_DESKTOP_BACKEND, MdmSettingsDesktopBackendClass))
#define MDM_IS_SETTINGS_DESKTOP_BACKEND(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SETTINGS_DESKTOP_BACKEND))
#define MDM_IS_SETTINGS_DESKTOP_BACKEND_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SETTINGS_DESKTOP_BACKEND))
#define MDM_SETTINGS_DESKTOP_BACKEND_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SETTINGS_DESKTOP_BACKEND, MdmSettingsDesktopBackendClass))

typedef struct MdmSettingsDesktopBackendPrivate MdmSettingsDesktopBackendPrivate;

typedef struct
{
        MdmSettingsBackend                parent;
        MdmSettingsDesktopBackendPrivate *priv;
} MdmSettingsDesktopBackend;

typedef struct
{
        MdmSettingsBackendClass   parent_class;
} MdmSettingsDesktopBackendClass;

GType                      mdm_settings_desktop_backend_get_type        (void);

MdmSettingsBackend        *mdm_settings_desktop_backend_new             (void);

G_END_DECLS

#endif /* __MDM_SETTINGS_DESKTOP_BACKEND_H */
