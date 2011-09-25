/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
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
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <X11/Xlib.h> /* for Display */
#include <X11/cursorfont.h> /* for watch cursor */
#include <X11/Xatom.h>

#include "mdm-common.h"
#include "mdm-xerrors.h"

#include "mdm-slave.h"
#include "mdm-slave-glue.h"

#include "mdm-server.h"

#define MDM_SLAVE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SLAVE, MdmSlavePrivate))

#define CK_NAME      "org.freedesktop.ConsoleKit"
#define CK_PATH      "/org/freedesktop/ConsoleKit"
#define CK_INTERFACE "org.freedesktop.ConsoleKit"

#define CK_MANAGER_PATH      "/org/freedesktop/ConsoleKit/Manager"
#define CK_MANAGER_INTERFACE "org.freedesktop.ConsoleKit.Manager"
#define CK_SEAT_INTERFACE    "org.freedesktop.ConsoleKit.Seat"
#define CK_SESSION_INTERFACE "org.freedesktop.ConsoleKit.Session"

#define MDM_DBUS_NAME              "org.mate.DisplayManager"
#define MDM_DBUS_DISPLAY_INTERFACE "org.mate.DisplayManager.Display"

#define MAX_CONNECT_ATTEMPTS 10

struct MdmSlavePrivate
{
        char            *id;
        GPid             pid;
        guint            output_watch_id;
        guint            error_watch_id;

        Display         *server_display;

        /* cached display values */
        char            *display_id;
        char            *display_name;
        int              display_number;
        char            *display_hostname;
        gboolean         display_is_local;
        gboolean         display_is_parented;
        char            *display_seat_id;
        char            *display_x11_authority_file;
        char            *parent_display_name;
        char            *parent_display_x11_authority_file;
        char            *windowpath;

        GArray          *display_x11_cookie;

        DBusGProxy      *display_proxy;
        DBusGConnection *connection;
};

enum {
        PROP_0,
        PROP_DISPLAY_ID,
        PROP_DISPLAY_NAME,
        PROP_DISPLAY_NUMBER,
        PROP_DISPLAY_HOSTNAME,
        PROP_DISPLAY_IS_LOCAL,
        PROP_DISPLAY_SEAT_ID,
        PROP_DISPLAY_X11_AUTHORITY_FILE
};

