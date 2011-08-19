/*
 * test-focus.c: A test application to sort focus issues.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2001 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include "config.h"

#include <stdlib.h>
#include <libmatecomponentui.h>

#include <matecomponent/matecomponent-widget.h>
#include <matecomponent/matecomponent-ui-main.h>
#include <glib/gi18n.h>

static void
clicked_fn (GtkButton *button, GtkWidget *control)
{
	g_signal_handlers_disconnect_matched (
		button, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, control);

	gtk_widget_destroy (control);
	gtk_widget_destroy (GTK_WIDGET (button));
}

static int
exit_cb (GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
	return FALSE;
}

static void
add_control (GtkBox *box)
{
	GtkWidget *tmp, *control;

#if 1
	control = matecomponent_widget_new_control ("OAFIID:MateComponent_Sample_Entry", NULL);
#else
	control = matecomponent_widget_new_control ("OAFIID:Nautilus_Text_View", NULL);
#endif

	g_assert (control != NULL);
	gtk_box_pack_start_defaults (box, control);
	gtk_widget_show (control);

	tmp = gtk_button_new_with_label ("Destroy remote control");
	g_signal_connect (tmp, "clicked",
			  G_CALLBACK (clicked_fn), control);
	gtk_box_pack_start_defaults (box, tmp);
	gtk_widget_show (tmp);
}


static void
add_fn (GtkButton *button, GtkBox *box)
{
	add_control (box);
}

int
main (int argc, char **argv)
{
	GtkWidget *tmp;
	GtkWidget *window;
	GtkWidget *vbox;
	MateProgram *program;

	free (malloc (8));

	textdomain (GETTEXT_PACKAGE);

	program = mate_program_init ("test-focus", VERSION,
			    LIBMATECOMPONENTUI_MODULE,
			    argc, argv, NULL);

	matecomponent_activate ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Focus test");
	g_signal_connect (GTK_OBJECT (window),
			    "delete_event",
			    G_CALLBACK (exit_cb), NULL);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	tmp = gtk_entry_new ();
	gtk_box_pack_start_defaults (GTK_BOX (vbox), tmp);

	tmp = gtk_button_new_with_label ("In Container A");
	gtk_box_pack_start_defaults (GTK_BOX (vbox), tmp);

	add_control (GTK_BOX (vbox));

	tmp = gtk_button_new_with_label ("add control");
	g_signal_connect (tmp, "clicked",
			  G_CALLBACK (add_fn), vbox);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), tmp);

	gtk_widget_show_all (window);

	gtk_main ();

	g_object_unref (program);

	return matecomponent_ui_debug_shutdown ();
}
