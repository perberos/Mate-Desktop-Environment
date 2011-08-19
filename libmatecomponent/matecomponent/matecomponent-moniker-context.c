/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-moniker-context.c: A global moniker interface
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright (C) 2000, Ximian, Inc.
 */
#include <config.h>
#include <glib-object.h>

#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker-util.h>
#include <matecomponent/matecomponent-moniker-extender.h>
#include <matecomponent/matecomponent-moniker-context.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static MateComponent_Moniker
impl_MateComponent_MonikerContext_createFromName (PortableServer_Servant servant,
					      const CORBA_char      *name,
					      CORBA_Environment     *ev)
{
	return matecomponent_moniker_client_new_from_name (name, ev);
}

static MateComponent_Unknown
impl_MateComponent_MonikerContext_getObject (PortableServer_Servant servant,
					 const CORBA_char      *name,
					 const CORBA_char      *repo_id,
					 CORBA_Environment     *ev)
{
	return matecomponent_get_object (name, repo_id, ev);
}

static MateComponent_MonikerExtender
impl_MateComponent_MonikerContext_getExtender (PortableServer_Servant servant,
					   const CORBA_char      *monikerPrefix,
					   const CORBA_char      *interfaceId,
					   CORBA_Environment     *ev)
{
	return matecomponent_moniker_find_extender (monikerPrefix, interfaceId, ev);
}

static void
matecomponent_moniker_context_class_init (MateComponentMonikerContextClass *klass)
{
	POA_MateComponent_MonikerContext__epv *epv = &klass->epv;

	epv->getObject        = impl_MateComponent_MonikerContext_getObject;
	epv->createFromName   = impl_MateComponent_MonikerContext_createFromName;
	epv->getExtender      = impl_MateComponent_MonikerContext_getExtender;
}

static void 
matecomponent_moniker_context_init (GObject *object)
{
	/* nothing to do */
}

static
MATECOMPONENT_TYPE_FUNC_FULL (MateComponentMonikerContext,
		       MateComponent_MonikerContext,
		       PARENT_TYPE,
		       matecomponent_moniker_context)

MateComponentObject *
matecomponent_moniker_context_new (void)
{
        return MATECOMPONENT_OBJECT (g_object_new (
		matecomponent_moniker_context_get_type (), NULL));
}
