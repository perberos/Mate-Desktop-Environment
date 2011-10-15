/*
 * matecomponent-property-bag-client.c: C sugar for property bags.
 *
 * Author:
 *   Dietmar Maurer (dietmar@ximian.com)
 *   Michael Meeks  (michael@ximian.com)
 *   Nat Friedman   (nat@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef __MATECOMPONENT_PROPERTY_BAG_CLIENT_H__
#define __MATECOMPONENT_PROPERTY_BAG_CLIENT_H__

#include <matecomponent/matecomponent-property-bag.h>

#ifdef __cplusplus
extern "C" {
#endif

CORBA_TypeCode
matecomponent_pbclient_get_type                 (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gchar *
matecomponent_pbclient_get_string               (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gchar *
matecomponent_pbclient_get_default_string       (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gchar *
matecomponent_pbclient_get_string_with_default  (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gchar              *defval,
					  gboolean           *def);
gint16
matecomponent_pbclient_get_short                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gint16
matecomponent_pbclient_get_default_short        (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gint16
matecomponent_pbclient_get_short_with_default   (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gint16              defval,
					  gboolean           *def);
guint16
matecomponent_pbclient_get_ushort               (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
guint16
matecomponent_pbclient_get_default_ushort       (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
guint16
matecomponent_pbclient_get_ushort_with_default  (MateComponent_PropertyBag  bag,
					  const char         *key,
					  guint16             defval,
					  gboolean           *def);
gint32
matecomponent_pbclient_get_long                 (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gint32
matecomponent_pbclient_get_default_long         (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gint32
matecomponent_pbclient_get_long_with_default    (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gint32              defval,
					  gboolean           *def);
guint32
matecomponent_pbclient_get_ulong                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
guint32
matecomponent_pbclient_get_default_ulong        (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
guint32
matecomponent_pbclient_get_ulong_with_default   (MateComponent_PropertyBag  bag,
					  const char         *key,
					  guint32             defval,
					  gboolean           *def);
gfloat
matecomponent_pbclient_get_float                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gfloat
matecomponent_pbclient_get_default_float        (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gfloat
matecomponent_pbclient_get_float_with_default   (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gfloat              defval,
					  gboolean           *def);
gdouble
matecomponent_pbclient_get_double               (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gdouble
matecomponent_pbclient_get_default_double       (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gdouble
matecomponent_pbclient_get_double_with_default  (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gdouble             defval,
					  gboolean           *def);
gboolean
matecomponent_pbclient_get_boolean              (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gboolean
matecomponent_pbclient_get_default_boolean      (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gboolean
matecomponent_pbclient_get_boolean_with_default (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gboolean            defval,
					  gboolean           *def);
gchar
matecomponent_pbclient_get_char                 (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gchar
matecomponent_pbclient_get_default_char         (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
gchar
matecomponent_pbclient_get_char_with_default    (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gchar               defval,
					  gboolean           *def);
CORBA_any *
matecomponent_pbclient_get_value                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_TypeCode      opt_tc,
					  CORBA_Environment  *opt_ev);

CORBA_any *
matecomponent_pbclient_get_default_value        (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_TypeCode      opt_tc,
					  CORBA_Environment  *opt_ev);

void
matecomponent_pbclient_set_string               (MateComponent_PropertyBag  bag,
					  const char         *key,
					  const char         *value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_short                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gint16              value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_ushort               (MateComponent_PropertyBag  bag,
					  const char         *key,
					  guint16             value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_long                 (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gint32              value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_ulong                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  guint32             value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_float                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gfloat              value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_double               (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gdouble             value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_boolean              (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gboolean            value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_char                 (MateComponent_PropertyBag  bag,
					  const char         *key,
					  gchar               value,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set_value                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_any          *value,
					  CORBA_Environment  *opt_ev);
char *
matecomponent_pbclient_get_doc_title            (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
char *
matecomponent_pbclient_get_doc                  (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
GList *
matecomponent_pbclient_get_keys                 (MateComponent_PropertyBag  bag,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_free_keys                (GList *key_list);

MateComponent_PropertyFlags
matecomponent_pbclient_get_flags                (MateComponent_PropertyBag  bag,
					  const char         *key,
					  CORBA_Environment  *opt_ev);
void
matecomponent_pbclient_set                      (MateComponent_PropertyBag  bag,
					  CORBA_Environment  *opt_ev,
					  const char         *first_prop,
					  ...) G_GNUC_NULL_TERMINATED;
void
matecomponent_pbclient_get                      (MateComponent_PropertyBag  bag,
					  CORBA_Environment  *opt_ev,
					  const char         *first_prop,
					  ...) G_GNUC_NULL_TERMINATED;
char *
matecomponent_pbclient_setv                     (MateComponent_PropertyBag  bag,
					  CORBA_Environment  *ev,
					  const char         *first_arg,
					  va_list             var_args);
char *
matecomponent_pbclient_getv                     (MateComponent_PropertyBag  bag,
					  CORBA_Environment  *ev,
					  const char         *first_arg,
					  va_list             var_args);

void
matecomponent_pbclient_set_value_async          (MateComponent_PropertyBag bag,
					  const char        *key,
					  CORBA_any         *value,
					  CORBA_Environment *opt_ev);

/* just to be compatible */

