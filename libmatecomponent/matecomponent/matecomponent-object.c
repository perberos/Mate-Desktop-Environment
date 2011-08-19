/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-object.c: MateComponent Unknown interface base implementation
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright 1999,2001 Ximian, Inc.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib/gi18n-lib.h>

#include <glib-object.h>
#include <gobject/gmarshal.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-foreign-object.h>
#include <matecomponent/matecomponent-shlib-factory.h>
#include <matecomponent/matecomponent-running-context.h>
#include <matecomponent/matecomponent-marshal.h>
#include <matecomponent/matecomponent-types.h>
#include <matecomponent/matecomponent-private.h>
#include <matecomponent/matecomponent-debug.h>

/* We need decent ORB cnx. flushing on shutdown to make this work */
#undef ASYNC_UNREFS

/* Some simple tracking - always on */
static glong   matecomponent_total_aggregates      = 0;
static glong   matecomponent_total_aggregate_refs  = 0;

enum {
  PROP_0,
  PROP_POA
};

/* you may debug by setting MATECOMPONENT_DEBUG_FLAGS environment variable to
   a colon separated list of a subset of {refs,aggregate,lifecycle} */

typedef struct {
	const char *fn;
	gboolean    ref;
	int         line;
} MateComponentDebugRefData;

typedef struct {
	int      ref_count;
	gboolean immortal;
	GList   *objs;
	GList   *bags;
#ifdef G_ENABLE_DEBUG
	/* the following is required for reference debugging */
	GList   *refs;
	int      destroyed;
#endif /* G_ENABLE_DEBUG */
} MateComponentAggregateObject;

struct _MateComponentObjectPrivate {
	MateComponentAggregateObject *ao;
	PortableServer_POA     poa;
};

enum {
	DESTROY,
	SYSTEM_EXCEPTION,
	LAST_SIGNAL
};

static guint matecomponent_object_signals [LAST_SIGNAL];
static GObjectClass *matecomponent_object_parent_class;
static void matecomponent_object_bag_cleanup_T (MateComponentAggregateObject *ao);

#ifdef G_ENABLE_DEBUG
static GHashTable *living_ao_ht = NULL;
#endif /* G_ENABLE_DEBUG */

/* Do not use this function, it is not what you want; see unref */
static void
matecomponent_object_destroy_T (MateComponentAggregateObject *ao)
{
	GList *l;

	g_return_if_fail (ao->ref_count > 0);

	for (l = ao->objs; l; l = l->next) {
		GObject *o = l->data;

		matecomponent_object_bag_cleanup_T (ao);

		if (o->ref_count >= 1) {
			g_object_ref (o);
			MATECOMPONENT_UNLOCK();
			g_signal_emit (o, matecomponent_object_signals [DESTROY], 0);
			MATECOMPONENT_LOCK();
			g_object_unref (o);
		} else
			g_warning ("Serious ref-counting error [%p]", o);
	}
#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) 
		ao->destroyed = TRUE;
#endif /* G_ENABLE_DEBUG */
}

static void
matecomponent_object_corba_deactivate_T (MateComponentObject *object)
{
	CORBA_Environment        ev;
	PortableServer_ObjectId *oid;
	PortableServer_POA       poa;

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_LIFECYCLE)
		matecomponent_debug_print("deactivate",
				   "MateComponentObject corba deactivate %p", object);
#endif /* G_ENABLE_DEBUG */

	g_assert (object->priv->ao == NULL);

	CORBA_exception_init (&ev);

	if (object->corba_objref != CORBA_OBJECT_NIL) {
		matecomponent_running_context_remove_object_T (object->corba_objref);
		CORBA_Object_release (object->corba_objref, &ev);
		object->corba_objref = CORBA_OBJECT_NIL;
	}

	poa = matecomponent_object_get_poa (object);
	oid = PortableServer_POA_servant_to_id (poa, &object->servant, &ev);
	PortableServer_POA_deactivate_object (poa, oid, &ev);
	
	CORBA_free (oid);
	CORBA_exception_free (&ev);
}

/*
 * matecomponent_object_finalize_internal_T:
 * 
 * This method splits apart the aggregate object, so that each
 * GObject can be finalized individualy.
 *
 * Note that since the (embedded) servant keeps a ref on the
 * GObject, it won't neccessarily be finalized through this
 * routine, but from the poa later.
 */
static void
matecomponent_object_finalize_internal_T (MateComponentAggregateObject *ao)
{
	GList *l, *objs;

	g_return_if_fail (ao->ref_count == 0);

	objs = ao->objs;
	ao->objs = NULL;

	for (l = objs; l; l = l->next) {
		GObject *o = G_OBJECT (l->data);

		if (!o)
			g_error ("Serious matecomponent object corruption");
		else {
			g_assert (MATECOMPONENT_OBJECT (o)->priv->ao != NULL);
#ifdef G_ENABLE_DEBUG
			if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
				g_assert (MATECOMPONENT_OBJECT (o)->priv->ao->destroyed);

				matecomponent_debug_print ("finalize", 
						    "[%p] %-20s corba_objref=[%p]"
						    " g_ref_count=%d", o,
						    G_OBJECT_TYPE_NAME (o),
						    MATECOMPONENT_OBJECT (o)->corba_objref,
						    G_OBJECT (o)->ref_count);
			}
#endif /* G_ENABLE_DEBUG */

			/*
			 * Disconnect the GObject from the aggregate object
			 * and unref it so that it is possibly finalized ---
			 * other parts of glib may still have references to it.
			 *
			 * The GObject was already destroy()ed in
			 * matecomponent_object_destroy_T().
			 */

			MATECOMPONENT_OBJECT (o)->priv->ao = NULL;
			if (!g_type_is_a (G_OBJECT_TYPE(o), MATECOMPONENT_TYPE_FOREIGN_OBJECT))
				matecomponent_object_corba_deactivate_T (MATECOMPONENT_OBJECT (o));
			else	/* (is foreign object) */
				matecomponent_running_context_remove_object_T
					(MATECOMPONENT_OBJECT (o)->corba_objref);

			MATECOMPONENT_UNLOCK ();
			g_object_unref (o);
			MATECOMPONENT_LOCK ();
#ifdef G_ENABLE_DEBUG
			if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_LIFECYCLE)
				matecomponent_debug_print ("finalize",
						    "Done finalize internal on %p",
						    o);
#endif /* G_ENABLE_DEBUG */
		}
	}

	g_list_free (objs);

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
		for (l = ao->refs; l; l = l->next)
			g_free (l->data);
		g_list_free (ao->refs);
	}
#endif

	g_free (ao);

	/* Some simple debugging - count aggregate free */
	matecomponent_total_aggregates--;
}


