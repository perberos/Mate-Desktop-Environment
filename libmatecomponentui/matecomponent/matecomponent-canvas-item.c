/*
 * matecomponent-canvas-item.c: MateCanvasItem implementation to serve as a client-
 *			 proxy for embedding remote canvas-items.
 *
 * Author:
 *     Miguel de Icaza (miguel@kernel.org)
 *
 * (C) 1999, 2000 Ximian, Inc.
 */

/* FIXME: this needs re-writing to use MateComponentObject ! */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-control.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-canvas-item.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-types.h>
#include <matecomponent/matecomponent-main.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkprivate.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#else
#error Port to this GDK backend
#endif
#include <gtk/gtk.h>

G_DEFINE_TYPE (MateComponentCanvasItem, matecomponent_canvas_item, MATE_TYPE_CANVAS_ITEM)

typedef struct {
	POA_MateComponent_Canvas_ComponentProxy proxy_servant;
	MateCanvasItem           *item_bound;
	PortableServer_ObjectId   *oid;
	MateComponent_UIContainer	   ui_container;
} ComponentProxyServant;

struct _MateComponentCanvasItemPrivate {
	MateComponent_Canvas_Component object;
	ComponentProxyServant  *proxy;
	int               realize_pending;
};

enum {
	PROP_0,
	PROP_CORBA_FACTORY,
	PROP_CORBA_UI_CONTAINER
};

/*
 * Horizontal space saver
 */
#define GBI(x)          MATECOMPONENT_CANVAS_ITEM(x)
typedef MateComponentCanvasItem Gbi;

/*
 * Creates a MateComponent_Canvas_SVPSegment structure representing the ArtSVPSeg
 * structure, suitable for sending over the network
 */
static gboolean
art_svp_segment_to_CORBA_SVP_Segment (ArtSVPSeg *seg, MateComponent_Canvas_SVPSegment *segment)
{
	int i;
	
	segment->points._buffer = CORBA_sequence_MateComponent_Canvas_Point_allocbuf (seg->n_points);
	if (segment->points._buffer == NULL)
		return FALSE;

	segment->points._maximum = seg->n_points;
	segment->points._length = seg->n_points;
	
	if (seg->dir == 0)
		segment->up = CORBA_TRUE;
	else
		segment->up = CORBA_FALSE;

	segment->bbox.x0 = seg->bbox.x0;
	segment->bbox.x1 = seg->bbox.x1;
	segment->bbox.y0 = seg->bbox.y0;
	segment->bbox.y1 = seg->bbox.y1;

	for (i = 0; i < seg->n_points; i++){
		segment->points._buffer [i].x = seg->points [i].x;
		segment->points._buffer [i].y = seg->points [i].y;
	}

	return TRUE;
}

/*
 * Creates a MateComponent_Canvas_SVP CORBA structure from the art_svp, suitable
 * for sending over the wire
 */
static MateComponent_Canvas_SVP *
art_svp_to_CORBA_SVP (ArtSVP *art_svp)
{
	MateComponent_Canvas_SVP *svp;
	int i;
	
	svp = MateComponent_Canvas_SVP__alloc ();
	if (!svp)
		return NULL;
	
	if (art_svp){
		svp->_buffer = CORBA_sequence_MateComponent_Canvas_SVPSegment_allocbuf (art_svp->n_segs);
		if (svp->_buffer == NULL){
			svp->_length = 0;
			svp->_maximum = 0;
			return svp;
		}
		svp->_maximum = art_svp->n_segs;
		svp->_length = art_svp->n_segs;

		for (i = 0; i < art_svp->n_segs; i++){
			gboolean ok;
			
			ok = art_svp_segment_to_CORBA_SVP_Segment (
				&art_svp->segs [i], &svp->_buffer [i]);
			if (!ok){
				int j;
				
				for (j = 0; j < i; j++)
					CORBA_free (&svp->_buffer [j]);
				CORBA_free (svp);
				return NULL;
			}
		}
	} else {
		svp->_maximum = 0;
		svp->_length = 0;
	}

	return svp;
}

