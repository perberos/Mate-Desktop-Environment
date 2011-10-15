#ifndef CORBA_CONTEXT_H
#define CORBA_CONTEXT_H 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MATECORBA2_INTERNAL_API

struct CORBA_Context_type {
	struct MateCORBA_RootObject_struct parent;

	GHashTable    *mappings;
	GSList        *children;

	gchar         *the_name;

	CORBA_Context  parent_ctx;
};

void     MateCORBA_Context_marshal     (CORBA_Context                   ctx,
				    const MateCORBA_ContextMarshalItem *mlist,
				    CORBA_unsigned_long             nitems,
				    GIOPSendBuffer                 *buf);

gboolean MateCORBA_Context_demarshal   (CORBA_Context   parent,
				    CORBA_Context   initme,
				    GIOPRecvBuffer *recv_buffer);

void     MateCORBA_Context_server_free (CORBA_Context ctx);

#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif
