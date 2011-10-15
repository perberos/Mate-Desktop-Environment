/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-moniker-simple: Simplified object naming abstraction
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_MONIKER_SIMPLE_SIMPLE_H_
#define _MATECOMPONENT_MONIKER_SIMPLE_SIMPLE_H_

#include <matecomponent/matecomponent-types.h>
#include <matecomponent/matecomponent-moniker.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_MONIKER_SIMPLE        (matecomponent_moniker_simple_get_type ())
#define MATECOMPONENT_MONIKER_SIMPLE_TYPE        MATECOMPONENT_TYPE_MONIKER_SIMPLE /* deprecated, you should use MATECOMPONENT_TYPE_MONIKER_SIMPLE */
#define MATECOMPONENT_MONIKER_SIMPLE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_MONIKER_SIMPLE, MateComponentMonikerSimple))
#define MATECOMPONENT_MONIKER_SIMPLE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_MONIKER_SIMPLE, MateComponentMonikerSimpleClass))
#define MATECOMPONENT_IS_MONIKER_SIMPLE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_MONIKER_SIMPLE))
#define MATECOMPONENT_IS_MONIKER_SIMPLE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_MONIKER_SIMPLE))

#define MATECOMPONENT_TYPE_RESOLVE_FLAG          (matecomponent_resolve_flag_get_type ())
#define MATECOMPONENT_RESOLVE_FLAG_TYPE        MATECOMPONENT_TYPE_RESOLVE_FLAG /* deprecated, you should use MATECOMPONENT_TYPE_RESOLVE_FLAG */
GType matecomponent_resolve_flag_get_type (void) G_GNUC_CONST;

typedef struct _MateComponentMonikerSimple        MateComponentMonikerSimple;
typedef struct _MateComponentMonikerSimplePrivate MateComponentMonikerSimplePrivate;

typedef MateComponent_Unknown (*MateComponentMonikerSimpleResolveFn) (MateComponentMoniker               *moniker,
							const MateComponent_ResolveOptions *options,
							const CORBA_char            *requested_interface,
							CORBA_Environment           *ev);

struct _MateComponentMonikerSimple {
        MateComponentMoniker                moniker;

	MateComponentMonikerSimplePrivate  *priv;
};

typedef struct {
	MateComponentMonikerClass parent_class;
} MateComponentMonikerSimpleClass;

GType          matecomponent_moniker_simple_get_type    (void) G_GNUC_CONST;

MateComponentMoniker *matecomponent_moniker_simple_construct   (MateComponentMonikerSimple         *moniker,
						  const char                  *name,
						  GClosure                    *resolve_closure);

MateComponentMoniker *matecomponent_moniker_simple_new         (const char                  *name,
						  MateComponentMonikerSimpleResolveFn resolve_fn);

MateComponentMoniker *matecomponent_moniker_simple_new_closure (const char                  *name,
						  GClosure                    *resolve_closure);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_MONIKER_SIMPLE_H_ */

