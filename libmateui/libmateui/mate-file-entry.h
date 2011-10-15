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

/* MateFileEntry widget - Combo box with "Browse" button for files
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */


#ifndef MATE_FILE_ENTRY_H
#define MATE_FILE_ENTRY_H


#include <libmateui/mate-entry.h>
#include <gtk/gtk.h>

#ifndef MATE_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif


#define MATE_TYPE_FILE_ENTRY            (mate_file_entry_get_type ())
#define MATE_FILE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_FILE_ENTRY, MateFileEntry))
#define MATE_FILE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_FILE_ENTRY, MateFileEntryClass))
#define MATE_IS_FILE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_FILE_ENTRY))
#define MATE_IS_FILE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_FILE_ENTRY))
#define MATE_FILE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_FILE_ENTRY, MateFileEntryClass))

/* Note:  This supports GtkEditable interface */

typedef struct _MateFileEntry        MateFileEntry;
typedef struct _MateFileEntryPrivate MateFileEntryPrivate;
typedef struct _MateFileEntryClass   MateFileEntryClass;

struct _MateFileEntry {
	GtkVBox hbox;

	/*the file dialog widget*/
	/* For now we depend on this being public, as
	 * some apps bind it and mess with the file selector.
	 * this is evil, but can't be helped unless we provide
	 * alternatives. */
	GtkWidget *fsw;

	/* mate-icon-entry needs to access this, but it's
	 * not for public consumption, think of it as protected */
	/*< private >*/
	char *default_path;
	MateFileEntryPrivate *_priv;
};

struct _MateFileEntryClass {
	GtkVBoxClass parent_class;

	/*if you want to modify the browse dialog, bind this with
	  connect_after and modify object->fsw, or you could just
	  create your own and set it to object->fsw in a normally
	  connected handler, it has to be a gtk_file_selection though*/
	void (* browse_clicked) (MateFileEntry *fentry);

	/* Like in GtkEntry */
	void (* activate) (MateFileEntry *fentry);

	gpointer reserved1, reserved2; /* Reserved for future use,
					  we'll need to proxy insert_text
					  and delete_text signals */
};


GType      mate_file_entry_get_type    (void) G_GNUC_CONST;
GtkWidget *mate_file_entry_new         (const char *history_id,
					 const char *browse_dialog_title);
void       mate_file_entry_construct  (MateFileEntry *fentry,
					const char *history_id,
					const char *browse_dialog_title);

GtkWidget *mate_file_entry_mate_entry (MateFileEntry *fentry);
GtkWidget *mate_file_entry_gtk_entry   (MateFileEntry *fentry);
void       mate_file_entry_set_title   (MateFileEntry *fentry,
					 const char *browse_dialog_title);

/*set default path for the browse dialog*/
void	   mate_file_entry_set_default_path(MateFileEntry *fentry,
					     const char *path);

/*sets up the file entry to be a directory picker rather then a file picker*/
void	   mate_file_entry_set_directory_entry(MateFileEntry *fentry,
						gboolean directory_entry);
gboolean   mate_file_entry_get_directory_entry(MateFileEntry *fentry);

/*returns a filename which is a full path with WD or the default
  directory prepended if it's not an absolute path, returns
  NULL on empty entry or if the file doesn't exist and that was
  a requirement*/
char      *mate_file_entry_get_full_path(MateFileEntry *fentry,
					  gboolean file_must_exist);

/* set the filename to something, this is like setting the internal
 * GtkEntry */
void       mate_file_entry_set_filename(MateFileEntry *fentry,
					 const char *filename);

/*set modality of the file browse dialog, only applies for the
  next time a dialog is created*/
void       mate_file_entry_set_modal	(MateFileEntry *fentry,
					 gboolean is_modal);
gboolean   mate_file_entry_get_modal	(MateFileEntry *fentry);

#ifndef MATE_DISABLE_DEPRECATED
/* DEPRECATED, use mate_file_entry_set_directory_entry */
void	   mate_file_entry_set_directory(MateFileEntry *fentry,
					  gboolean directory_entry);
#endif /* MATE_DISABLE_DEPRECATED */

G_END_DECLS

#endif

#endif /* MATE_DISABLE_DEPRECATED */
