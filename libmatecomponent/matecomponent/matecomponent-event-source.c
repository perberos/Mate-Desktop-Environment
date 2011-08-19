/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-event-source.c: Generic event emitter.
 *
 * Author:
 *	Alex Graveley (alex@ximian.com)
 *	Iain Holmes   (iain@ximian.com)
 *      docs, Miguel de Icaza (miguel@ximian.com)
 *
 * Copyright (C) 2001, Ximian, Inc.
 */
#include <config.h>
#include <time.h>
#include <string.h>
#include <glib-object.h>
#include <matecomponent/matecomponent-listener.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-event-source.h>
#include <matecomponent/matecomponent-running-context.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static GObjectClass *matecomponent_event_source_parent_class;

struct _MateComponentEventSourcePrivate {
	GSList  *listeners;  /* CONTAINS: ListenerDesc* */
	gboolean ignore;
	gint     counter;    /* to create unique listener Ids */
};

typedef struct {
	MateComponent_Listener listener;
	gchar         **event_masks; /* send all events if NULL */
} ListenerDesc;

static inline MateComponentEventSource * 
matecomponent_event_source_from_servant (PortableServer_Servant servant)
{
	return MATECOMPONENT_EVENT_SOURCE (matecomponent_object_from_servant (servant));
}

static void
desc_free (ListenerDesc *desc, CORBA_Environment *ev)
{
	if (desc) {
		g_strfreev (desc->event_masks);
		matecomponent_object_release_unref (desc->listener, ev);
		g_free (desc);
	}
}

static void
impl_MateComponent_EventSource_addListenerWithMask (PortableServer_Servant servant,
					     const MateComponent_Listener  l,
					     const CORBA_char      *event_mask,
					     CORBA_Environment     *ev)
{
	MateComponentEventSource *event_source;
	ListenerDesc      *desc;

	g_return_if_fail (l != CORBA_OBJECT_NIL);

	event_source = matecomponent_event_source_from_servant (servant);

	if (event_source->priv->ignore) /* Hook for running context */
		matecomponent_running_context_ignore_object (l);

	desc = g_new0 (ListenerDesc, 1);
	desc->listener = matecomponent_object_dup_ref (l, ev);

	if (event_mask)
		desc->event_masks = g_strsplit (event_mask, ",", 0);

	event_source->priv->listeners = g_slist_prepend (
		event_source->priv->listeners, desc);
}

static void
impl_MateComponent_EventSource_addListener (PortableServer_Servant servant,
				     const MateComponent_Listener  l,
				     CORBA_Environment     *ev)
{
	impl_MateComponent_EventSource_addListenerWithMask (servant, l, NULL, ev);
}

static void
impl_MateComponent_EventSource_removeListener (PortableServer_Servant servant,
					const MateComponent_Listener  listener,
					CORBA_Environment     *ev)
{
	GSList                   *l, *next;
	MateComponentEventSourcePrivate *priv;

	priv = matecomponent_event_source_from_servant (servant)->priv;

	for (l = priv->listeners; l; l = next) {
		ListenerDesc *desc = l->data;

		next = l->next;

		if (CORBA_Object_is_equivalent (listener, desc->listener, ev)) {
			priv->listeners = g_slist_remove_link (
				priv->listeners, l);
			g_slist_free_1 (l);
			desc_free (desc, ev);
			return;
		}
	}

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_MateComponent_EventSource_UnknownListener, 
			     NULL);
}

/*
 * if the mask starts with a '=', we do exact compares - else we only check 
 * if the mask is a prefix of name.
 */
static gboolean
event_match (const char *name, gchar **event_masks)
{
	int i = 0, j = 0;

	while (event_masks[j]) {
		char *mask = event_masks[j];
		
		if (mask [0] == '=')
			if (!strcmp (name, mask + 1))
				return TRUE;

		while (name [i] && mask [i] && name [i] == mask [i])
			i++;
		
		if (mask [i] == '\0')
			return TRUE;

		j++;
	}
	
	return FALSE;
} 

/**
 * matecomponent_event_source_has_listener:
 * @event_source: the Event Source that will emit the event.
 * @event_name: Name of the event being emitted
 * 
 *   This method determines if there are any listeners for
 * the event to be broadcast. This can be used to detect
 * whether it is worth constructing a potentialy expensive
 * state update, before sending it to no-one.
 * 
 * Return value: TRUE if it's worth sending, else FALSE
 **/
gboolean
matecomponent_event_source_has_listener (MateComponentEventSource *event_source,
				  const char        *event_name)
{
	GSList  *l;
	gboolean notify;

	g_return_val_if_fail (MATECOMPONENT_IS_EVENT_SOURCE (event_source), FALSE);

	notify = FALSE;
	for (l = event_source->priv->listeners; l; l = l->next) {
		ListenerDesc *desc = (ListenerDesc *) l->data;

		if (desc->event_masks == NULL || 
		    event_match (event_name, desc->event_masks)) {
			notify = TRUE;
			break;
		}
	}

	return notify;
}

/**
 * matecomponent_event_source_notify_listeners:
 * @event_source: the Event Source that will emit the event.
 * @event_name: Name of the event being emitted
 * @opt_value: A CORBA_any value that contains the data that is passed
 * to interested clients, or NULL for an empty value
 * @opt_ev: A CORBA_Environment where a failure code can be returned, can be NULL.
 *
 * This will notify all clients that have registered with this EventSource
 * (through the addListener or addListenerWithMask methods) of the availability
 * of the event named @event_name.  The @value CORBA::any value is passed to
 * all listeners.
 *
 * @event_name can not contain comma separators, as commas are used to
 * separate the various event names. 
 */
void
matecomponent_event_source_notify_listeners (MateComponentEventSource *event_source,
				      const char        *event_name,
				      const CORBA_any   *opt_value,
				      CORBA_Environment *opt_ev)
{
	GSList *l, *notify;
	CORBA_Environment  *ev, temp_ev;
	const MateComponentArg *my_value;

	g_return_if_fail (MATECOMPONENT_IS_EVENT_SOURCE (event_source));
	
	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	if (!opt_value)
		my_value = matecomponent_arg_new (MATECOMPONENT_ARG_NULL);
	else
		my_value = opt_value;
	
	notify = NULL;

	for (l = event_source->priv->listeners; l; l = l->next) {
		ListenerDesc *desc = (ListenerDesc *) l->data;

		if (desc->event_masks == NULL || 
		    event_match (event_name, desc->event_masks)) {
			notify = g_slist_prepend (
				notify,
				CORBA_Object_duplicate (desc->listener, ev));
		}
	}

	matecomponent_object_ref (MATECOMPONENT_OBJECT (event_source));

	for (l = notify; l; l = l->next) {
		MateComponent_Listener_event (l->data, event_name, my_value, ev);
		CORBA_Object_release (l->data, ev);
	}

	matecomponent_object_unref (MATECOMPONENT_OBJECT (event_source));

	g_slist_free (notify);

	if (!opt_ev)
		CORBA_exception_free (ev);

	if (!opt_value)
		matecomponent_arg_release ((MateComponentArg *) my_value);
}

void
matecomponent_event_source_notify_listeners_full (MateComponentEventSource *event_source,
					   const char        *path,
					   const char        *type,
					   const char        *subtype,
					   const CORBA_any   *opt_value,
					   CORBA_Environment *opt_ev)
{
	char *event_name;

	event_name = matecomponent_event_make_name (path, type, subtype);

	matecomponent_event_source_notify_listeners (event_source, event_name,
					      opt_value, opt_ev);

	g_free (event_name);
}

static void
matecomponent_event_source_destroy (MateComponentObject *object)
{
	CORBA_Environment         ev;
	MateComponentEventSourcePrivate *priv = MATECOMPONENT_EVENT_SOURCE (object)->priv;
	
	CORBA_exception_init (&ev);

	while (priv->listeners) {
		ListenerDesc *d = priv->listeners->data;

		priv->listeners = g_slist_remove (priv->listeners, d);

		desc_free (d, &ev);
	}
	
	CORBA_exception_free (&ev);

	((MateComponentObjectClass *)matecomponent_event_source_parent_class)->destroy (object);
}

static void
matecomponent_event_source_finalize (GObject *object)
{
	MateComponentEventSourcePrivate *priv;
	
	priv = MATECOMPONENT_EVENT_SOURCE (object)->priv;

	/* in case of strange re-enterancy */
	matecomponent_event_source_destroy (MATECOMPONENT_OBJECT (object));

	g_free (priv);

	matecomponent_event_source_parent_class->finalize (object);
}

static void
matecomponent_event_source_class_init (MateComponentEventSourceClass *klass)
{
	GObjectClass *oclass = (GObjectClass *) klass;
	MateComponentObjectClass *boclass = (MateComponentObjectClass *) klass;
	POA_MateComponent_EventSource__epv *epv = &klass->epv;

	matecomponent_event_source_parent_class = g_type_class_peek_parent (klass);

	oclass->finalize = matecomponent_event_source_finalize;
	boclass->destroy = matecomponent_event_source_destroy;

	epv->addListener         = impl_MateComponent_EventSource_addListener;
	epv->addListenerWithMask = impl_MateComponent_EventSource_addListenerWithMask;
	epv->removeListener      = impl_MateComponent_EventSource_removeListener;
}

static void
matecomponent_event_source_init (GObject *object)
{
	MateComponentEventSource *event_source;

	event_source = MATECOMPONENT_EVENT_SOURCE (object);
	event_source->priv = g_new0 (MateComponentEventSourcePrivate, 1);
	event_source->priv->listeners = NULL;
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentEventSource, 
		       MateComponent_EventSource,
		       PARENT_TYPE,
		       matecomponent_event_source)

