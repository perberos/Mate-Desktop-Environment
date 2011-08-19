/* screenshot-dialog.h - main MATE Screenshot dialog
 *
 * Copyright (C) 2001-2006  Jonathan Blandford <jrb@alum.mit.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#ifndef __SCREENSHOT_DIALOG_H__
#define __SCREENSHOT_DIALOG_H__

#include <gtk/gtk.h>

typedef struct ScreenshotDialog ScreenshotDialog;

/* Keep in sync with the value defined in the UI file */
#define SCREENSHOT_RESPONSE_COPY 1

ScreenshotDialog *screenshot_dialog_new          (GdkPixbuf        *screenshot,
						  char             *initial_uri,
						  gboolean          take_window_shot);
void              screenshot_dialog_enable_dnd   (ScreenshotDialog *dialog);
GtkWidget        *screenshot_dialog_get_toplevel (ScreenshotDialog *dialog);
char             *screenshot_dialog_get_uri      (ScreenshotDialog *dialog);
char             *screenshot_dialog_get_folder   (ScreenshotDialog *dialog);
GdkPixbuf        *screenshot_dialog_get_screenshot (ScreenshotDialog *dialog);
void              screenshot_dialog_set_busy     (ScreenshotDialog *dialog,
						  gboolean          busy);
void              screenshot_dialog_focus_entry  (ScreenshotDialog *dialog);

#endif /* __SCREENSHOT_DIALOG_H__ */
