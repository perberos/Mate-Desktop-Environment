/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-socket.c: a Gtk+ socket wrapper
 *
 * Authors:
 *   Martin Baulig     (martin@home-of-linux.org)
 *   Michael Meeks     (michael@ximian.com)
 *
 * Copyright 2001, Ximian, Inc.
 *                 Martin Baulig.
 */

#undef GTK_DISABLE_DEPRECATED

#include "config.h"
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <matecomponent/matecomponent-socket.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-control-frame.h>
#include <matecomponent/matecomponent-control-internal.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif


/* Used to turn on any socket sizing
 * bits layered over gtk we have */
#undef DEBUG_RAW_GTK

/* Private part of the MateComponentSocket structure */
typedef struct {
	/* Signal handler ID for the toplevel's GtkWindow::set_focus() */
	gulong set_focus_id;

	/* Whether a descendant of us has the focus.  If this is the case, it
	 * means that we are out-of-process.
	 */
	guint descendant_has_focus : 1;
} MateComponentSocketPrivate;

G_DEFINE_TYPE (MateComponentSocket, matecomponent_socket, GTK_TYPE_SOCKET)

static void
matecomponent_socket_finalize (GObject *object)
{
	MateComponentSocket *socket;
	MateComponentSocketPrivate *priv;

	dbgprintf ("matecomponent_socket_finalize %p\n", object);

	socket = MATECOMPONENT_SOCKET (object);
	priv = socket->priv;

	priv->descendant_has_focus = FALSE;

	g_free (priv);
	socket->priv = NULL;

	G_OBJECT_CLASS (matecomponent_socket_parent_class)->finalize (object);
}

gboolean
matecomponent_socket_disposed (MateComponentSocket *socket)
{
	return (socket->frame == NULL);
}

static void
matecomponent_socket_dispose (GObject *object)
{
	MateComponentSocket *socket = (MateComponentSocket *) object;
	MateComponentSocketPrivate *priv;

	dbgprintf ("matecomponent_socket_dispose %p\n", object);

	priv = socket->priv;

	if (socket->frame) {
		matecomponent_socket_set_control_frame (socket, NULL);
		g_assert (socket->frame == NULL);
	}

	if (priv->set_focus_id) {
		g_assert (socket->socket.toplevel != NULL);
		g_signal_handler_disconnect (socket->socket.toplevel, priv->set_focus_id);
		priv->set_focus_id = 0;
	}

	G_OBJECT_CLASS (matecomponent_socket_parent_class)->dispose (object);
}

static void
matecomponent_socket_realize (GtkWidget *widget)
{
	MateComponentSocket *socket;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATECOMPONENT_IS_SOCKET (widget));

	socket = MATECOMPONENT_SOCKET (widget);

	dbgprintf ("matecomponent_socket_realize %p\n", widget);

	GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->realize (widget);

	if (socket->frame) {
		g_object_ref (socket->frame);
		matecomponent_control_frame_get_remote_window (socket->frame, NULL);
		g_object_unref (socket->frame);
	}

	g_assert (GTK_WIDGET_REALIZED (widget));
}

static void
matecomponent_socket_unrealize (GtkWidget *widget)
{
	dbgprintf ("unrealize %p\n", widget);

	g_assert (GTK_WIDGET_REALIZED (widget));
	g_assert (GTK_WIDGET (widget)->window);

	/* To stop evilness inside Gtk+ */
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED);

	GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->unrealize (widget);
}

static gboolean
matecomponent_socket_expose_event (GtkWidget      *widget,
			    GdkEventExpose *event)
{
	gboolean retval;

	retval = GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->expose_event (widget, event);

	dbgprintf ("matecomponent_socket_expose_event %p (%d, %d), (%d, %d)\n",
		 widget,
		 event->area.x, event->area.y,
		 event->area.width, event->area.height);

#ifdef DEBUG_CONTROL
	gdk_draw_line (widget->window,
		       widget->style->black_gc,
		       event->area.x, event->area.y,
		       event->area.x + event->area.width,
		       event->area.y + event->area.height);
#endif

	return retval;
}

static void
matecomponent_socket_state_changed (GtkWidget   *widget,
			     GtkStateType previous_state)
{
	MateComponentSocket *socket = MATECOMPONENT_SOCKET (widget);

	if (!socket->frame)
		return;

	if (!matecomponent_control_frame_get_autostate (socket->frame))
		return;

	matecomponent_control_frame_control_set_state (
		socket->frame, GTK_WIDGET_STATE (widget));
}

