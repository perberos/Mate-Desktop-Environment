/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-shlib-factory.h: a ShlibFactory object.
 *
 * Author:
 *    Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, 2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_SHLIB_FACTORY_H_
#define _MATECOMPONENT_SHLIB_FACTORY_H_


#include <glib-object.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-generic-factory.h>
#include <matecomponent/matecomponent-exception.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_SHLIB_FACTORY        (matecomponent_shlib_factory_get_type ())
#define MATECOMPONENT_SHLIB_FACTORY_TYPE        MATECOMPONENT_TYPE_SHLIB_FACTORY /* deprecated, you should use MATECOMPONENT_TYPE_SHLIB_FACTORY */
#define MATECOMPONENT_SHLIB_FACTORY(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_SHLIB_FACTORY, MateComponentShlibFactory))
#define MATECOMPONENT_SHLIB_FACTORY_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_SHLIB_FACTORY, MateComponentShlibFactoryClass))
#define MATECOMPONENT_IS_SHLIB_FACTORY(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_SHLIB_FACTORY))
#define MATECOMPONENT_IS_SHLIB_FACTORY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_SHLIB_FACTORY))

typedef struct _MateComponentShlibFactoryPrivate MateComponentShlibFactoryPrivate;
typedef struct _MateComponentShlibFactory        MateComponentShlibFactory;

struct _MateComponentShlibFactory {
	MateComponentGenericFactory base;

	MateComponentShlibFactoryPrivate *priv;
};

typedef struct {
	MateComponentGenericFactoryClass parent_class;
} MateComponentShlibFactoryClass;

GType               matecomponent_shlib_factory_get_type     (void) G_GNUC_CONST;

MateComponentShlibFactory *matecomponent_shlib_factory_construct    (MateComponentShlibFactory    *factory,
						       const char            *act_iid,
						       PortableServer_POA     poa,
						       gpointer               act_impl_ptr,
						       GClosure              *closure);

MateComponentShlibFactory *matecomponent_shlib_factory_new          (const char            *component_id,
						       PortableServer_POA     poa,
						       gpointer               act_impl_ptr,
						       MateComponentFactoryCallback  factory_cb,
						       gpointer               user_data);

MateComponentShlibFactory *matecomponent_shlib_factory_new_closure  (const char            *act_iid,
						       PortableServer_POA     poa,
						       gpointer               act_impl_ptr,
						       GClosure              *factory_closure);

MateComponent_Unknown      matecomponent_shlib_factory_std          (const char            *component_id,
						       PortableServer_POA     poa,
						       gpointer               act_impl_ptr,
						       MateComponentFactoryCallback  factory_cb,
						       gpointer               user_data,
						       CORBA_Environment     *ev);

#define MATECOMPONENT_OAF_SHLIB_FACTORY(oafiid, descr, fn, data)                     \
	MATECOMPONENT_ACTIVATION_SHLIB_FACTORY(oafiid, descr, fn, data)
#define MATECOMPONENT_OAF_SHLIB_FACTORY_MULTI(oafiid, descr, fn, data)               \
	MATECOMPONENT_ACTIVATION_SHLIB_FACTORY(oafiid, descr, fn, data)

#define MATECOMPONENT_ACTIVATION_SHLIB_FACTORY(oafiid, descr, fn, data)	      \
static MateComponent_Unknown                                                         \
make_factory (PortableServer_POA poa, const char *iid, gpointer impl_ptr,     \
	      CORBA_Environment *ev)                                          \
{                                                                             \
	return matecomponent_shlib_factory_std (oafiid, poa, impl_ptr, fn, data, ev);\
}                                                                             \
static MateComponentActivationPluginObject plugin_list[] = {{oafiid, make_factory}, { NULL } };   \
const  MateComponentActivationPlugin MateComponent_Plugin_info = { plugin_list, descr };

#ifdef __cplusplus
}
#endif

#endif
