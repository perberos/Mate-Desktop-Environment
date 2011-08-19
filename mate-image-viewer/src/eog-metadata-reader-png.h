/* Eye Of MATE -- PNG Metadata Reader
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.mate.org>
 *
 * Based on the old EogMetadataReader code.
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

#ifndef _EOG_METADATA_READER_PNG_H_
#define _EOG_METADATA_READER_PNG_H_

G_BEGIN_DECLS

#define EOG_TYPE_METADATA_READER_PNG		(eog_metadata_reader_png_get_type ())
#define EOG_METADATA_READER_PNG(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_METADATA_READER_PNG, EogMetadataReaderPng))
#define EOG_METADATA_READER_PNG_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), EOG_TYPE_METADATA_READER_PNG, EogMetadataReaderPngClass))
#define EOG_IS_METADATA_READER_PNG(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_METADATA_READER_PNG))
#define EOG_IS_METADATA_READER_PNG_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), EOG_TYPE_METADATA_READER_PNG))
#define EOG_METADATA_READER_PNG_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_METADATA_READER_PNG, EogMetadataReaderPngClass))

typedef struct _EogMetadataReaderPng EogMetadataReaderPng;
typedef struct _EogMetadataReaderPngClass EogMetadataReaderPngClass;
typedef struct _EogMetadataReaderPngPrivate EogMetadataReaderPngPrivate;

struct _EogMetadataReaderPng {
	GObject parent;

	EogMetadataReaderPngPrivate *priv;
};

struct _EogMetadataReaderPngClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType		      eog_metadata_reader_png_get_type	(void) G_GNUC_CONST;

G_END_DECLS

#endif /* _EOG_METADATA_READER_PNG_H_ */
