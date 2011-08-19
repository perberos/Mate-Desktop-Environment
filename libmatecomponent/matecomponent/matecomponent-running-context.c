/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-running-context.c: A global running interface
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright (C) 2000, Ximian, Inc.
 */
#include <config.h>
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>

#include <matecomponent/matecomponent-context.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-event-source.h>
#include <matecomponent/matecomponent-moniker-util.h>
#include <matecomponent/matecomponent-running-context.h>
#include <matecomponent/matecomponent-private.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-debug.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

/* you may debug by adding item "running" to MATECOMPONENT_DEBUG_FLAGS
   environment variable. */

static MateComponentObjectClass *matecomponent_running_context_parent_class = NULL;

typedef struct {
	gboolean    emitted_last_unref;
	GHashTable *objects;
	GHashTable *keys;
} MateComponentRunningInfo;

static MateComponentRunningInfo *matecomponent_running_info = NULL;
static MateComponentObject      *matecomponent_running_context = NULL;
static MateComponentEventSource *matecomponent_running_event_source = NULL;

enum {
	LAST_UNREF,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

static void
key_free (gpointer name, gpointer dummy1, gpointer user_data)
{
	g_free (name);
}

#ifdef G_ENABLE_DEBUG
static void
matecomponent_ri_debug_foreach (gpointer key, gpointer value, gpointer user_data)
{
	CORBA_Object *o = value;
	
	matecomponent_debug_print ("", "[%p]:CORBA_Object still running", o);
		
}
#endif

void
matecomponent_running_context_shutdown (void)
{
	if (matecomponent_running_info) {

		MateComponentRunningInfo *ri = matecomponent_running_info;

#ifdef G_ENABLE_DEBUG
		if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_RUNNING) {
			matecomponent_debug_print ("rinfo-start", 
					    "-------------------------------------------------");
			
			matecomponent_debug_print ("running-objects", "%d running objects", 
					    g_hash_table_size (ri->objects));
			g_hash_table_foreach (ri->objects,
					      matecomponent_ri_debug_foreach, NULL);
			matecomponent_debug_print ("rinfo-end", 
					    "-------------------------------------------------");
		}
#endif /* G_ENABLE_DEBUG */
		if (ri->objects)
			g_hash_table_destroy (ri->objects);
		ri->objects = NULL;

		if (ri->keys) {
			g_hash_table_foreach_remove (
				ri->keys, (GHRFunc) key_free, NULL);
			g_hash_table_destroy (ri->keys);
			ri->keys = NULL;
		}
		g_free (ri);
	}
	matecomponent_running_info = NULL;
	matecomponent_running_context = NULL;
	matecomponent_running_event_source = NULL;
}

static void
check_destroy (GObject *object)
{
	matecomponent_running_context = NULL;
	matecomponent_running_event_source = NULL;
}

static MateComponentRunningInfo *
get_running_info_T (gboolean create)
{
	if (!matecomponent_running_info && create) {
		matecomponent_running_info = g_new (MateComponentRunningInfo, 1);
		matecomponent_running_info->objects = g_hash_table_new (NULL, NULL);
		matecomponent_running_info->keys    = g_hash_table_new (g_str_hash, g_str_equal);
		matecomponent_running_info->emitted_last_unref = FALSE;
	}

	return matecomponent_running_info;
}

static void
check_empty_T (void)
{
	MateComponentRunningInfo *ri = get_running_info_T (FALSE);

	if (!ri || !matecomponent_running_context)
		return;

	if (!ri->emitted_last_unref &&
	    (g_hash_table_size (ri->objects) == 0) &&
	    (g_hash_table_size (ri->keys) == 0)) {

		ri->emitted_last_unref = TRUE;
		MATECOMPONENT_UNLOCK ();

		g_signal_emit (G_OBJECT (matecomponent_running_context),
			       signals [LAST_UNREF], 0);
		g_return_if_fail (matecomponent_running_event_source != NULL);

		matecomponent_event_source_notify_listeners (
			matecomponent_running_event_source,
			"matecomponent:last_unref", NULL, NULL);

		MATECOMPONENT_LOCK ();
	}
}

#ifndef matecomponent_running_context_add_object_T
void
matecomponent_running_context_add_object_T (CORBA_Object object)
{
#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_RUNNING)
		matecomponent_running_context_trace_objects_T (object, "local", 0, 0);
	else
#endif /* G_ENABLE_DEBUG */
	{
		MateComponentRunningInfo *ri = get_running_info_T (TRUE);

		ri->emitted_last_unref = FALSE;
		g_hash_table_insert (ri->objects, object, object);
	}
}
#endif


#ifndef matecomponent_running_context_remove_object_T
void
matecomponent_running_context_remove_object_T (CORBA_Object object)
{
#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_RUNNING)
		matecomponent_running_context_trace_objects_T (object, "local", 0, 1);
	else
#endif /* G_ENABLE_DEBUG */
	{
		MateComponentRunningInfo *ri = get_running_info_T (FALSE);

		if (ri) {
			g_hash_table_remove (ri->objects, object);

			check_empty_T ();
		}
	}
}
#endif

#ifndef matecomponent_running_context_ignore_object
void
matecomponent_running_context_ignore_object (CORBA_Object object)
{
	MATECOMPONENT_LOCK ();
#ifdef G_ENABLE_DEBUG
	if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_RUNNING)
		matecomponent_running_context_trace_objects_T (object, "local", 0, 2);
	else
#endif /* G_ENABLE_DEBUG */
	{
		MateComponentRunningInfo *ri = get_running_info_T (FALSE);

		if (ri)
			g_hash_table_remove (ri->objects, object);
	}
	MATECOMPONENT_UNLOCK ();
}
#endif

