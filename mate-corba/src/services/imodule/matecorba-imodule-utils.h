/*
 * matecorba-imodule-utils.h:
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 *                    Ximian, Inc.
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
 *
 * Authors:
 *	Mark McLoughlin <mark@skynet.ie>
 */
#ifndef __MATECORBA_IMODULE_UTILS_H__
#define __MATECORBA_IMODULE_UTILS_H__

#include <glib.h>
#include <libIDL/IDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	IDL_tree get_op;
	IDL_tree set_op;
} MateCORBA_imodule_fakeops;

GHashTable      *MateCORBA_imodule_new_typecodes         (void);
void             MateCORBA_imodule_free_typecodes        (GHashTable     *typecodes);

CORBA_sequence_CORBA_TypeCode *
                 MateCORBA_imodule_get_typecodes_seq     (GHashTable     *typecodes);


CORBA_TypeCode   MateCORBA_imodule_get_typecode          (GHashTable     *typecodes,
						      IDL_tree        tree);
CORBA_TypeCode   MateCORBA_imodule_create_alias_typecode (GHashTable     *typecodes,
						      IDL_tree        tree,
						      CORBA_TypeCode  original_type);

IDL_tree         MateCORBA_imodule_get_typespec          (IDL_tree        tree);
gboolean         MateCORBA_imodule_type_is_fixed_length  (IDL_tree        tree);
void             MateCORBA_imodule_traverse_parents      (IDL_tree        tree,
						      GFunc           callback,
						      gpointer        user_data);

#ifdef __cplusplus
}
#endif

#endif /* __MATECORBA_IMODULE_UTILS_H__ */
