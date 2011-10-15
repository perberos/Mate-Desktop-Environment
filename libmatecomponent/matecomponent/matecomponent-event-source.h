/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-event-source.h: Generic event emitter.
 *
 * Author:
 *	Alex Graveley (alex@ximian.com)
 *
 * Copyright (C) 2001, Ximian, Inc.
 */
#ifndef _MATECOMPONENT_EVENT_SOURCE_H_
#define _MATECOMPONENT_EVENT_SOURCE_H_

#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-listener.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_EVENT_SOURCE        (matecomponent_event_source_get_type ())
#define MATECOMPONENT_EVENT_SOURCE_TYPE        MATECOMPONENT_TYPE_EVENT_SOURCE /* deprecated, you should use MATECOMPONENT_TYPE_EVENT_SOURCE */
#define MATECOMPONENT_EVENT_SOURCE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_EVENT_SOURCE, MateComponentEventSource))
#define MATECOMPONENT_EVENT_SOURCE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_EVENT_SOURCE, MateComponentEventSourceClass))
#define MATECOMPONENT_IS_EVENT_SOURCE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_EVENT_SOURCE))
#define MATECOMPONENT_IS_EVENT_SOURCE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_EVENT_SOURCE))

typedef struct _MateComponentEventSourcePrivate MateComponentEventSourcePrivate;
typedef struct _MateComponentEventSource        MateComponentEventSource;

struct _MateComponentEventSource {
	MateComponentObject             parent;
	MateComponentEventSourcePrivate *priv;
};

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_EventSource__epv epv;
} MateComponentEventSourceClass;

GType              matecomponent_event_source_get_type         (void) G_GNUC_CONST;
MateComponentEventSource *matecomponent_event_source_new              (void);
gboolean           matecomponent_event_source_has_listener     (MateComponentEventSource *event_source,
							 const char        *event_name);
void               matecomponent_event_source_notify_listeners (MateComponentEventSource *event_source,
							 const char        *event_name,
							 const CORBA_any   *opt_value,
							 CORBA_Environment *opt_ev);

void        matecomponent_event_source_notify_listeners_full   (MateComponentEventSource *event_source,
							 const char        *path,
							 const char        *type,
							 const char        *subtype,
							 const CORBA_any   *opt_value,
							 CORBA_Environment *opt_ev);

void        matecomponent_event_source_client_remove_listener  (MateComponent_Unknown     object,
							 MateComponent_Listener    listener,
							 CORBA_Environment *opt_ev);

void        matecomponent_event_source_client_add_listener     (MateComponent_Unknown           object,
							 MateComponentListenerCallbackFn event_callback,
							 const char              *opt_mask,
							 CORBA_Environment       *opt_ev,
							 gpointer                 user_data);


void        matecomponent_event_source_client_add_listener_closure   (MateComponent_Unknown     object,
							       GClosure          *callback,
							       const char        *opt_mask,
							       CORBA_Environment *opt_ev);

MateComponent_Listener matecomponent_event_source_client_add_listener_full  (MateComponent_Unknown     object,
							       GClosure          *callback,
							       const char        *opt_mask,
							       CORBA_Environment *opt_ev);

/* You don't want this routine */
void            matecomponent_event_source_ignore_listeners        (MateComponentEventSource *event_source);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_EVENT_SOURCE_H_ */

