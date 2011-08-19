/* MateConf
 * Copyright (C) 1999, 2000, 2002 Red Hat Inc.
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

#include "mateconf-schema.h"
#include "mateconf-internals.h"

/* FIXME clean this up, obviously RealSchema isn't needed. */
struct _MateConfSchema {
  int dummy;
};

typedef struct {
  MateConfValueType type; /* Type of the described entry */
  MateConfValueType list_type; /* List type of the described entry */
  MateConfValueType car_type; /* Pair car type of the described entry */
  MateConfValueType cdr_type; /* Pair cdr type of the described entry */
  gchar* locale;       /* Schema locale */
  gchar* owner;        /* Name of creating application */
  gchar* short_desc;   /* 40 char or less description, no newlines */
  gchar* long_desc;    /* could be a paragraph or so */
  MateConfValue* default_value; /* Default value of the key */
} MateConfRealSchema;

#define REAL_SCHEMA(schema) ((MateConfRealSchema*)(schema))

MateConfSchema*  
mateconf_schema_new (void)
{
  MateConfRealSchema *real;

  real = g_new0 (MateConfRealSchema, 1);

  real->type = MATECONF_VALUE_INVALID;
  real->list_type = MATECONF_VALUE_INVALID;
  real->car_type = MATECONF_VALUE_INVALID;
  real->cdr_type = MATECONF_VALUE_INVALID;

  return (MateConfSchema*) real;
}

void          
mateconf_schema_free (MateConfSchema* sc)
{
  MateConfRealSchema *real = REAL_SCHEMA (sc);
  
  if (real->locale)
    g_free (real->locale);

  if (real->short_desc)
    g_free (real->short_desc);

  if (real->long_desc)
    g_free (real->long_desc);

  if (real->owner)
    g_free (real->owner);

  if (real->default_value)
    mateconf_value_free (real->default_value);
  
  g_free (sc);
}

MateConfSchema*  
mateconf_schema_copy (const MateConfSchema* sc)
{
  MateConfRealSchema *dest;
  const MateConfRealSchema *real;

  real = REAL_SCHEMA (sc);
  dest = (MateConfRealSchema*) mateconf_schema_new ();

  dest->type = real->type;
  dest->list_type = real->list_type;
  dest->car_type = real->car_type;
  dest->cdr_type = real->cdr_type;

  dest->locale = real->locale ? g_strdup (real->locale) : NULL;
  
  dest->short_desc = real->short_desc ? g_strdup (real->short_desc) : NULL;

  dest->long_desc = real->long_desc ? g_strdup (real->long_desc) : NULL;

  dest->owner = real->owner ? g_strdup (real->owner) : NULL;

  dest->default_value = real->default_value ? mateconf_value_copy (real->default_value) : NULL;
  
  return (MateConfSchema*) dest;
}

void          
mateconf_schema_set_type (MateConfSchema* sc, MateConfValueType type)
{
  REAL_SCHEMA (sc)->type = type;
}

void          
mateconf_schema_set_list_type (MateConfSchema* sc, MateConfValueType type)
{
  REAL_SCHEMA (sc)->list_type = type;
}

void          
mateconf_schema_set_car_type (MateConfSchema* sc, MateConfValueType type)
{
  REAL_SCHEMA (sc)->car_type = type;
}

void          
mateconf_schema_set_cdr_type (MateConfSchema* sc, MateConfValueType type)
{
  REAL_SCHEMA (sc)->cdr_type = type;
}

void
mateconf_schema_set_locale (MateConfSchema* sc, const gchar* locale)
{
  g_return_if_fail (locale == NULL || g_utf8_validate (locale, -1, NULL));
  
  if (REAL_SCHEMA (sc)->locale)
    g_free (REAL_SCHEMA (sc)->locale);

  if (locale)
    REAL_SCHEMA (sc)->locale = g_strdup (locale);
  else 
    REAL_SCHEMA (sc)->locale = NULL;
}

void          
mateconf_schema_set_short_desc (MateConfSchema* sc, const gchar* desc)
{
  g_return_if_fail (desc == NULL || g_utf8_validate (desc, -1, NULL));
  
  if (REAL_SCHEMA (sc)->short_desc)
    g_free (REAL_SCHEMA (sc)->short_desc);

  if (desc)
    REAL_SCHEMA (sc)->short_desc = g_strdup (desc);
  else 
    REAL_SCHEMA (sc)->short_desc = NULL;
}

