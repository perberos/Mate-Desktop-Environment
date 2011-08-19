/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-persist-client.c: Client-side utility functions dealing with persistancy
 *
 * Author:
 *   ÉRDI Gergõ <cactus@cactus.rulez.org>
 *
 * Copyright 2001 Gergõ Érdi
 */

#include <matecomponent/matecomponent-persist-client.h>

#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-stream-client.h>
#include <matecomponent/matecomponent-moniker-util.h>

void
matecomponent_object_save_to_stream (MateComponent_Unknown     object,
			      MateComponent_Stream      stream,
			      CORBA_Environment *opt_ev)
{
	char                           *iid = NULL;
	CORBA_Environment               my_ev;
	MateComponent_PersistStream            pstream = CORBA_OBJECT_NIL;

	CORBA_exception_init (&my_ev);
	pstream = MateComponent_Unknown_queryInterface (object, "IDL:MateComponent/PersistStream:1.0", &my_ev);
	CORBA_exception_free (&my_ev);

	if (!pstream) {
		matecomponent_exception_set (opt_ev, ex_MateComponent_Moniker_InterfaceNotFound);
		goto out;
	}

	CORBA_exception_init (&my_ev);
	iid = MateComponent_Persist_getIId (pstream, &my_ev);
	matecomponent_stream_client_write_string (stream, iid, TRUE, &my_ev);
	if (MATECOMPONENT_EX (&my_ev)) {
		if (opt_ev)
			matecomponent_exception_set (opt_ev, MATECOMPONENT_EX_REPOID (&my_ev));
		CORBA_exception_free (&my_ev);
		goto out;
	}

	if (opt_ev) {
		MateComponent_PersistStream_save (pstream, stream, "", opt_ev);
	} else {
		MateComponent_PersistStream_save (pstream, stream, "", opt_ev);
		CORBA_exception_free (&my_ev);
	}

 out:
	g_free (iid);
	if (pstream != CORBA_OBJECT_NIL) {
		CORBA_exception_init (&my_ev);
		MateComponent_Unknown_unref (pstream, &my_ev);
		CORBA_exception_free (&my_ev);
	}
}


MateComponent_Unknown
matecomponent_object_from_stream (MateComponent_Stream      stream,
			   CORBA_Environment *opt_ev)
{
	char                 *iid = NULL;
	CORBA_Environment     my_ev, *ev;
	MateComponent_PersistStream  pstream = CORBA_OBJECT_NIL;

	CORBA_exception_init (&my_ev);

	if (opt_ev)
		ev = opt_ev;
	else
		ev = &my_ev;

	matecomponent_stream_client_read_string (stream, &iid, ev);
	if (MATECOMPONENT_EX (ev)) {
		goto out;
	}
	
	pstream = matecomponent_get_object (iid, "IDL:MateComponent/PersistStream:1.0",
				     ev);
	if (MATECOMPONENT_EX (ev)) {
		pstream = CORBA_OBJECT_NIL;
		goto out;
	}
	
	MateComponent_PersistStream_load (pstream, stream, "", ev);
	
 out:
	CORBA_exception_free (&my_ev);
	g_free (iid);
	
	return pstream;
}
