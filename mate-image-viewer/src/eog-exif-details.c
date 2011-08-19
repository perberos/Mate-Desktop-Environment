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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eog-exif-details.h"
#include "eog-util.h"

#if HAVE_EXIF
#include <libexif/exif-entry.h>
#include <libexif/exif-utils.h>
#endif
#if HAVE_EXEMPI
#include <exempi/xmp.h>
#include <exempi/xmpconsts.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <string.h>

#define EOG_EXIF_DETAILS_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_EXIF_DETAILS, EogExifDetailsPrivate))

G_DEFINE_TYPE (EogExifDetails, eog_exif_details, GTK_TYPE_TREE_VIEW)

typedef enum {
	EXIF_CATEGORY_CAMERA,
	EXIF_CATEGORY_IMAGE_DATA,
	EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS,
	EXIF_CATEGORY_MAKER_NOTE,
	EXIF_CATEGORY_OTHER,
#ifdef HAVE_EXEMPI
	XMP_CATEGORY_EXIF,
	XMP_CATEGORY_IPTC,
	XMP_CATEGORY_RIGHTS,
	XMP_CATEGORY_OTHER
#endif
} ExifCategory;

typedef struct {
	char *label;
	char *path;
} ExifCategoryInfo;

static ExifCategoryInfo exif_categories[] = {
	{ N_("Camera"),                  "0" },
	{ N_("Image Data"),              "1" },
	{ N_("Image Taking Conditions"), "2" },
	{ N_("Maker Note"),              "3" },
	{ N_("Other"),                   "4" },
#ifdef HAVE_EXEMPI
	{ N_("XMP Exif"),                "5" },
	{ N_("XMP IPTC"),                "6" },
	{ N_("XMP Rights Management"),   "7" },
	{ N_("XMP Other"),               "8" },
#endif
	{ NULL, NULL }
};

typedef struct {
	int id;
	ExifCategory category;
} ExifTagCategory;

#ifdef HAVE_EXIF
static ExifTagCategory exif_tag_category_map[] = {
	{ EXIF_TAG_INTEROPERABILITY_INDEX,    EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_INTEROPERABILITY_VERSION,  EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_IMAGE_WIDTH,               EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_IMAGE_LENGTH        ,      EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_BITS_PER_SAMPLE 	     ,EXIF_CATEGORY_CAMERA },
	{ EXIF_TAG_COMPRESSION 		    , EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_PHOTOMETRIC_INTERPRETATION 	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_FILL_ORDER 		, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_DOCUMENT_NAME 	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_IMAGE_DESCRIPTION 	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_MAKE 		, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_MODEL 		, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_STRIP_OFFSETS 	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_ORIENTATION 		, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_SAMPLES_PER_PIXEL 	, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_ROWS_PER_STRIP 	, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_STRIP_BYTE_COUNTS	, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_X_RESOLUTION 	, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_Y_RESOLUTION 	, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_PLANAR_CONFIGURATION , EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_RESOLUTION_UNIT 	, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_TRANSFER_FUNCTION 	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_SOFTWARE 		, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_DATE_TIME		, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_ARTIST		, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_WHITE_POINT		, 	EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_PRIMARY_CHROMATICITIES, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_TRANSFER_RANGE	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_JPEG_PROC		, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_JPEG_INTERCHANGE_FORMAT, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH, },
	{ EXIF_TAG_YCBCR_COEFFICIENTS	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_YCBCR_SUB_SAMPLING	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_YCBCR_POSITIONING	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_REFERENCE_BLACK_WHITE, 	EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_RELATED_IMAGE_FILE_FORMAT,EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_RELATED_IMAGE_WIDTH	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_RELATED_IMAGE_LENGTH	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_CFA_REPEAT_PATTERN_DIM, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_CFA_PATTERN		, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_BATTERY_LEVEL	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_COPYRIGHT		, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_EXPOSURE_TIME	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_FNUMBER		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_IPTC_NAA		, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_EXIF_IFD_POINTER	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_INTER_COLOR_PROFILE	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_EXPOSURE_PROGRAM	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_SPECTRAL_SENSITIVITY	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_GPS_INFO_IFD_POINTER	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_ISO_SPEED_RATINGS	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_OECF			, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_EXIF_VERSION		, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_DATE_TIME_ORIGINAL	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_DATE_TIME_DIGITIZED	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_COMPONENTS_CONFIGURATION	, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_COMPRESSED_BITS_PER_PIXEL, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_SHUTTER_SPEED_VALUE	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_APERTURE_VALUE	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_BRIGHTNESS_VALUE	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_EXPOSURE_BIAS_VALUE	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_MAX_APERTURE_VALUE	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_SUBJECT_DISTANCE	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_METERING_MODE	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_LIGHT_SOURCE		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_FLASH		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_FOCAL_LENGTH		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_SUBJECT_AREA		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_MAKER_NOTE		, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_USER_COMMENT		, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_SUBSEC_TIME		, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_SUB_SEC_TIME_ORIGINAL, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_SUB_SEC_TIME_DIGITIZED, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_FLASH_PIX_VERSION	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_COLOR_SPACE		, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_PIXEL_X_DIMENSION	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_PIXEL_Y_DIMENSION	, EXIF_CATEGORY_IMAGE_DATA},
	{ EXIF_TAG_RELATED_SOUND_FILE	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_INTEROPERABILITY_IFD_POINTER, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_FLASH_ENERGY		,EXIF_CATEGORY_OTHER },
	{ EXIF_TAG_SPATIAL_FREQUENCY_RESPONSE, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_FOCAL_PLANE_X_RESOLUTION, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_SUBJECT_LOCATION, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_EXPOSURE_INDEX, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_SENSING_METHOD, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_FILE_SOURCE	, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_SCENE_TYPE, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_NEW_CFA_PATTERN, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_CUSTOM_RENDERED, EXIF_CATEGORY_OTHER},
	{ EXIF_TAG_EXPOSURE_MODE, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_WHITE_BALANCE, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_DIGITAL_ZOOM_RATIO, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_SCENE_CAPTURE_TYPE	, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_GAIN_CONTROL		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_CONTRAST		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_SATURATION		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_SHARPNESS		, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_DEVICE_SETTING_DESCRIPTION, EXIF_CATEGORY_CAMERA},
	{ EXIF_TAG_SUBJECT_DISTANCE_RANGE, EXIF_CATEGORY_IMAGE_TAKING_CONDITIONS},
	{ EXIF_TAG_IMAGE_UNIQUE_ID	, EXIF_CATEGORY_IMAGE_DATA},
	{ -1, -1 }
};
#endif

