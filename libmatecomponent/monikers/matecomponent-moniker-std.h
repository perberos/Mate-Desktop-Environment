#ifndef _MATECOMPONENT_MONIKER_STD_H_
#define _MATECOMPONENT_MONIKER_STD_H_

#include <matecomponent/matecomponent-moniker-simple.h>
#include <matecomponent/matecomponent-moniker-extender.h>

MateComponent_Unknown matecomponent_moniker_item_resolve    (MateComponentMoniker               *moniker,
					       const MateComponent_ResolveOptions *options,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);

MateComponent_Unknown matecomponent_moniker_ior_resolve     (MateComponentMoniker               *moniker,
					       const MateComponent_ResolveOptions *options,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);
MateComponent_Unknown matecomponent_moniker_oaf_resolve     (MateComponentMoniker               *moniker,
					       const MateComponent_ResolveOptions *options,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);

MateComponent_Unknown matecomponent_moniker_cache_resolve   (MateComponentMoniker               *moniker,
					       const MateComponent_ResolveOptions *options,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);

MateComponent_Unknown matecomponent_moniker_query_resolve   (MateComponentMoniker               *moniker,
					       const MateComponent_ResolveOptions *options,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);

MateComponent_Unknown matecomponent_moniker_new_resolve     (MateComponentMoniker               *moniker,
					       const MateComponent_ResolveOptions *options,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);

/* extender functions */

MateComponent_Unknown matecomponent_stream_extender_resolve (MateComponentMonikerExtender       *extender,
					       const MateComponent_Moniker         m,
					       const MateComponent_ResolveOptions *options,
					       const CORBA_char            *display_name,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);

#endif /* _MATECOMPONENT_MONIKER_STD_H_ */
