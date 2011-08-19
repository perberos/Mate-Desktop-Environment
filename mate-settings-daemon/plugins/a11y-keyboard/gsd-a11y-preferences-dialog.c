/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>

#include <mateconf/mateconf-client.h>

#include "gsd-a11y-preferences-dialog.h"

#define SM_DBUS_NAME      "org.mate.SessionManager"
#define SM_DBUS_PATH      "/org/mate/SessionManager"
#define SM_DBUS_INTERFACE "org.mate.SessionManager"


#define GSD_A11Y_PREFERENCES_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_A11Y_PREFERENCES_DIALOG, GsdA11yPreferencesDialogPrivate))

#define GTKBUILDER_UI_FILE "gsd-a11y-preferences-dialog.ui"

#define KEY_A11Y_DIR              "/desktop/mate/accessibility"
#define KEY_STICKY_KEYS_ENABLED   KEY_A11Y_DIR "/keyboard/stickykeys_enable"
#define KEY_BOUNCE_KEYS_ENABLED   KEY_A11Y_DIR "/keyboard/bouncekeys_enable"
#define KEY_SLOW_KEYS_ENABLED     KEY_A11Y_DIR "/keyboard/slowkeys_enable"
#define KEY_MOUSE_KEYS_ENABLED    KEY_A11Y_DIR "/keyboard/mousekeys_enable"

#define KEY_AT_DIR                "/desktop/mate/applications/at"
#define KEY_AT_SCREEN_KEYBOARD_ENABLED  KEY_AT_DIR "/screen_keyboard_enabled"
#define KEY_AT_SCREEN_MAGNIFIER_ENABLED KEY_AT_DIR "/screen_magnifier_enabled"
#define KEY_AT_SCREEN_READER_ENABLED    KEY_AT_DIR "/screen_reader_enabled"

#define FONT_RENDER_DIR        "/desktop/mate/font_rendering"
#define KEY_FONT_DPI           FONT_RENDER_DIR "/dpi"
/* X servers sometimes lie about the screen's physical dimensions, so we cannot
 * compute an accurate DPI value.  When this happens, the user gets fonts that
 * are too huge or too tiny.  So, we see what the server returns:  if it reports
 * something outside of the range [DPI_LOW_REASONABLE_VALUE,
 * DPI_HIGH_REASONABLE_VALUE], then we assume that it is lying and we use
 * DPI_FALLBACK instead.
 *
 * See get_dpi_from_mateconf_or_server() below, and also
 * https://bugzilla.novell.com/show_bug.cgi?id=217790
 */
#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500

#define DPI_FACTOR_LARGE   1.25
#define DPI_FACTOR_LARGER  1.5
#define DPI_FACTOR_LARGEST 2.0
#define DPI_DEFAULT        96

#define KEY_GTK_THEME          "/desktop/mate/interface/gtk_theme"
#define KEY_COLOR_SCHEME       "/desktop/mate/interface/gtk_color_scheme"
#define KEY_MARCO_THEME     "/apps/marco/general/theme"
#define KEY_ICON_THEME         "/desktop/mate/interface/icon_theme"

#define HIGH_CONTRAST_THEME    "HighContrast"

struct GsdA11yPreferencesDialogPrivate
{
        GtkWidget *sticky_keys_checkbutton;
        GtkWidget *slow_keys_checkbutton;
        GtkWidget *bounce_keys_checkbutton;

        GtkWidget *large_print_checkbutton;
        GtkWidget *high_contrast_checkbutton;

        GtkWidget *screen_reader_checkbutton;
        GtkWidget *screen_keyboard_checkbutton;
        GtkWidget *screen_magnifier_checkbutton;

        guint      a11y_dir_cnxn;
        guint      gsd_a11y_dir_cnxn;
};

enum {
        PROP_0,
};

static void     gsd_a11y_preferences_dialog_class_init  (GsdA11yPreferencesDialogClass *klass);
static void     gsd_a11y_preferences_dialog_init        (GsdA11yPreferencesDialog      *a11y_preferences_dialog);
static void     gsd_a11y_preferences_dialog_finalize    (GObject                       *object);

G_DEFINE_TYPE (GsdA11yPreferencesDialog, gsd_a11y_preferences_dialog, GTK_TYPE_DIALOG)

static void
gsd_a11y_preferences_dialog_set_property (GObject        *object,
                                          guint           prop_id,
                                          const GValue   *value,
                                          GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_a11y_preferences_dialog_get_property (GObject        *object,
                                          guint           prop_id,
                                          GValue         *value,
                                          GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_a11y_preferences_dialog_constructor (GType                  type,
                                         guint                  n_construct_properties,
                                         GObjectConstructParam *construct_properties)
{
        GsdA11yPreferencesDialog      *a11y_preferences_dialog;

        a11y_preferences_dialog = GSD_A11Y_PREFERENCES_DIALOG (G_OBJECT_CLASS (gsd_a11y_preferences_dialog_parent_class)->constructor (type,
                                                                                                                                       n_construct_properties,
                                                                                                                                       construct_properties));

        return G_OBJECT (a11y_preferences_dialog);
}

static void
gsd_a11y_preferences_dialog_dispose (GObject *object)
{
        G_OBJECT_CLASS (gsd_a11y_preferences_dialog_parent_class)->dispose (object);
}

static void
gsd_a11y_preferences_dialog_class_init (GsdA11yPreferencesDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_a11y_preferences_dialog_get_property;
        object_class->set_property = gsd_a11y_preferences_dialog_set_property;
        object_class->constructor = gsd_a11y_preferences_dialog_constructor;
        object_class->dispose = gsd_a11y_preferences_dialog_dispose;
        object_class->finalize = gsd_a11y_preferences_dialog_finalize;

        g_type_class_add_private (klass, sizeof (GsdA11yPreferencesDialogPrivate));
}

static void
on_response (GsdA11yPreferencesDialog *dialog,
             gint                      response_id)
{
        switch (response_id) {
        default:
                break;
        }
}

static char *
config_get_string (const char *key,
                   gboolean   *is_writable)
{
        char        *str;
        MateConfClient *client;

        client = mateconf_client_get_default ();

        if (is_writable) {
                *is_writable = mateconf_client_key_is_writable (client,
                                                             key,
                                                             NULL);
        }

        str = mateconf_client_get_string (client, key, NULL);

        g_object_unref (client);

        return str;
}

static gboolean
config_get_bool (const char *key,
                 gboolean   *is_writable)
{
        int          enabled;
        MateConfClient *client;

        client = mateconf_client_get_default ();

        if (is_writable) {
                *is_writable = mateconf_client_key_is_writable (client,
                                                             key,
                                                             NULL);
        }

        enabled = mateconf_client_get_bool (client, key, NULL);

        g_object_unref (client);

        return enabled;
}

static double
dpi_from_pixels_and_mm (int pixels,
                        int mm)
{
        double dpi;

        if (mm >= 1) {
                dpi = pixels / (mm / 25.4);
        } else {
                dpi = 0;
        }

        return dpi;
}

static double
get_dpi_from_x_server (void)
{
        GdkScreen *screen;
        double     dpi;

        screen = gdk_screen_get_default ();
        if (screen != NULL) {
                double width_dpi;
                double height_dpi;

                width_dpi = dpi_from_pixels_and_mm (gdk_screen_get_width (screen),
                                                    gdk_screen_get_width_mm (screen));
                height_dpi = dpi_from_pixels_and_mm (gdk_screen_get_height (screen),
                                                     gdk_screen_get_height_mm (screen));
                if (width_dpi < DPI_LOW_REASONABLE_VALUE
                    || width_dpi > DPI_HIGH_REASONABLE_VALUE
                    || height_dpi < DPI_LOW_REASONABLE_VALUE
                    || height_dpi > DPI_HIGH_REASONABLE_VALUE) {
                        dpi = DPI_DEFAULT;
                } else {
                        dpi = (width_dpi + height_dpi) / 2.0;
                }
        } else {
                /* Huh!?  No screen? */
                dpi = DPI_DEFAULT;
        }

        return dpi;
}

static gboolean
config_get_large_print (gboolean *is_writable)
{
        gboolean     ret;
        MateConfClient *client;
        MateConfValue  *value;
        gdouble      x_dpi;
        gdouble      u_dpi;

        client = mateconf_client_get_default ();
        value = mateconf_client_get_without_default (client, KEY_FONT_DPI, NULL);

        if (value != NULL) {
                u_dpi = mateconf_value_get_float (value);
                mateconf_value_free (value);
        } else {
                u_dpi = DPI_DEFAULT;
        }

        x_dpi = get_dpi_from_x_server ();

        g_object_unref (client);

        g_debug ("GsdA11yPreferences: got x-dpi=%f user-dpi=%f", x_dpi, u_dpi);

        ret = (((double)DPI_FACTOR_LARGE * x_dpi) < u_dpi);

        return ret;
}

static void
config_set_large_print (gboolean enabled)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();

        if (enabled) {
                gdouble x_dpi;
                gdouble u_dpi;

                x_dpi = get_dpi_from_x_server ();
                u_dpi = (double)DPI_FACTOR_LARGER * x_dpi;

                g_debug ("GsdA11yPreferences: setting x-dpi=%f user-dpi=%f", x_dpi, u_dpi);

                mateconf_client_set_float (client, KEY_FONT_DPI, u_dpi, NULL);
        } else {
                mateconf_client_unset (client, KEY_FONT_DPI, NULL);
        }

        g_object_unref (client);
}

static gboolean
config_get_high_contrast (gboolean *is_writable)
{
        gboolean ret;
        char    *gtk_theme;

        ret = FALSE;

        gtk_theme = config_get_string (KEY_GTK_THEME, is_writable);
        if (gtk_theme != NULL && strcmp (gtk_theme, HIGH_CONTRAST_THEME) == 0) {
                ret = TRUE;
        }
        g_free (gtk_theme);

        return ret;
}

static void
config_set_high_contrast (gboolean enabled)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();

        if (enabled) {
                mateconf_client_set_string (client, KEY_GTK_THEME, HIGH_CONTRAST_THEME, NULL);
                mateconf_client_set_string (client, KEY_ICON_THEME, HIGH_CONTRAST_THEME, NULL);
                /* there isn't a high contrast marco theme afaik */
        } else {
                mateconf_client_unset (client, KEY_GTK_THEME, NULL);
                mateconf_client_unset (client, KEY_ICON_THEME, NULL);
                mateconf_client_unset (client, KEY_MARCO_THEME, NULL);
        }

        g_object_unref (client);
}

