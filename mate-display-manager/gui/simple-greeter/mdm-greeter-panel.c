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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#ifdef ENABLE_RBAC_SHUTDOWN
#include <auth_attr.h>
#include <secdb.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <mateconf/mateconf-client.h>
#include <dbus/dbus-glib.h>

#ifdef HAVE_UPOWER
#include <upower.h>
#endif

#include "mdm-languages.h"
#include "mdm-layouts.h"
#include "mdm-greeter-panel.h"
#include "mdm-clock-widget.h"
#include "mdm-language-option-widget.h"
#include "mdm-layout-option-widget.h"
#include "mdm-session-option-widget.h"
#include "mdm-timer.h"
#include "mdm-profile.h"
#include "mdm-common.h"

#include "na-tray.h"

#define CK_NAME              "org.freedesktop.ConsoleKit"
#define CK_MANAGER_PATH      "/org/freedesktop/ConsoleKit/Manager"
#define CK_MANAGER_INTERFACE "org.freedesktop.ConsoleKit.Manager"

#define GPM_DBUS_NAME      "org.freedesktop.PowerManagement"
#define GPM_DBUS_PATH      "/org/freedesktop/PowerManagement"
#define GPM_DBUS_INTERFACE "org.freedesktop.PowerManagement"

#define KEY_DISABLE_RESTART_BUTTONS "/apps/mdm/simple-greeter/disable_restart_buttons"
#define KEY_NOTIFICATION_AREA_PADDING "/apps/notification_area_applet/prefs/padding"

#define MDM_GREETER_PANEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_GREETER_PANEL, MdmGreeterPanelPrivate))

struct MdmGreeterPanelPrivate
{
        int                     monitor;
        GdkRectangle            geometry;
        GtkWidget              *hbox;
        GtkWidget              *alignment;
        GtkWidget              *option_hbox;
        GtkWidget              *hostname_label;
        GtkWidget              *clock;
        GtkWidget              *shutdown_button;
        GtkWidget              *shutdown_menu;
        GtkWidget              *language_option_widget;
        GtkWidget              *layout_option_widget;
        GtkWidget              *session_option_widget;

        MdmTimer               *animation_timer;
        double                  progress;

        char                   *default_session_name;
        char                   *default_language_name;

        MateConfClient            *client;
        guint                   display_is_local : 1;
};

enum {
        PROP_0,
        PROP_MONITOR,
        PROP_DISPLAY_IS_LOCAL
};

enum {
        LANGUAGE_SELECTED,
        LAYOUT_SELECTED,
        SESSION_SELECTED,
        NUMBER_OF_SIGNALS
};

static guint signals [NUMBER_OF_SIGNALS] = { 0, };

static void     mdm_greeter_panel_class_init  (MdmGreeterPanelClass *klass);
static void     mdm_greeter_panel_init        (MdmGreeterPanel      *greeter_panel);
static void     mdm_greeter_panel_finalize    (GObject              *object);

G_DEFINE_TYPE (MdmGreeterPanel, mdm_greeter_panel, GTK_TYPE_WINDOW)

static void
mdm_greeter_panel_set_monitor (MdmGreeterPanel *panel,
                               int              monitor)
{
        g_return_if_fail (MDM_IS_GREETER_PANEL (panel));

        if (panel->priv->monitor == monitor) {
                return;
        }

        panel->priv->monitor = monitor;

        gtk_widget_queue_resize (GTK_WIDGET (panel));

        g_object_notify (G_OBJECT (panel), "monitor");
}

static void
_mdm_greeter_panel_set_display_is_local (MdmGreeterPanel *panel,
                                         gboolean         is)
{
        if (panel->priv->display_is_local != is) {
                panel->priv->display_is_local = is;
                g_object_notify (G_OBJECT (panel), "display-is-local");
        }
}

