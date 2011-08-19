/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include <stdlib.h>
#include <string.h>

#include <matecomponent/matecomponent-stream-client.h>
#include <matecomponent/matecomponent-stream-memory.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-main.h>

#define BUFSIZE 100

static gboolean
test_read_write (MateComponentStream *memstream)
{
	CORBA_Environment ev;
	char *write_string = "This is the MateComponentStreamMem test application";
	char *read_string = 0;
	MateComponent_Stream stream;

	stream = matecomponent_object_corba_objref (MATECOMPONENT_OBJECT (memstream));
	
	CORBA_exception_init (&ev);
	printf ("\tWriting '%s' to stream\n", write_string);
	matecomponent_stream_client_write_string (stream,
					   write_string, TRUE,
					   &ev);
	if (MATECOMPONENT_EX (&ev))
		printf ("\tWrite failed");
	CORBA_exception_free (&ev);

	CORBA_exception_init (&ev);
	MateComponent_Stream_seek (stream, 0, MateComponent_Stream_SeekSet, &ev);
	CORBA_exception_free (&ev);

	CORBA_exception_init (&ev);
	matecomponent_stream_client_read_string (stream,
					  &read_string,
					  &ev);
	if (MATECOMPONENT_EX (&ev))
		printf ("\tRead failed");
	else
		printf ("\tRead '%s' from stream\n", read_string);
	CORBA_exception_free (&ev);

	if (!strcmp (read_string, write_string))
		return TRUE;
	else
		return FALSE;
}

int main (int argc, char *argv [])
{
	MateComponentStream *memstream;
	guint8       *buffer;

        g_thread_init (NULL);

	free (malloc (8));
	
	if (!matecomponent_init (&argc, argv))
		g_error ("matecomponent_init failed");
    
	printf ("Creating a stream in memory from scratch "
		"(size: %d bytes)\n", BUFSIZE);	
	memstream = matecomponent_stream_mem_create (NULL,
					      BUFSIZE,
					      FALSE, TRUE);
	if (test_read_write (memstream))
		printf ("Passed\n");
	else
		printf ("Failed\n");
	matecomponent_object_unref (memstream);


	printf ("Creating a stream in memory from pre-allocated buffer "
		"(size: %d bytes)\n", BUFSIZE);
	buffer = g_new0 (guint8, BUFSIZE);
	memstream = matecomponent_stream_mem_create (buffer,
					      BUFSIZE,
					      FALSE, TRUE);
	if (test_read_write (memstream))
		printf ("Passed\n");
	else
		printf ("Failed\n");
	matecomponent_object_unref (memstream);	

	return matecomponent_debug_shutdown ();
}
