/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-sync.h: An abstract base for MateComponent XML / widget sync sync'ing.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_SYNC_H_
#define _MATECOMPONENT_UI_SYNC_H_

/* Internal API */
#ifdef MATECOMPONENT_UI_INTERNAL

#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <matecomponent/matecomponent-ui-node.h>

typedef struct _MateComponentUISync MateComponentUISync;

#include <matecomponent/matecomponent-ui-engine.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_SYNC            (matecomponent_ui_sync_get_type ())
#define MATECOMPONENT_UI_SYNC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_SYNC, MateComponentUISync))
#define MATECOMPONENT_UI_SYNC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_SYNC, MateComponentUISyncClass))
#define MATECOMPONENT_IS_UI_SYNC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_SYNC))
#define MATECOMPONENT_IS_UI_SYNC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_SYNC))

typedef void (*MateComponentUISyncStateFn)         (MateComponentUISync     *sync,
					     MateComponentUINode     *node,
					     MateComponentUINode     *cmd_node,
					     GtkWidget        *widget,
					     GtkWidget        *parent);

typedef GtkWidget *(*MateComponentUISyncBuildFn)   (MateComponentUISync     *sync,
					     MateComponentUINode     *node,
					     MateComponentUINode     *cmd_node,
					     int              *pos,
					     GtkWidget        *parent);

typedef struct _MateComponentUISyncPrivate MateComponentUISyncPrivate;

struct _MateComponentUISync {
	GObject parent;

	MateComponentUIEngine *engine;
	gboolean        is_recursive;
	gboolean        has_widgets;

	MateComponentUISyncPrivate *priv;
};

typedef struct {
	GObjectClass parent_class;

	MateComponentUISyncStateFn sync_state;
	MateComponentUISyncStateFn sync_state_placeholder;
	MateComponentUISyncBuildFn build;
	MateComponentUISyncBuildFn build_placeholder;

	void          (*update_root)     (MateComponentUISync     *sync,
					  MateComponentUINode     *root);

	void          (*remove_root)     (MateComponentUISync     *sync,
					  MateComponentUINode     *root);

	GList        *(*get_widgets)     (MateComponentUISync     *sync,
					  MateComponentUINode     *node);

	void          (*state_update)    (MateComponentUISync     *sync,
					  GtkWidget        *widget,
					  const char       *new_state);

	gboolean      (*ignore_widget)   (MateComponentUISync     *sync,
					  GtkWidget        *widget);

	gboolean      (*can_handle)      (MateComponentUISync     *sync,
					  MateComponentUINode     *node);

        void          (*stamp_root)      (MateComponentUISync     *sync);

	GtkWidget    *(*get_attached)    (MateComponentUISync     *sync,
					  GtkWidget        *widget,
					  MateComponentUINode     *node);

	GtkWidget    *(*wrap_widget)     (MateComponentUISync     *sync,
					  GtkWidget        *custom_widget);
} MateComponentUISyncClass;

GType      matecomponent_ui_sync_get_type           (void) G_GNUC_CONST;
MateComponentUISync *matecomponent_ui_sync_construct       (MateComponentUISync     *sync,
					      MateComponentUIEngine   *engine,
					      gboolean          is_recursive,
					      gboolean          has_widgets);

gboolean   matecomponent_ui_sync_is_recursive       (MateComponentUISync     *sync);
gboolean   matecomponent_ui_sync_has_widgets        (MateComponentUISync     *sync);

void       matecomponent_ui_sync_remove_root        (MateComponentUISync     *sync,
					      MateComponentUINode     *root);
void       matecomponent_ui_sync_update_root        (MateComponentUISync     *sync,
					      MateComponentUINode     *root);

void       matecomponent_ui_sync_state              (MateComponentUISync     *sync,
					      MateComponentUINode     *node,
					      MateComponentUINode     *cmd_node,
					      GtkWidget        *widget,
					      GtkWidget        *parent);
void       matecomponent_ui_sync_state_placeholder  (MateComponentUISync     *sync,
					      MateComponentUINode     *node,
					      MateComponentUINode     *cmd_node,
					      GtkWidget        *widget,
					      GtkWidget        *parent);

GtkWidget *matecomponent_ui_sync_build              (MateComponentUISync     *sync,
					      MateComponentUINode     *node,
					      MateComponentUINode     *cmd_node,
					      int              *pos,
					      GtkWidget        *parent);
GtkWidget *matecomponent_ui_sync_build_placeholder  (MateComponentUISync     *sync,
					      MateComponentUINode     *node,
					      MateComponentUINode     *cmd_node,
					      int              *pos,
					      GtkWidget        *parent);

gboolean   matecomponent_ui_sync_ignore_widget      (MateComponentUISync     *sync,
					      GtkWidget        *widget);

GList     *matecomponent_ui_sync_get_widgets        (MateComponentUISync     *sync,
					      MateComponentUINode     *node);

void       matecomponent_ui_sync_stamp_root         (MateComponentUISync     *sync);

gboolean   matecomponent_ui_sync_can_handle         (MateComponentUISync     *sync,
					      MateComponentUINode     *node);

GtkWidget *matecomponent_ui_sync_get_attached       (MateComponentUISync     *sync,
					      GtkWidget        *widget,
					      MateComponentUINode     *node);

void       matecomponent_ui_sync_state_update       (MateComponentUISync     *sync,
					      GtkWidget        *widget,
					      const char       *new_state);

gboolean   matecomponent_ui_sync_do_show_hide       (MateComponentUISync     *sync,
					      MateComponentUINode     *node,
					      MateComponentUINode     *cmd_node,
					      GtkWidget        *widget);

GtkWidget *matecomponent_ui_sync_wrap_widget        (MateComponentUISync     *sync,
					      GtkWidget        *custom_widget);

/*
 *  These are to allow you to remove certain types of Sync
 * from a matecomponent-window to allow full sub-classing of that.
 * Do not use to instantiate syncs manualy or to sub-class.
 */
GType matecomponent_ui_sync_keys_get_type    (void);
GType matecomponent_ui_sync_menu_get_type    (void);
GType matecomponent_ui_sync_status_get_type  (void);
GType matecomponent_ui_sync_toolbar_get_type (void);

G_END_DECLS

#endif /* MATECOMPONENT_UI_INTERNAL */

#endif /* _MATECOMPONENT_UI_SYNC_H_ */
