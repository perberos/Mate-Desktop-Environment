/*
 * An attempt to shrink the beast to a managable size.
 */
#ifndef CORBA_SMALL_H
#define CORBA_SMALL_H 1

#include <glib-object.h>
#include <matecorba/orb-core/matecorba-interface.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CORBA_unsigned_long           version;
	MateCORBA_IInterface            **interfaces;
	CORBA_sequence_CORBA_TypeCode types;
} MateCORBA_IModule;

const char    *MateCORBA_get_safe_tmp      (void);

/* Builtin allocators */
gpointer       MateCORBA_small_alloc       (CORBA_TypeCode      tc);
gpointer       MateCORBA_small_allocbuf    (CORBA_TypeCode      tc,
					CORBA_unsigned_long length);
void           MateCORBA_small_freekids    (CORBA_TypeCode      tc,
					gpointer            p,
					gpointer            d);

/* More friendly(?) sequence allocators */
gpointer       MateCORBA_sequence_alloc    (CORBA_TypeCode      sequence_tc,
					CORBA_unsigned_long length);
void           MateCORBA_sequence_append   (gpointer            sequence,
					gconstpointer       element);
void           MateCORBA_sequence_set_size (gpointer            sequence,
					CORBA_unsigned_long length);
#define        MateCORBA_sequence_index(sequence,idx) (sequence)->_buffer[(idx)]
void           MateCORBA_sequence_concat   (gpointer            sequence,
					gconstpointer       append);
void           MateCORBA_sequence_remove   (gpointer            sequence,
                                        guint               idx);

typedef enum {
	MATECORBA_CONNECTION_CONNECTED,
	MATECORBA_CONNECTION_CONNECTING,
	MATECORBA_CONNECTION_DISCONNECTED,
	MATECORBA_CONNECTION_IN_PROC
} MateCORBAConnectionStatus;

gpointer              MateCORBA_small_get_servant              (CORBA_Object obj);
MateCORBAConnectionStatus MateCORBA_small_get_connection_status    (CORBA_Object obj);
MateCORBAConnectionStatus MateCORBA_small_listen_for_broken        (CORBA_Object obj,
							    GCallback    fn,
							    gpointer     user_data);
MateCORBAConnectionStatus MateCORBA_small_unlisten_for_broken_full (CORBA_Object obj,
							    GCallback    fn,
							    gpointer     user_data);
/* old / stale */
MateCORBAConnectionStatus MateCORBA_small_unlisten_for_broken      (CORBA_Object obj,
							    GCallback    fn);

typedef struct _MateCORBAConnection MateCORBAConnection;

MateCORBAConnection      *MateCORBA_small_get_connection        (CORBA_Object     obj);
MateCORBAConnection      *MateCORBA_small_get_connection_ref    (CORBA_Object     obj);
void                  MateCORBA_small_connection_unref      (MateCORBAConnection *cnx);
void                  MateCORBA_connection_set_max_buffer   (MateCORBAConnection *cnx,
							 gulong           max_buffer_bytes);

#if defined(MATECORBA2_INTERNAL_API) || defined (MATECORBA2_STUBS_API)

#define MATECORBA_SMALL_FAST_LOCALS           1
#define MATECORBA_SMALL_FORCE_GENERIC_MARSHAL 2

extern int     MateCORBA_small_flags;

/* Deprecated - only for bin-compat with pre 2.4 stubs */
void           MateCORBA_small_invoke_stub (CORBA_Object        object,
					MateCORBA_IMethod      *m_data,
					gpointer            ret,
					gpointer           *args,
					CORBA_Context       ctx,
					CORBA_Environment  *ev);


void           MateCORBA_small_invoke_stub_n (CORBA_Object        object,
					  MateCORBA_IMethods     *methods,
					  glong               index,
					  gpointer            ret,
					  gpointer           *args,
					  CORBA_Context       ctx,
					  CORBA_Environment  *ev);

#endif /* defined(MATECORBA2_INTERNAL_API) || defined (MATECORBA2_STUBS_API) */

#ifdef MATECORBA2_INTERNAL_API

#include <matecorba/GIOP/giop.h>

void           MateCORBA_small_invoke_adaptor (MateCORBA_OAObject     adaptor_obj,
					   GIOPRecvBuffer    *recv_buffer,
					   MateCORBA_IMethod     *m_data,
					   gpointer           data,
					   CORBA_Environment *ev);

#endif /* MATECORBA2_INTERNAL_API */

/* Type library work */
CORBA_char       *MateCORBA_small_get_type_id         (CORBA_Object       object,
						   CORBA_Environment *ev);
MateCORBA_IInterface *MateCORBA_small_get_iinterface      (CORBA_Object       opt_object,
					           const CORBA_char  *repo_id,
						   CORBA_Environment *ev);
gboolean          MateCORBA_small_load_typelib        (const char        *libname);
CORBA_sequence_CORBA_TypeCode *
                  MateCORBA_small_get_types           (const char        *name);
CORBA_sequence_MateCORBA_IInterface *
                  MateCORBA_small_get_iinterfaces     (const char        *name);

typedef struct _MateCORBAAsyncQueueEntry MateCORBAAsyncQueueEntry;

typedef void (*MateCORBAAsyncInvokeFunc) (CORBA_Object          object,
				      MateCORBA_IMethod        *m_data,
				      MateCORBAAsyncQueueEntry *aqe,
				      gpointer              user_data,
				      CORBA_Environment    *ev);

/* Various bits for Async work */
void MateCORBA_small_invoke_async        (CORBA_Object           obj,
				      MateCORBA_IMethod         *m_data,
				      MateCORBAAsyncInvokeFunc   fn,
				      gpointer               user_data,
				      gpointer              *args,
				      CORBA_Context          ctx,
				      CORBA_Environment     *ev);

void MateCORBA_small_demarshal_async     (MateCORBAAsyncQueueEntry  *aqe,
				      gpointer               ret,
				      gpointer              *args,
				      CORBA_Environment     *ev);

#ifdef __cplusplus
}
#endif

#endif /* CORBA_SMALL_H */
