#ifndef CORBA_OBJECT_TYPE_H
#define CORBA_OBJECT_TYPE_H 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MATECORBA2_INTERNAL_API)

/* This ABI is now purely internal and can be hacked around */

typedef CORBA_sequence_CORBA_octet MateCORBA_ObjectKey;

struct CORBA_Object_type {
	struct MateCORBA_RootObject_struct parent;

	GIOPConnection                *connection;        /* l */
	GQuark                         type_qid;
	GSList                        *profile_list;      /* l */
	GSList                        *forward_locations; /* l */
	MateCORBA_ObjectKey               *object_key;        /* l */
	struct _MateCORBAPolicy           *invoke_policy;     /* l */

	CORBA_ORB                      orb;
	MateCORBA_OAObject                 adaptor_obj;
};

#endif /* defined(MATECORBA2_INTERNAL_API) */

#ifdef __cplusplus
}
#endif

#endif
