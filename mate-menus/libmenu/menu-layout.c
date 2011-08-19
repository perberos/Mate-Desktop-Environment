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

#include <config.h>

#include "menu-layout.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "canonicalize.h"
#include "entry-directories.h"
#include "menu-util.h"

typedef struct MenuLayoutNodeMenu          MenuLayoutNodeMenu;
typedef struct MenuLayoutNodeRoot          MenuLayoutNodeRoot;
typedef struct MenuLayoutNodeLegacyDir     MenuLayoutNodeLegacyDir;
typedef struct MenuLayoutNodeMergeFile     MenuLayoutNodeMergeFile;
typedef struct MenuLayoutNodeDefaultLayout MenuLayoutNodeDefaultLayout;
typedef struct MenuLayoutNodeMenuname      MenuLayoutNodeMenuname;
typedef struct MenuLayoutNodeMerge         MenuLayoutNodeMerge;

struct MenuLayoutNode
{
  /* Node lists are circular, for length-one lists
   * prev/next point back to the node itself.
   */
  MenuLayoutNode *prev;
  MenuLayoutNode *next;
  MenuLayoutNode *parent;
  MenuLayoutNode *children;

  char *content;

  guint refcount : 20;
  guint type : 7;
};

struct MenuLayoutNodeRoot
{
  MenuLayoutNode node;

  char *basedir;
  char *name;

  GSList *monitors;
};

struct MenuLayoutNodeMenu
{
  MenuLayoutNode node;

  MenuLayoutNode *name_node; /* cache of the <Name> node */

  EntryDirectoryList *app_dirs;
  EntryDirectoryList *dir_dirs;
};

struct MenuLayoutNodeLegacyDir
{
  MenuLayoutNode node;

  char *prefix;
};

struct MenuLayoutNodeMergeFile
{
  MenuLayoutNode node;

  MenuMergeFileType type;
};

struct MenuLayoutNodeDefaultLayout
{
  MenuLayoutNode node;

  MenuLayoutValues layout_values;
};

struct MenuLayoutNodeMenuname
{
  MenuLayoutNode node;

  MenuLayoutValues layout_values;
};

struct MenuLayoutNodeMerge
{
  MenuLayoutNode node;

  MenuLayoutMergeType merge_type;
};

typedef struct
{
  MenuLayoutNodeEntriesChangedFunc callback;
  gpointer                         user_data;
} MenuLayoutNodeEntriesMonitor;


static inline MenuLayoutNode *
node_next (MenuLayoutNode *node)
{
  /* root nodes (no parent) never have siblings */
  if (node->parent == NULL)
    return NULL;

  /* circular list */
  if (node->next == node->parent->children)
    return NULL;

  return node->next;
}

static void
handle_entry_directory_changed (EntryDirectory *dir,
                                MenuLayoutNode *node)
{
  MenuLayoutNodeRoot *nr;
  GSList             *tmp;

  g_assert (node->type == MENU_LAYOUT_NODE_MENU);

  nr = (MenuLayoutNodeRoot *) menu_layout_node_get_root (node);

  tmp = nr->monitors;
  while (tmp != NULL)
    {
      MenuLayoutNodeEntriesMonitor *monitor = tmp->data;
      GSList                       *next    = tmp->next;

      monitor->callback ((MenuLayoutNode *) nr, monitor->user_data);

      tmp = next;
    }
}

static void
remove_entry_directory_list (MenuLayoutNodeMenu  *nm,
                             EntryDirectoryList **dirs)
{
  if (*dirs)
    {
      entry_directory_list_remove_monitors (*dirs,
                                            (EntryDirectoryChangedFunc) handle_entry_directory_changed,
                                            nm);
      entry_directory_list_unref (*dirs);
      *dirs = NULL;
    }
}

MenuLayoutNode *
menu_layout_node_ref (MenuLayoutNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);

  node->refcount += 1;

  return node;
}

void
menu_layout_node_unref (MenuLayoutNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->refcount > 0);

  node->refcount -= 1;
  if (node->refcount == 0)
    {
      MenuLayoutNode *iter;

      iter = node->children;
      while (iter != NULL)
        {
          MenuLayoutNode *next = node_next (iter);

          menu_layout_node_unref (iter);

          iter = next;
        }

      if (node->type == MENU_LAYOUT_NODE_MENU)
        {
          MenuLayoutNodeMenu *nm = (MenuLayoutNodeMenu *) node;

          if (nm->name_node)
            menu_layout_node_unref (nm->name_node);

          remove_entry_directory_list (nm, &nm->app_dirs);
          remove_entry_directory_list (nm, &nm->dir_dirs);
        }
      else if (node->type == MENU_LAYOUT_NODE_LEGACY_DIR)
        {
          MenuLayoutNodeLegacyDir *legacy = (MenuLayoutNodeLegacyDir *) node;

          g_free (legacy->prefix);
        }
      else if (node->type == MENU_LAYOUT_NODE_ROOT)
        {
          MenuLayoutNodeRoot *nr = (MenuLayoutNodeRoot*) node;

          g_slist_foreach (nr->monitors, (GFunc) g_free, NULL);
          g_slist_free (nr->monitors);

          g_free (nr->basedir);
          g_free (nr->name);
        }

      g_free (node->content);
      g_free (node);
    }
}

MenuLayoutNode *
menu_layout_node_new (MenuLayoutNodeType type)
{
  MenuLayoutNode *node;

  switch (type)
    {
    case MENU_LAYOUT_NODE_MENU:
      node = (MenuLayoutNode *) g_new0 (MenuLayoutNodeMenu, 1);
      break;

    case MENU_LAYOUT_NODE_LEGACY_DIR:
      node = (MenuLayoutNode *) g_new0 (MenuLayoutNodeLegacyDir, 1);
      break;

    case MENU_LAYOUT_NODE_ROOT:
      node = (MenuLayoutNode*) g_new0 (MenuLayoutNodeRoot, 1);
      break;

    case MENU_LAYOUT_NODE_MERGE_FILE:
      node = (MenuLayoutNode *) g_new0 (MenuLayoutNodeMergeFile, 1);
      break;

    case MENU_LAYOUT_NODE_DEFAULT_LAYOUT:
      node = (MenuLayoutNode *) g_new0 (MenuLayoutNodeDefaultLayout, 1);
      break;

    case MENU_LAYOUT_NODE_MENUNAME:
      node = (MenuLayoutNode *) g_new0 (MenuLayoutNodeMenuname, 1);
      break;

    case MENU_LAYOUT_NODE_MERGE:
      node = (MenuLayoutNode *) g_new0 (MenuLayoutNodeMerge, 1);
      break;

    default:
      node = g_new0 (MenuLayoutNode, 1);
      break;
    }

  node->type = type;

  node->refcount = 1;

  /* we're in a list of one node */
  node->next = node;
  node->prev = node;

  return node;
}

MenuLayoutNode *
menu_layout_node_get_next (MenuLayoutNode *node)
{
  return node_next (node);
}

MenuLayoutNode *
menu_layout_node_get_parent (MenuLayoutNode *node)
{
  return node->parent;
}

MenuLayoutNode *
menu_layout_node_get_children (MenuLayoutNode *node)
{
  return node->children;
}

MenuLayoutNode *
menu_layout_node_get_root (MenuLayoutNode *node)
{
  MenuLayoutNode *parent;

  parent = node;
  while (parent->parent != NULL)
    parent = parent->parent;

  g_assert (parent->type == MENU_LAYOUT_NODE_ROOT);

  return parent;
}

