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
#include <glib/gi18n.h>
#include <libmatecomponentui.h>
#include <matecomponent/matecomponent-control-internal.h>

typedef struct {
	/* Control */
	GtkWidget          *control_widget;
	MateComponentControl      *control;
	MateComponentPlug         *plug;
	/* Frame */
	GtkWidget          *matecomponent_widget;
	MateComponentControlFrame *frame;
	MateComponentSocket       *socket;
} Test;

typedef enum {
	DESTROY_CONTROL,
	DESTROY_TOPLEVEL,
	DESTROY_CONTAINED,
	DESTROY_SOCKET,
	DESTROY_TYPE_LAST
} DestroyType;

static void
destroy_test (Test *test, DestroyType type)
{
	switch (type) {
	case DESTROY_CONTAINED: {
		/* Highly non-useful, should never happen */
		MateComponentControlFrame *frame;

		gtk_widget_destroy (test->control_widget);

		g_assert (test->control_widget == NULL);

		frame = matecomponent_widget_get_control_frame (
			MATECOMPONENT_WIDGET (test->matecomponent_widget));
		g_assert (MATECOMPONENT_IS_CONTROL_FRAME (frame));
		break;
	}

	case DESTROY_SOCKET:
		g_warning ("unimpl");
		/* drop through */

	case DESTROY_CONTROL:
		matecomponent_object_unref (test->control);
		g_assert (test->plug == NULL);
		g_assert (test->control_widget == NULL);
		/* drop through */

	case DESTROY_TOPLEVEL:
		gtk_widget_destroy (test->matecomponent_widget);
		g_assert (test->matecomponent_widget == NULL);
		g_assert (test->socket == NULL);
		g_assert (test->frame == NULL);
		g_assert (test->plug == NULL);
		g_assert (test->control == NULL);
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	g_assert (test->control_widget == NULL);

	switch (type) {
	case DESTROY_CONTAINED:
		gtk_widget_destroy (test->matecomponent_widget);
		break;
	default:
		break;
	}

	g_assert (test->matecomponent_widget == NULL);
}

static void
destroy_cb (GObject *object, Test *text)
{
	dbgprintf ("destroy %s %p\n",
		 g_type_name_from_instance (
			 (GTypeInstance *) object), object);
}

static void
create_control (Test *test)
{
	test->control_widget = gtk_entry_new ();
	g_signal_connect (G_OBJECT (test->control_widget),
			  "destroy", G_CALLBACK (destroy_cb),
			  test);

	g_assert (test->control_widget != NULL);
	gtk_widget_show (test->control_widget);

	test->control = matecomponent_control_new (test->control_widget);
	g_assert (test->control != NULL);

	test->plug = matecomponent_control_get_plug (test->control);
	g_assert (test->plug != NULL);

	g_signal_connect (G_OBJECT (test->plug),
			  "destroy", G_CALLBACK (destroy_cb),
			  test);

	g_object_add_weak_pointer (G_OBJECT (test->plug),
				   (gpointer *) &test->plug);
	g_object_add_weak_pointer (G_OBJECT (test->control),
				   (gpointer *) &test->control);
	g_object_add_weak_pointer (G_OBJECT (test->control_widget),
				   (gpointer *) &test->control_widget);
}

/* An ugly hack into the ORB */
extern CORBA_Object MateCORBA_objref_get_proxy (CORBA_Object obj);

static void
create_frame (Test *test, gboolean fake_remote)
{
	MateComponent_Control control;

	control = MATECOMPONENT_OBJREF (test->control);
/*	if (fake_remote)
	control = MateCORBA_objref_get_proxy (control);*/

	test->matecomponent_widget = matecomponent_widget_new_control_from_objref (
		control, CORBA_OBJECT_NIL);
	matecomponent_object_unref (MATECOMPONENT_OBJECT (test->control));
	gtk_widget_show (test->matecomponent_widget);
	test->frame = matecomponent_widget_get_control_frame (
		MATECOMPONENT_WIDGET (test->matecomponent_widget));
	test->socket = matecomponent_control_frame_get_socket (test->frame);

	g_object_add_weak_pointer (G_OBJECT (test->frame),
				   (gpointer *) &test->frame);
	g_object_add_weak_pointer (G_OBJECT (test->socket),
				   (gpointer *) &test->socket);
	g_object_add_weak_pointer (G_OBJECT (test->matecomponent_widget),
				   (gpointer *) &test->matecomponent_widget);
}

static Test *
create_test (gboolean fake_remote)
{
	Test *test = g_new0 (Test, 1);

	create_control (test);

	create_frame (test, fake_remote);

	return test;
}

static int
timeout_cb (gpointer user_data)
{
	gboolean *done = user_data;

	*done = TRUE;

	return FALSE;
}

static void
mainloop_for (gulong interval)
{
	gboolean mainloop_done = FALSE;

	if (!interval) /* Wait for another process */
		interval = 50;

	g_timeout_add (interval, timeout_cb, &mainloop_done);

	while (g_main_context_pending (NULL))
		g_main_context_iteration (NULL, FALSE);

	while (g_main_context_iteration (NULL, TRUE) && !mainloop_done)
		;
}

#if 0
static void
realize_cb (GtkWidget *socket, gpointer user_data)
{
	GtkWidget *plug, *w;

	g_warning ("Realize");

	plug = gtk_plug_new (0);
	w = gtk_button_new_with_label ("Baa");
	gtk_widget_show_all (w);
	gtk_widget_show (plug);
	gtk_container_add (GTK_CONTAINER (plug), w);
	GTK_PLUG (plug)->socket_window = GTK_WIDGET (socket)->window;
	gtk_socket_add_id (GTK_SOCKET (socket),
			   gtk_plug_get_id (GTK_PLUG (plug)));
	gdk_window_show (GTK_WIDGET (plug)->window);
}
#endif

static void
run_tests (GtkContainer *parent,
	   gboolean      wait_for_realize,
	   gboolean      fake_remote)
{
	GtkWidget  *vbox;
	DestroyType t;
	Test       *tests[DESTROY_TYPE_LAST];

	vbox = gtk_vbox_new (TRUE, 2);
	gtk_widget_show (vbox);
	gtk_container_add (parent, vbox);


#if 0
	{ /* Test raw plug / socket */
		GtkWidget *socket;

		g_warning ("Foo Bar. !!!");

		socket = gtk_socket_new ();
		g_signal_connect (G_OBJECT (socket), "realize",
				  G_CALLBACK (realize_cb), NULL);
		gtk_widget_show (GTK_WIDGET (socket));
		gtk_box_pack_start (GTK_BOX (vbox), socket, TRUE, TRUE, 2);
	}
#endif

	printf ("create\n");
	for (t = 0; t < DESTROY_TYPE_LAST; t++) {

		tests [t] = create_test (fake_remote);

		gtk_box_pack_start (
			GTK_BOX (vbox),
			GTK_WIDGET (tests [t]->matecomponent_widget),
			TRUE, TRUE, 2);
	}

	if (wait_for_realize)
		mainloop_for (100);

	printf ("show / hide\n");
	gtk_widget_hide (GTK_WIDGET (parent));

	if (wait_for_realize)
		mainloop_for (100);

	gtk_widget_show (GTK_WIDGET (parent));

	if (wait_for_realize)
		mainloop_for (100);

	printf ("re-add\n");
	g_object_ref (G_OBJECT (vbox));
	gtk_container_remove (parent, vbox);
	gtk_container_add (parent, vbox);

	if (wait_for_realize)
		mainloop_for (100);

	printf ("re-parent\n");
	for (t = 0; t < DESTROY_TYPE_LAST; t++) {

		g_object_ref (tests [t]->matecomponent_widget);
		gtk_container_remove (GTK_CONTAINER (GTK_BOX (vbox)),
				      tests [t]->matecomponent_widget);

		gtk_box_pack_start (
			GTK_BOX (vbox),
			GTK_WIDGET (tests [t]->matecomponent_widget),
			TRUE, TRUE, 2);

		g_object_unref (tests [t]->matecomponent_widget);
	}

	if (wait_for_realize)
		mainloop_for (100);

	printf ("destroy\n");
	for (t = 0; t < DESTROY_TYPE_LAST; t++) {
		destroy_test (tests [t], t);
		mainloop_for (0);
	}

	gtk_widget_destroy (GTK_WIDGET (vbox));
}

static void
simple_tests (void)
{
	MateComponentControl *control;
	GtkWidget     *widget = gtk_button_new_with_label ("Foo");

	control = matecomponent_control_new (widget);
	matecomponent_object_unref (MATECOMPONENT_OBJECT (control));

	control = matecomponent_control_new (gtk_entry_new ());
	matecomponent_object_unref (MATECOMPONENT_OBJECT (control));
}

static void
test_gtk_weakrefs (void)
{
	gpointer   ref;
	GtkObject *object = g_object_new (GTK_TYPE_BUTTON, NULL);

	ref = object;
	g_object_ref_sink (object);
	g_object_add_weak_pointer (ref, &ref);
	gtk_object_destroy (object);
	g_object_unref (object);

	g_assert (ref == NULL);
}

int
main (int argc, char **argv)
{
	GtkWidget *window;

	free (malloc (8));

	textdomain (GETTEXT_PACKAGE);

	if (!matecomponent_ui_init ("mate-test-focus", VERSION, &argc, argv))
		g_error ("Can not matecomponent_ui_init");

	matecomponent_activate ();

	test_gtk_weakrefs ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Control test");

	gtk_widget_show_all (window);

	simple_tests ();

	run_tests (GTK_CONTAINER (window), TRUE, TRUE);
	run_tests (GTK_CONTAINER (window), FALSE, TRUE);

	gtk_widget_destroy (window);

	return matecomponent_ui_debug_shutdown ();
}
