/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-private.h: internal private init & shutdown routines
 *                    used by matecomponent_init & matecomponent_shutdown
 *
 * Authors:
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_PRIVATE_H_
#define _MATECOMPONENT_PRIVATE_H_

#ifdef HAVE_GTHREADS
extern GMutex *_matecomponent_lock;
#define MATECOMPONENT_LOCK()   g_mutex_lock(_matecomponent_lock);
#define MATECOMPONENT_UNLOCK() g_mutex_unlock(_matecomponent_lock);
#else
#define MATECOMPONENT_LOCK()   G_STMT_START{ (void)0; }G_STMT_END
#define MATECOMPONENT_UNLOCK() G_STMT_START{ (void)0; }G_STMT_END
#endif

void    matecomponent_context_init     (void);
void    matecomponent_context_shutdown (void);

int     matecomponent_object_shutdown  (void);

void    matecomponent_exception_shutdown       (void);
void    matecomponent_property_bag_shutdown    (void);
void    matecomponent_running_context_shutdown (void);

#endif /* _MATECOMPONENT_PRIVATE_H_ */
