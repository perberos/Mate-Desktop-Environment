/**
 * matecomponent-config-bag.c: config bag object implementation.
 *
 * Author:
 *   Dietmar Maurer (dietmar@ximian.com)
 *   Rodrigo Moya   (rodrigo@ximian.com)
 *
 * Copyright 2000 Ximian, Inc.
 */
#include <config.h>
#include <glib/gi18n-lib.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-arg.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-exception.h>
#include <string.h>

#include "matecomponent-config-bag.h"

#define PARENT_TYPE (MATECOMPONENT_TYPE_OBJECT)

#define GET_BAG_FROM_SERVANT(servant) MATECOMPONENT_CONFIG_BAG (matecomponent_object (servant))

static GObjectClass *parent_class = NULL;

#define CLASS(o) MATECOMPONENT_CONFIG_BAG_CLASS (G_OBJECT_GET_CLASS (o))

static void
matecomponent_config_bag_finalize (GObject *object)
{
	MateComponentConfigBag *cb = MATECOMPONENT_CONFIG_BAG (object);

	g_free (cb->path);
	g_object_unref (cb->conf_client);

	parent_class->finalize (object);
}

static MateComponent_KeyList *
impl_MateComponent_PropertyBag_getKeys (PortableServer_Servant  servant,
				 const CORBA_char       *filter,
				 CORBA_Environment      *ev)
{
	MateComponentConfigBag *cb = GET_BAG_FROM_SERVANT (servant);
	char            *path;
	MateComponent_KeyList  *retval;
	GSList          *slist, *sl;
	GError          *err = NULL;
	int              length;
	int              n;

	if (strchr (filter, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	path = g_strconcat (cb->path, "/", filter, NULL);

	/* get keys from MateConf */
	slist = mateconf_client_all_entries (cb->conf_client, path, &err);
	g_free (path);
	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
		return CORBA_OBJECT_NIL;
	}

	/* create CORBA sequence */
	length = g_slist_length (slist);
	retval = MateComponent_KeyList__alloc ();
	retval->_length = length;
	CORBA_sequence_set_release (retval, TRUE);
	retval->_buffer = MateComponent_KeyList_allocbuf (length);

	for (sl = slist, n = 0; n < length; sl = sl->next, n++) {
		MateConfEntry *entry = (MateConfEntry *) sl->data;
		const char *entry_name;

		entry_name = mateconf_entry_get_key (entry);
		retval->_buffer[n] = CORBA_string_dup (entry_name);
	}

	g_slist_free (slist);

	return retval;
}

static CORBA_TypeCode
impl_MateComponent_PropertyBag_getType (PortableServer_Servant  servant,
				 const CORBA_char       *key,
				 CORBA_Environment      *ev)
{
	MateComponentConfigBag *cb = GET_BAG_FROM_SERVANT (servant);
	char            *path;
	MateConfValue      *value;
	GError          *err = NULL;

	if (strchr (key, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return CORBA_OBJECT_NIL;
	}

	path = g_strconcat (cb->path, "/", key, NULL);

	/* get type for the given key */
	value = mateconf_client_get (cb->conf_client, path, &err);
	g_free (path);
	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
		return CORBA_OBJECT_NIL;
	}

	switch (value->type) {
	case MATECONF_VALUE_STRING :
		return (CORBA_TypeCode)
			CORBA_Object_duplicate ((CORBA_Object) MATECOMPONENT_ARG_STRING, ev);
	case MATECONF_VALUE_INT :
		return (CORBA_TypeCode)
			CORBA_Object_duplicate ((CORBA_Object) MATECOMPONENT_ARG_LONG, ev);
	case MATECONF_VALUE_FLOAT :
		return (CORBA_TypeCode)
			CORBA_Object_duplicate ((CORBA_Object) MATECOMPONENT_ARG_DOUBLE, ev);
	case MATECONF_VALUE_BOOL :
		return (CORBA_TypeCode)
			CORBA_Object_duplicate ((CORBA_Object) MATECOMPONENT_ARG_BOOLEAN, ev);
	default :
		/* FIXME */
		break;
	}

	return CORBA_OBJECT_NIL;
}

static MateComponentArg*
matecomponent_arg_new_from_mateconf_value (MateConfValue *value)
{
        if (value == NULL)
                return matecomponent_arg_new (MATECOMPONENT_ARG_NULL);

	switch (value->type) {
	case MATECONF_VALUE_STRING :
		return matecomponent_arg_new_from (MATECOMPONENT_ARG_STRING,
					    mateconf_value_get_string (value));
	case MATECONF_VALUE_INT : {
                long v = mateconf_value_get_int (value);
		return matecomponent_arg_new_from (MATECOMPONENT_ARG_LONG, &v);
        }
	case MATECONF_VALUE_FLOAT : {
                double v = mateconf_value_get_float (value);
		return matecomponent_arg_new_from (MATECOMPONENT_ARG_DOUBLE, &v);
        }
	case MATECONF_VALUE_BOOL : {
                gboolean v = mateconf_value_get_bool (value);
		return matecomponent_arg_new_from (MATECOMPONENT_ARG_BOOLEAN, &v);
        }
	default :
		return matecomponent_arg_new (MATECOMPONENT_ARG_NULL);
	}
}

static CORBA_any *
impl_MateComponent_PropertyBag_getValue (PortableServer_Servant  servant,
				  const CORBA_char       *key,
				  CORBA_Environment      *ev)
{
	MateComponentConfigBag *cb = GET_BAG_FROM_SERVANT (servant);
	char            *path;
	MateConfValue      *value;
	GError          *err = NULL;

	if (strchr (key, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	path = g_strconcat (cb->path, "/", key, NULL);

	value = mateconf_client_get (cb->conf_client, path, &err);
	g_free (path);
	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
		return CORBA_OBJECT_NIL;
	}

        /* FIXME The original code here returned MateComponentArg*
         * as a CORBA_any*, is that OK?
         */
        return matecomponent_arg_new_from_mateconf_value (value);
}

static void
impl_MateComponent_PropertyBag_setValue (PortableServer_Servant  servant,
				  const CORBA_char       *key,
				  const CORBA_any        *value,
				  CORBA_Environment      *ev)
{
	MateComponentConfigBag *cb = GET_BAG_FROM_SERVANT (servant);
	char            *path;
	GError          *err = NULL;

	if (strchr (key, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return;
	}

	path = g_strconcat (cb->path, "/", key, NULL);

	if (matecomponent_arg_type_is_equal (value->_type, MATECOMPONENT_ARG_STRING, ev)) {
		mateconf_client_set_string (cb->conf_client, path,
					 MATECOMPONENT_ARG_GET_STRING (value), &err);
	}
	else if (matecomponent_arg_type_is_equal (value->_type, MATECOMPONENT_ARG_LONG, ev)) {
		mateconf_client_set_int (cb->conf_client, path,
				      MATECOMPONENT_ARG_GET_LONG (value), &err);
	}
	else if (matecomponent_arg_type_is_equal (value->_type, MATECOMPONENT_ARG_DOUBLE, ev)) {
		mateconf_client_set_float (cb->conf_client, path,
					MATECOMPONENT_ARG_GET_DOUBLE (value), &err);
	}
	else if (matecomponent_arg_type_is_equal (value->_type, MATECOMPONENT_ARG_BOOLEAN, ev)) {
		mateconf_client_set_bool (cb->conf_client, path,
				       MATECOMPONENT_ARG_GET_BOOLEAN (value), &err);
	}
	else if (matecomponent_arg_type_is_equal (value->_type, MATECOMPONENT_ARG_NULL, ev)) {
		mateconf_client_unset (cb->conf_client, path, &err);
	}
	else {
		g_free (path);
		matecomponent_exception_general_error_set (ev, NULL, _("Unknown type"));
		return;
	}

	g_free (path);

	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
	}
}

static MateComponent_PropertySet *
impl_MateComponent_PropertyBag_getValues (PortableServer_Servant servant,
				   const CORBA_char       *filter,
				   CORBA_Environment      *ev)
{
	MateComponentConfigBag *cb = GET_BAG_FROM_SERVANT (servant);
	char *path;
	MateComponent_PropertySet *retval;
	GSList *slist, *sl;
	GError *err = NULL;
	int length;
	int n;

	if (strchr (filter, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	path = g_strconcat (cb->path, "/", filter, NULL);

	/* get keys from MateConf */
	slist = mateconf_client_all_entries (cb->conf_client, path, &err);
	g_free (path);
	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
		return CORBA_OBJECT_NIL;
	}

	/* create CORBA sequence */
	length = g_slist_length (slist);
	retval = MateComponent_PropertySet__alloc ();
	retval->_length = length;
	CORBA_sequence_set_release (retval, TRUE);
	retval->_buffer = CORBA_sequence_MateComponent_Pair_allocbuf (length);

	for (sl = slist, n = 0; n < length; sl = sl->next, n++) {
		MateConfEntry *entry = (MateConfEntry *) sl->data;
		MateComponentArg *arg;
                MateConfValue *value;

		retval->_buffer[n].name = CORBA_string_dup (mateconf_entry_get_key (entry));
                value = mateconf_entry_get_value (entry);

                arg = matecomponent_arg_new_from_mateconf_value (value);

		retval->_buffer[n].value = *arg;
	}

	g_slist_free (slist);

	return retval;
}

static void
impl_MateComponent_PropertyBag_setValues (PortableServer_Servant servant,
				   const MateComponent_PropertySet *set,
				   CORBA_Environment *ev)
{
	int i;

	for (i = 0; i < set->_length; i++) {
		impl_MateComponent_PropertyBag_setValue (servant,
						  set->_buffer [i].name,
						  &set->_buffer [i].value,
						  ev);
		if (MATECOMPONENT_EX (ev))
			return;
	}
}

static CORBA_any *
impl_MateComponent_PropertyBag_getDefault (PortableServer_Servant  servant,
				    const CORBA_char       *key,
				    CORBA_Environment      *ev)
{
	MateComponentConfigBag *cb = GET_BAG_FROM_SERVANT (servant);
	char            *path;
	MateConfValue      *value;
	GError          *err = NULL;

	if (strchr (key, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	path = g_strconcat (cb->path, "/", key, NULL);

	value = mateconf_client_get_default_from_schema (cb->conf_client, path, &err);
	g_free (path);
	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
		return CORBA_OBJECT_NIL;
	}

        return matecomponent_arg_new_from_mateconf_value (value);
}

static CORBA_char *
impl_MateComponent_PropertyBag_getDocTitle (PortableServer_Servant  servant,
				     const CORBA_char       *key,
				     CORBA_Environment      *ev)
{
	MateComponentConfigBag *cb = GET_BAG_FROM_SERVANT (servant);
	char            *path;
	CORBA_char      *retval;
	MateConfSchema     *schema;
	GError          *err = NULL;

	if (strchr (key, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	path = g_strconcat (cb->path, "/", key, NULL);
	schema = mateconf_client_get_schema (cb->conf_client, path, &err);
	g_free (path);
	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
		return NULL;
	}

	retval = CORBA_string_dup (mateconf_schema_get_short_desc (schema));

	mateconf_schema_free (schema);

	return retval;
}

static CORBA_char *
impl_MateComponent_PropertyBag_getDoc (PortableServer_Servant  servant,
				const CORBA_char       *key,
				CORBA_Environment      *ev)
{
	MateComponentConfigBag *cb = GET_BAG_FROM_SERVANT (servant);
	char            *path;
	CORBA_char      *retval;
	MateConfSchema     *schema;
	GError          *err = NULL;

	if (strchr (key, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	path = g_strconcat (cb->path, "/", key, NULL);

	schema = mateconf_client_get_schema (cb->conf_client, path, &err);
	g_free (path);
	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
		return NULL;
	}

	retval = CORBA_string_dup (mateconf_schema_get_long_desc (schema));

	mateconf_schema_free (schema);

	return retval;
}

static MateComponent_PropertyFlags
impl_MateComponent_PropertyBag_getFlags (PortableServer_Servant  servant,
				  const CORBA_char       *key,
				  CORBA_Environment      *ev)
{
	MateComponentConfigBag      *cb = GET_BAG_FROM_SERVANT (servant);
	char                 *path;
	MateComponent_PropertyFlags  retval = 0;
	MateConfEntry           *entry;
	GError               *err = NULL;

	if (strchr (key, '/')) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return 0;
	}

	path = g_strconcat (cb->path, "/", key, NULL);
	entry = mateconf_client_get_entry (cb->conf_client, path, NULL, TRUE, &err);
	g_free (path);
	if (err) {
		matecomponent_exception_general_error_set (ev, NULL, "%s", err->message);
		g_error_free (err);
		return 0;
	}

	retval |= MateComponent_PROPERTY_READABLE;
	if (mateconf_entry_get_is_writable (entry))
		retval |= MateComponent_PROPERTY_WRITEABLE;

	mateconf_entry_free (entry);

	return retval;
}

MateComponentConfigBag *
matecomponent_config_bag_new (const gchar *path)
{
	MateComponentConfigBag *cb;
	char *m;
	int l;

	g_return_val_if_fail (path != NULL, NULL);

	cb = g_object_new (MATECOMPONENT_TYPE_CONFIG_BAG, NULL);

	if (path[0] == '/')
		cb->path = g_strdup (path);
	else
		cb->path = g_strconcat ("/", path, NULL);

	while ((l = strlen (cb->path)) > 1 && path [l - 1] == '/')
		cb->path [l] = '\0';

	cb->es = matecomponent_event_source_new ();

	matecomponent_object_add_interface (MATECOMPONENT_OBJECT (cb),
				     MATECOMPONENT_OBJECT (cb->es));

	m = g_strconcat ("MateComponent/ConfigDatabase:change", cb->path, ":", NULL);

	/* matecomponent_event_source_client_add_listener (db, notify_cb, m, NULL, cb); */

	g_free (m);

	/* initialize MateConf client */
	if (!mateconf_is_initialized ())
		mateconf_init (0, NULL, NULL);
	cb->conf_client = mateconf_client_get_default ();

	return cb;
}

static void
matecomponent_config_bag_class_init (MateComponentConfigBagClass *class)
{
	GObjectClass *object_class = (GObjectClass *) class;
	POA_MateComponent_PropertyBag__epv *epv= &class->epv;

	parent_class = g_type_class_peek_parent (class);

	object_class->finalize = matecomponent_config_bag_finalize;

	epv->getKeys       = impl_MateComponent_PropertyBag_getKeys;
	epv->getType       = impl_MateComponent_PropertyBag_getType;
	epv->getValue      = impl_MateComponent_PropertyBag_getValue;
	epv->setValue      = impl_MateComponent_PropertyBag_setValue;
	epv->getValues     = impl_MateComponent_PropertyBag_getValues;
	epv->setValues     = impl_MateComponent_PropertyBag_setValues;
	epv->getDefault    = impl_MateComponent_PropertyBag_getDefault;
	epv->getDocTitle   = impl_MateComponent_PropertyBag_getDocTitle;
	epv->getDoc        = impl_MateComponent_PropertyBag_getDoc;
	epv->getFlags      = impl_MateComponent_PropertyBag_getFlags;
}

static void
matecomponent_config_bag_init (MateComponentConfigBag *cb)
{
	/* nothing to do */
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentConfigBag,
		       MateComponent_PropertyBag,
		       PARENT_TYPE,
		       matecomponent_config_bag);