static ArtUta *
uta_from_cuta (MateComponent_Canvas_ArtUTA *cuta)
{
	ArtUta *uta;

	uta = art_uta_new (cuta->x0, cuta->y0, cuta->x0 + cuta->width, cuta->y0 + cuta->height);
	memcpy (uta->utiles, cuta->utiles._buffer, cuta->width * cuta->height * sizeof (ArtUtaBbox));

	return uta;
}

static void
prepare_state (MateCanvasItem *item, MateComponent_Canvas_State *target)
{
	double item_affine [6];
	MateCanvas *canvas = item->canvas;
	int i;

	mate_canvas_item_i2w_affine (item, item_affine);
	for (i = 0; i < 6; i++)
		target->item_aff [i] = item_affine [i];

	target->pixels_per_unit = canvas->pixels_per_unit;
	target->canvas_scroll_x1 = canvas->scroll_x1;
	target->canvas_scroll_y1 = canvas->scroll_y1;
	target->zoom_xofs = canvas->zoom_xofs;
	target->zoom_yofs = canvas->zoom_yofs;
}

static void
gbi_update (MateCanvasItem *item, double *item_affine,
	    ArtSVP *item_clip_path, int item_flags)
{
	Gbi *gbi = GBI (item);
	MateComponent_Canvas_affine affine;
	MateComponent_Canvas_State state;
	MateComponent_Canvas_SVP *clip_path = NULL;
	CORBA_Environment ev;
	CORBA_double x1, y1, x2, y2;
	MateComponent_Canvas_ArtUTA *cuta;
	int i;

	if (getenv ("DEBUG_BI"))
		g_message ("gbi_update");

	MATE_CANVAS_ITEM_CLASS (matecomponent_canvas_item_parent_class)->update
			   (item, item_affine, item_clip_path, item_flags);
	
	for (i = 0; i < 6; i++)
		affine [i] = item_affine [i];

	clip_path = art_svp_to_CORBA_SVP (item_clip_path);
	if (!clip_path)
		return;

	CORBA_exception_init (&ev);
	prepare_state (item, &state);

	cuta = MateComponent_Canvas_Component_update (
		gbi->priv->object,
		&state, affine, clip_path, item_flags,
		&x1, &y1, &x2, &y2,
		&ev);

	if (!MATECOMPONENT_EX (&ev)){
		if (cuta->width > 0 && cuta->height > 0){

			ArtUta *uta;

			uta = uta_from_cuta (cuta);
			mate_canvas_request_redraw_uta (item->canvas, uta);
		}

		item->x1 = x1;
		item->y1 = y1;
		item->x2 = x2;
		item->y2 = y2;

		if (getenv ("DEBUG_BI"))
			g_message ("Bbox: %g %g %g %g", x1, y1, x2, y2);

		CORBA_free (cuta);
	}
	
	CORBA_exception_free (&ev);

	CORBA_free (clip_path);
}

static void
proxy_size_allocate (MateCanvas *canvas, GtkAllocation *allocation, MateComponentCanvasItem *matecomponent_item)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	MateComponent_Canvas_Component_setCanvasSize (
		matecomponent_item->priv->object,
		allocation->x, allocation->y,
		allocation->width, allocation->height, &ev);
	CORBA_exception_free (&ev);
}

