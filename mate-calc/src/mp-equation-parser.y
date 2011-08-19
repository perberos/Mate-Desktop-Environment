%{
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#include "mp-equation-private.h"
#include "mp-equation-parser.h"
#include "mp-equation-lexer.h"

// fixme support x log x
// treat exp NAME exp as a function always and pass both arguments, i.e.
// can do mod using both and all others use $1 * NAME($3)

static void set_error(yyscan_t yyscanner, int error, const char *token)
{
    _mp_equation_get_extra(yyscanner)->error = error;
    if (token)
        _mp_equation_get_extra(yyscanner)->error_token = strdup(token);
}

static void set_result(yyscan_t yyscanner, const MPNumber *x)
{
    mp_set_from_mp(x, &(_mp_equation_get_extra(yyscanner))->ret);
}

static char *
utf8_next_char (const char *c)
{
    c++;
    while ((*c & 0xC0) == 0x80)
        c++;
    return (char *)c;
}

static int get_variable(yyscan_t yyscanner, const char *name, int power, MPNumber *z)
{
    int result = 0;

    /* If defined, then get the variable */
    if (_mp_equation_get_extra(yyscanner)->get_variable(_mp_equation_get_extra(yyscanner), name, z)) {
        mp_xpowy_integer(z, power, z);
        return 1;
    }
    
    /* If has more than one character then assume a multiplication of variables */
    if (utf8_next_char(name)[0] != '\0') {
        const char *c, *next;
        char *buffer = malloc(sizeof(char) * strlen(name));
        MPNumber value;

        result = 1;
        mp_set_from_integer(1, &value);
        for (c = name; *c != '\0'; c = next) {
            MPNumber t;

            next = utf8_next_char(c);
            snprintf(buffer, next - c + 1, "%s", c);

            if (!_mp_equation_get_extra(yyscanner)->get_variable(_mp_equation_get_extra(yyscanner), buffer, &t)) {
                result = 0;
                break;
            }

            /* If last term do power */
            if (*next == '\0')
                mp_xpowy_integer(&t, power, &t);

            mp_multiply(&value, &t, &value);
        }

        free(buffer);
        if (result)
            mp_set_from_mp(&value, z);
    }

    if (!result)
        set_error(yyscanner, PARSER_ERR_UNKNOWN_VARIABLE, name);

    return result;
}

static void set_variable(yyscan_t yyscanner, const char *name, MPNumber *x)
{
    _mp_equation_get_extra(yyscanner)->set_variable(_mp_equation_get_extra(yyscanner), name, x);
}

static int get_function(yyscan_t yyscanner, const char *name, const MPNumber *x, MPNumber *z)
{
    if (!_mp_equation_get_extra(yyscanner)->get_function(_mp_equation_get_extra(yyscanner), name, x, z)) {
        set_error(yyscanner, PARSER_ERR_UNKNOWN_FUNCTION, name);
        return 0;
    }
    return 1;
}

static int get_inverse_function(yyscan_t yyscanner, const char *name, const MPNumber *x, MPNumber *z)
{
    char *inv_name;
    int result;
    
    inv_name = malloc(sizeof(char) * (strlen(name) + strlen("⁻¹") + 1));
    strcpy(inv_name, name);
    strcat(inv_name, "⁻¹");
    result = get_function(yyscanner, inv_name, x, z);
    free(inv_name);

    return result;
}

static void do_not(yyscan_t yyscanner, const MPNumber *x, MPNumber *z)
{
    if (!mp_is_overflow(x, _mp_equation_get_extra(yyscanner)->options->wordlen)) {
        set_error(yyscanner, PARSER_ERR_OVERFLOW, NULL);
    }
    mp_not(x, _mp_equation_get_extra(yyscanner)->options->wordlen, z);
}

static char *make_unit(const char *name, int power)
{
    char *name2;

    // FIXME: Hacky
    if (power == 2) {
        name2 = malloc(sizeof(char) * (strlen(name) + strlen("²") + 1));
        sprintf(name2, "%s²", name);
    }
    else if (power == 3) {
        name2 = malloc(sizeof(char) * (strlen(name) + strlen("³") + 1));
        sprintf(name2, "%s³", name);
    }
    else {
        name2 = malloc(sizeof(char) * (strlen(name) + strlen("?") + 1));
        sprintf(name2, "%s?", name);
    }
    
    return name2;
}

static void do_conversion(yyscan_t yyscanner, const MPNumber *x, const char *x_units, const char *z_units, MPNumber *z)
{
    if (!_mp_equation_get_extra(yyscanner)->convert(_mp_equation_get_extra(yyscanner), x, x_units, z_units, z))
        set_error(yyscanner, PARSER_ERR_UNKNOWN_CONVERSION, NULL);
}

%}

%pure-parser
%name-prefix="_mp_equation_"
%locations
%parse-param {yyscan_t yyscanner}
%lex-param {yyscan_t yyscanner}

%union {
  MPNumber int_t;
  int integer;
  char *name;
}

