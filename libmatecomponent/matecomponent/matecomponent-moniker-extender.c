/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-moniker-extender: extending monikers
 *
 * Author:
 *	Dietmar Maurer (dietmar@maurer-it.com)
 *
 * Copyright 2000, Ximian, Inc.
 */
#include <config.h>
  
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker.h>
#include <matecomponent/matecomponent-moniker-util.h>
#include <matecomponent/matecomponent-moniker-extender.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static GObjectClass *matecomponent_moniker_extender_parent_class;

#define CLASS(o) MATECOMPONENT_MONIKER_EXTENDER_CLASS (G_OBJECT_GET_CLASS (o))

static inline MateComponentMonikerExtender *
matecomponent_moniker_extender_from_servant (PortableServer_Servant servant)
{
	return MATECOMPONENT_MONIKER_EXTENDER (matecomponent_object_from_servant (servant));
}

static MateComponent_Unknown 
impl_MateComponent_MonikerExtender_resolve (PortableServer_Servant servant,
				     const MateComponent_Moniker   parent,
				     const MateComponent_ResolveOptions *options,
				     const CORBA_char      *display_name,
				     const CORBA_char      *requested_interface,
				     CORBA_Environment     *ev)
{
	MateComponentMonikerExtender *extender = matecomponent_moniker_extender_from_servant (servant);

	if (extender->resolve)
		return extender->resolve (extender, parent, options, display_name,
					  requested_interface, ev);
	else
		return CLASS (extender)->resolve (extender, parent, options, display_name,
						  requested_interface, ev);
}

static MateComponent_Unknown
matecomponent_moniker_extender_resolve (MateComponentMonikerExtender *extender,
				 const MateComponent_Moniker   parent,
				 const MateComponent_ResolveOptions *options,
				 const CORBA_char      *display_name,
				 const CORBA_char      *requested_interface,
				 CORBA_Environment     *ev)
{

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_MateComponent_Moniker_InterfaceNotFound, NULL);

	return CORBA_OBJECT_NIL;
}

static void
matecomponent_moniker_extender_finalize (GObject *object)
{
	matecomponent_moniker_extender_parent_class->finalize (object);
}

static void
matecomponent_moniker_extender_class_init (MateComponentMonikerExtenderClass *klass)
{
	GObjectClass *oclass = (GObjectClass *)klass;
	POA_MateComponent_MonikerExtender__epv *epv = &klass->epv;

	matecomponent_moniker_extender_parent_class = g_type_class_peek_parent (klass);

	oclass->finalize = matecomponent_moniker_extender_finalize;

	klass->resolve = matecomponent_moniker_extender_resolve;

	epv->resolve = impl_MateComponent_MonikerExtender_resolve;
}

static void
matecomponent_moniker_extender_init (GObject *object)
{
	/* nothing to do */
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentMonikerExtender, 
		       MateComponent_MonikerExtender,
		       PARENT_TYPE,
		       matecomponent_moniker_extender)

/**
 * matecomponent_moniker_extender_new:
 * @resolve: the resolve function that will be used to do the extension
 * @data: user data to be passed back to the resolve function.
 * 
 * This creates a new moniker extender.
 * 
 * Return value: the extender object
 **/
MateComponentMonikerExtender *
matecomponent_moniker_extender_new (MateComponentMonikerExtenderFn resolve, gpointer data)
{
	MateComponentMonikerExtender *extender = NULL;
	
	extender = g_object_new (matecomponent_moniker_extender_get_type (), NULL);

	extender->resolve = resolve;
	extender->data = data;

	return extender;
}

/**
 * matecomponent_moniker_find_extender:
 * @name: the name of the moniker we want to extend eg. 'file:'
 * @interface: the interface we want to resolve to
 * @opt_ev: an optional corba exception environment.
 * 
 *  This routine tries to locate an extender for our moniker
 * by examining a registry of extenders that map new interfaces
 * to certain moniker names.
 * 
 * Return value: an appropriate extender or CORBA_OBJECT_NIL.
 **/
MateComponent_MonikerExtender
matecomponent_moniker_find_extender (const gchar       *name, 
			      const gchar       *interface, 
			      CORBA_Environment *opt_ev)
{
	gchar            *query;
	MateComponent_Unknown    extender;
	CORBA_Environment  *ev, temp_ev;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	query = g_strdup_printf (
		"repo_ids.has ('IDL:MateComponent/MonikerExtender:1.0') AND "
		"repo_ids.has ('%s') AND "
		"matecomponent:moniker_extender.has ('%s')", interface, name);

	extender = matecomponent_activation_activate (query, NULL, 0, NULL, ev);

	g_free (query);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);

	return extender;
}

/**
 * matecomponent_moniker_use_extender:
 * @extender_oafiid: The IID of the extender to use
 * @moniker: the moniker to extend
 * @options: resolve options
 * @requested_interface: the requested interface
 * @opt_ev: optional corba environment
 * 
 *  Locates a known extender via. OAFIID; eg.
 * OAFIID:MateComponent_Moniker_Extender_file
 * 
 * Return value: the resolved result or CORBA_OBJECT_NIL.
 **/
MateComponent_Unknown
matecomponent_moniker_use_extender (const gchar                 *extender_oafiid,
			     MateComponentMoniker               *moniker,
			     const MateComponent_ResolveOptions *options,
			     const CORBA_char            *requested_interface,
			     CORBA_Environment           *opt_ev)
{
	MateComponent_MonikerExtender extender;
	MateComponent_Unknown         retval;
	CORBA_Environment  *ev, temp_ev;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	g_return_val_if_fail (ev != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (options != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (moniker != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (extender_oafiid != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (requested_interface != NULL, CORBA_OBJECT_NIL);

	extender = matecomponent_activation_activate_from_id (
		(gchar *) extender_oafiid, 0, NULL, ev);

	if (MATECOMPONENT_EX (ev) || extender == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;

	retval = MateComponent_MonikerExtender_resolve (extender, 
	        MATECOMPONENT_OBJREF (moniker), options, 
		matecomponent_moniker_get_name_full (moniker),
		requested_interface, ev);

	matecomponent_object_release_unref (extender, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);

	return retval;
}
