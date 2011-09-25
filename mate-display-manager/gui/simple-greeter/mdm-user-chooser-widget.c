/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2007 Ray Strode <rstrode@redhat.com>
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include <mateconf/mateconf-client.h>

#include "mdm-user-manager.h"
#include "mdm-user-chooser-widget.h"
#include "mdm-settings-keys.h"
#include "mdm-settings-client.h"

#define KEY_DISABLE_USER_LIST "/apps/mdm/simple-greeter/disable_user_list"

enum {
        USER_NO_DISPLAY              = 1 << 0,
        USER_ACCOUNT_DISABLED        = 1 << 1,
};

#define DEFAULT_USER_ICON "avatar-default"
#define OLD_DEFAULT_USER_ICON "stock_person"

#define MDM_USER_CHOOSER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_USER_CHOOSER_WIDGET, MdmUserChooserWidgetPrivate))

#define MAX_ICON_SIZE 128
#define NUM_USERS_TO_ADD_PER_ITERATION 50

struct MdmUserChooserWidgetPrivate
{
        MdmUserManager *manager;
        GtkIconTheme   *icon_theme;

        GSList         *users_to_add;

        GdkPixbuf      *logged_in_pixbuf;
        GdkPixbuf      *stock_person_pixbuf;

        guint           loaded : 1;
        guint           show_user_other : 1;
        guint           show_user_guest : 1;
        guint           show_user_auto : 1;
        guint           show_normal_users : 1;

        guint           has_user_other : 1;

        guint           update_other_visibility_idle_id;
        guint           load_idle_id;
        guint           add_users_idle_id;
};

enum {
        PROP_0,
        PROP_SHOW_USER_GUEST,
        PROP_SHOW_USER_AUTO,
        PROP_SHOW_USER_OTHER,
};

static void     mdm_user_chooser_widget_class_init  (MdmUserChooserWidgetClass *klass);
static void     mdm_user_chooser_widget_init        (MdmUserChooserWidget      *user_chooser_widget);
static void     mdm_user_chooser_widget_finalize    (GObject                   *object);

G_DEFINE_TYPE (MdmUserChooserWidget, mdm_user_chooser_widget, MDM_TYPE_CHOOSER_WIDGET)

static void     add_user_other    (MdmUserChooserWidget *widget);
static void     remove_user_other (MdmUserChooserWidget *widget);

static int
get_font_height_for_widget (GtkWidget *widget)
{
        PangoFontMetrics *metrics;
        PangoContext     *context;
        int               ascent;
        int               descent;
        int               height;

        gtk_widget_ensure_style (widget);
        context = gtk_widget_get_pango_context (widget);
        metrics = pango_context_get_metrics (context,
                                             gtk_widget_get_style (widget)->font_desc,
                                             pango_context_get_language (context));

        ascent = pango_font_metrics_get_ascent (metrics);
        descent = pango_font_metrics_get_descent (metrics);
        height = PANGO_PIXELS (ascent + descent);
        pango_font_metrics_unref (metrics);
        return height;
}

static int
get_icon_height_for_widget (GtkWidget *widget)
{
        int font_height;
        int height;

        font_height = get_font_height_for_widget (widget);
        height = 3 * font_height;
        if (height > MAX_ICON_SIZE) {
                height = MAX_ICON_SIZE;
        }

        g_debug ("MdmUserChooserWidget: font height %d; using icon size %d", font_height, height);

        return height;
}

static gboolean
update_other_user_visibility (MdmUserChooserWidget *widget)
{
        int number_of_users;

        g_debug ("MdmUserChooserWidget: updating other user visibility");

        if (!widget->priv->show_user_other) {
                if (widget->priv->has_user_other) {
                        remove_user_other (widget);
                }

                goto out;
        }

        /* Always show the Other user if requested */
        if (!widget->priv->has_user_other) {
                add_user_other (widget);
        }

 out:
        widget->priv->update_other_visibility_idle_id = 0;
        return FALSE;
}

static void
queue_update_other_user_visibility (MdmUserChooserWidget *widget)
{
        if (widget->priv->update_other_visibility_idle_id == 0) {
                widget->priv->update_other_visibility_idle_id =
                        g_idle_add ((GSourceFunc) update_other_user_visibility, widget);
        }
}

static void
update_item_for_user (MdmUserChooserWidget *widget,
                      MdmUser              *user)
{
        GdkPixbuf    *pixbuf;
        char         *tooltip;
        gboolean      is_logged_in;
        int           size;
        char         *escaped_username;
        char         *escaped_real_name;

        if (!mdm_user_is_loaded (user)) {
                return;
        }

        size = get_icon_height_for_widget (GTK_WIDGET (widget));
        pixbuf = mdm_user_render_icon (user, size);

        if (pixbuf == NULL && widget->priv->stock_person_pixbuf != NULL) {
                pixbuf = g_object_ref (widget->priv->stock_person_pixbuf);
        }

        tooltip = g_strdup_printf (_("Log in as %s"),
                                   mdm_user_get_user_name (user));

        is_logged_in = mdm_user_is_logged_in (user);

        g_debug ("MdmUserChooserWidget: User added name:%s logged-in:%d pixbuf:%p",
                 mdm_user_get_user_name (user),
                 is_logged_in,
                 pixbuf);

        escaped_username = g_markup_escape_text (mdm_user_get_user_name (user), -1);
        escaped_real_name = g_markup_escape_text (mdm_user_get_real_name (user), -1);
        mdm_chooser_widget_update_item (MDM_CHOOSER_WIDGET (widget),
                                        escaped_username,
                                        pixbuf,
                                        escaped_real_name,
                                        tooltip,
                                        mdm_user_get_login_frequency (user),
                                        is_logged_in,
                                        FALSE);
        g_free (escaped_real_name);
        g_free (escaped_username);
        g_free (tooltip);

        if (pixbuf != NULL) {
                g_object_unref (pixbuf);
        }
}

static void
on_item_load (MdmChooserWidget     *widget,
              const char           *id,
              MdmUserChooserWidget *user_chooser)
{
        MdmUser *user;

        g_debug ("MdmUserChooserWidget: Loading item for id=%s", id);

        if (user_chooser->priv->manager == NULL) {
                return;
        }

        if (strcmp (id, MDM_USER_CHOOSER_USER_OTHER) == 0) {
                return;
        }

        if (strcmp (id, MDM_USER_CHOOSER_USER_GUEST) == 0) {
                return;
        }

        user = mdm_user_manager_get_user (user_chooser->priv->manager, id);
        if (user != NULL) {
                update_item_for_user (user_chooser, user);
        }
}

static void
add_user_other (MdmUserChooserWidget *widget)
{
        widget->priv->has_user_other = TRUE;
        mdm_chooser_widget_add_item (MDM_CHOOSER_WIDGET (widget),
                                     MDM_USER_CHOOSER_USER_OTHER,
                                     NULL,
                                     /* translators: This option prompts
                                      * the user to type in a username
                                      * manually instead of choosing from
                                      * a list.
                                      */
                                     C_("user", "Other…"),
                                     _("Choose a different account"),
                                     0,
                                     FALSE,
                                     TRUE,
                                     (MdmChooserWidgetItemLoadFunc) on_item_load,
                                     widget);
}

static void
add_user_guest (MdmUserChooserWidget *widget)
{
        mdm_chooser_widget_add_item (MDM_CHOOSER_WIDGET (widget),
                                     MDM_USER_CHOOSER_USER_GUEST,
                                     widget->priv->stock_person_pixbuf,
                                     _("Guest"),
                                     _("Log in as a temporary guest"),
                                     0,
                                     FALSE,
                                     TRUE,
                                     (MdmChooserWidgetItemLoadFunc) on_item_load,
                                     widget);
        queue_update_other_user_visibility (widget);
}

static void
add_user_auto (MdmUserChooserWidget *widget)
{
        mdm_chooser_widget_add_item (MDM_CHOOSER_WIDGET (widget),
                                     MDM_USER_CHOOSER_USER_AUTO,
                                     NULL,
                                     _("Automatic Login"),
                                     _("Automatically log into the system after selecting options"),
                                     0,
                                     FALSE,
                                     TRUE,
                                     (MdmChooserWidgetItemLoadFunc) on_item_load,
                                     widget);
        queue_update_other_user_visibility (widget);
}

static void
remove_user_other (MdmUserChooserWidget *widget)
{
        widget->priv->has_user_other = FALSE;
        mdm_chooser_widget_remove_item (MDM_CHOOSER_WIDGET (widget),
                                        MDM_USER_CHOOSER_USER_OTHER);
}

