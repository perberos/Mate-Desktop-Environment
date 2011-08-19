/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/***************************************************************************
 *
 * polkit-mate-action.c : 
 *
 * Copyright (C) 2007 David Zeuthen, <david@fubar.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 **************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <polkit/polkit.h>
#include <polkit-dbus/polkit-dbus.h>

#include "polkit-mate-context.h"
#include "polkit-mate-action.h"
#include "polkit-mate-auth.h"

/**
 * SECTION:polkit-mate-action
 * @short_description: A GtkAction subclass where instances can be tied to a specific PolicyKit action.
 *
 * Actions in GTK+ represent operations that the user can be perform,
 * along with some information how it should be presented in the
 * interface. Each action provides methods to create icons, menu items
 * and toolbar items representing itself. Each action can have one or
 * more proxy menu item, toolbar button or other proxy
 * widgets. Proxies mirror the state of the action (text label,
 * tooltip, icon, visible, sensitive, etc), and should change when the
 * action's state changes. When the proxy is activated, it should
 * activate its action.
 *
 * Instances of #PolKitMateAction class updates the label, tooltip,
 * icon-name, visible and properties of the parent #GtkAction instance
 * according to what result PolicyKit gives about a given
 * #PolKitAction object. The #PolKitMateContext class is used
 * internally to track changes. This means that external events (such
 * as the editing of the /etc/PolicyKit/PolicyKit.conf file,
 * ConsoleKit session activity changes or if the user gains a
 * privilege via authentication) will trigger the action, and thus
 * connected proxy widgets, to be updated.
 *
 * In addition, the #PolKitMateAction class intercepts the ::activate
 * signal defined in #GtkAction. When the result from PolicyKit is
 * yes, the signal is propagated. If the result is auth,
 * polkit_mate_auth_show_dialog() will be used to bring up an
 * authentication dialog for the given #PolKitAction. If the user
 * succesfully gained the privilege, a ::activate signal will be
 * synthesized. If the result is no, the signal is also propagated.
 *
 * As a result, everything happens under the covers; the application
 * programmer using #PolKitMateAction will only get the ::activate
 * signal when the answer from PolicyKit is yes and as such don't have
 * to worry about bringing up authentication dialogs if the property
 * "no-sensitive" is set to #FALSE.
 *
 * When an authentication dialog is show, the #PolKitMateAction class
 * will pass the XID of the top-level window that the proxy widget
 * causing the activation to polkit_mate_auth_show_dialog().
 *
 * An example of how to use #PolKitMateAction follows. First, build
 * the following program
 * 
 * <programlisting><xi:include xmlns:xi="http://www.w3.org/2001/XInclude" href="../../examples/polkit-mate-example.c" parse="text"><xi:fallback>FIXME: MISSING XINCLUDE CONTENT</xi:fallback></xi:include></programlisting>
 *
 * with
 *
 * <programlisting>gcc -o polkit-mate-example `pkg-config --cflags --libs polkit-mate` polkit-mate-example.c</programlisting>
 *
 * Then, put the following content
 *
 * <programlisting><xi:include xmlns:xi="http://www.w3.org/2001/XInclude" href="../../examples/polkit-mate-example.policy" parse="text"><xi:fallback>FIXME: MISSING XINCLUDE CONTENT</xi:fallback></xi:include></programlisting>
 *
 * into a file
 * <literal>/usr/share/PolicyKit/policy/polkit-mate-example.policy</literal>. Finally,
 * run <literal>polkit-mate-example</literal>. It should display a
 * window like this:
 *
 * <inlinegraphic fileref="polkit-mate-example-screenshot.png" format="PNG"/>
 *
 * If the "Twiddle!" button is pressed, an authentication dialog
 * should pop up
 *
 * <inlinegraphic fileref="polkit-mate-example-auth-dialog-twiddle.png" format="PNG"/>
 *
 * Here is how what the application looks like if the user gains
 * authorization for all the actions:
 *
 * <inlinegraphic fileref="polkit-mate-example-screenshot-authorized.png" format="PNG"/>
 *
 * Regarding how to build an example mechanism that this GTK+
 * application can take advantage of, please refer to the
 * #PolKitContext class for examples.
 **/

#define POLKIT_MATE_ACTION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), POLKIT_MATE_TYPE_ACTION, PolKitMateActionPrivate))

static void polkit_mate_action_activate (PolKitMateAction *action);
static void _auth_start (PolKitMateAction *action);

static void polkit_mate_action_set_polkit_action         (PolKitMateAction *action, PolKitAction *pk_action);
static void polkit_mate_action_set_polkit_action_sufficient (PolKitMateAction *action, const GValue *pk_action_array);

static void polkit_mate_action_set_self_blocked_visible     (PolKitMateAction *action, gboolean visible);
static void polkit_mate_action_set_self_blocked_sensitive   (PolKitMateAction *action, gboolean sensitive);
static void polkit_mate_action_set_self_blocked_short_label (PolKitMateAction *action, const gchar *short_label);
static void polkit_mate_action_set_self_blocked_label       (PolKitMateAction *action, const gchar *label);
static void polkit_mate_action_set_self_blocked_tooltip     (PolKitMateAction *action, const gchar *tooltip);
static void polkit_mate_action_set_self_blocked_icon_name   (PolKitMateAction *action, const gchar *icon_name);

static void polkit_mate_action_set_no_visible               (PolKitMateAction *action, gboolean visible);
static void polkit_mate_action_set_no_sensitive             (PolKitMateAction *action, gboolean sensitive);
static void polkit_mate_action_set_no_short_label           (PolKitMateAction *action, const gchar *short_label);
static void polkit_mate_action_set_no_label                 (PolKitMateAction *action, const gchar *label);
static void polkit_mate_action_set_no_tooltip               (PolKitMateAction *action, const gchar *tooltip);
static void polkit_mate_action_set_no_icon_name             (PolKitMateAction *action, const gchar *icon_name);

static void polkit_mate_action_set_auth_visible             (PolKitMateAction *action, gboolean visible);
static void polkit_mate_action_set_auth_sensitive           (PolKitMateAction *action, gboolean sensitive);
static void polkit_mate_action_set_auth_short_label         (PolKitMateAction *action, const gchar *short_label);
static void polkit_mate_action_set_auth_label               (PolKitMateAction *action, const gchar *label);
static void polkit_mate_action_set_auth_tooltip             (PolKitMateAction *action, const gchar *tooltip);
static void polkit_mate_action_set_auth_icon_name           (PolKitMateAction *action, const gchar *icon_name);

static void polkit_mate_action_set_yes_visible              (PolKitMateAction *action, gboolean visible);
static void polkit_mate_action_set_yes_sensitive            (PolKitMateAction *action, gboolean sensitive);
static void polkit_mate_action_set_yes_short_label          (PolKitMateAction *action, const gchar *short_label);
static void polkit_mate_action_set_yes_label                (PolKitMateAction *action, const gchar *label);
static void polkit_mate_action_set_yes_tooltip              (PolKitMateAction *action, const gchar *tooltip);
static void polkit_mate_action_set_yes_icon_name            (PolKitMateAction *action, const gchar *icon_name);

static void polkit_mate_action_set_target_pid               (PolKitMateAction *action, guint target_pid);

static void _pk_config_changed (PolKitMateContext *pk_g_context, PolKitMateAction  *action);
static void _pk_console_kit_db_changed (PolKitMateContext *pk_g_context, PolKitMateAction  *action);

struct _PolKitMateActionPrivate 
{
        gboolean self_blocked_visible;
        gboolean self_blocked_sensitive;
        gchar *self_blocked_short_label;
        gchar *self_blocked_label;
        gchar *self_blocked_tooltip;
        gchar *self_blocked_icon_name;

        gboolean no_visible;
        gboolean no_sensitive;
        gchar *no_short_label;
        gchar *no_label;
        gchar *no_tooltip;
        gchar *no_icon_name;

        gboolean auth_visible;
        gboolean auth_sensitive;
        gchar *auth_short_label;
        gchar *auth_label;
        gchar *auth_tooltip;
        gchar *auth_icon_name;

