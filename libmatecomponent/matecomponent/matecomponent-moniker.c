/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-moniker: Object naming abstraction
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, Ximian, Inc.
 */
#include "config.h"
#include <string.h>

#include <glib/gi18n-lib.h>    

#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker.h>
#include <matecomponent/matecomponent-moniker-util.h>
#include <matecomponent/matecomponent-moniker-extender.h>

struct _MateComponentMonikerPrivate {
	MateComponent_Moniker parent;

	int            prefix_len;
	char          *prefix;

	char          *name;

	gboolean       sensitive;
};

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static GObjectClass *matecomponent_moniker_parent_class;

#define CLASS(o) MATECOMPONENT_MONIKER_CLASS (G_OBJECT_GET_CLASS (o))

static inline MateComponentMoniker *
matecomponent_moniker_from_servant (PortableServer_Servant servant)
{
	return MATECOMPONENT_MONIKER (matecomponent_object_from_servant (servant));
}

static MateComponent_Moniker
impl_get_parent (PortableServer_Servant servant,
		 CORBA_Environment     *ev)
{
	MateComponentMoniker *moniker = matecomponent_moniker_from_servant (servant);

	return matecomponent_moniker_get_parent (moniker, ev);
}

static void
impl_set_parent (PortableServer_Servant servant,
 		 const MateComponent_Moniker   value,
 		 CORBA_Environment     *ev)
{
 	MateComponentMoniker *moniker = matecomponent_moniker_from_servant (servant);

	matecomponent_moniker_set_parent (moniker, value, ev);
}
 
/**
 * matecomponent_moniker_set_parent:
 * @moniker: the moniker
 * @parent: the parent
 * @opt_ev: an optional corba exception environment
 * 
 *  This sets the monikers parent; a moniker is really a long chain
 * of hierarchical monikers; referenced by the most local moniker.
 * This sets the parent pointer.
 **/
void
matecomponent_moniker_set_parent (MateComponentMoniker     *moniker,
 			   MateComponent_Moniker     parent,
			   CORBA_Environment *opt_ev)
{
	CORBA_Environment ev, *my_ev;

 	matecomponent_return_if_fail (MATECOMPONENT_IS_MONIKER (moniker), opt_ev);

	if (!opt_ev) {
		CORBA_exception_init (&ev);
		my_ev = &ev;
	} else
		my_ev = opt_ev;
	
	if (moniker->priv->parent != CORBA_OBJECT_NIL) {
		MateComponent_Moniker_setParent (moniker->priv->parent, parent, my_ev);
	} else
		moniker->priv->parent = matecomponent_object_dup_ref (parent, my_ev);
	
	if (!opt_ev)
		CORBA_exception_free (&ev);
}
 
/**
 * matecomponent_moniker_get_parent:
 * @moniker: the moniker
 * @opt_ev: an optional corba exception environment
 * 
 * See matecomponent_moniker_set_parent;
 *
 * Return value: the parent of this moniker
 **/
