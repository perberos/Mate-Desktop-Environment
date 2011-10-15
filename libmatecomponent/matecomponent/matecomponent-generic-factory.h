/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-generic-factory.h: a GenericFactory object.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *   ÉRDI Gergõ (cactus@cactus.rulez.org): cleanup
 *
 * Copyright 1999, 2001 Ximian, Inc., 2001 Gergõ Érdi
 */
#ifndef _MATECOMPONENT_GENERIC_FACTORY_H_
#define _MATECOMPONENT_GENERIC_FACTORY_H_


#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-i18n.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_GENERIC_FACTORY        (matecomponent_generic_factory_get_type ())
#define MATECOMPONENT_GENERIC_FACTORY_TYPE        MATECOMPONENT_TYPE_GENERIC_FACTORY /* deprecated, you should use MATECOMPONENT_TYPE_GENERIC_FACTORY */
#define MATECOMPONENT_GENERIC_FACTORY(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_GENERIC_FACTORY, MateComponentGenericFactory))
#define MATECOMPONENT_GENERIC_FACTORY_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_GENERIC_FACTORY, MateComponentGenericFactoryClass))
#define MATECOMPONENT_IS_GENERIC_FACTORY(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_GENERIC_FACTORY))
#define MATECOMPONENT_IS_GENERIC_FACTORY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_GENERIC_FACTORY))

typedef struct _MateComponentGenericFactoryPrivate MateComponentGenericFactoryPrivate;
typedef struct _MateComponentGenericFactory        MateComponentGenericFactory;

typedef MateComponentObject * (*MateComponentFactoryCallback) (MateComponentGenericFactory *factory,
						 const char           *component_id,
						 gpointer              closure);

struct _MateComponentGenericFactory {
	MateComponentObject                 base;

	MateComponentGenericFactoryPrivate *priv;
};

typedef struct {
	MateComponentObjectClass            parent_class;

	POA_MateComponent_GenericFactory__epv epv;

	MateComponentObject *(*new_generic) (MateComponentGenericFactory *factory,
				      const char           *act_iid);

} MateComponentGenericFactoryClass;

GType                 matecomponent_generic_factory_get_type        (void) G_GNUC_CONST;

MateComponentGenericFactory *matecomponent_generic_factory_new	     (const char           *act_iid,
							      MateComponentFactoryCallback factory_cb,
							      gpointer              user_data);

MateComponentGenericFactory *matecomponent_generic_factory_new_closure     (const char           *act_iid,
							      GClosure             *factory_closure);

MateComponentGenericFactory *matecomponent_generic_factory_construct	     (MateComponentGenericFactory *factory,
							      const char           *act_iid,
							      GClosure             *factory_closure);

void                  matecomponent_generic_factory_construct_noreg (MateComponentGenericFactory *factory,
							      const char           *act_iid,
							      GClosure             *factory_closure);

int                   matecomponent_generic_factory_main	     (const char           *act_iid,
							      MateComponentFactoryCallback factory_cb,
							      gpointer              user_data);
int                   matecomponent_generic_factory_main_timeout    (const char           *act_iid,
							      MateComponentFactoryCallback factory_cb,
							      gpointer              user_data,
							      guint                 quit_timeout);


#if defined (__MATECOMPONENT_UI_MAIN_H__) || defined (LIBMATECOMPONENTUI_H)
#define MATECOMPONENT_FACTORY_INIT(descr, version, argcp, argv)			\
	if (!matecomponent_ui_init (descr, version, argcp, argv))			\
		g_error ("Could not initialize MateComponent");
#else
#define MATECOMPONENT_FACTORY_INIT(desc, version, argcp, argv)				\
	if (!matecomponent_init (argcp, argv))						\
		g_error ("Could not initialize MateComponent");
#endif

#define MATECOMPONENT_OAF_FACTORY(oafiid, descr, version, callback, data)		\
	MATECOMPONENT_ACTIVATION_FACTORY(oafiid, descr, version, callback, data)
#define MATECOMPONENT_OAF_FACTORY_MULTI(oafiid, descr, version, callback, data)	\
	MATECOMPONENT_ACTIVATION_FACTORY(oafiid, descr, version, callback, data)

#define MATECOMPONENT_ACTIVATION_FACTORY(oafiid, descr, version, callback, data)	\
int main (int argc, char *argv [])						\
{										\
	g_thread_init (NULL);							\
										\
	MATECOMPONENT_FACTORY_INIT (descr, version, &argc, argv);			\
									        \
	return matecomponent_generic_factory_main (oafiid, callback, data);		\
}

#define MATECOMPONENT_ACTIVATION_FACTORY_TIMEOUT(oafiid, descr, version, callback, data, quit_timeout)	\
int main (int argc, char *argv [])								\
{												\
	g_thread_init (NULL);									\
												\
	MATECOMPONENT_FACTORY_INIT (descr, version, &argc, argv);					\
												\
	return matecomponent_generic_factory_main_timeout (oafiid, callback, data, quit_timeout);	\
}

#ifdef __cplusplus
}
#endif

#endif
