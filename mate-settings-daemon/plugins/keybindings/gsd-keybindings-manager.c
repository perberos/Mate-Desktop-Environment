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

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/keysym.h>
#include <mateconf/mateconf-client.h>

#include "mate-settings-profile.h"
#include "gsd-keybindings-manager.h"

#include "gsd-keygrab.h"
#include "eggaccelerators.h"

#define MATECONF_BINDING_DIR "/desktop/mate/keybindings"
#define ALLOWED_KEYS_KEY MATECONF_BINDING_DIR "/allowed_keys"

#define GSD_KEYBINDINGS_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_KEYBINDINGS_MANAGER, GsdKeybindingsManagerPrivate))

typedef struct {
        char *binding_str;
        char *action;
        char *mateconf_key;
        Key   key;
        Key   previous_key;
} Binding;

struct GsdKeybindingsManagerPrivate
{
        GSList *binding_list;
	GSList *allowed_keys;
        GSList *screens;
        guint   notify;
};

static void     gsd_keybindings_manager_class_init  (GsdKeybindingsManagerClass *klass);
static void     gsd_keybindings_manager_init        (GsdKeybindingsManager      *keybindings_manager);
static void     gsd_keybindings_manager_finalize    (GObject             *object);

G_DEFINE_TYPE (GsdKeybindingsManager, gsd_keybindings_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static GSList *
get_screens_list (void)
{
        GdkDisplay *display = gdk_display_get_default();
        int         n_screens;
        GSList     *list = NULL;
        int         i;

        n_screens = gdk_display_get_n_screens (display);

        if (n_screens == 1) {
                list = g_slist_append (list, gdk_screen_get_default ());
        } else {
                for (i = 0; i < n_screens; i++) {
                        GdkScreen *screen;

                        screen = gdk_display_get_screen (display, i);
                        if (screen != NULL) {
                                list = g_slist_prepend (list, screen);
                        }
                }
                list = g_slist_reverse (list);
        }

        return list;
}

static char *
entry_get_string (MateConfEntry *entry)
{
        MateConfValue *value = mateconf_entry_get_value (entry);

        if (value == NULL || value->type != MATECONF_VALUE_STRING) {
                return NULL;
        }

        return g_strdup (mateconf_value_get_string (value));
}

static gboolean
parse_binding (Binding *binding)
{
        gboolean success;

        g_return_val_if_fail (binding != NULL, FALSE);

        binding->key.keysym = 0;
        binding->key.state = 0;
        g_free (binding->key.keycodes);
        binding->key.keycodes = NULL;

        if (binding->binding_str == NULL ||
            binding->binding_str[0] == '\0' ||
            strcmp (binding->binding_str, "Disabled") == 0) {
                return FALSE;
        }

        success = egg_accelerator_parse_virtual (binding->binding_str,
                                                 &binding->key.keysym,
                                                 &binding->key.keycodes,
                                                 &binding->key.state);

        if (!success)
            g_warning (_("Key binding (%s) is invalid"), binding->mateconf_key);

        return success;
}

static gint
compare_bindings (gconstpointer a,
                  gconstpointer b)
{
        Binding *key_a = (Binding *) a;
        char    *key_b = (char *) b;

        return strcmp (key_b, key_a->mateconf_key);
}

static gboolean
bindings_get_entry (GsdKeybindingsManager *manager,
                    MateConfClient           *client,
                    const char            *subdir)
{
        Binding *new_binding;
        GSList  *tmp_elem;
        GSList  *list;
        GSList  *li;
        char    *mateconf_key;
        char    *action = NULL;
        char    *key = NULL;

        g_return_val_if_fail (subdir != NULL, FALSE);

        mateconf_key = g_path_get_basename (subdir);

        if (!mateconf_key) {
                return FALSE;
        }

        /* Get entries for this binding */
        list = mateconf_client_all_entries (client, subdir, NULL);

        for (li = list; li != NULL; li = li->next) {
                MateConfEntry *entry = li->data;
                char *key_name = g_path_get_basename (mateconf_entry_get_key (entry));

                if (key_name == NULL) {
                        /* ignore entry */
                } else if (strcmp (key_name, "action") == 0) {
                        action = entry_get_string (entry);
                } else if (strcmp (key_name, "binding") == 0) {
                        key = entry_get_string (entry);
                }

                g_free (key_name);
                mateconf_entry_free (entry);
        }

        g_slist_free (list);

        if (!action || !key) {
                g_warning (_("Key binding (%s) is incomplete"), mateconf_key);
                g_free (mateconf_key);
                g_free (action);
                g_free (key);
                return FALSE;
        }

        tmp_elem = g_slist_find_custom (manager->priv->binding_list,
                                        mateconf_key,
                                        compare_bindings);

        if (!tmp_elem) {
                new_binding = g_new0 (Binding, 1);
        } else {
                new_binding = (Binding *) tmp_elem->data;
                g_free (new_binding->binding_str);
                g_free (new_binding->action);
                g_free (new_binding->mateconf_key);

                new_binding->previous_key.keysym = new_binding->key.keysym;
                new_binding->previous_key.state = new_binding->key.state;
                new_binding->previous_key.keycodes = new_binding->key.keycodes;
                new_binding->key.keycodes = NULL;
        }

        new_binding->binding_str = key;
        new_binding->action = action;
        new_binding->mateconf_key = mateconf_key;

        if (parse_binding (new_binding)) {
                if (!tmp_elem)
                        manager->priv->binding_list = g_slist_prepend (manager->priv->binding_list, new_binding);
        } else {
                g_free (new_binding->binding_str);
                g_free (new_binding->action);
                g_free (new_binding->mateconf_key);
                g_free (new_binding->previous_key.keycodes);
                g_free (new_binding);

                if (tmp_elem)
                        manager->priv->binding_list = g_slist_delete_link (manager->priv->binding_list, tmp_elem);
                return FALSE;
        }

        return TRUE;
}

static gboolean
same_keycode (const Key *key, const Key *other)
{
        if (key->keycodes != NULL && other->keycodes != NULL) {
                guint *c;

                for (c = key->keycodes; *c; ++c) {
                        if (key_uses_keycode (other, *c))
                                return TRUE;
                }
        }
        return FALSE;
}

static gboolean
same_key (const Key *key, const Key *other)
{
        if (key->state == other->state) {
                if (key->keycodes != NULL && other->keycodes != NULL) {
                        guint *c1, *c2;

                        for (c1 = key->keycodes, c2 = other->keycodes;
                             *c1 || *c2; ++c1, ++c2) {
                                     if (*c1 != *c2)
                                        return FALSE;
                        }
                } else if (key->keycodes != NULL || other->keycodes != NULL)
                        return FALSE;


                return TRUE;
        }

        return FALSE;
}

static gboolean
key_already_used (GsdKeybindingsManager *manager,
                  Binding               *binding)
{
        GSList *li;

        for (li = manager->priv->binding_list; li != NULL; li = li->next) {
                Binding *tmp_binding =  (Binding*) li->data;

                if (tmp_binding != binding &&
                    same_keycode (&tmp_binding->key, &binding->key) &&
                    tmp_binding->key.state == binding->key.state) {
                        return TRUE;
                }
        }

        return FALSE;
}

static void
binding_unregister_keys (GsdKeybindingsManager *manager)
{
        GSList *li;
        gboolean need_flush = FALSE;

        gdk_error_trap_push ();

        for (li = manager->priv->binding_list; li != NULL; li = li->next) {
                Binding *binding = (Binding *) li->data;

                if (binding->key.keycodes) {
                        need_flush = TRUE;
                        grab_key_unsafe (&binding->key, FALSE, manager->priv->screens);
                }
        }

        if (need_flush)
                gdk_flush ();
        gdk_error_trap_pop ();
}

static void
binding_register_keys (GsdKeybindingsManager *manager)
{
        GSList *li;
        gboolean need_flush = FALSE;

        gdk_error_trap_push ();

        /* Now check for changes and grab new key if not already used */
        for (li = manager->priv->binding_list; li != NULL; li = li->next) {
                Binding *binding = (Binding *) li->data;

		if (manager->priv->allowed_keys != NULL &&
                    !g_slist_find_custom (manager->priv->allowed_keys,
                                          binding->mateconf_key,
                                          (GCompareFunc) g_strcmp0)) {
                        continue;
		}

                if (!same_key (&binding->previous_key, &binding->key)) {
                        /* Ungrab key if it changed and not clashing with previously set binding */
                        if (!key_already_used (manager, binding)) {
                                gint i;

                                need_flush = TRUE;
                                if (binding->previous_key.keycodes) {
                                        grab_key_unsafe (&binding->previous_key, FALSE, manager->priv->screens);
                                }
                                grab_key_unsafe (&binding->key, TRUE, manager->priv->screens);

                                binding->previous_key.keysym = binding->key.keysym;
                                binding->previous_key.state = binding->key.state;
                                g_free (binding->previous_key.keycodes);
                                for (i = 0; binding->key.keycodes[i]; ++i);
                                binding->previous_key.keycodes = g_new0 (guint, i);
                                for (i = 0; binding->key.keycodes[i]; ++i)
                                        binding->previous_key.keycodes[i] = binding->key.keycodes[i];
                        } else
                                g_warning ("Key binding (%s) is already in use", binding->binding_str);
                }
        }

        if (need_flush)
                gdk_flush ();
        if (gdk_error_trap_pop ())
                g_warning ("Grab failed for some keys, another application may already have access the them.");
}

extern char **environ;

static char *
screen_exec_display_string (GdkScreen *screen)
{
        GString    *str;
        const char *old_display;
        char       *p;

        g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

        old_display = gdk_display_get_name (gdk_screen_get_display (screen));

        str = g_string_new ("DISPLAY=");
        g_string_append (str, old_display);

        p = strrchr (str->str, '.');
        if (p && p >  strchr (str->str, ':')) {
                g_string_truncate (str, p - str->str);
        }

        g_string_append_printf (str, ".%d", gdk_screen_get_number (screen));

        return g_string_free (str, FALSE);
}

/**
 * get_exec_environment:
 *
 * Description: Modifies the current program environment to
 * ensure that $DISPLAY is set such that a launched application
 * inheriting this environment would appear on screen.
 *
 * Returns: a newly-allocated %NULL-terminated array of strings or
 * %NULL on error. Use g_strfreev() to free it.
 *
 * mainly ripped from egg_screen_exec_display_string in
 * mate-panel/egg-screen-exec.c
 **/
static char **
get_exec_environment (XEvent *xevent)
{
        char     **retval = NULL;
        int        i;
        int        display_index = -1;
        GdkScreen *screen = NULL;
        GdkWindow *window = gdk_xid_table_lookup (xevent->xkey.root);

        if (window) {
                screen = gdk_drawable_get_screen (GDK_DRAWABLE (window));
        }

        g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

        for (i = 0; environ [i]; i++) {
                if (!strncmp (environ [i], "DISPLAY", 7)) {
                        display_index = i;
                }
        }

        if (display_index == -1) {
                display_index = i++;
        }

        retval = g_new (char *, i + 1);

        for (i = 0; environ [i]; i++) {
                if (i == display_index) {
                        retval [i] = screen_exec_display_string (screen);
                } else {
                        retval [i] = g_strdup (environ [i]);
                }
        }

        retval [i] = NULL;

        return retval;
}

static GdkFilterReturn
keybindings_filter (GdkXEvent             *gdk_xevent,
                    GdkEvent              *event,
                    GsdKeybindingsManager *manager)
{
        XEvent *xevent = (XEvent *) gdk_xevent;
        GSList *li;

        if (xevent->type != KeyPress) {
                return GDK_FILTER_CONTINUE;
        }

        for (li = manager->priv->binding_list; li != NULL; li = li->next) {
                Binding *binding = (Binding *) li->data;

                if (match_key (&binding->key, xevent)) {
                        GError  *error = NULL;
                        gboolean retval;
                        gchar  **argv = NULL;
                        gchar  **envp = NULL;

                        g_return_val_if_fail (binding->action != NULL, GDK_FILTER_CONTINUE);

                        if (!g_shell_parse_argv (binding->action,
                                                 NULL, &argv,
                                                 &error)) {
                                return GDK_FILTER_CONTINUE;
                        }

                        envp = get_exec_environment (xevent);

                        retval = g_spawn_async (NULL,
                                                argv,
                                                envp,
                                                G_SPAWN_SEARCH_PATH,
                                                NULL,
                                                NULL,
                                                NULL,
                                                &error);
                        g_strfreev (argv);
                        g_strfreev (envp);

                        if (!retval) {
                                GtkWidget *dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING,
                                                                            GTK_BUTTONS_CLOSE,
                                                                            _("Error while trying to run (%s)\n"\
                                                                              "which is linked to the key (%s)"),
                                                                            binding->action,
                                                                            binding->binding_str);
                                g_signal_connect (dialog,
                                                  "response",
                                                  G_CALLBACK (gtk_widget_destroy),
                                                  NULL);
                                gtk_widget_show (dialog);
                        }
                        return GDK_FILTER_REMOVE;
                }
        }
        return GDK_FILTER_CONTINUE;
}