MateComponent_Moniker
matecomponent_moniker_get_parent (MateComponentMoniker     *moniker,
			   CORBA_Environment *opt_ev)
{
	MateComponent_Moniker rval;

	matecomponent_return_val_if_fail (MATECOMPONENT_IS_MONIKER (moniker),
				   CORBA_OBJECT_NIL, opt_ev);
	
	if (moniker->priv->parent == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;

	rval = matecomponent_object_dup_ref (moniker->priv->parent, opt_ev);

	return rval;
}

/**
 * matecomponent_moniker_get_name:
 * @moniker: the moniker
 * 
 * gets the unescaped name of the moniker less the prefix eg
 * file:/tmp/hash\#.gz returns /tmp/hash#.gz
 * 
 * Return value: the string or NULL in case of failure
 **/
const char *
matecomponent_moniker_get_name (MateComponentMoniker *moniker)
{	
	const char *str;

	g_return_val_if_fail (MATECOMPONENT_IS_MONIKER (moniker), NULL);

	str = CLASS (moniker)->get_internal_name (moniker);

	if (str)
		return str + moniker->priv->prefix_len;
	else
		return "";
}

/**
 * matecomponent_moniker_get_name_full:
 * @moniker: the moniker
 * 
 * gets the full unescaped name of the moniker eg.
 * file:/tmp/hash\#.gz returns file:/tmp/hash#.gz
 * 
 * Return value: the string in case of failure
 **/
const char *
matecomponent_moniker_get_name_full (MateComponentMoniker *moniker)
{	
	g_return_val_if_fail (MATECOMPONENT_IS_MONIKER (moniker), NULL);

	return CLASS (moniker)->get_internal_name (moniker);
}

/**
 * matecomponent_moniker_get_name_escaped:
 * @moniker: a moniker
 * 
 * Get the full, escaped name of the moniker eg.
 * file:/tmp/hash\#.gz returns file:/tmp/hash\#.gz
 * 
 * Return value: the dynamically allocated string,
 * or NULL in case of failure.
 * Must release with g_free().
 **/
char *
matecomponent_moniker_get_name_escaped (MateComponentMoniker *moniker)
{
	g_return_val_if_fail (MATECOMPONENT_IS_MONIKER (moniker), NULL);

	return matecomponent_moniker_util_escape (
		CLASS (moniker)->get_internal_name (moniker), 0);
}

/**
 * matecomponent_moniker_set_name:
 * @moniker: the MateComponentMoniker to configure.
 * @name: new, unescaped, name for this moniker.
 *
 * This functions sets the moniker name in @moniker to be @name.
 */
void
matecomponent_moniker_set_name (MateComponentMoniker *moniker,
			 const char    *name)
{
	char *str;

	g_return_if_fail (MATECOMPONENT_IS_MONIKER (moniker));

	str = matecomponent_moniker_util_unescape (name, strlen (name));

	CLASS (moniker)->set_internal_name (moniker, str);

	g_free (str);
}

/**
 * matecomponent_moniker_get_prefix:
 * @moniker: a moniker
 * 
 * Return value: the registered prefix for this moniker or
 * NULL if there isn't one. eg "file:", or in case of failure
 **/
const char *
matecomponent_moniker_get_prefix (MateComponentMoniker *moniker)
{
	g_return_val_if_fail (MATECOMPONENT_IS_MONIKER (moniker), NULL);

	return moniker->priv->prefix;
}

static void
impl_matecomponent_moniker_set_internal_name (MateComponentMoniker *moniker,
				       const char    *unescaped_name)
{
	g_return_if_fail (MATECOMPONENT_IS_MONIKER (moniker));
	g_return_if_fail (strlen (unescaped_name) >= moniker->priv->prefix_len);

	g_free (moniker->priv->name);
	moniker->priv->name = g_strdup (unescaped_name);
}

static const char *
impl_matecomponent_moniker_get_internal_name (MateComponentMoniker *moniker)
{
	g_return_val_if_fail (MATECOMPONENT_IS_MONIKER (moniker), NULL);

	return moniker->priv->name;
}

static CORBA_char *
impl_get_name (PortableServer_Servant servant,
	       CORBA_Environment     *ev)
{
	MateComponentMoniker *moniker = matecomponent_moniker_from_servant (servant);
	CORBA_char    *ans, *parent_name;
	char          *tmp;
	
	parent_name = MateComponent_Moniker_getName (moniker->priv->parent, ev);

	if (MATECOMPONENT_EX (ev))
		return NULL;

	if (!parent_name)
		return CORBA_string_dup (moniker->priv->name);

	if (!moniker->priv->name)
		return parent_name;

	tmp = g_strdup_printf ("%s%s", parent_name, moniker->priv->name);

	if (parent_name)
		CORBA_free (parent_name);

	ans = CORBA_string_dup (tmp);

	g_free (tmp);
	
	return ans;
}

static void
impl_set_name (PortableServer_Servant servant,
	       const CORBA_char      *name,
	       CORBA_Environment     *ev)
{
	const char    *mname;
	int            plen;
	MateComponent_Moniker parent;
	MateComponentMoniker *moniker = matecomponent_moniker_from_servant (servant);

	matecomponent_return_if_fail (moniker->priv != NULL, ev);
	matecomponent_return_if_fail (strlen (name) >= moniker->priv->prefix_len, ev);

	mname = matecomponent_moniker_util_parse_name (name, &plen);

	if (plen) {
		char *pname;

		pname = g_strndup (name, plen);

		parent = matecomponent_moniker_client_new_from_name (pname, ev);
		
		g_free (pname);

		if (MATECOMPONENT_EX (ev))
			return;

		matecomponent_object_release_unref (moniker->priv->parent, NULL);
		moniker->priv->parent = matecomponent_object_dup_ref (parent, ev);
	}

	matecomponent_moniker_set_name (moniker, mname);
}

static MateComponent_Unknown
impl_resolve (PortableServer_Servant       servant,
	      const MateComponent_ResolveOptions *options,
	      const CORBA_char            *requested_interface,
	      CORBA_Environment           *ev)
{
	MateComponentMoniker *moniker = matecomponent_moniker_from_servant (servant);
	MateComponent_Unknown retval;

	/* Try a standard resolve */
	retval = CLASS (moniker)->resolve (moniker, options,
					   requested_interface, ev);

	/* Try an extender */
	if (!MATECOMPONENT_EX (ev) && retval == CORBA_OBJECT_NIL &&
	    moniker->priv->prefix) {

		MateComponent_Unknown extender;
		
		extender = matecomponent_moniker_find_extender (
			moniker->priv->prefix,
			requested_interface, ev);
		
		if (MATECOMPONENT_EX (ev))
			return CORBA_OBJECT_NIL;

		else if (extender != CORBA_OBJECT_NIL) {
			retval = MateComponent_MonikerExtender_resolve (
				extender, MATECOMPONENT_OBJREF (moniker),
				options, moniker->priv->name,
				requested_interface, ev);

			matecomponent_object_release_unref (extender, ev);
		}
	}

	if (!MATECOMPONENT_EX (ev) && retval == CORBA_OBJECT_NIL) {
		matecomponent_exception_general_error_set (
			ev, NULL, _("Failed to resolve, or extend '%s'"),
			matecomponent_moniker_get_name_full (moniker));
	}

	return retval;
}

static CORBA_long
impl_equal (PortableServer_Servant servant,
	    const CORBA_char      *moniker_name,
	    CORBA_Environment     *ev)
{
	int            i, retval;
	CORBA_long     offset;
	const char    *p;
	char          *name;
	MateComponentMoniker *moniker = matecomponent_moniker_from_servant (servant);
	
	if (moniker->priv->parent != CORBA_OBJECT_NIL) {
		offset = MateComponent_Moniker_equal (
			moniker->priv->parent, moniker_name, ev);
		if (MATECOMPONENT_EX (ev) || offset == 0)
			return 0;
	} else
		offset = 0;
	
	p = &moniker_name [offset];

	i = matecomponent_moniker_util_seek_std_separator (p, moniker->priv->prefix_len);
	
	name = matecomponent_moniker_get_name_escaped (moniker);

/*	g_warning ("Compare %d chars of '%s' to '%s' - case sensitive ?%c",
	i, name, p, moniker->priv->sensitive?'y':'n');*/
	
	if (( moniker->priv->sensitive && !strncmp       (name, p, i)) ||
	    (!moniker->priv->sensitive && !g_ascii_strncasecmp (name, p, i))) {
/*		g_warning ("Matching moniker - equal");*/
		retval = i + offset;
	} else {
/*		g_warning ("No match");*/
		retval = 0;
	}
 
	g_free (name);
 
	return retval;
}

static void
matecomponent_moniker_finalize (GObject *object)
{
	MateComponentMoniker *moniker = MATECOMPONENT_MONIKER (object);

	if (moniker->priv->parent != CORBA_OBJECT_NIL)
		matecomponent_object_release_unref (moniker->priv->parent, NULL);

	g_free (moniker->priv->prefix);
	g_free (moniker->priv->name);
	g_free (moniker->priv);

	matecomponent_moniker_parent_class->finalize (object);
}

static void
matecomponent_moniker_class_init (MateComponentMonikerClass *klass)
{
	GObjectClass *oclass = (GObjectClass *)klass;
	POA_MateComponent_Moniker__epv *epv = &klass->epv;

	matecomponent_moniker_parent_class = g_type_class_peek_parent (klass);

	oclass->finalize = matecomponent_moniker_finalize;

	klass->set_internal_name = impl_matecomponent_moniker_set_internal_name;
	klass->get_internal_name = impl_matecomponent_moniker_get_internal_name;

	epv->getParent           = impl_get_parent;
	epv->setParent           = impl_set_parent;
	epv->getName             = impl_get_name;
	epv->setName             = impl_set_name;
	epv->resolve             = impl_resolve;
	epv->equal               = impl_equal;
}

static void
matecomponent_moniker_init (GObject *object)
{
	MateComponentMoniker *moniker = MATECOMPONENT_MONIKER (object);

	moniker->priv = g_new (MateComponentMonikerPrivate, 1);

	moniker->priv->parent = CORBA_OBJECT_NIL;
	moniker->priv->name   = NULL;
	moniker->priv->prefix = NULL;
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentMoniker, 
		       MateComponent_Moniker,
		       PARENT_TYPE,
		       matecomponent_moniker)

/**
 * matecomponent_moniker_construct:
 * @moniker: an un-constructed moniker object.
 * @prefix: the prefix name of the moniker eg. 'file:', '!' or 'tar:' or NULL
 * 
 *  Constructs a newly created matecomponent moniker with the given arguments.
 * 
 * Return value: the constructed moniker or NULL on failure.
 **/
MateComponentMoniker *
matecomponent_moniker_construct (MateComponentMoniker *moniker,
			  const char    *prefix)
{
	if (prefix) {
		moniker->priv->prefix = g_strdup (prefix);
		moniker->priv->prefix_len = strlen (prefix);
	}

	moniker->priv->sensitive = TRUE;

	return moniker;
}

/**
 * matecomponent_moniker_set_case_sensitive:
 * @moniker: the moniker
 * @sensitive: whether to see case on equality compare
 * 
 * Sets up whether we use case sensitivity for the default equal impl.
 **/
void
matecomponent_moniker_set_case_sensitive (MateComponentMoniker *moniker,
				   gboolean       sensitive)
{
	g_return_if_fail (MATECOMPONENT_IS_MONIKER (moniker));

	moniker->priv->sensitive = sensitive;
}

/**
 * matecomponent_moniker_get_case_sensitive:
 * @moniker: the moniker
 *
 * Return value: whether we use case sensitivity for the default equal impl.
 **/
gboolean
matecomponent_moniker_get_case_sensitive (MateComponentMoniker *moniker)
{
	g_return_val_if_fail (MATECOMPONENT_IS_MONIKER (moniker), FALSE);

	return moniker->priv->sensitive;
}
