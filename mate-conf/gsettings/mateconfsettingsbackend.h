/*
 * Copyright (C) 2010 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Vincent Untz <vuntz@mate.org>
 */

#ifndef _mateconfsettingsbackend_h_
#define _mateconfsettingsbackend_h_

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

G_BEGIN_DECLS

#define MATECONF_TYPE_SETTINGS_BACKEND                   (mateconf_settings_backend_get_type ())
#define MATECONF_SETTINGS_BACKEND(inst)                  (G_TYPE_CHECK_INSTANCE_CAST ((inst),    \
                                                       MATECONF_TYPE_SETTINGS_BACKEND,           \
                                                       MateConfSettingsBackend))
#define MATECONF_SETTINGS_BACKEND_CLASS(class)           (G_TYPE_CHECK_CLASS_CAST ((class),      \
                                                       MATECONF_TYPE_SETTINGS_BACKEND,           \
                                                       MateConfSettingsBackendClass))
#define MATECONF_IS_SETTINGS_BACKEND(inst)               (G_TYPE_CHECK_INSTANCE_TYPE ((inst),    \
                                                       MATECONF_TYPE_SETTINGS_BACKEND))
#define MATECONF_IS_SETTINGS_BACKEND_CLASS(class)        (G_TYPE_CHECK_CLASS_TYPE ((class),      \
                                                       MATECONF_TYPE_SETTINGS_BACKEND))
#define MATECONF_SETTINGS_BACKEND_GET_CLASS(inst)        (G_TYPE_INSTANCE_GET_CLASS ((inst),     \
                                                       MATECONF_TYPE_SETTINGS_BACKEND,           \
                                                       MateConfSettingsBackendClass))

/**
 * MateConfSettingsBackend:
 *
 * A backend to GSettings that stores the settings in mateconf.
 **/
typedef struct _MateConfSettingsBackendPrivate               MateConfSettingsBackendPrivate;
typedef struct _MateConfSettingsBackendClass                 MateConfSettingsBackendClass;
typedef struct _MateConfSettingsBackend                      MateConfSettingsBackend;

struct _MateConfSettingsBackendClass
{
  GSettingsBackendClass parent_class;
};

struct _MateConfSettingsBackend
{
  GSettingsBackend parent_instance;

  /*< private >*/
  MateConfSettingsBackendPrivate *priv;
};

GType mateconf_settings_backend_get_type (void);
void  mateconf_settings_backend_register (GIOModule *module);

G_END_DECLS

#endif /* _mateconfsettingsbackend_h_ */
