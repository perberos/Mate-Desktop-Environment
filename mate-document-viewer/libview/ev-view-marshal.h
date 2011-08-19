
#ifndef __ev_view_marshal_MARSHAL_H__
#define __ev_view_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:ENUM,BOOLEAN (./ev-view-marshal.list:1) */
G_GNUC_INTERNAL void ev_view_marshal_VOID__ENUM_BOOLEAN (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* VOID:INT,INT (./ev-view-marshal.list:2) */
G_GNUC_INTERNAL void ev_view_marshal_VOID__INT_INT (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data);

G_END_DECLS

#endif /* __ev_view_marshal_MARSHAL_H__ */

