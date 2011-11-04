/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>

#include <security/pam_appl.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gio/gio.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <X11/Xauth.h>

#include "ck-connector.h"

#include "mdm-common.h"
#include "mdm-session-worker.h"
#include "mdm-marshal.h"

#if defined (HAVE_ADT)
#include "mdm-session-solaris-auditor.h"
#elif defined (HAVE_LIBAUDIT)
#include "mdm-session-linux-auditor.h"
#else
#include "mdm-session-auditor.h"
#endif

#include "mdm-session-settings.h"

#define MDM_SESSION_WORKER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SESSION_WORKER, MdmSessionWorkerPrivate))

#define MDM_SESSION_DBUS_PATH         "/org/mate/DisplayManager/Session"
#define MDM_SESSION_DBUS_INTERFACE    "org.mate.DisplayManager.Session"
#define MDM_SESSION_DBUS_ERROR_CANCEL "org.mate.DisplayManager.Session.Error.Cancel"

#ifndef MDM_PASSWD_AUXILLARY_BUFFER_SIZE
#define MDM_PASSWD_AUXILLARY_BUFFER_SIZE 1024
#endif

#ifndef MDM_SESSION_DEFAULT_PATH
#define MDM_SESSION_DEFAULT_PATH "/usr/local/bin:/usr/bin:/bin"
#endif

#ifndef MDM_SESSION_ROOT_UID
#define MDM_SESSION_ROOT_UID 0
#endif

#ifndef MDM_SESSION_LOG_FILENAME
#define MDM_SESSION_LOG_FILENAME ".xsession-errors"
#endif

#define MAX_FILE_SIZE     65536

enum {
        MDM_SESSION_WORKER_STATE_NONE = 0,
        MDM_SESSION_WORKER_STATE_SETUP_COMPLETE,
        MDM_SESSION_WORKER_STATE_AUTHENTICATED,
        MDM_SESSION_WORKER_STATE_AUTHORIZED,
        MDM_SESSION_WORKER_STATE_ACCREDITED,
        MDM_SESSION_WORKER_STATE_SESSION_OPENED,
        MDM_SESSION_WORKER_STATE_SESSION_STARTED,
        MDM_SESSION_WORKER_STATE_REAUTHENTICATED,
        MDM_SESSION_WORKER_STATE_REAUTHORIZED,
        MDM_SESSION_WORKER_STATE_REACCREDITED,
};

struct MdmSessionWorkerPrivate
{
        int               state;

        int               exit_code;

        CkConnector      *ckc;
        pam_handle_t     *pam_handle;

        GPid              child_pid;
        guint             child_watch_id;

        /* from Setup */
        char             *service;
        char             *x11_display_name;
        char             *x11_authority_file;
        char             *display_device;
        char             *hostname;
        char             *username;
        uid_t             uid;
        gid_t             gid;
        gboolean          password_is_required;

        int               cred_flags;

        char            **arguments;
        GHashTable       *environment;
        guint32           cancelled : 1;
        guint32           timed_out : 1;
        guint             state_change_idle_id;

        char             *server_address;
        DBusConnection   *connection;

        MdmSessionAuditor  *auditor;
        MdmSessionSettings *user_settings;
};

enum {
        PROP_0,
        PROP_SERVER_ADDRESS,
};

static void     mdm_session_worker_class_init   (MdmSessionWorkerClass *klass);
static void     mdm_session_worker_init         (MdmSessionWorker      *session_worker);
static void     mdm_session_worker_finalize     (GObject               *object);

static void     queue_state_change              (MdmSessionWorker      *worker);

typedef int (* MdmSessionWorkerPamNewMessagesFunc) (int,
                                                    const struct pam_message **,
                                                    struct pam_response **,
                                                    gpointer);

G_DEFINE_TYPE (MdmSessionWorker, mdm_session_worker, G_TYPE_OBJECT)

GQuark
mdm_session_worker_error_quark (void)
{
        static GQuark error_quark = 0;

        if (error_quark == 0)
                error_quark = g_quark_from_static_string ("mdm-session-worker");

        return error_quark;
}

static gboolean
open_ck_session (MdmSessionWorker  *worker)
{
        struct passwd *pwent;
        gboolean       ret;
        int            res;
        DBusError      error;
        const char     *display_name;
        const char     *display_device;
        const char     *display_hostname;
        gboolean        is_local;

        ret = FALSE;

        if (worker->priv->x11_display_name != NULL) {
                display_name = worker->priv->x11_display_name;
        } else {
                display_name = "";
        }
        if (worker->priv->hostname != NULL) {
                display_hostname = worker->priv->hostname;
        } else {
                display_hostname = "";
        }
        if (worker->priv->display_device != NULL) {
                display_device = worker->priv->display_device;
        } else {
                display_device = "";
        }

        g_assert (worker->priv->username != NULL);

        /* FIXME: this isn't very good */
        if (display_hostname == NULL
            || display_hostname[0] == '\0'
            || strcmp (display_hostname, "localhost") == 0) {
                is_local = TRUE;
        } else {
                is_local = FALSE;
        }

        mdm_get_pwent_for_name (worker->priv->username, &pwent);
        if (pwent == NULL) {
                goto out;
        }

        worker->priv->ckc = ck_connector_new ();
        if (worker->priv->ckc == NULL) {
                g_warning ("Couldn't create new ConsoleKit connector");
                goto out;
        }

        dbus_error_init (&error);
        res = ck_connector_open_session_with_parameters (worker->priv->ckc,
                                                         &error,
                                                         "unix-user", &pwent->pw_uid,
                                                         "x11-display", &display_name,
                                                         "x11-display-device", &display_device,
                                                         "remote-host-name", &display_hostname,
                                                         "is-local", &is_local,
                                                         NULL);

        if (! res) {
                if (dbus_error_is_set (&error)) {
                        g_warning ("%s\n", error.message);
                        dbus_error_free (&error);
                } else {
                        g_warning ("cannot open CK session: OOM, D-Bus system bus not available,\n"
                                   "ConsoleKit not available or insufficient privileges.\n");
                }
                goto out;
        }

        ret = TRUE;

 out:
        return ret;
}

/* adapted from glib script_execute */
static void
script_execute (const gchar *file,
                char       **argv,
                char       **envp,
                gboolean     search_path)
{
        /* Count the arguments.  */
        int argc = 0;

        while (argv[argc]) {
                ++argc;
        }

        /* Construct an argument list for the shell.  */
        {
                char **new_argv;

                new_argv = g_new0 (gchar*, argc + 2); /* /bin/sh and NULL */

                new_argv[0] = (char *) "/bin/sh";
                new_argv[1] = (char *) file;
                while (argc > 0) {
                        new_argv[argc + 1] = argv[argc];
                        --argc;
                }

                /* Execute the shell. */
                if (envp) {
                        execve (new_argv[0], new_argv, envp);
                } else {
                        execv (new_argv[0], new_argv);
                }

                g_free (new_argv);
        }
}

static char *
my_strchrnul (const char *str, char c)
{
        char *p = (char*) str;
        while (*p && (*p != c)) {
                ++p;
        }

        return p;
}

/* adapted from glib g_execute */
static gint
mdm_session_execute (const char *file,
                     char      **argv,
                     char      **envp,
                     gboolean    search_path)
{
        if (*file == '\0') {
                /* We check the simple case first. */
                errno = ENOENT;
                return -1;
        }

        if (!search_path || strchr (file, '/') != NULL) {
                /* Don't search when it contains a slash. */
                if (envp) {
                        execve (file, argv, envp);
                } else {
                        execv (file, argv);
                }

                if (errno == ENOEXEC) {
                        script_execute (file, argv, envp, FALSE);
                }
        } else {
                gboolean got_eacces = 0;
                const char *path, *p;
                char *name, *freeme;
                gsize len;
                gsize pathlen;

                path = g_getenv ("PATH");
                if (path == NULL) {
                        /* There is no `PATH' in the environment.  The default
                         * search path in libc is the current directory followed by
                         * the path `confstr' returns for `_CS_PATH'.
                         */

                        /* In GLib we put . last, for security, and don't use the
                         * unportable confstr(); UNIX98 does not actually specify
                         * what to search if PATH is unset. POSIX may, dunno.
                         */

                        path = "/bin:/usr/bin:.";
                }

                len = strlen (file) + 1;
                pathlen = strlen (path);
                freeme = name = g_malloc (pathlen + len + 1);

                /* Copy the file name at the top, including '\0'  */
                memcpy (name + pathlen + 1, file, len);
                name = name + pathlen;
                /* And add the slash before the filename  */
                *name = '/';

                p = path;
                do {
                        char *startp;

                        path = p;
                        p = my_strchrnul (path, ':');

                        if (p == path) {
                                /* Two adjacent colons, or a colon at the beginning or the end
                                 * of `PATH' means to search the current directory.
                                 */
                                startp = name + 1;
                        } else {
                                startp = memcpy (name - (p - path), path, p - path);
                        }

                        /* Try to execute this name.  If it works, execv will not return.  */
                        if (envp) {
                                execve (startp, argv, envp);
                        } else {
                                execv (startp, argv);
                        }

                        if (errno == ENOEXEC) {
                                script_execute (startp, argv, envp, search_path);
                        }

                        switch (errno) {
                        case EACCES:
                                /* Record the we got a `Permission denied' error.  If we end
                                 * up finding no executable we can use, we want to diagnose
                                 * that we did find one but were denied access.
                                 */
                                got_eacces = TRUE;

                                /* FALL THRU */

                        case ENOENT:
#ifdef ESTALE
                        case ESTALE:
#endif
#ifdef ENOTDIR
                        case ENOTDIR:
#endif
                                /* Those errors indicate the file is missing or not executable
                                 * by us, in which case we want to just try the next path
                                 * directory.
                                 */
                                break;

                        default:
                                /* Some other error means we found an executable file, but
                                 * something went wrong executing it; return the error to our
                                 * caller.
                                 */
                                g_free (freeme);
                                return -1;
                        }
                } while (*p++ != '\0');

                /* We tried every element and none of them worked.  */
                if (got_eacces) {
                        /* At least one failure was due to permissions, so report that
                         * error.
                         */
                        errno = EACCES;
                }

                g_free (freeme);
        }

        /* Return the error from the last attempt (probably ENOENT).  */
        return -1;
}