enum {
        STOPPED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_slave_class_init    (MdmSlaveClass *klass);
static void     mdm_slave_init          (MdmSlave      *slave);
static void     mdm_slave_finalize      (GObject       *object);

G_DEFINE_ABSTRACT_TYPE (MdmSlave, mdm_slave, G_TYPE_OBJECT)

#define CURSOR_WATCH XC_watch

static void
mdm_slave_whack_temp_auth_file (MdmSlave *slave)
{
#if 0
        uid_t old;

        old = geteuid ();
        if (old != 0)
                seteuid (0);
        if (d->parent_temp_auth_file != NULL) {
                VE_IGNORE_EINTR (g_unlink (d->parent_temp_auth_file));
        }
        g_free (d->parent_temp_auth_file);
        d->parent_temp_auth_file = NULL;
        if (old != 0)
                seteuid (old);
#endif
}


static void
create_temp_auth_file (MdmSlave *slave)
{
#if 0
        if (d->type == TYPE_FLEXI_XNEST &&
            d->parent_auth_file != NULL) {
                if (d->parent_temp_auth_file != NULL) {
                        VE_IGNORE_EINTR (g_unlink (d->parent_temp_auth_file));
                }
                g_free (d->parent_temp_auth_file);
                d->parent_temp_auth_file =
                        copy_auth_file (d->server_uid,
                                        mdm_daemon_config_get_mdmuid (),
                                        d->parent_auth_file);
        }
#endif
}

static void
listify_hash (const char *key,
              const char *value,
              GPtrArray  *env)
{
        char *str;
        str = g_strdup_printf ("%s=%s", key, value);
        g_debug ("MdmSlave: script environment: %s", str);
        g_ptr_array_add (env, str);
}

static GPtrArray *
get_script_environment (MdmSlave   *slave,
                        const char *username)
{
        GPtrArray     *env;
        GHashTable    *hash;
        struct passwd *pwent;
        char          *x_servers_file;
        char          *temp;

        env = g_ptr_array_new ();

        /* create a hash table of current environment, then update keys has necessary */
        hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

        /* modify environment here */
        g_hash_table_insert (hash, g_strdup ("HOME"), g_strdup ("/"));
        g_hash_table_insert (hash, g_strdup ("PWD"), g_strdup ("/"));
        g_hash_table_insert (hash, g_strdup ("SHELL"), g_strdup ("/bin/sh"));

        if (username != NULL) {
                g_hash_table_insert (hash, g_strdup ("LOGNAME"),
                                     g_strdup (username));
                g_hash_table_insert (hash, g_strdup ("USER"),
                                     g_strdup (username));
                g_hash_table_insert (hash, g_strdup ("USERNAME"),
                                     g_strdup (username));

                mdm_get_pwent_for_name (username, &pwent);
                if (pwent != NULL) {
                        if (pwent->pw_dir != NULL && pwent->pw_dir[0] != '\0') {
                                g_hash_table_insert (hash, g_strdup ("HOME"),
                                                     g_strdup (pwent->pw_dir));
                                g_hash_table_insert (hash, g_strdup ("PWD"),
                                                     g_strdup (pwent->pw_dir));
                        }

                        g_hash_table_insert (hash, g_strdup ("SHELL"),
                                             g_strdup (pwent->pw_shell));
                }
        }

#if 0
        if (display_is_parented) {
                g_hash_table_insert (hash, g_strdup ("MDM_PARENT_DISPLAY"), g_strdup (parent_display_name));

                /*g_hash_table_insert (hash, "MDM_PARENT_XAUTHORITY"), slave->priv->parent_temp_auth_file));*/
        }
#endif

        /* some env for use with the Pre and Post scripts */
        temp = g_strconcat (slave->priv->display_name, ".Xservers", NULL);
        x_servers_file = g_build_filename (AUTHDIR, temp, NULL);
        g_free (temp);

        g_hash_table_insert (hash, g_strdup ("X_SERVERS"), x_servers_file);

        if (! slave->priv->display_is_local) {
                g_hash_table_insert (hash, g_strdup ("REMOTE_HOST"), g_strdup (slave->priv->display_hostname));
        }

        /* Runs as root */
        g_hash_table_insert (hash, g_strdup ("XAUTHORITY"), g_strdup (slave->priv->display_x11_authority_file));
        g_hash_table_insert (hash, g_strdup ("DISPLAY"), g_strdup (slave->priv->display_name));
        g_hash_table_insert (hash, g_strdup ("PATH"), g_strdup (MDM_SESSION_DEFAULT_PATH));
        g_hash_table_insert (hash, g_strdup ("RUNNING_UNDER_MDM"), g_strdup ("true"));

        g_hash_table_remove (hash, "MAIL");


        g_hash_table_foreach (hash, (GHFunc)listify_hash, env);
        g_hash_table_destroy (hash);

        g_ptr_array_add (env, NULL);

        return env;
}

gboolean
mdm_slave_run_script (MdmSlave   *slave,
                      const char *dir,
                      const char *login)
{
        char      *script;
        char     **argv;
        gint       status;
        GError    *error;
        GPtrArray *env;
        gboolean   res;
        gboolean   ret;

        ret = FALSE;

        g_assert (dir != NULL);
        g_assert (login != NULL);

        script = g_build_filename (dir, slave->priv->display_name, NULL);
        g_debug ("MdmSlave: Trying script %s", script);
        if (! (g_file_test (script, G_FILE_TEST_IS_REGULAR)
               && g_file_test (script, G_FILE_TEST_IS_EXECUTABLE))) {
                g_debug ("MdmSlave: script %s not found; skipping", script);
                g_free (script);
                script = NULL;
        }

        if (script == NULL
            && slave->priv->display_hostname != NULL
            && slave->priv->display_hostname[0] != '\0') {
                script = g_build_filename (dir, slave->priv->display_hostname, NULL);
                g_debug ("MdmSlave: Trying script %s", script);
                if (! (g_file_test (script, G_FILE_TEST_IS_REGULAR)
                       && g_file_test (script, G_FILE_TEST_IS_EXECUTABLE))) {
                        g_debug ("MdmSlave: script %s not found; skipping", script);
                        g_free (script);
                        script = NULL;
                }
        }

        if (script == NULL) {
                script = g_build_filename (dir, "Default", NULL);
                g_debug ("MdmSlave: Trying script %s", script);
                if (! (g_file_test (script, G_FILE_TEST_IS_REGULAR)
                       && g_file_test (script, G_FILE_TEST_IS_EXECUTABLE))) {
                        g_debug ("MdmSlave: script %s not found; skipping", script);
                        g_free (script);
                        script = NULL;
                }
        }

        if (script == NULL) {
                g_debug ("MdmSlave: no script found");
                return TRUE;
        }

        create_temp_auth_file (slave);

        g_debug ("MdmSlave: Running process: %s", script);
        error = NULL;
        if (! g_shell_parse_argv (script, NULL, &argv, &error)) {
                g_warning ("Could not parse command: %s", error->message);
                g_error_free (error);
                goto out;
        }

        env = get_script_environment (slave, login);

        res = g_spawn_sync (NULL,
                            argv,
                            (char **)env->pdata,
                            G_SPAWN_SEARCH_PATH,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &status,
                            &error);

        g_ptr_array_foreach (env, (GFunc)g_free, NULL);
        g_ptr_array_free (env, TRUE);
        g_strfreev (argv);

        if (! res) {
                g_warning ("MdmSlave: Unable to run script: %s", error->message);
                g_error_free (error);
        }

        mdm_slave_whack_temp_auth_file (slave);

        if (WIFEXITED (status)) {
                g_debug ("MdmSlave: Process exit status: %d", WEXITSTATUS (status));
                ret = WEXITSTATUS (status) != 0;
        } else {
                ret = TRUE;
        }

 out:
        g_free (script);

        return ret;
}

void
mdm_slave_set_busy_cursor (MdmSlave *slave)
{
        if (slave->priv->server_display != NULL) {
                Cursor xcursor;

                xcursor = XCreateFontCursor (slave->priv->server_display, CURSOR_WATCH);
                XDefineCursor (slave->priv->server_display,
                               DefaultRootWindow (slave->priv->server_display),
                               xcursor);
                XFreeCursor (slave->priv->server_display, xcursor);
                XSync (slave->priv->server_display, False);
        }
}

static void
mdm_slave_setup_xhost_auth (XHostAddress *host_entries, XServerInterpretedAddress *si_entries)
{
        si_entries[0].type        = "localuser";
        si_entries[0].typelength  = strlen ("localuser");
        si_entries[1].type        = "localuser";
        si_entries[1].typelength  = strlen ("localuser");

        si_entries[0].value       = "root";
        si_entries[0].valuelength = strlen ("root");
        si_entries[1].value       = MDM_USERNAME;
        si_entries[1].valuelength = strlen (MDM_USERNAME);

        host_entries[0].family    = FamilyServerInterpreted;
        host_entries[0].address   = (char *) &si_entries[0];
        host_entries[0].length    = sizeof (XServerInterpretedAddress);
        host_entries[1].family    = FamilyServerInterpreted;
        host_entries[1].address   = (char *) &si_entries[1];
        host_entries[1].length    = sizeof (XServerInterpretedAddress);
}

static void
mdm_slave_set_windowpath (MdmSlave *slave)
{
        /* setting WINDOWPATH for clients */
        Atom prop;
        Atom actualtype;
        int actualformat;
        unsigned long nitems;
        unsigned long bytes_after;
        unsigned char *buf;
        const char *windowpath;
        char *newwindowpath;
        unsigned long num;
        char nums[10];
        int numn;

        prop = XInternAtom (slave->priv->server_display, "XFree86_VT", False);
        if (prop == None) {
                g_debug ("no XFree86_VT atom\n");
                return;
        }
        if (XGetWindowProperty (slave->priv->server_display,
                DefaultRootWindow (slave->priv->server_display), prop, 0, 1,
                False, AnyPropertyType, &actualtype, &actualformat,
                &nitems, &bytes_after, &buf)) {
                g_debug ("no XFree86_VT property\n");
                return;
        }

        if (nitems != 1) {
                g_debug ("%lu items in XFree86_VT property!\n", nitems);
                XFree (buf);
                return;
        }

        switch (actualtype) {
        case XA_CARDINAL:
        case XA_INTEGER:
        case XA_WINDOW:
                switch (actualformat) {
                case  8:
                        num = (*(uint8_t  *)(void *)buf);
                        break;
                case 16:
                        num = (*(uint16_t *)(void *)buf);
                        break;
                case 32:
                        num = (*(long *)(void *)buf);
                        break;
                default:
                        g_debug ("format %d in XFree86_VT property!\n", actualformat);
                        XFree (buf);
                        return;
                }
                break;
        default:
                g_debug ("type %lx in XFree86_VT property!\n", actualtype);
                XFree (buf);
                return;
        }
        XFree (buf);

        windowpath = getenv ("WINDOWPATH");
        numn = snprintf (nums, sizeof (nums), "%lu", num);
        if (!windowpath) {
                newwindowpath = malloc (numn + 1);
                sprintf (newwindowpath, "%s", nums);
        } else {
                newwindowpath = malloc (strlen (windowpath) + 1 + numn + 1);
                sprintf (newwindowpath, "%s:%s", windowpath, nums);
        }

        slave->priv->windowpath = newwindowpath;

        g_setenv ("WINDOWPATH", newwindowpath, TRUE);
}

gboolean
mdm_slave_connect_to_x11_display (MdmSlave *slave)
{
        gboolean ret;
        sigset_t mask;
        sigset_t omask;

        ret = FALSE;

        /* We keep our own (windowless) connection (dsp) open to avoid the
         * X server resetting due to lack of active connections. */

        g_debug ("MdmSlave: Server is ready - opening display %s", slave->priv->display_name);

        g_setenv ("DISPLAY", slave->priv->display_name, TRUE);
        g_setenv ("XAUTHORITY", slave->priv->display_x11_authority_file, TRUE);

        sigemptyset (&mask);
        sigaddset (&mask, SIGCHLD);
        sigprocmask (SIG_BLOCK, &mask, &omask);

        /* Give slave access to the display independent of current hostname */
        XSetAuthorization ("MIT-MAGIC-COOKIE-1",
                           strlen ("MIT-MAGIC-COOKIE-1"),
                           slave->priv->display_x11_cookie->data,
                           slave->priv->display_x11_cookie->len);

        slave->priv->server_display = XOpenDisplay (slave->priv->display_name);

        sigprocmask (SIG_SETMASK, &omask, NULL);


        if (slave->priv->server_display == NULL) {
                g_warning ("Unable to connect to display %s", slave->priv->display_name);
                ret = FALSE;
        } else if (slave->priv->display_is_local) {
                XServerInterpretedAddress si_entries[2];
                XHostAddress              host_entries[2];

                g_debug ("MdmSlave: Connected to display %s", slave->priv->display_name);
                ret = TRUE;

                /* Give programs run by the slave and greeter access to the
                 * display independent of current hostname
                 */
                mdm_slave_setup_xhost_auth (host_entries, si_entries);

                mdm_error_trap_push ();
                XAddHosts (slave->priv->server_display, host_entries,
                           G_N_ELEMENTS (host_entries));
                XSync (slave->priv->server_display, False);
                if (mdm_error_trap_pop ()) {
                        g_warning ("Failed to give slave programs access to the display. Trying to proceed.");
                }

                mdm_slave_set_windowpath (slave);
        } else {
                g_debug ("MdmSlave: Connected to display %s", slave->priv->display_name);
                ret = TRUE;
        }

        return ret;
}

static void
display_proxy_destroyed_cb (DBusGProxy *display_proxy,
                            MdmSlave   *slave)
{
        g_debug ("MdmSlave: Disconnected from display");

        slave->priv->display_proxy = NULL;
}

static gboolean
mdm_slave_set_slave_bus_name (MdmSlave *slave)
{
        gboolean    res;
        GError     *error;
        const char *name;

        name = dbus_bus_get_unique_name (dbus_g_connection_get_connection (slave->priv->connection));

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "SetSlaveBusName",
                                 &error,
                                 G_TYPE_STRING, name,
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);

        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to set slave bus name on parent: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to set slave bus name on parent");
                }
        }

        return res;
}

