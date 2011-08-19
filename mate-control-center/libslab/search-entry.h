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

#ifndef __NLD_SEARCH_ENTRY_H__
#define __NLD_SEARCH_ENTRY_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NLD_TYPE_SEARCH_ENTRY            (nld_search_entry_get_type ())
#define NLD_SEARCH_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NLD_TYPE_SEARCH_ENTRY, NldSearchEntry))
#define NLD_SEARCH_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NLD_TYPE_SEARCH_ENTRY, NldSearchEntryClass))
#define NLD_IS_SEARCH_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NLD_TYPE_SEARCH_ENTRY))
#define NLD_IS_SEARCH_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NLD_TYPE_SEARCH_ENTRY))
#define NLD_SEARCH_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NLD_TYPE_SEARCH_ENTRY, NldSearchEntryClass))

typedef struct
{
	GtkEntry parent;
} NldSearchEntry;

typedef struct
{
	GtkEntryClass parent_class;
} NldSearchEntryClass;

GType nld_search_entry_get_type (void);

GtkWidget *nld_search_entry_new (void);

G_END_DECLS
#endif /* __NLD_SEARCH_ENTRY_H__ */
