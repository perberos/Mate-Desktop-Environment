/**
 * matecomponent-canvas-item.h: Canvas item implementation for embedding remote 
 * 			 canvas-items
 *
 * Author:
 *     Miguel de Icaza (miguel@kernel.org)
 *
 * (C) 1999, 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_CANVAS_ITEM_H_
#define _MATECOMPONENT_CANVAS_ITEM_H_

#include <glib.h>
#include <libmatecanvas/mate-canvas.h>

#ifndef MATECOMPONENT_UI_DISABLE_DEPRECATED

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_CANVAS_ITEM          (matecomponent_canvas_item_get_type ())
#define MATECOMPONENT_CANVAS_ITEM(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), matecomponent_canvas_item_get_type (), MateComponentCanvasItem))
#define MATECOMPONENT_CANVAS_ITEM_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), matecomponent_canvas_item_get_type (), MateComponentCanvasItemClass))
#define MATECOMPONENT_IS_CANVAS_ITEM(o)         (G_TYPE_CHECK_INSTANCE_TYPE((o), matecomponent_canvas_item_get_type ()))

typedef struct _MateComponentCanvasItemPrivate MateComponentCanvasItemPrivate;
typedef struct _MateComponentCanvasItem        MateComponentCanvasItem;

struct _MateComponentCanvasItem {
	MateCanvasItem         canvas_item;
	MateComponentCanvasItemPrivate *priv;
};

typedef struct {
	MateCanvasItemClass parent_class;
} MateComponentCanvasItemClass;

GType matecomponent_canvas_item_get_type   (void) G_GNUC_CONST;
void  matecomponent_canvas_item_set_bounds (MateComponentCanvasItem *item,
				     double x1, double y1,
				     double x2, double y2);

G_END_DECLS

#endif /* MATECOMPONENT_UI_DISABLE_DEPRECATED */

#endif /* _MATECOMPONENT_CANVAS_ITEM_H_ */