/**
 * matecomponent_event_source_new:
 *
 * Creates a new MateComponentEventSource object.  Typically this
 * object will be exposed to clients through CORBA and they
 * will register and unregister functions to be notified
 * of events that this EventSource generates.
 * 
 * To notify clients of an event, use the matecomponent_event_source_notify_listeners()
 * function.
 *
 * Returns: A new #MateComponentEventSource server object.
 */
MateComponentEventSource *
matecomponent_event_source_new (void)
{
	return g_object_new (MATECOMPONENT_TYPE_EVENT_SOURCE, NULL);
}

/**
 * matecomponent_event_source_ignore_listeners:
 * @event_source: 
 * 
 *  Instructs the event source to de-register any listeners
 * that are added from the global running context.
 **/
void
matecomponent_event_source_ignore_listeners (MateComponentEventSource *event_source)
{
	g_return_if_fail (MATECOMPONENT_IS_EVENT_SOURCE (event_source));

	event_source->priv->ignore = TRUE;
}

void
matecomponent_event_source_client_remove_listener (MateComponent_Unknown     object,
					    MateComponent_Listener    listener,
					    CORBA_Environment *opt_ev)
{
	MateComponent_Unknown     es;
	CORBA_Environment *ev, temp_ev;

	g_return_if_fail (object != CORBA_OBJECT_NIL);
       
	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	es = MateComponent_Unknown_queryInterface (object, 
	        "IDL:MateComponent/EventSource:1.0", ev);

	if (!MATECOMPONENT_EX (ev) && es) {

		MateComponent_EventSource_removeListener (es, listener, ev);

		MateComponent_Unknown_unref (es, ev);
	}

	if (!opt_ev) {
		if (MATECOMPONENT_EX (ev)) {
			char *text = matecomponent_exception_get_text (ev);
			g_warning ("remove_listener failed '%s'", text);
			g_free (text);
		}
		CORBA_exception_free (ev);
	}
}

MateComponent_Listener
matecomponent_event_source_client_add_listener_full (MateComponent_Unknown     object,
					      GClosure          *event_callback,
					      const char        *opt_mask,
					      CORBA_Environment *opt_ev)
{
	MateComponentListener    *listener = NULL;
	MateComponent_Listener    corba_listener = CORBA_OBJECT_NIL;
	MateComponent_Unknown     es;
	CORBA_Environment *ev, temp_ev;

	g_return_val_if_fail (event_callback != NULL, CORBA_OBJECT_NIL);
	
	if (!opt_ev) {
		ev = &temp_ev;
		CORBA_exception_init (ev);
	} else
		ev = opt_ev;

	es = MateComponent_Unknown_queryInterface (object, 
		"IDL:MateComponent/EventSource:1.0", ev);

	if (MATECOMPONENT_EX (ev) || !es)
		goto add_listener_end;

	if (!(listener = matecomponent_listener_new_closure (event_callback)))
		goto add_listener_end;

	corba_listener = MATECOMPONENT_OBJREF (listener);
	
	if (opt_mask)
		MateComponent_EventSource_addListenerWithMask (
			es, corba_listener, opt_mask, ev);
	else 
		MateComponent_EventSource_addListener (
			es, corba_listener, ev);

	corba_listener = CORBA_Object_duplicate (corba_listener, ev);

	matecomponent_object_unref (MATECOMPONENT_OBJECT (listener));

	matecomponent_object_release_unref (es, ev);

 add_listener_end:

	if (!opt_ev) {
		if (MATECOMPONENT_EX (ev)) {
			char *text = matecomponent_exception_get_text (ev);
			g_warning ("add_listener failed '%s'", text);
			g_free (text);
		}
		CORBA_exception_free (ev);
	}

	return corba_listener;
} 

void
matecomponent_event_source_client_add_listener_closure (MateComponent_Unknown     object,
						 GClosure          *event_callback,
						 const char        *opt_mask,
						 CORBA_Environment *opt_ev)
{
	MateComponent_Listener l;

	l = matecomponent_event_source_client_add_listener_full (
		object, event_callback, opt_mask, opt_ev);

	if (l != CORBA_OBJECT_NIL)
		CORBA_Object_release (l, NULL);
}

void
matecomponent_event_source_client_add_listener (MateComponent_Unknown           object,
					 MateComponentListenerCallbackFn event_callback,
					 const char              *opt_mask,
					 CORBA_Environment       *opt_ev,
					 gpointer                 user_data)
{
	matecomponent_event_source_client_add_listener_closure (
		object, g_cclosure_new (G_CALLBACK (event_callback), user_data, NULL),
		opt_mask, opt_ev);
}

