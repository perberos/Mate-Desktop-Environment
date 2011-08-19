/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-stream-memory.c: Memory based stream
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
 *
 * Copyright 1999, 2000 Ximian, Inc.
 */
#include <config.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include <matecomponent/matecomponent-stream-memory.h>
#include <matecomponent/matecomponent-exception.h>
#include <errno.h>

static MateComponentObjectClass *matecomponent_stream_mem_parent_class;

static MateComponent_StorageInfo*
mem_get_info (PortableServer_Servant         servant,
	      const MateComponent_StorageInfoFields mask,
	      CORBA_Environment             *ev)
{
	MateComponent_StorageInfo *si;
	MateComponentStreamMem    *smem = MATECOMPONENT_STREAM_MEM (
		matecomponent_object (servant));

	si = MateComponent_StorageInfo__alloc ();

	si->name = CORBA_string_dup (smem->name);

	if (mask & MateComponent_FIELD_SIZE)
		si->size = smem->size;
	if (mask & MateComponent_FIELD_TYPE)
		si->type = MateComponent_STORAGE_TYPE_REGULAR;
	si->content_type = CORBA_string_dup (
		(mask & MateComponent_FIELD_CONTENT_TYPE)
		? smem->content_type
		: "");

	return si;
}

static void
mem_set_info (PortableServer_Servant servant,
	      const MateComponent_StorageInfo *info,
	      const MateComponent_StorageInfoFields mask,
	      CORBA_Environment *ev)
{
	MateComponentStreamMem *smem = MATECOMPONENT_STREAM_MEM (
		matecomponent_object (servant));

	if (smem->read_only) {
		CORBA_exception_set (
			ev, CORBA_USER_EXCEPTION,
			ex_MateComponent_Stream_NoPermission, NULL);
		return;
	}

	if (mask & MateComponent_FIELD_SIZE) {
		CORBA_exception_set (
			ev, CORBA_USER_EXCEPTION,
			ex_MateComponent_Stream_NotSupported, NULL);
		return;
	}

	if ((mask & MateComponent_FIELD_TYPE) &&
	    (info->type != MateComponent_STORAGE_TYPE_REGULAR)) {
		CORBA_exception_set (
			ev, CORBA_USER_EXCEPTION,
			ex_MateComponent_Stream_NotSupported, NULL);
		return;
	}

	if (mask & MateComponent_FIELD_CONTENT_TYPE) {
		matecomponent_return_if_fail (info->content_type != NULL, ev);
		g_free (smem->content_type);
		smem->content_type = g_strdup (info->content_type);
	}

	if (strcmp (info->name, smem->name)) {
		matecomponent_return_if_fail (info->name != NULL, ev);
		g_free (smem->name);
		smem->name = g_strdup (info->name);
	}
}

static void
mem_truncate (PortableServer_Servant servant,
	      const CORBA_long new_size, 
	      CORBA_Environment *ev)
{
	MateComponentStreamMem *smem = MATECOMPONENT_STREAM_MEM (
		matecomponent_object (servant));
	void *newp;
	
	if (smem->read_only)
		return;

	newp = g_realloc (smem->buffer, new_size);
	if (!newp) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_Stream_NoPermission, NULL);
		return;
	}

	smem->buffer = newp;
	smem->size = new_size;

	if (smem->pos > new_size)
		smem->pos = new_size;
}

static void
mem_write (PortableServer_Servant servant,
	   const MateComponent_Stream_iobuf *buffer,
	   CORBA_Environment *ev)
{
	MateComponentStreamMem *smem = MATECOMPONENT_STREAM_MEM (
		matecomponent_object (servant));
	long len = buffer->_length;

	if (smem->read_only){
		g_warning ("Should signal an exception here");
		return;
	}

	if (smem->pos + len > smem->size){
		if (smem->resizable){
			smem->size = smem->pos + len;
			smem->buffer = g_realloc (smem->buffer, smem->size);
		} else {
			mem_truncate (servant, smem->pos + len, ev);
			g_warning ("Should check for an exception here");
		}
	}

	if (smem->pos + len > smem->size)
		len = smem->size - smem->pos;
	
	memcpy (smem->buffer + smem->pos, buffer->_buffer, len);
	smem->pos += len;
		
	return;
}

