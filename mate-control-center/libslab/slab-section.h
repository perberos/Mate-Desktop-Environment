/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __SLAB_SECTION_H__
#define __SLAB_SECTION_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SLAB_SECTION_TYPE (slab_section_get_type ())
#define SLAB_SECTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SLAB_SECTION_TYPE,    SlabSection))
#define SLAB_SECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SLAB_SECTION_TYPE, SlabSectionClass))
#define IS_SLAB_SECTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SLAB_SECTION_TYPE ))
#define IS_SLAB_SECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SLAB_SECTION_TYPE ))
#define SLAB_SECTION_GET_CLASS(obj) (G_TYPE_CHECK_GET_CLASS ((obj), SLAB_SECTION_TYPE, SlabSectionClass))

#define SLAB_TOP_PADDING  5
#define SLAB_BOTTOM_PADDING  5
#define SLAB_LEFT_PADDING 10

typedef enum
{
	Style1,		/* SlabSections in left pane - no padding */
	Style2		/* SlabSections in right pane - padding, label text changes as group is selected */
} SlabStyle;

typedef struct
{
	GtkVBox parent_vbox;

	GtkWidget *title;
	GtkWidget *contents;
	SlabStyle style;
	gulong expose_handler_id;
	GtkBox *childbox;
	gboolean selected;
} SlabSection;

typedef struct
{
	GtkVBoxClass parent_class;
} SlabSectionClass;

GType slab_section_get_type (void);
GtkWidget *slab_section_new (const gchar * title, SlabStyle style);
GtkWidget *slab_section_new_with_markup (const gchar * title_markup, SlabStyle style);
void slab_section_set_title (SlabSection * section, const gchar * title);
void slab_section_set_contents (SlabSection * section, GtkWidget * contents);
void slab_section_set_selected (SlabSection * section, gboolean selected);

G_END_DECLS
#endif /* __SLAB_SECTION_H__ */