#define MODEL_COLUMN_ATTRIBUTE 0
#define MODEL_COLUMN_VALUE     1

struct _EogExifDetailsPrivate {
	GtkTreeModel *model;

	GHashTable   *id_path_hash;
	GHashTable   *id_path_hash_mnote;
};

static char*  set_row_data (GtkTreeStore *store, char *path, char *parent, const char *attribute, const char *value);

static void eog_exif_details_reset (EogExifDetails *exif_details);

static void
eog_exif_details_dispose (GObject *object)
{
	EogExifDetailsPrivate *priv;

	priv = EOG_EXIF_DETAILS (object)->priv;

	if (priv->model) {
		g_object_unref (priv->model);
		priv->model = NULL;
	}

	if (priv->id_path_hash) {
		g_hash_table_destroy (priv->id_path_hash);
		priv->id_path_hash = NULL;
	}

	if (priv->id_path_hash_mnote) {
		g_hash_table_destroy (priv->id_path_hash_mnote);
		priv->id_path_hash_mnote = NULL;
	}
	G_OBJECT_CLASS (eog_exif_details_parent_class)->dispose (object);
}

static void
eog_exif_details_init (EogExifDetails *exif_details)
{
	EogExifDetailsPrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;

	exif_details->priv = EOG_EXIF_DETAILS_GET_PRIVATE (exif_details);

	priv = exif_details->priv;

	priv->model = GTK_TREE_MODEL (gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_STRING));
	priv->id_path_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
	priv->id_path_hash_mnote = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

	/* Tag name column */
	cell = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Tag"), cell,
                                                           "text", MODEL_COLUMN_ATTRIBUTE,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (exif_details), column);

	/* Value column */
	cell = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Value"), cell,
                                                           "text", MODEL_COLUMN_VALUE,
							    NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (exif_details), column);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (exif_details), TRUE);

	eog_exif_details_reset (exif_details);

	gtk_tree_view_set_model (GTK_TREE_VIEW (exif_details),
				 GTK_TREE_MODEL (priv->model));
}

static void
eog_exif_details_class_init (EogExifDetailsClass *klass)
{
	GObjectClass *object_class = (GObjectClass*) klass;

	object_class->dispose = eog_exif_details_dispose;

	g_type_class_add_private (object_class, sizeof (EogExifDetailsPrivate));
}