char *
menu_layout_node_get_content_as_path (MenuLayoutNode *node)
{
  if (node->content == NULL)
    {
      menu_verbose ("  (node has no content to get as a path)\n");
      return NULL;
    }

  if (g_path_is_absolute (node->content))
    {
      return g_strdup (node->content);
    }
  else
    {
      MenuLayoutNodeRoot *root;

      root = (MenuLayoutNodeRoot *) menu_layout_node_get_root (node);

      if (root->basedir == NULL)
        {
          menu_verbose ("No basedir available, using \"%s\" as-is\n",
                        node->content);
          return g_strdup (node->content);
        }
      else
        {
          menu_verbose ("Using basedir \"%s\" filename \"%s\"\n",
                        root->basedir, node->content);
          return g_build_filename (root->basedir, node->content, NULL);
        }
    }
}

#define RETURN_IF_NO_PARENT(node) G_STMT_START {                  \
    if ((node)->parent == NULL)                                   \
      {                                                           \
        g_warning ("To add siblings to a menu node, "             \
                   "it must not be the root node, "               \
                   "and must be linked in below some root node\n" \
                   "node parent = %p and type = %d",              \
                   (node)->parent, (node)->type);                 \
        return;                                                   \
      }                                                           \
  } G_STMT_END

#define RETURN_IF_HAS_ENTRY_DIRS(node) G_STMT_START {             \
    if ((node)->type == MENU_LAYOUT_NODE_MENU &&                  \
        (((MenuLayoutNodeMenu*)(node))->app_dirs != NULL ||       \
         ((MenuLayoutNodeMenu*)(node))->dir_dirs != NULL))        \
      {                                                           \
        g_warning ("node acquired ->app_dirs or ->dir_dirs "      \
                   "while not rooted in a tree\n");               \
        return;                                                   \
      }                                                           \
  } G_STMT_END                                                    \

void
menu_layout_node_insert_before (MenuLayoutNode *node,
                                MenuLayoutNode *new_sibling)
{
  g_return_if_fail (new_sibling != NULL);
  g_return_if_fail (new_sibling->parent == NULL);

  RETURN_IF_NO_PARENT (node);
  RETURN_IF_HAS_ENTRY_DIRS (new_sibling);

  new_sibling->next = node;
  new_sibling->prev = node->prev;

  node->prev = new_sibling;
  new_sibling->prev->next = new_sibling;

  new_sibling->parent = node->parent;

  if (node == node->parent->children)
    node->parent->children = new_sibling;

  menu_layout_node_ref (new_sibling);
}

void
menu_layout_node_insert_after (MenuLayoutNode *node,
                               MenuLayoutNode *new_sibling)
{
  g_return_if_fail (new_sibling != NULL);
  g_return_if_fail (new_sibling->parent == NULL);

  RETURN_IF_NO_PARENT (node);
  RETURN_IF_HAS_ENTRY_DIRS (new_sibling);

  new_sibling->prev = node;
  new_sibling->next = node->next;

  node->next = new_sibling;
  new_sibling->next->prev = new_sibling;

  new_sibling->parent = node->parent;

  menu_layout_node_ref (new_sibling);
}

void
menu_layout_node_prepend_child (MenuLayoutNode *parent,
                                MenuLayoutNode *new_child)
{
  RETURN_IF_HAS_ENTRY_DIRS (new_child);

  if (parent->children)
    {
      menu_layout_node_insert_before (parent->children, new_child);
    }
  else
    {
      parent->children = menu_layout_node_ref (new_child);
      new_child->parent = parent;
    }
}

void
menu_layout_node_append_child (MenuLayoutNode *parent,
                               MenuLayoutNode *new_child)
{
  RETURN_IF_HAS_ENTRY_DIRS (new_child);

  if (parent->children)
    {
      menu_layout_node_insert_after (parent->children->prev, new_child);
    }
  else
    {
      parent->children = menu_layout_node_ref (new_child);
      new_child->parent = parent;
    }
}

void
menu_layout_node_unlink (MenuLayoutNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->parent != NULL);

  menu_layout_node_steal (node);
  menu_layout_node_unref (node);
}

static void
recursive_clean_entry_directory_lists (MenuLayoutNode *node,
                                       gboolean        apps)
{
  EntryDirectoryList **dirs;
  MenuLayoutNodeMenu  *nm;
  MenuLayoutNode      *iter;

  if (node->type != MENU_LAYOUT_NODE_MENU)
    return;

  nm = (MenuLayoutNodeMenu *) node;

  dirs = apps ? &nm->app_dirs : &nm->dir_dirs;

  if (*dirs == NULL || entry_directory_list_get_length (*dirs) == 0)
    return; /* child menus continue to have valid lists */

  remove_entry_directory_list (nm, dirs);

  iter = node->children;
  while (iter != NULL)
    {
      if (iter->type == MENU_LAYOUT_NODE_MENU)
        recursive_clean_entry_directory_lists (iter, apps);

      iter = node_next (iter);
    }
}

void
menu_layout_node_steal (MenuLayoutNode *node)
{
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->parent != NULL);

  switch (node->type)
    {
    case MENU_LAYOUT_NODE_NAME:
      {
        MenuLayoutNodeMenu *nm = (MenuLayoutNodeMenu *) node->parent;

        if (nm->name_node == node)
          {
            menu_layout_node_unref (nm->name_node);
            nm->name_node = NULL;
          }
      }
      break;

    case MENU_LAYOUT_NODE_APP_DIR:
      recursive_clean_entry_directory_lists (node->parent, TRUE);
      break;

    case MENU_LAYOUT_NODE_DIRECTORY_DIR:
      recursive_clean_entry_directory_lists (node->parent, FALSE);
      break;

    default:
      break;
    }

  if (node->parent && node->parent->children == node)
    {
      if (node->next != node)
        node->parent->children = node->next;
      else
        node->parent->children = NULL;
    }

  /* these are no-ops for length-one node lists */
  node->prev->next = node->next;
  node->next->prev = node->prev;

  node->parent = NULL;

  /* point to ourselves, now we're length one */
  node->next = node;
  node->prev = node;
}

MenuLayoutNodeType
menu_layout_node_get_type (MenuLayoutNode *node)
{
  return node->type;
}

const char *
menu_layout_node_get_content (MenuLayoutNode *node)
{
  return node->content;
}

void
menu_layout_node_set_content (MenuLayoutNode *node,
                              const char     *content)
{
  if (node->content == content)
    return;

  g_free (node->content);
  node->content = g_strdup (content);
}

const char *
menu_layout_node_root_get_name (MenuLayoutNode *node)
{
  MenuLayoutNodeRoot *nr;

  g_return_val_if_fail (node->type == MENU_LAYOUT_NODE_ROOT, NULL);

  nr = (MenuLayoutNodeRoot*) node;

  return nr->name;
}

const char *
menu_layout_node_root_get_basedir (MenuLayoutNode *node)
{
  MenuLayoutNodeRoot *nr;

  g_return_val_if_fail (node->type == MENU_LAYOUT_NODE_ROOT, NULL);

  nr = (MenuLayoutNodeRoot*) node;

  return nr->basedir;
}

const char *
menu_layout_node_menu_get_name (MenuLayoutNode *node)
{
  MenuLayoutNodeMenu *nm;

  g_return_val_if_fail (node->type == MENU_LAYOUT_NODE_MENU, NULL);

  nm = (MenuLayoutNodeMenu*) node;

  if (nm->name_node == NULL)
    {
      MenuLayoutNode *iter;

      iter = node->children;
      while (iter != NULL)
        {
          if (iter->type == MENU_LAYOUT_NODE_NAME)
            {
              nm->name_node = menu_layout_node_ref (iter);
              break;
            }

          iter = node_next (iter);
        }
    }

  if (nm->name_node == NULL)
    return NULL;

  return menu_layout_node_get_content (nm->name_node);
}

