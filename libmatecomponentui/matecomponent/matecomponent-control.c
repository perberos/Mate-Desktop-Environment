/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * MateComponent control object
 *
 * Authors:
 *   Michael Meeks     (michael@ximian.com)
 *   Nat Friedman      (nat@ximian.com)
 *   Miguel de Icaza   (miguel@ximian.com)
 *   Maciej Stachowiak (mjs@eazel.com)
 *
 * Copyright 1999, 2001 Ximian, Inc.
 *                 2000 Eazel, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gdkconfig.h>
#include <gtk/gtk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#define interface _win32_interface
#include <gdk/gdkwin32.h>
#undef interface
#elif defined (GDK_WINDOWING_QUARTZ)
#else
#error Port to this GDK backend
#endif

#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-plug.h>
#include <matecomponent/matecomponent-control.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-ui-sync-menu.h>
#include <matecomponent/matecomponent-control-internal.h>
#include <matecomponent/matecomponent-property-bag-client.h>

enum {
	PLUG_CREATED,
	DISCONNECTED,
	SET_FRAME,
	ACTIVATE,
	LAST_SIGNAL
};

static guint control_signals [LAST_SIGNAL];

static GObjectClass *matecomponent_control_parent_class = NULL;

struct _MateComponentControlPrivate {
	MateComponent_ControlFrame  frame;
	MateComponentControlFrame  *inproc_frame;
	MateComponentUIComponent   *ui_component;
	MateComponent_PropertyBag   propbag;
	MateComponentUIContainer   *popup_ui_container;
	MateComponentUIComponent   *popup_ui_component;
	MateComponentUIEngine      *popup_ui_engine;
	MateComponentUISync        *popup_ui_sync;

	GtkWidget           *plug;
	GtkWidget           *widget;

	guint                no_frame_timeout_id;

	guint                active : 1;
	guint                automerge : 1;
	guint                sent_disconnect : 1;
};

static void
matecomponent_control_disconnected (MateComponentControl *control)
{
	g_return_if_fail (control != NULL);
	g_return_if_fail (control->priv != NULL);

	if (!control->priv->sent_disconnect) {
		control->priv->sent_disconnect = TRUE;
		g_signal_emit (control, control_signals [DISCONNECTED], 0);
	}
}

static void
control_frame_connection_died_cb (gpointer connection,
				  gpointer user_data)
{
	MateComponentControl *control = MATECOMPONENT_CONTROL (user_data);

	g_return_if_fail (control != NULL);

	matecomponent_control_disconnected (control);

	dbgprintf ("The remote control frame died unexpectedly");
	matecomponent_object_unref (MATECOMPONENT_OBJECT (control));
}

void
matecomponent_control_add_listener (CORBA_Object        object,
			     GCallback           fn,
			     gpointer            user_data,
			     CORBA_Environment  *ev)
{
	MateCORBAConnectionStatus status;

	if (object == CORBA_OBJECT_NIL)
		return;
	
	status = MateCORBA_small_listen_for_broken (
		object, fn, user_data);
	
	switch (status) {
	case MATECORBA_CONNECTION_CONNECTED:
		break;
	default:
		dbgprintf ("premature CORBA_Object death");
		matecomponent_exception_general_error_set (
			ev, NULL, "Control died prematurely");
		break;
	}
}

/**
 * matecomponent_control_window_id_from_x11:
 * @x11_id: the x11 window id or Windows HWND.
 * 
 * Return value: the window id or handle as a string; free after use.
 **/
MateComponent_Gdk_WindowId
matecomponent_control_window_id_from_x11 (guint32 x11_id)
{
	gchar str[32];

	snprintf (str, 31, "%u", x11_id);
	str[31] = '\0';

/*	printf ("Mangled %d to '%s'\n", x11_id, str);*/

	return CORBA_string_dup (str);
}

/**
 * matecomponent_control_x11_from_window_id:
 * @id: CORBA_char *
 * 
 * De-mangle a window id string,
 * fields are separated by ':' character,
 * currently only the first field is used.
 * 
 * Return value: the native window id.
 **/
guint32
matecomponent_control_x11_from_window_id (const CORBA_char *id)
{
	guint32 x11_id;
	char **elements;
	
/*	printf ("ID string '%s'\n", id);*/

	elements = g_strsplit (id, ":", -1);
	if (elements && elements [0])
		x11_id = strtol (elements [0], NULL, 10);
	else {
#if defined (GDK_WINDOWING_X11)
		g_warning ("Serious X id mangling error");
#elif defined (GDK_WINDOWING_WIN32)
		g_warning ("Serious window handle mangling error");
#endif
		x11_id = 0;
	}
	g_strfreev (elements);

/*	printf ("x11 : %d\n", x11_id);*/

	return x11_id;
}

static void
matecomponent_control_auto_merge (MateComponentControl *control)
{
	MateComponent_UIContainer remote_container;

	if (control->priv->ui_component == NULL)
		return;

	/* 
	 * this makes a CORBA call, so re-entrancy can occur here
	 */
	remote_container = matecomponent_control_get_remote_ui_container (control, NULL);
	if (remote_container == CORBA_OBJECT_NIL)
		return;

	/*
	 * we could have been re-entereted in the previous call, so
	 * make sure we are still active
	 */
	if (control->priv->active)
		matecomponent_ui_component_set_container (
			control->priv->ui_component, remote_container, NULL);

	matecomponent_object_release_unref (remote_container, NULL);
}


static void
matecomponent_control_auto_unmerge (MateComponentControl *control)
{
	if (control->priv->ui_component == NULL)
		return;
	
	matecomponent_ui_component_unset_container (control->priv->ui_component, NULL);
}

static void
impl_MateComponent_Control_activate (PortableServer_Servant servant,
			      CORBA_boolean activated,
			      CORBA_Environment *ev)
{
	MateComponentControl *control = MATECOMPONENT_CONTROL (matecomponent_object_from_servant (servant));
	gboolean old_activated;

	if (activated == control->priv->active)
		return;
	
	/* 
	 * store the old activated value as we can be re-entered
	 * during (un)merge
	 */
	old_activated = control->priv->active;
	control->priv->active = activated;

	if (control->priv->automerge) {
		if (activated)
			matecomponent_control_auto_merge (control);
		else
			matecomponent_control_auto_unmerge (control);
	}

	/* 
	 * if our active state is not what we are changing it to, then
	 * don't emit the signal
	 */
	if (control->priv->active == old_activated)
		return;

	g_signal_emit (control, control_signals [ACTIVATE], 0, control->priv->active);
	matecomponent_control_activate_notify (control, control->priv->active, ev);
}

static void
matecomponent_control_unset_control_frame (MateComponentControl     *control,
				    CORBA_Environment *opt_ev)
{
	CORBA_Environment *ev, tmp_ev;

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	if (control->priv->no_frame_timeout_id != 0) {
		g_source_remove (control->priv->no_frame_timeout_id);
		control->priv->no_frame_timeout_id = 0;
	}

	if (control->priv->frame != CORBA_OBJECT_NIL) {
		MateComponent_ControlFrame frame = control->priv->frame;

		control->priv->frame = CORBA_OBJECT_NIL;

		MateCORBA_small_unlisten_for_broken (
			frame, G_CALLBACK (control_frame_connection_died_cb));

		if (control->priv->active)
			MateComponent_ControlFrame_notifyActivated (
				frame, FALSE, ev);

		CORBA_Object_release (frame, ev);
	}

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

static void
create_plug (MateComponentControl *control)
{
	GtkWidget *plug;
	
	plug = matecomponent_plug_new (0);

	g_object_ref_sink (plug);
	
	matecomponent_control_set_plug (control, MATECOMPONENT_PLUG (plug));

	if (control->priv->widget)
		gtk_container_add (GTK_CONTAINER (plug),
				   control->priv->widget);

	g_signal_emit (control, control_signals [PLUG_CREATED], 0);

	g_object_unref (plug);
}

static int
parse_cookie (const CORBA_char *cookie)
{
	GString    *ident = NULL;
	GString    *value = NULL;
	const char *p;
	char       *screen = NULL;
	int         retval = -1;

	for (p = cookie; *p && !screen; p++) {
		switch (*p) {
		case ',':
			if (!ident || !value)
				goto parse_failed;

			if (strcmp (ident->str, "screen")) {
				g_string_free (ident, TRUE); ident = NULL;
				g_string_free (value, TRUE); value = NULL;
				break;
			}

			screen = value->str;
			break;
		case '=':
			if (!ident || value)
				goto parse_failed;
			value = g_string_new (NULL);
			break;
		default:
			if (!ident)
				ident = g_string_new (NULL);
			
			if (value)
				g_string_append_c (value, *p);
			else
				g_string_append_c (ident, *p);
			break;
		}
	}

	if (ident && value && !strcmp (ident->str, "screen"))
		screen = value->str;

	if (screen)
		retval = atoi (screen);

parse_failed:
	if (ident) g_string_free (ident, TRUE);
	if (value) g_string_free (value, TRUE);

	return retval;
}

static CORBA_char *
impl_MateComponent_Control_getWindowId (PortableServer_Servant servant,
				 const CORBA_char      *cookie,
				 CORBA_Environment     *ev)
{
	guint32        x11_id;
	MateComponentControl *control = MATECOMPONENT_CONTROL (
		matecomponent_object_from_servant (servant));
	GdkScreen *gdkscreen;
	int        screen_num;

	if (!control->priv->plug)
		create_plug (control);

	g_assert (control->priv->plug != NULL);

	screen_num = parse_cookie (cookie);
	if (screen_num != -1)
		gdkscreen = gdk_display_get_screen (
				gdk_display_get_default (), screen_num);
	else
		gdkscreen = gdk_screen_get_default ();

	gtk_window_set_screen (GTK_WINDOW (control->priv->plug), gdkscreen);

	gtk_widget_show (control->priv->plug);

	x11_id = gtk_plug_get_id (GTK_PLUG (control->priv->plug));
		
	dbgprintf ("plug id %u\n", x11_id);

	return matecomponent_control_window_id_from_x11 (x11_id);
}

static MateComponent_UIContainer
impl_MateComponent_Control_getPopupContainer (PortableServer_Servant servant,
				       CORBA_Environment     *ev)
{
	MateComponentUIContainer *container;
	
	container = matecomponent_control_get_popup_ui_container (
		MATECOMPONENT_CONTROL (matecomponent_object_from_servant (servant)));

	return matecomponent_object_dup_ref (MATECOMPONENT_OBJREF (container), ev);
}
	
static void
impl_MateComponent_Control_setFrame (PortableServer_Servant servant,
			      MateComponent_ControlFrame    frame,
			      CORBA_Environment     *ev)
{
	MateComponentControl *control = MATECOMPONENT_CONTROL (
		matecomponent_object_from_servant (servant));

	g_object_ref (control);

	if (control->priv->frame != frame) {
		matecomponent_control_unset_control_frame (control, ev);
	
		if (frame == CORBA_OBJECT_NIL)
			control->priv->frame = CORBA_OBJECT_NIL;
		else {
			control->priv->frame = CORBA_Object_duplicate (
				frame, NULL);
		}
	
		control->priv->inproc_frame = (MateComponentControlFrame *)
			matecomponent_object (MateCORBA_small_get_servant (frame));

		if (!control->priv->inproc_frame)
			matecomponent_control_add_listener (
				frame,
				G_CALLBACK (control_frame_connection_died_cb),
				control, ev);
	
		g_signal_emit (control, control_signals [SET_FRAME], 0);
	}

	g_object_unref (control);
}

static void
impl_MateComponent_Control_setSize (PortableServer_Servant  servant,
			     const CORBA_short       width,
			     const CORBA_short       height,
			     CORBA_Environment      *ev)
{
	GtkAllocation  size;
	MateComponentControl *control = MATECOMPONENT_CONTROL (
		matecomponent_object_from_servant (servant));
	
	size.x = size.y = 0;
	size.width = width;
	size.height = height;

	/*
	 * In the Mate implementation of MateComponent, all size assignment
	 * is handled by GtkPlug/GtkSocket for us.
	 */

	g_warning ("setSize untested");

	gtk_widget_size_allocate (GTK_WIDGET (control->priv->plug), &size);
}

static MateComponent_Gtk_Requisition
impl_MateComponent_Control_getDesiredSize (PortableServer_Servant servant,
				    CORBA_Environment     *ev)
{
	MateComponentControl         *control;
	GtkRequisition         requisition;
	MateComponent_Gtk_Requisition req;

	control = MATECOMPONENT_CONTROL (matecomponent_object (servant));

	gtk_widget_size_request (control->priv->widget, &requisition);

	req.width  = requisition.width;
	req.height = requisition.height;

	return req;
}

static GtkStateType
matecomponent_control_gtk_state_from_corba (const MateComponent_Gtk_State state)
{
	switch (state) {
	case MateComponent_Gtk_StateNormal:
		return GTK_STATE_NORMAL;

	case MateComponent_Gtk_StateActive:
		return GTK_STATE_ACTIVE;

	case MateComponent_Gtk_StatePrelight:
		return GTK_STATE_PRELIGHT;

	case MateComponent_Gtk_StateSelected:
		return GTK_STATE_SELECTED;

	case MateComponent_Gtk_StateInsensitive:
		return GTK_STATE_INSENSITIVE;

	default:
		g_warning ("matecomponent_control_gtk_state_from_corba: Unknown state: %d", (gint) state);
		return GTK_STATE_NORMAL;
	}
}

static void
impl_MateComponent_Control_setState (PortableServer_Servant  servant,
			       const MateComponent_Gtk_State state,
			       CORBA_Environment     *ev)
{
	MateComponentControl *control = MATECOMPONENT_CONTROL (matecomponent_object_from_servant (servant));
	GtkStateType   gtk_state = matecomponent_control_gtk_state_from_corba (state);

	g_return_if_fail (control->priv->widget != NULL);

	if (gtk_state == GTK_STATE_INSENSITIVE)
		gtk_widget_set_sensitive (control->priv->widget, FALSE);
	else {
		if (! GTK_WIDGET_SENSITIVE (control->priv->widget))
			gtk_widget_set_sensitive (control->priv->widget, TRUE);

		gtk_widget_set_state (control->priv->widget,
				      gtk_state);
	}
}

static MateComponent_PropertyBag
impl_MateComponent_Control_getProperties (PortableServer_Servant  servant,
				   CORBA_Environment      *ev)
{
	MateComponentControl *control = MATECOMPONENT_CONTROL (matecomponent_object_from_servant (servant));
	if (control->priv->propbag == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;

	return matecomponent_object_dup_ref (control->priv->propbag, ev);
}

static CORBA_boolean
impl_MateComponent_Control_focus (PortableServer_Servant servant,
			   MateComponent_Gtk_Direction   corba_direction,
			   CORBA_Environment     *ev)
{
	MateComponentControl        *control;
	GtkDirectionType      direction;
	MateComponentControlPrivate *priv;

	control = MATECOMPONENT_CONTROL (matecomponent_object (servant));
	priv = control->priv;

	/* FIXME: this will not work for local controls. */

	if (!priv->plug)
		return FALSE;

	switch (corba_direction) {
	case MateComponent_Gtk_DirectionTabForward:
		direction = GTK_DIR_TAB_FORWARD;
		break;

	case MateComponent_Gtk_DirectionTabBackward:
		direction = GTK_DIR_TAB_BACKWARD;
		break;

	case MateComponent_Gtk_DirectionUp:
		direction = GTK_DIR_UP;
		break;

	case MateComponent_Gtk_DirectionDown:
		direction = GTK_DIR_DOWN;
		break;

	case MateComponent_Gtk_DirectionLeft:
		direction = GTK_DIR_LEFT;
		break;

	case MateComponent_Gtk_DirectionRight:
		direction = GTK_DIR_RIGHT;
		break;

	default:
		/* Hmmm, we should throw an exception. */
		return FALSE;
	}

	return gtk_widget_child_focus (GTK_WIDGET (priv->plug), direction);
}

#define DEFAULT_CONTROL_PURGE_DELAY_MS 60 * 1000 /* 60 seconds */

static int control_purge_delay = DEFAULT_CONTROL_PURGE_DELAY_MS;

/**
 * matecomponent_control_life_set_purge:
 * @ms: time to wait in milliseconds.
 * 
 *   Set time we're prepared to wait without a ControlFrame
 * before terminating the Control. This can happen if the
 * panel activates us but crashes before the set_frame.
 **/
void
matecomponent_control_life_set_purge (long ms)
{
	control_purge_delay = ms;
}

static gboolean
never_got_frame_timeout (gpointer user_data)
{
	g_warning ("Never got frame, control died - abnormal exit condition");

	matecomponent_control_disconnected (user_data);
	
	return FALSE;
}

MateComponentControl *
matecomponent_control_construct (MateComponentControl  *control,
			  GtkWidget      *widget)
{
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), NULL);

#ifdef GDK_WINDOWING_X11
	/*
	 * This sets up the X handler for MateComponent objects.  We basically will
	 * ignore X errors if our container dies (because X will kill the
	 * windows of the container and our container without telling us).
	 */
	matecomponent_setup_x_error_handler ();
#endif

	/*
	 *   Start the clock ticking until we emit 'disconnected'
	 * since, we never got a valid frame.
	 */
	control->priv->no_frame_timeout_id = g_timeout_add (
		control_purge_delay,
		(GSourceFunc) never_got_frame_timeout,
		control);

	control->priv->widget = g_object_ref_sink (widget);

	gtk_container_add (GTK_CONTAINER (control->priv->plug),
			   control->priv->widget);

	control->priv->ui_component = NULL;
	control->priv->propbag = CORBA_OBJECT_NIL;

	return control;
}