static gboolean
config_get_sticky_keys (gboolean *is_writable)
{
        return config_get_bool (KEY_STICKY_KEYS_ENABLED, is_writable);
}

static void
config_set_sticky_keys (gboolean enabled)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();
        mateconf_client_set_bool (client, KEY_STICKY_KEYS_ENABLED, enabled, NULL);
        g_object_unref (client);
}

static gboolean
config_get_bounce_keys (gboolean *is_writable)
{
        return config_get_bool (KEY_BOUNCE_KEYS_ENABLED, is_writable);
}

static void
config_set_bounce_keys (gboolean enabled)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();
        mateconf_client_set_bool (client, KEY_BOUNCE_KEYS_ENABLED, enabled, NULL);
        g_object_unref (client);
}

static gboolean
config_get_slow_keys (gboolean *is_writable)
{
        return config_get_bool (KEY_SLOW_KEYS_ENABLED, is_writable);
}

static void
config_set_slow_keys (gboolean enabled)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();
        mateconf_client_set_bool (client, KEY_SLOW_KEYS_ENABLED, enabled, NULL);
        g_object_unref (client);
}

static gboolean
config_have_at_mateconf_condition (const char *condition)
{
        DBusGProxy      *sm_proxy;
        DBusGConnection *connection;
        GError          *error;
        gboolean         res;
        gboolean         is_handled;

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (connection == NULL) {
                g_warning ("Unable to connect to session bus: %s", error->message);
                return FALSE;
        }
        sm_proxy = dbus_g_proxy_new_for_name (connection,
                                              SM_DBUS_NAME,
                                              SM_DBUS_PATH,
                                              SM_DBUS_INTERFACE);
        if (sm_proxy == NULL) {
                return FALSE;
        }

        is_handled = FALSE;
        res = dbus_g_proxy_call (sm_proxy,
                                 "IsAutostartConditionHandled",
                                 &error,
                                 G_TYPE_STRING, condition,
                                 G_TYPE_INVALID,
                                 G_TYPE_BOOLEAN, &is_handled,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("Unable to call IsAutostartConditionHandled (%s): %s",
                           condition,
                           error->message);
        }

        g_object_unref (sm_proxy);

        return is_handled;
}

static gboolean
config_get_at_screen_reader (gboolean *is_writable)
{
        return config_get_bool (KEY_AT_SCREEN_READER_ENABLED, is_writable);
}

static gboolean
config_get_at_screen_keyboard (gboolean *is_writable)
{
        return config_get_bool (KEY_AT_SCREEN_KEYBOARD_ENABLED, is_writable);
}

static gboolean
config_get_at_screen_magnifier (gboolean *is_writable)
{
        return config_get_bool (KEY_AT_SCREEN_MAGNIFIER_ENABLED, is_writable);
}

static void
config_set_at_screen_reader (gboolean enabled)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();
        mateconf_client_set_bool (client, KEY_AT_SCREEN_READER_ENABLED, enabled, NULL);
        g_object_unref (client);
}

static void
config_set_at_screen_keyboard (gboolean enabled)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();
        mateconf_client_set_bool (client, KEY_AT_SCREEN_KEYBOARD_ENABLED, enabled, NULL);
        g_object_unref (client);
}

static void
config_set_at_screen_magnifier (gboolean enabled)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();
        mateconf_client_set_bool (client, KEY_AT_SCREEN_MAGNIFIER_ENABLED, enabled, NULL);
        g_object_unref (client);
}

static void
on_sticky_keys_checkbutton_toggled (GtkToggleButton          *button,
                                    GsdA11yPreferencesDialog *dialog)
{
        config_set_sticky_keys (gtk_toggle_button_get_active (button));
}

static void
on_bounce_keys_checkbutton_toggled (GtkToggleButton          *button,
                                 GsdA11yPreferencesDialog *dialog)
{
        config_set_bounce_keys (gtk_toggle_button_get_active (button));
}

static void
on_slow_keys_checkbutton_toggled (GtkToggleButton          *button,
                                  GsdA11yPreferencesDialog *dialog)
{
        config_set_slow_keys (gtk_toggle_button_get_active (button));
}

static void
on_high_contrast_checkbutton_toggled (GtkToggleButton          *button,
                                      GsdA11yPreferencesDialog *dialog)
{
        config_set_high_contrast (gtk_toggle_button_get_active (button));
}

static void
on_at_screen_reader_checkbutton_toggled (GtkToggleButton          *button,
                                         GsdA11yPreferencesDialog *dialog)
{
        config_set_at_screen_reader (gtk_toggle_button_get_active (button));
}

static void
on_at_screen_keyboard_checkbutton_toggled (GtkToggleButton          *button,
                                           GsdA11yPreferencesDialog *dialog)
{
        config_set_at_screen_keyboard (gtk_toggle_button_get_active (button));
}

static void
on_at_screen_magnifier_checkbutton_toggled (GtkToggleButton          *button,
                                            GsdA11yPreferencesDialog *dialog)
{
        config_set_at_screen_magnifier (gtk_toggle_button_get_active (button));
}

static void
on_large_print_checkbutton_toggled (GtkToggleButton          *button,
                                    GsdA11yPreferencesDialog *dialog)
{
        config_set_large_print (gtk_toggle_button_get_active (button));
}

static void
ui_set_sticky_keys (GsdA11yPreferencesDialog *dialog,
                    gboolean                  enabled)
{
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->sticky_keys_checkbutton));
        if (active != enabled) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->sticky_keys_checkbutton), enabled);
        }
}

static void
ui_set_bounce_keys (GsdA11yPreferencesDialog *dialog,
                    gboolean                  enabled)
{
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->bounce_keys_checkbutton));
        if (active != enabled) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->bounce_keys_checkbutton), enabled);
        }
}

static void
ui_set_slow_keys (GsdA11yPreferencesDialog *dialog,
                  gboolean                  enabled)
{
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->slow_keys_checkbutton));
        if (active != enabled) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->slow_keys_checkbutton), enabled);
        }
}

