/*
 * Copyright (C) 2002 Alexander Larsson <alexl@redhat.com>.
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

#ifndef MATE_ICON_LOOKUP_H
#define MATE_ICON_LOOKUP_H

#include <gtk/gtk.h>
#include <libmatevfs/mate-vfs-file-info.h>
#include <libmateui/mate-thumbnail.h>
/* We used to include this, so keep doing it for backwards compat */
#include <libmateui/mate-icon-theme.h>

G_BEGIN_DECLS

typedef enum {
  MATE_ICON_LOOKUP_FLAGS_NONE = 0,
  MATE_ICON_LOOKUP_FLAGS_EMBEDDING_TEXT = 1<<0,
  MATE_ICON_LOOKUP_FLAGS_SHOW_SMALL_IMAGES_AS_THEMSELVES = 1<<1,
  MATE_ICON_LOOKUP_FLAGS_ALLOW_SVG_AS_THEMSELVES = 1<<2
} MateIconLookupFlags;

typedef enum {
  MATE_ICON_LOOKUP_RESULT_FLAGS_NONE = 0,
  MATE_ICON_LOOKUP_RESULT_FLAGS_THUMBNAIL = 1<<0
} MateIconLookupResultFlags;


/* Also takes MateIconTheme for backwards compat */
char *mate_icon_lookup      (GtkIconTheme               *icon_theme,
			      MateThumbnailFactory      *thumbnail_factory,
			      const char                 *file_uri,
			      const char                 *custom_icon,
			      MateVFSFileInfo           *file_info,
			      const char                 *mime_type,
			      MateIconLookupFlags        flags,
			      MateIconLookupResultFlags *result);
char *mate_icon_lookup_sync (GtkIconTheme               *icon_theme,
			      MateThumbnailFactory      *thumbnail_factory,
			      const char                 *file_uri,
			      const char                 *custom_icon,
			      MateIconLookupFlags        flags,
			      MateIconLookupResultFlags *result);


G_END_DECLS

#endif /* MATE_ICON_LOOKUP_H */
