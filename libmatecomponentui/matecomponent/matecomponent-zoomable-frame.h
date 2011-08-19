/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 *  MateComponent::ZoomableFrame - container side part of MateComponent::Zoomable.
 *
 *  Authors: Maciej Stachowiak <mjs@eazel.com>
 *           Martin Baulig <baulig@suse.de>
 *
 *  Copyright (C) 2000 Eazel, Inc.
 *                2000 SuSE GmbH.
 */
#ifndef _MATECOMPONENT_ZOOMABLE_FRAME_H_
#define _MATECOMPONENT_ZOOMABLE_FRAME_H_

#include <matecomponent/matecomponent-object.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_ZOOMABLE_FRAME        (matecomponent_zoomable_frame_get_type ())
#define MATECOMPONENT_ZOOMABLE_FRAME(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_ZOOMABLE_FRAME, MateComponentZoomableFrame))
#define MATECOMPONENT_ZOOMABLE_FRAME_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_ZOOMABLE_FRAME, MateComponentZoomableFrameClass))
#define MATECOMPONENT_IS_ZOOMABLE_FRAME(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_ZOOMABLE_FRAME))
#define MATECOMPONENT_IS_ZOOMABLE_FRAME_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_ZOOMABLE_FRAME))

typedef struct _MateComponentZoomableFramePrivate	MateComponentZoomableFramePrivate;

typedef struct {
        MateComponentObject			object;

	MateComponentZoomableFramePrivate	*priv;
} MateComponentZoomableFrame;

typedef struct {
	MateComponentObjectClass		parent;

	POA_MateComponent_ZoomableFrame__epv   epv;

	void (*zoom_level_changed)	(MateComponentZoomableFrame *zframe,
					 float zoom_level);
	void (*zoom_parameters_changed)	(MateComponentZoomableFrame *zframe);

	gpointer dummy[2];
} MateComponentZoomableFrameClass;

GType			 matecomponent_zoomable_frame_get_type			(void) G_GNUC_CONST;

MateComponentZoomableFrame	*matecomponent_zoomable_frame_new			(void);
MateComponent_Zoomable          matecomponent_zoomable_frame_get_zoomable             (MateComponentZoomableFrame    *zframe);

float		 matecomponent_zoomable_frame_get_zoom_level			(MateComponentZoomableFrame	*zframe);

float		 matecomponent_zoomable_frame_get_min_zoom_level		(MateComponentZoomableFrame	*zframe);
float		 matecomponent_zoomable_frame_get_max_zoom_level		(MateComponentZoomableFrame	*zframe);
gboolean	 matecomponent_zoomable_frame_has_min_zoom_level		(MateComponentZoomableFrame	*zframe);
gboolean	 matecomponent_zoomable_frame_has_max_zoom_level		(MateComponentZoomableFrame	*zframe);
gboolean	 matecomponent_zoomable_frame_is_continuous			(MateComponentZoomableFrame	*zframe);

GList		*matecomponent_zoomable_frame_get_preferred_zoom_levels	(MateComponentZoomableFrame	*zframe);
GList		*matecomponent_zoomable_frame_get_preferred_zoom_level_names	(MateComponentZoomableFrame	*zframe);

void		 matecomponent_zoomable_frame_set_zoom_level			(MateComponentZoomableFrame	*zframe,
									 float			 zoom_level);

void		 matecomponent_zoomable_frame_zoom_in				(MateComponentZoomableFrame	*zframe);
void		 matecomponent_zoomable_frame_zoom_out				(MateComponentZoomableFrame	*zframe);
void		 matecomponent_zoomable_frame_zoom_to_fit			(MateComponentZoomableFrame	*zframe);
void		 matecomponent_zoomable_frame_zoom_to_default			(MateComponentZoomableFrame	*zframe);
void		 matecomponent_zoomable_frame_bind_to_zoomable			(MateComponentZoomableFrame	*zframe,
									 MateComponent_Zoomable	 zoomable,
									 CORBA_Environment      *opt_ev);

G_END_DECLS

#endif /* _MATECOMPONENT_ZOOMABLE_FRAME_H_ */

