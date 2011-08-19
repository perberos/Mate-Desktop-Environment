/*
 * Sample user for the Echo MateComponent component
 *
 * Author:
 *   Miguel de Icaza  (miguel@helixcode.com)
 */

#include <config.h>

#include <glib/gi18n.h>
#include <libmatecomponent.h>
#include "MateComponent_Sample_Echo.h"

int 
main (int argc, char *argv [])
{
	MateComponent_Sample_Echo echo_server;
	CORBA_Environment  ev;

	/*
	 * Initialize matecomponent.
	 */
	if (!matecomponent_init (&argc, argv))
		g_error (_("I could not initialize MateComponent"));
	
	/*
	 * Enable CORBA/MateComponent to start processing requests
	 */
	matecomponent_activate ();

	echo_server = matecomponent_get_object ("OAFIID:MateComponent_Sample_Echo",
					 "MateComponent/Sample/Echo", NULL);

	if (echo_server == CORBA_OBJECT_NIL) {
		g_warning (_("Could not create an instance of the sample echo component"));
		return matecomponent_debug_shutdown ();
	}

	/* Send a message */
	CORBA_exception_init (&ev);

	MateComponent_Sample_Echo_echo (echo_server, "This is the message from the client\n", &ev);

	/* Check for exceptions */
	if (MATECOMPONENT_EX (&ev)) {
		char *err = matecomponent_exception_get_text (&ev);
		g_warning (_("An exception occurred '%s'"), err);
		g_free (err);
	}

	CORBA_exception_free (&ev);

	matecomponent_object_release_unref (echo_server, NULL);
	
	return matecomponent_debug_shutdown ();
}