static gboolean
mdm_slave_real_start (MdmSlave *slave)
{
        gboolean res;
        char    *id;
        GError  *error;

        g_debug ("MdmSlave: Starting slave");

        g_assert (slave->priv->display_proxy == NULL);

        g_debug ("MdmSlave: Creating proxy for %s", slave->priv->display_id);
        error = NULL;
        slave->priv->display_proxy = dbus_g_proxy_new_for_name_owner (slave->priv->connection,
                                                                      MDM_DBUS_NAME,
                                                                      slave->priv->display_id,
                                                                      MDM_DBUS_DISPLAY_INTERFACE,
                                                                      &error);
        g_signal_connect (slave->priv->display_proxy,
                          "destroy",
                          G_CALLBACK (display_proxy_destroyed_cb),
                          slave);

        if (slave->priv->display_proxy == NULL) {
                if (error != NULL) {
                        g_warning ("Failed to create display proxy %s: %s", slave->priv->display_id, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Unable to create display proxy");
                }
                return FALSE;
        }

        /* Make sure display ID works */
        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "GetId",
                                 &error,
                                 G_TYPE_INVALID,
                                 DBUS_TYPE_G_OBJECT_PATH, &id,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get display ID %s: %s", slave->priv->display_id, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get display ID %s", slave->priv->display_id);
                }

                return FALSE;
        }

        g_debug ("MdmSlave: Got display ID: %s", id);

        if (strcmp (id, slave->priv->display_id) != 0) {
                g_critical ("Display ID doesn't match");
                exit (1);
        }

        mdm_slave_set_slave_bus_name (slave);

        /* cache some values up front */
        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "IsLocal",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_BOOLEAN, &slave->priv->display_is_local,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get value: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get value");
                }

                return FALSE;
        }

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "GetX11DisplayName",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &slave->priv->display_name,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get value: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get value");
                }

                return FALSE;
        }

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "GetX11DisplayNumber",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_INT, &slave->priv->display_number,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get value: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get value");
                }

                return FALSE;
        }

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "GetRemoteHostname",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &slave->priv->display_hostname,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get value: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get value");
                }

                return FALSE;
        }

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "GetX11Cookie",
                                 &error,
                                 G_TYPE_INVALID,
                                 dbus_g_type_get_collection ("GArray", G_TYPE_CHAR),
                                 &slave->priv->display_x11_cookie,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get value: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get value");
                }

                return FALSE;
        }

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "GetX11AuthorityFile",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &slave->priv->display_x11_authority_file,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get value: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get value");
                }

                return FALSE;
        }

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "GetSeatId",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &slave->priv->display_seat_id,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get value: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get value");
                }

                return FALSE;
        }

        return TRUE;
}

