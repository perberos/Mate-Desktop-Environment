#ifndef _EOG_IMAGE_SAVE_INFO_H_
#define _EOG_IMAGE_SAVE_INFO_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

struct EogImage;

#define EOG_TYPE_IMAGE_SAVE_INFO            (eog_image_save_info_get_type ())
#define EOG_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_IMAGE_SAVE_INFO, EogImageSaveInfo))
#define EOG_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_CAST((k), EOG_TYPE_IMAGE_SAVE_INFO, EogImageSaveInfoClass))
#define EOG_IS_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_IMAGE_SAVE_INFO))
#define EOG_IS_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOG_TYPE_IMAGE_SAVE_INFO))
#define EOG_IMAGE_SAVE_INFO_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_IMAGE_SAVE_INFO, EogImageSaveInfoClass))

typedef struct _EogImageSaveInfo EogImageSaveInfo;
typedef struct _EogImageSaveInfoClass EogImageSaveInfoClass;

struct _EogImageSaveInfo {
	GObject parent;

	GFile       *file;
	char        *format;
	gboolean     exists;
	gboolean     local;
	gboolean     has_metadata;
	gboolean     modified;
	gboolean     overwrite;

	float        jpeg_quality; /* valid range: [0.0 ... 1.0] */
};

struct _EogImageSaveInfoClass {
	GObjectClass parent_klass;
};

#define EOG_FILE_FORMAT_JPEG   "jpeg"

GType             eog_image_save_info_get_type     (void) G_GNUC_CONST;

EogImageSaveInfo *eog_image_save_info_from_image   (gpointer        data);

EogImageSaveInfo *eog_image_save_info_from_uri     (const char      *uri,
						    GdkPixbufFormat *format);

EogImageSaveInfo *eog_image_save_info_from_file    (GFile           *file,
						    GdkPixbufFormat *format);

G_END_DECLS

#endif /* _EOG_IMAGE_SAVE_INFO_H_ */
