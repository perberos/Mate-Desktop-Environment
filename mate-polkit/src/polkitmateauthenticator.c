/*
 * Copyright (C) 2009 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */


#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <glib/gi18n.h>

#include <polkit/polkit.h>
#include <polkitagent/polkitagent.h>

#include "polkitmateauthenticator.h"
#include "polkitmateauthenticationdialog.h"

struct _PolkitMateAuthenticator
{
  GObject parent_instance;

  PolkitAuthority *authority;
  gchar *action_id;
  gchar *message;
  gchar *icon_name;
  PolkitDetails *details;
  gchar *cookie;
  GList *identities;

  PolkitActionDescription *action_desc;
  gchar **users;

  gboolean gained_authorization;
  gboolean was_cancelled;
  gboolean new_user_selected;
  gchar *selected_user;

  PolkitAgentSession *session;
  GtkWidget *dialog;
  GMainLoop *loop;
};

struct _PolkitMateAuthenticatorClass
{
  GObjectClass parent_class;

};

enum
{
  COMPLETED_SIGNAL,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (PolkitMateAuthenticator, polkit_mate_authenticator, G_TYPE_OBJECT);

static void
polkit_mate_authenticator_init (PolkitMateAuthenticator *authenticator)
{
}

static void
polkit_mate_authenticator_finalize (GObject *object)
{
  PolkitMateAuthenticator *authenticator;

  authenticator = POLKIT_MATE_AUTHENTICATOR (object);

  if (authenticator->authority != NULL)
    g_object_unref (authenticator->authority);
  g_free (authenticator->action_id);
  g_free (authenticator->message);
  g_free (authenticator->icon_name);
  if (authenticator->details != NULL)
    g_object_unref (authenticator->details);
  g_free (authenticator->cookie);
  g_list_foreach (authenticator->identities, (GFunc) g_object_unref, NULL);
  g_list_free (authenticator->identities);

  if (authenticator->action_desc != NULL)
    g_object_unref (authenticator->action_desc);
  g_strfreev (authenticator->users);

  g_free (authenticator->selected_user);
  if (authenticator->session != NULL)
    g_object_unref (authenticator->session);
  if (authenticator->dialog != NULL)
    gtk_widget_destroy (authenticator->dialog);
  if (authenticator->loop != NULL)
    g_main_loop_unref (authenticator->loop);

  if (G_OBJECT_CLASS (polkit_mate_authenticator_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (polkit_mate_authenticator_parent_class)->finalize (object);
}

static void
polkit_mate_authenticator_class_init (PolkitMateAuthenticatorClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polkit_mate_authenticator_finalize;

  /**
   * PolkitMateAuthenticator::completed:
   * @authenticator: A #PolkitMateAuthenticator.
   * @gained_authorization: Whether the authorization was gained.
   *
   * Emitted when the authentication is completed. The user is supposed to dispose of @authenticator
   * upon receiving this signal.
   **/
  signals[COMPLETED_SIGNAL] = g_signal_new ("completed",
                                            POLKIT_MATE_TYPE_AUTHENTICATOR,
                                            G_SIGNAL_RUN_LAST,
                                            0,                      /* class offset     */
                                            NULL,                   /* accumulator      */
                                            NULL,                   /* accumulator data */
                                            g_cclosure_marshal_VOID__BOOLEAN,
                                            G_TYPE_NONE,
                                            1,
                                            G_TYPE_BOOLEAN);
}

static PolkitActionDescription *
get_desc_for_action (PolkitAuthority *authority,
                     const gchar     *action_id)
{
  GList *action_descs;
  GList *l;
  PolkitActionDescription *result;

  result = NULL;

  action_descs = polkit_authority_enumerate_actions_sync (authority,
                                                          NULL,
                                                          NULL);
  for (l = action_descs; l != NULL; l = l->next)
    {
      PolkitActionDescription *action_desc = POLKIT_ACTION_DESCRIPTION (l->data);

      if (strcmp (polkit_action_description_get_action_id (action_desc), action_id) == 0)
        {
          result = g_object_ref (action_desc);
          goto out;
        }
    }

 out:

  g_list_foreach (action_descs, (GFunc) g_object_unref, NULL);
  g_list_free (action_descs);

  return result;
}

static void
on_dialog_deleted (GtkWidget *widget,
                   GdkEvent  *event,
                   gpointer   user_data)
{
  PolkitMateAuthenticator *authenticator = POLKIT_MATE_AUTHENTICATOR (user_data);

  polkit_mate_authenticator_cancel (authenticator);
}

static void
on_user_selected (GObject    *object,
                  GParamSpec *pspec,
                  gpointer    user_data)
{
  PolkitMateAuthenticator *authenticator = POLKIT_MATE_AUTHENTICATOR (user_data);

  /* clear any previous messages */
  polkit_mate_authentication_dialog_set_info_message (POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog), "");

  polkit_mate_authenticator_cancel (authenticator);
  authenticator->new_user_selected = TRUE;
}

PolkitMateAuthenticator *
polkit_mate_authenticator_new (const gchar     *action_id,
                                const gchar     *message,
                                const gchar     *icon_name,
                                PolkitDetails   *details,
                                const gchar     *cookie,
                                GList           *identities)
{
  PolkitMateAuthenticator *authenticator;
  GList *l;
  guint n;
  GError *error;

  authenticator = POLKIT_MATE_AUTHENTICATOR (g_object_new (POLKIT_MATE_TYPE_AUTHENTICATOR, NULL));

  error = NULL;
  authenticator->authority = polkit_authority_get_sync (NULL /* GCancellable* */, &error);
  if (authenticator->authority == NULL)
    {
      g_critical ("Error getting authority: %s", error->message);
      g_error_free (error);
      goto error;
    }

  authenticator->action_id = g_strdup (action_id);
  authenticator->message = g_strdup (message);
  authenticator->icon_name = g_strdup (icon_name);
  if (details != NULL)
    authenticator->details = g_object_ref (details);
  authenticator->cookie = g_strdup (cookie);
  authenticator->identities = g_list_copy (identities);
  g_list_foreach (authenticator->identities, (GFunc) g_object_ref, NULL);

  authenticator->action_desc = get_desc_for_action (authenticator->authority,
                                                    authenticator->action_id);
  if (authenticator->action_desc == NULL)
    goto error;

  authenticator->users = g_new0 (gchar *, g_list_length (authenticator->identities) + 1);
  for (l = authenticator->identities, n = 0; l != NULL; l = l->next, n++)
    {
      PolkitUnixUser *user = POLKIT_UNIX_USER (l->data);
      uid_t uid;
      struct passwd *passwd;

      uid = polkit_unix_user_get_uid (user);
      passwd = getpwuid (uid);
      authenticator->users[n] = g_strdup (passwd->pw_name);
    }

  authenticator->dialog = polkit_mate_authentication_dialog_new
                            (authenticator->action_id,
                             polkit_action_description_get_vendor_name (authenticator->action_desc),
                             polkit_action_description_get_vendor_url (authenticator->action_desc),
                             authenticator->icon_name,
                             authenticator->message,
                             authenticator->details,
                             authenticator->users);
  g_signal_connect (authenticator->dialog,
                    "delete-event",
                    G_CALLBACK (on_dialog_deleted),
                    authenticator);
  g_signal_connect (authenticator->dialog,
                    "notify::selected-user",
                    G_CALLBACK (on_user_selected),
                    authenticator);

  return authenticator;

 error:
  g_object_unref (authenticator);
  return NULL;
}

static void
session_request (PolkitAgentSession *session,
                 const char         *request,
                 gboolean            echo_on,
                 gpointer            user_data)
{
  PolkitMateAuthenticator *authenticator = POLKIT_MATE_AUTHENTICATOR (user_data);
  gchar *password;
  gchar *modified_request;

  password = NULL;

  //g_debug ("in conversation_pam_prompt, request='%s', echo_on=%d", request, echo_on);

  /* Fix up, and localize, password prompt if it's password auth */
  if (g_ascii_strncasecmp (request, "password:", 9) == 0)
    {
      if (strcmp (g_get_user_name (), authenticator->selected_user) != 0)
        {
          modified_request = g_strdup_printf (_("_Password for %s:"), authenticator->selected_user);
        }
      else
        {
          modified_request = g_strdup (_("_Password:"));
        }
    }
  else
    {
      modified_request = g_strdup (request);
    }

  gtk_widget_show_all (GTK_WIDGET (authenticator->dialog));
  gtk_window_present (GTK_WINDOW (authenticator->dialog));
  password = polkit_mate_authentication_dialog_run_until_response_for_prompt (POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog),
                                                                               modified_request,
                                                                               echo_on,
                                                                               &authenticator->was_cancelled,
                                                                               &authenticator->new_user_selected);

  /* cancel auth unless user provided a password */
  if (password == NULL)
    {
      polkit_mate_authenticator_cancel (authenticator);
      goto out;
    }
  else
    {
      polkit_agent_session_response (authenticator->session, password);
      g_free (password);
    }

out:
  g_free (modified_request);
}

static void
session_show_error (PolkitAgentSession *session,
                    const gchar        *msg,
                    gpointer            user_data)
{
  PolkitMateAuthenticator *authenticator = POLKIT_MATE_AUTHENTICATOR (user_data);
  gchar *s;

  s = g_strconcat ("<b>", msg, "</b>", NULL);
  polkit_mate_authentication_dialog_set_info_message (POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog), s);
  g_free (s);
}

