/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-stream-memory.h: Memory based stream
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_STREAM_MEM_H_
#define _MATECOMPONENT_STREAM_MEM_H_

#include <matecomponent/matecomponent-storage.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _MateComponentStreamMem;
typedef struct _MateComponentStreamMem MateComponentStreamMem;
typedef struct _MateComponentStreamMemPrivate MateComponentStreamMemPrivate;

#define MATECOMPONENT_TYPE_STREAM_MEM        (matecomponent_stream_mem_get_type ())
#define MATECOMPONENT_STREAM_MEM_TYPE        MATECOMPONENT_TYPE_STREAM_MEM /* deprecated, you should use MATECOMPONENT_TYPE_STREAM_MEM */
#define MATECOMPONENT_STREAM_MEM(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_STREAM_MEM, MateComponentStreamMem))
#define MATECOMPONENT_STREAM_MEM_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_STREAM_MEM, MateComponentStreamMemClass))
#define MATECOMPONENT_IS_STREAM_MEM(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_STREAM_MEM))
#define MATECOMPONENT_IS_STREAM_MEM_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_STREAM_MEM))

struct _MateComponentStreamMem {
	MateComponentObject parent;

	char         *buffer;
	size_t        size;
	long          pos;
	gboolean      read_only;
	gboolean      resizable;
	char         *content_type;
	char         *name;

	MateComponentStreamMemPrivate *priv;
};

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_Stream__epv epv;

	char           *(*get_buffer) (MateComponentStreamMem *stream_mem);
	size_t          (*get_size)   (MateComponentStreamMem *stream_mem);
} MateComponentStreamMemClass;

GType            matecomponent_stream_mem_get_type   (void) G_GNUC_CONST;
MateComponentStreamMem *matecomponent_stream_mem_construct  (MateComponentStreamMem  *stream_mem,
					       const char       *buffer,
					       size_t            size,
					       gboolean          read_only,
					       gboolean          resizable);

MateComponentObject    *matecomponent_stream_mem_create     (const char       *buffer,
					       size_t            size,
					       gboolean          read_only,
					       gboolean          resizable);

const char      *matecomponent_stream_mem_get_buffer (MateComponentStreamMem  *stream_mem);
size_t           matecomponent_stream_mem_get_size   (MateComponentStreamMem  *stream_mem);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_STREAM_MEM_H_ */
