/*
 * Copyright (C) 2002 - 2004 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __ENTRY_DIRECTORIES_H__
#define __ENTRY_DIRECTORIES_H__

#include <glib.h>
#include "desktop-entries.h"

G_BEGIN_DECLS

typedef struct EntryDirectory EntryDirectory;

typedef void (*EntryDirectoryChangedFunc) (EntryDirectory *ed,
                                           gpointer        user_data);

EntryDirectory *entry_directory_new        (DesktopEntryType  entry_type,
                                            const char       *path);
EntryDirectory *entry_directory_new_legacy (DesktopEntryType  entry_type,
                                            const char       *path,
                                            const char       *legacy_prefix);

EntryDirectory *entry_directory_ref   (EntryDirectory *ed);
void            entry_directory_unref (EntryDirectory *ed);

void entry_directory_get_flat_contents (EntryDirectory   *ed,
                                        DesktopEntrySet  *desktop_entries,
                                        DesktopEntrySet  *directory_entries,
                                        GSList          **subdirs);


typedef struct EntryDirectoryList EntryDirectoryList;

EntryDirectoryList *entry_directory_list_new   (void);
EntryDirectoryList *entry_directory_list_ref   (EntryDirectoryList *list);
void                entry_directory_list_unref (EntryDirectoryList *list);

int  entry_directory_list_get_length  (EntryDirectoryList *list);
gboolean _entry_directory_list_compare (const EntryDirectoryList *a,
                                        const EntryDirectoryList *b);

void entry_directory_list_prepend     (EntryDirectoryList *list,
                                       EntryDirectory     *ed);
void entry_directory_list_append_list (EntryDirectoryList *list,
                                       EntryDirectoryList *to_append);

void entry_directory_list_add_monitors    (EntryDirectoryList        *list,
                                           EntryDirectoryChangedFunc  callback,
                                           gpointer                   user_data);
void entry_directory_list_remove_monitors (EntryDirectoryList        *list,
                                           EntryDirectoryChangedFunc  callback,
                                           gpointer                   user_data);

DesktopEntry* entry_directory_list_get_directory (EntryDirectoryList *list,
                                                  const char         *relative_path);

DesktopEntrySet *_entry_directory_list_get_all_desktops (EntryDirectoryList *list);
void             _entry_directory_list_empty_desktop_cache (void);

G_END_DECLS

#endif /* __ENTRY_DIRECTORIES_H__ */
