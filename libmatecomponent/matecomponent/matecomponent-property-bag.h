/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-property-bag.h: property bag object implementation.
 *
 * Authors:
 *   Nat Friedman   (nat@ximian.com)
 *   Michael Meeks  (michael@ximian.com)
 *   Dietmar Maurer (dietmar@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef __MATECOMPONENT_PROPERTY_BAG_H__
#define __MATECOMPONENT_PROPERTY_BAG_H__

#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-arg.h>
#include <matecomponent/matecomponent-event-source.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_PROPERTY_READABLE      MateComponent_PROPERTY_READABLE
#define MATECOMPONENT_PROPERTY_WRITEABLE     MateComponent_PROPERTY_WRITEABLE
#define MATECOMPONENT_PROPERTY_WRITABLE      MateComponent_PROPERTY_WRITEABLE
#define MATECOMPONENT_PROPERTY_NO_LISTENING  MateComponent_PROPERTY_NO_LISTENING
#define MATECOMPONENT_PROPERTY_NO_AUTONOTIFY MateComponent_PROPERTY_NO_AUTONOTIFY

typedef struct _MateComponentPropertyBagPrivate MateComponentPropertyBagPrivate;
typedef struct _MateComponentPropertyBag        MateComponentPropertyBag;

typedef struct _MateComponentProperty           MateComponentProperty;
typedef struct _MateComponentPropertyPrivate    MateComponentPropertyPrivate;

typedef void (*MateComponentPropertyGetFn) (MateComponentPropertyBag *bag,
				     MateComponentArg         *arg,
				     guint              arg_id,
				     CORBA_Environment *ev,
				     gpointer           user_data);
typedef void (*MateComponentPropertySetFn) (MateComponentPropertyBag *bag,
				     const MateComponentArg   *arg,
				     guint              arg_id,
				     CORBA_Environment *ev,
				     gpointer           user_data);

struct _MateComponentProperty {
	char		      *name;
	int                    idx;
	MateComponentArgType          type;
	MateComponentArg             *default_value;
	char		      *doctitle;
	char		      *docstring;
	MateComponent_PropertyFlags   flags;

	MateComponentPropertyPrivate *priv;
};

struct _MateComponentPropertyBag {
	MateComponentObject             parent;
	MateComponentPropertyBagPrivate *priv;
	MateComponentEventSource        *es;
};

typedef struct {
	MateComponentObjectClass        parent;

	POA_MateComponent_PropertyBag__epv epv;
} MateComponentPropertyBagClass;

#define MATECOMPONENT_TYPE_PROPERTY_BAG        (matecomponent_property_bag_get_type ())
#define MATECOMPONENT_PROPERTY_BAG_TYPE        MATECOMPONENT_TYPE_PROPERTY_BAG /* deprecated, you should use MATECOMPONENT_TYPE_PROPERTY_BAG */
#define MATECOMPONENT_PROPERTY_BAG(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_PROPERTY_BAG, MateComponentPropertyBag))
#define MATECOMPONENT_PROPERTY_BAG_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_PROPERTY_BAG, MateComponentPropertyBagClass))
#define MATECOMPONENT_IS_PROPERTY_BAG(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_PROPERTY_BAG))
#define MATECOMPONENT_IS_PROPERTY_BAG_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_PROPERTY_BAG))

GType
matecomponent_property_bag_get_type  (void) G_GNUC_CONST;

MateComponentPropertyBag *
matecomponent_property_bag_new	           (MateComponentPropertyGetFn get_prop_cb,
			            MateComponentPropertySetFn set_prop_cb,
			            gpointer            user_data);

MateComponentPropertyBag *
matecomponent_property_bag_new_closure   (GClosure          *get_prop,
				   GClosure          *set_prop);

MateComponentPropertyBag *
matecomponent_property_bag_new_full      (GClosure          *get_prop,
				   GClosure          *set_prop,
				   MateComponentEventSource *es);

MateComponentPropertyBag *
matecomponent_property_bag_construct     (MateComponentPropertyBag *pb,
				   GClosure          *get_prop,
				   GClosure          *set_prop,
				   MateComponentEventSource *es);

void
matecomponent_property_bag_add           (MateComponentPropertyBag   *pb,
				   const char          *name,
				   int                  idx,
				   MateComponentArgType        type,
				   MateComponentArg           *default_value,
				   const char          *doctitle,
				   MateComponent_PropertyFlags flags);

void
matecomponent_property_bag_add_full      (MateComponentPropertyBag    *pb,
				   const char           *name,
				   int                   idx,
				   MateComponentArgType         type,
				   MateComponentArg            *default_value,
				   const char           *doctitle,
				   const char           *docstring,
				   MateComponent_PropertyFlags  flags,
				   GClosure             *get_prop,
				   GClosure             *set_prop);

void
matecomponent_property_bag_remove        (MateComponentPropertyBag    *pb,
				   const char           *name);

void
matecomponent_property_bag_map_params    (MateComponentPropertyBag   *pb,
				   GObject             *on_instance,
				   const GParamSpec   **pspecs,
				   guint                n_params);

GList *
matecomponent_property_bag_get_prop_list (MateComponentPropertyBag *pb);

#ifdef __cplusplus
}
#endif

#endif /* ! __MATECOMPONENT_PROPERTY_BAG_H__ */