static void
mem_read (PortableServer_Servant servant, CORBA_long count,
	  MateComponent_Stream_iobuf ** buffer,
	  CORBA_Environment *ev)
{
	MateComponentStreamMem *smem = MATECOMPONENT_STREAM_MEM (
		matecomponent_object (servant));

	if (smem->pos + count > smem->size)
		count = smem->size - smem->pos;
	    
	*buffer = MateComponent_Stream_iobuf__alloc ();
	CORBA_sequence_set_release (*buffer, TRUE);
	(*buffer)->_buffer = CORBA_sequence_CORBA_octet_allocbuf (count);
	(*buffer)->_length = count;
	
	memcpy ((*buffer)->_buffer, smem->buffer + smem->pos, count);

	smem->pos += count;
}

static CORBA_long
mem_seek (PortableServer_Servant servant,
	  CORBA_long offset, MateComponent_Stream_SeekType whence,
	  CORBA_Environment *ev)
{
	MateComponentStreamMem *smem = MATECOMPONENT_STREAM_MEM (
		matecomponent_object (servant));
	int pos = 0;
	
	switch (whence){
	case MateComponent_Stream_SeekSet:
		pos = offset;
		break;

	case MateComponent_Stream_SeekCur:
		pos = smem->pos + offset;
		break;

	case MateComponent_Stream_SeekEnd:
		pos = smem->size + offset;
		break;

	default:
		g_warning ("Signal exception");
	}

	if (pos > smem->size){
		if (smem->resizable){
			smem->buffer = g_realloc (smem->buffer, pos);
			memset (smem->buffer + smem->size, 0,
				pos - smem->size);
			smem->size = pos;
		} else
			mem_truncate (servant, pos, ev);
	}
	smem->pos = pos;
	return pos;
}

static void
mem_commit (PortableServer_Servant servant,
	    CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_MateComponent_Stream_NotSupported, NULL);
}

static void
mem_revert (PortableServer_Servant servant,
	    CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_MateComponent_Stream_NotSupported, NULL);
}

static void
mem_finalize (GObject *object)
{
	MateComponentStreamMem *smem = MATECOMPONENT_STREAM_MEM (object);
	
	g_free (smem->buffer);
	g_free (smem->name);
	g_free (smem->content_type);
	
	G_OBJECT_CLASS (matecomponent_stream_mem_parent_class)->finalize (object);
}

static char *
mem_get_buffer (MateComponentStreamMem *stream_mem)
{
	g_return_val_if_fail (MATECOMPONENT_IS_STREAM_MEM (stream_mem), NULL);

	return stream_mem->buffer;
}

static size_t
mem_get_size (MateComponentStreamMem *stream_mem)
{
	g_return_val_if_fail (MATECOMPONENT_IS_STREAM_MEM (stream_mem), 0);

	return stream_mem->size;
}

static void
matecomponent_stream_mem_class_init (MateComponentStreamMemClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_Stream__epv *epv = &klass->epv;
	
	matecomponent_stream_mem_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = mem_finalize;

	epv->getInfo  = mem_get_info;
	epv->setInfo  = mem_set_info;
	epv->write    = mem_write;
	epv->read     = mem_read;
	epv->seek     = mem_seek;
	epv->truncate = mem_truncate;
	epv->commit   = mem_commit;
	epv->revert   = mem_revert;

	klass->get_buffer = mem_get_buffer;
	klass->get_size   = mem_get_size;
}

/**
 * matecomponent_stream_mem_get_type:
 *
 * Returns: the GType of the MateComponentStreamMem class.
 */
