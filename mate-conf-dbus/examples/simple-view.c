/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* MateConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* A very simple program that monitors a single key for changes. */

#include <mateconf/mateconf-client.h>
#include <gtk/gtk.h>

void
key_changed_callback(MateConfClient* client,
                     guint cnxn_id,
                     MateConfEntry *entry,
                     gpointer user_data)
{
  GtkWidget* label;
  
  label = GTK_WIDGET(user_data);

  if (mateconf_entry_get_value (entry) == NULL)
    {
      gtk_label_set_text (GTK_LABEL (label), "<unset>");
    }
  else
    {
      if (mateconf_entry_get_value (entry)->type == MATECONF_VALUE_STRING)
        {
          gtk_label_set_text (GTK_LABEL (label),
                              mateconf_value_get_string (mateconf_entry_get_value (entry)));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (label), "<wrong type>");
        }
    }
}

int
main(int argc, char** argv)
{
  GtkWidget* window;
  GtkWidget* label;
  MateConfClient* client;
  gchar* str;
  
  gtk_init(&argc, &argv);
  mateconf_init(argc, argv, NULL);

  client = mateconf_client_get_default();
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  str = mateconf_client_get_string(client, "/extra/test/directory/key",
                                NULL);
  
  label = gtk_label_new(str ? str : "<unset>");

  if (str)
    g_free(str);
  
  gtk_container_add(GTK_CONTAINER(window), label);

  mateconf_client_add_dir(client,
                       "/extra/test/directory",
                       MATECONF_CLIENT_PRELOAD_NONE,
                       NULL);

  mateconf_client_notify_add(client, "/extra/test/directory/key",
                          key_changed_callback,
                          label,
                          NULL, NULL);
  
  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}




