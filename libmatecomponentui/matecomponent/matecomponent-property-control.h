/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-property-control.h: Property control implementation.
 *
 * Author:
 *      Iain Holmes  <iain@helixcode.com>
 *
 * Copyright 2000, Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_PROPERTY_CONTROL_H_
#define _MATECOMPONENT_PROPERTY_CONTROL_H_

#include <matecomponent/matecomponent-control.h>
#include <matecomponent/matecomponent-event-source.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateComponentPropertyControl        MateComponentPropertyControl;
typedef struct _MateComponentPropertyControlPrivate MateComponentPropertyControlPrivate;

#define MATECOMPONENT_PROPERTY_CONTROL_CHANGED "MateComponent::PropertyControl_changed"

#define MATECOMPONENT_TYPE_PROPERTY_CONTROL        (matecomponent_property_control_get_type ())
#define MATECOMPONENT_PROPERTY_CONTROL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_PROPERTY_CONTROL, MateComponentPropertyControl))
#define MATECOMPONENT_PROPERTY_CONTROL_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_PROPERTY_CONTROL, MateComponentPropertyControlClass))
#define MATECOMPONENT_IS_PROPERTY_CONTROL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_PROPERTY_CONTROL))
#define MATECOMPONENT_IS_PROPERTY_CONTROL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_PROPERTY_CONTROL))

typedef MateComponentControl *(* MateComponentPropertyControlGetControlFn) (MateComponentPropertyControl *control,
							      int page_number,
							      void *closure);

struct _MateComponentPropertyControl {
        MateComponentObject		object;

	MateComponentPropertyControlPrivate *priv;
};

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_PropertyControl__epv epv;

	gpointer dummy[4];

	void (* action) (MateComponentPropertyControl *property_control,
			 MateComponent_PropertyControl_Action action);
} MateComponentPropertyControlClass;

GType matecomponent_property_control_get_type (void) G_GNUC_CONST;

MateComponentPropertyControl *matecomponent_property_control_construct (MateComponentPropertyControl *property_control,
							  MateComponentEventSource     *event_source,
							  MateComponentPropertyControlGetControlFn get_fn,
							  int                    num_pages,
							  void                  *closure);
MateComponentPropertyControl *matecomponent_property_control_new_full  (MateComponentPropertyControlGetControlFn get_fn,
							  int                    num_pages,
							  MateComponentEventSource     *event_source,
							  void                  *closure);
MateComponentPropertyControl *matecomponent_property_control_new       (MateComponentPropertyControlGetControlFn get_fn,
							  int                    num_pages,
							  void                  *closure);
void                   matecomponent_property_control_changed   (MateComponentPropertyControl *property_control,
							  CORBA_Environment     *opt_ev);
MateComponentEventSource *matecomponent_property_control_get_event_source (MateComponentPropertyControl *property_control);

G_END_DECLS

#endif /* _MATECOMPONENT_PROPERTY_CONTROL_H_ */