static void
ensure_dir_lists (MenuLayoutNodeMenu *nm)
{
  MenuLayoutNode     *node;
  MenuLayoutNode     *iter;
  EntryDirectoryList *app_dirs;
  EntryDirectoryList *dir_dirs;

  node = (MenuLayoutNode *) nm;

  if (nm->app_dirs && nm->dir_dirs)
    return;

  app_dirs = NULL;
  dir_dirs = NULL;

  if (nm->app_dirs == NULL)
    {
      app_dirs = entry_directory_list_new ();

      if (node->parent && node->parent->type == MENU_LAYOUT_NODE_MENU)
        {
          EntryDirectoryList *dirs;

          if ((dirs = menu_layout_node_menu_get_app_dirs (node->parent)))
            entry_directory_list_append_list (app_dirs, dirs);
        }
    }

  if (nm->dir_dirs == NULL)
    {
      dir_dirs = entry_directory_list_new ();

      if (node->parent && node->parent->type == MENU_LAYOUT_NODE_MENU)
        {
          EntryDirectoryList *dirs;

          if ((dirs = menu_layout_node_menu_get_directory_dirs (node->parent)))
            entry_directory_list_append_list (dir_dirs, dirs);
        }
    }

  iter = node->children;
  while (iter != NULL)
    {
      EntryDirectory *ed;

      if (app_dirs != NULL && iter->type == MENU_LAYOUT_NODE_APP_DIR)
        {
          char *path;

          path = menu_layout_node_get_content_as_path (iter);

          ed = entry_directory_new (DESKTOP_ENTRY_DESKTOP, path);
          if (ed != NULL)
            {
              entry_directory_list_prepend (app_dirs, ed);
              entry_directory_unref (ed);
            }

          g_free (path);
        }

      if (dir_dirs != NULL && iter->type == MENU_LAYOUT_NODE_DIRECTORY_DIR)
        {
          char *path;

          path = menu_layout_node_get_content_as_path (iter);

          ed = entry_directory_new (DESKTOP_ENTRY_DIRECTORY, path);
          if (ed != NULL)
            {
              entry_directory_list_prepend (dir_dirs, ed);
              entry_directory_unref (ed);
            }

          g_free (path);
        }

      if (iter->type == MENU_LAYOUT_NODE_LEGACY_DIR)
        {
          MenuLayoutNodeLegacyDir *legacy = (MenuLayoutNodeLegacyDir *) iter;
          char                    *path;

          path = menu_layout_node_get_content_as_path (iter);

          if (app_dirs != NULL) /* we're loading app dirs */
            {
              ed = entry_directory_new_legacy (DESKTOP_ENTRY_DESKTOP,
                                               path,
                                               legacy->prefix);
              if (ed != NULL)
                {
                  entry_directory_list_prepend (app_dirs, ed);
                  entry_directory_unref (ed);
                }
            }

          if (dir_dirs != NULL) /* we're loading dir dirs */
            {
              ed = entry_directory_new_legacy (DESKTOP_ENTRY_DIRECTORY,
                                               path,
                                               legacy->prefix);
              if (ed != NULL)
                {
                  entry_directory_list_prepend (dir_dirs, ed);
                  entry_directory_unref (ed);
                }
            }

          g_free (path);
        }

      iter = node_next (iter);
    }

  if (app_dirs)
    {
      g_assert (nm->app_dirs == NULL);

      nm->app_dirs = app_dirs;
      entry_directory_list_add_monitors (nm->app_dirs,
                                         (EntryDirectoryChangedFunc) handle_entry_directory_changed,
                                         nm);
    }

  if (dir_dirs)
    {
      g_assert (nm->dir_dirs == NULL);

      nm->dir_dirs = dir_dirs;
      entry_directory_list_add_monitors (nm->dir_dirs,
                                         (EntryDirectoryChangedFunc) handle_entry_directory_changed,
                                         nm);
    }
}

EntryDirectoryList *
menu_layout_node_menu_get_app_dirs (MenuLayoutNode *node)
{
  MenuLayoutNodeMenu *nm;

  g_return_val_if_fail (node->type == MENU_LAYOUT_NODE_MENU, NULL);

  nm = (MenuLayoutNodeMenu *) node;

  ensure_dir_lists (nm);

  return nm->app_dirs;
}

EntryDirectoryList *
menu_layout_node_menu_get_directory_dirs (MenuLayoutNode *node)
{
  MenuLayoutNodeMenu *nm;

  g_return_val_if_fail (node->type == MENU_LAYOUT_NODE_MENU, NULL);

  nm = (MenuLayoutNodeMenu *) node;

  ensure_dir_lists (nm);

  return nm->dir_dirs;
}

const char *
menu_layout_node_move_get_old (MenuLayoutNode *node)
{
  MenuLayoutNode *iter;

  iter = node->children;
  while (iter != NULL)
    {
      if (iter->type == MENU_LAYOUT_NODE_OLD)
        return iter->content;

      iter = node_next (iter);
    }

  return NULL;
}

const char *
menu_layout_node_move_get_new (MenuLayoutNode *node)
{
  MenuLayoutNode *iter;

  iter = node->children;
  while (iter != NULL)
    {
      if (iter->type == MENU_LAYOUT_NODE_NEW)
        return iter->content;

      iter = node_next (iter);
    }

  return NULL;
}

const char *
menu_layout_node_legacy_dir_get_prefix (MenuLayoutNode *node)
{
  MenuLayoutNodeLegacyDir *legacy;

  g_return_val_if_fail (node->type == MENU_LAYOUT_NODE_LEGACY_DIR, NULL);

  legacy = (MenuLayoutNodeLegacyDir *) node;

  return legacy->prefix;
}

void
menu_layout_node_legacy_dir_set_prefix (MenuLayoutNode *node,
                                        const char     *prefix)
{
  MenuLayoutNodeLegacyDir *legacy;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_LEGACY_DIR);

  legacy = (MenuLayoutNodeLegacyDir *) node;

  g_free (legacy->prefix);
  legacy->prefix = g_strdup (prefix);
}

MenuMergeFileType
menu_layout_node_merge_file_get_type (MenuLayoutNode *node)
{
  MenuLayoutNodeMergeFile *merge_file;

  g_return_val_if_fail (node->type == MENU_LAYOUT_NODE_MERGE_FILE, FALSE);

  merge_file = (MenuLayoutNodeMergeFile *) node;

  return merge_file->type;
}

void
menu_layout_node_merge_file_set_type (MenuLayoutNode    *node,
				      MenuMergeFileType  type)
{
  MenuLayoutNodeMergeFile *merge_file;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_MERGE_FILE);

  merge_file = (MenuLayoutNodeMergeFile *) node;

  merge_file->type = type;
}

MenuLayoutMergeType
menu_layout_node_merge_get_type (MenuLayoutNode *node)
{
  MenuLayoutNodeMerge *merge;

  g_return_val_if_fail (node->type == MENU_LAYOUT_NODE_MERGE, 0);

  merge = (MenuLayoutNodeMerge *) node;

  return merge->merge_type;
}

static void
menu_layout_node_merge_set_type (MenuLayoutNode *node,
                                 const char     *merge_type)
{
  MenuLayoutNodeMerge *merge;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_MERGE);

  merge = (MenuLayoutNodeMerge *) node;

  merge->merge_type = MENU_LAYOUT_MERGE_NONE;

  if (strcmp (merge_type, "menus") == 0)
    {
      merge->merge_type = MENU_LAYOUT_MERGE_MENUS;
    }
  else if (strcmp (merge_type, "files") == 0)
    {
      merge->merge_type = MENU_LAYOUT_MERGE_FILES;
    }
  else if (strcmp (merge_type, "all") == 0)
    {
      merge->merge_type = MENU_LAYOUT_MERGE_ALL;
    }
}

void
menu_layout_node_default_layout_get_values (MenuLayoutNode   *node,
					    MenuLayoutValues *values)
{
  MenuLayoutNodeDefaultLayout *default_layout;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_DEFAULT_LAYOUT);
  g_return_if_fail (values != NULL);

  default_layout = (MenuLayoutNodeDefaultLayout *) node;

  *values = default_layout->layout_values;
}

