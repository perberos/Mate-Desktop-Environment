/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * MateComponent PersistFile
 *
 * Author:
 *   Matt Loper (matt@mate-support.com)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_PERSIST_FILE_H_
#define _MATECOMPONENT_PERSIST_FILE_H_

#include <matecomponent/matecomponent-persist.h>

#ifndef MATECOMPONENT_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_PERSIST_FILE (matecomponent_persist_file_get_type ())
#define MATECOMPONENT_PERSIST_FILE_TYPE        MATECOMPONENT_TYPE_PERSIST_FILE /* deprecated, you should use MATECOMPONENT_TYPE_PERSIST_FILE */
#define MATECOMPONENT_PERSIST_FILE(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_PERSIST_FILE, MateComponentPersistFile))
#define MATECOMPONENT_PERSIST_FILE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_PERSIST_FILE, MateComponentPersistFileClass))
#define MATECOMPONENT_IS_PERSIST_FILE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_PERSIST_FILE))
#define MATECOMPONENT_IS_PERSIST_FILE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_PERSIST_FILE))

typedef struct _MateComponentPersistFilePrivate MateComponentPersistFilePrivate;
typedef struct _MateComponentPersistFile        MateComponentPersistFile;

typedef int (*MateComponentPersistFileIOFn) (MateComponentPersistFile *pf,
				      const CORBA_char  *uri,
				      CORBA_Environment *ev,
				      void              *closure);

struct _MateComponentPersistFile {
	MateComponentPersist persist;

	char *uri;

	/*
	 * For the sample routines, NULL if we use the ::save and ::load
	 * methods from the class
	 */
	MateComponentPersistFileIOFn  save_fn;
	MateComponentPersistFileIOFn  load_fn;
	void *closure;

	MateComponentPersistFilePrivate *priv;
};

typedef struct {
	MateComponentPersistClass parent_class;

	POA_MateComponent_PersistFile__epv epv;

	/* methods */
	int   (*load)             (MateComponentPersistFile *ps,
				   const CORBA_char  *uri,
				   CORBA_Environment *ev);

	int   (*save)             (MateComponentPersistFile *ps,
				   const CORBA_char  *uri,
				   CORBA_Environment *ev);

	char *(*get_current_file) (MateComponentPersistFile *ps,
				   CORBA_Environment *ev);

} MateComponentPersistFileClass;

GType              matecomponent_persist_file_get_type  (void) G_GNUC_CONST;

MateComponentPersistFile *matecomponent_persist_file_new       (MateComponentPersistFileIOFn load_fn,
						  MateComponentPersistFileIOFn save_fn,
						  const gchar          *iid,
						  void                 *closure);

MateComponentPersistFile *matecomponent_persist_file_construct (MateComponentPersistFile    *pf,
						  MateComponentPersistFileIOFn load_fn,
						  MateComponentPersistFileIOFn save_fn,
						  const gchar          *iid,
						  void                 *closure);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_DISABLE_DEPRECATED */

#endif /* _MATECOMPONENT_PERSIST_FILE_H_ */
