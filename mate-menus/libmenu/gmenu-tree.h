/*
 * Copyright (C) 2004 Red Hat, Inc.
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

#ifndef __GMENU_TREE_H__
#define __GMENU_TREE_H__

#ifndef GMENU_I_KNOW_THIS_IS_UNSTABLE
#error "libmate-menu should only be used if you understand that it's subject to frequent change, and is not supported as a fixed API/ABI or as part of the platform"
#endif

#include <glib.h>

G_BEGIN_DECLS

typedef struct GMenuTree          GMenuTree;
typedef struct GMenuTreeItem      GMenuTreeItem;
typedef struct GMenuTreeDirectory GMenuTreeDirectory;
typedef struct GMenuTreeEntry     GMenuTreeEntry;
typedef struct GMenuTreeSeparator GMenuTreeSeparator;
typedef struct GMenuTreeHeader    GMenuTreeHeader;
typedef struct GMenuTreeAlias     GMenuTreeAlias;

typedef void (*GMenuTreeChangedFunc) (GMenuTree *tree,
				      gpointer  user_data);

typedef enum
{
  GMENU_TREE_ITEM_INVALID = 0,
  GMENU_TREE_ITEM_DIRECTORY,
  GMENU_TREE_ITEM_ENTRY,
  GMENU_TREE_ITEM_SEPARATOR,
  GMENU_TREE_ITEM_HEADER,
  GMENU_TREE_ITEM_ALIAS
} GMenuTreeItemType;

#define GMENU_TREE_ITEM(i)      ((GMenuTreeItem *)(i))
#define GMENU_TREE_DIRECTORY(i) ((GMenuTreeDirectory *)(i))
#define GMENU_TREE_ENTRY(i)     ((GMenuTreeEntry *)(i))
#define GMENU_TREE_SEPARATOR(i) ((GMenuTreeSeparator *)(i))
#define GMENU_TREE_HEADER(i)    ((GMenuTreeHeader *)(i))
#define GMENU_TREE_ALIAS(i)     ((GMenuTreeAlias *)(i))

typedef enum
{
  GMENU_TREE_FLAGS_NONE                = 0,
  GMENU_TREE_FLAGS_INCLUDE_EXCLUDED    = 1 << 0,
  GMENU_TREE_FLAGS_SHOW_EMPTY          = 1 << 1,
  GMENU_TREE_FLAGS_INCLUDE_NODISPLAY   = 1 << 2,
  GMENU_TREE_FLAGS_SHOW_ALL_SEPARATORS = 1 << 3,
  GMENU_TREE_FLAGS_MASK                = 0x0f
} GMenuTreeFlags;

typedef enum
{
  #define GMENU_TREE_SORT_FIRST GMENU_TREE_SORT_NAME
  GMENU_TREE_SORT_NAME = 0,
  GMENU_TREE_SORT_DISPLAY_NAME
  #define GMENU_TREE_SORT_LAST GMENU_TREE_SORT_DISPLAY_NAME
} GMenuTreeSortKey;

GMenuTree *gmenu_tree_lookup (const char     *menu_file,
			      GMenuTreeFlags  flags);

GMenuTree *gmenu_tree_ref   (GMenuTree *tree);
void       gmenu_tree_unref (GMenuTree *tree);

void     gmenu_tree_set_user_data (GMenuTree       *tree,
				   gpointer        user_data,
				   GDestroyNotify  dnotify);
gpointer gmenu_tree_get_user_data (GMenuTree       *tree);

const char         *gmenu_tree_get_menu_file           (GMenuTree  *tree);
GMenuTreeDirectory *gmenu_tree_get_root_directory      (GMenuTree  *tree);
GMenuTreeDirectory *gmenu_tree_get_directory_from_path (GMenuTree  *tree,
							const char *path);

GMenuTreeSortKey     gmenu_tree_get_sort_key (GMenuTree        *tree);
void                 gmenu_tree_set_sort_key (GMenuTree        *tree,
					      GMenuTreeSortKey  sort_key);



gpointer gmenu_tree_item_ref   (gpointer item);
void     gmenu_tree_item_unref (gpointer item);

void     gmenu_tree_item_set_user_data (GMenuTreeItem   *item,
					gpointer        user_data,
					GDestroyNotify  dnotify);
gpointer gmenu_tree_item_get_user_data (GMenuTreeItem   *item);

GMenuTreeItemType   gmenu_tree_item_get_type   (GMenuTreeItem *item);
GMenuTreeDirectory *gmenu_tree_item_get_parent (GMenuTreeItem *item);


GSList     *gmenu_tree_directory_get_contents          (GMenuTreeDirectory *directory);
const char *gmenu_tree_directory_get_name              (GMenuTreeDirectory *directory);
const char *gmenu_tree_directory_get_comment           (GMenuTreeDirectory *directory);
const char* gmenu_tree_directory_get_icon(GMenuTreeDirectory* directory);
const char *gmenu_tree_directory_get_desktop_file_path (GMenuTreeDirectory *directory);
const char *gmenu_tree_directory_get_menu_id           (GMenuTreeDirectory *directory);
GMenuTree  *gmenu_tree_directory_get_tree              (GMenuTreeDirectory *directory);

gboolean gmenu_tree_directory_get_is_nodisplay (GMenuTreeDirectory *directory);

char *gmenu_tree_directory_make_path (GMenuTreeDirectory *directory,
				      GMenuTreeEntry     *entry);


const char *gmenu_tree_entry_get_name               (GMenuTreeEntry *entry);
const char *gmenu_tree_entry_get_generic_name       (GMenuTreeEntry *entry);
const char *gmenu_tree_entry_get_display_name       (GMenuTreeEntry *entry);
const char *gmenu_tree_entry_get_comment            (GMenuTreeEntry *entry);
const char *gmenu_tree_entry_get_icon               (GMenuTreeEntry *entry);
const char *gmenu_tree_entry_get_exec               (GMenuTreeEntry *entry);
gboolean    gmenu_tree_entry_get_launch_in_terminal (GMenuTreeEntry *entry);

const char *gmenu_tree_entry_get_desktop_file_path (GMenuTreeEntry *entry);
const char *gmenu_tree_entry_get_desktop_file_id   (GMenuTreeEntry *entry);

gboolean gmenu_tree_entry_get_is_excluded  (GMenuTreeEntry *entry);
gboolean gmenu_tree_entry_get_is_nodisplay (GMenuTreeEntry *entry);

GMenuTreeDirectory *gmenu_tree_header_get_directory (GMenuTreeHeader *header);

GMenuTreeDirectory *gmenu_tree_alias_get_directory (GMenuTreeAlias *alias);
GMenuTreeItem      *gmenu_tree_alias_get_item      (GMenuTreeAlias *alias);

void gmenu_tree_add_monitor    (GMenuTree            *tree,
				GMenuTreeChangedFunc  callback,
				gpointer             user_data);
void gmenu_tree_remove_monitor (GMenuTree            *tree,
				GMenuTreeChangedFunc  callback,
				gpointer             user_data);

G_END_DECLS

#endif /* __GMENU_TREE_H__ */
