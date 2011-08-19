/*  Copyright (c) 2004-2008 Sami Pietila
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

#include <ctype.h>

#include "mp-equation-private.h"
#include "mp-equation-parser.h"
#include "mp-equation-lexer.h"

extern int _mp_equation_parse(yyscan_t yyscanner);


static int
variable_is_defined(MPEquationParserState *state, const char *name)
{
    /* FIXME: Make more generic */
    if (strcmp(name, "e") == 0 || strcmp(name, "i") == 0 || strcmp(name, "π") == 0)
        return 1;
    if (state->options->variable_is_defined)
        return state->options->variable_is_defined(name, state->options->callback_data);
    return 0;
}


static int
get_variable(MPEquationParserState *state, const char *name, MPNumber *z)
{
    int result = 1;

    if (strcmp(name, "e") == 0)
        mp_get_eulers(z);
    else if (strcmp(name, "i") == 0)
        mp_get_i(z);
    else if (strcmp(name, "π") == 0)
        mp_get_pi(z);
    else if (state->options->get_variable)
        result = state->options->get_variable(name, z, state->options->callback_data);
    else
        result = 0;

    return result;
}

static void
set_variable(MPEquationParserState *state, const char *name, const MPNumber *x)
{
    // Reserved words, e, π, mod, and, or, xor, not, abs, log, ln, sqrt, int, frac, sin, cos, ...
    if (strcmp(name, "e") == 0 || strcmp(name, "i") == 0 || strcmp(name, "π") == 0)
        return; // FALSE

    if (state->options->set_variable)
        state->options->set_variable(name, x, state->options->callback_data);
}

// FIXME: Accept "2sin" not "2 sin", i.e. let the tokenizer collect the multiple
// Parser then distinguishes between "sin"="s*i*n" or "sin5" = "sin 5" = "sin(5)"
// i.e. numbers+letters = variable or function depending on following arg
// letters+numbers = numbers+letters+numbers = function


int
sub_atoi(const char *data)
{
    int i, value = 0;
    const char *digits[] = {"₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉", NULL};

    do {
        for(i = 0; digits[i] != NULL && strncmp(data, digits[i], strlen(digits[i])) != 0; i++);
        if(digits[i] == NULL)
            return -1;
        data += strlen(digits[i]);
        value = value * 10 + i;
    } while(*data != '\0');

    return value;
}

int
super_atoi(const char *data)
{
   int i, sign = 1, value = 0;
   const char *digits[11] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹", NULL};

   if(strncmp(data, "⁻", strlen("⁻")) == 0) {
      sign = -1;
      data += strlen("⁻");
   }

   do {
      for(i = 0; digits[i] != NULL && strncmp(data, digits[i], strlen(digits[i])) != 0; i++);
      if(digits[i] == NULL)
         return 0;
      value = value * 10 + i;
      data += strlen(digits[i]);
   } while(*data != '\0');

   return sign * value;
}


static int
function_is_defined(MPEquationParserState *state, const char *name)
{
    char *c, *lower_name;

    lower_name = strdup(name);
    for (c = lower_name; *c; c++)
        *c = tolower(*c);

    /* FIXME: Make more generic */
    if (strcmp(lower_name, "log") == 0 ||
        (strncmp(lower_name, "log", 3) == 0 && sub_atoi(lower_name + 3) >= 0) ||
        strcmp(lower_name, "ln") == 0 ||
        strcmp(lower_name, "sqrt") == 0 ||
        strcmp(lower_name, "abs") == 0 ||
        strcmp(lower_name, "sgn") == 0 ||
        strcmp(lower_name, "arg") == 0 ||
        strcmp(lower_name, "conj") == 0 ||
        strcmp(lower_name, "int") == 0 ||
        strcmp(lower_name, "frac") == 0 ||
        strcmp(lower_name, "floor") == 0 ||
        strcmp(lower_name, "ceil") == 0 ||
        strcmp(lower_name, "round") == 0 ||
        strcmp(lower_name, "re") == 0 ||
        strcmp(lower_name, "im") == 0 ||
        strcmp(lower_name, "sin") == 0 || strcmp(lower_name, "cos") == 0 || strcmp(lower_name, "tan") == 0 ||
        strcmp(lower_name, "sin⁻¹") == 0 || strcmp(lower_name, "cos⁻¹") == 0 || strcmp(lower_name, "tan⁻¹") == 0 ||
        strcmp(lower_name, "sinh") == 0 || strcmp(lower_name, "cosh") == 0 || strcmp(lower_name, "tanh") == 0 ||
        strcmp(lower_name, "sinh⁻¹") == 0 || strcmp(lower_name, "cosh⁻¹") == 0 || strcmp(lower_name, "tanh⁻¹") == 0 ||
        strcmp(lower_name, "asinh") == 0 || strcmp(lower_name, "acosh") == 0 || strcmp(lower_name, "atanh") == 0 ||
        strcmp(lower_name, "ones") == 0 ||
        strcmp(lower_name, "twos") == 0) {
        g_free (lower_name);
        return 1;
    }
    g_free (lower_name);

    if (state->options->function_is_defined)
        return state->options->function_is_defined(name, state->options->callback_data);
    return 0;
}


static int
get_function(MPEquationParserState *state, const char *name, const MPNumber *x, MPNumber *z)
{
    char *c, *lower_name;
    int result = 1;

    lower_name = strdup(name);
    for (c = lower_name; *c; c++)
        *c = tolower(*c);

    // FIXME: Re Im ?

    if (strcmp(lower_name, "log") == 0)
        mp_logarithm(10, x, z); // FIXME: Default to ln
    else if (strncmp(lower_name, "log", 3) == 0) {
        int base;

        base = sub_atoi(lower_name + 3);
        if (base < 0)
            result = 0;
        else
            mp_logarithm(base, x, z);
    }
    else if (strcmp(lower_name, "ln") == 0)
        mp_ln(x, z);
    else if (strcmp(lower_name, "sqrt") == 0) // √x
        mp_sqrt(x, z);
    else if (strcmp(lower_name, "abs") == 0) // |x|
        mp_abs(x, z);
    else if (strcmp(lower_name, "sgn") == 0)
        mp_sgn(x, z);
    else if (strcmp(lower_name, "arg") == 0)
        mp_arg(x, state->options->angle_units, z);
    else if (strcmp(lower_name, "conj") == 0)
        mp_conjugate(x, z);
    else if (strcmp(lower_name, "int") == 0)
        mp_integer_component(x, z);
    else if (strcmp(lower_name, "frac") == 0)
        mp_fractional_component(x, z);
    else if (strcmp(lower_name, "floor") == 0)
        mp_floor(x, z);
    else if (strcmp(lower_name, "ceil") == 0)
        mp_ceiling(x, z);
    else if (strcmp(lower_name, "round") == 0)
        mp_round(x, z);
    else if (strcmp(lower_name, "re") == 0)
        mp_real_component(x, z);
    else if (strcmp(lower_name, "im") == 0)
        mp_imaginary_component(x, z);
    else if (strcmp(lower_name, "sin") == 0)
        mp_sin(x, state->options->angle_units, z);
    else if (strcmp(lower_name, "cos") == 0)
        mp_cos(x, state->options->angle_units, z);
    else if (strcmp(lower_name, "tan") == 0)
        mp_tan(x, state->options->angle_units, z);
    else if (strcmp(lower_name, "sin⁻¹") == 0 || strcmp(lower_name, "asin") == 0)
        mp_asin(x, state->options->angle_units, z);
    else if (strcmp(lower_name, "cos⁻¹") == 0 || strcmp(lower_name, "acos") == 0)
        mp_acos(x, state->options->angle_units, z);
    else if (strcmp(lower_name, "tan⁻¹") == 0 || strcmp(lower_name, "atan") == 0)
        mp_atan(x, state->options->angle_units, z);
    else if (strcmp(lower_name, "sinh") == 0)
        mp_sinh(x, z);
    else if (strcmp(lower_name, "cosh") == 0)
        mp_cosh(x, z);
    else if (strcmp(lower_name, "tanh") == 0)
        mp_tanh(x, z);
    else if (strcmp(lower_name, "sinh⁻¹") == 0 || strcmp(lower_name, "asinh") == 0)
        mp_asinh(x, z);
    else if (strcmp(lower_name, "cosh⁻¹") == 0 || strcmp(lower_name, "acosh") == 0)
        mp_acosh(x, z);
    else if (strcmp(lower_name, "tanh⁻¹") == 0 || strcmp(lower_name, "atanh") == 0)
        mp_atanh(x, z);
    else if (strcmp(lower_name, "ones") == 0)
        mp_ones_complement(x, state->options->wordlen, z);
    else if (strcmp(lower_name, "twos") == 0)
        mp_twos_complement(x, state->options->wordlen, z);
    else if (state->options->get_function)
        result = state->options->get_function(name, x, z, state->options->callback_data);
    else
        result = 0;

    free(lower_name);

    return result;
}


static int
do_convert(const char *units[][2], const MPNumber *x, const char *x_units, const char *z_units, MPNumber *z)
{
    int x_index, z_index;
    MPNumber x_factor, z_factor;
    
    for (x_index = 0; units[x_index][0] != NULL && strcmp(units[x_index][0], x_units) != 0; x_index++);
    if (units[x_index][0] == NULL)
        return 0;
    for (z_index = 0; units[z_index][0] != NULL && strcmp(units[z_index][0], z_units) != 0; z_index++);
    if (units[z_index][0] == NULL)
        return 0;

    mp_set_from_string(units[x_index][1], 10, &x_factor);
    mp_set_from_string(units[z_index][1], 10, &z_factor);
    mp_multiply(x, &x_factor, z);
    mp_divide(z, &z_factor, z);

    return 1;
}


static int
convert(MPEquationParserState *state, const MPNumber *x, const char *x_units, const char *z_units, MPNumber *z)
{
    const char *length_units[][2] = {
        {"parsec",     "30857000000000000"},
        {"parsecs",    "30857000000000000"},
        {"pc",         "30857000000000000"},
        {"lightyear",   "9460730472580800"},
        {"lightyears",  "9460730472580800"},
        {"ly",          "9460730472580800"},
        {"au",              "149597870691"},
        {"nm",                   "1852000"},
        {"mile",                    "1609.344"},
        {"miles",                   "1609.344"},
        {"mi",                      "1609.344"},
        {"kilometer",               "1000"},
        {"kilometers",              "1000"},
        {"km",                      "1000"},
        {"kms",                     "1000"},
        {"cable",                    "219.456"},
        {"cables",                   "219.456"},
        {"cb",                       "219.456"},
        {"fathom",                     "1.8288"},
        {"fathoms",                    "1.8288"},
        {"ftm",                        "1.8288"},
        {"meter",                      "1"},
        {"meters",                     "1"},
        {"m",                          "1"},
        {"yard",                       "0.9144"},
        {"yd",                         "0.9144"},
        {"foot",                       "0.3048"},
        {"feet",                       "0.3048"},
        {"ft",                         "0.3048"},
        {"inch",                       "0.0254"},
        {"inches",                     "0.0254"},
        {"centimeter",                 "0.01"},
        {"centimeters",                "0.01"},
        {"cm",                         "0.01"},
        {"cms",                        "0.01"},
        {"millimeter",                 "0.001"},
        {"millimeters",                "0.001"},
        {"mm",                         "0.001"},
        {"micrometer",                 "0.000001"},
        {"micrometers",                "0.000001"},
        {"um",                         "0.000001"},
        {"nanometer",                  "0.000000001"},
        {"nanometers",                 "0.000000001"},
        {NULL, NULL}
    };

    const char *area_units[][2] = {
        {"hectare",         "10000"},
        {"hectares",        "10000"},
        {"acre",             "4046.8564224"},
        {"acres",            "4046.8564224"},
        {"m²",                  "1"},
        {"cm²",                 "0.001"},
        {"mm²",                 "0.000001"},
        {NULL, NULL}
    };

    const char *volume_units[][2] = {
        {"m³",               "1000"},
        {"gallon",              "3.785412"},
        {"gallons",             "3.785412"},
        {"gal",                 "3.785412"},
        {"litre",               "1"},
        {"litres",              "1"},
        {"liter",               "1"},
        {"liters",              "1"},
        {"L",                   "1"},
        {"quart",               "0.9463529"},
        {"quarts",              "0.9463529"},
        {"qt",                  "0.9463529"},
        {"pint",                "0.4731765"},
        {"pints",               "0.4731765"},
        {"pt",                  "0.4731765"},
        {"millilitre",          "0.001"},
        {"millilitres",         "0.001"},
        {"milliliter",          "0.001"},
        {"milliliters",         "0.001"},
        {"mL",                  "0.001"},
        {"cm³",                 "0.001"},
        {"mm³",                 "0.000001"},
        {NULL, NULL}
    };

    const char *weight_units[][2] = {
        {"tonne",            "1000"},
        {"tonnes",           "1000"},
        {"kilograms",           "1"},
        {"kilogramme",          "1"},
        {"kilogrammes",         "1"},
        {"kg",                  "1"},
        {"kgs",                 "1"},
        {"pound",               "0.45359237"},
        {"pounds",              "0.45359237"},
        {"lb",                  "0.45359237"},
        {"ounce",               "0.002834952"},
        {"ounces",              "0.002834952"},
        {"oz",                  "0.002834952"},
        {"gram",                "0.001"},
        {"grams",               "0.001"},
        {"gramme",              "0.001"},
        {"grammes",             "0.001"},
        {"g",                   "0.001"},
        {NULL, NULL}
    };
    
    const char *time_units[][2] = {
        {"year",         "31557600"},
        {"years",        "31557600"},
        {"day",             "86400"},
        {"days",            "86400"},
        {"hour",             "3600"},
        {"hours",            "3600"},
        {"minute",             "60"},
        {"minutes",            "60"},
        {"second",              "1"},
        {"seconds",             "1"},
        {"s",                   "1"},
        {"millisecond",         "0.001"},
        {"milliseconds",        "0.001"},
        {"ms",                  "0.001"},
        {"microsecond",         "0.000001"},
        {"microseconds",        "0.000001"},
        {"us",                  "0.000001"},
        {NULL, NULL}
    };

    if (do_convert(length_units, x, x_units, z_units, z) ||
        do_convert(area_units, x, x_units, z_units, z) ||
        do_convert(volume_units, x, x_units, z_units, z) ||
        do_convert(weight_units, x, x_units, z_units, z) ||
        do_convert(time_units, x, x_units, z_units, z))
        return 1;

    if (state->options->convert)
        return state->options->convert(x, x_units, z_units, z, state->options->callback_data);
  
    return 0;
}


