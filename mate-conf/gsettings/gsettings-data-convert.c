/*
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Matthias Clasen <mclasen@redhat.com>
 */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <glib.h>
#include <gio/gio.h>
#include <mateconf/mateconf-client.h>

static const gchar convert_dir[] = DATADIR "/MateConf/gsettings";

static gboolean verbose = FALSE;
static gboolean dry_run = FALSE;

extern const gchar *mateconf_value_type_to_string (int type);

static gboolean
handle_file (const gchar *filename)
{
  GKeyFile *keyfile;
  MateConfClient *client;
  MateConfValue *value;
  gint i, j;
  gchar *mateconf_key;
  gchar **groups;
  gchar **keys;
  GVariantBuilder *builder;
  GVariant *v;
  const gchar *s;
  gchar *str;
  gint ii;
  GSList *list, *l;
  GSettings *settings;
  GError *error;

  keyfile = g_key_file_new ();

  error = NULL;
  if (!g_key_file_load_from_file (keyfile, filename, 0, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);

      g_key_file_free (keyfile);

      return FALSE;
    }

  client = mateconf_client_get_default ();

  groups = g_key_file_get_groups (keyfile, NULL);
  for (i = 0; groups[i]; i++)
    {
      gchar **schema_path;

      schema_path = g_strsplit (groups[i], ":", 2);

      if (verbose)
        {
          g_print ("collecting settings for schema '%s'\n", schema_path[0]);
          if (schema_path[1])
            g_print ("for storage at '%s'\n", schema_path[1]);
        }

      if (schema_path[1] != NULL)
        settings = g_settings_new_with_path (schema_path[0], schema_path[1]);
      else
        settings = g_settings_new (schema_path[0]);

      g_settings_delay (settings);

      error = NULL;
      if ((keys = g_key_file_get_keys (keyfile, groups[i], NULL, &error)) == NULL)
        {
          g_printerr ("%s", error->message);
          g_error_free (error);

          continue;
        }

      for (j = 0; keys[j]; j++)
        {
          if (strchr (keys[j], '/') != 0)
            {
              g_printerr ("Key '%s' contains a '/'\n", keys[j]);

              continue;
            }

          error = NULL;
          if ((mateconf_key = g_key_file_get_string (keyfile, groups[i], keys[j], &error)) ==  NULL)
            {
              g_printerr ("%s", error->message);
              g_error_free (error);

              continue;
            }

          error = NULL;
          if ((value = mateconf_client_get_without_default (client, mateconf_key, &error)) == NULL)
            {
              if (error)
                {
                  g_printerr ("Failed to get MateConf key '%s': %s\n",
                              mateconf_key, error->message);
                  g_error_free (error);
                }
              else
                {
                  if (verbose)
                    g_print ("Skipping MateConf key '%s', no user value\n",
                             mateconf_key);
                }

              g_free (mateconf_key);

              continue;
            }

          switch (value->type)
            {
            case MATECONF_VALUE_STRING:
              if (dry_run)
                g_print ("set key '%s' to string '%s'\n", keys[j],
                         mateconf_value_get_string (value));
              else
                g_settings_set (settings, keys[j], "s",
                                mateconf_value_get_string (value));
              break;

            case MATECONF_VALUE_INT:
              if (dry_run)
                g_print ("set key '%s' to integer '%d'\n",
                         keys[j], mateconf_value_get_int (value));
              else
                g_settings_set (settings, keys[j], "i",
                                mateconf_value_get_int (value));
              break;

            case MATECONF_VALUE_BOOL:
              if (dry_run)
                g_print ("set key '%s' to boolean '%d'\n",
                         keys[j], mateconf_value_get_bool (value));
              else
                g_settings_set (settings, keys[j], "b",
                                mateconf_value_get_bool (value));
              break;

            case MATECONF_VALUE_FLOAT:
              if (dry_run)
                g_print ("set key '%s' to double '%g'\n",
                         keys[j], mateconf_value_get_float (value));
              else
                g_settings_set (settings, keys[j], "d",
                                mateconf_value_get_float (value));
              break;

            case MATECONF_VALUE_LIST:
              switch (mateconf_value_get_list_type (value))
                {
                case MATECONF_VALUE_STRING:
                  builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
                  list = mateconf_value_get_list (value);
                  if (list != NULL)
                    {
                      for (l = list; l; l = l->next)
                        {
                          MateConfValue *lv = l->data;
                          s = mateconf_value_get_string (lv);
                          g_variant_builder_add (builder, "s", s);
                        }
                      v = g_variant_new ("as", builder);
                    }
                  else
                    v = g_variant_new_array (G_VARIANT_TYPE_STRING, NULL, 0);
                  g_variant_ref_sink (v);

                  if (dry_run)
                    {
                      str = g_variant_print (v, FALSE);
                      g_print ("set key '%s' to a list of strings: %s\n",
                               keys[j], str);
                      g_free (str);
                    }
                  else
                    g_settings_set_value (settings, keys[j], v);

                  g_variant_unref (v);
                  g_variant_builder_unref (builder);
                  break;

                case MATECONF_VALUE_INT:
                  builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
                  list = mateconf_value_get_list (value);
                  if (list != NULL)
                    {
                      for (l = list; l; l = l->next)
                        {
                          MateConfValue *lv = l->data;
                          ii = mateconf_value_get_int (lv);
                          g_variant_builder_add (builder, "i", ii);
                        }
                      v = g_variant_new ("ai", builder);
                    }
                  else
                    v = g_variant_new_array (G_VARIANT_TYPE_INT32, NULL, 0);
                  g_variant_ref_sink (v);

                  if (dry_run)
                    {
                      str = g_variant_print (v, FALSE);
                      g_print ("set key '%s' to a list of integers: %s\n",
                               keys[j], str);
                      g_free (str);
                    }
                  else
                    g_settings_set_value (settings, keys[j], v);

                  g_variant_unref (v);
                  g_variant_builder_unref (builder);
                  break;

                default:
                  g_printerr ("Keys of type 'list of %s' not handled yet\n",
                              mateconf_value_type_to_string (mateconf_value_get_list_type (value)));
                  break;
                }
              break;

            default:
              g_printerr ("Keys of type %s not handled yet\n",
                          mateconf_value_type_to_string (value->type));
              break;
            }

          mateconf_value_free (value);
          g_free (mateconf_key);
        }

      g_strfreev (keys);

      if (!dry_run)
        g_settings_apply (settings);

      g_object_unref (settings);
      g_strfreev (schema_path);
    }

  g_strfreev (groups);

  g_object_unref (client);

  return TRUE;
}

