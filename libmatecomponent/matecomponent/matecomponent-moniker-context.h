/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-moniker-context.c: A global moniker interface
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright (C) 2000, Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_MONIKER_CONTEXT_H_
#define _MATECOMPONENT_MONIKER_CONTEXT_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateComponentMonikerContextPrivate MateComponentMonikerContextPrivate;

typedef struct {
	MateComponentObject parent;

	MateComponentMonikerContextPrivate *priv;
} MateComponentMonikerContext;

typedef struct {
	MateComponentObjectClass parent;

	POA_MateComponent_MonikerContext__epv epv;
} MateComponentMonikerContextClass;

MateComponentObject *matecomponent_moniker_context_new (void);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_MONIKER_CONTEXT_H_ */

