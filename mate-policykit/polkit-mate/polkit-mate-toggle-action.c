/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/***************************************************************************
 *
 * polkit-mate-toggle-action.c : 
 *
 * Copyright (C) 2007 David Zeuthen, <david@fubar.dk>
 *
 * Based loosely on gtk/gtktoggleaction.c from GTK+ written by James
 * Henstridge <james@daa.com.au>.
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

#include "polkit-mate-toggle-action.h"

/**
 * SECTION:polkit-mate-toggle-action
 * @short_description: A GtkAction that can be toggled to
 * obtain and give up PolicyKit authorizations.
 *
 * For an example of how to use this class, see the documentation for
 * #PolKitMateAction.
 **/

#define POLKIT_MATE_TOGGLE_ACTION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), POLKIT_MATE_TYPE_TOGGLE_ACTION, PolKitMateToggleActionPrivate))

struct _PolKitMateToggleActionPrivate 
{
        gboolean active;
        gboolean am_authenticating;

        gboolean auth_underway_visible;
        gboolean auth_underway_sensitive;
        gchar *auth_underway_short_label;
        gchar *auth_underway_label;
        gchar *auth_underway_tooltip;
        gchar *auth_underway_icon_name;
};

enum 
{
        TOGGLED_SIGNAL,
        LAST_SIGNAL
};

enum
{
        PROP_0,
        PROP_POLKIT_AUTH_UNDERWAY_VISIBLE,
        PROP_POLKIT_AUTH_UNDERWAY_SENSITIVE,
        PROP_POLKIT_AUTH_UNDERWAY_SHORT_LABEL,
        PROP_POLKIT_AUTH_UNDERWAY_LABEL,
        PROP_POLKIT_AUTH_UNDERWAY_TOOLTIP,
        PROP_POLKIT_AUTH_UNDERWAY_ICON_NAME,
};

G_DEFINE_TYPE (PolKitMateToggleAction, polkit_mate_toggle_action, POLKIT_MATE_TYPE_ACTION)

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
polkit_mate_toggle_action_init (PolKitMateToggleAction *toggle_action)
{
        toggle_action->priv = POLKIT_MATE_TOGGLE_ACTION_GET_PRIVATE (toggle_action);

}

static void
polkit_mate_toggle_action_finalize (GObject *object)
{
        PolKitMateToggleAction *toggle_action;

        toggle_action = POLKIT_MATE_TOGGLE_ACTION (object);

        G_OBJECT_CLASS (polkit_mate_toggle_action_parent_class)->finalize (object);
}

static GObject *
polkit_mate_toggle_action_constructor (GType                  type,
                                        guint                  n_construct_properties,
                                        GObjectConstructParam *construct_properties)
{
        PolKitMateToggleAction      *toggle_action;
        PolKitMateToggleActionClass *klass;

        klass = POLKIT_MATE_TOGGLE_ACTION_CLASS (g_type_class_peek (POLKIT_MATE_TYPE_TOGGLE_ACTION));

        toggle_action = POLKIT_MATE_TOGGLE_ACTION (G_OBJECT_CLASS (parent_class)->constructor (type,
                                                                                   n_construct_properties,
                                                                                   construct_properties));

        if (polkit_mate_action_get_polkit_result (POLKIT_MATE_ACTION (toggle_action)) == POLKIT_RESULT_YES)
                toggle_action->priv->active = TRUE;
        else
                toggle_action->priv->active = FALSE;

        return G_OBJECT (toggle_action);
}

static void polkit_mate_toggle_action_auth_end (PolKitMateToggleAction *toggle_action, gboolean gained_privilege);
static void polkit_mate_toggle_polkit_result_changed (PolKitMateToggleAction *toggle_action, PolKitResult current_result);
static void polkit_mate_toggle_action_activate (PolKitMateToggleAction *toggle_action);
static void polkit_mate_toggle_action_real_toggled (PolKitMateToggleAction *action);
static void connect_proxy                  (GtkAction       *action,
					    GtkWidget       *proxy);
static void disconnect_proxy               (GtkAction       *action,
					    GtkWidget       *proxy);


