/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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

/* MatePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style (well kind of)
 *
 *
 * Author: George Lebl <jirka@5z.com>
 */

#ifndef MATE_PIXMAP_ENTRY_H
#define MATE_PIXMAP_ENTRY_H

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <libmateui/mate-file-entry.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_PIXMAP_ENTRY            (mate_pixmap_entry_get_type ())
#define MATE_PIXMAP_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_PIXMAP_ENTRY, MatePixmapEntry))
#define MATE_PIXMAP_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_PIXMAP_ENTRY, MatePixmapEntryClass))
#define MATE_IS_PIXMAP_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_PIXMAP_ENTRY))
#define MATE_IS_PIXMAP_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_PIXMAP_ENTRY))
#define MATE_PIXMAP_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_PIXMAP_ENTRY, MatePixmapEntryClass))

/* Note:  This derives from MateFileEntry and thus supports GtkEditable
 * interface */

typedef struct _MatePixmapEntry        MatePixmapEntry;
typedef struct _MatePixmapEntryPrivate MatePixmapEntryPrivate;
typedef struct _MatePixmapEntryClass   MatePixmapEntryClass;

struct _MatePixmapEntry {
	MateFileEntry fentry;

	/*< private >*/
	MatePixmapEntryPrivate *_priv;
};

struct _MatePixmapEntryClass {
	MateFileEntryClass parent_class;

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};


GType      mate_pixmap_entry_get_type    (void) G_GNUC_CONST;
GtkWidget *mate_pixmap_entry_new         (const gchar *history_id,
					   const gchar *browse_dialog_title,
					   gboolean do_preview);
void       mate_pixmap_entry_construct   (MatePixmapEntry *pentry,
					   const gchar *history_id,
					   const gchar *browse_dialog_title,
					   gboolean do_preview);

/*by default mate_pixmap entry sets the default directory to the
  mate pixmap directory, this will set it to a subdirectory of that,
  or one would use the file_entry functions for any other path*/
void       mate_pixmap_entry_set_pixmap_subdir(MatePixmapEntry *pentry,
						const gchar *subdir);

/* entry widgets */

/* This is now derived from MateFileEntry, so use it's methods */
#ifndef MATE_DISABLE_DEPRECATED
GtkWidget *mate_pixmap_entry_mate_file_entry(MatePixmapEntry *pentry);
GtkWidget *mate_pixmap_entry_mate_entry (MatePixmapEntry *pentry);
GtkWidget *mate_pixmap_entry_gtk_entry   (MatePixmapEntry *pentry);
#endif

/* preview widgets */
GtkWidget  *mate_pixmap_entry_scrolled_window(MatePixmapEntry *pentry);
GtkWidget  *mate_pixmap_entry_preview_widget(MatePixmapEntry *pentry);


/*set the preview parameters, if preview is off then the preview frame
  will be hidden*/
void       mate_pixmap_entry_set_preview (MatePixmapEntry *pentry,
					   gboolean do_preview);
void	   mate_pixmap_entry_set_preview_size(MatePixmapEntry *pentry,
					       gint preview_w,
					       gint preview_h);

/*only return a file if it was possible to load it with gdk-pixbuf*/
gchar      *mate_pixmap_entry_get_filename(MatePixmapEntry *pentry);

#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif

