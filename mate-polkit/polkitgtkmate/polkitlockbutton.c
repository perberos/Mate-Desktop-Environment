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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <string.h>
#include <polkit/polkit.h>

#include "polkitlockbutton.h"

/**
 * SECTION:polkitlockbutton
 * @title: PolkitLockButton
 * @short_description: Widget for obtaining/revoking authorizations
 * @stability: Stable
 *
 * #PolkitLockButton is a widget that can be used in control panels to
 * allow users to obtain and revoke authorizations needed for the
 * control panel UI to function.
 *
 * If the user lacks the authorization but authorization can be
 * obtained through authentication, the widget looks like this
 * <mediaobject id="lock-button-locked">
 *  <imageobject>
 *    <imagedata fileref="polkit-lock-button-locked.png" format="PNG"/>
 *  </imageobject>
 * </mediaobject>
 * and the user can click the button to obtain the authorization. This
 * will pop up an authentication dialog.
 * Once authorization is obtained, the widget changes to this
 * <mediaobject id="lock-button-unlocked">
 *  <imageobject>
 *    <imagedata fileref="polkit-lock-button-unlocked.png" format="PNG"/>
 *  </imageobject>
 * </mediaobject>
 * and the authorization can be dropped by clicking the button.
 * If the user is not able to obtain authorization at all, the widget
 * looks like this
 * <mediaobject id="lock-button-unlocked-not-authorized">
 *  <imageobject>
 *    <imagedata fileref="polkit-lock-button-not-authorized.png" format="PNG"/>
 *  </imageobject>
 * </mediaobject>
 * If the user is authorized (either implicitly via the .policy file
 * defaults or through e.g. Local Authority configuration) and no
 * authentication is necessary and the Authority Implementation
 * supports lock-down, the widget looks like this
 * <mediaobject id="lock-button-unlocked-lock-down">
 *  <imageobject>
 *    <imagedata fileref="polkit-lock-button-lock-down.png" format="PNG"/>
 *  </imageobject>
 * </mediaobject>
 * allowing the user to lock down the action. The lockdown can be
 * removed by right clicking the button - the user can discover this
 * through the tooltip. If the Authority implementation does not
 * support lockdown, the widget will be hidden.
 *
 * Finally, if the user is not authorized but authorization can be
 * obtained and the obtained authorization will be a one-shot
 * authorization, the widget will be hidden. This means that any
 * attempt to use the Mechanism that requires authorization for the
 * specified action will always prompt for authentication. This
 * condition happens exactly when
 * (!polkit_lock_button_get_is_authorized() &&
 * polkit_lock_button_get_can_obtain() &&
 * !polkit_lock_button_get_is_visible()) is %TRUE.
 *
 * Typically #PolkitLockButton is only useful for actions where
 * authorization is obtained through authentication (and retained) or
 * where users are implictly authorized (cf. the defaults specified in
 * the <literal>.policy</literal> file for the action) but note that
 * this behavior can be overridden by the Authority implementation.
 *
 * The typical usage of this widget is like this:
 * <programlisting>
 * static void
 * update_sensitivity_according_to_lock_button (FooBar *bar)
 * {
 *   gboolean make_sensitive;
 *
 *   make_sensitive = FALSE;
 *   if (polkit_lock_button_get_is_authorized (POLKIT_LOCK_BUTTON (bar->priv->lock_button)))
 *     {
 *       make_sensitive = TRUE;
 *     }
 *   else
 *     {
 *       /<!-- -->* Catch the case where authorization is one-shot - this means
 *        * an authentication dialog will be shown every time a widget the user
 *        * manipulates calls into the Mechanism.
 *        *<!-- -->/
 *       if (polkit_lock_button_get_can_obtain (POLKIT_LOCK_BUTTON (bar->priv->lock_button)) &&
 *           !polkit_lock_button_get_is_visible (POLKIT_LOCK_BUTTON (bar->priv->lock_button)))
 *         make_sensitive = TRUE;
 *     }
 *
 *
 *   /<!-- -->* Make all widgets relying on authorization sensitive according to
 *    * the value of make_sensitive.
 *    *<!-- -->/
 * }
 *
 * static void
 * on_lock_button_changed (PolkitLockButton *button,
 *                         gpointer          user_data)
 * {
 *   FooBar *bar = FOO_BAR (user_data);
 *
 *   update_sensitivity_according_to_lock_button (bar);
 * }
 *
 * static void
 * foo_bar_init (FooBar *bar)
 * {
 *   /<!-- -->* Construct other widgets *<!-- -->/
 *
 *   bar->priv->lock_button = polkit_lock_button_new ("org.project.mechanism.action-name");
 *   g_signal_connect (bar->priv->lock_button,
 *                     "changed",
 *                     G_CALLBACK (on_lock_button_changed),
 *                     bar);
 *   update_sensitity_according_to_lock_button (bar);
 *
 *   /<!-- -->* Pack bar->priv->lock_button into widget hierarchy *<!-- -->/
 * }
 * </programlisting>
 */

