/**
 * matecomponent-config-bag.h: config bag object implementation.
 *
 * Author:
 *   Dietmar Maurer  (dietmar@ximian.com)
 *   Rodrigo Moya    (rodrigo@ximian.com)
 *
 * Copyright 2000, 2001 Ximian, Inc.
 */
#ifndef __MATECOMPONENT_CONFIG_BAG_H__
#define __MATECOMPONENT_CONFIG_BAG_H__

#include <mateconf/mateconf-client.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-event-source.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_CONFIG_BAG        (matecomponent_config_bag_get_type ())
#define MATECOMPONENT_CONFIG_BAG_TYPE        MATECOMPONENT_TYPE_CONFIG_BAG // deprecated, you should use MATECOMPONENT_TYPE_CONFIG_BAG
#define MATECOMPONENT_CONFIG_BAG(o)	      (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_CONFIG_BAG, MateComponentConfigBag))
#define MATECOMPONENT_CONFIG_BAG_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST ((k), MATECOMPONENT_TYPE_CONFIG_BAG, MateComponentConfigBagClass))
#define MATECOMPONENT_IS_CONFIG_BAG(o)	      (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_CONFIG_BAG))
#define MATECOMPONENT_IS_CONFIG_BAG_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), MATECOMPONENT_TYPE_CONFIG_BAG))

typedef struct _MateComponentConfigBag        MateComponentConfigBag;

struct _MateComponentConfigBag {
	MateComponentObject          base;

	gchar                 *path;
	MateComponentEventSource     *es;
	MateConfClient           *conf_client;
};

typedef struct {
	MateComponentObjectClass  parent_class;

	POA_MateComponent_PropertyBag__epv epv;

} MateComponentConfigBagClass;


GType		  matecomponent_config_bag_get_type  (void);
MateComponentConfigBag	 *matecomponent_config_bag_new	      (const gchar *path);

#ifdef __cplusplus
}
#endif

#endif /* ! __MATECOMPONENT_CONFIG_BAG_H__ */
