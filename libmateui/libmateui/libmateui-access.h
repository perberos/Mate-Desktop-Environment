/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright 2002 Sun Microsystems Inc.
 *
 * Lib MATE UI Accessibility support module
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __LIBMATEUI_ACCESS_H__
#define __LIBMATEUI_ACCESS_H__

#include <glib-object.h>
#include <atk/atk.h>
#include <gtk/gtk.h>

void _add_atk_name_desc   (GtkWidget *widget, gchar *name, gchar *desc);
void _add_atk_description (GtkWidget *widget, gchar *desc);
void _add_atk_relation    (GtkWidget *widget1, GtkWidget *widget2,
			   AtkRelationType w1_to_w2, AtkRelationType w2_to_w1);



/* Copied from eel */

typedef void     (*_AccessibilityClassInitFn)    (AtkObjectClass *klass);

AtkObject    *_accessibility_get_atk_object        (gpointer              object);
AtkObject    *_accessibility_for_object            (gpointer              object);
gpointer      _accessibility_get_gobject           (AtkObject            *object);
AtkObject    *_accessibility_set_atk_object_return (gpointer              object,
						    AtkObject            *atk_object);
GType         _accessibility_create_derived_type   (const char           *type_name,
						    GType                 existing_gobject_with_proxy,
						    _AccessibilityClassInitFn class_init);


#endif /* ! __LIBMATEUI_ACCESS_H__ */
