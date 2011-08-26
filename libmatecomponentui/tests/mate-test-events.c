/*
 * test-events.c: A test application to sort X events issues.
 *
 * Author:
 *	Mark McLoughlin <mark@skynet.ie>
 *
 * Copyright 2001 Sun Microsytems, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include "config.h"
#include <string.h>

#include <libmatecomponentui.h>

static gboolean
event_cb (GtkWidget *widget, GdkEventButton *event)
{
	switch (event->type) {
	case GDK_BUTTON_PRESS:
		g_message (_("button press event %d"), event->button);
		break;
	case GDK_BUTTON_RELEASE:
		g_message (_("button release event %d"), event->button);
		break;
	default:
		g_message (_("event type: %d"), event->type);
		break;
	}

	return FALSE;
}

static int
exit_cb (GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
	return FALSE;
}

int
main (int argc, char **argv)
{
	MateProgram *program;
	GtkWidget *window;
	GtkWidget *control;
	gchar     *iid;

	if (argc != 2 || strncmp (argv [1], "OAFIID", 6))
		g_error (_("usage: mate-test-events <oaf-iid>"));

	iid = argv [1];

	textdomain (GETTEXT_PACKAGE);

	program = mate_program_init ("mate-test-events", VERSION,
			    LIBMATECOMPONENTUI_MODULE,
			    argc, argv, NULL);

	matecomponent_activate ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Events test");

	g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (exit_cb), NULL);
	g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (event_cb), NULL);
	g_signal_connect (G_OBJECT (window), "button-release-event", G_CALLBACK (event_cb), NULL);

	control = matecomponent_widget_new_control (iid, NULL);
	if (!control)
		g_error (_("Cannot get control widget for '%s'"), iid);

	gtk_container_add (GTK_CONTAINER (window), control);

	gtk_widget_show_all (window);

	gtk_main ();

	g_object_unref (program);

	return matecomponent_debug_shutdown ();
}