static gboolean
mdm_slave_real_stop (MdmSlave *slave)
{
        g_debug ("MdmSlave: Stopping slave");

        if (slave->priv->display_proxy != NULL) {
                g_object_unref (slave->priv->display_proxy);
        }

        return TRUE;
}

gboolean
mdm_slave_start (MdmSlave *slave)
{
        gboolean ret;

        g_return_val_if_fail (MDM_IS_SLAVE (slave), FALSE);

        g_debug ("MdmSlave: starting slave");

        g_object_ref (slave);
        ret = MDM_SLAVE_GET_CLASS (slave)->start (slave);
        g_object_unref (slave);

        return ret;
}

gboolean
mdm_slave_stop (MdmSlave *slave)
{
        gboolean ret;

        g_return_val_if_fail (MDM_IS_SLAVE (slave), FALSE);

        g_debug ("MdmSlave: stopping slave");

        g_object_ref (slave);
        ret = MDM_SLAVE_GET_CLASS (slave)->stop (slave);
        g_object_unref (slave);

        return ret;
}

void
mdm_slave_stopped (MdmSlave *slave)
{
        g_return_if_fail (MDM_IS_SLAVE (slave));

        g_signal_emit (slave, signals [STOPPED], 0);
}

gboolean
mdm_slave_add_user_authorization (MdmSlave   *slave,
                                  const char *username,
                                  char      **filenamep)
{
        XServerInterpretedAddress si_entries[2];
        XHostAddress              host_entries[2];
        gboolean                  res;
        GError                   *error;
        char                     *filename;

        filename = NULL;

        if (filenamep != NULL) {
                *filenamep = NULL;
        }

        g_debug ("MdmSlave: Requesting user authorization");

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "AddUserAuthorization",
                                 &error,
                                 G_TYPE_STRING, username,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &filename,
                                 G_TYPE_INVALID);

        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to add user authorization: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to add user authorization");
                }
        } else {
                g_debug ("MdmSlave: Got user authorization: %s", filename);
        }

        if (filenamep != NULL) {
                *filenamep = g_strdup (filename);
        }
        g_free (filename);

        /* Remove access for the programs run by slave and greeter now that the
         * user session is starting.
         */
        mdm_slave_setup_xhost_auth (host_entries, si_entries);
        mdm_error_trap_push ();
        XRemoveHosts (slave->priv->server_display, host_entries,
                      G_N_ELEMENTS (host_entries));
        XSync (slave->priv->server_display, False);
        if (mdm_error_trap_pop ()) {
                g_warning ("Failed to remove slave program access to the display. Trying to proceed.");
        }


        return res;
}

