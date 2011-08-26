/*
 * Sensitivity showing hack, ugly code
 *  -George
 *
 * Used to be:
 *
 * sample-control-container.c
 *
 * Authors:
 *   Nat Friedman  (nat@ximian.com)
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 1999, 2001 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <stdlib.h>

#include <glib/gi18n.h>

#include <libmatecomponentui.h>

static void
app_destroy_cb (GtkWidget *app, MateComponentUIContainer *uic)
{
	gtk_main_quit ();
}

static void
toggle_sensitive (GtkWidget *w, GtkWidget *control)
{
	if (GTK_WIDGET_SENSITIVE (control))
		gtk_widget_set_sensitive (control, FALSE);
	else
		gtk_widget_set_sensitive (control, TRUE);
}

static GtkWidget *
make_inprocess_control (MateComponentUIContainer *uic)
{
	MateComponentControl *control;
	GtkWidget *controlw;
	GtkWidget *w = gtk_label_new (">>> this is a control <<<");
	gtk_widget_show (w);

	control = matecomponent_control_new (w);

	controlw = matecomponent_widget_new_control_from_objref
		(MATECOMPONENT_OBJREF (control), MATECOMPONENT_OBJREF (uic));

	matecomponent_object_unref (MATECOMPONENT_OBJECT (control));

	return controlw;
}


static guint
container_create (void)
{
	GtkWidget       *control;
	GtkWidget       *box;
	MateComponentUIContainer *uic;
	GtkWindow       *window;
	GtkWidget       *app;

	app = matecomponent_window_new ("sample-control-container",
				 "Sample MateComponent Control Container");

	window = GTK_WINDOW (app);

	uic = matecomponent_window_get_ui_container (MATECOMPONENT_WINDOW (app));

	gtk_window_set_default_size (window, 500, 440);
	gtk_window_set_resizable (window, TRUE);

	g_signal_connect (window, "destroy",
			  G_CALLBACK (app_destroy_cb), uic);

	box = gtk_vbox_new (FALSE, 5);
	matecomponent_window_set_contents (MATECOMPONENT_WINDOW (app), box);

	control = matecomponent_widget_new_control ("OAFIID:MateComponent_Sample_Clock",
					     MATECOMPONENT_OBJREF (uic));

	if (control) {
		GtkWidget *w;

		gtk_widget_set_sensitive (GTK_WIDGET (control), FALSE);

		gtk_box_pack_start (GTK_BOX (box),
				    gtk_label_new ("Initially insensitive:"),
				    FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);
		w = gtk_button_new_with_label ("toggle_sensitivity");
		gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (w), "clicked",
				    G_CALLBACK (toggle_sensitive),
				    control);
	}

	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);

	control = matecomponent_widget_new_control ("OAFIID:MateComponent_Sample_Clock",
					     MATECOMPONENT_OBJREF (uic));

	if (control) {
		GtkWidget *w;

		gtk_box_pack_start (GTK_BOX (box),
				    gtk_label_new ("Initially sensitive:"),
				    FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);
		w = gtk_button_new_with_label ("toggle_sensitivity");
		gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (w), "clicked",
				    G_CALLBACK (toggle_sensitive),
				    control);
	}

	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);


	control = make_inprocess_control (uic);

	if (control) {
		GtkWidget *w;

		gtk_widget_set_sensitive (GTK_WIDGET (control), FALSE);

		gtk_box_pack_start (GTK_BOX (box),
				    gtk_label_new ("inprocess initially insensitive:"),
				    FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);
		w = gtk_button_new_with_label ("toggle_sensitivity");
		gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (w), "clicked",
				    G_CALLBACK (toggle_sensitive),
				    control);
	}

	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box),
			    gtk_hseparator_new (),
			    FALSE, FALSE, 0);

	control = make_inprocess_control (uic);

	if (control) {
		GtkWidget *w;

		gtk_box_pack_start (GTK_BOX (box),
				    gtk_label_new ("inprocess initially sensitive:"),
				    FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);
		w = gtk_button_new_with_label ("toggle_sensitivity");
		gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
		g_signal_connect (GTK_OBJECT (w), "clicked",
				    G_CALLBACK (toggle_sensitive),
				    control);
	}

	gtk_widget_show_all (GTK_WIDGET (window));

	return FALSE;
}

int
main (int argc, char **argv)
{
	CORBA_Environment ev;
	MateProgram *program;

	CORBA_exception_init (&ev);

	/* Encorage -lefence to play ball */
	{ char *tmp = malloc (4); if (tmp) free (tmp); }

	program = mate_program_init ("mate-test-sensitivity", VERSION,
			    LIBMATECOMPONENTUI_MODULE,
			    argc, argv, NULL);

	/*
	 * We can't make any CORBA calls unless we're in the main
	 * loop.  So we delay creating the container here.
	 */
	g_idle_add ((GSourceFunc) container_create, NULL);

	matecomponent_main ();

	g_object_unref (program);

	return matecomponent_ui_debug_shutdown ();
}