static void
gbi_realize (MateCanvasItem *item)
{
	Gbi *gbi = GBI (item);
	MateComponent_Gdk_WindowId id;

	CORBA_Environment ev;

	if (getenv ("DEBUG_BI"))
		g_message ("gbi_realize");
	
	MATE_CANVAS_ITEM_CLASS (matecomponent_canvas_item_parent_class)->realize (item);

	if (gbi->priv->object == CORBA_OBJECT_NIL) {
		gbi->priv->realize_pending = 1;
		return;
	}

        proxy_size_allocate (
                item->canvas,
                &(GTK_WIDGET (item->canvas)->allocation),
                MATECOMPONENT_CANVAS_ITEM(item));
		
	g_signal_connect (item->canvas, "size_allocate",
			  G_CALLBACK (proxy_size_allocate), item);

	CORBA_exception_init (&ev);
	gdk_flush ();

#if defined (GDK_WINDOWING_X11)
	id = matecomponent_control_window_id_from_x11
		(GDK_WINDOW_XWINDOW (item->canvas->layout.bin_window));
#elif defined (GDK_WINDOWING_WIN32)
	id = matecomponent_control_window_id_from_x11
		((guint32) GDK_WINDOW_HWND (item->canvas->layout.bin_window));
#endif

	MateComponent_Canvas_Component_realize (gbi->priv->object, id, &ev);

	CORBA_free (id);

	CORBA_exception_free (&ev);
}

static void
gbi_unrealize (MateCanvasItem *item)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	if (getenv ("DEBUG_BI"))
		g_message ("gbi_unrealize");

	if (gbi->priv->object != CORBA_OBJECT_NIL){
		CORBA_exception_init (&ev);
		MateComponent_Canvas_Component_unrealize (gbi->priv->object, &ev);
		CORBA_exception_free (&ev);
	}

	MATE_CANVAS_ITEM_CLASS (matecomponent_canvas_item_parent_class)->unrealize (item);
}

static void
gbi_draw (MateCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	MateComponent_Canvas_State state;
	MateComponent_Gdk_WindowId id;
	
	if (getenv ("DEBUG_BI"))
		g_message ("draw: %d %d %d %d", x, y, width, height);

	/*
	 * This call ensures the drawable XID is allocated on the X server
	 */
	gdk_flush ();
	CORBA_exception_init (&ev);

	prepare_state (item, &state);
#if defined (GDK_WINDOWING_X11)
	id = matecomponent_control_window_id_from_x11
		(GDK_WINDOW_XWINDOW (drawable));
#elif defined (GDK_WINDOWING_WIN32)
	id = matecomponent_control_window_id_from_x11
		((guint32) GDK_WINDOW_HWND (drawable));
#endif
	
	MateComponent_Canvas_Component_draw (
		gbi->priv->object,
		&state,
		id, x, y, width, height,
		&ev);

	CORBA_free (id);
	CORBA_exception_free (&ev);
}

static double
gbi_point (MateCanvasItem *item, double x, double y, int cx, int cy, MateCanvasItem **actual)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	if (getenv ("DEBUG_BI"))
		g_message ("gbi_point %g %g", x, y);
	
	CORBA_exception_init (&ev);
	if (MateComponent_Canvas_Component_contains (gbi->priv->object, x, y, &ev)){
		CORBA_exception_free (&ev);
		*actual = item;
		if (getenv ("DEBUG_BI"))
			g_message ("event inside");
		return 0.0;
	}
	CORBA_exception_free (&ev);

	if (getenv ("DEBUG_BI"))
		g_message ("event outside");
	*actual = NULL;
	return 1000.0;
}

static void
gbi_bounds (MateCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	MateComponent_Canvas_State state;
	
	if (getenv ("DEBUG_BI"))
		g_message ("gbi_bounds");
	
	CORBA_exception_init (&ev);
	prepare_state (item, &state);
	MateComponent_Canvas_Component_bounds (gbi->priv->object, &state, x1, y1, x2, y2, &ev);
	CORBA_exception_free (&ev);

	if (getenv ("DEBUG_BI"))
		g_message ("gbi_bounds %g %g %g %g", *x1, *y1, *x2, *y2);
}

