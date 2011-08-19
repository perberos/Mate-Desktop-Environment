/* Random utility functions for menu code */

/*
 * Copyright (C) 2003 Red Hat, Inc.
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

#include "menu-util.h"

#include <stdio.h>
#include <stdarg.h>


#ifdef G_ENABLE_DEBUG

static gboolean verbose = FALSE;
static gboolean initted = FALSE;

static inline gboolean
menu_verbose_enabled (void)
{
  if (!initted)
    {
      verbose = g_getenv ("MENU_VERBOSE") != NULL;
      initted = TRUE;
    }

  return verbose;
}

static int
utf8_fputs (const char *str,
            FILE       *f)
{
  char *l;
  int ret;

  l = g_locale_from_utf8 (str, -1, NULL, NULL, NULL);

  if (l == NULL)
    ret = fputs (str, f); /* just print it anyway, better than nothing */
  else
    ret = fputs (l, f);

  g_free (l);

  return ret;
}

void
menu_verbose (const char *format,
              ...)
{
  va_list args;
  char *str;

  if (!menu_verbose_enabled ())
    return;

  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  utf8_fputs (str, stderr);
  fflush (stderr);

  g_free (str);
}

static void append_to_string (MenuLayoutNode *node,
                              gboolean        onelevel,
                              int             depth,
                              GString        *str);

static void
append_spaces (GString *str,
               int      depth)
{
  while (depth > 0)
    {
      g_string_append_c (str, ' ');
      --depth;
    }
}

static void
append_children (MenuLayoutNode *node,
                 int             depth,
                 GString        *str)
{
  MenuLayoutNode *iter;

  iter = menu_layout_node_get_children (node);
  while (iter != NULL)
    {
      append_to_string (iter, FALSE, depth, str);

      iter = menu_layout_node_get_next (iter);
    }
}

static void
append_simple_with_attr (MenuLayoutNode *node,
                         int             depth,
                         const char     *node_name,
                         const char     *attr_name,
                         const char     *attr_value,
                         GString        *str)
{
  const char *content;

  append_spaces (str, depth);

  if ((content = menu_layout_node_get_content (node)))
    {
      char *escaped;

      escaped = g_markup_escape_text (content, -1);

      if (attr_name && attr_value)
        {
          char *attr_escaped;

          attr_escaped = g_markup_escape_text (attr_value, -1);

          g_string_append_printf (str, "<%s %s=\"%s\">%s</%s>\n",
                                  node_name, attr_name,
                                  attr_escaped, escaped, node_name);

          g_free (attr_escaped);
        }
      else
        {
          g_string_append_printf (str, "<%s>%s</%s>\n",
                                  node_name, escaped, node_name);
        }

      g_free (escaped);
    }
  else
    {
      if (attr_name && attr_value)
        {
          char *attr_escaped;

          attr_escaped = g_markup_escape_text (attr_value, -1);

          g_string_append_printf (str, "<%s %s=\"%s\"/>\n",
                                  node_name, attr_name,
                                  attr_escaped);

          g_free (attr_escaped);
        }
      else
        {
          g_string_append_printf (str, "<%s/>\n", node_name);
        }
    }
}

static void
append_layout (MenuLayoutNode   *node,
	       int               depth,
	       const char       *node_name,
	       MenuLayoutValues *layout_values,
	       GString          *str)
{
  const char *content;

  append_spaces (str, depth);

  if ((content = menu_layout_node_get_content (node)))
    {
      char *escaped;

      escaped = g_markup_escape_text (content, -1);

      g_string_append_printf (str,
			      "<%s show_empty=\"%s\" inline=\"%s\" inline_header=\"%s\""
			         " inline_alias=\"%s\" inline_limit=\"%d\">%s</%s>\n",
			      node_name,
			      layout_values->show_empty    ? "true" : "false",
			      layout_values->inline_menus  ? "true" : "false",
			      layout_values->inline_header ? "true" : "false",
			      layout_values->inline_alias  ? "true" : "false",
			      layout_values->inline_limit,
			      escaped,
			      node_name);

      g_free (escaped);
    }
  else
    {
      g_string_append_printf (str,
			      "<%s show_empty=\"%s\" inline=\"%s\" inline_header=\"%s\""
			         " inline_alias=\"%s\" inline_limit=\"%d\"/>\n",
			      node_name,
			      layout_values->show_empty    ? "true" : "false",
			      layout_values->inline_menus  ? "true" : "false",
			      layout_values->inline_header ? "true" : "false",
			      layout_values->inline_alias  ? "true" : "false",
			      layout_values->inline_limit);
    }
}

