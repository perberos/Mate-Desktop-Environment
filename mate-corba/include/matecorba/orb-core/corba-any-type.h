#ifndef CORBA_ANY_TYPE_H
#define CORBA_ANY_TYPE_H 1

#include <matecorba/orb-core/corba-pobj.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CORBA_any_struct {
	CORBA_TypeCode  _type;
	gpointer        _value;
	CORBA_boolean   _release;
};

#ifdef __cplusplus
}
#endif

#endif
