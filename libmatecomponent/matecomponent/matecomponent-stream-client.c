/**
 * mate-stream-client.c: Helper routines to access a MateComponent_Stream CORBA object
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *   Miguel de Icaza (miguel@kernel.org).
 *   Michael Meekss  (michael@helixcode.com)
 *
 * Copyright 1999, 2000 Ximian, Inc.
 */
#include "config.h"
#include <string.h>
#include <unistd.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent-stream-client.h>

#define CORBA_BLOCK_SIZE 65536

/**
 * matecomponent_stream_client_write:
 * @stream: A CORBA Object reference to a MateComponent_Stream
 * @buffer: the buffer to write
 * @size: number of bytes to write
 * @ev: a CORBA environment to return status information.
 *
 * This is a helper routine to write @size bytes from @buffer to the
 * @stream.  It will continue to write bytes until a fatal error
 * occurs. It works around serious problems in MateCORBA's handling of
 * sequences, and makes for nicer, saner protocol usage for
 * transfering huge chunks of data.
 */
void
matecomponent_stream_client_write (const MateComponent_Stream stream,
			    const void *buffer, const size_t size,
			    CORBA_Environment *ev)
{
	MateComponent_Stream_iobuf *buf;
	size_t               pos;
	guint8              *mem = (guint8 *)buffer;

	if (size == 0)
		return;

	g_return_if_fail (ev != NULL);
	
	if (buffer == NULL || stream == CORBA_OBJECT_NIL)
		goto bad_param;

	buf = MateComponent_Stream_iobuf__alloc ();
	if (!buf)
		goto alloc_error;

	for (pos = 0; pos < size;) {
		buf->_buffer = (mem + pos);
		buf->_length = (pos + CORBA_BLOCK_SIZE < size) ?
			CORBA_BLOCK_SIZE : size - pos;
		buf->_maximum = buf->_length;

		MateComponent_Stream_write (stream, buf, ev);
		if (MATECOMPONENT_EX (ev)) {
			CORBA_free (buf);
			return;
		}
		pos += buf->_length;
	}

	CORBA_free (buf);
	return;

 alloc_error:
	CORBA_exception_set_system (ev, ex_CORBA_NO_MEMORY,
				    CORBA_COMPLETED_NO);
	return;

 bad_param:
	CORBA_exception_set_system (ev, ex_CORBA_BAD_PARAM,
				    CORBA_COMPLETED_NO);
	return;
}

/**
 * matecomponent_stream_client_write_string:
 * @stream: A CORBA object reference to a #MateComponent_Stream.
 * @str: A string.
 * @terminate: Whether or not to write the \0 at the end of the
 * string.
 * @ev: A pointer to a #CORBA_Environment
 *
 * This is a helper routine to write the string in @str to @stream.
 * If @terminate is TRUE, a NULL character will be written out at the
 * end of the string.  This function will not return until the entire
 * string has been written out, unless an exception is raised.  See
 * also matecomponent_stream_client_write(). Continues writing until finished
 * or a fatal exception occurs.
 *
 */
void
matecomponent_stream_client_write_string (const MateComponent_Stream stream, const char *str,
				   gboolean terminate, CORBA_Environment *ev)
{
	size_t total_length;

	g_return_if_fail (ev != NULL);
	g_return_if_fail (str != NULL);

	total_length = strlen (str) + (terminate ? 1 : 0);

	matecomponent_stream_client_write (stream, str, total_length, ev);
}

/**
 * matecomponent_stream_client_printf:
 * @stream: A CORBA object reference to a #MateComponent_Stream.
 * @terminate: Whether or not to null-terminate the string when it is
 * written out to the stream.
 * @ev: A CORBA_Environment pointer.
 * @fmt: The printf format string.
 * @Varargs: format arguments
 *
 * Processes @fmt and the arguments which follow it to produce a
 * string.  Writes this string out to @stream.  This function will not
 * return until the entire string is written out, unless an exception
 * is raised.  See also matecomponent_stream_client_write_string() and
 * matecomponent_stream_client_write().
 */
void
matecomponent_stream_client_printf (const MateComponent_Stream stream, const gboolean terminate,
			     CORBA_Environment *ev, const char *fmt, ...)
{
	va_list      args;
	char        *str;

	g_return_if_fail (fmt != NULL);

	va_start (args, fmt);
	str = g_strdup_vprintf (fmt, args);
	va_end (args);

	matecomponent_stream_client_write_string (stream, str, terminate, ev);

	g_free (str);
}

