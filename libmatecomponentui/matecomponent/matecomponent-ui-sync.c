/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * matecomponent-ui-sync.h: An abstract base for MateComponent XML / widget sync sync'ing.
 *
 * Author:
 *	Michael Meeks (michael@ximian.com)
 *
 * Copyright 2000 Ximian, Inc.
 */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-sync.h>

#define PARENT_TYPE G_TYPE_OBJECT
#define CLASS(o) MATECOMPONENT_UI_SYNC_CLASS (G_OBJECT_GET_CLASS (o))

static void
impl_sync_state_placeholder (MateComponentUISync     *sync,
			     MateComponentUINode     *node,
			     MateComponentUINode     *cmd_node,
			     GtkWidget        *widget,
			     GtkWidget        *parent)
{
	gboolean show = FALSE;
	char    *txt;
		
	if ((txt = matecomponent_ui_engine_get_attr (
		node, cmd_node, "delimit"))) {

		show = !strcmp (txt, "top");
		matecomponent_ui_node_free_string (txt);
	}
		
	if (show)
		gtk_widget_show (widget);	
	else
		gtk_widget_hide (widget);
}

static void
class_init (MateComponentUISyncClass *sync_class)
{
	sync_class->sync_state_placeholder = impl_sync_state_placeholder;
}

/**
 * matecomponent_ui_sync_get_type:
 * @void: 
 * 
 * Synchronizer type function for derivation.
 * 
 * Return value: the GType index.
 **/
GType
matecomponent_ui_sync_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (MateComponentUISyncClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentUISync),
			0, /* n_preallocs */
			(GInstanceInitFunc) NULL
		};

		type = g_type_register_static (PARENT_TYPE, "MateComponentUISync",
					       &info, 0);
	}

	return type;
}

/**
 * matecomponent_ui_sync_construct:
 * @sync: the synchronizer
 * @engine: the associated engine
 * @is_recursive: whether it deals with its children recursively
 * @has_widgets: whether it has associated widgets.
 * 
 * Used to construct a new synchronizer object
 * 
 * Return value: the new object.
 **/
MateComponentUISync *
matecomponent_ui_sync_construct (MateComponentUISync   *sync,
			  MateComponentUIEngine *engine,
			  gboolean        is_recursive,
			  gboolean        has_widgets)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), NULL);

	sync->engine = engine;
	sync->is_recursive = is_recursive;
	sync->has_widgets  = has_widgets;

	return sync;
}

/**
 * matecomponent_ui_sync_is_recursive:
 * @sync: the synchronizer
 * 
 * Return value: whether this deals with its children recursively
 **/
gboolean
matecomponent_ui_sync_is_recursive (MateComponentUISync *sync)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), FALSE);

	return sync->is_recursive;
}

/**
 * matecomponent_ui_sync_has_widgets:
 * @sync: the synchronizer 
 * 
 * Return value: whether this deals with widgets
 **/
gboolean
matecomponent_ui_sync_has_widgets (MateComponentUISync *sync)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), FALSE);

	return sync->has_widgets;
}

/**
 * matecomponent_ui_sync_state:
 * @sync: the synchronizer 
 * @node: the node 
 * @cmd_node: the associated command node 
 * @widget: the widget 
 * @parent: the parent of @node
 * 
 * This method is used to synchronize the state of a @node
 * with that of a @widget, by ensuring the pertainant
 * attributes are reflected in the widget view.
 **/
void
matecomponent_ui_sync_state (MateComponentUISync     *sync,
		      MateComponentUINode     *node,
		      MateComponentUINode     *cmd_node,
		      GtkWidget        *widget,
		      GtkWidget        *parent)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_SYNC (sync));

	CLASS (sync)->sync_state (sync, node, cmd_node, widget, parent);
}

/**
 * matecomponent_ui_sync_state_placeholder:
 * @sync: the synchronizer 
 * @node: the node 
 * @cmd_node: the associated command node 
 * @widget: the widget 
 * @parent: the parent of @node
 * 
 * This synchronizes the state of a placeholder, there is
 * a default implementation for this method.
 **/
void
matecomponent_ui_sync_state_placeholder (MateComponentUISync     *sync,
				  MateComponentUINode     *node,
				  MateComponentUINode     *cmd_node,
				  GtkWidget        *widget,
				  GtkWidget        *parent)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_SYNC (sync));

	CLASS (sync)->sync_state_placeholder (sync, node, cmd_node, widget, 
					      parent);
}

/**
 * matecomponent_ui_sync_build:
 * @sync: the synchronizer 
 * @node: the node 
 * @cmd_node: the associated command node 
 * @pos: the position in the parent container to insert at
 * @parent: the parent of @node 
 * 
 * This function causes a child widget to be build that matches
 * @node's attributes. This should then be inserted by into
 * @parent's associated widget at position @pos in the container.
 * 
 * Return value: the freshly built widget.
 **/
GtkWidget *
matecomponent_ui_sync_build (MateComponentUISync     *sync,
		      MateComponentUINode     *node,
		      MateComponentUINode     *cmd_node,
		      int              *pos,
		      GtkWidget        *parent)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), NULL);

	return CLASS (sync)->build (sync, node, cmd_node, pos, parent);
}

/**
 * matecomponent_ui_sync_build_placeholder:
 * @sync: the synchronizer 
 * @node: the node 
 * @cmd_node: the associated command node 
 * @pos: position in the parent to insert the built widget
 * @parent: the parent of @node 
 * 
 * As for #matecomponent_ui_sync_build but for placeholders
 * 
 * Return value: the freshly built widget.
 **/
GtkWidget *
matecomponent_ui_sync_build_placeholder (MateComponentUISync     *sync,
				  MateComponentUINode     *node,
				  MateComponentUINode     *cmd_node,
				  int              *pos,
				  GtkWidget        *parent)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), NULL);

	return CLASS (sync)->build_placeholder (
		sync, node, cmd_node, pos, parent);
}

/**
 * matecomponent_ui_sync_get_widgets:
 * @sync: the synchronizer 
 * @node: the node 
 * 
 * This method is used to obtain a sensibly ordered list
 * of child widgets of the container associated with @node.
 * Essentialy this does something like gtk_container_children
 * but preserving the visible order of the widgets in the list.
 * 
 * Return value: An ordered list of child widgets of @node
 **/
GList *
matecomponent_ui_sync_get_widgets (MateComponentUISync *sync,
			    MateComponentUINode *node)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), NULL);

	if (CLASS (sync)->get_widgets)
		return CLASS (sync)->get_widgets (sync, node);
	else
		return NULL;
}

/**
 * matecomponent_ui_sync_state_update:
 * @sync: the synchronizer 
 * @widget: the widget 
 * @new_state: the new state
 * 
 * This is used to synchronize state with a stateful widget,
 * eg. when a "state" attribute is set, this is not reflected
 * in the normal 'state-sync' process, but occurs later with
 * a set of state_updates to avoid re-enterancy problems.
 **/
void
matecomponent_ui_sync_state_update (MateComponentUISync     *sync,
			     GtkWidget        *widget,
			     const char       *new_state)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_SYNC (sync));

	CLASS (sync)->state_update (sync, widget, new_state);
}

/**
 * matecomponent_ui_sync_remove_root:
 * @sync: the synchronizer 
 * @root: the toplevel node to be removed.
 * 
 * This is called when a 'root' or toplevel node is
 * removed that this synchronizer deals with. eg. in
 * the toolbar case, this might trigger hiding an
 * associated dock item.
 **/
void
matecomponent_ui_sync_remove_root (MateComponentUISync *sync,
			    MateComponentUINode *root)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_SYNC (sync));

	if (CLASS (sync)->remove_root)
		CLASS (sync)->remove_root (sync, root);
}

/**
 * matecomponent_ui_sync_update_root:
 * @sync: the synchronizer 
 * @root: the toplevel node
 * 
 * This flags the fact that a toplevel node has changed
 * and is used primarily by non-recursive handlers, such
 * as the keybinding sync method.
 **/
void
matecomponent_ui_sync_update_root (MateComponentUISync *sync,
			    MateComponentUINode *root)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_SYNC (sync));

	if (CLASS (sync)->update_root)
		CLASS (sync)->update_root (sync, root);
}

/**
 * matecomponent_ui_sync_ignore_widget:
 * @sync: the synchronizer 
 * @widget: the widget 
 * 
 * Return value: TRUE if this widget should be ignored in a container
 * this is the case for eg. menu tearoffs items, and toolbar popout items.
 **/
gboolean
matecomponent_ui_sync_ignore_widget (MateComponentUISync *sync,
			      GtkWidget    *widget)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), FALSE);

	if (CLASS (sync)->ignore_widget)
		return CLASS (sync)->ignore_widget (sync, widget);
	else
		return FALSE;
}

/**
 * matecomponent_ui_sync_stamp_root:
 * @sync: the synchronizer 
 * 
 * This asks the synchronizer to stamp all its associated
 * root widget containers into the XML tree.
 **/
void
matecomponent_ui_sync_stamp_root (MateComponentUISync *sync)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_SYNC (sync));

	if (CLASS (sync)->stamp_root)
		CLASS (sync)->stamp_root (sync);
}

/**
 * matecomponent_ui_sync_can_handle:
 * @sync: the synchronizer 
 * @node: the node 
 * 
 * This is used to determine which, of multiple synchronizers
 * can be used to deal with a specific node type. Each synchronizer
 * deals with different types of node.
 * 
 * Return value: TRUE if the synchronizer can deal with this node type
 **/
gboolean
matecomponent_ui_sync_can_handle (MateComponentUISync *sync,
			   MateComponentUINode *node)
{
	if (CLASS (sync)->can_handle)
		return CLASS (sync)->can_handle (sync, node);
	else
		return FALSE;
}

/**
 * matecomponent_ui_sync_get_attached:
 * @sync: the synchronizer 
 * @widget: the widget 
 * @node: the node 
 * 
 * This is used to get an 'attached' widget - some
 * widgets have associated widgets that are coupled
 * in strange ways - eg. GtkMenuItem <-> GtkMenuShell
 * It is neccessary to store the GtkContainer item of
 * these couples in the XML tree, since then we can
 * do things more genericaly and cleanly.
 * 
 * Return value: an associated widget or NULL if none exists.
 **/
GtkWidget *
matecomponent_ui_sync_get_attached (MateComponentUISync *sync,
			     GtkWidget    *widget,
			     MateComponentUINode *node)
{
	/*
	 *   For some widgets such as menus, the submenu widget
	 * is attached to the actual container in a strange way
	 * this works around only having single inheritance.
	 */
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), NULL);

	if (CLASS (sync)->get_attached)
		return CLASS (sync)->get_attached (sync, widget, node);
	else
		return NULL;
}

/**
 * matecomponent_ui_sync_do_show_hide:
 * @sync: the synchronizer 
 * @node: the node 
 * @cmd_node: the associated command node 
 * @widget: the widget 
 * 
 * This is a helper function that applies the hidden attribute
 * from either the @node or fallback to the @cmd_node to the
 * @widget.
 * 
 * Return value: TRUE if the widget's hidden / shown state changed,
 * this is needed to work around some nasty dock sizing bugs.
 **/
gboolean
matecomponent_ui_sync_do_show_hide (MateComponentUISync *sync,
			     MateComponentUINode *node,
			     MateComponentUINode *cmd_node,
			     GtkWidget    *widget)
{
	char      *txt;
	gboolean   changed;
	GtkWidget *attached;

	if (sync && 
	    (attached = matecomponent_ui_sync_get_attached (
		    sync, widget, node)))

		widget = attached;

	if (!widget)
		return FALSE;

	if ((txt = matecomponent_ui_engine_get_attr (node, cmd_node, "hidden"))) {
		if (atoi (txt)) {
			changed = GTK_WIDGET_VISIBLE (widget);
			gtk_widget_hide (widget);
		} else {
			changed = !GTK_WIDGET_VISIBLE (widget);
			gtk_widget_show (widget);
		}
		matecomponent_ui_node_free_string (txt);
	} else {
		changed = !GTK_WIDGET_VISIBLE (widget);
		gtk_widget_show (widget);
	}

	return changed;
}

GtkWidget *
matecomponent_ui_sync_wrap_widget (MateComponentUISync *sync,
			    GtkWidget    *custom_widget)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_SYNC (sync), NULL);

	if (CLASS (sync)->wrap_widget)
		return CLASS (sync)->wrap_widget (sync, custom_widget);
	else
		return custom_widget;
}
