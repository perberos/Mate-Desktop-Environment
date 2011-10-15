/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-engine.h: The MateComponent UI/XML Sync engine.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_ENGINE_H_
#define _MATECOMPONENT_UI_ENGINE_H_

#include <glib.h>
#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateComponentUIEngine MateComponentUIEngine;

#include <matecomponent/matecomponent-ui-container.h>

/* Various useful bits */
void matecomponent_ui_engine_deregister_dead_components     (MateComponentUIEngine *engine);
void matecomponent_ui_engine_deregister_component_by_ref    (MateComponentUIEngine *engine,
						      MateComponent_Unknown  ref);
void matecomponent_ui_engine_deregister_component           (MateComponentUIEngine *engine,
						      const char     *name);
void matecomponent_ui_engine_register_component             (MateComponentUIEngine *engine,
						      const char     *name,
						      MateComponent_Unknown  component);
GList         *matecomponent_ui_engine_get_component_names  (MateComponentUIEngine *engine);
MateComponent_Unknown matecomponent_ui_engine_get_component        (MateComponentUIEngine *engine,
						      const char     *name);

/* Configuration stuff */
void               matecomponent_ui_engine_config_set_path  (MateComponentUIEngine *engine,
						      const char     *path);
const char        *matecomponent_ui_engine_config_get_path  (MateComponentUIEngine *engine);

/* Misc. */
void               matecomponent_ui_engine_set_ui_container (MateComponentUIEngine    *engine,
						      MateComponentUIContainer *ui_container);
MateComponentUIContainer *matecomponent_ui_engine_get_ui_container (MateComponentUIEngine    *engine);


/* Used in Nautilus */
#ifndef MATECOMPONENT_UI_DISABLE_DEPRECATED
void matecomponent_ui_engine_freeze (MateComponentUIEngine *engine);
void matecomponent_ui_engine_thaw   (MateComponentUIEngine *engine);
void matecomponent_ui_engine_update (MateComponentUIEngine *engine);
#endif

#define MATECOMPONENT_TYPE_UI_ENGINE            (matecomponent_ui_engine_get_type ())
#define MATECOMPONENT_UI_ENGINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_ENGINE, MateComponentUIEngine))
#define MATECOMPONENT_UI_ENGINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_ENGINE, MateComponentUIEngineClass))
#define MATECOMPONENT_IS_UI_ENGINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_ENGINE))
#define MATECOMPONENT_IS_UI_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_ENGINE))

GType         matecomponent_ui_engine_get_type      (void) G_GNUC_CONST;

/* Private - implementation details */
#ifdef MATECOMPONENT_UI_INTERNAL

typedef enum {
	MATECOMPONENT_UI_ERROR_OK = 0,
	MATECOMPONENT_UI_ERROR_BAD_PARAM,
	MATECOMPONENT_UI_ERROR_INVALID_PATH,
	MATECOMPONENT_UI_ERROR_INVALID_XML
} MateComponentUIError;

#include <matecomponent/matecomponent-ui-sync.h>

typedef struct _MateComponentUIEnginePrivate MateComponentUIEnginePrivate;

struct _MateComponentUIEngine {
	GObject parent;

	MateComponentUIEnginePrivate *priv;
};

typedef struct {
	GObjectClass parent_class;

	/* Signals */
	void (*add_hint)      (MateComponentUIEngine *engine,
			       const char     *str);
	void (*remove_hint)   (MateComponentUIEngine *engine);

	void (*emit_verb_on)  (MateComponentUIEngine *engine,
			       MateComponentUINode   *node);

	void (*emit_event_on) (MateComponentUIEngine *engine,
			       MateComponentUINode   *node,
			       const char     *state);

	void (*destroy)       (MateComponentUIEngine *engine);

} MateComponentUIEngineClass;

MateComponentUIEngine *matecomponent_ui_engine_construct     (MateComponentUIEngine   *engine,
						GObject          *view);
MateComponentUIEngine *matecomponent_ui_engine_new           (GObject          *view);
GObject        *matecomponent_ui_engine_get_view      (MateComponentUIEngine   *engine);

void          matecomponent_ui_engine_add_sync        (MateComponentUIEngine   *engine,
						MateComponentUISync     *sync);
void          matecomponent_ui_engine_remove_sync     (MateComponentUIEngine   *engine,
						MateComponentUISync     *sync);
GSList       *matecomponent_ui_engine_get_syncs       (MateComponentUIEngine   *engine);

void          matecomponent_ui_engine_update_node     (MateComponentUIEngine   *engine,
						MateComponentUISync     *sync,
						MateComponentUINode     *node);
void          matecomponent_ui_engine_queue_update    (MateComponentUIEngine   *engine,
						GtkWidget        *widget,
						MateComponentUINode     *node,
						MateComponentUINode     *cmd_node);

GtkWidget    *matecomponent_ui_engine_build_control   (MateComponentUIEngine   *engine,
						MateComponentUINode     *node);

MateComponentUINode *matecomponent_ui_engine_widget_get_node   (GtkWidget        *widget);
void          matecomponent_ui_engine_widget_set_node   (MateComponentUIEngine   *engine,
						  GtkWidget        *widget,
						  MateComponentUINode     *node);