/*
 * matecomponent_object_finalize_servant:
 * 
 * This routine is called from either an object de-activation
 * or from the poa. It is called to signal the fact that finaly
 * the object is no longer exposed to the world and thus we
 * can safely loose it's GObject reference, and thus de-allocate
 * the memory associated with it.
 */
static void
matecomponent_object_finalize_servant (PortableServer_Servant servant,
				CORBA_Environment *ev)
{
	MateComponentObject *object = matecomponent_object(servant);
	MateComponentObjectClass *klass = MATECOMPONENT_OBJECT_GET_CLASS(object);

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_LIFECYCLE)
		matecomponent_debug_print ("finalize",
				    "MateComponentObject Servant finalize %p",
				    object);
#endif /* G_ENABLE_DEBUG */

	if (klass->poa_fini_fn)
		klass->poa_fini_fn (servant, ev);
	else /* Actually quicker and nicer */
		PortableServer_ServantBase__fini (servant, ev);

	g_object_unref (G_OBJECT (object));
}

static void
matecomponent_object_ref_T (MateComponentObject *object)
{
	if (!object->priv->ao->immortal) {
		object->priv->ao->ref_count++;
		matecomponent_total_aggregate_refs++;
	}
}

#ifndef matecomponent_object_ref
/**
 * matecomponent_object_ref:
 * @obj: A MateComponentObject you want to ref-count
 *
 * Increments the reference count for the aggregate MateComponentObject.
 *
 * Returns: @object
 */
gpointer
matecomponent_object_ref (gpointer obj)
{
	MateComponentObject *object = obj;

	if (!object)
		return object;

	g_return_val_if_fail (MATECOMPONENT_IS_OBJECT (object), object);
	g_return_val_if_fail (object->priv->ao->ref_count > 0, object);

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
		matecomponent_object_trace_refs (object, "local", 0, TRUE);
	}
	else
#endif /* G_ENABLE_DEBUG */
	{
		MATECOMPONENT_LOCK ();
		matecomponent_object_ref_T (object);
		MATECOMPONENT_UNLOCK ();
	}

	return object;
}
#endif /* matecomponent_object_ref */


#ifndef matecomponent_object_unref
/**
 * matecomponent_object_unref:
 * @obj: A MateComponentObject you want to unref.
 *
 * Decrements the reference count for the aggregate MateComponentObject.
 *
 * Returns: %NULL.
 */
gpointer
matecomponent_object_unref (gpointer obj)
{
#ifdef G_ENABLE_DEBUG
	if(!(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS)) {
#endif /* G_ENABLE_DEBUG */
		MateComponentAggregateObject *ao;
		MateComponentObject *object = obj;

		if (!object)
			return NULL;

		g_return_val_if_fail (MATECOMPONENT_IS_OBJECT (object), NULL);

		ao = object->priv->ao;
		g_return_val_if_fail (ao != NULL, NULL);
		g_return_val_if_fail (ao->ref_count > 0, NULL);

		MATECOMPONENT_LOCK ();

		if (!ao->immortal) {
			if (ao->ref_count == 1)
				matecomponent_object_destroy_T (ao);
			
			ao->ref_count--;
			matecomponent_total_aggregate_refs--;
		
			if (ao->ref_count == 0)
				matecomponent_object_finalize_internal_T (ao);
		}

		MATECOMPONENT_UNLOCK ();
		return NULL;
#ifdef G_ENABLE_DEBUG
	}
	else
		return matecomponent_object_trace_refs (obj, "local", 0, FALSE);
#endif /* G_ENABLE_DEBUG */
}
#endif /* matecomponent_object_unref */

gpointer
matecomponent_object_trace_refs (gpointer    obj,
			  const char *fn,
			  int         line,
			  gboolean    ref)
{
#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
		MateComponentObject *object = obj;
		MateComponentAggregateObject *ao;
		MateComponentDebugRefData *descr;

		if (!object)
			return NULL;
	
		g_return_val_if_fail (MATECOMPONENT_IS_OBJECT (object), ref ? object : NULL);
		ao = object->priv->ao;
		g_return_val_if_fail (ao != NULL, ref ? object : NULL);
		
		MATECOMPONENT_LOCK ();

		descr  = g_new (MateComponentDebugRefData, 1);
		ao->refs = g_list_prepend (ao->refs, descr);
		descr->fn = fn;
		descr->ref = ref;
		descr->line = line;

		if (ref) {
			g_return_val_if_fail (ao->ref_count > 0, object);

			if (!object->priv->ao->immortal) {
				object->priv->ao->ref_count++;
				matecomponent_total_aggregate_refs++;
			}
		
			matecomponent_debug_print ("ref", "[%p]:[%p]:%s to %d at %s:%d", 
					    object, ao,
					    G_OBJECT_TYPE_NAME (object),
					    ao->ref_count, fn, line);

			MATECOMPONENT_UNLOCK ();

			return object;
		} else { /* unref */
			matecomponent_debug_print ("unref", "[%p]:[%p]:%s from %d at %s:%d", 
					    object, ao,
					    G_OBJECT_TYPE_NAME (object),
					    ao->ref_count, fn, line);

			g_return_val_if_fail (ao->ref_count > 0, NULL);

			if (ao->immortal)
				matecomponent_debug_print ("unusual", "immortal object");
			else {
				if (ao->ref_count == 1) {
					matecomponent_object_destroy_T (ao);
					
					g_return_val_if_fail (ao->ref_count > 0, NULL);
				}
				
				/*
				 * If this blows it is likely some loony used
				 * g_object_unref somewhere instead of
				 * matecomponent_object_unref, send them my regards.
				 */
				g_assert (object->priv->ao == ao);
				
				ao->ref_count--;
				matecomponent_total_aggregate_refs--;
				
				if (ao->ref_count == 0) {
					
					g_assert (g_hash_table_lookup (living_ao_ht, ao) == ao);
					g_hash_table_remove (living_ao_ht, ao);
					
					matecomponent_object_finalize_internal_T (ao);
					
				} else if (ao->ref_count < 0) {
					matecomponent_debug_print ("unusual", 
							    "[%p] already finalized", ao);
				}
			}
			
			MATECOMPONENT_UNLOCK ();

			return NULL;
		}
	}
	else
#endif /* G_ENABLE_DEBUG */
	if (ref)
		return matecomponent_object_ref (obj);
	else {
		matecomponent_object_unref (obj);
		return NULL;
	}
}

static void
impl_MateComponent_Unknown_ref (PortableServer_Servant servant, CORBA_Environment *ev)
{
	MateComponentObject *object;

	object = matecomponent_object_from_servant (servant);

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
#ifndef matecomponent_object_ref
		matecomponent_object_trace_refs (object, "remote", 0, TRUE);
#else
		matecomponent_object_ref (object);
#endif
	}
	else
