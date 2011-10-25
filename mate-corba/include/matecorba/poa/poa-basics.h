#ifndef _POA_BASICS_H_
#define _POA_BASICS_H_ 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

	#if !defined(_PortableServer_Servant_defined)
		#define _PortableServer_Servant_defined 1
		typedef gpointer PortableServer_Servant;
	#else
		#error "Include mixup: poa-defs.h included before poa-basics.h"
	#endif

	#if !defined(MATECORBA_DECL_PortableServer_POA) && !defined(_PortableServer_POA_defined)
		#define MATECORBA_DECL_PortableServer_POA 1
		#define _PortableServer_POA_defined 1
		typedef struct PortableServer_POA_type *PortableServer_POA;
	#endif

	typedef struct {
		void* _private;
		void (*finalize) (PortableServer_Servant, CORBA_Environment*);
		PortableServer_POA (*default_POA) (PortableServer_Servant, CORBA_Environment*);
		void (*add_ref) (PortableServer_Servant, CORBA_Environment*);
		void (*remove_ref) (PortableServer_Servant, CORBA_Environment*);
		CORBA_InterfaceDef (*get_interface) (PortableServer_Servant, CORBA_Environment*);
		CORBA_boolean (*is_a) (PortableServer_Servant, const char*, CORBA_Environment*);
		CORBA_boolean (*non_existent) (PortableServer_Servant, CORBA_Environment*);
	} PortableServer_ServantBase__epv;

	typedef PortableServer_ServantBase__epv* PortableServer_ServantBase__vepv;

	typedef struct {
		void* _private;
		PortableServer_ServantBase__vepv* vepv;
	} PortableServer_ServantBase;

	typedef PortableServer_ServantBase__epv PortableServer_RefCountServantBase__epv;
	typedef PortableServer_ServantBase__epv* PortableServer_RefCountServantBase__vepv;
	typedef PortableServer_ServantBase PortableServer_RefCountServantBase;

	#if defined(MATECORBA2_INTERNAL_API) || defined (MATECORBA2_STUBS_API)

		typedef struct MateCORBA_POAObject_type* MateCORBA_POAObject;
		typedef struct MateCORBA_OAObject_type* MateCORBA_OAObject;

		typedef gshort MateCORBA_VepvIdx;

		typedef void (*MateCORBASmallSkeleton) (PortableServer_ServantBase* servant, gpointer ret, gpointer* args, gpointer ctx, CORBA_Environment* ev, gpointer implementation);

		typedef MateCORBASmallSkeleton (*MateCORBA_impl_finder) (PortableServer_ServantBase* servant, const char* method, gpointer* m_data, gpointer* implementation);
		/* stub compatibility */
		typedef MateCORBA_impl_finder MateCORBA_small_impl_finder;

	#endif /* defined(MATECORBA2_INTERNAL_API) || defined (MATECORBA2_STUBS_API) */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _POA_BASICS_H_ 1 */
