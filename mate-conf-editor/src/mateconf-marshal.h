
#ifndef __mateconf_marshal_MARSHAL_H__
#define __mateconf_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,BOXED (mateconf-marshal.list:1) */
extern void mateconf_marshal_VOID__STRING_BOXED (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* BOOLEAN:STRING (mateconf-marshal.list:2) */
extern void mateconf_marshal_BOOLEAN__STRING (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

G_END_DECLS

#endif /* __mateconf_marshal_MARSHAL_H__ */

