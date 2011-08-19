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

#include "math-buttons.h"
#include "financial.h"
#include "currency.h"

enum {
    PROP_0,
    PROP_EQUATION,
    PROP_MODE
};

static GType button_mode_type;

#define MAXBITS 64      /* Bit panel: number of bit fields. */

struct MathButtonsPrivate
{
    MathEquation *equation;

    ButtonMode mode;
    gint programming_base;
  
    GtkBuilder *basic_ui, *advanced_ui, *financial_ui, *programming_ui;

    GdkColor color_numbers, color_action, color_operator, color_function, color_memory, color_group;

    GtkWidget *bas_panel, *adv_panel, *fin_panel, *prog_panel;
    GtkWidget *active_panel;

    GtkWidget *shift_left_menu, *shift_right_menu;

    GtkWidget *function_menu;

    GList *superscript_toggles;
    GList *subscript_toggles;

    GtkWidget *angle_combo;
    GtkWidget *angle_label;
  
    GtkWidget *base_combo;  
    GtkWidget *base_label;
    GtkWidget *bit_panel;
    GtkWidget *bit_labels[MAXBITS];

    GtkWidget *source_currency_combo;
    GtkWidget *target_currency_combo;
    GtkWidget *currency_label;

    GtkWidget *character_code_dialog;
    GtkWidget *character_code_entry;
};

G_DEFINE_TYPE (MathButtons, math_buttons, GTK_TYPE_VBOX);

#define UI_BASIC_FILE       UI_DIR "/buttons-basic.ui"
#define UI_ADVANCED_FILE    UI_DIR "/buttons-advanced.ui"
#define UI_FINANCIAL_FILE   UI_DIR "/buttons-financial.ui"
#define UI_PROGRAMMING_FILE UI_DIR "/buttons-programming.ui"

#define GET_WIDGET(ui, name) \
          GTK_WIDGET(gtk_builder_get_object((ui), (name)))

#define WM_WIDTH_FACTOR  10
#define WM_HEIGHT_FACTOR 30

typedef enum
{
    NUMBER,
    NUMBER_BOLD,
    OPERATOR,
    FUNCTION,
    MEMORY,
    GROUP,
    ACTION
} ButtonClass;

typedef struct {
    const char *widget_name;
    const char *data;
    ButtonClass class;
    const char *tooltip;
} ButtonData;

static ButtonData button_data[] = {
    {"pi",                 "π", NUMBER,
      /* Tooltip for the Pi button */
      N_("Pi [Ctrl+P]")},
    {"eulers_number",      "e", NUMBER,
      /* Tooltip for the Euler's Number button */
      N_("Euler’s Number")},
    {"imaginary",          "i", NUMBER, NULL},
    {"numeric_point", NULL, NUMBER, NULL},
    {"subscript", NULL, NUMBER_BOLD,
      /* Tooltip for the subscript button */
      N_("Subscript mode [Alt]")},
    {"superscript", NULL, NUMBER_BOLD,
      /* Tooltip for the superscript button */
      N_("Superscript mode [Ctrl]")},
    {"exponential", NULL, NUMBER_BOLD,
      /* Tooltip for the scientific exponent button */
      N_("Scientific exponent [Ctrl+E]")},
    {"add",                "+", OPERATOR,
      /* Tooltip for the add button */
      N_("Add [+]")},
    {"subtract",           "−", OPERATOR,
      /* Tooltip for the subtract button */
      N_("Subtract [-]")},
    {"multiply",           "×", OPERATOR,
      /* Tooltip for the multiply button */
      N_("Multiply [*]")},
    {"divide",             "÷", OPERATOR,
      /* Tooltip for the divide button */
      N_("Divide [/]")},
    {"modulus_divide",     " mod ", OPERATOR,
      /* Tooltip for the modulus divide button */
      N_("Modulus divide")},
    {"function",           NULL, FUNCTION,
      /* Tooltip for the additional functions button */
      N_("Additional Functions")},
    {"x_pow_y",            "^", FUNCTION,
      /* Tooltip for the exponent button */
      N_("Exponent [^ or **]")},
    {"x_squared",          "²", FUNCTION,
      /* Tooltip for the square button */
      N_("Square [Ctrl+2]")},
    {"percentage",         "%", NUMBER,
      /* Tooltip for the percentage button */
      N_("Percentage [%]")},
    {"factorial",          "!", FUNCTION,
      /* Tooltip for the factorial button */
      N_("Factorial [!]")},
    {"abs",                "|", FUNCTION,
      /* Tooltip for the absolute value button */
      N_("Absolute value [|]")},
    {"arg",                "Arg ", FUNCTION,
      /* Tooltip for the complex argument component button */
      N_("Complex argument")},
    {"conjugate",          "conj ", FUNCTION,
      /* Tooltip for the complex conjugate button */
      N_("Complex conjugate")},
    {"root",               "√", FUNCTION,
      /* Tooltip for the root button */
      N_("Root [Ctrl+R]")},
    {"square_root",        "√", FUNCTION,
      /* Tooltip for the square root button */
      N_("Square root [Ctrl+R]")},
    {"logarithm",          "log ", FUNCTION,
      /* Tooltip for the logarithm button */
      N_("Logarithm")},
    {"natural_logarithm",  "ln ", FUNCTION,
      /* Tooltip for the natural logarithm button */
      N_("Natural Logarithm")},
    {"sine",               "sin ", FUNCTION,
      /* Tooltip for the sine button */
      N_("Sine")},
    {"cosine",             "cos ", FUNCTION,
      /* Tooltip for the cosine button */
      N_("Cosine")},
    {"tangent",            "tan ", FUNCTION,
      /* Tooltip for the tangent button */
      N_("Tangent")},
    {"hyperbolic_sine",    "sinh ", FUNCTION,
      /* Tooltip for the hyperbolic sine button */
      N_("Hyperbolic Sine")},
    {"hyperbolic_cosine",  "cosh ", FUNCTION,
      /* Tooltip for the hyperbolic cosine button */
      N_("Hyperbolic Cosine")},
    {"hyperbolic_tangent", "tanh ", FUNCTION,
      /* Tooltip for the hyperbolic tangent button */
      N_("Hyperbolic Tangent")},
    {"inverse",            "⁻¹", FUNCTION,
      /* Tooltip for the inverse button */
      N_("Inverse [Ctrl+I]")},
    {"and",                "∧", OPERATOR,
      /* Tooltip for the boolean AND button */
      N_("Boolean AND")},
    {"or",                 "∨", OPERATOR,
      /* Tooltip for the boolean OR button */
      N_("Boolean OR")},
    {"xor",                "⊻", OPERATOR,
      /* Tooltip for the exclusive OR button */
      N_("Boolean Exclusive OR")},
    {"not",                "¬", FUNCTION,
      /* Tooltip for the boolean NOT button */
      N_("Boolean NOT")},
    {"integer_portion",    "int ", FUNCTION,
      /* Tooltip for the integer component button */
      N_("Integer Component")},
    {"fractional_portion", "frac ", FUNCTION,
      /* Tooltip for the fractional component button */
      N_("Fractional Component")},
    {"real_portion",       "Re ", FUNCTION,
      /* Tooltip for the real component button */
      N_("Real Component")},
    {"imaginary_portion",  "Im ", FUNCTION,
      /* Tooltip for the imaginary component button */
      N_("Imaginary Component")},
    {"ones_complement",    "ones ", FUNCTION,
      /* Tooltip for the ones complement button */
      N_("Ones Complement")},
    {"twos_complement",    "twos ", FUNCTION,
      /* Tooltip for the twos complement button */
      N_("Twos Complement")},
    {"trunc",              "trunc ", FUNCTION,
      /* Tooltip for the truncate button */
      N_("Truncate")},
    {"start_group",        "(", GROUP,
      /* Tooltip for the start group button */
      N_("Start Group [(]")},
    {"end_group",          ")", GROUP,
      /* Tooltip for the end group button */
      N_("End Group [)]")},
    {"store", NULL, MEMORY,
      /* Tooltip for the assign variable button */
      N_("Assign Variable")},
    {"recall", NULL, MEMORY,
      /* Tooltip for the insert variable button */
      N_("Insert Variable")},
    {"character", NULL, MEMORY,
      /* Tooltip for the insert character code button */
      N_("Insert Character Code")},
    {"result", NULL, ACTION,
      /* Tooltip for the solve button */
      N_("Calculate Result")},
    {"factor", NULL, ACTION,
      /* Tooltip for the factor button */
      N_("Factorize [Ctrl+F]")},
    {"clear", NULL, GROUP,
      /* Tooltip for the clear button */
      N_("Clear Display [Escape]")},
    {"undo", NULL, GROUP,
      /* Tooltip for the undo button */
      N_("Undo [Ctrl+Z]")},
    {"shift_left", NULL, ACTION,
      /* Tooltip for the shift left button */
      N_("Shift Left")},  
    {"shift_right", NULL, ACTION,
      /* Tooltip for the shift right button */
      N_("Shift Right")},  
    {"finc_compounding_term", NULL, FUNCTION,
      /* Tooltip for the compounding term button */
      N_("Compounding Term")},
    {"finc_double_declining_depreciation", NULL, FUNCTION,
      /* Tooltip for the double declining depreciation button */
      N_("Double Declining Depreciation")},
    {"finc_future_value", NULL, FUNCTION,
      /* Tooltip for the future value button */
      N_("Future Value")},
    {"finc_term", NULL, FUNCTION,
      /* Tooltip for the financial term button */
      N_("Financial Term")},
    {"finc_sum_of_the_years_digits_depreciation", NULL, FUNCTION,
      /* Tooltip for the sum of the years digits depreciation button */
      N_("Sum of the Years Digits Depreciation")},
    {"finc_straight_line_depreciation", NULL, FUNCTION,
      /* Tooltip for the straight line depreciation button */
      N_("Straight Line Depreciation")},
    {"finc_periodic_interest_rate", NULL, FUNCTION,
      /* Tooltip for the periodic interest rate button */
      N_("Periodic Interest Rate")},
    {"finc_present_value", NULL, FUNCTION,
      /* Tooltip for the present value button */
      N_("Present Value")},
    {"finc_periodic_payment", NULL, FUNCTION,
      /* Tooltip for the periodic payment button */
      N_("Periodic Payment")},
    {"finc_gross_profit_margin", NULL, FUNCTION,
      /* Tooltip for the gross profit margin button */
      N_("Gross Profit Margin")},
    {NULL, NULL, 0, NULL}
};

typedef enum {
    CURRENCY_TARGET_UPPER,
    CURRENCY_TARGET_LOWER
} CurrencyTargetRow;

/* The names of each field in the dialogs for the financial functions */
static char *finc_dialog_fields[][5] = {
    {"ctrm_pint", "ctrm_fv",     "ctrm_pv",    NULL,         NULL},
    {"ddb_cost",  "ddb_life",    "ddb_period", NULL,         NULL},
    {"fv_pmt",    "fv_pint",     "fv_n",       NULL,         NULL},
    {"gpm_cost",  "gpm_margin",  NULL,         NULL,         NULL},
    {"pmt_prin",  "pmt_pint",    "pmt_n",      NULL,         NULL},
    {"pv_pmt",    "pv_pint",     "pv_n",       NULL,         NULL},
    {"rate_fv",   "rate_pv",     "rate_n",     NULL,         NULL},
    {"sln_cost",  "sln_salvage", "sln_life",   NULL,         NULL},
    {"syd_cost",  "syd_salvage", "syd_life",   "syd_period", NULL},
    {"term_pmt",  "term_fv",     "term_pint",  NULL,         NULL},
    {NULL,        NULL,          NULL,         NULL,         NULL}
};


MathButtons *
math_buttons_new(MathEquation *equation)
{
    return g_object_new (math_buttons_get_type(), "equation", equation, NULL);
}


static void
set_tint (GtkWidget *widget, GdkColor *tint, gint alpha)
{
    GtkStyle *style;
    int j;
  
    if (!widget)
      return;

    gtk_widget_ensure_style(widget);
    style = gtk_widget_get_style(widget);
  
    for (j = 0; j < 5; j++) {
        GdkColor color;

        color.red = (style->bg[j].red * (10 - alpha) + tint->red * alpha) / 10;
        color.green = (style->bg[j].green * (10 - alpha) + tint->green * alpha) / 10;
        color.blue = (style->bg[j].blue * (10 - alpha) + tint->blue * alpha) / 10;
        gdk_colormap_alloc_color(gdk_colormap_get_system(), &color, FALSE, TRUE);
        gtk_widget_modify_bg(widget, j, &color);
    }
}


static void
set_data(GtkBuilder *ui, const gchar *object_name, const gchar *name, const char *value)
{
    GObject *object;
    object = gtk_builder_get_object(ui, object_name);
    if (object)
        g_object_set_data(object, name, GINT_TO_POINTER(value));
}


static void
set_int_data(GtkBuilder *ui, const gchar *object_name, const gchar *name, gint value)
{
    GObject *object;  
    object = gtk_builder_get_object(ui, object_name);
    if (object)
        g_object_set_data(object, name, GINT_TO_POINTER(value));
}


static void
load_finc_dialogs(MathButtons *buttons)
{
    int i, j;

    set_int_data(buttons->priv->financial_ui, "ctrm_dialog", "finc_dialog", FINC_CTRM_DIALOG);
    set_int_data(buttons->priv->financial_ui, "ddb_dialog", "finc_dialog", FINC_DDB_DIALOG);
    set_int_data(buttons->priv->financial_ui, "fv_dialog", "finc_dialog", FINC_FV_DIALOG);
    set_int_data(buttons->priv->financial_ui, "gpm_dialog", "finc_dialog", FINC_GPM_DIALOG);
    set_int_data(buttons->priv->financial_ui, "pmt_dialog", "finc_dialog", FINC_PMT_DIALOG);
    set_int_data(buttons->priv->financial_ui, "pv_dialog", "finc_dialog", FINC_PV_DIALOG);
    set_int_data(buttons->priv->financial_ui, "rate_dialog", "finc_dialog", FINC_RATE_DIALOG);
    set_int_data(buttons->priv->financial_ui, "sln_dialog", "finc_dialog", FINC_SLN_DIALOG);
    set_int_data(buttons->priv->financial_ui, "syd_dialog", "finc_dialog", FINC_SYD_DIALOG);
    set_int_data(buttons->priv->financial_ui, "term_dialog", "finc_dialog", FINC_TERM_DIALOG);

    for (i = 0; finc_dialog_fields[i][0] != NULL; i++) {
        for (j = 0; finc_dialog_fields[i][j]; j++) {
            GObject *o;
            o = gtk_builder_get_object(buttons->priv->financial_ui, finc_dialog_fields[i][j]);
          if(!o)
            printf("missing '%s'\n", finc_dialog_fields[i][j]);
            g_object_set_data(o, "finc_field", GINT_TO_POINTER(j));
            g_object_set_data(o, "finc_dialog", GINT_TO_POINTER(i));
        }
    }
}