static void
session_show_info (PolkitAgentSession *session,
                   const gchar        *msg,
                   gpointer            user_data)
{
  PolkitMateAuthenticator *authenticator = POLKIT_MATE_AUTHENTICATOR (user_data);
  gchar *s;

  s = g_strconcat ("<b>", msg, "</b>", NULL);
  polkit_mate_authentication_dialog_set_info_message (POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog), s);
  g_free (s);

  gtk_widget_show_all (GTK_WIDGET (authenticator->dialog));
  gtk_window_present (GTK_WINDOW (authenticator->dialog));
}


static void
session_completed (PolkitAgentSession *session,
                   gboolean            gained_authorization,
                   gpointer            user_data)
{
  PolkitMateAuthenticator *authenticator = POLKIT_MATE_AUTHENTICATOR (user_data);

  authenticator->gained_authorization = gained_authorization;

  //g_debug ("in conversation_done gained=%d", gained_authorization);

  g_main_loop_quit (authenticator->loop);
}


static gboolean
do_initiate (gpointer user_data)
{
  PolkitMateAuthenticator *authenticator = POLKIT_MATE_AUTHENTICATOR (user_data);
  PolkitIdentity *identity;
  gint num_tries;

  gtk_widget_show_all (GTK_WIDGET (authenticator->dialog));
  gtk_window_present (GTK_WINDOW (authenticator->dialog));
  if (!polkit_mate_authentication_dialog_run_until_user_is_selected (POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog)))
    {
      /* user cancelled the dialog */
      /*g_debug ("User cancelled before selecting a user");*/
      authenticator->was_cancelled = TRUE;
      goto out;
    }

  authenticator->loop = g_main_loop_new (NULL, TRUE);

  num_tries = 0;

 try_again:

  g_free (authenticator->selected_user);
  authenticator->selected_user = polkit_mate_authentication_dialog_get_selected_user (POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog));

  /*g_debug ("Authenticating user %s", authenticator->selected_user);*/
  identity = polkit_unix_user_new_for_name (authenticator->selected_user, NULL);

  authenticator->session = polkit_agent_session_new (identity, authenticator->cookie);

  g_object_unref (identity);

  g_signal_connect (authenticator->session,
                    "request",
                    G_CALLBACK (session_request),
                    authenticator);

  g_signal_connect (authenticator->session,
                    "show-info",
                    G_CALLBACK (session_show_info),
                    authenticator);

  g_signal_connect (authenticator->session,
                    "show-error",
                    G_CALLBACK (session_show_error),
                    authenticator);

  g_signal_connect (authenticator->session,
                    "completed",
                    G_CALLBACK (session_completed),
                    authenticator);

  polkit_agent_session_initiate (authenticator->session);

  g_main_loop_run (authenticator->loop);

  /*g_debug ("gained_authorization=%d was_cancelled=%d new_user_selected=%d.",
           authenticator->gained_authorization,
           authenticator->was_cancelled,
           authenticator->new_user_selected);*/

  if (authenticator->new_user_selected)
    {
      /*g_debug ("New user selected");*/
      authenticator->new_user_selected = FALSE;
      g_object_unref (authenticator->session);
      authenticator->session = NULL;
      goto try_again;
    }

  num_tries++;

  if (!authenticator->gained_authorization && !authenticator->was_cancelled)
    {
      if (authenticator->dialog != NULL)
        {
          gchar *s;

          s = g_strconcat ("<b>", _("Authentication Failure"), "</b>", NULL);
          polkit_mate_authentication_dialog_set_info_message (
                                  POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog),
                                  s);
          g_free (s);
          gtk_widget_queue_draw (authenticator->dialog);

          /* shake the dialog to indicate error */
          polkit_mate_authentication_dialog_indicate_error (POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog));

          if (num_tries < 3)
            {
              g_object_unref (authenticator->session);
              authenticator->session = NULL;
              goto try_again;
            }
        }
    }

 out:
  g_signal_emit_by_name (authenticator, "completed", authenticator->gained_authorization);

  g_object_unref (authenticator);

  return FALSE;
}

void
polkit_mate_authenticator_initiate (PolkitMateAuthenticator *authenticator)
{
  /* run from idle since we're going to block the main loop in the dialog (which has a recursive mainloop) */
  g_idle_add (do_initiate, g_object_ref (authenticator));
}

void
polkit_mate_authenticator_cancel (PolkitMateAuthenticator *authenticator)
{
  if (authenticator->dialog != NULL)
    polkit_mate_authentication_dialog_cancel (POLKIT_MATE_AUTHENTICATION_DIALOG (authenticator->dialog));

  authenticator->was_cancelled = TRUE;

  if (authenticator->session != NULL)
    {
      polkit_agent_session_cancel (authenticator->session);
    }
}

const gchar *
polkit_mate_authenticator_get_cookie (PolkitMateAuthenticator *authenticator)
{
  return authenticator->cookie;
}


