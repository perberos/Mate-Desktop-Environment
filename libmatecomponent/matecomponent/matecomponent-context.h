/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-context.h: Handle Global Component contexts.
 *
 * Author:
 *     Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_CONTEXT_H_
#define _MATECOMPONENT_CONTEXT_H_

#include <matecomponent/matecomponent-object.h>

MateComponent_Unknown matecomponent_context_get (const CORBA_char  *context_name,
				   CORBA_Environment *opt_ev);

void           matecomponent_context_add (const CORBA_char  *context_name,
				   MateComponent_Unknown     context);

/* emits a 'last_unref' signal */
MateComponentObject  *matecomponent_context_running_get (void);

void           matecomponent_running_context_auto_exit_unref (MateComponentObject *object);

#endif /* _MATECOMPONENT_CONTEXT_H_ */