static void
load_state (time_t  *mtime,
            gchar ***converted)
{
  gchar *filename;
  GKeyFile *keyfile;
  GError *error;
  gchar *str;
  gchar **list;

  *mtime = 0;
  *converted = g_new0 (gchar *, 1);

  filename = g_build_filename (g_get_user_data_dir (), "gsettings-data-convert", NULL);
  keyfile = g_key_file_new ();

  /* ensure file exists */
  if (!g_file_test (filename, G_FILE_TEST_EXISTS))
    return;

  error = NULL;
  if (!g_key_file_load_from_file (keyfile, filename, 0, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      return;
    }

  error = NULL;
  if ((str = g_key_file_get_string (keyfile, "State", "timestamp", &error)) == NULL)
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
    }
  else
    {
      *mtime = (time_t)g_ascii_strtoll (str, NULL, 0);
      g_free (str);
    }

  error = NULL;
  if ((list = g_key_file_get_string_list (keyfile, "State", "converted", NULL, &error)) == NULL)
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_strfreev (*converted);
      *converted = list;
    }

  g_key_file_free (keyfile);
  g_free (filename);
}

static gboolean
save_state (time_t   mtime,
            gchar  **converted)
{
  gchar *filename;
  GKeyFile *keyfile;
  gchar *str;
  GError *error;
  gboolean result;

  /* Make sure the state directory exists */
  if (g_mkdir_with_parents (g_get_user_data_dir (), 0755))
    {
      g_printerr ("Failed to create directory %s: %s\n",
                  g_get_user_data_dir (), g_strerror (errno));
      return FALSE;
    }

  filename = g_build_filename (g_get_user_data_dir (), "gsettings-data-convert", NULL);
  keyfile = g_key_file_new ();

  str = g_strdup_printf ("%ld", mtime);
  g_key_file_set_string (keyfile,
                         "State", "timestamp", str);
  g_free (str);

  g_key_file_set_string_list (keyfile,
                              "State", "converted",
                              (const gchar * const *)converted, g_strv_length (converted));

  str = g_key_file_to_data (keyfile, NULL, NULL);
  g_key_file_free (keyfile);

  error = NULL;
  if (!g_file_set_contents (filename, str, -1, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);

      result = FALSE;
    }
  else
    result = TRUE;

  g_free (filename);
  g_free (str);

  return result;
}

int
main (int argc, char *argv[])
{
  time_t stored_mtime;
  time_t dir_mtime;
  struct stat statbuf;
  GError *error;
  gchar **converted;
  GDir *dir;
  const gchar *name;
  gchar *filename;
  gint i;
  GOptionContext *context;
  GOptionEntry entries[] = {
    { "verbose", 0, 0, G_OPTION_ARG_NONE, &verbose, "show verbose messages", NULL },
    { "dry-run", 0, 0, G_OPTION_ARG_NONE, &dry_run, "do not perform any changes", NULL },
    { NULL }
  };

  context = g_option_context_new ("");

  g_option_context_set_summary (context,
    "Migrate settings from the users MateConf database to GSettings.");

  g_option_context_add_main_entries (context, entries, NULL);

  error = NULL;
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      return 1;
    }

  load_state (&stored_mtime, &converted);

  /* If the directory is not newer, exit */
  if (stat (convert_dir, &statbuf) == 0)
    dir_mtime = statbuf.st_mtime;
 else
    {
      if (verbose)
        g_print ("Directory '%s' does not exist, nothing to do\n", convert_dir);
      return 0;
    }

  if (dir_mtime <= stored_mtime)
    {
      if (verbose)
        g_print ("All uptodate, nothing to do\n");
      return 0;
    }

  error = NULL;
  dir = g_dir_open (convert_dir, 0, &error);
  if (dir == NULL)
    {
      g_printerr ("Failed to open '%s': %s\n", convert_dir, error->message);
      return 1;
    }

  while ((name = g_dir_read_name (dir)) != NULL)
    {
       for (i = 0; converted[i]; i++)
         {
           if (strcmp (name, converted[i]) == 0)
             {
               if (verbose)
                 g_print ("File '%s already converted, skipping\n", name);
               goto next;
             }
         }

      filename = g_build_filename (convert_dir, name, NULL);

      if (handle_file (filename))
        {
          gint len;

          /* Add the the file to the converted list */
          len = g_strv_length (converted);
          converted = g_realloc (converted, (len + 2) * sizeof (gchar *));
          converted[len] = g_strdup (name);
          converted[len + 1] = NULL;
        }

      g_free (filename);

 next: ;
    }

  if (!dry_run)
    {
      if (!save_state (dir_mtime, converted))
        return 1;
    }

  return 0;
}

