/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 *  MateComponent::Zoomable - zoomable interface for Controls.
 *
 *  Authors: Maciej Stachowiak <mjs@eazel.com>
 *           Martin Baulig <baulig@suse.de>
 *
 *  Copyright (C) 2000 Eazel, Inc.
 *                2000 SuSE GmbH.
 */

#ifndef _MATECOMPONENT_ZOOMABLE_H_
#define _MATECOMPONENT_ZOOMABLE_H_

#include <matecomponent/matecomponent-object.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_ZOOMABLE        (matecomponent_zoomable_get_type ())
#define MATECOMPONENT_ZOOMABLE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_ZOOMABLE, MateComponentZoomable))
#define MATECOMPONENT_ZOOMABLE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_ZOOMABLE, MateComponentZoomableClass))
#define MATECOMPONENT_IS_ZOOMABLE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_ZOOMABLE))
#define MATECOMPONENT_IS_ZOOMABLE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_ZOOMABLE))
#define MATECOMPONENT_ZOOMABLE_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), MATECOMPONENT_TYPE_ZOOMABLE, MateComponentZoomableClass))

typedef struct _MateComponentZoomablePrivate	MateComponentZoomablePrivate;

typedef struct {
        MateComponentObject		object;

	MateComponentZoomablePrivate	*priv;
} MateComponentZoomable;

typedef struct {
	MateComponentObjectClass	parent;

	POA_MateComponent_Zoomable__epv epv;

	void (*set_frame)	(MateComponentZoomable *zoomable);
	void (*set_zoom_level)	(MateComponentZoomable *zoomable,
				 CORBA_float     zoom_level);

	void (*zoom_in)		(MateComponentZoomable *zoomable);
	void (*zoom_out)	(MateComponentZoomable *zoomable);
	void (*zoom_to_fit)	(MateComponentZoomable *zoomable);
	void (*zoom_to_default)	(MateComponentZoomable *zoomable);

	gpointer dummy[4];
} MateComponentZoomableClass;

GType		 matecomponent_zoomable_get_type                       (void) G_GNUC_CONST;

MateComponentZoomable	*matecomponent_zoomable_new				(void);

void		 matecomponent_zoomable_set_parameters			(MateComponentZoomable	*zoomable,
								 float           zoom_level,
								 float		 min_zoom_level,
								 float		 max_zoom_level,
								 gboolean	 has_min_zoom_level,
								 gboolean	 has_max_zoom_level);

void		 matecomponent_zoomable_set_parameters_full		(MateComponentZoomable	*zoomable,
								 float           zoom_level,
								 float		 min_zoom_level,
								 float		 max_zoom_level,
								 gboolean	 has_min_zoom_level,
								 gboolean	 has_max_zoom_level,
								 gboolean	 is_continuous,
								 CORBA_float    *preferred_zoom_levels,
								 const gchar   **preferred_zoom_level_names,
								 gint		 num_preferred_zoom_levels);
void             matecomponent_zoomable_add_preferred_zoom_level       (MateComponentZoomable *zoomable,
                                                                 CORBA_float     zoom_level,
                                                                 const gchar    *zoom_level_name);

void		 matecomponent_zoomable_report_zoom_level_changed	(MateComponentZoomable	   *zoomable,
								 float		    new_zoom_level,
								 CORBA_Environment *opt_ev);
void		 matecomponent_zoomable_report_zoom_parameters_changed	(MateComponentZoomable    *zoomable,
								 CORBA_Environment *opt_ev);


G_END_DECLS

#endif /* _MATECOMPONENT_ZOOMABLE_H_ */