#endif /* G_ENABLE_DEBUG */
		matecomponent_object_ref (object);
}

void
matecomponent_object_set_immortal (MateComponentObject *object,
			    gboolean      immortal)
{
	MateComponentAggregateObject *ao;

	g_return_if_fail (MATECOMPONENT_IS_OBJECT (object));
	g_return_if_fail (object->priv != NULL);
	g_return_if_fail (object->priv->ao != NULL);

	ao = object->priv->ao;

	ao->immortal = immortal;
}

/**
 * matecomponent_object_dup_ref:
 * @object: a MateComponent_Unknown corba object
 * @opt_ev: an optional exception environment
 * 
 *   This function returns a duplicated CORBA Object reference;
 * it also bumps the ref count on the object. This is ideal to
 * use in any method returning a MateComponent_Object in a CORBA impl.
 * If object is CORBA_OBJECT_NIL it is returned unaffected.
 * 
 * Return value: duplicated & ref'd corba object reference.
 **/
MateComponent_Unknown
matecomponent_object_dup_ref (MateComponent_Unknown     object,
		       CORBA_Environment *opt_ev)
{
	MateComponent_Unknown    ans;
	CORBA_Environment  *ev, temp_ev;
       
	if (object == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	MateComponent_Unknown_ref (object, ev);
	ans = CORBA_Object_duplicate (object, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);

	return ans;
}

#ifdef ASYNC_UNREFS
static MateCORBA_IMethod *
get_unknown_unref_imethod (void)
{
	static MateCORBA_IMethod *imethod = NULL;

	if (!imethod) {
		guint i;
		MateCORBA_IMethods *methods;

		methods = &MateComponent_Unknown__iinterface.methods;

		for (i = 0; i < methods->_length; i++) {
			if (!strcmp (methods->_buffer [i].name,
				     "unref"))
				imethod = &methods->_buffer [i];
		}
		g_assert (imethod != NULL);
	}

	return imethod;
}
#endif

/**
 * matecomponent_object_release_unref:
 * @object: a MateComponent_Unknown corba object
 * @opt_ev: an optional exception environment
 * 
 *   This function releases a CORBA Object reference;
 * it also decrements the ref count on the matecomponent object.
 * This is the converse of matecomponent_object_dup_ref. We
 * tolerate object == CORBA_OBJECT_NIL silently.
 *
 * Returns: %CORBA_OBJECT_NIL.
 **/
MateComponent_Unknown
matecomponent_object_release_unref (MateComponent_Unknown     object,
			     CORBA_Environment *opt_ev)
{
	CORBA_Environment *ev, temp_ev;

	if (object == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	MateComponent_Unknown_unref (object, ev);
#ifdef ASYNC_UNREFS
	if (MateCORBA_small_get_servant (object))
		MateComponent_Unknown_unref (object, ev);
	else
		MateCORBA_small_invoke_async
			(object, get_unknown_unref_imethod (),
			NULL, NULL, NULL, NULL, ev);*/
#endif
	
	CORBA_Object_release (object, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);

	return CORBA_OBJECT_NIL;
}

static void
impl_MateComponent_Unknown_unref (PortableServer_Servant servant, CORBA_Environment *ev)
{
	MateComponentObject *object;

	object = matecomponent_object_from_servant (servant);

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
#ifndef matecomponent_object_unref
		matecomponent_object_trace_refs (object, "remote", 0, FALSE);
#else
		matecomponent_object_unref (object);
#endif
	}
	else
#endif /* G_ENABLE_DEBUG */
		matecomponent_object_unref (object);
}

/**
 * matecomponent_object_query_local_interface:
 * @object: A #MateComponentObject which is the aggregate of multiple objects.
 * @repo_id: The id of the interface being queried.
 *
 * Returns: A #MateComponentObject for the requested interface.
 */
MateComponentObject *
matecomponent_object_query_local_interface (MateComponentObject *object,
				     const char   *repo_id)
{
	GList             *l;
	CORBA_Environment  ev;

	g_return_val_if_fail (MATECOMPONENT_IS_OBJECT (object), NULL);

	CORBA_exception_init (&ev);

	for (l = object->priv->ao->objs; l; l = l->next){
		MateComponentObject *tryme = l->data;

		if (CORBA_Object_is_a (
			tryme->corba_objref, (char *) repo_id, &ev)) {

			if (MATECOMPONENT_EX (&ev)) {
				CORBA_exception_free (&ev);
				continue;
			}
			
			matecomponent_object_ref_T (object);

			return tryme;
		}
	}

	CORBA_exception_free (&ev);

	return NULL;
}

static CORBA_Object
impl_MateComponent_Unknown_queryInterface (PortableServer_Servant  servant,
				    const CORBA_char       *repoid,
				    CORBA_Environment      *ev)
{
	MateComponentObject *object = matecomponent_object_from_servant (servant);
	MateComponentObject *local_interface;

	local_interface = matecomponent_object_query_local_interface (
		object, repoid);

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
		matecomponent_debug_print ("query-interface", 
				    "[%p]:[%p]:%s repoid=%s", 
				    object, object->priv->ao,
				    G_OBJECT_TYPE_NAME (object),
				    repoid);
	}
#endif /* G_ENABLE_DEBUG */

	if (local_interface == NULL)
		return CORBA_OBJECT_NIL;

	return CORBA_Object_duplicate (local_interface->corba_objref, ev);
}

static void
matecomponent_object_epv_init (POA_MateComponent_Unknown__epv *epv)
{
	epv->ref            = impl_MateComponent_Unknown_ref;
	epv->unref          = impl_MateComponent_Unknown_unref;
	epv->queryInterface = impl_MateComponent_Unknown_queryInterface;
}

static void
matecomponent_object_finalize_gobject (GObject *gobject)
{
	MateComponentObject *object = (MateComponentObject *) gobject;

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_LIFECYCLE)
		matecomponent_debug_print ("finalize",
				    "MateComponent Object finalize GObject %p",
				    gobject);
#endif /* G_ENABLE_DEBUG */

	if (object->priv->ao != NULL)
		g_error ("g_object_unreffing a matecomponent_object that "
			 "still has %d refs", object->priv->ao->ref_count);

	g_free (object->priv);

	matecomponent_object_parent_class->finalize (gobject);
}

static void
matecomponent_object_dummy_destroy (MateComponentObject *object)
{
	/* Just to make chaining possibly cleaner */
}