struct _PolkitLockButtonPrivate {
	PolkitAuthority* authority;
	PolkitSubject* subject;
	gchar* action_id;

	gchar* text_unlock;
	gchar* text_lock;
	gchar* text_lock_down;
	gchar* text_not_authorized;

	gchar* tooltip_unlock;
	gchar* tooltip_lock;
	gchar* tooltip_lock_down;
	gchar* tooltip_not_authorized;

	GtkWidget* button;
	GtkWidget* label;

	gboolean can_obtain;
	gboolean retains_after_challenge;
	gboolean authorized;
	gboolean hidden;

	/* is non-NULL exactly when we are authorized and have a temporary authorization */
	gchar* tmp_authz_id;

	/* This is non-NULL exactly when we have a non-interactive check outstanding */
	GCancellable* check_cancellable;

	/* This is non-NULL exactly when we have an interactive check outstanding */
	GCancellable* interactive_check_cancellable;
};

enum{
	PROP_0,
	PROP_ACTION_ID,
	PROP_IS_AUTHORIZED,
	PROP_IS_VISIBLE,
	PROP_CAN_OBTAIN,
	PROP_TEXT_UNLOCK,
	PROP_TEXT_LOCK,
	PROP_TEXT_LOCK_DOWN,
	PROP_TEXT_NOT_AUTHORIZED,
	PROP_TOOLTIP_UNLOCK,
	PROP_TOOLTIP_LOCK,
	PROP_TOOLTIP_LOCK_DOWN,
	PROP_TOOLTIP_NOT_AUTHORIZED,
};

enum {
	CHANGED_SIGNAL,
	LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = {0, };

static void initiate_check(PolkitLockButton* button);
static void do_sync_check(PolkitLockButton* button);
static void update_state(PolkitLockButton* button);

static void on_authority_changed(PolkitAuthority* authority, gpointer user_data);

static void on_clicked(GtkButton* button, gpointer user_data);

G_DEFINE_TYPE(PolkitLockButton, polkit_lock_button, GTK_TYPE_HBOX);

static void polkit_lock_button_finalize(GObject* object)
{
	PolkitLockButton *button = POLKIT_LOCK_BUTTON(object);

	g_free(button->priv->action_id);
	g_free(button->priv->tmp_authz_id);
	g_object_unref(button->priv->subject);

	if (button->priv->check_cancellable != NULL)
	{
		g_cancellable_cancel(button->priv->check_cancellable);
		g_object_unref(button->priv->check_cancellable);
	}

	if (button->priv->interactive_check_cancellable != NULL)
	{
		g_cancellable_cancel(button->priv->interactive_check_cancellable);
		g_object_unref(button->priv->interactive_check_cancellable);
	}

	g_signal_handlers_disconnect_by_func(button->priv->authority, on_authority_changed, button);
	g_object_unref(button->priv->authority);

	if (G_OBJECT_CLASS(polkit_lock_button_parent_class)->finalize != NULL)
	{
		G_OBJECT_CLASS(polkit_lock_button_parent_class)->finalize(object);
	}
}

static void
polkit_lock_button_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PolkitLockButton *button = POLKIT_LOCK_BUTTON (object);

