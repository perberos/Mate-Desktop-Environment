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

#include "desktop-entries.h"

#include <string.h>

#include "menu-util.h"

#define DESKTOP_ENTRY_GROUP     "Desktop Entry"
#define KDE_DESKTOP_ENTRY_GROUP "KDE Desktop Entry"

enum
{
  DESKTOP_ENTRY_NO_DISPLAY     = 1 << 0,
  DESKTOP_ENTRY_HIDDEN         = 1 << 1,
  DESKTOP_ENTRY_SHOW_IN_MATE  = 1 << 2,
  DESKTOP_ENTRY_TRYEXEC_FAILED = 1 << 3
};

struct DesktopEntry
{
  char *path;
  char *basename;

  GQuark *categories;

  char     *name;
  char     *generic_name;
  char     *full_name;
  char     *comment;
  char     *icon;
  char     *exec;
  gboolean terminal;

  guint type : 2;
  guint flags : 4;
  guint refcount : 24;
};

struct DesktopEntrySet
{
  int         refcount;
  GHashTable *hash;
};

/*
 * Desktop entries
 */

static guint
get_flags_from_key_file (DesktopEntry *entry,
                         GKeyFile     *key_file,
                         const char   *desktop_entry_group)
{
  GError    *error;
  char     **strv;
  gboolean   no_display;
  gboolean   hidden;
  gboolean   show_in_mate;
  gboolean   tryexec_failed;
  char      *tryexec;
  guint      flags;
  int        i;

  error = NULL;
  no_display = g_key_file_get_boolean (key_file,
                                       desktop_entry_group,
                                       "NoDisplay",
                                       &error);
  if (error)
    {
      no_display = FALSE;
      g_error_free (error);
    }

  error = NULL;
  hidden = g_key_file_get_boolean (key_file,
                                   desktop_entry_group,
                                   "Hidden",
                                   &error);
  if (error)
    {
      hidden = FALSE;
      g_error_free (error);
    }

  show_in_mate = TRUE;
  strv = g_key_file_get_string_list (key_file,
                                     desktop_entry_group,
                                     "OnlyShowIn",
                                     NULL,
                                     NULL);
  if (strv)
    {
      show_in_mate = FALSE;
      for (i = 0; strv[i]; i++)
        {
          if (!strcmp (strv[i], "MATE"))
            {
              show_in_mate = TRUE;
              break;
            }
        }
    }
  else
    {
      strv = g_key_file_get_string_list (key_file,
                                         desktop_entry_group,
                                         "NotShowIn",
                                         NULL,
                                         NULL);
      if (strv)
        {
          show_in_mate = TRUE;
          for (i = 0; strv[i]; i++)
            {
              if (!strcmp (strv[i], "MATE"))
                {
                  show_in_mate = FALSE;
                }
            }
        }
    }
  g_strfreev (strv);

  tryexec_failed = FALSE;
  tryexec = g_key_file_get_string (key_file,
                                   desktop_entry_group,
                                   "TryExec",
                                   NULL);
  if (tryexec)
    {
      char *path;

      path = g_find_program_in_path (g_strstrip (tryexec));

      tryexec_failed = (path == NULL);

      g_free (path);
      g_free (tryexec);
    }

  flags = 0;
  if (no_display)
    flags |= DESKTOP_ENTRY_NO_DISPLAY;
  if (hidden)
    flags |= DESKTOP_ENTRY_HIDDEN;
  if (show_in_mate)
    flags |= DESKTOP_ENTRY_SHOW_IN_MATE;
  if (tryexec_failed)
    flags |= DESKTOP_ENTRY_TRYEXEC_FAILED;

  return flags;
}

static GQuark *
get_categories_from_key_file (DesktopEntry *entry,
                              GKeyFile     *key_file,
                              const char   *desktop_entry_group)
{
  GQuark  *retval;
  char   **strv;
  gsize    len;
  int      i;

  strv = g_key_file_get_string_list (key_file,
                                     desktop_entry_group,
                                     "Categories",
                                     &len,
                                     NULL);
  if (!strv)
    return NULL;

  retval = g_new0 (GQuark, len + 1);

  for (i = 0; strv[i]; i++)
    retval[i] = g_quark_from_string (strv[i]);

  g_strfreev (strv);

  return retval;
}

