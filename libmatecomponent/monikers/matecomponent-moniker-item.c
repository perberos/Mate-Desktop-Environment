/*
 * mate-moniker-item.c: Sample item-system based Moniker implementation
 *
 * This is the item (container) based Moniker implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 */
#include <config.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker-util.h>

#include "matecomponent-moniker-std.h"

MateComponent_Unknown
matecomponent_moniker_item_resolve (MateComponentMoniker               *moniker,
			     const MateComponent_ResolveOptions *options,
			     const CORBA_char            *requested_interface,
			     CORBA_Environment           *ev)
{
	MateComponent_Moniker       parent;
	MateComponent_ItemContainer container;
	MateComponent_Unknown       containee;
	MateComponent_Unknown       retval = CORBA_OBJECT_NIL;
	
	parent = matecomponent_moniker_get_parent (moniker, ev);

	if (MATECOMPONENT_EX (ev))
		return CORBA_OBJECT_NIL;
	
	if (parent == CORBA_OBJECT_NIL) {
#ifdef G_ENABLE_DEBUG
		g_warning ("Item moniker with no parent !");
#endif
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_Moniker_InterfaceNotFound, NULL);
		return CORBA_OBJECT_NIL;
	}

	container = MateComponent_Moniker_resolve (parent, options,
					    "IDL:MateComponent/ItemContainer:1.0", ev);

	if (MATECOMPONENT_EX (ev))
		goto return_unref_parent;

	if (container == CORBA_OBJECT_NIL) {
#ifdef G_ENABLE_DEBUG
		g_warning ("Failed to extract a container from our parent");
#endif
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_Moniker_InterfaceNotFound, NULL);
		goto return_unref_parent;
	}

	containee = MateComponent_ItemContainer_getObjectByName (
		container, matecomponent_moniker_get_name (moniker),
		TRUE, ev);

	matecomponent_object_release_unref (container, ev);

	return matecomponent_moniker_util_qi_return (containee, requested_interface, ev);

 return_unref_parent:
	matecomponent_object_release_unref (parent, ev);

	return retval;
}