/**
 * matecomponent_stream_client_read_string:
 * @stream: The #MateComponent_Stream from which the string will be read.
 * @str: The string pointer in which the string will be stored.
 * @ev: A pointer to a #CORBA_Environment.
 *
 * Reads a NULL-terminated string from @stream and stores it in a
 * newly-allocated string in @str.
 *
 * Returns: The number of bytes read, or -1 if an error occurs.
 * If an exception occurs, @ev will contain the exception.
 */
CORBA_long
matecomponent_stream_client_read_string (const MateComponent_Stream stream, char **str,
				  CORBA_Environment *ev)
{
	MateComponent_Stream_iobuf *buffer;
	GString             *gstr;
	gboolean             all;

	gstr = g_string_sized_new (16);

	for (all = FALSE; !all; ) {

		MateComponent_Stream_read (stream, 1,
				    &buffer, ev);

		if (MATECOMPONENT_EX (ev))
			break;

		else if (buffer->_length == 0 ||
			 buffer->_buffer [0] == '\0')
			all = TRUE;
		
		else {
			g_string_append_c (gstr, buffer->_buffer [0]);
			CORBA_free (buffer);
		}
	}

	if (MATECOMPONENT_EX (ev)) {
		*str = NULL;
		g_string_free (gstr, TRUE);

		return -1;
	} else {
		CORBA_long l;

		l    = gstr->len;
		*str = gstr->str;
		g_string_free (gstr, FALSE);

		return l;
	}
}

/**
 * matecomponent_stream_client_get_length:
 * @stream: The stream.
 * @ev: Exception environment
 * 
 *   Does the grunt work to get the length of a stream,
 * returns -1 if the length is not available. Returns -1
 * on exception.
 * 
 * Return value: Length or -1
 **/
CORBA_long
matecomponent_stream_client_get_length (const MateComponent_Stream stream,
				 CORBA_Environment  *opt_ev)
{
	CORBA_long len;
	MateComponent_StorageInfo *info;
	CORBA_Environment  *ev, temp_ev;
       
	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	info = MateComponent_Stream_getInfo (stream, MateComponent_FIELD_SIZE, ev);

	if (MATECOMPONENT_EX (ev) || !info)
		len = -1;

	else {
		len = info->size;

		CORBA_free (info);
	}

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);
	
	return len;
}

/**
 * matecomponent_stream_client_read:
 * @stream: A CORBA Object reference to a MateComponent_Stream
 * @size: number of bytes to read or -1 for whole stream.
 * @length_read: if non NULL will be set to the length read
 * @ev: a CORBA environment to return status information.
 *
 * This is a helper routine to read @size bytes from the @stream into
 * a freshly g_ allocated buffer which is returned. Whilst this
 * routine may seem pointless; it reads the stream in small chunks
 * avoiding possibly massive alloca's inside MateCORBA's stub/skel code.
 *
 * Returns NULL on any sort of failure & 0 size read.
 */
guint8 *
matecomponent_stream_client_read (const MateComponent_Stream stream,
			   const size_t        size,
			   CORBA_long         *length_read,
			   CORBA_Environment  *ev)
{
	size_t  pos;
	guint8 *mem;
	ssize_t length;

	g_return_val_if_fail (ev != NULL, NULL);

	if (length_read)
		*length_read = size;

	length = size;

	if (length == -1) {
		length = matecomponent_stream_client_get_length (stream, ev);
		if (MATECOMPONENT_EX (ev) || length == -1) {
			char *err = matecomponent_exception_get_text (ev);
			g_warning ("Exception '%s' getting length of stream", err);
			g_free (err);
			return NULL;
		}
	} 

	if (length_read)
		*length_read = length;

	if (length == 0)
		return NULL;

	mem = g_try_malloc (length);
	if (!mem) {
		CORBA_exception_set_system (ev, ex_CORBA_NO_MEMORY,
					    CORBA_COMPLETED_NO);
		return NULL;
	}
	
	for (pos = 0; pos < length;) {
		MateComponent_Stream_iobuf *buf;
		CORBA_long           len;

		len = (pos + CORBA_BLOCK_SIZE < length) ?
			CORBA_BLOCK_SIZE : length - pos;

		MateComponent_Stream_read (stream, len, &buf, ev);

		if (MATECOMPONENT_EX (ev) || !buf)
			goto io_error;

		if (buf->_length > 0) {
			memcpy (mem + pos, buf->_buffer, buf->_length);
			pos += buf->_length;
		} else {
			g_warning ("Buffer length %u", buf->_length);
			goto io_error;
		}
		CORBA_free (buf);
	}

	return mem;

 io_error:
	return NULL;
}
