/*
 * matecomponent-stream-cache.c: 
 *
 * A simple cache for streams (direct mapped, write back)
 *
 * Authors:
 *	Dietmar Maurer (dietmar@ximian.com)
 *      Michael Meeks  (michael@ximian.com)
 *
 * Copyright 2000, 2001 Ximian, Inc.
 */

#include <config.h>
#include <string.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-stream-client.h>

#include "matecomponent-stream-cache.h"

/* configurable values for cache size */

#define SC_PAGE_SIZE_BITS 14
#define SC_CACHE_SIZE_BITS 5

/* some handy macros */

#define SC_PAGE_SIZE         (1 << (SC_PAGE_SIZE_BITS - 1))
#define SC_CACHE_SIZE        (1 << (SC_CACHE_SIZE_BITS - 1))

#define SC_CACHE_TAG(pos)    (pos >> (SC_PAGE_SIZE_BITS - 1))
#define SC_TAG_POS(tag)      (tag << (SC_PAGE_SIZE_BITS - 1))
#define SC_CACHE_INDEX(pos)  (SC_CACHE_TAG(pos) & (SC_CACHE_SIZE - 1))
#define SC_BLOCK_OFFSET(pos) (pos & (SC_PAGE_SIZE - 1))


typedef struct {
	char     buf [SC_PAGE_SIZE];
	long     tag;
	gboolean valid;
	gboolean dirty;
} CacheEntry; 

struct _MateComponentStreamCachePrivate {
	MateComponent_Stream cs;
	long          pos;
	long          size;
	CacheEntry    cache [SC_CACHE_SIZE];
};

static void
matecomponent_stream_cache_invalidate (MateComponentStreamCache *stream_cache, 
				long               pos)
{
	long i, tag = SC_CACHE_TAG (pos);

	for (i = 0; i < SC_CACHE_SIZE; i++) {
		if (stream_cache->priv->cache [i].valid &&
		    (stream_cache->priv->cache [i].tag >= tag))
			stream_cache->priv->cache [i].valid = FALSE;
	}
}

static void
matecomponent_stream_cache_flush (MateComponentStreamCache *stream, 
			   int                index,
			   CORBA_Environment *ev)
{
	long i, end, pos;
	
	end = index < 0 ? SC_CACHE_SIZE : index + 1;

	for (i = index < 0 ? 0 : index; i < end; i++) {
		if (((index < 0) || (index == i)) &&
		    stream->priv->cache [i].valid &&
		    stream->priv->cache [i].dirty) {
			pos = SC_TAG_POS (stream->priv->cache [i].tag);
			
			MateComponent_Stream_seek (stream->priv->cs, pos,
					    MateComponent_Stream_SeekSet, ev);
			if (MATECOMPONENT_EX (ev))
				continue;

			matecomponent_stream_client_write (stream->priv->cs,
			        stream->priv->cache [i].buf,
						    SC_PAGE_SIZE, ev);
			if (!MATECOMPONENT_EX (ev))
				stream->priv->cache [i].dirty = FALSE;
		}
	}
}

static void
matecomponent_stream_cache_load (MateComponentStreamCache *stream, 
			  long               tag,
			  CORBA_Environment *ev)
{
	MateComponent_Stream_iobuf *iobuf;
	long pos, index;

	pos = SC_TAG_POS (tag);
	index = SC_CACHE_INDEX (pos);

	matecomponent_stream_cache_flush (stream, index, ev);
	if (MATECOMPONENT_EX (ev))
		return;

	MateComponent_Stream_seek (stream->priv->cs, pos, MateComponent_Stream_SeekSet, ev);
	if (MATECOMPONENT_EX (ev))
		return;

	MateComponent_Stream_read (stream->priv->cs, SC_PAGE_SIZE, &iobuf, ev);
	if (MATECOMPONENT_EX (ev))
		return;
	
	if (iobuf->_length < SC_PAGE_SIZE) /* eof  - fill with zero */
		memset (stream->priv->cache [index].buf + iobuf->_length, 0,
			SC_PAGE_SIZE - iobuf->_length);
				
	if ((pos + iobuf->_length) > stream->priv->size)
		stream->priv->size = pos + iobuf->_length;

	memcpy (stream->priv->cache [index].buf, iobuf->_buffer, 
		iobuf->_length);
	
	stream->priv->cache [index].valid = TRUE;
	stream->priv->cache [index].dirty = FALSE;
	stream->priv->cache [index].tag = tag;

	CORBA_free (iobuf);
}

static long
matecomponent_stream_cache_read (MateComponentStreamCache *stream, 
			  long               count, 
			  char              *buffer,
			  CORBA_Environment *ev)
{
	long tag, bytes_read = 0;
	int index, offset, bc, d;

	while (bytes_read < count) {
		index = SC_CACHE_INDEX (stream->priv->pos);
		offset = SC_BLOCK_OFFSET (stream->priv->pos);
		tag = SC_CACHE_TAG (stream->priv->pos);

		if ((stream->priv->pos < stream->priv->size) &&
		    stream->priv->cache [index].valid &&
		    stream->priv->cache [index].tag == tag) {
			bc = SC_PAGE_SIZE - offset;

			if ((bytes_read + bc) > count)
				bc = count - bytes_read;

			if ((d = (stream->priv->pos + bc) - 
			     stream->priv->size) > 0)
				bc -= d;
			if (!bc)
				return bytes_read;

			memcpy (buffer + bytes_read, 
				stream->priv->cache [index].buf + offset, bc);
			bytes_read += bc;
			stream->priv->pos += bc;
		} else {
			matecomponent_stream_cache_load (stream, tag, ev);
			if (MATECOMPONENT_EX (ev))
				break;
			if (stream->priv->pos >= stream->priv->size)
				break;
		}
	}

	return bytes_read;
}

static void
matecomponent_stream_cache_destroy (MateComponentObject *object)
{
	MateComponentStreamCache *stream_cache = MATECOMPONENT_STREAM_CACHE (object);

	if (stream_cache->priv->cs)
		matecomponent_object_release_unref (stream_cache->priv->cs, NULL);

	g_free (stream_cache->priv);
}

static MateComponent_StorageInfo*
cache_getInfo (PortableServer_Servant          servant, 
	       const MateComponent_StorageInfoFields  mask,
	       CORBA_Environment              *ev)
{
	MateComponentStreamCache *stream_cache = MATECOMPONENT_STREAM_CACHE (
		matecomponent_object (servant));
	
	return MateComponent_Stream_getInfo (stream_cache->priv->cs, mask, ev);
}

static void
cache_setInfo (PortableServer_Servant          servant, 
	       const MateComponent_StorageInfo       *info,
	       const MateComponent_StorageInfoFields  mask, 
	       CORBA_Environment              *ev)
{
	MateComponentStreamCache *stream_cache = MATECOMPONENT_STREAM_CACHE (
		matecomponent_object (servant));
	
	MateComponent_Stream_setInfo (stream_cache->priv->cs, info, mask, ev);
}

static void
cache_write (PortableServer_Servant     servant, 
	     const MateComponent_Stream_iobuf *buffer,
	     CORBA_Environment         *ev)
{
	MateComponentStreamCache *stream = MATECOMPONENT_STREAM_CACHE (
		matecomponent_object (servant));
	long tag, bytes_written = 0;
	int index, offset, bc;
	
	while (bytes_written < buffer->_length) {
		index = SC_CACHE_INDEX (stream->priv->pos);
		offset = SC_BLOCK_OFFSET (stream->priv->pos);
		tag = SC_CACHE_TAG (stream->priv->pos);

		if (stream->priv->cache [index].valid &&
		    stream->priv->cache [index].tag == tag) {
			bc = SC_PAGE_SIZE - offset;
			if (bc > buffer->_length) 
				bc = buffer->_length;
			memcpy (stream->priv->cache [index].buf + offset,
				buffer->_buffer + bytes_written, bc);
			bytes_written += bc;
			stream->priv->pos += bc;
			stream->priv->cache [index].dirty = TRUE;
		} else {
			matecomponent_stream_cache_load (stream, tag, ev);
			if (MATECOMPONENT_EX (ev))
				break;
		}
	}
}

static void
cache_read (PortableServer_Servant servant, 
	    CORBA_long             count,
	    MateComponent_Stream_iobuf  **buffer, 
	    CORBA_Environment     *ev)
{
	MateComponentStreamCache *stream_cache = MATECOMPONENT_STREAM_CACHE (
		matecomponent_object (servant));
	CORBA_octet *data;

	if (count < 0) {
		matecomponent_exception_set (ev, ex_MateComponent_Stream_IOError);
		return;
	}

	*buffer = MateComponent_Stream_iobuf__alloc ();
	CORBA_sequence_set_release (*buffer, TRUE);
	data = CORBA_sequence_CORBA_octet_allocbuf (count);
	(*buffer)->_buffer = data;
	(*buffer)->_length = matecomponent_stream_cache_read (stream_cache, count, 
						       data, ev);
}

