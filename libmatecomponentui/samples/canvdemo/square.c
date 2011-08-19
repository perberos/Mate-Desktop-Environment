/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>

/* Comment the below line out and rebuild to see the difference with and 
   without MateComponent.
 */
#define WITH_MATECOMPONENT

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libmatecanvas/libmatecanvas.h>

#ifdef WITH_MATECOMPONENT
#include <matecomponent/MateComponent.h>
#include <matecomponent.h>
#endif

#define INITSPEED 100.0

typedef struct
{
        int state;
        int dragging;
        int timer;
        double pos;
        double inc;
        double last_x;
        double last_y;
        const char *color;
        GtkWidget *button;
        MateCanvasItem *item;
} ObjectData;

static int
move_component(ObjectData *object)
{
        object->pos += object->inc;
        if (object->pos > 50 || object->pos < -50) object->inc *= -1.0;
        if (object->inc >= 0) object->color = "purple";
        else object->color = "red";

        if (object->item && object->state)
        {
                mate_canvas_item_set(object->item,
                       "x1", object->pos,
                       "x2", object->pos + 20,
                       "outline_color", "orange",
                       "fill_color", object->color, NULL);
        }
        return 1;
}

static void
update_button(GtkButton *button, ObjectData *object)
{
        gtk_button_set_label(button, object->state ? "Stop" : "Start");
}

static void
on_press(GtkButton *button, ObjectData *object)
{
        object->state = !object->state;
        update_button(button, object);
}

static void
set_speed(GtkAdjustment *adj, ObjectData *object)
{
        if (object->timer) g_source_remove(object->timer);

        if (adj->value > 0)
                object->timer = g_timeout_add((int) (10000/adj->value), 
                                (GSourceFunc)move_component, object);
}

GtkWidget *
square_control_new (ObjectData *object)
{
        GtkWidget *button, *frame, *spin, *hbox, *spin_label;
        GtkObject *adj;

        frame = gtk_frame_new("Square");
        hbox = gtk_hbox_new(FALSE, 2);
        gtk_container_add(GTK_CONTAINER(frame), hbox);
        button = gtk_button_new();
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
        spin_label = gtk_label_new ("Speed:");
        gtk_box_pack_start (GTK_BOX (hbox), spin_label, FALSE, FALSE, 0);
        adj = gtk_adjustment_new(INITSPEED, 0.0, 1000.0, 1.0, 10.0, 10.0);
        spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.0, 0);
        gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);

        gtk_widget_show_all(frame);

        object->button = button;

        g_signal_connect(button, "clicked", G_CALLBACK(on_press), object);
        g_signal_connect(adj, "value_changed", G_CALLBACK(set_speed), object);

        update_button(GTK_BUTTON(button), object);
        set_speed(GTK_ADJUSTMENT(adj), object);

        return frame;
}

static void
drag_component(ObjectData *object, gpointer data)
{
        if (object->item)
        {
                mate_canvas_item_move (object->item, object->last_x,
                                object->last_y);
        }
}
 
static gboolean
item_event (MateCanvasItem *item, GdkEvent *event, ObjectData
                *object)
{
        double item_x, item_y;

        item_x = event->button.x;
        item_y = event->button.y;

        mate_canvas_item_w2i (item->parent, &item_x, &item_y);

        switch (event->type) {
        case GDK_BUTTON_PRESS:
                switch (event->button.button) {
                case 1:
                        object->last_x = item_x;
                        object->last_y = item_y;

                        object->dragging = TRUE;
                        break;

                default:
                        break;
                }

                break;

        case GDK_MOTION_NOTIFY:
                if (object->dragging && 
                                (event->motion.state & GDK_BUTTON1_MASK)) {
                        object->last_x = item_x - object->last_x;
                        object->last_y = item_y - object->last_y;
                        drag_component(object, NULL);
                        object->last_x = item_x;
                        object->last_y = item_y;
                }
                break;

        case GDK_BUTTON_RELEASE:
                object->dragging = FALSE;
                break;

        default:
                break;
        }

        return FALSE;
}

static MateCanvasItem *
canvas_item_new (MateCanvas *canvas, ObjectData *object)
{
	MateCanvasItem *group;

        group = mate_canvas_item_new (
		MATE_CANVAS_GROUP (mate_canvas_root (canvas)),
		mate_canvas_group_get_type (),
		NULL);


        object->item = mate_canvas_item_new (
		MATE_CANVAS_GROUP (group),
		mate_canvas_rect_get_type (),
		"x1", 0.0,
		"y1", 0.0,
		"x2", 20.0,
		"y2", 20.0,
		"outline_color", "violet",
		"fill_color", "brown",
		NULL);

        g_signal_connect(G_OBJECT(object->item), "event",
                         G_CALLBACK (item_event),
                         object);
	return group;
}

#ifdef WITH_MATECOMPONENT

static MateComponentObject *
control_factory (MateComponentGenericFactory *this,
                 const char           *object_id,
                 GSList               **list)
{
        MateComponentControl      *control = NULL;
        GtkWidget *widget;
        GSList *li;
        ObjectData *object = NULL;

        g_return_val_if_fail (object_id != NULL, NULL);

        if (!strcmp (object_id, "OAFIID:Square_Controller"))
        {
                li = g_slist_last(*list);

                if (li) {
                        object = li->data;
                }

                if (!object || object->button) {
                        object = g_new0(ObjectData, 1);
                        *list = g_slist_append(*list, object);
                }

                widget = square_control_new (object);
                gtk_widget_show_all(widget);
                control = matecomponent_control_new (widget);
                matecomponent_control_life_instrument (control);
        }

        return MATECOMPONENT_OBJECT(control);
}

static MateComponentCanvasComponent *
item_factory(MateCanvas *canvas, gpointer data)
{
        GSList **list = (GSList **) data;
        GSList *li;
        ObjectData *object = NULL;
        MateCanvasItem *item;

        li = g_slist_last(*list);

        if (li) {
               object = li->data;
        }

        if (!object || object->item) {
                object = g_new0(ObjectData, 1);
                *list = g_slist_append(*list, object);
        }

        object->state = 1;
        object->pos = 0.0;
        object->inc = 8.0;
 
        item = canvas_item_new(canvas, object);
        return matecomponent_canvas_component_new(item);
}

static MateComponentObject *
matecomponent_item_factory (MateComponentGenericFactory *factory, const char *component,
                     gpointer data)
{
        MateComponentObject *object = NULL;

        g_return_val_if_fail (component != NULL, NULL);

        if (!strcmp (component, "OAFIID:SquareItem")) {
                g_print("activation requested\n");

                object = MATECOMPONENT_OBJECT(
                   matecomponent_canvas_component_factory_new (
                      item_factory, data));
        }
        else {
                g_print("attempted to activate w/ bad oid %s\n", component);
        }
        return object;
}

int
main (int argc, char *argv [])
{
        char *iid, *iid2;
        int retval;
        MateComponentObject *factory;
        GSList *list = NULL;

        if (!matecomponent_ui_init (argv[0], VERSION, &argc, argv))
                g_error (_("Could not initialize MateComponent UI"));

        iid = matecomponent_activation_make_registration_id (
                "OAFIID:SquareItem_Factory",
                gdk_display_get_name (gdk_display_get_default()));

        iid2 = matecomponent_activation_make_registration_id (
                "OAFIID:Square_ControllerFactory",
                gdk_display_get_name (gdk_display_get_default()));

        factory = MATECOMPONENT_OBJECT(matecomponent_generic_factory_new
                        (iid,
                         matecomponent_item_factory, &list));
        if (factory) {
                matecomponent_running_context_auto_exit_unref(factory);
        }

        retval = matecomponent_generic_factory_main (iid2,
                        (MateComponentFactoryCallback)control_factory, &list);

        g_free (iid);
        g_free (iid2);

        return retval;
}

#else

static gboolean
quit_cb (GtkWidget *widget, GdkEventAny *event, gpointer dummy)
{
	gtk_main_quit ();

	return TRUE;
}

int
main (int argc, char *argv[])
{
	GtkWidget *app, *canvas, *box, *hbox, *control; 
        ObjectData object;

	gtk_init (&argc, &argv);

        memset(&object, 0, sizeof(object));
        object.state = 1;
        object.inc = 8.0;

	app = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_widget_set_usize (GTK_WIDGET(app), 400, 300);
	g_signal_connect(app, "delete_event", G_CALLBACK(quit_cb), NULL);
        box = gtk_vbox_new (FALSE, 2);
        gtk_container_add(GTK_CONTAINER(app), box);

	canvas = mate_canvas_new();
        gtk_box_pack_start_defaults (GTK_BOX (box), canvas);
        canvas_item_new(MATE_CANVAS(canvas), &object);

        hbox = gtk_hbox_new(FALSE, 2);
        gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

        control = square_control_new(&object);
        gtk_box_pack_start (GTK_BOX (hbox), control, FALSE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET(app));

        g_signal_connect(app, "delete_event", G_CALLBACK(quit_cb), NULL);

	gtk_main ();

	return 0;
}

#endif
