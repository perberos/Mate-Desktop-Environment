/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-plug.c: a Gtk plug wrapper.
 *
 * Author:
 *   Martin Baulig     (martin@home-of-linux.org)
 *   Michael Meeks     (michael@ximian.com)
 *
 * Copyright 2001, Ximian, Inc.
 *                 Martin Baulig.
 */

#undef GTK_DISABLE_DEPRECATED

#include "config.h"
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n-lib.h>
#include <matecomponent/matecomponent-plug.h>
#include <matecomponent/matecomponent-control.h>
#include <matecomponent/matecomponent-control-internal.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#else
#error Port to this GDK backend
#endif

struct _MateComponentPlugPrivate {
	gboolean forward_events;
};

enum {
	PROP_0,
	PROP_FORWARD_EVENTS
};

G_DEFINE_TYPE (MateComponentPlug, matecomponent_plug, GTK_TYPE_PLUG)

/**
 * matecomponent_plug_construct:
 * @plug: The #MateComponentPlug.
 * @socket_id: the XID of the socket's window.
 *
 * Finish the creation of a #MateComponentPlug widget. This function
 * will generally only be used by classes deriving
 * from #MateComponentPlug.
 */
void
matecomponent_plug_construct (MateComponentPlug *plug, guint32 socket_id)
{
	matecomponent_plug_construct_full (plug, gdk_display_get_default (), socket_id);
}

/**
 * matecomponent_plug_construct_full:
 * @plug: The #MateComponentPlug.
 * @socket_id: the XID of the socket's window.
 *
 * Finish the creation of a #MateComponentPlug widget. This function
 * will generally only be used by classes deriving
 * from #MateComponentPlug.
 */
void
matecomponent_plug_construct_full (MateComponentPlug *plug,
			    GdkDisplay *display,
			    guint32     socket_id)
{
	gtk_plug_construct_for_display
		(GTK_PLUG (plug), display, socket_id);
}

/**
 * matecomponent_plug_new_for_display:
 * @display: the associated display
 * @socket_id: the XID of the socket's window.
 *
 * Create a new plug widget inside the #GtkSocket identified
 * by @socket_id.
 *
 * Returns: the new #MateComponentPlug widget.
 */
GtkWidget*
matecomponent_plug_new_for_display (GdkDisplay *display,
			     guint32     socket_id)
{
	MateComponentPlug *plug;

	plug = MATECOMPONENT_PLUG (g_object_new (matecomponent_plug_get_type (), NULL));

	matecomponent_plug_construct_full (plug, display, socket_id);

	dbgprintf ("matecomponent_plug_new => %p\n", plug);

	return GTK_WIDGET (plug);
}

/**
 * matecomponent_plug_new:
 * @socket_id: the XID of the socket's window.
 *
 * Create a new plug widget inside the #GtkSocket identified
 * by @socket_id.
 *
 * Returns: the new #MateComponentPlug widget.
 */
GtkWidget*
matecomponent_plug_new (guint32 socket_id)
{
	return matecomponent_plug_new_for_display
		(gdk_display_get_default (), socket_id);
}

MateComponentControl *
matecomponent_plug_get_control (MateComponentPlug *plug)
{
	g_return_val_if_fail (MATECOMPONENT_IS_PLUG (plug), NULL);

	return plug->control;
}

void
matecomponent_plug_set_control (MateComponentPlug    *plug,
			 MateComponentControl *control)
{
	MateComponentControl *old_control;

	g_return_if_fail (MATECOMPONENT_IS_PLUG (plug));

	if (plug->control == control)
		return;

	dbgprintf ("matecomponent_plug_set_control (%p, %p) [%p]\n",
		 plug, control, plug->control);

	old_control = plug->control;

	if (control)
		plug->control = g_object_ref (control);
	else
		plug->control = NULL;

	if (old_control) {
		matecomponent_control_set_plug (old_control, NULL);
		g_object_unref (old_control);
	}

	if (control)
		matecomponent_control_set_plug (control, plug);
}

static gboolean
matecomponent_plug_delete_event (GtkWidget   *widget,
			  GdkEventAny *event)
{
	dbgprintf ("matecomponent_plug_delete_event %p\n", widget);

	return FALSE;
}

static void
matecomponent_plug_realize (GtkWidget *widget)
{
	MateComponentPlug *plug = (MateComponentPlug *) widget;

	dbgprintf ("matecomponent_plug_realize %p\n", plug);

	GTK_WIDGET_CLASS (matecomponent_plug_parent_class)->realize (widget);
}

static void
matecomponent_plug_unrealize (GtkWidget *widget)
{
	MateComponentPlug *plug = (MateComponentPlug *) widget;

	dbgprintf ("matecomponent_plug_unrealize %p\n", plug);

	GTK_WIDGET_CLASS (matecomponent_plug_parent_class)->unrealize (widget);
}

static void
matecomponent_plug_map (GtkWidget *widget)
{
	dbgprintf ("matecomponent_plug_map %p at size %d, %d\n",
		 widget, widget->allocation.width,
		 widget->allocation.height);
	GTK_WIDGET_CLASS (matecomponent_plug_parent_class)->map (widget);
}

static void
matecomponent_plug_dispose (GObject *object)
{
	MateComponentPlug *plug = (MateComponentPlug *) object;
	GtkBin *bin_plug = (GtkBin *) object;

	dbgprintf ("matecomponent_plug_dispose %p\n", object);

	if (bin_plug->child) {
		gtk_container_remove (
			&bin_plug->container, bin_plug->child);
		dbgprintf ("Removing child ...");
	}

	if (plug->control)
		matecomponent_plug_set_control (plug, NULL);

	G_OBJECT_CLASS (matecomponent_plug_parent_class)->dispose (object);
}

static void
matecomponent_plug_finalize (GObject *object)
{
	MateComponentPlug *plug = (MateComponentPlug *) object;

	g_free (plug->priv);

	G_OBJECT_CLASS (matecomponent_plug_parent_class)->finalize (object);
}

static void
matecomponent_plug_set_property (GObject      *object,
			  guint         param_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	MateComponentPlug *plug;

	g_return_if_fail (MATECOMPONENT_IS_PLUG (object));

	plug = MATECOMPONENT_PLUG (object);

	switch (param_id) {
	case PROP_FORWARD_EVENTS:
		plug->priv->forward_events = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
matecomponent_plug_get_property (GObject    *object,
			  guint       param_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	MateComponentPlug *plug;

	g_return_if_fail (MATECOMPONENT_IS_PLUG (object));

	plug = MATECOMPONENT_PLUG (object);

	switch (param_id) {
	case PROP_FORWARD_EVENTS:
		g_value_set_boolean (value, plug->priv->forward_events);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
matecomponent_plug_size_allocate (GtkWidget     *widget,
			   GtkAllocation *allocation)
{
	dbgprintf ("matecomponent_plug_size_allocate %p: (%d, %d), (%d, %d) %d! %s\n",
		 widget,
		 allocation->x, allocation->y,
		 allocation->width, allocation->height,
		 GTK_WIDGET_TOPLEVEL (widget),
		 GTK_BIN (widget)->child ?
		 g_type_name_from_instance ((gpointer)GTK_BIN (widget)->child):
		 "No child!");

	GTK_WIDGET_CLASS (matecomponent_plug_parent_class)->size_allocate (widget, allocation);
}

static void
matecomponent_plug_size_request (GtkWidget      *widget,
			  GtkRequisition *requisition)
{
	GTK_WIDGET_CLASS (matecomponent_plug_parent_class)->size_request (widget, requisition);

	dbgprintf ("matecomponent_plug_size_request %p: %d, %d\n",
		 widget, requisition->width, requisition->height);
}

static gboolean
matecomponent_plug_expose_event (GtkWidget      *widget,
			  GdkEventExpose *event)
{
	gboolean retval;

	retval = GTK_WIDGET_CLASS (matecomponent_plug_parent_class)->expose_event (widget, event);

	dbgprintf ("matecomponent_plug_expose_event %p (%d, %d), (%d, %d)"
		 "%s (%d && %d == %d)\n",
		 widget,
		 event->area.x, event->area.y,
		 event->area.width, event->area.height,
		 GTK_WIDGET_TOPLEVEL (widget) ? "toplevel" : "bin class",
		 GTK_WIDGET_VISIBLE (widget),
		 GTK_WIDGET_MAPPED (widget),
		 GTK_WIDGET_DRAWABLE (widget));

#ifdef DEBUG_CONTROL
	gdk_draw_line (widget->window,
		       widget->style->black_gc,
		       event->area.x + event->area.width,
		       event->area.y,
		       event->area.x, 
		       event->area.y + event->area.height);

	gdk_draw_line (widget->window,
		       widget->style->black_gc,
		       widget->allocation.x,
		       widget->allocation.y,
		       widget->allocation.x + widget->allocation.width,
		       widget->allocation.y + widget->allocation.height);
#endif

	return retval;
}

static gboolean
matecomponent_plug_button_event (GtkWidget      *widget,
			  GdkEventButton *event)
{
#if defined (GDK_WINDOWING_X11)
	XEvent xevent;
#endif

	g_return_val_if_fail (MATECOMPONENT_IS_PLUG (widget), FALSE);

	if (!MATECOMPONENT_PLUG (widget)->priv->forward_events || !GTK_WIDGET_TOPLEVEL (widget))
		return FALSE;

#if defined (GDK_WINDOWING_X11)

	if (event->type == GDK_BUTTON_PRESS) {
		xevent.xbutton.type = ButtonPress;

		/* X does an automatic pointer grab on button press
		 * if we have both button press and release events
		 * selected.
		 * We don't want to hog the pointer on our parent.
		 */
		gdk_display_pointer_ungrab
			(gtk_widget_get_display (widget),
			 GDK_CURRENT_TIME);
	} else
		xevent.xbutton.type = ButtonRelease;
    
	xevent.xbutton.display     = GDK_WINDOW_XDISPLAY (widget->window);
	xevent.xbutton.window      = GDK_WINDOW_XWINDOW (GTK_PLUG (widget)->socket_window);
	xevent.xbutton.root        = GDK_WINDOW_XWINDOW (gdk_screen_get_root_window
							 (gdk_drawable_get_screen (widget->window)));
	/*
	 * FIXME: the following might cause
	 *        big problems for non-GTK apps
	 */
	xevent.xbutton.x           = 0;
	xevent.xbutton.y           = 0;
	xevent.xbutton.x_root      = 0;
	xevent.xbutton.y_root      = 0;
	xevent.xbutton.state       = event->state;
	xevent.xbutton.button      = event->button;
	xevent.xbutton.same_screen = TRUE; /* FIXME ? */

	gdk_error_trap_push ();

	XSendEvent (GDK_WINDOW_XDISPLAY (widget->window),
		    GDK_WINDOW_XWINDOW (GTK_PLUG (widget)->socket_window),
		    False, NoEventMask, &xevent);

	gdk_flush ();
	gdk_error_trap_pop ();

#elif defined (GDK_WINDOWING_WIN32)
	/* FIXME: Need to do something? */
#endif
	return TRUE;
}

static void
matecomponent_plug_init (MateComponentPlug *plug)
{
	MateComponentPlugPrivate *priv;

	priv = g_new0 (MateComponentPlugPrivate, 1);

	plug->priv = priv;

	priv->forward_events = TRUE;
}

static void
matecomponent_plug_class_init (MateComponentPlugClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

	gobject_class->dispose      = matecomponent_plug_dispose;
	gobject_class->finalize     = matecomponent_plug_finalize;
	gobject_class->set_property = matecomponent_plug_set_property;
	gobject_class->get_property = matecomponent_plug_get_property;

	widget_class->realize              = matecomponent_plug_realize;
	widget_class->unrealize            = matecomponent_plug_unrealize;
	widget_class->delete_event         = matecomponent_plug_delete_event;
	widget_class->size_request         = matecomponent_plug_size_request;
	widget_class->size_allocate        = matecomponent_plug_size_allocate;
	widget_class->expose_event         = matecomponent_plug_expose_event;
	widget_class->button_press_event   = matecomponent_plug_button_event;
	widget_class->button_release_event = matecomponent_plug_button_event;
	widget_class->map                  = matecomponent_plug_map;

	g_object_class_install_property (
		gobject_class,
		PROP_FORWARD_EVENTS,
		g_param_spec_boolean ("event_forwarding",
				    _("Event Forwarding"),
				    _("Whether X events should be forwarded"),
				      TRUE,
				      G_PARAM_READABLE | G_PARAM_WRITABLE));
}
