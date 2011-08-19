/*
 * mate-moniker-query.c: Sample query-activation based Moniker implementation
 *
 * This is the Oaf query based Moniker implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 */
#include <config.h>
#include <string.h>
#include <matecomponent/matecomponent-moniker.h>
#include <matecomponent/matecomponent-moniker-util.h>

#include "matecomponent-moniker-std.h"

MateComponent_Unknown
matecomponent_moniker_query_resolve (MateComponentMoniker               *moniker,
			      const MateComponent_ResolveOptions *options,
			      const CORBA_char            *requested_interface,
			      CORBA_Environment           *ev)
{
	MateComponent_Moniker       parent;
	MateComponent_Unknown       object;
	char                *query;
	
	parent = matecomponent_moniker_get_parent (moniker, ev);

	if (ev->_major != CORBA_NO_EXCEPTION)
		return CORBA_OBJECT_NIL;
	
	if (parent != CORBA_OBJECT_NIL) {
		matecomponent_object_release_unref (parent, ev);

#ifdef G_ENABLE_DEBUG
		g_warning ("wierd; queryied moniker with a parent; strange");
#endif
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_Moniker_InterfaceNotFound, NULL);
		return CORBA_OBJECT_NIL;
	}

	query = g_strdup_printf ("%s AND repo_ids.has ('%s')",
				 matecomponent_moniker_get_name (moniker),
				 requested_interface);

	object = matecomponent_activation_activate (query, NULL, 0, NULL, ev);

	g_free (query);

	return matecomponent_moniker_util_qi_return (object, requested_interface, ev);
}
