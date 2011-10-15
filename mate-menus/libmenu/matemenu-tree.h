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

#ifndef __MATEMENU_TREE_H__
#define __MATEMENU_TREE_H__

#ifndef MATEMENU_I_KNOW_THIS_IS_UNSTABLE
	#error "libmate-menu should only be used if you understand that it's subject to frequent change, and is not supported as a fixed API/ABI or as part of the platform"
#endif

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MateMenuTree          MateMenuTree;
typedef struct MateMenuTreeItem      MateMenuTreeItem;
typedef struct MateMenuTreeDirectory MateMenuTreeDirectory;
typedef struct MateMenuTreeEntry     MateMenuTreeEntry;
typedef struct MateMenuTreeSeparator MateMenuTreeSeparator;
typedef struct MateMenuTreeHeader    MateMenuTreeHeader;
typedef struct MateMenuTreeAlias     MateMenuTreeAlias;

typedef void (*MateMenuTreeChangedFunc) (MateMenuTree *tree,
				      gpointer  user_data);

typedef enum {
	MATEMENU_TREE_ITEM_INVALID = 0,
	MATEMENU_TREE_ITEM_DIRECTORY,
	MATEMENU_TREE_ITEM_ENTRY,
	MATEMENU_TREE_ITEM_SEPARATOR,
	MATEMENU_TREE_ITEM_HEADER,
	MATEMENU_TREE_ITEM_ALIAS
} MateMenuTreeItemType;

#define MATEMENU_TREE_ITEM(i)      ((MateMenuTreeItem *)(i))
#define MATEMENU_TREE_DIRECTORY(i) ((MateMenuTreeDirectory *)(i))
#define MATEMENU_TREE_ENTRY(i)     ((MateMenuTreeEntry *)(i))
#define MATEMENU_TREE_SEPARATOR(i) ((MateMenuTreeSeparator *)(i))
#define MATEMENU_TREE_HEADER(i)    ((MateMenuTreeHeader *)(i))
#define MATEMENU_TREE_ALIAS(i)     ((MateMenuTreeAlias *)(i))

typedef enum
{
  MATEMENU_TREE_FLAGS_NONE                = 0,
  MATEMENU_TREE_FLAGS_INCLUDE_EXCLUDED    = 1 << 0,
  MATEMENU_TREE_FLAGS_SHOW_EMPTY          = 1 << 1,
  MATEMENU_TREE_FLAGS_INCLUDE_NODISPLAY   = 1 << 2,
  MATEMENU_TREE_FLAGS_SHOW_ALL_SEPARATORS = 1 << 3,
  MATEMENU_TREE_FLAGS_MASK                = 0x0f
} MateMenuTreeFlags;

typedef enum
{
  #define MATEMENU_TREE_SORT_FIRST MATEMENU_TREE_SORT_NAME
  MATEMENU_TREE_SORT_NAME = 0,
  MATEMENU_TREE_SORT_DISPLAY_NAME
  #define MATEMENU_TREE_SORT_LAST MATEMENU_TREE_SORT_DISPLAY_NAME
} MateMenuTreeSortKey;

MateMenuTree *matemenu_tree_lookup (const char     *menu_file,
			      MateMenuTreeFlags  flags);

MateMenuTree *matemenu_tree_ref   (MateMenuTree *tree);
void       matemenu_tree_unref (MateMenuTree *tree);

void     matemenu_tree_set_user_data (MateMenuTree       *tree,
				   gpointer        user_data,
				   GDestroyNotify  dnotify);
gpointer matemenu_tree_get_user_data (MateMenuTree       *tree);

const char         *matemenu_tree_get_menu_file           (MateMenuTree  *tree);
MateMenuTreeDirectory *matemenu_tree_get_root_directory      (MateMenuTree  *tree);
MateMenuTreeDirectory *matemenu_tree_get_directory_from_path (MateMenuTree  *tree,
							const char *path);

MateMenuTreeSortKey     matemenu_tree_get_sort_key (MateMenuTree        *tree);
void                 matemenu_tree_set_sort_key (MateMenuTree        *tree,
					      MateMenuTreeSortKey  sort_key);



gpointer matemenu_tree_item_ref   (gpointer item);
void     matemenu_tree_item_unref (gpointer item);

void     matemenu_tree_item_set_user_data (MateMenuTreeItem   *item,
					gpointer        user_data,
					GDestroyNotify  dnotify);
gpointer matemenu_tree_item_get_user_data (MateMenuTreeItem   *item);

MateMenuTreeItemType   matemenu_tree_item_get_type   (MateMenuTreeItem *item);
MateMenuTreeDirectory *matemenu_tree_item_get_parent (MateMenuTreeItem *item);


GSList     *matemenu_tree_directory_get_contents          (MateMenuTreeDirectory *directory);
const char *matemenu_tree_directory_get_name              (MateMenuTreeDirectory *directory);
const char *matemenu_tree_directory_get_comment           (MateMenuTreeDirectory *directory);
const char* matemenu_tree_directory_get_icon(MateMenuTreeDirectory* directory);
const char *matemenu_tree_directory_get_desktop_file_path (MateMenuTreeDirectory *directory);
const char *matemenu_tree_directory_get_menu_id           (MateMenuTreeDirectory *directory);
MateMenuTree  *matemenu_tree_directory_get_tree              (MateMenuTreeDirectory *directory);

gboolean matemenu_tree_directory_get_is_nodisplay (MateMenuTreeDirectory *directory);

char *matemenu_tree_directory_make_path (MateMenuTreeDirectory *directory,
				      MateMenuTreeEntry     *entry);


const char *matemenu_tree_entry_get_name               (MateMenuTreeEntry *entry);
const char *matemenu_tree_entry_get_generic_name       (MateMenuTreeEntry *entry);
const char *matemenu_tree_entry_get_display_name       (MateMenuTreeEntry *entry);
const char *matemenu_tree_entry_get_comment            (MateMenuTreeEntry *entry);
const char *matemenu_tree_entry_get_icon               (MateMenuTreeEntry *entry);
const char *matemenu_tree_entry_get_exec               (MateMenuTreeEntry *entry);
gboolean    matemenu_tree_entry_get_launch_in_terminal (MateMenuTreeEntry *entry);

const char *matemenu_tree_entry_get_desktop_file_path (MateMenuTreeEntry *entry);
const char *matemenu_tree_entry_get_desktop_file_id   (MateMenuTreeEntry *entry);

gboolean matemenu_tree_entry_get_is_excluded  (MateMenuTreeEntry *entry);
gboolean matemenu_tree_entry_get_is_nodisplay (MateMenuTreeEntry *entry);

MateMenuTreeDirectory *matemenu_tree_header_get_directory (MateMenuTreeHeader *header);

MateMenuTreeDirectory *matemenu_tree_alias_get_directory (MateMenuTreeAlias *alias);
MateMenuTreeItem      *matemenu_tree_alias_get_item      (MateMenuTreeAlias *alias);

void matemenu_tree_add_monitor    (MateMenuTree            *tree,
				MateMenuTreeChangedFunc  callback,
				gpointer             user_data);
void matemenu_tree_remove_monitor (MateMenuTree            *tree,
				MateMenuTreeChangedFunc  callback,
				gpointer             user_data);

#ifdef __cplusplus
}
#endif

#endif /* __MATEMENU_TREE_H__ */