static void
polkit_mate_toggle_action_class_init (PolKitMateToggleActionClass *klass)
{
        GObjectClass *gobject_class;
        GtkActionClass *action_class;
        PolKitMateActionClass *polkit_mate_action_class;

        parent_class = g_type_class_peek_parent (klass);
        gobject_class = G_OBJECT_CLASS (klass);
        action_class = GTK_ACTION_CLASS (klass);
        polkit_mate_action_class = POLKIT_MATE_ACTION_CLASS (klass);

        gobject_class->constructor = polkit_mate_toggle_action_constructor;
        gobject_class->set_property = set_property;
        gobject_class->get_property = get_property;
        gobject_class->finalize = polkit_mate_toggle_action_finalize;

        action_class->menu_item_type = GTK_TYPE_CHECK_MENU_ITEM;
        action_class->toolbar_item_type = GTK_TYPE_TOGGLE_TOOL_BUTTON;

        action_class->connect_proxy = connect_proxy;
        action_class->disconnect_proxy = disconnect_proxy;

        klass->toggled = polkit_mate_toggle_action_real_toggled;

        action_class->activate = (void (*)(GtkAction*)) polkit_mate_toggle_action_activate;

        polkit_mate_action_class->auth_end = 
                (void (*)(PolKitMateAction *, gboolean)) polkit_mate_toggle_action_auth_end;

        polkit_mate_action_class->polkit_result_changed = 
                (void (*) (PolKitMateAction *, PolKitResult)) polkit_mate_toggle_polkit_result_changed;

        /*------------------------------*/

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_UNDERWAY_VISIBLE,
                g_param_spec_boolean (
                        "auth-underway-visible",
                        "When authentication is underway, whether the action will be visible",
                        "When authentication is underway, whether the action will be visible",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_UNDERWAY_SENSITIVE,
                g_param_spec_boolean (
                        "auth-underway-sensitive",
                        "When authentication is underway, whether the action will be sensitive",
                        "When authentication is underway, whether the action will be sensitive",
                        TRUE,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_UNDERWAY_SHORT_LABEL,
                g_param_spec_string (
                        "auth-underway-short-label",
                        "When authentication is underway, use this short-label",
                        "When authentication is underway, use this short-label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_UNDERWAY_LABEL,
                g_param_spec_string (
                        "auth-underway-label",
                        "When authentication is underway, use this label",
                        "When authentication is underway, use this label",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_UNDERWAY_TOOLTIP,
                g_param_spec_string (
                        "auth-underway-tooltip",
                        "When authentication is underway, use this tooltip",
                        "When authentication is underway, use this tooltip",
                        NULL,
                        G_PARAM_READWRITE));

        g_object_class_install_property (
                gobject_class,
                PROP_POLKIT_AUTH_UNDERWAY_ICON_NAME,
                g_param_spec_string (
                        "auth-underway-icon-name",
                        "When authentication is underway, use this icon-name",
                        "When authentication is underway, use this icon-name",
                        NULL,
                        G_PARAM_READWRITE));

        /*------------------------------*/


        /**
         * PolKitMateToggleAction::toggled:
         * @toggle_action: the object
         *
         * The ::toggled signal is emitted when the button is toggled.
         **/
        signals [TOGGLED_SIGNAL] =
                g_signal_new ("toggled",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PolKitMateToggleActionClass, toggled),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        g_type_class_add_private (gobject_class, sizeof (PolKitMateToggleActionPrivate));
}


/**
 * polkit_mate_toggle_action_new:
 * @name: A unique name for the toggle_action
 * @error: Return location for error
 *
 * Creates a new #PolKitMateToggleAction object. The typical use for
 * this function is for specialized use where
 * polkit_mate_toggle_action_new_default() does not meet the needs of
 * the application.
 *
 * If the 'polkit-toggle_action' property is #NULL the behavior is similar to
 * as if a #PolKitAction returned #POLKIT_RESULT_YES.
 *
 * Returns: a new #PolKitMateToggleAction or #NULL if error is set
 */
PolKitMateToggleAction *
polkit_mate_toggle_action_new (const gchar *name)
{
        PolKitMateToggleAction *toggle_action = NULL;

        toggle_action = g_object_new (POLKIT_MATE_TYPE_TOGGLE_ACTION,
                                      "name", name,
                                      NULL);
        return toggle_action;
}

/**
 * polkit_mate_toggle_action_new_default:
 * @name: A unique name for the toggle_action
 * @polkit_action: the #PolKitAction to track
 * @locked_label: The label to show when the user do not possess the
 * authorization, e.g. "Click to make changes"
 * @unlocked_label: The label to show when the user does posses the
 * authorization; e.g. "Click to prevent changes"
 *
 * Creates a new #PolKitMateToggleAction object with the default
 * behavior for a given #PolKitAction object. The toggle represents
 * whether the user is authorized for the given #PolKitAction. If the
 * user is not authorized, clicking on a proxy widget (if it's
 * sensitive) will cause an authentication dialog to appear. If the
 * user is authorized, clicking on a proxy widget will cause all
 * authorizations to be given up (aka revoked). The ::toggled signal
 * is only emitted when such transitions occur. Thus, the user of this
 * class will never have to deal with bringing up authentication
 * dialogs; it's all handled behind the scenes.
 *
 * As such, the typical use case for this action is an UI where the
 * user is encouraged to give up authorizations.
 *
 * There's also support for the corner case where the user is
 * authorized because of implicit authorizations. In this case,
 * toggling the action will "grant" a negative authorization for the
 * user. By toggling the action again, the negative authorization will
 * be revoked.
 *
 * Default behavior is defined as having the icon_name be 'security-medium'
 * by default except for the YES PolicyKit answer where it's set to
 * 'security-low'. No tooltips are set. The label and short-label
 * will be 'locked_label' everywhere except in the YES state where
 * it's set to 'unlocked-label'. When authentication is underway, the
 * label will be "Authenticating..." and icon_name is untouched. If
 * the PolicyKit answer is NO, sensitivity is set to
 * #FALSE. Visibility is always set to #TRUE.
 *
 * The caller can always modify individual aspects of the
 * toggle_action after creation, e.g. change the tooltip for the no,
 * auth and yes states; see the parent class #PolKitMateAction. In
 * addition to the properties in the parent class, this subclass
 * sports six new properties, "auth-underway-*" to control the look
 * of proxy widgets when authentication is underway.
 *
 * If the given polkit_toggle_action is #NULL the behavior is similar to as
 * if a #PolKitAction returned #POLKIT_RESULT_YES.
 *
 * Returns: a new #PolKitMateToggleAction or #NULL if error is set
 */
PolKitMateToggleAction *
polkit_mate_toggle_action_new_default (const gchar  *name, 
                                        PolKitAction *polkit_action, 
                                        const gchar  *locked_label, 
                                        const gchar  *unlocked_label)
{
        PolKitMateToggleAction *toggle_action;

        toggle_action = g_object_new (POLKIT_MATE_TYPE_TOGGLE_ACTION,
                                      "name",             name,
                                      "polkit-action",    polkit_action,
                                      
                                      "self-blocked-visible",     TRUE,
                                      "self-blocked-sensitive",   TRUE,
                                      "self-blocked-short-label", locked_label,
                                      "self-blocked-label",       locked_label,
                                      "self-blocked-tooltip",     NULL,
                                      "self-blocked-icon-name",   "security-medium",

                                      "no-visible",       TRUE,
                                      "no-sensitive",     FALSE,
                                      "no-short-label",   locked_label,
                                      "no-label",         locked_label,
                                      "no-tooltip",       NULL,
                                      "no-icon-name",     "security-medium",
                                      
                                      "auth-visible",     TRUE,
                                      "auth-sensitive",   TRUE,
                                      "auth-short-label", locked_label,
                                      "auth-label",       locked_label,
                                      "auth-tooltip",     NULL,
                                      "auth-icon-name",   "security-medium",
                                      
                                      "yes-visible",      TRUE,
                                      "yes-sensitive",    TRUE,
                                      "yes-short-label",  unlocked_label,
                                      "yes-label",        unlocked_label,
                                      "yes-tooltip",      NULL,
                                      "yes-icon-name",    "security-low",

                                      "auth-underway-visible",      TRUE,
                                      "auth-underway-sensitive",    TRUE,
                                      "auth-underway-short-label",  _("Authenticating..."),
                                      "auth-underway-label",        _("Authenticating..."),
                                      "auth-underway-tooltip",      NULL,
                                      "auth-underway-icon-name",    "security-medium",
                                      
                                      "master-visible",   TRUE,
                                      "master-sensitive", TRUE,
                                      NULL);
        return toggle_action;
}

/*---------------------------------------------------------------------------------security-medium-------------------*/

static void
polkit_mate_toggle_action_set_auth_underway_visible (PolKitMateToggleAction *action, gboolean visible)
{
        action->priv->auth_underway_visible = visible;
}

static void
polkit_mate_toggle_action_set_auth_underway_sensitive (PolKitMateToggleAction *action, gboolean sensitive)
{
        action->priv->auth_underway_sensitive = sensitive;
}

static void
polkit_mate_toggle_action_set_auth_underway_short_label (PolKitMateToggleAction *action, const gchar *short_label)
{
        g_free (action->priv->auth_underway_short_label);
        action->priv->auth_underway_short_label = g_strdup (short_label);
}

static void
polkit_mate_toggle_action_set_auth_underway_label (PolKitMateToggleAction *action, const gchar *label)
{
        g_free (action->priv->auth_underway_label);
        action->priv->auth_underway_label = g_strdup (label);
}

static void
polkit_mate_toggle_action_set_auth_underway_tooltip (PolKitMateToggleAction *action, const gchar *tooltip)
{
        g_free (action->priv->auth_underway_tooltip);
        action->priv->auth_underway_tooltip = g_strdup (tooltip);
}

static void
polkit_mate_toggle_action_set_auth_underway_icon_name (PolKitMateToggleAction *action, const gchar *icon_name)
{
        g_free (action->priv->auth_underway_icon_name);
        action->priv->auth_underway_icon_name = g_strdup (icon_name);
}

static void
get_property (GObject     *object,
	      guint        prop_id,
	      GValue      *value,
	      GParamSpec  *pspec)
{
        PolKitMateToggleAction *action = POLKIT_MATE_TOGGLE_ACTION (object);
        
        switch (prop_id)
        {

        case PROP_POLKIT_AUTH_UNDERWAY_VISIBLE:
                g_value_set_boolean (value, action->priv->auth_underway_visible);
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_SENSITIVE:
                g_value_set_boolean (value, action->priv->auth_underway_sensitive);
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_SHORT_LABEL:
                g_value_set_string (value, action->priv->auth_underway_short_label);
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_LABEL:
                g_value_set_string (value, action->priv->auth_underway_label);
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_TOOLTIP:
                g_value_set_string (value, action->priv->auth_underway_tooltip);
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_ICON_NAME:
                g_value_set_string (value, action->priv->auth_underway_icon_name);
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
        PolKitMateToggleAction *action = POLKIT_MATE_TOGGLE_ACTION (object);
        
        switch (prop_id)
        {

        case PROP_POLKIT_AUTH_UNDERWAY_VISIBLE:
                polkit_mate_toggle_action_set_auth_underway_visible (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_SENSITIVE:
                polkit_mate_toggle_action_set_auth_underway_sensitive (action, g_value_get_boolean (value));
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_SHORT_LABEL:
                polkit_mate_toggle_action_set_auth_underway_short_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_LABEL:
                polkit_mate_toggle_action_set_auth_underway_label (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_TOOLTIP:
                polkit_mate_toggle_action_set_auth_underway_tooltip (action, g_value_get_string (value));
                break;
        case PROP_POLKIT_AUTH_UNDERWAY_ICON_NAME:
                polkit_mate_toggle_action_set_auth_underway_icon_name (action, g_value_get_string (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
_set_proxy_state (PolKitMateToggleAction *toggle_action)
{
        GSList *i;

        for (i = gtk_action_get_proxies (GTK_ACTION (toggle_action)); i; i = i->next) {
                GtkWidget *proxy = i->data;
                
                gtk_action_block_activate_from (GTK_ACTION (toggle_action), proxy);
                if (GTK_IS_CHECK_MENU_ITEM (proxy))
                        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (proxy),
                                                        toggle_action->priv->active);
                else if (GTK_IS_TOGGLE_TOOL_BUTTON (proxy))
                        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (proxy),
                                                           toggle_action->priv->active);
                else if (GTK_IS_TOGGLE_BUTTON (proxy))
                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proxy),
                                                      toggle_action->priv->active);
                else {
                        g_warning ("Don't know how to toggle `%s' widgets",
                                   G_OBJECT_TYPE_NAME (proxy));
                }
                gtk_action_unblock_activate_from (GTK_ACTION (toggle_action), proxy);
        }
}

static void
_update_toggled (PolKitMateToggleAction *toggle_action)
{
        gboolean is_active;
        
        g_return_if_fail (POLKIT_MATE_IS_TOGGLE_ACTION (toggle_action));

        if (polkit_mate_action_get_polkit_result (POLKIT_MATE_ACTION (toggle_action)) == POLKIT_RESULT_YES)
                is_active = TRUE;
        else
                is_active = FALSE;

        if (toggle_action->priv->active != is_active) {
                toggle_action->priv->active = is_active;

                _set_proxy_state (toggle_action);

                g_signal_emit (toggle_action, signals[TOGGLED_SIGNAL], 0);
        }
}

static void 
polkit_mate_toggle_polkit_result_changed (PolKitMateToggleAction *toggle_action, PolKitResult current_result)
{
        _update_toggled (toggle_action);
}

static void 
polkit_mate_toggle_action_auth_end (PolKitMateToggleAction *toggle_action, gboolean gained_privilege)
{
        _update_toggled (toggle_action);
        toggle_action->priv->am_authenticating = FALSE;
}

static polkit_bool_t
_auth_foreach_revoke (PolKitAuthorizationDB *authdb,
                      PolKitAuthorization   *auth, 
                      void                  *user_data)
{
        PolKitError *pk_error;
        int *num_auths_revoked = (int *) user_data;

        pk_error = NULL;
        if (!polkit_authorization_db_revoke_entry (authdb, auth, &pk_error)) {
                g_warning ("Error revoking authorization: %s: %s\n", 
                           polkit_error_get_error_name (pk_error),
                           polkit_error_get_error_message (pk_error));
                polkit_error_free (pk_error);
        }

        if (num_auths_revoked != NULL)
                *num_auths_revoked += 1;
        
        return FALSE;
}

static void
polkit_mate_toggle_action_activate (PolKitMateToggleAction *toggle_action)
{
        PolKitError *pk_error;
        PolKitAction *pk_action;
        PolKitMateContext *pkgc;
        PolKitAuthorizationDB *authdb;
        PolKitResult pk_result;
        polkit_bool_t do_not_grant_negative;
                                
        pkgc = polkit_mate_context_get (NULL);
        authdb = polkit_context_get_authorization_db (pkgc->pk_context);

        pk_action = NULL;
        g_object_get (toggle_action, "polkit-action", &pk_action, NULL);

        g_return_if_fail (POLKIT_MATE_IS_TOGGLE_ACTION (toggle_action));

        pk_result = polkit_mate_action_get_polkit_result (POLKIT_MATE_ACTION (toggle_action));

        do_not_grant_negative = FALSE;
reevaluate:

        switch (pk_result) {
        case POLKIT_RESULT_YES:

                if (!toggle_action->priv->am_authenticating) {
                        /* If we already got the authorization.. revoke it! */
                        
                        if (pk_action != NULL && authdb != NULL) {
                                int num_auths_revoked;
                                
                                pk_error = NULL;
                                num_auths_revoked = 0;
                                polkit_authorization_db_foreach_for_action_for_uid (authdb,
                                                                                    pk_action,
                                                                                    getuid (),
                                                                                    _auth_foreach_revoke,
                                                                                    &num_auths_revoked,
                                                                                    &pk_error);
                                if (pk_error != NULL) {
                                        g_warning ("Error removing authorizations: code=%d: %s",
                                                   polkit_error_get_error_code (pk_error),
                                                   polkit_error_get_error_message (pk_error));
                                        polkit_error_free (pk_error);
                                }

                                if (pk_error == NULL && num_auths_revoked == 0 && !do_not_grant_negative) {
                                        /* no authorizations, yet we are authorized.. "grant" a
                                         * negative authorization...
                                         */
                                        
                                        if (!polkit_authorization_db_grant_negative_to_uid (
                                                    authdb, 
                                                    pk_action, 
                                                    getuid (), 
                                                    NULL, /* no constraints */
                                                    &pk_error)) {
                                                g_warning ("Error granting negative auth: %s: %s\n", 
                                                           polkit_error_get_error_name (pk_error),
                                                           polkit_error_get_error_message (pk_error));
                                                polkit_error_free (pk_error);
                                        }
                                }
                                
                        }

                        _update_toggled (toggle_action);
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

                g_signal_stop_emission_by_name (toggle_action, "activate");

                if (pk_action != NULL) {
                        toggle_action->priv->am_authenticating = TRUE;

                        g_object_set (
                                toggle_action, 
                                "visible", toggle_action->priv->auth_underway_visible && 
                                           polkit_mate_action_get_visible (POLKIT_MATE_ACTION (toggle_action)),
                                "sensitive", toggle_action->priv->auth_underway_sensitive &&
                                             polkit_mate_action_get_sensitive (POLKIT_MATE_ACTION (toggle_action)),
                                "short-label", toggle_action->priv->auth_underway_short_label,
                                "label", toggle_action->priv->auth_underway_label,
                                "tooltip", toggle_action->priv->auth_underway_tooltip,
                                "icon-name", toggle_action->priv->auth_underway_icon_name,
                                NULL);

                        g_signal_emit_by_name (toggle_action, "auth-start", 0);
                }
                break;

        default:
        case POLKIT_RESULT_NO:
                if (pk_action != NULL && authdb != NULL) {
                        if (polkit_authorization_db_is_uid_blocked_by_self (authdb, 
                                                                            pk_action,
                                                                            getuid (),
                                                                            NULL)) {
                                int num_auths_revoked;

                                /* block granted by self.. revoke it then.. */
                                pk_error = NULL;
                                num_auths_revoked = 0;
                                polkit_authorization_db_foreach_for_action_for_uid (authdb,
                                                                                    pk_action,
                                                                                    getuid (),
                                                                                    _auth_foreach_revoke,
                                                                                    &num_auths_revoked,
                                                                                    &pk_error);
                                if (pk_error != NULL) {
                                        g_warning ("Error removing authorizations: code=%d: %s",
                                                   polkit_error_get_error_code (pk_error),
                                                   polkit_error_get_error_message (pk_error));
                                        polkit_error_free (pk_error);
                                }

                                if (pk_error == NULL && num_auths_revoked > 0) {
                                        PolKitResult pk_result_new;

                                        /* we managed to revoke something... so with this change the
                                         * result should now change to AUTH_*... so go ahead and just
                                         * reevaluate the result... also remember to avoid granting
                                         * another negative auth!
                                         */

                                        polkit_context_force_reload (pkgc->pk_context);
                                        pk_result_new = polkit_mate_action_get_polkit_result (POLKIT_MATE_ACTION (toggle_action));
                                        if (pk_result_new != pk_result) {
                                                pk_result = pk_result_new;
                                                do_not_grant_negative = TRUE;
                                                goto reevaluate;
                                        }

                                }

                        }
                }

                /* If PolicyKit says no... and we got here.. it means
                 * that the user set the property "no-sensitive" to
                 * TRUE.. Otherwise we couldn't be handling this signal.
                 *
                 * Hence, they probably have a good reason for doing
                 * this so do let the 'activate' signal propagate.. 
                 */
                break;
        }

        _set_proxy_state (toggle_action);

        if (pk_action != NULL)
                polkit_action_unref (pk_action);
}

static void
polkit_mate_toggle_action_real_toggled (PolKitMateToggleAction *action)
{
        GSList *i;
        
        g_return_if_fail (POLKIT_MATE_IS_TOGGLE_ACTION (action));
        
        for (i = gtk_action_get_proxies (GTK_ACTION (action)); i; i = i->next) {
                GtkWidget *proxy = i->data;
                
                gtk_action_block_activate_from (GTK_ACTION (action), proxy);
                if (GTK_IS_CHECK_MENU_ITEM (proxy))
                        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (proxy),
                                                        action->priv->active);
                else if (GTK_IS_TOGGLE_TOOL_BUTTON (proxy))
                        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (proxy),
                                                           action->priv->active);
                else if (GTK_IS_TOGGLE_BUTTON (proxy))
                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proxy),
                                                      action->priv->active);
                else {
                        g_warning ("Don't know how to toggle `%s' widgets",
                                   G_OBJECT_TYPE_NAME (proxy));
                }
                gtk_action_unblock_activate_from (GTK_ACTION (action), proxy);
        }
}

static void
connect_proxy (GtkAction *action, GtkWidget *proxy)
{
        PolKitMateToggleAction *toggle_action;
        
        toggle_action = POLKIT_MATE_TOGGLE_ACTION (action);
        
        /* do this before hand, so that we don't call the "activate" handler */
        if (GTK_IS_CHECK_MENU_ITEM (proxy))
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (proxy),
                                                toggle_action->priv->active);
        else if (GTK_IS_TOGGLE_TOOL_BUTTON (proxy))
                gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (proxy),
                                                   toggle_action->priv->active);
        else if (GTK_IS_TOGGLE_BUTTON (proxy))
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proxy),
                                              toggle_action->priv->active);
        
        (* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
}