static void
mdm_greeter_panel_set_property (GObject        *object,
                                guint           prop_id,
                                const GValue   *value,
                                GParamSpec     *pspec)
{
        MdmGreeterPanel *self;

        self = MDM_GREETER_PANEL (object);

        switch (prop_id) {
        case PROP_MONITOR:
                mdm_greeter_panel_set_monitor (self, g_value_get_int (value));
                break;
        case PROP_DISPLAY_IS_LOCAL:
                _mdm_greeter_panel_set_display_is_local (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_greeter_panel_get_property (GObject        *object,
                                guint           prop_id,
                                GValue         *value,
                                GParamSpec     *pspec)
{
        MdmGreeterPanel *self;

        self = MDM_GREETER_PANEL (object);

        switch (prop_id) {
        case PROP_MONITOR:
                g_value_set_int (value, self->priv->monitor);
                break;
        case PROP_DISPLAY_IS_LOCAL:
                g_value_set_boolean (value, self->priv->display_is_local);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_greeter_panel_dispose (GObject *object)
{
        G_OBJECT_CLASS (mdm_greeter_panel_parent_class)->dispose (object);
}

/* copied from panel-toplevel.c */
static void
mdm_greeter_panel_move_resize_window (MdmGreeterPanel *panel,
                                      gboolean         move,
                                      gboolean         resize)
{
        GtkWidget *widget;

        widget = GTK_WIDGET (panel);

        g_assert (gtk_widget_get_realized (widget));

        if (move && resize) {
                gdk_window_move_resize (gtk_widget_get_window (widget),
                                        panel->priv->geometry.x,
                                        panel->priv->geometry.y,
                                        panel->priv->geometry.width,
                                        panel->priv->geometry.height);
        } else if (move) {
                gdk_window_move (gtk_widget_get_window (widget),
                                 panel->priv->geometry.x,
                                 panel->priv->geometry.y);
        } else if (resize) {
                gdk_window_resize (gtk_widget_get_window (widget),
                                   panel->priv->geometry.width,
                                   panel->priv->geometry.height);
        }
}

static void
on_screen_size_changed (GdkScreen       *screen,
                        MdmGreeterPanel *panel)
{
        gtk_widget_queue_resize (GTK_WIDGET (panel));
}

static void
mdm_greeter_panel_real_realize (GtkWidget *widget)
{
        if (GTK_WIDGET_CLASS (mdm_greeter_panel_parent_class)->realize) {
                GTK_WIDGET_CLASS (mdm_greeter_panel_parent_class)->realize (widget);
        }

        gdk_window_set_geometry_hints (widget->window, NULL, GDK_HINT_POS);

        mdm_greeter_panel_move_resize_window (MDM_GREETER_PANEL (widget), TRUE, TRUE);

        g_signal_connect (gtk_window_get_screen (GTK_WINDOW (widget)),
                          "size_changed",
                          G_CALLBACK (on_screen_size_changed),
                          widget);
}

static void
mdm_greeter_panel_real_unrealize (GtkWidget *widget)
{
        g_signal_handlers_disconnect_by_func (gtk_window_get_screen (GTK_WINDOW (widget)),
                                              on_screen_size_changed,
                                              widget);

        if (GTK_WIDGET_CLASS (mdm_greeter_panel_parent_class)->unrealize) {
                GTK_WIDGET_CLASS (mdm_greeter_panel_parent_class)->unrealize (widget);
        }
}

static void
set_struts (MdmGreeterPanel *panel,
            int              x,
            int              y,
            int              width,
            int              height)
{
        gulong        data[12] = { 0, };
        int           screen_height;

        /* _NET_WM_STRUT_PARTIAL: CARDINAL[12]/32
         *
         * 0: left          1: right       2:  top             3:  bottom
         * 4: left_start_y  5: left_end_y  6:  right_start_y   7:  right_end_y
         * 8: top_start_x   9: top_end_x   10: bottom_start_x  11: bottom_end_x
         *
         * Note: In xinerama use struts relative to combined screen dimensions,
         *       not just the current monitor.
         */

        /* for bottom panel */
        screen_height = gdk_screen_get_height (gtk_window_get_screen (GTK_WINDOW (panel)));

        /* bottom */
        data[3] = screen_height - panel->priv->geometry.y - panel->priv->geometry.height + height;
        /* bottom_start_x */
        data[10] = x;
        /* bottom_end_x */
        data[11] = x + width;

#if 0
        g_debug ("Setting strut: bottom=%lu bottom_start_x=%lu bottom_end_x=%lu", data[3], data[10], data[11]);
#endif

        gdk_error_trap_push ();

        gdk_property_change (gtk_widget_get_window (GTK_WIDGET (panel)),
                             gdk_atom_intern ("_NET_WM_STRUT_PARTIAL", FALSE),
                             gdk_atom_intern ("CARDINAL", FALSE),
                             32,
                             GDK_PROP_MODE_REPLACE,
                             (guchar *) &data,
                             12);

        gdk_property_change (gtk_widget_get_window (GTK_WIDGET (panel)),
                             gdk_atom_intern ("_NET_WM_STRUT", FALSE),
                             gdk_atom_intern ("CARDINAL", FALSE),
                             32,
                             GDK_PROP_MODE_REPLACE,
                             (guchar *) &data,
                             4);

        gdk_error_trap_pop ();
}

static void
update_struts (MdmGreeterPanel *panel)
{
        GdkRectangle geometry;

        gdk_screen_get_monitor_geometry (gtk_window_get_screen (GTK_WINDOW (panel)),
                                         panel->priv->monitor,
                                         &geometry);

        /* FIXME: assumes only one panel */
        set_struts (panel,
                    panel->priv->geometry.x,
                    panel->priv->geometry.y,
                    panel->priv->geometry.width,
                    panel->priv->geometry.height);
}

static void
update_geometry (MdmGreeterPanel *panel,
                 GtkRequisition  *requisition)
{
        GdkRectangle geometry;

        gdk_screen_get_monitor_geometry (gtk_window_get_screen (GTK_WINDOW (panel)),
                                         panel->priv->monitor,
                                         &geometry);

        panel->priv->geometry.width = geometry.width;
        panel->priv->geometry.height = requisition->height + 2 * gtk_container_get_border_width (GTK_CONTAINER (panel));

        panel->priv->geometry.x = geometry.x;
        panel->priv->geometry.y = geometry.y + geometry.height - panel->priv->geometry.height + (1.0 - panel->priv->progress) * panel->priv->geometry.height;

#if 0
        g_debug ("Setting geometry x:%d y:%d w:%d h:%d",
                 panel->priv->geometry.x,
                 panel->priv->geometry.y,
                 panel->priv->geometry.width,
                 panel->priv->geometry.height);
#endif

        update_struts (panel);
}

static void
mdm_greeter_panel_real_size_request (GtkWidget      *widget,
                                     GtkRequisition *requisition)
{
        MdmGreeterPanel *panel;
        GtkBin          *bin;
        GdkRectangle     old_geometry;
        int              position_changed = FALSE;
        int              size_changed = FALSE;

        panel = MDM_GREETER_PANEL (widget);
        bin = GTK_BIN (widget);

        old_geometry = panel->priv->geometry;

        update_geometry (panel, requisition);

        requisition->width  = panel->priv->geometry.width;
        requisition->height = panel->priv->geometry.height;

        if (gtk_bin_get_child (bin) && gtk_widget_get_visible (gtk_bin_get_child (bin))) {
                gtk_widget_size_request (gtk_bin_get_child (bin), requisition);
        }

        if (! gtk_widget_get_realized (widget)) {
                return;
        }

        if (old_geometry.width  != panel->priv->geometry.width ||
            old_geometry.height != panel->priv->geometry.height) {
                size_changed = TRUE;
        }

        if (old_geometry.x != panel->priv->geometry.x ||
            old_geometry.y != panel->priv->geometry.y) {
                position_changed = TRUE;
        }

        mdm_greeter_panel_move_resize_window (panel, position_changed, size_changed);
}

static void
mdm_greeter_panel_real_show (GtkWidget *widget)
{
        MdmGreeterPanel *panel;
        GtkSettings *settings;
        gboolean     animations_are_enabled;

        settings = gtk_settings_get_for_screen (gtk_widget_get_screen (widget));
        g_object_get (settings, "gtk-enable-animations", &animations_are_enabled, NULL);

        panel = MDM_GREETER_PANEL (widget);

        if (animations_are_enabled) {
                mdm_timer_start (panel->priv->animation_timer, 1.0);
        } else {
                panel->priv->progress = 1.0;
        }

        GTK_WIDGET_CLASS (mdm_greeter_panel_parent_class)->show (widget);
}

static void
mdm_greeter_panel_real_hide (GtkWidget *widget)
{
        MdmGreeterPanel *panel;

        panel = MDM_GREETER_PANEL (widget);

        mdm_timer_stop (panel->priv->animation_timer);
        panel->priv->progress = 0.0;

        GTK_WIDGET_CLASS (mdm_greeter_panel_parent_class)->hide (widget);
}

static void
on_language_activated (MdmLanguageOptionWidget *widget,
                       MdmGreeterPanel         *panel)
{

        char *language;

        language = mdm_language_option_widget_get_current_language_name (MDM_LANGUAGE_OPTION_WIDGET (panel->priv->language_option_widget));

        if (language == NULL) {
                return;
        }

        g_signal_emit (panel, signals[LANGUAGE_SELECTED], 0, language);

        g_free (language);
}

static void
on_layout_activated (MdmLayoutOptionWidget *widget,
                     MdmGreeterPanel       *panel)
{

        char *layout;

        layout = mdm_layout_option_widget_get_current_layout_name (MDM_LAYOUT_OPTION_WIDGET (panel->priv->layout_option_widget));

        if (layout == NULL) {
                return;
        }

        g_debug ("MdmGreeterPanel: activating selected layout %s", layout);
        mdm_layout_activate (layout);

        g_signal_emit (panel, signals[LAYOUT_SELECTED], 0, layout);

        g_free (layout);
}
static void
on_session_activated (MdmSessionOptionWidget *widget,
                      MdmGreeterPanel        *panel)
{

        char *session;

        session = mdm_session_option_widget_get_current_session (MDM_SESSION_OPTION_WIDGET (panel->priv->session_option_widget));

        if (session == NULL) {
                return;
        }

        g_signal_emit (panel, signals[SESSION_SELECTED], 0, session);

        g_free (session);
}

static void
on_animation_tick (MdmGreeterPanel *panel,
                   double           progress)
{
        panel->priv->progress = progress * log ((G_E - 1.0) * progress + 1.0);

        gtk_widget_queue_resize (GTK_WIDGET (panel));
}

static gboolean
try_system_stop (DBusGConnection *connection,
                 GError         **error)
{
        DBusGProxy      *proxy;
        gboolean         res;

        g_debug ("MdmGreeterPanel: trying to stop system");

        proxy = dbus_g_proxy_new_for_name (connection,
                                           CK_NAME,
                                           CK_MANAGER_PATH,
                                           CK_MANAGER_INTERFACE);
        res = dbus_g_proxy_call_with_timeout (proxy,
                                              "Stop",
                                              INT_MAX,
                                              error,
                                              /* parameters: */
                                              G_TYPE_INVALID,
                                              /* return values: */
                                              G_TYPE_INVALID);
        return res;
}

static gboolean
try_system_restart (DBusGConnection *connection,
                    GError         **error)
{
        DBusGProxy      *proxy;
        gboolean         res;

        g_debug ("MdmGreeterPanel: trying to restart system");

        proxy = dbus_g_proxy_new_for_name (connection,
                                           CK_NAME,
                                           CK_MANAGER_PATH,
                                           CK_MANAGER_INTERFACE);
        res = dbus_g_proxy_call_with_timeout (proxy,
                                              "Restart",
                                              INT_MAX,
                                              error,
                                              /* parameters: */
                                              G_TYPE_INVALID,
                                              /* return values: */
                                              G_TYPE_INVALID);
        return res;
}

static gboolean
can_suspend (void)
{
        gboolean ret = FALSE;

#ifdef HAVE_UPOWER
        UpClient *up_client;

        /* use UPower to get data */
        up_client = up_client_new ();
	ret = up_client_get_can_suspend (up_client);
        g_object_unref (up_client);
#endif

        return ret;
}

static void
do_system_suspend (void)
{
#ifdef HAVE_UPOWER
        gboolean ret;
        UpClient *up_client;
        GError *error = NULL;

        /* use UPower to trigger suspend */
        up_client = up_client_new ();
        ret = up_client_suspend_sync (up_client, NULL, &error);
        if (!ret) {
                g_warning ("Couldn't suspend: %s", error->message);
                g_error_free (error);
                return;
        }
        g_object_unref (up_client);
#endif
}

static void
do_system_restart (void)
{
        gboolean         res;
        GError          *error;
        DBusGConnection *connection;

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (connection == NULL) {
                g_warning ("Unable to get system bus connection: %s", error->message);
                g_error_free (error);
                return;
        }

        res = try_system_restart (connection, &error);
        if (!res) {
                g_debug ("MdmGreeterPanel: unable to restart system: %s: %s",
                         dbus_g_error_get_name (error),
                         error->message);
                g_error_free (error);
        }
}

static void
do_system_stop (void)
{
        gboolean         res;
        GError          *error;
        DBusGConnection *connection;

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (connection == NULL) {
                g_warning ("Unable to get system bus connection: %s", error->message);
                g_error_free (error);
                return;
        }

        res = try_system_stop (connection, &error);
        if (!res) {
                g_debug ("MdmGreeterPanel: unable to stop system: %s: %s",
                         dbus_g_error_get_name (error),
                         error->message);
                g_error_free (error);
        }
}

static void
do_disconnect (void)
{
        gtk_main_quit ();
}

static gboolean
get_show_restart_buttons (MdmGreeterPanel *panel)
{
        gboolean     show;
        GError      *error;

        error = NULL;
        show = ! mateconf_client_get_bool (panel->priv->client, KEY_DISABLE_RESTART_BUTTONS, &error);
        if (error != NULL) {
                g_debug ("MdmGreeterPanel: unable to get disable-restart-buttons configuration: %s", error->message);
                g_error_free (error);
        }

#ifdef ENABLE_RBAC_SHUTDOWN
        {
                char *username;

                username = g_get_user_name ();
                if (username == NULL || !chkauthattr (RBAC_SHUTDOWN_KEY, username)) {
                        show = FALSE;
                        g_debug ("MdmGreeterPanel: Not showing stop/restart buttons for user %s due to RBAC key %s",
                                 username, RBAC_SHUTDOWN_KEY);
                } else {
                        g_debug ("MdmGreeterPanel: Showing stop/restart buttons for user %s due to RBAC key %s",
                                 username, RBAC_SHUTDOWN_KEY);
                }
        }
#endif
        return show;
}

static void
position_shutdown_menu (GtkMenu         *menu,
                        int             *x,
                        int             *y,
                        gboolean        *push_in,
                        MdmGreeterPanel *panel)
{
        GtkRequisition menu_requisition;
        GdkScreen *screen;
        int monitor;
        GtkAllocation shutdown_button_allocation;
        gtk_widget_get_allocation (panel->priv->shutdown_button, &shutdown_button_allocation);
        *push_in = TRUE;

        screen = gtk_widget_get_screen (GTK_WIDGET (panel));
        monitor = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (GTK_WIDGET (panel)));
        gtk_menu_set_monitor (menu, monitor);

        gtk_widget_translate_coordinates (GTK_WIDGET (panel->priv->shutdown_button),
                                          GTK_WIDGET (panel),
                                          shutdown_button_allocation.x,
                                          shutdown_button_allocation.y,
                                          x, y);

        gtk_window_get_position (GTK_WINDOW (panel), NULL, y);

        gtk_widget_size_request (GTK_WIDGET (menu), &menu_requisition);

        *y -= menu_requisition.height;
}

static void
on_shutdown_button_toggled (MdmGreeterPanel *panel)
{
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->priv->shutdown_button))) {
                gtk_menu_popup (GTK_MENU (panel->priv->shutdown_menu), NULL, NULL,
                                (GtkMenuPositionFunc) position_shutdown_menu,
                                panel, 0, 0);
        } else {
                gtk_menu_popdown (GTK_MENU (panel->priv->shutdown_menu));
        }
}

static void
on_shutdown_menu_deactivate (MdmGreeterPanel *panel)
{
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->priv->shutdown_button),
                                      FALSE);
}

