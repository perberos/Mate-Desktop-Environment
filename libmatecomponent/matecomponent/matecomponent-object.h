/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * MateComponent Unknown interface base implementation
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright 1999,2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_OBJECT_H_
#define _MATECOMPONENT_OBJECT_H_

#include <matecomponent-activation/matecomponent-activation.h>

#include <glib-object.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-macros.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef MATECOMPONENT_OBJECT_DEBUG

#define MATECOMPONENT_TYPE_OBJECT        (matecomponent_object_get_type ())
#define MATECOMPONENT_OBJECT_TYPE        MATECOMPONENT_TYPE_OBJECT /* deprecated, you should use MATECOMPONENT_TYPE_OBJECT */
#define MATECOMPONENT_OBJECT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_OBJECT, MateComponentObject))
#define MATECOMPONENT_OBJECT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_OBJECT, MateComponentObjectClass))
#define MATECOMPONENT_IS_OBJECT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_OBJECT))
#define MATECOMPONENT_IS_OBJECT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_OBJECT))
#define MATECOMPONENT_OBJECT_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), MATECOMPONENT_TYPE_OBJECT, MateComponentObjectClass))

#define MATECOMPONENT_OBJREF(o)          (matecomponent_object_corba_objref(MATECOMPONENT_OBJECT(o)))

typedef void  (*MateComponentObjectPOAFn) (PortableServer_Servant servant,
				    CORBA_Environment     *ev);

typedef struct _MateComponentObjectPrivate MateComponentObjectPrivate;
typedef struct _MateComponentObjectBag     MateComponentObjectBag;
typedef struct _MateComponentObject        MateComponentObject;

typedef struct {
	GObject              base;             /* pointer + guint + pointer */
	MateComponentObjectPrivate *priv;             /* pointer */
	guint                object_signature; /* guint   */
} MateComponentObjectHeader;

#define MATECOMPONENT_OBJECT_HEADER_SIZE (sizeof (MateComponentObjectHeader))
#define MATECOMPONENT_OBJECT_SIGNATURE   0xaef2
#define MATECOMPONENT_SERVANT_SIGNATURE  0x2fae

struct _MateComponentObject {
	/* A GObject and its signature of type MateComponentObjectHeader */
	GObject              base;             /* pointer + guint + pointer */
	MateComponentObjectPrivate *priv;             /* pointer */
	guint                object_signature; /* guint   */

	/* A Servant and its signature - same memory layout */
	POA_MateComponent_Unknown   servant;          /* pointer + pointer */
	guint                dummy;            /* guint   */
	MateComponent_Unknown       corba_objref;     /* pointer */
	guint                servant_signature;
};

typedef struct {
	GObjectClass parent_class;

	/* signals. */
	void         (*destroy)          (MateComponentObject *object);
	void         (*system_exception) (MateComponentObject *object,
					  CORBA_Object  cobject,
					  CORBA_Environment *ev);

	MateComponentObjectPOAFn          poa_init_fn;
	MateComponentObjectPOAFn          poa_fini_fn;

	POA_MateComponent_Unknown__vepv       *vepv;

	/* The offset of this class' additional epv */
	int                             epv_struct_offset;

	PortableServer_ServantBase__epv base_epv;
	POA_MateComponent_Unknown__epv         epv;

	gpointer                        dummy[4];
} MateComponentObjectClass;

GType                    matecomponent_object_get_type               (void) G_GNUC_CONST;
void                     matecomponent_object_add_interface          (MateComponentObject           *object,
							       MateComponentObject           *newobj);
MateComponentObject            *matecomponent_object_query_local_interface  (MateComponentObject           *object,
							       const char             *repo_id);
MateComponent_Unknown           matecomponent_object_query_remote           (MateComponent_Unknown          unknown,
							       const char             *repo_id,
							       CORBA_Environment      *opt_ev);
MateComponent_Unknown           matecomponent_object_query_interface        (MateComponentObject           *object,
							       const char             *repo_id,
							       CORBA_Environment      *opt_ev);
MateComponent_Unknown           matecomponent_object_corba_objref           (MateComponentObject           *object);
void                     matecomponent_object_set_poa                (MateComponentObject           *object,
							       PortableServer_POA      poa);

/*
 * Mate Object Life Cycle
 */
MateComponent_Unknown           matecomponent_object_dup_ref                (MateComponent_Unknown          object,
							       CORBA_Environment      *opt_ev);
MateComponent_Unknown           matecomponent_object_release_unref          (MateComponent_Unknown          object,
							       CORBA_Environment      *opt_ev);
gpointer                 matecomponent_object_ref                    (gpointer                obj);
void                     matecomponent_object_idle_unref             (gpointer                obj);
gpointer                 matecomponent_object_unref                  (gpointer                obj);
void                     matecomponent_object_set_immortal           (MateComponentObject           *object,
							       gboolean                immortal);
gpointer                 matecomponent_object_trace_refs             (gpointer                obj,
							       const char             *fn,
							       int                     line,
							       gboolean                ref);

#ifdef MATECOMPONENT_OBJECT_DEBUG
#	define           matecomponent_object_ref(o)   matecomponent_object_trace_refs ((o),G_STRFUNC,__LINE__,TRUE);
#	define           matecomponent_object_unref(o) matecomponent_object_trace_refs ((o),G_STRFUNC,__LINE__,FALSE);
#endif	/* MATECOMPONENT_OBJECT_DEBUG */
void                     matecomponent_object_dump_interfaces        (MateComponentObject *object);

