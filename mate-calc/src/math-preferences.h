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

#ifndef MATH_PREFERENCES_H
#define MATH_PREFERENCES_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "math-equation.h"

G_BEGIN_DECLS

#define MATH_PREFERENCES(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), math_preferences_get_type(), MathPreferencesDialog))

typedef struct MathPreferencesDialogPrivate MathPreferencesDialogPrivate;

typedef struct
{
    GtkDialog                 parent_instance;
    MathPreferencesDialogPrivate *priv;
} MathPreferencesDialog;

typedef struct
{
    GtkDialogClass parent_class;
} MathPreferencesDialogClass;

GType math_preferences_get_type(void);

MathPreferencesDialog *math_preferences_dialog_new(MathEquation *equation);

#endif /* MATH_PREFERENCES_H */
