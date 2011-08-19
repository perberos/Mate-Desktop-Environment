/*  Copyright (c) 2008-2009 Robert Ancell
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

#include <string.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "math-display.h"

enum {
    PROP_0,
    PROP_EQUATION
};

struct MathDisplayPrivate
{
    /* Equation being displayed */
    MathEquation *equation;

    /* Display widget */
    GtkWidget *text_view;

    /* Buffer that shows errors etc */
    GtkTextBuffer *info_buffer;
};

G_DEFINE_TYPE (MathDisplay, math_display, GTK_TYPE_VBOX);

#define GET_WIDGET(ui, name)  GTK_WIDGET(gtk_builder_get_object(ui, name))

MathDisplay *
math_display_new()
{
    return g_object_new (math_display_get_type(), "equation", math_equation_new(), NULL);
}


MathDisplay *
math_display_new_with_equation(MathEquation *equation)
{
    return g_object_new (math_display_get_type(), "equation", equation, NULL);
}


MathEquation *
math_display_get_equation(MathDisplay *display)
{
    return display->priv->equation;
}


static gboolean
display_key_press_cb(GtkWidget *widget, GdkEventKey *event, MathDisplay *display)
{
    int state;
    guint32 c;

    state = event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK);
    c = gdk_keyval_to_unicode(event->keyval);

    /* Solve on enter */
    if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter) {
        math_equation_solve(display->priv->equation);
        return TRUE;
    }

    /* Clear on escape */
    if ((event->keyval == GDK_Escape && state == 0) ||
        (event->keyval == GDK_BackSpace && state == GDK_CONTROL_MASK) ||
        (event->keyval == GDK_Delete && state == GDK_SHIFT_MASK)) {
        math_equation_clear(display->priv->equation);
        return TRUE;
    }

    /* Substitute */
    if (state == 0) {
        if (c == '*') {
            math_equation_insert(display->priv->equation, "×");
            return TRUE;
        }
        if (c == '/') {
            math_equation_insert(display->priv->equation, "÷");
            return TRUE;
        }
        if (c == '-') {
            math_equation_insert_subtract(display->priv->equation);
            return TRUE;
        }
    }

    /* Shortcuts */
    if (state == GDK_CONTROL_MASK) {
        switch(event->keyval)
        {
        case GDK_bracketleft:
            math_equation_insert(display->priv->equation, "⌈");
            return TRUE;
        case GDK_bracketright:
            math_equation_insert(display->priv->equation, "⌉");
            return TRUE;
        case GDK_e:
            math_equation_insert_exponent(display->priv->equation);
            return TRUE;
        case GDK_f:
            math_equation_factorize(display->priv->equation);
            return TRUE;
        case GDK_i:
            math_equation_insert(display->priv->equation, "⁻¹");
            return TRUE;
        case GDK_p:
            math_equation_insert(display->priv->equation, "π");
            return TRUE;
        case GDK_r:
            math_equation_insert(display->priv->equation, "√");
            return TRUE;
        case GDK_u:
            math_equation_insert(display->priv->equation, "µ");
            return TRUE;
        case GDK_minus:
             math_equation_insert(display->priv->equation, "⁻");
             return TRUE;
        case GDK_apostrophe:
             math_equation_insert(display->priv->equation, "°");
             return TRUE;
        }
    }
    if (state == GDK_MOD1_MASK) {
        switch(event->keyval)
        {
        case GDK_bracketleft:
            math_equation_insert(display->priv->equation, "⌊");
            return TRUE;
        case GDK_bracketright:
            math_equation_insert(display->priv->equation, "⌋");
            return TRUE;
        }
    }

    if (state == GDK_CONTROL_MASK || math_equation_get_number_mode(display->priv->equation) == SUPERSCRIPT) {
        switch(event->keyval)
        {
        case GDK_0:
            math_equation_insert(display->priv->equation, "⁰");
            return TRUE;
        case GDK_1:
            math_equation_insert(display->priv->equation, "¹");
            return TRUE;
        case GDK_2:
            math_equation_insert(display->priv->equation, "²");
            return TRUE;
        case GDK_3:
            math_equation_insert(display->priv->equation, "³");
            return TRUE;
        case GDK_4:
            math_equation_insert(display->priv->equation, "⁴");
            return TRUE;
        case GDK_5:
            math_equation_insert(display->priv->equation, "⁵");
            return TRUE;
        case GDK_6:
            math_equation_insert(display->priv->equation, "⁶");
            return TRUE;
        case GDK_7:
            math_equation_insert(display->priv->equation, "⁷");
            return TRUE;
        case GDK_8:
            math_equation_insert(display->priv->equation, "⁸");
            return TRUE;
        case GDK_9:
            math_equation_insert(display->priv->equation, "⁹");
            return TRUE;
        }
    }
    else if (state == GDK_MOD1_MASK || math_equation_get_number_mode(display->priv->equation) == SUBSCRIPT) {
        switch(event->keyval)
        {
        case GDK_0:
            math_equation_insert(display->priv->equation, "₀");
            return TRUE;
        case GDK_1:
            math_equation_insert(display->priv->equation, "₁");
            return TRUE;
        case GDK_2:
            math_equation_insert(display->priv->equation, "₂");
            return TRUE;
        case GDK_3:
            math_equation_insert(display->priv->equation, "₃");
            return TRUE;
        case GDK_4:
            math_equation_insert(display->priv->equation, "₄");
            return TRUE;
        case GDK_5:
            math_equation_insert(display->priv->equation, "₅");
            return TRUE;
        case GDK_6:
            math_equation_insert(display->priv->equation, "₆");
            return TRUE;
        case GDK_7:
            math_equation_insert(display->priv->equation, "₇");
            return TRUE;
        case GDK_8:
            math_equation_insert(display->priv->equation, "₈");
            return TRUE;
        case GDK_9:
            math_equation_insert(display->priv->equation, "₉");
            return TRUE;
        }
    }

    return FALSE;
}


static gboolean
key_press_cb(MathDisplay *display, GdkEventKey *event)
{
    gboolean result;
    g_signal_emit_by_name(display->priv->text_view, "key-press-event", event, &result);
    return result;
}


static void
status_changed_cb(MathEquation *equation, GParamSpec *spec, MathDisplay *display)
{
    gtk_text_buffer_set_text(display->priv->info_buffer, math_equation_get_status(equation), -1);
}


static void
create_gui(MathDisplay *display)
{
    GtkWidget *info_view;
    PangoFontDescription *font_desc;
  
    g_signal_connect(display, "key-press-event", G_CALLBACK(key_press_cb), display);

    display->priv->text_view = gtk_text_view_new_with_buffer(GTK_TEXT_BUFFER(display->priv->equation));
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(display->priv->text_view), GTK_WRAP_WORD);
    gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(display->priv->text_view), FALSE);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(display->priv->text_view), 8);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(display->priv->text_view), 2);
    /* TEMP: Disabled for now as GTK+ doesn't properly render a right aligned right margin, see bug #482688 */
    /*gtk_text_view_set_right_margin(GTK_TEXT_VIEW(display->priv->text_view), 6);*/
    gtk_text_view_set_justification(GTK_TEXT_VIEW(display->priv->text_view), GTK_JUSTIFY_RIGHT);
    gtk_widget_ensure_style(display->priv->text_view);
    font_desc = pango_font_description_copy(gtk_widget_get_style(display->priv->text_view)->font_desc);
    pango_font_description_set_size(font_desc, 16 * PANGO_SCALE);
    gtk_widget_modify_font(display->priv->text_view, font_desc);
    pango_font_description_free(font_desc);
    gtk_widget_set_name(display->priv->text_view, "displayitem");
    atk_object_set_role(gtk_widget_get_accessible(display->priv->text_view), ATK_ROLE_EDITBAR);
  //FIXME:<property name="AtkObject::accessible-description" translatable="yes" comments="Accessible description for the area in which results are displayed">Result Region</property>
    g_signal_connect(display->priv->text_view, "key-press-event", G_CALLBACK(display_key_press_cb), display);
    gtk_box_pack_start(GTK_BOX(display), display->priv->text_view, TRUE, TRUE, 0);
  
    info_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(info_view), GTK_WRAP_WORD);
    gtk_widget_set_can_focus(info_view, TRUE); // FIXME: This should be FALSE but it locks the cursor inside the main view for some reason
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(info_view), FALSE); // FIXME: Just here so when incorrectly gets focus doesn't look editable
    gtk_text_view_set_editable(GTK_TEXT_VIEW(info_view), FALSE);
    gtk_text_view_set_justification(GTK_TEXT_VIEW(info_view), GTK_JUSTIFY_RIGHT);
    /* TEMP: Disabled for now as GTK+ doesn't properly render a right aligned right margin, see bug #482688 */
    /*gtk_text_view_set_right_margin(GTK_TEXT_VIEW(info_view), 6);*/
    gtk_box_pack_start(GTK_BOX(display), info_view, FALSE, TRUE, 0);
    display->priv->info_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_view));

    gtk_widget_show(info_view);
    gtk_widget_show(display->priv->text_view);

    g_signal_connect(display->priv->equation, "notify::status", G_CALLBACK(status_changed_cb), display);
    status_changed_cb(display->priv->equation, NULL, display);
}


static void
math_display_set_property(GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    MathDisplay *self;

    self = MATH_DISPLAY (object);

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
math_display_get_property(GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    MathDisplay *self;

    self = MATH_DISPLAY (object);

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
math_display_class_init (MathDisplayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = math_display_get_property;
    object_class->set_property = math_display_set_property;

    g_type_class_add_private (klass, sizeof (MathDisplayPrivate));

    g_object_class_install_property(object_class,
                                    PROP_EQUATION,
                                    g_param_spec_object("equation",
                                                        "equation",
                                                        "Equation being displayed",
                                                        math_equation_get_type(),
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void 
math_display_init(MathDisplay *display)
{
    display->priv = G_TYPE_INSTANCE_GET_PRIVATE (display, math_display_get_type(), MathDisplayPrivate);
}
