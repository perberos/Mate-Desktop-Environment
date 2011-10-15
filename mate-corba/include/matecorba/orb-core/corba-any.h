#ifndef CORBA_ANY_H
#define CORBA_ANY_H 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

void       CORBA_any__copy     (CORBA_any       *out,
				const CORBA_any *in);
CORBA_any *CORBA_any__alloc    (void);

#define    CORBA_any_alloc     CORBA_any__alloc

gpointer   CORBA_any__freekids (gpointer mem,
				gpointer data);

#define CORBA_any_set_release(a, r) (a)->_release = r
#define CORBA_any_get_release(a)    (a)->_release

gpointer MateCORBA_copy_value       (gconstpointer  value,
				 CORBA_TypeCode tc);

CORBA_boolean
	 MateCORBA_any_equivalent   (CORBA_any         *obj,
				 CORBA_any         *any,
				 CORBA_Environment *ev);

#ifdef MATECORBA2_INTERNAL_API

void     MateCORBA_marshal_arg      (GIOPSendBuffer *buf,
				 gconstpointer   val,
				 CORBA_TypeCode  tc);

void     MateCORBA_marshal_any      (GIOPSendBuffer  *buf,
				 const CORBA_any *val);

gpointer MateCORBA_demarshal_arg    (GIOPRecvBuffer *buf,
				 CORBA_TypeCode  tc,
				 CORBA_ORB       orb);

gboolean MateCORBA_demarshal_any    (GIOPRecvBuffer *buf,
				 CORBA_any      *retval,
				 CORBA_ORB       orb);

gboolean MateCORBA_demarshal_value  (CORBA_TypeCode  tc,
				 gpointer       *val,
				 GIOPRecvBuffer *buf,
				 CORBA_ORB       orb);

void     MateCORBA_marshal_value    (GIOPSendBuffer *buf,
				 gconstpointer  *val,
				 CORBA_TypeCode  tc);

CORBA_boolean
	 MateCORBA_value_equivalent (gpointer          *a,
				 gpointer          *b,
				 CORBA_TypeCode     tc,
				 CORBA_Environment *ev);

size_t  MateCORBA_gather_alloc_info (CORBA_TypeCode tc);

#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif
