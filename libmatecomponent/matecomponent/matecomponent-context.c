/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-context.h: Handle Global Component contexts.
 *
 * Author:
 *     Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Ximian, Inc.
 */
#include <config.h>

#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-context.h>
#include <matecomponent/matecomponent-private.h>
#include <matecomponent/matecomponent-running-context.h>
#include <matecomponent/matecomponent-moniker-context.h>

static GHashTable *matecomponent_contexts = NULL;

/**
 * matecomponent_context_add:
 * @context_name: the name to refer to the context by
 * @context: The MateComponent_Unknown; a ref. is taken on this.
 * 
 * This function adds a new context to the context system
 **/
void
matecomponent_context_add (const CORBA_char *context_name,
		    MateComponent_Unknown    context)
{
	g_return_if_fail (context != CORBA_OBJECT_NIL);

	if (!matecomponent_contexts)
		matecomponent_contexts = g_hash_table_new (
			g_str_hash, g_str_equal);

	g_hash_table_insert (matecomponent_contexts,
			     g_strdup (context_name),
			     matecomponent_object_dup_ref (context, NULL));
}

/**
 * matecomponent_context_get:
 * @context_name: the name of the context
 * @opt_ev: optional Environment, or NULL
 * 
 *  The most useful context is named 'Activation' and returns
 * the IDL:MateComponent/ActivationContext:1.0 interface.
 * 
 * Return value: a new reference to a global MateComponent context or CORBA_OBJECT_NIL
 **/
MateComponent_Unknown
matecomponent_context_get (const CORBA_char  *context_name,
		    CORBA_Environment *opt_ev)
{
	MateComponent_Unknown ret;

	g_return_val_if_fail (context_name != NULL, CORBA_OBJECT_NIL);

	if ((ret = g_hash_table_lookup (matecomponent_contexts, context_name)))
		return matecomponent_object_dup_ref (ret, opt_ev);
	else
		return CORBA_OBJECT_NIL;
}

static void
context_add (MateComponentObject *object, const char *name)
{
	CORBA_Object ref;

	ref = MATECOMPONENT_OBJREF (object);

	matecomponent_context_add (name, ref);

	/* Don't count it as a running object; we always have it */
	matecomponent_running_context_ignore_object (ref);

	matecomponent_object_unref (object);
}

/**
 * matecomponent_context_init:
 * @void: 
 * 
 * Sets up the context system, internal use only, called
 * by matecomponent_init.
 **/
void
matecomponent_context_init (void)
{
	context_add (matecomponent_moniker_context_new (), "Moniker");
	context_add (matecomponent_running_context_new (), "Running");
}

static gboolean
context_destroy (char *key, MateComponent_Unknown handle, gpointer dummy)
{
	g_free (key);
	matecomponent_object_release_unref (handle, NULL);
	return TRUE;
}

/**
 * matecomponent_context_shutdown:
 * @void: 
 * 
 * Shuts down the context system, internal use only
 **/
void
matecomponent_context_shutdown (void)
{
	MateComponent_Unknown m_context;

	if (!matecomponent_contexts)
		return;

	m_context = g_hash_table_lookup (matecomponent_contexts, "Moniker");
	if (!MateCORBA_small_get_servant (m_context))
		g_error ("In-proc object has no servant association\n"
			 "this probably means you shutdown the ORB before "
			 "you shutdown libmatecomponent\n");

	g_hash_table_foreach_remove (
		matecomponent_contexts, (GHRFunc) context_destroy, NULL);
	g_hash_table_destroy (matecomponent_contexts);
	matecomponent_contexts = NULL;
}
