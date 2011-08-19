/*
 * matecomponent-canvas-component.c: implements the CORBA interface for
 * the MateComponent::Canvas:Item interface used in MateComponent::Views.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * (C) 1999-2001 Ximian, Inc.
 */
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <matecomponent/MateComponent.h>
#include <libart_lgpl/art_affine.h>
#include <libmatecanvas/mate-canvas.h>
#include <gdkconfig.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include <gdk/gdkprivate.h>
#include <gtk/gtk.h>
#include <matecomponent/matecomponent-control.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-ui-component.h>
#include <matecomponent/matecomponent-canvas-component.h>
#include <matecomponent/matecomponent-ui-marshal.h>
#undef MATECOMPONENT_DISABLE_DEPRECATED
#include <matecomponent/matecomponent-xobject.h>

enum {
	SET_BOUNDS,
	EVENT,
	LAST_SIGNAL
};

static gint gcc_signals [LAST_SIGNAL] = { 0, 0, };

typedef MateComponentCanvasComponent Gcc;
#define GCC(x) MATECOMPONENT_CANVAS_COMPONENT(x)

struct _MateComponentCanvasComponentPrivate {
	MateCanvasItem   *item;
};

#define PARENT_TYPE MATECOMPONENT_TYPE_OBJECT

/* Returns the MateCanvasItemClass of an object */
#define ICLASS(x) MATE_CANVAS_ITEM_CLASS ((GTK_OBJECT_GET_CLASS (x)))

static GObjectClass *gcc_parent_class;

static gboolean do_update_flag = FALSE;

typedef struct
{
        gpointer *arg1;
        gpointer *arg2;
} EmitLater;


static gboolean
CORBA_SVP_Segment_to_SVPSeg (MateComponent_Canvas_SVPSegment *seg, ArtSVPSeg *art_seg)
{
	int i;

	art_seg->points = art_new (ArtPoint, seg->points._length);
	if (!art_seg->points)
		return FALSE;

	art_seg->dir = seg->up ? 0 : 1;
	art_seg->bbox.x0 = seg->bbox.x0;
	art_seg->bbox.x1 = seg->bbox.x1;
	art_seg->bbox.y0 = seg->bbox.y0;
	art_seg->bbox.y1 = seg->bbox.y1;

	art_seg->n_points = seg->points._length;

	for (i = 0; i < art_seg->n_points; i++){
		art_seg->points [i].x = seg->points._buffer [i].x;
		art_seg->points [i].y = seg->points._buffer [i].y;
	}

	return TRUE;
}

static void
free_seg (ArtSVPSeg *seg)
{
	g_assert (seg != NULL);
	g_assert (seg->points != NULL);
	
	art_free (seg->points);
}

/*
 * Encodes an ArtUta
 */
static MateComponent_Canvas_ArtUTA *
CORBA_UTA (ArtUta *uta)
{
	MateComponent_Canvas_ArtUTA *cuta;

	cuta = MateComponent_Canvas_ArtUTA__alloc ();
	if (!cuta)
		return NULL;

	if (!uta) {
		cuta->width = 0;
		cuta->height = 0;
		cuta->utiles._length = 0;
		cuta->utiles._maximum = 0;

		return cuta;
	}
	cuta->utiles._buffer = CORBA_sequence_MateComponent_Canvas_int32_allocbuf (uta->width * uta->height);
	cuta->utiles._length = uta->width * uta->height;
	cuta->utiles._maximum = uta->width * uta->height;
	if (!cuta->utiles._buffer) {
		CORBA_free (cuta);
		return NULL;
	}
		
	cuta->x0 = uta->x0;
	cuta->y0 = uta->y0;
	cuta->width = uta->width;
	cuta->height = uta->height;

	memcpy (cuta->utiles._buffer, uta->utiles, uta->width * uta->height * sizeof (ArtUtaBbox));

	return cuta;
}

static void
restore_state (MateCanvasItem *item, const MateComponent_Canvas_State *state)
{
	double affine [6];
	int i;

	for (i = 0; i < 6; i++)
		affine [i] = state->item_aff [i];

	mate_canvas_item_affine_absolute (item->canvas->root, affine);
	item->canvas->pixels_per_unit = state->pixels_per_unit;
	item->canvas->scroll_x1 = state->canvas_scroll_x1;
	item->canvas->scroll_y1 = state->canvas_scroll_y1;
	item->canvas->zoom_xofs = state->zoom_xofs;
	item->canvas->zoom_yofs = state->zoom_yofs;
}

/* This is copied from mate-canvas.c since it is declared static */
static void
invoke_update (MateCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	int child_flags;
	double *child_affine;
        double i2w[6], w2c[6], i2c[6];

	child_flags = flags;
	if (!(item->object.flags & MATE_CANVAS_ITEM_VISIBLE))
		child_flags &= ~MATE_CANVAS_UPDATE_IS_VISIBLE;

	/* Apply the child item's transform */
        mate_canvas_item_i2w_affine (item, i2w);
        mate_canvas_w2c_affine (item->canvas, w2c);
        art_affine_multiply (i2c, i2w, w2c);
        child_affine = i2c;

	/* apply object flags to child flags */

	child_flags &= ~MATE_CANVAS_UPDATE_REQUESTED;

	if (item->object.flags & MATE_CANVAS_ITEM_NEED_UPDATE)
		child_flags |= MATE_CANVAS_UPDATE_REQUESTED;

	if (item->object.flags & MATE_CANVAS_ITEM_NEED_AFFINE)
		child_flags |= MATE_CANVAS_UPDATE_AFFINE;

	if (item->object.flags & MATE_CANVAS_ITEM_NEED_CLIP)
		child_flags |= MATE_CANVAS_UPDATE_CLIP;

	if (item->object.flags & MATE_CANVAS_ITEM_NEED_VIS)
		child_flags |= MATE_CANVAS_UPDATE_VISIBILITY;

	if ((child_flags & (MATE_CANVAS_UPDATE_REQUESTED
			    | MATE_CANVAS_UPDATE_AFFINE
			    | MATE_CANVAS_UPDATE_CLIP
			    | MATE_CANVAS_UPDATE_VISIBILITY))
	    && MATE_CANVAS_ITEM_GET_CLASS (item)->update)
		(* MATE_CANVAS_ITEM_GET_CLASS (item)->update) (
			item, child_affine, clip_path, child_flags);
}

static MateComponent_Canvas_ArtUTA *
impl_MateComponent_Canvas_Component_update (PortableServer_Servant     servant,
				     const MateComponent_Canvas_State *state,
				     const MateComponent_Canvas_affine aff,
				     const MateComponent_Canvas_SVP   *clip_path,
				     CORBA_long                 flags,
				     CORBA_double              *x1, 
				     CORBA_double              *y1, 
				     CORBA_double              *x2, 
				     CORBA_double              *y2, 
				     CORBA_Environment         *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
	double affine [6];
	int i;
	ArtSVP *svp = NULL;
	MateComponent_Canvas_ArtUTA *cuta;

	MateCanvasItemClass *gci_class = g_type_class_ref (
					mate_canvas_item_get_type ());

	restore_state (item, state);
	for (i = 0; i < 6; i++)
		affine [i] = aff [i];

	if (clip_path->_length > 0) {
		svp = art_alloc (sizeof (ArtSVP) + (clip_path->_length * sizeof (ArtSVPSeg)));
		if (svp == NULL)
			goto fail;

		svp->n_segs = clip_path->_length;
		
		for (i = 0; svp->n_segs; i++) {
			gboolean ok;
		
			ok = CORBA_SVP_Segment_to_SVPSeg (&clip_path->_buffer [i], &svp->segs [i]);

			if (!ok) {
				int j;

				for (j = 0; j < i; j++) {
					free_seg (&svp->segs [j]);
					art_free (svp);
					goto fail;
				}
			}
		}
	}

	invoke_update (item, (double *)aff, svp, flags);

	if (svp){
		for (i = 0; i < svp->n_segs; i++)
			free_seg (&svp->segs [i]);
		art_free (svp);
	}

 fail:
	if (getenv ("CC_DEBUG"))
		printf ("%g %g %g %g\n", item->x1, item->x2, item->y1, item->y2);
	*x1 = item->x1;
	*x2 = item->x2;
	*y1 = item->y1;
	*y2 = item->y2;

	cuta = CORBA_UTA (item->canvas->redraw_area);
	if (cuta == NULL) {
		CORBA_exception_set_system (ev, ex_CORBA_NO_MEMORY, CORBA_COMPLETED_NO);
		return NULL;
	}

	/*
	 * Now, mark our canvas as fully up to date
	 */

        /* Clears flags for root item. */
	(* gci_class->update) (item->canvas->root, affine, svp, flags);

	if (item->canvas->redraw_area) {
		art_uta_free (item->canvas->redraw_area);
		item->canvas->redraw_area = NULL;
	}
	item->canvas->need_redraw = FALSE;
	
	return cuta;
}

static void
impl_MateComponent_Canvas_Component_realize (PortableServer_Servant  servant,
				      const CORBA_char       *window,
				      CORBA_Environment      *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
	GdkWindow *gdk_window = gdk_window_foreign_new_for_display
		(gtk_widget_get_display (GTK_WIDGET (item->canvas)),
		 matecomponent_control_x11_from_window_id (window));

	if (gdk_window == NULL) {
		g_warning ("Invalid window id passed='%s'", window);
		return;
	}

	item->canvas->layout.bin_window = gdk_window;
	ICLASS (item)->realize (item);
}

static void
impl_MateComponent_Canvas_Component_unrealize (PortableServer_Servant servant,
					CORBA_Environment     *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);

	ICLASS (item)->unrealize (item);

	if (item->canvas->layout.bin_window) {
		g_object_unref (item->canvas->layout.bin_window);
		item->canvas->layout.bin_window = NULL;
	}
}

static void
impl_MateComponent_Canvas_Component_map (PortableServer_Servant servant,
				  CORBA_Environment     *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
	
	ICLASS (item)->map (item);
}

static void
impl_MateComponent_Canvas_Component_unmap (PortableServer_Servant servant,
				    CORBA_Environment     *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
	
	ICLASS (item)->unmap (item);
}

static void
my_gdk_pixmap_foreign_release (GdkPixmap *pixmap)
{
	GdkWindowObject *priv = (GdkWindowObject *) pixmap;

	if (G_OBJECT (priv)->ref_count != 1){
		g_warning ("This item is keeping a refcount to a foreign pixmap");
		return;
	}

#ifdef FIXME
	gdk_xid_table_remove (priv->xwindow);
	g_dataset_destroy (priv);
	g_free (priv);
#endif
}

static void
impl_MateComponent_Canvas_Component_draw (PortableServer_Servant        servant,
				   const MateComponent_Canvas_State    *state,
				   const CORBA_char             *drawable_id,
				   CORBA_short                   x,
				   CORBA_short                   y,
				   CORBA_short                   width,
				   CORBA_short                   height,
				   CORBA_Environment            *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
	GdkPixmap *pix;
	
	gdk_flush ();
	pix = gdk_pixmap_foreign_new_for_display
		(gtk_widget_get_display (GTK_WIDGET (item->canvas)),
		 matecomponent_control_x11_from_window_id (drawable_id));

	if (pix == NULL){
		g_warning ("Invalid window id passed='%s'", drawable_id);
		return;
	}

	restore_state (item, state);
	ICLASS (item)->draw (item, pix, x, y, width, height);

	my_gdk_pixmap_foreign_release (pix);
	gdk_flush ();
}

static void
impl_MateComponent_Canvas_Component_render (PortableServer_Servant servant,
				     MateComponent_Canvas_Buf     *buf,
				     CORBA_Environment     *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
	MateCanvasBuf canvas_buf;

	if (!(buf->flags & MateComponent_Canvas_IS_BUF)) {
		buf->rgb_buf._length = buf->row_stride * (buf->rect.y1 - buf->rect.y0);
		buf->rgb_buf._maximum = buf->rgb_buf._length;
		
		buf->rgb_buf._buffer = CORBA_sequence_CORBA_octet_allocbuf (
			buf->rgb_buf._length);
		CORBA_sequence_set_release (&buf->rgb_buf, TRUE);

		if (buf->rgb_buf._buffer == NULL) {
			CORBA_exception_set_system (
				ev, ex_CORBA_NO_MEMORY, CORBA_COMPLETED_NO);
			return;
		}
	}

	canvas_buf.buf = buf->rgb_buf._buffer;
	
	canvas_buf.buf_rowstride = buf->row_stride;
	canvas_buf.rect.x0 = buf->rect.x0;
	canvas_buf.rect.x1 = buf->rect.x1;
	canvas_buf.rect.y0 = buf->rect.y0;
	canvas_buf.rect.y1 = buf->rect.y1;
	canvas_buf.bg_color = buf->bg_color;
	if (buf->flags & MateComponent_Canvas_IS_BG)
		canvas_buf.is_bg = 1;
	else
		canvas_buf.is_bg = 0;

	if (buf->flags & MateComponent_Canvas_IS_BUF)
		canvas_buf.is_buf = 1;
	else
		canvas_buf.is_buf = 0;

	ICLASS (item)->render (item, &canvas_buf);

	/* return */
	buf->flags =
		(canvas_buf.is_bg ? MateComponent_Canvas_IS_BG : 0) |
		(canvas_buf.is_buf ? MateComponent_Canvas_IS_BUF : 0);
}

static CORBA_boolean 
impl_MateComponent_Canvas_Component_contains (PortableServer_Servant servant,
				       CORBA_double           x,
				       CORBA_double           y,
				       CORBA_Environment     *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
        MateCanvasItem *new_item;
        int cx, cy;
	CORBA_boolean ret;

        mate_canvas_w2c (item->canvas, x, y, &cx, &cy);

	if (getenv ("CC_DEBUG"))
		printf ("Point %g %g: ", x, y);
	ret = ICLASS (item)->point (item, x, y, cx, cy, &new_item) == 0.0 &&
                new_item != NULL;
	if (getenv ("CC_DEBUG"))
		printf ("=> %s\n", ret ? "yes" : "no");
	
	return ret;
}

static void
impl_MateComponent_Canvas_Component_bounds (PortableServer_Servant     servant,
				     const MateComponent_Canvas_State *state,
				     CORBA_double              *x1,
				     CORBA_double              *x2,
				     CORBA_double              *y1,
				     CORBA_double              *y2,
				     CORBA_Environment         *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);

	restore_state (item, state);
	ICLASS (item)->bounds (item, x1, y1, x2, y2);
}

/*
 * Converts the event marshalled from the container into a GdkEvent
 */