static void
disconnect_proxy (GtkAction *action, GtkWidget *proxy)
{
        (* GTK_ACTION_CLASS (parent_class)->disconnect_proxy) (action, proxy);
}

static void
_update_tooltips (PolKitMateToggleAction *action, GParamSpec *arg1, GtkWidget *widget)
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
_update_label (PolKitMateToggleAction *action, GParamSpec *arg1, GtkWidget *widget)
{
        char *label;

        label = NULL;
        g_object_get (action, "label", &label, NULL);
        gtk_button_set_label (GTK_BUTTON (widget), label);
        g_free (label);
}

static void
_update_icon_name (PolKitMateToggleAction *action, GParamSpec *arg1, GtkWidget *widget)
{
        gtk_button_set_image (GTK_BUTTON (widget), gtk_action_create_icon (GTK_ACTION (action), GTK_ICON_SIZE_BUTTON));
}

static void 
_button_toggled (GtkToggleButton *button, PolKitMateToggleAction *action)
{
        /* g_debug ("in _button_toggled"); */

        switch (polkit_mate_action_get_polkit_result (POLKIT_MATE_ACTION (action))) {
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
                /* g_debug ("blocking toggled"); */
                g_signal_stop_emission_by_name (button, "toggled");
                break;

        default:
        case POLKIT_RESULT_NO:
                break;
        }
}

/**
 * polkit_mate_toggle_action_create_toggle_button:
 * @action: The #PolKitMateToggleAction object
 *
 * Create a toggle button for the given action that displays the
 * label, tooltip and icon_name corresponding to whether the state,
 * according to PolicyKit, is no, auth or yes.
 * 
 * Returns: A #GtkToggleButton instance connected to the action
 */
GtkWidget *
polkit_mate_toggle_action_create_toggle_button (PolKitMateToggleAction *action)
{
        GtkWidget *button;

        button = gtk_toggle_button_new ();

        gtk_action_connect_proxy (GTK_ACTION (action), button);

        _update_label (action, NULL, button);
        _update_tooltips (action, NULL, button);
        _update_icon_name (action, NULL, button);

        g_signal_connect (action, "notify::tooltip", G_CALLBACK (_update_tooltips), button);
        g_signal_connect (action, "notify::label", G_CALLBACK (_update_label), button);
        g_signal_connect (action, "notify::icon-name", G_CALLBACK (_update_icon_name), button);

        /* hook into the ::toggled signal and block it unless
         * PolicyKit says it's good to go. 
         */
        g_signal_connect (button, "toggled", G_CALLBACK (_button_toggled), action);

        return button;
}
