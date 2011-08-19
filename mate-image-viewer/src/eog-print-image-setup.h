/* Eye Of MATE -- Print Dialog Custom Widget
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "eog-image.h"

#ifndef EOG_PRINT_IMAGE_SETUP_H
#define EOG_PRINT_IMAGE_SETUP_H

G_BEGIN_DECLS

typedef struct _EogPrintImageSetup         EogPrintImageSetup;
typedef struct _EogPrintImageSetupClass    EogPrintImageSetupClass;
typedef struct EogPrintImageSetupPrivate   EogPrintImageSetupPrivate;

#define EOG_TYPE_PRINT_IMAGE_SETUP            (eog_print_image_setup_get_type ())
#define EOG_PRINT_IMAGE_SETUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_PRINT_IMAGE_SETUP, EogPrintImageSetup))
#define EOG_PRINT_IMAGE_SETUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EOG_TYPE_PRINT_IMAGE_SETUP, EogPrintImageSetupClass))
#define EOG_IS_PRINT_IMAGE_SETUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_PRINT_IMAGE_SETUP))
#define EOG_IS_PRINT_IMAGE_SETUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EOG_TYPE_PRINT_IMAGE_SETUP))
#define EOG_PRINT_IMAGE_SETUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EOG_TYPE_PRINT_IMAGE_SETUP, EogPrintImageSetupClass))

struct _EogPrintImageSetup {
	GtkTable parent_instance;

	EogPrintImageSetupPrivate *priv;
};

struct _EogPrintImageSetupClass {
	GtkTableClass parent_class;
};

G_GNUC_INTERNAL
GType		  eog_print_image_setup_get_type    (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget        *eog_print_image_setup_new         (EogImage     *image,
						     GtkPageSetup *page_setup);

G_GNUC_INTERNAL
void              eog_print_image_setup_get_options (EogPrintImageSetup *setup,
						     gdouble            *left,
						     gdouble            *top,
						     gdouble            *scale,
						     GtkUnit            *unit);
void              eog_print_image_setup_update      (GtkPrintOperation *operation,
						     GtkWidget         *custom_widget,
						     GtkPageSetup      *page_setup,
						     GtkPrintSettings  *print_settings,
						     gpointer           user_data);

G_END_DECLS

#endif /* EOG_PRINT_IMAGE_SETUP_H */