GType
matecomponent_stream_mem_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (MateComponentStreamMemClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) matecomponent_stream_mem_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentStreamMem),
			0, /* n_preallocs */
			(GInstanceInitFunc) NULL
		};

		type = matecomponent_type_unique (
			MATECOMPONENT_TYPE_OBJECT,
			POA_MateComponent_Stream__init, NULL,
			G_STRUCT_OFFSET (MateComponentStreamMemClass, epv),
			&info, "MateComponentStreamMem");
	}

	return type;
}

MateComponentStreamMem *
matecomponent_stream_mem_construct (MateComponentStreamMem *stream_mem,
			     const char      *buffer,
			     size_t           size,
			     gboolean         read_only,
			     gboolean         resizable)
{
	g_return_val_if_fail (MATECOMPONENT_IS_STREAM_MEM (stream_mem), NULL);

	if (buffer == NULL) {
		stream_mem->buffer = g_malloc (size);
		memset (stream_mem->buffer, 0, size);
	} else
		stream_mem->buffer = g_memdup (buffer, size);

	stream_mem->size = size;
	stream_mem->pos = 0;
	stream_mem->read_only = read_only;
	stream_mem->resizable = resizable;
	stream_mem->name = g_strdup ("");
	stream_mem->content_type = g_strdup ("application/octet-stream");

	return stream_mem;
}

/**
 * matecomponent_stream_mem_create:
 * @buffer: The data for which a MateComponentStreamMem object is to be created.
 * @size: The size in bytes of @buffer.
 * @read_only: Specifies whether or not the returned MateComponentStreamMem
 * object should allow write() operations.
 * @resizable: Whether or not the buffer should be resized as needed.
 *
 * Creates a new MateComponentStreamMem object.
 *
 * If @buffer is non-%NULL, @size bytes are copied from it into a new
 * buffer. If @buffer is %NULL, a new buffer of size @size is created
 * and filled with zero bytes.
 *
 * When data is read out of or (if @read_only is FALSE) written into
 * the returned MateComponentStream object, the read() and write() operations
 * operate on the new buffer. If @resizable is TRUE, writing or seeking
 * past the end of the buffer will cause the buffer to be expanded (with
 * the new space zero-filled for a seek).
 *
 * Returns: the constructed MateComponentStream object
 **/
MateComponentObject *
matecomponent_stream_mem_create (const char *buffer, size_t size,
			  gboolean read_only, gboolean resizable)
{
	MateComponentStreamMem *stream_mem;

	stream_mem = g_object_new (
		matecomponent_stream_mem_get_type (), NULL);

	if (!stream_mem)
		return NULL;

	return MATECOMPONENT_OBJECT (matecomponent_stream_mem_construct (
		stream_mem, buffer, size,
		read_only, resizable));
}

/**
 * matecomponent_stream_mem_get_buffer:
 * @stream_mem: a MateComponentStreamMem
 *
 * Returns the buffer associated with a MateComponentStreamMem. If the stream
 * is set to automatically resize itself, this buffer is only guaranteed
 * to stay valid until the next write operation on the stream.
 *
 * Return value: a buffer containing the data written to the stream (or
 * the data the stream was initialized with if nothing has been written).
 **/
const char *
matecomponent_stream_mem_get_buffer (MateComponentStreamMem *stream_mem)
{
	return MATECOMPONENT_STREAM_MEM_CLASS(
		G_OBJECT_GET_CLASS (stream_mem))->get_buffer (stream_mem);
}

/**
 * matecomponent_stream_mem_get_size:
 * @stream_mem: a MateComponentStreamMem
 *
 * Returns the size of the data associated with a MateComponentStreamMem
 * see matecomponent_stream_mem_get_buffer
 *
 * Return value: the size.
 **/
size_t
matecomponent_stream_mem_get_size (MateComponentStreamMem *stream_mem)
{
	return MATECOMPONENT_STREAM_MEM_CLASS(
		G_OBJECT_GET_CLASS (stream_mem))->get_size (stream_mem);
}