/**
 * matecomponent_control_new:
 * @widget: a GTK widget that contains the control and will be passed to the
 * container process.
 *
 * This function creates a new MateComponentControl object for @widget.
 *
 * Returns: a MateComponentControl object that implements the MateComponent::Control CORBA
 * service that will transfer the @widget to the container process.
 */
MateComponentControl *
matecomponent_control_new (GtkWidget *widget)
{
	MateComponentControl *control;
	
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	control = g_object_new (matecomponent_control_get_type (), NULL);
	
	return matecomponent_control_construct (control, widget);
}

/**
 * matecomponent_control_get_widget:
 * @control: a MateComponentControl
 *
 * Returns the GtkWidget associated with a MateComponentControl.
 *
 * Return value: the MateComponentControl's widget
 **/
GtkWidget *
matecomponent_control_get_widget (MateComponentControl *control)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), NULL);

	return control->priv->widget;
}

/**
 * matecomponent_control_set_automerge:
 * @control: A #MateComponentControl.
 * @automerge: Whether or not menus and toolbars should be
 * automatically merged when the control is activated.
 *
 * Sets whether or not the control handles menu/toolbar merging
 * automatically.  If automerge is on, the control will automatically
 * register its MateComponentUIComponent with the remote MateComponentUIContainer
 * when it is activated.
 */
