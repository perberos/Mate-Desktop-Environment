/* Eye of Mate - Print Operations
 *
 * Copyright (C) 2005-2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@mate.org>
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

#ifndef __EOG_PRINT_H__
#define __EOG_PRINT_H__

#include "eog-image.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
GtkPrintOperation*    eog_print_operation_new (EogImage *image,
					       GtkPrintSettings *print_settings,
					       GtkPageSetup *page_setup);

G_GNUC_INTERNAL
GtkPageSetup*         eog_print_get_page_setup (void);

G_GNUC_INTERNAL
void                  eog_print_set_page_setup (GtkPageSetup *page_setup);

G_GNUC_INTERNAL
GtkPrintSettings *    eog_print_get_print_settings (void);

G_GNUC_INTERNAL
void                  eog_print_set_print_settings (GtkPrintSettings *print_settings);

G_END_DECLS

#endif /* __EOG_PRINT_H__ */
