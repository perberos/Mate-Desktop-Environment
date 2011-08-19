#include "config.h"
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-canvas-item.h>
#include <libmatecanvas/mate-canvas-rect-ellipse.h>

#include "doc-view.h"

typedef struct {
	SampleDoc 		*doc;
	MateComponent_UIContainer 	 uic;
	MateCanvas		*canvas;
	MateCanvasItem		*selection;
	MateCanvasItem		*handle_group;
} SampleDocView;
	

static void
layout_changed_cb (SampleComponent *comp, MateCanvasItem *item)
{
	gdouble affine [6];

	sample_component_get_affine (comp, affine);
	mate_canvas_item_affine_absolute (item, affine);
}

static gboolean
item_pressed_cb (MateCanvasItem *item, SampleDocView *view)
{
	if (view->selection && (view->selection != item))
		mate_canvas_item_set (view->selection, 
				       "selected", FALSE, 
				       NULL);

	view->selection = item;
	mate_canvas_item_set (view->selection, "selected", TRUE, NULL);

	return TRUE;
}

static void
add_canvas_item (SampleDocView *view, SampleComponent *comp)
{
	MateCanvasItem *item;
	gdouble affine[6];

	item = mate_canvas_item_new (mate_canvas_root (view->canvas),
				      matecomponent_canvas_item_get_type (),
				      "corba_server",
				      sample_component_get_server (comp),
				      "corba_ui_container",
				      view->uic,
				      NULL);

	sample_component_get_affine (comp, affine);
	mate_canvas_item_affine_absolute (item, affine);

	g_signal_connect_data (G_OBJECT (comp), "changed", 
			       G_CALLBACK (layout_changed_cb), item, 
			       NULL, 0);

	g_signal_connect_data (G_OBJECT (item), "button_press_event", 
			       G_CALLBACK (item_pressed_cb), view, 
			       NULL, 0);

}

static void
add_components (SampleDocView *view)
{
	GList *l, *comps;

	comps = sample_doc_get_components (view->doc);

	for (l = comps; l; l = l->next)
		add_canvas_item (view, SAMPLE_COMPONENT (l->data));

	g_list_free (comps);
}

static void
background_cb (MateCanvasItem *item, GdkEvent *event, SampleDocView *view)
{
	if (view->selection && (event->button.button == 1)) {
		g_object_unref (view->handle_group);
		view->handle_group = NULL;
		view->selection = NULL;
	}
}

static void
destroy_view (GObject *obj, SampleDocView *view)
{
	g_object_unref (G_OBJECT (view->doc));
	matecomponent_object_release_unref (view->uic, NULL);
	g_free (view);
}
	
GtkWidget *
sample_doc_view_new (SampleDoc *doc, MateComponent_UIContainer uic)
{
	SampleDocView *view;
	GtkWidget *canvas;
	MateCanvasItem *bg;

	view = g_new0 (SampleDocView, 1);
	if (!view)
		return NULL;

        canvas = mate_canvas_new ();
	view->canvas = MATE_CANVAS (canvas);
	mate_canvas_set_scroll_region (view->canvas, -400.0, -300.0, 
					400.0, 300.0);
	g_signal_connect_data (G_OBJECT (canvas), "finalize",
			       G_CALLBACK (destroy_view), view,
			       NULL, 0);

	bg = mate_canvas_item_new (mate_canvas_root (view->canvas),
				    mate_canvas_rect_get_type (),
			            "x1", -400.0, "y1", -300.0,
			            "x2", 400.0, "y2", 300.0,
			            "fill_color", "white", NULL);

	g_signal_connect_data (G_OBJECT (bg), "button_press_event",
			       G_CALLBACK (background_cb), view,
			       NULL, 0);

	view->uic = matecomponent_object_dup_ref (uic, NULL);

	g_object_ref (G_OBJECT (doc));
	view->doc = doc;

	add_components (view);

	gtk_widget_show_all (canvas);

	return canvas;
}