static CORBA_long
cache_seek (PortableServer_Servant servant, 
	    CORBA_long             offset, 
	    MateComponent_Stream_SeekType whence, 
	    CORBA_Environment     *ev)
{
	MateComponentStreamCache *stream_cache = MATECOMPONENT_STREAM_CACHE (
		matecomponent_object (servant));
	
	stream_cache->priv->pos = MateComponent_Stream_seek (stream_cache->priv->cs, 
						      offset, whence, ev);

	return stream_cache->priv->pos;
}

static void
cache_truncate (PortableServer_Servant servant, 
		const CORBA_long       new_size, 
		CORBA_Environment     *ev)
{
	MateComponentStreamCache *stream_cache = MATECOMPONENT_STREAM_CACHE (
		matecomponent_object (servant));
	
	matecomponent_stream_cache_invalidate (stream_cache, new_size);
	
	stream_cache->priv->size = new_size;

	MateComponent_Stream_truncate (stream_cache->priv->cs, new_size, ev);
}

static void
cache_commit (PortableServer_Servant servant, 
	      CORBA_Environment     *ev)
{
	MateComponentStreamCache *stream_cache = MATECOMPONENT_STREAM_CACHE (
		matecomponent_object (servant));

	matecomponent_stream_cache_flush (stream_cache, -1, ev);
	 
	MateComponent_Stream_commit (stream_cache->priv->cs, ev);
}

static void
cache_revert (PortableServer_Servant servant, 
	      CORBA_Environment     *ev)
{
	MateComponentStreamCache *stream_cache = MATECOMPONENT_STREAM_CACHE (
		matecomponent_object (servant));

	matecomponent_stream_cache_invalidate (stream_cache, 0);

	MateComponent_Stream_revert (stream_cache->priv->cs, ev);
}

static void
matecomponent_stream_cache_init (MateComponentStreamCache *stream)
{
	stream->priv = g_new0 (MateComponentStreamCachePrivate, 1);
}

static void
matecomponent_stream_cache_class_init (MateComponentStreamCacheClass *klass)
{
	MateComponentObjectClass *object_class = (MateComponentObjectClass *) klass;
	POA_MateComponent_Stream__epv *epv = &klass->epv;

	epv->getInfo  = cache_getInfo;
	epv->setInfo  = cache_setInfo;
	epv->write    = cache_write;
	epv->read     = cache_read;
	epv->seek     = cache_seek;
	epv->truncate = cache_truncate;
	epv->commit   = cache_commit;
	epv->revert   = cache_revert;

	object_class->destroy = matecomponent_stream_cache_destroy;
}

GType
matecomponent_stream_cache_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (MateComponentStreamCacheClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) matecomponent_stream_cache_class_init,
			(GClassFinalizeFunc) NULL,
			NULL, /* class_data */
			sizeof (MateComponentStreamCache),
			0,
			(GInstanceInitFunc) matecomponent_stream_cache_init
		};
		
		type = matecomponent_type_unique (
			MATECOMPONENT_TYPE_OBJECT,
			POA_MateComponent_Stream__init, NULL,
			G_STRUCT_OFFSET (MateComponentStreamCacheClass, epv),
			&info, "MateComponentStreamCache");
	}
  
	return type;
}

/** 
 * matecomponent_stream_cache_create:
 * @cs: a reference to the stream we want to cache
 * @opt_ev: an optional environment
 *
 * Returns a new MateComponentStream object
 */
MateComponentObject *
matecomponent_stream_cache_create (MateComponent_Stream      cs,
			    CORBA_Environment *opt_ev)
{
	MateComponentStreamCache *stream;
	CORBA_Environment  ev, *my_ev;

	matecomponent_return_val_if_fail (cs != NULL, NULL, opt_ev);

	if (!(stream = g_object_new (matecomponent_stream_cache_get_type (), NULL))) {
		if (opt_ev)
			matecomponent_exception_set (opt_ev, ex_MateComponent_Storage_IOError);
		return NULL;
	}
	
	if (!opt_ev) {
		CORBA_exception_init (&ev);
		my_ev = &ev;
	} else
		my_ev = opt_ev;

	stream->priv->cs = matecomponent_object_dup_ref (cs, my_ev);

	if (MATECOMPONENT_EX (my_ev)) {
		if (!opt_ev)
			CORBA_exception_free (&ev);
		matecomponent_object_unref (MATECOMPONENT_OBJECT (stream));
		return NULL;
	}

	if (!opt_ev)
		CORBA_exception_free (&ev);

	return (MateComponentObject *) stream;
}