static void
setup_panel (MdmGreeterPanel *panel)
{
        NaTray    *tray;
        GtkWidget *spacer;
        int        padding;

        mdm_profile_start (NULL);

        gtk_widget_set_can_focus (GTK_WIDGET (panel), TRUE);

        panel->priv->geometry.x      = -1;
        panel->priv->geometry.y      = -1;
        panel->priv->geometry.width  = -1;
        panel->priv->geometry.height = -1;

        gtk_window_set_title (GTK_WINDOW (panel), _("Panel"));
        gtk_window_set_decorated (GTK_WINDOW (panel), FALSE);

        gtk_window_set_keep_above (GTK_WINDOW (panel), TRUE);
        gtk_window_set_type_hint (GTK_WINDOW (panel), GDK_WINDOW_TYPE_HINT_DOCK);
        gtk_window_set_opacity (GTK_WINDOW (panel), 0.85);

        panel->priv->hbox = gtk_hbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (panel->priv->hbox), 0);
        gtk_widget_show (panel->priv->hbox);
        gtk_container_add (GTK_CONTAINER (panel), panel->priv->hbox);

        panel->priv->alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_box_pack_start (GTK_BOX (panel->priv->hbox), panel->priv->alignment, TRUE, TRUE, 0);
        gtk_widget_show (panel->priv->alignment);

        panel->priv->option_hbox = gtk_hbox_new (FALSE, 12);
        gtk_widget_show (panel->priv->option_hbox);
        gtk_container_add (GTK_CONTAINER (panel->priv->alignment), panel->priv->option_hbox);

        spacer = gtk_label_new ("");
        gtk_box_pack_start (GTK_BOX (panel->priv->option_hbox), spacer, TRUE, TRUE, 6);
        gtk_widget_show (spacer);

        panel->priv->client = mateconf_client_get_default ();

        mdm_profile_start ("creating option widget");
        panel->priv->language_option_widget = mdm_language_option_widget_new ();
        g_signal_connect (G_OBJECT (panel->priv->language_option_widget),
                          "language-activated",
                          G_CALLBACK (on_language_activated), panel);
        gtk_box_pack_start (GTK_BOX (panel->priv->option_hbox), panel->priv->language_option_widget, FALSE, FALSE, 6);
        mdm_profile_end ("creating option widget");

        panel->priv->layout_option_widget = mdm_layout_option_widget_new ();
        g_signal_connect (G_OBJECT (panel->priv->layout_option_widget),
                          "layout-activated",
                          G_CALLBACK (on_layout_activated), panel);
        gtk_box_pack_start (GTK_BOX (panel->priv->option_hbox), panel->priv->layout_option_widget, FALSE, FALSE, 6);

        panel->priv->session_option_widget = mdm_session_option_widget_new ();
        g_signal_connect (G_OBJECT (panel->priv->session_option_widget),
                          "session-activated",
                          G_CALLBACK (on_session_activated), panel);
        gtk_box_pack_start (GTK_BOX (panel->priv->option_hbox), panel->priv->session_option_widget, FALSE, FALSE, 6);

        spacer = gtk_label_new ("");
        gtk_box_pack_start (GTK_BOX (panel->priv->option_hbox), spacer, TRUE, TRUE, 6);
        gtk_widget_show (spacer);

        /* FIXME: we should only show hostname on panel when connected
           to a remote host */
        if (0) {
                panel->priv->hostname_label = gtk_label_new (g_get_host_name ());
                gtk_box_pack_start (GTK_BOX (panel->priv->hbox), panel->priv->hostname_label, FALSE, FALSE, 6);
                gtk_widget_show (panel->priv->hostname_label);
        }

        if (!panel->priv->display_is_local || get_show_restart_buttons (panel)) {
                GtkWidget *menu_item;
                GtkWidget *image;

                panel->priv->shutdown_button = gtk_toggle_button_new ();

                gtk_widget_set_tooltip_text (panel->priv->shutdown_button,
                                             _("Shutdown Options…"));
                gtk_button_set_relief (GTK_BUTTON (panel->priv->shutdown_button),
                                       GTK_RELIEF_NONE);

                panel->priv->shutdown_menu = gtk_menu_new ();
                gtk_menu_attach_to_widget (GTK_MENU (panel->priv->shutdown_menu),
                                           panel->priv->shutdown_button, NULL);
                g_signal_connect_swapped (panel->priv->shutdown_button, "toggled",
                                          G_CALLBACK (on_shutdown_button_toggled), panel);
                g_signal_connect_swapped (panel->priv->shutdown_menu, "deactivate",
                                          G_CALLBACK (on_shutdown_menu_deactivate), panel);

                image = gtk_image_new_from_icon_name ("system-shutdown", GTK_ICON_SIZE_BUTTON);
                gtk_widget_show (image);
                gtk_container_add (GTK_CONTAINER (panel->priv->shutdown_button), image);

                if (! panel->priv->display_is_local) {
                        menu_item = gtk_menu_item_new_with_label ("Disconnect");
                        g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (do_disconnect), NULL);
                        gtk_menu_shell_append (GTK_MENU_SHELL (panel->priv->shutdown_menu), menu_item);
                } else if (get_show_restart_buttons (panel)) {
                        if (can_suspend ()) {
                                menu_item = gtk_menu_item_new_with_label (_("Suspend"));
                                g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (do_system_suspend), NULL);
                                gtk_menu_shell_append (GTK_MENU_SHELL (panel->priv->shutdown_menu), menu_item);
                        }

                        menu_item = gtk_menu_item_new_with_label (_("Restart"));
                        g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (do_system_restart), NULL);
                        gtk_menu_shell_append (GTK_MENU_SHELL (panel->priv->shutdown_menu), menu_item);

                        menu_item = gtk_menu_item_new_with_label (_("Shut Down"));
                        g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (do_system_stop), NULL);
                        gtk_menu_shell_append (GTK_MENU_SHELL (panel->priv->shutdown_menu), menu_item);
                }

                gtk_box_pack_end (GTK_BOX (panel->priv->hbox), GTK_WIDGET (panel->priv->shutdown_button), FALSE, FALSE, 0);
                gtk_widget_show_all (panel->priv->shutdown_menu);
                gtk_widget_show_all (GTK_WIDGET (panel->priv->shutdown_button));
        }

        panel->priv->clock = mdm_clock_widget_new ();
        gtk_box_pack_end (GTK_BOX (panel->priv->hbox),
                            GTK_WIDGET (panel->priv->clock), FALSE, FALSE, 6);
        gtk_widget_show (panel->priv->clock);

        tray = na_tray_new_for_screen (gtk_window_get_screen (GTK_WINDOW (panel)),
                                       GTK_ORIENTATION_HORIZONTAL);

        padding = mateconf_client_get_int (panel->priv->client, KEY_NOTIFICATION_AREA_PADDING, NULL);
        na_tray_set_padding (tray, padding);
        gtk_box_pack_end (GTK_BOX (panel->priv->hbox), GTK_WIDGET (tray), FALSE, FALSE, 6);
        gtk_widget_show (GTK_WIDGET (tray));
        mdm_greeter_panel_hide_user_options (panel);

        panel->priv->progress = 0.0;
        panel->priv->animation_timer = mdm_timer_new ();
        g_signal_connect_swapped (panel->priv->animation_timer,
                                  "tick",
                                  G_CALLBACK (on_animation_tick),
                                  panel);

        mdm_profile_end (NULL);
}

