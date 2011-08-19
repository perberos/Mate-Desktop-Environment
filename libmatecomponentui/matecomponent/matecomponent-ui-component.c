/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * mate-component-ui.c: Client UI signal multiplexer and verb repository.
 *
 * Author:
 *     Michael Meeks (michael@ximian.com)
 *
 * Copyright 1999, 2001 Ximian, Inc.
 */
#include <config.h>
#include <string.h>
#include <unistd.h>
#include <matecomponent/matecomponent-types.h>
#include <matecomponent/matecomponent-ui-xml.h>
#include <matecomponent/matecomponent-ui-util.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-component.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-ui-marshal.h>
#include <matecomponent/matecomponent-control.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static GObjectClass *matecomponent_ui_component_parent_class;

enum {
	EXEC_VERB,
	UI_EVENT,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

#define GET_CLASS(c) (MATECOMPONENT_UI_COMPONENT_CLASS (G_OBJECT_GET_CLASS (c)))

typedef struct {
	char     *id;
	GClosure *closure;
} UIListener;

typedef struct {
	char     *cname;
	GClosure *closure;
} UIVerb;

struct _MateComponentUIComponentPrivate {
	GHashTable        *verbs;
	GHashTable        *listeners;
	char              *name;
	MateComponent_UIContainer container;
	int                frozenness;
};

static inline MateComponentUIComponent *
matecomponent_ui_from_servant (PortableServer_Servant servant)
{
	return MATECOMPONENT_UI_COMPONENT (matecomponent_object_from_servant (servant));
}

static gboolean
verb_destroy (gpointer dummy, UIVerb *verb, gpointer dummy2)
{
	if (verb) {
		if (verb->closure)
			g_closure_unref (verb->closure);
		verb->closure = NULL;
		g_free (verb->cname);
		g_free (verb);
	}
	return TRUE;
}

static gboolean
listener_destroy (gpointer dummy, UIListener *l, gpointer dummy2)
{
	if (l) {
		if (l->closure)
			g_closure_unref (l->closure);
		l->closure = NULL;
		g_free (l->id);
		g_free (l);
	}
	return TRUE;
}

static void
ui_event (MateComponentUIComponent           *component,
	  const char                  *id,
	  MateComponent_UIComponent_EventType type,
	  const char                  *state)
{
	UIListener *list;

	list = g_hash_table_lookup (component->priv->listeners, id);
	if (list && list->closure)
		matecomponent_closure_invoke (
			list->closure, G_TYPE_NONE,
			MATECOMPONENT_TYPE_UI_COMPONENT, component,
			G_TYPE_STRING, id,
			G_TYPE_INT, type,
			G_TYPE_STRING, state,
			G_TYPE_INVALID);
}

static void
impl_MateComponent_UIComponent_setContainer (PortableServer_Servant   servant,
				      const MateComponent_UIContainer container,
				      CORBA_Environment       *ev)
{
	MateComponentUIComponent *component = matecomponent_ui_from_servant (servant);

	matecomponent_ui_component_set_container (component, container, ev);
}

static void
impl_MateComponent_UIComponent_unsetContainer (PortableServer_Servant servant,
					CORBA_Environment     *ev)
{
	MateComponentUIComponent *component = matecomponent_ui_from_servant (servant);

	matecomponent_ui_component_unset_container (component, ev);
}

static CORBA_string
impl_MateComponent_UIComponent__get_name (PortableServer_Servant servant,
				   CORBA_Environment     *ev)
{
	return CORBA_string_dup ("");
}

static CORBA_char *
impl_MateComponent_UIComponent_describeVerbs (PortableServer_Servant servant,
				       CORBA_Environment     *ev)
{
	g_warning ("FIXME: Describe verbs unimplemented");
	return CORBA_string_dup ("<NoUIVerbDescriptionCodeYet/>");
}

static void
impl_MateComponent_UIComponent_execVerb (PortableServer_Servant servant,
				  const CORBA_char      *cname,
				  CORBA_Environment     *ev)
{
	MateComponentUIComponent *component;
	UIVerb *verb;

	component = matecomponent_ui_from_servant (servant);

	matecomponent_object_ref (MATECOMPONENT_OBJECT (component));
	
/*	g_warning ("TESTME: Exec verb '%s'", cname);*/

	verb = g_hash_table_lookup (component->priv->verbs, cname);
	if (verb && verb->closure)
		/* We need a funny arg order here - so for
		   our C closure we do odd things ! */
		matecomponent_closure_invoke (
			verb->closure, G_TYPE_NONE,
			MATECOMPONENT_TYPE_UI_COMPONENT, component,
			G_TYPE_STRING, cname,
			G_TYPE_INVALID);
	else
		g_warning ("FIXME: verb '%s' not found, emit exception", cname);

	g_signal_emit (component, signals [EXEC_VERB], 0, cname);

	matecomponent_object_unref (MATECOMPONENT_OBJECT (component));
}

static void
impl_MateComponent_UIComponent_uiEvent (PortableServer_Servant             servant,
				 const CORBA_char                  *id,
				 const MateComponent_UIComponent_EventType type,
				 const CORBA_char                  *state,
				 CORBA_Environment                 *ev)
{
	MateComponentUIComponent *component;

	component = matecomponent_ui_from_servant (servant);

/*	g_warning ("TESTME: Event '%s' '%d' '%s'\n", path, type, state);*/

	matecomponent_object_ref (MATECOMPONENT_OBJECT (component));

	g_signal_emit (component, signals [UI_EVENT], 0, id, type, state);

	matecomponent_object_unref (MATECOMPONENT_OBJECT (component));
}

static void
marshal_VOID__USER_DATA_STRING (GClosure     *closure,
				GValue       *return_value,
				guint         n_param_values,
				const GValue *param_values,
				gpointer      invocation_hint,
				gpointer      marshal_data)
{
  typedef void (*marshal_func_VOID__USER_DATA_STRING_t) (gpointer     data1,
							 gpointer     data2,
							 gpointer     arg_1);
  register marshal_func_VOID__USER_DATA_STRING_t callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (marshal_func_VOID__USER_DATA_STRING_t) (
	  marshal_data ? marshal_data : cc->callback);

  callback (data1, data2, (char*) g_value_get_string (param_values + 1));
}


/**
 * matecomponent_ui_component_add_verb_full:
 * @component: the component to add it to
 * @cname: the programmatic name of the verb
 * @fn: the callback function for invoking it
 * @user_data: the associated user data for the callback
 * @destroy_fn: a destroy function for the callback data
 * 
 * Add a verb to the UI component, that can be invoked by
 * the container.
 **/
void
matecomponent_ui_component_add_verb_full (MateComponentUIComponent  *component,
				   const char         *cname,
				   GClosure           *closure)
{
	UIVerb *verb;
	MateComponentUIComponentPrivate *priv;

	g_return_if_fail (cname != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	priv = component->priv;

	if ((verb = g_hash_table_lookup (priv->verbs, cname))) {
		g_hash_table_remove (priv->verbs, cname);
		verb_destroy (NULL, verb, NULL);
	}

	verb = g_new (UIVerb, 1);
	verb->cname      = g_strdup (cname);
	verb->closure    = matecomponent_closure_store
		(closure, marshal_VOID__USER_DATA_STRING);
	
	/*	verb->cb (component, verb->user_data, cname); */

	g_hash_table_insert (priv->verbs, verb->cname, verb);
}

/**
 * matecomponent_ui_component_add_verb:
 * @component: the component to add it to
 * @cname: the programmatic name of the verb
 * @fn: the callback function for invoking it
 * @user_data: the associated user data for the callback
 *
 * Add a verb to the UI component, that can be invoked by
 * the container.
 **/
void
matecomponent_ui_component_add_verb (MateComponentUIComponent  *component,
			      const char         *cname,
			      MateComponentUIVerbFn      fn,
			      gpointer            user_data)
{
	matecomponent_ui_component_add_verb_full (
		component, cname, g_cclosure_new (
			G_CALLBACK (fn), user_data, NULL));
}

typedef struct {
	gboolean    by_name;
	const char *name;
	gboolean    by_closure;
	GClosure   *closure;
} RemoveInfo;

static gboolean
remove_verb (gpointer	key,
	     gpointer	value,
	     gpointer	user_data)
{
	RemoveInfo *info = user_data;
	UIVerb     *verb = value;

	if (info->by_name && info->name &&
	    !strcmp (verb->cname, info->name))
		return verb_destroy (NULL, verb, NULL);

	else if (info->by_closure &&
		 info->closure == verb->closure)
		return verb_destroy (NULL, verb, NULL);

	return FALSE;
}

/**
 * matecomponent_ui_component_remove_verb:
 * @component: the component to add it to
 * @cname: the programmatic name of the verb
 * 
 * Remove a verb by it's unique name
 **/
void
matecomponent_ui_component_remove_verb (MateComponentUIComponent  *component,
				 const char         *cname)
{
	RemoveInfo info;

	memset (&info, 0, sizeof (info));

	info.by_name = TRUE;
	info.name = cname;

	g_hash_table_foreach_remove (component->priv->verbs, remove_verb, &info);
}

/**
 * matecomponent_ui_component_remove_verb_by_closure:
 * @component: the component to add it to
 * @fn: the function pointer
 * 
 * remove any verb handled by @fn.
 **/
void
matecomponent_ui_component_remove_verb_by_closure (MateComponentUIComponent  *component,
					    GClosure           *closure)
{
	RemoveInfo info;

	memset (&info, 0, sizeof (info));

	info.by_closure = TRUE;
	info.closure = closure;

	g_hash_table_foreach_remove (component->priv->verbs, remove_verb, &info);
}

/**
 * matecomponent_ui_component_add_listener_full:
 * @component: the component to add it to
 * @id: the programmatic name of the id
 * @fn: the callback function for invoking it
 * @user_data: the associated user data for the callback
 * @destroy_fn: a destroy function for the callback data
 * 
 * Add a listener for stateful events.
 **/
void
matecomponent_ui_component_add_listener_full (MateComponentUIComponent  *component,
				       const char         *id,
				       GClosure           *closure)
{
	UIListener *list;
	MateComponentUIComponentPrivate *priv;

	g_return_if_fail (closure != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	priv = component->priv;

	if ((list = g_hash_table_lookup (priv->listeners, id))) {
		g_hash_table_remove (priv->listeners, id);
		listener_destroy (NULL, list, NULL);
	}

	list = g_new (UIListener, 1);
	list->id = g_strdup (id);
	list->closure = matecomponent_closure_store
		(closure, matecomponent_ui_marshal_VOID__STRING_INT_STRING);

	g_hash_table_insert (priv->listeners, list->id, list);	
}

/**
 * matecomponent_ui_component_add_listener:
 * @component: the component to add it to
 * @id: the programmatic name of the id
 * @fn: the callback function for invoking it
 * @user_data: the associated user data for the callback
 * 
 * Add a listener for stateful events.
 **/
void
matecomponent_ui_component_add_listener (MateComponentUIComponent  *component,
				  const char         *id,
				  MateComponentUIListenerFn  fn,
				  gpointer            user_data)
{
	matecomponent_ui_component_add_listener_full (
		component, id, g_cclosure_new (G_CALLBACK (fn), user_data, NULL));
}

static gboolean
remove_listener (gpointer	key,
		 gpointer	value,
		 gpointer	user_data)
{
	RemoveInfo *info = user_data;
	UIListener     *listener = value;

	if (info->by_name && info->name &&
	    !strcmp (listener->id, info->name))
		return listener_destroy (NULL, listener, NULL);

	else if (info->by_closure &&
		 info->closure == listener->closure)
		return listener_destroy (NULL, listener, NULL);

	return FALSE;
}

/**
 * matecomponent_ui_component_remove_listener:
 * @component: the component to add it to
 * @cname: the programmatic name of the id
 * 
 * Remove any listener by its unique id
 **/
void
matecomponent_ui_component_remove_listener (MateComponentUIComponent  *component,
				     const char         *cname)
{
	RemoveInfo info;

	memset (&info, 0, sizeof (info));

	info.by_name = TRUE;
	info.name = cname;

	g_hash_table_foreach_remove (component->priv->listeners, remove_listener, &info);
}

/**
 * matecomponent_ui_component_remove_by_closure:
 * @component: the component to add it to
 * @fn: the function pointer
 * 
 * Remove any listener with associated function @fn
 **/
void
matecomponent_ui_component_remove_listener_by_closure (MateComponentUIComponent *component,
						GClosure          *closure)
{
	RemoveInfo info;

	memset (&info, 0, sizeof (info));

	info.by_closure = TRUE;
	info.closure = closure;

	g_hash_table_foreach_remove (component->priv->listeners, remove_listener, &info);
}

static void
matecomponent_ui_component_finalize (GObject *object)
{
	MateComponentUIComponent *comp = (MateComponentUIComponent *) object;
	MateComponentUIComponentPrivate *priv = comp->priv;

	if (priv) {
		g_hash_table_foreach_remove (
			priv->verbs, (GHRFunc) verb_destroy, NULL);
		g_hash_table_destroy (priv->verbs);
		priv->verbs = NULL;

		g_hash_table_foreach_remove (
			priv->listeners,
			(GHRFunc) listener_destroy, NULL);
		g_hash_table_destroy (priv->listeners);
		priv->listeners = NULL;

		g_free (priv->name);

		g_free (priv);
	}
	comp->priv = NULL;

	matecomponent_ui_component_parent_class->finalize (object);
}

/**
 * matecomponent_ui_component_construct:
 * @ui_component: the UI component itself
 * @name: the name of the UI component
 * 
 * Construct the UI component with name @name
 * 
 * Return value: a constructed UI component or NULL on error
 **/
MateComponentUIComponent *
matecomponent_ui_component_construct (MateComponentUIComponent *ui_component,
			       const char        *name)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_COMPONENT (ui_component), NULL);

	ui_component->priv->name = g_strdup (name);

	return ui_component;
}

/**
 * matecomponent_ui_component_new:
 * @name: the name of the UI component
 * 
 * Create a new UI component with the specified name
 * 
 * Return value: a new UI component 
 **/
MateComponentUIComponent *
matecomponent_ui_component_new (const char *name)
{
	MateComponentUIComponent *component;

	component = g_object_new (MATECOMPONENT_TYPE_UI_COMPONENT, NULL);
	if (!component)
		return NULL;

	return MATECOMPONENT_UI_COMPONENT (
		matecomponent_ui_component_construct (
			component, name));
}

/**
 * matecomponent_ui_component_new_default:
 * @void: 
 * 
 * Create a UI component with a unique default name
 * constructed from various available system properties.
 * 
 * Return value: a new UI component
 **/
MateComponentUIComponent *
matecomponent_ui_component_new_default (void)
{
	char              *name;
	MateComponentUIComponent *component;
	static int idx = 0;
	static int pid = 0;

	if (!pid)
		pid = getpid ();

	name = g_strdup_printf ("%d-%d", pid, idx++);

	component = matecomponent_ui_component_new (name);
	
	g_free (name);

	return component;
}

/**
 * matecomponent_ui_component_set_name:
 * @component: the UI component
 * @name: the new name
 * 
 * Set the @name of the UI @component
 **/
void
matecomponent_ui_component_set_name (MateComponentUIComponent  *component,
			      const char         *name)
{
	g_return_if_fail (name != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));
	