#ifdef HAVE_EXIF
static ExifCategory
get_exif_category (ExifEntry *entry)
{
	ExifCategory cat = EXIF_CATEGORY_OTHER;
	int i;

	for (i = 0; exif_tag_category_map [i].id != -1; i++) {
		if (exif_tag_category_map[i].id == (int) entry->tag) {
			cat = exif_tag_category_map[i].category;
			break;
		}
	}

	return cat;
}
#endif

static char*
set_row_data (GtkTreeStore *store, char *path, char *parent, const char *attribute, const char *value)
{
	GtkTreeIter iter;
	gchar *utf_attribute = NULL;
	gchar *utf_value = NULL;
	gboolean iter_valid = FALSE;

	if (!attribute) return NULL;

	if (path != NULL) {
		iter_valid = gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (store), &iter, path);
	}

	if (!iter_valid) {
		GtkTreePath *tree_path;
		GtkTreeIter parent_iter;
		gboolean parent_valid = FALSE;

		if (parent != NULL) {
			parent_valid = gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (store),
									    &parent_iter,
									    parent);
		}

		gtk_tree_store_append (store, &iter, parent_valid ? &parent_iter : NULL);

		if (path == NULL) {
			tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);

			if (tree_path != NULL) {
				path = gtk_tree_path_to_string (tree_path);
				gtk_tree_path_free (tree_path);
			}
		}
	}

	utf_attribute = eog_util_make_valid_utf8 (attribute);

	gtk_tree_store_set (store, &iter, MODEL_COLUMN_ATTRIBUTE, utf_attribute, -1);
	g_free (utf_attribute);

	if (value != NULL) {
		utf_value = eog_util_make_valid_utf8 (value);
		gtk_tree_store_set (store, &iter, MODEL_COLUMN_VALUE, utf_value, -1);
		g_free (utf_value);
	}

	return path;
}

#ifdef HAVE_EXIF
static void
exif_entry_cb (ExifEntry *entry, gpointer data)
{
	GtkTreeStore *store;
	EogExifDetails *view;
	EogExifDetailsPrivate *priv;
	ExifCategory cat;
	char *path;
	char b[1024];

	view = EOG_EXIF_DETAILS (data);
	priv = view->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

	path = g_hash_table_lookup (priv->id_path_hash, GINT_TO_POINTER (entry->tag));

	if (path != NULL) {
		set_row_data (store,
			      path,
			      NULL,
			      exif_tag_get_name (entry->tag),
			      exif_entry_get_value (entry, b, sizeof(b)));
	} else {

		ExifMnoteData *mnote = (entry->tag == EXIF_TAG_MAKER_NOTE ?
			exif_data_get_mnote_data (entry->parent->parent) : NULL);

		if (mnote) {
			// Supported MakerNote Found
			unsigned int i, c = exif_mnote_data_count (mnote);

			for (i = 0; i < c; i++) {
				path = g_hash_table_lookup (priv->id_path_hash_mnote, GINT_TO_POINTER (i));
				if (path != NULL) {
					set_row_data (store, path, NULL,
						exif_mnote_data_get_title (mnote, i),
						exif_mnote_data_get_value (mnote, i, b, sizeof(b)));
				} else {
					path = set_row_data (store,
							     NULL,
							     exif_categories[EXIF_CATEGORY_MAKER_NOTE].path,
							     exif_mnote_data_get_title (mnote, i),
							     exif_mnote_data_get_value (mnote, i, b, sizeof(b)));
					g_hash_table_insert (priv->id_path_hash_mnote, GINT_TO_POINTER (i), path);
				}
			}
		} else {
			cat = get_exif_category (entry);

			path = set_row_data (store,
					     NULL,
					     exif_categories[cat].path,
					     exif_tag_get_name (entry->tag),
					     exif_entry_get_value (entry, b,
								   sizeof(b)));

			g_hash_table_insert (priv->id_path_hash,
					     GINT_TO_POINTER (entry->tag),
					     path);
		}
	}
}
#endif

#ifdef HAVE_EXIF
static void
exif_content_cb (ExifContent *content, gpointer data)
{
	exif_content_foreach_entry (content, exif_entry_cb, data);
}
#endif

GtkWidget *
eog_exif_details_new (void)
{
	GObject *object;

	object = g_object_new (EOG_TYPE_EXIF_DETAILS, NULL);

	return GTK_WIDGET (object);
}

