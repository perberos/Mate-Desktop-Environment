/* Eye Of MATE -- Metadata Reader Interface
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.mate.org>
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

#ifndef _EOG_METADATA_READER_H_
#define _EOG_METADATA_READER_H_

#include <glib-object.h>
#if HAVE_EXIF
#include <libexif/exif-data.h>
#endif
#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif
#if HAVE_LCMS
#include <lcms.h>
#endif

G_BEGIN_DECLS

#define EOG_TYPE_METADATA_READER	      (eog_metadata_reader_get_type ())
#define EOG_METADATA_READER(o)		      (G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_METADATA_READER, EogMetadataReader))
#define EOG_IS_METADATA_READER(o)	      (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_METADATA_READER))
#define EOG_METADATA_READER_GET_INTERFACE(o)  (G_TYPE_INSTANCE_GET_INTERFACE ((o), EOG_TYPE_METADATA_READER, EogMetadataReaderInterface))

typedef struct _EogMetadataReader EogMetadataReader;
typedef struct _EogMetadataReaderInterface EogMetadataReaderInterface;

struct _EogMetadataReaderInterface {
	GTypeInterface parent;

	void		(*consume)		(EogMetadataReader *self,
						 const guchar *buf,
						 guint len);

	gboolean	(*finished)		(EogMetadataReader *self);

	void		(*get_raw_exif)		(EogMetadataReader *self,
						 guchar **data,
						 guint *len);

	gpointer	(*get_exif_data)	(EogMetadataReader *self);

	gpointer	(*get_icc_profile)	(EogMetadataReader *self);

	gpointer	(*get_xmp_ptr)		(EogMetadataReader *self);
};

typedef enum {
	EOG_METADATA_JPEG,
	EOG_METADATA_PNG
} EogMetadataFileType;

G_GNUC_INTERNAL
GType                eog_metadata_reader_get_type	(void) G_GNUC_CONST;

G_GNUC_INTERNAL
EogMetadataReader*   eog_metadata_reader_new 		(EogMetadataFileType type);

G_GNUC_INTERNAL
void                 eog_metadata_reader_consume	(EogMetadataReader *emr,
							 const guchar *buf,
							 guint len);

G_GNUC_INTERNAL
gboolean             eog_metadata_reader_finished	(EogMetadataReader *emr);

G_GNUC_INTERNAL
void                 eog_metadata_reader_get_exif_chunk (EogMetadataReader *emr,
							 guchar **data,
							 guint *len);

#ifdef HAVE_EXIF
G_GNUC_INTERNAL
ExifData*            eog_metadata_reader_get_exif_data	(EogMetadataReader *emr);
#endif

#ifdef HAVE_EXEMPI
G_GNUC_INTERNAL
XmpPtr	     	     eog_metadata_reader_get_xmp_data	(EogMetadataReader *emr);
#endif

#if 0
gpointer             eog_metadata_reader_get_iptc_chunk	(EogMetadataReader *emr);
IptcData*            eog_metadata_reader_get_iptc_data	(EogMetadataReader *emr);
#endif

#ifdef HAVE_LCMS
G_GNUC_INTERNAL
cmsHPROFILE          eog_metadata_reader_get_icc_profile (EogMetadataReader *emr);
#endif

G_END_DECLS

#endif /* _EOG_METADATA_READER_H_ */
