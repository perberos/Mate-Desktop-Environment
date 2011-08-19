/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _EOG_FILE_CHOOSER_H_
#define _EOG_FILE_CHOOSER_H_

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define EOG_TYPE_FILE_CHOOSER          (eog_file_chooser_get_type ())
#define EOG_FILE_CHOOSER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_FILE_CHOOSER, EogFileChooser))
#define EOG_FILE_CHOOSER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOG_TYPE_FILE_CHOOSER, EogFileChooserClass))

#define EOG_IS_FILE_CHOOSER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_FILE_CHOOSER))
#define EOG_IS_FILE_CHOOSER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOG_TYPE_FILE_CHOOSER))
#define EOG_FILE_CHOOSER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_FILE_CHOOSER, EogFileChooserClass))

typedef struct _EogFileChooser         EogFileChooser;
typedef struct _EogFileChooserClass    EogFileChooserClass;
typedef struct _EogFileChooserPrivate  EogFileChooserPrivate;

struct _EogFileChooser
{
	GtkFileChooserDialog  parent;

	EogFileChooserPrivate *priv;
};

struct _EogFileChooserClass
{
	GtkFileChooserDialogClass  parent_class;
};


GType		 eog_file_chooser_get_type	(void) G_GNUC_CONST;

GtkWidget	*eog_file_chooser_new		(GtkFileChooserAction action);

GdkPixbufFormat	*eog_file_chooser_get_format	(EogFileChooser *chooser);


G_END_DECLS

#endif /* _EOG_FILE_CHOOSER_H_ */