static void
append_merge (MenuLayoutNode      *node,
	      int                  depth,
	      const char          *node_name,
	      MenuLayoutMergeType  merge_type,
	      GString             *str)
{
  const char *merge_type_str;

  merge_type_str = NULL;

  switch (merge_type)
    {
    case MENU_LAYOUT_MERGE_NONE:
      merge_type_str = "none";
      break;

    case MENU_LAYOUT_MERGE_MENUS:
      merge_type_str = "menus";
      break;
      
    case MENU_LAYOUT_MERGE_FILES:
      merge_type_str = "files";
      break;
      
    case MENU_LAYOUT_MERGE_ALL:
      merge_type_str = "all";
      break;

    default:
      g_assert_not_reached ();
      break;
    }
      
  append_simple_with_attr (node, depth, node_name, "type", merge_type_str, str);
}

static void
append_simple (MenuLayoutNode *node,
               int             depth,
               const char     *node_name,
               GString        *str)
{
  append_simple_with_attr (node, depth, node_name, NULL, NULL, str);
}

static void
append_start (MenuLayoutNode *node,
              int             depth,
              const char     *node_name,
              GString        *str)
{
  append_spaces (str, depth);

  g_string_append_printf (str, "<%s>\n", node_name);
}

static void
append_end (MenuLayoutNode *node,
            int             depth,
            const char     *node_name,
            GString        *str)
{
  append_spaces (str, depth);

  g_string_append_printf (str, "</%s>\n", node_name);
}

static void
append_container (MenuLayoutNode *node,
                  gboolean        onelevel,
                  int             depth,
                  const char     *node_name,
                  GString        *str)
{
  append_start (node, depth, node_name, str);
  if (!onelevel)
    {
      append_children (node, depth + 2, str);
      append_end (node, depth, node_name, str);
    }
}