static void
bindings_callback (MateConfClient           *client,
                   guint                  cnxn_id,
                   MateConfEntry            *entry,
                   GsdKeybindingsManager *manager)
{
        char** key_elems;
        char* binding_entry;

        if (strcmp (mateconf_entry_get_key (entry), ALLOWED_KEYS_KEY) == 0) {
                g_slist_foreach (manager->priv->allowed_keys, (GFunc)g_free, NULL);
                g_slist_free (manager->priv->allowed_keys);
                manager->priv->allowed_keys = mateconf_client_get_list (client,
                                                                     ALLOWED_KEYS_KEY,
                                                                     MATECONF_VALUE_STRING,
                                                                     NULL);
        }
        else {
                /* ensure we get binding dir not a sub component */
                key_elems = g_strsplit (mateconf_entry_get_key (entry), "/", 15);
                binding_entry = g_strdup_printf ("/%s/%s/%s/%s",
                                                 key_elems[1],
                                                 key_elems[2],
                                                 key_elems[3],
                                                 key_elems[4]);
                g_strfreev (key_elems);

                bindings_get_entry (manager, client, binding_entry);
                g_free (binding_entry);
        }

        binding_register_keys (manager);
}

static guint
register_config_callback (GsdKeybindingsManager   *manager,
                          MateConfClient             *client,
                          const char              *path,
                          MateConfClientNotifyFunc    func)
{
        mateconf_client_add_dir (client, path, MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);
        return mateconf_client_notify_add (client, path, func, manager, NULL, NULL);
}