static DesktopEntry *
desktop_entry_load (DesktopEntry *entry)
{
  DesktopEntry *retval = NULL;
  GKeyFile     *key_file;
  GError       *error;
  const char   *desktop_entry_group;
  char         *name_str;
  char         *type_str;

  key_file = g_key_file_new ();

  error = NULL;
  if (!g_key_file_load_from_file (key_file, entry->path, 0, &error))
    {
      menu_verbose ("Failed to load \"%s\": %s\n",
                    entry->path, error->message);
      g_error_free (error);
      goto out;
    }

  if (g_key_file_has_group (key_file, DESKTOP_ENTRY_GROUP))
    {
      desktop_entry_group = DESKTOP_ENTRY_GROUP;
    }
  else
    {
      menu_verbose ("\"%s\" contains no \"" DESKTOP_ENTRY_GROUP "\" group\n",
                    entry->path);

      if (g_key_file_has_group (key_file, KDE_DESKTOP_ENTRY_GROUP))
        {
          desktop_entry_group = KDE_DESKTOP_ENTRY_GROUP;
          menu_verbose ("\"%s\" contains deprecated \"" KDE_DESKTOP_ENTRY_GROUP "\" group\n",
                        entry->path);
        }
      else
        {
          goto out;
        }
    }

  if (!g_key_file_has_key (key_file, desktop_entry_group, "Name", NULL))
    {
      menu_verbose ("\"%s\" contains no \"Name\" key\n", entry->path);
      goto out;
    }

  name_str = g_key_file_get_locale_string (key_file, desktop_entry_group, "Name", NULL, NULL);
  if (!name_str)
    {
      menu_verbose ("\"%s\" contains an invalid \"Name\" key\n", entry->path);
      goto out;
    }

  g_free (name_str);

  type_str = g_key_file_get_string (key_file, desktop_entry_group, "Type", NULL);
  if (!type_str)
    {
      menu_verbose ("\"%s\" contains no \"Type\" key\n", entry->path);
      goto out;
    }

  if ((entry->type == DESKTOP_ENTRY_DESKTOP && strcmp (type_str, "Application") != 0) ||
      (entry->type == DESKTOP_ENTRY_DIRECTORY && strcmp (type_str, "Directory") != 0))
    {
      menu_verbose ("\"%s\" does not contain the correct \"Type\" value\n", entry->path);
      g_free (type_str);
      goto out;
    }

  g_free (type_str);

  if (entry->type == DESKTOP_ENTRY_DESKTOP &&
      !g_key_file_has_key (key_file, desktop_entry_group, "Exec", NULL))
    {
      menu_verbose ("\"%s\" does not contain an \"Exec\" key\n", entry->path);
      goto out;
    }

  retval = entry;

#define GET_LOCALE_STRING(n) g_key_file_get_locale_string (key_file, desktop_entry_group, (n), NULL, NULL)

  retval->name         = GET_LOCALE_STRING ("Name");
  retval->generic_name = GET_LOCALE_STRING ("GenericName");
  retval->full_name    = GET_LOCALE_STRING ("X-MATE-FullName");
  retval->comment      = GET_LOCALE_STRING ("Comment");
  retval->icon         = GET_LOCALE_STRING ("Icon");
  retval->flags        = get_flags_from_key_file (retval, key_file, desktop_entry_group);
  retval->categories   = get_categories_from_key_file (retval, key_file, desktop_entry_group);

  if (entry->type == DESKTOP_ENTRY_DESKTOP)
    {
      retval->exec = g_key_file_get_string (key_file, desktop_entry_group, "Exec", NULL);
      retval->terminal = g_key_file_get_boolean (key_file, desktop_entry_group, "Terminal", NULL);
    }
  
#undef GET_LOCALE_STRING

  menu_verbose ("Desktop entry \"%s\" (%s, %s, %s, %s, %s) flags: NoDisplay=%s, Hidden=%s, ShowInMATE=%s, TryExecFailed=%s\n",
                retval->basename,
                retval->name,
                retval->generic_name ? retval->generic_name : "(null)",
                retval->full_name ? retval->full_name : "(null)",
                retval->comment ? retval->comment : "(null)",
                retval->icon ? retval->icon : "(null)",
                retval->flags & DESKTOP_ENTRY_NO_DISPLAY     ? "(true)" : "(false)",
                retval->flags & DESKTOP_ENTRY_HIDDEN         ? "(true)" : "(false)",
                retval->flags & DESKTOP_ENTRY_SHOW_IN_MATE  ? "(true)" : "(false)",
                retval->flags & DESKTOP_ENTRY_TRYEXEC_FAILED ? "(true)" : "(false)");

 out:
  g_key_file_free (key_file);

  if (!retval)
    desktop_entry_unref (entry);

  return retval;
}

