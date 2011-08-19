#undef GTK_DISABLE_DEPRECATED

#include "config.h"
#include <libmate/mate-init.h>
#include <matecomponent.h>
#include <matecomponent/matecomponent-canvas-item.h>
#include <matecomponent/matecomponent-moniker-util.h>

/*
static gboolean
test_event (MateCanvasItem *item, GdkEvent *event, MateCanvasItem *item2)
{
        double item_x, item_y;
        static double last_x, last_y;
        static gboolean dragging = FALSE;

        item_x = event->button.x;
        item_y = event->button.y;

        mate_canvas_item_w2i (item->parent, &item_x, &item_y);

        switch (event->type) {
        case GDK_BUTTON_PRESS:
                printf("GDK_BUTTON_PRESS\n");
                switch (event->button.button) {
                case 1:
                        last_x = item_x;
                        last_y = item_y;

                        dragging = TRUE;
                        break;

                default:
                        break;
                }

                break;

        case GDK_MOTION_NOTIFY:
                printf("GDK_MOTION_NOTIFY\n");
                if (dragging && 
                                (event->motion.state & GDK_BUTTON1_MASK)) {
                        last_x = item_x - last_x;
                        last_y = item_y - last_y;
                        mate_canvas_item_move(item2, last_x, last_y);
                        last_x = item_x;
                        last_y = item_y;
                }
                break;

        case GDK_BUTTON_RELEASE:
                printf("GDK_BUTTON_RELEASE\n");
                dragging = FALSE;
                break;

        case GDK_ENTER_NOTIFY:
                printf("GDK_ENTER_NOTIFY\n");
                break;

        case GDK_LEAVE_NOTIFY:
                printf("GDK_LEAVE_NOTIFY\n");
                break;

        case GDK_KEY_PRESS:
                printf("GDK_KEY_PRESS\n");
                break;

        case GDK_KEY_RELEASE:
                printf("GDK_KEY_RELEASE\n");
                break;

        case GDK_FOCUS_CHANGE:
                printf("GDK_FOCUS_CHANGE\n");
                break;

        default:
                printf("OTHER\n");
                break;
        }

        return FALSE;
}

static MateCanvasGroup*
setup_return_test(MateCanvasGroup *group)
{
        MateCanvasItem *group2, *item2;

        group2 = mate_canvas_item_new (
		MATE_CANVAS_GROUP (group),
		mate_canvas_group_get_type (),
		NULL);

        item2 = mate_canvas_item_new (
		MATE_CANVAS_GROUP (group2),
		mate_canvas_rect_get_type (),
		"x1", 90.0,
		"y1", 90.0,
		"x2", 110.0,
		"y2", 110.0,
		"outline_color", "black",
		"fill_color", "red",
		NULL);

        g_signal_connect(G_OBJECT(group2), "event",
                         G_CALLBACK (test_event),
                         item2);
        return MATE_CANVAS_GROUP(group2);
}
*/

static void
on_destroy  (GtkWidget *app, void *data)
{
	g_print ("Thank you for using canvas components!\n");
        matecomponent_main_quit ();
}


MateCanvasItem* get_square(MateCanvasGroup* group)
{
        CORBA_Object server;
        MateCanvasItem *item;
        CORBA_Environment ev;
  
        CORBA_exception_init (&ev);
  
        server = matecomponent_activation_activate_from_id ("OAFIID:SquareItem", 
                0, NULL, &ev);
  
        if (server == CORBA_OBJECT_NIL || MATECOMPONENT_EX (&ev))
        {
                g_warning (_("Could not activate square: '%s'"),
                              matecomponent_exception_get_text (&ev));
                CORBA_exception_free(&ev);
                exit(0);
        }
        g_print("Got square component connect.\n");
        item = mate_canvas_item_new (group, matecomponent_canvas_item_get_type (),
                "corba_factory", server, NULL);
  
        /* I think this tells the object it is OK to exit.
        CORBA_Object_release(server, &ev);*/
        matecomponent_object_release_unref(server, &ev);
        CORBA_exception_free(&ev);

        return item;
}

MateCanvasItem* get_circle(MateCanvasGroup* group)
{
        CORBA_Object server;
        MateCanvasItem *item;
        CORBA_Environment ev;
  
        CORBA_exception_init (&ev);
  
        server = matecomponent_activation_activate_from_id ("OAFIID:CircleItem", 
                0, NULL, &ev);
  
        if (server == CORBA_OBJECT_NIL || MATECOMPONENT_EX (&ev))
        {
                g_warning (_("Could activate Circle: '%s'"),
                              matecomponent_exception_get_text (&ev));
                CORBA_exception_free(&ev);
                exit(0);
        }
        g_print("Got circle component connect.\n");

        item = mate_canvas_item_new (group, matecomponent_canvas_item_get_type (),
                "corba_factory", server, NULL);
  
        /* I think this tells the object it is OK to exit.
           Probably want to call this when I close.
           or matecomponent_object_release_unref(server, &ev)
        CORBA_Object_release(server, &ev);*/
        matecomponent_object_release_unref(server, &ev);
        CORBA_exception_free(&ev);

        return item;
}

static guint
create_app (void)
{
	GtkWidget *canvas, *window, *box, *hbox, *control; 
	
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT(window), "destroy", 
              G_CALLBACK(on_destroy), NULL);

	gtk_widget_set_usize (GTK_WIDGET(window), 400, 300);
        box = gtk_vbox_new (FALSE, 2);
        gtk_container_add(GTK_CONTAINER(window), box);

	canvas = mate_canvas_new();
        gtk_box_pack_start_defaults (GTK_BOX (box), canvas);
	get_square(mate_canvas_root(MATE_CANVAS(canvas)));
        get_circle(mate_canvas_root(MATE_CANVAS(canvas)));

        hbox = gtk_hbox_new(FALSE, 2);
        gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

        control = matecomponent_widget_new_control("OAFIID:Circle_Controller", NULL);
        gtk_box_pack_start (GTK_BOX (hbox), control, FALSE, FALSE, 0);

        control = matecomponent_widget_new_control("OAFIID:Square_Controller", NULL);
        gtk_box_pack_start (GTK_BOX (hbox), control, FALSE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET(window));

	return FALSE;
}

int 
main (int argc, char** argv)
{
        if (!matecomponent_ui_init (argv[0], VERSION, &argc, argv))
                g_error ("Could not initialize libmatecomponentui!\n");

	gtk_idle_add ((GtkFunction) create_app, NULL);

	matecomponent_main ();

	return 0;
}

