/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-generic-factory.c: a GenericFactory object.
 *
 * The MateComponentGenericFactory object is used to instantiate new
 * MateComponent::GenericFactory objects.  It acts as a wrapper for the
 * MateComponent::GenericFactory CORBA interface, and dispatches to
 * a user-specified factory function whenever its create_object()
 * method is invoked.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *   ÉRDI Gergõ (cactus@cactus.rulez.org): cleanup
 *
 * Copyright 1999, 2001 Ximian, Inc., 2001 Gergõ Érdi
 */
#include <config.h>
#include <string.h>
#include <glib-object.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-types.h>
#include <matecomponent/matecomponent-marshal.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-generic-factory.h>
#include <matecomponent/matecomponent-context.h>
#include <matecomponent/matecomponent-running-context.h>
#include <matecomponent/matecomponent-debug.h>
#include "matecomponent-private.h"

#define DEFAULT_LAST_UNREF_TIMEOUT 	2000
#define STARTUP_TIMEOUT 		60

struct _MateComponentGenericFactoryPrivate {
	/* The function factory */
	GClosure *factory_closure;
	/* The component_id for this generic factory */
	char     *act_iid;
	/* Whether we should register with the activation server */
	gboolean  noreg;
	  /* Startup timeout when no object is ever requested */
	guint startup_timeout_id;
	  /* Timeout after last unref before quitting */
	guint last_unref_timeout_id;
	guint last_unref_timeout; /* miliseconds */
	guint last_unref_ignore; /* ignore last-unref caused by temporary objects */
	gboolean last_unref_set; /* keep track of last-unref status during "ignore" period */
};

static GObjectClass *matecomponent_generic_factory_parent_class = NULL;

static CORBA_Object
impl_MateComponent_ObjectFactory_createObject (PortableServer_Servant   servant,
					const CORBA_char        *obj_act_iid,
					CORBA_Environment       *ev)
{
	MateComponentGenericFactoryClass *class;
	MateComponentGenericFactory      *factory;
	MateComponentObject              *object;

	factory = MATECOMPONENT_GENERIC_FACTORY (matecomponent_object (servant));

	class = MATECOMPONENT_GENERIC_FACTORY_CLASS (G_OBJECT_GET_CLASS (factory));

	MATECOMPONENT_LOCK();
	  /* Cancel the startup timeout, if active. */
	if (factory->priv->startup_timeout_id) {
	    g_source_destroy (g_main_context_find_source_by_id
			      (NULL, factory->priv->startup_timeout_id));
	    factory->priv->startup_timeout_id = 0;
	}

	  /* Cancel the last unref timeout, if active. */
	if (factory->priv->last_unref_timeout_id) {
	    g_source_destroy (g_main_context_find_source_by_id
			      (NULL, factory->priv->last_unref_timeout_id));
	    factory->priv->last_unref_timeout_id = 0;
	}
	MATECOMPONENT_UNLOCK();

	object = (*class->new_generic) (factory, obj_act_iid);
	factory = NULL; /* unreffed by new_generic in the shlib case */

	if (!object)
		return CORBA_OBJECT_NIL;
	
	return CORBA_Object_duplicate (MATECOMPONENT_OBJREF (object), ev);
}

/**
 * matecomponent_generic_factory_construct_noreg:
 * @factory: The object to be initialized.
 * @act_iid: The GOAD id that the new factory will implement.
 * @factory_closure: A Multi object factory closure.
 *
 * Initializes @c_factory with the supplied closure and iid.
 */
void
matecomponent_generic_factory_construct_noreg (MateComponentGenericFactory   *factory,
					const char             *act_iid,
					GClosure               *factory_closure)
{
	g_return_if_fail (MATECOMPONENT_IS_GENERIC_FACTORY (factory));
	
	factory->priv->act_iid = g_strdup (act_iid);
	factory->priv->noreg   = TRUE;

	if (factory_closure)
		factory->priv->factory_closure =
			matecomponent_closure_store (factory_closure, matecomponent_marshal_OBJECT__STRING);
}

/**
 * matecomponent_generic_factory_construct:
 * @factory: The object to be initialized.
 * @act_iid: The MateComponent activation id that the new factory will implement.
 * MateComponent::GenericFactory interface and which will be used to
 * construct this MateComponentGenericFactory Gtk object.
 * @factory_closure: A Multi object factory closure.
 *
 * Initializes @c_factory with and registers the new factory with
 * the name server.
 *
 * Returns: The initialized MateComponentGenericFactory object or NULL
 *          if already registered.
 */
