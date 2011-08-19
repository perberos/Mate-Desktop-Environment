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

#ifndef __NLD_SEARCH_CONTEXT_PICKER_H__
#define __NLD_SEARCH_CONTEXT_PICKER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NLD_TYPE_SEARCH_CONTEXT_PICKER            (nld_search_context_picker_get_type ())
#define NLD_SEARCH_CONTEXT_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NLD_TYPE_SEARCH_CONTEXT_PICKER, NldSearchContextPicker))
#define NLD_SEARCH_CONTEXT_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NLD_TYPE_SEARCH_CONTEXT_PICKER, NldSearchContextPickerClass))
#define NLD_IS_SEARCH_CONTEXT_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NLD_TYPE_SEARCH_CONTEXT_PICKER))
#define NLD_IS_SEARCH_CONTEXT_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NLD_TYPE_SEARCH_CONTEXT_PICKER))
#define NLD_SEARCH_CONTEXT_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NLD_TYPE_SEARCH_CONTEXT_PICKER, NldSearchContextPickerClass))

typedef struct
{
	GtkButton parent;
} NldSearchContextPicker;

typedef struct
{
	GtkButtonClass parent_class;

	void (*context_changed) (NldSearchContextPicker *);
} NldSearchContextPickerClass;

GType nld_search_context_picker_get_type (void);

GtkWidget *nld_search_context_picker_new (void);

void nld_search_context_picker_add_context (NldSearchContextPicker * picker, const char *label,
	const char *icon_name, int context_id);

int nld_search_context_picker_get_context (NldSearchContextPicker * picker);
void nld_search_context_picker_set_context (NldSearchContextPicker * picker, int context_id);

G_END_DECLS
#endif /* __NLD_SEARCH_CONTEXT_PICKER_H__ */
