#ifndef _MATECOMPONENT_MONIKER_EXTRA_H_
#define _MATECOMPONENT_MONIKER_EXTRA_H_

#include <matecomponent/matecomponent-moniker-simple.h>
#include <matecomponent/matecomponent-moniker-extender.h>

MateComponent_Unknown matecomponent_moniker_config_resolve (
	MateComponentMoniker               *moniker,
	const MateComponent_ResolveOptions *options,
	const CORBA_char            *requested_interface,
	CORBA_Environment           *ev);

MateComponent_Unknown matecomponent_moniker_conf_indirect_resolve (
	MateComponentMoniker               *moniker,
	const MateComponent_ResolveOptions *options,
	const CORBA_char            *requested_interface,
	CORBA_Environment           *ev);

#endif