void
menu_layout_node_menuname_get_values (MenuLayoutNode   *node,
				      MenuLayoutValues *values)
{
  MenuLayoutNodeMenuname *menuname;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_MENUNAME);
  g_return_if_fail (values != NULL);

  menuname = (MenuLayoutNodeMenuname *) node;

  *values = menuname->layout_values;
}

static void
menu_layout_values_set (MenuLayoutValues *values,
			const char       *show_empty,
			const char       *inline_menus,
			const char       *inline_limit,
			const char       *inline_header,
			const char       *inline_alias)
{
  values->mask          = MENU_LAYOUT_VALUES_NONE;
  values->show_empty    = FALSE;
  values->inline_menus  = FALSE;
  values->inline_limit  = 4;
  values->inline_header = FALSE;
  values->inline_alias  = FALSE;

  if (show_empty != NULL)
    {
      values->show_empty = strcmp (show_empty, "true") == 0;
      values->mask |= MENU_LAYOUT_VALUES_SHOW_EMPTY;
    }

  if (inline_menus != NULL)
    {
      values->inline_menus = strcmp (inline_menus, "true") == 0;
      values->mask |= MENU_LAYOUT_VALUES_INLINE_MENUS;
    }
  
  if (inline_limit != NULL)
    {
      char *end;
      int   limit;

      limit = strtol (inline_limit, &end, 10);
      if (*end == '\0')
	{
	  values->inline_limit = limit;
	  values->mask |= MENU_LAYOUT_VALUES_INLINE_LIMIT;
	}
    }

  if (inline_header != NULL)
    {
      values->inline_header = strcmp (inline_header, "true") == 0;
      values->mask |= MENU_LAYOUT_VALUES_INLINE_HEADER;
    }

  if (inline_alias != NULL)
    {
      values->inline_alias = strcmp (inline_alias, "true") == 0;
      values->mask |= MENU_LAYOUT_VALUES_INLINE_ALIAS;
    }
}

static void
menu_layout_node_default_layout_set_values (MenuLayoutNode *node,
					    const char     *show_empty,
					    const char     *inline_menus,
					    const char     *inline_limit,
					    const char     *inline_header,
					    const char     *inline_alias)
{
  MenuLayoutNodeDefaultLayout *default_layout;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_DEFAULT_LAYOUT);

  default_layout = (MenuLayoutNodeDefaultLayout *) node;

  menu_layout_values_set (&default_layout->layout_values,
			  show_empty,
			  inline_menus,
			  inline_limit,
			  inline_header,
			  inline_alias);
}

static void
menu_layout_node_menuname_set_values (MenuLayoutNode *node,
				      const char     *show_empty,
				      const char     *inline_menus,
				      const char     *inline_limit,
				      const char     *inline_header,
				      const char     *inline_alias)
{
  MenuLayoutNodeMenuname *menuname;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_MENUNAME);

  menuname = (MenuLayoutNodeMenuname *) node;

  menu_layout_values_set (&menuname->layout_values,
			  show_empty,
			  inline_menus,
			  inline_limit,
			  inline_header,
			  inline_alias);
}

void
menu_layout_node_root_add_entries_monitor (MenuLayoutNode                   *node,
                                           MenuLayoutNodeEntriesChangedFunc  callback,
                                           gpointer                          user_data)
{
  MenuLayoutNodeEntriesMonitor *monitor;
  MenuLayoutNodeRoot           *nr;
  GSList                       *tmp;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_ROOT);

  nr = (MenuLayoutNodeRoot *) node;

  tmp = nr->monitors;
  while (tmp != NULL)
    {
      monitor = tmp->data;

      if (monitor->callback  == callback &&
          monitor->user_data == user_data)
        break;

      tmp = tmp->next;
    }

  if (tmp == NULL)
    {
      monitor            = g_new0 (MenuLayoutNodeEntriesMonitor, 1);
      monitor->callback  = callback;
      monitor->user_data = user_data;

      nr->monitors = g_slist_append (nr->monitors, monitor);
    }
}

void
menu_layout_node_root_remove_entries_monitor (MenuLayoutNode                   *node,
                                              MenuLayoutNodeEntriesChangedFunc  callback,
                                              gpointer                          user_data)
{
  MenuLayoutNodeRoot *nr;
  GSList             *tmp;

  g_return_if_fail (node->type == MENU_LAYOUT_NODE_ROOT);

  nr = (MenuLayoutNodeRoot *) node;

  tmp = nr->monitors;
  while (tmp != NULL)
    {
      MenuLayoutNodeEntriesMonitor *monitor = tmp->data;
      GSList                       *next = tmp->next;

      if (monitor->callback == callback &&
          monitor->user_data == user_data)
        {
          nr->monitors = g_slist_delete_link (nr->monitors, tmp);
          g_free (monitor);
        }

      tmp = next;
    }
}


/*
 * Menu file parsing
 */

typedef struct
{
  MenuLayoutNode *root;
  MenuLayoutNode *stack_top;
} MenuParser;

static void set_error (GError             **err,
                       GMarkupParseContext *context,
                       int                  error_domain,
                       int                  error_code,
                       const char          *format,
                       ...) G_GNUC_PRINTF (5, 6);

static void add_context_to_error (GError             **err,
                                  GMarkupParseContext *context);

static void start_element_handler (GMarkupParseContext  *context,
                                   const char           *element_name,
                                   const char          **attribute_names,
                                   const char          **attribute_values,
                                   gpointer              user_data,
                                   GError              **error);
static void end_element_handler   (GMarkupParseContext  *context,
                                   const char           *element_name,
                                   gpointer              user_data,
                                   GError              **error);
static void text_handler          (GMarkupParseContext  *context,
                                   const char           *text,
                                   gsize                 text_len,
                                   gpointer              user_data,
                                   GError              **error);
static void passthrough_handler   (GMarkupParseContext  *context,
                                   const char           *passthrough_text,
                                   gsize                 text_len,
                                   gpointer              user_data,
                                   GError              **error);


static GMarkupParser menu_funcs = {
  start_element_handler,
  end_element_handler,
  text_handler,
  passthrough_handler,
  NULL
};

static void
set_error (GError              **err,
           GMarkupParseContext  *context,
           int                   error_domain,
           int                   error_code,
           const char           *format,
           ...)
{
  int      line, ch;
  va_list  args;
  char    *str;

  g_markup_parse_context_get_position (context, &line, &ch);

  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  g_set_error (err, error_domain, error_code,
               "Line %d character %d: %s",
               line, ch, str);

  g_free (str);
}

static void
add_context_to_error (GError             **err,
                      GMarkupParseContext *context)
{
  int   line, ch;
  char *str;

  if (err == NULL || *err == NULL)
    return;

  g_markup_parse_context_get_position (context, &line, &ch);

  str = g_strdup_printf ("Line %d character %d: %s",
                         line, ch, (*err)->message);
  g_free ((*err)->message);
  (*err)->message = str;
}

#define ELEMENT_IS(name) (strcmp (element_name, (name)) == 0)

typedef struct
{
  const char  *name;
  const char **retloc;
} LocateAttr;

