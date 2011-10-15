/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-persist-stream.c: PersistStream implementation.  Can be used as a
 * base class, or directly for implementing objects that use PersistStream.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_PERSIST_STREAM_H_
#define _MATECOMPONENT_PERSIST_STREAM_H_

#include <matecomponent/matecomponent-persist.h>

#ifndef MATECOMPONENT_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_PERSIST_STREAM        (matecomponent_persist_stream_get_type ())
#define MATECOMPONENT_PERSIST_STREAM_TYPE        MATECOMPONENT_TYPE_PERSIST_STREAM /* deprecated, you should use MATECOMPONENT_TYPE_PERSIST_STREAM */
#define MATECOMPONENT_PERSIST_STREAM(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_PERSIST_STREAM, MateComponentPersistStream))
#define MATECOMPONENT_PERSIST_STREAM_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_PERSIST_STREAM, MateComponentPersistStreamClass))
#define MATECOMPONENT_IS_PERSIST_STREAM(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_PERSIST_STREAM))
#define MATECOMPONENT_IS_PERSIST_STREAM_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_PERSIST_STREAM))

typedef struct _MateComponentPersistStreamPrivate MateComponentPersistStreamPrivate;
typedef struct _MateComponentPersistStream        MateComponentPersistStream;

typedef void  (*MateComponentPersistStreamIOFn) (MateComponentPersistStream         *ps,
					  const MateComponent_Stream         stream,
					  MateComponent_Persist_ContentType  type,
					  void                       *closure,
					  CORBA_Environment          *ev);

typedef MateComponent_Persist_ContentTypeList * (*MateComponentPersistStreamTypesFn) (MateComponentPersistStream *ps,
									void                *closure,
									CORBA_Environment   *ev);

struct _MateComponentPersistStream {
	MateComponentPersist persist;

	gboolean     is_dirty;

	/*
	 * For the sample routines, NULL if we use the
	 * methods from the class
	 */
	MateComponentPersistStreamIOFn     save_fn;
	MateComponentPersistStreamIOFn     load_fn;
	MateComponentPersistStreamTypesFn  types_fn;

	void                       *closure;

	MateComponentPersistStreamPrivate *priv;
};

typedef struct {
	MateComponentPersistClass parent_class;

	POA_MateComponent_PersistStream__epv epv;

	/* methods */
	void       (*load)         (MateComponentPersistStream        *ps,
				    MateComponent_Stream              stream,
				    MateComponent_Persist_ContentType type,
				    CORBA_Environment          *ev);
	void       (*save)         (MateComponentPersistStream        *ps,
				    MateComponent_Stream              stream,
				    MateComponent_Persist_ContentType type,
				    CORBA_Environment          *ev);

	MateComponent_Persist_ContentTypeList * (*get_content_types) (MateComponentPersistStream *ps,
							       CORBA_Environment   *ev);

} MateComponentPersistStreamClass;

GType                matecomponent_persist_stream_get_type  (void) G_GNUC_CONST;

MateComponentPersistStream *matecomponent_persist_stream_new       (MateComponentPersistStreamIOFn    load_fn,
						      MateComponentPersistStreamIOFn    save_fn,
						      MateComponentPersistStreamTypesFn types_fn,
						      const gchar               *iid,
						      void                      *closure);

MateComponentPersistStream *matecomponent_persist_stream_construct (MateComponentPersistStream       *ps,
						      MateComponentPersistStreamIOFn    load_fn,
						      MateComponentPersistStreamIOFn    save_fn,
						      MateComponentPersistStreamTypesFn types_fn,
						      const gchar               *iid,
						      void                      *closure);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_DISABLE_DEPRECATED */

#endif /* _MATECOMPONENT_PERSIST_STREAM_H_ */