static gboolean
send_dbus_string_method (DBusConnection *connection,
                         const char     *method,
                         const char     *payload)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter;
        const char     *str;

        if (payload != NULL) {
                str = payload;
        } else {
                str = "";
        }

        g_debug ("MdmSessionWorker: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                MDM_SESSION_DBUS_PATH,
                                                MDM_SESSION_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter,
                                        DBUS_TYPE_STRING,
                                        &str);

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &error);

        dbus_message_unref (message);

        if (dbus_error_is_set (&error)) {
                g_debug ("%s %s raised: %s\n",
                         method,
                         error.name,
                         error.message);
                return FALSE;
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }
        dbus_connection_flush (connection);

        return TRUE;
}

static gboolean
send_dbus_int_method (DBusConnection *connection,
                      const char     *method,
                      int             payload)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter;

        g_debug ("MdmSessionWorker: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                MDM_SESSION_DBUS_PATH,
                                                MDM_SESSION_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter,
                                        DBUS_TYPE_INT32,
                                        &payload);

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &error);
        dbus_message_unref (message);
        if (reply != NULL) {
                dbus_message_unref (reply);
        }
        dbus_connection_flush (connection);

        if (dbus_error_is_set (&error)) {
                g_debug ("%s %s raised: %s\n",
                         method,
                         error.name,
                         error.message);
                return FALSE;
        }

        return TRUE;
}

static gboolean
send_dbus_void_method (DBusConnection *connection,
                       const char     *method)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;

        g_debug ("MdmSessionWorker: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                MDM_SESSION_DBUS_PATH,
                                                MDM_SESSION_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &error);
        dbus_message_unref (message);
        if (reply != NULL) {
                dbus_message_unref (reply);
        }
        dbus_connection_flush (connection);

        if (dbus_error_is_set (&error)) {
                g_debug ("%s %s raised: %s\n",
                         method,
                         error.name,
                         error.message);
                return FALSE;
        }

        return TRUE;
}

static gboolean
mdm_session_worker_get_username (MdmSessionWorker  *worker,
                                 char             **username)
{
        gconstpointer item;

        g_assert (worker->priv->pam_handle != NULL);

        if (pam_get_item (worker->priv->pam_handle, PAM_USER, &item) == PAM_SUCCESS) {
                if (username != NULL) {
                        *username = g_strdup ((char *) item);
                        g_debug ("MdmSessionWorker: username is '%s'",
                                 *username != NULL ? *username : "<unset>");
                }
                return TRUE;
        }

        return FALSE;
}

static void
attempt_to_load_user_settings (MdmSessionWorker *worker,
                               const char       *username)
{
        mdm_session_settings_load (worker->priv->user_settings,
                                   username,
                                   NULL);
}

static void
mdm_session_worker_update_username (MdmSessionWorker *worker)
{
        char    *username;
        gboolean res;

        username = NULL;
        res = mdm_session_worker_get_username (worker, &username);
        if (res) {
                g_debug ("MdmSessionWorker: old-username='%s' new-username='%s'",
                         worker->priv->username != NULL ? worker->priv->username : "<unset>",
                         username != NULL ? username : "<unset>");


                mdm_session_auditor_set_username (worker->priv->auditor, worker->priv->username);

                if ((worker->priv->username == username) ||
                    ((worker->priv->username != NULL) && (username != NULL) &&
                     (strcmp (worker->priv->username, username) == 0)))
                        goto out;

                g_debug ("MdmSessionWorker: setting username to '%s'", username);

                g_free (worker->priv->username);
                worker->priv->username = username;
                username = NULL;

                send_dbus_string_method (worker->priv->connection,
                                         "UsernameChanged",
                                         worker->priv->username);

                /* We have a new username to try. If we haven't been able to
                 * read user settings up until now, then give it a go now
                 * (see the comment in do_setup for rationale on why it's useful
                 * to keep trying to read settings)
                 */
                if (worker->priv->username != NULL &&
                    !mdm_session_settings_is_loaded (worker->priv->user_settings)) {
                        attempt_to_load_user_settings (worker, worker->priv->username);
                }
        }

 out:
        g_free (username);
}

static gboolean
send_question_method (MdmSessionWorker *worker,
                      const char       *method,
                      const char       *question,
                      char            **answerp)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter;
        gboolean        ret;
        const char     *answer;

        ret = FALSE;

        g_debug ("MdmSessionWorker: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                MDM_SESSION_DBUS_PATH,
                                                MDM_SESSION_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter,
                                        DBUS_TYPE_STRING,
                                        &question);

        dbus_error_init (&error);

        /*
         * Pass in INT_MAX for the timeout.  This is a special value that
         * means block forever.  This fixes bug #607861
         */
        reply = dbus_connection_send_with_reply_and_block (worker->priv->connection,
                                                           message,
                                                           INT_MAX,
                                                           &error);
        dbus_message_unref (message);

        if (dbus_error_is_set (&error)) {
                if (dbus_error_has_name (&error, MDM_SESSION_DBUS_ERROR_CANCEL)) {
                        worker->priv->cancelled = TRUE;
                } else if (dbus_error_has_name (&error, DBUS_ERROR_NO_REPLY)) {
                        worker->priv->timed_out = TRUE;
                }
                g_debug ("%s %s raised: %s\n",
                         method,
                         error.name,
                         error.message);
                goto out;
        }

        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &answer);
        if (answerp != NULL) {
                *answerp = g_strdup (answer);
        }
        ret = TRUE;

        dbus_message_unref (reply);
        dbus_connection_flush (worker->priv->connection);

 out:

        return ret;
}

static gboolean
mdm_session_worker_ask_question (MdmSessionWorker *worker,
                                 const char       *question,
                                 char            **answerp)
{
        return send_question_method (worker, "InfoQuery", question, answerp);
}

static gboolean
mdm_session_worker_ask_for_secret (MdmSessionWorker *worker,
                                   const char       *question,
                                   char            **answerp)
{
        return send_question_method (worker, "SecretInfoQuery", question, answerp);
}

static gboolean
mdm_session_worker_report_info (MdmSessionWorker *worker,
                                const char       *info)
{
        return send_dbus_string_method (worker->priv->connection,
                                        "Info",
                                        info);
}

static gboolean
mdm_session_worker_report_problem (MdmSessionWorker *worker,
                                   const char       *problem)
{
        return send_dbus_string_method (worker->priv->connection,
                                        "Problem",
                                        problem);
}

static char *
convert_to_utf8 (const char *str)
{
        char *utf8;
        utf8 = g_locale_to_utf8 (str,
                                 -1,
                                 NULL,
                                 NULL,
                                 NULL);

        /* if we couldn't convert text from locale then
         * assume utf-8 and hope for the best */
        if (utf8 == NULL) {
                char *p;
                char *q;

                utf8 = g_strdup (str);

                p = utf8;
                while (*p != '\0' && !g_utf8_validate ((const char *)p, -1, (const char **)&q)) {
                        *q = '?';
                        p = q + 1;
                }
        }

        return utf8;
}

