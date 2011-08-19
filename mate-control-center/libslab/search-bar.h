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

#ifndef __NLD_SEARCH_BAR_H__
#define __NLD_SEARCH_BAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NLD_TYPE_SEARCH_BAR            (nld_search_bar_get_type ())
#define NLD_SEARCH_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NLD_TYPE_SEARCH_BAR, NldSearchBar))
#define NLD_SEARCH_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NLD_TYPE_SEARCH_BAR, NldSearchBarClass))
#define NLD_IS_SEARCH_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NLD_TYPE_SEARCH_BAR))
#define NLD_IS_SEARCH_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NLD_TYPE_SEARCH_BAR))
#define NLD_SEARCH_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NLD_TYPE_SEARCH_BAR, NldSearchBarClass))

typedef struct
{
	GtkVBox parent;
} NldSearchBar;

typedef struct
{
	GtkVBoxClass parent_class;

	void (*search) (NldSearchBar *, int context_id, const char *text);
} NldSearchBarClass;

GType nld_search_bar_get_type (void);

GtkWidget *nld_search_bar_new (void);

void nld_search_bar_clear (NldSearchBar * search_bar);
gboolean nld_search_bar_has_focus (NldSearchBar * search_bar);

gboolean nld_search_bar_get_show_contexts (NldSearchBar * search_bar);
void nld_search_bar_set_show_contexts (NldSearchBar * search_bar, gboolean show_contexts);
void nld_search_bar_add_context (NldSearchBar * search_bar, const char *label,
	const char *icon_name, int context_id);

gboolean nld_search_bar_get_show_button (NldSearchBar * search_bar);
void nld_search_bar_set_show_button (NldSearchBar * search_bar, gboolean show_button);

int nld_search_bar_get_search_timeout (NldSearchBar * search_bar);
void nld_search_bar_set_search_timeout (NldSearchBar * search_bar, int search_timeout);

const char *nld_search_bar_get_text (NldSearchBar * search_bar);
void nld_search_bar_set_text (NldSearchBar * search_bar, const char *text, gboolean activate);

int nld_search_bar_get_context_id (NldSearchBar * search_bar);
void nld_search_bar_set_context_id (NldSearchBar * search_bar, int context_id);

G_END_DECLS
#endif /* __NLD_SEARCH_BAR_H__ */