static void
matecomponent_object_set_property (GObject         *g_object,
			    guint            prop_id,
			    const GValue    *value,
			    GParamSpec      *pspec)
{
	MateComponentObject *object = (MateComponentObject *) g_object;

	switch (prop_id) {
	case PROP_POA:
		object->priv->poa = g_value_get_pointer (value);
		break;
	default:
		break;
	}
}

static void
matecomponent_object_get_property (GObject         *g_object,
			    guint            prop_id,
			    GValue          *value,
			    GParamSpec      *pspec)
{
	MateComponentObject *object = (MateComponentObject *) g_object;

	switch (prop_id) {
	case PROP_POA:
		g_value_set_pointer (value, object->priv->poa);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
do_corba_setup_T (MateComponentObject *object)
{
	CORBA_Object obj;
	CORBA_Environment ev[1];
	MateComponentObjectClass *xklass;
	MateComponentObjectClass *klass = MATECOMPONENT_OBJECT_GET_CLASS (object);

	CORBA_exception_init (ev);

	/* Setup the servant structure */
	object->servant._private = NULL;
	object->servant.vepv     = klass->vepv;

	/* Initialize the servant structure with our POA__init fn */
	{
		for (xklass = klass; xklass && !xklass->poa_init_fn;)
			xklass = g_type_class_peek_parent (xklass);
		if (!xklass || !xklass->epv_struct_offset) {
			/*   Also, people using MATECOMPONENT_TYPE_FUNC instead of
			 * MATECOMPONENT_TYPE_FUNC_FULL might see this; you need
			 * to tell it about the CORBA interface you're
			 * implementing - of course */
			g_warning ("It looks like you used g_type_unique "
				   "instead of b_type_unique on type '%s'",
				   G_OBJECT_CLASS_NAME (klass));
			return;
		}
		xklass->poa_init_fn ((PortableServer_Servant) &object->servant, ev);
		if (MATECOMPONENT_EX (ev)) {
			char *text = matecomponent_exception_get_text (ev);
			g_warning ("Exception initializing servant '%s'", text);
			g_free (text);
			return;
		}
	}

	/*  Instantiate a CORBA_Object reference for the servant
	 * assumes the matecomponent POA supports implicit activation */
	obj = PortableServer_POA_servant_to_reference (
		matecomponent_object_get_poa (object), &object->servant, ev);

	if (MATECOMPONENT_EX (ev)) {
		g_warning ("Exception '%s' getting reference for servant",
			   matecomponent_exception_get_text (ev));
		return;
	}

	object->corba_objref = obj;
	matecomponent_running_context_add_object_T (obj);

#ifdef G_ENABLE_DEBUG
	if (!CORBA_Object_is_a (obj, "IDL:MateComponent/Unknown:1.0", ev))
		g_error ("Attempt to instantiate non-MateComponent/Unknown "
			 "derived object via. MateComponentObject");
#endif

	CORBA_exception_free (ev);
}

static GObject *
matecomponent_object_constructor (GType                  type,
			   guint                  n_construct_properties,
			   GObjectConstructParam *construct_properties)
{
	GObject *g_object;
	MateComponentObject *object;

	g_object = matecomponent_object_parent_class->constructor
		(type, n_construct_properties, construct_properties);
	if (g_object) {
		object = (MateComponentObject *) g_object;

		/* Though this make look strange, destruction of this object
		   can only occur when the servant is deactivated by the poa.
		   The poa maintains its own ref count over method invocations
		   and delays finalization which happens only after:
		   matecomponent_object_finalize_servant: is invoked */
		g_object_ref (g_object);

		MATECOMPONENT_LOCK ();
#ifdef G_ENABLE_DEBUG
		if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
			MateComponentAggregateObject *ao = object->priv->ao;

			matecomponent_debug_print ("create", "[%p]:[%p]:%s to %d on poa %p", object, ao,
					    g_type_name (type), ao->ref_count, object->priv->poa);

			g_assert (g_hash_table_lookup (living_ao_ht, ao) == NULL);
			g_hash_table_insert (living_ao_ht, ao, ao);
		}
#endif /* G_ENABLE_DEBUG */
		if (!g_type_is_a (type, MATECOMPONENT_TYPE_FOREIGN_OBJECT))
			do_corba_setup_T (object);
		MATECOMPONENT_UNLOCK ();
	}
	
	return g_object;
}

/* VOID:CORBA_OBJECT,BOXED */
static void
matecomponent_marshal_VOID__CORBA_BOXED (GClosure     *closure,
				  GValue       *return_value,
				  guint         n_param_values,
				  const GValue *param_values,
				  gpointer      invocation_hint,
				  gpointer      marshal_data)
{
	typedef void (*GMarshalFunc_VOID__OBJECT_BOXED) (gpointer     data1,
							 gpointer     arg_1,
							 gpointer     arg_2,
							 gpointer     data2);
	register GMarshalFunc_VOID__OBJECT_BOXED callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;
	CORBA_Object arg1;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__OBJECT_BOXED) (
		marshal_data ? marshal_data : cc->callback);

	arg1 = matecomponent_value_get_corba_object (param_values + 1);
	callback (data1, arg1, g_value_get_boxed (param_values + 2), data2);
	CORBA_Object_release (arg1, NULL);
}

static void
matecomponent_object_class_init (MateComponentObjectClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	/* Ensure that the signature checking is going to work */
	g_assert (sizeof (POA_MateComponent_Unknown) == sizeof (gpointer) * 2);
	g_assert (sizeof (MateComponentObjectHeader) * 2 == sizeof (MateComponentObject));

	if (G_UNLIKELY (!matecomponent_is_initialized()) )
		g_warning ("MateComponent must be initialized before use");
	
	matecomponent_object_parent_class = g_type_class_peek_parent (klass);

	object_class->set_property = matecomponent_object_set_property;
	object_class->get_property = matecomponent_object_get_property;
	object_class->constructor  = matecomponent_object_constructor;
	object_class->finalize     = matecomponent_object_finalize_gobject;
	klass->destroy = matecomponent_object_dummy_destroy;

	matecomponent_object_signals [DESTROY] =
		g_signal_new ("destroy",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentObjectClass,destroy),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	matecomponent_object_signals [SYSTEM_EXCEPTION] =
		g_signal_new ("system_exception",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentObjectClass,system_exception),
			      NULL, NULL,
			      matecomponent_marshal_VOID__CORBA_BOXED,
			      G_TYPE_NONE, 2,
			      MATECOMPONENT_TYPE_STATIC_CORBA_OBJECT,
			      MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION);

	g_object_class_install_property
		(object_class, PROP_POA,
		 g_param_spec_pointer
			("poa", _("POA"), _("Custom CORBA POA"),
			 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
matecomponent_object_instance_init (GObject    *g_object,
			     GTypeClass *klass)
{
	MateComponentObject *object = MATECOMPONENT_OBJECT (g_object);
	MateComponentAggregateObject *ao;

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_OBJECT) {
		matecomponent_debug_print ("object",
				    "matecomponent_object_instance init '%s' '%s' -> %p",
				    G_OBJECT_TYPE_NAME (g_object),
				    G_OBJECT_CLASS_NAME (klass), object);
	}
#endif /* G_ENABLE_DEBUG */

	/* Some simple debugging - count aggregate allocate */
	MATECOMPONENT_LOCK ();
	matecomponent_total_aggregates++;
	matecomponent_total_aggregate_refs++;
	MATECOMPONENT_UNLOCK ();

	/* Setup aggregate */
	ao            = g_new0 (MateComponentAggregateObject, 1);
	ao->objs      = g_list_append (ao->objs, object);
	ao->ref_count = 1;

	/* Setup Private fields */
	object->priv = g_new (MateComponentObjectPrivate, 1);
	object->priv->ao = ao;
	object->priv->poa = NULL;

	/* Setup signatures */
	object->object_signature  = MATECOMPONENT_OBJECT_SIGNATURE;
	object->servant_signature = MATECOMPONENT_SERVANT_SIGNATURE;
}

/**
 * matecomponent_object_get_type:
 *
 * Returns: the GType associated with the base MateComponentObject class type.
 */
GType
matecomponent_object_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (MateComponentObjectClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) matecomponent_object_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentObject),
			0, /* n_preallocs */
			(GInstanceInitFunc) matecomponent_object_instance_init
		};
		
