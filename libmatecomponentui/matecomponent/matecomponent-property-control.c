/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-property-control.c: MateComponent PropertyControl implementation
 *
 * Author:
 *      Iain Holmes  <iain@ximian.com>
 *
 * Copyright 2000, 2001 Ximian, Inc.
 */
#include <config.h>
#include <stdio.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-property-control.h>
#include <matecomponent/matecomponent-event-source.h>
#include <matecomponent/matecomponent-ui-marshal.h>
#include <matecomponent/MateComponent.h>

struct _MateComponentPropertyControlPrivate {
	MateComponentPropertyControlGetControlFn get_fn;
	MateComponentEventSource *event_source;

	void *closure;
	int page_count;
};

enum {
	ACTION,
	LAST_SIGNAL
};

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static GObjectClass *parent_class;

static guint32 signals[LAST_SIGNAL] = { 0 };

static CORBA_long
impl_MateComponent_PropertyControl__get_pageCount (PortableServer_Servant servant,
					    CORBA_Environment *ev)
{
	MateComponentObject *matecomponent_object;
	MateComponentPropertyControl *property_control;
	MateComponentPropertyControlPrivate *priv;

	matecomponent_object = matecomponent_object_from_servant (servant);
	property_control = MATECOMPONENT_PROPERTY_CONTROL (matecomponent_object);
	priv = property_control->priv;

	return priv->page_count;
}
       
static MateComponent_Control
impl_MateComponent_PropertyControl_getControl (PortableServer_Servant servant,
					CORBA_long pagenumber,
					CORBA_Environment *ev)
{
	MateComponentObject *matecomponent_object;
	MateComponentPropertyControl *property_control;
	MateComponentPropertyControlPrivate *priv;
	MateComponentControl *control;

	matecomponent_object = matecomponent_object_from_servant (servant);
	property_control = MATECOMPONENT_PROPERTY_CONTROL (matecomponent_object);
	priv = property_control->priv;

	if (pagenumber < 0 || pagenumber >= priv->page_count) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_PropertyControl_NoPage, NULL);
		return CORBA_OBJECT_NIL;
	}

	control = priv->get_fn (property_control, 
			        pagenumber,
		                priv->closure);

	if (control == NULL)
		return CORBA_OBJECT_NIL;

	return (MateComponent_Control) CORBA_Object_duplicate 
		(MATECOMPONENT_OBJREF (control), ev);
}

static void
impl_MateComponent_PropertyControl_notifyAction (PortableServer_Servant servant,
					  CORBA_long pagenumber,
					  MateComponent_PropertyControl_Action action,
					  CORBA_Environment *ev)
{
	MateComponentObject *matecomponent_object;
	MateComponentPropertyControl *property_control;
	MateComponentPropertyControlPrivate *priv;

	matecomponent_object = matecomponent_object_from_servant (servant);
	property_control = MATECOMPONENT_PROPERTY_CONTROL (matecomponent_object);
	priv = property_control->priv;

	if (pagenumber < 0 || pagenumber >= priv->page_count) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_PropertyControl_NoPage, NULL);
		return;
	}
	
	g_signal_emit (matecomponent_object, signals [ACTION], 0, pagenumber, action);
}