static void
MateComponent_Gdk_Event_to_GdkEvent (const MateComponent_Gdk_Event *mate_event, GdkEvent *gdk_event)
{
	switch (mate_event->_d){
	case MateComponent_Gdk_FOCUS:
		gdk_event->type = GDK_FOCUS_CHANGE;
		gdk_event->focus_change.in = mate_event->_u.focus.inside;
		return;
		
	case MateComponent_Gdk_KEY:
		if (mate_event->_u.key.type == MateComponent_Gdk_KEY_PRESS)
			gdk_event->type = GDK_KEY_PRESS;
		else
			gdk_event->type = GDK_KEY_RELEASE;
		gdk_event->key.time = mate_event->_u.key.time;
		gdk_event->key.state = mate_event->_u.key.state;
		gdk_event->key.keyval = mate_event->_u.key.keyval;
		gdk_event->key.length = mate_event->_u.key.length;
		gdk_event->key.string = g_strdup (mate_event->_u.key.str);
		return;
		
	case MateComponent_Gdk_MOTION:
		gdk_event->type = GDK_MOTION_NOTIFY;
		gdk_event->motion.time = mate_event->_u.motion.time;
		gdk_event->motion.x = mate_event->_u.motion.x;
		gdk_event->motion.y = mate_event->_u.motion.y;
		gdk_event->motion.x_root = mate_event->_u.motion.x_root;
		gdk_event->motion.y_root = mate_event->_u.motion.y_root;
#ifdef FIXME
		gdk_event->motion.xtilt = mate_event->_u.motion.xtilt;
		gdk_event->motion.ytilt = mate_event->_u.motion.ytilt;
#endif
		gdk_event->motion.state = mate_event->_u.motion.state;
		gdk_event->motion.is_hint = mate_event->_u.motion.is_hint;
		return;
		
	case MateComponent_Gdk_BUTTON:
		switch (mate_event->_u.button.type){
		case MateComponent_Gdk_BUTTON_PRESS:
			gdk_event->type = GDK_BUTTON_PRESS;
			break;
		case MateComponent_Gdk_BUTTON_RELEASE:
			gdk_event->type = GDK_BUTTON_RELEASE;
			break;
		case MateComponent_Gdk_BUTTON_2_PRESS:
			gdk_event->type = GDK_2BUTTON_PRESS;
			break;
		case MateComponent_Gdk_BUTTON_3_PRESS:
			gdk_event->type = GDK_3BUTTON_PRESS;
			break;
		}
		gdk_event->button.time   = mate_event->_u.button.time;
		gdk_event->button.x      = mate_event->_u.button.x;
		gdk_event->button.y      = mate_event->_u.button.y;
		gdk_event->button.x_root = mate_event->_u.button.x_root;
		gdk_event->button.y_root = mate_event->_u.button.y_root;
		gdk_event->button.button = mate_event->_u.button.button;
		return;
		
	case MateComponent_Gdk_CROSSING:
		if (mate_event->_u.crossing.type == MateComponent_Gdk_ENTER)
			gdk_event->type = GDK_ENTER_NOTIFY;
		else
			gdk_event->type = GDK_LEAVE_NOTIFY;
		
		gdk_event->crossing.time   = mate_event->_u.crossing.time;
		gdk_event->crossing.x      = mate_event->_u.crossing.x;
		gdk_event->crossing.y      = mate_event->_u.crossing.y;
		gdk_event->crossing.x_root = mate_event->_u.crossing.x_root;
		gdk_event->crossing.y_root = mate_event->_u.crossing.y_root;
		gdk_event->crossing.state  = mate_event->_u.crossing.state;
		switch (mate_event->_u.crossing.mode){
		case MateComponent_Gdk_NORMAL:
			gdk_event->crossing.mode = GDK_CROSSING_NORMAL;
			break;
			
		case MateComponent_Gdk_GRAB:
			gdk_event->crossing.mode = GDK_CROSSING_GRAB;
			break;
		case MateComponent_Gdk_UNGRAB:
			gdk_event->crossing.mode = GDK_CROSSING_UNGRAB;
			break;
		}
		return;
	}
	g_assert_not_reached ();
}

/**
 * handle_event:
 * @canvas: the pseudo-canvas that this component is part of.
 * @ev: The GdkEvent event as set up for the component.
 *
 * Returns: True if a canvas item handles the event and returns true.
 *
 * Passes the remote item's event back into the local pseudo-canvas so that
 * canvas items can see events the normal way as if they were not using matecomponent.
 */
