/* Menu layout in-memory data structure (a custom "DOM tree") */

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

#ifndef __MENU_LAYOUT_H__
#define __MENU_LAYOUT_H__

#include <glib.h>

#include "entry-directories.h"

G_BEGIN_DECLS

typedef struct MenuLayoutNode MenuLayoutNode;

typedef enum
{
  MENU_LAYOUT_NODE_ROOT,
  MENU_LAYOUT_NODE_PASSTHROUGH,
  MENU_LAYOUT_NODE_MENU,
  MENU_LAYOUT_NODE_APP_DIR,
  MENU_LAYOUT_NODE_DEFAULT_APP_DIRS,
  MENU_LAYOUT_NODE_DIRECTORY_DIR,
  MENU_LAYOUT_NODE_DEFAULT_DIRECTORY_DIRS,
  MENU_LAYOUT_NODE_DEFAULT_MERGE_DIRS,
  MENU_LAYOUT_NODE_NAME,
  MENU_LAYOUT_NODE_DIRECTORY,
  MENU_LAYOUT_NODE_ONLY_UNALLOCATED,
  MENU_LAYOUT_NODE_NOT_ONLY_UNALLOCATED,
  MENU_LAYOUT_NODE_INCLUDE,
  MENU_LAYOUT_NODE_EXCLUDE,
  MENU_LAYOUT_NODE_FILENAME,
  MENU_LAYOUT_NODE_CATEGORY,
  MENU_LAYOUT_NODE_ALL,
  MENU_LAYOUT_NODE_AND,
  MENU_LAYOUT_NODE_OR,
  MENU_LAYOUT_NODE_NOT,
  MENU_LAYOUT_NODE_MERGE_FILE,
  MENU_LAYOUT_NODE_MERGE_DIR,
  MENU_LAYOUT_NODE_LEGACY_DIR,
  MENU_LAYOUT_NODE_KDE_LEGACY_DIRS,
  MENU_LAYOUT_NODE_MOVE,
  MENU_LAYOUT_NODE_OLD,
  MENU_LAYOUT_NODE_NEW,
  MENU_LAYOUT_NODE_DELETED,
  MENU_LAYOUT_NODE_NOT_DELETED,
  MENU_LAYOUT_NODE_LAYOUT,
  MENU_LAYOUT_NODE_DEFAULT_LAYOUT,
  MENU_LAYOUT_NODE_MENUNAME,
  MENU_LAYOUT_NODE_SEPARATOR,
  MENU_LAYOUT_NODE_MERGE
} MenuLayoutNodeType;

typedef enum
{
  MENU_MERGE_FILE_TYPE_PATH = 0,
  MENU_MERGE_FILE_TYPE_PARENT
} MenuMergeFileType;

typedef enum
{
  MENU_LAYOUT_MERGE_NONE,
  MENU_LAYOUT_MERGE_MENUS,
  MENU_LAYOUT_MERGE_FILES,
  MENU_LAYOUT_MERGE_ALL
} MenuLayoutMergeType;

typedef enum
{
  MENU_LAYOUT_VALUES_NONE          = 0,
  MENU_LAYOUT_VALUES_SHOW_EMPTY    = 1 << 0,
  MENU_LAYOUT_VALUES_INLINE_MENUS  = 1 << 1,
  MENU_LAYOUT_VALUES_INLINE_LIMIT  = 1 << 2,
  MENU_LAYOUT_VALUES_INLINE_HEADER = 1 << 3,
  MENU_LAYOUT_VALUES_INLINE_ALIAS  = 1 << 4
} MenuLayoutValuesMask;

typedef struct
{
  MenuLayoutValuesMask mask;

  guint show_empty : 1;
  guint inline_menus : 1;
  guint inline_header : 1;
  guint inline_alias : 1;

  guint inline_limit;
} MenuLayoutValues;


MenuLayoutNode *menu_layout_load (const char  *filename,
                                  const char  *non_prefixed_basename,
                                  GError     **error);

MenuLayoutNode *menu_layout_node_new   (MenuLayoutNodeType  type);
MenuLayoutNode *menu_layout_node_ref   (MenuLayoutNode     *node);
void            menu_layout_node_unref (MenuLayoutNode     *node);

MenuLayoutNodeType menu_layout_node_get_type (MenuLayoutNode *node);

MenuLayoutNode *menu_layout_node_get_root     (MenuLayoutNode *node);
MenuLayoutNode *menu_layout_node_get_parent   (MenuLayoutNode *node);
MenuLayoutNode *menu_layout_node_get_children (MenuLayoutNode *node);
MenuLayoutNode *menu_layout_node_get_next     (MenuLayoutNode *node);

void menu_layout_node_insert_before (MenuLayoutNode *node,
                                     MenuLayoutNode *new_sibling);
void menu_layout_node_insert_after  (MenuLayoutNode *node,
                                     MenuLayoutNode *new_sibling);
void menu_layout_node_prepend_child (MenuLayoutNode *parent,
                                     MenuLayoutNode *new_child);
void menu_layout_node_append_child  (MenuLayoutNode *parent,
                                     MenuLayoutNode *new_child);

void menu_layout_node_unlink (MenuLayoutNode *node);
void menu_layout_node_steal  (MenuLayoutNode *node);

const char *menu_layout_node_get_content (MenuLayoutNode *node);
void        menu_layout_node_set_content (MenuLayoutNode *node,
                                          const char     *content);

char *menu_layout_node_get_content_as_path (MenuLayoutNode *node);

const char *menu_layout_node_root_get_name    (MenuLayoutNode *node);
const char *menu_layout_node_root_get_basedir (MenuLayoutNode *node);

const char         *menu_layout_node_menu_get_name           (MenuLayoutNode *node);
EntryDirectoryList *menu_layout_node_menu_get_app_dirs       (MenuLayoutNode *node);
EntryDirectoryList *menu_layout_node_menu_get_directory_dirs (MenuLayoutNode *node);

const char *menu_layout_node_move_get_old (MenuLayoutNode *node);
const char *menu_layout_node_move_get_new (MenuLayoutNode *node);

const char *menu_layout_node_legacy_dir_get_prefix (MenuLayoutNode *node);
void        menu_layout_node_legacy_dir_set_prefix (MenuLayoutNode *node,
                                                    const char     *prefix);

MenuMergeFileType menu_layout_node_merge_file_get_type (MenuLayoutNode    *node);
void              menu_layout_node_merge_file_set_type (MenuLayoutNode    *node,
							MenuMergeFileType  type);

MenuLayoutMergeType menu_layout_node_merge_get_type (MenuLayoutNode *node);

void menu_layout_node_default_layout_get_values (MenuLayoutNode   *node,
						 MenuLayoutValues *values);
void menu_layout_node_menuname_get_values       (MenuLayoutNode   *node,
						 MenuLayoutValues *values);

typedef void (* MenuLayoutNodeEntriesChangedFunc) (MenuLayoutNode *node,
                                                   gpointer        user_data);

void menu_layout_node_root_add_entries_monitor    (MenuLayoutNode                   *node,
                                                   MenuLayoutNodeEntriesChangedFunc  callback,
                                                   gpointer                          user_data);
void menu_layout_node_root_remove_entries_monitor (MenuLayoutNode                   *node,
                                                   MenuLayoutNodeEntriesChangedFunc  callback,
                                                   gpointer                          user_data);

G_END_DECLS

#endif /* __MENU_LAYOUT_H__ */
