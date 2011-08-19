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

#ifndef MATH_BUTTONS_H
#define MATH_BUTTONS_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "math-equation.h"

G_BEGIN_DECLS

#define MATH_BUTTONS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), math_buttons_get_type(), MathButtons))

typedef struct MathButtonsPrivate MathButtonsPrivate;

typedef struct
{
    GtkVBox parent_instance;
    MathButtonsPrivate *priv;
} MathButtons;

typedef struct
{
    GtkVBoxClass parent_class;
} MathButtonsClass;

typedef enum {
    BASIC,
    ADVANCED,
    FINANCIAL,
    PROGRAMMING
} ButtonMode;

GType math_buttons_get_type(void);

MathButtons *math_buttons_new(MathEquation *equation);

void math_buttons_set_mode(MathButtons *buttons, ButtonMode mode);

ButtonMode math_buttons_get_mode(MathButtons *buttons);

void math_buttons_set_programming_base(MathButtons *buttons, gint base);

gint math_buttons_get_programming_base(MathButtons *buttons);

#endif /* MATH_BUTTONS_H */