void
matecomponent_control_set_automerge (MateComponentControl *control,
			      gboolean       automerge)
{
	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));

	control->priv->automerge = automerge;

	if (automerge && !control->priv->ui_component)
		control->priv->ui_component = matecomponent_ui_component_new_default ();
}

/**
 * matecomponent_control_get_automerge:
 * @control: A #MateComponentControl.
 *
 * Returns: Whether or not the control is set to automerge its
 * menus/toolbars.  See matecomponent_control_set_automerge().
 */
gboolean
matecomponent_control_get_automerge (MateComponentControl *control)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), FALSE);

	return control->priv->automerge;
}

static void
matecomponent_control_destroy (MateComponentObject *object)
{
	MateComponentControl *control = (MateComponentControl *) object;

	dbgprintf ("matecomponent_control_destroy %p\n", object);

	if (control->priv->plug)
		matecomponent_control_set_plug (control, NULL);

	matecomponent_control_unset_control_frame (control, NULL);
	matecomponent_control_set_properties      (control, CORBA_OBJECT_NIL, NULL);
	matecomponent_control_set_ui_component    (control, NULL);
	matecomponent_control_disconnected (control);

	if (control->priv->widget) {
		gtk_widget_destroy (GTK_WIDGET (control->priv->widget));
		g_object_unref (control->priv->widget);
	}
	control->priv->widget = NULL;

	control->priv->popup_ui_container = matecomponent_object_unref (
		(MateComponentObject *) control->priv->popup_ui_container);

	if (control->priv->popup_ui_engine)
		g_object_unref (control->priv->popup_ui_engine);
	control->priv->popup_ui_engine = NULL;

	control->priv->popup_ui_component = matecomponent_object_unref (
		(MateComponentObject *) control->priv->popup_ui_component);

	control->priv->popup_ui_sync = NULL;
	control->priv->inproc_frame  = NULL;

	MATECOMPONENT_OBJECT_CLASS (matecomponent_control_parent_class)->destroy (object);
}

static void
matecomponent_control_finalize (GObject *object)
{
	MateComponentControl *control = MATECOMPONENT_CONTROL (object);

	dbgprintf ("matecomponent_control_finalize %p\n", object);

	g_free (control->priv);

	matecomponent_control_parent_class->finalize (object);
}

/**
 * matecomponent_control_get_control_frame:
 * @control: A MateComponentControl object whose MateComponent_ControlFrame CORBA interface is
 * being retrieved.
 * @opt_ev: an optional exception environment
 *
 * Returns: The MateComponent_ControlFrame CORBA object associated with @control, this is
 * a CORBA_Object_duplicated object.  You need to CORBA_Object_release it when you are
 * done with it.
 */
MateComponent_ControlFrame
matecomponent_control_get_control_frame (MateComponentControl     *control,
				  CORBA_Environment *opt_ev)
{
	MateComponent_ControlFrame frame;
	CORBA_Environment *ev, tmp_ev;
	
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), CORBA_OBJECT_NIL);

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;
	
	frame = CORBA_Object_duplicate (control->priv->frame, ev);
	
	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);

	return frame;
}

/**
 * matecomponent_control_get_ui_component:
 * @control: The control
 * 
 * Return value: the associated UI component
 **/
MateComponentUIComponent *
matecomponent_control_get_ui_component (MateComponentControl *control)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), NULL);

	if (!control->priv->ui_component)
		control->priv->ui_component = matecomponent_ui_component_new_default ();

	return control->priv->ui_component;
}

void
matecomponent_control_set_ui_component (MateComponentControl     *control,
				 MateComponentUIComponent *component)
{
	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));
	g_return_if_fail (component == NULL ||
			  MATECOMPONENT_IS_UI_COMPONENT (component));

	if (component == control->priv->ui_component)
		return;

	if (control->priv->ui_component) {
		matecomponent_ui_component_unset_container (control->priv->ui_component, NULL);
		matecomponent_object_unref (MATECOMPONENT_OBJECT (control->priv->ui_component));
	}

	control->priv->ui_component = matecomponent_object_ref ((MateComponentObject *) component);
}

/**
 * matecomponent_control_set_properties:
 * @control: A #MateComponentControl object.
 * @pb: A #MateComponent_PropertyBag.
 * @opt_ev: An optional exception environment
 *
 * Binds @pb to @control.  When a remote object queries @control
 * for its property bag, @pb will be used in the responses.
 */
void
matecomponent_control_set_properties (MateComponentControl      *control,
			       MateComponent_PropertyBag  pb,
			       CORBA_Environment  *opt_ev)
{
	MateComponent_PropertyBag old_bag;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));

	if (pb == control->priv->propbag)
		return;

	old_bag = control->priv->propbag;

	control->priv->propbag = matecomponent_object_dup_ref (pb, opt_ev);
	matecomponent_object_release_unref (old_bag, opt_ev);
}

/**
 * matecomponent_control_get_properties:
 * @control: A #MateComponentControl whose PropertyBag has already been set.
 *
 * Returns: The #MateComponent_PropertyBag bound to @control.
 */
MateComponent_PropertyBag 
matecomponent_control_get_properties (MateComponentControl *control)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), NULL);

	return control->priv->propbag;
}

/**
 * matecomponent_control_get_ambient_properties:
 * @control: A #MateComponentControl which is bound to a remote
 * #MateComponentControlFrame.
 * @opt_ev: an optional exception environment
 *
 * Returns: A #MateComponent_PropertyBag bound to the bag of ambient
 * properties associated with this #Control's #ControlFrame.
 */
MateComponent_PropertyBag
matecomponent_control_get_ambient_properties (MateComponentControl     *control,
				       CORBA_Environment *opt_ev)
{
	MateComponent_ControlFrame frame;
	MateComponent_PropertyBag pbag;
	CORBA_Environment *ev = NULL, tmp_ev;

	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), NULL);

	frame = control->priv->frame;

	if (frame == CORBA_OBJECT_NIL)
		return NULL;

	if (opt_ev)
		ev = opt_ev;
	else {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	}

	pbag = MateComponent_ControlFrame_getAmbientProperties (
		frame, ev);

	if (MATECOMPONENT_EX (ev)) {
		if (!opt_ev)
			CORBA_exception_free (&tmp_ev);
		pbag = CORBA_OBJECT_NIL;
	}

	return pbag;
}

/**
 * matecomponent_control_get_remote_ui_container:
 * @control: A MateComponentControl object which is associated with a remote
 * ControlFrame.
 * @opt_ev: an optional exception environment
 *
 * Returns: The MateComponent_UIContainer CORBA server for the remote MateComponentControlFrame.
 */
MateComponent_UIContainer
matecomponent_control_get_remote_ui_container (MateComponentControl     *control,
					CORBA_Environment *opt_ev)
{
	CORBA_Environment  tmp_ev, *ev;
	MateComponent_UIContainer ui_container;

	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), CORBA_OBJECT_NIL);

	g_return_val_if_fail (control->priv->frame != CORBA_OBJECT_NIL,
			      CORBA_OBJECT_NIL);

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	ui_container = MateComponent_ControlFrame_getUIContainer (control->priv->frame, ev);

	matecomponent_object_check_env (MATECOMPONENT_OBJECT (control), control->priv->frame, ev);

	if (MATECOMPONENT_EX (ev))
		ui_container = CORBA_OBJECT_NIL;

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);

	return ui_container;
}

/**
 * matecomponent_control_activate_notify:
 * @control: A #MateComponentControl object which is bound
 * to a remote ControlFrame.
 * @activated: Whether or not @control has been activated.
 * @opt_ev: An optional exception environment
 *
 * Notifies the remote ControlFrame which is associated with
 * @control that @control has been activated/deactivated.
 */
void
matecomponent_control_activate_notify (MateComponentControl     *control,
				gboolean           activated,
				CORBA_Environment *opt_ev)
{
	CORBA_Environment *ev, tmp_ev;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));
	g_return_if_fail (control->priv->frame != CORBA_OBJECT_NIL);

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	MateComponent_ControlFrame_notifyActivated (control->priv->frame, activated, ev);

	matecomponent_object_check_env (MATECOMPONENT_OBJECT (control), control->priv->frame, ev);

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

static void
matecomponent_control_class_init (MateComponentControlClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;
	MateComponentObjectClass *matecomponent_object_class = (MateComponentObjectClass *)klass;
	POA_MateComponent_Control__epv *epv;

	matecomponent_control_parent_class = g_type_class_peek_parent (klass);

	control_signals [PLUG_CREATED] =
                g_signal_new ("plug_created",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentControlClass, plug_created),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	control_signals [DISCONNECTED] =
                g_signal_new ("disconnected",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentControlClass, disconnected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	control_signals [SET_FRAME] =
                g_signal_new ("set_frame",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentControlClass, set_frame),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	control_signals [ACTIVATE] =
                g_signal_new ("activate",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentControlClass, activate),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);

	object_class->finalize = matecomponent_control_finalize;
	matecomponent_object_class->destroy = matecomponent_control_destroy;

	epv = &klass->epv;

	epv->getProperties     = impl_MateComponent_Control_getProperties;
	epv->getDesiredSize    = impl_MateComponent_Control_getDesiredSize;
	epv->getAccessible     = NULL;
	epv->getWindowId       = impl_MateComponent_Control_getWindowId;
	epv->getPopupContainer = impl_MateComponent_Control_getPopupContainer;
	epv->setFrame          = impl_MateComponent_Control_setFrame;
	epv->setSize           = impl_MateComponent_Control_setSize;
	epv->setState          = impl_MateComponent_Control_setState;
	epv->activate          = impl_MateComponent_Control_activate;
	epv->focus             = impl_MateComponent_Control_focus;
}

static void
matecomponent_control_init (MateComponentControl *control)
{
	control->priv = g_new0 (MateComponentControlPrivate, 1);

	control->priv->frame = CORBA_OBJECT_NIL;

	create_plug (control);
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentControl, 
		       MateComponent_Control,
		       MATECOMPONENT_OBJECT_TYPE,
		       matecomponent_control)

/*
 * Varrarg Property get/set simplification wrappers.
 */

/**
 * matecomponent_control_set_property:
 * @control: the control with associated property bag
 * @opt_ev: optional corba exception environment
 * @first_prop: the first property's name
 * 
 * This method takes a NULL terminated list of name, type, value
 * triplicates, and sets the corresponding values on the control's
 * associated property bag.
 **/
void
matecomponent_control_set_property (MateComponentControl     *control,
			     CORBA_Environment *opt_ev,
			     const char        *first_prop,
			     ...)
{
	MateComponent_PropertyBag  bag;
	char               *err;
	CORBA_Environment  *ev, tmp_ev;
	va_list             args;

	g_return_if_fail (first_prop != NULL);
	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));

	va_start (args, first_prop);

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;
		
	bag = control->priv->propbag;

	if ((err = matecomponent_property_bag_client_setv (bag, ev, first_prop, args)))
		g_warning ("Error '%s'", err);

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);

	va_end (args);
}

/**
 * matecomponent_control_get_property:
 * @control: the control with associated property bag
 * @opt_ev: optional corba exception environment
 * @first_prop: the first property's name
 * 
 * This method takes a NULL terminated list of name, type, value
 * triplicates, and fetches the corresponding values on the control's
 * associated property bag.
 **/
void
matecomponent_control_get_property (MateComponentControl     *control,
			     CORBA_Environment *opt_ev,
			     const char        *first_prop,
			     ...)
{
	MateComponent_PropertyBag  bag;
	char               *err;
	CORBA_Environment  *ev, tmp_ev;
	va_list             args;

	g_return_if_fail (first_prop != NULL);
	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));

	va_start (args, first_prop);

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	bag = control->priv->propbag;

	if ((err = matecomponent_property_bag_client_getv (bag, ev, first_prop, args)))
		g_warning ("Error '%s'", err);

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);

	va_end (args);
}

/*
 * Transient / Modality handling logic
 */

#undef TRANSIENT_DEBUG

static void
window_transient_realize_gdk_cb (GtkWidget *widget)
{
	GdkWindow *win;

	win = g_object_get_data (G_OBJECT (widget), "transient");
	g_return_if_fail (win != NULL);

#ifdef TRANSIENT_DEBUG
	g_warning ("Set transient");
#endif
	gdk_window_set_transient_for (
		GTK_WIDGET (widget)->window, win);
}

static void
window_transient_unrealize_gdk_cb (GtkWidget *widget)
{
	GdkWindow *win;

	win = g_object_get_data (G_OBJECT (widget), "transient");
	g_return_if_fail (win != NULL);

	gdk_property_delete (
		win, gdk_atom_intern ("WM_TRANSIENT_FOR", FALSE));
}


static void
window_transient_destroy_gdk_cb (GtkWidget *widget)
{
	GdkWindow *win;
	
	if ((win = g_object_get_data (G_OBJECT (widget), "transient")))
		g_object_unref (win);
}

static void       
window_set_transient_for_gdk (GtkWindow *window, 
			      GdkWindow *parent)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (g_object_get_data (
		G_OBJECT (window), "transient") == NULL);

	g_object_ref (parent);

	g_object_set_data (G_OBJECT (window), "transient", parent);

	if (GTK_WIDGET_REALIZED (window)) {
#ifdef TRANSIENT_DEBUG
		g_warning ("Set transient");
#endif
		gdk_window_set_transient_for (
			GTK_WIDGET (window)->window, parent);
	}

	/*
	 * FIXME: we need to be able to get the screen from
	 * the remote toplevel window for multi-head to work
	 * could use an ambient property on the ControlFrame
	 */

	g_signal_connect (
		window, "realize",
		G_CALLBACK (window_transient_realize_gdk_cb), NULL);

	g_signal_connect (
		window, "unrealize",
		G_CALLBACK (window_transient_unrealize_gdk_cb), NULL);
	
	g_signal_connect (
		window, "destroy",
		G_CALLBACK (window_transient_destroy_gdk_cb), NULL);
}

/**
 * matecomponent_control_set_transient_for:
 * @control: a control with associated control frame
 * @window: a window upon which to set the transient window.
 * 
 *   Attempts to make the @window transient for the toplevel
 * of any associated controlframe the MateComponentControl may have.
 **/
void
matecomponent_control_set_transient_for (MateComponentControl     *control,
				  GtkWindow         *window,
				  CORBA_Environment *opt_ev)
{
	CORBA_char         *id;
	GdkDisplay         *display;
	GdkWindow          *win;
	GdkNativeWindow    window_id;
	CORBA_Environment  *ev = NULL, tmp_ev;
	MateComponent_ControlFrame frame;
	gpointer            local_win;

	g_return_if_fail (GTK_IS_WINDOW (window));
	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));

	/* FIXME: special case the local case !
	 * we can only do this if set_transient is virtualized
	 * and thus we can catch it in MateComponentSocket and chain up
	 * again if we are embedded inside an embedded thing. */

	frame = control->priv->frame;

	if (frame == CORBA_OBJECT_NIL)
		return;

	if (opt_ev)
		ev = opt_ev;
	else {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	}

	id = MateComponent_ControlFrame_getToplevelId (frame, ev);
	g_return_if_fail (!MATECOMPONENT_EX (ev) && id != NULL);

	window_id = matecomponent_control_x11_from_window_id (id);

#ifdef TRANSIENT_DEBUG
	g_warning ("Got id '%s' -> %d", id, window_id);
#endif
	CORBA_free (id);

	display = gtk_widget_get_display (GTK_WIDGET (window));

#if defined (GDK_WINDOWING_X11)
	local_win = gdk_xid_table_lookup_for_display (display, window_id);
#elif defined (GDK_WINDOWING_WIN32)
	local_win = gdk_win32_handle_table_lookup (window_id);
#endif
	if (local_win == NULL)
		win = gdk_window_foreign_new_for_display (display, window_id);
	else {
		win = GDK_WINDOW (local_win);
		g_object_ref (win);
	}
	g_return_if_fail (win != NULL);

	window_set_transient_for_gdk (window, win);

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

/**
 * matecomponent_control_unset_transient_for:
 * @control: a control with associated control frame
 * @window: a window upon which to unset the transient window.
 * 
 **/
void
matecomponent_control_unset_transient_for (MateComponentControl     *control,
				    GtkWindow         *window,
				    CORBA_Environment *opt_ev)
{
	g_return_if_fail (GTK_IS_WINDOW (window));

	g_signal_handlers_disconnect_by_func (
		window, G_CALLBACK (window_transient_realize_gdk_cb), NULL);

	g_signal_handlers_disconnect_by_func (
		window, G_CALLBACK (window_transient_unrealize_gdk_cb), NULL);
	
	g_signal_handlers_disconnect_by_func (
		window, G_CALLBACK (window_transient_destroy_gdk_cb), NULL);

	window_transient_unrealize_gdk_cb (GTK_WIDGET (window));
}

void
matecomponent_control_set_plug (MateComponentControl *control,
			 MateComponentPlug    *plug)
{
	MateComponentPlug *old_plug;

	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));

	if ((MateComponentPlug *) control->priv->plug == plug)
		return;

	dbgprintf ("matecomponent_control_set_plug (%p, %p) [%p]\n",
		 control, plug, control->priv->plug);

	old_plug = (MateComponentPlug *) control->priv->plug;

	if (plug)
		control->priv->plug = g_object_ref (plug);
	else
		control->priv->plug = NULL;

	if (old_plug) {
		matecomponent_plug_set_control (old_plug, NULL);
		gtk_widget_destroy (GTK_WIDGET (old_plug));
		g_object_unref (old_plug);
	}

	if (plug)
		matecomponent_plug_set_control (plug, control);
}

/**
 * matecomponent_control_get_plug:
 * @control: the control.a
 * 
 * This methods returns the current plug associated with
 * the control. If the remote container re-parents the
 * control - the plug will die, and a new plug has to be
 * created. Thus this should really only be called from
 * a 'plug_created' signal handler.
 * 
 * Return value: the _current_ plug.
 **/
MateComponentPlug *
matecomponent_control_get_plug (MateComponentControl *control)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), NULL);

	return (MateComponentPlug *) control->priv->plug;
}

void
matecomponent_control_set_popup_ui_container (MateComponentControl     *control,
				       MateComponentUIContainer *ui_container)
{
	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));
	g_return_if_fail (MATECOMPONENT_IS_UI_CONTAINER (ui_container));

	g_assert (control->priv->popup_ui_container == NULL);

	control->priv->popup_ui_container = matecomponent_object_ref (
		MATECOMPONENT_OBJECT (ui_container));
}

MateComponentUIContainer *
matecomponent_control_get_popup_ui_container (MateComponentControl *control)
{
	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), NULL);

	if (!control->priv->popup_ui_container) {
		MateComponentUIEngine *ui_engine;
		MateComponentUISync   *ui_sync;

		ui_engine = matecomponent_ui_engine_new (G_OBJECT (control));

		ui_sync = matecomponent_ui_sync_menu_new (ui_engine, NULL, NULL, NULL);

		matecomponent_ui_engine_add_sync (ui_engine, ui_sync);

		/* re-entrancy guard */
		if (control->priv->popup_ui_container) {
			g_object_unref (ui_engine);

			return control->priv->popup_ui_container;
		}

		control->priv->popup_ui_engine = ui_engine;
		control->priv->popup_ui_sync   = ui_sync;

		control->priv->popup_ui_container = matecomponent_ui_container_new ();
		matecomponent_ui_container_set_engine (
			control->priv->popup_ui_container,
			control->priv->popup_ui_engine);
	}

	return control->priv->popup_ui_container;
}

MateComponentUIComponent *
matecomponent_control_get_popup_ui_component (MateComponentControl *control)
{
	MateComponentUIContainer *ui_container;

	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), NULL);

	if (!control->priv->popup_ui_component) {
		ui_container = matecomponent_control_get_popup_ui_container (control);

		control->priv->popup_ui_component = 
			matecomponent_ui_component_new_default ();

		matecomponent_ui_component_set_container (
			control->priv->popup_ui_component,
			MATECOMPONENT_OBJREF (ui_container), NULL);
	}

	return control->priv->popup_ui_component;
}

gboolean
matecomponent_control_do_popup_path (MateComponentControl       *control,
			      GtkWidget           *parent_menu_shell,
			      GtkWidget           *parent_menu_item,
			      GtkMenuPositionFunc  func,
			      gpointer             data,
			      guint                button,
			      const char          *popup_path,
			      guint32              activate_time)
{
	GtkWidget *menu;

	g_return_val_if_fail (MATECOMPONENT_IS_CONTROL (control), FALSE);

	if (!control->priv->popup_ui_container)
		return FALSE;

	menu = gtk_menu_new ();

	matecomponent_ui_sync_menu_add_popup (
		MATECOMPONENT_UI_SYNC_MENU (control->priv->popup_ui_sync),
		GTK_MENU (menu), popup_path);

	gtk_menu_set_screen (
		GTK_MENU (menu),
		gtk_window_get_screen (GTK_WINDOW (control->priv->plug)));

	gtk_widget_show (menu);

	gtk_menu_popup (GTK_MENU (menu),
			parent_menu_shell, parent_menu_item,
			func, data,
			button, activate_time);

	return TRUE;
}

gboolean
matecomponent_control_do_popup_full (MateComponentControl       *control,
			      GtkWidget           *parent_menu_shell,
			      GtkWidget           *parent_menu_item,
			      GtkMenuPositionFunc  func,
			      gpointer             data,
			      guint                button,
			      guint32              activate_time)
{
	char    *path;
	gboolean retval;

	path = g_strdup_printf ("/popups/button%u", button);

	retval = matecomponent_control_do_popup_path
		(control, parent_menu_shell, parent_menu_item,
		 func, data, button, path, activate_time);

	g_free (path);

	return retval;
}

gboolean
matecomponent_control_do_popup (MateComponentControl       *control,
			 guint                button,
			 guint32              activate_time)
{
	return matecomponent_control_do_popup_full (
		control, NULL, NULL, NULL, NULL,
		button, activate_time);
}

/* Track the living controls */
static GSList *live_controls = NULL;
static MateComponentControlLifeCallback life_callback =
	(MateComponentControlLifeCallback) matecomponent_main_quit;

/**
 * matecomponent_control_life_set_callback:
 * @all_dead_callback: method to call at idle when no controls remain
 * 
 * See #matecomponent_control_life_instrument
 **/
void
matecomponent_control_life_set_callback (MateComponentControlLifeCallback all_dead_callback)
{
	life_callback = all_dead_callback;
}

static gboolean
control_life_idle_cb (gpointer data)
{
	if (!live_controls && life_callback)
		life_callback ();

	return FALSE;
}

static void
control_life_disconnected (MateComponentControl *control)
{
	live_controls = g_slist_remove (live_controls, control);

	if (!live_controls)
		g_idle_add (control_life_idle_cb, NULL);
}

/**
 * matecomponent_control_life_instrument:
 * @control: control to manage.
 * 
 * Request that @control is lifecycle managed by this code;
 * when it (and all other registerees are dead, the
 * all_dead_callback set by #matecomponent_control_life_set_callback
 * will be called at idle.
 **/
void
matecomponent_control_life_instrument (MateComponentControl *control)
{
	g_return_if_fail (MATECOMPONENT_IS_CONTROL (control));

	g_signal_connect (control, "disconnected",
			  G_CALLBACK (control_life_disconnected),
			  NULL);

	live_controls = g_slist_prepend (live_controls, control);
}

/**
 * matecomponent_control_life_get_count:
 * @void: 
 * 
 * calculates the number of live controls under management.
 * 
 * Return value: the number of live controls.
 **/
int
matecomponent_control_life_get_count (void)
{
	return g_slist_length (live_controls);
}
