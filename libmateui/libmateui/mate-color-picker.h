/*
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/* Color picker button for MATE
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef MATE_COLOR_PICKER_H
#define MATE_COLOR_PICKER_H

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The MateColorPicker widget is a simple color picker in a button.  The button displays a sample
 * of the currently selected color.  When the user clicks on the button, a color selection dialog
 * pops up.  The color picker emits the "color_set" signal when the color is set
 *
 * By default, the color picker does dithering when drawing the color sample box.  This can be
 * disabled for cases where it is useful to see the allocated color without dithering.
 */

#define MATE_TYPE_COLOR_PICKER            (mate_color_picker_get_type ())
#define MATE_COLOR_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_COLOR_PICKER, MateColorPicker))
#define MATE_COLOR_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_COLOR_PICKER, MateColorPickerClass))
#define MATE_IS_COLOR_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_COLOR_PICKER))
#define MATE_IS_COLOR_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_COLOR_PICKER))
#define MATE_COLOR_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_COLOR_PICKER, MateColorPickerClass))


typedef struct _MateColorPicker        MateColorPicker;
typedef struct _MateColorPickerPrivate MateColorPickerPrivate;
typedef struct _MateColorPickerClass   MateColorPickerClass;

struct _MateColorPicker {
	GtkButton button;

	/*< private >*/
	MateColorPickerPrivate *_priv;
};

struct _MateColorPickerClass {
	GtkButtonClass parent_class;

	/* Signal that is emitted when the color is set.  The rgba values
	 * are in the [0, 65535] range.  If you need a different color
	 * format, use the provided functions to get the values from the
	 * color picker.
	 */
        /*  (should be gushort, but Gtk can't marshal that.) */
	void (* color_set) (MateColorPicker *cp, guint r, guint g, guint b, guint a);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

/* Standard Gtk function */
GType mate_color_picker_get_type (void) G_GNUC_CONST;

/* Creates a new color picker widget */
GtkWidget *mate_color_picker_new (void);

/* Set/get the color in the picker.  Values are in [0.0, 1.0] */
void mate_color_picker_set_d (MateColorPicker *cp, gdouble r, gdouble g, gdouble b, gdouble a);
void mate_color_picker_get_d (MateColorPicker *cp, gdouble *r, gdouble *g, gdouble *b, gdouble *a);

/* Set/get the color in the picker.  Values are in [0, 255] */
void mate_color_picker_set_i8 (MateColorPicker *cp, guint8 r, guint8 g, guint8 b, guint8 a);
void mate_color_picker_get_i8 (MateColorPicker *cp, guint8 *r, guint8 *g, guint8 *b, guint8 *a);

/* Set/get the color in the picker.  Values are in [0, 65535] */
void mate_color_picker_set_i16 (MateColorPicker *cp, gushort r, gushort g, gushort b, gushort a);
void mate_color_picker_get_i16 (MateColorPicker *cp, gushort *r, gushort *g, gushort *b, gushort *a);

/* Sets whether the picker should dither the color sample or just paint a solid rectangle */
void mate_color_picker_set_dither (MateColorPicker *cp, gboolean dither);
gboolean mate_color_picker_get_dither (MateColorPicker *cp);

/* Sets whether the picker should use the alpha channel or not */
void mate_color_picker_set_use_alpha (MateColorPicker *cp, gboolean use_alpha);
gboolean mate_color_picker_get_use_alpha (MateColorPicker *cp);

/* Sets the title for the color selection dialog */
void mate_color_picker_set_title (MateColorPicker *cp, const gchar *title);
const char * mate_color_picker_get_title (MateColorPicker *cp);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* MATE_COLOR_PICKER_H */
