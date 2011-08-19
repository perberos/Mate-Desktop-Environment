/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <libmatecomponent.h>

static gboolean
timeout (gpointer data)
{
	printf ("timeout\n");
	matecomponent_main_quit ();
	return FALSE;
}


static gboolean
run_tests (gpointer data)
{
	printf ("main loop level = %u\n", matecomponent_main_level ());
	g_timeout_add (1000, timeout, NULL);
	return FALSE;
}

int
main (int argc, char **argv)
{
	g_thread_init (NULL);
	if (!matecomponent_init (&argc, argv))
		g_error ("Cannot init matecomponent");
	g_idle_add (run_tests, NULL);
	matecomponent_main ();
	return matecomponent_debug_shutdown ();
}
