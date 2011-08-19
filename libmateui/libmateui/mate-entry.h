/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

/* MateEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef MATE_ENTRY_H
#define MATE_ENTRY_H

#ifndef MATE_DISABLE_DEPRECATED

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MATE_TYPE_ENTRY            (mate_entry_get_type ())
#define MATE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_ENTRY, MateEntry))
#define MATE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_ENTRY, MateEntryClass))
#define MATE_IS_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_ENTRY))
#define MATE_IS_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_ENTRY))
#define MATE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_ENTRY, MateEntryClass))

/* This also supports the GtkEditable interface so
 * to get text use the gtk_editable_get_chars method
 * on this object */

typedef struct _MateEntry        MateEntry;
typedef struct _MateEntryPrivate MateEntryPrivate;
typedef struct _MateEntryClass   MateEntryClass;

struct _MateEntry {
	GtkCombo combo;

	/*< private >*/
	MateEntryPrivate *_priv;
};

struct _MateEntryClass {
	GtkComboClass parent_class;

	/* Like the GtkEntry signals */
	void (* activate) (MateEntry *entry);

	gpointer reserved1, reserved2; /* Reserved for future use,
					  we'll need to proxy insert_text
					  and delete_text signals */
};


GType        mate_entry_get_type         (void) G_GNUC_CONST;
GtkWidget   *mate_entry_new              (const gchar *history_id);

/* for language bindings and subclassing, use mate_entry_new */

GtkWidget   *mate_entry_gtk_entry        (MateEntry  *gentry);

const gchar *mate_entry_get_history_id   (MateEntry  *gentry);

void         mate_entry_set_history_id   (MateEntry  *gentry,
					   const gchar *history_id);

void         mate_entry_set_max_saved    (MateEntry  *gentry,
					   guint        max_saved);
guint        mate_entry_get_max_saved    (MateEntry  *gentry);

void         mate_entry_prepend_history  (MateEntry  *gentry,
					   gboolean    save,
					   const gchar *text);
void         mate_entry_append_history   (MateEntry  *gentry,
					   gboolean     save,
					   const gchar *text);
void         mate_entry_clear_history    (MateEntry  *gentry);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* MATE_ENTRY_H */

