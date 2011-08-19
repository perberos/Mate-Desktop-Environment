/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <libmatecomponentui.h>

int
main (int argc, char *argv[])
{
	MateProgram *program;
	gchar *text;

	program = mate_program_init ("matecomponent-selector", VERSION,
			    LIBMATECOMPONENTUI_MODULE,
			    argc, argv, NULL);

	text = matecomponent_selector_select_id (_("Select an object"), NULL);
	g_print ("OAFIID: '%s'\n", text ? text : "<Null>");

	g_free (text);

	g_object_unref (program);
	
	return matecomponent_ui_debug_shutdown ();
}
