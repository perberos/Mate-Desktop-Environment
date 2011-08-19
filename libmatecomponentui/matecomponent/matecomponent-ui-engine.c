/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * matecomponent-ui-engine.c: The MateComponent UI/XML Sync engine.
 *
 * Author:
 *	Michael Meeks (michael@ximian.com)
 *
 * Copyright 2000,2001 Ximian, Inc.
 */

/* FIXME: matecomponent_ui_engine_update should take
 * a MateComponentUINode *, which we can walk up from
 * looking for cleanliness & then re-building
 * from there on down */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include <matecomponent/matecomponent-widget.h>
#include <matecomponent/matecomponent-ui-xml.h>
#include <glib/gi18n-lib.h>
#include <matecomponent/matecomponent-ui-util.h>
#include <matecomponent/matecomponent-ui-container.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-engine-config.h>
#include <matecomponent/matecomponent-ui-engine-private.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-ui-marshal.h>
#include <matecomponent/matecomponent-ui-preferences.h>

/* only for embedded widgets */
#include <matecomponent/matecomponent-ui-toolbar-item.h>

#include <matecomponent/matecomponent-ui-node-private.h>

#ifdef LOWLEVEL_DEBUG
#  define ACCESS(str) access (str, 0)
#else
#  define ACCESS(str)
#endif

/* Various debugging output defines */
#undef STATE_SYNC_DEBUG
#undef WIDGET_SYNC_DEBUG
#undef XML_MERGE_DEBUG

#define PARENT_TYPE G_TYPE_OBJECT

static GObjectClass *parent_class = NULL;

static GQuark id_id        = 0;
static GQuark cmd_id       = 0;
static GQuark verb_id      = 0;
static GQuark name_id      = 0;
static GQuark state_id     = 0;
static GQuark hidden_id    = 0;
static GQuark commands_id  = 0;
static GQuark sensitive_id = 0;

enum {
	ADD_HINT,
	REMOVE_HINT,
	EMIT_VERB_ON,
	EMIT_EVENT_ON,
	DESTROY,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

static gint matecomponent_ui_engine_inhibit_events = 0;

static void
add_debug_menu (MateComponentUIEngine *engine)
{
	char *xml;
	MateComponentUINode *node;

	xml = g_strdup_printf (
		"<menu>"
		"  <submenu name=\"MateComponentDebug\" label=\"%s\">"
		"      <menuitem name=\"MateComponentUIDump\" verb=\"\""
		"       label=\"%s\" tip=\"%s\"/>"
		"  </submenu>"
		"</menu>", _("Debug"), _("_Dump XML"),
		_("Dump the entire UI's XML description to the console"));

	node = matecomponent_ui_node_from_string (xml);
	matecomponent_ui_engine_xml_merge_tree (engine, "/", node, "BuiltIn");
	g_free (xml);
}

/*
 *  Mapping from nodes to their synchronization
 * class information functions.
 */

static MateComponentUISync *
find_sync_for_node (MateComponentUIEngine *engine,
		    MateComponentUINode   *node)
{
	GSList       *l;
	MateComponentUISync *ret = NULL;

	if (!node)
		return NULL;

	if (node->name_id == cmd_id ||
	    node->name_id == commands_id)
		return NULL;

	for (l = engine->priv->syncs; l; l = l->next) {
		if (matecomponent_ui_sync_can_handle (l->data, node)) {
			ret = l->data;
			break;
		}
	}

	if (ret) {
/*		fprintf (stderr, "Found sync '%s' for path '%s'\n",
			 gtk_type_name (G_TYPE_FROM_CLASS (GTK_OBJECT_GET_CLASS (ret))),
			 matecomponent_ui_xml_make_path (node));*/
		return ret;
	}

	return find_sync_for_node (engine, node->parent);
}

/**
 * matecomponent_ui_engine_add_sync:
 * @engine: the enginer
 * @sync: the synchronizer
 * 
 * Add a #MateComponentUISync synchronizer to the engine
 **/
void
matecomponent_ui_engine_add_sync (MateComponentUIEngine *engine,
			   MateComponentUISync   *sync)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if (g_slist_find (engine->priv->syncs, sync))
		g_warning ("Already added this Synchronizer %p", sync);
	else
		engine->priv->syncs = g_slist_append (
			engine->priv->syncs, sync);
}

/**
 * matecomponent_ui_engine_remove_sync:
 * @engine: the engine
 * @sync: the sync
 * 
 * Remove a specified #MateComponentUISync synchronizer from the engine
 **/
void
matecomponent_ui_engine_remove_sync (MateComponentUIEngine *engine,
			      MateComponentUISync   *sync)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	engine->priv->syncs = g_slist_remove (
		engine->priv->syncs, sync);
}

/**
 * matecomponent_ui_engine_get_syncs:
 * @engine: the engine
 * 
 * Retrieve a list of available synchronizers.
 * 
 * Return value: a GSList of #MateComponentUISync s
 **/
GSList *
matecomponent_ui_engine_get_syncs (MateComponentUIEngine *engine)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	return g_slist_copy (engine->priv->syncs);
}

/*
 * Cmd -> Node mapping functionality.
 */

typedef struct {
	char   *name;
	GSList *nodes;
} CmdToNode;

static const char *
node_get_id (MateComponentUINode *node)
{
	const char *txt;

	g_return_val_if_fail (node != NULL, NULL);

	if (!(txt = matecomponent_ui_node_get_attr_by_id (node, id_id))) {

		txt = matecomponent_ui_node_get_attr_by_id (node, verb_id);

		if (txt && txt [0] == '\0')
			txt = matecomponent_ui_node_get_attr_by_id (node, name_id);
	}

	return txt;
}

static void
cmd_to_node_add_node (MateComponentUIEngine *engine,
		      MateComponentUINode   *node,
		      gboolean        recurse)
{
	CmdToNode  *ctn;
	const char *name;

	if (recurse) {
		MateComponentUINode *l;

		for (l = node->children; l; l = l->next)
			cmd_to_node_add_node (engine, l, TRUE);
	}

	name = node_get_id (node);
	if (!name)
		return;

	ctn = g_hash_table_lookup (
		engine->priv->cmd_to_node, (gpointer) name);

	if (!ctn) {
		ctn = g_new (CmdToNode, 1);

		ctn->name = g_strdup (name);
		ctn->nodes = NULL;
		g_hash_table_insert (
			engine->priv->cmd_to_node, ctn->name, ctn);
	}

/*	fprintf (stderr, "Adding %d'th '%s'\n",
	g_slist_length (ctn->nodes), ctn->name);*/

	ctn->nodes = g_slist_prepend (ctn->nodes, node);
}

static void
cmd_to_node_remove_node (MateComponentUIEngine *engine,
			 MateComponentUINode   *node,
			 gboolean        recurse)
{
	CmdToNode  *ctn;
	const char *name;

	if (recurse) {
		MateComponentUINode *l;

		for (l = node->children; l; l = l->next)
			cmd_to_node_remove_node (engine, l, TRUE);
	}

	name = node_get_id (node);
	if (!name)
		return;

	ctn = g_hash_table_lookup (
		engine->priv->cmd_to_node, name);

	if (!ctn)
		g_warning ("Removing non-registered name '%s'", name);
	else {
/*		fprintf (stderr, "Removing %d'th '%s'\n",
		g_slist_length (ctn->nodes), name);*/
		ctn->nodes = g_slist_remove (ctn->nodes, node);
	}

	/*
	 * NB. we leave the CmdToNode structures around
	 * for future use.
	 */
}

static int
cmd_to_node_clear_hash (gpointer key,
			gpointer value,
			gpointer user_data)
{
	CmdToNode *ctn = value;

	g_free (ctn->name);
	g_slist_free (ctn->nodes);
	g_free (ctn);

	return TRUE;
}

static const GSList *
cmd_to_nodes (MateComponentUIEngine *engine,
	      const char     *name)
{
	CmdToNode *ctn;

	if (!name)
		return NULL;

	ctn = g_hash_table_lookup (
		engine->priv->cmd_to_node, name);

	return ctn ? ctn->nodes : NULL;
}

#define NODE_IS_ROOT_WIDGET(n)   ((n->type & ROOT_WIDGET) != 0)
#define NODE_IS_CUSTOM_WIDGET(n) ((n->type & CUSTOM_WIDGET) != 0)

typedef enum {
	ROOT_WIDGET   = 0x1,
	CUSTOM_WIDGET = 0x2
} NodeType;

typedef struct {
	MateComponentUIXmlData parent;

	int             type;
	GtkWidget      *widget;
	MateComponent_Unknown  object;
} NodeInfo;

/*
 * MateComponentUIXml impl functions
 */

static MateComponentUIXmlData *
info_new_fn (void)
{
	NodeInfo *info = g_new0 (NodeInfo, 1);

	info->object = CORBA_OBJECT_NIL;

	return (MateComponentUIXmlData *) info;
}

static void
widget_unref (GtkWidget **ref)
{
	GtkWidget *w;

	g_return_if_fail (ref != NULL);
	
	if ((w = *ref)) {
		*ref = NULL;
		g_object_unref (w);
	}
}

static void
info_free_fn (MateComponentUIXmlData *data)
{
	NodeInfo *info = (NodeInfo *) data;

	if (info->object != CORBA_OBJECT_NIL) {
		dbgprintf ("** Releasing object %p on info %p\n", info->object, info);
		matecomponent_object_release_unref (info->object, NULL);
		info->object = CORBA_OBJECT_NIL;
	}

	widget_unref (&info->widget);

	g_free (data);
}

static void
info_dump_fn (MateComponentUIXml *tree, MateComponentUINode *node)
{
	NodeInfo *info = matecomponent_ui_xml_get_data (tree, node);

	if (info) {
		fprintf (stderr, " '%15s' object %8p type %d ",
			 (char *)info->parent.id, info->object, info->type);

		if (info->widget) {
			MateComponentUINode *attached_node = 
				matecomponent_ui_engine_widget_get_node (info->widget);

			fprintf (stderr, "widget '%8p' with node '%8p' attached ",
				 info->widget, attached_node);

			if (attached_node == NULL)
				fprintf (stderr, "is NULL\n");

			else if (attached_node != node)
				fprintf (stderr, "Serious mismatch attaches should be '%8p'\n",
					 node);
			else {
				if (info->widget->parent)
					fprintf (stderr, "and matching; parented\n");
				else
					fprintf (stderr, "and matching; BUT NO PARENT!\n");
			}
 		} else
			fprintf (stderr, " no associated widget\n");
	} else
		fprintf (stderr, " very weird no data on node '%p'\n", node);
}

static void
add_node_fn (MateComponentUINode *parent,
	     MateComponentUINode *child,
	     gpointer      user_data)
{
	cmd_to_node_add_node (user_data, child, TRUE);
}

/*
 * MateComponentUIXml signal functions
 */

static void
custom_widget_unparent (NodeInfo *info)
{
	GtkContainer *container;

	g_return_if_fail (info != NULL);

	if (!info->widget)
		return;

	g_return_if_fail (GTK_IS_WIDGET (info->widget));

	if (info->widget->parent) {
		container = GTK_CONTAINER (info->widget->parent);
		g_return_if_fail (container != NULL);

		gtk_container_remove (container, info->widget);
	}
}

static void
sync_widget_set_node (MateComponentUISync *sync,
		      GtkWidget    *widget,
		      MateComponentUINode *node)
{
	GtkWidget *attached;

	if (!widget)
		return;

	g_return_if_fail (sync != NULL);

	matecomponent_ui_engine_widget_attach_node (widget, node);

	attached = matecomponent_ui_sync_get_attached (sync, widget, node);

	if (attached)
		matecomponent_ui_engine_widget_attach_node (attached, node);
}

static void
replace_override_fn (GObject        *object,
		     MateComponentUINode   *new,
		     MateComponentUINode   *old,
		     MateComponentUIEngine *engine)
{
	NodeInfo  *info = matecomponent_ui_xml_get_data (engine->priv->tree, new);
	NodeInfo  *old_info = matecomponent_ui_xml_get_data (engine->priv->tree, old);
	GtkWidget *old_widget;

	g_return_if_fail (info != NULL);
	g_return_if_fail (old_info != NULL);

	cmd_to_node_remove_node (engine, old, FALSE);
	cmd_to_node_add_node    (engine, new, FALSE);

/*	g_warning ("Replace override on '%s' '%s' widget '%p'",
		   matecomponent_ui_node_get_name (old),
		   matecomponent_ui_node_get_attr (old, "name"),
		   old_info->widget);
	info_dump_fn (old_info);
	info_dump_fn (info);*/

	/* Copy useful stuff across & tranfer widget ref */
	old_widget = old_info->widget;
	old_info->widget = NULL;

	info->type = old_info->type;
	info->widget = old_widget;

	/* Re-stamp the widget - get sync from old node actually in tree */
	{
		MateComponentUISync *sync = find_sync_for_node (engine, old);

		sync_widget_set_node (sync, info->widget, new);
	}

	/* Steal object reference */
	info->object = old_info->object;
	old_info->object = CORBA_OBJECT_NIL;
}

static void
prune_node (MateComponentUIEngine *engine,
	    MateComponentUINode   *node,
	    gboolean        save_custom)
{
	NodeInfo     *info;

	if (!node)
		return;

	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	if (info->widget) {
		gboolean save;
		
		save = NODE_IS_CUSTOM_WIDGET (info) && save_custom;

		if (!NODE_IS_ROOT_WIDGET (info) && !save) {
			MateComponentUISync *sync;
			GtkWidget    *item;

			item = info->widget;

			if ((sync = find_sync_for_node (engine, node))) {
				GtkWidget *attached;

				attached = matecomponent_ui_sync_get_attached (
					sync, item, node);

				if (attached) {
#ifdef XML_MERGE_DEBUG
					fprintf (stderr, "Got '%p' attached to '%p' for node '%s'\n",
						 attached, item,
						 matecomponent_ui_xml_make_path (node));
#endif

					item = attached;
				}
			}

#ifdef XML_MERGE_DEBUG
			fprintf (stderr, "Destroy widget '%s' '%p'\n",
				 matecomponent_ui_xml_make_path (node), item);
#endif

			gtk_widget_destroy (item);
			widget_unref (&info->widget);
		} else {
			if (save)
				custom_widget_unparent (info);
/*			printf ("leave widget '%s'\n",
			matecomponent_ui_xml_make_path (node));*/
		}
	}
}

/**
 * matecomponent_ui_engine_prune_widget_info:
 * @engine: the engine
 * @node: the node
 * @save_custom: whether to save custom widgets
 * 
 * This function destroys any widgets associated with
 * @node and all its children, if @save_custom, any widget
 * that is a custom widget ( such as a control ) will be
 * preserved. All widgets flagged ROOT are preserved always.
 **/
void
matecomponent_ui_engine_prune_widget_info (MateComponentUIEngine *engine,
				    MateComponentUINode   *node,
				    gboolean        save_custom)
{
	MateComponentUINode *l;

	if (!node)
		return;

	for (l = matecomponent_ui_node_children (node); l;
             l = matecomponent_ui_node_next (l))
		matecomponent_ui_engine_prune_widget_info (
			engine, l, TRUE);

	prune_node (engine, node, save_custom);
}

static void
override_fn (GObject        *object,
	     MateComponentUINode   *new,
	     MateComponentUINode   *old,
	     MateComponentUIEngine *engine)
{
#ifdef XML_MERGE_DEBUG
	fprintf (stderr, "Override '%s'\n", 
		 matecomponent_ui_xml_make_path (old));
#endif
	if (matecomponent_ui_node_same_name (new, old)) {
		replace_override_fn (object, new, old, engine);

	} else {
		matecomponent_ui_engine_prune_widget_info (engine, old, TRUE);

		cmd_to_node_remove_node (engine, old, FALSE);
		cmd_to_node_add_node    (engine, new, FALSE);
	}
}

static void
reinstate_fn (GObject        *object,
	      MateComponentUINode   *node,
	      MateComponentUIEngine *engine)
{
#ifdef XML_MERGE_DEBUG
	fprintf (stderr, "Reinstate '%s'\n", 
		 matecomponent_ui_xml_make_path (node));
/*	matecomponent_ui_engine_dump (engine, stderr, "pre reinstate_fn");*/
#endif

	matecomponent_ui_engine_prune_widget_info (engine, node, TRUE);

	cmd_to_node_add_node (engine, node, TRUE);
}

static void
rename_fn (GObject        *object,
	   MateComponentUINode   *node,
	   MateComponentUIEngine *engine)
{
#ifdef XML_MERGE_DEBUG
	fprintf (stderr, "Rename '%s'\n", 
		 matecomponent_ui_xml_make_path (node));
#endif
}

static void
remove_fn (GObject        *object,
	   MateComponentUINode   *node,
	   MateComponentUIEngine *engine)
{
#ifdef XML_MERGE_DEBUG
	fprintf (stderr, "Remove on '%s'\n",
		 matecomponent_ui_xml_make_path (node));

/*	matecomponent_ui_engine_dump (engine, stderr, "before remove_fn");*/
#endif
	matecomponent_ui_engine_prune_widget_info (engine, node, FALSE);

	if (matecomponent_ui_node_parent (node) == engine->priv->tree->root) {
		MateComponentUISync *sync = find_sync_for_node (engine, node);

		if (sync)
			matecomponent_ui_sync_remove_root (sync, node);
	}

	cmd_to_node_remove_node (engine, node, TRUE);
}

/*
 * Sub Component management functions
 */

/*
 *  This indirection is needed so we can serialize user settings
 * on a per component basis in future.
 */
typedef struct {
	char          *name;
	MateComponent_Unknown object;
} SubComponent;


static SubComponent *
sub_component_get (MateComponentUIEngine *engine, const char *name)
{
	SubComponent *component;
	GSList       *l;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	for (l = engine->priv->components; l; l = l->next) {
		component = l->data;
		
		if (!strcmp (component->name, name))
			return component;
	}
	
	component = g_new (SubComponent, 1);
	component->name = g_strdup (name);
	component->object = CORBA_OBJECT_NIL;

	engine->priv->components = g_slist_prepend (
		engine->priv->components, component);

	return component;
}

static SubComponent *
sub_component_get_by_ref (MateComponentUIEngine *engine, CORBA_Object obj)
{
	GSList *l;
	SubComponent *component = NULL;
	CORBA_Environment ev;

	g_return_val_if_fail (obj != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	CORBA_exception_init (&ev);

	for (l = engine->priv->components; l; l = l->next) {
		gboolean equiv;
		component = l->data;

		equiv = CORBA_Object_is_equivalent (component->object, obj, &ev);

		if (MATECOMPONENT_EX (&ev)) { /* Something very badly wrong */
			component = NULL;
			break;
		} else if (equiv)
			break;
	}

	CORBA_exception_free (&ev);

	return component;
}

static MateComponent_Unknown
sub_component_objref (MateComponentUIEngine *engine, const char *name)
{
	SubComponent *component = sub_component_get (engine, name);

	g_return_val_if_fail (component != NULL, CORBA_OBJECT_NIL);

	return component->object;
}

static void
sub_components_dump (MateComponentUIEngine *engine, FILE *out)
{
	GSList *l;

	g_return_if_fail (engine != NULL);
	g_return_if_fail (engine->priv != NULL);

	fprintf (out, "Component mappings:\n");

	for (l = engine->priv->components; l; l = l->next) {
		SubComponent *component = l->data;
		
		fprintf (out, "\t'%s' -> '%p'\n",
			 component->name, component->object);
	}
}

/* Use the pointer identity instead of a costly compare */
static char *
sub_component_cmp_name (MateComponentUIEngine *engine, const char *name)
{
	SubComponent *component;

	/*
	 * NB. For overriding if we get a NULL we just update the
	 * node without altering the id.
	 */
	if (!name || name [0] == '\0') {
		g_warning ("This should never happen");
		return NULL;
	}

	component = sub_component_get (engine, name);
	g_return_val_if_fail (component != NULL, NULL);

	return component->name;
}

static void
sub_component_destroy (MateComponentUIEngine *engine, SubComponent *component)
{
	engine->priv->components = g_slist_remove (
		engine->priv->components, component);

	if (component) {
		g_free (component->name);
		if (component->object != CORBA_OBJECT_NIL) {
			CORBA_Environment ev;

			CORBA_exception_init (&ev);
			MateComponent_UIComponent_unsetContainer (component->object, &ev);
			CORBA_exception_free (&ev);

			matecomponent_object_release_unref (component->object, NULL);
		}
		g_free (component);
	}
}

/**
 * matecomponent_ui_engine_deregister_dead_components:
 * @engine: the engine
 * 
 * Detect any components that have died and deregister
 * them - unmerging their UI elements.
 **/
void
matecomponent_ui_engine_deregister_dead_components (MateComponentUIEngine *engine)
{
	SubComponent      *component;
	GSList            *l, *next;
	CORBA_Environment  ev;

	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	for (l = engine->priv->components; l; l = next) {
		next = l->next;

		component = l->data;
		CORBA_exception_init (&ev);

		if (CORBA_Object_non_existent (component->object, &ev))
			matecomponent_ui_engine_deregister_component (
				engine, component->name);

		CORBA_exception_free (&ev);
	}
}

/**
 * matecomponent_ui_engine_get_component_names:
 * @engine: the engine
 * 
 * Return value: the names of all registered components
 **/
GList *
matecomponent_ui_engine_get_component_names (MateComponentUIEngine *engine)
{
	GSList *l;
	GList  *retval;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	retval = NULL;

	for (l = engine->priv->components; l; l = l->next) {
		SubComponent *component = l->data;
	
		retval = g_list_prepend (retval, component->name);
	}

	return retval;
}

/**
 * matecomponent_ui_engine_get_component:
 * @engine: the engine
 * @name: the name of the component to fetch
 * 
 * Return value: the component with name @name
 **/
MateComponent_Unknown
matecomponent_ui_engine_get_component (MateComponentUIEngine *engine,
				const char     *name)
{
	GSList *l;

	g_return_val_if_fail (name != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), CORBA_OBJECT_NIL);
		
	for (l = engine->priv->components; l; l = l->next) {
		SubComponent *component = l->data;
		
		if (!strcmp (component->name, name))
			return component->object;
	}

	return CORBA_OBJECT_NIL;
}

/**
 * matecomponent_ui_engine_register_component:
 * @engine: the engine
 * @name: a name to associate a component with
 * @component: the component
 * 
 * Registers @component with @engine by @name.
 **/
void
matecomponent_ui_engine_register_component (MateComponentUIEngine *engine,
				     const char     *name,
				     MateComponent_Unknown  component)
{
	SubComponent *subcomp;

	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if ((subcomp = sub_component_get (engine, name))) {
		if (subcomp->object != CORBA_OBJECT_NIL)
			matecomponent_object_release_unref (subcomp->object, NULL);
	}

	if (component != CORBA_OBJECT_NIL)
		subcomp->object = matecomponent_object_dup_ref (component, NULL);
	else
		subcomp->object = CORBA_OBJECT_NIL;
}

/**
 * matecomponent_ui_engine_deregister_component:
 * @engine: the engine
 * @name: the component name
 * 
 * Deregisters component of @name from @engine.
 **/
void
matecomponent_ui_engine_deregister_component (MateComponentUIEngine *engine,
				       const char     *name)
{
	SubComponent *component;

	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if ((component = sub_component_get (engine, name))) {
		matecomponent_ui_engine_xml_rm (engine, "/", component->name);
		sub_component_destroy (engine, component);
	} else
		g_warning ("Attempting to deregister non-registered "
			   "component '%s'", name);
}

/**
 * matecomponent_ui_engine_deregister_component_by_ref:
 * @engine: the engine
 * @ref: the ref.
 * 
 * Deregisters component with reference @ref from @engine.
 **/
void
matecomponent_ui_engine_deregister_component_by_ref (MateComponentUIEngine *engine,
					      MateComponent_Unknown  ref)
{
	SubComponent *component;

	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if ((component = sub_component_get_by_ref (engine, ref))) {
		matecomponent_ui_engine_xml_rm (engine, "/", component->name);
		sub_component_destroy (engine, component);
	} else
		g_warning ("Attempting to deregister non-registered "
			   "component");
}

/*
 * State update signal queueing functions
 */

typedef struct {
	MateComponentUISync *sync;
	GtkWidget    *widget;
	char         *state;
} StateUpdate;

/*
 * Update the state later, but other aspects of the widget right now.
 * It's dangerous to update the state now because we can reenter if we
 * do that.
 */
static StateUpdate *
state_update_new (MateComponentUISync *sync,
		  GtkWidget    *widget,
		  MateComponentUINode *node)
{
	char        *state;
	const char  *hidden, *sensitive;
	StateUpdate *su;

	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	hidden = matecomponent_ui_node_get_attr_by_id (node, hidden_id);
	if (hidden && atoi (hidden))
		gtk_widget_hide (widget);
	else
		gtk_widget_show (widget);

	sensitive = matecomponent_ui_node_get_attr_by_id (node, sensitive_id);
	if (sensitive)
		gtk_widget_set_sensitive (widget, atoi (sensitive));
	else
		gtk_widget_set_sensitive (widget, TRUE);

	if ((state = matecomponent_ui_node_get_attr (node, "state"))) {
		su = g_new0 (StateUpdate, 1);
		
		su->sync   = sync;
		su->widget = widget;
		g_object_ref (su->widget);
		su->state  = state;
	} else
		su = NULL;

	return su;
}

static void
state_update_destroy (StateUpdate *su)
{
	if (su) {
		g_object_unref (su->widget);
		matecomponent_ui_node_free_string (su->state);

		g_free (su);
	}
}

static void
state_update_now (MateComponentUIEngine *engine,
		  MateComponentUINode   *node,
		  GtkWidget      *widget)
{
	StateUpdate  *su;
	MateComponentUISync *sync;

	if (!widget)
		return;

	sync = find_sync_for_node (engine, node);
	g_return_if_fail (sync != NULL);

	su = state_update_new (sync, widget, node);
	
	if (su) {
		matecomponent_ui_sync_state_update (su->sync, su->widget, su->state);
		state_update_destroy (su);
	}
}

/**
 * matecomponent_ui_engine_xml_get_prop:
 * @engine: the engine
 * @path: the path into the tree
 * @prop: The property
 * 
 * This function fetches the property @prop at node
 * at @path in the internal structure.
 *
 * Return value: a CORBA allocated string
 **/
CORBA_char *
matecomponent_ui_engine_xml_get_prop (MateComponentUIEngine *engine,
			       const char     *path,
			       const char     *prop,
			       gboolean       *invalid_path)
{
 	const char   *str;
 	MateComponentUINode *node;
  
  	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

  	node = matecomponent_ui_xml_get_path (engine->priv->tree, path);
  	if (!node) {
		if (invalid_path)
			*invalid_path = TRUE;
  		return NULL;
 	} else {
		if (invalid_path)
			*invalid_path = FALSE;
  
 		str = matecomponent_ui_node_peek_attr (node, prop);

		if (!str)
			return NULL;

 		return CORBA_string_dup (str);
  	}
}

/**
 * matecomponent_ui_engine_xml_get:
 * @engine: the engine
 * @path: the path into the tree
 * @node_only: just the node, or children too.
 * 
 * This function fetches the node at @path in the
 * internal structure, and if @node_only dumps the
 * node to an XML string, otherwise it dumps it and
 * its children.
 *
 * Return value: the XML string - use CORBA_free to free
 **/
CORBA_char *
matecomponent_ui_engine_xml_get (MateComponentUIEngine *engine,
			  const char     *path,
			  gboolean        node_only)
{
 	char         *str;
 	MateComponentUINode *node;
  	CORBA_char   *ret;
  
  	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);
  
  	node = matecomponent_ui_xml_get_path (engine->priv->tree, path);
  	if (!node)
  		return NULL;
 	else {		
 		str = matecomponent_ui_node_to_string (node, !node_only);
 		ret = CORBA_string_dup (str);
 		g_free (str);
 		return ret;
  	}
}

/**
 * matecomponent_ui_engine_xml_node_exists:
 * @engine: the engine
 * @path: the path into the tree
 * 
 * Return value: true if the node at @path exists
 **/
gboolean
matecomponent_ui_engine_xml_node_exists (MateComponentUIEngine   *engine,
				  const char       *path)
{
	MateComponentUINode *node;
	gboolean      wildcard;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), FALSE);

	node = matecomponent_ui_xml_get_path_wildcard (
		engine->priv->tree, path, &wildcard);

	if (!wildcard)
		return (node != NULL);
	else
		return (node != NULL &&
			matecomponent_ui_node_children (node) != NULL);
}

/**
 * matecomponent_ui_engine_object_set:
 * @engine: the engine
 * @path: the path into the tree
 * @object: an object reference
 * @ev: CORBA exception environment
 * 
 * This associates a CORBA Object reference with a node
 * in the tree, most often this is done to insert a Control's
 * reference into a 'control' element.
 * 
 * Return value: flag if success
 **/
MateComponentUIError
matecomponent_ui_engine_object_set (MateComponentUIEngine   *engine,
			     const char       *path,
			     MateComponent_Unknown    object,
			     CORBA_Environment *ev)
{
	NodeInfo     *info;
	MateComponentUINode *node;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), 
			      MATECOMPONENT_UI_ERROR_BAD_PARAM);

	node = matecomponent_ui_xml_get_path (engine->priv->tree, path);
	if (!node)
		return MATECOMPONENT_UI_ERROR_INVALID_PATH;

	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	if (info->object != CORBA_OBJECT_NIL) {
		matecomponent_object_release_unref (info->object, ev);
		if (info->widget)
			gtk_widget_destroy (info->widget);
		widget_unref (&info->widget);
	}
	
	dbgprintf ("** Setting object %p on info %p\n", object, info);
	info->object = matecomponent_object_dup_ref (object, ev);

	matecomponent_ui_xml_set_dirty (engine->priv->tree, node);

/*	fprintf (stderr, "Object set '%s'\n", path);
	matecomponent_ui_engine_dump (win, "Before object set updatew");*/

	matecomponent_ui_engine_update (engine);

/*	matecomponent_ui_engine_dump (win, "After object set updatew");*/

	return MATECOMPONENT_UI_ERROR_OK;
}

void
matecomponent_ui_engine_widget_set (MateComponentUIEngine    *engine,
			     const char        *path,
			     GtkWidget         *widget)
{
	NodeInfo *info;
	GtkWidget *custom_widget;
	MateComponentUINode *node;
	MateComponentUISync *sync;

	g_return_if_fail (widget != NULL);

	matecomponent_ui_engine_freeze (engine);

	matecomponent_ui_engine_object_set (
		engine, path, CORBA_OBJECT_NIL, NULL);

	node = matecomponent_ui_engine_get_path (engine, path);
	g_return_if_fail (node != NULL);
	g_return_if_fail (!strcmp (matecomponent_ui_node_get_name (node), "control"));

	sync = find_sync_for_node (engine, node);

	custom_widget = matecomponent_ui_sync_wrap_widget (sync, widget);

	if (custom_widget) {
		info = matecomponent_ui_xml_get_data (engine->priv->tree, node);
		info->widget = g_object_ref_sink (custom_widget);

		matecomponent_ui_engine_stamp_custom (engine, node);
	}
		
	matecomponent_ui_engine_thaw (engine);
}

/**
 * matecomponent_ui_engine_object_get:
 * @engine: the engine
 * @path: the path into the tree
 * @object: an pointer to an object reference
 * @ev: CORBA exception environment
 * 
 * This extracts a CORBA object reference associated with
 * the node at @path in @engine, and returns it in the
 * reference pointed to by @object.
 * 
 * Return value: flag if success
 **/
MateComponentUIError
matecomponent_ui_engine_object_get (MateComponentUIEngine    *engine,
			     const char        *path,
			     MateComponent_Unknown    *object,
			     CORBA_Environment *ev)
{
	NodeInfo     *info;
	MateComponentUINode *node;

	g_return_val_if_fail (object != NULL,
			      MATECOMPONENT_UI_ERROR_BAD_PARAM);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), 
			      MATECOMPONENT_UI_ERROR_BAD_PARAM);

	*object = CORBA_OBJECT_NIL;

	node = matecomponent_ui_xml_get_path (engine->priv->tree, path);

	if (!node)
		return MATECOMPONENT_UI_ERROR_INVALID_PATH;

	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	if (info->object != CORBA_OBJECT_NIL)
		*object = matecomponent_object_dup_ref (info->object, ev);

	return MATECOMPONENT_UI_ERROR_OK;
}

static int
find_last_slash (const char *path)
{
	int i, last_slash = 0;

	for (i = 0; path [i]; i++) {
		if (path [i] == '/')
			last_slash = i;
	}

	return last_slash;
}

/**
 * matecomponent_ui_engine_xml_set_prop:
 * @engine: the engine
 * @path: the path into the tree
 * @property: The property to set
 * @value: The new value of the property
 * @component: the component ID associated with the nodes.
 * 
 * This function sets the property of a node in the internal tree
 * representation at @path in @engine.
 * 
 * Return value: flag on error
 **/
MateComponentUIError
matecomponent_ui_engine_xml_set_prop (MateComponentUIEngine *engine,
			       const char     *path,
			       const char     *property,
			       const char     *value,
			       const char     *component)
{
	char *cmp_name;
	const char *old_value;
	MateComponentUINode *original;
	NodeInfo     *info;
	
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), 
			      MATECOMPONENT_UI_ERROR_BAD_PARAM);

	original = matecomponent_ui_engine_get_path (engine, path);

	if (!original) 
		return MATECOMPONENT_UI_ERROR_INVALID_PATH;

	info = matecomponent_ui_xml_get_data (engine->priv->tree, original);
	cmp_name = sub_component_cmp_name (engine, component);

	if (info->parent.id == cmp_name) {
		old_value = matecomponent_ui_node_peek_attr (original, property);
		if (!old_value && !value)
			return MATECOMPONENT_UI_ERROR_OK;
		
		else if (old_value && value && !strcmp (old_value, value))
			return MATECOMPONENT_UI_ERROR_OK;

		else {
			matecomponent_ui_node_set_attr (original, property, value);
			matecomponent_ui_xml_set_dirty (engine->priv->tree, original);

			matecomponent_ui_engine_update (engine);
		}
	} else {
		int last_slash;
		char *parent_path;
		MateComponentUINode *copy;

		copy = matecomponent_ui_node_new (
			matecomponent_ui_node_get_name (original));

		matecomponent_ui_node_copy_attrs (original, copy);
		matecomponent_ui_node_set_attr (copy, property, value);

		last_slash = find_last_slash (path);

		parent_path = g_alloca (last_slash + 1);
		memcpy (parent_path, path, last_slash);
		parent_path [last_slash] = '\0';

		matecomponent_ui_xml_merge (
			engine->priv->tree, parent_path,
			copy, cmp_name);

		matecomponent_ui_engine_update (engine);
	}

	return MATECOMPONENT_UI_ERROR_OK;
}


/**
 * matecomponent_ui_engine_xml_merge_tree:
 * @engine: the engine
 * @path: the path into the tree
 * @tree: the nodes
 * @component: the component ID associated with these nodes.
 * 
 * This function merges the XML @tree into the internal tree
 * representation as children of the node at @path in @engine.
 * 
 * Return value: flag on error
 **/
MateComponentUIError
matecomponent_ui_engine_xml_merge_tree (MateComponentUIEngine    *engine,
				 const char        *path,
				 MateComponentUINode      *tree,
				 const char        *component)
{
	MateComponentUIError err;
	
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), 
			      MATECOMPONENT_UI_ERROR_BAD_PARAM);

	if (!tree || !matecomponent_ui_node_get_name (tree))
		return MATECOMPONENT_UI_ERROR_OK;

	if (!tree) {
		g_warning ("Stripped tree to nothing");
		return MATECOMPONENT_UI_ERROR_OK;
	}

	/*
	 *  Because peer to peer merging makes the code hard, and
	 * paths non-inituitive and since we want to merge root
	 * elements as peers to save lots of redundant CORBA calls
	 * we special case root.
	 */
	if (matecomponent_ui_node_has_name (tree, "Root")) {
		err = matecomponent_ui_xml_merge (
			engine->priv->tree, path, matecomponent_ui_node_children (tree),
			sub_component_cmp_name (engine, component));
		matecomponent_ui_node_free (tree);
	} else
		err = matecomponent_ui_xml_merge (
			engine->priv->tree, path, tree,
			sub_component_cmp_name (engine, component));

#ifdef XML_MERGE_DEBUG
/*	matecomponent_ui_engine_dump (engine, stderr, "after merge");*/
#endif

	matecomponent_ui_engine_update (engine);

	return err;
}

/**
 * matecomponent_ui_engine_xml_rm:
 * @engine: the engine
 * @path: the path into the tree
 * @by_component: whether to remove elements from only a
 * specific component
 * 
 * Remove a chunk of the xml tree pointed at by @path
 * in @engine, if @by_component then only remove items
 * associated with that component - possibly revealing
 * other overridden items.
 * 
 * Return value: flag on error
 **/
MateComponentUIError
matecomponent_ui_engine_xml_rm (MateComponentUIEngine *engine,
			 const char     *path,
			 const char     *by_component)
{
	MateComponentUIError err;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine),
			      MATECOMPONENT_UI_ERROR_BAD_PARAM);

	err = matecomponent_ui_xml_rm (
		engine->priv->tree, path,
		sub_component_cmp_name (engine, by_component));

	matecomponent_ui_engine_update (engine);

	return err;
}

/**
 * matecomponent_ui_engine_set_ui_container:
 * @engine: the engine
 * @ui_container: a UI Container matecomponent object.
 * 
 * Associates a given UI Container with this MateComponentUIEngine.
 **/
void
matecomponent_ui_engine_set_ui_container (MateComponentUIEngine    *engine,
				   MateComponentUIContainer *ui_container)
{
	MateComponentUIContainer *old_container;

	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if (engine->priv->container == ui_container)
		return;

	g_return_if_fail (!ui_container ||
			  MATECOMPONENT_IS_UI_CONTAINER (ui_container));

	old_container = engine->priv->container;

	if (ui_container)
		engine->priv->container = MATECOMPONENT_UI_CONTAINER (
			matecomponent_object_ref (MATECOMPONENT_OBJECT (ui_container)));
	else
		engine->priv->container = NULL;

	if (old_container) {
		matecomponent_ui_container_set_engine (old_container, NULL);
		matecomponent_object_unref (MATECOMPONENT_OBJECT (old_container));
	}

	if (ui_container)
		matecomponent_ui_container_set_engine (ui_container, engine);
}

/**
 * matecomponent_ui_engine_get_ui_container:
 * @engine: the engine
 * 
 * Fetches the associated UI Container
 *
 * Return value: the associated UI container.
 **/
MateComponentUIContainer *
matecomponent_ui_engine_get_ui_container (MateComponentUIEngine *engine)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	return engine->priv->container;
}

static void
real_exec_verb (MateComponentUIEngine *engine,
		const char     *component_name,
		const char     *verb)
{
	char *verb_cpy;
	MateComponent_UIComponent component;

	g_return_if_fail (verb != NULL);
	g_return_if_fail (component_name != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if (matecomponent_ui_engine_inhibit_events > 0)
		return;

	g_object_ref (engine);

	component = sub_component_objref (engine, component_name);

	verb_cpy = g_strdup (verb);

	if (component != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);

		CORBA_Object_duplicate (component, &ev);

		MateComponent_UIComponent_execVerb (
			component, verb_cpy, &ev);

		if (engine->priv->container)
			matecomponent_object_check_env (
				MATECOMPONENT_OBJECT (engine->priv->container),
				component, &ev);

		if (MATECOMPONENT_EX (&ev))
			g_warning ("Exception executing verb '%s'"
				   "major %d, %s", verb_cpy, ev._major,
				   MATECOMPONENT_EX_REPOID (&ev));

		CORBA_Object_release (component, &ev);
		
		CORBA_exception_free (&ev);
	}

	g_free (verb_cpy);
	g_object_unref (engine);
}

static void
impl_emit_verb_on (MateComponentUIEngine *engine,
		   MateComponentUINode   *node)
{
	const char      *verb;
	const char      *sensitive;
	MateComponentUINode    *cmd_node;
	MateComponentUIXmlData *data;
	
	g_return_if_fail (node != NULL);

	data = matecomponent_ui_xml_get_data (NULL, node);
	g_return_if_fail (data != NULL);

	verb = node_get_id (node);
	if (!verb)
		return;

	if ((cmd_node = matecomponent_ui_engine_get_cmd_node (engine, node)) &&
	    (sensitive = matecomponent_ui_node_get_attr_by_id (cmd_node, sensitive_id)) &&
	    !atoi (sensitive))
		return;

	/* Builtins */
	if (!strcmp (verb, "MateComponentCustomize"))
		matecomponent_ui_engine_config_configure (engine->priv->config);

	else if (!strcmp (verb, "MateComponentUIDump"))
		matecomponent_ui_engine_dump (engine, stderr, "from verb");

	else {
		if (!data->id) {
			g_warning ("Weird; no ID on verb '%s'", verb);
			return;
		}

		real_exec_verb (engine, data->id, verb);
	}
}

static MateComponentUINode *
cmd_get_node (MateComponentUIEngine *engine,
	      MateComponentUINode   *from_node)
{
	char         *path;
	MateComponentUINode *ret;
	const char   *cmd_name;

	g_return_val_if_fail (engine != NULL, NULL);

	if (!from_node)
		return NULL;

	if (!(cmd_name = node_get_id (from_node)))
		return NULL;

	path = g_strconcat ("/commands/", cmd_name, NULL);
	ret  = matecomponent_ui_xml_get_path (engine->priv->tree, path);

	if (!ret) {
		MateComponentUIXmlData *data_from;
		MateComponentUINode *commands;
		MateComponentUINode *node;

		commands = matecomponent_ui_node_new ("commands");
		node     = matecomponent_ui_node_new_child (commands, "cmd");

		matecomponent_ui_node_set_attr (node, "name", cmd_name);

		data_from = matecomponent_ui_xml_get_data (
			engine->priv->tree, from_node);

		matecomponent_ui_xml_merge (
			engine->priv->tree, "/",
			commands, data_from->id);
		
		ret = matecomponent_ui_xml_get_path (
			engine->priv->tree, path);
		g_assert (ret != NULL);
	}

	g_free (path);

	return ret;
}

static GSList *
make_updates_for_command (MateComponentUIEngine *engine,
			  GSList         *list,
			  MateComponentUINode   *state,
			  const char     *search_id)
{
	const GSList *l;

	l = cmd_to_nodes (engine, search_id);

	if (!l)
		return list;

/*	printf ("Update cmd state if %s == %s on node '%s'\n", search_id, id,
	matecomponent_ui_xml_make_path (search));*/

	for (; l; l = l->next) {

		NodeInfo *info = matecomponent_ui_xml_get_data (
			engine->priv->tree, l->data);

		if (info->widget) {
			MateComponentUISync *sync;
			StateUpdate  *su;

			sync = find_sync_for_node (engine, l->data);
			g_return_val_if_fail (sync != NULL, list);

			su = state_update_new (sync, info->widget, state);

			if (su)
				list = g_slist_prepend (list, su);
		}
	}

	return list;
}

static void
execute_state_updates (GSList *updates)
{
	GSList *l;

	matecomponent_ui_engine_inhibit_events ++;

	for (l = updates; l; l = l->next) {
		StateUpdate *su = l->data;

		matecomponent_ui_sync_state_update (su->sync, su->widget, su->state);
	}

	for (l = updates; l; l = l->next)
		state_update_destroy (l->data);

	g_slist_free (updates);

	matecomponent_ui_engine_inhibit_events --;
}

/*
 * set_cmd_attr:
 *   Syncs cmd / widgets on events [ event flag set ]
 *   or helps evil people who set state on menu /
 *      toolbar items instead of on the associated verb / id.
 **/
static void
set_cmd_attr (MateComponentUIEngine *engine,
	      MateComponentUINode   *node,
	      GQuark          prop,
	      const char     *value,
	      gboolean        event)
{
	MateComponentUINode *cmd_node;

	g_return_if_fail (node != NULL);
	g_return_if_fail (value != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if (!(cmd_node = cmd_get_node (engine, node))) { /* A non cmd widget */
		NodeInfo *info = matecomponent_ui_xml_get_data (
			engine->priv->tree, node);

		if (matecomponent_ui_node_try_set_attr (node, prop, value))
			state_update_now (engine, node, info->widget);
		return;
	}

#ifdef STATE_SYNC_DEBUG
	fprintf (stderr, "Set '%s' : '%s' to '%s'",
		 matecomponent_ui_node_peek_attr (cmd_node, "name"),
		 g_quark_to_string (prop), value);
#endif

	if (!matecomponent_ui_node_try_set_attr (cmd_node, prop, value))
		return;

	if (event) {
		GSList     *updates;
		const char *cmd_name;

		cmd_name = matecomponent_ui_node_peek_attr (cmd_node, "name");

		updates = make_updates_for_command (
			engine, NULL, cmd_node, cmd_name);

		execute_state_updates (updates);

	} else {
		MateComponentUIXmlData *data =
			matecomponent_ui_xml_get_data (
				engine->priv->tree, cmd_node);

		data->dirty = TRUE;
	}
}

static void
real_emit_ui_event (MateComponentUIEngine *engine,
		    const char     *component_name,
		    const char     *id,
		    int             type,
		    const char     *new_state)
{
	MateComponent_UIComponent component;

	g_return_if_fail (id != NULL);
	g_return_if_fail (new_state != NULL);

	if (!component_name) /* Auto-created entry, no-one can listen to it */
		return;
	if (matecomponent_ui_engine_inhibit_events > 0)
		return;

	g_object_ref (engine);

	component = sub_component_objref (engine, component_name);

	if (component != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);

		MateComponent_UIComponent_uiEvent (
			component, id, type, new_state, &ev);

		if (engine->priv->container)
			matecomponent_object_check_env (
				MATECOMPONENT_OBJECT (engine->priv->container),
				component, &ev);

		if (MATECOMPONENT_EX (&ev))
			g_warning ("Exception emitting state change to %d '%s' '%s'"
				   "major %d, %s",
				   type, id, new_state, ev._major,
				   MATECOMPONENT_EX_REPOID (&ev));
		
		CORBA_exception_free (&ev);
	}

	g_object_unref (engine);
}

static void
impl_emit_event_on (MateComponentUIEngine *engine,
		    MateComponentUINode   *node,
		    const char     *state)
{
	const char      *id;
	const char      *sensitive;
	MateComponentUINode    *cmd_node;
	MateComponentUIXmlData *data;
	char            *component_id, *real_id, *real_state;

	g_return_if_fail (node != NULL);

	if (!(id = node_get_id (node)))
		return;

	if ((cmd_node = matecomponent_ui_engine_get_cmd_node (engine, node)) &&
	    (sensitive = matecomponent_ui_node_get_attr_by_id (cmd_node, sensitive_id)) &&
	    !atoi (sensitive))
		return;

	data = matecomponent_ui_xml_get_data (NULL, node);
	g_return_if_fail (data != NULL);

	g_object_ref (engine);

	component_id = g_strdup (data->id);
	real_id      = g_strdup (id);
	real_state   = g_strdup (state);

	/* This could invoke a CORBA method that might de-register the component */
	set_cmd_attr (engine, node, state_id, state, TRUE);

	real_emit_ui_event (engine, component_id, real_id,
			    MateComponent_UIComponent_STATE_CHANGED,
			    real_state);

	g_object_unref (engine);

	g_free (real_state);
	g_free (real_id);
	g_free (component_id);
}

void
matecomponent_ui_engine_dispose (MateComponentUIEngine *engine)
{
	GSList *l;
	MateComponentUIEnginePrivate *priv = engine->priv;

	dbgprintf ("matecomponent_ui_engine_dispose %p\n", engine);

	matecomponent_ui_engine_freeze (engine);

	while (priv->components)
		sub_component_destroy (
			engine, priv->components->data);

	matecomponent_ui_engine_set_ui_container (engine, NULL);

	/* Remove the engine from the configuration notify list */
	matecomponent_ui_preferences_remove_engine (engine);

	if (priv->config) {
		g_object_unref (priv->config);
		priv->config = NULL;
	}

	if (priv->tree) {
		g_object_unref (priv->tree);
		priv->tree = NULL;
	}

	g_hash_table_foreach_remove (
		priv->cmd_to_node,
		cmd_to_node_clear_hash,
		NULL);

	for (l = priv->syncs; l; l = l->next)
		g_object_unref (l->data);
	g_slist_free (priv->syncs);
	priv->syncs = NULL;

	matecomponent_ui_engine_thaw (engine);
}

static void
impl_dispose (GObject *object)
{
	matecomponent_ui_engine_dispose (MATECOMPONENT_UI_ENGINE (object));

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
impl_finalize (GObject *object)
{
	MateComponentUIEngine *engine;

	dbgprintf ("matecomponent_ui_engine_finalize %p\n", object);
       
	engine = MATECOMPONENT_UI_ENGINE (object);

	g_hash_table_destroy (engine->priv->cmd_to_node);

	g_free (engine->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (MateComponentUIEngineClass *engine_class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (engine_class);
 
 	id_id        = g_quark_from_static_string ("id");
	cmd_id       = g_quark_from_static_string ("cmd");
 	verb_id      = g_quark_from_static_string ("verb");
 	name_id      = g_quark_from_static_string ("name");
	state_id     = g_quark_from_static_string ("state");
 	hidden_id    = g_quark_from_static_string ("hidden");
	commands_id  = g_quark_from_static_string ("commands");
 	sensitive_id = g_quark_from_static_string ("sensitive");

	object_class = G_OBJECT_CLASS (engine_class);
	object_class->dispose  = impl_dispose;
	object_class->finalize = impl_finalize;

	engine_class->emit_verb_on  = impl_emit_verb_on;
	engine_class->emit_event_on = impl_emit_event_on;

	signals [ADD_HINT]
		= g_signal_new ("add_hint",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIEngineClass, add_hint),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);
	signals [REMOVE_HINT]
		= g_signal_new ("remove_hint",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIEngineClass, remove_hint),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);

	signals [EMIT_VERB_ON]
		= g_signal_new ("emit_verb_on",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIEngineClass, emit_verb_on),
				NULL, NULL,
				g_cclosure_marshal_VOID__POINTER,
				G_TYPE_NONE, 1, G_TYPE_POINTER);

	signals [EMIT_EVENT_ON]
		= g_signal_new ("emit_event_on",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIEngineClass, emit_event_on),
				NULL, NULL,
				matecomponent_ui_marshal_VOID__POINTER_STRING,
				G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_STRING);

	signals [DESTROY]
		= g_signal_new ("destroy",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIEngineClass, destroy),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
}

static void
init (MateComponentUIEngine *engine)
{
	MateComponentUIEnginePrivate *priv;

	priv = g_new0 (MateComponentUIEnginePrivate, 1);

	engine->priv = priv;

	priv->cmd_to_node = g_hash_table_new (
		g_str_hash, g_str_equal);
}

GType
matecomponent_ui_engine_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (MateComponentUIEngineClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentUIEngine),
			0, /* n_preallocs */
			(GInstanceInitFunc) init
		};
		
		type = g_type_register_static (PARENT_TYPE, "MateComponentUIEngine",
					       &info, 0);
	}

	return type;
}

static void
add_node (MateComponentUINode *parent, const char *name)
{
        MateComponentUINode *node = matecomponent_ui_node_new (name);

	matecomponent_ui_node_add_child (parent, node);
}

static void
build_skeleton (MateComponentUIXml *xml)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_XML (xml));

	add_node (xml->root, "keybindings");
	add_node (xml->root, "commands");
}

/**
 * matecomponent_ui_engine_construct:
 * @engine: the engine.
 * @view: the view [ often a MateComponentWindow ]
 * 
 * Construct a new matecomponent_ui_engine
 * 
 * Return value: the constructed engine.
 **/
MateComponentUIEngine *
matecomponent_ui_engine_construct (MateComponentUIEngine *engine,
			    GObject        *view)
{
	GtkWindow *opt_parent;
	MateComponentUIEnginePrivate *priv;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	priv = engine->priv;
	priv->view = view;

	priv->tree = matecomponent_ui_xml_new (
		NULL, info_new_fn, info_free_fn,
		info_dump_fn, add_node_fn, engine);


	if (GTK_IS_WINDOW (view))
		opt_parent = GTK_WINDOW (view);
	else
		opt_parent = NULL;
	
	priv->config = matecomponent_ui_engine_config_new (engine, opt_parent);

	build_skeleton (priv->tree);

	if (g_getenv ("MATECOMPONENT_DEBUG"))
		add_debug_menu (engine);

	g_signal_connect (priv->tree, "override",
			  G_CALLBACK (override_fn), engine);

	g_signal_connect (priv->tree, "replace_override",
			  G_CALLBACK (replace_override_fn), engine);

	g_signal_connect (priv->tree, "reinstate",
			  G_CALLBACK (reinstate_fn), engine);

	g_signal_connect (priv->tree, "rename",
			  G_CALLBACK (rename_fn), engine);

	g_signal_connect (priv->tree, "remove",
			  G_CALLBACK (remove_fn), engine);

	/* Add the engine to the configuration notify list */
	matecomponent_ui_preferences_add_engine (engine);
	
	return engine;
}


/**
 * matecomponent_ui_engine_new:
 * @void: 
 * 
 * Create a new #MateComponentUIEngine structure
 * 
 * Return value: the new UI Engine.
 **/
MateComponentUIEngine *
matecomponent_ui_engine_new (GObject *view)
{
	MateComponentUIEngine *engine = g_object_new (MATECOMPONENT_TYPE_UI_ENGINE, NULL);

	return matecomponent_ui_engine_construct (engine, view);
}


/**
 * matecomponent_ui_engine_get_view:
 * @engine: the engine
 * 
 * This returns the associated view, often a MateComponentWindow
 * 
 * Return value: the view widget.
 **/
GObject *
matecomponent_ui_engine_get_view (MateComponentUIEngine *engine)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	return engine->priv->view;
}

static void
hide_all_widgets (MateComponentUIEngine *engine,
		  MateComponentUINode   *node)
{
	NodeInfo *info;
	MateComponentUINode *child;
	
	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);
	if (info->widget)
		gtk_widget_hide (info->widget);
	
	for (child = matecomponent_ui_node_children (node);
	     child != NULL;
	     child = matecomponent_ui_node_next (child))
		hide_all_widgets (engine, child);
}

static gboolean
contains_visible_widget (MateComponentUIEngine *engine,
			 MateComponentUINode   *node)
{
	MateComponentUINode *child;
	NodeInfo     *info;
	
	for (child = matecomponent_ui_node_children (node);
	     child != NULL;
	     child = matecomponent_ui_node_next (child)) {
		info = matecomponent_ui_xml_get_data (engine->priv->tree, child);
		if (info->widget && GTK_WIDGET_VISIBLE (info->widget))
			return TRUE;
		if (contains_visible_widget (engine, child))
			return TRUE;
	}

	return FALSE;
}

static void
hide_placeholder_if_empty_or_hidden (MateComponentUIEngine *engine,
				     MateComponentUINode   *node)
{
	NodeInfo   *info;
	const char *txt;
	gboolean hide_placeholder_and_contents;
	gboolean has_visible_separator;

	txt = matecomponent_ui_node_get_attr_by_id (node, hidden_id);
	hide_placeholder_and_contents = txt && atoi (txt);

	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);
	has_visible_separator = info && info->widget &&
		GTK_WIDGET_VISIBLE (info->widget);

	if (hide_placeholder_and_contents)
		hide_all_widgets (engine, node);

	else if (has_visible_separator &&
		 !contains_visible_widget (engine, node))
		gtk_widget_hide (info->widget);
}

static gboolean
node_is_dirty (MateComponentUIEngine *engine,
	       MateComponentUINode   *node)
{
	MateComponentUIXmlData *data = matecomponent_ui_xml_get_data (
		engine->priv->tree, node);

	if (!data)
		return TRUE;
	else
		return data->dirty;
}

static void
matecomponent_ui_engine_sync (MateComponentUIEngine   *engine,
		       MateComponentUISync     *sync,
		       MateComponentUINode     *node,
		       GtkWidget        *parent,
		       GList           **widgets,
		       int              *pos)
{
	MateComponentUINode *a;
	GList        *b, *nextb;

#ifdef WIDGET_SYNC_DEBUG
	printf ("In sync to pos %d with widgets:\n", *pos);
	for (b = *widgets; b; b = b->next) {
		MateComponentUINode *node = matecomponent_ui_engine_widget_get_node (b->data);

		if (node)
			printf ("\t'%s'\n", matecomponent_ui_xml_make_path (node));
		else
			printf ("\tno node ptr\n");
	}
#endif

	b = *widgets;
	for (a = node; a; b = nextb) {
		gboolean same;

		nextb = b ? b->next : NULL;

		if (b && matecomponent_ui_sync_ignore_widget (sync, b->data)) {
			(*pos)++;
			continue;
		}

		same = (b != NULL) &&
			(matecomponent_ui_engine_widget_get_node (b->data) == a);

#ifdef WIDGET_SYNC_DEBUG
		printf ("Node '%s(%p)' Dirty '%d' same %d on b %d widget %p\n",
			matecomponent_ui_xml_make_path (a), a,
			node_is_dirty (engine, a), same, b != NULL, b?b->data:NULL);
#endif

		if (node_is_dirty (engine, a)) {
			MateComponentUISyncStateFn ss;
			MateComponentUISyncBuildFn bw;
			MateComponentUINode       *cmd_node;
			
			if (matecomponent_ui_node_has_name (a, "placeholder")) {
				ss = matecomponent_ui_sync_state_placeholder;
				bw = matecomponent_ui_sync_build_placeholder;
			} else {
				ss = matecomponent_ui_sync_state;
				bw = matecomponent_ui_sync_build;
			}

			cmd_node = matecomponent_ui_engine_get_cmd_node (engine, a);

			if (same) {
#ifdef WIDGET_SYNC_DEBUG
				printf ("-- just syncing state --\n");
#endif
				ss (sync, a, cmd_node, b->data, parent);

				(*pos)++;
			} else {
				NodeInfo   *info;
				GtkWidget  *widget;

				info = matecomponent_ui_xml_get_data (
					engine->priv->tree, a);

#ifdef WIDGET_SYNC_DEBUG
				printf ("re-building widget\n");
#endif

				widget = bw (sync, a, cmd_node, pos, parent);

#ifdef WIDGET_SYNC_DEBUG
				printf ("Built item '%p' '%s' and inserted at '%d'\n",
					widget, matecomponent_ui_node_get_name (a), *pos);
#endif

				info->widget = widget ? g_object_ref (widget) : NULL;
				if (widget) {
					matecomponent_ui_engine_widget_set_node (
						sync->engine, widget, a);

					ss (sync, a, cmd_node, widget, parent);
				}
#ifdef WIDGET_SYNC_DEBUG
				else
					printf ("Failed to build widget\n");
#endif

				nextb = b; /* NB. don't advance 'b' */
			}

		} else {
			if (!same) {
				MateComponentUINode *bn = b ? matecomponent_ui_engine_widget_get_node (b->data) : NULL;
				NodeInfo   *info;

				info = matecomponent_ui_xml_get_data (engine->priv->tree, a);

				if (!info->widget) {
					/*
					 *  A control that hasn't been filled out yet
					 * and thus has no widget in 'b' list but has
					 * a node in 'a' list, thus we want to stick
					 * on this 'b' node until a more favorable 'a'
					 */
					nextb = b;
					(*pos)--;
					g_assert (info->type | CUSTOM_WIDGET);
#ifdef WIDGET_SYNC_DEBUG
					printf ("not dirty & not same, but has no widget\n");
#endif
				} else {
					g_warning ("non dirty node, but widget mismatch "
						   "a: '%s:%s', b: '%s:%s' '%p'",
						   matecomponent_ui_node_get_name (a) ? matecomponent_ui_node_get_name (a) : "NULL",
						   matecomponent_ui_node_peek_attr (a, "name") ? matecomponent_ui_node_peek_attr (a, "name") : "NULL",
						   bn ? (matecomponent_ui_node_get_name (bn) ? matecomponent_ui_node_get_name (bn) : "NULL") : "NULL",
						   bn ? (matecomponent_ui_node_peek_attr (bn, "name") ? matecomponent_ui_node_peek_attr (bn, "name") : "NULL" ) : "NULL",
						   info->widget);
				}
			}
#ifdef WIDGET_SYNC_DEBUG
			else
				printf ("not dirty & same: no change\n");
#endif
			(*pos)++;
		}

		if (matecomponent_ui_node_has_name (a, "placeholder")) {
			matecomponent_ui_engine_sync (
				engine, sync,
				matecomponent_ui_node_children (a),
				parent, &nextb, pos);

			hide_placeholder_if_empty_or_hidden (engine, a);
		}

		a = matecomponent_ui_node_next (a);
	}

	while (b && matecomponent_ui_sync_ignore_widget (sync, b->data))
		b = b->next;

	*widgets = b;
}

static void
check_excess_widgets (MateComponentUISync *sync, GList *wptr)
{
	if (wptr) {
		GList *b;
		int warned = 0;

		for (b = wptr; b; b = b->next) {
			MateComponentUINode *node;

			if (matecomponent_ui_sync_ignore_widget (sync, b->data))
				continue;
			
			if (!warned++)
				g_warning ("Excess widgets at the "
					   "end of the container; weird");

			node = matecomponent_ui_engine_widget_get_node (b->data);
			g_message ("Widget type '%s' with node: '%s'",
				   b->data ? G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (b->data)) : "NULL",
				   node ? matecomponent_ui_xml_make_path (node) : "NULL");
		}
	}
}

static void
do_sync (MateComponentUIEngine *engine,
	 MateComponentUISync   *sync,
	 MateComponentUINode   *node)
{
#ifdef WIDGET_SYNC_DEBUG
	fprintf (stderr, "Syncing ('%s') on node '%s'\n",
		 G_OBJECT_CLASS_NAME (sync),
		 matecomponent_ui_xml_make_path (node));
#endif
	matecomponent_ui_node_ref (node);

	if (matecomponent_ui_node_parent (node) == engine->priv->tree->root)
		matecomponent_ui_sync_update_root (sync, node);

	/* FIXME: it would be nice to sub-class again to get rid of this */
	if (matecomponent_ui_sync_has_widgets (sync)) {
		int       pos;
		GList    *widgets, *wptr;

		wptr = widgets = matecomponent_ui_sync_get_widgets (sync, node);

		pos = 0;
		matecomponent_ui_engine_sync (
			engine, sync, matecomponent_ui_node_children (node),
			matecomponent_ui_engine_node_get_widget (engine, node),
			&wptr, &pos);
		
		check_excess_widgets (sync, wptr);
		
		g_list_free (widgets);
	}

	matecomponent_ui_xml_clean (engine->priv->tree, node);

	matecomponent_ui_node_unref (node);
}

static void
seek_dirty (MateComponentUIEngine *engine,
	    MateComponentUISync   *sync,
	    MateComponentUINode   *node)
{
	MateComponentUIXmlData *info;

	if (!node)
		return;

	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);
	if (info->dirty) { /* Rebuild tree from here down */

		do_sync (engine, sync, node);

	} else {
		MateComponentUINode *l;

		for (l = matecomponent_ui_node_children (node); l;
		     l = matecomponent_ui_node_next (l))
			seek_dirty (engine, sync, l);
	}
}

/**
 * matecomponent_ui_engine_update_node:
 * @engine: the engine
 * @node: the node to start updating.
 * 
 * This function is used to write recursive synchronizers
 * and is intended only for internal / privilaged use.
 *
 * By the time this returns, due to re-enterancy, node
 * points at undefined memory.
 **/
void
matecomponent_ui_engine_update_node (MateComponentUIEngine *engine,
			      MateComponentUISync   *sync,
			      MateComponentUINode   *node)
{
	if (sync) {
		if (matecomponent_ui_sync_is_recursive (sync))
			seek_dirty (engine, sync, node);
		else 
			do_sync (engine, sync, node);
	}
#ifdef WIDGET_SYNC_DEBUG
	else if (!matecomponent_ui_node_has_name (node, "commands"))
		g_warning ("No syncer for '%s'",
			   matecomponent_ui_xml_make_path (node));
#endif
}

static void
dirty_by_cmd (MateComponentUIEngine *engine,
	      const char     *search_id)
{
	const GSList *l;

	g_return_if_fail (search_id != NULL);

/*	printf ("Dirty node by cmd if %s == %s on node '%s'\n", search_id, id,
	matecomponent_ui_xml_make_path (search));*/

	for (l = cmd_to_nodes (engine, search_id); l; l = l->next)
		matecomponent_ui_xml_set_dirty (engine->priv->tree, l->data);
}

static void
move_dirt_cmd_to_widget (MateComponentUIEngine *engine)
{
	MateComponentUINode *cmds, *l;

	cmds = matecomponent_ui_xml_get_path (engine->priv->tree, "/commands");

	if (!cmds)
		return;

	for (l = cmds->children; l; l = l->next) {
		MateComponentUIXmlData *data = matecomponent_ui_xml_get_data (
			engine->priv->tree, l);

		if (data->dirty) {
			const char *cmd_name;

			cmd_name = matecomponent_ui_node_get_attr_by_id (l, name_id);
			if (!cmd_name)
				g_warning ("Serious error, cmd without name");
			else
				dirty_by_cmd (engine, cmd_name);
		}
	}
}

static void
update_commands_state (MateComponentUIEngine *engine)
{
	MateComponentUINode *cmds, *l;
	GSList       *updates = NULL;

	cmds = matecomponent_ui_xml_get_path (engine->priv->tree, "/commands");

/*	g_warning ("Update commands state!");
	matecomponent_ui_engine_dump (priv->win, "before update");*/

	if (!cmds)
		return;

	for (l = cmds->children; l; l = l->next) {
		MateComponentUIXmlData *data = matecomponent_ui_xml_get_data (
			engine->priv->tree, l);
		const char *cmd_name;

		cmd_name = matecomponent_ui_node_get_attr_by_id (l, name_id);
		if (!cmd_name)
			g_warning ("Internal error; cmd with no id");

		else if (data->dirty)
			updates = make_updates_for_command (
				engine, updates, l, cmd_name);

		data->dirty = FALSE;
	}

	execute_state_updates (updates);
}

static void
process_state_updates (MateComponentUIEngine *engine)
{
	while (engine->priv->state_updates) {
		StateUpdate *su = engine->priv->state_updates->data;

		engine->priv->state_updates = g_slist_remove (
			engine->priv->state_updates, su);

		matecomponent_ui_sync_state_update (
			su->sync, su->widget, su->state);

		state_update_destroy (su);
	}
}

/**
 * matecomponent_ui_engine_update:
 * @engine: the engine.
 * 
 * This function is called to update the entire
 * UI model synchronizing any changes in it with
 * the widget tree where neccessary
 **/
void
matecomponent_ui_engine_update (MateComponentUIEngine *engine)
{
	MateComponentUINode *node;
	GSList *l;

	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if (engine->priv->frozen || !engine->priv->tree)
		return;

	ACCESS ("MateComponent: UI engine update - start");

	for (l = engine->priv->syncs; l; l = l->next)
		matecomponent_ui_sync_stamp_root (l->data);

	ACCESS ("MateComponent: UI engine update - after stamp");

	move_dirt_cmd_to_widget (engine);

	ACCESS ("MateComponent: UI engine update - after dirt transfer");

/*	matecomponent_ui_engine_dump (priv->win, "before update");*/

	for (node = matecomponent_ui_node_children (engine->priv->tree->root);
	     node; node = matecomponent_ui_node_next (node)) {
		MateComponentUISync *sync;

		if (!matecomponent_ui_node_get_name (node))
			continue;

		sync = find_sync_for_node (engine, node);

		matecomponent_ui_engine_update_node (engine, sync, node);
	}

	ACCESS ("MateComponent: UI engine update - after update nodes");

	update_commands_state (engine);

	ACCESS ("MateComponent: UI engine update - after cmd state");

	process_state_updates (engine);

	ACCESS ("MateComponent: UI engine update - end");

/*	matecomponent_ui_engine_dump (priv->win, "after update");*/
}

/**
 * matecomponent_ui_engine_queue_update:
 * @engine: the engine
 * @widget: the widget to update later
 * @node: the node
 * @cmd_node: the associated command's node
 * 
 * This function is used to queue a state update on
 * @widget, essentialy transfering any state from the 
 * XML model into the widget view. This is queued to
 * avoid re-enterancy problems.
 **/
void
matecomponent_ui_engine_queue_update (MateComponentUIEngine   *engine,
			       GtkWidget        *widget,
			       MateComponentUINode     *node,
			       MateComponentUINode     *cmd_node)
{
	StateUpdate  *su;
	MateComponentUISync *sync;
	
	g_return_if_fail (node != NULL);

	sync = find_sync_for_node (engine, node);
	g_return_if_fail (sync != NULL);

	su = state_update_new (
		sync, widget, 
		cmd_node != NULL ? cmd_node : node);

	if (su)
		engine->priv->state_updates = g_slist_prepend (
			engine->priv->state_updates, su);
}      

/**
 * matecomponent_ui_engine_build_control:
 * @engine: the engine
 * @node: the control node.
 * 
 * A helper function for synchronizers, this creates a control
 * if possible from the node's associated object, stamps the
 * node as containing a control and sets its widget.
 * 
 * Return value: a Control's GtkWidget.
 **/
GtkWidget *
matecomponent_ui_engine_build_control (MateComponentUIEngine *engine,
				MateComponentUINode   *node)
{
	GtkWidget *control = NULL;
	NodeInfo  *info = matecomponent_ui_xml_get_data (
		engine->priv->tree, node);

/*	fprintf (stderr, "Control '%p', type '%d' object '%p'\n",
	info->widget, info->type, info->object);*/

	if (info->widget) {
		control = info->widget;
		g_assert (info->widget->parent == NULL);

	} else if (info->object != CORBA_OBJECT_NIL) {

		control = matecomponent_widget_new_control_from_objref (
			info->object, CORBA_OBJECT_NIL);
		g_return_val_if_fail (control != NULL, NULL);
		
		info->type |= CUSTOM_WIDGET;
	}

	matecomponent_ui_sync_do_show_hide (NULL, node, NULL, control);

/*	fprintf (stderr, "Type on '%s' '%s' is %d widget %p\n",
		 matecomponent_ui_node_get_name (node),
		 matecomponent_ui_node_peek_attr (node, "name"),
		 info->type, info->widget);*/

	return control;
}

/* Node info accessors */

/**
 * matecomponent_ui_engine_stamp_custom:
 * @engine: the engine
 * @node: the node
 * 
 * Marks a node as containing a custom widget.
 **/
void
matecomponent_ui_engine_stamp_custom (MateComponentUIEngine *engine,
			       MateComponentUINode   *node)
{
	NodeInfo *info;
	
	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	info->type |= CUSTOM_WIDGET;
}

/**
 * matecomponent_ui_engine_node_get_object:
 * @engine: the engine
 * @node: the node
 * 
 * Return value: the CORBA_Object associated with a @node
 **/
CORBA_Object
matecomponent_ui_engine_node_get_object (MateComponentUIEngine   *engine,
				  MateComponentUINode     *node)
{
	NodeInfo *info;
	
	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	return info->object;
}

/**
 * matecomponent_ui_engine_node_is_dirty:
 * @engine: the engine
 * @node: the node
 * 
 * Return value: whether the @node is marked dirty
 **/
gboolean
matecomponent_ui_engine_node_is_dirty (MateComponentUIEngine *engine,
				MateComponentUINode   *node)
{
	MateComponentUIXmlData *data;
	
	data = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	return data->dirty;
}

/**
 * matecomponent_ui_engine_node_get_id:
 * @engine: the engine
 * @node: the node
 * 
 * Each component has an associated textual id or name - see
 * matecomponent_ui_engine_register_component
 *
 * Return value: the component id associated with the node
 **/
const char *
matecomponent_ui_engine_node_get_id (MateComponentUIEngine *engine,
			      MateComponentUINode   *node)
{
	MateComponentUIXmlData *data;
	
	data = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	return data->id;
}

/**
 * matecomponent_ui_engine_node_set_dirty:
 * @engine: the engine
 * @node: the node
 * @dirty: whether the node should be dirty.
 * 
 * Set @node s dirty bit to @dirty.
 **/
void
matecomponent_ui_engine_node_set_dirty (MateComponentUIEngine *engine,
				 MateComponentUINode   *node,
				 gboolean        dirty)
{
	MateComponentUIXmlData *data;
	
	data = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	data->dirty = dirty;
}

/**
 * matecomponent_ui_engine_node_get_widget:
 * @engine: the engine
 * @node: the node
 * 
 * Gets the widget associated with @node
 * 
 * Return value: the widget
 **/
GtkWidget *
matecomponent_ui_engine_node_get_widget (MateComponentUIEngine   *engine,
				  MateComponentUINode     *node)
{
	NodeInfo *info;

	g_return_val_if_fail (engine != NULL, NULL);
	
	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	return info->widget;
}


/* Helpers */

/**
 * matecomponent_ui_engine_get_attr:
 * @node: the node
 * @cmd_node: the command's node
 * @attr: the attribute name
 * 
 * This function is used to get node attributes in many
 * UI synchronizers, it first attempts to get the attribute
 * from @node, and if this fails falls back to @cmd_node.
 * 
 * Return value: the attr or NULL if it doesn't exist.
 **/
char *
matecomponent_ui_engine_get_attr (MateComponentUINode *node,
			   MateComponentUINode *cmd_node,
			   const char   *attr)
{
	char *txt;

	if ((txt = matecomponent_ui_node_get_attr (node, attr)))
		return txt;

	if (cmd_node && (txt = matecomponent_ui_node_get_attr (cmd_node, attr)))
		return txt;

	return NULL;
}

/**
 * matecomponent_ui_engine_add_hint:
 * @engine: the engine
 * @str: the hint string
 * 
 * This fires the 'add_hint' signal.
 **/
void
matecomponent_ui_engine_add_hint (MateComponentUIEngine   *engine,
			   const char       *str)
{
	g_signal_emit (G_OBJECT (engine),
		       signals [ADD_HINT], 0, str);
}

/**
 * matecomponent_ui_engine_remove_hint:
 * @engine: the engine
 * 
 * This fires the 'remove_hint' signal
 **/
void
matecomponent_ui_engine_remove_hint (MateComponentUIEngine *engine)
{
	g_signal_emit (G_OBJECT (engine),
		       signals [REMOVE_HINT], 0);
}

/**
 * matecomponent_ui_engine_emit_verb_on:
 * @engine: the engine
 * @node: the node
 * 
 * This fires the 'emit_verb' signal
 **/
void
matecomponent_ui_engine_emit_verb_on (MateComponentUIEngine   *engine,
			       MateComponentUINode     *node)
{
	g_signal_emit (G_OBJECT (engine),
		       signals [EMIT_VERB_ON], 0, node);
}

/**
 * matecomponent_ui_engine_emit_event_on:
 * @engine: the engine
 * @node: the node
 * @state: the new state of the node
 * 
 * This fires the 'emit_event_on' signal
 **/
void
matecomponent_ui_engine_emit_event_on (MateComponentUIEngine   *engine,
				MateComponentUINode     *node,
				const char       *state)
{
	g_signal_emit (G_OBJECT (engine),
		       signals [EMIT_EVENT_ON], 0,
		       node, state);
}

#define WIDGET_NODE_KEY "MateComponentUIEngine:NodeKey"

/**
 * matecomponent_ui_engine_widget_get_node:
 * @widget: the widget
 * 
 * Return value: the #MateComponentUINode associated with this widget
 **/
MateComponentUINode *
matecomponent_ui_engine_widget_get_node (GtkWidget *widget)
{
	g_return_val_if_fail (widget != NULL, NULL);
	
	return g_object_get_data (G_OBJECT (widget),
				  WIDGET_NODE_KEY);
}

/**
 * matecomponent_ui_engine_widget_attach_node:
 * @widget: the widget
 * @node: the node
 * 
 * Associate @node with @widget
 **/
void
matecomponent_ui_engine_widget_attach_node (GtkWidget    *widget,
				     MateComponentUINode *node)
{
	if (widget)
		g_object_set_data (G_OBJECT (widget),
				   WIDGET_NODE_KEY, node);
}

/**
 * matecomponent_ui_engine_widget_set_node:
 * @engine: the engine
 * @widget: the widget
 * @node: the node
 * 
 * Used internaly to associate a widget with a node,
 * some synchronisers need to be able to execute code
 * on widget creation.
 **/
void
matecomponent_ui_engine_widget_set_node (MateComponentUIEngine *engine,
				  GtkWidget      *widget,
				  MateComponentUINode   *node)
{
	MateComponentUISync *sync;

	/* FIXME: this looks broken. why is it public ?
	 * and why is it re-looking up the sync - v. slow */

	if (!widget)
		return;

	sync = find_sync_for_node (engine, node);

	sync_widget_set_node (sync, widget, node);
}

/**
 * matecomponent_ui_engine_get_cmd_node:
 * @engine: the engine
 * @from_node: the node
 * 
 * This function seeks the command node associated
 * with @from_node in @engine 's internal tree.
 * 
 * Return value: the command node or NULL
 **/
MateComponentUINode *
matecomponent_ui_engine_get_cmd_node (MateComponentUIEngine *engine,
			       MateComponentUINode   *from_node)
{
	char         *path;
	MateComponentUINode *ret;
	const char   *cmd_name;

	g_return_val_if_fail (engine != NULL, NULL);

	if (!from_node)
		return NULL;

	if (!(cmd_name = node_get_id (from_node)))
		return NULL;

	path = g_strconcat ("/commands/", cmd_name, NULL);
	ret  = matecomponent_ui_xml_get_path (engine->priv->tree, path);

	if (!ret) {
		MateComponentUIXmlData *data_from;
		MateComponentUINode *commands;
		MateComponentUINode *node;

		commands = matecomponent_ui_node_new ("commands");
		node     = matecomponent_ui_node_new_child (commands, "cmd");

		matecomponent_ui_node_set_attr (node, "name", cmd_name);

		data_from   = matecomponent_ui_xml_get_data (
			engine->priv->tree, from_node);

		matecomponent_ui_xml_merge (
			engine->priv->tree, "/",
			commands, data_from->id);
		
		ret = matecomponent_ui_xml_get_path (engine->priv->tree, path);
		g_assert (ret != NULL);
	}

	g_free (path);

	return ret;
}

/**
 * matecomponent_ui_engine_emit_verb_on_w:
 * @engine: the engine
 * @widget: the widget
 * 
 * This function looks up the node from @widget and
 * emits the 'emit_verb_on' signal on that node.
 **/
void
matecomponent_ui_engine_emit_verb_on_w (MateComponentUIEngine *engine,
				 GtkWidget      *widget)
{
	MateComponentUINode *node = matecomponent_ui_engine_widget_get_node (widget);

	g_signal_emit (G_OBJECT (engine),
		       signals [EMIT_VERB_ON], 0, node);
}

/**
 * matecomponent_ui_engine_emit_event_on_w:
 * @engine: the engine
 * @widget: the widget
 * @state: the new state
 * 
 * This function looks up the node from @widget and
 * emits the 'emit_event_on' signal on that node
 * passint @state as the new state.
 **/
void
matecomponent_ui_engine_emit_event_on_w (MateComponentUIEngine *engine,
				  GtkWidget      *widget,
				  const char     *state)
{
	MateComponentUINode *node = matecomponent_ui_engine_widget_get_node (widget);

	g_signal_emit (G_OBJECT (engine),
		       signals [EMIT_EVENT_ON], 0, node, state);
}

/**
 * matecomponent_ui_engine_stamp_root:
 * @engine: the engine
 * @node: the node
 * @widget: the root widget
 * 
 * This stamps @node with @widget which is marked as
 * being a ROOT node, so the engine will never destroy
 * it.
 **/
void
matecomponent_ui_engine_stamp_root (MateComponentUIEngine *engine,
			     MateComponentUINode   *node,
			     GtkWidget      *widget)
{
	NodeInfo *info;
	GtkWidget *new_root;

	if (!node)
		return;

	info = matecomponent_ui_xml_get_data (engine->priv->tree, node);

	new_root = widget ? g_object_ref (widget) : NULL;
	if (info->widget)
		g_object_unref (info->widget);
	info->widget = new_root;
	info->type |= ROOT_WIDGET;

	matecomponent_ui_engine_widget_attach_node (widget, node);
}

/**
 * matecomponent_ui_engine_get_path:
 * @engine: the engine.
 * @path: the path into the tree
 * 
 * This routine gets a node from the internal XML tree
 * pointed at by @path
 * 
 * Return value: the node.
 **/
MateComponentUINode *
matecomponent_ui_engine_get_path (MateComponentUIEngine *engine,
			   const char     *path)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);

	return matecomponent_ui_xml_get_path (engine->priv->tree, path);
}

/**
 * matecomponent_ui_engine_freeze:
 * @engine: the engine
 * 
 * This increments the freeze count on the tree, while
 * this count > 0 no syncronization between the internal
 * XML model and the widget views occurs. This means that
 * many simple merges can be glupped together with little
 * performance impact and overhead.
 **/
void
matecomponent_ui_engine_freeze (MateComponentUIEngine *engine)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	engine->priv->frozen++;
}

/**
 * matecomponent_ui_engine_thaw:
 * @engine: the engine
 * 
 * This decrements the freeze count and if it is 0
 * causes the UI widgets to be re-synched with the
 * XML model, see also matecomponent_ui_engine_freeze
 **/
void
matecomponent_ui_engine_thaw (MateComponentUIEngine *engine)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));
	
	if (--engine->priv->frozen <= 0) {
		matecomponent_ui_engine_update (engine);
		engine->priv->frozen = 0;
	}
}

/**
 * matecomponent_ui_engine_dump:
 * @engine: the engine
 * @out: the FILE stream to dump to
 * @msg: user visible message
 * 
 * This is a debugging function mostly for internal
 * and testing use, it dumps the XML tree, including
 * the associated, and overridden nodes in a wierd
 * hackish format to the @out stream with the
 * helpful @msg prepended.
 **/
void
matecomponent_ui_engine_dump (MateComponentUIEngine *engine,
		       FILE           *out,
		       const char     *msg)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	fprintf (out, "MateComponent UI Engine : frozen '%d'\n",
		 engine->priv->frozen);

	sub_components_dump (engine, out);

	/* FIXME: propagate the FILE * */
	matecomponent_ui_xml_dump (engine->priv->tree,
			    engine->priv->tree->root, msg);
}

/**
 * matecomponent_ui_engine_dirty_tree:
 * @engine: the engine
 * @node: the node
 * 
 * Mark all the node's children as being dirty and needing
 * a re-synch with their widget views.
 **/
void
matecomponent_ui_engine_dirty_tree (MateComponentUIEngine *engine,
			     MateComponentUINode   *node)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

#ifdef WIDGET_SYNC_DEBUG
	fprintf (stderr, "Dirty tree '%s'",
		 matecomponent_ui_xml_make_path (node));
#endif
	if (node) {
		matecomponent_ui_xml_set_dirty (engine->priv->tree, node);

		matecomponent_ui_engine_update (engine);
	}
}

/**
 * matecomponent_ui_engine_clean_tree:
 * @engine: the engine
 * @node: the node
 * 
 * This cleans the tree, marking the node and its children
 * as not needing a re-synch with their widget views.
 **/
void
matecomponent_ui_engine_clean_tree (MateComponentUIEngine *engine,
			     MateComponentUINode   *node)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine));

	if (node)
		matecomponent_ui_xml_clean (engine->priv->tree, node);
}

/**
 * matecomponent_ui_engine_get_xml:
 * @engine: the engine
 * 
 * Private - internal API
 *
 * Return value: the #MateComponentUIXml engine used for
 * doing the XML merge logic
 **/
MateComponentUIXml *
matecomponent_ui_engine_get_xml (MateComponentUIEngine *engine)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);
	
	return engine->priv->tree;
}

/**
 * matecomponent_ui_engine_get_config:
 * @engine: the engine
 * 
 * Private - internal API
 *
 * Return value: the associated configuration engine
 **/
MateComponentUIEngineConfig *
matecomponent_ui_engine_get_config (MateComponentUIEngine *engine)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), NULL);
	
	return engine->priv->config;
}

void
matecomponent_ui_engine_exec_verb (MateComponentUIEngine    *engine,
			    const CORBA_char  *cname,
			    CORBA_Environment *ev)
{
	g_return_if_fail (ev != NULL);
	g_return_if_fail (cname != NULL);
	matecomponent_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), ev);

	g_warning ("Emit Verb '%s'", cname);
}

void
matecomponent_ui_engine_ui_event (MateComponentUIEngine                    *engine,
			   const CORBA_char                  *id,
			   const MateComponent_UIComponent_EventType type,
			   const CORBA_char                  *state,
			   CORBA_Environment                 *ev)
{
	g_return_if_fail (ev != NULL);
	g_return_if_fail (id != NULL);
	g_return_if_fail (state != NULL);
	matecomponent_return_if_fail (MATECOMPONENT_IS_UI_ENGINE (engine), ev);

	g_warning ("Emit UI Event '%s' %s'", id, state);
}