static gboolean
mdm_session_worker_process_pam_message (MdmSessionWorker          *worker,
                                        const struct pam_message  *query,
                                        char                     **response_text)
{
        char    *user_answer;
        gboolean res;
        char    *utf8_msg;

        if (response_text != NULL) {
                *response_text = NULL;
        }

        mdm_session_worker_update_username (worker);

        g_debug ("MdmSessionWorker: received pam message of type %u with payload '%s'",
                 query->msg_style, query->msg);

        utf8_msg = convert_to_utf8 (query->msg);

        worker->priv->cancelled = FALSE;
        worker->priv->timed_out = FALSE;

        user_answer = NULL;
        res = FALSE;
        switch (query->msg_style) {
        case PAM_PROMPT_ECHO_ON:
                res = mdm_session_worker_ask_question (worker, utf8_msg, &user_answer);
                break;
        case PAM_PROMPT_ECHO_OFF:
                res = mdm_session_worker_ask_for_secret (worker, utf8_msg, &user_answer);
                break;
        case PAM_TEXT_INFO:
                res = mdm_session_worker_report_info (worker, utf8_msg);
                break;
        case PAM_ERROR_MSG:
                res = mdm_session_worker_report_problem (worker, utf8_msg);
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        if (worker->priv->timed_out) {
                send_dbus_void_method (worker->priv->connection, "CancelPendingQuery");
                worker->priv->timed_out = FALSE;
        }

        if (user_answer != NULL) {
                /* we strdup and g_free to make sure we return malloc'd
                 * instead of g_malloc'd memory
                 */
                if (res && response_text != NULL) {
                        *response_text = strdup (user_answer);
                }

                memset (user_answer, '\0', strlen (user_answer));
                g_free (user_answer);

                g_debug ("MdmSessionWorker: trying to get updated username");

                res = TRUE;
        }

        g_free (utf8_msg);

        return res;
}

static int
mdm_session_worker_pam_new_messages_handler (int                        number_of_messages,
                                             const struct pam_message **messages,
                                             struct pam_response      **responses,
                                             MdmSessionWorker          *worker)
{
        struct pam_response *replies;
        int                  return_value;
        int                  i;

        g_debug ("MdmSessionWorker: %d new messages received from PAM\n", number_of_messages);

        return_value = PAM_CONV_ERR;

        if (number_of_messages < 0) {
                return PAM_CONV_ERR;
        }

        if (number_of_messages == 0) {
                if (responses) {
                        *responses = NULL;
                }

                return PAM_SUCCESS;
        }

        /* we want to generate one reply for every question
         */
        replies = (struct pam_response *) calloc (number_of_messages,
                                                  sizeof (struct pam_response));
        for (i = 0; i < number_of_messages; i++) {
                gboolean got_response;
                char    *response_text;

                response_text = NULL;
                got_response = mdm_session_worker_process_pam_message (worker,
                                                                       messages[i],
                                                                       &response_text);
                if (!got_response) {
                        if (response_text != NULL) {
                                memset (response_text, '\0', strlen (response_text));
                                g_free (response_text);
                        }
                        goto out;
                }

                replies[i].resp = response_text;
                replies[i].resp_retcode = PAM_SUCCESS;
        }

        return_value = PAM_SUCCESS;

 out:
        if (return_value != PAM_SUCCESS) {
                for (i = 0; i < number_of_messages; i++) {
                        if (replies[i].resp != NULL) {
                                memset (replies[i].resp, 0, strlen (replies[i].resp));
                                free (replies[i].resp);
                        }
                        memset (&replies[i], 0, sizeof (replies[i]));
                }
                free (replies);
                replies = NULL;
        }

        if (responses) {
                *responses = replies;
        }

        g_debug ("MdmSessionWorker: PAM conversation returning %d: %s",
                 return_value,
                 pam_strerror (worker->priv->pam_handle, return_value));

        return return_value;
}

static void
mdm_session_worker_start_auditor (MdmSessionWorker *worker)
{

/* FIXME: it may make sense at some point to keep a list of
 * auditors, instead of assuming they are mutually exclusive
 */
#if defined (HAVE_ADT)
        worker->priv->auditor = mdm_session_solaris_auditor_new (worker->priv->hostname,
                                                                 worker->priv->display_device);
#elif defined (HAVE_LIBAUDIT)
        worker->priv->auditor = mdm_session_linux_auditor_new (worker->priv->hostname,
                                                               worker->priv->display_device);
#else
        worker->priv->auditor = mdm_session_auditor_new (worker->priv->hostname,
                                                         worker->priv->display_device);
#endif
}

static void
mdm_session_worker_stop_auditor (MdmSessionWorker *worker)
{
        g_object_unref (worker->priv->auditor);
        worker->priv->auditor = NULL;
}

static gboolean
check_user_copy_file (const char *srcfile,
                      const char *destfile,
                      uid_t       user,
                      gssize      max_file_size)
{
        struct stat srcfileinfo;
        struct stat destfileinfo;

        if (max_file_size < 0) {
                max_file_size = G_MAXSIZE;
        }

        /* Exists/Readable? */
        if (g_stat (srcfile, &srcfileinfo) < 0) {
                g_debug ("File does not exist");
                return FALSE;
        }

        /* Is newer than the file already in the cache? */
        if (destfile != NULL && g_stat (destfile, &destfileinfo) == 0) {
                if (srcfileinfo.st_mtime <= destfileinfo.st_mtime) {
                        g_debug ("Destination file is newer");
                        return FALSE;
                }
        }

        /* Is a regular file */
        if (G_UNLIKELY (!S_ISREG (srcfileinfo.st_mode))) {
                g_debug ("File is not a regular file");
                return FALSE;
        }

        /* Owned by user? */
        if (G_UNLIKELY (srcfileinfo.st_uid != user)) {
                g_debug ("File is not owned by user");
                return FALSE;
        }

        /* Size is kosher? */
        if (G_UNLIKELY (srcfileinfo.st_size > max_file_size)) {
                g_debug ("File is too large");
                return FALSE;
        }

        return TRUE;
}

static gboolean
mdm_cache_copy_file (MdmSessionWorker *worker,
                     const char *userfilename,
                     const char *cachefilename)
{
        gboolean res;

        g_debug ("MdmSessionWorker: Checking if %s should be copied to cache %s",
                 userfilename, cachefilename);

        res = check_user_copy_file (userfilename,
                                    cachefilename,
                                    worker->priv->uid,
                                    MAX_FILE_SIZE);

        if (res) {
                 GFile  *src_file;
                 GFile  *dst_file;
                 GError *error;

                 src_file = g_file_new_for_path (userfilename);
                 dst_file = g_file_new_for_path (cachefilename);

                 error = NULL;
                 res = g_file_copy (src_file,
                                    dst_file,
                                    G_FILE_COPY_OVERWRITE |
                                    G_FILE_COPY_NOFOLLOW_SYMLINKS,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &error);

                 if (! res) {
                        g_warning ("Could not copy file to cache: %s",
                                   error->message);
                        g_error_free (error);
                 } else {
                        g_debug ("Copy successful");
                }

                g_object_unref (src_file);
                g_object_unref (dst_file);
        } else {
                g_debug ("Not copying file %s to cache",
                         userfilename);
        }
        return res;
}

static char *
mdm_session_worker_create_cachedir (MdmSessionWorker *worker)
{
        struct stat statbuf;
        char       *cachedir;
        int         r;

        cachedir = g_build_filename (MDM_CACHE_DIR,
                                     worker->priv->username,
                                     NULL);

        /* Verify user cache directory exists, create if needed */
        r = g_stat (cachedir, &statbuf);
        if (r < 0) {
                g_debug ("Making user cache directory %s", cachedir);
                g_mkdir (cachedir,
                         S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH);
                g_chmod (cachedir,
                         S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH);
        }
        r = chown (cachedir, worker->priv->uid, worker->priv->gid);
        if (r == -1) {
                g_warning ("MdmSessionWorker: Error setting owner of cache directory: %s",
                           g_strerror (errno));
        }

        return cachedir;
}

static void
mdm_session_worker_cache_userfiles (MdmSessionWorker *worker)
{
        struct passwd *passwd_entry;
        char          *cachedir;
        char          *cachefile;
        char          *userfile;
        gboolean       res;

        mdm_get_pwent_for_name (worker->priv->username, &passwd_entry);
        if (passwd_entry == NULL)
                return;

        cachedir = mdm_session_worker_create_cachedir (worker);

        g_debug ("Copying user dmrc file to cache");
        cachefile = g_build_filename (cachedir, "dmrc", NULL);
        userfile = g_build_filename (passwd_entry->pw_dir, ".dmrc", NULL);

        mdm_cache_copy_file (worker, userfile, cachefile);
        g_free (cachefile);
        g_free (userfile);

        g_debug ("Copying user face file to cache");
        cachefile = g_build_filename (cachedir,
                                          "face",
                                          NULL);

        /* First, try "~/.face" */
        userfile = g_build_filename (passwd_entry->pw_dir, ".face", NULL);
        res = mdm_cache_copy_file (worker, userfile, cachefile);

        /* Next, try "~/.face.icon" */
        if (!res) {
                g_free (userfile);
                userfile = g_build_filename (passwd_entry->pw_dir,
                                             ".face.icon",
                                             NULL);
                res = mdm_cache_copy_file (worker,
                                           userfile,
                                           cachefile);
        }

        /* Still nothing, try the user's personal MDM config */
        if (!res) {
                char *tempfilename;

                tempfilename = g_build_filename (passwd_entry->pw_dir,
                                                 ".mate",
                                                 "mdm",
                                                 NULL);

                g_debug ("Checking user's ~/.mate/mdm file");
                res = check_user_copy_file (tempfilename,
                                            NULL,
                                            worker->priv->uid,
                                            MAX_FILE_SIZE);
                if (res) {
                        GKeyFile *keyfile;

                        g_free (userfile);

                        keyfile = g_key_file_new ();
                        g_key_file_load_from_file (keyfile,
                                                   userfile,
                                                   G_KEY_FILE_NONE,
                                                   NULL);

                        userfile = g_key_file_get_string (keyfile,
                                                          "face",
                                                          "picture",
                                                          NULL);
                        res = mdm_cache_copy_file (worker,
                                                   userfile,
                                                   cachefile);

                        g_key_file_free (keyfile);
                  }
                  g_free (tempfilename);
        }

        g_free (cachedir);
        g_free (cachefile);
        g_free (userfile);
}

static void
mdm_session_worker_uninitialize_pam (MdmSessionWorker *worker,
                                     int               status)
{
        g_debug ("MdmSessionWorker: uninitializing PAM");

        if (worker->priv->pam_handle == NULL)
                return;

        if (worker->priv->state >= MDM_SESSION_WORKER_STATE_SESSION_OPENED) {
                pid_t pid;

                pid = fork ();

                if (pid == 0) {
                        if (setuid (worker->priv->uid) < 0) {
                                g_debug ("MdmSessionWorker: could not reset uid: %s", g_strerror (errno));
                                _exit (1);
                        }

                        mdm_session_worker_cache_userfiles (worker);
                        _exit (0);
                }

                if (pid > 0) {
                        mdm_wait_on_pid (pid);
                }
                pam_close_session (worker->priv->pam_handle, 0);
                mdm_session_auditor_report_logout (worker->priv->auditor);
        } else {
                const void *p;

                if ((pam_get_item (worker->priv->pam_handle, PAM_USER, &p)) == PAM_SUCCESS) {
                        mdm_session_auditor_set_username (worker->priv->auditor, (const char *)p);
                }

                mdm_session_auditor_report_login_failure (worker->priv->auditor,
                                                          status,
                                                          pam_strerror (worker->priv->pam_handle, status));
        }

        if (worker->priv->state >= MDM_SESSION_WORKER_STATE_ACCREDITED) {
                pam_setcred (worker->priv->pam_handle, PAM_DELETE_CRED);
        }

        pam_end (worker->priv->pam_handle, status);
        worker->priv->pam_handle = NULL;

        mdm_session_worker_stop_auditor (worker);

        g_debug ("MdmSessionWorker: state NONE");
        worker->priv->state = MDM_SESSION_WORKER_STATE_NONE;
}

static char *
_get_tty_for_pam (const char *x11_display_name,
                  const char *display_device)
{
#ifdef __sun
        return g_strdup (display_device);
#else
        return g_strdup (x11_display_name);
#endif
}

#ifdef PAM_XAUTHDATA
static struct pam_xauth_data *
_get_xauth_for_pam (const char *x11_authority_file)
{
        FILE                  *fh;
        Xauth                 *auth = NULL;
        struct pam_xauth_data *retval = NULL;
        gsize                  len = sizeof (*retval) + 1;

        fh = fopen (x11_authority_file, "r");
        if (fh) {
                auth = XauReadAuth (fh);
                fclose (fh);
        }
        if (auth) {
                len += auth->name_length + auth->data_length;
                retval = g_malloc0 (len);
        }
        if (retval) {
                retval->namelen = auth->name_length;
                retval->name = (char *) (retval + 1);
                memcpy (retval->name, auth->name, auth->name_length);
                retval->datalen = auth->data_length;
                retval->data = retval->name + auth->name_length + 1;
                memcpy (retval->data, auth->data, auth->data_length);
        }
        XauDisposeAuth (auth);
        return retval;
}
#endif

static gboolean
mdm_session_worker_initialize_pam (MdmSessionWorker *worker,
                                   const char       *service,
                                   const char       *username,
                                   const char       *hostname,
                                   const char       *x11_display_name,
                                   const char       *x11_authority_file,
                                   const char       *display_device,
                                   GError          **error)
{
        struct pam_conv        pam_conversation;
#ifdef PAM_XAUTHDATA
        struct pam_xauth_data *pam_xauth;
#endif
        int                    error_code;
        char                  *pam_tty;

        g_assert (worker->priv->pam_handle == NULL);

        g_debug ("MdmSessionWorker: initializing PAM");

        pam_conversation.conv = (MdmSessionWorkerPamNewMessagesFunc) mdm_session_worker_pam_new_messages_handler;
        pam_conversation.appdata_ptr = worker;

        mdm_session_worker_start_auditor (worker);
        error_code = pam_start (service,
                                username,
                                &pam_conversation,
                                &worker->priv->pam_handle);

        if (error_code != PAM_SUCCESS) {
                g_debug ("MdmSessionWorker: could not initialize PAM");
                /* we don't use pam_strerror here because it requires a valid
                 * pam handle, and if pam_start fails pam_handle is undefined
                 */
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_AUTHENTICATING,
                             _("error initiating conversation with authentication system: %s"),
                             error_code == PAM_ABORT? _("general failure") :
                             error_code == PAM_BUF_ERR? _("out of memory") :
                             error_code == PAM_SYSTEM_ERR? _("application programmer error") :
                             _("unknown error"));

                goto out;
        }

        /* set USER PROMPT */
        if (username == NULL) {
                error_code = pam_set_item (worker->priv->pam_handle, PAM_USER_PROMPT, _("Username:"));

                if (error_code != PAM_SUCCESS) {
                        g_set_error (error,
                                     MDM_SESSION_WORKER_ERROR,
                                     MDM_SESSION_WORKER_ERROR_AUTHENTICATING,
                                     _("error informing authentication system of preferred username prompt: %s"),
                                     pam_strerror (worker->priv->pam_handle, error_code));
                        goto out;
                }
        }

        /* set RHOST */
        if (hostname != NULL && hostname[0] != '\0') {
                error_code = pam_set_item (worker->priv->pam_handle, PAM_RHOST, hostname);

                if (error_code != PAM_SUCCESS) {
                        g_set_error (error,
                                     MDM_SESSION_WORKER_ERROR,
                                     MDM_SESSION_WORKER_ERROR_AUTHENTICATING,
                                     _("error informing authentication system of user's hostname: %s"),
                                     pam_strerror (worker->priv->pam_handle, error_code));
                        goto out;
                }
        }

        /* set TTY */
        pam_tty = _get_tty_for_pam (x11_display_name, display_device);
        error_code = pam_set_item (worker->priv->pam_handle, PAM_TTY, pam_tty);
        g_free (pam_tty);

        if (error_code != PAM_SUCCESS) {
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_AUTHENTICATING,
                             _("error informing authentication system of user's console: %s"),
                             pam_strerror (worker->priv->pam_handle, error_code));
                goto out;
        }