static GObject *
mdm_greeter_panel_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        MdmGreeterPanel      *greeter_panel;

        mdm_profile_start (NULL);

        greeter_panel = MDM_GREETER_PANEL (G_OBJECT_CLASS (mdm_greeter_panel_parent_class)->constructor (type,
                                                                                                         n_construct_properties,
                                                                                                         construct_properties));

        setup_panel (greeter_panel);

        mdm_profile_end (NULL);

        return G_OBJECT (greeter_panel);
}

static void
mdm_greeter_panel_init (MdmGreeterPanel *panel)
{
        panel->priv = MDM_GREETER_PANEL_GET_PRIVATE (panel);

}

static void
mdm_greeter_panel_finalize (GObject *object)
{
        MdmGreeterPanel *greeter_panel;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_GREETER_PANEL (object));

        greeter_panel = MDM_GREETER_PANEL (object);

        g_return_if_fail (greeter_panel->priv != NULL);

        g_signal_handlers_disconnect_by_func (object, on_animation_tick, greeter_panel);
        g_object_unref (greeter_panel->priv->animation_timer);

        if (greeter_panel->priv->client != NULL) {
                g_object_unref (greeter_panel->priv->client);
        }

        G_OBJECT_CLASS (mdm_greeter_panel_parent_class)->finalize (object);
}

