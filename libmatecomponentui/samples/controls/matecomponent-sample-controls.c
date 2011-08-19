/*
 * matecomponent-clock-control.c
 *
 * Author:
 *    Michael Meeks (michael@ximian.com)
 *
 * Copyright 2001, Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <string.h>
#include <libmatecomponentui.h>
#include <libmatecanvas/libmatecanvas.h>
#undef USE_SCROLLED

static void
activate_cb (GtkEditable *editable, MateComponentControl *control)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (
		NULL, 0, GTK_MESSAGE_INFO,
		GTK_BUTTONS_OK,
		"This dialog demonstrates transient dialogs");

	matecomponent_control_set_transient_for (
		control, GTK_WINDOW (dialog), NULL);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

MateComponentObject *
matecomponent_entry_control_new (void)
{
	MateComponentPropertyBag  *pb;
	MateComponentControl      *control;
	GtkWidget	   *entry;
	GParamSpec        **pspecs;
	guint               n_pspecs;
	GtkWidget          *box;
	int i;

	
	/* Create the control. */

	box = gtk_vbox_new (FALSE, 0);
	for (i = 0; i < 3; i++) {
		entry = gtk_entry_new ();
		gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
		gtk_widget_show (entry);
	}

	gtk_widget_show (box);

#ifdef USE_SCROLLED
	{
		GtkWidget *canvas, *scrolled;
		MateCanvasItem *item;
		
		canvas = mate_canvas_new ();
		gtk_widget_show (canvas);
		
		item = mate_canvas_item_new (
			mate_canvas_root (MATE_CANVAS (canvas)),
			MATE_TYPE_CANVAS_WIDGET,
			"x", 0.0, "y", 0.0, "width", 100.0,
			"height", 100.0, "widget", box, NULL);
		mate_canvas_item_show (item);

		scrolled = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (
			GTK_SCROLLED_WINDOW (scrolled),
			GTK_POLICY_AUTOMATIC,
			GTK_POLICY_AUTOMATIC);

		gtk_container_add (
			GTK_CONTAINER (scrolled), canvas);
		gtk_widget_show (scrolled);

		control = matecomponent_control_new (scrolled);
	}
#else
	control = matecomponent_control_new (box);
#endif
	pb = matecomponent_property_bag_new (NULL, NULL, NULL);
	matecomponent_control_set_properties (control, MATECOMPONENT_OBJREF (pb), NULL);
	matecomponent_object_unref (MATECOMPONENT_OBJECT (pb));

	g_signal_connect (
		GTK_OBJECT (entry), "activate",
		G_CALLBACK (activate_cb), control);

	pspecs = g_object_class_list_properties (
		G_OBJECT_GET_CLASS (entry), &n_pspecs);

	matecomponent_property_bag_map_params (
		pb, G_OBJECT (entry), (const GParamSpec **)pspecs, n_pspecs);

	g_free (pspecs);

	matecomponent_control_life_instrument (control);

	return MATECOMPONENT_OBJECT (control);
}

static MateComponentObject *
control_factory (MateComponentGenericFactory *this,
		 const char           *object_id,
		 void                 *data)
{
	MateComponentObject *object = NULL;
	
	g_return_val_if_fail (object_id != NULL, NULL);

	if (!strcmp (object_id, "OAFIID:MateComponent_Sample_Entry"))

		object = matecomponent_entry_control_new ();

	return object;
}

int
main (int argc, char *argv [])
{
	int retval;

	if (!matecomponent_ui_init ("matecomponent-sample-controls-2",
			     VERSION, &argc, argv))
		g_error (_("Could not initialize MateComponent UI"));

	retval = matecomponent_generic_factory_main
		("OAFIID:MateComponent_Sample_ControlFactory",
		 control_factory, NULL);

	return retval;
}                                                                             
