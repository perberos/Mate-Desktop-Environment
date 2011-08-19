
/* MateConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATECONF_MATECONF_SCHEMA_H
#define MATECONF_MATECONF_SCHEMA_H

#include <glib.h>

#include <mateconf/mateconf-value.h>

G_BEGIN_DECLS

/*
 *  A "schema" is a value that describes a key-value pair.
 *  It might include the type of the pair, documentation describing 
 *  the pair, the name of the application creating the pair, 
 *  etc.
 */

MateConfSchema* mateconf_schema_new  (void);
void         mateconf_schema_free (MateConfSchema *sc);
MateConfSchema* mateconf_schema_copy (const MateConfSchema *sc);

void mateconf_schema_set_type                 (MateConfSchema    *sc,
                                            MateConfValueType  type);
void mateconf_schema_set_list_type            (MateConfSchema    *sc,
                                            MateConfValueType  type);
void mateconf_schema_set_car_type             (MateConfSchema    *sc,
                                            MateConfValueType  type);
void mateconf_schema_set_cdr_type             (MateConfSchema    *sc,
                                            MateConfValueType  type);
void mateconf_schema_set_locale               (MateConfSchema    *sc,
                                            const gchar    *locale);
void mateconf_schema_set_short_desc           (MateConfSchema    *sc,
                                            const gchar    *desc);
void mateconf_schema_set_long_desc            (MateConfSchema    *sc,
                                            const gchar    *desc);
void mateconf_schema_set_owner                (MateConfSchema    *sc,
                                            const gchar    *owner);
void mateconf_schema_set_default_value        (MateConfSchema    *sc,
                                            const MateConfValue     *val);
void mateconf_schema_set_default_value_nocopy (MateConfSchema    *sc,
                                            MateConfValue     *val);


MateConfValueType mateconf_schema_get_type          (const MateConfSchema *schema);
MateConfValueType mateconf_schema_get_list_type     (const MateConfSchema *schema);
MateConfValueType mateconf_schema_get_car_type      (const MateConfSchema *schema);
MateConfValueType mateconf_schema_get_cdr_type      (const MateConfSchema *schema);
const char*    mateconf_schema_get_locale        (const MateConfSchema *schema);
const char*    mateconf_schema_get_short_desc    (const MateConfSchema *schema);
const char*    mateconf_schema_get_long_desc     (const MateConfSchema *schema);
const char*    mateconf_schema_get_owner         (const MateConfSchema *schema);
MateConfValue*    mateconf_schema_get_default_value (const MateConfSchema *schema);


G_END_DECLS

#endif


