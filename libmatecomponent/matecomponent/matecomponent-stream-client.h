/**
 * mate-stream-client.c: Helper routines to access a MateComponent_Stream CORBA object
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *   Miguel de Icaza (miguel@kernel.org).
 *   Michael Meekss  (michael@helixcode.com)
 *
 * Copyright 1999,2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_STREAM_CLIENT_H_
#define _MATECOMPONENT_STREAM_CLIENT_H_

#include <matecomponent/MateComponent.h>

void       matecomponent_stream_client_write        (const MateComponent_Stream stream,
					      const void         *buffer,
					      const size_t        size,
					      CORBA_Environment  *ev);

guint8    *matecomponent_stream_client_read         (const MateComponent_Stream stream,
					      const size_t        size,
					      CORBA_long         *length_read,
					      CORBA_Environment  *ev);

void       matecomponent_stream_client_write_string (const MateComponent_Stream stream,
					      const char         *str,
					      const gboolean      terminate,
					      CORBA_Environment  *ev);

void       matecomponent_stream_client_printf       (const MateComponent_Stream stream,
					      const gboolean      terminate,
					      CORBA_Environment  *ev,
					      const char         *fmt, ...) G_GNUC_PRINTF(4,5);

CORBA_long matecomponent_stream_client_read_string  (const MateComponent_Stream stream,
					      char              **str,
					      CORBA_Environment  *ev);

CORBA_long matecomponent_stream_client_get_length   (const MateComponent_Stream stream,
					      CORBA_Environment  *ev);

#endif /* _MATECOMPONENT_STREAM_CLIENT_H_ */
