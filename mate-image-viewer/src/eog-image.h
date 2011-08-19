/* Eye Of Mate - Image
 *
 * Copyright (C) 2007 The Free Software Foundation
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

#ifndef __EOG_IMAGE_H__
#define __EOG_IMAGE_H__

#include "eog-jobs.h"
#include "eog-window.h"
#include "eog-transform.h"
#include "eog-image-save-info.h"
#include "eog-enums.h"

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#endif

#ifdef HAVE_LCMS
#include <lcms.h>
#endif

#ifdef HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

G_BEGIN_DECLS

#ifndef __EOG_IMAGE_DECLR__
#define __EOG_IMAGE_DECLR__
typedef struct _EogImage EogImage;
#endif
typedef struct _EogImageClass EogImageClass;
typedef struct _EogImagePrivate EogImagePrivate;

#define EOG_TYPE_IMAGE            (eog_image_get_type ())
#define EOG_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_IMAGE, EogImage))
#define EOG_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_IMAGE, EogImageClass))
#define EOG_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_IMAGE))
#define EOG_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOG_TYPE_IMAGE))
#define EOG_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOG_TYPE_IMAGE, EogImageClass))

typedef enum {
	EOG_IMAGE_ERROR_SAVE_NOT_LOCAL,
	EOG_IMAGE_ERROR_NOT_LOADED,
	EOG_IMAGE_ERROR_VFS,
	EOG_IMAGE_ERROR_FILE_EXISTS,
	EOG_IMAGE_ERROR_TMP_FILE_FAILED,
	EOG_IMAGE_ERROR_GENERIC,
	EOG_IMAGE_ERROR_UNKNOWN
} EogImageError;

#define EOG_IMAGE_ERROR eog_image_error_quark ()

typedef enum {
	EOG_IMAGE_STATUS_UNKNOWN,
	EOG_IMAGE_STATUS_LOADING,
	EOG_IMAGE_STATUS_LOADED,
	EOG_IMAGE_STATUS_SAVING,
	EOG_IMAGE_STATUS_FAILED
} EogImageStatus;

typedef enum {
  EOG_IMAGE_METADATA_NOT_READ,
  EOG_IMAGE_METADATA_NOT_AVAILABLE,
  EOG_IMAGE_METADATA_READY
} EogImageMetadataStatus;

struct _EogImage {
	GObject parent;

	EogImagePrivate *priv;
};

struct _EogImageClass {
	GObjectClass parent_class;

	void (* changed) 	   (EogImage *img);

	void (* size_prepared)     (EogImage *img,
				    int       width,
				    int       height);

	void (* thumbnail_changed) (EogImage *img);

	void (* save_progress)     (EogImage *img,
				    gfloat    progress);

	void (* next_frame)        (EogImage *img,
				    gint delay);

	void (* file_changed)      (EogImage *img);
};

GType	          eog_image_get_type	             (void) G_GNUC_CONST;

GQuark            eog_image_error_quark              (void);

EogImage         *eog_image_new                      (const char *txt_uri);

EogImage         *eog_image_new_file                 (GFile *file);

gboolean          eog_image_load                     (EogImage   *img,
					              EogImageData data2read,
					              EogJob     *job,
					              GError    **error);

void              eog_image_cancel_load              (EogImage   *img);

gboolean          eog_image_has_data                 (EogImage   *img,
					              EogImageData data);

void              eog_image_data_ref                 (EogImage   *img);

void              eog_image_data_unref               (EogImage   *img);

void              eog_image_set_thumbnail            (EogImage   *img,
					              GdkPixbuf  *pixbuf);

gboolean          eog_image_save_as_by_info          (EogImage   *img,
		      			              EogImageSaveInfo *source,
		      			              EogImageSaveInfo *target,
		      			              GError    **error);

gboolean          eog_image_save_by_info             (EogImage   *img,
					              EogImageSaveInfo *source,
					              GError    **error);

GdkPixbuf*        eog_image_get_pixbuf               (EogImage   *img);

GdkPixbuf*        eog_image_get_thumbnail            (EogImage   *img);

void              eog_image_get_size                 (EogImage   *img,
					              gint       *width,
					              gint       *height);

goffset           eog_image_get_bytes                (EogImage   *img);

gboolean          eog_image_is_modified              (EogImage   *img);

void              eog_image_modified                 (EogImage   *img);

const gchar*      eog_image_get_caption              (EogImage   *img);

const gchar      *eog_image_get_collate_key          (EogImage   *img);

gpointer          eog_image_get_exif_info            (EogImage   *img);

gpointer          eog_image_get_xmp_info             (EogImage   *img);

GFile*            eog_image_get_file                 (EogImage   *img);

gchar*            eog_image_get_uri_for_display      (EogImage   *img);

EogImageStatus    eog_image_get_status               (EogImage   *img);

EogImageMetadataStatus eog_image_get_metadata_status (EogImage   *img);

void              eog_image_transform                (EogImage   *img,
						      EogTransform *trans,
						      EogJob     *job);

#ifdef HAVE_EXIF
void              eog_image_autorotate               (EogImage   *img);
#endif

#ifdef HAVE_LCMS
cmsHPROFILE       eog_image_get_profile              (EogImage    *img);

void              eog_image_apply_display_profile    (EogImage    *img,
						      cmsHPROFILE  display_profile);
#endif

void              eog_image_undo                     (EogImage   *img);

GList		 *eog_image_get_supported_mime_types (void);

gboolean          eog_image_is_supported_mime_type   (const char *mime_type);

gboolean          eog_image_is_animation             (EogImage *img);

gboolean          eog_image_start_animation          (EogImage *img);

#ifdef HAVE_RSVG
gboolean          eog_image_is_svg                   (EogImage *img);
RsvgHandle       *eog_image_get_svg                  (EogImage *img);
EogTransform     *eog_image_get_transform            (EogImage *img);
#endif

void              eog_image_file_changed             (EogImage *img);

G_END_DECLS

#endif /* __EOG_IMAGE_H__ */
