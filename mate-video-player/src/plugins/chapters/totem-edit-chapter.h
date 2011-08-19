/*
 * Copyright (C) 2010 Alexander Saprykin <xelfium@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 */

#ifndef TOTEM_EDIT_CHAPTER_H
#define TOTEM_EDIT_CHAPTER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_EDIT_CHAPTER			(totem_edit_chapter_get_type ())
#define TOTEM_EDIT_CHAPTER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_EDIT_CHAPTER, TotemEditChapter))
#define TOTEM_EDIT_CHAPTER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_EDIT_CHAPTER, TotemEditChapterClass))
#define TOTEM_IS_EDIT_CHAPTER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_EDIT_CHAPTER))
#define TOTEM_IS_EDIT_CHAPTER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_EDIT_CHAPTER))

typedef struct TotemEditChapter			TotemEditChapter;
typedef struct TotemEditChapterClass		TotemEditChapterClass;
typedef struct TotemEditChapterPrivate		TotemEditChapterPrivate;

struct TotemEditChapter {
	GtkDialog parent;
	TotemEditChapterPrivate *priv;
};

struct TotemEditChapterClass {
	GtkDialogClass parent_class;
};

GType totem_edit_chapter_get_type (void);
GtkWidget * totem_edit_chapter_new (void);
void totem_edit_chapter_set_title (TotemEditChapter *edit_chapter, const gchar *title);
gchar * totem_edit_chapter_get_title (TotemEditChapter *edit_chapter);

G_END_DECLS

#endif /* TOTEM_EDIT_CHAPTER_H */