static gboolean
locate_attributes (GMarkupParseContext  *context,
                   const char           *element_name,
                   const char          **attribute_names,
                   const char          **attribute_values,
                   GError              **error,
                   const char           *first_attribute_name,
                   const char          **first_attribute_retloc,
                   ...)
{
#define MAX_ATTRS 24
  LocateAttr   attrs[MAX_ATTRS];
  int          n_attrs;
  va_list      args;
  const char  *name;
  const char **retloc;
  gboolean     retval;
  int          i;

  g_return_val_if_fail (first_attribute_name != NULL, FALSE);
  g_return_val_if_fail (first_attribute_retloc != NULL, FALSE);

  retval = TRUE;

  n_attrs = 1;
  attrs[0].name = first_attribute_name;
  attrs[0].retloc = first_attribute_retloc;
  *first_attribute_retloc = NULL;

  va_start (args, first_attribute_retloc);

  name = va_arg (args, const char *);
  retloc = va_arg (args, const char **);

  while (name != NULL)
    {
      g_return_val_if_fail (retloc != NULL, FALSE);

      g_assert (n_attrs < MAX_ATTRS);

      attrs[n_attrs].name = name;
      attrs[n_attrs].retloc = retloc;
      n_attrs += 1;
      *retloc = NULL;

      name = va_arg (args, const char *);
      retloc = va_arg (args, const char **);
    }

  va_end (args);

  i = 0;
  while (attribute_names[i])
    {
      int j;

      j = 0;
      while (j < n_attrs)
        {
          if (strcmp (attrs[j].name, attribute_names[i]) == 0)
            {
              retloc = attrs[j].retloc;

              if (*retloc != NULL)
                {
                  set_error (error, context,
                             G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                             "Attribute \"%s\" repeated twice on the same <%s> element",
                             attrs[j].name, element_name);
                  retval = FALSE;
                  goto out;
                }

              *retloc = attribute_values[i];
              break;
            }

          ++j;
        }

      if (j == n_attrs)
        {
          set_error (error, context,
                     G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                     "Attribute \"%s\" is invalid on <%s> element in this context",
                     attribute_names[i], element_name);
          retval = FALSE;
          goto out;
        }

      ++i;
    }

 out:
  return retval;

#undef MAX_ATTRS
}

static gboolean
check_no_attributes (GMarkupParseContext  *context,
                     const char           *element_name,
                     const char          **attribute_names,
                     const char          **attribute_values,
                     GError              **error)
{
  if (attribute_names[0] != NULL)
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 "Attribute \"%s\" is invalid on <%s> element in this context",
                 attribute_names[0], element_name);
      return FALSE;
    }

  return TRUE;
}

static int
has_child_of_type (MenuLayoutNode     *node,
                   MenuLayoutNodeType  type)
{
  MenuLayoutNode *iter;

  iter = node->children;
  while (iter)
    {
      if (iter->type == type)
        return TRUE;

      iter = node_next (iter);
    }

  return FALSE;
}

static void
push_node (MenuParser         *parser,
           MenuLayoutNodeType  type)
{
  MenuLayoutNode *node;

  node = menu_layout_node_new (type);
  menu_layout_node_append_child (parser->stack_top, node);
  menu_layout_node_unref (node);

  parser->stack_top = node;
}

static void
start_menu_element (MenuParser           *parser,
                    GMarkupParseContext  *context,
                    const char           *element_name,
                    const char          **attribute_names,
                    const char          **attribute_values,
                    GError              **error)
{
  if (!check_no_attributes (context, element_name,
                            attribute_names, attribute_values,
                            error))
    return;

  if (!(parser->stack_top->type == MENU_LAYOUT_NODE_ROOT ||
        parser->stack_top->type == MENU_LAYOUT_NODE_MENU))
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 "<Menu> element can only appear below other <Menu> elements or at toplevel\n");
    }
  else
    {
      push_node (parser, MENU_LAYOUT_NODE_MENU);
    }
}

static void
start_menu_child_element (MenuParser           *parser,
                          GMarkupParseContext  *context,
                          const char           *element_name,
                          const char          **attribute_names,
                          const char          **attribute_values,
                          GError              **error)
{
  if (ELEMENT_IS ("LegacyDir"))
    {
      const char *prefix;

      push_node (parser, MENU_LAYOUT_NODE_LEGACY_DIR);

      if (!locate_attributes (context, element_name,
                              attribute_names, attribute_values,
                              error,
                              "prefix", &prefix,
                              NULL))
        return;

      menu_layout_node_legacy_dir_set_prefix (parser->stack_top, prefix);
    }
  else if (ELEMENT_IS ("MergeFile"))
    {
      const char *type;

      push_node (parser, MENU_LAYOUT_NODE_MERGE_FILE);

      if (!locate_attributes (context, element_name,
                              attribute_names, attribute_values,
                              error,
                              "type", &type,
                              NULL))
        return;

      if (type != NULL && strcmp (type, "parent") == 0)
	{
	  menu_layout_node_merge_file_set_type (parser->stack_top,
						MENU_MERGE_FILE_TYPE_PARENT);
	}
    }
  else if (ELEMENT_IS ("DefaultLayout"))
    {
      const char *show_empty;
      const char *inline_menus;
      const char *inline_limit;
      const char *inline_header;
      const char *inline_alias;

      push_node (parser, MENU_LAYOUT_NODE_DEFAULT_LAYOUT);

      locate_attributes (context, element_name,
                         attribute_names, attribute_values,
                         error,
                         "show_empty",    &show_empty,
                         "inline",        &inline_menus,
                         "inline_limit",  &inline_limit,
                         "inline_header", &inline_header,
                         "inline_alias",  &inline_alias,
                         NULL);

      menu_layout_node_default_layout_set_values (parser->stack_top,
						  show_empty,
						  inline_menus,
						  inline_limit,
						  inline_header,
						  inline_alias);
    }
  else
    {
      if (!check_no_attributes (context, element_name,
                                attribute_names, attribute_values,
                                error))
        return;

      if (ELEMENT_IS ("AppDir"))
        {
          push_node (parser, MENU_LAYOUT_NODE_APP_DIR);
        }
      else if (ELEMENT_IS ("DefaultAppDirs"))
        {
          push_node (parser, MENU_LAYOUT_NODE_DEFAULT_APP_DIRS);
        }
      else if (ELEMENT_IS ("DirectoryDir"))
        {
          push_node (parser, MENU_LAYOUT_NODE_DIRECTORY_DIR);
        }
      else if (ELEMENT_IS ("DefaultDirectoryDirs"))
        {
          push_node (parser, MENU_LAYOUT_NODE_DEFAULT_DIRECTORY_DIRS);
        }
      else if (ELEMENT_IS ("DefaultMergeDirs"))
        {
          push_node (parser, MENU_LAYOUT_NODE_DEFAULT_MERGE_DIRS);
        }
      else if (ELEMENT_IS ("Name"))
        {
          if (has_child_of_type (parser->stack_top, MENU_LAYOUT_NODE_NAME))
            {
              set_error (error, context,
                         G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                         "Multiple <Name> elements in a <Menu> element is not allowed\n");
              return;
            }

          push_node (parser, MENU_LAYOUT_NODE_NAME);
        }
      else if (ELEMENT_IS ("Directory"))
        {
          push_node (parser, MENU_LAYOUT_NODE_DIRECTORY);
        }
      else if (ELEMENT_IS ("OnlyUnallocated"))
        {
          push_node (parser, MENU_LAYOUT_NODE_ONLY_UNALLOCATED);
        }
      else if (ELEMENT_IS ("NotOnlyUnallocated"))
        {
          push_node (parser, MENU_LAYOUT_NODE_NOT_ONLY_UNALLOCATED);
        }
      else if (ELEMENT_IS ("Include"))
        {
          push_node (parser, MENU_LAYOUT_NODE_INCLUDE);
        }
      else if (ELEMENT_IS ("Exclude"))
        {
          push_node (parser, MENU_LAYOUT_NODE_EXCLUDE);
        }
      else if (ELEMENT_IS ("MergeDir"))
        {
          push_node (parser, MENU_LAYOUT_NODE_MERGE_DIR);
        }
      else if (ELEMENT_IS ("KDELegacyDirs"))
        {
          push_node (parser, MENU_LAYOUT_NODE_KDE_LEGACY_DIRS);
        }
      else if (ELEMENT_IS ("Move"))
        {
          push_node (parser, MENU_LAYOUT_NODE_MOVE);
        }
      else if (ELEMENT_IS ("Deleted"))
        {
          push_node (parser, MENU_LAYOUT_NODE_DELETED);

        }
      else if (ELEMENT_IS ("NotDeleted"))
        {
          push_node (parser, MENU_LAYOUT_NODE_NOT_DELETED);
        }
      else if (ELEMENT_IS ("Layout"))
        {
          push_node (parser, MENU_LAYOUT_NODE_LAYOUT);
        }
      else
        {
          set_error (error, context,
                     G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                     "Element <%s> may not appear below <%s>\n",
                     element_name, "Menu");
        }
    }
}