#ifdef G_ENABLE_DEBUG
		matecomponent_debug_init();
		if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS)
			living_ao_ht = g_hash_table_new (NULL, NULL);
#endif /* G_ENABLE_DEBUG */
		type = g_type_register_static (G_TYPE_OBJECT, "MateComponentObject",
					       &info, 0);
	}

	return type;
}

#ifdef G_ENABLE_DEBUG
static void
matecomponent_ao_debug_foreach (gpointer key, gpointer value, gpointer user_data)
{
	MateComponentAggregateObject *ao = value;
	GList *l;

	g_return_if_fail (ao != NULL);

	matecomponent_debug_print ("object-status", 
			    "[%p] %-20s ref_count=%d, interfaces=%d", ao, "",
			    ao->ref_count, g_list_length (ao->objs));
		
	for (l = ao->objs; l; l = l->next) {
		MateComponentObject *object = MATECOMPONENT_OBJECT (l->data);
		
		matecomponent_debug_print ("", "[%p] %-20s corba_objref=[%p]"
				    " g_ref_count=%d", object,
				    G_OBJECT_TYPE_NAME (object),
				    object->corba_objref,
				    G_OBJECT (object)->ref_count);
	}

	l = g_list_last (ao->refs);

	if (l)
		matecomponent_debug_print ("referencing" ,"");

	for (; l; l = l->prev) {
		MateComponentDebugRefData *descr = l->data;

		matecomponent_debug_print ("", "%-7s - %s:%d", 
				    descr->ref ? "ref" : "unref",
				    descr->fn, descr->line);
	}
}
#endif /* G_ENABLE_DEBUG */

int
matecomponent_object_shutdown (void)
{
#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {	
		matecomponent_debug_print ("shutdown-start", 
				    "-------------------------------------------------");

		if (living_ao_ht)
			g_hash_table_foreach (living_ao_ht,
					      matecomponent_ao_debug_foreach, NULL);
		
		matecomponent_debug_print ("living-objects",
				    "living matecomponent objects count = %d",
				    g_hash_table_size (living_ao_ht));
		
		matecomponent_debug_print ("shutdown-end", 
				    "-------------------------------------------------");
	}
#endif /* G_ENABLE_DEBUG */

	if (matecomponent_total_aggregates > 0) {
		g_warning ("Leaked a total of %ld refs to %ld matecomponent object(s)",
			   matecomponent_total_aggregate_refs,
			   matecomponent_total_aggregates);
		return 1;
	}

	return 0;
}

/**
 * matecomponent_object_add_interface:
 * @object: The MateComponentObject to which an interface is going to be added.
 * @newobj: The MateComponentObject containing the new interface to be added.
 *
 * Adds the interfaces supported by @newobj to the list of interfaces
 * for @object.  This function adds the interfaces supported by
 * @newobj to the list of interfaces support by @object. It should never
 * be used when the object has been exposed to the world. This is a firm
 * part of the contract.
 */
void
matecomponent_object_add_interface (MateComponentObject *object, MateComponentObject *newobj)
{
       MateComponentAggregateObject *oldao, *ao;
       GList *l;

       g_return_if_fail (object->priv->ao->ref_count > 0);
       g_return_if_fail (newobj->priv->ao->ref_count > 0);

       if (object->priv->ao == newobj->priv->ao)
               return;

       if (newobj->corba_objref == CORBA_OBJECT_NIL)
	       g_warning ("Adding an interface with a NULL Corba objref");

       /*
	* Explanation:
	*   MateComponent Objects should not be assembled after they have been
	*   exposed, or we would be breaking the contract we have with
	*   the other side.
	*/

       ao = object->priv->ao;
       oldao = newobj->priv->ao;
       ao->ref_count = ao->ref_count + oldao->ref_count - 1;
       matecomponent_total_aggregate_refs--;

#ifdef G_ENABLE_DEBUG
       if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
	       matecomponent_debug_print ("add_interface", 
				   "[%p]:[%p]:%s to [%p]:[%p]:%s ref_count=%d", 
				   object, object->priv->ao,
				   G_OBJECT_TYPE_NAME (object),
				   newobj, newobj->priv->ao,
				   G_OBJECT_TYPE_NAME (newobj),
				   ao->ref_count);
       }
#endif /* G_ENABLE_DEBUG */

       /* Merge the two AggregateObject lists */
       for (l = oldao->objs; l; l = l->next) {
	       MateComponentObject *new_if = l->data;

#ifdef G_ENABLE_DEBUG
	       if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_AGGREGATE) {
		       GList *i;
		       CORBA_Environment ev;
		       CORBA_char *new_id;
		       
		       CORBA_exception_init (&ev);

		       new_id = MateCORBA_small_get_type_id (new_if->corba_objref, &ev);

		       for (i = ao->objs; i; i = i->next) {
			       MateComponentObject *old_if = i->data;

			       if (old_if == new_if)
				       g_error ("attempting to merge identical "
						"interfaces [%p]", new_if);
			       else {
				       CORBA_char *old_id;
				       
				       old_id = MateCORBA_small_get_type_id (old_if->corba_objref, &ev);
				       
				       if (!strcmp (new_id, old_id))
					       g_error ("Aggregating two MateComponentObject that implement "
							"the same interface '%s' [%p]", new_id, new_if);
				       CORBA_free (old_id);
			       }
		       }
		       
		       CORBA_free (new_id);
		       CORBA_exception_free (&ev);
	       }
