/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * MateComponent control internal prototypes / helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_CONTROL_INTERNAL_H_
#define _MATECOMPONENT_CONTROL_INTERNAL_H_

#include <matecomponent/matecomponent-plug.h>
#include <matecomponent/matecomponent-socket.h>
#include <matecomponent/matecomponent-control.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent/matecomponent-control-frame.h>

G_BEGIN_DECLS

void     matecomponent_control_add_listener            (CORBA_Object        object,
						 GCallback           fn,
						 gpointer            user_data,
						 CORBA_Environment  *ev);

void     matecomponent_control_frame_get_remote_window (MateComponentControlFrame *frame,
						 CORBA_Environment  *opt_ev);
gboolean matecomponent_control_frame_focus             (MateComponentControlFrame *frame,
						 GtkDirectionType    direction);
void     matecomponent_control_frame_size_request      (MateComponentControlFrame *frame,
						 GtkRequisition     *requisition,
						 CORBA_Environment  *opt_ev);
void     matecomponent_control_frame_set_inproc_widget (MateComponentControlFrame *frame,
						 MateComponentPlug         *matecomponent_plug,
						 GtkWidget          *control_widget);

MateComponentSocket       *matecomponent_control_frame_get_socket (MateComponentControlFrame *frame);
MateComponentControlFrame *matecomponent_socket_get_control_frame (MateComponentSocket       *socket);
void                matecomponent_control_frame_set_socket (MateComponentControlFrame *frame,
						     MateComponentSocket       *socket);
void                matecomponent_socket_set_control_frame (MateComponentSocket       *socket,
						     MateComponentControlFrame *frame);

MateComponentControl      *matecomponent_plug_get_control         (MateComponentPlug         *plug);
void                matecomponent_control_set_plug         (MateComponentControl      *control,
						     MateComponentPlug         *plug);
void                matecomponent_plug_set_control         (MateComponentPlug         *plug,
						     MateComponentControl      *control);
gboolean            matecomponent_socket_disposed          (MateComponentSocket       *socket);

G_END_DECLS

#endif /* _MATECOMPONENT_CONTROL_INTERNAL_H_ */
