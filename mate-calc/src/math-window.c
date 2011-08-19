/*  Copyright (c) 1987-2008 Sun Microsystems, Inc. All Rights Reserved.
 *  Copyright (c) 2008-2009 Robert Ancell
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "math-window.h"
#include "math-preferences.h"

enum {
    PROP_0,
    PROP_EQUATION
};

struct MathWindowPrivate
{
    GtkWidget *menu_bar;
    MathEquation *equation;
    MathDisplay *display;
    MathButtons *buttons;
    MathPreferencesDialog *preferences_dialog;
    gboolean right_aligned;
    GtkWidget *mode_basic_menu_item, *mode_advanced_menu_item, *mode_financial_menu_item, *mode_programming_menu_item;
};

G_DEFINE_TYPE (MathWindow, math_window, GTK_TYPE_WINDOW);

enum {
    QUIT,
    LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };


MathWindow *
math_window_new(MathEquation *equation)
{
    return g_object_new (math_window_get_type(), "equation", equation, NULL);
}


GtkWidget *
math_window_get_menu_bar(MathWindow *window)
{
   return window->priv->menu_bar;
}


MathEquation *math_window_get_equation(MathWindow *window)
{
    return window->priv->equation;
}


MathDisplay *
math_window_get_display(MathWindow *window)
{
    return window->priv->display;
}


MathButtons *
math_window_get_buttons(MathWindow *window)
{
    return window->priv->buttons;
}


void
math_window_critical_error(MathWindow *window, const gchar *title, const gchar *contents)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(NULL, 0,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_NONE,
                                    "%s", title);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                             "%s", contents);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_QUIT, GTK_RESPONSE_ACCEPT, NULL);

    gtk_dialog_run(GTK_DIALOG(dialog));

    g_signal_emit(window, signals[QUIT], 0);
}


static void
copy_cb(GtkWidget *widget, MathWindow *window)
{
    math_equation_copy (window->priv->equation);  
}


static void
paste_cb(GtkWidget *widget, MathWindow *window)
{
    math_equation_paste (window->priv->equation);
}


static void
undo_cb(GtkWidget *widget, MathWindow *window)
{
    math_equation_undo(window->priv->equation);
}


static void
redo_cb(GtkWidget *widget, MathWindow *window)
{
    math_equation_redo(window->priv->equation);
}


static void
mode_changed_cb(GtkWidget *menu, MathWindow *window)
{
    int mode;

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)))
        return;

    mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu), "calcmode"));
    math_buttons_set_mode(window->priv->buttons, mode);
}


static void
show_preferences_cb(GtkMenuItem *menu, MathWindow *window)
{
    if (!window->priv->preferences_dialog) {
        window->priv->preferences_dialog = math_preferences_dialog_new(window->priv->equation);
        gtk_window_set_transient_for(GTK_WINDOW(window->priv->preferences_dialog), GTK_WINDOW(window));
    }
    gtk_window_present(GTK_WINDOW(window->priv->preferences_dialog));
}


static void
help_cb(GtkWidget *widget, MathWindow *window)
{
    GdkScreen *screen;
    GError *error = NULL;

    screen = gtk_widget_get_screen (GTK_WIDGET (window));
    gtk_show_uri (screen, "ghelp:gcalctool", gtk_get_current_event_time (), &error);

    if (error != NULL)
    {
        GtkWidget *d;
        /* Translators: Error message displayed when unable to launch help browser */
        const char *message = _("Unable to open help file");

        d = gtk_message_dialog_new (GTK_WINDOW (window),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                    "%s", message);
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (d),
                                                  "%s", error->message);
        g_signal_connect (d, "response", G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_window_present (GTK_WINDOW (d));

        g_error_free (error);
    }
}


static void
about_cb(GtkWidget *widget, MathWindow *window)
{
    const gchar *authors[] = {
        "Rich Burridge <rich.burridge@sun.com>",
        "Robert Ancell <robert.ancell@gmail.com>",
        "Klaus Niederkrüger <kniederk@umpa.ens-lyon.fr>",
        NULL
    };
    const gchar *documenters[] = {
        "Sun Microsystems",
        NULL
    };

    /* The translator credits. Please translate this with your name(s). */
    const gchar *translator_credits = _("translator-credits");

    /* The license this software is under (GPL2+) */
    char *license = _("Gcalctool is free software; you can redistribute it and/or modify\n"
          "it under the terms of the GNU General Public License as published by\n"
          "the Free Software Foundation; either version 2 of the License, or\n"
          "(at your option) any later version.\n"
          "\n"
          "Gcalctool is distributed in the hope that it will be useful,\n"
          "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
          "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
          "GNU General Public License for more details.\n"
          "\n"
          "You should have received a copy of the GNU General Public License\n"
          "along with Gcalctool; if not, write to the Free Software Foundation, Inc.,\n"
          "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA");

    gtk_show_about_dialog(GTK_WINDOW(window),
                          "name",
                          /* Program name in the about dialog */
                          _("Gcalctool"),
                          "version", VERSION,
                          "copyright",
                          /* Copyright notice in the about dialog */
                          _("\xc2\xa9 1986–2010 The Gcalctool authors"),
                          "license", license,
                          "comments",
                          /* Short description in the about dialog */
                          _("Calculator with financial and scientific modes."),
                          "authors", authors,
                          "documenters", documenters,
                          "translator_credits", translator_credits,
                          "logo-icon-name", "accessories-calculator",
                          NULL);
}


static void
quit_cb(GtkWidget *widget, MathWindow *window)
{
    g_signal_emit(window, signals[QUIT], 0);
}


static gboolean
key_press_cb(MathWindow *window, GdkEventKey *event)
{
    gboolean result;
    g_signal_emit_by_name(window->priv->display, "key-press-event", event, &result);
    return result;
}


static void
delete_cb(MathWindow *window, GdkEvent *event)
{
    g_signal_emit(window, signals[QUIT], 0);
}


static void
scroll_changed_cb(GtkAdjustment *adjustment, MathWindow *window)
{
    if (window->priv->right_aligned)
        gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));
}


static void
scroll_value_changed_cb(GtkAdjustment *adjustment, MathWindow *window)
{
    if (gtk_adjustment_get_value(adjustment) == gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment))
        window->priv->right_aligned = TRUE;
    else
        window->priv->right_aligned = FALSE;
}


static void
button_mode_changed_cb(MathButtons *buttons, GParamSpec *spec, MathWindow *window)
{
    GtkWidget *menu;

    switch(math_buttons_get_mode(buttons))
    {
    default:
    case BASIC:
      menu = window->priv->mode_basic_menu_item;
      //FIXME: Should it revert to decimal mode? math_equation_set_number_format(window->priv->equation, DEC);
      break;

    case ADVANCED:
      menu = window->priv->mode_advanced_menu_item;
      break;

    case FINANCIAL:
      menu = window->priv->mode_financial_menu_item;
      break;

    case PROGRAMMING:
      menu = window->priv->mode_programming_menu_item;
      break;
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
}


static GtkWidget *
add_menu(GtkWidget *menu_bar, const gchar *name)
{
    GtkWidget *menu_item, *menu;

    menu_item = gtk_menu_item_new_with_mnemonic(name);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
    gtk_widget_show(menu_item);
    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

    return menu;
}


static GtkWidget *
add_menu_item(GtkWidget *menu, GtkWidget *menu_item, GCallback callback, gpointer callback_data)
{
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    if (callback)
        g_signal_connect(G_OBJECT(menu_item), "activate", callback, callback_data);
    return menu_item;
}


static GtkWidget *
radio_menu_item_new(GSList **group, const gchar *name)
{
    GtkWidget *menu_item;
    menu_item = gtk_radio_menu_item_new_with_mnemonic(*group, name);
    *group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
    return menu_item;
}


static void
create_menu(MathWindow *window)
{
    GtkAccelGroup *accel_group;
    GtkWidget *menu, *menu_item;
    GSList *group = NULL;
  
    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    /* Calculator menu */  
    #define CALCULATOR_MENU_LABEL _("_Calculator")
    /* Mode menu */
    #define MODE_MENU_LABEL _("_Mode")
    /* Help menu label */
    #define HELP_MENU_LABEL _("_Help")
    /* Basic menu label */  
    #define MODE_BASIC_LABEL _("_Basic")
    /* Advanced menu label */  
    #define MODE_ADVANCED_LABEL _("_Advanced")
    /* Financial menu label */  
    #define MODE_FINANCIAL_LABEL _("_Financial")
    /* Programming menu label */
    #define MODE_PROGRAMMING_LABEL _("_Programming")
    /* Help>Contents menu label */
    #define HELP_CONTENTS_LABEL _("_Contents")

    menu = add_menu(window->priv->menu_bar, CALCULATOR_MENU_LABEL);
    add_menu_item(menu, gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, accel_group), G_CALLBACK(copy_cb), window);
    add_menu_item(menu, gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, accel_group), G_CALLBACK(paste_cb), window);
    menu_item = add_menu_item(menu, gtk_image_menu_item_new_from_stock(GTK_STOCK_UNDO, accel_group), G_CALLBACK(undo_cb), window);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group, GDK_z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    menu_item = add_menu_item(menu, gtk_image_menu_item_new_from_stock(GTK_STOCK_REDO, accel_group), G_CALLBACK(redo_cb), window);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group, GDK_z, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
    add_menu_item(menu, gtk_separator_menu_item_new(), NULL, NULL);
    add_menu_item(menu, gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, accel_group), G_CALLBACK(show_preferences_cb), window);
    add_menu_item(menu, gtk_separator_menu_item_new(), NULL, NULL);
    menu_item = add_menu_item(menu, gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group), G_CALLBACK(quit_cb), window);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group, GDK_w, GDK_CONTROL_MASK, 0);

    menu = add_menu(window->priv->menu_bar, MODE_MENU_LABEL);
    window->priv->mode_basic_menu_item = add_menu_item(menu, radio_menu_item_new(&group, MODE_BASIC_LABEL), G_CALLBACK(mode_changed_cb), window);
    g_object_set_data(G_OBJECT(window->priv->mode_basic_menu_item), "calcmode", GINT_TO_POINTER(BASIC));
    window->priv->mode_advanced_menu_item = add_menu_item(menu, radio_menu_item_new(&group, MODE_ADVANCED_LABEL), G_CALLBACK(mode_changed_cb), window);
    g_object_set_data(G_OBJECT(window->priv->mode_advanced_menu_item), "calcmode", GINT_TO_POINTER(ADVANCED));
    window->priv->mode_financial_menu_item = add_menu_item(menu, radio_menu_item_new(&group, MODE_FINANCIAL_LABEL), G_CALLBACK(mode_changed_cb), window);
    g_object_set_data(G_OBJECT(window->priv->mode_financial_menu_item), "calcmode", GINT_TO_POINTER(FINANCIAL));
    window->priv->mode_programming_menu_item = add_menu_item(menu, radio_menu_item_new(&group, MODE_PROGRAMMING_LABEL), G_CALLBACK(mode_changed_cb), window);
    g_object_set_data(G_OBJECT(window->priv->mode_programming_menu_item), "calcmode", GINT_TO_POINTER(PROGRAMMING));

    menu = add_menu(window->priv->menu_bar, HELP_MENU_LABEL);
    menu_item = add_menu_item(menu, gtk_menu_item_new_with_mnemonic(HELP_CONTENTS_LABEL), G_CALLBACK(help_cb), window);
    gtk_widget_add_accelerator(menu_item, "activate", accel_group, GDK_F1, 0, GTK_ACCEL_VISIBLE);
    add_menu_item(menu, gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, accel_group), G_CALLBACK(about_cb), window);
}


static void
create_gui(MathWindow *window)
{
    GtkWidget *main_vbox, *vbox;
    GtkWidget *scrolled_window;
  
    main_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);
    gtk_widget_show(main_vbox);

    window->priv->menu_bar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(main_vbox), window->priv->menu_bar, TRUE, TRUE, 0);
    gtk_widget_show(window->priv->menu_bar);
  
    create_menu(window);

    vbox = gtk_vbox_new(FALSE, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
    gtk_box_pack_start(GTK_BOX(main_vbox), vbox, TRUE, TRUE, 0);  
    gtk_widget_show(vbox);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(scrolled_window), TRUE, TRUE, 0);
    g_signal_connect(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled_window)), "changed", G_CALLBACK(scroll_changed_cb), window);
    g_signal_connect(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled_window)), "value-changed", G_CALLBACK(scroll_value_changed_cb), window);
    window->priv->right_aligned = TRUE;
    gtk_widget_show(scrolled_window);

    window->priv->display = math_display_new_with_equation(window->priv->equation);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), GTK_WIDGET(window->priv->display));
    gtk_widget_show(GTK_WIDGET(window->priv->display));

    window->priv->buttons = math_buttons_new(window->priv->equation);
    g_signal_connect(window->priv->buttons, "notify::mode", G_CALLBACK(button_mode_changed_cb), window);
    button_mode_changed_cb(window->priv->buttons, NULL, window);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(window->priv->buttons), TRUE, TRUE, 0);
    gtk_widget_show(GTK_WIDGET(window->priv->buttons));
}


static void
math_window_set_property(GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
    MathWindow *self;

    self = MATH_WINDOW (object);

    switch (prop_id) {
    case PROP_EQUATION:
        self->priv->equation = g_value_get_object (value);
        create_gui(self);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}


static void
math_window_get_property(GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
    MathWindow *self;

    self = MATH_WINDOW (object);

    switch (prop_id) {
    case PROP_EQUATION:
        g_value_set_object (value, self->priv->equation);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}


static void
math_window_class_init (MathWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = math_window_get_property;
    object_class->set_property = math_window_set_property;

    g_type_class_add_private (klass, sizeof (MathWindowPrivate));

    g_object_class_install_property(object_class,
                                    PROP_EQUATION,
                                    g_param_spec_object("equation",
                                                        "equation",
                                                        "Equation being calculated",
                                                        math_equation_get_type(),
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    signals[QUIT] =
        g_signal_new ("quit",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MathWindowClass, quit),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}


static void
math_window_init(MathWindow *window)
{
    window->priv = G_TYPE_INSTANCE_GET_PRIVATE (window, math_window_get_type(), MathWindowPrivate);
    gtk_window_set_title(GTK_WINDOW(window),
                         /* Title of main window */
                         _("Calculator"));
    gtk_window_set_icon_name(GTK_WINDOW(window), "accessories-calculator");
    gtk_window_set_role(GTK_WINDOW(window), "gcalctool");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    g_signal_connect_after(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_cb), NULL);
    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(delete_cb), NULL);
}
