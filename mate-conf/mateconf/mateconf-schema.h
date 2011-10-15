
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef MATECONF_MATECONF_SCHEMA_H
#define MATECONF_MATECONF_SCHEMA_H

#include <glib.h>

#include "mateconf/mateconf-value.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Skipped from introspection because it's not registered as boxed */
/**
 * MateConfSchema: (skip)
 *
 * An opaque data type representing a description of a key-value pair.
 */

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
void mateconf_schema_set_gettext_domain       (MateConfSchema    *sc,
                                            const gchar    *domain);
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
const char*    mateconf_schema_get_gettext_domain(const MateConfSchema *schema);
const char*    mateconf_schema_get_short_desc    (const MateConfSchema *schema);
const char*    mateconf_schema_get_long_desc     (const MateConfSchema *schema);
const char*    mateconf_schema_get_owner         (const MateConfSchema *schema);
MateConfValue*    mateconf_schema_get_default_value (const MateConfSchema *schema);


G_END_DECLS

#endif