static void
start_matching_rule_element (MenuParser           *parser,
                             GMarkupParseContext  *context,
                             const char           *element_name,
                             const char          **attribute_names,
                             const char          **attribute_values,
                             GError              **error)
{
  if (!check_no_attributes (context, element_name,
                            attribute_names, attribute_values,
                            error))
    return;


  if (ELEMENT_IS ("Filename"))
    {
      push_node (parser, MENU_LAYOUT_NODE_FILENAME);
    }
  else if (ELEMENT_IS ("Category"))
    {
      push_node (parser, MENU_LAYOUT_NODE_CATEGORY);
    }
  else if (ELEMENT_IS ("All"))
    {
      push_node (parser, MENU_LAYOUT_NODE_ALL);
    }
  else if (ELEMENT_IS ("And"))
    {
      push_node (parser, MENU_LAYOUT_NODE_AND);
    }
  else if (ELEMENT_IS ("Or"))
    {
      push_node (parser, MENU_LAYOUT_NODE_OR);
    }
  else if (ELEMENT_IS ("Not"))
    {
      push_node (parser, MENU_LAYOUT_NODE_NOT);
    }
  else
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                 "Element <%s> may not appear in this context\n",
                 element_name);
    }
}

static void
start_move_child_element (MenuParser           *parser,
                          GMarkupParseContext  *context,
                          const char           *element_name,
                          const char          **attribute_names,
                          const char          **attribute_values,
                          GError              **error)
{
  if (!check_no_attributes (context, element_name,
                            attribute_names, attribute_values,
                            error))
    return;

  if (ELEMENT_IS ("Old"))
    {
      push_node (parser, MENU_LAYOUT_NODE_OLD);
    }
  else if (ELEMENT_IS ("New"))
    {
      push_node (parser, MENU_LAYOUT_NODE_NEW);
    }
  else
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                 "Element <%s> may not appear below <%s>\n",
                 element_name, "Move");
    }
}

static void
start_layout_child_element (MenuParser           *parser,
                            GMarkupParseContext  *context,
                            const char           *element_name,
                            const char          **attribute_names,
                            const char          **attribute_values,
                            GError              **error)
{
  if (ELEMENT_IS ("Menuname"))
    {
      const char *show_empty;
      const char *inline_menus;
      const char *inline_limit;
      const char *inline_header;
      const char *inline_alias;

      push_node (parser, MENU_LAYOUT_NODE_MENUNAME);

      locate_attributes (context, element_name,
                         attribute_names, attribute_values,
                         error,
                         "show_empty",    &show_empty,
                         "inline",        &inline_menus,
                         "inline_limit",  &inline_limit,
                         "inline_header", &inline_header,
                         "inline_alias",  &inline_alias,
                         NULL);

       menu_layout_node_menuname_set_values (parser->stack_top,
					     show_empty,
					     inline_menus,
					     inline_limit,
					     inline_header,
					     inline_alias);
    }
  else if (ELEMENT_IS ("Merge"))
    {
        const char *type;

        push_node (parser, MENU_LAYOUT_NODE_MERGE);

        locate_attributes (context, element_name,
                           attribute_names, attribute_values,
                           error,
                           "type", &type,
                           NULL);

	menu_layout_node_merge_set_type (parser->stack_top, type);
    }
  else
    {
      if (!check_no_attributes (context, element_name,
                                attribute_names, attribute_values,
                                error))
        return;

      if (ELEMENT_IS ("Filename"))
        {
          push_node (parser, MENU_LAYOUT_NODE_FILENAME);
        }
      else if (ELEMENT_IS ("Separator"))
        {
          push_node (parser, MENU_LAYOUT_NODE_SEPARATOR);
        }
      else
        {
          set_error (error, context,
                     G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                     "Element <%s> may not appear below <%s>\n",
                     element_name, "Move");
        }
    }
}

static void
start_element_handler (GMarkupParseContext   *context,
                       const char            *element_name,
                       const char           **attribute_names,
                       const char           **attribute_values,
                       gpointer               user_data,
                       GError               **error)
{
  MenuParser *parser = user_data;

  if (ELEMENT_IS ("Menu"))
    {
      if (parser->stack_top == parser->root &&
          has_child_of_type (parser->root, MENU_LAYOUT_NODE_MENU))
        {
          set_error (error, context,
                     G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                     "Multiple root elements in menu file, only one toplevel <Menu> is allowed\n");
          return;
        }

      start_menu_element (parser, context, element_name,
                          attribute_names, attribute_values,
                          error);
    }
  else if (parser->stack_top == parser->root)
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 "Root element in a menu file must be <Menu>, not <%s>\n",
                 element_name);
    }
  else if (parser->stack_top->type == MENU_LAYOUT_NODE_MENU)
    {
      start_menu_child_element (parser, context, element_name,
                                attribute_names, attribute_values,
                                error);
    }
  else if (parser->stack_top->type == MENU_LAYOUT_NODE_INCLUDE ||
           parser->stack_top->type == MENU_LAYOUT_NODE_EXCLUDE ||
           parser->stack_top->type == MENU_LAYOUT_NODE_AND     ||
           parser->stack_top->type == MENU_LAYOUT_NODE_OR      ||
           parser->stack_top->type == MENU_LAYOUT_NODE_NOT)
    {
      start_matching_rule_element (parser, context, element_name,
                                   attribute_names, attribute_values,
                                   error);
    }
  else if (parser->stack_top->type == MENU_LAYOUT_NODE_MOVE)
    {
      start_move_child_element (parser, context, element_name,
                                attribute_names, attribute_values,
                                error);
    }
  else if (parser->stack_top->type == MENU_LAYOUT_NODE_LAYOUT ||
           parser->stack_top->type == MENU_LAYOUT_NODE_DEFAULT_LAYOUT)
    {
      start_layout_child_element (parser, context, element_name,
                                  attribute_names, attribute_values,
                                  error);
    }
  else
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                 "Element <%s> may not appear in this context\n",
                 element_name);
    }

  add_context_to_error (error, context);
}

/* we want to make sure that the <Layout> or <DefaultLayout> is either empty,
 * or contain one <Merge> of type "all", or contain one <Merge> of type "menus"
 * and one <Merge> of type "files". If this is not the case, we try to clean up
 * things:
 *  + if there is at least one <Merge> of type "all", then we only keep the
 *    last <Merge> of type "all" and remove all others <Merge>
 *  + if there is no <Merge> with type "all", we keep only the last <Merge> of
 *    type "menus" and the last <Merge> of type "files". If there's no <Merge>
 *    of type "menus" we append one, and then if there's no <Merge> of type
 *    "files", we append one. (So menus are before files)
 */
static gboolean
fixup_layout_node (GMarkupParseContext   *context,
                   MenuParser            *parser,
                   MenuLayoutNode        *node,
                   GError              **error)
{
  MenuLayoutNode *child;
  MenuLayoutNode *last_all;
  MenuLayoutNode *last_menus;
  MenuLayoutNode *last_files;
  int             n_all;
  int             n_menus;
  int             n_files;

  if (!node->children)
    {
      return TRUE;
    }

  last_all   = NULL;
  last_menus = NULL;
  last_files = NULL;
  n_all   = 0;
  n_menus = 0;
  n_files = 0;

  child = node->children;
  while (child != NULL)
    {
      switch (child->type)
        {
        case MENU_LAYOUT_NODE_MERGE:
	  switch (menu_layout_node_merge_get_type (child))
	    {
	    case MENU_LAYOUT_MERGE_NONE:
	      break;

	    case MENU_LAYOUT_MERGE_MENUS:
	      last_menus = child;
	      n_menus++;
	      break;

	    case MENU_LAYOUT_MERGE_FILES:
	      last_files = child;
	      n_files++;
	      break;

	    case MENU_LAYOUT_MERGE_ALL:
	      last_all = child;
	      n_all++;
	      break;

	    default:
	      g_assert_not_reached ();
	      break;
	    }
          break;

	  default:
	    break;
	}

      child = node_next (child);
    }

  if ((n_all == 1 && n_menus == 0 && n_files == 0) ||
      (n_all == 0 && n_menus == 1 && n_files == 1))
    {
      return TRUE;
    }
  else if (n_all > 1 || n_menus > 1 || n_files > 1 ||
           (n_all == 1 && (n_menus != 0 || n_files != 0)))
    {
      child = node->children;
      while (child != NULL)
	{
	  MenuLayoutNode *next;

	  next = node_next (child);

	  switch (child->type)
	    {
	    case MENU_LAYOUT_NODE_MERGE:
	      switch (menu_layout_node_merge_get_type (child))
		{
		case MENU_LAYOUT_MERGE_NONE:
		  break;

		case MENU_LAYOUT_MERGE_MENUS:
		  if (n_all || last_menus != child)
		    {
		      menu_verbose ("removing duplicated merge menus element\n");
		      menu_layout_node_unlink (child);
		    }
		  break;

		case MENU_LAYOUT_MERGE_FILES:
		  if (n_all || last_files != child)
		    {
		      menu_verbose ("removing duplicated merge files element\n");
		      menu_layout_node_unlink (child);
		    }
		  break;

		case MENU_LAYOUT_MERGE_ALL:
		  if (last_all != child)
		    {
		      menu_verbose ("removing duplicated merge all element\n");
		      menu_layout_node_unlink (child);
		    }
		  break;

		default:
		  g_assert_not_reached ();
		  break;
		}
	      break;

	      default:
		break;
	    }

	  child = next;
	}
    }

  if (n_all == 0 && n_menus == 0)
    {
      last_menus = menu_layout_node_new (MENU_LAYOUT_NODE_MERGE);
      menu_layout_node_merge_set_type (last_menus, "menus");
      menu_verbose ("appending missing merge menus element\n");
      menu_layout_node_append_child (node, last_menus);
    }

  if (n_all == 0 && n_files == 0)
    {
      last_files = menu_layout_node_new (MENU_LAYOUT_NODE_MERGE);
      menu_layout_node_merge_set_type (last_files, "files");
      menu_verbose ("appending missing merge files element\n");
      menu_layout_node_append_child (node, last_files);
    }

  return TRUE;
}

/* we want to a) check that we have old-new pairs and b) canonicalize
 * such that each <Move> has exactly one old-new pair
 */
static gboolean
fixup_move_node (GMarkupParseContext   *context,
                 MenuParser            *parser,
                 MenuLayoutNode        *node,
                 GError              **error)
{
  MenuLayoutNode *child;
  int             n_old;
  int             n_new;

  n_old = 0;
  n_new = 0;

  child = node->children;
  while (child != NULL)
    {
      switch (child->type)
        {
        case MENU_LAYOUT_NODE_OLD:
          if (n_new != n_old)
            {
              set_error (error, context,
                         G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                         "<Old>/<New> elements not paired properly\n");
              return FALSE;
            }

          n_old += 1;

          break;

        case MENU_LAYOUT_NODE_NEW:
          n_new += 1;

          if (n_new != n_old)
            {
              set_error (error, context,
                         G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                         "<Old>/<New> elements not paired properly\n");
              return FALSE;
            }

          break;

        default:
          g_assert_not_reached ();
          break;
        }

      child = node_next (child);
    }

  if (n_new == 0 || n_old == 0)
    {
      set_error (error, context,
                 G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                 "<Old>/<New> elements missing under <Move>\n");
      return FALSE;
    }

  g_assert (n_new == n_old);
  g_assert ((n_new + n_old) % 2 == 0);

  if (n_new > 1)
    {
      MenuLayoutNode *prev;
      MenuLayoutNode *append_after;

      /* Need to split the <Move> into multiple <Move> */

      n_old = 0;
      n_new = 0;
      prev = NULL;
      append_after = node;

      child = node->children;
      while (child != NULL)
        {
          MenuLayoutNode *next;

          next = node_next (child);

          switch (child->type)
            {
            case MENU_LAYOUT_NODE_OLD:
              n_old += 1;
              break;

            case MENU_LAYOUT_NODE_NEW:
              n_new += 1;
              break;

            default:
              g_assert_not_reached ();
              break;
            }

          if (n_old == n_new &&
              n_old > 1)
            {
              /* Move the just-completed pair */
              MenuLayoutNode *new_move;

              g_assert (prev != NULL);

              new_move = menu_layout_node_new (MENU_LAYOUT_NODE_MOVE);
              menu_verbose ("inserting new_move after append_after\n");
              menu_layout_node_insert_after (append_after, new_move);
              append_after = new_move;

              menu_layout_node_steal (prev);
              menu_layout_node_steal (child);

              menu_verbose ("appending prev to new_move\n");
              menu_layout_node_append_child (new_move, prev);
              menu_verbose ("appending child to new_move\n");
              menu_layout_node_append_child (new_move, child);

              menu_verbose ("Created new move element old = %s new = %s\n",
                            menu_layout_node_move_get_old (new_move),
                            menu_layout_node_move_get_new (new_move));

              menu_layout_node_unref (new_move);
              menu_layout_node_unref (prev);
              menu_layout_node_unref (child);

              prev = NULL;
            }
          else
            {
              prev = child;
            }

          prev = child;
          child = next;
        }
    }

  return TRUE;
}

static void
end_element_handler (GMarkupParseContext  *context,
                     const char           *element_name,
                     gpointer              user_data,
                     GError              **error)
{
  MenuParser *parser = user_data;

  g_assert (parser->stack_top != NULL);

  switch (parser->stack_top->type)
    {
    case MENU_LAYOUT_NODE_APP_DIR:
    case MENU_LAYOUT_NODE_DIRECTORY_DIR:
    case MENU_LAYOUT_NODE_NAME:
    case MENU_LAYOUT_NODE_DIRECTORY:
    case MENU_LAYOUT_NODE_FILENAME:
    case MENU_LAYOUT_NODE_CATEGORY:
    case MENU_LAYOUT_NODE_MERGE_DIR:
    case MENU_LAYOUT_NODE_LEGACY_DIR:
    case MENU_LAYOUT_NODE_OLD:
    case MENU_LAYOUT_NODE_NEW:
    case MENU_LAYOUT_NODE_MENUNAME:
      if (menu_layout_node_get_content (parser->stack_top) == NULL)
        {
          set_error (error, context,
                     G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                     "Element <%s> is required to contain text and was empty\n",
                     element_name);
          goto out;
        }
      break;

    case MENU_LAYOUT_NODE_MENU:
      if (!has_child_of_type (parser->stack_top, MENU_LAYOUT_NODE_NAME))
        {
          set_error (error, context,
                     G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                     "<Menu> elements are required to contain a <Name> element\n");
          goto out;
        }
      break;

    case MENU_LAYOUT_NODE_ROOT:
    case MENU_LAYOUT_NODE_PASSTHROUGH:
    case MENU_LAYOUT_NODE_DEFAULT_APP_DIRS:
    case MENU_LAYOUT_NODE_DEFAULT_DIRECTORY_DIRS:
    case MENU_LAYOUT_NODE_DEFAULT_MERGE_DIRS:
    case MENU_LAYOUT_NODE_ONLY_UNALLOCATED:
    case MENU_LAYOUT_NODE_NOT_ONLY_UNALLOCATED:
    case MENU_LAYOUT_NODE_INCLUDE:
    case MENU_LAYOUT_NODE_EXCLUDE:
    case MENU_LAYOUT_NODE_ALL:
    case MENU_LAYOUT_NODE_AND:
    case MENU_LAYOUT_NODE_OR:
    case MENU_LAYOUT_NODE_NOT:
    case MENU_LAYOUT_NODE_KDE_LEGACY_DIRS:
    case MENU_LAYOUT_NODE_DELETED:
    case MENU_LAYOUT_NODE_NOT_DELETED:
    case MENU_LAYOUT_NODE_SEPARATOR:
    case MENU_LAYOUT_NODE_MERGE:
    case MENU_LAYOUT_NODE_MERGE_FILE:
      break;

    case MENU_LAYOUT_NODE_LAYOUT:
    case MENU_LAYOUT_NODE_DEFAULT_LAYOUT:
      if (!fixup_layout_node (context, parser, parser->stack_top, error))
        goto out;
      break;

    case MENU_LAYOUT_NODE_MOVE:
      if (!fixup_move_node (context, parser, parser->stack_top, error))
        goto out;
      break;
    }

 out:
  parser->stack_top = parser->stack_top->parent;
}