static char *
mdm_slave_parse_enriched_login (MdmSlave   *slave,
                                const char *username,
                                const char *display_name)
{
        char     **argv;
        int        username_len;
        GPtrArray *env;
        GError    *error;
        gboolean   res;
        char      *parsed_username;
        char      *command;
        char      *std_output;
        char      *std_error;

        parsed_username = NULL;

        if (username == NULL || username[0] == '\0') {
                return NULL;
        }

        /* A script may be used to generate the automatic/timed login name
           based on the display/host by ending the name with the pipe symbol
           '|'. */

        username_len = strlen (username);
        if (username[username_len - 1] != '|') {
                return g_strdup (username);
        }

        /* Remove the pipe symbol */
        command = g_strndup (username, username_len - 1);

        argv = NULL;
        error = NULL;
        if (! g_shell_parse_argv (command, NULL, &argv, &error)) {
                g_warning ("MdmSlave: Could not parse command '%s': %s", command, error->message);
                g_error_free (error);

                g_free (command);
                goto out;
        }

        g_debug ("MdmSlave: running '%s' to acquire auto/timed username", command);
        g_free (command);

        env = get_script_environment (slave, NULL);

        error = NULL;
        std_output = NULL;
        std_error = NULL;
        res = g_spawn_sync (NULL,
                            argv,
                            (char **)env->pdata,
                            G_SPAWN_SEARCH_PATH,
                            NULL,
                            NULL,
                            &std_output,
                            &std_error,
                            NULL,
                            &error);

        g_ptr_array_foreach (env, (GFunc)g_free, NULL);
        g_ptr_array_free (env, TRUE);
        g_strfreev (argv);

        if (! res) {
                g_warning ("MdmSlave: Unable to launch auto/timed login script '%s': %s", username, error->message);
                g_error_free (error);

                g_free (std_output);
                g_free (std_error);
                goto out;
        }

        if (std_output != NULL) {
                g_strchomp (std_output);
                if (std_output[0] != '\0') {
                        parsed_username = g_strdup (std_output);
                }
        }

 out:

        return parsed_username;
}

gboolean
mdm_slave_get_timed_login_details (MdmSlave   *slave,
                                   gboolean   *enabledp,
                                   char      **usernamep,
                                   int        *delayp)
{
        struct passwd *pwent;
        GError        *error;
        gboolean       res;
        gboolean       enabled;
        char          *username;
        int            delay;

        username = NULL;
        enabled = FALSE;
        delay = 0;

        g_debug ("MdmSlave: Requesting timed login details");

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->display_proxy,
                                 "GetTimedLoginDetails",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_BOOLEAN, &enabled,
                                 G_TYPE_STRING, &username,
                                 G_TYPE_INT, &delay,
                                 G_TYPE_INVALID);

        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get timed login details: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get timed login details");
                }
        } else {
                g_debug ("MdmSlave: Got timed login details: %d %s %d", enabled, username, delay);
        }

        if (usernamep != NULL) {
                *usernamep = mdm_slave_parse_enriched_login (slave,
                                                             username,
                                                             slave->priv->display_name);
        } else {
                g_free (username);

                if (enabledp != NULL) {
                        *enabledp = enabled;
                }
                if (delayp != NULL) {
                        *delayp = delay;
                }
                return TRUE;
        }
        g_free (username);

        if (usernamep != NULL && *usernamep != NULL) {
                mdm_get_pwent_for_name (*usernamep, &pwent);
                if (pwent == NULL) {
                        g_debug ("Invalid username %s for auto/timed login",
                                 *usernamep);
                        g_free (*usernamep);
                        *usernamep = NULL;
                } else {
                        g_debug ("Using username %s for auto/timed login",
                                 *usernamep);

                        if (enabledp != NULL) {
                                *enabledp = enabled;
                        }
                        if (delayp != NULL) {
                                *delayp = delay;
                        }
               }
        } else {
                g_debug ("Invalid NULL username for auto/timed login");
        }

        return res;
}

