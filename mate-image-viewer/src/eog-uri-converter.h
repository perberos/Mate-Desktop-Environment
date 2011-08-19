#ifndef _EOG_URI_CONVERTER_H_
#define _EOG_URI_CONVERTER_H_

#include <glib-object.h>
#include <glib/gi18n.h>
#include "eog-image.h"

G_BEGIN_DECLS

#define EOG_TYPE_URI_CONVERTER          (eog_uri_converter_get_type ())
#define EOG_URI_CONVERTER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_URI_CONVERTER, EogURIConverter))
#define EOG_URI_CONVERTER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOG_TYPE_URI_CONVERTER, EogURIConverterClass))
#define EOG_IS_URI_CONVERTER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_URI_CONVERTER))
#define EOG_IS_URI_CONVERTER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOG_TYPE_URI_CONVERTER))
#define EOG_URI_CONVERTER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_URI_CONVERTER, EogURIConverterClass))

#ifndef __EOG_URI_CONVERTER_DECLR__
#define __EOG_URI_CONVERTER_DECLR__
typedef struct _EogURIConverter EogURIConverter;
#endif
typedef struct _EogURIConverterClass EogURIConverterClass;
typedef struct _EogURIConverterPrivate EogURIConverterPrivate;

typedef enum {
	EOG_UC_STRING,
	EOG_UC_FILENAME,
	EOG_UC_COUNTER,
	EOG_UC_COMMENT,
	EOG_UC_DATE,
	EOG_UC_TIME,
	EOG_UC_DAY,
	EOG_UC_MONTH,
	EOG_UC_YEAR,
	EOG_UC_HOUR,
	EOG_UC_MINUTE,
	EOG_UC_SECOND,
	EOG_UC_END
} EogUCType;

typedef struct {
	char     *description;
	char     *rep;
	gboolean req_exif;
} EogUCInfo;

typedef enum {
	EOG_UC_ERROR_INVALID_UNICODE,
	EOG_UC_ERROR_INVALID_CHARACTER,
	EOG_UC_ERROR_EQUAL_FILENAMES,
	EOG_UC_ERROR_UNKNOWN
} EogUCError;

#define EOG_UC_ERROR eog_uc_error_quark ()


struct _EogURIConverter {
	GObject parent;

	EogURIConverterPrivate *priv;
};

struct _EogURIConverterClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType              eog_uri_converter_get_type      (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GQuark             eog_uc_error_quark              (void);

G_GNUC_INTERNAL
EogURIConverter*   eog_uri_converter_new           (GFile *base_file,
                                                    GdkPixbufFormat *img_format,
						    const char *format_string);

G_GNUC_INTERNAL
gboolean           eog_uri_converter_check         (EogURIConverter *converter,
                                                    GList *img_list,
                                                    GError **error);

G_GNUC_INTERNAL
gboolean           eog_uri_converter_requires_exif (EogURIConverter *converter);

G_GNUC_INTERNAL
gboolean           eog_uri_converter_do            (EogURIConverter *converter,
                                                    EogImage *image,
                                                    GFile **file,
                                                    GdkPixbufFormat **format,
                                                    GError **error);

G_GNUC_INTERNAL
char*              eog_uri_converter_preview       (const char *format_str,
                                                    EogImage *img,
                                                    GdkPixbufFormat *format,
						    gulong counter,
						    guint n_images,
						    gboolean convert_spaces,
						    gunichar space_char);

/* for debugging purpose only */
G_GNUC_INTERNAL
void                eog_uri_converter_print_list (EogURIConverter *conv);

G_END_DECLS

#endif /* _EOG_URI_CONVERTER_H_ */
