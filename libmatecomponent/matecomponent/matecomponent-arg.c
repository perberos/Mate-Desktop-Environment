/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-arg.c MateComponent argument support:
 *
 *  A thin wrapper of CORBA_any's with macros
 * to assist in handling values safely.
 *
 * Author:
 *    Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, Ximian., Inc.
 */
#include <config.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-arg.h>
#include "matecomponent-types.h"


/* Key: CORBA_TypeCode; Value: MateComponentArgToGValueFn. */
static GHashTable *matecomponent_arg_to_gvalue_mapping   = NULL;
/* Key: GType; Value: MateComponentArgFromGValueFn */
static GHashTable *matecomponent_arg_from_gvalue_mapping = NULL;


/**
 * matecomponent_arg_new:
 * @t: the MateComponentArgType eg. TC_CORBA_long
 * 
 * Create a new MateComponentArg with the specified type
 * the value of the MateComponentArg is initially empty.
 * 
 * Returns: the new #MateComponentArg
 **/
MateComponentArg *
matecomponent_arg_new (MateComponentArgType t)
{
	CORBA_any *any = CORBA_any__alloc ();
	any->_release = TRUE;
	any->_type = (CORBA_TypeCode) CORBA_Object_duplicate ((CORBA_Object) t, NULL);
	any->_value = MateCORBA_small_alloc (t);
	return any;
}

/**
 * matecomponent_arg_new_from
 * @t: the MateComponentArgType eg. TC_CORBA_long
 * @data: the data for the MateComponentArg to be created
 *
 * Create a new MateComponentArg with the specified type and data
 *
 * Returns: the new #MateComponentArg
 */
MateComponentArg * 
matecomponent_arg_new_from (MateComponentArgType t, gconstpointer data)
{
	MateComponentArg *arg;

	arg = CORBA_any__alloc ();
	arg->_release = TRUE;
	arg->_type = (CORBA_TypeCode) CORBA_Object_duplicate ((CORBA_Object) t, NULL);
	arg->_value = MateCORBA_copy_value (data, t);

	return arg;
}

/**
 * matecomponent_arg_release:
 * @arg: the matecomponent arg.
 * 
 * This frees the memory associated with @arg
 **/
void
matecomponent_arg_release (MateComponentArg *arg)
{
	if (arg)
		CORBA_free (arg);	
}

/**
 * matecomponent_arg_copy:
 * @arg: the matecomponent arg
 * 
 * This function duplicates @a by a deep copy
 * 
 * Returns: a copy of @arg
 **/
MateComponentArg *
matecomponent_arg_copy (const MateComponentArg *arg)
{
	MateComponentArg *copy = CORBA_any__alloc ();

	if (!arg) {
		copy->_type = TC_null;
		g_warning ("Duplicating a NULL MateComponent Arg");
	} else
		CORBA_any__copy (copy, arg);

	return copy;
}

#undef MAPPING_DEBUG

/**
 * matecomponent_arg_type_from_gruntime:
 * @id: the GType id.
 * 
 * This maps a GType to a MateComponentArgType
 * 
 * Return value: the mapped type or %NULL on failure.
 **/
MateComponentArgType
matecomponent_arg_type_from_gtype (GType id)
{
	switch (id) {
	case G_TYPE_CHAR:   return TC_CORBA_char;
	case G_TYPE_UCHAR:  return TC_CORBA_char;
	case G_TYPE_BOOLEAN:return TC_CORBA_boolean;
	case G_TYPE_INT:    return TC_CORBA_short;
	case G_TYPE_UINT:   return TC_CORBA_unsigned_short;
	case G_TYPE_LONG:   return TC_CORBA_long;
	case G_TYPE_ULONG:  return TC_CORBA_unsigned_long;
	case G_TYPE_FLOAT:  return TC_CORBA_float;
	case G_TYPE_DOUBLE: return TC_CORBA_double;
	case G_TYPE_STRING: return TC_CORBA_string;
	default:
#ifdef MAPPING_DEBUG
		g_warning ("Unmapped arg type '%d'", id);
#endif
		break;
	}

	return NULL;
}

/**
 * matecomponent_arg_type_to_gtype:
 * @id: the MateComponentArgType
 * 
 * This maps a MateComponentArgType to a GType
 * 
 * Return value: the mapped type or %0 on failure
 **/
