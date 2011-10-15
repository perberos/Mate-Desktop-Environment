/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 - Paolo Maggi
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

#ifndef MATE_SETTINGS_MODULE_H
#define MATE_SETTINGS_MODULE_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_SETTINGS_MODULE               (mate_settings_module_get_type ())
#define MATE_SETTINGS_MODULE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_SETTINGS_MODULE, MateSettingsModule))
#define MATE_SETTINGS_MODULE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_SETTINGS_MODULE, MateSettingsModuleClass))
#define MATE_IS_SETTINGS_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_SETTINGS_MODULE))
#define MATE_IS_SETTINGS_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((obj), MATE_TYPE_SETTINGS_MODULE))
#define MATE_SETTINGS_MODULE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj), MATE_TYPE_SETTINGS_MODULE, MateSettingsModuleClass))

typedef struct _MateSettingsModule MateSettingsModule;

GType                    mate_settings_module_get_type          (void) G_GNUC_CONST;

MateSettingsModule     *mate_settings_module_new               (const gchar *path);

const char              *mate_settings_module_get_path          (MateSettingsModule *module);

GObject                 *mate_settings_module_new_object        (MateSettingsModule *module);

#ifdef __cplusplus
}
#endif

#endif
