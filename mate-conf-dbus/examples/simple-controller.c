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

/* A very simple program that sets a single key value when you type
   it in an entry and press return */

#include <mateconf/mateconf-client.h>
#include <gtk/gtk.h>

void
entry_activated_callback(GtkWidget* entry, gpointer user_data)
{
  MateConfClient* client;
  gchar* str;
  
  client = MATECONF_CLIENT(user_data);

  str = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

  mateconf_client_set_string(client, "/extra/test/directory/key",
                          str, NULL);

  g_free(str);
}

int
main(int argc, char** argv)
{
  GtkWidget* window;
  GtkWidget* entry;
  MateConfClient* client;

  gtk_init(&argc, &argv);
  mateconf_init(argc, argv, NULL);
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  entry = gtk_entry_new();

  gtk_container_add(GTK_CONTAINER(window), entry);  

  client = mateconf_client_get_default();

  mateconf_client_add_dir(client,
                       "/extra/test/directory",
                       MATECONF_CLIENT_PRELOAD_NONE,
                       NULL);


  g_signal_connect (G_OBJECT (entry), "activate",
                    G_CALLBACK (entry_activated_callback),
                    client);

  /* If key isn't writable, then set insensitive */
  gtk_widget_set_sensitive (entry,
                            mateconf_client_key_is_writable (client,
                                                          "/extra/test/directory/key", NULL));
  
  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}