GType
matecomponent_arg_type_to_gtype (MateComponentArgType id)
{
	CORBA_Environment ev;
	GType g_type = G_TYPE_NONE;

	CORBA_exception_init (&ev);

	if (matecomponent_arg_type_is_equal (TC_CORBA_char, id, &ev))
		g_type = G_TYPE_CHAR;
	else if (matecomponent_arg_type_is_equal (TC_CORBA_boolean, id, &ev))
		g_type = G_TYPE_BOOLEAN;
	else if (matecomponent_arg_type_is_equal (TC_CORBA_short,   id, &ev))
		g_type = G_TYPE_INT;
	else if (matecomponent_arg_type_is_equal (TC_CORBA_unsigned_short, id, &ev))
		g_type = G_TYPE_UINT;
	else if (matecomponent_arg_type_is_equal (TC_CORBA_long,    id, &ev))
		g_type = G_TYPE_LONG;
	else if (matecomponent_arg_type_is_equal (TC_CORBA_unsigned_long, id, &ev))
		g_type = G_TYPE_ULONG;
	else if (matecomponent_arg_type_is_equal (TC_CORBA_float,   id, &ev))
		g_type = G_TYPE_FLOAT;
	else if (matecomponent_arg_type_is_equal (TC_CORBA_double,  id, &ev))
		g_type = G_TYPE_DOUBLE;
	else if (matecomponent_arg_type_is_equal (TC_CORBA_string,  id, &ev))
		g_type = G_TYPE_STRING;
#ifdef MAPPING_DEBUG
	else
		g_warning ("Unmapped matecomponent arg type");
#endif

	CORBA_exception_free (&ev);

	return g_type;
}

#define MAKE_FROM_GVALUE(gtype,gtypename,tcid,unionid,corbatype,corbakind)	\
	case G_TYPE_##gtype:							\
		*((corbatype *)a->_value) = (corbatype)g_value_get_##gtypename (value);	\
		break;

/**
 * matecomponent_arg_from_gvalue:
 * @a: pointer to blank MateComponentArg
 * @value: #GValue to copy
 * 
 * This maps a GValue @value to a MateComponentArg @a;
 * @a must point to a freshly allocated MateComponentArg
 * eg. such as returned by matecomponent_arg_new
 **/
void
matecomponent_arg_from_gvalue (MateComponentArg *a, const GValue *value)
{
	int        id;

	g_return_if_fail (a != NULL);
	g_return_if_fail (value != NULL);

	id = G_VALUE_TYPE (value);

	switch (id) {

	case G_TYPE_INVALID:
	case G_TYPE_NONE:
		g_warning ("Strange GValue type %s", g_type_name (id));
		break;
		
		MAKE_FROM_GVALUE (CHAR,    char,    TC_CORBA_char,     char_data, CORBA_char,           CORBA_tk_char);
		MAKE_FROM_GVALUE (UCHAR,   uchar,   TC_CORBA_char,    uchar_data, CORBA_char,           CORBA_tk_char);
		MAKE_FROM_GVALUE (BOOLEAN, boolean, TC_CORBA_boolean,  bool_data, CORBA_boolean,        CORBA_tk_boolean);
		MAKE_FROM_GVALUE (INT,     int,     TC_CORBA_short,     int_data, CORBA_short,          CORBA_tk_short);
		MAKE_FROM_GVALUE (UINT,    uint,    TC_CORBA_unsigned_short,   uint_data, CORBA_unsigned_short, CORBA_tk_ushort);
		MAKE_FROM_GVALUE (LONG,    long,    TC_CORBA_long,     long_data, CORBA_long,           CORBA_tk_long);
		MAKE_FROM_GVALUE (ULONG,   ulong,   TC_CORBA_unsigned_long,   ulong_data, CORBA_unsigned_long,  CORBA_tk_ulong);
		MAKE_FROM_GVALUE (FLOAT,   float,   TC_CORBA_float,   float_data, CORBA_float,          CORBA_tk_float);
		MAKE_FROM_GVALUE (DOUBLE,  double,  TC_CORBA_double, double_data, CORBA_double,         CORBA_tk_double);

	case G_TYPE_STRING:
		if (G_VALUE_HOLDS_STRING (value))
			*((CORBA_char **)a->_value) = CORBA_string_dup (g_value_get_string (value));
		else
			*((CORBA_char **)a->_value) = CORBA_string_dup ("");
		break;

	case G_TYPE_POINTER:
		g_warning ("We can map user data callbacks locally");
		break;

	case G_TYPE_OBJECT:
		g_warning ("All objects can be mapped to base gtk types"
			   "in due course");
		break;

	case G_TYPE_ENUM:
	case G_TYPE_FLAGS:
	case G_TYPE_BOXED:
	default:
		g_warning ("Unmapped GValue type %d", id);
		break;
	}
}

