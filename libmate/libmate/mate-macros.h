/* mate-macros.h
 *   Macros for making GObject objects to avoid typos and reduce code size
 * Copyright (C) 2000  Eazel, Inc.
 *
 * Authors: George Lebl <jirka@5z.com>
 *
 * All rights reserved.
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
/*
  @NOTATION@
*/

#ifndef MATE_MACROS_H
#define MATE_MACROS_H

#ifndef MATE_DISABLE_DEPRECATED

#include <matecomponent/matecomponent-macros.h>

/* Macros for defining classes.  Ideas taken from Caja and GOB. */

/* Define the boilerplate type stuff to reduce typos and code size.  Defines
 * the get_type method and the parent_class static variable. */
#define MATE_CLASS_BOILERPLATE(type, type_as_function,			\
				parent_type, parent_type_macro)		\
	MATECOMPONENT_BOILERPLATE(type, type_as_function, type,		\
			  parent_type, parent_type_macro,		\
			  MATE_REGISTER_TYPE)
#define MATE_REGISTER_TYPE(type, type_as_function, corba_type,		\
			    parent_type, parent_type_macro)		\
	g_type_register_static (parent_type_macro, #type, &object_info, 0)

/* Just call the parent handler.  This assumes that there is a variable
 * named parent_class that points to the (duh!) parent class.  Note that
 * this macro is not to be used with things that return something, use
 * the _WITH_DEFAULT version for that */
#define MATE_CALL_PARENT(parent_class_cast, name, args)		\
	MATECOMPONENT_CALL_PARENT (parent_class_cast, name, args)

/* Same as above, but in case there is no implementation, it evaluates
 * to def_return */
#define MATE_CALL_PARENT_WITH_DEFAULT(parent_class_cast,		\
				       name, args, def_return)		\
	MATECOMPONENT_CALL_PARENT_WITH_DEFAULT (				\
		parent_class_cast, name, args, def_return)

#endif /* !MATE_DISABLE_DEPRECATED */

#endif /* MATE_MACROS_H */