static void
update_angle_label (MathButtons *buttons)
{
    MPNumber x;
    MPNumber pi, max_value, min_value, fraction, input, output;
    char *label, input_text[1024], output_text[1024];

    if (!buttons->priv->angle_label)
        return;

    if (!math_equation_get_number(buttons->priv->equation, &x))
        return;

    mp_get_pi(&pi);
    switch (math_equation_get_angle_units(buttons->priv->equation)) {
    default:
    case MP_DEGREES:
        label = g_strdup("");
        break;
    case MP_RADIANS:
        /* Clip to the range ±2π */
        mp_multiply_integer(&pi, 2, &max_value);
        mp_invert_sign(&max_value, &min_value);
        if (!mp_is_equal(&x, &max_value) && !mp_is_equal(&x, &min_value)) {
            mp_divide(&x, &max_value, &fraction);
            mp_fractional_component(&fraction, &fraction);
            mp_multiply(&fraction, &max_value, &input);
        }
        else {
            mp_set_from_mp(&x, &input);
            mp_set_from_integer(mp_is_negative(&input) ? -1 : 1, &fraction);
        }
        mp_cast_to_string(&input, 10, 10, 2, false, input_text, 1024);

        mp_multiply_integer(&fraction, 360, &output);
        mp_cast_to_string(&output, 10, 10, 2, false, output_text, 1024);
        label = g_strdup_printf("%s radians = %s degrees", input_text, output_text);
        break;
    case MP_GRADIANS:
        /* Clip to the range ±400 */
        mp_set_from_integer(400, &max_value);
        mp_invert_sign(&max_value, &min_value);
        if (!mp_is_equal(&x, &max_value) && !mp_is_equal(&x, &min_value)) {
            mp_divide(&x, &max_value, &fraction);
            mp_fractional_component(&fraction, &fraction);
            mp_multiply(&fraction, &max_value, &input);
        }
        else {
            mp_set_from_mp(&x, &input);
            mp_set_from_integer(mp_is_negative(&input) ? -1 : 1, &fraction);
        }

        mp_cast_to_string(&input, 10, 10, 2, false, input_text, 1024);

        mp_multiply_integer(&fraction, 360, &output);
        mp_cast_to_string(&output, 10, 10, 2, false, output_text, 1024);
        label = g_strdup_printf("%s gradians = %s degrees", input_text, output_text);
        break;
    }

    gtk_label_set_text(GTK_LABEL(buttons->priv->angle_label), label);
    g_free(label);
}


static void
update_bit_panel(MathButtons *buttons)
{
    MPNumber x;
    gboolean enabled;
    guint64 bits;
    int i;
    GString *label;
    gint base;

    if (!buttons->priv->bit_panel)
        return;
  
    enabled = math_equation_get_number(buttons->priv->equation, &x);

    if (enabled) {
        MPNumber max, fraction;

        mp_set_from_unsigned_integer(G_MAXUINT64, &max);
        mp_fractional_part(&x, &fraction);
        if (mp_is_negative(&x) || mp_is_greater_than(&x, &max) || !mp_is_zero(&fraction))
            enabled = FALSE;
        else
            bits = mp_cast_to_unsigned_int(&x);
    }

    gtk_widget_set_sensitive(buttons->priv->bit_panel, enabled);
    gtk_widget_set_sensitive(buttons->priv->base_label, enabled);
      
    if (!enabled)
        return;

    for (i = 0; i < MAXBITS; i++) {
        const gchar *label;

        if (bits & (1LL << (MAXBITS-i-1)))
            label = " 1";
        else
            label = " 0";
        gtk_label_set_text(GTK_LABEL(buttons->priv->bit_labels[i]), label);
    }

    base = math_equation_get_base(buttons->priv->equation);      
    label = g_string_new("");
    if (base != 8) {
        if (label->len != 0)
            g_string_append(label, " = ");
        g_string_append_printf(label, "%lo", bits);
        g_string_append(label, "₈");
    }
    if (base != 10) {
        if (label->len != 0)
            g_string_append(label, " = ");
        g_string_append_printf(label, "%lu", bits);
        g_string_append(label, "₁₀");
    }
    if (base != 16) {
        if (label->len != 0)
            g_string_append(label, " = ");
        g_string_append_printf(label, "%lX", bits);
        g_string_append(label, "₁₆");
    }

    gtk_label_set_text(GTK_LABEL(buttons->priv->base_label), label->str);
    g_string_free(label, TRUE);
}


static void
update_currency_label(MathButtons *buttons)
{
    MPNumber x, value;
    char *label;

    if (!buttons->priv->currency_label)
        return;

    if (!math_equation_get_number(buttons->priv->equation, &x))
        return;
  
    if (currency_convert(&x,
                         math_equation_get_source_currency(buttons->priv->equation),
                         math_equation_get_target_currency(buttons->priv->equation),
                         &value)) {
        char input_text[1024], output_text[1024];
        const char *source_symbol, *target_symbol;
        int i;

        mp_cast_to_string(&x, 10, 10, 2, false, input_text, 1024);
        mp_cast_to_string(&value, 10, 10, 2, false, output_text, 1024);

        for (i = 0; strcmp(math_equation_get_source_currency(buttons->priv->equation), currency_names[i].short_name) != 0; i++);
        source_symbol = currency_names[i].symbol;
        for (i = 0; strcmp(math_equation_get_target_currency(buttons->priv->equation), currency_names[i].short_name) != 0; i++);
        target_symbol = currency_names[i].symbol;

        label = g_strdup_printf("%s%s = %s%s",
                                source_symbol, input_text,
                                target_symbol, output_text);
    }
    else
        label = g_strdup("");

    gtk_label_set_text(GTK_LABEL(buttons->priv->currency_label), label);
    g_free(label);
}


static void
display_changed_cb(MathEquation *equation, GParamSpec *spec, MathButtons *buttons)
{
    update_angle_label(buttons);
    update_currency_label(buttons);
    update_bit_panel(buttons);
}


static void
angle_unit_combobox_changed_cb(GtkWidget *combo, MathButtons *buttons)
{
    MPAngleUnit value;
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, 1, &value, -1);
    math_equation_set_angle_units(buttons->priv->equation, value);
}


static void
angle_unit_cb(MathEquation *equation, GParamSpec *spec, MathButtons *buttons)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(buttons->priv->angle_combo));
    valid = gtk_tree_model_get_iter_first(model, &iter);

    while (valid) {
        gint v;

        gtk_tree_model_get(model, &iter, 1, &v, -1);
        if (v == math_equation_get_angle_units(buttons->priv->equation))
            break;
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    if (!valid)
        valid = gtk_tree_model_get_iter_first(model, &iter);

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(buttons->priv->angle_combo), &iter);
}


static void
base_combobox_changed_cb(GtkWidget *combo, MathButtons *buttons)
{
    gint value;
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, 1, &value, -1);

    math_equation_set_base(buttons->priv->equation, value);
}


static void
base_changed_cb(MathEquation *equation, GParamSpec *spec, MathButtons *buttons)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid;
  
    if (buttons->priv->mode != PROGRAMMING)
        return;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(buttons->priv->base_combo));
    valid = gtk_tree_model_get_iter_first(model, &iter);
    buttons->priv->programming_base = math_equation_get_base(buttons->priv->equation);

    while (valid) {
        gint v;

        gtk_tree_model_get(model, &iter, 1, &v, -1);
        if (v == buttons->priv->programming_base)
            break;
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    if (!valid)
        valid = gtk_tree_model_get_iter_first(model, &iter);

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(buttons->priv->base_combo), &iter);
}


static void
source_currency_combo_changed_cb(GtkWidget *combo, MathButtons *buttons)
{
    gchar *value;
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, 0, &value, -1);

    math_equation_set_source_currency(buttons->priv->equation, value);
    g_free (value);
}


static void
source_currency_changed_cb(MathEquation *equation, GParamSpec *spec, MathButtons *buttons)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid;
  
    if (buttons->priv->mode != FINANCIAL)
        return;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(buttons->priv->source_currency_combo));
    valid = gtk_tree_model_get_iter_first(model, &iter);

    while (valid) {
        gchar *v;
        gboolean matched;

        gtk_tree_model_get(model, &iter, 0, &v, -1);
        matched = strcmp (math_equation_get_source_currency(buttons->priv->equation), v) == 0;
        g_free (v);
        if (matched)
            break;
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    if (!valid)
        valid = gtk_tree_model_get_iter_first(model, &iter);

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(buttons->priv->source_currency_combo), &iter);
    update_currency_label(buttons);
}


static void
target_currency_combo_changed_cb(GtkWidget *combo, MathButtons *buttons)
{
    gchar *value;
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, 0, &value, -1);

    math_equation_set_target_currency(buttons->priv->equation, value);
    g_free (value);
}


static void
target_currency_changed_cb(MathEquation *equation, GParamSpec *spec, MathButtons *buttons)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid;
  
    if (buttons->priv->mode != FINANCIAL)
        return;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(buttons->priv->target_currency_combo));
    valid = gtk_tree_model_get_iter_first(model, &iter);

    while (valid) {
        gchar *v;
        gboolean matched;

        gtk_tree_model_get(model, &iter, 0, &v, -1);
        matched = strcmp (math_equation_get_target_currency(buttons->priv->equation), v) == 0;
        g_free (v);
        if (matched)
            break;
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    if (!valid)
        valid = gtk_tree_model_get_iter_first(model, &iter);

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(buttons->priv->target_currency_combo), &iter);
    update_currency_label(buttons);
}


static GtkWidget *
load_mode(MathButtons *buttons, ButtonMode mode)
{
    GtkBuilder *builder, **builder_ptr;
    gint i;
    gchar *name;
    const gchar *builder_file;
    static gchar *objects[] = { "button_panel", "character_code_dialog", "currency_dialog",
                                "ctrm_dialog", "ddb_dialog", "fv_dialog", "gpm_dialog",
                                "pmt_dialog", "pv_dialog", "rate_dialog", "sln_dialog",
                                "syd_dialog", "term_dialog", "adjustment1", "adjustment2", NULL };
    GtkWidget *widget, **panel;
    GError *error = NULL;

    switch (mode) {
    case BASIC:
        builder_ptr = &buttons->priv->basic_ui;
        builder_file = UI_BASIC_FILE;
        panel = &buttons->priv->bas_panel;
        break;
    case ADVANCED:
        builder_ptr = &buttons->priv->advanced_ui;
        builder_file = UI_ADVANCED_FILE;
        panel = &buttons->priv->adv_panel;
        break;
    case FINANCIAL:
        builder_ptr = &buttons->priv->financial_ui;
        builder_file = UI_FINANCIAL_FILE;
        panel = &buttons->priv->fin_panel;
        break;
    case PROGRAMMING:
        builder_ptr = &buttons->priv->programming_ui;
        builder_file = UI_PROGRAMMING_FILE;
        panel = &buttons->priv->prog_panel;
        break;
    }
  
    if (*panel)
        return *panel;

    builder = *builder_ptr = gtk_builder_new();
    // FIXME: Show dialog if failed to load
    gtk_builder_add_objects_from_file(builder, builder_file, objects, &error);
    if (error) {
        g_warning("Error loading button UI: %s", error->message);
        g_clear_error(&error);
    }
    *panel = GET_WIDGET(builder, "button_panel");
    gtk_box_pack_end(GTK_BOX(buttons), *panel, FALSE, TRUE, 0);

    /* Configure buttons */
    for (i = 0; button_data[i].widget_name != NULL; i++) {
        GObject *object;
        GtkWidget *button;

        name = g_strdup_printf("calc_%s_button", button_data[i].widget_name);
        object = gtk_builder_get_object(*builder_ptr, name);
        g_free(name);

        if (!object)
            continue;
        button = GTK_WIDGET(object);
        if (button_data[i].data)
            g_object_set_data(object, "calc_text", (gpointer) button_data[i].data);

        if (button_data[i].tooltip)
            gtk_widget_set_tooltip_text(button, _(button_data[i].tooltip));
      
        atk_object_set_name (gtk_widget_get_accessible (button), button_data[i].widget_name);

        switch (button_data[i].class) {
        case NUMBER:
            set_tint(button, &buttons->priv->color_numbers, 1);          
            break;
        case NUMBER_BOLD:
            set_tint(button, &buttons->priv->color_numbers, 2);
            break;
        case OPERATOR:
            set_tint(button, &buttons->priv->color_operator, 1);          
            break;
        case FUNCTION:
            set_tint(button, &buttons->priv->color_function, 1);          
            break;
        case MEMORY:
            set_tint(button, &buttons->priv->color_memory, 1);
            break;
        case GROUP:
            set_tint(button, &buttons->priv->color_group, 1);
            break;
        case ACTION:
            set_tint(button, &buttons->priv->color_action, 2);
            break;
        }
    }

    /* Set special button data */
    for (i = 0; i < 16; i++) {
        GtkWidget *button;

        name = g_strdup_printf("calc_%d_button", i);
        button = GET_WIDGET(builder, name);
        if (button) {
            g_object_set_data(G_OBJECT(button), "calc_digit", GINT_TO_POINTER(i));
            set_tint(button, &buttons->priv->color_numbers, 1);
            gtk_button_set_label(GTK_BUTTON(button), math_equation_get_digit_text(buttons->priv->equation, i));
        }
        g_free(name);
    }
    widget = GET_WIDGET(builder, "calc_numeric_point_button");
    if (widget)
        gtk_button_set_label(GTK_BUTTON(widget), math_equation_get_numeric_point_text(buttons->priv->equation));
  
    widget = GET_WIDGET(builder, "calc_superscript_button");
    if (widget) {
        buttons->priv->superscript_toggles = g_list_append(buttons->priv->superscript_toggles, widget);
        if (math_equation_get_number_mode(buttons->priv->equation) == SUPERSCRIPT)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    }
    widget = GET_WIDGET(builder, "calc_subscript_button");
    if (widget) {
        buttons->priv->subscript_toggles = g_list_append(buttons->priv->subscript_toggles, widget);
        if (math_equation_get_number_mode(buttons->priv->equation) == SUBSCRIPT)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    }
  
    if (mode == ADVANCED) {
        GtkListStore *model;
        GtkTreeIter iter;
        GtkCellRenderer *renderer;

        buttons->priv->angle_label = GET_WIDGET(builder, "angle_label");

        buttons->priv->angle_combo = GET_WIDGET(builder, "angle_units_combo");
        model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
        gtk_combo_box_set_model(GTK_COMBO_BOX(buttons->priv->angle_combo), GTK_TREE_MODEL(model));
        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                           /* Advanced buttons: Angle unit combo box: Use degrees for trigonometric calculations */
                           _("Degrees"), 1, MP_DEGREES, -1);
        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                           /* Advanced buttons: Angle unit combo box: Use radians for trigonometric calculations */
                           _("Radians"), 1, MP_RADIANS, -1);
        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                           /* Advanced buttons: Angle unit combo box: Use gradians for trigonometric calculations */
                           _("Gradians"), 1, MP_GRADIANS, -1);
        renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(buttons->priv->angle_combo), renderer, TRUE);
        gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(buttons->priv->angle_combo), renderer, "text", 0);

        g_signal_connect(buttons->priv->angle_combo, "changed", G_CALLBACK(angle_unit_combobox_changed_cb), buttons);
        g_signal_connect(buttons->priv->equation, "notify::angle-units", G_CALLBACK(angle_unit_cb), buttons);
        angle_unit_cb(buttons->priv->equation, NULL, buttons);
    }

    if (mode == PROGRAMMING) {
        GtkListStore *model;
        GtkTreeIter iter;
        GtkCellRenderer *renderer;

        buttons->priv->base_label = GET_WIDGET(builder, "base_label");
        buttons->priv->character_code_dialog = GET_WIDGET(builder, "character_code_dialog");
        buttons->priv->character_code_entry = GET_WIDGET(builder, "character_code_entry");

        buttons->priv->bit_panel = GET_WIDGET(builder, "bit_table");
        for (i = 0; i < MAXBITS; i++) {
            name = g_strdup_printf("bit_label_%d", i);
            buttons->priv->bit_labels[i] = GET_WIDGET(builder, name);
            g_free(name);
            name = g_strdup_printf("bit_eventbox_%d", i);
            set_int_data(builder, name, "bit_index", i);
        }

        buttons->priv->base_combo = GET_WIDGET(builder, "base_combo");
        model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
        gtk_combo_box_set_model(GTK_COMBO_BOX(buttons->priv->base_combo), GTK_TREE_MODEL(model));
        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                           /* Number display mode combo: Binary, e.g. 10011010010₂ */
                           _("Binary"), 1, 2, -1);
        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                           /* Number display mode combo: Octal, e.g. 2322₈ */
                           _("Octal"), 1, 8, -1);
        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                           /* Number display mode combo: Decimal, e.g. 1234 */
                           _("Decimal"), 1, 10, -1);
        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,
                           /* Number display mode combo: Hexadecimal, e.g. 4D2₁₆ */
                           _("Hexadecimal"), 1, 16, -1);
        renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(buttons->priv->base_combo), renderer, TRUE);
        gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(buttons->priv->base_combo), renderer, "text", 0);

        g_signal_connect(buttons->priv->base_combo, "changed", G_CALLBACK(base_combobox_changed_cb), buttons);
        g_signal_connect(buttons->priv->equation, "notify::base", G_CALLBACK(base_changed_cb), buttons);
        base_changed_cb(buttons->priv->equation, NULL, buttons);
    }

    /* Setup financial functions */
    if (mode == FINANCIAL) {
        GtkListStore *model;
        GtkCellRenderer *renderer;

        load_finc_dialogs(buttons);

        buttons->priv->source_currency_combo = GET_WIDGET(builder, "source_currency_combo");
        buttons->priv->target_currency_combo = GET_WIDGET(builder, "target_currency_combo");
        buttons->priv->currency_label = GET_WIDGET(builder, "currency_label");

        model = gtk_list_store_new(1, G_TYPE_STRING);

        for (i = 0; currency_names[i].short_name != NULL; i++) {
            GtkTreeIter iter;

            gtk_list_store_append(GTK_LIST_STORE(model), &iter);
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, currency_names[i].short_name, -1);
        }

        gtk_combo_box_set_model(GTK_COMBO_BOX(buttons->priv->source_currency_combo), GTK_TREE_MODEL(model));
        gtk_combo_box_set_model(GTK_COMBO_BOX(buttons->priv->target_currency_combo), GTK_TREE_MODEL(model));

        renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(buttons->priv->source_currency_combo), renderer, TRUE);
        gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(buttons->priv->source_currency_combo), renderer, "text", 0);
        renderer = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(buttons->priv->target_currency_combo), renderer, TRUE);
        gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(buttons->priv->target_currency_combo), renderer, "text", 0);

        g_signal_connect(buttons->priv->source_currency_combo, "changed", G_CALLBACK(source_currency_combo_changed_cb), buttons);
        g_signal_connect(buttons->priv->target_currency_combo, "changed", G_CALLBACK(target_currency_combo_changed_cb), buttons);
        g_signal_connect(buttons->priv->equation, "notify::source-currency", G_CALLBACK(source_currency_changed_cb), buttons);
        g_signal_connect(buttons->priv->equation, "notify::target-currency", G_CALLBACK(target_currency_changed_cb), buttons);
        source_currency_changed_cb(buttons->priv->equation, NULL, buttons);
        target_currency_changed_cb(buttons->priv->equation, NULL, buttons);

        set_data(builder, "calc_finc_compounding_term_button", "finc_dialog", "ctrm_dialog");
        set_data(builder, "calc_finc_double_declining_depreciation_button", "finc_dialog", "ddb_dialog");
        set_data(builder, "calc_finc_future_value_button", "finc_dialog", "fv_dialog");
        set_data(builder, "calc_finc_gross_profit_margin_button", "finc_dialog", "gpm_dialog");
        set_data(builder, "calc_finc_periodic_payment_button", "finc_dialog", "pmt_dialog");
        set_data(builder, "calc_finc_present_value_button", "finc_dialog", "pv_dialog");
        set_data(builder, "calc_finc_periodic_interest_rate_button", "finc_dialog", "rate_dialog");
        set_data(builder, "calc_finc_straight_line_depreciation_button", "finc_dialog", "sln_dialog");
        set_data(builder, "calc_finc_sum_of_the_years_digits_depreciation_button", "finc_dialog", "syd_dialog");
        set_data(builder, "calc_finc_term_button", "finc_dialog", "term_dialog");
    }

    gtk_builder_connect_signals(builder, buttons);

    display_changed_cb(buttons->priv->equation, NULL, buttons);
  
    return *panel;
}



static void
load_buttons(MathButtons *buttons)
{
    GtkWidget *panel;

    if (!gtk_widget_get_visible(GTK_WIDGET(buttons)))
        return;

    panel = load_mode(buttons, buttons->priv->mode);
    if (buttons->priv->active_panel == panel)
        return;

    /* Hide old buttons */
    if (buttons->priv->active_panel)
        gtk_widget_hide(buttons->priv->active_panel);

    /* Load and display new buttons */
    buttons->priv->active_panel = panel;
    if (panel)
        gtk_widget_show(panel);
}


void
math_buttons_set_mode(MathButtons *buttons, ButtonMode mode)
{
    ButtonMode old_mode;
 
    if (buttons->priv->mode == mode)
        return;

    old_mode = buttons->priv->mode;
    buttons->priv->mode = mode;
  
    if (mode == PROGRAMMING)
        math_equation_set_base(buttons->priv->equation, buttons->priv->programming_base);
    else
        math_equation_set_base(buttons->priv->equation, 10);

    load_buttons(buttons);

    g_object_notify(G_OBJECT(buttons), "mode");
}


ButtonMode
math_buttons_get_mode(MathButtons *buttons)
{
    return buttons->priv->mode;
}


void
math_buttons_set_programming_base(MathButtons *buttons, gint base)
{
    buttons->priv->programming_base = base;
}


gint
math_buttons_get_programming_base(MathButtons *buttons)
{
    return buttons->priv->programming_base;
}


void exponent_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
exponent_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_insert_exponent(buttons->priv->equation);
}


void subtract_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
subtract_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_insert_subtract(buttons->priv->equation);  
}


void button_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
button_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_insert(buttons->priv->equation, g_object_get_data(G_OBJECT(widget), "calc_text"));
}


void solve_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
solve_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_solve(buttons->priv->equation);
}


void clear_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
clear_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_clear(buttons->priv->equation);
}


void delete_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
delete_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_delete(buttons->priv->equation);
}


void undo_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
undo_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_undo(buttons->priv->equation);
}


static void
shift_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_shift(buttons->priv->equation, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "shiftcount")));
}


static void
button_menu_position_func(GtkMenu *menu, gint *x, gint *y,
                          gboolean *push_in, gpointer user_data)
{
    GtkWidget *button = user_data;
    GtkAllocation allocation;
    GdkPoint loc;
    gint border;
  
    gdk_window_get_origin(gtk_widget_get_window(button), &loc.x, &loc.y);
    border = gtk_container_get_border_width(GTK_CONTAINER(button));
    gtk_widget_get_allocation(button, &allocation);
    *x = loc.x + allocation.x + border;
    *y = loc.y + allocation.y + border;
}


static void
popup_button_menu(GtkWidget *widget, GtkMenu *menu)
{
    gtk_menu_popup(menu, NULL, NULL,
                   button_menu_position_func, widget, 1, gtk_get_current_event_time());
}


static void
save_variable_cb(GtkWidget *widget, MathButtons *buttons)
{
  printf("save\n");
}


static void
delete_variable_cb(GtkWidget *widget, MathButtons *buttons)
{
  printf("delete\n");
}


static GtkWidget *
make_register_menu_item(MathButtons *buttons, const gchar *name, const MPNumber *value, gboolean can_modify, GCallback callback)
{
    gchar text[1024] = "", *mstr;
    GtkWidget *item, *label;

    if (value) {
        display_make_number(buttons->priv->equation, text, 1024, value);
        mstr = g_strdup_printf("<span weight=\"bold\">%s</span> = %s", name, text);
    }
    else
        mstr = g_strdup_printf("<span weight=\"bold\">%s</span>", name);
    label = gtk_label_new(mstr);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    g_free(mstr);

    item = gtk_menu_item_new();

    // FIXME: Buttons don't work inside menus...
    if (0) {//can_modify) {
        GtkWidget *hbox, *button;

        hbox = gtk_hbox_new(FALSE, 6);
        gtk_container_add(GTK_CONTAINER(item), hbox);

        gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

        button = gtk_button_new();
        gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_MENU));
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(delete_variable_cb), buttons);

        button = gtk_button_new();
        gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(save_variable_cb), buttons);
    }
    else
        gtk_container_add(GTK_CONTAINER(item), label);

    g_object_set_data(G_OBJECT(item), "register_id", g_strdup(name)); // FIXME: Memory leak
    g_signal_connect(item, "activate", callback, buttons);
  
    return item;
}


static void
store_menu_cb(GtkMenuItem *menu, MathButtons *buttons)
{
    math_equation_store(buttons->priv->equation, g_object_get_data(G_OBJECT(menu), "register_id"));
}


void store_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
store_cb(GtkWidget *widget, MathButtons *buttons)
{
    int i;
    GtkWidget *menu;
    GtkWidget *item;
    gchar **names;

    menu = gtk_menu_new();
    gtk_menu_set_reserve_toggle_size(GTK_MENU(menu), FALSE);
    set_tint(menu, &buttons->priv->color_memory, 1);

    names = math_variables_get_names(math_equation_get_variables(buttons->priv->equation));
    if (names[0] == NULL) {
        item = gtk_menu_item_new_with_label(/* Text shown in store menu when no variables defined */
                                            _("No variables defined"));
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }  
    for (i = 0; names[i]; i++) {
        MPNumber *value;
        value = math_variables_get_value(math_equation_get_variables(buttons->priv->equation), names[i]);
        item = make_register_menu_item(buttons, names[i], value, TRUE, G_CALLBACK(store_menu_cb));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    g_strfreev(names);

    // FIXME
    //item = gtk_menu_item_new_with_label(_("Add variable"));
    //gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    popup_button_menu(widget, GTK_MENU(menu));
}


static void
recall_menu_cb(GtkMenuItem *menu, MathButtons *buttons)
{
    math_equation_recall(buttons->priv->equation, g_object_get_data(G_OBJECT(menu), "register_id"));  
}


void recall_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
recall_cb(GtkWidget *widget, MathButtons *buttons)
{
    int i;
    GtkWidget *menu;
    GtkWidget *item;
    gchar **names;

    menu = gtk_menu_new();
    gtk_menu_set_reserve_toggle_size(GTK_MENU(menu), FALSE);
    set_tint(menu, &buttons->priv->color_memory, 1);

    names = math_variables_get_names(math_equation_get_variables(buttons->priv->equation));
    if (names[0] == NULL) {
        item = gtk_menu_item_new_with_label(/* Text shown in recall menu when no variables defined */
                                            _("No variables defined"));
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }  
    for (i = 0; names[i]; i++) {
        MPNumber *value;
        value = math_variables_get_value(math_equation_get_variables(buttons->priv->equation), names[i]);
        item = make_register_menu_item(buttons, names[i], value, TRUE, G_CALLBACK(recall_menu_cb));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    g_strfreev(names);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    item = make_register_menu_item(buttons, "ans", math_equation_get_answer(buttons->priv->equation), FALSE, G_CALLBACK(recall_menu_cb));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    item = make_register_menu_item(buttons, "rand", NULL, FALSE, G_CALLBACK(recall_menu_cb));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    popup_button_menu(widget, GTK_MENU(menu));
}


void shift_left_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
shift_left_cb(GtkWidget *widget, MathButtons *buttons)
{
    if (!buttons->priv->shift_left_menu) {
        gint i;
        GtkWidget *menu;

        menu = buttons->priv->shift_left_menu = gtk_menu_new();
        gtk_menu_set_reserve_toggle_size(GTK_MENU(menu), FALSE);
        set_tint(menu, &buttons->priv->color_action, 1);

        for (i = 1; i < 16; i++) {
            GtkWidget *item, *label;
            gchar *format, *text;

            if (i < 10) {
                /* Left Shift Popup: Menu item to shift left by n places (n < 10) */
                format = ngettext("_%d place", "_%d places", i);
            }
            else {
                /* Left Shift Popup: Menu item to shift left by n places (n >= 10) */
                format = ngettext("%d place", "%d places", i);
            }
            text = g_strdup_printf(format, i);
            label = gtk_label_new_with_mnemonic(text);

            item = gtk_menu_item_new();
            g_object_set_data(G_OBJECT(item), "shiftcount", GINT_TO_POINTER(i));
            gtk_container_add(GTK_CONTAINER(item), label);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            g_signal_connect(item, "activate", G_CALLBACK(shift_cb), buttons);

            gtk_widget_show(label);
            gtk_widget_show(item);
            g_free(text);
        }
    }

    popup_button_menu(widget, GTK_MENU(buttons->priv->shift_left_menu));
}


void shift_right_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
shift_right_cb(GtkWidget *widget, MathButtons *buttons)
{
    if (!buttons->priv->shift_right_menu) {
        gint i;
        GtkWidget *menu;

        menu = buttons->priv->shift_right_menu = gtk_menu_new();
        gtk_menu_set_reserve_toggle_size(GTK_MENU(menu), FALSE);
        set_tint(menu, &buttons->priv->color_action, 1);

        for (i = 1; i < 16; i++) {
            GtkWidget *item, *label;
            gchar *format, *text;

            if (i < 10) {
                /* Right Shift Popup: Menu item to shift right by n places (n < 10) */
                format = ngettext("_%d place", "_%d places", i);
            }
            else {
                /* Right Shift Popup: Menu item to shift right by n places (n >= 10) */
                format = ngettext("%d place", "%d places", i);
            }
            text = g_strdup_printf(format, i);
            label = gtk_label_new_with_mnemonic(text);

            item = gtk_menu_item_new();
            g_object_set_data(G_OBJECT(item), "shiftcount", GINT_TO_POINTER(-i));
            gtk_container_add(GTK_CONTAINER(item), label);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            g_signal_connect(item, "activate", G_CALLBACK(shift_cb), buttons);

            gtk_widget_show(label);
            gtk_widget_show(item);
            g_free(text);
        }
    }

    popup_button_menu(widget, GTK_MENU(buttons->priv->shift_right_menu));  
}


static void
insert_function_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_insert(buttons->priv->equation, g_object_get_data(G_OBJECT(widget), "function"));
}


void function_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
function_cb(GtkWidget *widget, MathButtons *buttons)
{
    if (!buttons->priv->function_menu) {
        gint i;
        GtkWidget *menu;
        struct 
        {
            gchar *name, *function;
        } functions[] = 
        {
            { /* Tooltip for the integer component button */
              N_("Integer Component"), "int " },
            { /* Tooltip for the fractional component button */
              N_("Fractional Component"), "frac " },
            { /* Tooltip for the round button */
              N_("Round"), "round " },
            { /* Tooltip for the floor button */
              N_("Floor"), "floor " },
            { /* Tooltip for the ceiling button */
              N_("Ceiling"), "ceil " },
            { /* Tooltip for the ceiling button */
              N_("Sign"), "sgn " },
            { NULL, NULL }
        };

        menu = buttons->priv->function_menu = gtk_menu_new();
        gtk_menu_set_reserve_toggle_size(GTK_MENU(menu), FALSE);
        set_tint(menu, &buttons->priv->color_function, 1);

        for (i = 0; functions[i].name != NULL; i++) {
            GtkWidget *item;
          
            item = gtk_menu_item_new_with_label(_(functions[i].name));
            g_object_set_data(G_OBJECT(item), "function", g_strdup (functions[i].function));
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            g_signal_connect(item, "activate", G_CALLBACK(insert_function_cb), buttons);
            gtk_widget_show(item);
        }
    }

    popup_button_menu(widget, GTK_MENU(buttons->priv->function_menu));  
}


void factorize_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
factorize_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_factorize (buttons->priv->equation);
}


void digit_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
digit_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_insert_digit(buttons->priv->equation, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "calc_digit")));
}


void numeric_point_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
numeric_point_cb(GtkWidget *widget, MathButtons *buttons)
{
    math_equation_insert_numeric_point(buttons->priv->equation);
}



void finc_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
finc_cb(GtkWidget *widget, MathButtons *buttons)
{
    gchar *name;

    name = g_object_get_data(G_OBJECT(widget), "finc_dialog");
    gtk_dialog_run(GTK_DIALOG(GET_WIDGET(buttons->priv->financial_ui, name)));
    gtk_widget_hide(GTK_WIDGET(GET_WIDGET(buttons->priv->financial_ui, name)));
}


void insert_character_code_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
insert_character_code_cb(GtkWidget *widget, MathButtons *buttons)
{
    gtk_window_present(GTK_WINDOW(buttons->priv->character_code_dialog));
}


void finc_activate_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
finc_activate_cb(GtkWidget *widget, MathButtons *buttons)
{
    gint dialog, field;

    dialog = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "finc_dialog"));
    field = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "finc_field"));

    if (finc_dialog_fields[dialog][field+1] == NULL) {
        GtkWidget *dialog_widget;
        dialog_widget = gtk_widget_get_toplevel(widget);
        if (gtk_widget_is_toplevel (dialog_widget)) {
            gtk_dialog_response(GTK_DIALOG(dialog_widget),
                                GTK_RESPONSE_OK);
            return;
        }
    }
    else {
        GtkWidget *next_widget;
        next_widget = GET_WIDGET(buttons->priv->financial_ui, finc_dialog_fields[dialog][field+1]);
        gtk_widget_grab_focus(next_widget);
    }
}


void finc_response_cb(GtkWidget *widget, gint response_id, MathButtons *buttons);
G_MODULE_EXPORT
void
finc_response_cb(GtkWidget *widget, gint response_id, MathButtons *buttons)
{
    int dialog;
    int i;
    MPNumber arg[4];
    GtkWidget *entry;

    if (response_id != GTK_RESPONSE_OK)
        return;

    dialog = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "finc_dialog"));

    for (i = 0; i < 4; i++) {
        if (finc_dialog_fields[dialog][i] == NULL) {
            continue;
        }
        entry = GET_WIDGET(buttons->priv->financial_ui, finc_dialog_fields[dialog][i]);
        mp_set_from_string(gtk_entry_get_text(GTK_ENTRY(entry)), 10, &arg[i]);
        gtk_entry_set_text(GTK_ENTRY(entry), "0");
    }
    gtk_widget_grab_focus(GET_WIDGET(buttons->priv->financial_ui, finc_dialog_fields[dialog][0]));

    do_finc_expression(buttons->priv->equation, dialog, &arg[0], &arg[1], &arg[2], &arg[3]);
}


void character_code_dialog_response_cb(GtkWidget *dialog, gint response_id, MathButtons *buttons);
G_MODULE_EXPORT
void
character_code_dialog_response_cb(GtkWidget *dialog, gint response_id, MathButtons *buttons)
{
    const gchar *text;

    text = gtk_entry_get_text(GTK_ENTRY(buttons->priv->character_code_entry));

    if (response_id == GTK_RESPONSE_OK) {     
        MPNumber x;
        int i = 0;

        mp_set_from_integer(0, &x);
        while (TRUE) {
            mp_add_integer(&x, text[i], &x);
            if (text[i+1]) {
                 mp_shift(&x, 8, &x);
                 i++;
            }
            else
                break;
        }

        math_equation_insert_number(buttons->priv->equation, &x);
    }

    gtk_widget_hide(dialog);
}


void character_code_dialog_activate_cb(GtkWidget *entry, MathButtons *buttons);
G_MODULE_EXPORT
void
character_code_dialog_activate_cb(GtkWidget *entry, MathButtons *buttons)
{
    character_code_dialog_response_cb(buttons->priv->character_code_dialog, GTK_RESPONSE_OK, buttons);
}


gboolean character_code_dialog_delete_cb(GtkWidget *dialog, GdkEvent *event, MathButtons *buttons);
G_MODULE_EXPORT
gboolean
character_code_dialog_delete_cb(GtkWidget *dialog, GdkEvent *event, MathButtons *buttons)
{
    character_code_dialog_response_cb(dialog, GTK_RESPONSE_CANCEL, buttons);
    return TRUE;
}


gboolean bit_toggle_cb(GtkWidget *event_box, GdkEventButton *event, MathButtons *buttons);
G_MODULE_EXPORT
gboolean
bit_toggle_cb(GtkWidget *event_box, GdkEventButton *event, MathButtons *buttons)
{
    math_equation_toggle_bit(buttons->priv->equation, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(event_box), "bit_index")));
    return TRUE;
}



void set_superscript_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
set_superscript_cb(GtkWidget *widget, MathButtons *buttons)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
       math_equation_set_number_mode(buttons->priv->equation, SUPERSCRIPT);
    else if (math_equation_get_number_mode(buttons->priv->equation) == SUPERSCRIPT)
       math_equation_set_number_mode(buttons->priv->equation, NORMAL);
}


void set_subscript_cb(GtkWidget *widget, MathButtons *buttons);
G_MODULE_EXPORT
void
set_subscript_cb(GtkWidget *widget, MathButtons *buttons)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
       math_equation_set_number_mode(buttons->priv->equation, SUBSCRIPT);
    else if (math_equation_get_number_mode(buttons->priv->equation) == SUBSCRIPT)
       math_equation_set_number_mode(buttons->priv->equation, NORMAL);
}


static void
number_mode_changed_cb(MathEquation *equation, GParamSpec *spec, MathButtons *buttons)
{
    GList *i;
    NumberMode mode;
  
    mode = math_equation_get_number_mode(equation);

    for (i = buttons->priv->superscript_toggles; i; i = i->next) {
        GtkWidget *widget = i->data;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), mode == SUPERSCRIPT);
    }
    for (i = buttons->priv->subscript_toggles; i; i = i->next) {
        GtkWidget *widget = i->data;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), mode == SUBSCRIPT);
    }
}


static void
math_buttons_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    MathButtons *self;

    self = MATH_BUTTONS (object);

    switch (prop_id) {
    case PROP_EQUATION:
        self->priv->equation = g_value_get_object (value);
        math_buttons_set_mode(self, self->priv->mode);
        g_signal_connect(self->priv->equation, "notify::display", G_CALLBACK(display_changed_cb), self);
        g_signal_connect(self->priv->equation, "notify::number-mode", G_CALLBACK(number_mode_changed_cb), self);
        g_signal_connect(self->priv->equation, "notify::angle-units", G_CALLBACK(display_changed_cb), self);
        g_signal_connect(self->priv->equation, "notify::number-format", G_CALLBACK(display_changed_cb), self);
        number_mode_changed_cb(self->priv->equation, NULL, self);
        display_changed_cb(self->priv->equation, NULL, self);
        break;
    case PROP_MODE:
        math_buttons_set_mode(self, g_value_get_int (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}


static void
math_buttons_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    MathButtons *self;

    self = MATH_BUTTONS (object);

    switch (prop_id) {
    case PROP_EQUATION:
        g_value_set_object (value, self->priv->equation);
        break;
    case PROP_MODE:
        g_value_set_int (value, self->priv->mode);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}


static void
math_buttons_class_init (MathButtonsClass *klass)
{
    static GEnumValue button_mode_values[] =
    {
      {BASIC,       "basic",       "basic"},
      {ADVANCED,    "advanced",    "advanced"},
      {FINANCIAL,   "financial",   "financial"},
      {PROGRAMMING, "programming", "programming"},
      {0, NULL, NULL}
    };
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = math_buttons_get_property;
    object_class->set_property = math_buttons_set_property;

    g_type_class_add_private (klass, sizeof (MathButtonsPrivate));

    button_mode_type = g_enum_register_static("ButtonMode", button_mode_values);

    g_object_class_install_property (object_class,
                                     PROP_EQUATION,
                                     g_param_spec_object ("equation",
                                                          "equation",
                                                          "Equation being controlled",
                                                          math_equation_get_type(),
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (object_class,
                                     PROP_MODE,
                                     g_param_spec_enum ("mode",
                                                        "mode",
                                                        "Button mode",
                                                        button_mode_type,
                                                        BASIC,
                                                        G_PARAM_READWRITE));
}


static void
math_buttons_init (MathButtons *buttons)
{
    buttons->priv = G_TYPE_INSTANCE_GET_PRIVATE (buttons, math_buttons_get_type(), MathButtonsPrivate);
    buttons->priv->programming_base = 10;
    gdk_color_parse("#0000FF", &buttons->priv->color_numbers);
    gdk_color_parse("#00FF00", &buttons->priv->color_action);
    gdk_color_parse("#FF0000", &buttons->priv->color_operator);
    gdk_color_parse("#00FFFF", &buttons->priv->color_function);
    gdk_color_parse("#FF00FF", &buttons->priv->color_memory);
    gdk_color_parse("#FFFFFF", &buttons->priv->color_group);
    g_signal_connect(G_OBJECT(buttons), "show", G_CALLBACK(load_buttons), NULL);
}
