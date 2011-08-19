#ifndef MATECORBA_DYNAMIC_H
#define MATECORBA_DYNAMIC_H 1

#include <matecorba/dynamic/dynamic-defs.h>

G_BEGIN_DECLS

#ifdef MATECORBA2_INTERNAL_API

gpointer MateCORBA_dynany_new_default (const CORBA_TypeCode tc);

#endif /* MATECORBA2_INTERNAL_API */

G_END_DECLS

#endif
