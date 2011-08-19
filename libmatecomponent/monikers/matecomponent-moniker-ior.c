/*
 * mate-moniker-ior.c: Sample ior-system based Moniker implementation
 *
 * This is the ior (container) based Moniker implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 */
#include <config.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker-util.h>

#include "matecomponent-moniker-std.h"

MateComponent_Unknown
matecomponent_moniker_ior_resolve (MateComponentMoniker               *moniker,
			    const MateComponent_ResolveOptions *options,
			    const CORBA_char            *requested_interface,
			    CORBA_Environment           *ev)
{
	const char    *ior;
	CORBA_Object   object;
	MateComponent_Unknown retval;
	gboolean       is_unknown, is_correct;

	ior = matecomponent_moniker_get_name (moniker);
	
	object = CORBA_ORB_string_to_object (matecomponent_orb (), ior, ev);
	MATECOMPONENT_RET_VAL_EX (ev, CORBA_OBJECT_NIL);

	is_unknown = CORBA_Object_is_a (object, "IDL:MateComponent/Unknown:1.0", ev);
	MATECOMPONENT_RET_VAL_EX (ev, CORBA_OBJECT_NIL);

	if (!is_unknown) {
		is_correct = CORBA_Object_is_a (object, requested_interface, ev);
		MATECOMPONENT_RET_VAL_EX (ev, CORBA_OBJECT_NIL);

		if (is_correct)
			return object;
		else {
			CORBA_exception_set (
				ev, CORBA_USER_EXCEPTION,
				ex_MateComponent_Moniker_InterfaceNotFound, NULL);
			return CORBA_OBJECT_NIL;
		}
	}

	retval = MateComponent_Unknown_queryInterface (
		object, requested_interface, ev);
	MATECOMPONENT_RET_VAL_EX (ev, CORBA_OBJECT_NIL);
	
	if (retval == CORBA_OBJECT_NIL)
		CORBA_exception_set (
			ev, CORBA_USER_EXCEPTION,
			ex_MateComponent_Moniker_InterfaceNotFound, NULL);

	return retval;
}