	g_free (component->priv->name);
	component->priv->name = g_strdup (name);
}

/**
 * matecomponent_ui_component_get_name:
 * @component: the UI component
 * 
 * Return value: the name of the UI @component
 **/
const char *
matecomponent_ui_component_get_name (MateComponentUIComponent  *component)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component), NULL);
	
	return component->priv->name;
}

/**
 * matecomponent_ui_component_set:
 * @component: the component
 * @path: the path to set
 * @xml: the xml to set
 * @opt_ev: the (optional) CORBA exception environment
 * 
 * Set the @xml fragment into the remote #MateComponentUIContainer's tree
 * attached to @component at the specified @path
 *
 * If you see blank menu items ( or just separators ) it's
 * likely that you should be using #matecomponent_ui_component_set_translate
 * which substantialy deprecates this routine.
 **/
void
matecomponent_ui_component_set (MateComponentUIComponent  *component,
			 const char         *path,
			 const char         *xml,
			 CORBA_Environment  *opt_ev)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	GET_CLASS (component)->xml_set (component, path, xml, opt_ev);
}

static void
impl_xml_set (MateComponentUIComponent  *component,
	      const char         *path,
	      const char         *xml,
	      CORBA_Environment  *ev)
{
	CORBA_Environment *real_ev, tmp_ev;
	MateComponent_UIContainer container;
	char              *name;

	container = component->priv->container;
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	if (xml [0] == '\0')
		return;

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	name = component->priv->name ? component->priv->name : "";

/*	fprintf (stderr, "setNode ( '%s', '%s', '%s' )", path, xml, name); */
	MateComponent_UIContainer_setNode (container, path, xml,
				    name, real_ev);

	if (MATECOMPONENT_EX (real_ev) && !ev)
		g_warning ("Serious exception on node_set '$%s' of '%s' to '%s'",
			   matecomponent_exception_get_text (real_ev), xml, path);

	if (!ev)
		CORBA_exception_free (&tmp_ev);
}

