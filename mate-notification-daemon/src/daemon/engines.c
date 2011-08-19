/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Christian Hammond <chipx86@chipx86.com>
 * Copyright (C) 2005 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <mateconf/mateconf-client.h>
#include "daemon.h"
#include "engines.h"

typedef struct
{
        GModule        *module;
        guint           ref_count;

        gboolean        (*theme_check_init)           (unsigned int major_ver,
                                                       unsigned int minor_ver,
                                                       unsigned int micro_ver);
        void            (*get_theme_info)             (char       **theme_name,
                                                       char       **theme_ver,
                                                       char       **author,
                                                       char       **homepage);
        GtkWindow      *(*create_notification)        (UrlClickedCb url_clicked_cb);
        void            (*destroy_notification)       (GtkWindow   *nw);
        void            (*show_notification)          (GtkWindow   *nw);
        void            (*hide_notification)          (GtkWindow   *nw);
        void            (*set_notification_hints)     (GtkWindow   *nw,
                                                       GHashTable  *hints);
        void            (*set_notification_text)      (GtkWindow   *nw,
                                                       const char  *summary,
                                                       const char  *body);
        void            (*set_notification_icon)      (GtkWindow   *nw,
                                                       GdkPixbuf   *pixbuf);
        void            (*set_notification_arrow)     (GtkWindow   *nw,
                                                       gboolean     visible,
                                                       int          x,
                                                       int          y);
        void            (*add_notification_action)    (GtkWindow   *nw,
                                                       const char  *label,
                                                       const char  *key,
                                                       GCallback    cb);
        void            (*clear_notification_actions) (GtkWindow   *nw);
        void            (*move_notification)          (GtkWindow   *nw,
                                                       int          x,
                                                       int          y);
        void            (*set_notification_timeout)   (GtkWindow   *nw,
                                                       glong        timeout);
        void            (*notification_tick)          (GtkWindow   *nw,
                                                       glong        timeout);
        gboolean        (*get_always_stack)           (GtkWindow   *nw);

} ThemeEngine;

static guint        theme_prop_notify_id = 0;
static ThemeEngine *active_engine = NULL;

