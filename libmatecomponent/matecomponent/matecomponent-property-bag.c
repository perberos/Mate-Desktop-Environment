/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-property-bag.c: property bag object implementation.
 *
 * Authors:
 *   Nat Friedman   (nat@ximian.com)
 *   Michael Meeks  (michael@ximian.com)
 *   Dietmar Maurer (dietmar@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#include <config.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-property-bag.h>

#include <matecomponent/matecomponent-marshal.h>
#include <matecomponent/matecomponent-types.h>

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

#define CLASS(o) MATECOMPONENT_PROPERTY_BAG_CLASS (G_OBJECT_GET_CLASS (o))

#define BAG_FROM_SERVANT(servant) MATECOMPONENT_PROPERTY_BAG (matecomponent_object (servant))

static GObjectClass *parent_class = NULL;
         

/*
 * Internal data structures.
 */
struct _MateComponentPropertyPrivate {
	GClosure             *get_prop;
	GClosure             *set_prop;	
};
	
struct _MateComponentPropertyBagPrivate {
	GHashTable *prop_hash;

	GClosure   *get_prop;
	GClosure   *set_prop;
};

static void
notify_listeners (MateComponentPropertyBag  *pb,
		  MateComponentProperty     *prop,
		  const MateComponentArg    *new_value,
		  CORBA_Environment  *opt_ev)
{
	if (prop->flags & MateComponent_PROPERTY_NO_LISTENING)
		return;
	
	matecomponent_event_source_notify_listeners_full (pb->es, "MateComponent/Property",
						   "change", prop->name,
						   new_value, opt_ev);
}

static void
matecomponent_property_bag_foreach_create_list (gpointer key, 
					 gpointer value,
					 gpointer data)
{
	GList **l = (GList **) data;

	*l = g_list_prepend (*l, value);
}

/**
 * matecomponent_property_bag_get_prop_list:
 * @pb: A #MateComponentPropertyBag.
 *
 * Returns a #GList of #MateComponentProperty structures.  This function is
 * private and should only be used internally, or in a PropertyBag
 * persistence implementation.  You should not touch the
 * #MateComponentProperty structure unless you know what you're doing.
 */
GList *
matecomponent_property_bag_get_prop_list (MateComponentPropertyBag *pb)
{
	GList *l;

	g_return_val_if_fail (pb != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_PROPERTY_BAG (pb), NULL);

	l = NULL;

	g_hash_table_foreach (pb->priv->prop_hash,
			      matecomponent_property_bag_foreach_create_list,
			      &l);

	return l;
}

static MateComponent_KeyList *
impl_MateComponent_PropertyBag_getKeys (PortableServer_Servant  servant,
				 const CORBA_char       *filter,
				 CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponent_KeyList  	*name_list;
	GList			*props;
	GList			*curr;
	int                      len;

	len = g_hash_table_size (pb->priv->prop_hash);

	name_list = MateComponent_KeyList__alloc ();

	if (len == 0)
		return name_list;

	name_list->_buffer = CORBA_sequence_CORBA_string_allocbuf (len);
	CORBA_sequence_set_release (name_list, TRUE);

	props = matecomponent_property_bag_get_prop_list (pb);

	for (curr = props; curr != NULL; curr = curr->next) {
		MateComponentProperty *prop = curr->data;

		name_list->_buffer [name_list->_length] = 
			CORBA_string_dup (prop->name);
		
		name_list->_length++;
	}

	g_list_free (props);

	return name_list;
}

static CORBA_TypeCode
impl_MateComponent_PropertyBag_getType (PortableServer_Servant  servant,
				 const CORBA_char       *key,
				 CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponentProperty          *prop;

	prop = g_hash_table_lookup (pb->priv->prop_hash, key);

	if (!prop || !prop->type) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return TC_null;
	}

	return (CORBA_TypeCode) CORBA_Object_duplicate 
		((CORBA_Object) prop->type, ev);
}

static CORBA_any *
impl_MateComponent_PropertyBag_getValue (PortableServer_Servant  servant,
				  const CORBA_char       *key,
				  CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponentProperty          *prop;
	MateComponentArg               *arg;

	prop = g_hash_table_lookup (pb->priv->prop_hash, key);

	if (!prop || !prop->priv->get_prop) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	matecomponent_closure_invoke (prop->priv->get_prop,
			       MATECOMPONENT_TYPE_STATIC_CORBA_ANY,       &arg,
			       MATECOMPONENT_TYPE_PROPERTY_BAG,           pb,
			       MATECOMPONENT_TYPE_STATIC_CORBA_TYPECODE,  prop->type,
			       G_TYPE_UINT,                        prop->idx,
			       MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION, ev,
			       G_TYPE_INVALID);

	return arg;
}