/**
 * matecomponent_ui_component_set_tree:
 * @component: the component
 * @path: the path to set
 * @node: the #MateComponentUINode representation of an xml tree to set
 * @ev: the (optional) CORBA exception environment
 * 
 * Set the @xml fragment into the remote #MateComponentUIContainer's tree
 * attached to @component at the specified @path
 * 
 **/
void
matecomponent_ui_component_set_tree (MateComponentUIComponent *component,
			      const char        *path,
			      MateComponentUINode      *node,
			      CORBA_Environment *ev)
{
	char *str;

	str = matecomponent_ui_node_to_string (node, TRUE);	

/*	fprintf (stderr, "Merging '%s'\n", str); */
	
	matecomponent_ui_component_set (
		component, path, str, ev);

	g_free (str);
}

/**
 * matecomponent_ui_component_set_translate:
 * @component: the component
 * @path: the path to set
 * @xml: the non translated xml to set
 * @opt_ev: the (optional) CORBA exception environment
 * 
 * This routine parses the XML strings, and converts any:
 * _label="Hello World" type strings into the translated,
 * and encoded format expected by the remote #MateComponentUIContainer.
 **/
void
matecomponent_ui_component_set_translate (MateComponentUIComponent  *component,
				   const char         *path,
				   const char         *xml,
				   CORBA_Environment  *opt_ev)
{
	MateComponentUINode *node;

	if (!xml)
		return;

	node = matecomponent_ui_node_from_string (xml);
	g_return_if_fail (node != NULL);

	matecomponent_ui_util_translate_ui (node);

	matecomponent_ui_component_set_tree (component, path, node, opt_ev);

	matecomponent_ui_node_free (node);
}

