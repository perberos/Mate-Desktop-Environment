/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * MateComponent control frame object.
 *
 * Authors:
 *   Nat Friedman    (nat@helixcode.com)
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_CONTROL_FRAME_H_
#define _MATECOMPONENT_CONTROL_FRAME_H_

#include <glib.h>
#include <gtk/gtk.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-ui-component.h>
#include <matecomponent/matecomponent-property-bag-client.h>

typedef struct _MateComponentControlFrame MateComponentControlFrame;

#include <matecomponent/matecomponent-socket.h>

G_BEGIN_DECLS
 
#define MATECOMPONENT_TYPE_CONTROL_FRAME        (matecomponent_control_frame_get_type ())
#define MATECOMPONENT_CONTROL_FRAME(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_CONTROL_FRAME, MateComponentControlFrame))
#define MATECOMPONENT_CONTROL_FRAME_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST ((k), MATECOMPONENT_TYPE_CONTROL_FRAME, MateComponentControlFrameClass))
#define MATECOMPONENT_IS_CONTROL_FRAME(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_CONTROL_FRAME))
#define MATECOMPONENT_IS_CONTROL_FRAME_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_CONTROL_FRAME))

typedef struct _MateComponentControlFramePrivate MateComponentControlFramePrivate;

struct _MateComponentControlFrame {
	MateComponentObject base;
	MateComponentControlFramePrivate *priv;
};

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_ControlFrame__epv epv;

	gpointer dummy[4];

	/* Signals. */
	void (*activated)           (MateComponentControlFrame *control_frame, gboolean state);
	void (*activate_uri)        (MateComponentControlFrame *control_frame, const char *uri, gboolean relative);
} MateComponentControlFrameClass;

#define MATECOMPONENT_CONTROL_FRAME_TOPLEVEL_PROP "matecomponent:toplevel"

/* Object construction stuff */
GType                         matecomponent_control_frame_get_type                  (void) G_GNUC_CONST;
MateComponentControlFrame           *matecomponent_control_frame_construct                 (MateComponentControlFrame  *control_frame,
									      MateComponent_UIContainer   ui_container,
									      CORBA_Environment   *ev);
MateComponentControlFrame           *matecomponent_control_frame_new                       (MateComponent_UIContainer   ui_container);

GtkWidget                    *matecomponent_control_frame_get_widget                (MateComponentControlFrame  *frame);

/* This is only allowed when the Control is deactivated */
void                          matecomponent_control_frame_set_ui_container          (MateComponentControlFrame  *control_frame,
									      MateComponent_UIContainer   uic,
									      CORBA_Environment   *ev);

/* Activating remote controls */
void                          matecomponent_control_frame_control_activate          (MateComponentControlFrame  *control_frame);
void                          matecomponent_control_frame_control_deactivate        (MateComponentControlFrame  *control_frame);
void                          matecomponent_control_frame_set_autoactivate          (MateComponentControlFrame  *control_frame,
									      gboolean             autoactivate);
gboolean                      matecomponent_control_frame_get_autoactivate          (MateComponentControlFrame  *control_frame);

/* Remote properties */
MateComponent_PropertyBag            matecomponent_control_frame_get_control_property_bag  (MateComponentControlFrame  *control_frame,
									      CORBA_Environment   *opt_ev);

/* Ambient properties */
void                          matecomponent_control_frame_set_propbag               (MateComponentControlFrame  *control_frame,
									      MateComponentPropertyBag   *propbag);
MateComponentPropertyBag            *matecomponent_control_frame_get_propbag               (MateComponentControlFrame  *control_frame);

/* Widget state proxying */
void                          matecomponent_control_frame_control_set_state         (MateComponentControlFrame  *control_frame,
									      GtkStateType         state);
void                          matecomponent_control_frame_set_autostate             (MateComponentControlFrame  *control_frame,
									      gboolean             autostate);
gboolean                      matecomponent_control_frame_get_autostate             (MateComponentControlFrame  *control_frame);

/* Connecting to the remote control */
void                          matecomponent_control_frame_bind_to_control           (MateComponentControlFrame  *control_frame,
									      MateComponent_Control       control,
									      CORBA_Environment   *opt_ev);

MateComponent_Control                matecomponent_control_frame_get_control               (MateComponentControlFrame  *control_frame);

MateComponent_UIContainer            matecomponent_control_frame_get_ui_container          (MateComponentControlFrame  *control_frame);

MateComponentUIComponent            *matecomponent_control_frame_get_popup_component       (MateComponentControlFrame  *control_frame,
									      CORBA_Environment   *opt_ev);
    
G_END_DECLS

#endif /* _MATECOMPONENT_CONTROL_FRAME_H_ */