/* Callback for GtkWindow::set_focus().  We watch the focused widget in this way. */
static void
toplevel_set_focus_cb (GtkWindow *window, GtkWidget *focus, gpointer data)
{
	MateComponentSocket *socket;
	MateComponentSocketPrivate *priv;
	GtkWidget *socket_widget;
	gboolean descendant_had_focus;
	gboolean should_autoactivate;

	socket = MATECOMPONENT_SOCKET (data);
	priv = socket->priv;

	g_assert (socket->socket.toplevel == GTK_WIDGET (window));


	socket_widget = GTK_WIDGET (socket);

	descendant_had_focus = priv->descendant_has_focus;

	should_autoactivate = (socket->socket.plug_widget	/* Only in the in-process case */
			       && socket->frame			/* We need an auto-activatable frame */
			       && matecomponent_control_frame_get_autoactivate (socket->frame));

	/* If a descendant of ours is focused then possibly activate its
	 * control, unless there are intermediate sockets between us --- they
	 * should take care of that themselves.
	 */

	if (focus && gtk_widget_get_ancestor (focus, GTK_TYPE_SOCKET) == socket_widget) {
		priv->descendant_has_focus = TRUE;

		if (!descendant_had_focus && should_autoactivate)
			matecomponent_control_frame_control_activate (socket->frame);
	} else {
		priv->descendant_has_focus = FALSE;

		if (descendant_had_focus && should_autoactivate)
			matecomponent_control_frame_control_deactivate (socket->frame);
	}
}

/* GtkWidget::hierarchy_changed() handler.  We have to monitor our toplevel so
 * that we can connect to its GtkWindow::set_focus() signal, so that we can keep
 * track of the currently focused widget.
 */
static void
matecomponent_socket_hierarchy_changed (GtkWidget *widget, GtkWidget *previous_toplevel)
{
	MateComponentSocket *socket;
	MateComponentSocketPrivate *priv;

	socket = MATECOMPONENT_SOCKET (widget);
	priv = socket->priv;

	if (priv->set_focus_id) {
		g_assert (socket->socket.toplevel != NULL);
		g_signal_handler_disconnect (socket->socket.toplevel, priv->set_focus_id);
		priv->set_focus_id = 0;
	}

	(* GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->hierarchy_changed) (widget, previous_toplevel);

	if (socket->socket.toplevel && GTK_IS_WINDOW (socket->socket.toplevel))
		priv->set_focus_id = g_signal_connect_after (socket->socket.toplevel, "set_focus",
							     G_CALLBACK (toplevel_set_focus_cb), socket);
}

/* NOTE: This will only get called in the out-of-process case.  GTK+ only sends
 * focus-in/out events to leaf widgets, not their ancestors.
 */
static gint
matecomponent_socket_focus_in (GtkWidget     *widget,
			GdkEventFocus *focus)
{
	MateComponentSocket *socket = MATECOMPONENT_SOCKET (widget);

	if (socket->frame &&
	    matecomponent_control_frame_get_autoactivate (socket->frame))
		matecomponent_control_frame_control_activate (socket->frame);
	else
		dbgprintf ("No activate on focus in");

	return GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->focus_in_event (widget, focus);
}

/* NOTE: This will only get called in the out-of-process case.  GTK+ only sends
 * focus-in/out events to leaf widgets, not their ancestors.
 */
static gint
matecomponent_socket_focus_out (GtkWidget     *widget,
			 GdkEventFocus *focus)
{
	MateComponentSocket *socket = MATECOMPONENT_SOCKET (widget);

	if (socket->frame &&
	    matecomponent_control_frame_get_autoactivate (socket->frame))
		matecomponent_control_frame_control_deactivate (socket->frame);
	else
		dbgprintf ("No de-activate on focus out");

	return GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->focus_out_event (widget, focus);
}

static void
matecomponent_socket_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
	GtkSocket *socket = (GtkSocket *) widget;

	dbgprintf ("matecomponent_socket_size_allocate %p: (%d, %d), (%d, %d), %p, %p\n",
		 widget, allocation->x, allocation->y,
		 allocation->width, allocation->height,
		 socket->plug_widget, socket->plug_window);

	GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->size_allocate (widget, allocation);
}

static void
matecomponent_socket_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
	MateComponentSocket *socket = (MateComponentSocket *) widget;
	GtkSocket    *gtk_socket = (GtkSocket *) widget;

	dbgprintf ("pre matecomponent_socket_size_request %p: realized %d, %s frame, %d %d\n",
		 widget, GTK_WIDGET_REALIZED (widget) ? 1:0,
		 socket->frame ? "has" : "no",
		 gtk_socket->is_mapped, gtk_socket->have_size);

#ifndef DEBUG_RAW_GTK
	if (GTK_WIDGET_REALIZED (widget) ||
	    !socket->frame ||
	    (gtk_socket->is_mapped && gtk_socket->have_size))
#endif
		GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->size_request
			(widget, requisition);

#ifndef DEBUG_RAW_GTK
	else if (gtk_socket->have_size &&
		 GTK_WIDGET_VISIBLE (gtk_socket)) {

		requisition->width = gtk_socket->request_width;
		requisition->height = gtk_socket->request_height;

	} else {
		CORBA_Environment tmp_ev, *ev;

		CORBA_exception_init ((ev = &tmp_ev));

		matecomponent_control_frame_size_request (
			socket->frame, requisition, ev);

		if (!MATECOMPONENT_EX (ev)) {
			gtk_socket->have_size = TRUE;
			gtk_socket->request_width = requisition->width;
			gtk_socket->request_height = requisition->height;
		}

		CORBA_exception_free (ev);
	}
