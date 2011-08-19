/* OBox Copyright (C) 2002 Red Hat Inc. based on GtkHBox */
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __NA_OBOX_H__
#define __NA_OBOX_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define NA_TYPE_OBOX            (na_obox_get_type ())
#define NA_OBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NA_TYPE_OBOX, NaOBox))
#define NA_OBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NA_TYPE_OBOX, NaOBoxClass))
#define NA_IS_OBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NA_TYPE_OBOX))
#define NA_IS_OBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NA_TYPE_OBOX))
#define NA_OBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NA_TYPE_OBOX, NaOBoxClass))


typedef struct _NaOBox       NaOBox;
typedef struct _NaOBoxClass  NaOBoxClass;

struct _NaOBox
{
  GtkBox box;

  GtkOrientation orientation;
};

struct _NaOBoxClass
{
  GtkBoxClass parent_class;
};


GType	   na_obox_get_type (void) G_GNUC_CONST;
GtkWidget* na_obox_new      (void);

void na_obox_set_orientation (NaOBox         *obox,
                              GtkOrientation  orientation);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NA_OBOX_H__ */
