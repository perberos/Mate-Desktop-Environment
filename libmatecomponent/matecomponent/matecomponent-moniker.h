/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-moniker: Object naming abstraction
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_MONIKER_H_
#define _MATECOMPONENT_MONIKER_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateComponentMonikerPrivate MateComponentMonikerPrivate;
typedef struct _MateComponentMoniker        MateComponentMoniker;

#define MATECOMPONENT_TYPE_MONIKER        (matecomponent_moniker_get_type ())
#define MATECOMPONENT_MONIKER_TYPE        MATECOMPONENT_TYPE_MONIKER /* deprecated, you should use MATECOMPONENT_TYPE_MONIKER */
#define MATECOMPONENT_MONIKER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_MONIKER, MateComponentMoniker))
#define MATECOMPONENT_MONIKER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_MONIKER, MateComponentMonikerClass))
#define MATECOMPONENT_IS_MONIKER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_MONIKER))
#define MATECOMPONENT_IS_MONIKER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_MONIKER))

struct _MateComponentMoniker {
        MateComponentObject         object;

	MateComponentMonikerPrivate *priv;
};

typedef struct {
	MateComponentObjectClass      parent_class;

	POA_MateComponent_Moniker__epv epv;

	/* virtual methods */
	MateComponent_Unknown (*resolve)            (MateComponentMoniker               *moniker,
					      const MateComponent_ResolveOptions *options,
					      const CORBA_char            *requested_interface,
					      CORBA_Environment           *ev);

	void           (*set_internal_name)  (MateComponentMoniker               *moniker,
					      const char                  *unescaped_name);
	const char    *(*get_internal_name)  (MateComponentMoniker               *moniker);

	gpointer        dummy;
} MateComponentMonikerClass;

GType          matecomponent_moniker_get_type           (void) G_GNUC_CONST;

MateComponentMoniker *matecomponent_moniker_construct          (MateComponentMoniker     *moniker,
						  const char        *prefix);

MateComponent_Moniker matecomponent_moniker_get_parent         (MateComponentMoniker     *moniker,
						  CORBA_Environment *opt_ev);

void           matecomponent_moniker_set_parent         (MateComponentMoniker     *moniker,
						  MateComponent_Moniker     parent,
						  CORBA_Environment *opt_ev);

const char    *matecomponent_moniker_get_name           (MateComponentMoniker     *moniker);

const char    *matecomponent_moniker_get_name_full      (MateComponentMoniker     *moniker);
char          *matecomponent_moniker_get_name_escaped   (MateComponentMoniker     *moniker);

void           matecomponent_moniker_set_name           (MateComponentMoniker     *moniker,
						  const char        *name);

const char    *matecomponent_moniker_get_prefix         (MateComponentMoniker     *moniker);

void           matecomponent_moniker_set_case_sensitive (MateComponentMoniker     *moniker,
						  gboolean           sensitive);
gboolean       matecomponent_moniker_get_case_sensitive (MateComponentMoniker     *moniker);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_MONIKER_H_ */
