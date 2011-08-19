/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-storage.c: Storage manipulation.
 *
 * Authors:
 *   Dietmar Maurer (dietmar@maurer-it.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#include <config.h>
#include <gmodule.h>

#include <matecomponent/matecomponent-storage.h>
#include <matecomponent/matecomponent-exception.h>

static void
copy_stream (MateComponent_Stream src, MateComponent_Stream dest, CORBA_Environment *ev) 
{
	MateComponent_Stream_iobuf *buf;

	do {
		MateComponent_Stream_read (src, 4096, &buf, ev);
		if (MATECOMPONENT_EX (ev)) 
			break;

		if (buf->_length == 0) {
			CORBA_free (buf);
			break;
		}

		MateComponent_Stream_write (dest, buf, ev);
		CORBA_free (buf);
		if (MATECOMPONENT_EX (ev)) 
			break;

	} while (1);

	if (MATECOMPONENT_EX (ev)) /* we must return a MateComponent_Storage exception */
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_MateComponent_Storage_IOError, NULL);
}

/**
 * matecomponent_storage_copy_to:
 * @src: the source storage
 * @dest: the destination storage
 * @ev: CORBA exception environment
 * 
 * Implements a pure CORBA method for copying one storage into
 * another, this is used by several MateComponentStorage implemetations
 * where a fast case localy copy cannot work.
 **/
void
matecomponent_storage_copy_to (MateComponent_Storage src, MateComponent_Storage dest,
			CORBA_Environment *ev) 
{
	MateComponent_Storage new_src, new_dest;
	MateComponent_Stream src_stream, dest_stream;
	MateComponent_Storage_DirectoryList *list;
	gint i;

	if ((src == CORBA_OBJECT_NIL) || (dest == CORBA_OBJECT_NIL) || !ev) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_MateComponent_Storage_IOError, NULL);
		return;
	}

	list = MateComponent_Storage_listContents (src, "", 
					    MateComponent_FIELD_CONTENT_TYPE |
					    MateComponent_FIELD_TYPE,
					    ev);
	if (MATECOMPONENT_EX (ev))
		return;

	for (i = 0; i <list->_length; i++) {

		if (list->_buffer[i].type == MateComponent_STORAGE_TYPE_DIRECTORY) {

			new_dest = MateComponent_Storage_openStorage
				(dest, list->_buffer[i].name, 
				 MateComponent_Storage_CREATE | 
				 MateComponent_Storage_FAILIFEXIST, ev);

			if (MATECOMPONENT_EX (ev)) 
				break;

			MateComponent_Storage_setInfo (new_dest, "",
						&list->_buffer[i],
						MateComponent_FIELD_CONTENT_TYPE,
						ev);

			if (MATECOMPONENT_EX (ev)) {
				matecomponent_object_release_unref (new_dest, NULL);
				break;
			}

			new_src = MateComponent_Storage_openStorage
				(src, list->_buffer[i].name, 
				 MateComponent_Storage_READ, ev);
			
			if (MATECOMPONENT_EX (ev)) {
				matecomponent_object_release_unref (new_dest, NULL);
				break;
			}

			matecomponent_storage_copy_to (new_src, new_dest, ev);
			
			matecomponent_object_release_unref (new_src, NULL);
			matecomponent_object_release_unref (new_dest, NULL);

			if (MATECOMPONENT_EX (ev))
				break;

		} else {
			dest_stream = MateComponent_Storage_openStream 
				(dest, list->_buffer[i].name,
				 MateComponent_Storage_CREATE | 
				 MateComponent_Storage_FAILIFEXIST, ev);

			if (MATECOMPONENT_EX (ev))
				break;

			MateComponent_Stream_setInfo (dest_stream,
					       &list->_buffer[i],
					       MateComponent_FIELD_CONTENT_TYPE,
					       ev);

			if (MATECOMPONENT_EX (ev)) {
				CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
						     ex_MateComponent_Storage_IOError,
						     NULL);
				matecomponent_object_release_unref (dest_stream,
							     NULL);
				break;
			}

			src_stream = MateComponent_Storage_openStream 
				(src, list->_buffer[i].name,
				 MateComponent_Storage_READ, ev);

			if (MATECOMPONENT_EX (ev)) {
				matecomponent_object_release_unref (dest_stream, 
							     NULL);
				break;
			}

			copy_stream (src_stream, dest_stream, ev);

			matecomponent_object_release_unref (src_stream, NULL);
			matecomponent_object_release_unref (dest_stream, NULL);

			if (MATECOMPONENT_EX (ev))
				break;
		}
	}

	CORBA_free (list);
}
