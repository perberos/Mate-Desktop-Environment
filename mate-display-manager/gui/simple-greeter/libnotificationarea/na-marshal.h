
#ifndef ___na_marshal_MARSHAL_H__
#define ___na_marshal_MARSHAL_H__

#include	<glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

/* VOID:OBJECT,OBJECT (na-marshal.list:1) */
extern void _na_marshal_VOID__OBJECT_OBJECT (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* VOID:OBJECT,STRING,LONG,LONG (na-marshal.list:2) */
extern void _na_marshal_VOID__OBJECT_STRING_LONG_LONG (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);

/* VOID:OBJECT,LONG (na-marshal.list:3) */
extern void _na_marshal_VOID__OBJECT_LONG (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

#ifdef __cplusplus
}
#endif

#endif /* ___na_marshal_MARSHAL_H__ */