static gboolean
_get_uid_and_gid_for_user (const char *username,
                           uid_t      *uid,
                           gid_t      *gid)
{
        struct passwd *passwd_entry;

        g_assert (username != NULL);

        errno = 0;
        mdm_get_pwent_for_name (username, &passwd_entry);

        if (passwd_entry == NULL) {
                return FALSE;
        }

        if (uid != NULL) {
                *uid = passwd_entry->pw_uid;
        }

        if (gid != NULL) {
                *gid = passwd_entry->pw_gid;
        }

        return TRUE;
}

static gboolean
x11_session_is_on_seat (MdmSlave        *slave,
                        const char      *session_id,
                        const char      *seat_id)
{
        DBusGProxy      *proxy;
        GError          *error;
        char            *sid;
        gboolean         res;
        gboolean         ret;
        char            *x11_display_device;
        char            *x11_display;

        ret = FALSE;

        if (seat_id == NULL || seat_id[0] == '\0' || session_id == NULL || session_id[0] == '\0') {
                return FALSE;
        }

        proxy = dbus_g_proxy_new_for_name (slave->priv->connection,
                                           CK_NAME,
                                           session_id,
                                           CK_SESSION_INTERFACE);
        if (proxy == NULL) {
                g_warning ("Failed to connect to the ConsoleKit seat object");
                goto out;
        }

        sid = NULL;
        error = NULL;
        res = dbus_g_proxy_call (proxy,
                                 "GetSeatId",
                                 &error,
                                 G_TYPE_INVALID,
                                 DBUS_TYPE_G_OBJECT_PATH, &sid,
                                 G_TYPE_INVALID);
        if (! res) {
                g_debug ("Failed to identify the current seat: %s", error->message);
                g_error_free (error);
                goto out;
        }

        if (sid == NULL || sid[0] == '\0' || strcmp (sid, seat_id) != 0) {
                g_debug ("MdmSlave: session not on current seat: %s", seat_id);
                goto out;
        }

        x11_display = NULL;
        error = NULL;
        res = dbus_g_proxy_call (proxy,
                                 "GetX11Display",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &x11_display,
                                 G_TYPE_INVALID);
        if (! res) {
                g_error_free (error);
                goto out;
        }

        /* don't try to switch to our own session */
        if (x11_display == NULL || x11_display[0] == '\0'
            || strcmp (slave->priv->display_name, x11_display) == 0) {
                g_free (x11_display);
                goto out;
        }
        g_free (x11_display);

        x11_display_device = NULL;
        error = NULL;
        res = dbus_g_proxy_call (proxy,
                                 "GetX11DisplayDevice",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &x11_display_device,
                                 G_TYPE_INVALID);
        if (! res) {
                g_error_free (error);
                goto out;
        }

        if (x11_display_device == NULL || x11_display_device[0] == '\0') {
                g_free (x11_display_device);
                goto out;
        }
        g_free (x11_display_device);

        ret = TRUE;

 out:
        if (proxy != NULL) {
                g_object_unref (proxy);
        }

        return ret;
}

char *
mdm_slave_get_primary_session_id_for_user (MdmSlave   *slave,
                                           const char *username)
{
        gboolean    res;
        gboolean    can_activate_sessions;
        GError     *error;
        DBusGProxy *manager_proxy;
        DBusGProxy *seat_proxy;
        GPtrArray  *sessions;
        char       *primary_ssid;
        int         i;
        uid_t       uid;

        if (slave->priv->display_seat_id == NULL || slave->priv->display_seat_id[0] == '\0') {
                g_debug ("MdmSlave: display seat ID is not set; can't switch sessions");
                return NULL;
        }

        manager_proxy = NULL;
        primary_ssid = NULL;
        sessions = NULL;

        g_debug ("MdmSlave: getting proxy for seat: %s", slave->priv->display_seat_id);

        seat_proxy = dbus_g_proxy_new_for_name (slave->priv->connection,
                                                CK_NAME,
                                                slave->priv->display_seat_id,
                                                CK_SEAT_INTERFACE);

        g_debug ("MdmSlave: checking if seat can activate sessions");

        error = NULL;
        res = dbus_g_proxy_call (seat_proxy,
                                 "CanActivateSessions",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_BOOLEAN, &can_activate_sessions,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("unable to determine if seat can activate sessions: %s",
                           error->message);
                g_error_free (error);
                goto out;
        }

        if (! can_activate_sessions) {
                g_debug ("MdmSlave: seat is unable to activate sessions");
                goto out;
        }

        manager_proxy = dbus_g_proxy_new_for_name (slave->priv->connection,
                                                   CK_NAME,
                                                   CK_MANAGER_PATH,
                                                   CK_MANAGER_INTERFACE);

        if (! _get_uid_and_gid_for_user (username, &uid, NULL)) {
                g_debug ("MdmSlave: unable to determine uid for user: %s", username);
                goto out;
        }

        error = NULL;
        res = dbus_g_proxy_call (manager_proxy,
                                 "GetSessionsForUnixUser",
                                 &error,
                                 G_TYPE_UINT, uid,
                                 G_TYPE_INVALID,
                                 dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH), &sessions,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("unable to determine sessions for user: %s",
                           error->message);
                g_error_free (error);
                goto out;
        }

        for (i = 0; i < sessions->len; i++) {
                char *ssid;

                ssid = g_ptr_array_index (sessions, i);

                if (x11_session_is_on_seat (slave, ssid, slave->priv->display_seat_id)) {
                        primary_ssid = g_strdup (ssid);
                        break;
                }
        }

        g_ptr_array_foreach (sessions, (GFunc)g_free, NULL);
        g_ptr_array_free (sessions, TRUE);

 out:

        if (seat_proxy != NULL) {
                g_object_unref (seat_proxy);
        }
        if (manager_proxy != NULL) {
                g_object_unref (manager_proxy);
        }

        return primary_ssid;
}