        gboolean yes_visible;
        gboolean yes_sensitive;
        gchar *yes_short_label;
        gchar *yes_label;
        gchar *yes_tooltip;
        gchar *yes_icon_name;

        gboolean master_visible;
        gboolean master_sensitive;

        PolKitAction *polkit_action;
        GSList *polkit_action_sufficient;

        gboolean polkit_action_set_once;

        guint target_pid;

        /* the current PolicyKit result for the given polkit_action_id */
        PolKitResult pk_result;

        PolKitMateContext *pk_g_context;

        gulong config_changed_handler_id;
        gulong console_kit_db_changed_handler_id;
};

enum 
{
        AUTH_START_SIGNAL,
        AUTH_END_SIGNAL,
        POLKIT_RESULT_CHANGED_SIGNAL,
        LAST_SIGNAL
};

enum
{
        PROP_0,
        PROP_POLKIT_ACTION_OBJ,
        PROP_POLKIT_ACTION_OBJ_SUFFICIENT,

        PROP_POLKIT_SELF_BLOCKED_VISIBLE,
        PROP_POLKIT_SELF_BLOCKED_SENSITIVE,
        PROP_POLKIT_SELF_BLOCKED_SHORT_LABEL,
        PROP_POLKIT_SELF_BLOCKED_LABEL,
        PROP_POLKIT_SELF_BLOCKED_TOOLTIP,
        PROP_POLKIT_SELF_BLOCKED_ICON_NAME,

        PROP_POLKIT_NO_VISIBLE,
        PROP_POLKIT_NO_SENSITIVE,
        PROP_POLKIT_NO_SHORT_LABEL,
        PROP_POLKIT_NO_LABEL,
        PROP_POLKIT_NO_TOOLTIP,
        PROP_POLKIT_NO_ICON_NAME,

        PROP_POLKIT_AUTH_VISIBLE,
        PROP_POLKIT_AUTH_SENSITIVE,
        PROP_POLKIT_AUTH_SHORT_LABEL,
        PROP_POLKIT_AUTH_LABEL,
        PROP_POLKIT_AUTH_TOOLTIP,
        PROP_POLKIT_AUTH_ICON_NAME,

        PROP_POLKIT_YES_VISIBLE,
        PROP_POLKIT_YES_SENSITIVE,
        PROP_POLKIT_YES_SHORT_LABEL,
        PROP_POLKIT_YES_LABEL,
        PROP_POLKIT_YES_TOOLTIP,
        PROP_POLKIT_YES_ICON_NAME,

        PROP_POLKIT_MASTER_VISIBLE,
        PROP_POLKIT_MASTER_SENSITIVE,

        PROP_POLKIT_TARGET_PID,
};

G_DEFINE_TYPE (PolKitMateAction, polkit_mate_action, GTK_TYPE_ACTION)

static void set_property                   (GObject         *object,
					    guint            prop_id,
					    const GValue    *value,
					    GParamSpec      *pspec);
static void get_property                   (GObject         *object,
					    guint            prop_id,
					    GValue          *value,
					    GParamSpec      *pspec);

static GObjectClass *parent_class = NULL;
static guint         signals[LAST_SIGNAL] = { 0 };

static void
polkit_mate_action_init (PolKitMateAction *action)
{
        action->priv = POLKIT_MATE_ACTION_GET_PRIVATE (action);


}


static void
free_pk_action_sufficient (PolKitMateAction *action)
{
        if (action->priv->polkit_action_sufficient != NULL) {
                GSList *l;

                for (l = action->priv->polkit_action_sufficient; l != NULL; l = g_slist_next (l)) {
                        polkit_action_unref ((PolKitAction *) l->data);
                }
                g_slist_free (action->priv->polkit_action_sufficient);
                action->priv->polkit_action_sufficient = NULL;
        }
}

static void
polkit_mate_action_finalize (GObject *object)
{
        PolKitMateAction *action;

        action = POLKIT_MATE_ACTION (object);

        if (action->priv->polkit_action != NULL)
                polkit_action_unref (action->priv->polkit_action);

        g_free (action->priv->self_blocked_short_label);
        g_free (action->priv->self_blocked_label);
        g_free (action->priv->self_blocked_tooltip);
        g_free (action->priv->self_blocked_icon_name);

        g_free (action->priv->no_short_label);
        g_free (action->priv->no_label);
        g_free (action->priv->no_tooltip);
        g_free (action->priv->no_icon_name);

        g_free (action->priv->auth_short_label);
        g_free (action->priv->auth_label);
        g_free (action->priv->auth_tooltip);
        g_free (action->priv->auth_icon_name);

        g_free (action->priv->yes_short_label);
        g_free (action->priv->yes_label);
        g_free (action->priv->yes_tooltip);
        g_free (action->priv->yes_icon_name);

        free_pk_action_sufficient (action);

        if (action->priv->pk_g_context != NULL) {
                g_signal_handler_disconnect (action->priv->pk_g_context, action->priv->config_changed_handler_id);
                g_signal_handler_disconnect (action->priv->pk_g_context, action->priv->console_kit_db_changed_handler_id);
                g_object_unref (action->priv->pk_g_context);
        }

        G_OBJECT_CLASS (polkit_mate_action_parent_class)->finalize (object);
}

static void
_ensure_pk_g_context (PolKitMateAction *action)
{
        if (action->priv->pk_g_context == NULL) {
                action->priv->pk_g_context = polkit_mate_context_get (NULL);
                
                action->priv->config_changed_handler_id = g_signal_connect (
                        action->priv->pk_g_context,
                        "config-changed",
                        G_CALLBACK (_pk_config_changed),
                        action);
                
                action->priv->console_kit_db_changed_handler_id = g_signal_connect (
                        action->priv->pk_g_context,
                        "console-kit-db-changed",
                        G_CALLBACK (_pk_console_kit_db_changed),
                        action);
        }
}

static GObject *
polkit_mate_action_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        PolKitMateAction      *action;
        PolKitMateActionClass *klass;

        klass = POLKIT_MATE_ACTION_CLASS (g_type_class_peek (POLKIT_MATE_TYPE_ACTION));

        action = POLKIT_MATE_ACTION (G_OBJECT_CLASS (parent_class)->constructor (type,
                                                                                   n_construct_properties,
                                                                                   construct_properties));

        action->priv->master_visible = TRUE;
        action->priv->master_sensitive = TRUE;

        return G_OBJECT (action);
}