#define MAKE_TO_GVALUE(gtype,gtypename,tcid,unionid,corbatype,corbakind)	\
	case corbakind:								\
		g_value_set_##gtypename (value, *((corbatype *)arg->_value));	\
		break;

/**
 * matecomponent_arg_to_gvalue:
 * @value: pointer to a blank #GValue
 * @arg: the MateComponentArg to copy
 * 
 * Maps a MateComponentArg to a GtkArg; @a must point
 * to a blank GtkArg.
 **/
void
matecomponent_arg_to_gvalue (GValue *value, const MateComponentArg *arg)
{
	int     id;

	g_return_if_fail (value != NULL);
	g_return_if_fail (arg != NULL);
	g_return_if_fail (arg->_type != NULL);

	id = arg->_type->kind;
	switch (id) {

	case CORBA_tk_null:
	case CORBA_tk_void:
		g_warning ("Strange corba arg type %d", id);
		break;
		
		MAKE_TO_GVALUE (CHAR,    char,    TC_CORBA_char,     char_data, CORBA_char,           CORBA_tk_char);
/*		MAKE_TO_GVALUE (UCHAR,   uchar,   TC_CORBA_char,    uchar_data, CORBA_char,           CORBA_tk_char);*/
		MAKE_TO_GVALUE (BOOLEAN, boolean, TC_CORBA_boolean,  bool_data, CORBA_boolean,        CORBA_tk_boolean);
		MAKE_TO_GVALUE (INT,     int,     TC_CORBA_short,     int_data, CORBA_short,          CORBA_tk_short);
		MAKE_TO_GVALUE (UINT,    uint,    TC_CORBA_ushort,   uint_data, CORBA_unsigned_short, CORBA_tk_ushort);
		MAKE_TO_GVALUE (LONG,    long,    TC_CORBA_long,     long_data, CORBA_long,           CORBA_tk_long);
		MAKE_TO_GVALUE (ULONG,   ulong,   TC_CORBA_ulong,   ulong_data, CORBA_unsigned_long,  CORBA_tk_ulong);
		MAKE_TO_GVALUE (FLOAT,   float,   TC_CORBA_float,   float_data, CORBA_float,          CORBA_tk_float);
		MAKE_TO_GVALUE (DOUBLE,  double,  TC_CORBA_double, double_data, CORBA_double,         CORBA_tk_double);

	case CORBA_tk_string:
		g_value_set_string (value, MATECOMPONENT_ARG_GET_STRING (arg));
		break;

	case CORBA_tk_objref:
		g_warning ("All objects can be mapped to base gtk types"
			   "in due course");
		break;

	case CORBA_tk_sequence:
	case CORBA_tk_alias:
	case CORBA_tk_enum:
	case CORBA_tk_array:
	case CORBA_tk_union:
		g_warning ("Clever things can be done for these");
		break;

	default:
		g_warning ("Unmapped corba arg type %d", id);
		break;
	}
}

/**
 * matecomponent_arg_type_is_equal:
 * @a: a type code
 * @b: a type code
 * @opt_ev: optional exception environment or NULL.
 * 
 * This compares two #MateComponentArgType's in @a and @b.
 * The @opt_ev is an optional #CORBA_Environment for
 * exceptions, or %NULL. This function is commutative.
 * 
 * Return value: %TRUE if equal, %FALSE if different
 **/
gboolean
matecomponent_arg_type_is_equal (MateComponentArgType a, MateComponentArgType b, CORBA_Environment *opt_ev)
{
	CORBA_Environment ev, *my_ev;
	gboolean retval;

	if (!opt_ev) {
		CORBA_exception_init (&ev);
		my_ev = &ev;
	} else
		my_ev = opt_ev;

	retval = CORBA_TypeCode_equal (a, b, my_ev);

	if (!opt_ev)
		CORBA_exception_free (&ev);

	return retval;
}

