/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-ui-preferences.h: private wrappers for global UI preferences.
 *
 * Authors:
 *     Michael Meeks (michael@helixcode.com)
 *     Martin Baulig (martin@home-of-linux.org)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_UI_PREFERENCES_H_
#define _MATECOMPONENT_UI_PREFERENCES_H_

#include <glib.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-toolbar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_UI_PAD          8
#define MATECOMPONENT_UI_PAD_SMALL    4
#define MATECOMPONENT_UI_PAD_BIG      12

/* Add a UI engine to the configuration list */
void matecomponent_ui_preferences_add_engine    (MateComponentUIEngine *engine);
void matecomponent_ui_preferences_remove_engine (MateComponentUIEngine *engine);

/* Default toolbar style */
MateComponentUIToolbarStyle matecomponent_ui_preferences_get_toolbar_style (void);
/* Whether menus have icons */
gboolean matecomponent_ui_preferences_get_menus_have_icons   (void);
/* Whether menus can be torn off */
gboolean matecomponent_ui_preferences_get_menus_have_tearoff (void);
/* Whether menubars can be detached */
gboolean matecomponent_ui_preferences_get_menubar_detachable (void);
/* Whether a per-app toolbar style has been specified */
gboolean matecomponent_ui_preferences_get_toolbar_detachable (void);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_UI_PREFERENCES_H_ */

