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


#ifndef __GDM_SETTINGS_DESKTOP_BACKEND_H
#define __GDM_SETTINGS_DESKTOP_BACKEND_H

#include <glib-object.h>
#include "gdm-settings-backend.h"

G_BEGIN_DECLS

#define GDM_TYPE_SETTINGS_DESKTOP_BACKEND         (gdm_settings_desktop_backend_get_type ())
#define GDM_SETTINGS_DESKTOP_BACKEND(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SETTINGS_DESKTOP_BACKEND, GdmSettingsDesktopBackend))
#define GDM_SETTINGS_DESKTOP_BACKEND_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SETTINGS_DESKTOP_BACKEND, GdmSettingsDesktopBackendClass))
#define GDM_IS_SETTINGS_DESKTOP_BACKEND(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SETTINGS_DESKTOP_BACKEND))
#define GDM_IS_SETTINGS_DESKTOP_BACKEND_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_SETTINGS_DESKTOP_BACKEND))
#define GDM_SETTINGS_DESKTOP_BACKEND_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_SETTINGS_DESKTOP_BACKEND, GdmSettingsDesktopBackendClass))

typedef struct GdmSettingsDesktopBackendPrivate GdmSettingsDesktopBackendPrivate;

typedef struct
{
        GdmSettingsBackend                parent;
        GdmSettingsDesktopBackendPrivate *priv;
} GdmSettingsDesktopBackend;

typedef struct
{
        GdmSettingsBackendClass   parent_class;
} GdmSettingsDesktopBackendClass;

GType                      gdm_settings_desktop_backend_get_type        (void);

GdmSettingsBackend        *gdm_settings_desktop_backend_new             (void);

G_END_DECLS

#endif /* __GDM_SETTINGS_DESKTOP_BACKEND_H */
