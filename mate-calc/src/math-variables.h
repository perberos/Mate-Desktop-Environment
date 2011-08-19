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

#ifndef MATH_VARIABLES_H
#define MATH_VARIABLES_H

#include <glib-object.h>
#include "mp.h"

G_BEGIN_DECLS

#define MATH_VARIABLES(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), math_equation_get_type(), MathVariables))

typedef struct MathVariablesPrivate MathVariablesPrivate;

typedef struct
{
    GObject parent_instance;
    MathVariablesPrivate *priv;
} MathVariables;

typedef struct
{
    GObjectClass parent_class;
} MathVariablesClass;

GType math_variables_get_type(void);

MathVariables *math_variables_new(void);

gchar **math_variables_get_names(MathVariables *variables);

void math_variables_set_value(MathVariables *variables, const char *name, const MPNumber *value);

MPNumber *math_variables_get_value(MathVariables *variables, const char *name);

#endif /* MATH_VARIABLES_H */
