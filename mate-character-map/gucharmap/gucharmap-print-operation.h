/*
*  Copyright © 2007, 2008 Christian Persch
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3, or (at your option)
*  any later version.
*
*  This program is distributed in the hope print_operation it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef GUCHARMAP_PRINT_OPERATION_H
#define GUCHARMAP_PRINT_OPERATION_H

#include <gtk/gtk.h>

#include <gucharmap/gucharmap.h>

G_BEGIN_DECLS

#define GUCHARMAP_TYPE_PRINT_OPERATION          (gucharmap_print_operation_get_type ())
#define GUCHARMAP_PRINT_OPERATION(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), GUCHARMAP_TYPE_PRINT_OPERATION, GucharmapPrintOperation))
#define GUCHARMAP_PRINT_OPERATION_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), GUCHARMAP_TYPE_PRINT_OPERATION, GucharmapPrintOperationClass))
#define GUCHARMAP_IS_PRINT_OPERATION(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), GUCHARMAP_TYPE_PRINT_OPERATION))
#define GUCHARMAP_IS_PRINT_OPERATION_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), GUCHARMAP_TYPE_PRINT_OPERATION))
#define GUCHARMAP_PRINT_OPERATION_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), GUCHARMAP_TYPE_PRINT_OPERATION, GucharmapPrintOperationClass))

typedef struct _GucharmapPrintOperation         GucharmapPrintOperation;
typedef struct _GucharmapPrintOperationClass    GucharmapPrintOperationClass;
typedef struct _GucharmapPrintOperationPrivate  GucharmapPrintOperationPrivate;

struct _GucharmapPrintOperation
{
  GtkPrintOperation parent_instance;

  /*< private >*/
  GucharmapPrintOperationPrivate *priv;
};

struct _GucharmapPrintOperationClass
{
  GtkPrintOperationClass parent_class;
};

GType               gucharmap_print_operation_get_type (void);

GtkPrintOperation * gucharmap_print_operation_new      (GucharmapCodepointList *codepoint_list,
                                                        PangoFontDescription *font_desc);

G_END_DECLS

#endif /* !GUCHARMAP_PRINT_OPERATION_H */
