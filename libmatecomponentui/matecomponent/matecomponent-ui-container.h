/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-container.h: The server side CORBA impl. for MateComponentWindow.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_UI_CONTAINER_H_
#define _MATECOMPONENT_UI_CONTAINER_H_

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateComponentUIContainer MateComponentUIContainer;

#include <matecomponent/matecomponent-ui-engine.h>

#define MATECOMPONENT_TYPE_UI_CONTAINER        (matecomponent_ui_container_get_type ())
#define MATECOMPONENT_UI_CONTAINER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_UI_CONTAINER, MateComponentUIContainer))
#define MATECOMPONENT_UI_CONTAINER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST ((k), MATECOMPONENT_TYPE_UI_CONTAINER, MateComponentUIContainerClass))
#define MATECOMPONENT_IS_UI_CONTAINER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_UI_CONTAINER))
#define MATECOMPONENT_IS_UI_CONTAINER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_UI_CONTAINER))

typedef struct _MateComponentUIContainerPrivate MateComponentUIContainerPrivate;

struct _MateComponentUIContainer {
	MateComponentObject base;

	MateComponentUIContainerPrivate *priv;
};

typedef struct {
	MateComponentObjectClass parent;

	POA_MateComponent_UIContainer__epv epv;

	gpointer dummy[2];
} MateComponentUIContainerClass;

GType                        matecomponent_ui_container_get_type            (void) G_GNUC_CONST;
MateComponentUIContainer           *matecomponent_ui_container_new                 (void);

void                         matecomponent_ui_container_set_engine          (MateComponentUIContainer  *container,
								      MateComponentUIEngine     *engine);
MateComponentUIEngine              *matecomponent_ui_container_get_engine          (MateComponentUIContainer  *container);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_CONTAINER_H_ */