gboolean
gsd_keybindings_manager_start (GsdKeybindingsManager *manager,
                               GError               **error)
{
        MateConfClient *client;
        GSList      *list;
        GSList      *li;
        GdkDisplay  *dpy;
        GdkScreen   *screen;
        int          screen_num;
        int          i;

        g_debug ("Starting keybindings manager");
        mate_settings_profile_start (NULL);

        client = mateconf_client_get_default ();

        manager->priv->notify = register_config_callback (manager,
                                                          client,
                                                          MATECONF_BINDING_DIR,
                                                          (MateConfClientNotifyFunc) bindings_callback);

        manager->priv->allowed_keys = mateconf_client_get_list (client,
                                                             ALLOWED_KEYS_KEY,
                                                             MATECONF_VALUE_STRING,
                                                             NULL);

        dpy = gdk_display_get_default ();
        screen_num = gdk_display_get_n_screens (dpy);

        for (i = 0; i < screen_num; i++) {
                screen = gdk_display_get_screen (dpy, i);
                gdk_window_add_filter (gdk_screen_get_root_window (screen),
                                       (GdkFilterFunc) keybindings_filter,
                                       manager);
        }

        list = mateconf_client_all_dirs (client, MATECONF_BINDING_DIR, NULL);
        manager->priv->screens = get_screens_list ();

        for (li = list; li != NULL; li = li->next) {
                bindings_get_entry (manager, client, li->data);
                g_free (li->data);
        }

        g_slist_free (list);
        g_object_unref (client);

        binding_register_keys (manager);

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_keybindings_manager_stop (GsdKeybindingsManager *manager)
{
        GsdKeybindingsManagerPrivate *p = manager->priv;
        GSList *l;

        g_debug ("Stopping keybindings manager");

        if (p->notify != 0) {
                MateConfClient *client = mateconf_client_get_default ();
                mateconf_client_remove_dir (client, MATECONF_BINDING_DIR, NULL);
                mateconf_client_notify_remove (client, p->notify);
                g_object_unref (client);
                p->notify = 0;
        }

        for (l = p->screens; l; l = l->next) {
                GdkScreen *screen = l->data;
                gdk_window_remove_filter (gdk_screen_get_root_window (screen),
                                          (GdkFilterFunc) keybindings_filter,
                                          manager);
        }

        binding_unregister_keys (manager);

        g_slist_free (p->screens);
        p->screens = NULL;

        for (l = p->binding_list; l; l = l->next) {
                Binding *b = l->data;
                g_free (b->binding_str);
                g_free (b->action);
                g_free (b->mateconf_key);
                g_free (b->previous_key.keycodes);
                g_free (b->key.keycodes);
                g_free (b);
        }
        g_slist_free (p->binding_list);
        p->binding_list = NULL;
}

static void
gsd_keybindings_manager_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        GsdKeybindingsManager *self;

        self = GSD_KEYBINDINGS_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_keybindings_manager_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        GsdKeybindingsManager *self;

        self = GSD_KEYBINDINGS_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_keybindings_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        GsdKeybindingsManager      *keybindings_manager;
        GsdKeybindingsManagerClass *klass;

        klass = GSD_KEYBINDINGS_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_KEYBINDINGS_MANAGER));

        keybindings_manager = GSD_KEYBINDINGS_MANAGER (G_OBJECT_CLASS (gsd_keybindings_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (keybindings_manager);
}

static void
gsd_keybindings_manager_dispose (GObject *object)
{
        GsdKeybindingsManager *keybindings_manager;

        keybindings_manager = GSD_KEYBINDINGS_MANAGER (object);

        G_OBJECT_CLASS (gsd_keybindings_manager_parent_class)->dispose (object);
}

static void
gsd_keybindings_manager_class_init (GsdKeybindingsManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_keybindings_manager_get_property;
        object_class->set_property = gsd_keybindings_manager_set_property;
        object_class->constructor = gsd_keybindings_manager_constructor;
        object_class->dispose = gsd_keybindings_manager_dispose;
        object_class->finalize = gsd_keybindings_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdKeybindingsManagerPrivate));
}

static void
gsd_keybindings_manager_init (GsdKeybindingsManager *manager)
{
        manager->priv = GSD_KEYBINDINGS_MANAGER_GET_PRIVATE (manager);

}

static void
gsd_keybindings_manager_finalize (GObject *object)
{
        GsdKeybindingsManager *keybindings_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_KEYBINDINGS_MANAGER (object));

        keybindings_manager = GSD_KEYBINDINGS_MANAGER (object);

        g_return_if_fail (keybindings_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_keybindings_manager_parent_class)->finalize (object);
}

GsdKeybindingsManager *
gsd_keybindings_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_KEYBINDINGS_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_KEYBINDINGS_MANAGER (manager_object);
}
