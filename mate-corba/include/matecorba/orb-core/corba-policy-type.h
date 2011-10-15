#ifndef CORBA_POLICY_TYPE_H
#define CORBA_POLICY_TYPE_H 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MATECORBA2_INTERNAL_API

struct CORBA_Policy_type {
	struct MateCORBA_RootObject_struct parent;

	CORBA_unsigned_long            type;
	CORBA_unsigned_long            value;
};

CORBA_Policy        MateCORBA_Policy_new (CORBA_unsigned_long type,
				      CORBA_unsigned_long value);

#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif
