#ifndef CORBA_OBJECT_H
#define CORBA_OBJECT_H 1

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MATECORBA2_INTERNAL_API

GIOPConnection *MateCORBA_object_get_connection  (CORBA_Object    obj);
GIOPConnection *MateCORBA_object_peek_connection (CORBA_Object   obj);


void            MateCORBA_marshal_object        (GIOPSendBuffer *buf,
					     CORBA_Object    obj);
gboolean        MateCORBA_demarshal_object      (CORBA_Object   *obj,
					     GIOPRecvBuffer *buf,
					     CORBA_ORB       orb);

CORBA_Object    MateCORBA_objref_new            (CORBA_ORB       orb,
					     MateCORBA_OAObject  adaptor_obj,
					     GQuark          type_id);

#endif /* MATECORBA2_INTERNAL_API */

/*
 * Client invocation policy API
 */
typedef struct _MateCORBAPolicy MateCORBAPolicy;

/* An extended policy - blocks re-enterancy by default */
#define MATECORBA_TYPE_POLICY_EX (MateCORBA_policy_ex_get_type ())

GType        MateCORBA_policy_ex_get_type (void) G_GNUC_CONST;
MateCORBAPolicy *MateCORBA_policy_new         (GType        type,
				       const char  *first_prop,
				       ...);
MateCORBAPolicy *MateCORBA_policy_ref         (MateCORBAPolicy *p);
void         MateCORBA_policy_unref       (MateCORBAPolicy *p);
void         MateCORBA_object_set_policy  (CORBA_Object obj,
				       MateCORBAPolicy *p);
MateCORBAPolicy *MateCORBA_object_get_policy  (CORBA_Object obj);
void         MateCORBA_policy_push        (MateCORBAPolicy *p);
void         MateCORBA_policy_pop         (void);

/*
 * CORBA_Object interface type data.
 */
#include <matecorba/orb-core/matecorba-interface.h>

#define CORBA_OBJECT_SMALL_GET_TYPE_ID    12
#define CORBA_OBJECT_SMALL_GET_IINTERFACE 13

extern MateCORBA_IInterface CORBA_Object__iinterface;
extern MateCORBA_IMethod    CORBA_Object__imethods[];

#define CORBA_Object_IMETHODS_LEN 12

#ifdef __cplusplus
}
#endif

#endif /* CORBA_OBJECT_H */