static gboolean 
handle_event(GtkWidget *canvas, GdkEvent *ev)
{
        GtkWidgetClass *klass = GTK_WIDGET_GET_CLASS(canvas);
        gboolean retval = FALSE;

        switch (ev->type) {
                case GDK_ENTER_NOTIFY:
                        mate_canvas_world_to_window(MATE_CANVAS(canvas),
                                        ev->crossing.x, ev->crossing.y,
                                        &ev->crossing.x, &ev->crossing.y);
                        retval = (klass->enter_notify_event)(canvas, 
                                        &ev->crossing);
                        break;
                case GDK_LEAVE_NOTIFY:
                        mate_canvas_world_to_window(MATE_CANVAS(canvas),
                                        ev->crossing.x, ev->crossing.y,
                                        &ev->crossing.x, &ev->crossing.y);
                        retval = (klass->leave_notify_event)(canvas, 
                                        &ev->crossing);
                        break;
                case GDK_MOTION_NOTIFY:
                        mate_canvas_world_to_window(MATE_CANVAS(canvas),
                                        ev->motion.x, ev->motion.y,
                                        &ev->motion.x, &ev->motion.y);
                        retval = (klass->motion_notify_event)(canvas, 
                                        &ev->motion);
                        break;
                case GDK_BUTTON_PRESS:
                case GDK_2BUTTON_PRESS:
                case GDK_3BUTTON_PRESS:
                        mate_canvas_world_to_window(MATE_CANVAS(canvas),
                                        ev->button.x, ev->button.y,
                                        &ev->button.x, &ev->button.y);
                        retval = (klass->button_press_event)(canvas, 
                                        &ev->button);
                        break;
                case GDK_BUTTON_RELEASE:
                        mate_canvas_world_to_window(MATE_CANVAS(canvas),
                                        ev->button.x, ev->button.y,
                                        &ev->button.x, &ev->button.y);
                        retval = (klass->button_release_event)(canvas, 
                                        &ev->button);
                        break;
                case GDK_KEY_PRESS:
                        retval = (klass->key_press_event)(canvas, &ev->key);
                        break;
                case GDK_KEY_RELEASE:
                        retval = (klass->key_release_event)(canvas, &ev->key);
                        break;
                case GDK_FOCUS_CHANGE:
                        if (&ev->focus_change.in)
                                retval = (klass->focus_in_event)(canvas,
                                        &ev->focus_change);
                        else
                                retval = (klass->focus_out_event)(canvas,
                                        &ev->focus_change);
                        break;
                default:
                        g_warning("Canvas event not handled %d", ev->type);
                        break;

        }
        return retval;
}

static gboolean
handle_event_later (EmitLater *data)
{
        GtkWidget *canvas = GTK_WIDGET(data->arg1);
        GdkEvent *event = (GdkEvent *)data->arg2;

        handle_event(canvas, event);
        gdk_event_free(event);
        g_free(data);
        return FALSE;
}

/*
 * Receives events from the container end, decodes it into a synthetic
 * GdkEvent and forwards this to the CanvasItem
 */
static CORBA_boolean
impl_MateComponent_Canvas_Component_event (PortableServer_Servant     servant,
				    const MateComponent_Canvas_State *state,
				    const MateComponent_Gdk_Event    *mate_event,
				    CORBA_Environment         *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
	GdkEvent *gdk_event = gdk_event_new(GDK_NOTHING);
	CORBA_boolean retval = FALSE;
        EmitLater *data;

	restore_state (item, state);
        
        gdk_event->any.window = item->canvas->layout.bin_window;
        g_object_ref(gdk_event->any.window);

	MateComponent_Gdk_Event_to_GdkEvent (mate_event, gdk_event);

        /*
         * Problem: When dealing with multiple canvas component's within the
         * same process it is possible to get to this point when one of the
         * pseudo-canvas's is inside its idle loop.  This is normally not a
         * problem unless an event from one component can trigger a request for
         * an update on another component.
         *
         * Solution: If any component is in do_update, set up an idle_handler
         * and send the event later. If the event is sent later, the client will
         * get a false return value.  Normally this value is used to determine
         * whether or not to propagate the event up the canvas group tree.
         */

        if (!do_update_flag) {
                retval = handle_event(GTK_WIDGET(item->canvas), gdk_event);
                gdk_event_free(gdk_event);
        }
        else {
                data = g_new0(EmitLater, 1);
                data->arg1 = (gpointer)item->canvas;
                data->arg2 = (gpointer)gdk_event;
                /* has a higher priority then do_update*/
                g_idle_add_full(GDK_PRIORITY_REDRAW-10,
                                (GSourceFunc)handle_event_later, data, NULL);
        }

	return retval;
}

static void
impl_MateComponent_Canvas_Component_setCanvasSize (PortableServer_Servant servant,
					    CORBA_short            x,
					    CORBA_short            y,
					    CORBA_short            width,
					    CORBA_short            height,
					    CORBA_Environment     *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
	MateCanvasItem *item = MATE_CANVAS_ITEM (gcc->priv->item);
	GtkAllocation alloc;

	alloc.x = x;
	alloc.y = y;
	alloc.width = width;
	alloc.height = height;

	gtk_widget_size_allocate (GTK_WIDGET (item->canvas), &alloc);
}

static void
set_bounds_later(EmitLater *data)
{
        CORBA_Environment ev;

        CORBA_exception_init (&ev);

        g_signal_emit(GCC(data->arg1), gcc_signals [SET_BOUNDS], 0, (const
                                MateComponent_Canvas_DRect *) data->arg2, &ev);

        g_free(data);
        CORBA_exception_free(&ev);
}

static void
impl_MateComponent_Canvas_Component_setBounds (PortableServer_Servant     servant,
					const MateComponent_Canvas_DRect *bbox,
					CORBA_Environment         *ev)
{
	Gcc *gcc = GCC (matecomponent_object_from_servant (servant));
        EmitLater *data;

        if (!do_update_flag) {
                g_signal_emit (gcc, gcc_signals [SET_BOUNDS], 0, bbox, &ev);
        }
        else {
                data = g_new0(EmitLater, 1);
                data->arg1 = (gpointer)gcc;
                data->arg2 = (gpointer)bbox;
                g_idle_add_full(GDK_PRIORITY_REDRAW-10,
                                (GSourceFunc)set_bounds_later, data, NULL);
        }
}