static void
mdm_greeter_panel_class_init (MdmGreeterPanelClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->get_property = mdm_greeter_panel_get_property;
        object_class->set_property = mdm_greeter_panel_set_property;
        object_class->constructor = mdm_greeter_panel_constructor;
        object_class->dispose = mdm_greeter_panel_dispose;
        object_class->finalize = mdm_greeter_panel_finalize;

        widget_class->realize = mdm_greeter_panel_real_realize;
        widget_class->unrealize = mdm_greeter_panel_real_unrealize;
        widget_class->size_request = mdm_greeter_panel_real_size_request;
        widget_class->show = mdm_greeter_panel_real_show;
        widget_class->hide = mdm_greeter_panel_real_hide;

        signals[LANGUAGE_SELECTED] =
                g_signal_new ("language-selected",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterPanelClass, language_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        signals[LAYOUT_SELECTED] =
                g_signal_new ("layout-selected",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterPanelClass, layout_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        signals[SESSION_SELECTED] =
                g_signal_new ("session-selected",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterPanelClass, session_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        g_object_class_install_property (object_class,
                                         PROP_MONITOR,
                                         g_param_spec_int ("monitor",
                                                           "Xinerama monitor",
                                                           "The monitor (in terms of Xinerama) which the window is on",
                                                           0,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_IS_LOCAL,
                                         g_param_spec_boolean ("display-is-local",
                                                               "display is local",
                                                               "display is local",
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        g_type_class_add_private (klass, sizeof (MdmGreeterPanelPrivate));
}

GtkWidget *
mdm_greeter_panel_new (GdkScreen *screen,
                       int        monitor,
                       gboolean   is_local)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_GREETER_PANEL,
                               "screen", screen,
                               "monitor", monitor,
                               "display-is-local", is_local,
                               NULL);

        return GTK_WIDGET (object);
}

void
mdm_greeter_panel_show_user_options (MdmGreeterPanel *panel)
{
        gtk_widget_show (panel->priv->session_option_widget);
        gtk_widget_show (panel->priv->language_option_widget);
        gtk_widget_show (panel->priv->layout_option_widget);
}

void
mdm_greeter_panel_hide_user_options (MdmGreeterPanel *panel)
{
        gtk_widget_hide (panel->priv->session_option_widget);
        gtk_widget_hide (panel->priv->language_option_widget);
        gtk_widget_hide (panel->priv->layout_option_widget);

        g_debug ("MdmGreeterPanel: activating default layout");
        mdm_layout_activate (NULL);
}

void
mdm_greeter_panel_reset (MdmGreeterPanel *panel)
{
        mdm_greeter_panel_set_keyboard_layout (panel, NULL);
        mdm_greeter_panel_set_default_language_name (panel, NULL);
        mdm_greeter_panel_set_default_session_name (panel, NULL);
        mdm_greeter_panel_hide_user_options (panel);
}

void
mdm_greeter_panel_set_default_language_name (MdmGreeterPanel *panel,
                                             const char      *language_name)
{
        char *normalized_language_name;

        g_return_if_fail (MDM_IS_GREETER_PANEL (panel));

        if (language_name != NULL) {
                normalized_language_name = mdm_normalize_language_name (language_name);
        } else {
                normalized_language_name = NULL;
        }

        if (normalized_language_name != NULL &&
            !mdm_option_widget_lookup_item (MDM_OPTION_WIDGET (panel->priv->language_option_widget),
                                            normalized_language_name, NULL, NULL, NULL)) {
                mdm_recent_option_widget_add_item (MDM_RECENT_OPTION_WIDGET (panel->priv->language_option_widget),
                                                   normalized_language_name);
        }

        mdm_option_widget_set_default_item (MDM_OPTION_WIDGET (panel->priv->language_option_widget),
                                            normalized_language_name);

        g_free (normalized_language_name);
}

void
mdm_greeter_panel_set_keyboard_layout (MdmGreeterPanel *panel,
                                       const char      *layout_name)
{
#ifdef HAVE_LIBXKLAVIER
        g_return_if_fail (MDM_IS_GREETER_PANEL (panel));

        if (layout_name != NULL &&
            !mdm_layout_is_valid (layout_name)) {
                const char *default_layout;

                default_layout = mdm_layout_get_default_layout ();

                g_debug ("MdmGreeterPanel: default layout %s is invalid, resetting to: %s",
                         layout_name, default_layout ? default_layout : "null");

                g_signal_emit (panel, signals[LAYOUT_SELECTED], 0, default_layout);

                layout_name = default_layout;
        }

        if (layout_name != NULL &&
            !mdm_option_widget_lookup_item (MDM_OPTION_WIDGET (panel->priv->layout_option_widget),
                                            layout_name, NULL, NULL, NULL)) {
                mdm_recent_option_widget_add_item (MDM_RECENT_OPTION_WIDGET (panel->priv->layout_option_widget),
                                                   layout_name);
        }

        mdm_option_widget_set_active_item (MDM_OPTION_WIDGET (panel->priv->layout_option_widget),
                                           layout_name);
        mdm_option_widget_set_default_item (MDM_OPTION_WIDGET (panel->priv->layout_option_widget),
                                            layout_name);

        g_debug ("MdmGreeterPanel: activating layout: %s", layout_name);
        mdm_layout_activate (layout_name);
#endif
}

void
mdm_greeter_panel_set_default_session_name (MdmGreeterPanel *panel,
                                            const char      *session_name)
{
        g_return_if_fail (MDM_IS_GREETER_PANEL (panel));

        if (session_name != NULL &&
            !mdm_option_widget_lookup_item (MDM_OPTION_WIDGET (panel->priv->session_option_widget),
                                            session_name, NULL, NULL, NULL)) {
                if (strcmp (session_name, MDM_CUSTOM_SESSION) == 0) {
                        mdm_option_widget_add_item (MDM_OPTION_WIDGET (panel->priv->session_option_widget),
                                                    MDM_CUSTOM_SESSION,
                                                    C_("customsession", "Custom"),
                                                    _("Custom session"),
                                                    MDM_OPTION_WIDGET_POSITION_TOP);
                } else {
                        g_warning ("Default session is not available");
                        return;
                }
        }

        mdm_option_widget_set_default_item (MDM_OPTION_WIDGET (panel->priv->session_option_widget),
                                            session_name);
}
