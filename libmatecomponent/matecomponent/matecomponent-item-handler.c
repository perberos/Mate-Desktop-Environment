/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-item-handler.c: a generic Item Container resolver (implements ItemContainer)
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 2000 Miguel de Icaza.
 */
#include <config.h>
#include <glib-object.h>
#include <gobject/gmarshal.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-exception.h>

#include <matecomponent/matecomponent-types.h>
#include <matecomponent/matecomponent-marshal.h>

#include "matecomponent-item-handler.h"

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

static GObjectClass *matecomponent_item_handler_parent_class;

struct _MateComponentItemHandlerPrivate
{
	GClosure *enum_objects;
	GClosure *get_object;
};

static void
matecomponent_marshal_POINTER__DUMMY_BOXED (GClosure     *closure,
				     GValue       *return_value,
				     guint         n_param_values,
				     const GValue *param_values,
				     gpointer      invocation_hint,
				     gpointer      marshal_data)
{
	typedef gpointer (*GMarshalFunc_POINTER__POINTER_BOXED) (gpointer     data1,
								 gpointer     arg_1,
								 gpointer     arg_2,
								 gpointer     data2);
	register GMarshalFunc_POINTER__POINTER_BOXED callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;
	gpointer v_return;

	g_return_if_fail (return_value != NULL);
	g_return_if_fail (n_param_values == 2);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_POINTER__POINTER_BOXED) (marshal_data ? marshal_data : cc->callback);

	v_return = callback (data1,
			     data2,
			     g_value_get_boxed (param_values + 1),
			     data2);

	g_value_set_pointer (return_value, v_return);
}

static void
matecomponent_marshal_BOXED__STRING_BOOLEAN_DUMMY_BOXED (GClosure     *closure,
						  GValue       *return_value,
						  guint         n_param_values,
						  const GValue *param_values,
						  gpointer      invocation_hint,
						  gpointer      marshal_data)
{
	typedef gpointer (*GMarshalFunc_BOXED__STRING_BOOLEAN_POINTER_BOXED) (gpointer     data1,
									      gpointer     arg_1,
									      gboolean     arg_2,
									      gpointer     arg_3,
									      gpointer     arg_4,
									      gpointer     data2);
	register GMarshalFunc_BOXED__STRING_BOOLEAN_POINTER_BOXED callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;
	gpointer v_return;

	g_return_if_fail (return_value != NULL);
	g_return_if_fail (n_param_values == 4);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_BOXED__STRING_BOOLEAN_POINTER_BOXED) (marshal_data ? marshal_data : cc->callback);

	v_return = callback (data1,
			     (char*) g_value_get_string (param_values + 1),
			     g_value_get_boolean (param_values + 2),
			     data2,
			     g_value_get_boxed (param_values + 3),
			     data2);

	g_value_take_boxed (return_value, v_return);
}

/*
 * Returns a list of the objects in this container
 */
static MateComponent_ItemContainer_ObjectNames *
impl_enum_objects (PortableServer_Servant servant, CORBA_Environment *ev)
{
	MateComponentObject *object = matecomponent_object_from_servant (servant);
	MateComponentItemHandler *handler = MATECOMPONENT_ITEM_HANDLER (object);

	if (handler->priv->enum_objects)
	{
		MateComponent_ItemContainer_ObjectNames *ret;

		matecomponent_closure_invoke (handler->priv->enum_objects,
				       G_TYPE_POINTER,                    &ret,
				       MATECOMPONENT_TYPE_ITEM_HANDLER,           handler,
				       MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION, ev,
				       G_TYPE_INVALID);

		return ret;
	} else
		return MateComponent_ItemContainer_ObjectNames__alloc ();
}

static MateComponent_Unknown
impl_get_object (PortableServer_Servant servant,
		 const CORBA_char      *item_name,
		 CORBA_boolean          only_if_exists,
		 CORBA_Environment     *ev)
{
	MateComponentObject *object = matecomponent_object_from_servant (servant);
	MateComponentItemHandler *handler = MATECOMPONENT_ITEM_HANDLER (object);

	if (handler->priv->get_object) {
		MateComponent_Unknown ret;

		matecomponent_closure_invoke (handler->priv->get_object,
				       MATECOMPONENT_TYPE_STATIC_UNKNOWN,         &ret,
				       MATECOMPONENT_TYPE_ITEM_HANDLER,           handler,
				       G_TYPE_STRING,                      item_name,
				       G_TYPE_BOOLEAN,                     only_if_exists,
				       MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION, ev,
				       G_TYPE_INVALID);
				       
		return ret;
	} else
		return CORBA_OBJECT_NIL;
}

static void
matecomponent_item_handler_finalize (GObject *object)
{
	MateComponentItemHandler *handler = MATECOMPONENT_ITEM_HANDLER (object);

	if (handler->priv) {
		if (handler->priv->enum_objects)
			g_closure_unref (handler->priv->enum_objects);

		if (handler->priv->get_object)
			g_closure_unref (handler->priv->get_object);

		g_free (handler->priv);
		handler->priv = NULL;
	}

	matecomponent_item_handler_parent_class->finalize (object);

}

static void 
matecomponent_item_handler_init (GObject *object)
{
	MateComponentItemHandler *handler = MATECOMPONENT_ITEM_HANDLER (object);

	handler->priv = g_new0 (MateComponentItemHandlerPrivate, 1);
}


static void
matecomponent_item_handler_class_init (MateComponentItemHandlerClass *klass)
{
	POA_MateComponent_ItemContainer__epv *epv = &klass->epv;

	matecomponent_item_handler_parent_class = g_type_class_peek_parent (klass);

	G_OBJECT_CLASS (klass)->finalize = matecomponent_item_handler_finalize;

	epv->enumObjects     = impl_enum_objects;
	epv->getObjectByName = impl_get_object;

}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentItemHandler, 
		       MateComponent_ItemContainer,
		       PARENT_TYPE,
		       matecomponent_item_handler)