DesktopEntry *
desktop_entry_new (const char *path)
{
  DesktopEntryType  type;
  DesktopEntry     *retval;

  menu_verbose ("Loading desktop entry \"%s\"\n", path);

  if (g_str_has_suffix (path, ".desktop"))
    {
      type = DESKTOP_ENTRY_DESKTOP;
    }
  else if (g_str_has_suffix (path, ".directory"))
    {
      type = DESKTOP_ENTRY_DIRECTORY;
    }
  else
    {
      menu_verbose ("Unknown desktop entry suffix in \"%s\"\n",
                    path);
      return NULL;
    }

  retval = g_new0 (DesktopEntry, 1);

  retval->refcount = 1;
  retval->type     = type;
  retval->basename = g_path_get_basename (path);
  retval->path     = g_strdup (path);

  return desktop_entry_load (retval);
}

DesktopEntry *
desktop_entry_reload (DesktopEntry *entry)
{
  g_return_val_if_fail (entry != NULL, NULL);

  menu_verbose ("Re-loading desktop entry \"%s\"\n", entry->path);

  g_free (entry->categories);
  entry->categories = NULL;

  g_free (entry->name);
  entry->name = NULL;

  g_free (entry->generic_name);
  entry->generic_name = NULL;

  g_free (entry->full_name);
  entry->full_name = NULL;

  g_free (entry->comment);
  entry->comment = NULL;

  g_free (entry->icon);
  entry->icon = NULL;

  g_free (entry->exec);
  entry->exec = NULL;

  entry->terminal = 0;
  entry->flags = 0;

  return desktop_entry_load (entry);
}

DesktopEntry *
desktop_entry_ref (DesktopEntry *entry)
{
  g_return_val_if_fail (entry != NULL, NULL);
  g_return_val_if_fail (entry->refcount > 0, NULL);

  entry->refcount += 1;

  return entry;
}

DesktopEntry *
desktop_entry_copy (DesktopEntry *entry)
{
  DesktopEntry *retval;
  int           i;

  menu_verbose ("Copying desktop entry \"%s\"\n",
                entry->basename);

  retval = g_new0 (DesktopEntry, 1);

  retval->refcount     = 1;
  retval->type         = entry->type;
  retval->basename     = g_strdup (entry->basename);
  retval->path         = g_strdup (entry->path);
  retval->name         = g_strdup (entry->name);
  retval->generic_name = g_strdup (entry->generic_name);
  retval->full_name    = g_strdup (entry->full_name);
  retval->comment      = g_strdup (entry->comment);
  retval->icon         = g_strdup (entry->icon);
  retval->exec         = g_strdup (entry->exec);
  retval->terminal     = entry->terminal;
  retval->flags        = entry->flags;

  i = 0;
  if (entry->categories != NULL)
    {
      for (; entry->categories[i]; i++);
    }

  retval->categories = g_new0 (GQuark, i + 1);

  i = 0;
  if (entry->categories != NULL)
    {
      for (; entry->categories[i]; i++)
        retval->categories[i] = entry->categories[i];
    }

  return retval;
}

void
desktop_entry_unref (DesktopEntry *entry)
{
  g_return_if_fail (entry != NULL);
  g_return_if_fail (entry->refcount > 0);

  entry->refcount -= 1;
  if (entry->refcount == 0)
    {
      g_free (entry->categories);
      entry->categories = NULL;

      g_free (entry->name);
      entry->name = NULL;

      g_free (entry->generic_name);
      entry->generic_name = NULL;

      g_free (entry->full_name);
      entry->full_name = NULL;

      g_free (entry->comment);
      entry->comment = NULL;

      g_free (entry->icon);
      entry->icon = NULL;

      g_free (entry->exec);
      entry->exec = NULL;

      g_free (entry->basename);
      entry->basename = NULL;

      g_free (entry->path);
      entry->path = NULL;

      g_free (entry);
    }
}

DesktopEntryType
desktop_entry_get_type (DesktopEntry *entry)
{
  return entry->type;
}

const char *
desktop_entry_get_path (DesktopEntry *entry)
{
  return entry->path;
}

const char *
desktop_entry_get_basename (DesktopEntry *entry)
{
  return entry->basename;
}

