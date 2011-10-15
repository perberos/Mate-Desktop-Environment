/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GRuntime types for CORBA Objects.
 *
 * Authors:
 *   Martin Baulig (baulig@suse.de)
 *
 * Copyright 2001 SuSE Linux AG.
 */
#ifndef _MATECOMPONENT_TYPES_H_
#define _MATECOMPONENT_TYPES_H_

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-arg.h>

#ifdef __cplusplus
extern "C" {
#endif

GType matecomponent_corba_object_type_register_static      (const gchar           *name,
                                                     const CORBA_TypeCode   tc,
						     gboolean               is_matecomponent_unknown) G_GNUC_CONST;

GType matecomponent_unknown_get_type                       (void) G_GNUC_CONST;
GType matecomponent_corba_any_get_type                     (void) G_GNUC_CONST;
GType matecomponent_corba_object_get_type                  (void) G_GNUC_CONST;
GType matecomponent_corba_typecode_get_type                (void) G_GNUC_CONST;
GType matecomponent_corba_exception_get_type               (void) G_GNUC_CONST;

#define MATECOMPONENT_TYPE_UNKNOWN                         (matecomponent_unknown_get_type ())
#define MATECOMPONENT_TYPE_CORBA_ANY                       (matecomponent_corba_any_get_type ())
#define MATECOMPONENT_TYPE_CORBA_OBJECT                    (matecomponent_corba_object_get_type ())
#define MATECOMPONENT_TYPE_CORBA_TYPECODE                  (matecomponent_corba_typecode_get_type ())
#define MATECOMPONENT_TYPE_CORBA_EXCEPTION                 (matecomponent_corba_exception_get_type ())

#define MATECOMPONENT_TYPE_STATIC_UNKNOWN                  (matecomponent_unknown_get_type () | G_SIGNAL_TYPE_STATIC_SCOPE)
#define MATECOMPONENT_TYPE_STATIC_CORBA_ANY                (matecomponent_corba_any_get_type () | G_SIGNAL_TYPE_STATIC_SCOPE)
#define MATECOMPONENT_TYPE_STATIC_CORBA_OBJECT             (matecomponent_corba_object_get_type () | G_SIGNAL_TYPE_STATIC_SCOPE)
#define MATECOMPONENT_TYPE_STATIC_CORBA_TYPECODE           (matecomponent_corba_typecode_get_type () | G_SIGNAL_TYPE_STATIC_SCOPE)
#define MATECOMPONENT_TYPE_STATIC_CORBA_EXCEPTION          (matecomponent_corba_exception_get_type () | G_SIGNAL_TYPE_STATIC_SCOPE)

#define MATECOMPONENT_VALUE_HOLDS_UNKNOWN(value)           (G_TYPE_CHECK_VALUE_TYPE ((value), MATECOMPONENT_TYPE_UNKNOWN))
#define MATECOMPONENT_VALUE_HOLDS_CORBA_ANY(value)         (G_TYPE_CHECK_VALUE_TYPE ((value), MATECOMPONENT_TYPE_CORBA_ANY))
#define MATECOMPONENT_VALUE_HOLDS_CORBA_OBJECT(value)      (G_TYPE_CHECK_VALUE_TYPE ((value), MATECOMPONENT_TYPE_CORBA_OBJECT))
#define MATECOMPONENT_VALUE_HOLDS_CORBA_TYPECODE(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), MATECOMPONENT_TYPE_CORBA_TYPECODE))
#define MATECOMPONENT_VALUE_HOLDS_CORBA_EXCEPTION(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), MATECOMPONENT_TYPE_CORBA_EXCEPTION))

MateComponent_Unknown           matecomponent_value_get_unknown         (const GValue *value);
MateComponentArg               *matecomponent_value_get_corba_any       (const GValue *value);
CORBA_Object             matecomponent_value_get_corba_object    (const GValue *value);
CORBA_TypeCode           matecomponent_value_get_corba_typecode  (const GValue *value);
const CORBA_Environment *matecomponent_value_get_corba_exception (const GValue *value);

void matecomponent_value_set_corba_object       (GValue                      *value,
                                          const CORBA_Object           object);

void matecomponent_value_set_unknown            (GValue                      *value,
                                          const MateComponent_Unknown         unknown);

void matecomponent_value_set_corba_any          (GValue                      *value,
                                          const CORBA_any             *any);

void matecomponent_value_set_corba_typecode     (GValue                      *value,
                                          const CORBA_TypeCode         tc);

void matecomponent_value_set_corba_environment  (GValue                      *value,
                                          const CORBA_Environment     *ev);

void       matecomponent_closure_invoke_va_list (GClosure            *closure,
					  GValue              *return_value,
					  va_list              var_args);

void       matecomponent_closure_invoke         (GClosure            *closure,
					  GType                return_type,
					  ...);

GClosure * matecomponent_closure_store          (GClosure            *closure,
					  GClosureMarshal      default_marshal);

#ifdef __cplusplus
}
#endif

#endif
