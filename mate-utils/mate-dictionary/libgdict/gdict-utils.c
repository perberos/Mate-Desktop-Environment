/* gdict-utils.c - Utility functions for Gdict
 *
 * Copyright (C) 2005  Emmanuele Bassi <ebassi@gmail.com>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "gdict-context-private.h"
#include "gdict-debug.h"
#include "gdict-utils.h"
#include "gdict-version.h"
#include "gdict-private.h"

guint gdict_major_version = GDICT_MAJOR_VERSION;
guint gdict_minor_version = GDICT_MINOR_VERSION;
guint gdict_micro_version = GDICT_MICRO_VERSION;

guint gdict_debug_flags = 0;  /* global gdict debug flag */

#ifdef GDICT_ENABLE_DEBUG
static const GDebugKey gdict_debug_keys[] = {
  { "misc", GDICT_DEBUG_MISC },
  { "context", GDICT_DEBUG_CONTEXT },
  { "dict", GDICT_DEBUG_DICT },
  { "source", GDICT_DEBUG_SOURCE },
  { "loader", GDICT_DEBUG_LOADER },
  { "chooser", GDICT_DEBUG_CHOOSER },
  { "defbux", GDICT_DEBUG_DEFBOX },
  { "speller", GDICT_DEBUG_SPELLER },
};
#endif /* GDICT_ENABLE_DEBUG */

static gboolean gdict_is_initialized = FALSE;

#ifdef GDICT_ENABLE_DEBUG
static gboolean
gdict_arg_debug_cb (const char *key,
                    const char *value,
                    gpointer    user_data)
{
  gdict_debug_flags |=
    g_parse_debug_string (value,
                          gdict_debug_keys,
                          G_N_ELEMENTS (gdict_debug_keys));
  return TRUE;
}

static gboolean
gdict_arg_no_debug_cb (const char *key,
                       const char *value,
                       gpointer    user_data)
{
  gdict_debug_flags &=
    ~g_parse_debug_string (value,
                           gdict_debug_keys,
                           G_N_ELEMENTS (gdict_debug_keys));
  return TRUE;
}
#endif /* CLUTTER_ENABLE_DEBUG */

static GOptionEntry gdict_args[] = {
#ifdef GDICT_ENABLE_DEBUG
  { "gdict-debug", 0, 0, G_OPTION_ARG_CALLBACK, gdict_arg_debug_cb,
    N_("GDict debugging flags to set"), N_("FLAGS") },
  { "gdict-no-debug", 0, 0, G_OPTION_ARG_CALLBACK, gdict_arg_no_debug_cb,
    N_("GDict debugging flags to unset"), N_("FLAGS") },
#endif /* GDICT_ENABLE_DEBUG */
  { NULL, },
};

static gboolean
pre_parse_hook (GOptionContext  *context,
                GOptionGroup    *group,
                gpointer         data,
                GError         **error)
{
  const char *env_string;

  if (gdict_is_initialized)
    return TRUE;

#ifdef GDICT_ENABLE_DEBUG
  env_string = g_getenv ("GDICT_DEBUG");
  if (env_string != NULL)
    {
      gdict_debug_flags =
        g_parse_debug_string (env_string,
                              gdict_debug_keys,
                              G_N_ELEMENTS (gdict_debug_keys));
    }
#else
  env_string = NULL;
#endif /* GDICT_ENABLE_DEBUG */

  return TRUE;
}

static gboolean
post_parse_hook (GOptionContext  *context,
                 GOptionGroup    *group,
                 gpointer         data,
                 GError         **error)
{
  gdict_is_initialized = TRUE;

  return TRUE;
}

/**
 * gdict_get_option_group:
 *
 * FIXME
 *
 * Return value: FIXME
 *
 * Since: 0.12
 */
GOptionGroup *
gdict_get_option_group (void)
{
  GOptionGroup *group;

  group = g_option_group_new ("gdict",
                              _("GDict Options"),
                              _("Show GDict Options"),
                              NULL,
                              NULL);
  
  g_option_group_set_parse_hooks (group, pre_parse_hook, post_parse_hook);
  g_option_group_add_entries (group, gdict_args);
  g_option_group_set_translation_domain (group, GETTEXT_PACKAGE);

  return group;
}

