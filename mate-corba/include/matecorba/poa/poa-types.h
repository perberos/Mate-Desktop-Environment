#ifndef POA_TYPES_H
#define POA_TYPES_H 1

#include <matecorba/poa/matecorba-adaptor.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	#if defined(MATECORBA2_INTERNAL_API) || defined (MATECORBA2_STUBS_API)

		typedef struct {
			void (*dummy_padding) (void);
			MateCORBA_impl_finder impl_finder;
			const char* class_name;
			CORBA_unsigned_long* class_id;
			MateCORBA_VepvIdx* vepvmap;
			MateCORBA_IInterface* idata;
		} PortableServer_ClassInfo;

	#endif /* defined(MATECORBA2_INTERNAL_API) || defined (MATECORBA2_STUBS_API) */

	#ifdef MATECORBA2_INTERNAL_API

		struct MateCORBA_POAObject_type {
			struct MateCORBA_OAObject_type base;

			PortableServer_Servant servant;
			PortableServer_POA poa;
			PortableServer_ObjectId* object_id;

			MateCORBA_VepvIdx* vepvmap_cache;

			guint16 life_flags;
			guint16 use_cnt;

			MateCORBA_POAObject next;
		};

		#define MateCORBA_LifeF_NeedPostInvoke      (1 << 0)
		#define MateCORBA_LifeF_DoEtherealize       (1 << 1)
		#define MateCORBA_LifeF_IsCleanup           (1 << 2)
		#define MateCORBA_LifeF_DeactivateDo        (1 << 4)
		#define MateCORBA_LifeF_Deactivating        (1 << 5)
		#define MateCORBA_LifeF_Deactivated         (1 << 6)
		#define MateCORBA_LifeF_DestroyDo           (1 << 8)
		#define MateCORBA_LifeF_Destroying          (1 << 9)
		#define MateCORBA_LifeF_Destroyed           (1 << 10)

		#define MATECORBA_SERVANT_TO_CLASSINFO(servant) \
			((PortableServer_ClassInfo*) (((PortableServer_ServantBase*) (servant))->vepv[0]->_private))

	#endif /* MATECORBA2_INTERNAL_API */

	#if defined(MATECORBA2_INTERNAL_API) || defined(MATECORBA2_STUBS_API)

		void MateCORBA_c_stub_invoke(CORBA_Object obj, MateCORBA_IMethods* methods, glong method_index, gpointer ret, gpointer args, CORBA_Context ctx, CORBA_Environment* ev, glong class_id, glong method_offset, MateCORBASmallSkeleton skel_impl);

		#define MATECORBA_VEPV_OFFSET(vepv_type,epv_member) \
			   ((CORBA_unsigned_long) (G_STRUCT_OFFSET(vepv_type, epv_member)) / sizeof(GFunc))


		/*
		 * These macros are deprecated, they remain on the off-chance that
		 * someone compiles a really, really old stub. Their functionality
		 * is always short-circuited because MateCORBA_small_flags &
		 * MATECORBA_SMALL_FAST_LOCALS is now never true.
		 */
		#define MATECORBA_STUB_IsBypass(obj, classid) FALSE
		#define MATECORBA_STUB_GetEpv(obj, clsid)     NULL
		#define MATECORBA_STUB_PreCall(x)
		#define MATECORBA_STUB_PostCall(x)

	#endif /* defined(MATECORBA2_INTERNAL_API) || defined (MATECORBA2_STUBS_API) */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* POA_TYPES_H */
