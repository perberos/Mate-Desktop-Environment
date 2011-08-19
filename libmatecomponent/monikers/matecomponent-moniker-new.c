/*
 * mate-moniker-new.c: Sample generic factory 'new'
 * Moniker implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, Ximian, Inc.
 */
#include <config.h>

#include <matecomponent/matecomponent-moniker-util.h>

#include "matecomponent-moniker-std.h"

MateComponent_Unknown
matecomponent_moniker_new_resolve (MateComponentMoniker               *moniker,
			    const MateComponent_ResolveOptions *options,
			    const CORBA_char            *requested_interface,
			    CORBA_Environment           *ev)
{
	MateComponent_Moniker        parent;
	MateComponent_GenericFactory factory;
	MateComponent_Unknown        containee;
	MateComponent_Unknown        retval = CORBA_OBJECT_NIL;
	
	parent = matecomponent_moniker_get_parent (moniker, ev);

	if (ev->_major != CORBA_NO_EXCEPTION)
		return CORBA_OBJECT_NIL;

	g_assert (parent != CORBA_OBJECT_NIL);

	factory = MateComponent_Moniker_resolve (parent, options,
					  "IDL:MateComponent/GenericFactory:1.0", ev);

	if (ev->_major != CORBA_NO_EXCEPTION)
		goto return_unref_parent;

	if (factory == CORBA_OBJECT_NIL) {
#ifdef G_ENABLE_DEBUG
		g_warning ("Failed to extract a factory from our parent");
#endif
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_Moniker_InterfaceNotFound, NULL);
		goto return_unref_parent;
	}

	containee = MateComponent_GenericFactory_createObject (
		factory, requested_interface, ev);

	matecomponent_object_release_unref (factory, ev);

	return matecomponent_moniker_util_qi_return (containee, requested_interface, ev);

 return_unref_parent:
	matecomponent_object_release_unref (parent, ev);

	return retval;
}
