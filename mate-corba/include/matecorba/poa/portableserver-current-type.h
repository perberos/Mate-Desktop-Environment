#ifndef PORTABLESERVER_CURRENT_TYPE_H
#define PORTABLESERVER_CURRENT_TYPE_H 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	#ifdef MATECORBA2_INTERNAL_API

	struct PortableServer_Current_type {
		struct MateCORBA_RootObject_struct parent;

		CORBA_ORB orb;
	};

	#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PORTABLESERVER_CURRENT_TYPE_H */