#endif /* G_ENABLE_DEBUG */

	       ao->objs = g_list_prepend (ao->objs, new_if);
	       new_if->priv->ao = ao;
       }

       g_assert (newobj->priv->ao == ao);

#ifdef G_ENABLE_DEBUG
       if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_REFS) {
	       MATECOMPONENT_LOCK ();
	       g_assert (g_hash_table_lookup (living_ao_ht, oldao) == oldao);
	       g_hash_table_remove (living_ao_ht, oldao);
	       ao->refs = g_list_concat (ao->refs, oldao->refs);
	       MATECOMPONENT_UNLOCK ();
       }
#endif /* G_ENABLE_DEBUG */

       g_list_free (oldao->objs);
       g_free (oldao);

       /* Some simple debugging - count aggregate free */
       MATECOMPONENT_LOCK ();
       matecomponent_total_aggregates--;
       MATECOMPONENT_UNLOCK ();
}

/**
 * matecomponent_object_query_interface:
 * @object: A MateComponentObject to be queried for a given interface.
 * @repo_id: The name of the interface to be queried.
 * @opt_ev: optional exception environment
 *
 * Returns: The CORBA interface named @repo_id for @object.
 */
CORBA_Object
matecomponent_object_query_interface (MateComponentObject      *object,
			       const char        *repo_id,
			       CORBA_Environment *opt_ev)
{
	CORBA_Object retval;
	CORBA_Environment  *ev, temp_ev;
       
	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	retval = MateComponent_Unknown_queryInterface (
		object->corba_objref, repo_id, ev);

	if (MATECOMPONENT_EX (ev))
		retval = CORBA_OBJECT_NIL;

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);

	return retval;
}

/**
 * matecomponent_object_corba_objref:
 * @object: A MateComponentObject whose CORBA object is requested.
 *
 * Returns: The CORBA interface object for which @object is a wrapper.
 */
CORBA_Object
matecomponent_object_corba_objref (MateComponentObject *object)
{
	g_return_val_if_fail (MATECOMPONENT_IS_OBJECT (object), NULL);

	return object->corba_objref;
}

/**
 * matecomponent_object_check_env:
 * @object: The object on which we operate
 * @ev: CORBA Environment to check
 *
 * This routine verifies the @ev environment for any fatal system
 * exceptions.  If a system exception occurs, the object raises a
 * "system_exception" signal.  The idea is that GObjects which are
 * used to wrap a CORBA interface can use this function to notify
 * the user if a fatal exception has occurred, causing the object
 * to become defunct.
 */
void
matecomponent_object_check_env (MateComponentObject      *object,
			 CORBA_Object       obj,
			 CORBA_Environment *ev)
{
	g_return_if_fail (ev != NULL);
	g_return_if_fail (MATECOMPONENT_IS_OBJECT (object));

	if (!MATECOMPONENT_EX (ev))
		return;

	if (ev->_major == CORBA_SYSTEM_EXCEPTION)
		g_signal_emit (
			G_OBJECT (object),
			matecomponent_object_signals [SYSTEM_EXCEPTION],
			0, obj, ev);
}

/**
 * matecomponent_unknown_ping:
 * @object: a CORBA object reference of type MateComponent::Unknown
 * @opt_ev: optional exception environment
 *
 * Pings the object @object using the ref/unref methods from MateComponent::Unknown.
 * You can use this one to see if a remote object has gone away.
 *
 * Returns: %TRUE if the MateComponent::Unknown @object is alive.
 */
gboolean
matecomponent_unknown_ping (MateComponent_Unknown     object,
		     CORBA_Environment *opt_ev)
{
	gboolean           alive;
	MateComponent_Unknown     unknown;
	CORBA_Environment *ev, temp_ev;
       
	g_return_val_if_fail (object != NULL, FALSE);

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	alive = FALSE;

	unknown = CORBA_Object_duplicate (object, ev);

	MateComponent_Unknown_ref (unknown, ev);

	if (!MATECOMPONENT_EX (ev)) {
		MateComponent_Unknown_unref (unknown, ev);
		if (!MATECOMPONENT_EX (ev))
			alive = TRUE;
	}

	CORBA_Object_release (unknown, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);

	return alive;
}

void
matecomponent_object_dump_interfaces (MateComponentObject *object)
{
	MateComponentAggregateObject *ao;
	GList                 *l;
	CORBA_Environment      ev;

	g_return_if_fail (MATECOMPONENT_IS_OBJECT (object));

	ao = object->priv->ao;
	
	CORBA_exception_init (&ev);

	fprintf (stderr, "references %d\n", ao->ref_count);
	for (l = ao->objs; l; l = l->next) {
		MateComponentObject *o = l->data;
		
		g_return_if_fail (MATECOMPONENT_IS_OBJECT (o));

		if (o->corba_objref != CORBA_OBJECT_NIL) {
			CORBA_char   *type_id;

			type_id = MateCORBA_small_get_type_id (o->corba_objref, &ev);
			fprintf (stderr, "I/F: '%s'\n", type_id);
			CORBA_free (type_id);
		} else
			fprintf (stderr, "I/F: NIL error\n");
	}

	CORBA_exception_free (&ev);
}

static gboolean
idle_unref_fn (MateComponentObject *object)
{
	matecomponent_object_unref (object);

	return FALSE;
}

void
matecomponent_object_idle_unref (gpointer object)
{
	g_return_if_fail (MATECOMPONENT_IS_OBJECT (object));

	g_idle_add ((GSourceFunc) idle_unref_fn, object);
}

static void
unref_list (GSList *l)
{
	for (; l; l = l->next)
		matecomponent_object_unref (l->data);
}

/**
 * matecomponent_object_list_unref_all:
 * @list: A list of MateComponentObjects *s
 * 
 *  This routine unrefs all valid objects in
 * the list and then removes them from @list if
 * they have not already been so removed.
 **/
void
matecomponent_object_list_unref_all (GList **list)
{
	GList *l;
	GSList *unrefs = NULL, *u;

	g_return_if_fail (list != NULL);

	for (l = *list; l; l = l->next) {
		if (l->data && !MATECOMPONENT_IS_OBJECT (l->data))
			g_warning ("Non object in unref list");
		else if (l->data)
			unrefs = g_slist_prepend (unrefs, l->data);
	}

	unref_list (unrefs);

	for (u = unrefs; u; u = u->next)
		*list = g_list_remove (*list, u->data);

	g_slist_free (unrefs);
}

