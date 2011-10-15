#ifndef MATECORBA_OBJECT_H
#define MATECORBA_OBJECT_H 1

#include <glib.h>

#ifdef G_PLATFORM_WIN32
#define MATECORBA2_MAYBE_CONST
#else
#define MATECORBA2_MAYBE_CONST const
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MATECORBA_ROT_NULL,
	MATECORBA_ROT_OBJREF,
	/* values below here all signify psuedo-objects */
	MATECORBA_ROT_ORB,
	MATECORBA_ROT_ADAPTOR,
	MATECORBA_ROT_POAMANAGER,
	MATECORBA_ROT_POLICY,
	MATECORBA_ROT_TYPECODE,
	MATECORBA_ROT_REQUEST,
	MATECORBA_ROT_SERVERREQUEST,
	MATECORBA_ROT_CONTEXT,
	MATECORBA_ROT_DYNANY,
	MATECORBA_ROT_OAOBJECT,
	MATECORBA_ROT_ORBGROUP,
	MATECORBA_ROT_POACURRENT,
	MATECORBA_ROT_CLIENT_POLICY
} MateCORBA_RootObject_Type;

typedef struct MateCORBA_RootObject_struct *MateCORBA_RootObject;

typedef void (* MateCORBA_RootObject_DestroyFunc) (MateCORBA_RootObject obj);

typedef struct _MateCORBA_RootObject_Interface {
	MateCORBA_RootObject_Type        type;
	MateCORBA_RootObject_DestroyFunc destroy;
} MateCORBA_RootObject_Interface;

struct MateCORBA_RootObject_struct {
	const MateCORBA_RootObject_Interface *interface;
  int refs;
};

#define MATECORBA_REFCOUNT_STATIC -10

void     MateCORBA_RootObject_init      (MateCORBA_RootObject obj,
				     const MateCORBA_RootObject_Interface *interface);

#ifdef MATECORBA2_INTERNAL_API

/* Used to determine whether the refcount is valid or not */
#define MATECORBA_REFCOUNT_MAX (1<<20)

#define MATECORBA_ROOT_OBJECT(obj)      ((MateCORBA_RootObject)(obj))
#define MATECORBA_ROOT_OBJECT_TYPE(obj) (((MateCORBA_RootObject)(obj))->interface->type)


gpointer MateCORBA_RootObject_duplicate   (gpointer obj);
gpointer MateCORBA_RootObject_duplicate_T (gpointer obj);
void     MateCORBA_RootObject_release     (gpointer obj);
void     MateCORBA_RootObject_release_T   (gpointer obj);

extern GMutex *MateCORBA_RootObject_lifecycle_lock;

#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif
