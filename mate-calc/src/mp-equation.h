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

#ifndef MP_EQUATION_H
#define MP_EQUATION_H

#include "mp.h"

typedef enum
{
    PARSER_ERR_NONE = 0,
    PARSER_ERR_INVALID,
    PARSER_ERR_OVERFLOW,
    PARSER_ERR_UNKNOWN_VARIABLE,
    PARSER_ERR_UNKNOWN_FUNCTION,
    PARSER_ERR_UNKNOWN_CONVERSION,
    PARSER_ERR_MP
} MPErrorCode;

/* Options for parser */
typedef struct {
    /* Default number base */
    int base;

    /* The wordlength for binary operations in bits (e.g. 8, 16, 32) */
    int wordlen;

    /* Units for angles (e.g. radians, degrees) */
    MPAngleUnit angle_units;

    // FIXME:
    // int enable_builtins;

    /* Data to pass to callbacks */
    void *callback_data;
  
    /* Function to check if a variable is defined */
    int (*variable_is_defined)(const char *name, void *data);

    /* Function to get variable values */
    int (*get_variable)(const char *name, MPNumber *z, void *data);

    /* Function to set variable values */
    void (*set_variable)(const char *name, const MPNumber *x, void *data);

    /* Function to check if a function is defined */
    int (*function_is_defined)(const char *name, void *data);

    /* Function to solve functions */
    int (*get_function)(const char *name, const MPNumber *x, MPNumber *z, void *data);

    /* Function to convert units */
    int (*convert)(const MPNumber *x, const char *x_units, const char *z_units, MPNumber *z, void *data);
} MPEquationOptions;

MPErrorCode mp_equation_parse(const char *expression, MPEquationOptions *options, MPNumber *result, char **error_token);
const char *mp_error_code_to_string(MPErrorCode error_code);

int sub_atoi(const char *data);
int super_atoi(const char *data);
#endif
