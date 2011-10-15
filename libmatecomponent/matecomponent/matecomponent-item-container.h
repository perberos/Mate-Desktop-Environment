/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-item-container.h: a generic container for monikers.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_ITEM_CONTAINER_H_
#define _MATECOMPONENT_ITEM_CONTAINER_H_


#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-moniker.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_ITEM_CONTAINER        (matecomponent_item_container_get_type ())
#define MATECOMPONENT_ITEM_CONTAINER_TYPE        MATECOMPONENT_TYPE_ITEM_CONTAINER /* deprecated, you should use MATECOMPONENT_TYPE_ITEM_CONTAINER */
#define MATECOMPONENT_ITEM_CONTAINER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_ITEM_CONTAINER, MateComponentItemContainer))
#define MATECOMPONENT_ITEM_CONTAINER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_ITEM_CONTAINER, MateComponentItemContainerClass))
#define MATECOMPONENT_IS_ITEM_CONTAINER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_ITEM_CONTAINER))
#define MATECOMPONENT_IS_ITEM_CONTAINER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_ITEM_CONTAINER))


typedef struct _MateComponentItemContainerPrivate MateComponentItemContainerPrivate;
typedef struct _MateComponentItemContainer        MateComponentItemContainer;

struct _MateComponentItemContainer {
	MateComponentObject base;

	MateComponentItemContainerPrivate *priv;
};

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_ItemContainer__epv epv;

	MateComponent_Unknown (*get_object) (MateComponentItemContainer *item_container,
				      CORBA_char          *item_name,
				      CORBA_boolean        only_if_exists,
				      CORBA_Environment   *ev);
} MateComponentItemContainerClass;

GType                matecomponent_item_container_get_type       (void) G_GNUC_CONST;
MateComponentItemContainer *matecomponent_item_container_new            (void);

void                 matecomponent_item_container_add            (MateComponentItemContainer *container,
							   const char          *name,
							   MateComponentObject        *object);

void                 matecomponent_item_container_remove_by_name (MateComponentItemContainer *container,
							   const char          *name);

#ifdef __cplusplus
}
#endif

#endif

