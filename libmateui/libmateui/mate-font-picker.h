/* MATE font picker button.
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nexo.es>
 * All rights reserved
 * Based on mate-color-picker by Federico Mena <federico@nuclecu.unam.mx>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#ifndef MATE_FONT_PICKER_H
#define MATE_FONT_PICKER_H

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* MateFontPicker is a button widget that allow user to select a font.
 */

/* Button Mode or What to show */
typedef enum {
    MATE_FONT_PICKER_MODE_PIXMAP,
    MATE_FONT_PICKER_MODE_FONT_INFO,
    MATE_FONT_PICKER_MODE_USER_WIDGET,
    MATE_FONT_PICKER_MODE_UNKNOWN
} MateFontPickerMode;
        
#define MATE_TYPE_FONT_PICKER            (mate_font_picker_get_type ())
#define MATE_FONT_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_FONT_PICKER, MateFontPicker))
#define MATE_FONT_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_FONT_PICKER, MateFontPickerClass))
#define MATE_IS_FONT_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_FONT_PICKER))
#define MATE_IS_FONT_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_FONT_PICKER))
#define MATE_FONT_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_FONT_PICKER, MateFontPickerClass))

typedef struct _MateFontPicker        MateFontPicker;
typedef struct _MateFontPickerPrivate MateFontPickerPrivate;
typedef struct _MateFontPickerClass   MateFontPickerClass;

struct _MateFontPicker {
        GtkButton button;
    
	/*< private >*/
	MateFontPickerPrivate *_priv;
};

struct _MateFontPickerClass {
	GtkButtonClass parent_class;

	/* font_set signal is emitted when font is chosen */
	void (* font_set) (MateFontPicker *gfp, const gchar *font_name);

	/* It is possible we may need more signals */
	gpointer padding1;
	gpointer padding2;
};


/* Standard Gtk function */
GType mate_font_picker_get_type (void) G_GNUC_CONST;

/* Creates a new font picker widget */
GtkWidget *mate_font_picker_new (void);

/* Sets the title for the font selection dialog */
void       mate_font_picker_set_title       (MateFontPicker *gfp,
					      const gchar *title);
const gchar * mate_font_picker_get_title    (MateFontPicker *gfp);

/* Button mode */
MateFontPickerMode
           mate_font_picker_get_mode        (MateFontPicker *gfp);

void       mate_font_picker_set_mode        (MateFontPicker *gfp,
                                              MateFontPickerMode mode);
/* With  MATE_FONT_PICKER_MODE_FONT_INFO */
/* If use_font_in_label is true, font name will be written using font chosen by user and
 using size passed to this function*/
void       mate_font_picker_fi_set_use_font_in_label (MateFontPicker *gfp,
                                                       gboolean use_font_in_label,
                                                       gint size);

void       mate_font_picker_fi_set_show_size (MateFontPicker *gfp,
                                               gboolean show_size);

/* With MATE_FONT_PICKER_MODE_USER_WIDGET */
void       mate_font_picker_uw_set_widget    (MateFontPicker *gfp,
                                               GtkWidget       *widget);
GtkWidget * mate_font_picker_uw_get_widget    (MateFontPicker *gfp);

/* Functions to interface with GtkFontSelectionDialog */
const gchar* mate_font_picker_get_font_name  (MateFontPicker *gfp);

GdkFont*   mate_font_picker_get_font	      (MateFontPicker *gfp);

gboolean   mate_font_picker_set_font_name    (MateFontPicker *gfp,
                                               const gchar     *fontname);

const gchar* mate_font_picker_get_preview_text (MateFontPicker *gfp);

void	   mate_font_picker_set_preview_text (MateFontPicker *gfp,
                                               const gchar     *text);

G_END_DECLS

#endif /* GTK_DISABLE_DEPRECATED */

#endif /* MATE_FONT_PICKER_H */