MateComponentUIError matecomponent_ui_engine_xml_set_prop      (MateComponentUIEngine   *engine,
						  const char       *path,
						  const char       *property,
						  const char       *value,
						  const char       *component);
CORBA_char   *matecomponent_ui_engine_xml_get_prop      (MateComponentUIEngine   *engine,
						  const char       *path,
						  const char       *prop,
						  gboolean         *invalid_path);
void          matecomponent_ui_engine_prune_widget_info (MateComponentUIEngine   *engine,
						  MateComponentUINode     *node,
						  gboolean          save_custom);

MateComponentUINode *matecomponent_ui_engine_get_path        (MateComponentUIEngine   *engine,
						const char       *path);
void          matecomponent_ui_engine_dirty_tree      (MateComponentUIEngine   *engine,
						MateComponentUINode     *node);
void          matecomponent_ui_engine_clean_tree      (MateComponentUIEngine   *engine,
						MateComponentUINode     *node);
void          matecomponent_ui_engine_dump            (MateComponentUIEngine   *engine,
						FILE             *out,
						const char       *msg);

/* Extra Node data accessors */
CORBA_Object  matecomponent_ui_engine_node_get_object (MateComponentUIEngine   *engine,
						MateComponentUINode     *node);
gboolean      matecomponent_ui_engine_node_is_dirty   (MateComponentUIEngine   *engine,
						MateComponentUINode     *node);
GtkWidget    *matecomponent_ui_engine_node_get_widget (MateComponentUIEngine   *engine,
						MateComponentUINode     *node);
const char   *matecomponent_ui_engine_node_get_id     (MateComponentUIEngine   *engine,
						MateComponentUINode     *node);
MateComponentUINode *matecomponent_ui_engine_get_cmd_node    (MateComponentUIEngine   *engine,
						MateComponentUINode     *from_node);
void          matecomponent_ui_engine_node_set_dirty  (MateComponentUIEngine   *engine,
						MateComponentUINode     *node,
						gboolean          dirty);
void          matecomponent_ui_engine_stamp_custom    (MateComponentUIEngine *engine,
						MateComponentUINode   *node);
void          matecomponent_ui_engine_widget_set      (MateComponentUIEngine    *engine,
						const char        *path,
						GtkWidget         *widget);
void          matecomponent_ui_engine_stamp_root      (MateComponentUIEngine *engine,
						MateComponentUINode   *node,
						GtkWidget      *widget);
/* Signal firers */
void          matecomponent_ui_engine_add_hint        (MateComponentUIEngine   *engine,
						const char       *str);
void          matecomponent_ui_engine_remove_hint     (MateComponentUIEngine   *engine);
void          matecomponent_ui_engine_emit_verb_on    (MateComponentUIEngine   *engine,
						MateComponentUINode     *node);
void          matecomponent_ui_engine_emit_event_on   (MateComponentUIEngine   *engine,
						MateComponentUINode     *node,
						const char       *state);
void          matecomponent_ui_engine_emit_verb_on_w  (MateComponentUIEngine   *engine,
						GtkWidget        *widget);
void          matecomponent_ui_engine_emit_event_on_w (MateComponentUIEngine   *engine,
						GtkWidget        *widget,
						const char       *state);


/* Helpers */
char         *matecomponent_ui_engine_get_attr           (MateComponentUINode     *node,
						   MateComponentUINode     *cmd_node,
						   const char       *attr);
void          matecomponent_ui_engine_widget_attach_node (GtkWidget        *widget,
						   MateComponentUINode     *node);


/* Interface used by UIContainer maps to MateComponentUIXml */
CORBA_char      *matecomponent_ui_engine_xml_get         (MateComponentUIEngine   *engine,
						   const char       *path,
						   gboolean          node_only);
gboolean         matecomponent_ui_engine_xml_node_exists (MateComponentUIEngine   *engine,
						   const char       *path);
MateComponentUIError    matecomponent_ui_engine_xml_merge_tree  (MateComponentUIEngine    *engine,
						   const char        *path,
						   MateComponentUINode      *tree,
						   const char        *component);
MateComponentUIError    matecomponent_ui_engine_xml_rm          (MateComponentUIEngine    *engine,
						   const char        *path,
						   const char        *by_component);
MateComponentUIError    matecomponent_ui_engine_object_set      (MateComponentUIEngine   *engine,
						   const char       *path,
						   MateComponent_Unknown    object,
						   CORBA_Environment *ev);
MateComponentUIError    matecomponent_ui_engine_object_get      (MateComponentUIEngine    *engine,
						   const char        *path,
						   MateComponent_Unknown    *object,
						   CORBA_Environment *ev);

void matecomponent_ui_engine_exec_verb (MateComponentUIEngine                    *engine,
				 const CORBA_char                  *cname,
				 CORBA_Environment                 *ev);
void matecomponent_ui_engine_ui_event  (MateComponentUIEngine                    *engine,
				 const CORBA_char                  *id,
				 const MateComponent_UIComponent_EventType type,
				 const CORBA_char                  *state,
				 CORBA_Environment                 *ev);

#endif /* MATECOMPONENT_UI_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_UI_ENGINE_H_ */