/**
 * matecomponent_ui_component_get:
 * @component: the component
 * @path: the path to get
 * @recurse: whether to get child nodes of @path
 * @opt_ev: the (optional) CORBA exception environment
 * 
 * This routine fetches a chunk of the XML tree in the
 * #MateComponentUIContainer associated with @component pointed
 * to by @path. If @recurse then the child nodes of @path
 * are returned too, otherwise they are not.
 *
 * Return value: an XML string (CORBA allocated)
 **/
CORBA_char *
matecomponent_ui_component_get (MateComponentUIComponent *component,
			 const char        *path,
			 gboolean           recurse,
			 CORBA_Environment *opt_ev)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component), NULL);

	return GET_CLASS (component)->xml_get (component, path, recurse, opt_ev);
}

static CORBA_char *
impl_xml_get (MateComponentUIComponent *component,
	      const char        *path,
	      gboolean           recurse,
	      CORBA_Environment *ev)
{
	CORBA_Environment *real_ev, tmp_ev;
	CORBA_char *xml;
	MateComponent_UIContainer container;

	container = component->priv->container;
	g_return_val_if_fail (container != CORBA_OBJECT_NIL, NULL);

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	xml = MateComponent_UIContainer_getNode (container, path, !recurse, real_ev);

	if (MATECOMPONENT_EX (real_ev)) {
		if (!ev)
			g_warning ("Serious exception getting node '%s' '$%s'",
				   path, matecomponent_exception_get_text (real_ev));
		if (!ev)
			CORBA_exception_free (&tmp_ev);
		
		return NULL;
	}

	if (!ev)
		CORBA_exception_free (&tmp_ev);
	
	return xml;
}

/**
 * matecomponent_ui_component_get_tree:
 * @component: the component
 * @path: the path to get
 * @recurse: whether to get child nodes of @path
 * @opt_ev: the (optional) CORBA exception environment
 * 
 * This routine fetches a chunk of the XML tree in the
 * #MateComponentUIContainer associated with @component pointed
 * to by @path. If @recurse then the child nodes of @path
 * are returned too, otherwise they are not.
 *
 * Return value: an #MateComponentUINode XML representation
 **/