/**
 * matecomponent_item_handler_construct:
 * @handler: The handler object to construct
 * @enum_objects: The closure implementing enumObjects
 * @get_object: The closure implementing getObject
 *
 * Constructs the @container MateComponentObject using the provided closures
 * for the actual implementation.
 *
 * Returns: The constructed MateComponentItemContainer object.
 */
MateComponentItemHandler *
matecomponent_item_handler_construct (MateComponentItemHandler *handler,
			       GClosure          *enum_objects,
			       GClosure          *get_object)
{
	g_return_val_if_fail (handler != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_ITEM_HANDLER (handler), NULL);

	if (enum_objects)
		handler->priv->enum_objects = matecomponent_closure_store
			(enum_objects, matecomponent_marshal_POINTER__DUMMY_BOXED);
	if (get_object)
		handler->priv->get_object = matecomponent_closure_store
			(get_object, matecomponent_marshal_BOXED__STRING_BOOLEAN_DUMMY_BOXED);
	
	return handler;
}

/**
 * matecomponent_item_handler_new:
 * @enum_objects: callback invoked for MateComponent::ItemContainer::enum_objects
 * @get_object: callback invoked for MateComponent::ItemContainer::get_objects
 * @user_data: extra data passed on the callbacks
 *
 * Creates a new MateComponentItemHandler object.  These are used to hold
 * client sites.
 *
 * Returns: The newly created MateComponentItemHandler object
 */
MateComponentItemHandler *
matecomponent_item_handler_new (MateComponentItemHandlerEnumObjectsFn enum_objects,
			 MateComponentItemHandlerGetObjectFn   get_object,
			 gpointer                       user_data)
{
	GClosure *enum_objects_closure = NULL;
	GClosure *get_object_closure = NULL;

	if (enum_objects)
		enum_objects_closure =
			g_cclosure_new (G_CALLBACK (enum_objects), user_data, NULL);

	if (get_object)
		get_object_closure =
			g_cclosure_new (G_CALLBACK (get_object), user_data, NULL);

	return matecomponent_item_handler_new_closure (enum_objects_closure, get_object_closure);
}

/**
 * matecomponent_item_handler_new_closure:
 * @enum_objects: closure invoked for MateComponent::ItemContainer::enum_objects
 * @get_object: closure invoked for MateComponent::ItemContainer::get_objects
 *
 * Creates a new MateComponentItemHandler object.  These are used to hold
 * client sites.
 *
 * Returns: The newly created MateComponentItemHandler object
 */
MateComponentItemHandler *
matecomponent_item_handler_new_closure (GClosure *enum_objects,
				 GClosure *get_object)
{
	MateComponentItemHandler *handler;

	handler = g_object_new (matecomponent_item_handler_get_type (), NULL);

	return matecomponent_item_handler_construct (handler, enum_objects, get_object);
}

static GSList *
matecomponent_item_option_new_append (GSList  *option_list,
			       GString *key,
			       GString *value)
{
	MateComponentItemOption *option;

	g_assert (key && key->str);

	option = g_new0 (MateComponentItemOption, 1);

	option->key  = key->str;
	g_string_free (key, FALSE);

	if (value) {
		option->value = value->str;
		g_string_free (value, FALSE);
	}

	return g_slist_append (option_list, option);
}

/**
 * matecomponent_parse_item_options:
 * @option_string: a string with a list of options
 *
 * The matecomponent_parse_item_options() routine parses the
 * @option_string which is a semi-colon separated list
 * of arguments.
 *
 * Each argument is of the form value[=key].  The entire
 * option string is defined by:
 *
 * option_string := keydef
 *                | keydef ; option_string
 *
 * keydef := value [=key]
 *
 * The key can be literal values, values with spaces, and the
 * \ character is used as an escape sequence.  To include a
 * literal ";" in a value you can use \;.
 *
 * Returns: A GSList that contains structures of type MateComponentItemOption
 * each MateComponentItemOption
 */
GSList *
matecomponent_item_option_parse (const char *option_string)
{
	GSList     *list  = NULL;
	GString    *key   = NULL;
	GString    *value = NULL;
	const char *p;
	
	for (p = option_string; *p; p++)
		switch (*p) {
		case '=':
			if (!key || value)
				goto parse_error_free;

			value = g_string_new (NULL);
			break;
		case ';':
			if (!key)
				break;

			list = matecomponent_item_option_new_append (list, key, value);
			key = NULL; value = NULL;
			break;
		case '\\':
			if (!key || !*++p)
				goto parse_error_free;

			/* drop through */
		default:
			if (!key)
				key = g_string_new (NULL);

			if (value)
				g_string_append_c (value, *p);
			else
				g_string_append_c (key, *p);
			break;
		}

	if (key)
		list = matecomponent_item_option_new_append (list, key, value);

	return list;

 parse_error_free:
	if (key)
		g_string_free (key, TRUE);

	if (value)
		g_string_free (value, TRUE);

	return list;
}

/** 
 * matecomponent_item_options_free:
 * @options: a GSList of MateComponentItemOption structures that was returned by matecomponent_item_option_parse()
 *
 * Use this to release a list returned by matecomponent_item_option_parse()
 */
void
matecomponent_item_options_free (GSList *options)
{
	GSList *l;

	for (l = options; l; l = l->next) {
		MateComponentItemOption *option = l->data;

		g_free (option->key);
		g_free (option->value);

		g_free (option);
	}

	g_slist_free (options);
}
