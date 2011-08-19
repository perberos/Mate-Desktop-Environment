/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-debug.c: A runtime-controllable debugging API.
 *
 * Author:
 *   Jaka Mocnik  <jaka@gnu.org>
 */
#ifndef _MATECOMPONENT_DEBUG_H_
#define _MATECOMPONENT_DEBUG_H_

typedef enum {
	MATECOMPONENT_DEBUG_NONE = 0,
	MATECOMPONENT_DEBUG_REFS = 1 << 0,
	MATECOMPONENT_DEBUG_AGGREGATE = 1 << 1,
	MATECOMPONENT_DEBUG_LIFECYCLE = 1 << 2,
	MATECOMPONENT_DEBUG_RUNNING = 1 << 3,
	MATECOMPONENT_DEBUG_OBJECT = 1 << 4
} MateComponentDebugFlags;

extern MateComponentDebugFlags _matecomponent_debug_flags;

void matecomponent_debug_init  (void);
void matecomponent_debug_print (const char *name, char *fmt, ...);

#endif /* _MATECOMPONENT_DEBUG_H_ */
