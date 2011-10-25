#ifndef MATECORBA_DYNAMIC_H
#define MATECORBA_DYNAMIC_H 1

#include <matecorba/dynamic/dynamic-defs.h>

#ifdef __cplusplus
extern "C" {
#endif

	#ifdef MATECORBA2_INTERNAL_API

		gpointer MateCORBA_dynany_new_default(const CORBA_TypeCode tc);

	#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif
