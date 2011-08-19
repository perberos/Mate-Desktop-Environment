/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * MateComponent accessibility helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2002 Sun Microsystems, Inc.
 */
#include "config.h"
#include <glib.h>
#include <atk/atkregistry.h>
#include <atk/atkobjectfactory.h>
#include <gtk/gtk.h>
#include <matecomponent-a11y.h>

GType
matecomponent_a11y_get_derived_type_for (GType                 widget_type,
				  const char           *gail_parent_class,
				  MateComponentA11YClassInitFn class_init)
{
	GType type;
	GType parent_atk_type;
	GTypeInfo tinfo = { 0 };
	GTypeQuery query;
	char *type_name;
		
	parent_atk_type = g_type_from_name (
		gail_parent_class ? gail_parent_class : "GailWidget");

	g_return_val_if_fail (parent_atk_type != G_TYPE_INVALID, G_TYPE_INVALID);

	/*
	 * Figure out the size of the class and instance 
	 * we are deriving from
	 */
	g_type_query (parent_atk_type, &query);

	tinfo.class_init    = (GClassInitFunc) class_init;
	tinfo.class_size    = query.class_size;
	tinfo.instance_size = query.instance_size;

	/* Make up a name */
	type_name = g_strconcat (g_type_name (widget_type),
				 "Accessible", NULL);

	/* Register the type */
	type = g_type_register_static (
		parent_atk_type, type_name, &tinfo, 0);

	g_free (type_name);
		
	return type;
}

AtkObject *
matecomponent_a11y_create_accessible_for (GtkWidget            *widget,
				   const char           *gail_parent_class,
				   MateComponentA11YClassInitFn class_init,
				   GType                 first_interface_type,
				   ...)
{
	va_list args;
	AtkObject *accessible;
	GType type, widget_type;
	static GHashTable *type_hash = NULL;

	va_start (args, first_interface_type);

	accessible = matecomponent_a11y_get_atk_object (widget);

	if (accessible)
		goto out;

	if (!type_hash)
		type_hash = g_hash_table_new (NULL, NULL);

	widget_type = G_TYPE_FROM_INSTANCE (widget);
	type = (GType) g_hash_table_lookup (type_hash, (gpointer) widget_type);

	if (!type) {
		GType it;

		type = matecomponent_a11y_get_derived_type_for (
			widget_type, gail_parent_class, class_init);

		g_return_val_if_fail (type != G_TYPE_INVALID, NULL);

		for (it = first_interface_type; it; it = va_arg (args, GType)) {
			const GInterfaceInfo *if_info = va_arg (args, gpointer);
			
			g_type_add_interface_static (type, it, if_info);
		}

		g_hash_table_insert (type_hash,
				     (gpointer) widget_type,
				     (gpointer) type);
	}

	g_return_val_if_fail (type != G_TYPE_INVALID, NULL);
					    
	accessible = g_object_new (type, NULL);

	matecomponent_a11y_set_atk_object_ret (widget, accessible);

 out:
	va_end (args);

	return accessible;
}

static GQuark
get_quark_accessible (void)
{
	static GQuark quark_accessible_object = 0;

	if (!quark_accessible_object)
		quark_accessible_object = g_quark_from_static_string (
			"gtk-accessible-object");

	return quark_accessible_object;
}

AtkObject *
matecomponent_a11y_get_atk_object (gpointer widget)
{
	return g_object_get_qdata (widget, get_quark_accessible ());
}

AtkObject *
matecomponent_a11y_set_atk_object_ret (GtkWidget *widget,
				AtkObject *object)
{
	atk_object_initialize (object, widget);
	
	g_object_set_qdata (
		G_OBJECT (widget),
		get_quark_accessible (),
		object);

	return object;
}

typedef struct {
	AtkActionIface  chain;
	GArray         *actions;
} MateComponentActionInterface;

typedef struct {
	AtkActionIface  chain;
	GArray         *actions;
} MateComponentActionIfData;

typedef struct {
	char *name;
	char *description;
	char *keybinding;
} MateComponentAction;

#define MATECOMPONENT_TYPE_ACTION matecomponent_a11y_action_get_type ()

static GType
matecomponent_a11y_action_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo tinfo = {
			sizeof (MateComponentActionInterface),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,

		};

		type = g_type_register_static (
			G_TYPE_INTERFACE, "MateComponentAction", &tinfo, 0);
	}

	return type;
}

static gboolean
matecomponent_a11y_action_do (AtkAction *action,
		       gint       i)
{
	MateComponentActionInterface *aif = 
		G_TYPE_INSTANCE_GET_INTERFACE (
			action, MATECOMPONENT_TYPE_ACTION, MateComponentActionInterface);

	if (aif->chain.do_action)
		return aif->chain.do_action (action, i);

	else
		g_warning ("matecomponent a11y action %d unimplemented on %p",
			   i, action);

	return FALSE;
}

static gint
matecomponent_a11y_action_get_n (AtkAction *action)
{
	int i, count;
	MateComponentActionInterface *aif = 
		G_TYPE_INSTANCE_GET_INTERFACE (
			action, MATECOMPONENT_TYPE_ACTION, MateComponentActionInterface);

	if (aif->chain.get_n_actions)
		return aif->chain.get_n_actions (action);

	for (i = count = 0; i < aif->actions->len; i++) {
		if (g_array_index (aif->actions, MateComponentAction, i).name)
			count++;
	}

	return count;
}

