#ifndef _MATECOMPONENT_UI_NODE_PRIVATE_H_
#define _MATECOMPONENT_UI_NODE_PRIVATE_H_

/* All this for xmlChar, xmlStrdup !? */
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <matecomponent/matecomponent-ui-node.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _MateComponentUINode {
	/* Tree management */
	MateComponentUINode *parent;
	MateComponentUINode *children;
	MateComponentUINode *prev;
	MateComponentUINode *next;

	/* The useful bits */
	GQuark        name_id;
	int           ref_count;
	xmlChar      *content;
	GArray       *attrs;
	gpointer      user_data;
};

typedef struct {
	GQuark        id;
	xmlChar      *value;
} MateComponentUIAttr;

gboolean      matecomponent_ui_node_try_set_attr   (MateComponentUINode *node,
					     GQuark        prop,
					     const char   *value);
void          matecomponent_ui_node_set_attr_by_id (MateComponentUINode *node,
					     GQuark        id,
					     const char   *value);
const char   *matecomponent_ui_node_get_attr_by_id (MateComponentUINode *node,
					     GQuark        id);
const char   *matecomponent_ui_node_peek_attr      (MateComponentUINode *node,
					     const char   *name);
const char   *matecomponent_ui_node_peek_content   (MateComponentUINode *node);
gboolean      matecomponent_ui_node_has_name_by_id (MateComponentUINode *node,
					     GQuark        id);
void          matecomponent_ui_node_add_after      (MateComponentUINode *before,
					     MateComponentUINode *new_after);
void          matecomponent_ui_node_move_children  (MateComponentUINode *from,
					     MateComponentUINode *to);
#define       matecomponent_ui_node_same_name(a,b) ((a)->name_id == (b)->name_id)
MateComponentUINode *matecomponent_ui_node_get_path_child (MateComponentUINode *node,
					     const char   *name);
MateComponentUINode *matecomponent_ui_node_ref            (MateComponentUINode *node);
void          matecomponent_ui_node_unref          (MateComponentUINode *node);


G_END_DECLS

#endif /* _MATECOMPONENT_UI_NODE_PRIVATE_H_ */
