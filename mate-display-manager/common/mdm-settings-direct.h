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


#ifndef __MDM_SETTINGS_DIRECT_H
#define __MDM_SETTINGS_DIRECT_H

#include <glib-object.h>
#include "mdm-settings.h"

G_BEGIN_DECLS

gboolean              mdm_settings_direct_init                       (MdmSettings       *settings,
                                                                      const char        *schemas_file,
                                                                      const char        *root);
void                  mdm_settings_direct_shutdown                   (void);

gboolean              mdm_settings_direct_get                        (const char        *key,
                                                                      GValue            *value);
gboolean              mdm_settings_direct_set                        (const char        *key,
                                                                      GValue            *value);
gboolean              mdm_settings_direct_get_int                    (const char        *key,
                                                                      int               *value);
gboolean              mdm_settings_direct_get_uint                   (const char        *key,
                                                                      uint              *value);
gboolean              mdm_settings_direct_get_boolean                (const char        *key,
                                                                      gboolean          *value);
gboolean              mdm_settings_direct_get_string                 (const char        *key,
                                                                      char             **value);

G_END_DECLS

#endif /* __MDM_SETTINGS_DIRECT_H */