/**
 * matecomponent_object_list_unref_all:
 * @list: A list of MateComponentObjects *s
 * 
 *  This routine unrefs all valid objects in
 * the list and then removes them from @list if
 * they have not already been so removed.
 **/
void
matecomponent_object_slist_unref_all (GSList **list)
{
	GSList *l;
	GSList *unrefs = NULL, *u;

	g_return_if_fail (list != NULL);

	for (l = *list; l; l = l->next) {
		if (l->data && !MATECOMPONENT_IS_OBJECT (l->data))
			g_warning ("Non object in unref list");
		else if (l->data)
			unrefs = g_slist_prepend (unrefs, l->data);
	}

	unref_list (unrefs);

	for (u = unrefs; u; u = u->next)
		*list = g_slist_remove (*list, u->data);

	g_slist_free (unrefs);
}

/**
 * matecomponent_object:
 * @p: a pointer to something
 * 
 * This function can be passed a MateComponentObject * or a
 * PortableServer_Servant, and it will return a MateComponentObject *.
 * 
 * Return value: a MateComponentObject or NULL on error.
 **/
MateComponentObject *
matecomponent_object (gpointer p)
{
	MateComponentObjectHeader *header;

	if (!p)
		return NULL;

	header = (MateComponentObjectHeader *) p;

	if (header->object_signature == MATECOMPONENT_OBJECT_SIGNATURE)
		return (MateComponentObject *) p;

	else if (header->object_signature == MATECOMPONENT_SERVANT_SIGNATURE)
		return (MateComponentObject *)(((guchar *) header) -
					MATECOMPONENT_OBJECT_HEADER_SIZE);

	g_warning ("Serious servant -> object mapping error '%p'", p);

	return NULL;
}

/**
 * matecomponent_type_setup:
 * @type: The type to initialize
 * @init_fn: the POA_init function for the CORBA interface or NULL
 * @fini_fn: NULL or a custom POA free fn.
 * @epv_struct_offset: the offset in the class structure where the epv is or 0
 * 
 *   This function initializes a type derived from MateComponentObject, such that
 * when you instantiate a new object of this type with g_type_new the
 * CORBA object will be correctly created and embedded.
 * 
 * Return value: TRUE on success, FALSE on error.
 **/
gboolean
matecomponent_type_setup (GType             type,
		   MateComponentObjectPOAFn init_fn,
		   MateComponentObjectPOAFn fini_fn,
		   int               epv_struct_offset)
{
	GType       p, b_type;
	int           depth;
	MateComponentObjectClass *klass;
	gpointer     *vepv;

	/* Setup our class data */
	klass = g_type_class_ref (type);
	klass->epv_struct_offset = epv_struct_offset;
	klass->poa_init_fn       = init_fn;
	klass->poa_fini_fn       = fini_fn;

	/* Calculate how far down the tree we are in epvs */
	b_type = matecomponent_object_get_type ();
	for (depth = 0, p = type; p && p != b_type;
	     p = g_type_parent (p)) {
		MateComponentObjectClass *xklass;

		xklass = g_type_class_peek (p);

		if (xklass->epv_struct_offset)
			depth++;
	}
	if (!p) {
		g_warning ("Trying to inherit '%s' from a MateComponentObject, but "
			   "no MateComponentObject in the ancestory",
			   g_type_name (type));
		return FALSE;
	}

#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_OBJECT) {
		matecomponent_debug_print ("object", "We are at depth %d with type '%s'",
				    depth, g_type_name (type));
	}
#endif /* G_ENABLE_DEBUG */

	/* Setup the Unknown epv */
	matecomponent_object_epv_init (&klass->epv);
	klass->epv._private = NULL;

	klass->base_epv._private = NULL;
	klass->base_epv.finalize = matecomponent_object_finalize_servant;
	klass->base_epv.default_POA = NULL;

	vepv = g_new0 (gpointer, depth + 2);
	klass->vepv = (POA_MateComponent_Unknown__vepv *) vepv;
	klass->vepv->_base_epv = &klass->base_epv;
	klass->vepv->MateComponent_Unknown_epv = &klass->epv;

	/* Build our EPV */
	if (depth > 0) {
		int i;

		for (p = type, i = depth; i > 0;) {
			MateComponentObjectClass *xklass;

			xklass = g_type_class_peek (p);

			if (xklass->epv_struct_offset) {
				vepv [i + 1] = ((guchar *)klass) +
					xklass->epv_struct_offset;
				i--;
			}

			p = g_type_parent (p);
		}
	}

	return TRUE;
}

/**
 * matecomponent_type_unique:
 * @parent_type: the parent GType
 * @init_fn: a POA initialization function
 * @fini_fn: a POA finialization function or NULL
 * @epv_struct_offset: the offset into the struct that the epv
 * commences at, or 0 if we are inheriting a plain GObject
 * from a MateComponentObject, adding no new CORBA interfaces
 * @info: the standard GTypeInfo.
 * @type_name: the name of the type being registered.
 * 
 * This function is the main entry point for deriving matecomponent
 * server interfaces.
 * 
 * Return value: the constructed GType.
 **/
GType
matecomponent_type_unique (GType             parent_type,
		    MateComponentObjectPOAFn init_fn,
		    MateComponentObjectPOAFn fini_fn,
		    int               epv_struct_offset,
		    const GTypeInfo  *info,
		    const gchar      *type_name)
{
	GType       type;

	/*
	 * Since we call g_type_class after the g_type_unique
	 * and before we can return the type to the get_type fn.
	 * it is possible we can re-enter here through eg. a
	 * type check macro, hence we need this guard.
	 */
	if ((type = g_type_from_name (type_name)))
		return type;

	type = g_type_register_static (parent_type, type_name, info, 0);
	if (!type)
		return 0;

	if (matecomponent_type_setup (type, init_fn, fini_fn,
			       epv_struct_offset))
		return type;
	else
		return 0;
}

/**
 * matecomponent_object_query_remote:
 * @unknown: an unknown object ref ( or NIL )
 * @repo_id: the interface to query for
 * @opt_ev: an optional exception environment
 * 
 * A helper wrapper for query interface
 * 
 * Return value: the interface or CORBA_OBJECT_NIL
 **/