static void
matecomponent_property_control_destroy (MateComponentObject *object)
{
	MateComponentPropertyControl *property_control;
	
	property_control = MATECOMPONENT_PROPERTY_CONTROL (object);
	if (property_control->priv == NULL)
		return;

	g_free (property_control->priv);
	property_control->priv = NULL;

	MATECOMPONENT_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
matecomponent_property_control_class_init (MateComponentPropertyControlClass *klass)
{
	MateComponentObjectClass *object_class;
	POA_MateComponent_PropertyControl__epv *epv = &klass->epv;

	object_class = MATECOMPONENT_OBJECT_CLASS (klass);
	object_class->destroy = matecomponent_property_control_destroy;

	signals [ACTION] = g_signal_new ("action",
					 G_TYPE_FROM_CLASS (object_class),
					 G_SIGNAL_RUN_FIRST, 
					 G_STRUCT_OFFSET (MateComponentPropertyControlClass, action),
					 NULL, NULL,
					 matecomponent_ui_marshal_VOID__INT_INT,
					 G_TYPE_NONE,
					 2, G_TYPE_INT, G_TYPE_INT);
				     
	parent_class = g_type_class_peek_parent (klass);

	epv->_get_pageCount = impl_MateComponent_PropertyControl__get_pageCount;
	epv->getControl     = impl_MateComponent_PropertyControl_getControl;
	epv->notifyAction   = impl_MateComponent_PropertyControl_notifyAction;
}

static void
matecomponent_property_control_init (MateComponentPropertyControl *property_control)
{
	MateComponentPropertyControlPrivate *priv;

	priv = g_new (MateComponentPropertyControlPrivate, 1);
	priv->get_fn = NULL;
	priv->closure = NULL;
	priv->page_count = 0;

	property_control->priv = priv;
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentPropertyControl, 
			   MateComponent_PropertyControl,
			   PARENT_TYPE,
			   matecomponent_property_control)

/**
 * matecomponent_property_control_construct:
 * @property_control: A MateComponentPropertyControl object.
 * @event_source: A MateComponentEventSource object that will be aggregated onto the
 * property control.
 * @get_fn: Creation routine.
 * @closure: Data passed to closure routine.
 *
 * Initialises the MateComponentPropertyControl object.
 *
 * Returns: The newly constructed MateComponentPropertyControl.
 */
MateComponentPropertyControl *
matecomponent_property_control_construct (MateComponentPropertyControl            *property_control,
				   MateComponentEventSource                *event_source,
				   MateComponentPropertyControlGetControlFn get_fn,
				   int                               num_pages,
				   void                             *closure)
{
	MateComponentPropertyControlPrivate *priv;

	g_return_val_if_fail (MATECOMPONENT_IS_EVENT_SOURCE (event_source), NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_PROPERTY_CONTROL (property_control), NULL);

	priv = property_control->priv;
	priv->get_fn = get_fn;
	priv->page_count = num_pages;
	priv->closure = closure;

	priv->event_source = event_source;
	matecomponent_object_add_interface (MATECOMPONENT_OBJECT (property_control),
				     MATECOMPONENT_OBJECT (priv->event_source));

	return property_control;
}

/**
 * matecomponent_property_control_new_full:
 * @get_fn: The function to be called when the getControl method is called.
 * @num_pages: The number of pages this property control has.
 * @event_source: The event source to use to emit events on.
 * @closure: The data to be passed into the @get_fn routine.
 *
 * Creates a MateComponentPropertyControl object.
 *
 * Returns: A pointer to a newly created MateComponentPropertyControl object.
 */
MateComponentPropertyControl *
matecomponent_property_control_new_full (MateComponentPropertyControlGetControlFn get_fn,
				  int                               num_pages,
				  MateComponentEventSource                *event_source,
				  void                             *closure)
{
	MateComponentPropertyControl *property_control;

	g_return_val_if_fail (num_pages > 0, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_EVENT_SOURCE (event_source), NULL);

	property_control = g_object_new (matecomponent_property_control_get_type (), 
					 NULL);
					
	return matecomponent_property_control_construct (
		property_control, event_source, get_fn, num_pages, closure);
}

/**
 * matecomponent_property_control_new:
 * @get_fn: The function to be called when the getControl method is called.
 * @num_pages: The number of pages this property control has.
 * @closure: The data to be passed into the @get_fn routine
 *
 * Creates a MateComponentPropertyControl object.
 *
 * Returns: A pointer to a newly created MateComponentPropertyControl object.
 */
MateComponentPropertyControl *
matecomponent_property_control_new (MateComponentPropertyControlGetControlFn get_fn,
			     int   num_pages,
			     void *closure)
{
	MateComponentEventSource *event_source;

	g_return_val_if_fail (num_pages > 0, NULL);

	event_source = matecomponent_event_source_new ();

	return matecomponent_property_control_new_full (
		get_fn, num_pages, event_source, closure);
}

/**
 * matecomponent_property_control_changed:
 * @property_control: The MateComponentPropertyControl that has changed.
 * @opt_ev: An optional CORBA_Environment for exception handling. 
 *
 * Tells the server that a value in the property control has been changed,
 * and that it should indicate this somehow.
 */
void
matecomponent_property_control_changed (MateComponentPropertyControl *property_control,
				 CORBA_Environment     *opt_ev)
{
	MateComponentPropertyControlPrivate *priv;
	CORBA_Environment ev;
	CORBA_any any;
	CORBA_short s;

	g_return_if_fail (property_control != NULL);
	g_return_if_fail (MATECOMPONENT_IS_PROPERTY_CONTROL (property_control));

	priv = property_control->priv;

	if (opt_ev == NULL)
		CORBA_exception_init (&ev);
	else
		ev = *opt_ev;

	s = 0;
	any._type = (CORBA_TypeCode) TC_CORBA_short;
	any._value = &s;

	matecomponent_event_source_notify_listeners (priv->event_source,
					      MATECOMPONENT_PROPERTY_CONTROL_CHANGED,
					      &any, &ev);
	if (opt_ev == NULL && MATECOMPONENT_EX (&ev)) {
		g_warning ("ERROR: %s", CORBA_exception_id (&ev));
	}

	if (opt_ev == NULL)
		CORBA_exception_free (&ev);
}

/**
 * matecomponent_property_control_get_event_source:
 * @property_control: The MateComponentPropertyControl.
 * 
 * Returns the MateComponentEventSource that @property_control uses.
 * Returns: A MateComponentEventSource.
 */
MateComponentEventSource *
matecomponent_property_control_get_event_source (MateComponentPropertyControl *property_control)
{
	g_return_val_if_fail (property_control != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_PROPERTY_CONTROL (property_control), NULL);

	return property_control->priv->event_source;
}
