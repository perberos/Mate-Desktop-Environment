/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-canvas-component.h: implements the CORBA interface for
 * the MateComponent::Canvas:Item interface used in MateComponent::Views.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * (C) 1999 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_CANVAS_COMPONENT_H_
#define _MATECOMPONENT_CANVAS_COMPONENT_H_

#include <glib.h>
#include <gdk/gdk.h>
#include <matecomponent/matecomponent-object.h>
#include <libmatecanvas/mate-canvas.h>

#ifndef MATECOMPONENT_UI_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_CANVAS_COMPONENT        (matecomponent_canvas_component_get_type ())
#define MATECOMPONENT_CANVAS_COMPONENT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_CANVAS_COMPONENT, MateComponentCanvasComponent))
#define MATECOMPONENT_CANVAS_COMPONENT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_CANVAS_COMPONENT_, MateComponentCanvasComponentClass))
#define MATECOMPONENT_IS_CANVAS_COMPONENT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_CANVAS_COMPONENT))
#define MATECOMPONENT_IS_CANVAS_COMPONENT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_CANVAS_COMPONENT))

typedef struct _MateComponentCanvasComponentPrivate MateComponentCanvasComponentPrivate;

typedef struct {
	MateComponentObject base;
	MateComponentCanvasComponentPrivate *priv;
} MateComponentCanvasComponent;

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_Canvas_Component__epv epv;

	/* Signals */
	void (*set_bounds) (MateComponentCanvasComponent *component,
			    MateComponent_Canvas_DRect   *bbox,
			    CORBA_Environment     *ev);

	gboolean (*event)  (MateComponentCanvasComponent *component,
			    GdkEvent              *event);
} MateComponentCanvasComponentClass;

GType                   matecomponent_canvas_component_get_type         (void) G_GNUC_CONST;
MateComponentCanvasComponent  *matecomponent_canvas_component_construct        (MateComponentCanvasComponent *comp,
								  MateCanvasItem       *item);
MateComponentCanvasComponent  *matecomponent_canvas_component_new              (MateCanvasItem       *item);
MateCanvasItem        *matecomponent_canvas_component_get_item         (MateComponentCanvasComponent *comp);
void		        matecomponent_canvas_component_grab		 (MateComponentCanvasComponent *comp,
								  guint                  mask,
								  GdkCursor             *cursor,
								  guint32                time,
								  CORBA_Environment     *opt_ev);
void		        matecomponent_canvas_component_ungrab		 (MateComponentCanvasComponent *comp,
								  guint32                time,
								  CORBA_Environment     *opt_ev);
MateComponent_UIContainer      matecomponent_canvas_component_get_ui_container (MateComponentCanvasComponent *comp,
								  CORBA_Environment     *opt_ev);

/* This is a helper function for creating a canvas with the root item replaced
 * by a proxy to the client side proxy.
 */
MateCanvas *matecomponent_canvas_new (gboolean                     is_aa,
				MateComponent_Canvas_ComponentProxy proxy);


/* Sets up a callback to be invoked when the container activates the object.
 * Creating the component factory will do nothing until the container connects.
 */
typedef MateComponentCanvasComponent *(*MateItemCreator)
   (MateCanvas *canvas, void *user_data);

MateComponentObject *matecomponent_canvas_component_factory_new(MateItemCreator item_factory,
      void *data);

G_END_DECLS

#endif /* MATECOMPONENT_UI_DISABLE_DEPRECATED */

#endif /* */
