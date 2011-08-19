/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-item-container.h: a generic container for monikers.
 *
 * The MateComponentItemContainer object represents a document which may have one
 * or more embedded document objects.  To embed an object in the
 * container, create a MateComponentClientSite, add it to the container, and
 * then create an object supporting MateComponent::Embeddable and bind it to
 * the client site.  The MateComponentItemContainer maintains a list of the client
 * sites which correspond to the objects embedded in the container.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999, 2000 Ximian, Inc.
 */
#include <config.h>
#include <glib-object.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-item-container.h>
#include <matecomponent/matecomponent-marshal.h>
#include <matecomponent/matecomponent-types.h>

enum {
	GET_OBJECT,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static GObjectClass *matecomponent_item_container_parent_class;

struct _MateComponentItemContainerPrivate {
	GHashTable *objects;
};

static gboolean
remove_object (gpointer key,
	       gpointer value,
	       gpointer user_data)
{
	g_free (key);
	matecomponent_object_unref (value);

	return TRUE;
}

static void
matecomponent_item_container_finalize (GObject *object)
{
	MateComponentItemContainer *container = MATECOMPONENT_ITEM_CONTAINER (object);

	/* Destroy all the ClientSites. */
	g_hash_table_foreach_remove (container->priv->objects,
				     remove_object, NULL);

	g_hash_table_destroy (container->priv->objects);
	g_free (container->priv);
	
	matecomponent_item_container_parent_class->finalize (object);
}

static void
get_object_names (gpointer key, gpointer value, gpointer user_data)
{
	GSList **l = user_data;

	*l = g_slist_prepend (*l, CORBA_string_dup (key));
}

/*
 * Returns a list of the objects in this container
 */
static MateComponent_ItemContainer_ObjectNames *
impl_MateComponent_ItemContainer_enumObjects (PortableServer_Servant servant,
				       CORBA_Environment     *ev)
{
	MateComponent_ItemContainer_ObjectNames *list;
	MateComponentItemContainer              *container;
	GSList                           *objects, *l;
	int                               i;

	container = MATECOMPONENT_ITEM_CONTAINER (
		matecomponent_object_from_servant (servant));
	g_return_val_if_fail (container != NULL, NULL);

	list = MateComponent_ItemContainer_ObjectNames__alloc ();
	if (!list)
		return NULL;

	objects = NULL;
	g_hash_table_foreach (container->priv->objects,
			      get_object_names, &objects);

	list->_length = list->_maximum = g_slist_length (objects);
	
	list->_buffer = CORBA_sequence_CORBA_string_allocbuf (list->_length);
	if (!list->_buffer) {
		CORBA_free (list);
		for (l = objects; l; l = l->next)
			CORBA_free (l->data);
		g_slist_free (objects);
		return NULL;
	}
	
	/* Assemble the list of objects */
	for (i = 0, l = objects; l; l = l->next)
		list->_buffer [i++] = l->data;

	g_slist_free (objects);

	return list;
}

static MateComponent_Unknown
impl_MateComponent_ItemContainer_getObjectByName (PortableServer_Servant servant,
					   const CORBA_char      *item_name,
					   CORBA_boolean          only_if_exists,
					   CORBA_Environment     *ev)
{
	MateComponent_Unknown ret;
	
	g_signal_emit (
		G_OBJECT (matecomponent_object_from_servant (servant)),
		signals [GET_OBJECT], 0, item_name, only_if_exists, ev, &ret);

	return ret;
}

/* MateComponentItemContainer class initialization routine  */
static void
matecomponent_item_container_class_init (MateComponentItemContainerClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_ItemContainer__epv *epv = &klass->epv;

	matecomponent_item_container_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = matecomponent_item_container_finalize;

	signals [GET_OBJECT] =
		g_signal_new  ("get_object",
			       G_TYPE_FROM_CLASS (object_class),
			       G_SIGNAL_RUN_LAST,
			       G_STRUCT_OFFSET (MateComponentItemContainerClass, get_object),
			       NULL, NULL,
			       matecomponent_marshal_BOXED__STRING_BOOLEAN_BOXED,
			       MATECOMPONENT_TYPE_UNKNOWN,
			       3,
			       G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
			       G_TYPE_BOOLEAN,
			       MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION);

	epv->enumObjects     = impl_MateComponent_ItemContainer_enumObjects;
	epv->getObjectByName = impl_MateComponent_ItemContainer_getObjectByName;
}

/*
 * MateComponentItemContainer instance initialization routine
 */
static void
matecomponent_item_container_init (MateComponentItemContainer *container)
{
	container->priv = g_new0 (MateComponentItemContainerPrivate, 1);
	container->priv->objects = g_hash_table_new (
		g_str_hash, g_str_equal);
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentItemContainer, 
		       MateComponent_ItemContainer,
		       PARENT_TYPE,
		       matecomponent_item_container)

/**
 * matecomponent_item_container_new:
 *
 * Creates a new MateComponentItemContainer object.  These are used to hold
 * client sites.
 *
 * Returns: The newly created MateComponentItemContainer object
 */
MateComponentItemContainer *
matecomponent_item_container_new (void)
{
	return g_object_new (matecomponent_item_container_get_type (), NULL);
}

/**
 * matecomponent_item_container_add:
 * @container: The object to operate on.
 * @name: The name of the object
 * @object: The object to add to the container
 *
 * Adds the @object to the list of objects managed by this
 * container
 */
void
matecomponent_item_container_add (MateComponentItemContainer *container,
			   const char          *name,
			   MateComponentObject        *object)
{
	g_return_if_fail (name != NULL);
	g_return_if_fail (MATECOMPONENT_IS_OBJECT (object));
	g_return_if_fail (MATECOMPONENT_IS_ITEM_CONTAINER (container));

	if (g_hash_table_lookup (container->priv->objects, name)) {
		g_warning ("Object of name '%s' already exists", name);
	} else {
		matecomponent_object_ref (object);
		g_hash_table_insert (container->priv->objects,
				     g_strdup (name),
				     object);
	}
}

/**
 * matecomponent_item_container_remove_by_name:
 * @container: The object to operate on.
 * @name: The name of the object to remove from the container
 *
 * Removes the named object from the @container
 */
void
matecomponent_item_container_remove_by_name (MateComponentItemContainer *container,
				      const char          *name)
{
	gpointer key, value;

	g_return_if_fail (name != NULL);
	g_return_if_fail (MATECOMPONENT_IS_ITEM_CONTAINER (container));

	if (!g_hash_table_lookup_extended (container->priv->objects, name,
					   &key, &value))
		g_warning ("Removing '%s' but not in container", name);
	else {
		g_free (key);
		matecomponent_object_unref (value);
		g_hash_table_remove (container->priv->objects, name);
	}
}

