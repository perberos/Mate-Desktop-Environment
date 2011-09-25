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


#ifndef __MDM_SETTINGS_UTILS_H
#define __MDM_SETTINGS_UTILS_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _MdmSettingsEntry MdmSettingsEntry;

MdmSettingsEntry *        mdm_settings_entry_new               (void);
MdmSettingsEntry *        mdm_settings_entry_copy              (MdmSettingsEntry *entry);
void                      mdm_settings_entry_free              (MdmSettingsEntry *entry);

const char *              mdm_settings_entry_get_key           (MdmSettingsEntry *entry);
const char *              mdm_settings_entry_get_signature     (MdmSettingsEntry *entry);
const char *              mdm_settings_entry_get_default_value (MdmSettingsEntry *entry);
const char *              mdm_settings_entry_get_value         (MdmSettingsEntry *entry);

void                      mdm_settings_entry_set_value         (MdmSettingsEntry *entry,
                                                                const char       *value);

gboolean                  mdm_settings_parse_schemas           (const char  *file,
                                                                const char  *root,
                                                                GSList     **list);

gboolean                  mdm_settings_parse_value_as_boolean  (const char *value,
                                                                gboolean   *bool);
gboolean                  mdm_settings_parse_value_as_integer  (const char *value,
                                                                int        *intval);
gboolean                  mdm_settings_parse_value_as_double   (const char *value,
                                                                gdouble    *doubleval);

char *                    mdm_settings_parse_boolean_as_value  (gboolean    boolval);
char *                    mdm_settings_parse_integer_as_value  (int         intval);
char *                    mdm_settings_parse_double_as_value   (gdouble     doubleval);


G_END_DECLS

#endif /* __MDM_SETTINGS_UTILS_H */
