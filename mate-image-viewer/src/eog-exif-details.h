/* Eye Of Mate - EOG Image Exif Details
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
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

#ifndef __EOG_EXIF_DETAILS__
#define __EOG_EXIF_DETAILS__

#include <glib-object.h>
#include <gtk/gtk.h>
#if HAVE_EXIF
#include <libexif/exif-data.h>
#endif
#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

G_BEGIN_DECLS

typedef struct _EogExifDetails EogExifDetails;
typedef struct _EogExifDetailsClass EogExifDetailsClass;
typedef struct _EogExifDetailsPrivate EogExifDetailsPrivate;

#define EOG_TYPE_EXIF_DETAILS            (eog_exif_details_get_type ())
#define EOG_EXIF_DETAILS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_EXIF_DETAILS, EogExifDetails))
#define EOG_EXIF_DETAILS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), EOG_TYPE_EXIF_DETAILS, EogExifDetailsClass))
#define EOG_IS_EXIF_DETAILS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_EXIF_DETAILS))
#define EOG_IS_EXIF_DETAILS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EOG_TYPE_EXIF_DETAILS))
#define EOG_EXIF_DETAILS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EOG_TYPE_EXIF_DETAILS, EogExifDetailsClass))

struct _EogExifDetails {
        GtkTreeView parent;

	EogExifDetailsPrivate *priv;
};

struct _EogExifDetailsClass {
	GtkTreeViewClass parent_class;
};

GType               eog_exif_details_get_type    (void) G_GNUC_CONST;

GtkWidget          *eog_exif_details_new         (void);

#if HAVE_EXIF
void                eog_exif_details_update      (EogExifDetails *view,
						  ExifData       *data);
#endif
#if HAVE_EXEMPI
void                eog_exif_details_xmp_update  (EogExifDetails *view,
							XmpPtr          xmp_data);
#endif

G_END_DECLS

#endif /* __EOG_EXIF_DETAILS__ */
