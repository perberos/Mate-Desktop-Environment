/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * MateComponent control frame object.
 *
 * Authors:
 *   Michael Meeks     (michael@ximian.com)
 *   Nat Friedman      (nat@ximian.com)
 *   Federico Mena     (federico@ximian.com)
 *   Miguel de Icaza   (miguel@ximian.com)
 *   George Lebel      (jirka@5z.com)
 *   Maciej Stachowiak (mjs@eazel.com)
 *
 * Copyright 1999, 2001 Ximian, Inc.
 *                 2000 Eazel, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-control.h>
#include <matecomponent/matecomponent-control-frame.h>
#include <gdk/gdkprivate.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#else
#error Port to this GDK backend
#endif
#include <gdk/gdk.h>
#include <matecomponent/matecomponent-socket.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-ui-marshal.h>
#include <matecomponent/matecomponent-control-internal.h>

enum {
	ACTIVATED,
	ACTIVATE_URI,
	LAST_SIGNAL
};

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static guint control_frame_signals [LAST_SIGNAL];

/* Parent object class in GTK hierarchy */
static GObjectClass *matecomponent_control_frame_parent_class;

struct _MateComponentControlFramePrivate {
	MateComponentControl     *inproc_control;
	MateComponent_Control	   control;
	GtkWidget	  *socket;
	MateComponent_UIContainer ui_container;
	MateComponentPropertyBag *propbag;
	gboolean           autoactivate;
	gboolean           autostate;
	gboolean           activated;
};

static void
control_connection_died_cb (gpointer connection,
			    gpointer user_data)
{
	CORBA_Object cntl;
	CORBA_Environment ev;
	MateComponentControlFrame *frame = MATECOMPONENT_CONTROL_FRAME (user_data);

	g_return_if_fail (frame != NULL);

	dbgprintf ("The remote control end died unexpectedly");

	CORBA_exception_init (&ev);

	cntl = CORBA_Object_duplicate (frame->priv->control, &ev);

	matecomponent_control_frame_bind_to_control (
		frame, CORBA_OBJECT_NIL, NULL);

	CORBA_exception_set_system (
		&ev, ex_CORBA_COMM_FAILURE, CORBA_COMPLETED_YES);

	matecomponent_object_check_env (MATECOMPONENT_OBJECT (frame), cntl, &ev);

	CORBA_Object_release (cntl, &ev);

	CORBA_exception_free (&ev);
}

static MateComponent_Gdk_WindowId
impl_MateComponent_ControlFrame_getToplevelId (PortableServer_Servant  servant,
					CORBA_Environment      *ev)
{
	MateComponentControlFrame *frame;
	GtkWidget          *toplev;
	MateComponent_Gdk_WindowId id;

	frame = MATECOMPONENT_CONTROL_FRAME (matecomponent_object (servant));

	for (toplev = matecomponent_control_frame_get_widget (frame);
	     toplev && toplev->parent;
	     toplev = toplev->parent)
		;

	matecomponent_return_val_if_fail (toplev != NULL, NULL, ev);

	if (MATECOMPONENT_IS_PLUG (toplev)) { 
		MateComponentControl      *control;
		MateComponent_ControlFrame frame;

		control = matecomponent_plug_get_control (MATECOMPONENT_PLUG (toplev));
		if (!control) {
			g_warning ("No control bound to plug from which to "
				   "get transient parent");
			return CORBA_string_dup ("");
		}

		frame = matecomponent_control_get_control_frame (control, ev);
		if (frame == CORBA_OBJECT_NIL) {
			g_warning ("No control frame associated with control from "
				   "which to get transient parent");
			return CORBA_string_dup ("");
		}

		id = MateComponent_ControlFrame_getToplevelId (frame, ev);
	} else
#if defined (GDK_WINDOWING_X11)
		id = matecomponent_control_window_id_from_x11 (
			GDK_WINDOW_XWINDOW (toplev->window));
#elif defined (GDK_WINDOWING_WIN32)
		id = matecomponent_control_window_id_from_x11 (
			(guint32) GDK_WINDOW_HWND (toplev->window));
#endif

	return id;
}

static MateComponent_PropertyBag
impl_MateComponent_ControlFrame_getAmbientProperties (PortableServer_Servant servant,
					       CORBA_Environment     *ev)
{
	MateComponentControlFrame *frame;

	frame = MATECOMPONENT_CONTROL_FRAME (matecomponent_object (servant));

	if (!frame->priv->propbag)
		return CORBA_OBJECT_NIL;

	return matecomponent_object_dup_ref (
		MATECOMPONENT_OBJREF (frame->priv->propbag), ev);
}

static MateComponent_UIContainer
impl_MateComponent_ControlFrame_getUIContainer (PortableServer_Servant  servant,
					 CORBA_Environment      *ev)
{
	MateComponentControlFrame *frame = MATECOMPONENT_CONTROL_FRAME (
		matecomponent_object_from_servant (servant));

	if (frame->priv->ui_container == NULL)
		return CORBA_OBJECT_NIL;

	return matecomponent_object_dup_ref (frame->priv->ui_container, ev);
}

/* --- Notifications --- */

static void
impl_MateComponent_ControlFrame_notifyActivated (PortableServer_Servant  servant,
					  const CORBA_boolean     state,
					  CORBA_Environment      *ev)
{
	MateComponentControlFrame *frame = MATECOMPONENT_CONTROL_FRAME (
		matecomponent_object_from_servant (servant));

	g_signal_emit (frame, control_frame_signals [ACTIVATED], 0, state);
}

static void
impl_MateComponent_ControlFrame_queueResize (PortableServer_Servant  servant,
				      CORBA_Environment      *ev)
{
	/* Supposedly Gtk+ handles size negotiation properly for us */
/*	MateComponentSocket *socket;
	MateComponentControlFrame *frame;

	frame = MATECOMPONENT_CONTROL_FRAME (matecomponent_object (servant));

	if ((socket = frame->priv->socket)) {
		GTK_SOCKET (socket)->have_size = FALSE;
		gtk_widget_queue_resize (GTK_WIDGET (socket));
		} */
}

static void
impl_MateComponent_ControlFrame_activateURI (PortableServer_Servant  servant,
				       const CORBA_char       *uri,
				       CORBA_boolean           relative,
				       CORBA_Environment      *ev)
{
	MateComponentControlFrame *frame = MATECOMPONENT_CONTROL_FRAME (matecomponent_object_from_servant (servant));

	g_signal_emit (frame,
		       control_frame_signals [ACTIVATE_URI], 0,
		       (const char *) uri, (gboolean) relative);
}

#ifdef DEBUG_CONTROL
static void
dump_geom (GdkWindow *window)
{
	gint x, y, width, height, depth;
	
	gdk_window_get_geometry (window, &x, &y,
				 &width, &height, &depth);
	
	fprintf (stderr, "geom (%d, %d), (%d, %d), %d ",
		 x, y, width, height, depth);
}

static void
dump_gdk_tree (GdkWindow *window)
{
	GList     *l;
	GtkWidget *widget = NULL;

	gdk_window_get_user_data (window, (gpointer) &widget);

	fprintf (stderr, "Window %p (parent %p) ", window,
		 gdk_window_get_parent (window));

	dump_geom (window);

	if (widget) {
		fprintf (stderr, "has widget '%s' %s ",
			 g_type_name_from_instance ((gpointer) widget),
			 GTK_WIDGET_VISIBLE (widget) ? "visible" : "hidden");
	} else
		fprintf (stderr, "No widget ");

	fprintf (stderr, "gdk: %s %s ", 
		 gdk_window_is_visible (window) ? "visible" : "invisible",
		 gdk_window_is_viewable (window) ? "viewable" : "not viewable");
	
	l = gdk_window_peek_children (window);
	fprintf (stderr, "%d children:\n", g_list_length (l));

	for (; l; l = l->next)
		dump_gdk_tree (l->data);

	fprintf (stderr, "\n");
}
#endif

static CORBA_char *
matecomponent_control_frame_get_remote_window_id (MateComponentControlFrame *frame,
					   CORBA_Environment  *ev)
{
	CORBA_char  *retval;
	char        *cookie;
	int          screen;

	screen = gdk_screen_get_number (
			gtk_widget_get_screen (frame->priv->socket));

	cookie = g_strdup_printf ("screen=%d", screen);

	retval = MateComponent_Control_getWindowId (
				frame->priv->control, cookie, ev);

	g_free (cookie);

	return retval;
}

void
matecomponent_control_frame_get_remote_window (MateComponentControlFrame *frame,
					CORBA_Environment  *opt_ev)
					
{
	CORBA_char *id;
	CORBA_Environment *ev, tmp_ev;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	dbgprintf ("matecomponent_control_frame_get_remote_window "
		 "%p %p %d %p\n", frame->priv, frame->priv->socket,
		 GTK_WIDGET_REALIZED (frame->priv->socket),
		 frame->priv->control);

	if (!frame->priv || !frame->priv->socket ||
	    !GTK_WIDGET_REALIZED (frame->priv->socket) ||
	    frame->priv->control == CORBA_OBJECT_NIL)
		return;

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	/* Introduce ourselves to the Control. */
	id = matecomponent_control_frame_get_remote_window_id (frame, ev);
	if (MATECOMPONENT_EX (ev)) {
		dbgprintf ("getWindowId exception\n");
		matecomponent_object_check_env (MATECOMPONENT_OBJECT (frame),
					 frame->priv->control, ev);

	} else {
		GdkNativeWindow xid;
		MateComponentPlug *plug = NULL;

		xid = matecomponent_control_x11_from_window_id (id);
		dbgprintf ("setFrame id '%s' (=%u)\n", id, xid);
		CORBA_free (id);

		{
			GdkDisplay *display = gtk_widget_get_display (frame->priv->socket);
			gpointer user_data = NULL;
			if (gdk_window_lookup_for_display (display, xid)) {
				gdk_window_get_user_data
					(gdk_window_lookup_for_display (display, xid),
					 &user_data);
				plug = user_data;
			}
		}

		/* FIXME: What happens if we have an in-proc CORBA proxy eg.
		 * for a remote X window ? - we need to treat these differently. */

		if (plug && !frame->priv->inproc_control) {
			g_warning ("ARGH - serious ORB screwup");
			frame->priv->inproc_control = matecomponent_plug_get_control (plug);
		} else if (!plug && frame->priv->inproc_control) 
			g_warning ("ARGH - different serious ORB screwup");

		matecomponent_socket_add_id (MATECOMPONENT_SOCKET (frame->priv->socket), xid);
	}		

	if (!opt_ev)
		CORBA_exception_free (ev);
}

/**
 * matecomponent_control_frame_construct:
 * @control_frame: The #MateComponentControlFrame object to be initialized.
 * @ui_container: A CORBA object for the UIContainer for the container application.
 *
 * Initializes @control_frame with the parameters.
 *
 * Returns: the initialized MateComponentControlFrame object @control_frame that implements the
 * MateComponent::ControlFrame CORBA service.
 */
MateComponentControlFrame *
matecomponent_control_frame_construct (MateComponentControlFrame *frame,
				MateComponent_UIContainer  ui_container,
				CORBA_Environment  *ev)
{
	g_return_val_if_fail (ev != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), NULL);

	matecomponent_control_frame_set_ui_container (frame, ui_container, ev);

	return frame;
}

/**
 * matecomponent_control_frame_new:
 * @ui_container: The #MateComponent_UIContainer for the container application.
 *
 * Returns: MateComponentControlFrame object that implements the
 * MateComponent::ControlFrame CORBA service. 
 */
MateComponentControlFrame *
matecomponent_control_frame_new (MateComponent_UIContainer ui_container)
{
	CORBA_Environment   ev;
	MateComponentControlFrame *frame;

	frame = g_object_new (MATECOMPONENT_TYPE_CONTROL_FRAME, NULL);

	CORBA_exception_init (&ev);
	frame = matecomponent_control_frame_construct (frame, ui_container, &ev);
	CORBA_exception_free (&ev);

	return frame;
}

static void
matecomponent_control_frame_dispose (GObject *object)
{
	MateComponentControlFrame *frame = MATECOMPONENT_CONTROL_FRAME (object);

	dbgprintf ("matecomponent_control_frame_dispose %p\n", object);

	if (frame->priv->socket)
		matecomponent_control_frame_set_socket (frame, NULL);

	matecomponent_control_frame_set_propbag (frame, NULL);

	matecomponent_control_frame_bind_to_control (
		frame, CORBA_OBJECT_NIL, NULL);

	matecomponent_control_frame_set_ui_container (
		frame, CORBA_OBJECT_NIL, NULL);

	matecomponent_control_frame_parent_class->dispose (object);
}

static void
matecomponent_control_frame_finalize (GObject *object)
{
	MateComponentControlFrame *frame = MATECOMPONENT_CONTROL_FRAME (object);

	dbgprintf ("matecomponent_control_frame_finalize %p\n", object);

	g_free (frame->priv);
	
	matecomponent_control_frame_parent_class->finalize (object);
}

static void
matecomponent_control_frame_activated (MateComponentControlFrame *frame, gboolean state)
{
	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	frame->priv->activated = state;
}

static void
matecomponent_control_frame_class_init (MateComponentControlFrameClass *klass)
{
	GObjectClass      *object_class = (GObjectClass *) klass;
	POA_MateComponent_ControlFrame__epv *epv = &klass->epv;

	matecomponent_control_frame_parent_class = g_type_class_peek_parent (klass);

	control_frame_signals [ACTIVATED] =
		g_signal_new ("activated",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentControlFrameClass, activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);

	
	control_frame_signals [ACTIVATE_URI] =
		g_signal_new ("activate_uri",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentControlFrameClass, activate_uri),
			      NULL, NULL,
			      matecomponent_ui_marshal_VOID__STRING_BOOLEAN,
			      G_TYPE_NONE, 2,
			      G_TYPE_STRING, G_TYPE_BOOLEAN);
	
	klass->activated = matecomponent_control_frame_activated;

	object_class->finalize = matecomponent_control_frame_finalize;
	object_class->dispose  = matecomponent_control_frame_dispose;
	
	epv->getToplevelId        = impl_MateComponent_ControlFrame_getToplevelId;
	epv->getAmbientProperties = impl_MateComponent_ControlFrame_getAmbientProperties;
	epv->getUIContainer       = impl_MateComponent_ControlFrame_getUIContainer;

	epv->notifyActivated      = impl_MateComponent_ControlFrame_notifyActivated;
	epv->queueResize          = impl_MateComponent_ControlFrame_queueResize;
	epv->activateURI          = impl_MateComponent_ControlFrame_activateURI;
}

static void
matecomponent_control_frame_init (MateComponentObject *object)
{
	MateComponentControlFrame *frame = MATECOMPONENT_CONTROL_FRAME (object);
	MateComponentSocket       *socket;

	frame->priv               = g_new0 (MateComponentControlFramePrivate, 1);
	frame->priv->autoactivate = FALSE;
	frame->priv->autostate    = TRUE;

	socket = MATECOMPONENT_SOCKET (matecomponent_socket_new ());
	gtk_widget_show (GTK_WIDGET (socket));
	matecomponent_control_frame_set_socket (frame, socket);
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentControlFrame, 
		       MateComponent_ControlFrame,
		       PARENT_TYPE,
		       matecomponent_control_frame)

/**
 * matecomponent_control_frame_control_activate:
 * @control_frame: The MateComponentControlFrame object whose control should be
 * activated.
 *
 * Activates the MateComponentControl embedded in @control_frame by calling the
 * activate() #MateComponent_Control interface method on it.
 */
void
matecomponent_control_frame_control_activate (MateComponentControlFrame *frame)
{
	CORBA_Environment ev;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	/*
	 * Check that this ControLFrame actually has a Control associated
	 * with it.
	 */
	g_return_if_fail (frame->priv->control != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	MateComponent_Control_activate (frame->priv->control, TRUE, &ev);

	if (MATECOMPONENT_EX (&ev)) {
		matecomponent_object_check_env (
			MATECOMPONENT_OBJECT (frame),
			(CORBA_Object) frame->priv->control, &ev);

	}

	CORBA_exception_free (&ev);
}


/**
 * matecomponent_control_frame_control_deactivate:
 * @control_frame: The MateComponentControlFrame object whose control should be
 * deactivated.
 *
 * Deactivates the MateComponentControl embedded in @frame by calling
 * the activate() CORBA method on it with the parameter %FALSE.
 */
void
matecomponent_control_frame_control_deactivate (MateComponentControlFrame *frame)
{
	CORBA_Environment ev;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	/*
	 * Check that this ControlFrame actually has a Control associated
	 * with it.
	 */
	g_return_if_fail (frame->priv->control != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	MateComponent_Control_activate (frame->priv->control, FALSE, &ev);

	if (MATECOMPONENT_EX (&ev)) {
		matecomponent_object_check_env (
			MATECOMPONENT_OBJECT (frame),
			(CORBA_Object) frame->priv->control, &ev);
	}

	CORBA_exception_free (&ev);
}

/**
 * matecomponent_control_frame_set_autoactivate:
 * @frame: A MateComponentControlFrame object.
 * @autoactivate: A flag which indicates whether or not the
 * ControlFrame should automatically perform activation on the Control
 * to which it is bound.
 *
 * Modifies the autoactivate behavior of @frame.  If
 * @frame is set to autoactivate, then it will automatically
 * send an "activate" message to the Control to which it is bound when
 * it gets a focus-in event, and a "deactivate" message when it gets a
 * focus-out event.  Autoactivation is off by default.
 */
void
matecomponent_control_frame_set_autoactivate (MateComponentControlFrame  *frame,
				       gboolean             autoactivate)
{
	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	frame->priv->autoactivate = autoactivate;
}


/**
 * matecomponent_control_frame_get_autoactivate:
 * @frame: A #MateComponentControlFrame object.
 *
 * Returns: A boolean which indicates whether or not @frame is
 * set to automatically activate its Control.  See
 * matecomponent_control_frame_set_autoactivate().
 */
gboolean
matecomponent_control_frame_get_autoactivate (MateComponentControlFrame *frame)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), FALSE);

	return frame->priv->autoactivate;
}

static MateComponent_Gtk_State
matecomponent_control_frame_state_to_corba (const GtkStateType state)
{
	switch (state) {
	case GTK_STATE_NORMAL:
		return MateComponent_Gtk_StateNormal;

	case GTK_STATE_ACTIVE:
		return MateComponent_Gtk_StateActive;

	case GTK_STATE_PRELIGHT:
		return MateComponent_Gtk_StatePrelight;

	case GTK_STATE_SELECTED:
		return MateComponent_Gtk_StateSelected;

	case GTK_STATE_INSENSITIVE:
		return MateComponent_Gtk_StateInsensitive;

	default:
		g_warning ("matecomponent_control_frame_state_to_corba: Unknown state: %d", (gint) state);
		return MateComponent_Gtk_StateNormal;
	}
}

/**
 * matecomponent_control_frame_control_set_state:
 * @frame: A #MateComponentControlFrame object which is bound to a
 * remote #MateComponentControl.
 * @state: A #GtkStateType value, specifying the widget state to apply
 * to the remote control.
 *
 * Proxies @state to the control bound to @frame.
 */
void
matecomponent_control_frame_control_set_state (MateComponentControlFrame  *frame,
					GtkStateType         state)
{
	MateComponent_Gtk_State  corba_state;
	CORBA_Environment ev;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));
	g_return_if_fail (frame->priv->control != CORBA_OBJECT_NIL);

	corba_state = matecomponent_control_frame_state_to_corba (state);

	CORBA_exception_init (&ev);

	MateComponent_Control_setState (frame->priv->control, corba_state, &ev);

	if (MATECOMPONENT_EX (&ev)) {
		matecomponent_object_check_env (
			MATECOMPONENT_OBJECT (frame),
			frame->priv->control, &ev);
	}

	CORBA_exception_free (&ev);
}

/**
 * matecomponent_control_frame_set_autostate:
 * @frame: A #MateComponentControlFrame object.
 * @autostate: Whether or not GtkWidget state changes should be
 * automatically propagated down to the Control.
 *
 * Changes whether or not @frame automatically proxies
 * state changes to its associated control.  The default mode
 * is for the control frame to autopropagate.
 */
void
matecomponent_control_frame_set_autostate (MateComponentControlFrame  *frame,
				    gboolean             autostate)
{
	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	frame->priv->autostate = autostate;
}

/**
 * matecomponent_control_frame_get_autostate:
 * @frame: A #MateComponentControlFrame object.
 *
 * Returns: Whether or not this control frame will automatically
 * proxy GtkState changes to its associated Control.
 */
gboolean
matecomponent_control_frame_get_autostate (MateComponentControlFrame *frame)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), FALSE);

	return frame->priv->autostate;
}


/**
 * matecomponent_control_frame_get_ui_container:
 * @frame: A MateComponentControlFrame object.

 * Returns: The MateComponent_UIContainer object reference associated with this
 * ControlFrame.  This ui_container is specified when the ControlFrame is
 * created.  See matecomponent_control_frame_new().
 */
MateComponent_UIContainer
matecomponent_control_frame_get_ui_container (MateComponentControlFrame *frame)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), CORBA_OBJECT_NIL);

	return frame->priv->ui_container;
}

/**
 * matecomponent_control_frame_set_ui_container:
 * @frame: A MateComponentControlFrame object.
 * @uic: A MateComponent_UIContainer object reference.
 *
 * Associates a new %MateComponent_UIContainer object with this ControlFrame. This
 * is only allowed while the Control is deactivated.
 */
void
matecomponent_control_frame_set_ui_container (MateComponentControlFrame *frame,
				       MateComponent_UIContainer  ui_container,
				       CORBA_Environment  *opt_ev)
{
	CORBA_Environment *ev, tmp_ev;
	MateComponent_UIContainer old_ui_container;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	old_ui_container = frame->priv->ui_container;

	if (old_ui_container == ui_container)
		return;

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	/* See ui-faq.txt if this dies on you. */
	if (ui_container != CORBA_OBJECT_NIL) {
		if (frame->priv->activated) 
			g_warning ("Trying to associate an new UI container with an activated control frame");
		
		g_assert (CORBA_Object_is_a (
			ui_container, "IDL:MateComponent/UIContainer:1.0", ev));

		frame->priv->ui_container = matecomponent_object_dup_ref (
			ui_container, ev);
	} else
		frame->priv->ui_container = CORBA_OBJECT_NIL;

	if (old_ui_container)
		matecomponent_object_release_unref (old_ui_container, ev);

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

/**
 * matecomponent_control_frame_bind_to_control:
 * @frame: A MateComponentControlFrame object.
 * @control: The CORBA object for the MateComponentControl embedded
 * in this MateComponentControlFrame.
 * @opt_ev: Optional exception environment
 *
 * Associates @control with this @frame.
 */
void
matecomponent_control_frame_bind_to_control (MateComponentControlFrame *frame,
				      MateComponent_Control      control,
				      CORBA_Environment  *opt_ev)
{
	CORBA_Environment *ev, tmp_ev;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	if (control == frame->priv->control)
		return;

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	g_object_ref (frame);

	if (frame->priv->control != CORBA_OBJECT_NIL) {
		if (!frame->priv->inproc_control)
			MateCORBA_small_unlisten_for_broken (
				frame->priv->control,
				G_CALLBACK (control_connection_died_cb));

		/* Unset ourselves as the frame */
		MateComponent_Control_setFrame (frame->priv->control,
					 CORBA_OBJECT_NIL, ev);

		if (frame->priv->control != CORBA_OBJECT_NIL)
			matecomponent_object_release_unref (frame->priv->control, ev);

		CORBA_exception_free (ev);
	}

	if (control == CORBA_OBJECT_NIL) {
		frame->priv->control = CORBA_OBJECT_NIL;
		frame->priv->inproc_control = NULL;
	} else {
		frame->priv->control = matecomponent_object_dup_ref (control, ev);

		frame->priv->inproc_control = (MateComponentControl *)
			matecomponent_object (MateCORBA_small_get_servant (control));

		if (!frame->priv->inproc_control)
			matecomponent_control_add_listener (
				frame->priv->control,
				G_CALLBACK (control_connection_died_cb),
				frame, ev);

		MateComponent_Control_setFrame (
			frame->priv->control,
			MATECOMPONENT_OBJREF (frame), ev);

		matecomponent_control_frame_get_remote_window (frame, ev);
	}

	g_object_unref (frame);

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

/**
 * matecomponent_control_frame_get_control:
 * @frame: A MateComponentControlFrame which is bound to a remote
 * MateComponentControl.
 *
 * Returns: The MateComponent_Control CORBA interface for the remote Control
 * which is bound to @frame.  See also
 * matecomponent_control_frame_bind_to_control().
 */
MateComponent_Control
matecomponent_control_frame_get_control (MateComponentControlFrame *frame)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), CORBA_OBJECT_NIL);

	return frame->priv->control;
}

/**
 * matecomponent_control_frame_get_widget:
 * @frame: The MateComponentControlFrame whose widget is being requested.a
 *
 * Use this function when you want to embed a MateComponentControl into your
 * container's widget hierarchy.  Once you have bound the
 * MateComponentControlFrame to a remote MateComponentControl, place the widget
 * returned by matecomponent_control_frame_get_widget() into your widget
 * hierarchy and the control will appear in your application.
 *
 * Returns: A GtkWidget which has the remote MateComponentControl physically
 * inside it.
 */
GtkWidget *
matecomponent_control_frame_get_widget (MateComponentControlFrame *frame)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), NULL);

	return frame->priv->socket;
}

/**
 * matecomponent_control_frame_set_propbag:
 * @frame: A MateComponentControlFrame object.
 * @propbag: A MateComponentPropertyBag which will hold @frame's
 * ambient properties.
 *
 * Makes @frame use @propbag for its ambient properties.  When
 * @frame's Control requests the ambient properties, it will
 * get them from @propbag.
 */

void
matecomponent_control_frame_set_propbag (MateComponentControlFrame  *frame,
				  MateComponentPropertyBag   *propbag)
{
	MateComponentPropertyBag *old_pb;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));
	g_return_if_fail (propbag == NULL ||
			  MATECOMPONENT_IS_PROPERTY_BAG (propbag));

	old_pb = frame->priv->propbag;

	if (old_pb == propbag)
		return;

	frame->priv->propbag = matecomponent_object_ref ((MateComponentObject *) propbag);
	matecomponent_object_unref ((MateComponentObject *) old_pb);
}

/**
 * matecomponent_control_frame_get_propbag:
 * @frame: A MateComponentControlFrame object whose PropertyBag has
 * been set.
 *
 * Returns: The MateComponentPropertyBag object which has been associated with
 * @frame.
 */
MateComponentPropertyBag *
matecomponent_control_frame_get_propbag (MateComponentControlFrame  *frame)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), NULL);

	return frame->priv->propbag;
}

/**
 * matecomponent_control_frame_get_control_property_bag:
 * @frame: the control frame
 * @ev: CORBA exception environment
 * 
 * This retrives a MateComponent_PropertyBag reference from its
 * associated MateComponent Control
 *
 * Return value: CORBA property bag reference or CORBA_OBJECT_NIL
 **/
MateComponent_PropertyBag
matecomponent_control_frame_get_control_property_bag (MateComponentControlFrame *frame,
					       CORBA_Environment  *opt_ev)
{
	MateComponent_PropertyBag pbag;
	MateComponent_Control control;
	CORBA_Environment *real_ev, tmp_ev;

	g_return_val_if_fail (frame != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), NULL);

	if (opt_ev)
		real_ev = opt_ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	control = frame->priv->control;

	/* FIXME: we could cache this here - is it called a lot ? */
	pbag = MateComponent_Control_getProperties (control, real_ev);

	if (MATECOMPONENT_EX (real_ev)) {
		if (!opt_ev)
			CORBA_exception_free (&tmp_ev);
		pbag = CORBA_OBJECT_NIL;
	}

	return pbag;
}

void
matecomponent_control_frame_size_request (MateComponentControlFrame *frame,
				   GtkRequisition     *requisition,
				   CORBA_Environment  *opt_ev)
{
	CORBA_Environment      *ev, tmp_ev;
	MateComponent_Gtk_Requisition  req;

	g_return_if_fail (requisition != NULL);
	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	if (frame->priv->control == CORBA_OBJECT_NIL) {
		/* We haven't been bound to a control yet, so return "I don't
		 * care about what I get assigned".
		 */
		requisition->width = requisition->height = 1;
		return;
	}

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	req = MateComponent_Control_getDesiredSize (frame->priv->control, ev);

	if (MATECOMPONENT_EX (ev)) {
		matecomponent_object_check_env (
			MATECOMPONENT_OBJECT (frame),
			(CORBA_Object) frame->priv->control, ev);

		req.width = req.height = 1;
	}

	requisition->width  = req.width;
	requisition->height = req.height;

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

/**
 * matecomponent_control_frame_focus:
 * @frame: A control frame.
 * @direction: Direction in which to change focus.
 * 
 * Proxies a #GtkContainer::focus() request to the embedded control.  This is an
 * internal function and it should only really be ever used by the #MateComponentSocket
 * implementation.
 * 
 * Return value: TRUE if the child kept the focus, FALSE if focus should be
 * passed on to the next widget.
 **/
gboolean
matecomponent_control_frame_focus (MateComponentControlFrame *frame,
			    GtkDirectionType    direction)
{
	MateComponentControlFramePrivate *priv;
	CORBA_Environment ev;
	gboolean result;
	MateComponent_Gtk_Direction corba_direction;

	g_return_val_if_fail (frame != NULL, FALSE);
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), FALSE);

	priv = frame->priv;

	if (priv->control == CORBA_OBJECT_NIL)
		return FALSE;

	switch (direction) {
	case GTK_DIR_TAB_FORWARD:
		corba_direction = MateComponent_Gtk_DirectionTabForward;
		break;

	case GTK_DIR_TAB_BACKWARD:
		corba_direction = MateComponent_Gtk_DirectionTabBackward;
		break;

	case GTK_DIR_UP:
		corba_direction = MateComponent_Gtk_DirectionUp;
		break;

	case GTK_DIR_DOWN:
		corba_direction = MateComponent_Gtk_DirectionDown;
		break;

	case GTK_DIR_LEFT:
		corba_direction = MateComponent_Gtk_DirectionLeft;
		break;

	case GTK_DIR_RIGHT:
		corba_direction = MateComponent_Gtk_DirectionRight;
		break;

	default:
		g_assert_not_reached ();
		return FALSE;
	}

	CORBA_exception_init (&ev);

	result = MateComponent_Control_focus (priv->control, corba_direction, &ev);
	if (MATECOMPONENT_EX (&ev)) {
		g_message ("matecomponent_control_frame_focus(): Exception while issuing focus "
			   "request: `%s'", matecomponent_exception_get_text (&ev));
		result = FALSE;
	}

	CORBA_exception_free (&ev);

	return result;
}


void
matecomponent_control_frame_set_socket (MateComponentControlFrame *frame,
				 MateComponentSocket       *socket)
{
	MateComponentSocket *old_socket;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame));

	if ((MateComponentSocket *) frame->priv->socket == socket)
		return;

	old_socket = (MateComponentSocket *) frame->priv->socket;

	if (socket)
		frame->priv->socket = g_object_ref (socket);
	else
		frame->priv->socket = NULL;

	if (old_socket) {
		matecomponent_socket_set_control_frame (
			MATECOMPONENT_SOCKET (old_socket), NULL);
		g_object_unref (old_socket);
	}

	if (socket)
		matecomponent_socket_set_control_frame (socket, frame);
}

MateComponentSocket *
matecomponent_control_frame_get_socket (MateComponentControlFrame *frame)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (frame), NULL);

	return (MateComponentSocket *) frame->priv->socket;
}

MateComponentUIComponent *
matecomponent_control_frame_get_popup_component (MateComponentControlFrame *control_frame,
					  CORBA_Environment  *opt_ev)
{
	CORBA_Environment *ev, tmp_ev;
	MateComponentUIComponent *ui_component;
	MateComponent_UIContainer popup_container;

	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL_FRAME (control_frame), NULL);

	if (control_frame->priv->control == CORBA_OBJECT_NIL)
		return NULL;

	ui_component = matecomponent_ui_component_new_default ();

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	popup_container = MateComponent_Control_getPopupContainer (
		control_frame->priv->control, ev);

	if (MATECOMPONENT_EX (ev))
		return NULL;

	matecomponent_ui_component_set_container (ui_component, popup_container, ev);

	MateComponent_Unknown_unref (popup_container, ev);

	if (ev->_major != CORBA_NO_EXCEPTION) {
		matecomponent_object_unref (MATECOMPONENT_OBJECT (ui_component));
		ui_component = NULL;
	}

	if (!opt_ev) {
		CORBA_exception_free (ev);
	}

	return ui_component;
}
