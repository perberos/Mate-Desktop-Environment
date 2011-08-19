/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-listener.c: Generic listener interface for callbacks.
 *
 * Authors:
 *	Alex Graveley (alex@helixcode.com)
 *	Mike Kestner  (mkestner@ameritech.net)
 *
 * Copyright (C) 2000, Ximian, Inc.
 */
#include <config.h>
#include <string.h>

#include <glib-object.h>

#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-listener.h>
#include <matecomponent/matecomponent-marshal.h>
#include <matecomponent/matecomponent-types.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static GObjectClass *matecomponent_listener_parent_class;

struct _MateComponentListenerPrivate {
	GClosure *event_callback;
};

enum SIGNALS {
	EVENT_NOTIFY,
	LAST_SIGNAL
};
static guint signals [LAST_SIGNAL] = { 0 };

static void
impl_MateComponent_Listener_event (PortableServer_Servant servant, 
			    const CORBA_char      *event_name, 
			    const CORBA_any       *args, 
			    CORBA_Environment     *ev)
{
	MateComponentListener *listener;

	listener = MATECOMPONENT_LISTENER (matecomponent_object_from_servant (servant));

	matecomponent_object_ref (MATECOMPONENT_OBJECT (listener));

	if (listener->priv->event_callback)
		matecomponent_closure_invoke (
			listener->priv->event_callback,
			G_TYPE_NONE,
			MATECOMPONENT_TYPE_LISTENER,                       listener,
			G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE, event_name,
			MATECOMPONENT_TYPE_STATIC_CORBA_ANY,               args,
			MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION,         ev,
			G_TYPE_INVALID);
		
	g_signal_emit (G_OBJECT (listener),
		       signals [EVENT_NOTIFY], 0,
		       event_name, args, ev);

	matecomponent_object_unref (MATECOMPONENT_OBJECT (listener));
}

static void
matecomponent_listener_destroy (MateComponentObject *object)
{
	MateComponentListener *listener = (MateComponentListener *) object;

	if (listener->priv->event_callback) {
		g_closure_unref (listener->priv->event_callback);
		listener->priv->event_callback = NULL;
	}
	
	((MateComponentObjectClass *)matecomponent_listener_parent_class)->destroy (object);
}

static void
matecomponent_listener_finalize (GObject *object)
{
	MateComponentListener *listener;

	listener = MATECOMPONENT_LISTENER (object);

	g_free (listener->priv);
	listener->priv = NULL;

	matecomponent_listener_parent_class->finalize (object);
}

static void
matecomponent_listener_class_init (MateComponentListenerClass *klass)
{
	GObjectClass *oclass = (GObjectClass *) klass;
	MateComponentObjectClass *boclass = (MateComponentObjectClass *) klass;
	POA_MateComponent_Listener__epv *epv = &klass->epv;

	matecomponent_listener_parent_class = g_type_class_peek_parent (klass);

	oclass->finalize = matecomponent_listener_finalize;
	boclass->destroy = matecomponent_listener_destroy;

	signals [EVENT_NOTIFY] = g_signal_new (
		"event_notify", G_TYPE_FROM_CLASS (oclass), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (MateComponentListenerClass, event_notify),
		NULL, NULL,
		matecomponent_marshal_VOID__STRING_BOXED_BOXED,
		G_TYPE_NONE, 3,
		G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
		MATECOMPONENT_TYPE_STATIC_CORBA_ANY,
		MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION);

	epv->event = impl_MateComponent_Listener_event;
}

static void
matecomponent_listener_init (GObject *object)
{
	MateComponentListener *listener;

	listener = MATECOMPONENT_LISTENER(object);
	listener->priv = g_new (MateComponentListenerPrivate, 1);
	listener->priv->event_callback = NULL;
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentListener, 
		       MateComponent_Listener,
		       PARENT_TYPE,
		       matecomponent_listener)

/**
 * matecomponent_listener_new_closure:
 * @event_closure: closure to be invoked when an event is emitted by the EventSource.
 *
 * Creates a generic event listener.  The listener invokes the @event_closure 
 * closure and emits an "event_notify" signal when notified of an event.  
 * The signal callback should be of the form:
 *
 * <informalexample>
 * <programlisting>
 *      void some_callback (MateComponentListener *listener,
 *                          char *event_name, 
 *			    CORBA_any *any,
 *			    CORBA_Environment *ev,
 *			    gpointer user_data);
 * </programlisting>
 * </informalexample>
 *
 * You will typically pass the CORBA_Object reference in the return value
 * to an EventSource (by invoking EventSource::addListener).
 *
 * Returns: A MateComponentListener object.
 */
MateComponentListener*
matecomponent_listener_new_closure (GClosure *event_closure)
{
	MateComponentListener *listener;

	listener = g_object_new (MATECOMPONENT_TYPE_LISTENER, NULL);

	listener->priv->event_callback = matecomponent_closure_store (
		event_closure, matecomponent_marshal_VOID__STRING_BOXED_BOXED);

	return listener;
}