MPErrorCode
mp_equation_parse(const char *expression, MPEquationOptions *options, MPNumber *result, char **error_token)
{
    int ret;
    MPEquationParserState state;
    yyscan_t yyscanner;
    YY_BUFFER_STATE buffer;

    if (!(expression && result) || strlen(expression) == 0)
        return PARSER_ERR_INVALID;

    memset(&state, 0, sizeof(MPEquationParserState));
    state.options = options;
    state.variable_is_defined = variable_is_defined;
    state.get_variable = get_variable;
    state.set_variable = set_variable;
    state.function_is_defined = function_is_defined;
    state.get_function = get_function;
    state.convert = convert;
    state.error = 0;

    mp_clear_error();

    _mp_equation_lex_init_extra(&state, &yyscanner);
    buffer = _mp_equation__scan_string(expression, yyscanner);

    ret = _mp_equation_parse(yyscanner);
    if (state.error_token != NULL && error_token != NULL) {
        *error_token = state.error_token;
    }

    _mp_equation__delete_buffer(buffer, yyscanner);
    _mp_equation_lex_destroy(yyscanner);

    /* Error during parsing */
    if (state.error)
        return state.error;

    if (mp_get_error())
        return PARSER_ERR_MP;

    /* Failed to parse */
    if (ret)
        return PARSER_ERR_INVALID;

    mp_set_from_mp(&state.ret, result);

    return PARSER_ERR_NONE;
}


const char *
mp_error_code_to_string(MPErrorCode error_code)
{
    switch(error_code)
    {
    case PARSER_ERR_NONE:
        return "PARSER_ERR_NONE";
    case PARSER_ERR_INVALID:
        return "PARSER_ERR_INVALID";
    case PARSER_ERR_OVERFLOW:
        return "PARSER_ERR_OVERFLOW";
    case PARSER_ERR_UNKNOWN_VARIABLE:
        return "PARSER_ERR_UNKNOWN_VARIABLE";
    case PARSER_ERR_UNKNOWN_FUNCTION:
        return "PARSER_ERR_UNKNOWN_FUNCTION";
    case PARSER_ERR_UNKNOWN_CONVERSION:
        return "PARSER_ERR_UNKNOWN_CONVERSION";
    case PARSER_ERR_MP:
        return "PARSER_ERR_MP";
    default:
        return "Unknown parser error";
    }
}


int _mp_equation_error(void *yylloc, MPEquationParserState *state, char *text)
{
    return 0;
}
