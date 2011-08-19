/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-widget.c: MateComponentWidget object.
 *
 * Authors:
 *   Nat Friedman    (nat@ximian.com)
 *
 * Copyright 1999, 2001 Ximian, Inc.
 * 
 * MateComponent component embedding for hydrocephalic imbeciles.
 *
 * Pure cane sugar.
 *
 * This purpose of MateComponentWidget is to make container-side use of
 * MateComponent as easy as pie.  This widget has one function:
 *
 *      Provide a simple wrapper for embedding Controls.  Embedding
 *      controls is already really easy, but MateComponentWidget reduces
 *      the work from about 5 lines to 1.  To embed a given control,
 *      just do:
 *
 *        bw = matecomponent_widget_new_control ("moniker for control");
 *        gtk_container_add (some_container, bw);
 *
 *      Ta da!
 *
 *      NB. A simple moniker might look like 'file:/tmp/a.jpeg' or
 *      OAFIID:MATE_Evolution_Calendar_Control
 */

#include <config.h>
#include <gtk/gtk.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-widget.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker-util.h>

G_DEFINE_TYPE (MateComponentWidget, matecomponent_widget, GTK_TYPE_BIN)

struct _MateComponentWidgetPrivate {
	/* Control stuff. */
	MateComponentControlFrame *frame;
};

static MateComponent_Unknown
matecomponent_widget_launch_component (const char        *moniker,
				const char        *if_name,
				CORBA_Environment *ev)
{
	MateComponent_Unknown component;

	component = matecomponent_get_object (moniker, if_name, ev);

	if (MATECOMPONENT_EX (ev))
		component = CORBA_OBJECT_NIL;

	if (component == CORBA_OBJECT_NIL)
		return NULL;

	return component;
}


/*
 *
 * Control support for MateComponentWidget.
 *
 */
/**
 * matecomponent_widget_construct_control_from_objref:
 * @bw: A MateComponentWidget to construct
 * @control: A CORBA Object reference to an IDL:MateComponent/Control:1.0
 * @uic: MateComponent_UIContainer for the launched object or CORBA_OBJECT_NIL
 * if there is no need of menu / toolbar merging.
 * @ev: a CORBA exception environment
 *
 * This is a constructor function.  Only usable for wrapping and
 * derivation of new objects.  For normal use, please refer to
 * #matecomponent_widget_new_control_from_objref.
 *
 * Returns: A #MateComponentWidget (the @bw)
 */
MateComponentWidget *
matecomponent_widget_construct_control_from_objref (MateComponentWidget      *bw,
					     MateComponent_Control     control,
					     MateComponent_UIContainer uic,
					     CORBA_Environment *ev)
{
	GtkWidget *frame_widget;

	/* Create a local ControlFrame for it. */
	bw->priv->frame = matecomponent_control_frame_new (uic);

	matecomponent_control_frame_bind_to_control (
		bw->priv->frame, control, ev);

	/* Grab the actual widget which visually contains the remote
	 * Control.  This is a GtkSocket, in reality. */
	frame_widget = matecomponent_control_frame_get_widget (bw->priv->frame);

	/* Now stick it into this MateComponentWidget. */
	gtk_container_add (GTK_CONTAINER (bw), frame_widget);
	gtk_widget_show (frame_widget);

	return bw;
}

/**
 * matecomponent_widget_construct_control:
 * @bw: A MateComponentWidget to construct
 * @moniker: A Moniker describing the object to be activated 
 * @uic: MateComponent_UIContainer for the launched object or CORBA_OBJECT_NIL
 * if there is no need of menu / toolbar merging.
 * @ev: a CORBA exception environment
 *
 * This is a constructor function.  Only usable for wrapping and
 * derivation of new objects.  For normal use, please refer to
 * #matecomponent_widget_new_control.
 *
 * This function will unref the passed in @bw in case it cannot launch
 * the component and return %NULL in such a case.  Otherwise it returns
 * the @bw itself.
 *
 * Returns: A #MateComponentWidget or %NULL
 */
MateComponentWidget *
matecomponent_widget_construct_control (MateComponentWidget      *bw,
				 const char        *moniker,
				 MateComponent_UIContainer uic,
				 CORBA_Environment *ev)
{
	MateComponentWidget  *widget;
	MateComponent_Control control;

	/* Create the remote Control object. */
	control = matecomponent_widget_launch_component (
		moniker, "IDL:MateComponent/Control:1.0", ev);
	if (MATECOMPONENT_EX (ev) || control == CORBA_OBJECT_NIL) {
		/* Kill it (it is a floating object) */
		g_object_ref_sink (bw);
		return NULL;
	}

	widget = matecomponent_widget_construct_control_from_objref (
		bw, control, uic, ev);

	matecomponent_object_release_unref (control, ev);

	return widget;
}