static void
polkit_mate_action_class_init (PolKitMateActionClass *klass)
{
        GObjectClass *gobject_class;
        GtkActionClass *action_class;

        parent_class = g_type_class_peek_parent (klass);
        gobject_class = G_OBJECT_CLASS (klass);
        action_class = GTK_ACTION_CLASS (klass);

        gobject_class->constructor = polkit_mate_action_constructor;
        gobject_class->set_property = set_property;
        gobject_class->get_property = get_property;
        gobject_class->finalize = polkit_mate_action_finalize;

        action_class->activate = (void (*)(GtkAction*)) polkit_mate_action_activate;

        klass->auth_start = _auth_start;

        g_object_class_install_property (gobject_class,
                                         PROP_POLKIT_ACTION_OBJ,
                                         g_param_spec_pointer ("polkit-action",
                                                               "The PolKitAction object this GTK+ action is tracking",
                                                               "The PolKitAction object this GTK+ action is tracking",
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_POLKIT_ACTION_OBJ_SUFFICIENT,
                                         g_param_spec_value_array (
                                                 "polkit-action-sufficient",
                                                 "An array of PolKitAction objects that are sufficient to have authorizations for.",
                                                 "An array of PolKitAction objects that are sufficient to have authorizations for.",
                                                 g_param_spec_pointer (
                                                         "polkit-action-sufficient-member",
                                                         "PolKitAction member of polkit-action-sufficient",
                                                         "PolKitAction member of polkit-action-sufficient",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT),
                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        /*------------------------------*/

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_SELF_BLOCKED_VISIBLE,
                g_param_spec_boolean (
                        "self-blocked-visible",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, whether the action will be visible",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, whether the action will be visible",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_SELF_BLOCKED_SENSITIVE,
                g_param_spec_boolean (
                        "self-blocked-sensitive",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, whether the action will be sensitive",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, whether the action will be sensitive",
                        FALSE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_SELF_BLOCKED_SHORT_LABEL,
                g_param_spec_string (
                        "self-blocked-short-label",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, use this short-label",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, use this short-label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_SELF_BLOCKED_LABEL,
                g_param_spec_string (
                        "self-blocked-label",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, use this label",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, use this label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_SELF_BLOCKED_TOOLTIP,
                g_param_spec_string (
                        "self-blocked-tooltip",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, use this tooltip",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, use this tooltip",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_SELF_BLOCKED_ICON_NAME,
                g_param_spec_string (
                        "self-blocked-icon-name",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, use this icon-name",
                        "If PolicyKit evaluates the result as 'no' and the reason is that the user has a self-granted negative authorization, use this icon-name",
                        NULL,
                        G_PARAM_READWRITE));

        /*------------------------------*/

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_NO_VISIBLE,
                g_param_spec_boolean (
                        "no-visible",
                        "If PolicyKit evaluates the result as 'no', whether the action will be visible",
                        "If PolicyKit evaluates the result as 'no', whether the action will be visible",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_NO_SENSITIVE,
                g_param_spec_boolean (
                        "no-sensitive",
                        "If PolicyKit evaluates the result as 'no', whether the action will be sensitive",
                        "If PolicyKit evaluates the result as 'no', whether the action will be sensitive",
                        FALSE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_NO_SHORT_LABEL,
                g_param_spec_string (
                        "no-short-label",
                        "If PolicyKit evaluates the result as 'no', use this short-label",
                        "If PolicyKit evaluates the result as 'no', use this short-label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_NO_LABEL,
                g_param_spec_string (
                        "no-label",
                        "If PolicyKit evaluates the result as 'no', use this label",
                        "If PolicyKit evaluates the result as 'no', use this label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_NO_TOOLTIP,
                g_param_spec_string (
                        "no-tooltip",
                        "If PolicyKit evaluates the result as 'no', use this tooltip",
                        "If PolicyKit evaluates the result as 'no', use this tooltip",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_NO_ICON_NAME,
                g_param_spec_string (
                        "no-icon-name",
                        "If PolicyKit evaluates the result as 'no', use this icon-name",
                        "If PolicyKit evaluates the result as 'no', use this icon-name",
                        NULL,
                        G_PARAM_READWRITE));

        /*------------------------------*/

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_VISIBLE,
                g_param_spec_boolean (
                        "auth-visible",
                        "If PolicyKit evaluates the result as 'auth', whether the action will be visible",
                        "If PolicyKit evaluates the result as 'auth', whether the action will be visible",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_SENSITIVE,
                g_param_spec_boolean (
                        "auth-sensitive",
                        "If PolicyKit evaluates the result as 'auth', whether the action will be sensitive",
                        "If PolicyKit evaluates the result as 'auth', whether the action will be sensitive",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_SHORT_LABEL,
                g_param_spec_string (
                        "auth-short-label",
                        "If PolicyKit evaluates the result as 'auth', use this short-label",
                        "If PolicyKit evaluates the result as 'auth', use this short-label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_LABEL,
                g_param_spec_string (
                        "auth-label",
                        "If PolicyKit evaluates the result as 'auth', use this label",
                        "If PolicyKit evaluates the result as 'auth', use this label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_TOOLTIP,
                g_param_spec_string (
                        "auth-tooltip",
                        "If PolicyKit evaluates the result as 'auth', use this tooltip",
                        "If PolicyKit evaluates the result as 'auth', use this tooltip",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_ICON_NAME,
                g_param_spec_string (
                        "auth-icon-name",
                        "If PolicyKit evaluates the result as 'auth', use this icon-name",
                        "If PolicyKit evaluates the result as 'auth', use this icon-name",
                        NULL,
                        G_PARAM_READWRITE));

        /*------------------------------*/

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_YES_VISIBLE,
                g_param_spec_boolean (
                        "yes-visible",
                        "If PolicyKit evaluates the result as 'yes', whether the action will be visible",
                        "If PolicyKit evaluates the result as 'yes', whether the action will be visible",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_YES_SENSITIVE,
                g_param_spec_boolean (
                        "yes-sensitive",
                        "If PolicyKit evaluates the result as 'yes', whether the action will be sensitive",
                        "If PolicyKit evaluates the result as 'yes', whether the action will be sensitive",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_YES_SHORT_LABEL,
                g_param_spec_string (
                        "yes-short-label",
                        "If PolicyKit evaluates the result as 'yes', use this short-label",
                        "If PolicyKit evaluates the result as 'yes', use this short-label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_YES_LABEL,
                g_param_spec_string (
                        "yes-label",
                        "If PolicyKit evaluates the result as 'yes', use this label",
                        "If PolicyKit evaluates the result as 'yes', use this label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_YES_TOOLTIP,
                g_param_spec_string (
                        "yes-tooltip",
                        "If PolicyKit evaluates the result as 'yes', use this tooltip",
                        "If PolicyKit evaluates the result as 'yes', use this tooltip",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_YES_ICON_NAME,
                g_param_spec_string (
                        "yes-icon-name",
                        "If PolicyKit evaluates the result as 'yes', use this icon-name",
                        "If PolicyKit evaluates the result as 'yes', use this icon-name",
                        NULL,
                        G_PARAM_READWRITE));

        /*------------------------------*/

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_MASTER_VISIBLE,
                g_param_spec_boolean (
                        "master-visible",
                        "Can be set to FALSE to force invisibility no matter what PolicyKit reports",
                        "Can be set to FALSE to force invisibility no matter what PolicyKit reports",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_MASTER_SENSITIVE,
                g_param_spec_boolean (
                        "master-sensitive",
                        "Can be set to FALSE to force insensitivity no matter what PolicyKit reports",
                        "Can be set to FALSE to force insensitivity no matter what PolicyKit reports",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_TARGET_PID,
                g_param_spec_uint (
                        "target-pid",
                        "The target process id to receive the authorization; if 0 it is the current process",
                        "The target process id to receive the authorization; if 0 it is the current process",
                        0,
                        G_MAXUINT,
                        0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        /**
         * PolKitMateAction::auth-start:
         * @action: the object
         *
         * The ::auth-start signal is emitted when an authentication
         * session starts.
         **/
        signals [AUTH_START_SIGNAL] =
                g_signal_new ("auth-start",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PolKitMateActionClass, auth_start),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        /**
         * PolKitMateAction::auth-end:
         * @action: the object
         * @gained_privilege: whether the privilege was gained
         *
         * The ::auth-end signal is emitted when the an authentication
         * session ends and carries information about whether the
         * privilege was obtained or not.
         **/
        signals [AUTH_END_SIGNAL] =
                g_signal_new ("auth-end",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PolKitMateActionClass, auth_end),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__BOOLEAN,
                              G_TYPE_NONE,
                              1, G_TYPE_BOOLEAN);

        /**
         * PolKitMateAction::polkit-result-changed:
         * @action: the object
         * @current_result: current #PolKitResult from PolicyKit regarding given #PolKitAction object
         *
         * The ::polkit-result-changed signal is emitted when the
         * PolicyKit result changes. This can happen when external
         * factors (config file, ConsoleKit, privilege granted /
         * revoked) change since the #PolKitMateAction class listens
         * for events using the #PolKitMateContext class.
         **/
        signals [POLKIT_RESULT_CHANGED_SIGNAL] =
                g_signal_new ("polkit-result-changed",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PolKitMateActionClass, polkit_result_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1, G_TYPE_INT);


        g_type_class_add_private (gobject_class, sizeof (PolKitMateActionPrivate));
}

static pid_t
_get_target_pid (PolKitMateAction *action)
{
        pid_t pid;

        if (action != NULL && action->priv->target_pid != 0)
                pid = action->priv->target_pid;
        else
                pid = getpid ();

        return pid;
}

static PolKitResult
_compute_polkit_result_direct  (PolKitMateAction *action)
{
        PolKitCaller *pk_caller;
        PolKitResult pk_result;
        DBusError dbus_error;

        _ensure_pk_g_context (action);

        dbus_error_init (&dbus_error);        
        pk_caller = polkit_tracker_get_caller_from_pid (action->priv->pk_g_context->pk_tracker, 
                                                        _get_target_pid (action),
                                                        &dbus_error);
        if (pk_caller == NULL) {
                g_warning ("Cannot get PolKitCaller object for target (pid=%d): %s: %s",
                           _get_target_pid (action), dbus_error.name, dbus_error.message);
                dbus_error_free (&dbus_error);

                /* this is bad so cop-out to UKNOWN */
                pk_result = POLKIT_RESULT_UNKNOWN;
        } else {
                pk_result = polkit_context_is_caller_authorized (action->priv->pk_g_context->pk_context, 
                                                                 action->priv->polkit_action, 
                                                                 pk_caller,
                                                                 FALSE,
                                                                 NULL);
                if (pk_result != POLKIT_RESULT_YES) {
                        GSList *i;

                        /* no dice.. see if one if the sufficient actions, if any, yields a YES */
                        for (i = action->priv->polkit_action_sufficient; i != NULL; i = g_slist_next (i)) {
                                PolKitResult r;
                                PolKitAction *a = (PolKitAction *) i->data;

                                r = polkit_context_is_caller_authorized (action->priv->pk_g_context->pk_context, 
                                                                         a, 
                                                                         pk_caller,
                                                                         FALSE,
                                                                         NULL);

                                if (r == POLKIT_RESULT_YES) {
                                        pk_result = r;
                                        break;
                                }
                        }
                }
        }

        if (pk_caller != NULL)
                polkit_caller_unref (pk_caller);

        return pk_result;
}

/* returns TRUE if the result changed */
static gboolean
_compute_polkit_result (PolKitMateAction *action)
{
        PolKitResult old_result;

        old_result = action->priv->pk_result;
        action->priv->pk_result = POLKIT_RESULT_UNKNOWN;

        if (action->priv->polkit_action == NULL) {
                action->priv->pk_result = POLKIT_RESULT_YES;
        } else {
                action->priv->pk_result = _compute_polkit_result_direct (action);
        }

        return old_result != action->priv->pk_result;
}

static void
_update_action (PolKitMateAction *action)
{
        PolKitMateContext *pkgc;
        PolKitAuthorizationDB *authdb;
                                
        pkgc = polkit_mate_context_get (NULL);
        authdb = polkit_context_get_authorization_db (pkgc->pk_context);

        switch (action->priv->pk_result) {
        default:
        case POLKIT_RESULT_UNKNOWN:
        case POLKIT_RESULT_NO:
                /* TODO: see if we're self-blocked */

                if (action->priv->polkit_action != NULL && 
                    polkit_authorization_db_is_uid_blocked_by_self (authdb, 
                                                                    action->priv->polkit_action,
                                                                    getuid (),
                                                                    NULL)) {
                        g_object_set (action, 
                                      "visible", action->priv->self_blocked_visible && action->priv->master_visible,
                                      "sensitive", action->priv->self_blocked_sensitive && action->priv->master_sensitive,
                                      "short-label", action->priv->self_blocked_short_label,
                                      "label", action->priv->self_blocked_label,
                                      "tooltip", action->priv->self_blocked_tooltip,
                                      "icon-name", action->priv->self_blocked_icon_name, 
                                      NULL);
                } else {
                        g_object_set (action, 
                                      "visible", action->priv->no_visible && action->priv->master_visible,
                                      "sensitive", action->priv->no_sensitive && action->priv->master_sensitive,
                                      "short-label", action->priv->no_short_label,
                                      "label", action->priv->no_label,
                                      "tooltip", action->priv->no_tooltip,
                                      "icon-name", action->priv->no_icon_name, 
                                      NULL);
                }
                break;

        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS:
                g_object_set (action, 
                              "visible", action->priv->auth_visible && action->priv->master_visible,
                              "sensitive", action->priv->auth_sensitive && action->priv->master_sensitive,
                              "short-label", action->priv->auth_short_label,
                              "label", action->priv->auth_label,
                              "tooltip", action->priv->auth_tooltip,
                              "icon-name", action->priv->auth_icon_name, 
                              NULL);
                break;

        case POLKIT_RESULT_YES:
                g_object_set (action, 
                              "visible", action->priv->yes_visible && action->priv->master_visible,
                              "sensitive", action->priv->yes_sensitive && action->priv->master_sensitive,
                              "short-label", action->priv->yes_short_label,
                              "label", action->priv->yes_label,
                              "tooltip", action->priv->yes_tooltip,
                              "icon-name", action->priv->yes_icon_name,
                              NULL);
                break;
        }
}

static void
_pk_config_changed (PolKitMateContext *pk_g_context, PolKitMateAction  *action)
{
        gboolean result_changed;
        result_changed = _compute_polkit_result (action);
        _update_action (action);
        if (result_changed) {
                g_signal_emit (action, signals [POLKIT_RESULT_CHANGED_SIGNAL], 0, action->priv->pk_result);
        }
}

static void
_pk_console_kit_db_changed (PolKitMateContext *pk_g_context, PolKitMateAction  *action)
{
        /* for now, just use the same code as above... */
        _pk_config_changed (pk_g_context, action);
}


/**
 * polkit_mate_action_new:
 * @name: A unique name for the action
 *
 * Creates a new #PolKitMateAction object. The typical use for this
 * function is for specialized use where
 * polkit_mate_action_new_default() does not meet the needs of the
 * application. A short example of the usage of this contructor
 * follows.
 *
 * If the 'polkit-action' property is #NULL the behavior is similar to
 * as if a #PolKitAction returned #POLKIT_RESULT_YES.
 *
 * One can set the 'polkit-action-sufficient' property to a
 * #GValueArray of pointers to #PolKitAction objects. The semantics of
 * this property is that if the user is authorized for any of the
 * given actions in 'polkit-action-sufficient' then the behavior is
 * the same as if they're authorized for the action denoted by
 * 'polkit-action'. This is useful in a setup where you have two
 * similar actions and one implies the other. For example, in
 * mate-system-monitor there are two actions
 * <literal>org.mate.system-monitor.increase-own-priority</literal>
 * (Increase the priority of a process owned by yourself) and
 * <literal>org.mate.system-monitor.change-priorty</literal> (Change
 * priority of any process). As the latter clearly implies the former,
 * one would set the latter in the 'polkit-action-sufficient'
 * property when constructing a #PolKitAction for the former.
 *
 * <programlisting>
 * PolKitAction *polkit_action;
 * PolKitMateAction *action;
 *
 * polkit_action = polkit_action_new ();
 * polkit_action_set_action_id (polkit_action, "org.example.some-policykit-action");
 *
 * action = polkit_mate_action_new ("blabla", NULL);
 * g_object_set (action,
 *               "polkit-action",    polkit_action,
 *               "no-visible",       TRUE,
 *               "no-sensitive",     FALSE,
 *               "no-short-label",   "The Action no can do!",
 *               "no-label",         "The Action is not permitted!",
 *               "no-tooltip",       "The Tooltip (no)",
 *               "no-icon-name",     GTK_STOCK_NO,
 *
 *               "auth-visible",     TRUE,
 *               "auth-sensitive",   TRUE,
 *               "auth-short-label", "The Action requires auth...",
 *               "auth-label",       "The Action requires auth...",
 *               "auth-tooltip",     "The Tooltip (auth)",
 *               "auth-icon-name",   GTK_STOCK_DIALOG_AUTHENTICATION,
 *
 *               "yes-visible",      TRUE,
 *               "yes-sensitive",    TRUE,
 *               "yes-short-label",  "Action!",
 *               "yes-label",        "Just do the Action!",
 *               "yes-tooltip",      "The Tooltip (yes)",
 *               "yes-icon-name",    GTK_STOCK_YES,
 *               NULL);
 * </programlisting>
 *
 * Returns: a new #PolKitMateAction or #NULL if error is set
 */
PolKitMateAction *
polkit_mate_action_new (const gchar *name)
{
        PolKitMateAction *action = NULL;

        action = g_object_new (POLKIT_MATE_TYPE_ACTION,
                               "name", name,
                               NULL);

        return action;
}

/**
 * polkit_mate_action_new_default:
 * @name: A unique name for the action
 * @polkit_action: the #PolKitAction to track
 * @label: the label to use (will also apply to short-label)
 * @tooltip: the tool tip to use
 *
 * Creates a new #PolKitMateAction object with the default
 * behavior for a given #PolKitAction object.
 *
 * Default behavior is defined by the label and tooltip being
 * identical across all three four states, the action being visible in
 * all four states, and the action being insensitive only in the state
 * where the result from PolicyKit is no and self-blocked. Only when
 * PolicyKit returns one of the 'auth*' results, the icon_name
 * property will be set to #GTK_STOCK_DIALOG_AUTHENTICATION.
 *
 * The caller can always modify individual aspects of the action after
 * creation, e.g. change the tooltip for the self-blocked, no, auth
 * and yes states.
 *
 * If the given polkit_action is #NULL the behavior is similar to as
 * if a #PolKitAction returned #POLKIT_RESULT_YES.
 *
 * Returns: a new #PolKitMateAction or #NULL if error is set
 */
PolKitMateAction *
polkit_mate_action_new_default (const gchar  *name, 
                                 PolKitAction *polkit_action, 
                                 const gchar  *label, 
                                 const gchar  *tooltip)
{
        PolKitMateAction *action;

        action = g_object_new (POLKIT_MATE_TYPE_ACTION,
                               "name", name,
                               "polkit-action", polkit_action,

                               "self-blocked-visible",       TRUE,
                               "self-blocked-sensitive",     FALSE,
                               "self-blocked-short-label",   label,
                               "self-blocked-label",         label,
                               "self-blocked-tooltip",       tooltip,
                               "self-blocked-icon-name",     NULL,

                               "no-visible",       TRUE,
                               "no-sensitive",     FALSE,
                               "no-short-label",   label,
                               "no-label",         label,
                               "no-tooltip",       tooltip,
                               "no-icon-name",     NULL,
                               
                               "auth-visible",     TRUE,
                               "auth-sensitive",   TRUE,
                               "auth-short-label", label,
                               "auth-label",       label,
                               "auth-tooltip",     tooltip,
                               "auth-icon-name",   GTK_STOCK_DIALOG_AUTHENTICATION,
                               
                               "yes-visible",      TRUE,
                               "yes-sensitive",    TRUE,
                               "yes-short-label",  label,
                               "yes-label",        label,
                               "yes-tooltip",      tooltip,
                               "yes-icon-name",    NULL,
                               
                               "master-visible",   TRUE,
                               "master-sensitive", TRUE,
                               NULL);

        return action;
}

/*----------------------------------------------------------------------------------------------------*/

/**
 * polkit_mate_action_set_polkit_action:
 * @action: The #PolKitMateAction object
 * @pk_action: The #PolKitAction object
 *
 * Sets the #PolKitAction object to track for updating the GTK+ action.
 */
static void
polkit_mate_action_set_polkit_action (PolKitMateAction *action, PolKitAction *pk_action)
{

        /* Don't bother updating polkit_action if it's the same
         * value.. it will just cause a lot of unnecessary work as
         * we'll recompute the answer via PolicyKit.. 
         *
         * unless it's on the initial call (where priv->polkit_action
         * is alread NULL) because we need that initial update;
         */
        if (!action->priv->polkit_action_set_once || action->priv->polkit_action != pk_action) {

                action->priv->polkit_action_set_once = TRUE;

                if (action->priv->polkit_action != NULL)
                        polkit_action_unref (action->priv->polkit_action);

                action->priv->polkit_action = pk_action != NULL ? polkit_action_ref (pk_action) : NULL;
                
                _compute_polkit_result (action);
                _update_action (action);
        }
}

static void 
polkit_mate_action_set_polkit_action_sufficient (PolKitMateAction *action, const GValue *pk_action_array)
{
        unsigned int n;
        GValueArray *value_array;

        free_pk_action_sufficient (action);

        if (pk_action_array == NULL)
                goto out;

	value_array = g_value_get_boxed (pk_action_array);
        if (value_array == NULL)
                goto out;

	for (n = 0; n < value_array->n_values; n++) {
                PolKitAction *pk_action;

                pk_action = (PolKitAction *) g_value_get_pointer (& (value_array->values[n]));
		action->priv->polkit_action_sufficient = g_slist_prepend (action->priv->polkit_action_sufficient, 
                                                                          polkit_action_ref (pk_action));
                char *s;
                polkit_action_get_action_id (pk_action, &s);
                g_warning ("Setting sufficient %d: %s", n, s);
	}

out:
        _compute_polkit_result (action);
        _update_action (action);
}


/**
 * polkit_mate_action_get_polkit_result:
 * @action: The #PolKitMateAction object
 *
 * Gets the #PolKitResult that indicates whether the user is
 * privileged to do the #PolKitAction associated with this
 * #PolKitMateAction object.
 *
 * Returns: The #PolKitAction object. The caller shall not unref this object.
 */
PolKitResult
polkit_mate_action_get_polkit_result (PolKitMateAction *action)
{
        _compute_polkit_result (action);
        _update_action (action);
        return action->priv->pk_result;
}

/**
 * polkit_mate_action_get_sensitive:
 * @action: The #PolKitMateAction object
 *
 * Get the master sensitivity, see PolKitMateAction:master-sensitive:
 * for details.
 * 
 * Returns: the master sensitivity
 */
gboolean
polkit_mate_action_get_sensitive (PolKitMateAction *action)
{
        return action->priv->master_sensitive;
}

/**
 * polkit_mate_action_set_sensitive:
 * @action: The #PolKitMateAction object
 * @sensitive: master sensitivity
 *
 * Set the master sensitivity, see PolKitMateAction:master-sensitive:
 * for details.
 */
void
polkit_mate_action_set_sensitive (PolKitMateAction *action, gboolean sensitive)
{
        if (action->priv->master_sensitive == sensitive)
                return;
        action->priv->master_sensitive = sensitive;
        _update_action (action);
}

static void 
polkit_mate_action_set_target_pid (PolKitMateAction *action, guint target_pid)
{
        action->priv->target_pid = target_pid;
        _compute_polkit_result (action);
        _update_action (action);
}

/**
 * polkit_mate_action_get_visible:
 * @action: The #PolKitMateAction object
 *
 * Get the master visibility, see PolKitMateAction:master-visible:
 * for details.
 * 
 * Returns: the master visibility
 */
gboolean
polkit_mate_action_get_visible (PolKitMateAction *action)
{
        return action->priv->master_visible;
}

/**
 * polkit_mate_action_set_visible:
 * @action: The #PolKitMateAction object
 * @visible: master visibility
 *
 * Set the master visibility, see PolKitMateAction:master-visible:
 * for details.
 */
void
polkit_mate_action_set_visible (PolKitMateAction *action, gboolean visible)
{
        if (action->priv->master_visible == visible)
                return;
        action->priv->master_visible = visible;
        _update_action (action);
}

/*----------------------------------------------------------------------------------------------------*/

/**
 * polkit_mate_action_set_self_blocked_visible:
 * @action: The #PolKitMateAction object
 * @visible: new value
 *
 * Sets the value of visible to use when PolicyKit returns the answer
 * no (and the reason is that the user has a self-granted negative
 * authorization) for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_self_blocked_visible (PolKitMateAction *action, gboolean visible)
{
        action->priv->self_blocked_visible = visible;
        _update_action (action);
}

/**
 * polkit_mate_action_set_self_blocked_sensitive:
 * @action: The #PolKitMateAction object
 * @sensitive: new value
 *
 * Sets the value of sensitive to use when PolicyKit returns the
 * answer no (and the reason is that the user has a self-granted
 * negative authorization) for the current #PolKitAction being
 * tracked.
 */
static void
polkit_mate_action_set_self_blocked_sensitive (PolKitMateAction *action, gboolean sensitive)
{
        action->priv->self_blocked_sensitive = sensitive;
        _update_action (action);
}

/**
 * polkit_mate_action_set_self_blocked_short_label:
 * @action: The #PolKitMateAction object
 * @short_label: new value
 *
 * Sets the value of short-label to use when PolicyKit returns the
 * answer no (and the reason is that the user has a self-granted
 * negative authorization) for the current #PolKitAction being
 * tracked.
 */
static void
polkit_mate_action_set_self_blocked_short_label (PolKitMateAction *action, const gchar *short_label)
{
        g_free (action->priv->self_blocked_short_label);
        action->priv->self_blocked_short_label = g_strdup (short_label);
        _update_action (action);
}

/**
 * polkit_mate_action_set_self_blocked_label:
 * @action: The #PolKitMateAction object
 * @label: new value
 *
 * Sets the value of label to use when PolicyKit returns the answer
 * no (and the reason is that the user has a self-granted
 * negative authorization) for the current #PolKitAction being
 * tracked.
 */
static void
polkit_mate_action_set_self_blocked_label (PolKitMateAction *action, const gchar *label)
{
        g_free (action->priv->self_blocked_label);
        action->priv->self_blocked_label = g_strdup (label);
        _update_action (action);
}

/**
 * polkit_mate_action_set_self_blocked_tooltip:
 * @action: The #PolKitMateAction object
 * @tooltip: new value
 *
 * Sets the value of tooltip to use when PolicyKit returns the answer
 * no (and the reason is that the user has a self-granted negative
 * authorization) for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_self_blocked_tooltip (PolKitMateAction *action, const gchar *tooltip)
{
        g_free (action->priv->self_blocked_tooltip);
        action->priv->self_blocked_tooltip = g_strdup (tooltip);
        _update_action (action);
}

/**
 * polkit_mate_action_set_self_blocked_icon_name:
 * @action: The #PolKitMateAction object
 * @icon_name: new value
 *
 * Sets the value of icon_name to use when PolicyKit returns the
 * answer no (and the reason is that the user has a self-granted
 * negative authorization) for the current #PolKitAction being
 * tracked.
 */
static void
polkit_mate_action_set_self_blocked_icon_name (PolKitMateAction *action, const gchar *icon_name)
{
        g_free (action->priv->self_blocked_icon_name);
        action->priv->self_blocked_icon_name = g_strdup (icon_name);
        _update_action (action);
}

/*----------------------------------------------------------------------------------------------------*/

/**
 * polkit_mate_action_set_no_visible:
 * @action: The #PolKitMateAction object
 * @visible: new value
 *
 * Sets the value of visible to use when PolicyKit returns the answer
 * no for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_no_visible (PolKitMateAction *action, gboolean visible)
{
        action->priv->no_visible = visible;
        _update_action (action);
}

/**
 * polkit_mate_action_set_no_sensitive:
 * @action: The #PolKitMateAction object
 * @sensitive: new value
 *
 * Sets the value of sensitive to use when PolicyKit returns the answer
 * no for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_no_sensitive (PolKitMateAction *action, gboolean sensitive)
{
        action->priv->no_sensitive = sensitive;
        _update_action (action);
}

/**
 * polkit_mate_action_set_no_short_label:
 * @action: The #PolKitMateAction object
 * @short_label: new value
 *
 * Sets the value of short-label to use when PolicyKit returns the
 * answer no for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_no_short_label (PolKitMateAction *action, const gchar *short_label)
{
        g_free (action->priv->no_short_label);
        action->priv->no_short_label = g_strdup (short_label);
        _update_action (action);
}

/**
 * polkit_mate_action_set_no_label:
 * @action: The #PolKitMateAction object
 * @label: new value
 *
 * Sets the value of label to use when PolicyKit returns the answer no
 * for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_no_label (PolKitMateAction *action, const gchar *label)
{
        g_free (action->priv->no_label);
        action->priv->no_label = g_strdup (label);
        _update_action (action);
}

/**
 * polkit_mate_action_set_no_tooltip:
 * @action: The #PolKitMateAction object
 * @tooltip: new value
 *
 * Sets the value of tooltip to use when PolicyKit returns the answer
 * no for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_no_tooltip (PolKitMateAction *action, const gchar *tooltip)
{
        g_free (action->priv->no_tooltip);
        action->priv->no_tooltip = g_strdup (tooltip);
        _update_action (action);
}

/**
 * polkit_mate_action_set_no_icon_name:
 * @action: The #PolKitMateAction object
 * @icon_name: new value
 *
 * Sets the value of icon_name to use when PolicyKit returns the
 * answer no for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_no_icon_name (PolKitMateAction *action, const gchar *icon_name)
{
        g_free (action->priv->no_icon_name);
        action->priv->no_icon_name = g_strdup (icon_name);
        _update_action (action);
}

/*----------------------------------------------------------------------------------------------------*/

/**
 * polkit_mate_action_set_auth_visible:
 * @action: The #PolKitMateAction object
 * @visible: new value
 *
 * Sets the value of visible to use when PolicyKit returns the answer
 * auth* for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_auth_visible (PolKitMateAction *action, gboolean visible)
{
        action->priv->auth_visible = visible;
        _update_action (action);
}

/**
 * polkit_mate_action_set_auth_sensitive:
 * @action: The #PolKitMateAction object
 * @sensitive: new value
 *
 * Sets the value of sensitive to use when PolicyKit returns the answer
 * auth* for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_auth_sensitive (PolKitMateAction *action, gboolean sensitive)
{
        action->priv->auth_sensitive = sensitive;
        _update_action (action);
}

/**
 * polkit_mate_action_set_auth_short_label:
 * @action: The #PolKitMateAction object
 * @short_label: new value
 *
 * Sets the value of short-label to use when PolicyKit returns the
 * answer auth for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_auth_short_label (PolKitMateAction *action, const gchar *short_label)
{
        g_free (action->priv->auth_short_label);
        action->priv->auth_short_label = g_strdup (short_label);
        _update_action (action);
}

/**
 * polkit_mate_action_set_auth_label:
 * @action: The #PolKitMateAction object
 * @label: new value
 *
 * Sets the value of label to use when PolicyKit returns the answer auth*
 * for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_auth_label (PolKitMateAction *action, const gchar *label)
{
        g_free (action->priv->auth_label);
        action->priv->auth_label = g_strdup (label);
        _update_action (action);
}

/**
 * polkit_mate_action_set_auth_tooltip:
 * @action: The #PolKitMateAction object
 * @tooltip: new value
 *
 * Sets the value of tooltip to use when PolicyKit returns the answer
 * auth* for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_auth_tooltip (PolKitMateAction *action, const gchar *tooltip)
{
        g_free (action->priv->auth_tooltip);
        action->priv->auth_tooltip = g_strdup (tooltip);
        _update_action (action);
}

/**
 * polkit_mate_action_set_auth_icon_name:
 * @action: The #PolKitMateAction object
 * @icon_name: new value
 *
 * Sets the value of icon_name to use when PolicyKit returns the
 * answer auth* for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_auth_icon_name (PolKitMateAction *action, const gchar *icon_name)
{
        g_free (action->priv->auth_icon_name);
        action->priv->auth_icon_name = g_strdup (icon_name);
        _update_action (action);
}

/*----------------------------------------------------------------------------------------------------*/

/**
 * polkit_mate_action_set_yes_visible:
 * @action: The #PolKitMateAction object
 * @visible: new value
 *
 * Sets the value of visible to use when PolicyKit returns the answer
 * yes for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_yes_visible (PolKitMateAction *action, gboolean visible)
{
        action->priv->yes_visible = visible;
        _update_action (action);
}

/**
 * polkit_mate_action_set_yes_sensitive:
 * @action: The #PolKitMateAction object
 * @sensitive: new value
 *
 * Sets the value of sensitive to use when PolicyKit returns the answer
 * yes for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_yes_sensitive (PolKitMateAction *action, gboolean sensitive)
{
        action->priv->yes_sensitive = sensitive;
        _update_action (action);
}

/**
 * polkit_mate_action_set_yes_short_label:
 * @action: The #PolKitMateAction object
 * @short_label: new value
 *
 * Sets the value of short-label to use when PolicyKit returns the
 * answer yes for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_yes_short_label (PolKitMateAction *action, const gchar *short_label)
{
        g_free (action->priv->yes_short_label);
        action->priv->yes_short_label = g_strdup (short_label);
        _update_action (action);
}

/**
 * polkit_mate_action_set_yes_label:
 * @action: The #PolKitMateAction object
 * @label: new value
 *
 * Sets the value of label to use when PolicyKit returns the answer yes
 * for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_yes_label (PolKitMateAction *action, const gchar *label)
{
        g_free (action->priv->yes_label);
        action->priv->yes_label = g_strdup (label);
        _update_action (action);
}

/**
 * polkit_mate_action_set_yes_tooltip:
 * @action: The #PolKitMateAction object
 * @tooltip: new value
 *
 * Sets the value of tooltip to use when PolicyKit returns the answer
 * yes for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_yes_tooltip (PolKitMateAction *action, const gchar *tooltip)
{
        g_free (action->priv->yes_tooltip);
        action->priv->yes_tooltip = g_strdup (tooltip);
        _update_action (action);
}

/**
 * polkit_mate_action_set_yes_icon_name:
 * @action: The #PolKitMateAction object
 * @icon_name: new value
 *
 * Sets the value of icon_name to use when PolicyKit returns the
 * answer yes for the current #PolKitAction being tracked.
 */
static void
polkit_mate_action_set_yes_icon_name (PolKitMateAction *action, const gchar *icon_name)
{
        g_free (action->priv->yes_icon_name);
        action->priv->yes_icon_name = g_strdup (icon_name);
        _update_action (action);
}

/*----------------------------------------------------------------------------------------------------*/

static void
get_property (GObject     *object,
	      guint        prop_id,
	      GValue      *value,
	      GParamSpec  *pspec)
{
        PolKitMateAction *action = POLKIT_MATE_ACTION (object);
        
        switch (prop_id)
        {
        case PROP_POLKIT_ACTION_OBJ:
                g_value_set_pointer (value, action->priv->polkit_action != NULL ? polkit_action_ref (action->priv->polkit_action) : NULL);
                break;

        case PROP_POLKIT_ACTION_OBJ_SUFFICIENT:
                //TODO: g_value_set_pointer (value, action->priv->polkit_action != NULL ? polkit_action_ref (action->priv->polkit_action) : NULL);
                break;

        case PROP_POLKIT_SELF_BLOCKED_VISIBLE:
                g_value_set_boolean (value, action->priv->self_blocked_visible);
                break;
        case PROP_POLKIT_SELF_BLOCKED_SENSITIVE:
                g_value_set_boolean (value, action->priv->self_blocked_sensitive);
                break;
        case PROP_POLKIT_SELF_BLOCKED_SHORT_LABEL:
                g_value_set_string (value, action->priv->self_blocked_short_label);
                break;
        case PROP_POLKIT_SELF_BLOCKED_LABEL:
                g_value_set_string (value, action->priv->self_blocked_label);
                break;
        case PROP_POLKIT_SELF_BLOCKED_TOOLTIP:
                g_value_set_string (value, action->priv->self_blocked_tooltip);
                break;
        case PROP_POLKIT_SELF_BLOCKED_ICON_NAME:
                g_value_set_string (value, action->priv->self_blocked_icon_name);
                break;

        case PROP_POLKIT_NO_VISIBLE:
                g_value_set_boolean (value, action->priv->no_visible);
                break;
        case PROP_POLKIT_NO_SENSITIVE:
                g_value_set_boolean (value, action->priv->no_sensitive);
                break;
        case PROP_POLKIT_NO_SHORT_LABEL:
                g_value_set_string (value, action->priv->no_short_label);
                break;
        case PROP_POLKIT_NO_LABEL:
                g_value_set_string (value, action->priv->no_label);
                break;
        case PROP_POLKIT_NO_TOOLTIP:
                g_value_set_string (value, action->priv->no_tooltip);
                break;
        case PROP_POLKIT_NO_ICON_NAME:
                g_value_set_string (value, action->priv->no_icon_name);
                break;

        case PROP_POLKIT_AUTH_VISIBLE:
                g_value_set_boolean (value, action->priv->auth_visible);
                break;
        case PROP_POLKIT_AUTH_SENSITIVE:
                g_value_set_boolean (value, action->priv->auth_sensitive);
                break;
        case PROP_POLKIT_AUTH_SHORT_LABEL:
                g_value_set_string (value, action->priv->auth_short_label);
                break;
        case PROP_POLKIT_AUTH_LABEL:
                g_value_set_string (value, action->priv->auth_label);
                break;
        case PROP_POLKIT_AUTH_TOOLTIP:
                g_value_set_string (value, action->priv->auth_tooltip);
                break;
        case PROP_POLKIT_AUTH_ICON_NAME:
                g_value_set_string (value, action->priv->auth_icon_name);
                break;

        case PROP_POLKIT_YES_VISIBLE:
                g_value_set_boolean (value, action->priv->yes_visible);
                break;
        case PROP_POLKIT_YES_SENSITIVE:
                g_value_set_boolean (value, action->priv->yes_sensitive);
                break;
        case PROP_POLKIT_YES_SHORT_LABEL:
                g_value_set_string (value, action->priv->yes_short_label);
                break;
        case PROP_POLKIT_YES_LABEL:
                g_value_set_string (value, action->priv->yes_label);
                break;
        case PROP_POLKIT_YES_TOOLTIP:
                g_value_set_string (value, action->priv->yes_tooltip);
                break;
        case PROP_POLKIT_YES_ICON_NAME:
                g_value_set_string (value, action->priv->yes_icon_name);
                break;

        case PROP_POLKIT_MASTER_VISIBLE:
                g_value_set_boolean (value, action->priv->master_visible);
                break;
        case PROP_POLKIT_MASTER_SENSITIVE:
                g_value_set_boolean (value, action->priv->master_sensitive);
                break;

        case PROP_POLKIT_TARGET_PID:
                g_value_set_uint (value, action->priv->target_pid);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
        PolKitMateAction *action = POLKIT_MATE_ACTION (object);
        
        switch (prop_id)
        {
        case PROP_POLKIT_ACTION_OBJ:
                polkit_mate_action_set_polkit_action (action, g_value_get_pointer (value));
                break;

        case PROP_POLKIT_ACTION_OBJ_SUFFICIENT:
                polkit_mate_action_set_polkit_action_sufficient (action, value);
                break;

        case PROP_POLKIT_SELF_BLOCKED_VISIBLE:
                polkit_mate_action_set_self_blocked_visible (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_SELF_BLOCKED_SENSITIVE:
                polkit_mate_action_set_self_blocked_sensitive (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_SELF_BLOCKED_SHORT_LABEL:
                polkit_mate_action_set_self_blocked_short_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_SELF_BLOCKED_LABEL:
                polkit_mate_action_set_self_blocked_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_SELF_BLOCKED_TOOLTIP:
                polkit_mate_action_set_self_blocked_tooltip (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_SELF_BLOCKED_ICON_NAME:
                polkit_mate_action_set_self_blocked_icon_name (action, g_value_get_string (value));
                break;

        case PROP_POLKIT_NO_VISIBLE:
                polkit_mate_action_set_no_visible (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_NO_SENSITIVE:
                polkit_mate_action_set_no_sensitive (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_NO_SHORT_LABEL:
                polkit_mate_action_set_no_short_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_NO_LABEL:
                polkit_mate_action_set_no_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_NO_TOOLTIP:
                polkit_mate_action_set_no_tooltip (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_NO_ICON_NAME:
                polkit_mate_action_set_no_icon_name (action, g_value_get_string (value));
                break;

        case PROP_POLKIT_AUTH_VISIBLE:
                polkit_mate_action_set_auth_visible (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_AUTH_SENSITIVE:
                polkit_mate_action_set_auth_sensitive (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_AUTH_SHORT_LABEL:
                polkit_mate_action_set_auth_short_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_AUTH_LABEL:
                polkit_mate_action_set_auth_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_AUTH_TOOLTIP:
                polkit_mate_action_set_auth_tooltip (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_AUTH_ICON_NAME:
                polkit_mate_action_set_auth_icon_name (action, g_value_get_string (value));
                break;

        case PROP_POLKIT_YES_VISIBLE:
                polkit_mate_action_set_yes_visible (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_YES_SENSITIVE:
                polkit_mate_action_set_yes_sensitive (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_YES_SHORT_LABEL:
                polkit_mate_action_set_yes_short_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_YES_LABEL:
                polkit_mate_action_set_yes_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_YES_TOOLTIP:
                polkit_mate_action_set_yes_tooltip (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_YES_ICON_NAME:
                polkit_mate_action_set_yes_icon_name (action, g_value_get_string (value));
                break;

        case PROP_POLKIT_MASTER_VISIBLE:
                polkit_mate_action_set_visible (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_MASTER_SENSITIVE:
                polkit_mate_action_set_sensitive (action, g_value_get_boolean (value));
                break;

        case PROP_POLKIT_TARGET_PID:
                polkit_mate_action_set_target_pid (action, g_value_get_uint (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static XID
_get_xid_from_proxy_widgets (PolKitMateAction *action)
{
        XID xid;
        GSList *i;
        GSList *proxies;

        /* unfortunately there's no way to get the proxy that caused
         * the ::activate signal.. so go through proxies and pick the
         * first one.. */

        /* TODO: this won't work for menus (mclasen) */

        xid = 0;

        proxies = gtk_action_get_proxies (GTK_ACTION (action));

        for (i = proxies; i != NULL; i = i->next) {
                GtkWidget *top_level;
                GtkWidget *proxy = i->data;

                top_level = gtk_widget_get_toplevel (proxy);

                if (top_level == NULL)
                        continue;

                if (! (GTK_WIDGET_TOPLEVEL (top_level) && GTK_IS_WINDOW (top_level)))
                        continue;

                if (top_level->window == NULL)
                        continue;

                xid = gdk_x11_drawable_get_xid (GDK_WINDOW (top_level->window));

                if (xid != 0)
                        break;
        }

        return xid;
}

static void
_show_dialog_cb (PolKitAction *pk_action, 
                 gboolean      gained_privilege, 
                 GError       *error, 
                 gpointer      user_data)
{
        PolKitMateAction *action = POLKIT_MATE_ACTION (user_data);

        if (gained_privilege) {
                /* better make sure our local pk_result is up-to-date.. */
                _compute_polkit_result (action);

                //g_debug ("end auth, obtained it");

                /* now emit the 'activate' signal again.. */
                gtk_action_activate (GTK_ACTION (action));

        } else {
                //g_debug ("end auth, didn't obtain it");

                if (error != NULL) {
                        g_warning ("Caught error: %s", error->message);
                        g_error_free (error);
                }
        }

        g_signal_emit (action, signals [AUTH_END_SIGNAL], 0, gained_privilege);
}

static void 
_auth_start (PolKitMateAction *action)
{
        GError *error = NULL;

        //g_debug ("starting auth");
        if (!polkit_mate_auth_obtain (action->priv->polkit_action, 
                                       (guint) _get_xid_from_proxy_widgets (action),
                                       (guint) _get_target_pid (action),
                                       _show_dialog_cb, 
                                       action, 
                                       &error)) {
                g_warning ("Caught error: %s", error->message);
                g_error_free (error);
        }
}

static void
polkit_mate_action_activate (PolKitMateAction *action)
{
        switch (action->priv->pk_result) {
        case POLKIT_RESULT_YES:
                /* If PolicyKit says yes.. then let 'activate' signal
                 * propagate
                 */
                break;

        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS:
                /* Otherwise, if the action needs auth..  stop the emission
                 * and start auth process.. 
                 */

                g_signal_stop_emission_by_name (action, "activate");

                if (action->priv->polkit_action != NULL) {
                        g_signal_emit (action, signals [AUTH_START_SIGNAL], 0);
                }
                break;

        default:
        case POLKIT_RESULT_NO:
                /* If PolicyKit says no... and we got here.. it means
                 * that the user set the property "no-sensitive" to
                 * TRUE.. Otherwise we couldn't be handling this signal.
                 *
                 * Hence, they probably have a good reason for doing
                 * this so do let the 'activate' signal propagate.. 
                 */
                break;
        }
}

static void
_update_tooltips (PolKitMateAction *action, GParamSpec *arg1, GtkWidget *widget)
{
        GtkTooltips *tips;
        GtkTooltipsData *ttd;
        gchar *tip_str;

        ttd = gtk_tooltips_data_get (widget);

        if (ttd == NULL) {
                tips = gtk_tooltips_new ();
        } else {
                tips = ttd->tooltips;
        }

        tip_str = NULL;
        g_object_get (action, "tooltip", &tip_str, NULL);

        /* TODO: if there is no tooltip the tip_str is NULL.
         * Unfortunately it seems that the tooltip isn't
         * cleared.. mmm.. gtk+ bug?
         */
        gtk_tooltips_set_tip (tips, widget, tip_str, tip_str);
        g_free (tip_str);
}

static void
_update_label (PolKitMateAction *action, GParamSpec *arg1, GtkWidget *widget)
{
        char *label;

        label = NULL;
        g_object_get (action, "label", &label, NULL);
        gtk_button_set_label (GTK_BUTTON (widget), label);
        g_free (label);
}

static void
_update_icon_name (PolKitMateAction *action, GParamSpec *arg1, GtkWidget *widget)
{
        gtk_button_set_image (GTK_BUTTON (widget), gtk_action_create_icon (GTK_ACTION (action), GTK_ICON_SIZE_BUTTON));
}

static void 
_button_clicked (GtkButton *button, PolKitMateAction *action)
{
        /* g_debug ("in _button_clicked"); */

        switch (action->priv->pk_result) {
        case POLKIT_RESULT_YES:
                break;

        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS:
                /* g_debug ("blocking clicked"); */
                g_signal_stop_emission_by_name (button, "clicked");
                break;

        default:
        case POLKIT_RESULT_NO:
                break;
        }
}

static void 
_button_auth_end (PolKitMateAction *action, gboolean gained_privilege, GtkWidget *button)
{
        /* g_debug ("in _button_auth_end gained_privilege=%d", gained_privilege); */
        if (gained_privilege) {
                /* g_debug ("emitting clicked"); */
                gtk_action_block_activate_from (GTK_ACTION (action), button);
                g_signal_emit_by_name (button, "clicked");
                gtk_action_unblock_activate_from (GTK_ACTION (action), button);
        }
}

/**
 * polkit_mate_action_create_button:
 * @action: The #PolKitMateAction object
 *
 * Create a button for the given action that displays the label,
 * tooltip and icon_name corresponding to whether the state, according
 * to PolicyKit, is no, auth or yes.
 * 
 * Returns: A #GtkButton instance connected to the action
 */
GtkWidget *
polkit_mate_action_create_button (PolKitMateAction *action)
{
        GtkWidget *button;

        button = gtk_button_new ();

        gtk_action_connect_proxy (GTK_ACTION (action), button);

        _update_label (action, NULL, button);
        _update_tooltips (action, NULL, button);
        _update_icon_name (action, NULL, button);

        g_signal_connect (action, "notify::tooltip", G_CALLBACK (_update_tooltips), button);
        g_signal_connect (action, "notify::label", G_CALLBACK (_update_label), button);
        g_signal_connect (action, "notify::icon-name", G_CALLBACK (_update_icon_name), button);

        /* hook into the ::clicked signal and block it unless
         * PolicyKit says it's good to go. This is necessary when the
         * button is embedded in e.g. a GtkDialog since that class
         * hooks in ::clicked signals from GtkButton instances...
         *
         * Also, hook into ::auth_end signal from the
         * PolKitMateAction and synthesize ::clicked the signal if
         * the privilege was gained..
         */
        g_signal_connect (button, "clicked", G_CALLBACK (_button_clicked), action);
        g_signal_connect (action, "auth-end", G_CALLBACK (_button_auth_end), button);

        return button;
}
