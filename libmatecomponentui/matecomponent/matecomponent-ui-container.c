/*
 * matecomponent-ui-container.c: The server side CORBA impl. for MateComponentWindow.
 *
 * Author:
 *	Michael Meeks (michael@ximian.com)
 *
 * Copyright 2000,2001 Ximian, Inc.
 */

#include "config.h"

#include <libmatecomponent.h>
#include <matecomponent/matecomponent-ui-xml.h>
#include <matecomponent/matecomponent-ui-util.h>
#include <matecomponent/matecomponent-ui-container.h>

#include <gtk/gtk.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static MateComponentObjectClass *matecomponent_ui_container_parent_class;

struct _MateComponentUIContainerPrivate {
	MateComponentUIEngine *engine;
	int             flags;
};

static MateComponentUIEngine *
get_engine (PortableServer_Servant servant)
{
	MateComponentUIContainer *container;

	container = MATECOMPONENT_UI_CONTAINER (matecomponent_object_from_servant (servant));
	g_return_val_if_fail (container != NULL, NULL);

	if (!container->priv->engine)
		g_warning ("Trying to invoke CORBA method "
			   "on unbound UIContainer");

	return container->priv->engine;
}

static void
impl_MateComponent_UIContainer_registerComponent (PortableServer_Servant   servant,
					   const CORBA_char        *component_name,
					   const MateComponent_Unknown     object,
					   CORBA_Environment       *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);

	matecomponent_ui_engine_register_component (engine, component_name, object);
}

static void
impl_MateComponent_UIContainer_deregisterComponent (PortableServer_Servant servant,
					     const CORBA_char      *component_name,
					     CORBA_Environment     *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);

	if (!engine)
		return;

	matecomponent_ui_engine_deregister_component (engine, component_name);
}

static void
impl_MateComponent_UIContainer_setNode (PortableServer_Servant   servant,
				 const CORBA_char        *path,
				 const CORBA_char        *xml,
				 const CORBA_char        *component_name,
				 CORBA_Environment       *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);
	MateComponentUIError   err;
	MateComponentUINode   *node;

/*	fprintf (stderr, "Merging :\n%s\n", xml);*/

	if (!xml)
		err = MATECOMPONENT_UI_ERROR_BAD_PARAM;
	else {
		if (xml [0] == '\0')
			err = MATECOMPONENT_UI_ERROR_OK;
		else {
			if (!(node = matecomponent_ui_node_from_string (xml)))
				err = MATECOMPONENT_UI_ERROR_INVALID_XML;

			else
				err = matecomponent_ui_engine_xml_merge_tree (
					engine, path, node, component_name);
		}
	}

	if (err) {
		if (err == MATECOMPONENT_UI_ERROR_INVALID_PATH)
			CORBA_exception_set (
				ev, CORBA_USER_EXCEPTION,
				ex_MateComponent_UIContainer_InvalidPath, NULL);
		else
			CORBA_exception_set (
				ev, CORBA_USER_EXCEPTION,
				ex_MateComponent_UIContainer_MalformedXML, NULL);
	}
}

static CORBA_char *
impl_MateComponent_UIContainer_getNode (PortableServer_Servant servant,
				 const CORBA_char      *path,
				 const CORBA_boolean    nodeOnly,
				 CORBA_Environment     *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);
	CORBA_char *xml;

	xml = matecomponent_ui_engine_xml_get (engine, path, nodeOnly);
	if (!xml) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_UIContainer_InvalidPath, NULL);
		return NULL;
	}

	return xml;
}

static void
impl_MateComponent_UIContainer_setAttr (PortableServer_Servant   servant,
				 const CORBA_char        *path,
				 const CORBA_char        *attr,
				 const CORBA_char        *value,
				 const CORBA_char        *component,
				 CORBA_Environment       *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);

	/* Ignore the error for speed */
	matecomponent_ui_engine_xml_set_prop (
		engine, path, attr, value, component);
}

static CORBA_char *
impl_MateComponent_UIContainer_getAttr (PortableServer_Servant servant,
				 const CORBA_char      *path,
				 const CORBA_char      *attr,
				 CORBA_Environment     *ev)
{
	CORBA_char     *xml;
	MateComponentUIEngine *engine = get_engine (servant);
	gboolean        invalid_path = FALSE;

	xml = matecomponent_ui_engine_xml_get_prop (
		engine, path, attr, &invalid_path);
	
	if (!xml) {
		if (invalid_path)
			CORBA_exception_set (
				ev, CORBA_USER_EXCEPTION,
				ex_MateComponent_UIContainer_InvalidPath, NULL);
		else
			CORBA_exception_set (
				ev, CORBA_USER_EXCEPTION,
				ex_MateComponent_UIContainer_NonExistentAttr, NULL);

		return NULL;
	}

	return xml;
}

static void
impl_MateComponent_UIContainer_removeNode (PortableServer_Servant servant,
				    const CORBA_char      *path,
				    const CORBA_char      *component_name,
				    CORBA_Environment     *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);
	MateComponentUIError err;

	if (!engine)
		return;

/*	g_warning ("Node remove '%s' for '%s'", path, component_name); */

	err = matecomponent_ui_engine_xml_rm (engine, path, component_name);

	if (err)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_UIContainer_InvalidPath, NULL);
}

static CORBA_boolean
impl_MateComponent_UIContainer_exists (PortableServer_Servant servant,
				const CORBA_char      *path,
				CORBA_Environment     *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);

	return matecomponent_ui_engine_xml_node_exists (engine, path);
}