#ifdef PAM_XDISPLAY
        /* set XDISPLAY */
        error_code = pam_set_item (worker->priv->pam_handle, PAM_XDISPLAY, x11_display_name);

        if (error_code != PAM_SUCCESS) {
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_AUTHENTICATING,
                             _("error informing authentication system of display string: %s"),
                             pam_strerror (worker->priv->pam_handle, error_code));
                goto out;
        }
#endif
#ifdef PAM_XAUTHDATA
        /* set XAUTHDATA */
        pam_xauth = _get_xauth_for_pam (x11_authority_file);
        error_code = pam_set_item (worker->priv->pam_handle, PAM_XAUTHDATA, pam_xauth);
        g_free (pam_xauth);

        if (error_code != PAM_SUCCESS) {
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_AUTHENTICATING,
                             _("error informing authentication system of display xauth credentials: %s"),
                             pam_strerror (worker->priv->pam_handle, error_code));
                goto out;
        }
#endif

        g_debug ("MdmSessionWorker: state SETUP_COMPLETE");
        worker->priv->state = MDM_SESSION_WORKER_STATE_SETUP_COMPLETE;

 out:
        if (error_code != PAM_SUCCESS) {
                mdm_session_worker_uninitialize_pam (worker, error_code);
                return FALSE;
        }

        return TRUE;
}

static gboolean
mdm_session_worker_authenticate_user (MdmSessionWorker *worker,
                                      gboolean          password_is_required,
                                      GError          **error)
{
        int error_code;
        int authentication_flags;

        g_debug ("MdmSessionWorker: authenticating user");

        authentication_flags = 0;

        if (password_is_required) {
                authentication_flags |= PAM_DISALLOW_NULL_AUTHTOK;
        }

        /* blocking call, does the actual conversation */
        error_code = pam_authenticate (worker->priv->pam_handle, authentication_flags);

        if (error_code != PAM_SUCCESS) {
                g_debug ("MdmSessionWorker: authentication returned %d: %s", error_code, pam_strerror (worker->priv->pam_handle, error_code));

                /*
                 * Do not display a different message for user unknown versus
                 * a failed password for a valid user.
                 */
                if (error_code == PAM_USER_UNKNOWN) {
                        error_code = PAM_AUTH_ERR;
                }

                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_AUTHENTICATING,
                             "%s", pam_strerror (worker->priv->pam_handle, error_code));
                goto out;
        }

        g_debug ("MdmSessionWorker: state AUTHENTICATED");
        worker->priv->state = MDM_SESSION_WORKER_STATE_AUTHENTICATED;

 out:
        if (error_code != PAM_SUCCESS) {
                mdm_session_worker_uninitialize_pam (worker, error_code);
                return FALSE;
        }

        return TRUE;
}

static gboolean
mdm_session_worker_authorize_user (MdmSessionWorker *worker,
                                   gboolean          password_is_required,
                                   GError          **error)
{
        int error_code;
        int authentication_flags;

        g_debug ("MdmSessionWorker: determining if authenticated user is authorized to session");

        authentication_flags = 0;

        if (password_is_required) {
                authentication_flags |= PAM_DISALLOW_NULL_AUTHTOK;
        }

        /* check that the account isn't disabled or expired
         */
        error_code = pam_acct_mgmt (worker->priv->pam_handle, authentication_flags);

        /* it's possible that the user needs to change their password or pin code
         */
        if (error_code == PAM_NEW_AUTHTOK_REQD) {
                error_code = pam_chauthtok (worker->priv->pam_handle, PAM_CHANGE_EXPIRED_AUTHTOK);

                if (error_code != PAM_SUCCESS) {
                        mdm_session_auditor_report_password_change_failure (worker->priv->auditor);
                } else {
                        mdm_session_auditor_report_password_changed (worker->priv->auditor);
                }
        }

        if (error_code != PAM_SUCCESS) {
                g_debug ("MdmSessionWorker: user is not authorized to log in: %s",
                         pam_strerror (worker->priv->pam_handle, error_code));
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_AUTHORIZING,
                             "%s", pam_strerror (worker->priv->pam_handle, error_code));
                goto out;
        }

        g_debug ("MdmSessionWorker: state AUTHORIZED");
        worker->priv->state = MDM_SESSION_WORKER_STATE_AUTHORIZED;

 out:
        if (error_code != PAM_SUCCESS) {
                mdm_session_worker_uninitialize_pam (worker, error_code);
                return FALSE;
        }

        return TRUE;
}

static void
mdm_session_worker_set_environment_variable (MdmSessionWorker *worker,
                                             const char       *key,
                                             const char       *value)
{
        /* FIXME: maybe we should use use pam_putenv instead of our
         * own hash table, so pam can override our choices if it knows
         * better?
         *
         * See https://bugzilla.gnome.org/show_bug.cgi?id=627530
         */
        g_hash_table_replace (worker->priv->environment,
                              g_strdup (key),
                              g_strdup (value));
}