MateComponentGenericFactory *
matecomponent_generic_factory_construct (MateComponentGenericFactory   *factory,
				  const char             *act_iid,
				  GClosure               *factory_closure)
{
	int ret;

	g_return_val_if_fail (MATECOMPONENT_IS_GENERIC_FACTORY (factory), NULL);
	
	matecomponent_generic_factory_construct_noreg (factory, act_iid, factory_closure);

	factory->priv->noreg = FALSE;

	ret = matecomponent_activation_active_server_register (act_iid, MATECOMPONENT_OBJREF (factory));

	if (ret != MateComponent_ACTIVATION_REG_SUCCESS) {
		matecomponent_object_unref (MATECOMPONENT_OBJECT (factory));
#ifdef G_ENABLE_DEBUG
		if (_matecomponent_debug_flags & MATECOMPONENT_DEBUG_LIFECYCLE) {
			const char *err;
			switch (ret)
			{
			case MateComponent_ACTIVATION_REG_SUCCESS:        err = "success";        break;
			case MateComponent_ACTIVATION_REG_NOT_LISTED:     err = "not listed";     break;
			case MateComponent_ACTIVATION_REG_ALREADY_ACTIVE: err = "already active"; break;
			case MateComponent_ACTIVATION_REG_ERROR: 	   err = "error";          break;
			default: err = "(invalid error!)"; break;
			}
			g_warning ("'%s' factory registration failed: %s",
				   act_iid, err);
		}
#endif
		return NULL;
	}

	return factory;
}

/**
 * matecomponent_generic_factory_new_closure:
 * @act_iid: The GOAD id that this factory implements
 * @factory_closure: A closure which is used to create new MateComponentObject instances.
 *
 * This is a helper routine that simplifies the creation of factory
 * objects for MATE objects.  The @factory_closure closure will be
 * invoked by the CORBA server when a request arrives to create a new
 * instance of an object supporting the MateComponent::Generic interface.
 * The factory callback routine is passed the @data pointer to provide
 * the creation function with some state information.
 *
 * Returns: A MateComponentGenericFactory object that has an activated
 * MateComponent::GenericFactory object that has registered with the MATE
 * name server.
 */
MateComponentGenericFactory *
matecomponent_generic_factory_new_closure (const char *act_iid,
				    GClosure   *factory_closure)
{
	MateComponentGenericFactory *factory;

	g_return_val_if_fail (act_iid != NULL, NULL);
	g_return_val_if_fail (factory_closure != NULL, NULL);
	
	factory = g_object_new (matecomponent_generic_factory_get_type (), NULL);

	return matecomponent_generic_factory_construct (
		factory, act_iid, factory_closure);
}


/**
 * matecomponent_generic_factory_new:
 * @act_iid: The GOAD id that this factory implements
 * @factory_cb: A callback which is used to create new MateComponentObject instances.
 * @user_data: The closure data to be passed to the @factory callback routine.
 *
 * This is a helper routine that simplifies the creation of factory
 * objects for MATE objects.  The @factory function will be
 * invoked by the CORBA server when a request arrives to create a new
 * instance of an object supporting the MateComponent::Generic interface.
 * The factory callback routine is passed the @data pointer to provide
 * the creation function with some state information.
 *
 * Returns: A MateComponentGenericFactory object that has an activated
 * MateComponent::GenericFactory object that has registered with the MATE
 * name server.
 */
MateComponentGenericFactory *
matecomponent_generic_factory_new (const char           *act_iid,
			    MateComponentFactoryCallback factory_cb,
			    gpointer              user_data)
{
	return matecomponent_generic_factory_new_closure (
		act_iid, g_cclosure_new (G_CALLBACK (factory_cb), user_data, NULL));
}

static void
matecomponent_generic_factory_destroy (MateComponentObject *object)
{
	MateComponentGenericFactory *factory = MATECOMPONENT_GENERIC_FACTORY (object);

	if (factory->priv) {
		if (!factory->priv->noreg && factory->priv->act_iid)
			matecomponent_activation_active_server_unregister (
				factory->priv->act_iid, MATECOMPONENT_OBJREF (factory));

		g_free (factory->priv->act_iid);

		if (factory->priv->factory_closure)
			g_closure_unref (factory->priv->factory_closure);

		  /* Cancel the startup timeout, if active. */
		if (factory->priv->startup_timeout_id)
			g_source_destroy (g_main_context_find_source_by_id
					  (NULL, factory->priv->startup_timeout_id));
		  /* Cancel the last unref timeout, if active. */
		if (factory->priv->last_unref_timeout_id)
			g_source_destroy (g_main_context_find_source_by_id
					  (NULL, factory->priv->last_unref_timeout_id));
		
		g_free (factory->priv);
		factory->priv = NULL;
	}
		
	MATECOMPONENT_OBJECT_CLASS (matecomponent_generic_factory_parent_class)->destroy (object);
}

static gboolean
last_unref_timeout (gpointer data)
{
	MateComponentGenericFactory *factory = MATECOMPONENT_GENERIC_FACTORY (data);
	matecomponent_main_quit ();
	factory->priv->last_unref_timeout_id = 0;
	return FALSE;
}