static void
impl_MateComponent_UIContainer_execVerb (PortableServer_Servant servant,
				  const CORBA_char      *cname,
				  CORBA_Environment     *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);

	matecomponent_ui_engine_exec_verb (engine, cname, ev);
}

static void
impl_MateComponent_UIContainer_uiEvent (PortableServer_Servant             servant,
				 const CORBA_char                  *id,
				 const MateComponent_UIComponent_EventType type,
				 const CORBA_char                  *state,
				 CORBA_Environment                 *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);

	matecomponent_ui_engine_ui_event (engine, id, type, state, ev);
}

static void
impl_MateComponent_UIContainer_setObject (PortableServer_Servant servant,
				   const CORBA_char      *path,
				   const MateComponent_Unknown   control,
				   CORBA_Environment     *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);
	MateComponentUIError err;

	err = matecomponent_ui_engine_object_set (engine, path, control, ev);

	if (err)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_UIContainer_InvalidPath, NULL);
}

static MateComponent_Unknown
impl_MateComponent_UIContainer_getObject (PortableServer_Servant servant,
				   const CORBA_char      *path,
				   CORBA_Environment     *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);
	MateComponentUIError err;
	MateComponent_Unknown object;

	err = matecomponent_ui_engine_object_get (engine, path, &object, ev);

	if (err)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_UIContainer_InvalidPath, NULL);

	return object;
}

static void
impl_MateComponent_UIContainer_freeze (PortableServer_Servant   servant,
				CORBA_Environment       *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);

	matecomponent_ui_engine_freeze (engine);
}

static void
impl_MateComponent_UIContainer_thaw (PortableServer_Servant   servant,
			      CORBA_Environment       *ev)
{
	MateComponentUIEngine *engine = get_engine (servant);

	matecomponent_ui_engine_thaw (engine);
}

static void
matecomponent_ui_container_dispose (GObject *object)
{
	MateComponentUIContainer *container = (MateComponentUIContainer *) object;

	matecomponent_ui_container_set_engine (container, NULL);

	G_OBJECT_CLASS (matecomponent_ui_container_parent_class)->dispose (object);
}

static void
matecomponent_ui_container_finalize (GObject *object)
{
	MateComponentUIContainer *container = (MateComponentUIContainer *) object;

	g_free (container->priv);
	container->priv = NULL;

	G_OBJECT_CLASS (matecomponent_ui_container_parent_class)->finalize (object);
}

static void
matecomponent_ui_container_init (GObject *object)
{
	MateComponentUIContainer *container = (MateComponentUIContainer *) object;

	container->priv = g_new0 (MateComponentUIContainerPrivate, 1);
}

static void
matecomponent_ui_container_class_init (MateComponentUIContainerClass *klass)
{
	GObjectClass                *g_class = (GObjectClass *) klass;
	POA_MateComponent_UIContainer__epv *epv = &klass->epv;

	matecomponent_ui_container_parent_class = g_type_class_peek_parent (klass);

	g_class->dispose  = matecomponent_ui_container_dispose;
	g_class->finalize = matecomponent_ui_container_finalize;

	epv->registerComponent   = impl_MateComponent_UIContainer_registerComponent;
	epv->deregisterComponent = impl_MateComponent_UIContainer_deregisterComponent;

	epv->setAttr    = impl_MateComponent_UIContainer_setAttr;
	epv->getAttr    = impl_MateComponent_UIContainer_getAttr;

	epv->setNode    = impl_MateComponent_UIContainer_setNode;
	epv->getNode    = impl_MateComponent_UIContainer_getNode;
	epv->removeNode = impl_MateComponent_UIContainer_removeNode;
	epv->exists     = impl_MateComponent_UIContainer_exists;

	epv->execVerb   = impl_MateComponent_UIContainer_execVerb;
	epv->uiEvent    = impl_MateComponent_UIContainer_uiEvent;

	epv->setObject  = impl_MateComponent_UIContainer_setObject;
	epv->getObject  = impl_MateComponent_UIContainer_getObject;

	epv->freeze     = impl_MateComponent_UIContainer_freeze;
	epv->thaw       = impl_MateComponent_UIContainer_thaw;
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentUIContainer, 
		       MateComponent_UIContainer,
		       PARENT_TYPE,
		       matecomponent_ui_container)

/**
 * matecomponent_ui_container_new:
 * @void: 
 * 
 * Return value: a newly created MateComponentUIContainer
 **/
MateComponentUIContainer *
matecomponent_ui_container_new (void)
{
	return g_object_new (MATECOMPONENT_TYPE_UI_CONTAINER, NULL);
}

/**
 * matecomponent_ui_container_set_engine:
 * @container: the container
 * @engine: the engine
 * 
 * Associates the MateComponentUIContainer with a #MateComponentUIEngine
 * that it will use to handle all the UI merging requests.
 **/
void
matecomponent_ui_container_set_engine (MateComponentUIContainer *container,
				MateComponentUIEngine    *engine)
{
	MateComponentUIEngine *old_engine;

	g_return_if_fail (MATECOMPONENT_IS_UI_CONTAINER (container));

	if (container->priv->engine == engine)
		return;

	old_engine = container->priv->engine;
	container->priv->engine = engine;

	if (old_engine)
		matecomponent_ui_engine_set_ui_container (old_engine, NULL);

	if (engine)
		matecomponent_ui_engine_set_ui_container (engine, container);
}

/**
 * matecomponent_ui_container_get_engine:
 * @container: the UI container
 * 
 * Get the associated #MateComponentUIEngine
 * 
 * Return value: the engine
 **/
MateComponentUIEngine *
matecomponent_ui_container_get_engine (MateComponentUIContainer *container)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_CONTAINER (container), NULL);

	return container->priv->engine;
}
