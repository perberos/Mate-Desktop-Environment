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

#ifndef MATH_WINDOW_H
#define MATH_WINDOW_H

#include <glib-object.h>
#include "math-equation.h"
#include "math-display.h"
#include "math-buttons.h"

G_BEGIN_DECLS

#define MATH_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), math_window_get_type(), MathWindow))

typedef struct MathWindowPrivate MathWindowPrivate;

typedef struct
{
    GtkWindow parent_instance;
    MathWindowPrivate *priv;
} MathWindow;

typedef struct
{
    GtkWindowClass parent_class;

    void (*quit)(MathWindow *window);
} MathWindowClass;

GType math_window_get_type(void);

MathWindow *math_window_new(MathEquation *equation);

GtkWidget *math_window_get_menu_bar(MathWindow *window);

MathEquation *math_window_get_equation(MathWindow *window);

MathDisplay *math_window_get_display(MathWindow *window);

MathButtons *math_window_get_buttons(MathWindow *window);

void math_window_critical_error(MathWindow *window, const gchar *title, const gchar *contents);

#endif /* MATH_WINDOW_H */