static G_CONST_RETURN gchar *
matecomponent_a11y_action_get_description (AtkAction *action,
				    gint       i)
{
	MateComponentActionInterface *aif = 
		G_TYPE_INSTANCE_GET_INTERFACE (
			action, MATECOMPONENT_TYPE_ACTION, MateComponentActionInterface);

	if (aif->chain.get_description)
		return aif->chain.get_description (action, i);

	/* I refuse to handle set_description it seems uber ugly */
	if (i >= 0 && i < aif->actions->len &&
	    g_array_index (aif->actions, MateComponentAction, i).description)
		return g_array_index (aif->actions, MateComponentAction, i).description;
	else
		return NULL;
}

static G_CONST_RETURN gchar *
matecomponent_a11y_action_get_name (AtkAction *action,
			     gint       i)
{
	MateComponentActionInterface *aif = 
		G_TYPE_INSTANCE_GET_INTERFACE (
			action, MATECOMPONENT_TYPE_ACTION, MateComponentActionInterface);

	if (aif->chain.get_name)
		return aif->chain.get_name (action, i);

	if (i >= 0 && i < aif->actions->len &&
	    g_array_index (aif->actions, MateComponentAction, i).name)
		return g_array_index (aif->actions, MateComponentAction, i).name;
	else
		return NULL;
}

static G_CONST_RETURN gchar *
matecomponent_a11y_action_get_keybinding (AtkAction *action,
				   gint       i)
{
	MateComponentActionInterface *aif = 
		G_TYPE_INSTANCE_GET_INTERFACE (
			action, MATECOMPONENT_TYPE_ACTION, MateComponentActionInterface);

	if (aif->chain.get_keybinding)
		return aif->chain.get_keybinding (action, i);

	if (i >= 0 && i < aif->actions->len &&
	    g_array_index (aif->actions, MateComponentAction, i).keybinding)
		return g_array_index (aif->actions, MateComponentAction, i).keybinding;
	else
		return NULL;
}

static gboolean
matecomponent_a11y_action_set_description (AtkAction   *action,
				    gint         i,
				    const gchar *desc)
{
	MateComponentActionInterface *aif = 
		G_TYPE_INSTANCE_GET_INTERFACE (
			action, MATECOMPONENT_TYPE_ACTION, MateComponentActionInterface);

	if (aif->chain.set_description)
		return aif->chain.set_description (action, i, desc);

	/* I refuse to handle set_description it seems uber ugly */
	return FALSE;
}

static void
matecomponent_a11y_action_if_init (gpointer g_iface,
			    gpointer iface_data)
{
	MateComponentActionIfData *aifd = iface_data;
	MateComponentActionInterface *aif = g_iface;

	aif->chain   = aifd->chain;
	aif->actions = aifd->actions;
}

static void
matecomponent_a11y_atk_action_if_init (gpointer g_iface,
				gpointer iface_data)
{
	AtkActionIface *aif = g_iface;

	aif->do_action       = matecomponent_a11y_action_do;
	aif->get_n_actions   = matecomponent_a11y_action_get_n;
	aif->get_description = matecomponent_a11y_action_get_description;
	aif->get_name        = matecomponent_a11y_action_get_name;
	aif->get_keybinding  = matecomponent_a11y_action_get_keybinding;
	aif->set_description = matecomponent_a11y_action_set_description;
}

static void
matecomponent_a11y_action_if_finalize (gpointer g_iface,
				gpointer iface_data)
{
	/* FIXME: should we free aifd->actions etc. here ? */
}

void
matecomponent_a11y_add_actions_interface (GType           a11y_object_type,
				   AtkActionIface *chain,
				   /* triplets of: */
				   int             first_id,
				   /* char * action name */
				   /* char * initial action description */
				   /* char * keybinding descr. */
				   ...)
{
	va_list args; 
	int     id;
	GInterfaceInfo iinfo;
	MateComponentActionIfData *aifd = g_new0 (MateComponentActionIfData, 1);

	va_start (args, first_id);

	aifd->chain = *chain;

	aifd->actions = g_array_new (FALSE, TRUE, sizeof (MateComponentAction));

	for (id = first_id; id >= 0; id = va_arg (args, int)) {
		if (id >= aifd->actions->len)
			g_array_set_size (aifd->actions, id + 2);

		g_array_index (aifd->actions, MateComponentAction, id).name =
			g_strdup (va_arg (args, char *));
		g_array_index (aifd->actions, MateComponentAction, id).description =
			g_strdup (va_arg (args, char *));
		g_array_index (aifd->actions, MateComponentAction, id).keybinding =
			g_strdup (va_arg (args, char *));
	}

	iinfo.interface_init     = matecomponent_a11y_action_if_init;
	iinfo.interface_finalize = matecomponent_a11y_action_if_finalize;
	iinfo.interface_data     = aifd;

	g_type_add_interface_static (
		a11y_object_type,
		matecomponent_a11y_action_get_type (),
		&iinfo);

	iinfo.interface_init     = matecomponent_a11y_atk_action_if_init;
	iinfo.interface_finalize = NULL;
	iinfo.interface_data     = NULL;

	g_type_add_interface_static (
		a11y_object_type, ATK_TYPE_ACTION, &iinfo);

	va_end (args);
}