#ifdef G_ENABLE_DEBUG
static void
_running_context_list_objects (gpointer key,
			       gpointer value,
			       gpointer user_data)
{
	CORBA_Object object = (CORBA_Object) value;
	CORBA_char *type_id;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	type_id = MateCORBA_small_get_type_id (object, &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		type_id = CORBA_string_dup (
			"<error determining object type id!>");
	CORBA_exception_free (&ev);

	matecomponent_debug_print ("Alive: ", "[%p]: %s", object, type_id);
	CORBA_free (type_id);
}
#endif

void          
matecomponent_running_context_trace_objects_T (CORBA_Object object,
					const char  *fn,
					int          line,
					int          mode)
{
	MateComponentRunningInfo *ri;
#ifdef G_ENABLE_DEBUG
	static const char cmode[][14] = {
		"add_object",
		"remove_object",
		"ignore_object"		
	};
#endif
	ri = get_running_info_T (mode == 0);	

	if (ri) {
		switch (mode) {
		case 0:
			g_hash_table_insert (ri->objects, object, object);
			ri->emitted_last_unref = FALSE;
			break;
		case 1:
			g_hash_table_remove (ri->objects, object);

			check_empty_T ();
			break;
		case 2:
			g_hash_table_remove (ri->objects, object);
			break;
		}

#ifdef G_ENABLE_DEBUG
		if(_matecomponent_debug_flags & MATECOMPONENT_DEBUG_RUNNING) {
			const char *mode_string = cmode[mode];
			matecomponent_debug_print (mode_string, 
					    "[%p]:CORBA_Object %d running objects at %s:%d",
					    object, g_hash_table_size (ri->objects), fn, line);
			g_hash_table_foreach (ri->objects, _running_context_list_objects, NULL);
		}
#endif /* G_ENABLE_DEBUG */
	}
}

static void
impl_MateComponent_RunningContext_addObject (PortableServer_Servant servant,
				      const CORBA_Object     object,
				      CORBA_Environment     *ev)
{
	MATECOMPONENT_LOCK ();
	matecomponent_running_context_add_object_T (object);
	MATECOMPONENT_UNLOCK ();
}

static void
impl_MateComponent_RunningContext_removeObject (PortableServer_Servant servant,
					 const CORBA_Object     object,
					 CORBA_Environment     *ev)
{
	MATECOMPONENT_LOCK ();
	matecomponent_running_context_remove_object_T (object);
	MATECOMPONENT_UNLOCK ();
}

static void
impl_MateComponent_RunningContext_addKey (PortableServer_Servant servant,
				   const CORBA_char      *key,
				   CORBA_Environment     *ev)
{
	char              *key_copy, *old_key;
	MateComponentRunningInfo *ri;

	MATECOMPONENT_LOCK ();

	ri = get_running_info_T (TRUE);

	old_key = g_hash_table_lookup (ri->keys, key);
	if (old_key) {
		g_free (old_key);
		g_hash_table_remove (ri->keys, key);
	}
	key_copy = g_strdup (key);

	g_hash_table_insert (ri->keys, key_copy, key_copy);

	MATECOMPONENT_UNLOCK ();
}

static void
impl_MateComponent_RunningContext_removeKey (PortableServer_Servant servant,
				      const CORBA_char      *key,
				      CORBA_Environment     *ev)
{
	MateComponentRunningInfo *ri;
	char              *old_key;

	MATECOMPONENT_LOCK ();

	ri = get_running_info_T (FALSE);

	if (ri) {
		old_key = g_hash_table_lookup (ri->keys, key);
		g_free (old_key);
		g_hash_table_remove (ri->keys, key);

		check_empty_T ();
	}

	MATECOMPONENT_UNLOCK ();
}

static void
impl_MateComponent_RunningContext_atExitUnref (PortableServer_Servant servant,
					const CORBA_Object     object,
					CORBA_Environment     *ev)
{
	matecomponent_running_context_at_exit_unref (object);
}

static void
matecomponent_running_context_class_init (MateComponentRunningContextClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_RunningContext__epv *epv = &klass->epv;

	matecomponent_running_context_parent_class = g_type_class_peek_parent (klass);

	((MateComponentRunningContextClass *)klass)->last_unref = NULL;

	signals [LAST_UNREF] = g_signal_new (
		"last_unref", G_TYPE_FROM_CLASS (object_class),
		G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (MateComponentRunningContextClass, last_unref),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	epv->addObject     = impl_MateComponent_RunningContext_addObject;
	epv->removeObject  = impl_MateComponent_RunningContext_removeObject;
	epv->addKey        = impl_MateComponent_RunningContext_addKey;
	epv->removeKey     = impl_MateComponent_RunningContext_removeKey;
	epv->atExitUnref   = impl_MateComponent_RunningContext_atExitUnref;

}

static void 
matecomponent_running_context_init (GObject *object)
{
	/* nothing to do */
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentRunningContext, 
		       MateComponent_RunningContext,
		       PARENT_TYPE,
		       matecomponent_running_context)

MateComponentObject *
matecomponent_running_context_new (void)
{
	if (matecomponent_running_context) {
		matecomponent_object_ref (matecomponent_running_context);
		return matecomponent_running_context;
	}

	matecomponent_running_context = g_object_new (
		matecomponent_running_context_get_type (), NULL);

	matecomponent_running_event_source = matecomponent_event_source_new ();
	matecomponent_running_context_ignore_object (
	        MATECOMPONENT_OBJREF (matecomponent_running_event_source));
	matecomponent_event_source_ignore_listeners (matecomponent_running_event_source);

	matecomponent_object_add_interface (MATECOMPONENT_OBJECT (matecomponent_running_context),
				     MATECOMPONENT_OBJECT (matecomponent_running_event_source));

	g_signal_connect (G_OBJECT (matecomponent_running_context),
			  "destroy", G_CALLBACK (check_destroy), NULL);

	return matecomponent_running_context;
}

MateComponentObject *
matecomponent_context_running_get (void)
{
	return matecomponent_running_context_new ();
}

static void
last_unref_cb (gpointer      context,
	       CORBA_Object  object)
{
	matecomponent_object_release_unref (object, NULL);
}

void 
matecomponent_running_context_at_exit_unref (CORBA_Object object)
{
	CORBA_Environment ev;
	CORBA_Object obj_dup;

	CORBA_exception_init (&ev);

	obj_dup = CORBA_Object_duplicate (object, &ev);

	matecomponent_running_context_ignore_object (obj_dup);

	if (matecomponent_running_context)
		g_signal_connect (G_OBJECT (matecomponent_running_context),
				  "last_unref", G_CALLBACK (last_unref_cb),
				  obj_dup);
	
	CORBA_exception_free (&ev);
}

static void
last_unref_exit_cb (gpointer      context,
		    MateComponentObject *object)
{
        matecomponent_object_unref (object);
	matecomponent_main_quit ();
}

void 
matecomponent_running_context_auto_exit_unref (MateComponentObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (MATECOMPONENT_IS_OBJECT (object));

	matecomponent_running_context_ignore_object (MATECOMPONENT_OBJREF (object));

	if (matecomponent_running_context)
		g_signal_connect (G_OBJECT (matecomponent_running_context),
				  "last_unref",
				  G_CALLBACK (last_unref_exit_cb),
				  object);
}

