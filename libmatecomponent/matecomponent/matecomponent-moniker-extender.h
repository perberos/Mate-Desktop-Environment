/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-moniker-extender: extending monikers
 *
 * Author:
 *	Dietmar Maurer (dietmar@maurer-it.com)
 *
 * Copyright 2000, Dietmar Maurer.
 */
#ifndef _MATECOMPONENT_MONIKER_EXTENDER_H_
#define _MATECOMPONENT_MONIKER_EXTENDER_H_

#include <matecomponent/matecomponent-moniker.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_MONIKER_EXTENDER        (matecomponent_moniker_extender_get_type ())
#define MATECOMPONENT_MONIKER_EXTENDER_TYPE        MATECOMPONENT_TYPE_MONIKER_EXTENDER /* deprecated, you should use MATECOMPONENT_TYPE_MONIKER_EXTENDER */
#define MATECOMPONENT_MONIKER_EXTENDER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_MONIKER_EXTENDER, MateComponentMonikerExtender))
#define MATECOMPONENT_MONIKER_EXTENDER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_MONIKER_EXTENDER, MateComponentMonikerExtenderClass))
#define MATECOMPONENT_IS_MONIKER_EXTENDER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_MONIKER_EXTENDER))
#define MATECOMPONENT_IS_MONIKER_EXTENDER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_MONIKER_EXTENDER))

typedef struct _MateComponentMonikerExtender MateComponentMonikerExtender;

typedef MateComponent_Unknown (*MateComponentMonikerExtenderFn) (MateComponentMonikerExtender       *extender,
						   const MateComponent_Moniker         parent,
						   const MateComponent_ResolveOptions *options,
						   const CORBA_char            *display_name,
						   const CORBA_char            *requested_interface,
						   CORBA_Environment           *ev);
struct _MateComponentMonikerExtender {
        MateComponentObject           object;
	MateComponentMonikerExtenderFn resolve;
	gpointer                data;
};

typedef struct {
	MateComponentObjectClass      parent_class;

	POA_MateComponent_MonikerExtender__epv epv;

	MateComponentMonikerExtenderFn resolve;
} MateComponentMonikerExtenderClass;

GType                  matecomponent_moniker_extender_get_type (void) G_GNUC_CONST;
MateComponentMonikerExtender *matecomponent_moniker_extender_new      (MateComponentMonikerExtenderFn      resolve,
							 gpointer                     data);

MateComponent_MonikerExtender matecomponent_moniker_find_extender     (const gchar                 *name,
							 const gchar                 *interface,
							 CORBA_Environment           *opt_ev);

MateComponent_Unknown         matecomponent_moniker_use_extender      (const gchar                 *extender_oafiid,
							 MateComponentMoniker               *moniker,
							 const MateComponent_ResolveOptions *options,
							 const CORBA_char            *requested_interface,
							 CORBA_Environment           *opt_ev);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_MONIKER_EXTENDER_H_ */