static gboolean
activate_session_id (MdmSlave   *slave,
                     const char *seat_id,
                     const char *session_id)
{
        DBusError    local_error;
        DBusMessage *message;
        DBusMessage *reply;
        gboolean     ret;

        ret = FALSE;
        reply = NULL;

        dbus_error_init (&local_error);
        message = dbus_message_new_method_call ("org.freedesktop.ConsoleKit",
                                                seat_id,
                                                "org.freedesktop.ConsoleKit.Seat",
                                                "ActivateSession");
        if (message == NULL) {
                goto out;
        }

        if (! dbus_message_append_args (message,
                                        DBUS_TYPE_OBJECT_PATH, &session_id,
                                        DBUS_TYPE_INVALID)) {
                goto out;
        }


        dbus_error_init (&local_error);
        reply = dbus_connection_send_with_reply_and_block (dbus_g_connection_get_connection (slave->priv->connection),
                                                           message,
                                                           -1,
                                                           &local_error);
        if (reply == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("Unable to activate session: %s", local_error.message);
                        dbus_error_free (&local_error);
                        goto out;
                }
        }

        ret = TRUE;
 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return ret;
}

static gboolean
session_unlock (MdmSlave   *slave,
                const char *ssid)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;

        g_debug ("ConsoleKit: Unlocking session %s", ssid);
        message = dbus_message_new_method_call (CK_NAME,
                                                ssid,
                                                CK_SESSION_INTERFACE,
                                                "Unlock");
        if (message == NULL) {
                g_debug ("MdmSlave: ConsoleKit couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (dbus_g_connection_get_connection (slave->priv->connection),
                                                           message,
                                                           -1, &error);
        dbus_message_unref (message);
        if (reply != NULL) {
                dbus_message_unref (reply);
        }
        dbus_connection_flush (dbus_g_connection_get_connection (slave->priv->connection));

        if (dbus_error_is_set (&error)) {
                g_debug ("MdmSlave: ConsoleKit %s raised:\n %s\n\n", error.name, error.message);
                return FALSE;
        }

        return TRUE;
}

gboolean
mdm_slave_switch_to_user_session (MdmSlave   *slave,
                                  const char *username)
{
        gboolean    res;
        gboolean    ret;
        char       *ssid_to_activate;

        ret = FALSE;

        ssid_to_activate = mdm_slave_get_primary_session_id_for_user (slave, username);
        if (ssid_to_activate == NULL) {
                g_debug ("MdmSlave: unable to determine session to activate");
                goto out;
        }

        g_debug ("MdmSlave: Activating session: '%s'", ssid_to_activate);

        res = activate_session_id (slave, slave->priv->display_seat_id, ssid_to_activate);
        if (! res) {
                g_debug ("MdmSlave: unable to activate session: %s", ssid_to_activate);
                goto out;
        }

        res = session_unlock (slave, ssid_to_activate);
        if (!res) {
                /* this isn't fatal */
                g_debug ("MdmSlave: unable to unlock session: %s", ssid_to_activate);
        }

        ret = TRUE;

 out:
        g_free (ssid_to_activate);

        return ret;
}

static void
_mdm_slave_set_display_id (MdmSlave   *slave,
                           const char *id)
{
        g_free (slave->priv->display_id);
        slave->priv->display_id = g_strdup (id);
}

static void
_mdm_slave_set_display_name (MdmSlave   *slave,
                             const char *name)
{
        g_free (slave->priv->display_name);
        slave->priv->display_name = g_strdup (name);
}

static void
_mdm_slave_set_display_number (MdmSlave   *slave,
                               int         number)
{
        slave->priv->display_number = number;
}

static void
_mdm_slave_set_display_hostname (MdmSlave   *slave,
                                 const char *name)
{
        g_free (slave->priv->display_hostname);
        slave->priv->display_hostname = g_strdup (name);
}

static void
_mdm_slave_set_display_x11_authority_file (MdmSlave   *slave,
                                           const char *name)
{
        g_free (slave->priv->display_x11_authority_file);
        slave->priv->display_x11_authority_file = g_strdup (name);
}

static void
_mdm_slave_set_display_seat_id (MdmSlave   *slave,
                                const char *id)
{
        g_free (slave->priv->display_seat_id);
        slave->priv->display_seat_id = g_strdup (id);
}

static void
_mdm_slave_set_display_is_local (MdmSlave   *slave,
                                 gboolean    is)
{
        slave->priv->display_is_local = is;
}