static void
ui_set_high_contrast (GsdA11yPreferencesDialog *dialog,
                      gboolean                  enabled)
{
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->high_contrast_checkbutton));
        if (active != enabled) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->high_contrast_checkbutton), enabled);
        }
}

static void
ui_set_at_screen_reader (GsdA11yPreferencesDialog *dialog,
                         gboolean                  enabled)
{
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->screen_reader_checkbutton));
        if (active != enabled) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->screen_reader_checkbutton), enabled);
        }
}

static void
ui_set_at_screen_keyboard (GsdA11yPreferencesDialog *dialog,
                           gboolean                  enabled)
{
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->screen_keyboard_checkbutton));
        if (active != enabled) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->screen_keyboard_checkbutton), enabled);
        }
}

static void
ui_set_at_screen_magnifier (GsdA11yPreferencesDialog *dialog,
                            gboolean                  enabled)
{
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->screen_magnifier_checkbutton));
        if (active != enabled) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->screen_magnifier_checkbutton), enabled);
        }
}

static void
ui_set_large_print (GsdA11yPreferencesDialog *dialog,
                    gboolean                  enabled)
{
        gboolean active;

        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->large_print_checkbutton));
        if (active != enabled) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->large_print_checkbutton), enabled);
        }
}

static void
key_changed_cb (MateConfClient              *client,
                guint                     cnxn_id,
                MateConfEntry               *entry,
                GsdA11yPreferencesDialog *dialog)
{
        const char *key;
        MateConfValue *value;

        key = mateconf_entry_get_key (entry);
        value = mateconf_entry_get_value (entry);

        if (strcmp (key, KEY_STICKY_KEYS_ENABLED) == 0) {
                if (value->type == MATECONF_VALUE_BOOL) {
                        gboolean enabled;

                        enabled = mateconf_value_get_bool (value);
                        ui_set_sticky_keys (dialog, enabled);
                } else {
                        g_warning ("Error retrieving configuration key '%s': Invalid type",
                                   key);
                }
        } else if (strcmp (key, KEY_BOUNCE_KEYS_ENABLED) == 0) {
                if (value->type == MATECONF_VALUE_BOOL) {
                        gboolean enabled;

                        enabled = mateconf_value_get_bool (value);
                        ui_set_bounce_keys (dialog, enabled);
                } else {
                        g_warning ("Error retrieving configuration key '%s': Invalid type",
                                   key);
                }
        } else if (strcmp (key, KEY_SLOW_KEYS_ENABLED) == 0) {
                if (value->type == MATECONF_VALUE_BOOL) {
                        gboolean enabled;

                        enabled = mateconf_value_get_bool (value);
                        ui_set_slow_keys (dialog, enabled);
                } else {
                        g_warning ("Error retrieving configuration key '%s': Invalid type",
                                   key);
                }
        } else if (strcmp (key, KEY_AT_SCREEN_READER_ENABLED) == 0) {
                if (value->type == MATECONF_VALUE_BOOL) {
                        gboolean enabled;

                        enabled = mateconf_value_get_bool (value);
                        ui_set_at_screen_reader (dialog, enabled);
                } else {
                        g_warning ("Error retrieving configuration key '%s': Invalid type",
                                   key);
                }
        } else if (strcmp (key, KEY_AT_SCREEN_KEYBOARD_ENABLED) == 0) {
                if (value->type == MATECONF_VALUE_BOOL) {
                        gboolean enabled;

                        enabled = mateconf_value_get_bool (value);
                        ui_set_at_screen_keyboard (dialog, enabled);
                } else {
                        g_warning ("Error retrieving configuration key '%s': Invalid type",
                                   key);
                }
        } else if (strcmp (key, KEY_AT_SCREEN_MAGNIFIER_ENABLED) == 0) {
                if (value->type == MATECONF_VALUE_BOOL) {
                        gboolean enabled;

                        enabled = mateconf_value_get_bool (value);
                        ui_set_at_screen_magnifier (dialog, enabled);
                } else {
                        g_warning ("Error retrieving configuration key '%s': Invalid type",
                                   key);
                }
        } else {
                g_debug ("Config key not handled: %s", key);
        }
}

