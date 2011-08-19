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

/* MateIconEntry widget - Combo box with "Browse" button for files and
 *			   A pick button which can display a list of icons
 *			   in a current directory, the browse button displays
 *			   same dialog as pixmap-entry
 *
 * Author: George Lebl <jirka@5z.com>
 * icon selection based on original dentry-edit code which was:
 *	Written by: Havoc Pennington, based on code by John Ellis.
 */

#ifndef MATE_ICON_ENTRY_H
#define MATE_ICON_ENTRY_H


#include <glib.h>
#include <gtk/gtk.h>
#include <libmateui/mate-file-entry.h>


G_BEGIN_DECLS


#define MATE_TYPE_ICON_ENTRY            (mate_icon_entry_get_type ())
#define MATE_ICON_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_ICON_ENTRY, MateIconEntry))
#define MATE_ICON_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_ICON_ENTRY, MateIconEntryClass))
#define MATE_IS_ICON_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_ICON_ENTRY))
#define MATE_IS_ICON_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_ICON_ENTRY))
#define MATE_ICON_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_ICON_ENTRY, MateIconEntryClass))


typedef struct _MateIconEntry         MateIconEntry;
typedef struct _MateIconEntryPrivate  MateIconEntryPrivate;
typedef struct _MateIconEntryClass    MateIconEntryClass;

struct _MateIconEntry {
	GtkVBox vbox;
	
	/*< private >*/
	MateIconEntryPrivate *_priv;
};

struct _MateIconEntryClass {
	GtkVBoxClass parent_class;

	void (*changed) (MateIconEntry *ientry);
	void (*browse) (MateIconEntry *ientry);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};


GType      mate_icon_entry_get_type    (void) G_GNUC_CONST;
GtkWidget *mate_icon_entry_new         (const gchar *history_id,
					 const gchar *browse_dialog_title);

/* for language bindings and subclassing, use mate_icon_entry_new from C */
void       mate_icon_entry_construct   (MateIconEntry *ientry,
					 const gchar *history_id,
					 const gchar *browse_dialog_title);

/*by default mate_pixmap entry sets the default directory to the
  mate pixmap directory, this will set it to a subdirectory of that,
  or one would use the file_entry functions for any other path*/
void       mate_icon_entry_set_pixmap_subdir(MateIconEntry *ientry,
					      const gchar *subdir);

/*only return a file if it was possible to load it with gdk-pixbuf*/
gchar      *mate_icon_entry_get_filename(MateIconEntry *ientry);

/* set the icon to something, returns TRUE on success */
gboolean   mate_icon_entry_set_filename(MateIconEntry *ientry,
					 const gchar *filename);

void       mate_icon_entry_set_browse_dialog_title(MateIconEntry *ientry,
						    const gchar *browse_dialog_title);
void       mate_icon_entry_set_history_id(MateIconEntry *ientry,
					   const gchar *history_id);
void       mate_icon_entry_set_max_saved (MateIconEntry *ientry,
					   guint max_saved);

GtkWidget *mate_icon_entry_pick_dialog	(MateIconEntry *ientry);

#ifndef MATE_EXCLUDE_DEPRECATED
/* DEPRECATED routines left for compatibility only, will disapear in
 * some very distant future */
/* this is deprecated in favour of the above */
void       mate_icon_entry_set_icon(MateIconEntry *ientry,
				     const gchar *filename);
GtkWidget *mate_icon_entry_mate_file_entry(MateIconEntry *ientry);
GtkWidget *mate_icon_entry_mate_entry (MateIconEntry *ientry);
GtkWidget *mate_icon_entry_gtk_entry   (MateIconEntry *ientry);
#endif


G_END_DECLS

#endif
