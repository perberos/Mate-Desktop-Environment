/*
 * Test to check if removing controls is borked
 *
 * Authors:
 *    Iain Holmes   <iain@ximian.com>
 *    Michael Meeks <michael@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <matecomponent.h>

static gboolean do_remote = TRUE;
static gboolean do_local  = TRUE;

static GtkWidget *window, *vbox, *button, *placeholder1, *placeholder2;
static GtkWidget *remote_widget = NULL, *inproc_widget = NULL;

#define TEST_OAFIID "OAFIID:MateComponent_Sample_Entry"

static gboolean
do_add (gpointer data)
{
	if (do_remote)
		gtk_container_add (GTK_CONTAINER (placeholder1), remote_widget);
	if (do_local)
		gtk_container_add (GTK_CONTAINER (placeholder2), inproc_widget);
	g_print ("Added..............\n");

	return FALSE;
}

static gboolean
do_remove (gpointer data)
{
	if (do_remote) {
		g_object_ref (remote_widget);
		gtk_container_remove (GTK_CONTAINER (placeholder1), remote_widget);
	}
	if (do_local) {
		g_object_ref (inproc_widget);
		gtk_container_remove (GTK_CONTAINER (placeholder2), inproc_widget);
	}
	g_print ("Removed............\n");
	g_timeout_add (100, do_add, NULL);

	return FALSE;
}

static void
remove_and_add (GtkWidget *click, gpointer useless_user_data)
{
	g_timeout_add (100, do_remove, NULL);
}

static void
make_remote_widget (void)
{
	g_assert (remote_widget == NULL);

	remote_widget = matecomponent_widget_new_control (TEST_OAFIID, CORBA_OBJECT_NIL);
	gtk_widget_show (remote_widget);
	gtk_container_add (GTK_CONTAINER (placeholder1), remote_widget);
}

static void
make_inproc_widget (void)
{
	MateComponentControl *control;
	GtkWidget     *entry;

	g_assert (inproc_widget == NULL);

	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), "In-proc");
	gtk_widget_show (entry);

	control = matecomponent_control_new (gtk_entry_new ());
	matecomponent_object_unref (MATECOMPONENT_OBJECT (control));

	control = matecomponent_control_new (entry);
	inproc_widget = matecomponent_widget_new_control_from_objref (
		MATECOMPONENT_OBJREF (control), CORBA_OBJECT_NIL);
	matecomponent_object_unref (MATECOMPONENT_OBJECT (control));

	gtk_widget_show (inproc_widget);
	gtk_container_add (GTK_CONTAINER (placeholder2), inproc_widget);
}


static gboolean
idle_init (gpointer data)
{
	if (do_remote)
		make_remote_widget ();
	if (do_local)
		make_inproc_widget ();

	return FALSE;
}

static gboolean
quit_cb (GtkWindow *window, GdkEvent *event, gpointer dummy)
{
	if (do_local)
		gtk_widget_destroy (inproc_widget);

	matecomponent_main_quit ();

	return TRUE;
}

int
main (int argc, char **argv)
{
	MateProgram *program;

	program = mate_program_init ("mate-test-reparent", VERSION,
			    LIBMATECOMPONENTUI_MODULE,
			    argc, argv, NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	g_signal_connect (G_OBJECT (window), "delete_event",
			  G_CALLBACK (quit_cb), NULL);

	placeholder1 = gtk_frame_new ("Out of proc");
	gtk_box_pack_start (GTK_BOX (vbox), placeholder1, TRUE, TRUE, 0);

	placeholder2 = gtk_frame_new ("In proc");
	gtk_box_pack_start (GTK_BOX (vbox), placeholder2, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Remove and add");
	gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (remove_and_add), NULL);

	gtk_widget_show_all (window);
	g_timeout_add (0, idle_init, NULL);
	matecomponent_main ();

	gtk_widget_destroy (window);

	g_object_unref (program);

	return matecomponent_ui_debug_shutdown ();
}
