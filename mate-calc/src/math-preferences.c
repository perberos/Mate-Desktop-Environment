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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "math-preferences.h"

G_DEFINE_TYPE (MathPreferencesDialog, math_preferences, GTK_TYPE_DIALOG);

enum {
    PROP_0,
    PROP_EQUATION
};

struct MathPreferencesDialogPrivate
{
    MathEquation *equation;
    GtkBuilder *ui;
};

#define UI_DIALOGS_FILE  UI_DIR "/preferences.ui"
#define GET_WIDGET(ui, name) \
          GTK_WIDGET(gtk_builder_get_object(ui, name))


MathPreferencesDialog *
math_preferences_dialog_new(MathEquation *equation)
{  
    return g_object_new (math_preferences_get_type(), "equation", equation, NULL);
}


static void
preferences_response_cb(GtkWidget *widget, gint id)
{
    gtk_widget_hide(widget);
}


static gboolean
preferences_dialog_delete_cb(GtkWidget *widget, GdkEvent *event)
{
    preferences_response_cb(widget, 0);
    return TRUE;
}


void number_format_combobox_changed_cb(GtkWidget *combo, MathPreferencesDialog *dialog);
G_MODULE_EXPORT
void
number_format_combobox_changed_cb(GtkWidget *combo, MathPreferencesDialog *dialog)
{
    DisplayFormat value;
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, 1, &value, -1);
    math_equation_set_number_format(dialog->priv->equation, value);
}


void angle_unit_combobox_changed_cb(GtkWidget *combo, MathPreferencesDialog *dialog);
G_MODULE_EXPORT
void
angle_unit_combobox_changed_cb(GtkWidget *combo, MathPreferencesDialog *dialog)
{
    MPAngleUnit value;
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, 1, &value, -1);
    math_equation_set_angle_units(dialog->priv->equation, value);
}


void word_size_combobox_changed_cb(GtkWidget *combo, MathPreferencesDialog *dialog);
G_MODULE_EXPORT
void
word_size_combobox_changed_cb(GtkWidget *combo, MathPreferencesDialog *dialog)
{
    gint value;
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, 1, &value, -1);
    math_equation_set_word_size(dialog->priv->equation, value);
}


void decimal_places_spin_change_value_cb(GtkWidget *spin, MathPreferencesDialog *dialog);
G_MODULE_EXPORT
void
decimal_places_spin_change_value_cb(GtkWidget *spin, MathPreferencesDialog *dialog)
{
    gint value = 0;

    value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
    math_equation_set_accuracy(dialog->priv->equation, value);
}


void thousands_separator_check_toggled_cb(GtkWidget *check, MathPreferencesDialog *dialog);
G_MODULE_EXPORT
void
thousands_separator_check_toggled_cb(GtkWidget *check, MathPreferencesDialog *dialog)
{
    gboolean value;

    value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
    math_equation_set_show_thousands_separators(dialog->priv->equation, value);
}


void trailing_zeroes_check_toggled_cb(GtkWidget *check, MathPreferencesDialog *dialog);
G_MODULE_EXPORT
void
trailing_zeroes_check_toggled_cb(GtkWidget *check, MathPreferencesDialog *dialog)
{
    gboolean value;

    value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
    math_equation_set_show_trailing_zeroes(dialog->priv->equation, value);
}


static void
set_combo_box_from_int(GtkWidget *combo, int value)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    valid = gtk_tree_model_get_iter_first(model, &iter);

    while (valid) {
        gint v;

        gtk_tree_model_get(model, &iter, 1, &v, -1);
        if (v == value)
            break;
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    if (!valid)
        valid = gtk_tree_model_get_iter_first(model, &iter);

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
}


static void
accuracy_cb(MathEquation *equation, GParamSpec *spec, MathPreferencesDialog *dialog)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(dialog->priv->ui, "decimal_places_spin")),
                              math_equation_get_accuracy(equation));
}


static void
show_thousands_separators_cb(MathEquation *equation, GParamSpec *spec, MathPreferencesDialog *dialog)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialog->priv->ui, "thousands_separator_check")),
                                 math_equation_get_show_thousands_separators(equation));
}


static void
show_trailing_zeroes_cb(MathEquation *equation, GParamSpec *spec, MathPreferencesDialog *dialog)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialog->priv->ui, "trailing_zeroes_check")),
                                 math_equation_get_show_trailing_zeroes(equation));
}


static void
number_format_cb(MathEquation *equation, GParamSpec *spec, MathPreferencesDialog *dialog)
{
    set_combo_box_from_int(GET_WIDGET(dialog->priv->ui, "number_format_combobox"), math_equation_get_number_format(equation));
}


static void
word_size_cb(MathEquation *equation, GParamSpec *spec, MathPreferencesDialog *dialog)
{
    set_combo_box_from_int(GET_WIDGET(dialog->priv->ui, "word_size_combobox"), math_equation_get_word_size(equation));
}


static void
angle_unit_cb(MathEquation *equation, GParamSpec *spec, MathPreferencesDialog *dialog)
{
    set_combo_box_from_int(GET_WIDGET(dialog->priv->ui, "angle_unit_combobox"), math_equation_get_angle_units(equation));  
}


static void
create_gui(MathPreferencesDialog *dialog)
{
    GtkWidget *widget;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    gchar *string, **tokens;
    GError *error = NULL;
    static gchar *objects[] = { "preferences_table", "angle_unit_model", "number_format_model",
                                "word_size_model", "decimal_places_adjustment", "number_base_model", NULL };

    // FIXME: Handle errors
    dialog->priv->ui = gtk_builder_new();
    gtk_builder_add_objects_from_file(dialog->priv->ui, UI_DIALOGS_FILE, objects, &error);
    if (error)
        g_warning("Error loading preferences UI: %s", error->message);
    g_clear_error(&error);

    gtk_window_set_title(GTK_WINDOW(dialog),
                         /* Title of preferences dialog */
                         _("Preferences"));
    gtk_window_set_icon_name(GTK_WINDOW(dialog), "accessories-calculator");
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 8);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          /* Label on close button in preferences dialog */
                          _("_Close"), 0);
    g_signal_connect(dialog, "response", G_CALLBACK(preferences_response_cb), NULL);
    g_signal_connect(dialog, "delete-event", G_CALLBACK(preferences_dialog_delete_cb), NULL);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), GET_WIDGET(dialog->priv->ui, "preferences_table"), TRUE, TRUE, 0);

    widget = GET_WIDGET(dialog->priv->ui, "angle_unit_combobox");
    model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                       /* Preferences dialog: Angle unit combo box: Use degrees for trigonometric calculations */
                       _("Degrees"), 1, MP_DEGREES, -1);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                       /* Preferences dialog: Angle unit combo box: Use radians for trigonometric calculations */
                       _("Radians"), 1, MP_RADIANS, -1);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                       /* Preferences dialog: Angle unit combo box: Use gradians for trigonometric calculations */
                       _("Gradians"), 1, MP_GRADIANS, -1);
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", 0);

    widget = GET_WIDGET(dialog->priv->ui, "number_format_combobox");
    model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                       /* Number display mode combo: Fixed, e.g. 1234 */
                       _("Fixed"), 1, FIX, -1);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                       /* Number display mode combo: Scientific, e.g. 1.234×10^3 */
                       _("Scientific"), 1, SCI, -1);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                       /* Number display mode combo: Engineering, e.g. 1.234k */
                       _("Engineering"), 1, ENG, -1);
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", 0);

    widget = GET_WIDGET(dialog->priv->ui, "word_size_combobox");
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", 0);

    /* Label used in preferences dialog.  The %d is replaced by a spinbutton */
    string = _("Show %d decimal _places");
    tokens = g_strsplit(string, "%d", 2);
    widget = GET_WIDGET(dialog->priv->ui, "decimal_places_label1");
    if (tokens[0])
        string = g_strstrip(tokens[0]);
    else
        string = "";
    if (string[0] != '\0')
        gtk_label_set_text_with_mnemonic(GTK_LABEL(widget), string);
    else
        gtk_widget_hide(widget);

    widget = GET_WIDGET(dialog->priv->ui, "decimal_places_label2");
    if (tokens[0] && tokens[1])
        string = g_strstrip(tokens[1]);
    else
        string = "";
    if (string[0] != '\0')
        gtk_label_set_text_with_mnemonic(GTK_LABEL(widget), string);
    else
        gtk_widget_hide(widget);

    g_strfreev(tokens);

    gtk_builder_connect_signals(dialog->priv->ui, dialog);

    g_signal_connect(dialog->priv->equation, "notify::accuracy", G_CALLBACK(accuracy_cb), dialog);
    g_signal_connect(dialog->priv->equation, "notify::show-thousands-separators", G_CALLBACK(show_thousands_separators_cb), dialog);
    g_signal_connect(dialog->priv->equation, "notify::show-trailing_zeroes", G_CALLBACK(show_trailing_zeroes_cb), dialog);
    g_signal_connect(dialog->priv->equation, "notify::number-format", G_CALLBACK(number_format_cb), dialog);
    g_signal_connect(dialog->priv->equation, "notify::word-size", G_CALLBACK(word_size_cb), dialog);
    g_signal_connect(dialog->priv->equation, "notify::angle-units", G_CALLBACK(angle_unit_cb), dialog);

    accuracy_cb(dialog->priv->equation, NULL, dialog);
    show_thousands_separators_cb(dialog->priv->equation, NULL, dialog);
    show_trailing_zeroes_cb(dialog->priv->equation, NULL, dialog);
    number_format_cb(dialog->priv->equation, NULL, dialog);
    word_size_cb(dialog->priv->equation, NULL, dialog);
    angle_unit_cb(dialog->priv->equation, NULL, dialog);
}


static void
math_preferences_set_property(GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    MathPreferencesDialog *self;

    self = MATH_PREFERENCES (object);

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
math_preferences_get_property(GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    MathPreferencesDialog *self;

    self = MATH_PREFERENCES (object);

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
math_preferences_class_init (MathPreferencesDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = math_preferences_get_property;
    object_class->set_property = math_preferences_set_property;

    g_type_class_add_private (klass, sizeof (MathPreferencesDialogPrivate));

    g_object_class_install_property(object_class,
                                    PROP_EQUATION,
                                    g_param_spec_object("equation",
                                                        "equation",
                                                        "Equation being configured",
                                                        math_equation_get_type(),
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
math_preferences_init(MathPreferencesDialog *dialog)
{
    dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, math_preferences_get_type(), MathPreferencesDialogPrivate);
}
