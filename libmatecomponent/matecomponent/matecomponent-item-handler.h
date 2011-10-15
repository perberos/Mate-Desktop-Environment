/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-item-handler.h: a generic ItemContainer handler for monikers.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 * Copyright 1999, 2000 Miguel de Icaza
 */

#ifndef _MATECOMPONENT_ITEM_HANDLER_H_
#define _MATECOMPONENT_ITEM_HANDLER_H_


#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_ITEM_HANDLER        (matecomponent_item_handler_get_type ())
#define MATECOMPONENT_ITEM_HANDLER_TYPE        MATECOMPONENT_TYPE_ITEM_HANDLER /* deprecated, you should use MATECOMPONENT_TYPE_ITEM_HANDLER */
#define MATECOMPONENT_ITEM_HANDLER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_ITEM_HANDLER, MateComponentItemHandler))
#define MATECOMPONENT_ITEM_HANDLER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_ITEM_HANDLER, MateComponentItemHandlerClass))
#define MATECOMPONENT_IS_ITEM_HANDLER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_ITEM_HANDLER))
#define MATECOMPONENT_IS_ITEM_HANDLER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_ITEM_HANDLER))

typedef struct _MateComponentItemHandlerPrivate MateComponentItemHandlerPrivate;
typedef struct _MateComponentItemHandler        MateComponentItemHandler;

typedef MateComponent_ItemContainer_ObjectNames *(*MateComponentItemHandlerEnumObjectsFn)
	(MateComponentItemHandler *h, gpointer data, CORBA_Environment *);

typedef MateComponent_Unknown (*MateComponentItemHandlerGetObjectFn)
	(MateComponentItemHandler *h, const char *item_name, gboolean only_if_exists,
	 gpointer data, CORBA_Environment *ev);

struct _MateComponentItemHandler {
	MateComponentObject base;

	POA_MateComponent_ItemContainer__epv epv;

	MateComponentItemHandlerPrivate      *priv;
};

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_ItemContainer__epv epv;
} MateComponentItemHandlerClass;

GType                matecomponent_item_handler_get_type    (void) G_GNUC_CONST;
MateComponentItemHandler   *matecomponent_item_handler_new         (MateComponentItemHandlerEnumObjectsFn enum_objects,
						      MateComponentItemHandlerGetObjectFn   get_object,
						      gpointer                       user_data);

MateComponentItemHandler   *matecomponent_item_handler_new_closure (GClosure *enum_objects,
						      GClosure *get_object);

MateComponentItemHandler   *matecomponent_item_handler_construct   (MateComponentItemHandler *handler,
						      GClosure          *enum_objects,
						      GClosure          *get_object);

/* Utility functions that can be used by getObject routines */
typedef struct {
	char *key;
	char *value;
} MateComponentItemOption;

GSList *matecomponent_item_option_parse (const char *option_string);
void    matecomponent_item_options_free (GSList *options);

#ifdef __cplusplus
}
#endif

#endif