static void
mdm_session_worker_update_environment_from_passwd_info (MdmSessionWorker *worker,
                                                        uid_t             uid,
                                                        gid_t             gid,
                                                        const char       *home,
                                                        const char       *shell)
{
        mdm_session_worker_set_environment_variable (worker, "LOGNAME", worker->priv->username);
        mdm_session_worker_set_environment_variable (worker, "USER", worker->priv->username);
        mdm_session_worker_set_environment_variable (worker, "USERNAME", worker->priv->username);
        mdm_session_worker_set_environment_variable (worker, "HOME", home);
        mdm_session_worker_set_environment_variable (worker, "SHELL", shell);
}

static gboolean
mdm_session_worker_environment_variable_is_set (MdmSessionWorker *worker,
                                                const char       *name)
{
        return g_hash_table_lookup (worker->priv->environment, name) != NULL;
}

static gboolean
_change_user (MdmSessionWorker  *worker,
              uid_t              uid,
              gid_t              gid)
{
        gboolean ret;

        ret = FALSE;

#ifdef THE_MAN_PAGE_ISNT_LYING
        /* pam_setcred wants to be called as the authenticated user
         * but pam_open_session needs to be called as super-user.
         *
         * Set the real uid and gid to the user and give the user a
         * temporary super-user effective id.
         */
        if (setreuid (uid, MDM_SESSION_ROOT_UID) < 0) {
                return FALSE;
        }
#endif
        worker->priv->uid = uid;
        worker->priv->gid = gid;

        if (setgid (gid) < 0) {
                return FALSE;
        }

        if (initgroups (worker->priv->username, gid) < 0) {
                return FALSE;
        }

        return TRUE;
}

static gboolean
_lookup_passwd_info (const char *username,
                     uid_t      *uidp,
                     gid_t      *gidp,
                     char      **homep,
                     char      **shellp)
{
        gboolean       ret;
        struct passwd *passwd_entry;
        struct passwd  passwd_buffer;
        char          *aux_buffer;
        long           required_aux_buffer_size;
        gsize          aux_buffer_size;

        ret = FALSE;
        aux_buffer = NULL;
        aux_buffer_size = 0;

        required_aux_buffer_size = sysconf (_SC_GETPW_R_SIZE_MAX);

        if (required_aux_buffer_size < 0) {
                aux_buffer_size = MDM_PASSWD_AUXILLARY_BUFFER_SIZE;
        } else {
                aux_buffer_size = (gsize) required_aux_buffer_size;
        }

        aux_buffer = g_slice_alloc0 (aux_buffer_size);

        /* we use the _r variant of getpwnam()
         * (with its weird semantics) so that the
         * passwd_entry doesn't potentially get stomped on
         * by a PAM module
         */
 again:
        passwd_entry = NULL;
#ifdef HAVE_POSIX_GETPWNAM_R
        errno = getpwnam_r (username,
                            &passwd_buffer,
                            aux_buffer,
                            (size_t) aux_buffer_size,
                            &passwd_entry);
#else
        passwd_entry = getpwnam_r (username,
                                   &passwd_buffer,
                                   aux_buffer,
                                   (size_t) aux_buffer_size);
        errno = 0;
#endif /* !HAVE_POSIX_GETPWNAM_R */
        if (errno == EINTR) {
                g_debug ("%s", g_strerror (errno));
                goto again;
        } else if (errno != 0) {
                g_warning ("%s", g_strerror (errno));
                goto out;
        }

        if (passwd_entry == NULL) {
                goto out;
        }

        if (uidp != NULL) {
                *uidp = passwd_entry->pw_uid;
        }
        if (gidp != NULL) {
                *gidp = passwd_entry->pw_gid;
        }
        if (homep != NULL) {
                *homep = g_strdup (passwd_entry->pw_dir);
        }
        if (shellp != NULL) {
                *shellp = g_strdup (passwd_entry->pw_shell);
        }
        ret = TRUE;
 out:
        if (aux_buffer != NULL) {
                g_assert (aux_buffer_size > 0);
                g_slice_free1 (aux_buffer_size, aux_buffer);
        }

        return ret;
}

static gboolean
mdm_session_worker_accredit_user (MdmSessionWorker  *worker,
                                  GError           **error)
{
        gboolean ret;
        gboolean res;
        uid_t    uid;
        gid_t    gid;
        char    *shell;
        char    *home;
        int      error_code;

        ret = FALSE;

        home = NULL;
        shell = NULL;

        if (worker->priv->username == NULL) {
                g_debug ("MdmSessionWorker: Username not set");
                error_code = PAM_USER_UNKNOWN;
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_GIVING_CREDENTIALS,
                             _("no user account available"));
                goto out;
        }

        uid = 0;
        gid = 0;
        res = _lookup_passwd_info (worker->priv->username,
                                   &uid,
                                   &gid,
                                   &home,
                                   &shell);
        if (! res) {
                g_debug ("MdmSessionWorker: Unable to lookup account info");
                error_code = PAM_AUTHINFO_UNAVAIL;
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_GIVING_CREDENTIALS,
                             _("no user account available"));
                goto out;
        }

        mdm_session_worker_update_environment_from_passwd_info (worker,
                                                                uid,
                                                                gid,
                                                                home,
                                                                shell);

        /* Let's give the user a default PATH if he doesn't already have one
         */
        if (!mdm_session_worker_environment_variable_is_set (worker, "PATH")) {
                if (strcmp (BINDIR, "/usr/bin") == 0) {
                        mdm_session_worker_set_environment_variable (worker, "PATH",
                                                                     MDM_SESSION_DEFAULT_PATH);
                } else {
                        mdm_session_worker_set_environment_variable (worker, "PATH",
                                                                     BINDIR ":" MDM_SESSION_DEFAULT_PATH);
                }
        }

        if (! _change_user (worker, uid, gid)) {
                g_debug ("MdmSessionWorker: Unable to change to user");
                error_code = PAM_SYSTEM_ERR;
                g_set_error (error, MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_GIVING_CREDENTIALS,
                             "%s", _("Unable to change to user"));
                goto out;
        }

        error_code = pam_setcred (worker->priv->pam_handle, worker->priv->cred_flags);

        if (error_code != PAM_SUCCESS) {
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_GIVING_CREDENTIALS,
                             "%s",
                             pam_strerror (worker->priv->pam_handle, error_code));
                goto out;
        }

        ret = TRUE;

 out:
        g_free (home);
        g_free (shell);
        if (ret) {
                g_debug ("MdmSessionWorker: state ACCREDITED");
                ret = TRUE;
                mdm_session_auditor_report_user_accredited (worker->priv->auditor);
                worker->priv->state = MDM_SESSION_WORKER_STATE_ACCREDITED;
        } else {
                mdm_session_worker_uninitialize_pam (worker, error_code);
        }

        return ret;
}

static void
mdm_session_worker_update_environment_from_pam (MdmSessionWorker *worker)
{
        char **environment;
        gsize i;

        environment = pam_getenvlist (worker->priv->pam_handle);

        for (i = 0; environment[i] != NULL; i++) {
                char **key_and_value;

                key_and_value = g_strsplit (environment[i], "=", 2);

                mdm_session_worker_set_environment_variable (worker, key_and_value[0], key_and_value[1]);

                g_strfreev (key_and_value);
        }

        for (i = 0; environment[i]; i++) {
                free (environment[i]);
        }

        free (environment);
}

static void
mdm_session_worker_fill_environment_array (const char *key,
                                           const char *value,
                                           GPtrArray  *environment)
{
        char *variable;

        if (value == NULL)
                return;

        variable = g_strdup_printf ("%s=%s", key, value);

        g_ptr_array_add (environment, variable);
}

static char **
mdm_session_worker_get_environment (MdmSessionWorker *worker)
{
        GPtrArray *environment;

        environment = g_ptr_array_new ();
        g_hash_table_foreach (worker->priv->environment,
                              (GHFunc) mdm_session_worker_fill_environment_array,
                              environment);
        g_ptr_array_add (environment, NULL);

        return (char **) g_ptr_array_free (environment, FALSE);
}

static void
register_ck_session (MdmSessionWorker *worker)
{
        const char *session_cookie;
        gboolean    res;

        session_cookie = NULL;
        res = open_ck_session (worker);
        if (res) {
                session_cookie = ck_connector_get_cookie (worker->priv->ckc);
        }
        if (session_cookie != NULL) {
                mdm_session_worker_set_environment_variable (worker,
                                                             "XDG_SESSION_COOKIE",
                                                             session_cookie);
        }
}

static void
session_worker_child_watch (GPid              pid,
                            int               status,
                            MdmSessionWorker *worker)
{
        g_debug ("MdmSessionWorker: child (pid:%d) done (%s:%d)",
                 (int) pid,
                 WIFEXITED (status) ? "status"
                 : WIFSIGNALED (status) ? "signal"
                 : "unknown",
                 WIFEXITED (status) ? WEXITSTATUS (status)
                 : WIFSIGNALED (status) ? WTERMSIG (status)
                 : -1);

        if (WIFEXITED (status)) {
                int code = WEXITSTATUS (status);

                send_dbus_int_method (worker->priv->connection,
                                      "SessionExited",
                                      code);
        } else if (WIFSIGNALED (status)) {
                int num = WTERMSIG (status);

                send_dbus_int_method (worker->priv->connection,
                                      "SessionDied",
                                      num);
        }

        if (worker->priv->ckc != NULL) {
                ck_connector_close_session (worker->priv->ckc, NULL);
                ck_connector_unref (worker->priv->ckc);
                worker->priv->ckc = NULL;
        }

        mdm_session_worker_uninitialize_pam (worker, PAM_SUCCESS);

        worker->priv->child_pid = -1;
}

static void
mdm_session_worker_watch_child (MdmSessionWorker *worker)
{

        worker->priv->child_watch_id = g_child_watch_add (worker->priv->child_pid,
                                                          (GChildWatchFunc)session_worker_child_watch,
                                                          worker);

}

