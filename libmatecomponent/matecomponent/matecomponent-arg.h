/*
 * matecomponent-arg.h MateComponent argument support:
 *
 *  A thin wrapper of CORBA_any's with macros
 * to assist in handling values safely.
 *
 * Author:
 *    Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, Helix Code, Inc.
 */
#ifndef __MATECOMPONENT_ARG_H__
#define __MATECOMPONENT_ARG_H__

#include <matecomponent/MateComponent.h>

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef CORBA_any      MateComponentArg;
typedef CORBA_TypeCode MateComponentArgType;

typedef void (*MateComponentArgToGValueFn)   (MateComponentArg const *arg,
				       GValue          *out_value);

typedef void (*MateComponentArgFromGValueFn) (MateComponentArg       *out_arg,
				       GValue const    *value);


#define MATECOMPONENT_ARG_NULL     TC_null
#define MATECOMPONENT_ARG_BOOLEAN  TC_CORBA_boolean
#define MATECOMPONENT_ARG_SHORT    TC_CORBA_short
#define MATECOMPONENT_ARG_INT      TC_CORBA_long
#define MATECOMPONENT_ARG_LONG     TC_CORBA_long
#define MATECOMPONENT_ARG_LONGLONG TC_CORBA_long_long
#define MATECOMPONENT_ARG_FLOAT    TC_CORBA_float
#define MATECOMPONENT_ARG_DOUBLE   TC_CORBA_double
#define MATECOMPONENT_ARG_CHAR     TC_CORBA_char
#define MATECOMPONENT_ARG_STRING   TC_CORBA_string

#if defined(__GNUC__) && !defined(__STRICT_ANSI__) && !defined(__cplusplus)
#	define MATECOMPONENT_ARG_GET_GENERAL(a,c,t,e)   ({g_assert (matecomponent_arg_type_is_equal ((a)->_type, c, e)); *((t *)((a)->_value)); })
#	define MATECOMPONENT_ARG_SET_GENERAL(a,v,c,t,e) ({g_assert (matecomponent_arg_type_is_equal ((a)->_type, c, e)); *((t *)((a)->_value)) = (t)(v); })
#else
#	define MATECOMPONENT_ARG_GET_GENERAL(a,c,t,e)   (*((t *)((a)->_value)))
#	define MATECOMPONENT_ARG_SET_GENERAL(a,v,c,t,e) (*((t *)((a)->_value)) = (v))
#endif

#define MATECOMPONENT_ARG_GET_BOOLEAN(a)   (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_boolean, CORBA_boolean, NULL))
#define MATECOMPONENT_ARG_SET_BOOLEAN(a,v) (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_boolean, CORBA_boolean, NULL))

#define MATECOMPONENT_ARG_GET_SHORT(a)     (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_short, CORBA_short, NULL))
#define MATECOMPONENT_ARG_SET_SHORT(a,v)   (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_short, CORBA_short, NULL))
#define MATECOMPONENT_ARG_GET_INT(a)       (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_long, CORBA_long, NULL))
#define MATECOMPONENT_ARG_SET_INT(a,v)     (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_long, CORBA_long, NULL))
#define MATECOMPONENT_ARG_GET_LONG(a)      (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_long, CORBA_long, NULL))
#define MATECOMPONENT_ARG_SET_LONG(a,v)    (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_long, CORBA_long, NULL))
#define MATECOMPONENT_ARG_GET_ULONG(a)     (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_unsigned_long, CORBA_unsigned_long, NULL))
#define MATECOMPONENT_ARG_SET_ULONG(a,v)   (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_unsigned_long, CORBA_unsigned_long, NULL))
#define MATECOMPONENT_ARG_GET_LONGLONG(a)  (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_long_long, CORBA_long_long, NULL))
#define MATECOMPONENT_ARG_SET_LONGLONG(a,v) (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_long_long, CORBA_long_long, NULL))

#define MATECOMPONENT_ARG_GET_FLOAT(a)     (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_float, CORBA_float, NULL))
#define MATECOMPONENT_ARG_SET_FLOAT(a,v)   (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_float, CORBA_float, NULL))

#define MATECOMPONENT_ARG_GET_DOUBLE(a)    (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_double, CORBA_double, NULL))
#define MATECOMPONENT_ARG_SET_DOUBLE(a,v)  (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_double, CORBA_double, NULL))

#define MATECOMPONENT_ARG_GET_CHAR(a)      (MATECOMPONENT_ARG_GET_GENERAL (a, TC_CORBA_char, CORBA_char, NULL))
#define MATECOMPONENT_ARG_SET_CHAR(a,v)    (MATECOMPONENT_ARG_SET_GENERAL (a, v, TC_CORBA_char, CORBA_char, NULL))

#if defined(__GNUC__) && !defined(__STRICT_ANSI__) && !defined(__cplusplus)
#define MATECOMPONENT_ARG_GET_STRING(a)    ({g_assert ((a)->_type->kind == CORBA_tk_string); *((CORBA_char **)((a)->_value)); })
#define MATECOMPONENT_ARG_SET_STRING(a,v)  ({g_assert ((a)->_type->kind == CORBA_tk_string); CORBA_free (*(char **)(a)->_value); *((CORBA_char **)((a)->_value)) = CORBA_string_dup ((v)?(v):""); })
#else
#define MATECOMPONENT_ARG_GET_STRING(a)    (*((CORBA_char **)((a)->_value)))
#define MATECOMPONENT_ARG_SET_STRING(a,v)  {CORBA_free (*(char **)(a)->_value); *((CORBA_char **)((a)->_value)) = CORBA_string_dup ((v)?(v):""); }
#endif

MateComponentArg    *matecomponent_arg_new             (MateComponentArgType      t);

MateComponentArg    *matecomponent_arg_new_from        (MateComponentArgType      t,
					  gconstpointer      data);

void          matecomponent_arg_release         (MateComponentArg         *arg);

MateComponentArg    *matecomponent_arg_copy            (const MateComponentArg   *arg);

void          matecomponent_arg_from_gvalue     (MateComponentArg         *a,
					   const GValue      *value);
MateComponentArgType matecomponent_arg_type_from_gtype (GType              t);

void          matecomponent_arg_to_gvalue       (GValue            *value,
					  const MateComponentArg   *arg);

GType         matecomponent_arg_type_to_gtype   (MateComponentArgType      id);

gboolean      matecomponent_arg_is_equal        (const MateComponentArg   *a,
					  const MateComponentArg   *b,
					  CORBA_Environment *opt_ev);

gboolean      matecomponent_arg_type_is_equal   (MateComponentArgType      a,
					  MateComponentArgType      b,
					  CORBA_Environment *opt_ev);

gboolean matecomponent_arg_to_gvalue_alloc                (MateComponentArg const       *arg,
						    GValue                *value);
gboolean matecomponent_arg_from_gvalue_alloc              (MateComponentArg             *arg,
						    GValue const          *value);
void     matecomponent_arg_register_to_gvalue_converter   (MateComponentArgType          arg_type,
						    MateComponentArgToGValueFn    converter);
void     matecomponent_arg_register_from_gvalue_converter (GType                  gtype,
						    MateComponentArgFromGValueFn  converter);
  /* private */
void     matecomponent_arg_init                           (void);


#ifdef __cplusplus
}
#endif

#endif /* ! __MATECOMPONENT_ARG_H__ */