static void
append_to_string (MenuLayoutNode *node,
                  gboolean        onelevel,
                  int             depth,
                  GString        *str)
{
  MenuLayoutValues layout_values;

  switch (menu_layout_node_get_type (node))
    {
    case MENU_LAYOUT_NODE_ROOT:
      if (!onelevel)
        append_children (node, depth - 1, str); /* -1 to ignore depth of root */
      else
        append_start (node, depth - 1, "Root", str);
      break;

    case MENU_LAYOUT_NODE_PASSTHROUGH:
      g_string_append (str,
                       menu_layout_node_get_content (node));
      g_string_append_c (str, '\n');
      break;

    case MENU_LAYOUT_NODE_MENU:
      append_container (node, onelevel, depth, "Menu", str);
      break;

    case MENU_LAYOUT_NODE_APP_DIR:
      append_simple (node, depth, "AppDir", str);
      break;

    case MENU_LAYOUT_NODE_DEFAULT_APP_DIRS:
      append_simple (node, depth, "DefaultAppDirs", str);
      break;

    case MENU_LAYOUT_NODE_DIRECTORY_DIR:
      append_simple (node, depth, "DirectoryDir", str);
      break;

    case MENU_LAYOUT_NODE_DEFAULT_DIRECTORY_DIRS:
      append_simple (node, depth, "DefaultDirectoryDirs", str);
      break;

    case MENU_LAYOUT_NODE_DEFAULT_MERGE_DIRS:
      append_simple (node, depth, "DefaultMergeDirs", str);
      break;

    case MENU_LAYOUT_NODE_NAME:
      append_simple (node, depth, "Name", str);
      break;

    case MENU_LAYOUT_NODE_DIRECTORY:
      append_simple (node, depth, "Directory", str);
      break;

    case MENU_LAYOUT_NODE_ONLY_UNALLOCATED:
      append_simple (node, depth, "OnlyUnallocated", str);
      break;

    case MENU_LAYOUT_NODE_NOT_ONLY_UNALLOCATED:
      append_simple (node, depth, "NotOnlyUnallocated", str);
      break;

    case MENU_LAYOUT_NODE_INCLUDE:
      append_container (node, onelevel, depth, "Include", str);
      break;

    case MENU_LAYOUT_NODE_EXCLUDE:
      append_container (node, onelevel, depth, "Exclude", str);
      break;

    case MENU_LAYOUT_NODE_FILENAME:
      append_simple (node, depth, "Filename", str);
      break;

    case MENU_LAYOUT_NODE_CATEGORY:
      append_simple (node, depth, "Category", str);
      break;

    case MENU_LAYOUT_NODE_ALL:
      append_simple (node, depth, "All", str);
      break;

    case MENU_LAYOUT_NODE_AND:
      append_container (node, onelevel, depth, "And", str);
      break;

    case MENU_LAYOUT_NODE_OR:
      append_container (node, onelevel, depth, "Or", str);
      break;

    case MENU_LAYOUT_NODE_NOT:
      append_container (node, onelevel, depth, "Not", str);
      break;

    case MENU_LAYOUT_NODE_MERGE_FILE:
      {
	MenuMergeFileType type;

	type = menu_layout_node_merge_file_get_type (node);

	append_simple_with_attr (node, depth, "MergeFile",
				 "type", type == MENU_MERGE_FILE_TYPE_PARENT ? "parent" : "path",
				 str);
	break;
      }

    case MENU_LAYOUT_NODE_MERGE_DIR:
      append_simple (node, depth, "MergeDir", str);
      break;

    case MENU_LAYOUT_NODE_LEGACY_DIR:
      append_simple_with_attr (node, depth, "LegacyDir",
                               "prefix", menu_layout_node_legacy_dir_get_prefix (node),
                               str);
      break;

    case MENU_LAYOUT_NODE_KDE_LEGACY_DIRS:
      append_simple (node, depth, "KDELegacyDirs", str);
      break;

    case MENU_LAYOUT_NODE_MOVE:
      append_container (node, onelevel, depth, "Move", str);
      break;

    case MENU_LAYOUT_NODE_OLD:
      append_simple (node, depth, "Old", str);
      break;

    case MENU_LAYOUT_NODE_NEW:
      append_simple (node, depth, "New", str);
      break;

    case MENU_LAYOUT_NODE_DELETED:
      append_simple (node, depth, "Deleted", str);
      break;

    case MENU_LAYOUT_NODE_NOT_DELETED:
      append_simple (node, depth, "NotDeleted", str);
      break;

    case MENU_LAYOUT_NODE_LAYOUT:
      append_container (node, onelevel, depth, "Layout", str);
      break;

    case MENU_LAYOUT_NODE_DEFAULT_LAYOUT:
      menu_layout_node_default_layout_get_values (node, &layout_values);
      append_layout (node, depth, "DefaultLayout", &layout_values, str);
      break;

    case MENU_LAYOUT_NODE_MENUNAME:
      menu_layout_node_menuname_get_values (node, &layout_values);
      append_layout (node, depth, "MenuName", &layout_values, str);
      break;

    case MENU_LAYOUT_NODE_SEPARATOR:
      append_simple (node, depth, "Name", str);
      break;

    case MENU_LAYOUT_NODE_MERGE:
      append_merge (node,
		    depth,
		    "Merge",
		    menu_layout_node_merge_get_type (node),
		    str);
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

void
menu_debug_print_layout (MenuLayoutNode *node,
                         gboolean        onelevel)
{
  if (menu_verbose_enabled ())
    {
      GString *str;

      str = g_string_new (NULL);
      append_to_string (node, onelevel, 0, str);

      utf8_fputs (str->str, stderr);
      fflush (stderr);

      g_string_free (str, TRUE);
    }
}

#endif /* G_ENABLE_DEBUG */