%left <int_t> tNUMBER
%left tLFLOOR tRFLOOR tLCEILING tRCEILING
%left UNARY_PLUS
%left tADD tSUBTRACT
%left tAND tOR tXOR tXNOR
%left tMULTIPLY tDIVIDE tMOD MULTIPLICATION
%left tNOT
%left tROOT tROOT3 tROOT4
%left <name> tVARIABLE tFUNCTION
%right <integer> tSUBNUM tSUPNUM tNSUPNUM
%left BOOLEAN_OPERATOR
%left PERCENTAGE
%left UNARY_MINUS
%right '^' '!' '|'
%left tIN

%type <int_t> exp variable term
%type <name> unit
%start statement

%%

statement:
  exp { set_result(yyscanner, &$1); }
| exp '=' { set_result(yyscanner, &$1); }
| tVARIABLE '=' exp {set_variable(yyscanner, $1, &$3); set_result(yyscanner, &$3); }
| tNUMBER unit tIN unit { MPNumber t; do_conversion(yyscanner, &$1, $2, $4, &t); set_result(yyscanner, &t); free($2); free($4); }
| unit tIN unit { MPNumber x, t; mp_set_from_integer(1, &x); do_conversion(yyscanner, &x, $1, $3, &t); set_result(yyscanner, &t); free($1); free($3); }
;

unit:
  tVARIABLE {$$ = $1;}
| tVARIABLE tSUPNUM {$$ = make_unit($1, $2); free($1);}

/* |x| gets confused and thinks = |x|(...||) */

exp:
  '(' exp ')' {mp_set_from_mp(&$2, &$$);}
| exp '(' exp ')' {mp_multiply(&$1, &$3, &$$);}
| tLFLOOR exp tRFLOOR {mp_floor(&$2, &$$);}
| tLCEILING exp tRCEILING {mp_ceiling(&$2, &$$);}
| '[' exp ']' {mp_round(&$2, &$$);}
| '{' exp '}' {mp_fractional_part(&$2, &$$);}
| '|' exp '|' {mp_abs(&$2, &$$);}
| exp '^' exp {mp_xpowy(&$1, &$3, &$$);}
| exp tSUPNUM {mp_xpowy_integer(&$1, $2, &$$);}
| exp tNSUPNUM {mp_xpowy_integer(&$1, $2, &$$);}
| exp '!' {mp_factorial(&$1, &$$);}
| variable {mp_set_from_mp(&$1, &$$);}
| tNUMBER variable %prec MULTIPLICATION {mp_multiply(&$1, &$2, &$$);}
| tSUBTRACT exp %prec UNARY_MINUS {mp_invert_sign(&$2, &$$);}
| tADD tNUMBER %prec UNARY_PLUS {mp_set_from_mp(&$2, &$$);}
| exp tDIVIDE exp {mp_divide(&$1, &$3, &$$);}
| exp tMOD exp {mp_modulus_divide(&$1, &$3, &$$);}
| exp tMULTIPLY exp {mp_multiply(&$1, &$3, &$$);}
| exp tADD exp '%' %prec PERCENTAGE {mp_add_integer(&$3, 100, &$3); mp_divide_integer(&$3, 100, &$3); mp_multiply(&$1, &$3, &$$);}
| exp tSUBTRACT exp '%' %prec PERCENTAGE {mp_add_integer(&$3, -100, &$3); mp_divide_integer(&$3, -100, &$3); mp_multiply(&$1, &$3, &$$);}
| exp tADD exp {mp_add(&$1, &$3, &$$);}
| exp tSUBTRACT exp {mp_subtract(&$1, &$3, &$$);}
| exp '%' {mp_divide_integer(&$1, 100, &$$);}
| tNOT exp {do_not(yyscanner, &$2, &$$);}
| exp tAND exp %prec BOOLEAN_OPERATOR {mp_and(&$1, &$3, &$$);}
| exp tOR exp %prec BOOLEAN_OPERATOR {mp_or(&$1, &$3, &$$);}
| exp tXOR exp %prec BOOLEAN_OPERATOR {mp_xor(&$1, &$3, &$$);}
| tNUMBER {mp_set_from_mp(&$1, &$$);}
;


variable:
  term {mp_set_from_mp(&$1, &$$);}
| tFUNCTION exp {if (!get_function(yyscanner, $1, &$2, &$$)) YYABORT; free($1);}
| tFUNCTION tSUPNUM exp {if (!get_function(yyscanner, $1, &$3, &$$)) YYABORT; mp_xpowy_integer(&$$, $2, &$$); free($1);}
| tFUNCTION tNSUPNUM exp {if (!get_inverse_function(yyscanner, $1, &$3, &$$)) YYABORT; mp_xpowy_integer(&$$, -$2, &$$); free($1);}
| tVARIABLE tSUPNUM exp {set_error(yyscanner, PARSER_ERR_UNKNOWN_FUNCTION, $1); free($1); YYABORT;}
| tSUBNUM tROOT exp {mp_root(&$3, $1, &$$);}
| tROOT exp {mp_sqrt(&$2, &$$);}
| tROOT3 exp {mp_root(&$2, 3, &$$);}
| tROOT4 exp {mp_root(&$2, 4, &$$);}
;

term:
  tVARIABLE {if (!get_variable(yyscanner, $1, 1, &$$)) YYABORT; free($1);}
| tVARIABLE tSUPNUM {if (!get_variable(yyscanner, $1, $2, &$$)) YYABORT; free($1);}
| term term {mp_multiply(&$1, &$2, &$$);}
;

%%