static void
remove_user_guest (MdmUserChooserWidget *widget)
{
        mdm_chooser_widget_remove_item (MDM_CHOOSER_WIDGET (widget),
                                        MDM_USER_CHOOSER_USER_GUEST);
        queue_update_other_user_visibility (widget);
}

static void
remove_user_auto (MdmUserChooserWidget *widget)
{
        mdm_chooser_widget_remove_item (MDM_CHOOSER_WIDGET (widget),
                                        MDM_USER_CHOOSER_USER_AUTO);
        queue_update_other_user_visibility (widget);
}

void
mdm_user_chooser_widget_set_show_user_other (MdmUserChooserWidget *widget,
                                             gboolean              show_user)
{
        g_return_if_fail (MDM_IS_USER_CHOOSER_WIDGET (widget));

        if (widget->priv->show_user_other != show_user) {
                widget->priv->show_user_other = show_user;
                queue_update_other_user_visibility (widget);
        }
}

void
mdm_user_chooser_widget_set_show_user_guest (MdmUserChooserWidget *widget,
                                             gboolean              show_user)
{
        g_return_if_fail (MDM_IS_USER_CHOOSER_WIDGET (widget));

        if (widget->priv->show_user_guest != show_user) {
                widget->priv->show_user_guest = show_user;
                if (show_user) {
                        add_user_guest (widget);
                } else {
                        remove_user_guest (widget);
                }
        }
}

void
mdm_user_chooser_widget_set_show_user_auto (MdmUserChooserWidget *widget,
                                            gboolean              show_user)
{
        g_return_if_fail (MDM_IS_USER_CHOOSER_WIDGET (widget));

        if (widget->priv->show_user_auto != show_user) {
                widget->priv->show_user_auto = show_user;
                if (show_user) {
                        add_user_auto (widget);
                } else {
                        remove_user_auto (widget);
                }
        }
}

char *
mdm_user_chooser_widget_get_chosen_user_name (MdmUserChooserWidget *widget)
{
        g_return_val_if_fail (MDM_IS_USER_CHOOSER_WIDGET (widget), NULL);

        return mdm_chooser_widget_get_active_item (MDM_CHOOSER_WIDGET (widget));
}

void
mdm_user_chooser_widget_set_chosen_user_name (MdmUserChooserWidget *widget,
                                              const char           *name)
{
        g_return_if_fail (MDM_IS_USER_CHOOSER_WIDGET (widget));

        mdm_chooser_widget_set_active_item (MDM_CHOOSER_WIDGET (widget), name);
}

