/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _MATECOMPONENT_MONIKER_QUERY_H_
#define _MATECOMPONENT_MONIKER_QUERY_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_MONIKER_QUERY        (matecomponent_moniker_query_get_type ())
#define MATECOMPONENT_MONIKER_QUERY_TYPE        MATECOMPONENT_TYPE_MONIKER_QUERY /* deprecated, you should use MATECOMPONENT_TYPE_MONIKER_QUERY */
#define MATECOMPONENT_MONIKER_QUERY(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_MONIKER_QUERY, MateComponentMonikerQuery))
#define MATECOMPONENT_MONIKER_QUERY_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_MONIKER_QUERY, MateComponentMonikerQueryClass))
#define MATECOMPONENT_IS_MONIKER_QUERY(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_MONIKER_QUERY))
#define MATECOMPONENT_IS_MONIKER_QUERY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_MONIKER_QUERY))

typedef struct _MateComponentMonikerQuery        MateComponentMonikerQuery;
typedef struct _MateComponentMonikerQueryPrivate MateComponentMonikerQueryPrivate;

struct _MateComponentMonikerQuery {
	MateComponentMoniker parent;

	MateComponentMonikerQueryPrivate *priv;
};

typedef struct {
	MateComponentMonikerClass parent_class;
} MateComponentMonikerQueryClass;

GType          matecomponent_moniker_query_get_type  (void);
MateComponentMoniker *matecomponent_moniker_query_construct (MateComponentMonikerQuery *moniker,
					       MateComponent_Moniker      corba_moniker);
MateComponentMoniker *matecomponent_moniker_query_new       (void);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_MONIKER_QUERY_H_ */