/**
 * matecomponent_arg_is_equal:
 * @a: a matecomponent arg
 * @b: another matecomponent arg
 * @opt_ev: optional exception environment or %NULL.
 * 
 * Compares two #MateComponentArg's for equivalence; will return %TRUE
 * if equivalent for all simple cases. For Object references
 * CORBA sometimes denies 2 object references are equivalent
 * even if they are [ this is a feature_not_bug ].
 *
 * This function is commutative.
 * 
 * Return value: %TRUE if @a == @b
 **/
gboolean
matecomponent_arg_is_equal (const MateComponentArg *a, const MateComponentArg *b, CORBA_Environment *opt_ev)
{
	CORBA_Environment ev, *my_ev;
	gboolean retval;

	if (!opt_ev) {

		CORBA_exception_init (&ev);
		my_ev = &ev;
	} else
		my_ev = opt_ev;

	retval = MateCORBA_any_equivalent ((CORBA_any *) a, (CORBA_any *) b, my_ev);

	if (!opt_ev)
		CORBA_exception_free (&ev);

	return retval;
}


/**
 * matecomponent_arg_to_gvalue_alloc:
 * @arg: source value
 * @value: destination value
 * 
 * Converts a #MateComponentArg @arg into a #GValue.  Unlike
 * matecomponent_arg_to_gvalue(), the destination #GValue does not need to --
 * and should not -- be initialized.
 * 
 * Return value: Returns %TRUE if conversion succeeds, %FALSE otherwise.
 **/