const char *
desktop_entry_get_name (DesktopEntry *entry)
{
  return entry->name;
}

const char *
desktop_entry_get_generic_name (DesktopEntry *entry)
{
  return entry->generic_name;
}

const char *
desktop_entry_get_full_name (DesktopEntry *entry)
{
  return entry->full_name;
}

const char *
desktop_entry_get_comment (DesktopEntry *entry)
{
  return entry->comment;
}

const char *
desktop_entry_get_icon (DesktopEntry *entry)
{
  return entry->icon;
}

const char *
desktop_entry_get_exec (DesktopEntry *entry)
{
  return entry->exec;
}

gboolean
desktop_entry_get_launch_in_terminal (DesktopEntry *entry)
{
  return entry->terminal;
}

gboolean
desktop_entry_get_hidden (DesktopEntry *entry)
{
  return (entry->flags & DESKTOP_ENTRY_HIDDEN) != 0;
}

gboolean
desktop_entry_get_no_display (DesktopEntry *entry)
{
  return (entry->flags & DESKTOP_ENTRY_NO_DISPLAY) != 0;
}

gboolean
desktop_entry_get_show_in_mate (DesktopEntry *entry)
{
  return (entry->flags & DESKTOP_ENTRY_SHOW_IN_MATE) != 0;
}

gboolean
desktop_entry_get_tryexec_failed (DesktopEntry *entry)
{
  return (entry->flags & DESKTOP_ENTRY_TRYEXEC_FAILED) != 0;
}

gboolean
desktop_entry_has_categories (DesktopEntry *entry)
{
  return (entry->categories != NULL && entry->categories[0] != 0);
}

gboolean
desktop_entry_has_category (DesktopEntry *entry,
                            const char   *category)
{
  GQuark quark;
  int    i;

  if (entry->categories == NULL)
    return FALSE;

  if (!(quark = g_quark_try_string (category)))
    return FALSE;

  for (i = 0; entry->categories[i]; i++)
    {
      if (quark == entry->categories[i])
        return TRUE;
    }

  return FALSE;
}

void
desktop_entry_add_legacy_category (DesktopEntry *entry)
{
  GQuark *categories;
  int     i;

  menu_verbose ("Adding Legacy category to \"%s\"\n",
                entry->basename);

  i = 0;
  if (entry->categories != NULL)
    {
      for (; entry->categories[i]; i++);
    }

  categories = g_new0 (GQuark, i + 2);

  i = 0;
  if (entry->categories != NULL)
    {
      for (; entry->categories[i]; i++)
        categories[i] = entry->categories[i];
    }

  categories[i] = g_quark_from_string ("Legacy");

  g_free (entry->categories);
  entry->categories = categories;
}

/*
 * Entry sets
 */

DesktopEntrySet *
desktop_entry_set_new (void)
{
  DesktopEntrySet *set;

  set = g_new0 (DesktopEntrySet, 1);
  set->refcount = 1;

  menu_verbose (" New entry set %p\n", set);

  return set;
}

DesktopEntrySet *
desktop_entry_set_ref (DesktopEntrySet *set)
{
  g_return_val_if_fail (set != NULL, NULL);
  g_return_val_if_fail (set->refcount > 0, NULL);

  set->refcount += 1;

  return set;
}

void
desktop_entry_set_unref (DesktopEntrySet *set)
{
  g_return_if_fail (set != NULL);
  g_return_if_fail (set->refcount > 0);

  set->refcount -= 1;
  if (set->refcount == 0)
    {
      menu_verbose (" Deleting entry set %p\n", set);

      if (set->hash)
        g_hash_table_destroy (set->hash);
      set->hash = NULL;

      g_free (set);
    }
}

void
desktop_entry_set_add_entry (DesktopEntrySet *set,
                             DesktopEntry    *entry,
                             const char      *file_id)
{
  menu_verbose (" Adding to set %p entry %s\n", set, file_id);

  if (set->hash == NULL)
    {
      set->hash = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         (GDestroyNotify) desktop_entry_unref);
    }

  g_hash_table_replace (set->hash,
                        g_strdup (file_id),
                        desktop_entry_ref (entry));
}

DesktopEntry *
desktop_entry_set_lookup (DesktopEntrySet *set,
                          const char      *file_id)
{
  if (set->hash == NULL)
    return NULL;

  return g_hash_table_lookup (set->hash, file_id);
}

