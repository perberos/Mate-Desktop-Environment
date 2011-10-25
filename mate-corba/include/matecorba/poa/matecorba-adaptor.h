#ifndef __MATECORBA_ADAPTOR_H__
#define __MATECORBA_ADAPTOR_H__

#include <glib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum {
		MATECORBA_THREAD_HINT_NONE = 0,
		MATECORBA_THREAD_HINT_PER_OBJECT,
		MATECORBA_THREAD_HINT_PER_REQUEST,
		MATECORBA_THREAD_HINT_PER_POA,
		MATECORBA_THREAD_HINT_PER_CONNECTION,
		MATECORBA_THREAD_HINT_ONEWAY_AT_IDLE,
		MATECORBA_THREAD_HINT_ALL_AT_IDLE,
		MATECORBA_THREAD_HINT_ON_CONTEXT
	} MateCORBAThreadHint;

	typedef struct MateCORBA_ObjectAdaptor_type* MateCORBA_ObjectAdaptor;

	void MateCORBA_ObjectAdaptor_set_thread_hint(MateCORBA_ObjectAdaptor adaptor, MateCORBAThreadHint thread_hint, ...);
	void MateCORBA_ObjectAdaptor_set_thread_hintv(MateCORBA_ObjectAdaptor adaptor, MateCORBAThreadHint thread_hint, va_list args);
	MateCORBAThreadHint MateCORBA_ObjectAdaptor_get_thread_hint(MateCORBA_ObjectAdaptor adaptor);

	void MateCORBA_ObjectAdaptor_object_bind_to_current_thread(CORBA_Object obj);


	#ifdef MATECORBA2_INTERNAL_API

		void MateCORBA_handle_locate_request(CORBA_ORB orb, GIOPRecvBuffer* recv_buffer);

		void MateCORBA_handle_request(CORBA_ORB orb, GIOPRecvBuffer* recv_buffer);

		void MateCORBA_small_handle_request(MateCORBA_OAObject adaptor_obj, CORBA_Identifier opname, gpointer ret, gpointer* args, CORBA_Context ctx, GIOPRecvBuffer* recv_buffer, CORBA_Environment* ev);

		gboolean MateCORBA_OAObject_is_active(MateCORBA_OAObject adaptor_obj);

		MateCORBA_ObjectKey* MateCORBA_OAObject_object_to_objkey(MateCORBA_OAObject adaptor_obj);

		void MateCORBA_OAObject_invoke(MateCORBA_OAObject adaptor_obj, gpointer ret, gpointer* args, CORBA_Context ctx, gpointer data, CORBA_Environment* ev);
		/*
		 * MateCORBA_OAObject
		 */

		typedef gboolean (*MateCORBAStateCheckFunc) (MateCORBA_OAObject adaptor_obj);

		typedef MateCORBA_ObjectKey *(*MateCORBAKeyGenFunc) (MateCORBA_OAObject adaptor_obj);

		typedef void (*MateCORBAInvokeFunc) (MateCORBA_OAObject adaptor_obj, gpointer ret, gpointer* args, CORBA_Context ctx, gpointer data, CORBA_Environment* ev);

		typedef void (*MateCORBAReqFunc) (MateCORBA_OAObject adaptor_obj, CORBA_Identifier opname, gpointer ret, gpointer* args, CORBA_Context ctx, GIOPRecvBuffer* recv_buffer, CORBA_Environment* ev);

		typedef enum {
			MATECORBA_ADAPTOR_POA = 1 << 0
		} MateCORBA_Adaptor_type;

		struct MateCORBA_OAObject_Interface_type {
			MateCORBA_Adaptor_type adaptor_type;

			MateCORBAStateCheckFunc is_active;
			MateCORBAKeyGenFunc object_to_objkey;
			MateCORBAInvokeFunc invoke;
			MateCORBAReqFunc handle_request;
		};

		typedef struct MateCORBA_OAObject_Interface_type* MateCORBA_OAObject_Interface;

		struct MateCORBA_OAObject_type {
			struct MateCORBA_RootObject_struct parent;

			CORBA_Object objref;

			MateCORBA_OAObject_Interface interface;
		};

		/*
		 * MateCORBA_ObjectAdaptor
		 */

		typedef CORBA_sequence_CORBA_octet MateCORBA_AdaptorKey;

		typedef void (*MateCORBAReqHandlerFunc) (MateCORBA_ObjectAdaptor adaptor, GIOPRecvBuffer* recv_buffer, MateCORBA_ObjectKey* objkey);

		struct MateCORBA_ObjectAdaptor_type {
			struct MateCORBA_RootObject_struct parent;

			GMutex* lock;

			MateCORBAReqHandlerFunc handle_request;

			MateCORBA_AdaptorKey adaptor_key;

			MateCORBAThreadHint thread_hint;

			GMainContext* context;
		};

		int MateCORBA_adaptor_setup(MateCORBA_ObjectAdaptor adaptor, CORBA_ORB orb);

	#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif /* __MATECORBA_ADAPTOR_H__ */