static MateComponent_PropertySet *
impl_MateComponent_PropertyBag_getValues (PortableServer_Servant  servant,
				   const CORBA_char       *filter,
				   CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponent_PropertySet      *set;
	GList		        *props;
	GList		        *curr;
	int		         len;

	len = g_hash_table_size (pb->priv->prop_hash);

	set = MateComponent_PropertySet__alloc ();

	if (len == 0)
		return set;

	set->_buffer = CORBA_sequence_MateComponent_Pair_allocbuf (len);
	CORBA_sequence_set_release (set, TRUE);

	props = matecomponent_property_bag_get_prop_list (pb);

	for (curr = props; curr != NULL; curr = curr->next) {
		MateComponentProperty *prop = curr->data;
		MateComponentArg *arg;

		set->_buffer [set->_length].name =  
			CORBA_string_dup (prop->name);

		matecomponent_closure_invoke (prop->priv->get_prop,
				       MATECOMPONENT_TYPE_STATIC_CORBA_ANY,       &arg,
				       MATECOMPONENT_TYPE_PROPERTY_BAG,           pb,
				       MATECOMPONENT_TYPE_STATIC_CORBA_TYPECODE,  prop->type,
				       G_TYPE_UINT,                        prop->idx,
				       MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION, ev,
				       G_TYPE_INVALID);

		set->_buffer [set->_length].value = *arg;

		set->_length++;
	}

	g_list_free (props);

	return set;
}

static void 
impl_MateComponent_PropertyBag_setValue (PortableServer_Servant  servant,
				  const CORBA_char       *key,
				  const CORBA_any        *value,
				  CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponentProperty          *prop;

	prop = g_hash_table_lookup (pb->priv->prop_hash, key);

	if (!prop || !prop->priv->set_prop) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return;
	}

	if (!matecomponent_arg_type_is_equal (prop->type, value->_type, ev)) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_InvalidType);
		return;
	}

	matecomponent_closure_invoke (prop->priv->set_prop,
			       G_TYPE_NONE,
			       MATECOMPONENT_TYPE_PROPERTY_BAG,           pb,
			       MATECOMPONENT_TYPE_STATIC_CORBA_ANY,       value,
			       G_TYPE_UINT,                        prop->idx,
			       MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION, ev,
			       G_TYPE_INVALID);

	if (prop->flags & MateComponent_PROPERTY_NO_AUTONOTIFY)
		return;
	
	if (!MATECOMPONENT_EX (ev))
		notify_listeners (pb, prop, value, NULL);
}

static void 
impl_MateComponent_PropertyBag_setValues (PortableServer_Servant    servant,
				   const MateComponent_PropertySet *set,
				   CORBA_Environment        *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponentProperty          *prop;
	int i;

	for (i = 0; i < set->_length; i++) {
		prop = g_hash_table_lookup (pb->priv->prop_hash, 
					    set->_buffer [i].name);
		
		if (!prop || !prop->priv->set_prop) {
			matecomponent_exception_set (ev, 
			        ex_MateComponent_PropertyBag_NotFound);
			return;
		}

		if (!matecomponent_arg_type_is_equal (prop->type, 
					       set->_buffer [i].value._type,
					       ev)) {
			matecomponent_exception_set (ev, 
			        ex_MateComponent_PropertyBag_InvalidType);
			return;
		}
	}

	for (i = 0; i < set->_length; i++) {
		prop = g_hash_table_lookup (pb->priv->prop_hash, 
					    set->_buffer [i].name);
		
		matecomponent_closure_invoke (prop->priv->set_prop,
				       G_TYPE_NONE,
				       MATECOMPONENT_TYPE_PROPERTY_BAG,           pb,
				       MATECOMPONENT_TYPE_STATIC_CORBA_ANY,       &set->_buffer [i].value,
				       G_TYPE_UINT,                        prop->idx,
				       MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION, ev,
				       G_TYPE_INVALID);

		if (MATECOMPONENT_EX (ev))
			return;

		if (! (prop->flags & MateComponent_PROPERTY_NO_AUTONOTIFY))
			notify_listeners (pb, prop, &set->_buffer [i].value, NULL);
	}
}