MateComponentUINode *
matecomponent_ui_component_get_tree (MateComponentUIComponent  *component,
			      const char         *path,
			      gboolean            recurse,
			      CORBA_Environment  *opt_ev)
{	
	char *xml;
	MateComponentUINode *node;

	xml = matecomponent_ui_component_get (component, path, recurse, opt_ev);

	if (!xml)
		return NULL;

	node = matecomponent_ui_node_from_string (xml);

	CORBA_free (xml);

	if (!node)
		return NULL;

	return node;
}

/**
 * matecomponent_ui_component_rm:
 * @component: the component
 * @path: the path to set
 * @ev: the (optional) CORBA exception environment
 *
 * This routine removes a chunk of the XML tree in the
 * #MateComponentUIContainer associated with @component pointed
 * to by @path.
 **/
void
matecomponent_ui_component_rm (MateComponentUIComponent  *component,
			const char         *path,
			CORBA_Environment  *ev)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	GET_CLASS (component)->xml_rm (component, path, ev);
}

static void
impl_xml_rm (MateComponentUIComponent  *component,
	     const char         *path,
	     CORBA_Environment  *ev)
{
	MateComponentUIComponentPrivate *priv;
	CORBA_Environment *real_ev, tmp_ev;
	MateComponent_UIContainer container;

	container = component->priv->container;
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	priv = component->priv;

	MateComponent_UIContainer_removeNode (
		container, path, priv->name, real_ev);

	if (!ev && MATECOMPONENT_EX (real_ev))
		g_warning ("Serious exception removing path  '%s' '%s'",
			   path, matecomponent_exception_get_text (real_ev));

	if (!ev)
		CORBA_exception_free (&tmp_ev);
}


/**
 * matecomponent_ui_component_object_set:
 * @component: the component
 * @path: the path to set
 * @control: a CORBA object reference
 * @opt_ev: the (optional) CORBA exception environment
 * 
 * This registers the @control CORBA object into the
 * #MateComponentUIContainer associated with this @component at
 * the specified @path. This is most often used to associate
 * controls with a certain path.
 **/
void
matecomponent_ui_component_object_set (MateComponentUIComponent  *component,
				const char         *path,
				MateComponent_Unknown      control,
				CORBA_Environment  *opt_ev)
{
	CORBA_Environment *real_ev, tmp_ev;
	MateComponent_UIContainer container;

	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));
	container = component->priv->container;
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	if (opt_ev)
		real_ev = opt_ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	MateComponent_UIContainer_setObject (container, path, control, real_ev);

	if (!opt_ev && MATECOMPONENT_EX (real_ev))
		g_warning ("Serious exception setting object '%s' '%s'",
			   path, matecomponent_exception_get_text (real_ev));

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

void
matecomponent_ui_component_widget_set (MateComponentUIComponent  *component,
				const char         *path,
				GtkWidget          *widget,
				CORBA_Environment  *opt_ev)
{
	gpointer in_proc_servant;
	MateComponentObject *in_proc_container;
	CORBA_Environment *real_ev, tmp_ev;
	MateComponent_UIContainer container;

	g_return_if_fail (widget != CORBA_OBJECT_NIL);
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));
	container = component->priv->container;
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	if (opt_ev)
		real_ev = opt_ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	if ((in_proc_servant = MateCORBA_small_get_servant (container)) &&
	    (in_proc_container = matecomponent_object (in_proc_servant)) &&
	    MATECOMPONENT_IS_UI_CONTAINER (in_proc_container)) {
		MateComponentUIEngine *engine;

		engine = matecomponent_ui_container_get_engine (
			MATECOMPONENT_UI_CONTAINER (in_proc_container));
		g_return_if_fail (engine != NULL);
		matecomponent_ui_engine_widget_set (engine, path, widget);
	} else {
		MateComponentControl *control = matecomponent_control_new (widget);
		MateComponent_UIContainer_setObject (
			container, path, MATECOMPONENT_OBJREF (control), real_ev);
		matecomponent_object_unref (control);
	}

	if (!opt_ev && MATECOMPONENT_EX (real_ev))
		g_warning ("Serious exception setting object '%s' '%s'",
			   path, matecomponent_exception_get_text (real_ev));

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

/**
 * matecomponent_ui_component_object_get:
 * @component: the component
 * @path: the path to set
 * @ev: the (optional) CORBA exception environment
 * 
 * This returns the @control CORBA object registered with the
 * #MateComponentUIContainer associated with this @component at
 * the specified @path.
 *
 * Returns: the associated remote CORBA object.
 **/
MateComponent_Unknown
matecomponent_ui_component_object_get (MateComponentUIComponent  *component,
				const char         *path,
				CORBA_Environment  *ev)
{
	CORBA_Environment *real_ev, tmp_ev;
	MateComponent_Unknown     ret;
	MateComponent_UIContainer container;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component),
			      CORBA_OBJECT_NIL);
	container = component->priv->container;
	g_return_val_if_fail (container != CORBA_OBJECT_NIL,
			      CORBA_OBJECT_NIL);

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	ret = MateComponent_UIContainer_getObject (container, path, real_ev);

	if (!ev && MATECOMPONENT_EX (real_ev))
		g_warning ("Serious exception getting object '%s' '%s'",
			   path, matecomponent_exception_get_text (real_ev));

	if (!ev)
		CORBA_exception_free (&tmp_ev);

	return ret;
}

