/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-engine-config.h: The MateComponent UI/XML Sync engine user config code
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2001 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_ENGINE_CONFIG_H_
#define _MATECOMPONENT_UI_ENGINE_CONFIG_H_

G_BEGIN_DECLS

#include <matecomponent/matecomponent-ui-xml.h>
#include <matecomponent/matecomponent-ui-engine.h>

#define MATECOMPONENT_TYPE_UI_ENGINE_CONFIG            (matecomponent_ui_engine_config_get_type ())
#define MATECOMPONENT_UI_ENGINE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_ENGINE_CONFIG, MateComponentUIEngineConfig))
#define MATECOMPONENT_UI_ENGINE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_ENGINE_CONFIG, MateComponentUIEngineConfigClass))
#define MATECOMPONENT_IS_UI_ENGINE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_ENGINE_CONFIG))
#define MATECOMPONENT_IS_UI_ENGINE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_ENGINE_CONFIG))

typedef struct _MateComponentUIEngineConfigPrivate MateComponentUIEngineConfigPrivate;

typedef struct {
	GObject parent;

	MateComponentUIEngineConfigPrivate *priv;
} MateComponentUIEngineConfig;

typedef struct {
	GObjectClass parent_class;

	gpointer       dummy;
} MateComponentUIEngineConfigClass;

GType matecomponent_ui_engine_config_get_type  (void) G_GNUC_CONST;

MateComponentUIEngineConfig *
        matecomponent_ui_engine_config_construct (MateComponentUIEngineConfig   *config,
					   MateComponentUIEngine         *engine,
					   GtkWindow              *opt_parent);

MateComponentUIEngineConfig *
        matecomponent_ui_engine_config_new       (MateComponentUIEngine         *engine,
					   GtkWindow              *opt_parent);

typedef char*(*MateComponentUIEngineConfigFn)    (MateComponentUIEngineConfig   *config,
					   MateComponentUINode           *config_node,
					   MateComponentUIEngine         *popup_engine);

typedef void (*MateComponentUIEngineConfigVerbFn)(MateComponentUIEngineConfig   *config,
					   const char             *path,
					   const char             *opt_state,
					   MateComponentUIEngine         *popup_engine,
					   MateComponentUINode           *popup_node);

void    matecomponent_ui_engine_config_connect   (GtkWidget                  *widget,
					   MateComponentUIEngine             *engine,
					   const char                 *path,
					   MateComponentUIEngineConfigFn      config_fn,
					   MateComponentUIEngineConfigVerbFn  verb_fn);
void    matecomponent_ui_engine_config_serialize (MateComponentUIEngineConfig   *config);
void    matecomponent_ui_engine_config_hydrate   (MateComponentUIEngineConfig   *config);
void    matecomponent_ui_engine_config_add       (MateComponentUIEngineConfig   *config,
					   const char             *path,
					   const char             *attr,
					   const char             *value);
void    matecomponent_ui_engine_config_remove    (MateComponentUIEngineConfig   *config,
					   const char             *path,
					   const char             *attr);

void    matecomponent_ui_engine_config_configure (MateComponentUIEngineConfig   *config);

MateComponentUIEngine *matecomponent_ui_engine_config_get_engine (MateComponentUIEngineConfig *config);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_ENGINE_CONFIG_H_ */