static CORBA_any *
impl_MateComponent_PropertyBag_getDefault (PortableServer_Servant  servant,
				    const CORBA_char       *key,
				    CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponentProperty          *prop;
	
	if (!(prop = g_hash_table_lookup (pb->priv->prop_hash, key))) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	if (prop->default_value)
		return matecomponent_arg_copy (prop->default_value);
	else {
		MateComponentArg *value = matecomponent_arg_new (prop->type);
		return value;
	}
}

static CORBA_char *
impl_MateComponent_PropertyBag_getDocTitle (PortableServer_Servant  servant,
				     const CORBA_char       *key,
				     CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponentProperty          *prop;

	if (!(prop = g_hash_table_lookup (pb->priv->prop_hash, key))) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	return prop->doctitle ? CORBA_string_dup (prop->doctitle) :
		CORBA_string_dup ("");		
}

static CORBA_char *
impl_MateComponent_PropertyBag_getDoc (PortableServer_Servant  servant,
				const CORBA_char       *key,
				CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponentProperty          *prop;

	if (!(prop = g_hash_table_lookup (pb->priv->prop_hash, key))) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return NULL;
	}

	return prop->docstring ? CORBA_string_dup (prop->docstring) :
		CORBA_string_dup ("");
}

static CORBA_long
impl_MateComponent_PropertyBag_getFlags (PortableServer_Servant  servant,
				  const CORBA_char       *key,
				  CORBA_Environment      *ev)
{
	MateComponentPropertyBag *pb = BAG_FROM_SERVANT (servant);
	MateComponentProperty          *prop;

	if (!(prop = g_hash_table_lookup (pb->priv->prop_hash, key))) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return 0;
	}

	return prop->flags;
}



/*
 * MateComponentPropertyBag construction/deconstruction functions. 
 */

static void
matecomponent_marshal_ANY__TYPECODE_UINT_EXCEPTION (GClosure     *closure,
					     GValue       *return_value,
					     guint         n_param_values,
					     const GValue *param_values,
					     gpointer      invocation_hint,
					     gpointer      marshal_data)
{
	typedef void (*GMarshalFunc_VOID__BOXED_UINT_BOXED) (gpointer     data1,
							     gpointer     arg_1,
							     guint        arg_2,
							     gpointer     arg_3,
							     gpointer     data2);
	register GMarshalFunc_VOID__BOXED_UINT_BOXED callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;
	CORBA_TypeCode tc;
	MateComponentArg *any;

	g_return_if_fail (n_param_values == 4);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__BOXED_UINT_BOXED) (marshal_data ? marshal_data : cc->callback);

	tc = matecomponent_value_get_corba_typecode (param_values + 1);
	any = matecomponent_arg_new (tc);
	CORBA_Object_release ((CORBA_Object) tc, NULL);

	callback (data1,
		  any,
		  g_value_get_uint (param_values + 2),
		  g_value_peek_pointer (param_values + 3),
		  data2);

	g_value_take_boxed (return_value, any);
}


/**
 * matecomponent_property_bag_construct:
 * @pb: #MateComponentPropertyBag to construct
 * @get_prop: the property get closure
 * @set_prop: the property set closure
 * @es: an event source to aggregate
 * 
 * Constructor, only for use in wrappers and object derivation, please
 * refer to the #matecomponent_property_bag_new for normal use.
 *
 * This function returns @pb, or %NULL in case of error.  If it returns %NULL,
 * the passed in @pb is unrefed.
 *
 * Returns:  #MateComponentPropertyBag pointer or %NULL.
 */
MateComponentPropertyBag *
matecomponent_property_bag_construct (MateComponentPropertyBag *pb,
			       GClosure          *get_prop,
			       GClosure          *set_prop,
			       MateComponentEventSource *es)
{
	pb->es             = es;
	pb->priv->get_prop = matecomponent_closure_store (get_prop, matecomponent_marshal_ANY__TYPECODE_UINT_EXCEPTION);
	pb->priv->set_prop = matecomponent_closure_store (set_prop, matecomponent_marshal_VOID__BOXED_UINT_BOXED);

	matecomponent_object_add_interface (MATECOMPONENT_OBJECT (pb), MATECOMPONENT_OBJECT (es));
	
	return pb;
}