gboolean
matecomponent_arg_to_gvalue_alloc (MateComponentArg const *arg, GValue *value)
{
	MateComponentArgToGValueFn converter;

	g_assert (matecomponent_arg_from_gvalue_mapping);

#define TO_GVALUE_CASE(gtypename, matecomponentargname, typename, typecode)			\
	if (CORBA_TypeCode_equal(arg->_type, typecode, NULL)) {				\
		g_value_init (value, G_TYPE_##gtypename);				\
		g_value_set_##typename (value, MATECOMPONENT_ARG_GET_##matecomponentargname(arg));	\
		return TRUE;								\
	}

	TO_GVALUE_CASE (STRING,  STRING,  string,  TC_CORBA_string);
	TO_GVALUE_CASE (CHAR,    CHAR,    char,    TC_CORBA_char);
	TO_GVALUE_CASE (BOOLEAN, BOOLEAN, boolean, TC_CORBA_boolean);
	TO_GVALUE_CASE (LONG,    LONG,    long,    TC_CORBA_long);
	TO_GVALUE_CASE (ULONG,   ULONG,   ulong,   TC_CORBA_unsigned_long);
	TO_GVALUE_CASE (FLOAT,   FLOAT,   float,   TC_CORBA_float);
	TO_GVALUE_CASE (DOUBLE,  DOUBLE,  double,  TC_CORBA_double);
	
	converter = g_hash_table_lookup (matecomponent_arg_to_gvalue_mapping,
					 arg->_type);
	if (converter)
	    converter (arg, value);
	else
	    return FALSE;
	return TRUE;
}


/**
 * matecomponent_arg_to_gvalue_alloc:
 * @arg: destination value
 * @value: source value
 * 
 * Converts a #GValue into a #MateComponentArg.  Unlike
 * matecomponent_arg_from_gvalue(), the destination #MateComponentArg does not need
 * to -- and should not -- be initialized.
 * 
 * Return value: Returns %TRUE if conversion succeeds, %FALSE otherwise.
 **/
gboolean
matecomponent_arg_from_gvalue_alloc (MateComponentArg *arg, GValue const *value)
{
	MateComponentArgFromGValueFn converter;

	g_return_val_if_fail (arg, FALSE);
	g_return_val_if_fail (value, FALSE);
	g_assert (matecomponent_arg_from_gvalue_mapping);

#define FROM_GVALUE_CASE(gtype, gtypename, tcid, corbatype)				\
case G_TYPE_##gtype:									\
	arg->_type = tcid;								\
	arg->_value = MateCORBA_alloc_tcval (tcid, 1);					\
	*((corbatype *)arg->_value) = (corbatype) g_value_get_##gtypename (value);	\
	arg->_release = CORBA_TRUE;							\
	return TRUE;
	
	switch (G_VALUE_TYPE (value))
	{
		FROM_GVALUE_CASE (CHAR,    char,    TC_CORBA_char,          CORBA_char);
		FROM_GVALUE_CASE (UCHAR,   uchar,   TC_CORBA_char,          CORBA_char);
		FROM_GVALUE_CASE (BOOLEAN, boolean, TC_CORBA_boolean,       CORBA_boolean);
		FROM_GVALUE_CASE (INT,     int,     TC_CORBA_long,          CORBA_long);
		FROM_GVALUE_CASE (UINT,    uint,    TC_CORBA_unsigned_long, CORBA_unsigned_long);
		FROM_GVALUE_CASE (LONG,    long,    TC_CORBA_long,          CORBA_long);
		FROM_GVALUE_CASE (ULONG,   ulong,   TC_CORBA_unsigned_long, CORBA_unsigned_long);
		FROM_GVALUE_CASE (FLOAT,   float,   TC_CORBA_float,         CORBA_float);
		FROM_GVALUE_CASE (DOUBLE,  double,  TC_CORBA_double,        CORBA_double);
#undef FROM_GVALUE_FN

	case G_TYPE_STRING: {
		const char *str = g_value_get_string (value);
		arg->_type = TC_CORBA_string;
		arg->_value = MateCORBA_alloc_tcval (TC_CORBA_string, 1);
		if (str) {
			*((CORBA_char **)arg->_value) =	CORBA_string_dup (str);
			arg->_release = CORBA_TRUE;
		} else {
			*((CORBA_char **)arg->_value) = "";
			arg->_release = CORBA_FALSE;
		}
		return TRUE;
	}
	}
	  /* default: try to lookup a converter function */
	converter = g_hash_table_lookup (matecomponent_arg_from_gvalue_mapping,
					 GUINT_TO_POINTER (G_VALUE_TYPE (value)));
	if (converter)
		converter (arg, value);
	else
		return FALSE;
	return TRUE;
}


/* GValue => MateComponentArg converters */

static void
__matecomponent_arg_from_CORBA_ANY (MateComponentArg    *out_arg,
			     GValue const *value)
{
	out_arg->_type    = TC_CORBA_any;
	out_arg->_value   = matecomponent_value_get_corba_any (value);
	out_arg->_release = CORBA_TRUE;
}

static void
__TC_CORBA_any_to_gvalue (MateComponentArg const *arg,
			  GValue          *out_value)
{
	g_value_init (out_value, MATECOMPONENT_TYPE_CORBA_ANY);
	matecomponent_value_set_corba_any (out_value, arg->_value);
}


void
matecomponent_arg_register_from_gvalue_converter (GType                 gtype,
					   MateComponentArgFromGValueFn converter)
{
	g_return_if_fail (matecomponent_arg_from_gvalue_mapping != NULL);
	g_hash_table_insert (matecomponent_arg_from_gvalue_mapping,
			     GUINT_TO_POINTER (gtype),
			     converter);
}

void
matecomponent_arg_register_to_gvalue_converter (MateComponentArgType       arg_type,
					 MateComponentArgToGValueFn converter)
{
	g_return_if_fail (matecomponent_arg_to_gvalue_mapping != NULL);
	g_hash_table_insert (matecomponent_arg_to_gvalue_mapping,
			     arg_type, converter);
}

void matecomponent_arg_init (void)
{
	g_assert (matecomponent_arg_to_gvalue_mapping == NULL);
	g_assert (matecomponent_arg_from_gvalue_mapping == NULL);

	matecomponent_arg_to_gvalue_mapping   = g_hash_table_new (g_direct_hash, g_direct_equal);
	matecomponent_arg_from_gvalue_mapping = g_hash_table_new (g_direct_hash, g_direct_equal);

	matecomponent_arg_register_from_gvalue_converter 
		(MATECOMPONENT_TYPE_CORBA_ANY, __matecomponent_arg_from_CORBA_ANY);
	matecomponent_arg_register_to_gvalue_converter
		(TC_CORBA_any, __TC_CORBA_any_to_gvalue);
}

