/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-engine-private.h: Private MateComponent UI/XML Sync engine bits
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2001 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_ENGINE_PRIVATE_H_
#define _MATECOMPONENT_UI_ENGINE_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <matecomponent/matecomponent-ui-xml.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-engine-config.h>


struct _MateComponentUIEnginePrivate {
	GObject      *view;

	MateComponentUIXml  *tree;
	int           frozen;
	GSList       *syncs;
	GSList       *state_updates;
	GSList       *components;

	MateComponentUIContainer    *container;
	MateComponentUIEngineConfig *config;

	GHashTable   *cmd_to_node;
};

MateComponentUIXml          *matecomponent_ui_engine_get_xml    (MateComponentUIEngine *engine);
MateComponentUIEngineConfig *matecomponent_ui_engine_get_config (MateComponentUIEngine *engine);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_ENGINE_H_ */
