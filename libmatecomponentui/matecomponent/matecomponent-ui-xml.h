/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-xml.h: A module for merging, overlaying and de-merging XML
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_UI_XML_H_
#define _MATECOMPONENT_UI_XML_H_

#include <glib-object.h>
#include <matecomponent/matecomponent-ui-node.h>
#include <matecomponent/matecomponent-ui-engine.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal API only */

#ifdef MATECOMPONENT_UI_INTERNAL

#define MATECOMPONENT_TYPE_UI_XML        (matecomponent_ui_xml_get_type ())
#define MATECOMPONENT_UI_XML(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_UI_XML, MateComponentUIXml))
#define MATECOMPONENT_UI_XML_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST ((k), MATECOMPONENT_TYPE_UI_XML, MateComponentUIXmlClass))
#define MATECOMPONENT_IS_UI_XML(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_UI_XML))
#define MATECOMPONENT_IS_UI_XML_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_UI_XML))

typedef struct _MateComponentUIXml MateComponentUIXml;

typedef struct {
	gpointer id;
	gboolean dirty;
	GSList  *overridden;
} MateComponentUIXmlData;

typedef gboolean         (*MateComponentUIXmlCompareFn)   (gpointer         id_a,
						    gpointer         id_b);
typedef MateComponentUIXmlData *(*MateComponentUIXmlDataNewFn)   (void);
typedef void             (*MateComponentUIXmlDataFreeFn)  (MateComponentUIXmlData *data);
typedef void             (*MateComponentUIXmlDumpFn)      (MateComponentUIXml      *tree,
						    MateComponentUINode     *node);
typedef void             (*MateComponentUIXmlAddNode)     (MateComponentUINode     *parent,
						    MateComponentUINode     *child,
						    gpointer          user_data);
typedef void             (*MateComponentUIXmlWatchFn)     (MateComponentUIXml      *xml,
						    const char       *path,
						    MateComponentUINode     *opt_node,
						    gpointer          user_data);

struct _MateComponentUIXml {
	GObject                object;

	MateComponentUIXmlCompareFn   compare;
	MateComponentUIXmlDataNewFn   data_new;
	MateComponentUIXmlDataFreeFn  data_free;
	MateComponentUIXmlDumpFn      dump;
	MateComponentUIXmlAddNode     add_node;
	MateComponentUIXmlWatchFn     watch;
	gpointer               user_data;

	MateComponentUINode          *root;

	GSList                *watches;
};

typedef struct {
	GObjectClass           object_klass;

	void                 (*override)          (MateComponentUINode *new_node,
						   MateComponentUINode *old_node);
	void                 (*replace_override)  (MateComponentUINode *new_node,
						   MateComponentUINode *old_node);
	void                 (*reinstate)         (MateComponentUINode *node);
	void                 (*rename)            (MateComponentUINode *node);
	void                 (*remove)            (MateComponentUINode *node);

	gpointer               dummy;
} MateComponentUIXmlClass;

GType          matecomponent_ui_xml_get_type          (void) G_GNUC_CONST;

MateComponentUIXml     *matecomponent_ui_xml_new               (MateComponentUIXmlCompareFn  compare,
						  MateComponentUIXmlDataNewFn  data_new,
						  MateComponentUIXmlDataFreeFn data_free,
						  MateComponentUIXmlDumpFn     dump,
						  MateComponentUIXmlAddNode    add_node,
						  gpointer              user_data);

/* Nominaly MateComponentUIXmlData * */
gpointer         matecomponent_ui_xml_get_data          (MateComponentUIXml  *tree,
						  MateComponentUINode *node);

void             matecomponent_ui_xml_set_dirty         (MateComponentUIXml  *tree,
						  MateComponentUINode *node);

void             matecomponent_ui_xml_clean             (MateComponentUIXml  *tree,
						  MateComponentUINode *node);

MateComponentUINode    *matecomponent_ui_xml_get_path          (MateComponentUIXml  *tree,
						  const char   *path);
MateComponentUINode    *matecomponent_ui_xml_get_path_wildcard (MateComponentUIXml  *tree,
						  const char   *path,
						  gboolean     *wildcard);

char            *matecomponent_ui_xml_make_path         (MateComponentUINode *node);
char            *matecomponent_ui_xml_get_parent_path   (const char   *path);

MateComponentUIError    matecomponent_ui_xml_merge             (MateComponentUIXml  *tree,
						  const char   *path,
						  MateComponentUINode *nodes,
						  gpointer      id);

MateComponentUIError    matecomponent_ui_xml_rm                (MateComponentUIXml  *tree,
						  const char   *path,
						  gpointer      id);

void             matecomponent_ui_xml_dump              (MateComponentUIXml  *tree,
						  MateComponentUINode *node,
						  const char   *msg);

void             matecomponent_ui_xml_set_watch_fn      (MateComponentUIXml  *tree,
						  MateComponentUIXmlWatchFn watch);

void             matecomponent_ui_xml_add_watch         (MateComponentUIXml  *tree,
						  const char   *path,
						  gpointer      user_data);

void             matecomponent_ui_xml_remove_watch_by_data (MateComponentUIXml  *tree,
						     gpointer      user_data);

#endif /* MATECOMPONENT_UI_INTERNAL */

G_END_DECLS

#endif /* _MATECOMPONENT_UI_XML_H_ */
