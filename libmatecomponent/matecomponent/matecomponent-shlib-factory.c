/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-shlib-factory.c: a ShlibFactory object.
 *
 * The MateComponentShlibFactory object is used to instantiate new
 * MateComponent::ShlibFactory objects.  It acts as a wrapper for the
 * MateComponent::ShlibFactory CORBA interface, and dispatches to
 * a user-specified factory function whenever its create_object()
 * method is invoked.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999, 2001 Ximian, Inc.
 */
#include <config.h>
#include <glib-object.h>
#include <gobject/gmarshal.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-shlib-factory.h>
#include <matecomponent/matecomponent-running-context.h>

static MateComponentObjectClass *matecomponent_shlib_factory_parent_class = NULL;

struct _MateComponentShlibFactoryPrivate {
	int      live_objects;
	gpointer act_impl_ptr;
};

/**
 * matecomponent_shlib_factory_construct:
 * @factory: The object to be initialized.
 * @act_iid: The GOAD id that the new factory will implement.
 * @poa: the poa.
 * @act_impl_ptr: Activation shlib handle
 * @closure: The closure used to create new MateShlib object instances.
 *
 * Initializes @c_factory with the supplied data.
 *
 * Returns: The initialized MateComponentShlibFactory object.
 */
MateComponentShlibFactory *
matecomponent_shlib_factory_construct (MateComponentShlibFactory    *factory,
				const char            *act_iid,
				PortableServer_POA     poa,
				gpointer               act_impl_ptr,
				GClosure              *closure)
{
	g_return_val_if_fail (factory != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_SHLIB_FACTORY (factory), NULL);

	factory->priv->live_objects = 0;
	factory->priv->act_impl_ptr = act_impl_ptr;

        matecomponent_activation_plugin_use (poa, act_impl_ptr);

	matecomponent_generic_factory_construct_noreg (
		MATECOMPONENT_GENERIC_FACTORY (factory), act_iid, closure);

	return factory;
}

/**
 * matecomponent_shlib_factory_new_closure:
 * @act_iid: The GOAD id that this factory implements
 * @poa: the poa.
 * @act_impl_ptr: Activation shlib handle
 * @factory_closure: A closure which is used to create new MateComponentObject instances.
 *
 * This is a helper routine that simplifies the creation of factory
 * objects for MATE objects.  The @factory_closure closure will be
 * invoked by the CORBA server when a request arrives to create a new
 * instance of an object supporting the MateComponent::Shlib interface.
 * The factory callback routine is passed the @data pointer to provide
 * the creation function with some state information.
 *
 * Returns: A MateComponentShlibFactory object that has an activated
 * MateComponent::ShlibFactory object that has registered with the MATE
 * name server.
 */
MateComponentShlibFactory *
matecomponent_shlib_factory_new_closure (const char           *act_iid,
				  PortableServer_POA    poa,
				  gpointer              act_impl_ptr,
				  GClosure             *factory_closure)
{
	MateComponentShlibFactory *factory;

	g_return_val_if_fail (act_iid != NULL, NULL);
	g_return_val_if_fail (factory_closure != NULL, NULL);
	
	factory = g_object_new (matecomponent_shlib_factory_get_type (), NULL);
	
	return matecomponent_shlib_factory_construct (
		factory, act_iid, poa, act_impl_ptr, factory_closure);
}

/**
 * matecomponent_shlib_factory_new:
 * @component_id: The GOAD id that this factory implements
 * @poa: the poa.
 * @act_impl_ptr: Activation shlib handle
 * @factory_cb: A callback which is used to create new MateComponentObject instances.
 * @user_data: The closure data to be passed to the @factory callback routine.
 *
 * This is a helper routine that simplifies the creation of factory
 * objects for MATE objects.  The @factory function will be
 * invoked by the CORBA server when a request arrives to create a new
 * instance of an object supporting the MateComponent::Shlib interface.
 * The factory callback routine is passed the @data pointer to provide
 * the creation function with some state information.
 *
 * Returns: A MateComponentShlibFactory object that has an activated
 * MateComponent::ShlibFactory object that has registered with the MATE
 * name server.
 */
MateComponentShlibFactory *
matecomponent_shlib_factory_new (const char           *component_id,
			  PortableServer_POA    poa,
			  gpointer              act_impl_ptr,
			  MateComponentFactoryCallback factory_cb,
			  gpointer              user_data)
{
	return matecomponent_shlib_factory_new_closure (
		component_id, poa, act_impl_ptr,
		g_cclosure_new (G_CALLBACK (factory_cb), user_data, NULL));
}

/*
 * FIXME: at some time in the future we should look into trying
 * to unload the shlib's we have linked in with
 * matecomponent_activation_plugin_unuse - this is dangerous though; we
 * need to be sure no code is using the shlib first.
 */
static void
matecomponent_shlib_factory_finalize (GObject *object)
{
	MateComponentShlibFactory *factory = MATECOMPONENT_SHLIB_FACTORY (object);

	/* act_plugin_unuse (c_factory->act_impl_ptr); */

	g_free (factory->priv);
	
	G_OBJECT_CLASS (matecomponent_shlib_factory_parent_class)->finalize (object);
}

static MateComponentObject *
matecomponent_shlib_factory_new_generic (MateComponentGenericFactory *factory,
				  const char           *act_iid)
{
	MateComponentObject *retval;

	retval = MATECOMPONENT_GENERIC_FACTORY_CLASS (
		matecomponent_shlib_factory_parent_class)->new_generic (factory, act_iid);

	/* The factory reference doesn't persist inside matecomponent-activation */
	matecomponent_object_unref (MATECOMPONENT_OBJECT (factory));

	return retval;
}

static void
matecomponent_shlib_factory_class_init (MateComponentGenericFactoryClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	matecomponent_shlib_factory_parent_class = g_type_class_peek_parent (klass);

	klass->new_generic = matecomponent_shlib_factory_new_generic;

	object_class->finalize = matecomponent_shlib_factory_finalize;
}

static void
matecomponent_shlib_factory_init (GObject *object)
{
	MateComponentShlibFactory *factory = MATECOMPONENT_SHLIB_FACTORY (object);

	factory->priv = g_new0 (MateComponentShlibFactoryPrivate, 1);
}

/**
 * matecomponent_shlib_factory_get_type:
 *
 * Returns: The GType of the MateComponentShlibFactory class.
 */
MATECOMPONENT_TYPE_FUNC (MateComponentShlibFactory, 
		  MATECOMPONENT_TYPE_GENERIC_FACTORY,
		  matecomponent_shlib_factory)

/**
 * matecomponent_shlib_factory_std:
 * @component_id:
 * @poa:
 * @act_impl_ptr:
 * @factory_cb:
 * @user_data:
 * @ev:
 * 
 *    A Generic std shlib routine so we don't stick a load of code
 * inside a public macro.
 * 
 * Return value: 0 on success, 1 on failure.
 **/
MateComponent_Unknown
matecomponent_shlib_factory_std (const char            *component_id,
			  PortableServer_POA     poa,
			  gpointer               act_impl_ptr,
			  MateComponentFactoryCallback  factory_cb,
			  gpointer               user_data,
			  CORBA_Environment     *ev)
{
	MateComponentShlibFactory *f;

	f = matecomponent_shlib_factory_new (
		component_id, poa,
		act_impl_ptr,
		factory_cb, user_data);

        return CORBA_Object_duplicate (MATECOMPONENT_OBJREF (f), ev);
}