typedef struct
{
  DesktopEntrySetForeachFunc func;
  gpointer                   user_data;
} EntryHashForeachData;

static void
entry_hash_foreach (const char           *file_id,
                    DesktopEntry         *entry,
                    EntryHashForeachData *fd)
{
  fd->func (file_id, entry, fd->user_data);
}

void
desktop_entry_set_foreach (DesktopEntrySet            *set,
                           DesktopEntrySetForeachFunc  func,
                           gpointer                    user_data)
{
  g_return_if_fail (set != NULL);
  g_return_if_fail (func != NULL);

  if (set->hash != NULL)
    {
      EntryHashForeachData fd;

      fd.func      = func;
      fd.user_data = user_data;

      g_hash_table_foreach (set->hash,
                            (GHFunc) entry_hash_foreach,
                            &fd);
    }
}

static void
desktop_entry_set_clear (DesktopEntrySet *set)
{
  menu_verbose (" Clearing set %p\n", set);

  if (set->hash != NULL)
    {
      g_hash_table_destroy (set->hash);
      set->hash = NULL;
    }
}

int
desktop_entry_set_get_count (DesktopEntrySet *set)
{
  if (set->hash == NULL)
    return 0;

  return g_hash_table_size (set->hash);
}

static void
union_foreach (const char      *file_id,
               DesktopEntry    *entry,
               DesktopEntrySet *set)
{
  /* we are iterating over "with" adding anything not
   * already in "set". We unconditionally overwrite
   * the stuff in "set" because we can assume
   * two entries with the same name are equivalent.
   */
  desktop_entry_set_add_entry (set, entry, file_id);
}

void
desktop_entry_set_union (DesktopEntrySet *set,
                         DesktopEntrySet *with)
{
  menu_verbose (" Union of %p and %p\n", set, with);

  if (desktop_entry_set_get_count (with) == 0)
    return; /* A fast simple case */

  g_hash_table_foreach (with->hash,
                        (GHFunc) union_foreach,
                        set);
}

typedef struct
{
  DesktopEntrySet *set;
  DesktopEntrySet *with;
} IntersectData;

static gboolean
intersect_foreach_remove (const char    *file_id,
                          DesktopEntry  *entry,
                          IntersectData *id)
{
  /* Remove everything in "set" which is not in "with" */

  if (g_hash_table_lookup (id->with->hash, file_id) != NULL)
    return FALSE;

  menu_verbose (" Removing from %p entry %s\n", id->set, file_id);

  return TRUE; /* return TRUE to remove */
}

void
desktop_entry_set_intersection (DesktopEntrySet *set,
                                DesktopEntrySet *with)
{
  IntersectData id;

  menu_verbose (" Intersection of %p and %p\n", set, with);

  if (desktop_entry_set_get_count (set) == 0 ||
      desktop_entry_set_get_count (with) == 0)
    {
      /* A fast simple case */
      desktop_entry_set_clear (set);
      return;
    }

  id.set  = set;
  id.with = with;

  g_hash_table_foreach_remove (set->hash,
                               (GHRFunc) intersect_foreach_remove,
                               &id);
}

typedef struct
{
  DesktopEntrySet *set;
  DesktopEntrySet *other;
} SubtractData;

static gboolean
subtract_foreach_remove (const char   *file_id,
                         DesktopEntry *entry,
                         SubtractData *sd)
{
  /* Remove everything in "set" which is not in "other" */

  if (g_hash_table_lookup (sd->other->hash, file_id) == NULL)
    return FALSE;

  menu_verbose (" Removing from %p entry %s\n", sd->set, file_id);

  return TRUE; /* return TRUE to remove */
}

void
desktop_entry_set_subtract (DesktopEntrySet *set,
                            DesktopEntrySet *other)
{
  SubtractData sd;

  menu_verbose (" Subtract from %p set %p\n", set, other);

  if (desktop_entry_set_get_count (set) == 0 ||
      desktop_entry_set_get_count (other) == 0)
    return; /* A fast simple case */

  sd.set   = set;
  sd.other = other;

  g_hash_table_foreach_remove (set->hash,
                               (GHRFunc) subtract_foreach_remove,
                               &sd);
}

void
desktop_entry_set_swap_contents (DesktopEntrySet *a,
                                 DesktopEntrySet *b)
{
  GHashTable *tmp;

  menu_verbose (" Swap contents of %p and %p\n", a, b);

  tmp = a->hash;
  a->hash = b->hash;
  b->hash = tmp;
}