static gboolean
all_whitespace (const char *text,
                int         text_len)
{
  const char *p;
  const char *end;

  p = text;
  end = text + text_len;

  while (p != end)
    {
      if (!g_ascii_isspace (*p))
        return FALSE;

      p = g_utf8_next_char (p);
    }

  return TRUE;
}

static void
text_handler (GMarkupParseContext  *context,
              const char           *text,
              gsize                 text_len,
              gpointer              user_data,
              GError              **error)
{
  MenuParser *parser = user_data;

  switch (parser->stack_top->type)
    {
    case MENU_LAYOUT_NODE_APP_DIR:
    case MENU_LAYOUT_NODE_DIRECTORY_DIR:
    case MENU_LAYOUT_NODE_NAME:
    case MENU_LAYOUT_NODE_DIRECTORY:
    case MENU_LAYOUT_NODE_FILENAME:
    case MENU_LAYOUT_NODE_CATEGORY:
    case MENU_LAYOUT_NODE_MERGE_FILE:
    case MENU_LAYOUT_NODE_MERGE_DIR:
    case MENU_LAYOUT_NODE_LEGACY_DIR:
    case MENU_LAYOUT_NODE_OLD:
    case MENU_LAYOUT_NODE_NEW:
    case MENU_LAYOUT_NODE_MENUNAME:
      g_assert (menu_layout_node_get_content (parser->stack_top) == NULL);

      menu_layout_node_set_content (parser->stack_top, text);
      break;

    case MENU_LAYOUT_NODE_ROOT:
    case MENU_LAYOUT_NODE_PASSTHROUGH:
    case MENU_LAYOUT_NODE_MENU:
    case MENU_LAYOUT_NODE_DEFAULT_APP_DIRS:
    case MENU_LAYOUT_NODE_DEFAULT_DIRECTORY_DIRS:
    case MENU_LAYOUT_NODE_DEFAULT_MERGE_DIRS:
    case MENU_LAYOUT_NODE_ONLY_UNALLOCATED:
    case MENU_LAYOUT_NODE_NOT_ONLY_UNALLOCATED:
    case MENU_LAYOUT_NODE_INCLUDE:
    case MENU_LAYOUT_NODE_EXCLUDE:
    case MENU_LAYOUT_NODE_ALL:
    case MENU_LAYOUT_NODE_AND:
    case MENU_LAYOUT_NODE_OR:
    case MENU_LAYOUT_NODE_NOT:
    case MENU_LAYOUT_NODE_KDE_LEGACY_DIRS:
    case MENU_LAYOUT_NODE_MOVE:
    case MENU_LAYOUT_NODE_DELETED:
    case MENU_LAYOUT_NODE_NOT_DELETED:
    case MENU_LAYOUT_NODE_LAYOUT:
    case MENU_LAYOUT_NODE_DEFAULT_LAYOUT:
    case MENU_LAYOUT_NODE_SEPARATOR:
    case MENU_LAYOUT_NODE_MERGE:
      if (!all_whitespace (text, text_len))
        {
          set_error (error, context,
                     G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                     "No text is allowed inside element <%s>",
                     g_markup_parse_context_get_element (context));
        }
      break;
    }

  add_context_to_error (error, context);
}

static void
passthrough_handler (GMarkupParseContext  *context,
                     const char           *passthrough_text,
                     gsize                 text_len,
                     gpointer              user_data,
                     GError              **error)
{
  MenuParser *parser = user_data;
  MenuLayoutNode *node;

  /* don't push passthrough on the stack, it's not an element */

  node = menu_layout_node_new (MENU_LAYOUT_NODE_PASSTHROUGH);
  menu_layout_node_set_content (node, passthrough_text);

  menu_layout_node_append_child (parser->stack_top, node);
  menu_layout_node_unref (node);

  add_context_to_error (error, context);
}

static void
menu_parser_init (MenuParser *parser)
{
  parser->root = menu_layout_node_new (MENU_LAYOUT_NODE_ROOT);
  parser->stack_top = parser->root;
}

static void
menu_parser_free (MenuParser *parser)
{
  if (parser->root)
    menu_layout_node_unref (parser->root);
}

MenuLayoutNode *
menu_layout_load (const char  *filename,
                  const char  *non_prefixed_basename,
                  GError     **err)
{
  GMarkupParseContext *context;
  MenuLayoutNodeRoot  *root;
  MenuLayoutNode      *retval;
  MenuParser           parser;
  GError              *error;
  GString             *str;
  char                *text;
  char                *s;
  gsize                length;

  text = NULL;
  length = 0;
  retval = NULL;
  context = NULL;

  menu_verbose ("Loading \"%s\" from disk\n", filename);

  if (!g_file_get_contents (filename,
                            &text,
                            &length,
                            err))
    {
      menu_verbose ("Failed to load \"%s\"\n",
                    filename);
      return NULL;
    }

  g_assert (text != NULL);

  menu_parser_init (&parser);

  root = (MenuLayoutNodeRoot *) parser.root;

  root->basedir = g_path_get_dirname (filename);
  menu_verbose ("Set basedir \"%s\"\n", root->basedir);

  if (non_prefixed_basename)
    s = g_strdup (non_prefixed_basename);
  else
    s = g_path_get_basename (filename);
  str = g_string_new (s);
  if (g_str_has_suffix (str->str, ".menu"))
    g_string_truncate (str, str->len - strlen (".menu"));

  root->name = str->str;
  menu_verbose ("Set menu name \"%s\"\n", root->name);

  g_string_free (str, FALSE);
  g_free (s);

  context = g_markup_parse_context_new (&menu_funcs, 0, &parser, NULL);

  error = NULL;
  if (!g_markup_parse_context_parse (context,
                                     text,
                                     length,
                                     &error))
    goto out;

  error = NULL;
  g_markup_parse_context_end_parse (context, &error);

 out:
  if (context)
    g_markup_parse_context_free (context);
  g_free (text);

  if (error)
    {
      menu_verbose ("Error \"%s\" loading \"%s\"\n",
                    error->message, filename);
      g_propagate_error (err, error);
    }
  else if (has_child_of_type (parser.root, MENU_LAYOUT_NODE_MENU))
    {
      menu_verbose ("File loaded OK\n");
      retval = parser.root;
      parser.root = NULL;
    }
  else
    {
      menu_verbose ("Did not have a root element in file\n");
      g_set_error (err, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                   "Menu file %s did not contain a root <Menu> element",
                   filename);
    }

  menu_parser_free (&parser);

  return retval;
}
