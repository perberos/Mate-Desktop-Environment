/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-stream.h: Stream manipulation, abstract class
 *
 * Author:
 *     Miguel de Icaza (miguel@gnu.org).
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */

/*
 * Deprecation Warning: this object should not be used
 * directly. Instead, use a moniker-based approach. For example,
 * matecomponent_get_object("file:/tmp/foo.txt", "IDL:MateComponent/Stream:1.0", &ev)
 * will return a MateComponent_Stream for file /tmp/foo.txt, for *reading* only!
 * To obtain a MateComponent_Stream for *writing*, you have to:
 *   1. Get a MateComponent_Storage for the directory containing the file you
 * wish to write (see comment in matecomponent-storage.h for how to do this)
 *   2. Use the openStream method of the MateComponent_Storage, with a
 * MateComponent_Storage_WRITE or MateComponent_Storage_CREATE flag. The 'path'
 * argument should be filename only, without directories. If the file
 * already exists, you may need to call store.erase(filename) first.
 */
#ifndef _MATECOMPONENT_STREAM_H_
#define _MATECOMPONENT_STREAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <matecomponent/matecomponent-storage.h>

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_STREAM_H_ */

