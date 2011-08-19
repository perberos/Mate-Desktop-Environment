/*
 * mate-moniker-config.c: MateConf Moniker implementation
 *
 * This is the MateConf based Moniker implementation.
 *
 * Author:
 *	Rodrigo Moya (rodrigo@mate-db.org)
 */

#include <string.h>
#include <matecomponent/matecomponent-exception.h>
#include "matecomponent-config-bag.h"
#include "matecomponent-moniker-extra.h"

MateComponent_Unknown
matecomponent_moniker_config_resolve (MateComponentMoniker *moniker,
			       const MateComponent_ResolveOptions *options,
			       const CORBA_char *requested_interface,
			       CORBA_Environment *ev)
{
	const gchar *name;

	name = matecomponent_moniker_get_name (moniker);

	if (!strcmp (requested_interface, "IDL:MateComponent/PropertyBag:1.0")) {
		MateComponentConfigBag *bag;

		bag = matecomponent_config_bag_new (name);
		if (bag) {
			return (MateComponent_Unknown) CORBA_Object_duplicate (
				MATECOMPONENT_OBJREF (bag), ev);
		}

		matecomponent_exception_set (ev, ex_MateComponent_Moniker_InterfaceNotFound);
	}
	else
		matecomponent_exception_set (ev, ex_MateComponent_Moniker_InterfaceNotFound);

	return CORBA_OBJECT_NIL;
}