/**
 * matecomponent_ui_component_add_verb_list_with_data:
 * @component: the component
 * @list: the list of verbs
 * @user_data: the user data passed to the verb callbacks
 * 
 * This is a helper function to save registering verbs individualy
 * it allows registration of a great batch of verbs at one time
 * in a list of #MateComponentUIVerb terminated by #MATECOMPONENT_UI_VERB_END
 **/
void
matecomponent_ui_component_add_verb_list_with_data (MateComponentUIComponent  *component,
					     const MateComponentUIVerb *list,
					     gpointer            user_data)
{
	const MateComponentUIVerb *l;

	g_return_if_fail (list != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	matecomponent_object_ref (MATECOMPONENT_OBJECT (component));

	for (l = list; l && l->cname; l++) {
		matecomponent_ui_component_add_verb (
			component, l->cname, l->cb,
			user_data ? user_data : l->user_data);
	}

	matecomponent_object_unref (MATECOMPONENT_OBJECT (component));
}

/**
 * matecomponent_ui_component_add_verb_list:
 * @component: the component
 * @list: the list of verbs.
 * 
 * Add a list of verbs with no associated user_data, you probably
 * want #matecomponent_ui_component_add_verb_list_with_data
 **/
void
matecomponent_ui_component_add_verb_list (MateComponentUIComponent  *component,
				   const MateComponentUIVerb *list)
{
	matecomponent_ui_component_add_verb_list_with_data (component, list, NULL);
}

/**
 * matecomponent_ui_component_freeze:
 * @component: the component
 * @ev: the (optional) CORBA exception environment
 * 
 * This increments the freeze count on the associated
 * #MateComponentUIContainer, (if not already frozen) this means that
 * a batch of update operations can be performed without a
 * re-render penalty per update.
 *
 * NB. if your GUI is frozen / not updating you probably have a
 * freeze / thaw reference leak/
 **/
void
matecomponent_ui_component_freeze (MateComponentUIComponent *component,
			    CORBA_Environment *ev)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	GET_CLASS (component)->freeze (component, ev);
}

static void
impl_freeze (MateComponentUIComponent *component,
	     CORBA_Environment *ev)
{
	if (component->priv->frozenness == 0) {
		CORBA_Environment *real_ev, tmp_ev;
		MateComponent_UIContainer container;

		container = component->priv->container;
		g_return_if_fail (container != CORBA_OBJECT_NIL);

		if (ev)
			real_ev = ev;
		else {
			CORBA_exception_init (&tmp_ev);
			real_ev = &tmp_ev;
		}

		MateComponent_UIContainer_freeze (container, real_ev);

		if (MATECOMPONENT_EX (real_ev) && !ev)
			g_warning ("Serious exception on UI freeze '$%s'",
				   matecomponent_exception_get_text (real_ev));

		if (!ev)
			CORBA_exception_free (&tmp_ev);
	}

	component->priv->frozenness++;
}

/**
 * matecomponent_ui_component_thaw:
 * @component: the component
 * @ev: the (optional) CORBA exception environment
 * 
 * This decrements the freeze count on the remote associated
 * #MateComponentUIContainer, (if frozen). This means that a batch
 * of update operations can be performed without a re-render
 * penalty per update.
 *
 * NB. if your GUI is frozen / not updating you probably have a
 * freeze / thaw reference leak/
 **/
void
matecomponent_ui_component_thaw (MateComponentUIComponent *component,
			  CORBA_Environment *ev)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	GET_CLASS (component)->thaw (component, ev);
}

static void
impl_thaw (MateComponentUIComponent *component,
	   CORBA_Environment *ev)
{
	component->priv->frozenness--;

	if (component->priv->frozenness == 0) {
		CORBA_Environment *real_ev, tmp_ev;
		MateComponent_UIContainer container;

		container = component->priv->container;
		g_return_if_fail (container != CORBA_OBJECT_NIL);

		if (ev)
			real_ev = ev;
		else {
			CORBA_exception_init (&tmp_ev);
			real_ev = &tmp_ev;
		}

		MateComponent_UIContainer_thaw (container, real_ev);

		if (MATECOMPONENT_EX (real_ev) && !ev)
			g_warning ("Serious exception on UI thaw '$%s'",
				   matecomponent_exception_get_text (real_ev));

		if (!ev)
			CORBA_exception_free (&tmp_ev);

	} else if (component->priv->frozenness < 0)
		g_warning ("Freeze/thaw mismatch on '%s'",
			   component->priv->name ? component->priv->name : "<Null>");
}

/**
 * matecomponent_ui_component_set_prop:
 * @component: the component
 * @path: the path to set the property on
 * @prop: the property name
 * @value: the property value
 * @opt_ev: the (optional) CORBA exception environment
 * 
 * This helper function sets an XML property ( or attribute )
 * on the XML node pointed at by @path. It does this by
 * a read / modify / write process. If you find yourself
 * doing this a lot, you need to consider batching this process.
 **/
void
matecomponent_ui_component_set_prop (MateComponentUIComponent  *component,
			      const char         *path,
			      const char         *prop,
			      const char         *value,
			      CORBA_Environment  *opt_ev)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	GET_CLASS (component)->set_prop (component, path, prop, value, opt_ev);
}