static MateComponentObject *
matecomponent_generic_factory_new_generic (MateComponentGenericFactory *factory,
				    const char           *act_iid)
{
	MateComponentObject *ret;
	
	g_return_val_if_fail (factory != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_GENERIC_FACTORY (factory), NULL);

	MATECOMPONENT_LOCK();
	factory->priv->last_unref_ignore++;
	MATECOMPONENT_UNLOCK();

	matecomponent_closure_invoke (factory->priv->factory_closure,
			       MATECOMPONENT_TYPE_OBJECT, &ret,
			       MATECOMPONENT_TYPE_GENERIC_FACTORY, factory,
			       G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE, act_iid, G_TYPE_INVALID);

	MATECOMPONENT_LOCK();
	factory->priv->last_unref_ignore--;

	if (ret) /* since we are returning a new object, even if
		  * last-unref was previously caught it no longer
		  * applies */
		factory->priv->last_unref_set = FALSE;

	if (factory->priv->last_unref_set) {
		factory->priv->last_unref_timeout_id = g_timeout_add
			(factory->priv->last_unref_timeout, last_unref_timeout, factory);
		factory->priv->last_unref_set = FALSE;
	}
	MATECOMPONENT_UNLOCK();

	return ret;
}

static void
matecomponent_generic_factory_class_init (MateComponentGenericFactoryClass *klass)
{
	MateComponentObjectClass *matecomponent_object_class = (MateComponentObjectClass *) klass;
	POA_MateComponent_GenericFactory__epv *epv = &klass->epv;

	epv->createObject = impl_MateComponent_ObjectFactory_createObject;

	matecomponent_object_class->destroy = matecomponent_generic_factory_destroy;

	klass->new_generic = matecomponent_generic_factory_new_generic;

	matecomponent_generic_factory_parent_class = g_type_class_peek_parent (klass);
}

static void
matecomponent_generic_factory_init (GObject *object)
{
	MateComponentGenericFactory *factory = MATECOMPONENT_GENERIC_FACTORY (object);

	factory->priv = g_new0 (MateComponentGenericFactoryPrivate, 1);
	factory->priv->noreg = FALSE;
	factory->priv->last_unref_timeout = DEFAULT_LAST_UNREF_TIMEOUT;
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentGenericFactory, 
		       MateComponent_GenericFactory,
		       MATECOMPONENT_TYPE_OBJECT,
		       matecomponent_generic_factory)


static gboolean
startup_timeout (gpointer data)
{
	MateComponentGenericFactory *factory = MATECOMPONENT_GENERIC_FACTORY (data);
	matecomponent_main_quit ();
	factory->priv->startup_timeout_id = 0;
	return FALSE;
}

static void
last_unref_cb (MateComponentRunningContext *context, MateComponentGenericFactory *factory)
{
	g_return_if_fail (MATECOMPONENT_IS_GENERIC_FACTORY (factory));
	if (factory->priv->last_unref_ignore) {
		factory->priv->last_unref_set = TRUE;
		return;
	}
	g_return_if_fail (!factory->priv->last_unref_timeout_id);
	factory->priv->last_unref_timeout_id = g_timeout_add
		(factory->priv->last_unref_timeout, last_unref_timeout, factory);
}

/**
 * matecomponent_generic_factory_main:
 * @act_iid: the oaf iid of the factory
 * @factory_cb: the factory callback
 * @user_data: a user data pointer
 * 
 *    A Generic 'main' routine so we don't stick a load of code
 * inside a public macro. See also matecomponent_generic_factory_main_timeout().
 * 
 * Return value: 0 on success, 1 on failure.
 **/
int
matecomponent_generic_factory_main (const char           *act_iid,
			     MateComponentFactoryCallback factory_cb,
			     gpointer              user_data)
{
	return matecomponent_generic_factory_main_timeout
		(act_iid, factory_cb, user_data, DEFAULT_LAST_UNREF_TIMEOUT);
}

/**
 * matecomponent_generic_factory_main_timeout:
 * @act_iid: the oaf iid of the factory
 * @factory_cb: the factory callback
 * @user_data: a user data pointer
 * @quit_timeout: ammount of time to wait (miliseconds) after all
 * objects have been released before quitting the main loop.
 * 
 *    A Generic 'main' routine so we don't stick a load of code
 * inside a public macro.
 * 
 * Return value: 0 on success, 1 on failure.
 **/
int
matecomponent_generic_factory_main_timeout (const char           *act_iid,
				     MateComponentFactoryCallback factory_cb,
				     gpointer              user_data,
				     guint                 quit_timeout)
{
	MateComponentGenericFactory *factory;

	factory = matecomponent_generic_factory_new (
		act_iid, factory_cb, user_data);

	if (factory) {
		MateComponentObject *context;
		guint         signal;

		factory->priv->last_unref_timeout = quit_timeout;
		context = matecomponent_running_context_new ();
		signal = g_signal_connect (G_OBJECT (context), "last-unref",
					   G_CALLBACK (last_unref_cb), factory);
		matecomponent_running_context_ignore_object (MATECOMPONENT_OBJREF (factory));

		  /* Create timeout here so if we haven't created anything
		   * within a few minutes we just quit */
		factory->priv->startup_timeout_id = g_timeout_add_seconds
			(STARTUP_TIMEOUT, startup_timeout, factory);

		matecomponent_main ();

		g_signal_handler_disconnect (G_OBJECT (context), signal);
		matecomponent_object_unref (MATECOMPONENT_OBJECT (factory));
		matecomponent_object_unref (context);

		return matecomponent_debug_shutdown ();
	} else
		return 1;
}

