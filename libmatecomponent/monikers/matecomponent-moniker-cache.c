/*
 * matecomponent-moniker-cache.c: 
 *
 * Author:
 *	Dietmar Maurer (dietmar@helixcode.com)
 *
 * Copyright 2000 Ximian, Inc.
 */
#include <config.h>
#include <string.h>
#include <matecomponent/matecomponent-exception.h>

#include "matecomponent-moniker-std.h"
#include "matecomponent-stream-cache.h"

MateComponent_Unknown
matecomponent_moniker_cache_resolve (MateComponentMoniker               *moniker,
			      const MateComponent_ResolveOptions *options,
			      const CORBA_char            *requested_interface,
			      CORBA_Environment           *ev)
{
	MateComponent_Moniker parent;
	MateComponentStream *stream;
	MateComponent_Stream in_stream;

	if (!strcmp (requested_interface, "IDL:MateComponent/Stream:1.0")) {

		parent = matecomponent_moniker_get_parent (moniker, ev);

		if (MATECOMPONENT_EX (ev) || parent == CORBA_OBJECT_NIL)
			return CORBA_OBJECT_NIL;
	
		in_stream = MateComponent_Moniker_resolve (parent, options, 
						    "IDL:MateComponent/Stream:1.0",
						    ev);

		if (MATECOMPONENT_EX (ev) || in_stream == CORBA_OBJECT_NIL) {
			matecomponent_object_release_unref (parent, NULL);
			return CORBA_OBJECT_NIL;
		}

		matecomponent_object_release_unref (parent, ev);

		if (MATECOMPONENT_EX (ev))
			return CORBA_OBJECT_NIL;
	       
		stream = matecomponent_stream_cache_create (in_stream, ev);

		if (MATECOMPONENT_EX (ev) || stream == CORBA_OBJECT_NIL) {
			matecomponent_object_release_unref (in_stream, NULL);
			return CORBA_OBJECT_NIL;
		}

		matecomponent_object_release_unref (in_stream, ev);

		if (MATECOMPONENT_EX (ev))
			return CORBA_OBJECT_NIL;
	      
		return CORBA_Object_duplicate (MATECOMPONENT_OBJREF (stream), ev);
	}

	return CORBA_OBJECT_NIL; /* use the extender */
}
