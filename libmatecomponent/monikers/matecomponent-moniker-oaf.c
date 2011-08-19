/*
 * mate-moniker-oaf.c: Sample oaf-system based Moniker implementation
 *
 * This is the oaf-activation based Moniker implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 */
#include <config.h>
#include <string.h>

#include <glib/gi18n-lib.h>

#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker-util.h>

#include "matecomponent-moniker-std.h"

MateComponent_Unknown
matecomponent_moniker_oaf_resolve (MateComponentMoniker               *moniker,
			    const MateComponent_ResolveOptions *options,
			    const CORBA_char            *requested_interface,
			    CORBA_Environment           *ev)
{
	MateComponent_Moniker       parent;
	MateComponent_Unknown       object;
	
	parent = matecomponent_moniker_get_parent (moniker, ev);

	if (ev->_major != CORBA_NO_EXCEPTION)
		return CORBA_OBJECT_NIL;
	
	if (parent != CORBA_OBJECT_NIL) {
		matecomponent_object_release_unref (parent, ev);

#ifdef G_ENABLE_DEBUG
		g_warning ("wierd; oafid moniker with a parent; strange");
#endif
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_Moniker_InterfaceNotFound, NULL);
		return CORBA_OBJECT_NIL;
	}

	object = matecomponent_activation_activate_from_id (
		(char *) matecomponent_moniker_get_name_full (moniker), 0, NULL, ev);

	if (MATECOMPONENT_EX (ev)) {
		if (ev->_major == CORBA_USER_EXCEPTION) {
			if (strcmp (ev->_id, ex_MateComponent_GeneralError)) {
				CORBA_exception_free (ev);

				matecomponent_exception_general_error_set (
					ev, NULL, _("Exception activating '%s'"),
					matecomponent_moniker_get_name_full (moniker));
			}
		}
		return CORBA_OBJECT_NIL;

	} else if (object == CORBA_OBJECT_NIL) {

		matecomponent_exception_general_error_set (
			ev, NULL, _("Failed to activate '%s'"),
			matecomponent_moniker_get_name_full (moniker));

		return CORBA_OBJECT_NIL;
	}

	return matecomponent_moniker_util_qi_return (object, requested_interface, ev);
}
