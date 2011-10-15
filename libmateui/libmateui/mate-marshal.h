
#ifndef ___mate_marshal_MARSHAL_H__
#define ___mate_marshal_MARSHAL_H__

#include	<glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BOOLEAN:INT,ENUM,BOOLEAN,ENUM,BOOLEAN (./mate-marshal.list:1) */
extern void _mate_marshal_BOOLEAN__INT_ENUM_BOOLEAN_ENUM_BOOLEAN (GClosure     *closure,
                                                                  GValue       *return_value,
                                                                  guint         n_param_values,
                                                                  const GValue *param_values,
                                                                  gpointer      invocation_hint,
                                                                  gpointer      marshal_data);

/* BOOLEAN:INT,STRING (./mate-marshal.list:2) */
extern void _mate_marshal_BOOLEAN__INT_STRING (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* BOOLEAN:OBJECT (./mate-marshal.list:3) */
extern void _mate_marshal_BOOLEAN__OBJECT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* BOOLEAN:VOID (./mate-marshal.list:4) */
extern void _mate_marshal_BOOLEAN__VOID (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

/* INT:VOID (./mate-marshal.list:5) */
extern void _mate_marshal_INT__VOID (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data);

/* VOID:ENUM,BOOLEAN (./mate-marshal.list:6) */
extern void _mate_marshal_VOID__ENUM_BOOLEAN (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:INT,BOXED (./mate-marshal.list:7) */
extern void _mate_marshal_VOID__INT_BOXED (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* VOID:OBJECT (./mate-marshal.list:8) */
#define _mate_marshal_VOID__OBJECT	g_cclosure_marshal_VOID__OBJECT

/* VOID:UINT,UINT,UINT,UINT (./mate-marshal.list:9) */
extern void _mate_marshal_VOID__UINT_UINT_UINT_UINT (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);

G_END_DECLS

#endif /* ___mate_marshal_MARSHAL_H__ */