static void
gcc_finalize (GObject *object)
{
	Gcc *gcc = GCC (object);
	MateCanvasItem *item = MATECOMPONENT_CANVAS_COMPONENT (object)->priv->item;

	gtk_object_destroy (GTK_OBJECT (item->canvas));
	g_free (gcc->priv);

	gcc_parent_class->finalize (object);
}

/* Ripped from gtk+/gtk/gtkmain.c */
static gboolean
_matecomponent_boolean_handled_accumulator (GSignalInvocationHint *ihint,
				     GValue                *return_accu,
				     const GValue          *handler_return,
				     gpointer               dummy)
{
	gboolean continue_emission;
	gboolean signal_handled;
  
	signal_handled = g_value_get_boolean (handler_return);
	g_value_set_boolean (return_accu, signal_handled);
	continue_emission = !signal_handled;
	
	return continue_emission;
}

static void
matecomponent_canvas_component_class_init (MateComponentCanvasComponentClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_Canvas_Component__epv *epv = &klass->epv;

	gcc_parent_class = g_type_class_peek_parent(klass);

	object_class->finalize = gcc_finalize;

	gcc_signals [SET_BOUNDS] = 
                g_signal_new ("set_bounds",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET(MateComponentCanvasComponentClass, set_bounds),
			      NULL, NULL,
			      matecomponent_ui_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2,
			      G_TYPE_POINTER, G_TYPE_POINTER);

	gcc_signals [EVENT] = 
		g_signal_new ("event", 
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET(MateComponentCanvasComponentClass, event),
			      _matecomponent_boolean_handled_accumulator, NULL,
			      matecomponent_ui_marshal_BOOLEAN__POINTER,
			      G_TYPE_BOOLEAN, 1,
			      G_TYPE_POINTER);

	epv->update         = impl_MateComponent_Canvas_Component_update;
	epv->realize        = impl_MateComponent_Canvas_Component_realize;
	epv->unrealize      = impl_MateComponent_Canvas_Component_unrealize;
	epv->map            = impl_MateComponent_Canvas_Component_map;
	epv->unmap          = impl_MateComponent_Canvas_Component_unmap;
	epv->draw           = impl_MateComponent_Canvas_Component_draw;
	epv->render         = impl_MateComponent_Canvas_Component_render;
	epv->bounds         = impl_MateComponent_Canvas_Component_bounds;
	epv->event          = impl_MateComponent_Canvas_Component_event;
	epv->contains       = impl_MateComponent_Canvas_Component_contains;
	epv->setCanvasSize  = impl_MateComponent_Canvas_Component_setCanvasSize;
	epv->setBounds      = impl_MateComponent_Canvas_Component_setBounds;
}

static void
matecomponent_canvas_component_init (GObject *object)
{
	Gcc *gcc = GCC (object);

	gcc->priv = g_new0 (MateComponentCanvasComponentPrivate, 1);
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentCanvasComponent, 
			   MateComponent_Canvas_Component,
			   PARENT_TYPE,
			   matecomponent_canvas_component)


/**
 * matecomponent_canvas_component_construct:
 * @comp: a #MateComponentCanvasComponent to initialize
 * @item: A #MateCanvasItem that is being exported
 *
 * Creates a CORBA server for the interface MateComponent::Canvas::Item
 * wrapping @item.
 *
 * Returns: The MateComponentCanvasComponent.
 */
MateComponentCanvasComponent *
matecomponent_canvas_component_construct (MateComponentCanvasComponent  *comp,
				  MateCanvasItem         *item)
{
	g_return_val_if_fail (MATE_IS_CANVAS_ITEM (item), NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_CANVAS_COMPONENT (comp), NULL);

	comp->priv->item = item;

	return comp;
}

				  
/**
 * matecomponent_canvas_component_new:
 * @item: A MateCanvasItem that is being exported
 *
 * Creates a CORBA server for the interface MateComponent::Canvas::Item
 * wrapping @item.
 *
 * Returns: The MateComponentCanvasComponent.
 */
MateComponentCanvasComponent *
matecomponent_canvas_component_new (MateCanvasItem *item)
{
	MateComponentCanvasComponent *comp;
	
	g_return_val_if_fail (MATE_IS_CANVAS_ITEM (item), NULL);
	
	comp = g_object_new (MATECOMPONENT_TYPE_CANVAS_COMPONENT, NULL);

	return matecomponent_canvas_component_construct (comp, item);
}

/** 
 * matecomponent_canvas_component_get_item:
 * @comp: A #MateComponentCanvasComponent object
 *
 * Returns: The MateCanvasItem that this MateComponentCanvasComponent proxies
 */
MateCanvasItem *
matecomponent_canvas_component_get_item (MateComponentCanvasComponent *comp)
{
	g_return_val_if_fail (comp != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_CANVAS_COMPONENT (comp), NULL);

	return comp->priv->item;
}

/*
 * Hack root item
 *
 * This is a hack since we can not modify the existing MATE Canvas to handle
 * this case.
 *
 * Here is the problem we are solving:
 *
 *    1. Items usually queue a request to be updated/redrawn by calling
 *       mate_canvas_item_request_update().  This triggers in the container
 *       canvas an idle handler to be queued to update the display on the
 *       idle handler.
 *        
 *    2. There is no way we can catch this on the Canvas.
 *
 * To catch this we do:
 *
 *    3. replace the regular Canvas' root field (of type MateCanvasGroup)
 *       with a RootItemHack item.  This item has an overriden ->update method
 *       that will notify the container canvas on the container process about
 *       our update requirement. 
 */

static MateCanvasGroupClass *rih_parent_class;

typedef struct {
	MateCanvasGroup       group;
	MateComponent_Canvas_ComponentProxy proxy;
	MateCanvasItem *orig_root;
} RootItemHack;

typedef struct {
	MateCanvasGroupClass parent_class;
} RootItemHackClass;

static GType root_item_hack_get_type (void);
#define ROOT_ITEM_HACK_TYPE (root_item_hack_get_type ())
#define ROOT_ITEM_HACK(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), ROOT_ITEM_HACK_TYPE, RootItemHack))

static void
rih_dispose (GObject *obj)
{
	RootItemHack *rih = ROOT_ITEM_HACK (obj);

	rih->proxy = matecomponent_object_release_unref (rih->proxy, NULL);

	if (rih->orig_root)
		gtk_object_destroy (GTK_OBJECT (rih->orig_root));
	rih->orig_root = NULL;

	G_OBJECT_CLASS (rih_parent_class)->dispose (obj);
}

/*
 * Invoked by our local canvas when an update is requested,
 * we forward this to the container canvas
 */
static void
rih_update (MateCanvasItem *item, double affine [6], ArtSVP *svp, int flags)
{
	RootItemHack *rih = (RootItemHack *) item;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

        /* If we get here then we know the MateCanvas must be inside
         * do_update. The flag tells ALL components not to send events that
         * might trigger another update request and thereby cause the canvas
         * NEED_UPDATE flags to become unsyncronized.
         */
        do_update_flag = TRUE;

	MateComponent_Canvas_ComponentProxy_requestUpdate (rih->proxy, &ev);

        do_update_flag = FALSE;

	CORBA_exception_free (&ev);
}

static void
rih_class_init (GObjectClass *klass)
{
	MateCanvasItemClass *item_class = (MateCanvasItemClass *) klass;
	rih_parent_class = g_type_class_peek_parent (klass);

	klass->dispose = rih_dispose;

	item_class->update = rih_update;
}
      
static GType
root_item_hack_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (RootItemHackClass),
			NULL, NULL,
			(GClassInitFunc) rih_class_init,
			NULL, NULL,
			sizeof (RootItemHack),
			0,
			NULL, NULL
		};

		type = g_type_register_static (
			mate_canvas_group_get_type (),
			"RootItemHack", &info, 0);
	}

	return type;
}

static RootItemHack *
root_item_hack_new (MateCanvas *canvas, MateComponent_Canvas_ComponentProxy proxy)
{
	RootItemHack *item_hack;

	item_hack = g_object_new (root_item_hack_get_type (), NULL);
	item_hack->proxy = proxy;
	item_hack->orig_root = canvas->root;
	MATE_CANVAS_ITEM (item_hack)->canvas = canvas;

	return item_hack;
}

/**
 * matecomponent_canvas_new:
 * @is_aa: Flag indicating is antialiased canvas is desired
 * @proxy: Remote proxy for the component this canvas will support
 *
 * Returns: A #MateCanvas with the root replaced by a forwarding item.
 */ 
MateCanvas *
matecomponent_canvas_new (gboolean is_aa, MateComponent_Canvas_ComponentProxy proxy)
{
	MateCanvas *canvas;

	if (is_aa)
		canvas = MATE_CANVAS (mate_canvas_new_aa ());
	else
		canvas = MATE_CANVAS (mate_canvas_new ());

	canvas->root = MATE_CANVAS_ITEM (root_item_hack_new (canvas, proxy));

        /* A hack to prevent gtkwidget from issuing a warning about there not
           being a parent class. */
        gtk_container_add(GTK_CONTAINER (gtk_window_new(GTK_WINDOW_TOPLEVEL)),
                            GTK_WIDGET(canvas));
	gtk_widget_realize (GTK_WIDGET (canvas));

	/* Gross */
	GTK_WIDGET_SET_FLAGS (canvas, GTK_VISIBLE | GTK_MAPPED);

	return canvas;
}

/**
 * matecomponent_canvas_component_grab:
 * @comp: A #MateComponentCanvasComponent object
 * @mask: Mask of events to grab
 * @cursor: #GdkCursor to display during grab
 * @time: Time of last event before grab
 *
 * Grabs the mouse focus via a call to the remote proxy.
 */
void
matecomponent_canvas_component_grab (MateComponentCanvasComponent *comp, guint mask,
			      GdkCursor *cursor, guint32 time,
			      CORBA_Environment     *opt_ev)
{
	CORBA_Environment *ev, temp_ev;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	MateComponent_Canvas_ComponentProxy_grabFocus (
		ROOT_ITEM_HACK (comp->priv->item->canvas->root)->proxy, 
		mask, cursor->type, time, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);
}

/**
 * matecomponent_canvas_component_ungrab:
 * @comp: A #MateComponentCanvasComponent object
 * @time: Time of last event before grab
 *
 * Grabs the mouse focus via a call to the remote proxy.
 */
void
matecomponent_canvas_component_ungrab (MateComponentCanvasComponent *comp, guint32 time,
				CORBA_Environment     *opt_ev)
{
	CORBA_Environment *ev, temp_ev;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	MateComponent_Canvas_ComponentProxy_ungrabFocus (
		ROOT_ITEM_HACK (comp->priv->item->canvas->root)->proxy, time, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);
}

/**
 * matecomponent_canvas_component_get_ui_container:
 * @comp: A #MateComponentCanvasComponent object
 *
 * Returns: The UI container for the component's remote proxy.
 */
MateComponent_UIContainer
matecomponent_canvas_component_get_ui_container (MateComponentCanvasComponent *comp,
					  CORBA_Environment     *opt_ev)
{
	MateComponent_UIContainer corba_uic;
	CORBA_Environment *ev, temp_ev;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	corba_uic = MateComponent_Canvas_ComponentProxy_getUIContainer (
		ROOT_ITEM_HACK (comp->priv->item->canvas->root)->proxy, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);

	return corba_uic;
}

/* MateComponentCanvasComponentFactory is used to replace the old MateComponentEmbeddable
 * object.  It sets up a canvas component factory to conform with the current
 * MateComponent IDL.
 */

#define MATECOMPONENT_CANVAS_COMPONENT_FACTORY_TYPE       \
   (matecomponent_canvas_component_factory_get_type())

#define MATECOMPONENT_CANVAS_COMPONENT_FACTORY(o)    \
   (G_TYPE_CHECK_INSTANCE_CAST ((o),\
   MATECOMPONENT_CANVAS_COMPONENT_FACTORY_TYPE, MateComponentCanvasComponentFactory))

typedef struct _MateComponentCanvasComponentFactoryPrivate \
   MateComponentCanvasComponentFactoryPrivate;

typedef struct {
        MateComponentObject base;
        MateComponentCanvasComponentFactoryPrivate *priv;
} MateComponentCanvasComponentFactory;

typedef struct {
        MateComponentObjectClass parent_class;

        POA_MateComponent_CanvasComponentFactory__epv epv;
} MateComponentCanvasComponentFactoryClass;
       
GType matecomponent_canvas_component_factory_get_type(void) G_GNUC_CONST;

struct _MateComponentCanvasComponentFactoryPrivate {
   MateItemCreator item_creator;
   void *item_creator_data;
};

static GObjectClass *gccf_parent_class;

static MateComponent_Canvas_Component
impl_MateComponent_canvas_component_factory_createCanvasItem (
   PortableServer_Servant servant, CORBA_boolean aa,
   const MateComponent_Canvas_ComponentProxy _item_proxy,
   CORBA_Environment *ev)
{
        MateComponentCanvasComponentFactory *factory = MATECOMPONENT_CANVAS_COMPONENT_FACTORY(
              matecomponent_object_from_servant (servant));
        MateComponent_Canvas_ComponentProxy item_proxy;
        MateComponentCanvasComponent *component;
        MateCanvas *pseudo_canvas;

        if (factory->priv->item_creator == NULL)
                return CORBA_OBJECT_NIL;

        item_proxy = CORBA_Object_duplicate (_item_proxy, ev);

	pseudo_canvas = matecomponent_canvas_new (aa, item_proxy);

        component = (*factory->priv->item_creator)(
                /*factory,*/ pseudo_canvas, factory->priv->item_creator_data);

        return matecomponent_object_dup_ref (MATECOMPONENT_OBJREF (component), ev);
}

static void
matecomponent_canvas_component_factory_class_init (
      MateComponentCanvasComponentFactoryClass *klass)
{
        POA_MateComponent_CanvasComponentFactory__epv *epv = &klass->epv;

        gccf_parent_class = g_type_class_peek_parent(klass);
        epv->createCanvasComponent = 
           impl_MateComponent_canvas_component_factory_createCanvasItem;
}

static void
matecomponent_canvas_component_factory_init (MateComponentCanvasComponentFactory *factory)
{
        factory->priv = g_new0 (MateComponentCanvasComponentFactoryPrivate, 1);
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentCanvasComponentFactory,
                       MateComponent_CanvasComponentFactory,
                       MATECOMPONENT_TYPE_X_OBJECT,
                       matecomponent_canvas_component_factory)

/**
 * matecomponent_canvas_component_factory_new:
 * @item_factory: A callback invoke when the container activates the object.
 * @user_data: User data pointer.
 *
 * Returns: The object to be passed into matecomponent_generic_factory_main.
 */
MateComponentObject *matecomponent_canvas_component_factory_new(
      MateItemCreator item_factory, void *user_data)
{
        MateComponentCanvasComponentFactory *factory;
        factory = g_object_new (MATECOMPONENT_CANVAS_COMPONENT_FACTORY_TYPE, NULL);

        factory->priv->item_creator = item_factory;
        factory->priv->item_creator_data = user_data;

        return MATECOMPONENT_OBJECT(factory); 
}