/**
 * matecomponent_property_bag_new_full:
 * @get_prop: the property get closure
 * @set_prop: the property set closure
 * @es: an event source to aggregate
 *
 * Creates a new property bag with the specified callbacks.
 *
 * Returns: A new #MateComponentPropertyBag object.
 */
MateComponentPropertyBag *
matecomponent_property_bag_new_full (GClosure          *get_prop,
			      GClosure          *set_prop,
			      MateComponentEventSource *es)
{
	MateComponentPropertyBag *pb;

	g_return_val_if_fail (es != NULL, NULL);

	pb = g_object_new (MATECOMPONENT_TYPE_PROPERTY_BAG, NULL);

	return matecomponent_property_bag_construct (pb, get_prop, set_prop, es);
}

/**
 * matecomponent_property_bag_new:
 * @get_prop_cb: the property get callback
 * @set_prop_cb: the property set callback
 * @user_data: user data for the callbacks
 *
 * Creates a new property bag with the specified callbacks.
 *
 * Returns: A new #MateComponentPropertyBag object.
 */
MateComponentPropertyBag *
matecomponent_property_bag_new	           (MateComponentPropertyGetFn get_prop_cb,
			            MateComponentPropertySetFn set_prop_cb,
			            gpointer            user_data)
{
	return matecomponent_property_bag_new_closure (
		get_prop_cb ? g_cclosure_new (
			G_CALLBACK (get_prop_cb), user_data, NULL) : NULL,
		set_prop_cb ? g_cclosure_new (
			G_CALLBACK (set_prop_cb), user_data, NULL) : NULL);
}

/**
 * matecomponent_property_bag_new_closure:
 * @get_prop: the property get closure
 * @set_prop: the property set closure
 *
 * Creates a new property bag with the specified callbacks.
 *
 * Returns: A new #MateComponentPropertyBag object.
 */
MateComponentPropertyBag *
matecomponent_property_bag_new_closure (GClosure *get_prop,
				 GClosure *set_prop)
{
	MateComponentEventSource *es;

	es = matecomponent_event_source_new ();

	return matecomponent_property_bag_new_full (get_prop, set_prop, es);
}

static gboolean
matecomponent_property_bag_foreach_remove_prop (gpointer key, 
					 gpointer value,
					 gpointer user_data)
{
	MateComponentProperty *prop = (MateComponentProperty *)value;

	g_free (prop->name);
	prop->idx = -1;

	matecomponent_arg_release (prop->default_value);

	g_free (prop->docstring);
	g_free (prop->doctitle);

	if (prop->priv->get_prop)
		g_closure_unref (prop->priv->get_prop);
	if (prop->priv->set_prop)
		g_closure_unref (prop->priv->set_prop);

	g_free (prop->priv);
	g_free (prop);

	return TRUE;
}

static void
matecomponent_property_bag_finalize (GObject *object)
{
	MateComponentPropertyBag *pb = MATECOMPONENT_PROPERTY_BAG (object);
	
	/* Destroy all properties. */
	g_hash_table_foreach_remove (pb->priv->prop_hash,
				     matecomponent_property_bag_foreach_remove_prop,
				     NULL);

	g_hash_table_destroy (pb->priv->prop_hash);

	if (pb->priv->get_prop)
		g_closure_unref (pb->priv->get_prop);
	if (pb->priv->set_prop)
		g_closure_unref (pb->priv->set_prop);
	
	g_free (pb->priv);

	parent_class->finalize (object);
}


/*
 * MateComponentPropertyBag property manipulation API.
 */

/**
 * matecomponent_property_bag_add_full:
 * @pb: property bag to add to
 * @name: name of new property
 * @idx: integer index for fast callback switch statement
 * @type: the CORBA type eg. TC_long
 * @default_value: the default value or NULL
 * @doctitle: the translated documentation title
 * @docstring: the translated documentation string
 * @flags: various flags
 * @get_prop: a per property get callback
 * @set_prop: a per property set callback
 * 
 * This adds a property to @pb at the full tilt of complexity.
 **/