static gboolean
_fd_is_normal_file (int fd)
{
        struct stat file_info;

        if (fstat (fd, &file_info) < 0) {
                return FALSE;
        }

        return S_ISREG (file_info.st_mode);
}

static int
_open_session_log (const char *dir)
{
        int   fd;
        char *filename;

        filename = g_build_filename (dir, MDM_SESSION_LOG_FILENAME, NULL);

        if (g_access (dir, R_OK | W_OK | X_OK) == 0 && g_access (filename, R_OK | W_OK) == 0) {
                char *filename_old;

                filename_old = g_strdup_printf ("%s.old", filename);
                g_rename (filename, filename_old);
                g_free (filename_old);
        }

        fd = g_open (filename, O_RDWR | O_APPEND | O_CREAT, 0600);

        if (fd < 0 || !_fd_is_normal_file (fd)) {
                char *temp_name;

                close (fd);

                temp_name = g_strdup_printf ("%s.XXXXXXXX", filename);

                fd = g_mkstemp (temp_name);

                if (fd < 0) {
                        g_free (temp_name);
                        goto out;
                }

                g_warning ("session log '%s' is not a normal file, logging session to '%s' instead.\n", filename,
                           temp_name);
                g_free (filename);
                filename = temp_name;
        } else {
                if (ftruncate (fd, 0) < 0) {
                        close (fd);
                        fd = -1;
                        goto out;
                }
        }

        if (fchmod (fd, 0600) < 0) {
                close (fd);
                fd = -1;
                goto out;
        }


out:
        g_free (filename);

        if (fd < 0) {
                g_warning ("unable to log session");
                fd = g_open ("/dev/null", O_RDWR);
        }

        return fd;
}

static void
_save_user_settings (MdmSessionWorker *worker,
                     const char       *home_dir)
{
        GError *error;

        if (!mdm_session_settings_is_loaded (worker->priv->user_settings)) {
                /*
                 * Even if the user did not change the defaults, there may
                 * be files to cache
                 */
                goto out;
        }

        error = NULL;
        if (!mdm_session_settings_save (worker->priv->user_settings,
                                        home_dir, &error)) {
                g_warning ("could not save session and language settings: %s",
                           error->message);
                g_error_free (error);
        }

out:
        mdm_session_worker_cache_userfiles (worker);
}

static gboolean
mdm_session_worker_start_user_session (MdmSessionWorker  *worker,
                                       GError           **error)
{
        struct passwd *passwd_entry;
        pid_t session_pid;
        int   error_code;

        g_debug ("MdmSessionWorker: querying pam for user environment");
        mdm_session_worker_update_environment_from_pam (worker);

        register_ck_session (worker);

        mdm_get_pwent_for_name (worker->priv->username, &passwd_entry);
        g_debug ("MdmSessionWorker: opening user session with program '%s'",
                 worker->priv->arguments[0]);

        error_code = PAM_SUCCESS;

        session_pid = fork ();

        if (session_pid < 0) {
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_OPENING_SESSION,
                             "%s", g_strerror (errno));
                error_code = PAM_ABORT;
                goto out;
        }

        if (session_pid == 0) {
                char **environment;
                char  *cachedirname;
                char  *home_dir;
                int    fd;

                /* Make sure cachedir gets created before we drop to user */
                cachedirname = mdm_session_worker_create_cachedir (worker);
                g_free (cachedirname);

                if (setuid (worker->priv->uid) < 0) {
                        g_debug ("MdmSessionWorker: could not reset uid: %s", g_strerror (errno));
                        _exit (1);
                }

                if (setsid () < 0) {
                        g_debug ("MdmSessionWorker: could not set pid '%u' as leader of new session and process group: %s",
                                 (guint) getpid (), g_strerror (errno));
                        _exit (2);
                }

                environment = mdm_session_worker_get_environment (worker);

                g_assert (geteuid () == getuid ());

                home_dir = g_hash_table_lookup (worker->priv->environment,
                                                "HOME");

                if ((home_dir == NULL) || g_chdir (home_dir) < 0) {
                        g_chdir ("/");
                }

                fd = open ("/dev/null", O_RDWR);
                dup2 (fd, STDIN_FILENO);
                close (fd);

                fd = _open_session_log (home_dir);
                dup2 (fd, STDOUT_FILENO);
                dup2 (fd, STDERR_FILENO);
                close (fd);

                _save_user_settings (worker, home_dir);

                mdm_session_execute (worker->priv->arguments[0],
                                     worker->priv->arguments,
                                     environment,
                                     TRUE);

                g_debug ("MdmSessionWorker: child '%s' could not be started: %s",
                         worker->priv->arguments[0],
                         g_strerror (errno));
                g_strfreev (environment);

                _exit (127);
        }

        worker->priv->child_pid = session_pid;

        g_debug ("MdmSessionWorker: session opened creating reply...");
        g_assert (sizeof (GPid) <= sizeof (int));

        g_debug ("MdmSessionWorker: state SESSION_STARTED");
        worker->priv->state = MDM_SESSION_WORKER_STATE_SESSION_STARTED;

        mdm_session_worker_watch_child (worker);

 out:
        if (error_code != PAM_SUCCESS) {
                mdm_session_worker_uninitialize_pam (worker, error_code);
                return FALSE;
        }

        return TRUE;
}

static gboolean
mdm_session_worker_open_user_session (MdmSessionWorker  *worker,
                                      GError           **error)
{
        int error_code;

        g_assert (worker->priv->state == MDM_SESSION_WORKER_STATE_ACCREDITED);
        g_assert (geteuid () == 0);

        error_code = pam_open_session (worker->priv->pam_handle, 0);

        if (error_code != PAM_SUCCESS) {
                g_set_error (error,
                             MDM_SESSION_WORKER_ERROR,
                             MDM_SESSION_WORKER_ERROR_OPENING_SESSION,
                             "%s", pam_strerror (worker->priv->pam_handle, error_code));
                goto out;
        }

        g_debug ("MdmSessionWorker: state SESSION_OPENED");
        worker->priv->state = MDM_SESSION_WORKER_STATE_SESSION_OPENED;

 out:
        if (error_code != PAM_SUCCESS) {
                mdm_session_worker_uninitialize_pam (worker, error_code);
                return FALSE;
        }

        mdm_session_auditor_report_login (worker->priv->auditor);

        return TRUE;
}

static void
mdm_session_worker_set_server_address (MdmSessionWorker *worker,
                                       const char       *address)
{
        g_free (worker->priv->server_address);
        worker->priv->server_address = g_strdup (address);
}