  switch (property_id)
    {
    case PROP_ACTION_ID:
      g_value_set_string (value, button->priv->action_id);
      break;

    case PROP_IS_AUTHORIZED:
      g_value_set_boolean (value, button->priv->authorized);
      break;

    case PROP_IS_VISIBLE:
      g_value_set_boolean (value, !button->priv->hidden);
      break;

    case PROP_CAN_OBTAIN:
      g_value_set_boolean (value, button->priv->can_obtain);
      break;

    case PROP_TEXT_UNLOCK:
      g_value_set_string (value, button->priv->text_unlock);
      break;

    case PROP_TEXT_LOCK:
      g_value_set_string (value, button->priv->text_lock);
      break;

    case PROP_TEXT_LOCK_DOWN:
      g_value_set_string (value, button->priv->text_lock_down);
      break;

    case PROP_TEXT_NOT_AUTHORIZED:
      g_value_set_string (value, button->priv->text_not_authorized);
      break;

    case PROP_TOOLTIP_UNLOCK:
      g_value_set_string (value, button->priv->tooltip_unlock);
      break;

    case PROP_TOOLTIP_LOCK:
      g_value_set_string (value, button->priv->tooltip_lock);
      break;

    case PROP_TOOLTIP_LOCK_DOWN:
      g_value_set_string (value, button->priv->tooltip_lock_down);
      break;

    case PROP_TOOLTIP_NOT_AUTHORIZED:
      g_value_set_string (value, button->priv->tooltip_not_authorized);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polkit_lock_button_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PolkitLockButton *button = POLKIT_LOCK_BUTTON (object);

  switch (property_id)
    {
    case PROP_ACTION_ID:
      button->priv->action_id = g_value_dup_string (value);
      break;

    case PROP_TEXT_UNLOCK:
      polkit_lock_button_set_unlock_text (button, g_value_get_string (value));
      break;

    case PROP_TEXT_LOCK:
      polkit_lock_button_set_lock_text (button, g_value_get_string (value));
      break;

    case PROP_TEXT_LOCK_DOWN:
      polkit_lock_button_set_lock_down_text (button, g_value_get_string (value));
      break;

    case PROP_TEXT_NOT_AUTHORIZED:
      polkit_lock_button_set_not_authorized_text (button, g_value_get_string (value));
      break;

    case PROP_TOOLTIP_UNLOCK:
      polkit_lock_button_set_unlock_tooltip (button, g_value_get_string (value));
      break;

    case PROP_TOOLTIP_LOCK:
      polkit_lock_button_set_lock_tooltip (button, g_value_get_string (value));
      break;

    case PROP_TOOLTIP_LOCK_DOWN:
      polkit_lock_button_set_lock_down_tooltip (button, g_value_get_string (value));
      break;

    case PROP_TOOLTIP_NOT_AUTHORIZED:
      polkit_lock_button_set_not_authorized_tooltip (button, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
polkit_lock_button_init (PolkitLockButton *button)
{
  button->priv = G_TYPE_INSTANCE_GET_PRIVATE (button,
                                              POLKIT_TYPE_LOCK_BUTTON,
                                              PolkitLockButtonPrivate);

}

static void
polkit_lock_button_constructed (GObject *object)
{
  PolkitLockButton *button = POLKIT_LOCK_BUTTON (object);
  GError *error;

  gtk_box_set_spacing (GTK_BOX (button), 2);

  /* TODO: should be async+failable (e.g. GAsyncInitable) instead of this */
  error = NULL;
  button->priv->authority = polkit_authority_get_sync (NULL /* GCancellable* */, &error);
  if (button->priv->authority == NULL)
    {
      g_critical ("Error getting authority: %s", error->message);
      g_error_free (error);
    }
  g_signal_connect (button->priv->authority,
                    "changed",
                    G_CALLBACK (on_authority_changed),
                    button);

  button->priv->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button->priv->button), GTK_RELIEF_NONE);
  /* image is set in update_state() */
  g_signal_connect (button->priv->button,
                    "clicked",
                    G_CALLBACK (on_clicked),
                    button);

  gtk_box_pack_start (GTK_BOX (button),
                      button->priv->button,
                      FALSE,
                      FALSE,
                      0);

  button->priv->label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (button),
                      button->priv->label,
                      FALSE,
                      FALSE,
                      0);

  /* take control of visibility of child widgets */
  gtk_widget_set_no_show_all (button->priv->button, TRUE);
  gtk_widget_set_no_show_all (button->priv->label, TRUE);

  if (button->priv->subject == NULL)
    {
      button->priv->subject = polkit_unix_process_new (getpid ());
    }

  /* synchronously check on construction - TODO: we could implement GAsyncInitable
   * in the future to avoid this sync check
   */
  do_sync_check (button);

  update_state (button);

  if (G_OBJECT_CLASS (polkit_lock_button_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (polkit_lock_button_parent_class)->constructed (object);
}

static void
polkit_lock_button_class_init (PolkitLockButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = polkit_lock_button_finalize;
  gobject_class->get_property = polkit_lock_button_get_property;
  gobject_class->set_property = polkit_lock_button_set_property;
  gobject_class->constructed  = polkit_lock_button_constructed;

  g_type_class_add_private (klass, sizeof (PolkitLockButtonPrivate));

  /**
   * PolkitLockButton:action-id:
   *
   * The action identifier to use for the button.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ACTION_ID,
                                   g_param_spec_string ("action-id",
                                                        _("Action Identifier"),
                                                        _("The action identifier to use for the button"),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:is-authorized:
   *
   * Whether the process is authorized.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_IS_AUTHORIZED,
                                   g_param_spec_boolean ("is-authorized",
                                                         _("Is Authorized"),
                                                         _("Whether the process is authorized"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:is-visible:
   *
   * Whether the widget is visible.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_IS_VISIBLE,
                                   g_param_spec_boolean ("is-visible",
                                                         _("Is Visible"),
                                                         _("Whether the widget is visible"),
                                                         TRUE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:can-obtain:
   *
   * Whether authorization can be obtained.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_CAN_OBTAIN,
                                   g_param_spec_boolean ("can-obtain",
                                                         _("Can Obtain"),
                                                         _("Whether authorization can be obtained"),
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:text-unlock:
   *
   * The text to display when prompting the user to unlock.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT_UNLOCK,
                                   g_param_spec_string ("text-unlock",
                                                        _("Unlock Text"),
                                                        _("The text to display when prompting the user to unlock."),
                                                        _("Click to make changes"),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:tooltip-unlock:
   *
   * The tooltip to display when prompting the user to unlock.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TOOLTIP_UNLOCK,
                                   g_param_spec_string ("tooltip-unlock",
                                                        _("Unlock Tooltip"),
                                                        _("The tooltip to display when prompting the user to unlock."),
                                                        _("Authentication is needed to make changes."),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:text-lock:
   *
   * The text to display when prompting the user to lock.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT_LOCK,
                                   g_param_spec_string ("text-lock",
                                                        _("Lock Text"),
                                                        _("The text to display when prompting the user to lock."),
                                                        _("Click to prevent changes"),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:tooltip-lock:
   *
   * The tooltip to display when prompting the user to lock.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TOOLTIP_LOCK,
                                   g_param_spec_string ("tooltip-lock",
                                                        _("Lock Tooltip"),
                                                        _("The tooltip to display when prompting the user to lock."),
                                                        _("To prevent further changes, click the lock."),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:text-lock-down:
   *
   * The text to display when prompting the user to lock down the action for all users.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT_LOCK_DOWN,
                                   g_param_spec_string ("text-lock-down",
                                                        _("Lock Down Text"),
                                                        _("The text to display when prompting the user to lock down the action for all users."),
                                                        _("Click to lock down"),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:tooltip-lock-down:
   *
   * The tooltip to display when prompting the user to lock down the action for all users.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TOOLTIP_LOCK_DOWN,
                                   g_param_spec_string ("tooltip-lock-down",
                                                        _("Lock Down Tooltip"),
                                                        _("The tooltip to display when prompting the user to lock down the action for all users."),
                                                        _("To prevent users without administrative privileges from making changes, click the lock."),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:text-not-authorized:
   *
   * The text to display when the user cannot obtain authorization through authentication.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT_NOT_AUTHORIZED,
                                   g_param_spec_string ("text-not-authorized",
                                                        _("Unlock Text"),
                                                        _("The text to display when the user cannot obtain authorization through authentication."),
                                                        _("Not authorized to make changes"),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton:tooltip-not-authorized:
   *
   * The tooltip to display when the user cannot obtain authorization through authentication.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TOOLTIP_NOT_AUTHORIZED,
                                   g_param_spec_string ("tooltip-not-authorized",
                                                        _("Unlock Tooltip"),
                                                        _("The tooltip to display when the user cannot obtain authorization through authentication."),
                                                        _("System policy prevents changes. Contact your system administator."),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  /**
   * PolkitLockButton::changed:
   * @button: A #PolkitLockButton.
   *
   * Emitted when something on @button changes.
   */
  signals[CHANGED_SIGNAL] = g_signal_new ("changed",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (PolkitLockButtonClass, changed),
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE,
                                          0);

}

/**
 * polkit_lock_button_new:
 * @action_id: An action identifer.
 *
 * Constructs a #PolkitLockButton for @action_id.
 *
 * Returns: A #PolkitLockButton.
 */
GtkWidget *
polkit_lock_button_new (const gchar *action_id)
{
  g_return_val_if_fail (action_id != NULL, NULL);

  return GTK_WIDGET (g_object_new (POLKIT_TYPE_LOCK_BUTTON,
                                   "action-id", action_id,
                                   NULL));
}

static void
update_state (PolkitLockButton *button)
{
  const gchar *text;
  const gchar *tooltip;
  gboolean sensitive;
  gboolean old_hidden;
  GtkWidget *image;

  old_hidden = button->priv->hidden;
  button->priv->hidden = FALSE;

  if (button->priv->authorized)
    {
      text = button->priv->text_lock;
      tooltip = button->priv->tooltip_lock;
      sensitive = TRUE;
      /* if the authorization isn't temporary => ask if user wants to lock the authorization down the
       * authority we're using has that capability
       */
      if (button->priv->tmp_authz_id == NULL)
        {
          //button->priv->hidden = TRUE;
        }
    }
  else
    {
      if (button->priv->can_obtain && button->priv->retains_after_challenge)
        {
          /* can retain and obtain authorization => show the unlock button */
          text = button->priv->text_unlock;
          tooltip = button->priv->tooltip_unlock;
          g_free (button->priv->tmp_authz_id);
          button->priv->tmp_authz_id = NULL;
          sensitive = TRUE;
        }
      else
        {
          if (button->priv->can_obtain)
            {
              /* we can obtain authorization, we just can't retain it => hidden */
              button->priv->hidden = TRUE;
            }
          else
            {
              /* cannot even obtain authorization => tell user he can't have a pony */
              text = button->priv->text_not_authorized;
              tooltip = button->priv->tooltip_not_authorized;
              g_free (button->priv->tmp_authz_id);
              button->priv->tmp_authz_id = NULL;
              sensitive = FALSE;
            }
        }
    }

  image = gtk_image_new_from_icon_name (button->priv->authorized ? "changes-allow" : "changes-prevent",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_button_set_image (GTK_BUTTON (button->priv->button), image);
  gtk_label_set_text (GTK_LABEL (button->priv->label), text);
  gtk_widget_set_sensitive (button->priv->button, sensitive);

  gtk_widget_set_tooltip_markup (GTK_WIDGET (button->priv->label), tooltip);
  gtk_widget_set_tooltip_markup (GTK_WIDGET (button->priv->button), tooltip);

  if (button->priv->hidden)
    {
      gtk_widget_hide (button->priv->button);
      gtk_widget_hide (button->priv->label);
    }
  else
    {
      gtk_widget_show (button->priv->button);
      gtk_widget_show (button->priv->label);
    }

  if (old_hidden != button->priv->hidden)
    g_object_notify (G_OBJECT (button), "is-visible");
}

static void
on_authority_changed (PolkitAuthority *authority,
                      gpointer         user_data)
{
  PolkitLockButton *button = POLKIT_LOCK_BUTTON (user_data);
  initiate_check (button);
}

static void
process_result (PolkitLockButton          *button,
                PolkitAuthorizationResult *result)
{
  gboolean old_can_obtain;
  gboolean old_authorized;

  old_can_obtain = button->priv->can_obtain;
  old_authorized = button->priv->authorized;
  button->priv->can_obtain = polkit_authorization_result_get_is_challenge (result);
  button->priv->authorized = polkit_authorization_result_get_is_authorized (result);

  /* save the temporary authorization id */
  g_free (button->priv->tmp_authz_id);
  button->priv->tmp_authz_id = g_strdup (polkit_authorization_result_get_temporary_authorization_id (result));
  button->priv->retains_after_challenge = polkit_authorization_result_get_retains_authorization (result);

  update_state (button);

  if (old_can_obtain != button->priv->can_obtain ||
      old_authorized != button->priv->authorized)
    {
      g_signal_emit (button,
                     signals[CHANGED_SIGNAL],
                     0);
    }

  if (old_can_obtain != button->priv->can_obtain)
    g_object_notify (G_OBJECT (button), "can-obtain");

  if (old_authorized != button->priv->authorized)
    g_object_notify (G_OBJECT (button), "is-authorized");
}

static void
check_cb (GObject       *source_object,
          GAsyncResult  *res,
          gpointer       user_data)
{
  PolkitAuthority *authority = POLKIT_AUTHORITY (source_object);
  PolkitLockButton *button = POLKIT_LOCK_BUTTON (user_data);
  PolkitAuthorizationResult *result;
  GError *error;

  error = NULL;
  result = polkit_authority_check_authorization_finish (authority,
                                                        res,
                                                        &error);
  if (error != NULL)
    {
      g_warning ("Error checking authorization for action id `%s': %s",
                 button->priv->action_id,
                 error->message);
      g_error_free (error);
    }
  else
    {
      process_result (button, result);
    }

  if (result != NULL)
    g_object_unref (result);

  if (button->priv->check_cancellable != NULL)
    {
      g_object_unref (button->priv->check_cancellable);
      button->priv->check_cancellable = NULL;
    }
}

static void
initiate_check (PolkitLockButton *button)
{

  /* if we have a check pending already, then do nothing */
  if (button->priv->check_cancellable != NULL)
    goto out;

  button->priv->check_cancellable = g_cancellable_new ();

  polkit_authority_check_authorization (button->priv->authority,
                                        button->priv->subject,
                                        button->priv->action_id,
                                        NULL, /* PolkitDetails */
                                        POLKIT_CHECK_AUTHORIZATION_FLAGS_NONE,
                                        button->priv->check_cancellable,
                                        check_cb,
                                        button);

 out:
  ;
}

static void
do_sync_check (PolkitLockButton *button)
{

  GError *error;
  PolkitAuthorizationResult *result;

  error = NULL;
  result = polkit_authority_check_authorization_sync (button->priv->authority,
                                                      button->priv->subject,
                                                      button->priv->action_id,
                                                      NULL, /* PolkitDetails */
                                                      POLKIT_CHECK_AUTHORIZATION_FLAGS_NONE,
                                                      NULL, /* cancellable */
                                                      &error);
  if (error != NULL)
    {
      g_warning ("Error sync-checking authorization for action id `%s': %s",
                 button->priv->action_id,
                 error->message);
      g_error_free (error);
    }
  else
    {
      process_result (button, result);
    }

  if (result != NULL)
    g_object_unref (result);
}

static void
interactive_check_cb (GObject       *source_object,
                      GAsyncResult  *res,
                      gpointer       user_data)
{

  PolkitAuthority *authority = POLKIT_AUTHORITY (source_object);
  PolkitLockButton *button = POLKIT_LOCK_BUTTON (user_data);
  PolkitAuthorizationResult *result;
  PolkitDetails *details;
  GError *error;

  error = NULL;
  result = polkit_authority_check_authorization_finish (authority,
                                                        res,
                                                        &error);
  if (error != NULL)
    {
      g_warning ("Error obtaining authorization for action id `%s': %s",
                 button->priv->action_id,
                 error->message);
      g_error_free (error);
      goto out;
    }

  /* state is updated in the ::changed signal handler */

  /* save the temporary authorization id */
  details = polkit_authorization_result_get_details (result);
  if (details != NULL)
    {
      button->priv->tmp_authz_id = g_strdup (polkit_details_lookup (details,
                                                                    "polkit.temporary_authorization_id"));
    }

 out:
  if (result != NULL)
    g_object_unref (result);

  if (button->priv->interactive_check_cancellable != NULL)
    {
      g_object_unref (button->priv->interactive_check_cancellable);
      button->priv->interactive_check_cancellable = NULL;
    }
}


static void on_clicked(GtkButton* _button, gpointer user_data)
{

	PolkitLockButton* button = POLKIT_LOCK_BUTTON(user_data);

	if (!button->priv->authorized && button->priv->can_obtain)
	{

		/* if we already have a pending interactive check, then do nothing */
		if (button->priv->interactive_check_cancellable != NULL)
		{
			goto out;
		}

		button->priv->interactive_check_cancellable = g_cancellable_new();

		polkit_authority_check_authorization(button->priv->authority, button->priv->subject, button->priv->action_id, NULL, /* PolkitDetails */ POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION, button->priv->interactive_check_cancellable, interactive_check_cb, button);
	}
	else if (button->priv->authorized && button->priv->tmp_authz_id != NULL)
	{
		polkit_authority_revoke_temporary_authorization_by_id (button->priv->authority, button->priv->tmp_authz_id, /* cancellable */ NULL, /* callback */ NULL, /* user_data */ NULL);
	}

	out:

	update_state(button);
}

/**
 * polkit_lock_button_get_is_authorized:
 * @button: A #PolkitLockButton.
 *
 * Gets whether the process is authorized.
 *
 * Returns: %TRUE if authorized.
 */
gboolean polkit_lock_button_get_is_authorized(PolkitLockButton* button)
{
	g_return_val_if_fail(POLKIT_IS_LOCK_BUTTON(button), FALSE);
	return button->priv->authorized;
}

/**
 * polkit_lock_button_get_can_obtain:
 * @button: A #PolkitLockButton.
 *
 * Gets whether the user can obtain an authorization through
 * authentication.
 *
 * Returns: Whether the authorization is obtainable.
 */
gboolean polkit_lock_button_get_can_obtain(PolkitLockButton* button)
{
	g_return_val_if_fail(POLKIT_IS_LOCK_BUTTON(button), FALSE);
	return button->priv->can_obtain;
}

/**
 * polkit_lock_button_get_is_visible:
 * @button: A #PolkitLockButton.
 *
 * Gets whether @button is currently being shown.
 *
 * Returns: %TRUE if @button has any visible UI elements.
 */
gboolean polkit_lock_button_get_is_visible(PolkitLockButton* button)
{
	g_return_val_if_fail(POLKIT_IS_LOCK_BUTTON(button), FALSE);
	return !button->priv->hidden;
}

/**
 * polkit_lock_button_set_unlock_text:
 * @button: A #PolkitLockButton.
 * @text: The text to set.
 *
 * Makes @button display @text when not authorized and clicking the button will obtain the authorization.
 */
void polkit_lock_button_set_unlock_text(PolkitLockButton* button, const gchar* text)
{
	g_return_if_fail(POLKIT_IS_LOCK_BUTTON(button));
	g_return_if_fail(text != NULL);

	if (button->priv->text_unlock != NULL)
	{
		button->priv->text_unlock = g_strdup(text);
		update_state(button);
	}
	else
	{
		button->priv->text_unlock = g_strdup(text);
	}
}

/**
 * polkit_lock_button_set_lock_text:
 * @button: A #PolkitLockButton.
 * @text: The text to set.
 *
 * Makes @button display @text when authorized and clicking the button will revoke the authorization.
 */
void polkit_lock_button_set_lock_text(PolkitLockButton* button, const gchar* text)
{
	g_return_if_fail(POLKIT_IS_LOCK_BUTTON(button));
	g_return_if_fail(text != NULL);

	if (button->priv->text_lock != NULL)
	{
		button->priv->text_lock = g_strdup(text);
		update_state(button);
	}
	else
	{
		button->priv->text_lock = g_strdup(text);
	}
}

/**
 * polkit_lock_button_set_lock_down_text:
 * @button: A #PolkitLockButton.
 * @text: The text to set.
 *
 * Makes @button display @text when authorized and it is possible to lock down the action.
 */
void polkit_lock_button_set_lock_down_text(PolkitLockButton* button, const gchar* text)
{
	g_return_if_fail(POLKIT_IS_LOCK_BUTTON(button));
	g_return_if_fail(text != NULL);

	if (button->priv->text_lock_down != NULL)
	{
		button->priv->text_lock_down = g_strdup(text);
		update_state(button);
	}
	else
	{
		button->priv->text_lock_down = g_strdup(text);
	}
}

/**
 * polkit_lock_button_set_not_authorized_text:
 * @button: A #PolkitLockButton.
 * @text: The text to set.
 *
 * Makes @button display @text when an authorization cannot be obtained.
 */
void polkit_lock_button_set_not_authorized_text(PolkitLockButton* button, const gchar* text)
{
	g_return_if_fail(POLKIT_IS_LOCK_BUTTON(button));
	g_return_if_fail(text != NULL);

	if (button->priv->text_not_authorized != NULL)
	{
		button->priv->text_not_authorized = g_strdup(text);
		update_state(button);
	}
	else
	{
		button->priv->text_not_authorized = g_strdup(text);
	}
}


/**
 * polkit_lock_button_set_unlock_tooltip:
 * @button: A #PolkitLockButton.
 * @tooltip: The text of the tooltip.
 *
 * Makes @button display @tooltip when not authorized and clicking the button will obtain the authorization.
 */
void polkit_lock_button_set_unlock_tooltip(PolkitLockButton* button, const gchar* tooltip)
{
	g_return_if_fail(POLKIT_IS_LOCK_BUTTON(button));
	g_return_if_fail(tooltip != NULL);

	if (button->priv->tooltip_unlock != NULL)
	{
		button->priv->tooltip_unlock = g_strdup(tooltip);
		update_state(button);
	}
	else
	{
		button->priv->tooltip_unlock = g_strdup(tooltip);
	}
}

/**
 * polkit_lock_button_set_lock_tooltip:
 * @button: A #PolkitLockButton.
 * @tooltip: The text of the tooltip.
 *
 * Makes @button display @tooltip when authorized and clicking the button will revoke the authorization.
 */
void polkit_lock_button_set_lock_tooltip(PolkitLockButton* button, const gchar* tooltip)
{
	g_return_if_fail(POLKIT_IS_LOCK_BUTTON(button));
	g_return_if_fail(tooltip != NULL);

	if (button->priv->tooltip_lock != NULL)
	{
		button->priv->tooltip_lock = g_strdup(tooltip);
		update_state (button);
	}
	else
	{
		button->priv->tooltip_lock = g_strdup(tooltip);
	}
}

/**
 * polkit_lock_button_set_lock_down_tooltip:
 * @button: A #PolkitLockButton.
 * @tooltip: The text of the tooltip.
 *
 * Makes @button display @tooltip when authorized and it is possible to lock down the action.
 */
void polkit_lock_button_set_lock_down_tooltip(PolkitLockButton* button, const gchar* tooltip)
{
	g_return_if_fail(POLKIT_IS_LOCK_BUTTON(button));
	g_return_if_fail(tooltip != NULL);

	if (button->priv->tooltip_lock_down != NULL)
	{
		button->priv->tooltip_lock_down = g_strdup(tooltip);
		update_state(button);
	}
	else
	{
		button->priv->tooltip_lock_down = g_strdup(tooltip);
	}
}

/**
 * polkit_lock_button_set_not_authorized_tooltip:
 * @button: A #PolkitLockButton.
 * @tooltip: The text of the tooltip.
 *
 * Makes @button display @tooltip when an authorization cannot be obtained.
 */
void polkit_lock_button_set_not_authorized_tooltip(PolkitLockButton* button, const gchar* tooltip)
{
	g_return_if_fail(POLKIT_IS_LOCK_BUTTON(button));
	g_return_if_fail(tooltip != NULL);

	if (button->priv->tooltip_not_authorized != NULL)
	{
		button->priv->tooltip_not_authorized = g_strdup(tooltip);
		update_state(button);
	}
	else
	{
		button->priv->tooltip_not_authorized = g_strdup(tooltip);
	}
}
