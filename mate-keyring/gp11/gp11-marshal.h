
#ifndef ___gp11_marshal_MARSHAL_H__
#define ___gp11_marshal_MARSHAL_H__

#include	<glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BOOLEAN:STRING,POINTER (gp11-marshal.list:1) */
extern void _gp11_marshal_BOOLEAN__STRING_POINTER (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* BOOLEAN:OBJECT,STRING,POINTER (gp11-marshal.list:2) */
extern void _gp11_marshal_BOOLEAN__OBJECT_STRING_POINTER (GClosure     *closure,
                                                          GValue       *return_value,
                                                          guint         n_param_values,
                                                          const GValue *param_values,
                                                          gpointer      invocation_hint,
                                                          gpointer      marshal_data);

/* BOOLEAN:ULONG (gp11-marshal.list:3) */
extern void _gp11_marshal_BOOLEAN__ULONG (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

#ifdef __cplusplus
}
#endif

#endif /* ___gp11_marshal_MARSHAL_H__ */