static void
mdm_session_worker_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        MdmSessionWorker *self;

        self = MDM_SESSION_WORKER (object);

        switch (prop_id) {
        case PROP_SERVER_ADDRESS:
                mdm_session_worker_set_server_address (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_session_worker_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        MdmSessionWorker *self;

        self = MDM_SESSION_WORKER (object);

        switch (prop_id) {
        case PROP_SERVER_ADDRESS:
                g_value_set_string (value, self->priv->server_address);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
on_set_environment_variable (MdmSessionWorker *worker,
                             DBusMessage      *message)
{
        DBusError   error;
        const char *key;
        const char *value;
        dbus_bool_t res;

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &key,
                                     DBUS_TYPE_STRING, &value,
                                     DBUS_TYPE_INVALID);
        if (res) {
                g_debug ("MdmSessionWorker: set env: %s = %s", key, value);
                mdm_session_worker_set_environment_variable (worker, key, value);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
mdm_session_worker_set_session_name (MdmSessionWorker *worker,
                                     const char       *session_name)
{
        mdm_session_settings_set_session_name (worker->priv->user_settings,
                                               session_name);
}

static void
on_set_session_name (MdmSessionWorker *worker,
                     DBusMessage      *message)
{
        DBusError   error;
        const char *session_name;
        dbus_bool_t res;

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &session_name,
                                     DBUS_TYPE_INVALID);
        if (res) {
                g_debug ("MdmSessionWorker: session name set to %s", session_name);
                mdm_session_worker_set_session_name (worker, session_name);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
mdm_session_worker_set_language_name (MdmSessionWorker *worker,
                                      const char       *language_name)
{
        mdm_session_settings_set_language_name (worker->priv->user_settings,
                                                language_name);
}

static void
mdm_session_worker_set_layout_name (MdmSessionWorker *worker,
                                    const char       *layout_name)
{
        mdm_session_settings_set_layout_name (worker->priv->user_settings,
                                              layout_name);
}

static void
on_set_language_name (MdmSessionWorker *worker,
                      DBusMessage      *message)
{
        DBusError   error;
        const char *language_name;
        dbus_bool_t res;

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &language_name,
                                     DBUS_TYPE_INVALID);
        if (res) {
                g_debug ("MdmSessionWorker: language name set to %s", language_name);
                mdm_session_worker_set_language_name (worker, language_name);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_set_layout_name (MdmSessionWorker *worker,
                    DBusMessage      *message)
{
        DBusError   error;
        const char *layout_name;
        dbus_bool_t res;

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &layout_name,
                                     DBUS_TYPE_INVALID);
        if (res) {
                g_debug ("MdmSessionWorker: layout name set to %s", layout_name);
                mdm_session_worker_set_layout_name (worker, layout_name);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_saved_language_name_read (MdmSessionWorker *worker)
{
        char *language_name;

        language_name = mdm_session_settings_get_language_name (worker->priv->user_settings);
        send_dbus_string_method (worker->priv->connection,
                                 "SavedLanguageNameRead",
                                 language_name);
        g_free (language_name);
}

static void
on_saved_layout_name_read (MdmSessionWorker *worker)
{
        char *layout_name;

        layout_name = mdm_session_settings_get_layout_name (worker->priv->user_settings);
        send_dbus_string_method (worker->priv->connection,
                                 "SavedLayoutNameRead",
                                 layout_name);
        g_free (layout_name);
}

static void
on_saved_session_name_read (MdmSessionWorker *worker)
{
        char *session_name;

        session_name = mdm_session_settings_get_session_name (worker->priv->user_settings);
        send_dbus_string_method (worker->priv->connection,
                                 "SavedSessionNameRead",
                                 session_name);
        g_free (session_name);
}

static void
do_setup (MdmSessionWorker *worker)
{
        GError  *error;
        gboolean res;

        worker->priv->user_settings = mdm_session_settings_new ();

        g_signal_connect_swapped (worker->priv->user_settings,
                                  "notify::language-name",
                                  G_CALLBACK (on_saved_language_name_read),
                                  worker);

        g_signal_connect_swapped (worker->priv->user_settings,
                                  "notify::layout-name",
                                  G_CALLBACK (on_saved_layout_name_read),
                                  worker);

        g_signal_connect_swapped (worker->priv->user_settings,
                                  "notify::session-name",
                                  G_CALLBACK (on_saved_session_name_read),
                                  worker);

        /* In some setups the user can read ~/.dmrc at this point.
         * In some other setups the user can only read ~/.dmrc after completing
         * the pam conversation.
         *
         * The user experience is better if we can read .dmrc now since we can
         * prefill in the language and session combo boxes in the greeter with
         * the right values.
         *
         * We'll try now, and if it doesn't work out, try later.
         */
        if (worker->priv->username != NULL) {
                attempt_to_load_user_settings (worker,
                                               worker->priv->username);
        }

        error = NULL;
        res = mdm_session_worker_initialize_pam (worker,
                                                 worker->priv->service,
                                                 worker->priv->username,
                                                 worker->priv->hostname,
                                                 worker->priv->x11_display_name,
                                                 worker->priv->x11_authority_file,
                                                 worker->priv->display_device,
                                                 &error);
        if (! res) {
                send_dbus_string_method (worker->priv->connection,
                                         "SetupFailed",
                                         error->message);
                g_error_free (error);
                return;
        }

        send_dbus_void_method (worker->priv->connection, "SetupComplete");
}

static void
do_authenticate (MdmSessionWorker *worker)
{
        GError  *error;
        gboolean res;

        /* find out who the user is and ensure they are who they say they are
         */
        error = NULL;
        res = mdm_session_worker_authenticate_user (worker,
                                                    worker->priv->password_is_required,
                                                    &error);
        if (! res) {
                g_debug ("MdmSessionWorker: Unable to verify user");
                send_dbus_string_method (worker->priv->connection,
                                         "AuthenticationFailed",
                                         error->message);
                g_error_free (error);
                return;
        }

        /* we're authenticated.  Let's make sure we've been given
         * a valid username for the system
         */
        g_debug ("MdmSessionWorker: trying to get updated username");
        mdm_session_worker_update_username (worker);

        send_dbus_void_method (worker->priv->connection, "Authenticated");
}

static void
do_authorize (MdmSessionWorker *worker)
{
        GError  *error;
        gboolean res;

        /* make sure the user is allowed to log in to this system
         */
        error = NULL;
        res = mdm_session_worker_authorize_user (worker,
                                                 worker->priv->password_is_required,
                                                 &error);
        if (! res) {
                send_dbus_string_method (worker->priv->connection,
                                         "AuthorizationFailed",
                                         error->message);
                g_error_free (error);
                return;
        }

        send_dbus_void_method (worker->priv->connection, "Authorized");
}

static void
do_accredit (MdmSessionWorker *worker)
{
        GError  *error;
        gboolean res;

        /* get kerberos tickets, setup group lists, etc
         */
        error = NULL;
        res = mdm_session_worker_accredit_user (worker, &error);

        if (! res) {
                send_dbus_string_method (worker->priv->connection,
                                         "AccreditationFailed",
                                         error->message);
                g_error_free (error);
                return;
        }

        send_dbus_void_method (worker->priv->connection, "Accredited");
}

static void
do_open_session (MdmSessionWorker *worker)
{
        GError  *error;
        gboolean res;

        error = NULL;
        res = mdm_session_worker_open_user_session (worker, &error);
        if (! res) {
                send_dbus_string_method (worker->priv->connection,
                                         "OpenFailed",
                                         error->message);
                g_error_free (error);
                return;
        }

        send_dbus_void_method (worker->priv->connection, "SessionOpened");
}

static void
do_start_session (MdmSessionWorker *worker)
{
        GError  *error;
        gboolean res;

        error = NULL;
        res = mdm_session_worker_start_user_session (worker, &error);
        if (! res) {
                send_dbus_string_method (worker->priv->connection,
                                         "StartFailed",
                                         error->message);
                g_error_free (error);
                return;
        }

        send_dbus_int_method (worker->priv->connection,
                              "SessionStarted",
                              (int)worker->priv->child_pid);
}

static const char *
get_state_name (int state)
{
        const char *name;

        name = NULL;

        switch (state) {
        case MDM_SESSION_WORKER_STATE_NONE:
                name = "NONE";
                break;
        case MDM_SESSION_WORKER_STATE_SETUP_COMPLETE:
                name = "SETUP_COMPLETE";
                break;
        case MDM_SESSION_WORKER_STATE_AUTHENTICATED:
                name = "AUTHENTICATED";
                break;
        case MDM_SESSION_WORKER_STATE_AUTHORIZED:
                name = "AUTHORIZED";
                break;
        case MDM_SESSION_WORKER_STATE_ACCREDITED:
                name = "ACCREDITED";
                break;
        case MDM_SESSION_WORKER_STATE_SESSION_OPENED:
                name = "SESSION_OPENED";
                break;
        case MDM_SESSION_WORKER_STATE_SESSION_STARTED:
                name = "SESSION_STARTED";
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        return name;
}

static gboolean
state_change_idle (MdmSessionWorker *worker)
{
        int new_state;

        new_state = worker->priv->state + 1;
        g_debug ("MdmSessionWorker: attempting to change state to %s",
                 get_state_name (new_state));

        worker->priv->state_change_idle_id = 0;

        switch (new_state) {
        case MDM_SESSION_WORKER_STATE_SETUP_COMPLETE:
                do_setup (worker);
                break;
        case MDM_SESSION_WORKER_STATE_AUTHENTICATED:
                do_authenticate (worker);
                break;
        case MDM_SESSION_WORKER_STATE_AUTHORIZED:
                do_authorize (worker);
                break;
        case MDM_SESSION_WORKER_STATE_ACCREDITED:
                do_accredit (worker);
                break;
        case MDM_SESSION_WORKER_STATE_SESSION_OPENED:
                do_open_session (worker);
                break;
        case MDM_SESSION_WORKER_STATE_SESSION_STARTED:
                do_start_session (worker);
                break;
        case MDM_SESSION_WORKER_STATE_NONE:
        default:
                g_assert_not_reached ();
        }
        return FALSE;
}

static void
queue_state_change (MdmSessionWorker *worker)
{
        if (worker->priv->state_change_idle_id > 0) {
                return;
        }

        worker->priv->state_change_idle_id = g_idle_add ((GSourceFunc)state_change_idle, worker);
}

static void
on_start_program (MdmSessionWorker *worker,
                  DBusMessage      *message)
{
        DBusError   error;
        const char *text;
        dbus_bool_t res;

        if (worker->priv->state != MDM_SESSION_WORKER_STATE_SESSION_OPENED) {
                g_debug ("MdmSessionWorker: ignoring spurious start program while in state %s", get_state_name (worker->priv->state));
                return;
        }

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID);
        if (res) {
                GError *parse_error;

                g_debug ("MdmSessionWorker: start program: %s", text);

                if (worker->priv->arguments != NULL) {
                        g_strfreev (worker->priv->arguments);
                        worker->priv->arguments = NULL;
                }

                parse_error = NULL;
                if (! g_shell_parse_argv (text, NULL, &worker->priv->arguments, &parse_error)) {
                        g_warning ("Unable to parse command: %s", parse_error->message);
                        g_error_free (parse_error);
                        return;
                }

                queue_state_change (worker);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_setup (MdmSessionWorker *worker,
          DBusMessage      *message)
{
        DBusError   error;
        const char *service;
        const char *x11_display_name;
        const char *x11_authority_file;
        const char *console;
        const char *hostname;
        dbus_bool_t res;

        if (worker->priv->state != MDM_SESSION_WORKER_STATE_NONE) {
                g_debug ("MdmSessionWorker: ignoring spurious setup while in state %s", get_state_name (worker->priv->state));
                return;
        }

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &service,
                                     DBUS_TYPE_STRING, &x11_display_name,
                                     DBUS_TYPE_STRING, &console,
                                     DBUS_TYPE_STRING, &hostname,
                                     DBUS_TYPE_STRING, &x11_authority_file,
                                     DBUS_TYPE_INVALID);
        if (res) {
                worker->priv->service = g_strdup (service);
                worker->priv->x11_display_name = g_strdup (x11_display_name);
                worker->priv->x11_authority_file = g_strdup (x11_authority_file);
                worker->priv->display_device = g_strdup (console);
                worker->priv->hostname = g_strdup (hostname);
                worker->priv->username = NULL;

                g_debug ("MdmSessionWorker: queuing setup: %s %s", service, console);
                queue_state_change (worker);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_setup_for_user (MdmSessionWorker *worker,
                   DBusMessage      *message)
{
        DBusError   error;
        const char *service;
        const char *x11_display_name;
        const char *x11_authority_file;
        const char *console;
        const char *hostname;
        const char *username;
        dbus_bool_t res;

        if (worker->priv->state != MDM_SESSION_WORKER_STATE_NONE) {
                g_debug ("MdmSessionWorker: ignoring spurious setup for user while in state %s", get_state_name (worker->priv->state));
                return;
        }

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &service,
                                     DBUS_TYPE_STRING, &x11_display_name,
                                     DBUS_TYPE_STRING, &console,
                                     DBUS_TYPE_STRING, &hostname,
                                     DBUS_TYPE_STRING, &x11_authority_file,
                                     DBUS_TYPE_STRING, &username,
                                     DBUS_TYPE_INVALID);
        if (res) {
                worker->priv->service = g_strdup (service);
                worker->priv->x11_display_name = g_strdup (x11_display_name);
                worker->priv->x11_authority_file = g_strdup (x11_authority_file);
                worker->priv->display_device = g_strdup (console);
                worker->priv->hostname = g_strdup (hostname);
                worker->priv->username = g_strdup (username);

                g_debug ("MdmSessionWorker: queuing setup for user: %s %s", service, console);
                queue_state_change (worker);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_authenticate (MdmSessionWorker *worker,
                 DBusMessage      *message)
{
        if (worker->priv->state != MDM_SESSION_WORKER_STATE_SETUP_COMPLETE) {
                g_debug ("MdmSessionWorker: ignoring spurious authenticate for user while in state %s", get_state_name (worker->priv->state));
                return;
        }

        queue_state_change (worker);
}

static void
on_authorize (MdmSessionWorker *worker,
              DBusMessage      *message)
{
        if (worker->priv->state != MDM_SESSION_WORKER_STATE_AUTHENTICATED) {
                g_debug ("MdmSessionWorker: ignoring spurious authorize for user while in state %s", get_state_name (worker->priv->state));
                return;
        }

        queue_state_change (worker);
}

static void
on_establish_credentials (MdmSessionWorker *worker,
                          DBusMessage      *message)
{
        if (worker->priv->state != MDM_SESSION_WORKER_STATE_AUTHORIZED) {
                g_debug ("MdmSessionWorker: ignoring spurious establish credentials for user while in state %s", get_state_name (worker->priv->state));
                return;
        }

        worker->priv->cred_flags = PAM_ESTABLISH_CRED;

        queue_state_change (worker);
}

static void
on_open_session (MdmSessionWorker *worker,
                 DBusMessage      *message)
{
        if (worker->priv->state != MDM_SESSION_WORKER_STATE_ACCREDITED) {
                g_debug ("MdmSessionWorker: ignoring spurious open session for user while in state %s", get_state_name (worker->priv->state));
                return;
        }

        queue_state_change (worker);
}

static void
on_reauthenticate (MdmSessionWorker *worker,
                   DBusMessage      *message)
{
        if (worker->priv->state != MDM_SESSION_WORKER_STATE_SESSION_STARTED) {
                g_debug ("MdmSessionWorker: ignoring spurious reauthenticate for user while in state %s", get_state_name (worker->priv->state));
                return;
        }

        queue_state_change (worker);
}

static void
on_reauthorize (MdmSessionWorker *worker,
                DBusMessage      *message)
{
        if (worker->priv->state != MDM_SESSION_WORKER_STATE_REAUTHENTICATED) {
                g_debug ("MdmSessionWorker: ignoring spurious reauthorize for user while in state %s", get_state_name (worker->priv->state));
                return;
        }

        queue_state_change (worker);
}

static void
on_refresh_credentials (MdmSessionWorker *worker,
                        DBusMessage      *message)
{
        int error_code;

        if (worker->priv->state != MDM_SESSION_WORKER_STATE_REAUTHORIZED) {
                g_debug ("MdmSessionWorker: ignoring spurious refreshing credentials for user while in state %s", get_state_name (worker->priv->state));
                return;
        }

        g_debug ("MdmSessionWorker: refreshing credentials");

        error_code = pam_setcred (worker->priv->pam_handle, PAM_REFRESH_CRED);
        if (error_code != PAM_SUCCESS) {
                g_debug ("MdmSessionWorker: %s", pam_strerror (worker->priv->pam_handle, error_code));
        }
}

static DBusHandlerResult
worker_dbus_handle_message (DBusConnection *connection,
                            DBusMessage    *message,
                            void           *user_data,
                            dbus_bool_t     local_interface)
{
        MdmSessionWorker *worker = MDM_SESSION_WORKER (user_data);

#if 0
        g_message ("obj_path=%s interface=%s method=%s destination=%s",
                   dbus_message_get_path (message),
                   dbus_message_get_interface (message),
                   dbus_message_get_member (message),
                   dbus_message_get_destination (message));
#endif

        g_return_val_if_fail (connection != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
        g_return_val_if_fail (message != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

        if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "Setup")) {
                on_setup (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "SetupForUser")) {
                on_setup_for_user (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "Authenticate")) {
                on_authenticate (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "Authorize")) {
                on_authorize (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "EstablishCredentials")) {
                on_establish_credentials (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "OpenSession")) {
                on_open_session (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "StartProgram")) {
                on_start_program (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "Reauthenticate")) {
                on_reauthenticate (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "Reauthorize")) {
                on_reauthorize (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "RefreshCredentials")) {
                on_refresh_credentials (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "SetEnvironmentVariable")) {
                on_set_environment_variable (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "SetLanguageName")) {
                on_set_language_name (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "SetLayoutName")) {
                on_set_layout_name (worker, message);
        } else if (dbus_message_is_signal (message, MDM_SESSION_DBUS_INTERFACE, "SetSessionName")) {
                on_set_session_name (worker, message);
        } else {
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
worker_dbus_filter_function (DBusConnection *connection,
                             DBusMessage    *message,
                             void           *user_data)
{
        MdmSessionWorker *worker = MDM_SESSION_WORKER (user_data);
        const char       *path;

        path = dbus_message_get_path (message);

        g_debug ("MdmSessionWorker: obj_path=%s interface=%s method=%s",
                 dbus_message_get_path (message),
                 dbus_message_get_interface (message),
                 dbus_message_get_member (message));

        if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected")
            && strcmp (path, DBUS_PATH_LOCAL) == 0) {

                g_debug ("MdmSessionWorker: Got disconnected from the server");

                dbus_connection_unref (connection);
                worker->priv->connection = NULL;

        } else if (dbus_message_is_signal (message,
                                           DBUS_INTERFACE_DBUS,
                                           "NameOwnerChanged")) {
                g_debug ("MdmSessionWorker: Name owner changed?");
        } else {
                return worker_dbus_handle_message (connection, message, user_data, FALSE);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

static GObject *
mdm_session_worker_constructor (GType                  type,
                                guint                  n_construct_properties,
                                GObjectConstructParam *construct_properties)
{
        MdmSessionWorker      *worker;
        DBusError              error;

        worker = MDM_SESSION_WORKER (G_OBJECT_CLASS (mdm_session_worker_parent_class)->constructor (type,
                                                                                                    n_construct_properties,
                                                                                                    construct_properties));

        g_debug ("MdmSessionWorker: connecting to address: %s", worker->priv->server_address);

        dbus_error_init (&error);
        worker->priv->connection = dbus_connection_open (worker->priv->server_address, &error);
        if (worker->priv->connection == NULL) {
                if (dbus_error_is_set (&error)) {
                        g_warning ("error opening connection: %s", error.message);
                        dbus_error_free (&error);
                } else {
                        g_warning ("Unable to open connection");
                }
                exit (1);
        }

        dbus_connection_setup_with_g_main (worker->priv->connection, NULL);
        dbus_connection_set_exit_on_disconnect (worker->priv->connection, TRUE);

        dbus_connection_add_filter (worker->priv->connection,
                                    worker_dbus_filter_function,
                                    worker,
                                    NULL);

        return G_OBJECT (worker);
}

static void
mdm_session_worker_class_init (MdmSessionWorkerClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_session_worker_get_property;
        object_class->set_property = mdm_session_worker_set_property;
        object_class->constructor = mdm_session_worker_constructor;
        object_class->finalize = mdm_session_worker_finalize;

        g_type_class_add_private (klass, sizeof (MdmSessionWorkerPrivate));

        g_object_class_install_property (object_class,
                                         PROP_SERVER_ADDRESS,
                                         g_param_spec_string ("server-address",
                                                              "server address",
                                                              "server address",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
mdm_session_worker_init (MdmSessionWorker *worker)
{

        worker->priv = MDM_SESSION_WORKER_GET_PRIVATE (worker);
        worker->priv->environment = g_hash_table_new_full (g_str_hash,
                                                           g_str_equal,
                                                           (GDestroyNotify) g_free,
                                                           (GDestroyNotify) g_free);
}

static void
mdm_session_worker_unwatch_child (MdmSessionWorker *worker)
{
        if (worker->priv->child_watch_id == 0)
                return;

        g_source_remove (worker->priv->child_watch_id);
        worker->priv->child_watch_id = 0;
}


static void
mdm_session_worker_finalize (GObject *object)
{
        MdmSessionWorker *worker;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_SESSION_WORKER (object));

        worker = MDM_SESSION_WORKER (object);

        g_return_if_fail (worker->priv != NULL);

        mdm_session_worker_unwatch_child (worker);

        g_free (worker->priv->service);
        g_free (worker->priv->x11_display_name);
        g_free (worker->priv->x11_authority_file);
        g_free (worker->priv->display_device);
        g_free (worker->priv->hostname);
        g_free (worker->priv->username);
        g_free (worker->priv->server_address);
        g_strfreev (worker->priv->arguments);
        if (worker->priv->environment != NULL) {
                g_hash_table_destroy (worker->priv->environment);
        }

        G_OBJECT_CLASS (mdm_session_worker_parent_class)->finalize (object);
}

MdmSessionWorker *
mdm_session_worker_new (const char *address)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_SESSION_WORKER,
                               "server-address", address,
                               NULL);

        return MDM_SESSION_WORKER (object);
}