MateComponent_Unknown
matecomponent_object_query_remote (MateComponent_Unknown     unknown,
			    const char        *repo_id,
			    CORBA_Environment *opt_ev)
{
	MateComponent_Unknown new_if;
	CORBA_Environment *ev, temp_ev;
       
	if (unknown == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	new_if = MateComponent_Unknown_queryInterface (
		unknown, repo_id, ev);

	if (MATECOMPONENT_EX (ev))
		new_if = CORBA_OBJECT_NIL;

	if (!opt_ev)
		CORBA_exception_free (ev);

	return new_if;
}

/**
 * matecomponent_object_get_poa:
 * @object: the object associated with an interface
 * 
 * Gets the POA associated with this part of the
 * MateComponentObject aggregate it is possible to have
 * different POAs per interface.
 * 
 * Return value: the poa, never NIL.
 **/
PortableServer_POA
matecomponent_object_get_poa (MateComponentObject *object)
{
	g_return_val_if_fail (object != CORBA_OBJECT_NIL, matecomponent_poa ());
	if (object->priv->poa)
		return object->priv->poa;
	else
		return matecomponent_poa ();
}

void
matecomponent_object_set_poa (MateComponentObject      *object,
		       PortableServer_POA poa)
{
	/* FIXME: implement me */
	g_warning ("This method is a pain, it needs hooks into "
		   "matecomponent_object_corba_objref");
}


/* ----------- a weak referenced MateComponentObject cache object ----------- */

struct _MateComponentObjectBag {
	gulong         size;
	GHashTable    *key_to_object;
	GHashTable    *object_to_key;
	MateComponentCopyFunc key_copy_func;
	GDestroyNotify key_destroy_func;
};

MateComponentObjectBag *
matecomponent_object_bag_new (GHashFunc       hash_func,
		       GEqualFunc      key_equal_func,
		       MateComponentCopyFunc  key_copy_func,
		       GDestroyNotify  key_destroy_func)
{
	MateComponentObjectBag *bag;

	g_return_val_if_fail (hash_func != NULL, NULL);
	g_return_val_if_fail (key_copy_func != NULL, NULL);
	g_return_val_if_fail (key_equal_func != NULL, NULL);
	g_return_val_if_fail (key_destroy_func != NULL, NULL);

	bag = g_new0 (MateComponentObjectBag, 1);
	bag->key_to_object = g_hash_table_new (hash_func, key_equal_func);
	bag->object_to_key = g_hash_table_new (NULL, NULL);
	bag->key_copy_func = key_copy_func;
	bag->key_destroy_func = key_destroy_func;

	return bag;
}

MateComponentObject *
matecomponent_object_bag_get_ref (MateComponentObjectBag *bag,
			   gconstpointer    key)
{
	MateComponentObject *obj;
	MateComponentAggregateObject *ao;

	g_return_val_if_fail (bag != NULL, NULL);

	MATECOMPONENT_LOCK();
	ao = g_hash_table_lookup (bag->key_to_object, key);
	if (ao)
		obj = matecomponent_object_ref (ao->objs->data);
	else
		obj = NULL;
	MATECOMPONENT_UNLOCK();

	return obj;
}

gboolean
matecomponent_object_bag_add_ref (MateComponentObjectBag *bag,
			   gconstpointer    key,
			   MateComponentObject    *object)
{
	gboolean success;

	g_return_val_if_fail (bag != NULL, FALSE);
	g_return_val_if_fail (object != NULL, FALSE);

	MATECOMPONENT_LOCK();

	if (g_hash_table_lookup (bag->key_to_object, key))
		success = FALSE;

	else if (g_hash_table_lookup (bag->object_to_key, object)) {
		success = FALSE;
		g_warning ("Adding the same object with two keys");

	} else {
		gpointer insert_key;
		MateComponentAggregateObject *ao = object->priv->ao;

		success = TRUE;

		bag->size++;
		insert_key = bag->key_copy_func (key);
		g_hash_table_insert (bag->key_to_object, insert_key, ao);
		g_hash_table_insert (bag->object_to_key, ao, insert_key);

		ao->bags = g_list_prepend (ao->bags, bag);
	}

	MATECOMPONENT_UNLOCK();

	return success;
}

static void
matecomponent_object_bag_cleanup_T (MateComponentAggregateObject *ao)
{
	GList *l;

	for (l = ao->bags; l; l = l->next) {
		gpointer key;
		MateComponentObjectBag *bag = l->data;

		key = g_hash_table_lookup (bag->object_to_key, ao);
		g_hash_table_remove (bag->object_to_key, ao);
		g_hash_table_remove (bag->key_to_object, key);

		g_warning ("FIXME: free the keys outside the lock");
	}
}

void
matecomponent_object_bag_remove  (MateComponentObjectBag *bag,
			   gconstpointer    key)
{
	gpointer hash_key = NULL;
	MateComponentObject *object;

	g_return_if_fail (bag != NULL);

	MATECOMPONENT_LOCK();

	if ((object = g_hash_table_lookup (bag->key_to_object, key))) {
		g_hash_table_remove (bag->key_to_object, key);
		hash_key = g_hash_table_lookup (bag->object_to_key, object);
		g_hash_table_remove (bag->object_to_key, object);
		bag->size--;
	}

	MATECOMPONENT_UNLOCK();

	bag->key_destroy_func (hash_key);
}

static void
bag_collect_ref_list_cb (gpointer key,
			 gpointer value,
			 gpointer user_data)
{
	g_ptr_array_add (user_data, matecomponent_object_ref (value));
}

GPtrArray *
matecomponent_object_bag_list_ref (MateComponentObjectBag *bag)
{
	GPtrArray *refs;
	g_return_val_if_fail (bag != NULL, NULL);

	MATECOMPONENT_LOCK();

	refs = g_ptr_array_sized_new (bag->size);
	g_hash_table_foreach (bag->key_to_object,
			      bag_collect_ref_list_cb,
			      refs);
	
	MATECOMPONENT_UNLOCK();

	return refs;
}

typedef struct {
	GSList *keys;
	MateComponentObjectBag *bag;
} BagDestroyClosure;

static void
bag_collect_key_list_cb (gpointer key,
			 gpointer value,
			 gpointer user_data)
{
	BagDestroyClosure *cl = user_data;
	MateComponentAggregateObject *ao = value;

	cl->keys = g_slist_prepend (cl->keys, key);
	ao->bags = g_list_remove (ao->bags, cl->bag);
}

void
matecomponent_object_bag_destroy (MateComponentObjectBag *bag)
{
	GSList *l;
	BagDestroyClosure cl;

	if (!bag)
		return;

	MATECOMPONENT_LOCK();

	cl.bag = bag;
	cl.keys = NULL;
	g_hash_table_foreach (bag->key_to_object,
			      bag_collect_key_list_cb, &cl);

	g_hash_table_destroy (bag->key_to_object);
	g_hash_table_destroy (bag->object_to_key);

	g_free (bag);

	MATECOMPONENT_UNLOCK();

	for (l = cl.keys; l; l = l->next)
		bag->key_destroy_func (l->data);
	g_slist_free (cl.keys);
}