static void
impl_set_prop (MateComponentUIComponent  *component,
	       const char         *path,
	       const char         *prop,
	       const char         *value,
	       CORBA_Environment  *opt_ev)
{
	MateComponent_UIContainer container;
	CORBA_Environment *real_ev, tmp_ev;

	g_return_if_fail (path != NULL);
	g_return_if_fail (prop != NULL);
	g_return_if_fail (value != NULL);

	container = component->priv->container;
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	if (opt_ev)
		real_ev = opt_ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	MateComponent_UIContainer_setAttr (
		container, path, prop, value,
		component->priv->name, real_ev);

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

/**
 * matecomponent_ui_component_get_prop:
 * @component: the component
 * @path: the path to set the property on
 * @prop: the property name
 * @value: the property value
 * @opt_ev: the (optional) CORBA exception environment
 * 
 * This helper function fetches an XML property ( or attribute )
 * from the XML node pointed at by @path in the #MateComponentUIContainer
 * associated with @component
 * 
 * Return value: the xml property value or NULL - free with g_free.
 **/
gchar *
matecomponent_ui_component_get_prop (MateComponentUIComponent *component,
			      const char        *path,
			      const char        *prop,
			      CORBA_Environment *opt_ev)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component), NULL);

	return GET_CLASS (component)->get_prop (component, path, prop, opt_ev);
}

static gchar *
impl_get_prop (MateComponentUIComponent *component,
	       const char        *path,
	       const char        *prop,
	       CORBA_Environment *opt_ev)
{
	MateComponent_UIContainer container;
	CORBA_Environment *ev, tmp_ev;
	CORBA_char        *ret;
	gchar             *retval;

	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (prop != NULL, NULL);

	container = component->priv->container;
	g_return_val_if_fail (container != CORBA_OBJECT_NIL, NULL);

	if (opt_ev)
		ev = opt_ev;
	else {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	}

	ret = MateComponent_UIContainer_getAttr (
		container, path, prop, ev);

	if (MATECOMPONENT_EX (ev)) {
		if (!opt_ev && strcmp (MATECOMPONENT_EX_REPOID (ev),
				       ex_MateComponent_UIContainer_NonExistentAttr))
			g_warning ("Invalid path '%s' on prop '%s' get",
				   path, prop);
		ret = NULL;
	}

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);

	if (ret) {
		retval = g_strdup (ret);
		CORBA_free (ret);
	} else
		retval = NULL;

	return retval;
}

/**
 * matecomponent_ui_component_path_exists:
 * @component: the component
 * @path: the path to set the property on
 * @ev: the (optional) CORBA exception environment
 * 
 * Return value: TRUE if the path exists in the container.
 **/
gboolean
matecomponent_ui_component_path_exists (MateComponentUIComponent *component,
				 const char        *path,
				 CORBA_Environment *ev)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component), FALSE);

	return GET_CLASS (component)->exists (component, path, ev);
}

static gboolean
impl_exists (MateComponentUIComponent *component,
	     const char        *path,
	     CORBA_Environment *opt_ev)
{
	gboolean ret;
	MateComponent_UIContainer container;
	CORBA_Environment *ev, tmp_ev;

	container = component->priv->container;
	g_return_val_if_fail (container != CORBA_OBJECT_NIL, FALSE);

	if (opt_ev)
		ev = opt_ev;
	else {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	}

	ret = MateComponent_UIContainer_exists (container, path, ev);

	if (MATECOMPONENT_EX (ev)) {
		ret = FALSE;
		if (!opt_ev)
			g_warning ("Serious exception on path_exists '$%s'",
				   matecomponent_exception_get_text (ev));
	}

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);

	return ret;
}

/**
 * matecomponent_ui_component_set_status:
 * @component: the component
 * @text: the new status text
 * @ev: the (optional) CORBA exception environment
 * 
 * This sets the contents of the status bar to @text in the
 * remote #MateComponentUIContainer associated with @component.
 * This is done by setting the contents of the /status/main
 * node.
 **/
void
matecomponent_ui_component_set_status (MateComponentUIComponent *component,
				const char        *text,
				CORBA_Environment *opt_ev)
{
	if (text == NULL ||
	    text [0] == '\0') {
		/*
		 * FIXME: Remove what was there to reveal other msgs
		 * NB. if we're using the same UI component as the view
		 * was merged in with, this will result in us loosing our
		 * status bar altogether - sub-optimal.
		 */
		matecomponent_ui_component_rm (component, "/status/main", opt_ev);
	} else {
		char *str, *tmp;

		tmp = g_markup_escape_text (text, -1);
		str = g_strdup_printf ("<item name=\"main\">%s</item>", tmp);
		g_free (tmp);
		
		matecomponent_ui_component_set (component, "/status", str, opt_ev);
		
		g_free (str);
	}
}

/**
 * matecomponent_ui_component_unset_container:
 * @component: the component
 * 
 * This dis-associates the @component from its associated
 * #MateComponentUIContainer.
 **/