static void
setup_dialog (GsdA11yPreferencesDialog *dialog,
              GtkBuilder               *builder)
{
        GtkWidget   *widget;
        gboolean     enabled;
        gboolean     is_writable;
        MateConfClient *client;

        widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "sticky_keys_checkbutton"));
        dialog->priv->sticky_keys_checkbutton = widget;
        g_signal_connect (widget,
                          "toggled",
                          G_CALLBACK (on_sticky_keys_checkbutton_toggled),
                          NULL);
        enabled = config_get_sticky_keys (&is_writable);
        ui_set_sticky_keys (dialog, enabled);
        if (! is_writable) {
                gtk_widget_set_sensitive (widget, FALSE);
        }

        widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "bounce_keys_checkbutton"));
        dialog->priv->bounce_keys_checkbutton = widget;
        g_signal_connect (widget,
                          "toggled",
                          G_CALLBACK (on_bounce_keys_checkbutton_toggled),
                          NULL);
        enabled = config_get_bounce_keys (&is_writable);
        ui_set_bounce_keys (dialog, enabled);
        if (! is_writable) {
                gtk_widget_set_sensitive (widget, FALSE);
        }

        widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "slow_keys_checkbutton"));
        dialog->priv->slow_keys_checkbutton = widget;
        g_signal_connect (widget,
                          "toggled",
                          G_CALLBACK (on_slow_keys_checkbutton_toggled),
                          NULL);
        enabled = config_get_slow_keys (&is_writable);
        ui_set_slow_keys (dialog, enabled);
        if (! is_writable) {
                gtk_widget_set_sensitive (widget, FALSE);
        }

        widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "high_contrast_checkbutton"));
        dialog->priv->high_contrast_checkbutton = widget;
        g_signal_connect (widget,
                          "toggled",
                          G_CALLBACK (on_high_contrast_checkbutton_toggled),
                          NULL);
        enabled = config_get_high_contrast (&is_writable);
        ui_set_high_contrast (dialog, enabled);
        if (! is_writable) {
                gtk_widget_set_sensitive (widget, FALSE);
        }

        widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "at_screen_keyboard_checkbutton"));
        dialog->priv->screen_keyboard_checkbutton = widget;
        g_signal_connect (widget,
                          "toggled",
                          G_CALLBACK (on_at_screen_keyboard_checkbutton_toggled),
                          NULL);
        enabled = config_get_at_screen_keyboard (&is_writable);
        ui_set_at_screen_keyboard (dialog, enabled);
        if (! is_writable) {
                gtk_widget_set_sensitive (widget, FALSE);
        }
        gtk_widget_set_no_show_all (widget, TRUE);
        if (config_have_at_mateconf_condition ("MATE " KEY_AT_SCREEN_KEYBOARD_ENABLED)) {
                gtk_widget_show_all (widget);
        } else {
                gtk_widget_hide (widget);
        }

        widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "at_screen_reader_checkbutton"));
        dialog->priv->screen_reader_checkbutton = widget;
        g_signal_connect (widget,
                          "toggled",
                          G_CALLBACK (on_at_screen_reader_checkbutton_toggled),
                          NULL);
        enabled = config_get_at_screen_reader (&is_writable);
        ui_set_at_screen_reader (dialog, enabled);
        if (! is_writable) {
                gtk_widget_set_sensitive (widget, FALSE);
        }
        gtk_widget_set_no_show_all (widget, TRUE);
        if (config_have_at_mateconf_condition ("MATE " KEY_AT_SCREEN_READER_ENABLED)) {
                gtk_widget_show_all (widget);
        } else {
                gtk_widget_hide (widget);
        }

        widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "at_screen_magnifier_checkbutton"));
        dialog->priv->screen_magnifier_checkbutton = widget;
        g_signal_connect (widget,
                          "toggled",
                          G_CALLBACK (on_at_screen_magnifier_checkbutton_toggled),
                          NULL);
        enabled = config_get_at_screen_magnifier (&is_writable);
        ui_set_at_screen_magnifier (dialog, enabled);
        if (! is_writable) {
                gtk_widget_set_sensitive (widget, FALSE);
        }
        gtk_widget_set_no_show_all (widget, TRUE);
        if (config_have_at_mateconf_condition ("MATE " KEY_AT_SCREEN_MAGNIFIER_ENABLED)) {
                gtk_widget_show_all (widget);
        } else {
                gtk_widget_hide (widget);
        }

        widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "large_print_checkbutton"));
        dialog->priv->large_print_checkbutton = widget;
        g_signal_connect (widget,
                          "toggled",
                          G_CALLBACK (on_large_print_checkbutton_toggled),
                          NULL);
        enabled = config_get_large_print (&is_writable);
        ui_set_large_print (dialog, enabled);
        if (! is_writable) {
                gtk_widget_set_sensitive (widget, FALSE);
        }


        client = mateconf_client_get_default ();
        mateconf_client_add_dir (client,
                              KEY_A11Y_DIR,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        dialog->priv->a11y_dir_cnxn = mateconf_client_notify_add (client,
                                                               KEY_A11Y_DIR,
                                                               (MateConfClientNotifyFunc)key_changed_cb,
                                                               dialog,
                                                               NULL,
                                                               NULL);

        mateconf_client_add_dir (client,
                              KEY_AT_DIR,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        dialog->priv->gsd_a11y_dir_cnxn = mateconf_client_notify_add (client,
                                                                   KEY_AT_DIR,
                                                                   (MateConfClientNotifyFunc)key_changed_cb,
                                                                   dialog,
                                                                   NULL,
                                                                   NULL);

        g_object_unref (client);
}

static void
gsd_a11y_preferences_dialog_init (GsdA11yPreferencesDialog *dialog)
{
        static const gchar *ui_file_path = GTKBUILDERDIR "/" GTKBUILDER_UI_FILE;
        gchar *objects[] = {"main_box", NULL};
        GError *error = NULL;
        GtkBuilder  *builder;

        dialog->priv = GSD_A11Y_PREFERENCES_DIALOG_GET_PRIVATE (dialog);

        builder = gtk_builder_new ();
        gtk_builder_set_translation_domain (builder, PACKAGE);
        if (gtk_builder_add_objects_from_file (builder, ui_file_path, objects,
                                               &error) == 0) {
                g_warning ("Could not load A11Y-UI: %s", error->message);
                g_error_free (error);
        } else {
                GtkWidget *widget;

                widget = GTK_WIDGET (gtk_builder_get_object (builder,
                                                             "main_box"));
                gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                                   widget);
                gtk_container_set_border_width (GTK_CONTAINER (widget), 12);
                setup_dialog (dialog, builder);
       }

        g_object_unref (builder);

        gtk_container_set_border_width (GTK_CONTAINER (dialog), 12);
        gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
        gtk_window_set_title (GTK_WINDOW (dialog), _("Universal Access Preferences"));
        gtk_window_set_icon_name (GTK_WINDOW (dialog), "preferences-desktop-accessibility");
        g_object_set (dialog,
                      "allow-shrink", FALSE,
                      "allow-grow", FALSE,
                      NULL);

        gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                NULL);
        g_signal_connect (dialog,
                          "response",
                          G_CALLBACK (on_response),
                          dialog);


        gtk_widget_show_all (GTK_WIDGET (dialog));
}

static void
gsd_a11y_preferences_dialog_finalize (GObject *object)
{
        GsdA11yPreferencesDialog *dialog;
        MateConfClient              *client;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_A11Y_PREFERENCES_DIALOG (object));

        dialog = GSD_A11Y_PREFERENCES_DIALOG (object);

        g_return_if_fail (dialog->priv != NULL);

        client = mateconf_client_get_default ();

        if (dialog->priv->a11y_dir_cnxn > 0) {
                mateconf_client_notify_remove (client, dialog->priv->a11y_dir_cnxn);
        }
        if (dialog->priv->gsd_a11y_dir_cnxn > 0) {
                mateconf_client_notify_remove (client, dialog->priv->gsd_a11y_dir_cnxn);
        }

        g_object_unref (client);

        G_OBJECT_CLASS (gsd_a11y_preferences_dialog_parent_class)->finalize (object);
}

GtkWidget *
gsd_a11y_preferences_dialog_new (void)
{
        GObject *object;

        object = g_object_new (GSD_TYPE_A11Y_PREFERENCES_DIALOG,
                               NULL);

        return GTK_WIDGET (object);
}