typedef struct {
	MateComponentWidget       *bw;
	MateComponentWidgetAsyncFn fn;
	gpointer            user_data;
	MateComponent_UIContainer  uic;
} async_closure_t;

static void
control_new_async_cb (MateComponent_Unknown     object,
		      CORBA_Environment *ev,
		      gpointer           user_data)
{
	async_closure_t *c = user_data;

	if (MATECOMPONENT_EX (ev) || object == CORBA_OBJECT_NIL)
		c->fn (NULL, ev, c->user_data);
	else {
		matecomponent_widget_construct_control_from_objref (
			c->bw, object, c->uic, ev);
		c->fn (c->bw, ev, c->user_data);
	}

	g_object_unref (c->bw);
	matecomponent_object_release_unref (c->uic, ev);
	matecomponent_object_release_unref (object, ev);
	g_free (c);
}

/**
 * matecomponent_widget_new_control_async:
 * @moniker: A Moniker describing the object to be activated 
 * @uic: MateComponent_UIContainer for the launched object or CORBA_OBJECT_NIL
 * if there is no need of menu / toolbar merging.
 * @fn: a callback function called when the activation has happend
 * @user_data: user data to be passed back to the callback.
 * 
 * This method creates a widget, returns it to the user, and
 * asynchronously activates a control to insert into the widget.
 * 
 * Return value: a (temporarily) empty Widget to be filled with the
 * control later
 **/
GtkWidget *
matecomponent_widget_new_control_async (const char         *moniker,
				 MateComponent_UIContainer  uic,
				 MateComponentWidgetAsyncFn fn,
				 gpointer            user_data)
{
	MateComponentWidget     *bw;
	async_closure_t  *c = g_new0 (async_closure_t, 1);
	CORBA_Environment ev;

	g_return_val_if_fail (fn != NULL, NULL);
	g_return_val_if_fail (moniker != NULL, NULL);

	bw = g_object_new (MATECOMPONENT_TYPE_WIDGET, NULL);

	CORBA_exception_init (&ev);

	c->bw = g_object_ref (bw);
	c->fn = fn;
	c->user_data = user_data;
	c->uic = matecomponent_object_dup_ref (uic, &ev);

	matecomponent_get_object_async (
		moniker, "IDL:MateComponent/Control:1.0", &ev,
		control_new_async_cb, c);

	if (MATECOMPONENT_EX (&ev)) {
		control_new_async_cb (CORBA_OBJECT_NIL, &ev, c);
		gtk_widget_destroy (GTK_WIDGET (bw));
		bw = NULL;
	}

	CORBA_exception_free (&ev);

	return (GtkWidget *) bw;
}

/**
 * matecomponent_widget_new_control_from_objref:
 * @control: A CORBA Object reference to an IDL:MateComponent/Control:1.0
 * @uic: MateComponent_UIContainer for the launched object or CORBA_OBJECT_NIL
 * if there is no need of menu / toolbar merging.
 *
 * This function is a simple wrapper for easily embedding controls
 * into applications.  This function is used when you have already
 * a CORBA object reference to an IDL:MateComponent/Control:1.0 (the
 * @control) argument. 
 *
 * Returns: the @control wrapped as a #GtkWidget.
 */
GtkWidget *
matecomponent_widget_new_control_from_objref (MateComponent_Control     control,
				       MateComponent_UIContainer uic)
{
	MateComponentWidget     *bw;
	CORBA_Environment ev;

	g_return_val_if_fail (control != CORBA_OBJECT_NIL, NULL);

	CORBA_exception_init (&ev);

	bw = g_object_new (MATECOMPONENT_TYPE_WIDGET, NULL);

	bw = matecomponent_widget_construct_control_from_objref (bw, control, uic, &ev);

	if (MATECOMPONENT_EX (&ev))
		bw = NULL;

	CORBA_exception_free (&ev);

	return (GtkWidget *) bw;
}

/**
 * matecomponent_widget_new_control:
 * @moniker: A Moniker describing the object to be activated 
 * @uic: MateComponent_UIContainer for the launched object or CORBA_OBJECT_NIL
 * if there is no need of menu / toolbar merging.
 *
 * This function is a simple wrapper for easily embedding controls
 * into applications.  It will launch the component identified by @id
 * and will return it as a GtkWidget.
 *
 * Returns: A #GtkWidget that is bound to the MateComponent Control. 
 */
GtkWidget *
matecomponent_widget_new_control (const char        *moniker,
			   MateComponent_UIContainer uic)
{
	MateComponentWidget *bw;
	CORBA_Environment ev;

	g_return_val_if_fail (moniker != NULL, NULL);

	CORBA_exception_init (&ev);

	bw = g_object_new (MATECOMPONENT_TYPE_WIDGET, NULL);

	bw = matecomponent_widget_construct_control (bw, moniker, uic, &ev);

	if (MATECOMPONENT_EX (&ev)) {
		char *txt;
		g_warning ("Activation exception '%s'",
			   (txt = matecomponent_exception_get_text (&ev)));
		g_free (txt);
		bw = NULL;
	}

	CORBA_exception_free (&ev);

	return (GtkWidget *) bw;
}

/**
 * matecomponent_widget_get_control_frame:
 * @matecomponent_widget: a MateComponent Widget returned by one of the matecomponent_widget_new() functions.
 *
 * Every IDL:MateComponent/Control:1.0 needs to be placed inside an
 * IDL:MateComponent/ControlFrame:1.0.  This returns the MateComponentControlFrame
 * object that wraps the Control in the @matecomponent_widget. 
 * 
 * Returns: The MateComponentControlFrame associated with the @matecomponent_widget
 */
MateComponentControlFrame *
matecomponent_widget_get_control_frame (MateComponentWidget *matecomponent_widget)
{
	g_return_val_if_fail (MATECOMPONENT_IS_WIDGET (matecomponent_widget), NULL);

	return matecomponent_widget->priv->frame;
}

/**
 * matecomponent_widget_get_ui_container:
 * @matecomponent_widget: the #MateComponentWidget to query.
 *
 * Returns: the CORBA object reference to the MateComponent_UIContainer
 * associated with the @matecomponent_widget.
 */
MateComponent_UIContainer
matecomponent_widget_get_ui_container (MateComponentWidget *matecomponent_widget)
{
	g_return_val_if_fail (MATECOMPONENT_IS_WIDGET (matecomponent_widget), NULL);

	if (!matecomponent_widget->priv->frame)
		return CORBA_OBJECT_NIL;

	return matecomponent_control_frame_get_ui_container (
		matecomponent_widget->priv->frame);
}

MateComponent_Unknown
matecomponent_widget_get_objref (MateComponentWidget *matecomponent_widget)
{
	g_return_val_if_fail (MATECOMPONENT_IS_WIDGET (matecomponent_widget), NULL);

	if (!matecomponent_widget->priv->frame)
		return CORBA_OBJECT_NIL;
	else
		return matecomponent_control_frame_get_control (matecomponent_widget->priv->frame);
}

static void
matecomponent_widget_dispose (GObject *object)
{
	MateComponentWidget *bw = MATECOMPONENT_WIDGET (object);
	MateComponentWidgetPrivate *priv = bw->priv;
	
	if (priv->frame) {
		matecomponent_object_unref (MATECOMPONENT_OBJECT (priv->frame));
		priv->frame = NULL;
	}

	G_OBJECT_CLASS (matecomponent_widget_parent_class)->dispose (object);
}

static void
matecomponent_widget_finalize (GObject *object)
{
	MateComponentWidget *bw = MATECOMPONENT_WIDGET (object);
	
	g_free (bw->priv);

	G_OBJECT_CLASS (matecomponent_widget_parent_class)->finalize (object);
}

static void
matecomponent_widget_size_request (GtkWidget *widget,
			    GtkRequisition *requisition)
{
	GtkBin *bin;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATECOMPONENT_IS_WIDGET (widget));
	g_return_if_fail (requisition != NULL);

	bin = GTK_BIN (widget);

	if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
		GtkRequisition child_requisition;
      
		gtk_widget_size_request (bin->child, &child_requisition);

		requisition->width = child_requisition.width;
		requisition->height = child_requisition.height;
	}
}

static void
matecomponent_widget_size_allocate (GtkWidget *widget,
			     GtkAllocation *allocation)
{
	GtkBin *bin;
	GtkAllocation child_allocation;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATECOMPONENT_IS_WIDGET (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;
	bin = GTK_BIN (widget);

	child_allocation.x = allocation->x;
	child_allocation.y = allocation->y;
	child_allocation.width = allocation->width;
	child_allocation.height = allocation->height;

	if (bin->child)
		gtk_widget_size_allocate (bin->child, &child_allocation);
}

static void
matecomponent_widget_remove (GtkContainer *container,
		      GtkWidget    *widget)
{
	MateComponentWidget *bw = (MateComponentWidget *) container;

	bw->priv->frame = NULL;

	GTK_CONTAINER_CLASS (matecomponent_widget_parent_class)->remove (container, widget);
}

static void
matecomponent_widget_class_init (MateComponentWidgetClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
	GtkContainerClass *container_class = (GtkContainerClass *) klass;

	container_class->remove = matecomponent_widget_remove;

	widget_class->size_request = matecomponent_widget_size_request;
	widget_class->size_allocate = matecomponent_widget_size_allocate;

	object_class->finalize = matecomponent_widget_finalize;
	object_class->dispose  = matecomponent_widget_dispose;
}

static void
matecomponent_widget_init (MateComponentWidget *bw)
{
	bw->priv = g_new0 (MateComponentWidgetPrivate, 1);
}

/**
 * matecomponent_widget_set_property:
 * @control: A #MateComponentWidget that represents an IDL:MateComponent/Control:1.0
 * @first_prop: first property name to set.
 *
 * This is a utility function used to set a number of properties
 * in the MateComponent Control in @control.
 *
 * This function takes a variable list of arguments that must be NULL
 * terminated.  Arguments come in tuples: a string (for the argument
 * name) and the data type that is to be transfered.  The
 * implementation of the actual setting of the PropertyBag values is
 * done by the matecomponent_property_bag_client_setv() function).
 *
 * This only works for MateComponentWidgets that represent controls (ie,
 * that were returned by matecomponent_widget_new_control_from_objref() or
 * matecomponent_widget_new_control().
 */
void
matecomponent_widget_set_property (MateComponentWidget      *control,
			    const char        *first_prop, ...)
{
	MateComponent_PropertyBag pb;
	CORBA_Environment  ev;

	va_list args;
	va_start (args, first_prop);

	g_return_if_fail (control != NULL);
	g_return_if_fail (first_prop != NULL);
	g_return_if_fail (control->priv != NULL);
	g_return_if_fail (MATECOMPONENT_IS_WIDGET (control));

	CORBA_exception_init (&ev);
	
	pb = matecomponent_control_frame_get_control_property_bag (
		control->priv->frame, &ev);

	if (MATECOMPONENT_EX (&ev))
		g_warning ("Error getting property bag from control");
	else {
		/* FIXME: this should use ev */
		char *err = matecomponent_property_bag_client_setv (pb, &ev, first_prop, args);

		if (err)
			g_warning ("Error '%s'", err);
	}

	matecomponent_object_release_unref (pb, &ev);

	CORBA_exception_free (&ev);

	va_end (args);
}


/**
 * matecomponent_widget_get_property:
 * @control: A #MateComponentWidget that represents an IDL:MateComponent/Control:1.0
 * @first_prop: first property name to set.
 *
 * This is a utility function used to get a number of properties
 * in the MateComponent Control in @control.
 *
 * This function takes a variable list of arguments that must be NULL
 * terminated.  Arguments come in tuples: a string (for the argument
 * name) and a pointer where the data will be stored.  The
 * implementation of the actual setting of the PropertyBag values is
 * done by the matecomponent_property_bag_client_setv() function).
 *
 * This only works for MateComponentWidgets that represent controls (ie,
 * that were returned by matecomponent_widget_new_control_from_objref() or
 * matecomponent_widget_new_control().
 */
void
matecomponent_widget_get_property (MateComponentWidget      *control,
			    const char        *first_prop, ...)
{
	MateComponent_PropertyBag pb;
	CORBA_Environment  ev;

	va_list args;
	va_start (args, first_prop);

	g_return_if_fail (control != NULL);
	g_return_if_fail (first_prop != NULL);
	g_return_if_fail (control->priv != NULL);
	g_return_if_fail (MATECOMPONENT_IS_WIDGET (control));

	CORBA_exception_init (&ev);
	
	pb = matecomponent_control_frame_get_control_property_bag (
		control->priv->frame, &ev);

	if (MATECOMPONENT_EX (&ev))
		g_warning ("Error getting property bag from control");
	else {
		/* FIXME: this should use ev */
		char *err = matecomponent_property_bag_client_getv (pb, &ev, first_prop, args);

		if (err)
			g_warning ("Error '%s'", err);
	}

	matecomponent_object_release_unref (pb, &ev);

	CORBA_exception_free (&ev);

	va_end (args);
}
