/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#undef GTK_DISABLE_DEPRECATED

#include "config.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libmatecanvas/libmatecanvas.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent.h>

typedef struct
{
        int state;
        int dragging;
        int timer;
        double speed;
        double pos;
        double inc;
        double last_x;
        double last_y;
        const char *color;
        GSList *list;
} CommonData;

typedef struct
{
        GtkWidget *button;
        GtkAdjustment *adj;
        MateCanvasItem *item;
} ObjectData;

static void
move_component(ObjectData *object, CommonData *com)
{
        if (object->item)
        {
                mate_canvas_item_set(object->item,
                       "y1", com->pos,
                       "y2", com->pos + 20,
                       "outline_color", "black",
                       "fill_color", com->color, NULL);
        }
}
                        

static gboolean 
move_all_components(CommonData *com)
{
        if (com->state && com->list)
        {
                com->pos += com->inc;
                if (com->pos > 50 || com->pos < -50) com->inc *= -1.0;
                if (com->inc >= 0) com->color = "blue";
                else com->color = "green";
                g_slist_foreach(com->list, (GFunc)move_component, com);
        }

        return 1;
}

static void
update_button(ObjectData *object, CommonData *com)
{
        if (object->button)
        {
                gtk_button_set_label(GTK_BUTTON(object->button), 
                                com->state ? "Stop" : "Start");
        }
}

static void
on_press (GtkWidget *button, CommonData *com)
{
        GSList *list = com->list;
        com->state = !com->state;

        g_slist_foreach(list, (GFunc)update_button, com);
}

static void
update_speed_label(ObjectData *object, int value)
{       
        if (object->adj)
        {
                gtk_adjustment_set_value(object->adj, (double)value);
        }
}

static void
set_speed(GtkAdjustment *adj, CommonData *com)
{
        int speed = adj->value > 0 ? (int) (10000/adj->value) : 0;

        if (speed != com->speed)
        {
                if (com->timer) g_source_remove(com->timer);

                if (speed > 0)
                        com->timer = g_timeout_add(speed,
                                (GSourceFunc)move_all_components, com);

                com->speed = speed;

                g_slist_foreach(com->list, (GFunc)update_speed_label, 
                                GINT_TO_POINTER((int)adj->value));
        }
}

MateComponentObject *
circle_control_new (CommonData *com)
{
        MateComponentPropertyBag  *pb;
        MateComponentControl      *control;
        GParamSpec        **pspecs;
        guint               n_pspecs;
        GtkWidget *button, *frame, *spin, *hbox, *spin_label;
        GtkObject *adj;
        GSList **list = &com->list;
        GSList *li;
        ObjectData *object = NULL;

        frame = gtk_frame_new("Circle");
        hbox = gtk_hbox_new(FALSE, 2);
        gtk_container_add(GTK_CONTAINER(frame), hbox);
        button = gtk_button_new();
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
        spin_label = gtk_label_new ("Speed:");
        gtk_box_pack_start (GTK_BOX (hbox), spin_label, FALSE, FALSE, 0);
        adj = gtk_adjustment_new(100.0, 0.0, 1000.0, 1.0, 10.0, 0.0);
        g_signal_connect(adj, "value_changed", G_CALLBACK(set_speed), com);
        spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 0.0, 0);
        gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);

        gtk_widget_show_all(frame);
        control = matecomponent_control_new (frame);

        pb = matecomponent_property_bag_new (NULL, NULL, NULL);
        matecomponent_control_set_properties (control, MATECOMPONENT_OBJREF (pb), NULL);
        matecomponent_object_unref (MATECOMPONENT_OBJECT (pb));

        g_signal_connect(button, "clicked", G_CALLBACK(on_press), com);

        pspecs = g_object_class_list_properties (
                G_OBJECT_GET_CLASS (button), &n_pspecs);

        matecomponent_property_bag_map_params (
                pb, G_OBJECT (button), (const GParamSpec **)pspecs, n_pspecs);

        g_free (pspecs);

        matecomponent_control_life_instrument (control);

        li = g_slist_last(*list);
        if (li)
        {
               object = li->data;
        }
        if (!object || object->button)
        {
                object = g_new0(ObjectData, 1);
                *list = g_slist_append(*list, object);
        }

        object->button = button;
        object->adj = GTK_ADJUSTMENT(adj);

        update_button(object, com);
        set_speed(GTK_ADJUSTMENT(adj), com);

        return MATECOMPONENT_OBJECT (control);
}

static MateComponentObject *
control_factory (MateComponentGenericFactory *this,
                 const char           *object_id,
                 gpointer             data)
{
        MateComponentObject *object = NULL;

        g_return_val_if_fail (object_id != NULL, NULL);

        if (!strcmp (object_id, "OAFIID:Circle_Controller"))
        {
                object = circle_control_new ((CommonData *) data);
        }

        return object;
}

static void
drag_component(ObjectData *object, CommonData *com)
{
        if (object->item)
        {
                mate_canvas_item_move (object->item, com->last_x, com->last_y);
        }
}

static gboolean
item_event (MateCanvasItem *item, GdkEvent *event, CommonData *com)
{
        double item_x, item_y;

        item_x = event->button.x;
        item_y = event->button.y;

        mate_canvas_item_w2i (item->parent, &item_x, &item_y);

        switch (event->type) {
        case GDK_BUTTON_PRESS:
                switch (event->button.button) {
                case 1:
                        com->last_x = item_x;
                        com->last_y = item_y;

                        com->dragging = TRUE;
                        break;

                default:
                        break;
                }

                break;

        case GDK_MOTION_NOTIFY:
                if (com->dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
                        com->last_x = item_x - com->last_x;
                        com->last_y = item_y - com->last_y;
                        g_slist_foreach(com->list, (GFunc)drag_component, com);
                        com->last_x = item_x;
                        com->last_y = item_y;
                }
                break;

        case GDK_BUTTON_RELEASE:
                com->dragging = FALSE;
                break;

        }

        return FALSE;
}

static MateCanvasItem*
canvas_item_new(MateCanvas *canvas, gpointer data)
{
	MateCanvasItem *item;
	MateCanvasItem *group;
        GSList **list = &((CommonData *)data)->list;
        GSList *li;
        ObjectData *object = NULL;

        group = mate_canvas_item_new (
		MATE_CANVAS_GROUP (mate_canvas_root (canvas)),
		mate_canvas_group_get_type (),
		NULL);

        item = mate_canvas_item_new (
		MATE_CANVAS_GROUP (group),
		mate_canvas_ellipse_get_type (),
		"x1", 0.0,
		"y1", 0.0,
		"x2", 20.0,
		"y2", 20.0,
		"outline_color", "red",
		"fill_color", "blue",
		NULL);

        li = g_slist_last(*list);
        if (li) {
                object = li->data;
        }

        if (!object || object->item) {
                object = g_new0(ObjectData, 1);
                *list = g_slist_append(*list, object);
        }

        object->item = item;

        g_signal_connect(G_OBJECT(item), "event",
                         G_CALLBACK (item_event),
                         data);

	return (group);
}

static MateComponentCanvasComponent *
item_factory (MateCanvas *canvas, gpointer data)
{
	MateCanvasItem *group;
	MateComponentCanvasComponent *component;

	group = canvas_item_new(canvas, data);

	component = matecomponent_canvas_component_new (group);

	return component;
}

static MateComponentObject *
matecomponent_item_factory (MateComponentGenericFactory *factory, const char *component,
                     gpointer data)
{
        MateComponentObject *object = NULL;

        g_return_val_if_fail (component != NULL, NULL);

        if (!strcmp (component, "OAFIID:CircleItem"))
        {
                g_print("activation requested\n");
                object = MATECOMPONENT_OBJECT(
                   matecomponent_canvas_component_factory_new (
                      item_factory, data));
        }
        else
        {
                g_print("attempted to activate w/ bad oid %s\n", component);
        }
        return object;
}

int
main (int argc, char *argv [])
{
        int retval;
        MateComponentObject *factory;
        CommonData data;

        data.state = 1;
        data.list = NULL;
        data.pos = 0.0;
        data.inc = 2.0;
        data.timer = 0;
        data.speed = 0;

        if (!matecomponent_ui_init (argv[0], VERSION, &argc, argv))
                g_error (_("Could not initialize MateComponent UI"));

        factory = MATECOMPONENT_OBJECT
		(matecomponent_generic_factory_new
			("OAFIID:CircleItem_Factory",
			 matecomponent_item_factory, &data));
        if (factory)
                matecomponent_running_context_auto_exit_unref (factory);

        retval = matecomponent_generic_factory_main
		("OAFIID:Circle_ControllerFactory",
		 control_factory, &data);

        return retval;
}
