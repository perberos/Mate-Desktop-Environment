/* gdict-private.h - Private definitions for Gdict
 *
 * Copyright (C) 2005  Emmanuele Bassi <ebassi@gmail.com>
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
 */

#ifndef __GDICT_PRIVATE_H__
#define __GDICT_PRIVATE_H__

#ifndef GDICT_ENABLE_INTERNALS
#error "You are trying to access Gdict's internals outside Gdict.  The API of these internal functions is not fixed."
#endif

#include <glib-object.h>

#include "gdict-context.h"

G_BEGIN_DECLS

/* boilerplate code, similar to G_DEFINE_TYPE in spirit, used to define
 * our boxed types and their ref/unref functions; you still have to
 * implement your own ref/unref functions!
 */
#define GDICT_DEFINE_BOXED_TYPE(TypeName,type_name)	\
\
static gpointer type_name##_intern_ref (gpointer self) \
{ \
  return type_name##_ref ((TypeName *) self); \
} \
static void type_name##_intern_unref (gpointer self) \
{ \
  type_name##_unref ((TypeName *) self); \
} \
\
GType \
type_name##_get_type (void) \
{ \
  static GType gdict_define_boxed_type = 0; \
  if (G_UNLIKELY (gdict_define_boxed_type == 0)) \
    gdict_define_boxed_type = g_boxed_type_register_static (#TypeName, (GBoxedCopyFunc) type_name##_intern_ref, (GBoxedFreeFunc) type_name##_intern_unref); \
  return gdict_define_boxed_type; \
}

/* Never, _ever_ access the members of these structures, unless you
 * know what you are doing.
 */

struct _GdictDatabase
{
  gchar *name;
  gchar *full_name;
  
  guint ref_count;
};

struct _GdictStrategy
{
  gchar *name;
  gchar *description;
  
  guint ref_count;
};

struct _GdictMatch
{
  gchar *database;
  gchar *word;
  
  guint ref_count;
};

struct _GdictDefinition
{
  gint total;
  
  gchar *word;
  gchar *database_name;
  gchar *database_full;
  gchar *definition;
  
  guint ref_count;
};

/* constructors for these objects are private, as the world outside do
 * not know what they hold, and accessor functions are getters only
 */
GdictDatabase *   _gdict_database_new   (const gchar *name);
GdictStrategy *   _gdict_strategy_new   (const gchar *name);
GdictMatch *      _gdict_match_new      (const gchar *word);
GdictDefinition * _gdict_definition_new (gint         total);

G_END_DECLS

#endif /* __GDICT_PRIVATE_H__ */
