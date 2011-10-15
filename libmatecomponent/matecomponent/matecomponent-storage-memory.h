/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-storage-memory.h: Memory based MateComponent::Storage implementation
 *
 * Author:
 *   ÉRDI Gergõ <cactus@cactus.rulez.org>
 *
 * Copyright 2001 Gergõ Érdi
 */
#ifndef _MATECOMPONENT_STORAGE_MEM_H_
#define _MATECOMPONENT_STORAGE_MEM_H_

#include <matecomponent/matecomponent-storage.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_STORAGE_MEM        (matecomponent_storage_mem_get_type ())
#define MATECOMPONENT_STORAGE_MEM_TYPE        MATECOMPONENT_TYPE_STORAGE_MEM /* deprecated, you should use MATECOMPONENT_TYPE_STORAGE_MEM */
#define MATECOMPONENT_STORAGE_MEM(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_STORAGE_MEM, MateComponentStorageMem))
#define MATECOMPONENT_STORAGE_MEM_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_STORAGE_MEM, MateComponentStorageMemClass))
#define MATECOMPONENT_IS_STORAGE_MEM(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_STORAGE_MEM))
#define MATECOMPONENT_IS_STORAGE_MEM_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_STORAGE_MEM))

typedef struct _MateComponentStorageMemPriv MateComponentStorageMemPriv;
typedef struct _MateComponentStorageMem     MateComponentStorageMem;

struct _MateComponentStorageMem {
	MateComponentObject parent;

	MateComponentStorageMemPriv *priv;
};

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_Storage__epv epv;
} MateComponentStorageMemClass;

GType             matecomponent_storage_mem_get_type   (void) G_GNUC_CONST;
MateComponentObject     *matecomponent_storage_mem_create     (void);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_STORAGE_MEM_H_ */