static void
gbi_render (MateCanvasItem *item, MateCanvasBuf *buf)
{
	Gbi *gbi = GBI (item);
	MateComponent_Canvas_Buf *cbuf;
	CORBA_Environment ev;

	if (getenv ("DEBUG_BI"))
		g_message ("gbi_render (%d %d)-(%d %d)",
			buf->rect.x0, buf->rect.y0,
			buf->rect.x1, buf->rect.y1);

	cbuf = MateComponent_Canvas_Buf__alloc ();
	if (!cbuf)
		return;

	cbuf->rgb_buf._buffer = buf->buf;

#if 0
	/*
	 * Inneficient!
	 */
	if (!buf->is_buf)
		mate_canvas_buf_ensure_buf (buf);
#endif
	
	if (buf->is_buf){
		cbuf->rgb_buf._maximum = buf->buf_rowstride * (buf->rect.y1 - buf->rect.y0);
		cbuf->rgb_buf._length = buf->buf_rowstride * (buf->rect.y1 - buf->rect.y0);
		cbuf->rgb_buf._buffer = buf->buf;
		CORBA_sequence_set_release (&cbuf->rgb_buf, FALSE);
	} else {
		cbuf->rgb_buf._maximum = 0;
		cbuf->rgb_buf._length = 0;
		cbuf->rgb_buf._buffer = NULL;
	}
	cbuf->row_stride = buf->buf_rowstride;
	
	cbuf->rect.x0 = buf->rect.x0;
	cbuf->rect.x1 = buf->rect.x1;
	cbuf->rect.y0 = buf->rect.y0;
	cbuf->rect.y1 = buf->rect.y1;
	cbuf->bg_color = buf->bg_color;
	cbuf->flags =
		(buf->is_bg  ? MateComponent_Canvas_IS_BG : 0) |
		(buf->is_buf ? MateComponent_Canvas_IS_BUF : 0);
	
	CORBA_exception_init (&ev);
	MateComponent_Canvas_Component_render (gbi->priv->object, cbuf, &ev);
	if (MATECOMPONENT_EX (&ev)){
		CORBA_exception_free (&ev);
		return;
	}
	CORBA_exception_free (&ev);
	
	memcpy (buf->buf, cbuf->rgb_buf._buffer, cbuf->rgb_buf._length);
	buf->is_bg  = (cbuf->flags & MateComponent_Canvas_IS_BG) != 0;
	buf->is_buf = (cbuf->flags & MateComponent_Canvas_IS_BUF) != 0;
	
	CORBA_free (cbuf);
}

static MateComponent_Gdk_Event *
gdk_event_to_matecomponent_event (GdkEvent *event)
{
	MateComponent_Gdk_Event *e = MateComponent_Gdk_Event__alloc ();

	if (e == NULL)
		return NULL;
			
	switch (event->type){

	case GDK_FOCUS_CHANGE:
		e->_d = MateComponent_Gdk_FOCUS;
		e->_u.focus.inside = event->focus_change.in;
		return e;
			
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		e->_d = MateComponent_Gdk_KEY;

		if (event->type == GDK_KEY_PRESS)
			e->_u.key.type = MateComponent_Gdk_KEY_PRESS;
		else
			e->_u.key.type = MateComponent_Gdk_KEY_RELEASE;
		e->_u.key.time =   event->key.time;
		e->_u.key.state =  event->key.state;
		e->_u.key.keyval = event->key.keyval;
		e->_u.key.length = event->key.length;
		e->_u.key.str = CORBA_string_dup (event->key.string);
		return e;

	case GDK_MOTION_NOTIFY:
		e->_d = MateComponent_Gdk_MOTION;
		e->_u.motion.time = event->motion.time;
		e->_u.motion.x = event->motion.x;
		e->_u.motion.y = event->motion.y;
		e->_u.motion.x_root = event->motion.x_root;
		e->_u.motion.y_root = event->motion.y_root;
#ifdef FIXME
		e->_u.motion.xtilt = event->motion.xtilt;
		e->_u.motion.ytilt = event->motion.ytilt;
#endif
		e->_u.motion.state = event->motion.state;
		e->_u.motion.is_hint = event->motion.is_hint != 0;
		return e;
		
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		e->_d = MateComponent_Gdk_BUTTON;
		if (event->type == GDK_BUTTON_PRESS)
			e->_u.button.type = MateComponent_Gdk_BUTTON_PRESS;
		else if (event->type == GDK_BUTTON_RELEASE)
			e->_u.button.type = MateComponent_Gdk_BUTTON_RELEASE;
		else if (event->type == GDK_2BUTTON_PRESS)
			e->_u.button.type = MateComponent_Gdk_BUTTON_2_PRESS;
		else if (event->type == GDK_3BUTTON_PRESS)
			e->_u.button.type = MateComponent_Gdk_BUTTON_3_PRESS;
		e->_u.button.time = event->button.time;
		e->_u.button.x = event->button.x;
		e->_u.button.y = event->button.y;
		e->_u.button.x_root = event->button.x_root;
		e->_u.button.y_root = event->button.y_root;
		e->_u.button.button = event->button.button;
		return e;

	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		e->_d = MateComponent_Gdk_CROSSING;
		if (event->type == GDK_ENTER_NOTIFY)
			e->_u.crossing.type = MateComponent_Gdk_ENTER;
		else
			e->_u.crossing.type = MateComponent_Gdk_LEAVE;
		e->_u.crossing.time = event->crossing.time;
		e->_u.crossing.x = event->crossing.x;
		e->_u.crossing.y = event->crossing.y;
		e->_u.crossing.x_root = event->crossing.x_root;
		e->_u.crossing.y_root = event->crossing.y_root;
		e->_u.crossing.state = event->crossing.state;

		switch (event->crossing.mode){
		case GDK_CROSSING_NORMAL:
			e->_u.crossing.mode = MateComponent_Gdk_NORMAL;
			break;

		case GDK_CROSSING_GRAB:
			e->_u.crossing.mode = MateComponent_Gdk_GRAB;
			break;
			
		case GDK_CROSSING_UNGRAB:
			e->_u.crossing.mode = MateComponent_Gdk_UNGRAB;
			break;
		}
		return e;

	default:
		g_warning ("Unsupported event received");
	}
	return NULL;
}

static gint
gbi_event (MateCanvasItem *item, GdkEvent *event)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	MateComponent_Gdk_Event *corba_event;
	MateComponent_Canvas_State state;
	CORBA_boolean ret;
	
	if (getenv ("DEBUG_BI"))
		g_message ("gbi_event");
	
	corba_event = gdk_event_to_matecomponent_event (event);
	if (corba_event == NULL)
		return FALSE;
	
	CORBA_exception_init (&ev);
	prepare_state (item, &state);
	ret = MateComponent_Canvas_Component_event (gbi->priv->object, &state, corba_event, &ev);
	CORBA_exception_free (&ev);
	CORBA_free (corba_event);

	return (gint) ret;
}

static void
gbi_set_property (GObject      *object,
		  guint         property_id,
		  const GValue *value,
		  GParamSpec   *pspec)
{
	Gbi *gbi = GBI (object);
	MateComponent_Canvas_ComponentProxy proxy_ref;
	MateComponent_CanvasComponentFactory factory;
	CORBA_Environment ev;

	switch (property_id) {
	case PROP_CORBA_FACTORY:

		CORBA_exception_init (&ev);

		gbi->priv->object = matecomponent_object_release_unref (gbi->priv->object, &ev);

                /* FIXME: I can't make it work as corba_object. 
                   Using matecomponent_unknown for now. */
		/* factory = matecomponent_value_get_corba_object (value); */
               
		factory = matecomponent_value_get_unknown (value);

		g_return_if_fail (factory != CORBA_OBJECT_NIL);

		proxy_ref = PortableServer_POA_servant_to_reference (
				matecomponent_poa (), (void *) gbi->priv->proxy, &ev);

		gbi->priv->object = 
			MateComponent_CanvasComponentFactory_createCanvasComponent (
				factory, MATE_CANVAS_ITEM (gbi)->canvas->aa, 
				proxy_ref, &ev);

		if (ev._major != CORBA_NO_EXCEPTION)
			gbi->priv->object = CORBA_OBJECT_NIL;

		CORBA_Object_release (factory, &ev);

		CORBA_exception_free (&ev);

		if (gbi->priv->object == CORBA_OBJECT_NIL) {
			g_object_unref (gbi);
			return;
		}

		if (gbi->priv->realize_pending){
			gbi->priv->realize_pending = 0;
			gbi_realize (MATE_CANVAS_ITEM (gbi));
		}
		break;

	case PROP_CORBA_UI_CONTAINER:
		gbi->priv->proxy->ui_container = matecomponent_value_get_unknown (value);

		g_return_if_fail (gbi->priv->proxy->ui_container != CORBA_OBJECT_NIL);
		break;

	default:
		g_warning ("Unexpected arg_id %u", property_id);
		break;
	}
}

static void
gbi_finalize (GObject *object)
{
	Gbi *gbi = GBI (object);
	CORBA_Environment ev;

	if (getenv ("DEBUG_BI"))
		g_message ("gbi_finalize");

	CORBA_exception_init (&ev);

	matecomponent_object_release_unref (gbi->priv->object, &ev);

	if (gbi->priv->proxy){
		ComponentProxyServant *proxy = gbi->priv->proxy;
		
		PortableServer_POA_deactivate_object (matecomponent_poa (), proxy->oid, &ev);
		POA_MateComponent_Unknown__fini ((void *) proxy, &ev);
		CORBA_free (proxy->oid);
		g_free (proxy);
	}
	
	g_free (gbi->priv);
	CORBA_exception_free (&ev);

	G_OBJECT_CLASS (matecomponent_canvas_item_parent_class)->finalize (object);
}

static void
matecomponent_canvas_item_class_init (MateComponentCanvasItemClass *object_class)
{
	GObjectClass *gobject_class = (GObjectClass *) object_class;
	MateCanvasItemClass *item_class =
		(MateCanvasItemClass *) object_class;

	gobject_class->set_property = gbi_set_property;

	g_object_class_install_property (
		gobject_class,
		PROP_CORBA_FACTORY,
		g_param_spec_boxed (
			"corba_factory",
			_("corba factory"),
			_("The factory pointer"),
			MATECOMPONENT_TYPE_STATIC_UNKNOWN,
			G_PARAM_WRITABLE));

	g_object_class_install_property (
		gobject_class,
		PROP_CORBA_UI_CONTAINER,
		g_param_spec_boxed (
			"corba_ui_factory",
			_("corba UI container"),
			_("The User interface container"),
			MATECOMPONENT_TYPE_STATIC_UNKNOWN,
			G_PARAM_WRITABLE));
	
	gobject_class->finalize     = gbi_finalize;

	item_class->update     = gbi_update;
	item_class->realize    = gbi_realize;
	item_class->unrealize  = gbi_unrealize;
	item_class->draw       = gbi_draw;
	item_class->point      = gbi_point;
	item_class->bounds     = gbi_bounds;
	item_class->render     = gbi_render;
	item_class->event      = gbi_event;
}

static gboolean
gbi_idle_handler (MateCanvasItem *item)
{
	mate_canvas_item_request_update (item);
        return FALSE;
}

static void
impl_MateComponent_Canvas_ComponentProxy_requestUpdate (PortableServer_Servant servant,
					         CORBA_Environment     *ev)
{
	ComponentProxyServant *item_proxy = (ComponentProxyServant *) servant;

        if (item_proxy->item_bound->canvas->idle_id == 0)
        {
        	mate_canvas_item_request_update (item_proxy->item_bound);
        }
        else
        {
                /* Problem: It is possible to get here at a time when the canvas
                 * is in the middle of a do_update.  This happens because
                 * this proxy call might be waiting in the queue when
                 * the this object calls MateComponent_Canvas_Component_update.
                 *
                 * Solution:
                 * The canvas is either in its do_update routine or it is
                 * waiting to do an update.  If it is waiting this idle_handler
                 * will get called just before the canvas's handler.  If it is
                 * in the middle of a do_update, then this will queue for the
                 * next one.
                 */

                 g_idle_add_full(GDK_PRIORITY_REDRAW-6,
                                 (GSourceFunc)gbi_idle_handler, 
                                 item_proxy->item_bound, NULL);
        }

}
					    
static void
impl_MateComponent_Canvas_ComponentProxy_grabFocus (PortableServer_Servant servant,
					     guint32                mask, 
					     gint32                 cursor_type,
					     guint32                time,
					     CORBA_Environment     *ev)
{
	ComponentProxyServant *item_proxy = (ComponentProxyServant *) servant;
	GdkCursor *cursor;

	cursor = gdk_cursor_new_for_display
		(gtk_widget_get_display (GTK_WIDGET (item_proxy->item_bound->canvas)),
		 (GdkCursorType) cursor_type);

	mate_canvas_item_grab (item_proxy->item_bound, mask, cursor, time);
}

static void
impl_MateComponent_Canvas_ComponentProxy_ungrabFocus (PortableServer_Servant servant,
					       guint32 time,
					       CORBA_Environment *ev)
{
	ComponentProxyServant *item_proxy = (ComponentProxyServant *) servant;

	mate_canvas_item_ungrab (item_proxy->item_bound, time);
}

static MateComponent_UIContainer
impl_MateComponent_Canvas_ComponentProxy_getUIContainer (PortableServer_Servant servant,
						  CORBA_Environment     *ev)
{
	ComponentProxyServant *item_proxy = (ComponentProxyServant *) servant;

	g_return_val_if_fail (item_proxy->ui_container != CORBA_OBJECT_NIL,
			      CORBA_OBJECT_NIL);

	return matecomponent_object_dup_ref (item_proxy->ui_container, NULL);
}

static PortableServer_ServantBase__epv item_proxy_base_epv;

static POA_MateComponent_Canvas_ComponentProxy__epv item_proxy_epv;

static POA_MateComponent_Canvas_ComponentProxy__vepv item_proxy_vepv = {
	&item_proxy_base_epv,
	&item_proxy_epv
};

/*
 * Creates a CORBA server to handle the ComponentProxy requests, it is not
 * activated by default
 */
static ComponentProxyServant *
create_proxy (MateCanvasItem *item)
{
	ComponentProxyServant *item_proxy = g_new0 (ComponentProxyServant, 1);
	CORBA_Environment ev;
	
        item_proxy->proxy_servant.vepv = &item_proxy_vepv;

	CORBA_exception_init (&ev);
	POA_MateComponent_Canvas_ComponentProxy__init ((PortableServer_Servant) item_proxy, &ev);

	item_proxy_epv.requestUpdate  = impl_MateComponent_Canvas_ComponentProxy_requestUpdate;
	item_proxy_epv.grabFocus      = impl_MateComponent_Canvas_ComponentProxy_grabFocus;
	item_proxy_epv.ungrabFocus    = impl_MateComponent_Canvas_ComponentProxy_ungrabFocus;
	item_proxy_epv.getUIContainer = impl_MateComponent_Canvas_ComponentProxy_getUIContainer;

	item_proxy->item_bound = item;

	item_proxy->oid = PortableServer_POA_activate_object (
		matecomponent_poa (), (void *) item_proxy, &ev);

	CORBA_exception_free (&ev);

	return item_proxy;
}

static void
matecomponent_canvas_item_init (MateComponentCanvasItem *gbi)
{
	gbi->priv = g_new0 (MateComponentCanvasItemPrivate, 1);
	gbi->priv->proxy = create_proxy (MATE_CANVAS_ITEM (gbi));
}

void
matecomponent_canvas_item_set_bounds (MateComponentCanvasItem *item, double x1, double y1,
			       double x2, double y2)
{
	g_warning ("Unimplemented");
}