void
matecomponent_property_bag_add_full (MateComponentPropertyBag    *pb,
			      const char           *name,
			      int                   idx,
			      MateComponentArgType         type,
			      MateComponentArg            *default_value,
			      const char           *doctitle,
			      const char           *docstring,
			      MateComponent_PropertyFlags  flags,
			      GClosure             *get_prop,
			      GClosure             *set_prop)
{
	MateComponentProperty *prop;

	g_return_if_fail (pb != NULL);
	g_return_if_fail (MATECOMPONENT_IS_PROPERTY_BAG (pb));
	g_return_if_fail (name != NULL);
	g_return_if_fail (type != NULL);
	g_return_if_fail (g_hash_table_lookup (pb->priv->prop_hash, name) == NULL);

	if (flags == 0) { /* Compatibility hack */
		flags = MateComponent_PROPERTY_READABLE |
			MateComponent_PROPERTY_WRITEABLE;
	}
			    
	if (((flags & MateComponent_PROPERTY_READABLE)  && !get_prop) ||
	    ((flags & MateComponent_PROPERTY_WRITEABLE) && !set_prop)) {
		g_warning ("Serious property error, missing get/set fn. "
			   "on %s", name);
		return;
	}

	if (!(flags & MateComponent_PROPERTY_READABLE) && default_value)
		g_warning ("Assigning a default value to a non readable "
			   "property '%s'", name);
	
	prop = g_new0 (MateComponentProperty, 1);

	prop->name           = g_strdup (name);
	prop->idx            = idx;
	prop->type           = type;
	prop->flags          = flags;
	prop->docstring      = g_strdup (docstring);
	prop->doctitle       = g_strdup (doctitle);

	prop->priv = g_new0 (MateComponentPropertyPrivate, 1);
	prop->priv->get_prop = matecomponent_closure_store (get_prop, matecomponent_marshal_ANY__TYPECODE_UINT_EXCEPTION);
	prop->priv->set_prop = matecomponent_closure_store (set_prop, matecomponent_marshal_VOID__BOXED_UINT_BOXED);


	if (default_value)
		prop->default_value = matecomponent_arg_copy (default_value);

	g_hash_table_insert (pb->priv->prop_hash, prop->name, prop);
}

/**
 * matecomponent_property_bag_remove:
 * @pb: the property bag
 * @name: name of property to remove.
 * 
 * removes the property with @name from @b.
 **/
void
matecomponent_property_bag_remove (MateComponentPropertyBag *pb,
			    const char        *name)
{
	gpointer key, value;

	g_return_if_fail (MATECOMPONENT_IS_PROPERTY_BAG (pb));
	g_return_if_fail (pb->priv != NULL);
	g_return_if_fail (pb->priv->prop_hash != NULL);

	if (g_hash_table_lookup_extended (pb->priv->prop_hash,
					  name, &key, &value))
		matecomponent_property_bag_foreach_remove_prop (key, value, NULL);
}

static MateComponent_PropertyFlags
flags_gparam_to_matecomponent (guint flags)
{
	MateComponent_PropertyFlags f = 0;

	if (!(flags & G_PARAM_READABLE))
		f |= MateComponent_PROPERTY_READABLE;

	if (!(flags & G_PARAM_WRITABLE))
		f |= MateComponent_PROPERTY_WRITEABLE;

	return f;
}

#define MATECOMPONENT_GTK_MAP_KEY "MateComponentGtkMapKey"

static GQuark quark_gobject_map = 0;

static void
get_prop (MateComponentPropertyBag *bag,
	  MateComponentArg         *arg,
	  guint              arg_id,
	  CORBA_Environment *ev,
	  gpointer           user_data)
{
	GParamSpec *pspec = user_data;
	GValue      new = { 0, };
	GObject    *obj;

	if (!(obj = g_object_get_qdata (G_OBJECT (bag), quark_gobject_map))) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return;
	}

/*	g_warning ("Get prop ... %d: %s", arg_id, g_arg->name);*/

	g_value_init (&new, G_PARAM_SPEC_VALUE_TYPE (pspec));
	g_object_get_property (obj, pspec->name, &new);

	matecomponent_arg_from_gvalue (arg, &new);

	g_value_unset (&new);
}

