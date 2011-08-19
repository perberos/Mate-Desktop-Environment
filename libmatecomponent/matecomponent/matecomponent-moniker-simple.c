/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-moniker-simple: Simplified object naming abstraction
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, Ximian, Inc.
 */
#include <config.h>

#include <matecomponent/matecomponent-moniker.h>
#include <matecomponent/matecomponent-moniker-simple.h>

#include <matecomponent/matecomponent-types.h>
#include <matecomponent/matecomponent-marshal.h>

static GObjectClass *parent_class = NULL;

struct _MateComponentMonikerSimplePrivate {
	GClosure *resolve_closure;
};

static void
matecomponent_marshal_BOXED__RESOLVEOPTIONS_STRING_BOXED (GClosure     *closure,
						   GValue       *return_value,
						   guint         n_param_values,
						   const GValue *param_values,
						   gpointer      invocation_hint,
						   gpointer      marshal_data)
{
	typedef gpointer (*GMarshalFunc_BOXED__POINTER_STRING_BOXED) (gpointer     data1,
								      gpointer     arg_1,
								      gpointer     arg_2,
								      gpointer     arg_3,
								      gpointer     data2);
	register GMarshalFunc_BOXED__POINTER_STRING_BOXED callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;
	MateComponent_ResolveOptions resopt;
	gpointer v_return;

	g_return_if_fail (return_value != NULL);
	g_return_if_fail (n_param_values == 5);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_BOXED__POINTER_STRING_BOXED) (marshal_data ? marshal_data : cc->callback);

	resopt.flags = g_value_get_flags (param_values + 1) ?
		MateComponent_MONIKER_ALLOW_USER_INTERACTION : 0;
	resopt.timeout = g_value_get_long (param_values + 2);

	v_return = callback (data1,
			     &resopt,
			     (char*) g_value_get_string (param_values + 3),
			     g_value_get_boxed (param_values + 4),
			     data2);

	g_value_take_boxed (return_value, v_return);
}

static MateComponent_Unknown
simple_resolve (MateComponentMoniker               *moniker,
		const MateComponent_ResolveOptions *options,
		const CORBA_char            *requested_interface,
		CORBA_Environment           *ev)
{
	MateComponentMonikerSimple *simple;
	MateComponent_Unknown       ret;
	MateComponent_ResolveFlag   resolve_flag;
	glong                timeout;

	g_return_val_if_fail (MATECOMPONENT_IS_MONIKER_SIMPLE (moniker),
			      CORBA_OBJECT_NIL);

	simple = MATECOMPONENT_MONIKER_SIMPLE (moniker);

	resolve_flag = options ? options->flags : 0;
	timeout = options ? options->timeout : -1;

	matecomponent_closure_invoke (
		simple->priv->resolve_closure,
		MATECOMPONENT_TYPE_STATIC_CORBA_OBJECT,            &ret,
		MATECOMPONENT_TYPE_MONIKER,                        moniker,
		MATECOMPONENT_TYPE_RESOLVE_FLAG,                   resolve_flag,
		G_TYPE_LONG,                                timeout,
		G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE, requested_interface,
		MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION,         ev,
		G_TYPE_INVALID);

	return ret;
}

static void
simple_finalize (GObject *object)
{
	MateComponentMonikerSimple *simple = (MateComponentMonikerSimple *) object;

	if (simple->priv) {
		if (simple->priv->resolve_closure)
			g_closure_unref (simple->priv->resolve_closure);

		g_free (simple->priv);
		simple->priv = NULL;
	}

	parent_class->finalize (object);
}

static void
matecomponent_moniker_simple_class_init (MateComponentMonikerClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *) klass;

	klass->resolve = simple_resolve;
	
	gobject_class->finalize = simple_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void 
matecomponent_moniker_simple_init (GObject *object)
{
	MateComponentMonikerSimple *simple = MATECOMPONENT_MONIKER_SIMPLE (object);

	simple->priv = g_new0 (MateComponentMonikerSimplePrivate, 1);
}

MATECOMPONENT_TYPE_FUNC (MateComponentMonikerSimple, 
		  matecomponent_moniker_get_type (),
		  matecomponent_moniker_simple)

/**
 * matecomponent_moniker_simple_construct:
 * @moniker: the moniker to construct
 * @name: the name of the moniker eg. 'file:'
 * @resolve_closure: the closure used to resolve the moniker
 * 
 * Constructs a simple moniker
 * 
 * Return value: the constructed moniker or NULL on failure.
 **/
MateComponentMoniker *
matecomponent_moniker_simple_construct (MateComponentMonikerSimple *moniker,
				 const char          *name,
				 GClosure            *resolve_closure)
{
	g_return_val_if_fail (resolve_closure != NULL, NULL);

	moniker->priv->resolve_closure =
		matecomponent_closure_store (resolve_closure, matecomponent_marshal_BOXED__RESOLVEOPTIONS_STRING_BOXED);
	
	return matecomponent_moniker_construct (
		MATECOMPONENT_MONIKER (moniker), name);
}

/**
 * matecomponent_moniker_simple_new_closure:
 * @name: the display name for the moniker
 * @resolve_closure: a closure for the resolve process.
 * 
 * Create a new instance of a simplified moniker.
 *
 * Instead of the MateComponent_ResolveOptions struct, the closure takes its
 * contents as two arguments: MATECOMPONENT_TYPE_RESOLVE_FLAG and G_TYPE_LONG.
 * 
 * Return value: the moniker object
 **/
MateComponentMoniker *
matecomponent_moniker_simple_new_closure (const char *name,
				   GClosure   *resolve_closure)
{
	MateComponentMoniker *moniker;

	moniker = g_object_new (matecomponent_moniker_simple_get_type (), NULL);

	return matecomponent_moniker_simple_construct (
		MATECOMPONENT_MONIKER_SIMPLE (moniker),
		name, resolve_closure);
}

/**
 * matecomponent_moniker_simple_new:
 * @name: the display name for the moniker
 * @resolve_fn: a resolve function for the moniker
 * 
 * Create a new instance of a simplified moniker.
 * 
 * Return value: the moniker object
 **/
MateComponentMoniker *
matecomponent_moniker_simple_new (const char                  *name,
			   MateComponentMonikerSimpleResolveFn resolve_fn)
{
	return matecomponent_moniker_simple_new_closure (
		name, g_cclosure_new (G_CALLBACK (resolve_fn), NULL, NULL));
}

GType
matecomponent_resolve_flag_get_type (void)
{
	static GType resolve_flag_type = 0;
	static GFlagsValue resolve_flag_values[] = {
		{
			MateComponent_MONIKER_ALLOW_USER_INTERACTION,
			"matecomponent-moniker-allow-user-interaction",
			"matecomponent-moniker-allow-user-interaction"
		}, {
			0, NULL, NULL
		}
	};

	if (!resolve_flag_type)
		resolve_flag_type = g_flags_register_static
			("MateComponentResolveFlag", resolve_flag_values);

	return resolve_flag_type;
}