#endif

	dbgprintf ("matecomponent_socket_size_request %p: %d, %d\n",
		 widget, requisition->width, requisition->height);
}

static void
matecomponent_socket_show (GtkWidget *widget)
{
	dbgprintf ("matecomponent_socket_show %p\n", widget);

	/* We do a check_resize here, since if we're in-proc we
	 * want to force a size_allocate on the contained GtkPlug,
	 * before we go and map it (waiting for the idle resize),
	 * since idle can be held off for a good while, and cause
	 * extreme ugliness and flicker */
	gtk_container_check_resize (GTK_CONTAINER (widget));

	GTK_WIDGET_CLASS (matecomponent_socket_parent_class)->show (widget);
}

static void
matecomponent_socket_show_all (GtkWidget *widget)
{
	/* Do nothing - we don't want this to
	 * propagate to an in-proc plug */
}

static gboolean
matecomponent_socket_plug_removed (GtkSocket *socket)
{
	dbgprintf ("matecomponent_socket_plug_removed %p\n", socket);

	return TRUE;
}

static void
matecomponent_socket_class_init (MateComponentSocketClass *klass)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;
	GtkSocketClass *socket_class;

	gobject_class = (GObjectClass *) klass;
	widget_class  = (GtkWidgetClass *) klass;
	socket_class  = (GtkSocketClass *) klass;

	gobject_class->finalize = matecomponent_socket_finalize;
	gobject_class->dispose  = matecomponent_socket_dispose;

	widget_class->realize           = matecomponent_socket_realize;
	widget_class->unrealize         = matecomponent_socket_unrealize;
	widget_class->state_changed     = matecomponent_socket_state_changed;
	widget_class->hierarchy_changed = matecomponent_socket_hierarchy_changed;
	widget_class->focus_in_event    = matecomponent_socket_focus_in;
	widget_class->focus_out_event   = matecomponent_socket_focus_out;
	widget_class->size_request    	= matecomponent_socket_size_request;
	widget_class->size_allocate   	= matecomponent_socket_size_allocate;
	widget_class->expose_event    	= matecomponent_socket_expose_event;
	widget_class->show            	= matecomponent_socket_show;
	widget_class->show_all        	= matecomponent_socket_show_all;

	socket_class->plug_removed = matecomponent_socket_plug_removed;
}

static void
matecomponent_socket_init (MateComponentSocket *socket)
{
	MateComponentSocketPrivate *priv;

	priv = g_new0 (MateComponentSocketPrivate, 1);
	socket->priv = priv;
}

/**
 * matecomponent_socket_new:
 *
 * Create a new empty #MateComponentSocket.
 *
 * Returns: A new #MateComponentSocket.
 */
GtkWidget*
matecomponent_socket_new (void)
{
	return g_object_new (matecomponent_socket_get_type (), NULL);
}

MateComponentControlFrame *
matecomponent_socket_get_control_frame (MateComponentSocket *socket)
{
	g_return_val_if_fail (MATECOMPONENT_IS_SOCKET (socket), NULL);

	return socket->frame;
}

void
matecomponent_socket_set_control_frame (MateComponentSocket       *socket,
				 MateComponentControlFrame *frame)
{
	MateComponentControlFrame *old_frame;

	g_return_if_fail (MATECOMPONENT_IS_SOCKET (socket));

	if (socket->frame == frame)
		return;

	old_frame = socket->frame;

	if (frame)
		socket->frame = MATECOMPONENT_CONTROL_FRAME (
			matecomponent_object_ref (MATECOMPONENT_OBJECT (frame)));
	else
		socket->frame = NULL;

	if (old_frame) {
		matecomponent_control_frame_set_socket (old_frame, NULL);
		matecomponent_object_unref (MATECOMPONENT_OBJECT (old_frame));
	}

	if (frame)
		matecomponent_control_frame_set_socket (frame, socket);
}

void
matecomponent_socket_add_id (MateComponentSocket   *socket,
		      GdkNativeWindow xid)
{
	GtkSocket *gtk_socket = (GtkSocket *) socket;

	gtk_socket_add_id (gtk_socket, xid);

	/* The allocate didn't get through even to the in-proc case,
	 * so do it again */
	if (gtk_socket->plug_widget) {
		GtkAllocation child_allocation;

		child_allocation.x = 0;
		child_allocation.y = 0;
		child_allocation.width = GTK_WIDGET (gtk_socket)->allocation.width;
		child_allocation.height = GTK_WIDGET (gtk_socket)->allocation.height;

		gtk_widget_size_allocate (gtk_socket->plug_widget,
					  &child_allocation);
	}
}