/**
 * gdict_debug_init:
 * @argc: FIXME
 * @argv: FIXME
 *
 * FIXME
 *
 * Since: 0.12
 */
void
gdict_debug_init (gint    *argc,
                  gchar ***argv)
{
  GOptionContext *option_context;
  GOptionGroup *gdict_group;
  GError *error = NULL;

  if (gdict_is_initialized)
    return;

  option_context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  g_option_context_set_help_enabled (option_context, FALSE); 

  gdict_group = gdict_get_option_group ();
  g_option_context_set_main_group (option_context, gdict_group);

  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_option_context_free (option_context);
}

gboolean
gdict_check_version (guint required_major,
                     guint required_minor,
                     guint required_micro)
{
  gint gdict_effective_micro = 100 * GDICT_MINOR_VERSION + GDICT_MICRO_VERSION;
  gint required_effective_micro = 100 * required_minor + required_micro;

  if (required_major > GDICT_MAJOR_VERSION)
    return FALSE;

  if (required_major < GDICT_MAJOR_VERSION)
    return FALSE;

  if (required_effective_micro < gdict_effective_micro)
    return FALSE;

  if (required_effective_micro > gdict_effective_micro)
    return FALSE;

  return TRUE;
}
/* gdict_has_ipv6: checks for the existence of the IPv6 extensions; if
 * IPv6 support was not enabled, this function always return false
 */
gboolean
_gdict_has_ipv6 (void)
{
#ifdef ENABLE_IPV6
  int s;

  s = socket (AF_INET6, SOCK_STREAM, 0);
  if (s != -1)
    {
      close(s);
      
      return TRUE;
    }
#endif

  return FALSE;
}

/* shows an error dialog making it transient for @parent */
static void
show_error_dialog (GtkWindow   *parent,
		   const gchar *message,
		   const gchar *detail)
{
  GtkWidget *dialog;
  
  dialog = gtk_message_dialog_new (parent,
  				   GTK_DIALOG_DESTROY_WITH_PARENT,
  				   GTK_MESSAGE_ERROR,
  				   GTK_BUTTONS_OK,
  				   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), "");
  
  if (detail)
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
  					      "%s", detail);
  
  if (parent && gtk_window_get_group (parent))
    gtk_window_group_add_window (gtk_window_get_group (parent), GTK_WINDOW (dialog));
  
  gtk_dialog_run (GTK_DIALOG (dialog));
  
  gtk_widget_destroy (dialog);
}

/* find the toplevel widget for @widget */
static GtkWindow *
get_toplevel_window (GtkWidget *widget)
{
  GtkWidget *toplevel;
  
  toplevel = gtk_widget_get_toplevel (widget);
  if (!gtk_widget_is_toplevel (toplevel))
    return NULL;
  else
    return GTK_WINDOW (toplevel);
}

/**
 * gdict_show_error_dialog:
 * @widget: the widget that emits the error
 * @title: the primary error message
 * @message: the secondary error message or %NULL
 *
 * Creates and shows an error dialog bound to @widget.
 *
 * Since: 1.0
 */
void
_gdict_show_error_dialog (GtkWidget   *widget,
			  const gchar *title,
			  const gchar *detail)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (title != NULL);
  
  show_error_dialog (get_toplevel_window (widget), title, detail);
}

/**
 * gdict_show_gerror_dialog:
 * @widget: the widget that emits the error
 * @title: the primary error message
 * @error: a #GError
 *
 * Creates and shows an error dialog bound to @widget, using @error
 * to fill the secondary text of the message dialog with the error
 * message.  Also takes care of freeing @error.
 *
 * Since: 1.0
 */
void
_gdict_show_gerror_dialog (GtkWidget   *widget,
			   const gchar *title,
			   GError      *error)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (title != NULL);
  g_return_if_fail (error != NULL);
  
  show_error_dialog (get_toplevel_window (widget), title, error->message);
      
  g_error_free (error);
}