void
mdm_user_chooser_widget_set_show_only_chosen (MdmUserChooserWidget *widget,
                                              gboolean              show_only) {
        g_return_if_fail (MDM_IS_USER_CHOOSER_WIDGET (widget));

        mdm_chooser_widget_set_hide_inactive_items (MDM_CHOOSER_WIDGET (widget),
                                                    show_only);

}
static void
mdm_user_chooser_widget_set_property (GObject        *object,
                                      guint           prop_id,
                                      const GValue   *value,
                                      GParamSpec     *pspec)
{
        MdmUserChooserWidget *self;

        self = MDM_USER_CHOOSER_WIDGET (object);

        switch (prop_id) {
        case PROP_SHOW_USER_AUTO:
                mdm_user_chooser_widget_set_show_user_auto (self, g_value_get_boolean (value));
                break;
        case PROP_SHOW_USER_GUEST:
                mdm_user_chooser_widget_set_show_user_guest (self, g_value_get_boolean (value));
                break;
        case PROP_SHOW_USER_OTHER:
                mdm_user_chooser_widget_set_show_user_other (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_user_chooser_widget_get_property (GObject        *object,
                                      guint           prop_id,
                                      GValue         *value,
                                      GParamSpec     *pspec)
{
        MdmUserChooserWidget *self;

        self = MDM_USER_CHOOSER_WIDGET (object);

        switch (prop_id) {
        case PROP_SHOW_USER_AUTO:
                g_value_set_boolean (value, self->priv->show_user_auto);
                break;
        case PROP_SHOW_USER_GUEST:
                g_value_set_boolean (value, self->priv->show_user_guest);
                break;
        case PROP_SHOW_USER_OTHER:
                g_value_set_boolean (value, self->priv->show_user_other);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
is_user_list_disabled (MdmUserChooserWidget *widget)
{
        MateConfClient *client;
        GError      *error;
        gboolean     result;

        client = mateconf_client_get_default ();
        error = NULL;
        result = mateconf_client_get_bool (client, KEY_DISABLE_USER_LIST, &error);
        if (error != NULL) {
                g_debug ("MdmUserChooserWidget: unable to get disable-user-list configuration: %s", error->message);
                g_error_free (error);
        }
        g_object_unref (client);

        return result;
}

static void
add_user (MdmUserChooserWidget *widget,
          MdmUser              *user)
{
        GdkPixbuf    *pixbuf;
        char         *tooltip;
        gboolean      is_logged_in;
        char         *escaped_username;
        char         *escaped_real_name;

        if (!widget->priv->show_normal_users) {
                return;
        }

        pixbuf = g_object_ref (widget->priv->stock_person_pixbuf);

        tooltip = g_strdup_printf (_("Log in as %s"),
                                   mdm_user_get_user_name (user));

        is_logged_in = mdm_user_is_logged_in (user);

        escaped_username = g_markup_escape_text (mdm_user_get_user_name (user), -1);
        escaped_real_name = g_markup_escape_text (mdm_user_get_real_name (user), -1);
        mdm_chooser_widget_add_item (MDM_CHOOSER_WIDGET (widget),
                                     escaped_username,
                                     pixbuf,
                                     escaped_real_name,
                                     tooltip,
                                     mdm_user_get_login_frequency (user),
                                     is_logged_in,
                                     FALSE,
                                     (MdmChooserWidgetItemLoadFunc) on_item_load,
                                     widget);
        g_free (escaped_real_name);
        g_free (escaped_username);
        g_free (tooltip);

        if (pixbuf != NULL) {
                g_object_unref (pixbuf);
        }

        queue_update_other_user_visibility (widget);
}

static void
on_user_added (MdmUserManager       *manager,
               MdmUser              *user,
               MdmUserChooserWidget *widget)
{
        /* wait for all users to be loaded */
        if (! widget->priv->loaded) {
                return;
        }
        add_user (widget, user);
}

static void
on_user_removed (MdmUserManager       *manager,
                 MdmUser              *user,
                 MdmUserChooserWidget *widget)
{
        const char *user_name;

        g_debug ("MdmUserChooserWidget: User removed: %s", mdm_user_get_user_name (user));
        /* wait for all users to be loaded */
        if (! widget->priv->loaded) {
                return;
        }

        user_name = mdm_user_get_user_name (user);

        mdm_chooser_widget_remove_item (MDM_CHOOSER_WIDGET (widget),
                                        user_name);

        queue_update_other_user_visibility (widget);
}

static void
on_user_is_logged_in_changed (MdmUserManager       *manager,
                              MdmUser              *user,
                              MdmUserChooserWidget *widget)
{
        const char *user_name;
        gboolean    is_logged_in;

        g_debug ("MdmUserChooserWidget: User logged in changed: %s", mdm_user_get_user_name (user));

        user_name = mdm_user_get_user_name (user);
        is_logged_in = mdm_user_is_logged_in (user);

        mdm_chooser_widget_set_item_in_use (MDM_CHOOSER_WIDGET (widget),
                                            user_name,
                                            is_logged_in);
}

static void
on_user_changed (MdmUserManager       *manager,
                 MdmUser              *user,
                 MdmUserChooserWidget *widget)
{
        /* wait for all users to be loaded */
        if (! widget->priv->loaded) {
                return;
        }
        if (! widget->priv->show_normal_users) {
                return;
        }

        update_item_for_user (widget, user);
}

static gboolean
add_users (MdmUserChooserWidget *widget)
{
        guint cnt;

        cnt = 0;
        while (widget->priv->users_to_add != NULL && cnt < NUM_USERS_TO_ADD_PER_ITERATION) {
                add_user (widget, widget->priv->users_to_add->data);
                g_object_unref (widget->priv->users_to_add->data);
                widget->priv->users_to_add = g_slist_delete_link (widget->priv->users_to_add, widget->priv->users_to_add);
                cnt++;
        }
        g_debug ("MdmUserChooserWidget: added %u items", cnt);

        if (! widget->priv->loaded) {
                widget->priv->loaded = TRUE;

                mdm_chooser_widget_loaded (MDM_CHOOSER_WIDGET (widget));
        }

        return (widget->priv->users_to_add != NULL);
}

static void
queue_add_users (MdmUserChooserWidget *widget)
{
        if (widget->priv->add_users_idle_id == 0) {
                widget->priv->add_users_idle_id = g_idle_add ((GSourceFunc) add_users, widget);
        }
}

static void
on_is_loaded_changed (MdmUserManager       *manager,
                      GParamSpec           *pspec,
                      MdmUserChooserWidget *widget)
{
        GSList *users;

        /* FIXME: handle is-loaded=FALSE */

        g_debug ("MdmUserChooserWidget: Users loaded");

        users = mdm_user_manager_list_users (manager);
        g_slist_foreach (users, (GFunc) g_object_ref, NULL);
        widget->priv->users_to_add = g_slist_concat (widget->priv->users_to_add, g_slist_copy (users));

        queue_add_users (widget);
}

static void
parse_string_list (char *value, GSList **retval)
{
        char **temp_array;
        int    i;

        *retval = NULL;

        if (value == NULL || *value == '\0') {
                g_debug ("Not adding NULL user");
                *retval = NULL;
                return;
        }

        temp_array = g_strsplit (value, ",", 0);
        for (i = 0; temp_array[i] != NULL; i++) {
                g_debug ("Adding value %s", temp_array[i]);
                g_strstrip (temp_array[i]);
                *retval = g_slist_prepend (*retval, g_strdup (temp_array[i]));
        }

        g_strfreev (temp_array);
}

static gboolean
load_users (MdmUserChooserWidget *widget)
{

        if (widget->priv->show_normal_users) {
                char          *temp;
                gboolean       res;
                gboolean       include_all;
                GSList        *includes;
                GSList        *excludes;

                widget->priv->manager = mdm_user_manager_ref_default ();

                /* exclude/include */
                g_debug ("Setting users to include:");
                res = mdm_settings_client_get_string (MDM_KEY_INCLUDE,
                                                      &temp);
                parse_string_list (temp, &includes);

                g_debug ("Setting users to exclude:");
                res = mdm_settings_client_get_string  (MDM_KEY_EXCLUDE,
                                                       &temp);
                parse_string_list (temp, &excludes);

                include_all = FALSE;
                res = mdm_settings_client_get_boolean (MDM_KEY_INCLUDE_ALL,
                                                       &include_all);
                g_object_set (widget->priv->manager,
                              "include-all", include_all,
                              "include-usernames-list", includes,
                              "exclude-usernames-list", excludes,
                              NULL);

                g_slist_foreach (includes, (GFunc) g_free, NULL);
                g_slist_free (includes);
                g_slist_foreach (excludes, (GFunc) g_free, NULL);
                g_slist_free (excludes);

                g_signal_connect (widget->priv->manager,
                                  "user-added",
                                  G_CALLBACK (on_user_added),
                                  widget);
                g_signal_connect (widget->priv->manager,
                                  "user-removed",
                                  G_CALLBACK (on_user_removed),
                                  widget);
                g_signal_connect (widget->priv->manager,
                                  "notify::is-loaded",
                                  G_CALLBACK (on_is_loaded_changed),
                                  widget);
                g_signal_connect (widget->priv->manager,
                                  "user-is-logged-in-changed",
                                  G_CALLBACK (on_user_is_logged_in_changed),
                                  widget);
                g_signal_connect (widget->priv->manager,
                                  "user-changed",
                                  G_CALLBACK (on_user_changed),
                                  widget);
                mdm_user_manager_queue_load (widget->priv->manager);
        } else {
                mdm_chooser_widget_loaded (MDM_CHOOSER_WIDGET (widget));
        }

        widget->priv->load_idle_id = 0;

        return FALSE;
}

static GObject *
mdm_user_chooser_widget_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        MdmUserChooserWidget      *widget;

        widget = MDM_USER_CHOOSER_WIDGET (G_OBJECT_CLASS (mdm_user_chooser_widget_parent_class)->constructor (type,
                                                                                                              n_construct_properties,
                                                                                                              construct_properties));

        widget->priv->show_normal_users = !is_user_list_disabled (widget);

        widget->priv->load_idle_id = g_idle_add ((GSourceFunc)load_users, widget);

        return G_OBJECT (widget);
}

static void
mdm_user_chooser_widget_dispose (GObject *object)
{
        MdmUserChooserWidget *widget;

        widget = MDM_USER_CHOOSER_WIDGET (object);

        G_OBJECT_CLASS (mdm_user_chooser_widget_parent_class)->dispose (object);

        if (widget->priv->load_idle_id > 0) {
                g_source_remove (widget->priv->load_idle_id);
                widget->priv->load_idle_id = 0;
        }

        if (widget->priv->add_users_idle_id > 0) {
                g_source_remove (widget->priv->add_users_idle_id);
                widget->priv->add_users_idle_id = 0;
        }

        if (widget->priv->update_other_visibility_idle_id > 0) {
                g_source_remove (widget->priv->update_other_visibility_idle_id);
                widget->priv->update_other_visibility_idle_id = 0;
        }

        if (widget->priv->users_to_add != NULL) {
                g_slist_foreach (widget->priv->users_to_add, (GFunc) g_object_ref, NULL);
                g_slist_free (widget->priv->users_to_add);
                widget->priv->users_to_add = NULL;
        }

        if (widget->priv->logged_in_pixbuf != NULL) {
                g_object_unref (widget->priv->logged_in_pixbuf);
                widget->priv->logged_in_pixbuf = NULL;
        }

        if (widget->priv->stock_person_pixbuf != NULL) {
                g_object_unref (widget->priv->stock_person_pixbuf);
                widget->priv->stock_person_pixbuf = NULL;
        }

        if (widget->priv->manager != NULL) {
                g_object_unref (widget->priv->manager);
                widget->priv->manager = NULL;
        }
}

static void
mdm_user_chooser_widget_class_init (MdmUserChooserWidgetClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_user_chooser_widget_get_property;
        object_class->set_property = mdm_user_chooser_widget_set_property;
        object_class->constructor = mdm_user_chooser_widget_constructor;
        object_class->dispose = mdm_user_chooser_widget_dispose;
        object_class->finalize = mdm_user_chooser_widget_finalize;


        g_object_class_install_property (object_class,
                                         PROP_SHOW_USER_AUTO,
                                         g_param_spec_boolean ("show-user-auto",
                                                               "show user auto",
                                                               "show user auto",
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_SHOW_USER_GUEST,
                                         g_param_spec_boolean ("show-user-guest",
                                                               "show user guest",
                                                               "show user guest",
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_SHOW_USER_OTHER,
                                         g_param_spec_boolean ("show-user-other",
                                                               "show user other",
                                                               "show user other",
                                                               TRUE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (MdmUserChooserWidgetPrivate));
}

static GdkPixbuf *
get_pixbuf_from_icon_names (MdmUserChooserWidget *widget,
                            const char           *first_name,
                            ...)
{
        GdkPixbuf   *pixbuf;
        GtkIconInfo *icon_info;
        GPtrArray   *array;
        int          size;
        const char  *icon_name;
        va_list      argument_list;

        array = g_ptr_array_new ();

        g_ptr_array_add (array, (gpointer) first_name);

        va_start (argument_list, first_name);
        icon_name = (const char *) va_arg (argument_list, const char *);
        while (icon_name != NULL) {
                g_ptr_array_add (array, (gpointer) icon_name);
                icon_name = (const char *) va_arg (argument_list, const char *);
        }
        va_end (argument_list);
        g_ptr_array_add (array, NULL);

        size = get_icon_height_for_widget (GTK_WIDGET (widget));

        icon_info = gtk_icon_theme_choose_icon (widget->priv->icon_theme,
                                                (const char **) array->pdata,
                                                size,
                                                GTK_ICON_LOOKUP_GENERIC_FALLBACK);
        g_ptr_array_free (array, FALSE);

        if (icon_info != NULL) {
                GError *error;

                error = NULL;
                pixbuf = gtk_icon_info_load_icon (icon_info, &error);
                gtk_icon_info_free (icon_info);

                if (error != NULL) {
                        g_warning ("Could not load icon '%s': %s",
                                   first_name, error->message);
                        g_error_free (error);
                }
        } else {
                g_warning ("Could not find icon '%s' or fallbacks", first_name);
                pixbuf = NULL;
        }

        return pixbuf;
}

static GdkPixbuf *
get_stock_person_pixbuf (MdmUserChooserWidget *widget)
{
        GdkPixbuf   *pixbuf;

        pixbuf = get_pixbuf_from_icon_names (widget,
                                             DEFAULT_USER_ICON,
                                             OLD_DEFAULT_USER_ICON,
                                             NULL);

        return pixbuf;
}

static GdkPixbuf *
get_logged_in_pixbuf (MdmUserChooserWidget *widget)
{
        GdkPixbuf *pixbuf;

        pixbuf = get_pixbuf_from_icon_names (widget,
                                             DEFAULT_USER_ICON,
                                             "emblem-default",
                                             NULL);

        return pixbuf;
}

typedef struct {
        GdkPixbuf *old_icon;
        GdkPixbuf *new_icon;
} IconUpdateData;

static gboolean
update_icons (MdmChooserWidget *widget,
              const char       *id,
              GdkPixbuf       **image,
              char            **name,
              char            **comment,
              gulong           *priority,
              gboolean         *is_in_use,
              gboolean         *is_separate,
              IconUpdateData   *data)
{
        if (data->old_icon == *image) {
                *image = data->new_icon;
                return TRUE;
        }

        return FALSE;
}

static void
load_icons (MdmUserChooserWidget *widget)
{
        GdkPixbuf     *old_pixbuf;
        IconUpdateData data;

        if (widget->priv->logged_in_pixbuf != NULL) {
                g_object_unref (widget->priv->logged_in_pixbuf);
        }
        widget->priv->logged_in_pixbuf = get_logged_in_pixbuf (widget);

        old_pixbuf = widget->priv->stock_person_pixbuf;
        widget->priv->stock_person_pixbuf = get_stock_person_pixbuf (widget);
        /* update the icons in the model */
        data.old_icon = old_pixbuf;
        data.new_icon = widget->priv->stock_person_pixbuf;
        mdm_chooser_widget_update_foreach_item (MDM_CHOOSER_WIDGET (widget),
                                                (MdmChooserUpdateForeachFunc)update_icons,
                                                &data);
        if (old_pixbuf != NULL) {
                g_object_unref (old_pixbuf);
        }
}

static void
on_icon_theme_changed (GtkIconTheme         *icon_theme,
                       MdmUserChooserWidget *widget)
{
        g_debug ("MdmUserChooserWidget: icon theme changed");
        load_icons (widget);
}

static void
setup_icons (MdmUserChooserWidget *widget)
{
        if (gtk_widget_has_screen (GTK_WIDGET (widget))) {
                widget->priv->icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (widget)));
        } else {
                widget->priv->icon_theme = gtk_icon_theme_get_default ();
        }

        if (widget->priv->icon_theme != NULL) {
                g_signal_connect (widget->priv->icon_theme,
                                  "changed",
                                  G_CALLBACK (on_icon_theme_changed),
                                  widget);
        }

        load_icons (widget);
}

static void
on_list_visible_changed (MdmChooserWidget *widget,
                         GParamSpec       *pspec,
                         gpointer          data)
{
        gboolean is_visible;

        g_object_get (G_OBJECT (widget), "list-visible", &is_visible, NULL);
        if (is_visible) {
                gtk_widget_grab_focus (GTK_WIDGET (widget));
        }
}

static void
mdm_user_chooser_widget_init (MdmUserChooserWidget *widget)
{
        widget->priv = MDM_USER_CHOOSER_WIDGET_GET_PRIVATE (widget);

        mdm_chooser_widget_set_separator_position (MDM_CHOOSER_WIDGET (widget),
                                                   MDM_CHOOSER_WIDGET_POSITION_BOTTOM);
        mdm_chooser_widget_set_in_use_message (MDM_CHOOSER_WIDGET (widget),
                                               _("Currently logged in"));

        g_signal_connect (widget,
                          "notify::list-visible",
                          G_CALLBACK (on_list_visible_changed),
                          NULL);

        setup_icons (widget);
}

static void
mdm_user_chooser_widget_finalize (GObject *object)
{
        MdmUserChooserWidget *widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_USER_CHOOSER_WIDGET (object));

        widget = MDM_USER_CHOOSER_WIDGET (object);

        g_return_if_fail (widget->priv != NULL);

        G_OBJECT_CLASS (mdm_user_chooser_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_user_chooser_widget_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_USER_CHOOSER_WIDGET,
                               NULL);

        return GTK_WIDGET (object);
}