#define matecomponent_property_bag_client_setv                                       \
matecomponent_pbclient_setv
#define matecomponent_property_bag_client_getv                                       \
matecomponent_pbclient_getv
#define matecomponent_property_bag_client_get_property_type                          \
matecomponent_pbclient_get_type
#define matecomponent_property_bag_client_get_value_gboolean                         \
matecomponent_pbclient_get_boolean
#define matecomponent_property_bag_client_get_value_gint                             \
matecomponent_pbclient_get_long
#define matecomponent_property_bag_client_get_value_glong                            \
matecomponent_pbclient_get_long
#define matecomponent_property_bag_client_get_value_gfloat                           \
matecomponent_pbclient_get_float
#define matecomponent_property_bag_client_get_value_gdouble                          \
matecomponent_pbclient_get_double
#define matecomponent_property_bag_client_get_value_string                           \
matecomponent_pbclient_get_string

#define matecomponent_property_bag_client_get_value_any(pb, name, ev)                \
matecomponent_pbclient_get_value (pb, name, NULL, ev);

#define matecomponent_property_bag_client_get_default_gboolean                       \
matecomponent_pbclient_get_default_boolean
#define matecomponent_property_bag_client_get_default_gint                           \
matecomponent_pbclient_get_default_long
#define matecomponent_property_bag_client_get_default_glong                          \
matecomponent_pbclient_get_default_long
#define matecomponent_property_bag_client_get_default_gfloat                         \
matecomponent_pbclient_get_default_float
#define matecomponent_property_bag_client_get_default_gdouble                        \
matecomponent_pbclient_get_default_double
#define matecomponent_property_bag_client_get_default_string                         \
matecomponent_pbclient_get_default_string

#define matecomponent_property_bag_client_get_default_any(pb, name, ev)              \
matecomponent_pbclient_get_default_value (pb, name, NULL, ev)

#define matecomponent_property_bag_client_set_value_gboolean                         \
matecomponent_pbclient_set_boolean
#define matecomponent_property_bag_client_set_value_gint                             \
matecomponent_pbclient_set_long
#define matecomponent_property_bag_client_set_value_glong                            \
matecomponent_pbclient_set_long
#define matecomponent_property_bag_client_set_value_gfloat                           \
matecomponent_pbclient_set_float
#define matecomponent_property_bag_client_set_value_gdouble                          \
matecomponent_pbclient_set_double
#define matecomponent_property_bag_client_set_value_string                           \
matecomponent_pbclient_set_string
#define matecomponent_property_bag_client_set_value_any                              \
matecomponent_pbclient_set_value

#define matecomponent_property_bag_client_get_docstring                              \
matecomponent_pbclient_get_doc_title
#define matecomponent_property_bag_client_get_flags	                              \
matecomponent_pbclient_get_flags


#ifdef __cplusplus
}
#endif

#endif /* __MATECOMPONENT_PROPERTY_BAG_CLIENT_H__ */