static void
set_prop (MateComponentPropertyBag *bag,
	  const MateComponentArg   *arg,
	  guint              arg_id,
	  CORBA_Environment *ev,
	  gpointer           user_data)
{
	GParamSpec *pspec = user_data;
	GValue      new = { 0, };
	GObject    *obj;

	if (!(obj = g_object_get_qdata (G_OBJECT (bag), quark_gobject_map))) {
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
		return;
	}

/*	g_warning ("Set prop ... %d: %s", arg_id, g_arg->name);*/

	g_value_init (&new, G_PARAM_SPEC_VALUE_TYPE (pspec));

	matecomponent_arg_to_gvalue (&new, arg);
	g_object_set_property (obj, pspec->name, &new);

	g_value_unset (&new);
}

#undef MAPPING_DEBUG

/**
 * matecomponent_property_bag_add_gtk_args:
 * @pb: destination property bag
 * @on_instance: the instance to associate the properties with
 * @pspecs: a list of the parameters to map
 * @n_params: the size of the list.
 * 
 * Transfers @params from the @on_instance to the property bag,
 * setting up a mapping between the two objects property systems.
 **/
void
matecomponent_property_bag_map_params (MateComponentPropertyBag *pb,
				GObject           *on_instance,
				const GParamSpec **pspecs,
				guint              n_params)
{
	int          i;

	g_return_if_fail (G_IS_OBJECT (on_instance));
	g_return_if_fail (MATECOMPONENT_IS_PROPERTY_BAG (pb));

	if (!n_params)
		return;
	g_return_if_fail (pspecs != NULL);

	if (g_object_get_qdata (G_OBJECT (pb), quark_gobject_map)) {
		g_warning ("Cannot proxy two GObjects in the same bag yet");
		return;
	}
	g_object_set_qdata (G_OBJECT (pb), quark_gobject_map, on_instance);

	/* Setup types, and names */
	for (i = 0; i < n_params; i++) {
		const GParamSpec    *pspec;
		GType                value_type;
		MateComponent_PropertyFlags flags;
		MateComponentArgType        type;

		pspec = pspecs [i];
		value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

		type = matecomponent_arg_type_from_gtype (value_type);
		if (!type) {
#ifdef MAPPING_DEBUG
			g_warning ("Can't handle type '%s' on arg '%s'",
				   g_type_name (value_type), pspec->name);
#endif
			continue;
		}

		flags = flags_gparam_to_matecomponent (pspec->flags);

		matecomponent_property_bag_add_full
			(pb, 
			 g_param_spec_get_name ((GParamSpec *)pspec), 
			 i, type, NULL,
			 g_param_spec_get_nick ((GParamSpec *)pspec),
			 g_param_spec_get_blurb ((GParamSpec *)pspec), 
			 flags,
			 g_cclosure_new (G_CALLBACK (get_prop), (gpointer) pspec, NULL),
			 g_cclosure_new (G_CALLBACK (set_prop), (gpointer) pspec, NULL));
	}
}

/**
 * matecomponent_property_bag_add:
 * @pb: property bag to add to
 * @name: name of new property
 * @idx: integer index for fast callback switch statement
 * @type: the CORBA type eg. TC_long
 * @default_value: the default value or NULL
 * @doctitle: the translated documentation string
 * @flags: various flags
 * 
 *  Adds a property to the property bag.
 **/
void
matecomponent_property_bag_add (MateComponentPropertyBag   *pb,
			 const char          *name,
			 int                  idx,
			 MateComponentArgType        type,
			 MateComponentArg           *default_value,
			 const char          *doctitle,
			 MateComponent_PropertyFlags flags)
{
	g_return_if_fail (pb != NULL);

	matecomponent_property_bag_add_full (pb, name, idx, type,
				      default_value, doctitle, 
				      NULL, flags,
				      pb->priv->get_prop,
				      pb->priv->set_prop);
}


/* Class/object initialization functions. */

static void
matecomponent_property_bag_class_init (MateComponentPropertyBagClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_PropertyBag__epv *epv = &klass->epv;

	if (!quark_gobject_map)
		quark_gobject_map = g_quark_from_static_string (
			"MateComponentGObjectMap");

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = matecomponent_property_bag_finalize;

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
matecomponent_property_bag_init (MateComponentPropertyBag *pb)
{
	pb->priv = g_new0 (MateComponentPropertyBagPrivate, 1);

	pb->priv->prop_hash = g_hash_table_new (g_str_hash, g_str_equal);
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentPropertyBag, 
		       MateComponent_PropertyBag,
		       PARENT_TYPE,
		       matecomponent_property_bag)
