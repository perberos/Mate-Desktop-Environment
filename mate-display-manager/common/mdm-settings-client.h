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


#ifndef __MDM_SETTINGS_CLIENT_H
#define __MDM_SETTINGS_CLIENT_H

#include <glib-object.h>
#include "mdm-settings-utils.h"

G_BEGIN_DECLS

typedef void        (*MdmSettingsClientNotifyFunc)        (guint             id,
                                                           MdmSettingsEntry *entry,
                                                           gpointer          user_data);

gboolean              mdm_settings_client_init                       (const char                 *schemas_file,
                                                                      const char                 *root);
void                  mdm_settings_client_shutdown                   (void);

gboolean              mdm_settings_client_get_int                    (const char                 *key,
                                                                      int                        *value);
gboolean              mdm_settings_client_get_boolean                (const char                 *key,
                                                                      gboolean                   *value);
gboolean              mdm_settings_client_get_string                 (const char                 *key,
                                                                      char                      **value);
gboolean              mdm_settings_client_get_locale_string          (const char                 *key,
                                                                      const char                 *locale,
                                                                      char                      **value);

gboolean              mdm_settings_client_set_int                    (const char                 *key,
                                                                      int                         value);
gboolean              mdm_settings_client_set_boolean                (const char                 *key,
                                                                      gboolean                    value);
gboolean              mdm_settings_client_set_string                 (const char                 *key,
                                                                      const char                 *value);

guint                 mdm_settings_client_notify_add                 (const char                 *namespace_section,
                                                                      MdmSettingsClientNotifyFunc func,
                                                                      gpointer                    user_data,
                                                                      GFreeFunc                   destroy_notify);
void                  mdm_settings_client_notify_remove              (guint                       id);

G_END_DECLS

#endif /* __MDM_SETTINGS_CLIENT_H */
