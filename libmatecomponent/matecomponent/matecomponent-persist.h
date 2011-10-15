/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-persist.h: a persistance interface
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_PERSIST_H_
#define _MATECOMPONENT_PERSIST_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_PERSIST        (matecomponent_persist_get_type ())
#define MATECOMPONENT_PERSIST_TYPE        MATECOMPONENT_TYPE_PERSIST /* deprecated, you should use MATECOMPONENT_TYPE_PERSIST */
#define MATECOMPONENT_PERSIST(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_PERSIST, MateComponentPersist))
#define MATECOMPONENT_PERSIST_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_PERSIST, MateComponentPersistClass))
#define MATECOMPONENT_IS_PERSIST(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_PERSIST))
#define MATECOMPONENT_IS_PERSIST_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_PERSIST))

typedef struct _MateComponentPersistPrivate MateComponentPersistPrivate;
typedef struct _MateComponentPersist        MateComponentPersist;

struct _MateComponentPersist {
	MateComponentObject object;

	MateComponentPersistPrivate *priv;
};

typedef struct {
	MateComponentObjectClass      parent_class;

	POA_MateComponent_Persist__epv epv;

	MateComponent_Persist_ContentTypeList *
	                      (*get_content_types) (MateComponentPersist     *persist,
						    CORBA_Environment *ev);
} MateComponentPersistClass;

GType                           matecomponent_persist_get_type (void) G_GNUC_CONST;

MateComponent_Persist_ContentTypeList *matecomponent_persist_generate_content_types (int num,
								       ...);

MateComponentPersist                  *matecomponent_persist_construct (MateComponentPersist *persist,
							  const gchar   *iid);

void				matecomponent_persist_set_dirty (MateComponentPersist *persist,
							  gboolean dirty);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_PERSIST_H_ */