/*
 * Error checking
 */
void                     matecomponent_object_check_env              (MateComponentObject           *object,
							       CORBA_Object            corba_object,
							       CORBA_Environment      *ev);

#define MATECOMPONENT_OBJECT_CHECK(o,c,e)				\
			G_STMT_START {				\
			if ((e)->_major != CORBA_NO_EXCEPTION)	\
				matecomponent_object_check_env(o,c,e);	\
			} G_STMT_END

/*
 * Others
 */

gboolean  matecomponent_unknown_ping           (MateComponent_Unknown     object,
					 CORBA_Environment *opt_ev);
void      matecomponent_object_list_unref_all  (GList            **list);
void      matecomponent_object_slist_unref_all (GSList           **list);

/*
 * A weak-ref cache scheme
 */

#define MATECOMPONENT_COPY_FUNC(fn) ((MateComponentCopyFunc)(fn))

typedef gpointer (*MateComponentCopyFunc) (gconstpointer key);

MateComponentObjectBag *matecomponent_object_bag_new      (GHashFunc       hash_func,
					     GEqualFunc      key_equal_func,
					     MateComponentCopyFunc  key_copy_func,
					     GDestroyNotify  key_destroy_func);
MateComponentObject    *matecomponent_object_bag_get_ref  (MateComponentObjectBag *bag,
					     gconstpointer    key);
gboolean         matecomponent_object_bag_add_ref  (MateComponentObjectBag *bag,
					     gconstpointer    key,
					     MateComponentObject    *object);
void             matecomponent_object_bag_remove   (MateComponentObjectBag *bag,
					     gconstpointer    key);
void             matecomponent_object_bag_destroy  (MateComponentObjectBag *bag);
GPtrArray       *matecomponent_object_bag_list_ref (MateComponentObjectBag *bag);


/* Detects the pointer type and returns the object reference - magic. */
MateComponentObject *matecomponent_object (gpointer p);
/* The same thing but faster - has a double evaluate */
#define       matecomponent_object_fast(o) \
	((((MateComponentObjectHeader *)(o))->object_signature == MATECOMPONENT_OBJECT_SIGNATURE) ? \
	 (MateComponentObject *)(o) : (MateComponentObject *)(((guchar *) (o)) - MATECOMPONENT_OBJECT_HEADER_SIZE))

/* Compat */
#define       matecomponent_object_from_servant(s) ((MateComponentObject *)(((guchar *) (s)) - MATECOMPONENT_OBJECT_HEADER_SIZE))
#define       matecomponent_object_get_servant(o)  ((PortableServer_Servant)((guchar *)(o) + MATECOMPONENT_OBJECT_HEADER_SIZE))


PortableServer_POA matecomponent_object_get_poa (MateComponentObject *object);

/* Use G_STRUCT_OFFSET to calc. epv_struct_offset */
GType          matecomponent_type_unique (GType             parent_type,
				   MateComponentObjectPOAFn init_fn,
				   MateComponentObjectPOAFn fini_fn,
				   int               epv_struct_offset,
				   const GTypeInfo  *info,
				   const gchar      *type_name);

gboolean       matecomponent_type_setup  (GType             type,
				   MateComponentObjectPOAFn init_fn,
				   MateComponentObjectPOAFn fini_fn,
				   int               epv_struct_offset);

#define MATECOMPONENT_TYPE_FUNC_FULL(class_name, corba_name, parent, prefix)         \
GType                                                                         \
prefix##_get_type (void)                                                      \
{                                                                             \
	GType ptype;                                                          \
	static GType type = 0;                                                \
                                                                              \
	if (type == 0) {                                                      \
		static GTypeInfo info = {                                     \
			sizeof (class_name##Class),                           \
			(GBaseInitFunc) NULL,                                 \
			(GBaseFinalizeFunc) NULL,                             \
			(GClassInitFunc) prefix##_class_init,                 \
			NULL, NULL,                                           \
			sizeof (class_name),                                  \
			0,                                                    \
			(GInstanceInitFunc) prefix##_init                     \
		};                                                            \
		ptype = (parent);                                             \
		type = matecomponent_type_unique (ptype,                             \
			POA_##corba_name##__init, POA_##corba_name##__fini,   \
			G_STRUCT_OFFSET (class_name##Class, epv),             \
			&info, #class_name);                                  \
	}                                                                     \
	return type;                                                          \
}

#define MATECOMPONENT_TYPE_FUNC(class_name, parent, prefix)                        \
GType                                                                         \
prefix##_get_type (void)                                                      \
{                                                                             \
	GType ptype;                                                          \
	static GType type = 0;                                                \
                                                                              \
	if (type == 0) {                                                      \
		static GTypeInfo info = {                                     \
			sizeof (class_name##Class),                           \
			(GBaseInitFunc) NULL,                                 \
			(GBaseFinalizeFunc) NULL,                             \
			(GClassInitFunc) prefix##_class_init,                 \
			NULL, NULL,                                           \
			sizeof (class_name),                                  \
			0,                                                    \
			(GInstanceInitFunc) prefix##_init                     \
		};                                                            \
		ptype = (parent);                                             \
		type = matecomponent_type_unique (ptype, NULL, NULL, 0,              \
				           &info, #class_name);               \
	}                                                                     \
	return type;                                                          \
}

#ifdef __cplusplus
}
#endif

#endif