static ThemeEngine *
load_theme_engine (const char *name)
{
        ThemeEngine    *engine;
        char           *filename;
        char           *path;

        filename = g_strdup_printf ("lib%s.so", name);
        path = g_build_filename (ENGINES_DIR, filename, NULL);
        g_free (filename);

        engine = g_new0 (ThemeEngine, 1);
        engine->ref_count = 1;
        engine->module = g_module_open (path, G_MODULE_BIND_LAZY);

        g_free (path);

        if (engine->module == NULL)
                goto error;

#define BIND_REQUIRED_FUNC(name) \
        if (!g_module_symbol(engine->module, #name, (gpointer *)&engine->name)) \
        { \
                /* Too harsh! Fall back to default. */ \
                g_warning("Theme doesn't provide the required function '%s'", #name); \
                goto error; \
        }

#define BIND_OPTIONAL_FUNC(name) \
        g_module_symbol(engine->module, #name, (gpointer *)&engine->name);

        BIND_REQUIRED_FUNC (theme_check_init);
        BIND_REQUIRED_FUNC (get_theme_info);
        BIND_REQUIRED_FUNC (create_notification);
        BIND_REQUIRED_FUNC (set_notification_text);
        BIND_REQUIRED_FUNC (set_notification_icon);
        BIND_REQUIRED_FUNC (set_notification_arrow);
        BIND_REQUIRED_FUNC (add_notification_action);
        BIND_REQUIRED_FUNC (clear_notification_actions);
        BIND_REQUIRED_FUNC (move_notification);

        BIND_OPTIONAL_FUNC (destroy_notification);
        BIND_OPTIONAL_FUNC (show_notification);
        BIND_OPTIONAL_FUNC (hide_notification);
        BIND_OPTIONAL_FUNC (set_notification_timeout);
        BIND_OPTIONAL_FUNC (set_notification_hints);
        BIND_OPTIONAL_FUNC (notification_tick);
        BIND_OPTIONAL_FUNC (get_always_stack);

        if (!engine->theme_check_init (NOTIFICATION_DAEMON_MAJOR_VERSION,
                                       NOTIFICATION_DAEMON_MINOR_VERSION,
                                       NOTIFICATION_DAEMON_MICRO_VERSION)) {
                g_warning ("Theme doesn't work with this version of mate-notification-daemon");
                goto error;
        }

        return engine;

 error:
        if (engine->module != NULL && !g_module_close (engine->module))
                g_warning ("%s: %s", filename, g_module_error ());

        g_free (engine);
        return NULL;
}

static void
destroy_engine (ThemeEngine *engine)
{
        g_assert (engine->ref_count == 0);

        if (active_engine == engine)
                active_engine = NULL;

        g_module_close (engine->module);
        g_free (engine);
}

static gboolean
theme_engine_destroy (ThemeEngine *engine)
{
        destroy_engine (engine);
        return FALSE;
}

static void
theme_engine_unref (ThemeEngine *engine)
{
        engine->ref_count--;

        if (engine->ref_count == 0) {
                /*
                 * Destroy the engine in an idle loop since the last reference
                 * might have been the one of a notification which is being
                 * destroyed and that still has references to the engine
                 * module. This way, we're sure the notification is completely
                 * destroyed before the engine is.
                 */
                g_idle_add ((GSourceFunc) theme_engine_destroy, engine);
        }
}

static void
theme_changed_cb (MateConfClient *client,
                  guint        cnxn_id,
                  MateConfEntry  *entry,
                  gpointer     user_data)
{
        if (active_engine == NULL)
                return;

        theme_engine_unref (active_engine);

        /* This is no longer the true active engine, so reset this. */
        active_engine = NULL;
}

static ThemeEngine *
get_theme_engine (void)
{
        if (active_engine == NULL) {
                MateConfClient    *client;
                char           *enginename;

                client = mateconf_client_get_default ();
                enginename = mateconf_client_get_string (client,
                                                      MATECONF_KEY_THEME,
                                                      NULL);

                if (theme_prop_notify_id == 0) {
                        theme_prop_notify_id =
                                mateconf_client_notify_add (client,
                                                         MATECONF_KEY_THEME,
                                                         theme_changed_cb,
                                                         NULL,
                                                         NULL,
                                                         NULL);
                }

                g_object_unref (client);

                if (enginename == NULL) {
                        active_engine = load_theme_engine ("standard");
                        g_assert (active_engine != NULL);
                } else {
                        active_engine = load_theme_engine (enginename);

                        if (active_engine == NULL) {
                                g_warning ("Unable to load theme engine '%s'",
                                           enginename);
                                active_engine = load_theme_engine ("standard");
                        }

                        g_free (enginename);

                        g_assert (active_engine != NULL);
                }
        }

        return active_engine;
}

GtkWindow *
theme_create_notification (UrlClickedCb url_clicked_cb)
{
        ThemeEngine *engine;
        GtkWindow   *nw;

        engine = get_theme_engine ();
        nw = engine->create_notification (url_clicked_cb);
        g_object_set_data_full (G_OBJECT (nw),
                                "_theme_engine",
                                engine,
                                (GDestroyNotify) theme_engine_unref);
        engine->ref_count++;
        return nw;
}

void
theme_destroy_notification (GtkWindow *nw)
{
        ThemeEngine *engine;

        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");

        if (engine->destroy_notification != NULL)
                engine->destroy_notification (nw);
        else
                gtk_widget_destroy (GTK_WIDGET (nw));
}

void
theme_show_notification (GtkWindow *nw)
{
        ThemeEngine *engine;

        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");

        if (engine->show_notification != NULL)
                engine->show_notification (nw);
        else
                gtk_widget_show (GTK_WIDGET (nw));
}

void
theme_hide_notification (GtkWindow *nw)
{
        ThemeEngine *engine;

        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");

        if (engine->hide_notification != NULL)
                engine->hide_notification (nw);
        else
                gtk_widget_hide (GTK_WIDGET (nw));
}

void
theme_set_notification_hints (GtkWindow  *nw,
                              GHashTable *hints)
{
        ThemeEngine *engine;

        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");

        if (engine->set_notification_hints != NULL)
                engine->set_notification_hints (nw, hints);
}

void
theme_set_notification_timeout (GtkWindow *nw, glong timeout)
{
        ThemeEngine *engine;

        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");

        if (engine->set_notification_timeout != NULL)
                engine->set_notification_timeout (nw, timeout);
}

void
theme_notification_tick (GtkWindow *nw,
                         glong      remaining)
{
        ThemeEngine *engine;

        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");

        if (engine->notification_tick != NULL)
                engine->notification_tick (nw, remaining);
}

void
theme_set_notification_text (GtkWindow  *nw,
                             const char *summary,
                             const char *body)
{
        ThemeEngine *engine;
        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");
        engine->set_notification_text (nw, summary, body);
}

void
theme_set_notification_icon (GtkWindow *nw,
                             GdkPixbuf *pixbuf)
{
        ThemeEngine *engine;
        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");
        engine->set_notification_icon (nw, pixbuf);
}

void
theme_set_notification_arrow (GtkWindow *nw,
                              gboolean   visible,
                              int        x,
                              int        y)
{
        ThemeEngine *engine;
        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");
        engine->set_notification_arrow (nw, visible, x, y);
}

void
theme_add_notification_action (GtkWindow  *nw,
                               const char *label,
                               const char *key,
                               GCallback   cb)
{
        ThemeEngine *engine;
        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");
        engine->add_notification_action (nw, label, key, cb);
}

void
theme_clear_notification_actions (GtkWindow *nw)
{
        ThemeEngine *engine;

        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");
        engine->clear_notification_actions (nw);
}

void
theme_move_notification (GtkWindow *nw,
                         int        x,
                         int        y)
{
        ThemeEngine *engine;
        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");
        engine->move_notification (nw, x, y);
}

gboolean
theme_get_always_stack (GtkWindow *nw)
{
        ThemeEngine *engine;

        engine = g_object_get_data (G_OBJECT (nw), "_theme_engine");

        if (engine->get_always_stack != NULL)
                return engine->get_always_stack (nw);
        else
                return FALSE;
}
