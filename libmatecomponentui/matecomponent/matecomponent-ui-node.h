/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-ui-node.h: Code to manipulate MateComponentUINode objects
 *
 * Author:
 *	Havoc Pennington <hp@redhat.com>
 *
 * Copyright 2000 Red Hat, Inc.
 */
#ifndef _MATECOMPONENT_UI_NODE_H_
#define _MATECOMPONENT_UI_NODE_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _MateComponentUINode MateComponentUINode;

MateComponentUINode *matecomponent_ui_node_new         (const char   *name);
MateComponentUINode *matecomponent_ui_node_new_child   (MateComponentUINode *parent,
                                          const char   *name);
MateComponentUINode *matecomponent_ui_node_copy        (MateComponentUINode *node,
					  gboolean      recursive);
void          matecomponent_ui_node_free        (MateComponentUINode *node);
void          matecomponent_ui_node_set_data    (MateComponentUINode *node,
                                          gpointer      data);
gpointer      matecomponent_ui_node_get_data    (MateComponentUINode *node);
void          matecomponent_ui_node_set_attr    (MateComponentUINode *node,
                                          const char   *name,
                                          const char   *value);
char *        matecomponent_ui_node_get_attr    (MateComponentUINode *node,
                                          const char   *name);
gboolean      matecomponent_ui_node_has_attr    (MateComponentUINode *node,
                                          const char   *name);
void          matecomponent_ui_node_remove_attr (MateComponentUINode *node,
                                          const char   *name);
void          matecomponent_ui_node_add_child   (MateComponentUINode *parent,
                                          MateComponentUINode *child);
void          matecomponent_ui_node_insert_before (MateComponentUINode *after,
					    MateComponentUINode *new_before);
void          matecomponent_ui_node_unlink      (MateComponentUINode *node);
void          matecomponent_ui_node_replace     (MateComponentUINode *old_node,
					  MateComponentUINode *new_node);
void          matecomponent_ui_node_set_content (MateComponentUINode *node,
                                          const char   *content);
char         *matecomponent_ui_node_get_content (MateComponentUINode *node);
MateComponentUINode *matecomponent_ui_node_next        (MateComponentUINode *node);
MateComponentUINode *matecomponent_ui_node_prev        (MateComponentUINode *node);
MateComponentUINode *matecomponent_ui_node_children    (MateComponentUINode *node);
MateComponentUINode *matecomponent_ui_node_parent      (MateComponentUINode *node);
const char   *matecomponent_ui_node_get_name    (MateComponentUINode *node);
gboolean      matecomponent_ui_node_has_name    (MateComponentUINode *node,
					  const char   *name);
gboolean      matecomponent_ui_node_transparent (MateComponentUINode *node);
void          matecomponent_ui_node_copy_attrs  (const MateComponentUINode *src,
					  MateComponentUINode *dest);

void          matecomponent_ui_node_free_string (char *str);
void          matecomponent_ui_node_strip       (MateComponentUINode **node);

char *        matecomponent_ui_node_to_string   (MateComponentUINode *node,
					  gboolean      recurse);
MateComponentUINode* matecomponent_ui_node_from_string (const char *str);
MateComponentUINode* matecomponent_ui_node_from_file   (const char *filename);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_NODE_H_ */
