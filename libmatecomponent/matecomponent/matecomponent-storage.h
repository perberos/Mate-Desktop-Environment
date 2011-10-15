/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * mate-storage.h: Storage manipulation.
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org).
 *
 * Copyright 1999 Helix Code, Inc.
 */

/*
 * Deprecation Warning: this object should not be used
 * directly. Instead, use a moniker-based approach. For example,
 * matecomponent_get_object("file:/tmp", "IDL:MateComponent/Storate:1.0", &ev) will
 * return a MateComponent_Storage for directory /tmp.
 */
#ifndef _MATECOMPONENT_STORAGE_H_
#define _MATECOMPONENT_STORAGE_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MATECOMPONENT_DISABLE_DEPRECATED

/* For backwards compatibility */
#define MateComponentStream MateComponentObject
#define MATECOMPONENT_STREAM(o)       ((MateComponentStream *)(o))
#define MATECOMPONENT_STREAM_CLASS(k) ((MateComponentObjectClass *)(k))

#define MateComponentStorage MateComponentObject
#define MATECOMPONENT_STORAGE(o)          ((MateComponentStorage *)(o))
#define MATECOMPONENT_STORAGE_CLASS(k)    ((MateComponentObjectClass *)(k))

#endif /* MATECOMPONENT_DISABLE_DEPRECATED */


/* The 1 useful impl. in here */
void matecomponent_storage_copy_to (MateComponent_Storage     src,
			     MateComponent_Storage     dest,
			     CORBA_Environment *ev);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_STORAGE_H_ */