void          
mateconf_schema_set_long_desc (MateConfSchema* sc, const gchar* desc)
{
  g_return_if_fail (desc == NULL || g_utf8_validate (desc, -1, NULL));
  
  if (REAL_SCHEMA (sc)->long_desc)
    g_free (REAL_SCHEMA (sc)->long_desc);

  if (desc)
    REAL_SCHEMA (sc)->long_desc = g_strdup (desc);
  else 
    REAL_SCHEMA (sc)->long_desc = NULL;
}

void          
mateconf_schema_set_owner (MateConfSchema* sc, const gchar* owner)
{
  g_return_if_fail (owner == NULL || g_utf8_validate (owner, -1, NULL));
  
  if (REAL_SCHEMA (sc)->owner)
    g_free (REAL_SCHEMA (sc)->owner);

  if (owner)
    REAL_SCHEMA (sc)->owner = g_strdup (owner);
  else
    REAL_SCHEMA (sc)->owner = NULL;
}

void
mateconf_schema_set_default_value (MateConfSchema* sc, const MateConfValue* val)
{
  if (REAL_SCHEMA (sc)->default_value != NULL)
    mateconf_value_free (REAL_SCHEMA (sc)->default_value);

  REAL_SCHEMA (sc)->default_value = mateconf_value_copy (val);
}

void
mateconf_schema_set_default_value_nocopy (MateConfSchema* sc, MateConfValue* val)
{
  if (REAL_SCHEMA (sc)->default_value != NULL)
    mateconf_value_free (REAL_SCHEMA (sc)->default_value);

  REAL_SCHEMA (sc)->default_value = val;
}

gboolean
mateconf_schema_validate (const MateConfSchema *sc,
                       GError           **err)
{
  MateConfRealSchema *real;

  real = REAL_SCHEMA (sc);
  
  if (real->locale && !g_utf8_validate (real->locale, -1, NULL))
    {
      g_set_error (err, MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Schema contains invalid UTF-8"));
      return FALSE;
    }

  if (real->short_desc && !g_utf8_validate (real->short_desc, -1, NULL))
    {
      g_set_error (err, MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Schema contains invalid UTF-8"));
      return FALSE;
    }

  if (real->long_desc && !g_utf8_validate (real->long_desc, -1, NULL))
    {
      g_set_error (err, MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Schema contains invalid UTF-8"));
      return FALSE;
    }

  if (real->owner && !g_utf8_validate (real->owner, -1, NULL))
    {
      g_set_error (err, MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Schema contains invalid UTF-8"));
      return FALSE;
    }

  if (real->type == MATECONF_VALUE_LIST &&
      real->list_type == MATECONF_VALUE_INVALID)
    {
      g_set_error (err, MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Schema specifies type list but doesn't specify the type of the list elements"));
      return FALSE;
    }
  
  if (real->type == MATECONF_VALUE_PAIR &&
      (real->car_type == MATECONF_VALUE_INVALID ||
       real->cdr_type == MATECONF_VALUE_INVALID))
    {
      g_set_error (err, MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Schema specifies type pair but doesn't specify the type of the car/cdr elements"));
      return FALSE;
    }
  
  return TRUE;
}

MateConfValueType
mateconf_schema_get_type (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, MATECONF_VALUE_INVALID);

  return REAL_SCHEMA (schema)->type;
}

MateConfValueType
mateconf_schema_get_list_type (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, MATECONF_VALUE_INVALID);

  return REAL_SCHEMA (schema)->list_type;
}

MateConfValueType
mateconf_schema_get_car_type (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, MATECONF_VALUE_INVALID);

  return REAL_SCHEMA (schema)->car_type;
}

MateConfValueType
mateconf_schema_get_cdr_type (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, MATECONF_VALUE_INVALID);

  return REAL_SCHEMA (schema)->cdr_type;
}

const char*
mateconf_schema_get_locale (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, NULL);

  return REAL_SCHEMA (schema)->locale;
}

const char*
mateconf_schema_get_short_desc (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, NULL);

  return REAL_SCHEMA (schema)->short_desc;
}

const char*
mateconf_schema_get_long_desc (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, NULL);

  return REAL_SCHEMA (schema)->long_desc;
}

const char*
mateconf_schema_get_owner (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, NULL);

  return REAL_SCHEMA (schema)->owner;
}

MateConfValue*
mateconf_schema_get_default_value (const MateConfSchema *schema)
{
  g_return_val_if_fail (schema != NULL, NULL);

  return REAL_SCHEMA (schema)->default_value;
}

MateConfValue*
mateconf_schema_steal_default_value (MateConfSchema *schema)
{
  MateConfValue *val;
  
  g_return_val_if_fail (schema != NULL, NULL);

  val = REAL_SCHEMA (schema)->default_value;
  REAL_SCHEMA (schema)->default_value = NULL;

  return val;
}