/**
 * matecomponent_listener_new:
 * @event_cb: function to be invoked when an event is emitted by the EventSource.
 * @user_data: data passed to the functioned pointed by @event_call.
 *
 * Creates a generic event listener.  The listener calls the @event_callback 
 * function and emits an "event_notify" signal when notified of an event.  
 * The signal callback should be of the form:
 *
 * <informalexample>
 * <programlisting>
 *      void some_callback (MateComponentListener *listener,
 *                          char *event_name, 
 *			    CORBA_any *any,
 *			    CORBA_Environment *ev,
 *			    gpointer user_data);
 * </programlisting>
 * </informalexample>
 *
 * You will typically pass the CORBA_Object reference in the return value
 * to an EventSource (by invoking EventSource::addListener).
 *
 * Returns: A MateComponentListener object.
 */
MateComponentListener *
matecomponent_listener_new (MateComponentListenerCallbackFn event_cb,
		     gpointer                 user_data)
{
	GClosure *closure;

	if (event_cb)
		closure = g_cclosure_new (G_CALLBACK (event_cb), user_data, NULL);
	else
		closure = NULL;

	return matecomponent_listener_new_closure (closure);
}

/**
 * matecomponent_event_make_name:
 * @idl_path: the IDL part of the event name.
 * @kind: the kind of the event
 * @subtype: an optional subtype
 *
 * Creates an event name. Event names consist of three parts. The @idl_path is
 * mainly to create a unique namespace, and should identify the interface 
 * which triggered the event, for example "MateComponent/Property". The @kind denotes
 * what happened, for example "change". Finally you can use the optional 
 * @subtype to make events more specific. All three parts of the name are 
 * joined together separated by colons. "MateComponent/Property:change" or 
 * "MateComponent/Property:change:autosave" are examples of valid event names.
 *
 * Returns: A valid event_name, or NULL on error.
 */
char *
matecomponent_event_make_name (const char *idl_path, 
			const char *kind,
			const char *subtype)
{
	g_return_val_if_fail (idl_path != NULL, NULL);
	g_return_val_if_fail (kind != NULL, NULL);
	g_return_val_if_fail (!strchr (idl_path, ':'), NULL);
	g_return_val_if_fail (!strchr (kind, ':'), NULL);
	g_return_val_if_fail (!subtype || !strchr (subtype, ':'), NULL);
	g_return_val_if_fail (strlen (idl_path), NULL);
	g_return_val_if_fail (strlen (kind), NULL);
	g_return_val_if_fail (!subtype || strlen (subtype), NULL);

	if (subtype)
		return g_strconcat (idl_path, ":", kind, ":", 
				    subtype, NULL);
	else
		return g_strconcat (idl_path, ":", kind, NULL);
}

static gboolean
matecomponent_event_name_valid (const char *event_name)
{
	gint i = 0, c = 0, l = -1;

	g_return_val_if_fail (event_name != NULL, FALSE);
	g_return_val_if_fail (strlen (event_name), FALSE);

	if (event_name [0] == ':') 
		return FALSE;

	if (event_name [strlen (event_name) - 1] == ':') 
		return FALSE;

	while (event_name [i]) {
		if (event_name [i] == ':') {
			if (l == (i -1))
				return FALSE;
			l = i;
			c++;
		}
		i++;
	}

	if ((c == 1) || (c == 2)) 
		return TRUE;

	return FALSE;
}

static char *
matecomponent_event_token (const char *event_name, gint pos)
{
	char **str_array, *res;

	if (!matecomponent_event_name_valid (event_name))
		return NULL;

	str_array = g_strsplit (event_name, ":", 3);

	res = g_strdup (str_array [pos]);

	g_strfreev (str_array);

	return res;
}

/**
 * matecomponent_event_type:
 * @event_name: the event name
 *
 * The event type consists of the first two parts of the event name, the idl_path
 * combined with the kind.
 *
 * Returns: The event type, or NULL on error.
 */
char *
matecomponent_event_type (const char *event_name)
{
	gint i = 0, c = 0;
       
	if (!matecomponent_event_name_valid (event_name))
		return NULL;

	while (event_name [i]) { 
		if (event_name [i] == ':') 
			c++;
		if (c == 2) 
			break;
		i++;
	}

	return g_strndup (event_name, i);
}

/**
 * matecomponent_event_type:
 * @event_name: the event name
 *
 * Returns: The event subtype, or NULL on error.
 */
char *
matecomponent_event_subtype (const char *event_name)
{
	return matecomponent_event_token (event_name, 2);
}

/**
 * matecomponent_event_kind:
 * @event_name: the event name
 *
 * Returns: The event kind, or NULL on error.
 */
char *
matecomponent_event_kind (const char *event_name)
{
	return matecomponent_event_token (event_name, 1);
}

/**
 * matecomponent_event_idl_path:
 * @event_name: the event name
 *
 * Returns: The event idl path, or NULL on error.
 */
char *
matecomponent_event_idl_path (const char *event_name)
{
	return matecomponent_event_token (event_name, 0);
}
