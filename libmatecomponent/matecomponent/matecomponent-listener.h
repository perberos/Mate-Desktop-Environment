/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-listener.h: Generic listener interface for callbacks.
 *
 * Authors:
 *	Alex Graveley (alex@helixcode.com)
 *	Mike Kestner  (mkestner@ameritech.net)
 *
 * Copyright (C) 2000, Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_LISTENER_H_
#define _MATECOMPONENT_LISTENER_H_

#include <matecomponent/matecomponent-arg.h>
#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_LISTENER        (matecomponent_listener_get_type ())
#define MATECOMPONENT_LISTENER_TYPE        MATECOMPONENT_TYPE_LISTENER /* deprecated, you should use MATECOMPONENT_TYPE_LISTENER */
#define MATECOMPONENT_LISTENER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_LISTENER, MateComponentListener))
#define MATECOMPONENT_LISTENER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_LISTENER, MateComponentListenerClass))
#define MATECOMPONENT_IS_LISTENER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_LISTENER))
#define MATECOMPONENT_IS_LISTENER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_LISTENER))

typedef struct _MateComponentListenerPrivate MateComponentListenerPrivate;
typedef struct _MateComponentListener        MateComponentListener;

struct _MateComponentListener {
        MateComponentObject          parent;

	MateComponentListenerPrivate *priv;
};

typedef struct {
	MateComponentObjectClass     parent_class;

	POA_MateComponent_Listener__epv epv;

	/* Signals */
	void (* event_notify) (MateComponentListener    *listener,
			       char              *event_name,
			       MateComponentArg         *event_data,
			       CORBA_Environment *ev);
} MateComponentListenerClass;


typedef void (*MateComponentListenerCallbackFn)    (MateComponentListener    *listener,
					     const char        *event_name,
					     const CORBA_any   *any,
					     CORBA_Environment *ev,
					     gpointer           user_data);

GType           matecomponent_listener_get_type    (void) G_GNUC_CONST;

MateComponentListener *matecomponent_listener_new         (MateComponentListenerCallbackFn event_cb,
					     gpointer                 user_data);

MateComponentListener *matecomponent_listener_new_closure (GClosure                *event_closure);

char           *matecomponent_event_make_name      (const char *idl_path,
					     const char *kind,
					     const char *subtype);

char           *matecomponent_event_type           (const char *event_name);
char           *matecomponent_event_subtype        (const char *event_name);
char           *matecomponent_event_kind           (const char *event_name);
char           *matecomponent_event_idl_path       (const char *event_name);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_LISTENER_H_ */