void
matecomponent_ui_component_unset_container (MateComponentUIComponent *component,
				     CORBA_Environment *opt_ev)
{
	MateComponent_UIContainer container;

	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	matecomponent_object_ref (MATECOMPONENT_OBJECT (component));

	container = component->priv->container;
	component->priv->container = CORBA_OBJECT_NIL;

	if (container != CORBA_OBJECT_NIL) {
		CORBA_Environment *ev, temp_ev;
		char              *name;
		
		if (!opt_ev) {
			CORBA_exception_init (&temp_ev);
			ev = &temp_ev;
		} else
			ev = opt_ev;

		name = component->priv->name ? component->priv->name : "";

		MateComponent_UIContainer_deregisterComponent (container, name, ev);
		
		if (!opt_ev && MATECOMPONENT_EX (ev)) {
			char *err;
			g_warning ("Serious exception deregistering component '%s'",
				   (err = matecomponent_exception_get_text (ev)));
			g_free (err);
		}

		CORBA_Object_release (container, ev);

		if (!opt_ev)
			CORBA_exception_free (&temp_ev);
	}

	matecomponent_object_unref (MATECOMPONENT_OBJECT (component));
}

/**
 * matecomponent_ui_component_set_container:
 * @component: the component
 * @container: a remote container object.
 * 
 * This associates this @component with a remote @container
 * object.
 **/
void
matecomponent_ui_component_set_container (MateComponentUIComponent *component,
				   MateComponent_UIContainer container,
				   CORBA_Environment *opt_ev)
{
	MateComponent_UIContainer ref_cont;

	g_return_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component));

	matecomponent_object_ref (MATECOMPONENT_OBJECT (component));

	if (container != CORBA_OBJECT_NIL) {
		MateComponent_UIComponent corba_component;
		CORBA_Environment *ev, temp_ev;
		char              *name;
		
		if (!opt_ev) {
			CORBA_exception_init (&temp_ev);
			ev = &temp_ev;
		} else
			ev = opt_ev;

		ref_cont = CORBA_Object_duplicate (container, ev);

		corba_component = MATECOMPONENT_OBJREF (component);

		name = component->priv->name ? component->priv->name : "";

		MateComponent_UIContainer_registerComponent (
			ref_cont, name, corba_component, ev);

		if (!opt_ev && MATECOMPONENT_EX (ev)) {
			char *err;
			g_warning ("Serious exception registering component '%s'",
				   (err = matecomponent_exception_get_text (ev)));
			g_free (err);
		}
		
		if (!opt_ev)
			CORBA_exception_free (&temp_ev);
	} else
		ref_cont = CORBA_OBJECT_NIL;

	matecomponent_ui_component_unset_container (component, NULL);

	component->priv->container = ref_cont;

	matecomponent_object_unref (MATECOMPONENT_OBJECT (component));
}

/**
 * matecomponent_ui_component_get_container:
 * @component: the component.
 * 
 * Return value: the associated remote container
 **/
MateComponent_UIContainer
matecomponent_ui_component_get_container (MateComponentUIComponent *component)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_COMPONENT (component),
			      CORBA_OBJECT_NIL);
	
	return component->priv->container;
}

static void
matecomponent_ui_component_class_init (MateComponentUIComponentClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	MateComponentUIComponentClass *uclass = MATECOMPONENT_UI_COMPONENT_CLASS (klass);
	POA_MateComponent_UIComponent__epv *epv = &klass->epv;
	
	matecomponent_ui_component_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = matecomponent_ui_component_finalize;

	uclass->ui_event = ui_event;

	signals [EXEC_VERB] = g_signal_new (
		"exec_verb", G_TYPE_FROM_CLASS (object_class),
		G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (MateComponentUIComponentClass, exec_verb),
		NULL, NULL,
		g_cclosure_marshal_VOID__STRING,
		G_TYPE_NONE, 1, G_TYPE_STRING);

	signals [UI_EVENT] = g_signal_new (
		"ui_event", G_TYPE_FROM_CLASS (object_class),
		G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (MateComponentUIComponentClass, ui_event),
		NULL, NULL,
		matecomponent_ui_marshal_VOID__STRING_INT_STRING,
		G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_INT,
		G_TYPE_STRING);

	uclass->freeze   = impl_freeze;
	uclass->thaw     = impl_thaw;
	uclass->xml_set  = impl_xml_set;
	uclass->xml_get  = impl_xml_get;
	uclass->xml_rm   = impl_xml_rm;
	uclass->set_prop = impl_set_prop;
	uclass->get_prop = impl_get_prop;
	uclass->exists   = impl_exists;

	epv->setContainer   = impl_MateComponent_UIComponent_setContainer;
	epv->unsetContainer = impl_MateComponent_UIComponent_unsetContainer;
	epv->_get_name      = impl_MateComponent_UIComponent__get_name;
	epv->describeVerbs  = impl_MateComponent_UIComponent_describeVerbs;
	epv->execVerb       = impl_MateComponent_UIComponent_execVerb;
	epv->uiEvent        = impl_MateComponent_UIComponent_uiEvent;
}

static void
matecomponent_ui_component_init (MateComponentUIComponent *component)
{
	MateComponentUIComponentPrivate *priv;

	priv = g_new0 (MateComponentUIComponentPrivate, 1);
	priv->verbs = g_hash_table_new (g_str_hash, g_str_equal);
	priv->listeners = g_hash_table_new (g_str_hash, g_str_equal);
	priv->container = CORBA_OBJECT_NIL;

	component->priv = priv;
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentUIComponent, 
			   MateComponent_UIComponent,
			   PARENT_TYPE,
			   matecomponent_ui_component)
