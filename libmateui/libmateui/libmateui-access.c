/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright 2002 Sun Microsystems Inc.
 *
 * Lib MATE UI Accessibility support module
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <config.h>
#include <gtk/gtk.h>
#include "libmateui-access.h"

static gint is_gail_loaded (GtkWidget *widget);

/* variable that indicates whether GAIL is loaded or not */
static gint gail_loaded = -1;

/* Accessibility support routines for libmateui */
static gint
is_gail_loaded (GtkWidget *widget)
{
	AtkObject *aobj;
	if (gail_loaded == -1) {
		aobj = gtk_widget_get_accessible (widget);
		if (!GTK_IS_ACCESSIBLE (aobj))
			gail_loaded = 0;
		else
			gail_loaded = 1;
	}
	return gail_loaded;
}

/* routine to add accessible name and description to an atk object */
void
_add_atk_name_desc (GtkWidget *widget, gchar *name, gchar *desc)
{
	AtkObject *aobj;

	g_return_if_fail (GTK_IS_WIDGET (widget));

	if (! is_gail_loaded (widget))
		return;

	aobj = gtk_widget_get_accessible (widget);

	if (name)
		atk_object_set_name (aobj, name);
	if (desc)
		atk_object_set_description (aobj, desc);
}

void
_add_atk_description (GtkWidget *widget,
		      gchar     *desc)
{
	_add_atk_name_desc (widget, NULL, desc);
}

void
_add_atk_relation (GtkWidget *widget1, GtkWidget *widget2,
		   AtkRelationType w1_to_w2, AtkRelationType w2_to_w1)
{
	AtkObject      *atk_widget1;
	AtkObject      *atk_widget2;
	AtkRelationSet *relation_set;
	AtkRelation    *relation;
	AtkObject      *targets[1];

	atk_widget1 = gtk_widget_get_accessible (widget1);
	atk_widget2 = gtk_widget_get_accessible (widget2);

	/* Create the widget1 -> widget2 relation */
	relation_set = atk_object_ref_relation_set (atk_widget1);
	targets[0] = atk_widget2;
	relation = atk_relation_new (targets, 1, w1_to_w2);
	atk_relation_set_add (relation_set, relation);
	g_object_unref (relation);
	g_object_unref (relation_set);

	/* Create the widget2 -> widget1 relation */
	relation_set = atk_object_ref_relation_set (atk_widget2);
	targets[0] = atk_widget1;
	relation = atk_relation_new (targets, 1, w2_to_w1);
	atk_relation_set_add (relation_set, relation);
	g_object_unref (relation);
	g_object_unref (relation_set);
}



/* Copied from eel */

static GQuark
get_quark_accessible (void)
{
	static GQuark quark_accessible_object = 0;

	if (!quark_accessible_object) {
		quark_accessible_object = g_quark_from_static_string
			("accessible-object");
	}

	return quark_accessible_object;
}

static GQuark
get_quark_gobject (void)
{
	static GQuark quark_accessible_gobject = 0;

	if (!quark_accessible_gobject) {
		quark_accessible_gobject = g_quark_from_static_string
			("object-for-accessible");
	}

	return quark_accessible_gobject;
}

/**
 * _accessibility_get_atk_object:
 * @object: a GObject of some sort
 * 
 * gets an AtkObject associated with a GObject
 * 
 * Return value: the associated accessible if one exists or NULL
 **/
AtkObject *
_accessibility_get_atk_object (gpointer object)
{
	return g_object_get_qdata (object, get_quark_accessible ());
}

/**
 * _accessibilty_for_object:
 * @object: a GObject of some sort
 * 
 * gets an AtkObject associated with a GObject and if it doesn't
 * exist creates a suitable accessible object.
 * 
 * Return value: an associated accessible.
 **/
AtkObject *
_accessibility_for_object (gpointer object)
{
	if (GTK_IS_WIDGET (object))
		return gtk_widget_get_accessible (object);

	return atk_gobject_accessible_for_object (object);
}

/**
 * _accessibility_get_gobject:
 * @object: an AtkObject
 * 
 * gets the GObject associated with the AtkObject, for which
 * @object provides accessibility support.
 * 
 * Return value: the accessible's associated GObject
 **/
gpointer
_accessibility_get_gobject (AtkObject *object)
{
	return g_object_get_qdata (G_OBJECT (object), get_quark_gobject ());
}

static void
_accessibility_weak_unref (gpointer data,
			   GObject *where_the_object_was)
{
	g_object_set_qdata (data, get_quark_gobject (), NULL);

	atk_object_notify_state_change
		(ATK_OBJECT (data), ATK_STATE_DEFUNCT, TRUE); 

	g_object_unref (data);
}

/**
 * _accessibility_set_atk_object_return:
 * @object: a GObject
 * @atk_object: it's AtkObject
 * 
 * used to register and return a new accessible object for something
 * 
 * Return value: @atk_object.
 **/
AtkObject *
_accessibility_set_atk_object_return (gpointer   object,
					 AtkObject *atk_object)
{
	atk_object_initialize (atk_object, object);

	if (!ATK_IS_GOBJECT_ACCESSIBLE (atk_object)) {
		g_object_weak_ref (object, _accessibility_weak_unref, atk_object);
		g_object_set_qdata
			(object, get_quark_accessible (), atk_object);
		g_object_set_qdata
			(G_OBJECT (atk_object), get_quark_gobject (), object);
	}

	return atk_object;
}

/**
 * _accessibility_create_derived_type:
 * @type_name: the name for the new accessible type eg. CajaIconCanvasItemAccessible
 * @existing_gobject_with_proxy: the GType of an object that has a registered factory that
 *      manufactures the type we want to inherit from. ie. to inherit from a GailCanvasItem
 *      we need to pass MATE_TYPE_CANVAS_ITEM - since GailCanvasItem is registered against
 *      that type.
 * @class_init: the init function to run for this class
 * 
 * This should be run to register the type, it can subsequently be run with
 * the same name and will not re-register it, but simply return it.
 *
 * NB. to do instance init, you prolly want to override AtkObject::initialize
 * 
 * Return value: the registered type, or 0 on failure.
 **/
GType
_accessibility_create_derived_type (const char *type_name,
				    GType existing_gobject_with_proxy,
				    _AccessibilityClassInitFn class_init)
{
	GType type;
	GType parent_atk_type;
	GTypeInfo tinfo = { 0 };
	GTypeQuery query;
	AtkObjectFactory *factory;

	if ((type = g_type_from_name (type_name))) {
		return type;
	}

	factory = atk_registry_get_factory
		(atk_get_default_registry (),
		 existing_gobject_with_proxy);
	if (!factory) {
		return G_TYPE_INVALID;
	}
	
	parent_atk_type = atk_object_factory_get_accessible_type (factory);
	if (!parent_atk_type) {
		return G_TYPE_INVALID;
	}

	/*
	 * Figure out the size of the class and instance 
	 * we are deriving from
	 */
	g_type_query (parent_atk_type, &query);

	if (class_init) {
		tinfo.class_init = (GClassInitFunc) class_init;
	}

	tinfo.class_size    = query.class_size;
	tinfo.instance_size = query.instance_size;

	/* Register the type */
	type = g_type_register_static (
		parent_atk_type, type_name, &tinfo, 0);

	return type;
}
