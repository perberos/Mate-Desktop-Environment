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

#ifndef MATH_DISPLAY_H
#define MATH_DISPLAY_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "math-equation.h"

G_BEGIN_DECLS

#define MATH_DISPLAY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), math_display_get_type(), MathDisplay))

typedef struct MathDisplayPrivate MathDisplayPrivate;

typedef struct
{
    GtkVBox parent_instance;
    MathDisplayPrivate *priv;
} MathDisplay;

typedef struct
{
    GtkVBoxClass parent_class;
} MathDisplayClass;

GType math_display_get_type(void);

MathDisplay *math_display_new(void);

MathDisplay *math_display_new_with_equation(MathEquation *equation);

MathEquation *math_display_get_equation(MathDisplay *display);

#endif /* MATH_DISPLAY_H */