static void
mdm_slave_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
        MdmSlave *self;

        self = MDM_SLAVE (object);

        switch (prop_id) {
        case PROP_DISPLAY_ID:
                _mdm_slave_set_display_id (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_NAME:
                _mdm_slave_set_display_name (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_NUMBER:
                _mdm_slave_set_display_number (self, g_value_get_int (value));
                break;
        case PROP_DISPLAY_HOSTNAME:
                _mdm_slave_set_display_hostname (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_SEAT_ID:
                _mdm_slave_set_display_seat_id (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_X11_AUTHORITY_FILE:
                _mdm_slave_set_display_x11_authority_file (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_IS_LOCAL:
                _mdm_slave_set_display_is_local (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_slave_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
        MdmSlave *self;

        self = MDM_SLAVE (object);

        switch (prop_id) {
        case PROP_DISPLAY_ID:
                g_value_set_string (value, self->priv->display_id);
                break;
        case PROP_DISPLAY_NAME:
                g_value_set_string (value, self->priv->display_name);
                break;
        case PROP_DISPLAY_NUMBER:
                g_value_set_int (value, self->priv->display_number);
                break;
        case PROP_DISPLAY_HOSTNAME:
                g_value_set_string (value, self->priv->display_hostname);
                break;
        case PROP_DISPLAY_SEAT_ID:
                g_value_set_string (value, self->priv->display_seat_id);
                break;
        case PROP_DISPLAY_X11_AUTHORITY_FILE:
                g_value_set_string (value, self->priv->display_x11_authority_file);
                break;
        case PROP_DISPLAY_IS_LOCAL:
                g_value_set_boolean (value, self->priv->display_is_local);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
register_slave (MdmSlave *slave)
{
        GError *error;

        error = NULL;
        slave->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (slave->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                exit (1);
        }

        dbus_g_connection_register_g_object (slave->priv->connection, slave->priv->id, G_OBJECT (slave));

        return TRUE;
}

static GObject *
mdm_slave_constructor (GType                  type,
                       guint                  n_construct_properties,
                       GObjectConstructParam *construct_properties)
{
        MdmSlave      *slave;
        gboolean       res;
        const char    *id;

        slave = MDM_SLAVE (G_OBJECT_CLASS (mdm_slave_parent_class)->constructor (type,
                                                                                 n_construct_properties,
                                                                                 construct_properties));
        /* Always match the slave id with the master */

        id = NULL;
        if (g_str_has_prefix (slave->priv->display_id, "/org/mate/DisplayManager/Display")) {
                id = slave->priv->display_id + strlen ("/org/mate/DisplayManager/Display");
        }

        g_assert (id != NULL);

        slave->priv->id = g_strdup_printf ("/org/mate/DisplayManager/Slave%s", id);
        g_debug ("MdmSlave: Registering %s", slave->priv->id);

        res = register_slave (slave);
        if (! res) {
                g_warning ("Unable to register slave with system bus");
        }

        return G_OBJECT (slave);
}

static void
mdm_slave_class_init (MdmSlaveClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_slave_get_property;
        object_class->set_property = mdm_slave_set_property;
        object_class->constructor = mdm_slave_constructor;
        object_class->finalize = mdm_slave_finalize;

        klass->start = mdm_slave_real_start;
        klass->stop = mdm_slave_real_stop;

        g_type_class_add_private (klass, sizeof (MdmSlavePrivate));

        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_ID,
                                         g_param_spec_string ("display-id",
                                                              "id",
                                                              "id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_NAME,
                                         g_param_spec_string ("display-name",
                                                              "display name",
                                                              "display name",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_NUMBER,
                                         g_param_spec_int ("display-number",
                                                           "display number",
                                                           "display number",
                                                           -1,
                                                           G_MAXINT,
                                                           -1,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_HOSTNAME,
                                         g_param_spec_string ("display-hostname",
                                                              "display hostname",
                                                              "display hostname",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_SEAT_ID,
                                         g_param_spec_string ("display-seat-id",
                                                              "",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_X11_AUTHORITY_FILE,
                                         g_param_spec_string ("display-x11-authority-file",
                                                              "",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_IS_LOCAL,
                                         g_param_spec_boolean ("display-is-local",
                                                               "display is local",
                                                               "display is local",
                                                               TRUE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        signals [STOPPED] =
                g_signal_new ("stopped",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmSlaveClass, stopped),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        dbus_g_object_type_install_info (MDM_TYPE_SLAVE, &dbus_glib_mdm_slave_object_info);
}

static void
mdm_slave_init (MdmSlave *slave)
{

        slave->priv = MDM_SLAVE_GET_PRIVATE (slave);

        slave->priv->pid = -1;
}

static void
mdm_slave_finalize (GObject *object)
{
        MdmSlave *slave;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_SLAVE (object));

        slave = MDM_SLAVE (object);

        g_return_if_fail (slave->priv != NULL);

        mdm_slave_real_stop (slave);

        g_free (slave->priv->id);
        g_free (slave->priv->display_id);
        g_free (slave->priv->display_name);
        g_free (slave->priv->display_hostname);
        g_free (slave->priv->display_seat_id);
        g_free (slave->priv->display_x11_authority_file);
        g_free (slave->priv->parent_display_name);
        g_free (slave->priv->parent_display_x11_authority_file);
        g_free (slave->priv->windowpath);
        if (slave->priv->display_x11_cookie != NULL) {
                g_array_free (slave->priv->display_x11_cookie, TRUE);
        }

        G_OBJECT_CLASS (mdm_slave_parent_class)->finalize (object);
}