static void
eog_exif_details_reset (EogExifDetails *exif_details)
{
	int i;
	EogExifDetailsPrivate *priv = exif_details->priv;

	gtk_tree_store_clear (GTK_TREE_STORE (priv->model));

	g_hash_table_remove_all (priv->id_path_hash);
	g_hash_table_remove_all (priv->id_path_hash_mnote);

	for (i = 0; exif_categories [i].label != NULL; i++) {
		char *translated_string;

		translated_string = gettext (exif_categories[i].label);

		set_row_data (GTK_TREE_STORE (priv->model),
			      exif_categories[i].path,
			      NULL,
			      translated_string,
			      NULL);
	}
}

#ifdef HAVE_EXIF
void
eog_exif_details_update (EogExifDetails *exif_details, ExifData *data)
{
	EogExifDetailsPrivate *priv;

	g_return_if_fail (EOG_IS_EXIF_DETAILS (exif_details));

	priv = exif_details->priv;

	eog_exif_details_reset (exif_details);
	if (data) {
		exif_data_foreach_content (data, exif_content_cb, exif_details);
	}
}
#endif /* HAVE_EXIF */

#ifdef HAVE_EXEMPI
typedef struct {
	const char *id;
	ExifCategory category;
} XmpNsCategory;

static XmpNsCategory xmp_ns_category_map[] = {
	{ NS_EXIF,                  XMP_CATEGORY_EXIF},
	{ NS_TIFF,                  XMP_CATEGORY_EXIF},
	{ NS_XAP,                   XMP_CATEGORY_EXIF},
	{ NS_XAP_RIGHTS,            XMP_CATEGORY_RIGHTS},
	{ NS_EXIF_AUX,              XMP_CATEGORY_EXIF},
	{ NS_DC,                    XMP_CATEGORY_IPTC},
	{ NS_IPTC4XMP,              XMP_CATEGORY_IPTC},
	{ NS_CC,                    XMP_CATEGORY_RIGHTS},
	{ NULL, -1}
};

static ExifCategory
get_xmp_category (XmpStringPtr schema)
{
	ExifCategory cat = XMP_CATEGORY_OTHER;
	const char *s = xmp_string_cstr(schema);
	int i;

	for (i = 0; xmp_ns_category_map[i].id != NULL; i++) {
		if (strcmp (xmp_ns_category_map[i].id, s) == 0) {
			cat = xmp_ns_category_map[i].category;
			break;
		}
	}

	return cat;
}

static void
xmp_entry_insert (EogExifDetails *view, XmpStringPtr xmp_schema,
		  XmpStringPtr xmp_path, XmpStringPtr xmp_prop)
{
	GtkTreeStore *store;
	EogExifDetailsPrivate *priv;
	ExifCategory cat;
	char *path;
	gchar *key;

	priv = view->priv;

	key = g_strconcat (xmp_string_cstr (xmp_schema), ":",
			   xmp_string_cstr (xmp_path), NULL);

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

	path = g_hash_table_lookup (priv->id_path_hash, key);

	if (path != NULL) {
		set_row_data (store, path, NULL,
			      xmp_string_cstr (xmp_path),
			      xmp_string_cstr (xmp_prop));

		g_free(key);
	}
	else {
		cat = get_xmp_category (xmp_schema);

		path = set_row_data (store, NULL, exif_categories[cat].path,
				     xmp_string_cstr(xmp_path),
				     xmp_string_cstr(xmp_prop));

		g_hash_table_insert (priv->id_path_hash, key, path);
	}
}

void
eog_exif_details_xmp_update (EogExifDetails *view, XmpPtr data)
{
	EogExifDetailsPrivate *priv;

	g_return_if_fail (EOG_IS_EXIF_DETAILS (view));

	priv = view->priv;

	if (data) {
		XmpIteratorPtr iter = xmp_iterator_new(data, NULL, NULL, XMP_ITER_JUSTLEAFNODES);
		XmpStringPtr the_schema = xmp_string_new ();
		XmpStringPtr the_path = xmp_string_new ();
		XmpStringPtr the_prop = xmp_string_new ();

		while (xmp_iterator_next (iter, the_schema, the_path, the_prop, NULL)) {
			xmp_entry_insert (view, the_schema, the_path, the_prop);
		}

		xmp_string_free (the_prop);
		xmp_string_free (the_path);
		xmp_string_free (the_schema);
		xmp_iterator_free (iter);
	}
}
#endif
