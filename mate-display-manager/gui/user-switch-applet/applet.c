/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004-2005 James M. Cape <jcape@ignore-your.tv>.
 * Copyright (C) 2008      Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#include <dbus/dbus-glib.h>

#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-ui-util.h>

#include <mate-panel-applet.h>
#include <mate-panel-applet-mateconf.h>

#include "mdm-user-manager.h"
#include "mdm-entry-menu-item.h"
#include "mdm-settings-client.h"

#define LOCKDOWN_DIR    "/desktop/mate/lockdown"
#define LOCKDOWN_USER_SWITCHING_KEY LOCKDOWN_DIR "/disable_user_switching"
#define LOCKDOWN_LOCK_SCREEN_KEY    LOCKDOWN_DIR "/disable_lock_screen"
#define LOCKDOWN_COMMAND_LINE_KEY   LOCKDOWN_DIR "/disable_command_line"

typedef enum {
        GSM_PRESENCE_STATUS_AVAILABLE = 0,
        GSM_PRESENCE_STATUS_INVISIBLE,
        GSM_PRESENCE_STATUS_BUSY,
        GSM_PRESENCE_STATUS_IDLE,
} GsmPresenceStatus;

typedef struct _MdmAppletData
{
        MatePanelApplet    *applet;

        MateConfClient    *client;
        MdmUserManager *manager;
        MdmUser        *user;

        GtkWidget      *menubar;
        GtkWidget      *menuitem;
        GtkWidget      *menu;
#ifdef BUILD_PRESENSE_STUFF
        GtkWidget      *user_item;
#endif
        GtkWidget      *control_panel_item;
        GtkWidget      *account_item;
        GtkWidget      *lock_screen_item;
        GtkWidget      *login_screen_item;
        GtkWidget      *quit_session_item;

        guint           client_notify_lockdown_id;

        guint           current_status;
        guint           user_loaded_notify_id;
        guint           user_changed_notify_id;
        gint8           pixel_size;
        gint            panel_size;
        GtkIconSize     icon_size;
#ifdef BUILD_PRESENSE_STUFF
        DBusGProxy     *presence_proxy;
#endif
} MdmAppletData;

typedef struct _SelectorResponseData
{
        MdmAppletData  *adata;
        GtkRadioButton *radio;
} SelectorResponseData;

static void reset_icon   (MdmAppletData *adata);
static void update_label (MdmAppletData *adata);

static gboolean applet_factory (MatePanelApplet   *applet,
                                const char    *iid,
                                gpointer       data);

MATE_PANEL_APPLET_MATECOMPONENT_FACTORY ("OAFIID:MATE_FastUserSwitchApplet_Factory",
                             PANEL_TYPE_APPLET,
                             "mdm-user-switch-applet", "0",
                             (MatePanelAppletFactoryCallback)applet_factory,
                             NULL)

static void
about_me_cb (MateComponentUIComponent *ui_container,
             gpointer           data,
             const char        *cname)
{
        GError *err;

        err = NULL;
        if (! g_spawn_command_line_async ("mate-about-me", &err)) {
                g_critical ("Could not run `mate-about-me': %s",
                            err->message);
                g_error_free (err);
                matecomponent_ui_component_set_prop (ui_container,
                                              "/commands/MdmAboutMe",
                                              "hidden", "1",
                                              NULL);
        }
}

/*
 * mate-panel/applets/wncklet/window-menu.c:window_filter_button_press()
 *
 * Copyright (C) 2005 James M. Cape.
 * Copyright (C) 2003 Sun Microsystems, Inc.
 * Copyright (C) 2001 Free Software Foundation, Inc.
 * Copyright (C) 2000 Helix Code, Inc.
 */
static gboolean
menubar_button_press_event_cb (GtkWidget      *menubar,
                               GdkEventButton *event,
                               MdmAppletData  *adata)
{
        if (event->button != 1) {
                g_signal_stop_emission_by_name (menubar, "button-press-event");
                /* Reset the login window item */
        }

        return FALSE;
}

static void
about_cb (MateComponentUIComponent *ui_container,
          gpointer           data,
          const char        *cname)
{
        static const char *authors[] = {
                "James M. Cape <jcape@ignore-your.tv>",
                "Thomas Thurman <thomas@thurman.org.uk>",
                "William Jon McCann <jmccann@redhat.com>",
                NULL
        };
        static char *license[] = {
                N_("The User Switch Applet is free software; you can redistribute it and/or modify "
                   "it under the terms of the GNU General Public License as published by "
                   "the Free Software Foundation; either version 2 of the License, or "
                   "(at your option) any later version."),
                N_("This program is distributed in the hope that it will be useful, "
                   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
                   "GNU General Public License for more details."),
                N_("You should have received a copy of the GNU General Public License "
                   "along with this program; if not, write to the Free Software "
                   "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA "),
                NULL
        };
        char *license_i18n;

        license_i18n = g_strconcat (_(license[0]), "\n\n", _(license[1]), "\n\n", _(license[2]), NULL);

        gtk_show_about_dialog (NULL,
                               "version", VERSION,
                               "copyright", "Copyright \xc2\xa9 2004-2005 James M. Cape.\n"
                               "Copyright \xc2\xa9 2006 Thomas Thurman.\n"
                               "Copyright \xc2\xa9 2008 Red Hat, Inc.",
                               "comments", _("A menu to quickly switch between users."),
                               "authors", authors,
                               "license", license_i18n,
                               "wrap-license", TRUE,
                               "translator-credits", _("translator-credits"),
                               "logo-icon-name", "stock_people",
                               NULL);

        g_free (license_i18n);
}


static void
admin_cb (MateComponentUIComponent *ui_container,
          gpointer           data,
          const char        *cname)
{
#ifdef USERS_ADMIN
        char   **args;
        gboolean res;
        GError  *err;

        err = NULL;
        if (!g_shell_parse_argv (USERS_ADMIN, NULL, &args, &err)) {
                g_critical ("Could not parse users and groups management command line `%s': %s",
                            USERS_ADMIN, err->message);
                return;
        }

        res = g_spawn_async (g_get_home_dir (),
                             args,
                             NULL,
                             (G_SPAWN_STDOUT_TO_DEV_NULL |
                              G_SPAWN_STDERR_TO_DEV_NULL |
                              G_SPAWN_SEARCH_PATH),
                             NULL,
                             NULL,
                             NULL,
                             &err);
        if (! res) {
                g_critical ("Could not run `%s' to manage users and groups: %s",
                            USERS_ADMIN, err->message);
                g_error_free (err);
        }
        g_strfreev (args);
#endif /* USERS_ADMIN */
}

static void
set_menuitem_icon (MateComponentUIComponent *component,
                   const char        *item_path,
                   GtkIconTheme      *theme,
                   const char        *icon_name,
                   gint               icon_size)
{
        GdkPixbuf *pixbuf;
        int        width;
        int        height;

        pixbuf = gtk_icon_theme_load_icon (theme, icon_name, icon_size, 0, NULL);
        if (pixbuf == NULL) {
                return;
        }

        width = gdk_pixbuf_get_width (pixbuf);
        height = gdk_pixbuf_get_height (pixbuf);
        if (width > icon_size + 4 || height > icon_size + 4) {
                GdkPixbuf *tmp;
                if (height > width) {
                        width *= (gdouble) icon_size / (gdouble) height;
                        height = icon_size;
                } else {
                        height *= (gdouble) icon_size / (gdouble) width;
                        width = icon_size;
                }
                tmp = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
                g_object_unref (pixbuf);
                pixbuf = tmp;
        }

        matecomponent_ui_util_set_pixbuf (component, item_path, pixbuf, NULL);
        g_object_unref (pixbuf);
}

static void
applet_style_set_cb (GtkWidget *widget,
                     GtkStyle  *old_style,
                     gpointer   data)
{
        MateComponentUIComponent *component;
        GdkScreen         *screen;
        GtkIconTheme      *theme;
        int                width;
        int                height;
        int                icon_size;

        if (gtk_widget_has_screen (widget)) {
                screen = gtk_widget_get_screen (widget);
        } else {
                screen = gdk_screen_get_default ();
        }

        if (gtk_icon_size_lookup_for_settings (gtk_settings_get_for_screen (screen),
                                               GTK_ICON_SIZE_MENU, &width, &height)) {
                icon_size = MAX (width, height);
        } else {
                icon_size = 16;
        }

        theme = gtk_icon_theme_get_for_screen (screen);
        component = mate_panel_applet_get_popup_component (MATE_PANEL_APPLET (widget));

        set_menuitem_icon (component,
                           "/commands/MdmAboutMe",
                           theme,
                           "user-info",
                           icon_size);
        set_menuitem_icon (component,
                           "/commands/MdmUsersGroupsAdmin",
                           theme,
                           "stock_people",
                           icon_size);
}

static void
applet_change_background_cb (MatePanelApplet               *applet,
                             MatePanelAppletBackgroundType  type,
                             GdkColor                  *color,
                             GdkPixmap                 *pixmap,
                             MdmAppletData             *adata)
{
        GtkRcStyle *rc_style;
        GtkStyle   *style;

        gtk_widget_set_style (adata->menubar, NULL);
        rc_style = gtk_rc_style_new ();
        gtk_widget_modify_style (GTK_WIDGET (adata->menubar), rc_style);
        g_object_unref (rc_style);

        switch (type) {
        case PANEL_NO_BACKGROUND:
                break;
        case PANEL_COLOR_BACKGROUND:
                gtk_widget_modify_bg (adata->menubar, GTK_STATE_NORMAL, color);
                break;
        case PANEL_PIXMAP_BACKGROUND:
                style = gtk_style_copy (gtk_widget_get_style (adata->menubar));
                if (style->bg_pixmap[GTK_STATE_NORMAL]) {
                        g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);
                }

                style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
                gtk_widget_set_style (adata->menubar, style);
                g_object_unref (style);
                break;
        }
}

/*
 * mate-panel/applets/wncklet/window-menu.c:window_menu_key_press_event()
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
 * Copyright (C) 2001 Free Software Foundation, Inc.
 * Copyright (C) 2000 Helix Code, Inc.
 */
static gboolean
applet_key_press_event_cb (GtkWidget     *widget,
                           GdkEventKey   *event,
                           MdmAppletData *adata)
{
        GtkMenuShell *menu_shell;

        switch (event->keyval) {
        case GDK_KP_Enter:
        case GDK_ISO_Enter:
        case GDK_3270_Enter:
        case GDK_Return:
        case GDK_space:
        case GDK_KP_Space:
                menu_shell = GTK_MENU_SHELL (adata->menubar);
                /*
                 * We need to call _gtk_menu_shell_activate() here as is done in
                 * window_key_press_handler in gtkmenubar.c which pops up menu
                 * when F10 is pressed.
                 *
                 * As that function is private its code is replicated here.
                 */
                if (!menu_shell->active) {
                        gtk_grab_add (GTK_WIDGET (menu_shell));
                        menu_shell->have_grab = TRUE;
                        menu_shell->active = TRUE;
                }

                gtk_menu_shell_select_first (menu_shell, FALSE);
                return TRUE;
        default:
                break;
        }

        return FALSE;
}

static void
set_item_text_angle_and_alignment (GtkWidget *item,
                                   double     text_angle,
                                   float      xalign,
                                   float      yalign)
{
        GtkWidget *label;

        label = gtk_bin_get_child (GTK_BIN (item));

        gtk_label_set_angle (GTK_LABEL (label), text_angle);

        gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
}

/*
 * mate-panel/applets/wncklet/window-menu.c:window_menu_size_allocate()
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
 * Copyright (C) 2001 Free Software Foundation, Inc.
 * Copyright (C) 2000 Helix Code, Inc.
 */
static void
applet_size_allocate_cb (GtkWidget     *widget,
                         GtkAllocation *allocation,
                         MdmAppletData *adata)
{
        GList            *children;
        GtkWidget        *top_item;
        MatePanelAppletOrient orient;
        gint              pixel_size;
        gdouble           text_angle;
        GtkPackDirection  pack_direction;
        float             text_xalign;
        float             text_yalign;

        pack_direction = GTK_PACK_DIRECTION_LTR;
        text_angle = 0.0;
        text_xalign = 0.0;
        text_yalign = 0.5;

        children = gtk_container_get_children (GTK_CONTAINER (adata->menubar));
        top_item = GTK_WIDGET (children->data);
        g_list_free (children);

        orient = mate_panel_applet_get_orient (MATE_PANEL_APPLET (widget));

        switch (orient) {
        case MATE_PANEL_APPLET_ORIENT_UP:
        case MATE_PANEL_APPLET_ORIENT_DOWN:
                gtk_widget_set_size_request (top_item, -1, allocation->height);
                pixel_size = allocation->height - gtk_widget_get_style (top_item)->ythickness * 2;
                break;
        case MATE_PANEL_APPLET_ORIENT_LEFT:
                gtk_widget_set_size_request (top_item, allocation->width, -1);
                pixel_size = allocation->width - gtk_widget_get_style (top_item)->xthickness * 2;
                pack_direction = GTK_PACK_DIRECTION_TTB;
                text_angle = 270.0;
                text_xalign = 0.5;
                text_yalign = 0.0;
                break;
        case MATE_PANEL_APPLET_ORIENT_RIGHT:
                gtk_widget_set_size_request (top_item, allocation->width, -1);
                pixel_size = allocation->width - gtk_widget_get_style (top_item)->xthickness * 2;
                pack_direction = GTK_PACK_DIRECTION_BTT;
                text_angle = 90.0;
                text_xalign = 0.5;
                text_yalign = 0.0;
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        gtk_menu_bar_set_pack_direction (GTK_MENU_BAR (adata->menubar),
                                         pack_direction);
        gtk_menu_bar_set_child_pack_direction (GTK_MENU_BAR (adata->menubar),
                                               pack_direction);

        set_item_text_angle_and_alignment (adata->menuitem,
                                           text_angle,
                                           text_xalign,
                                           text_yalign);

        if (adata->panel_size != pixel_size) {
                adata->panel_size = pixel_size;
                reset_icon (adata);
        }
}


static void
mdm_applet_data_free (MdmAppletData *adata)
{
        mateconf_client_notify_remove (adata->client, adata->client_notify_lockdown_id);

        if (adata->user_loaded_notify_id != 0) {
                g_signal_handler_disconnect (adata->user, adata->user_loaded_notify_id);
        }

        if (adata->user_changed_notify_id != 0) {
                g_signal_handler_disconnect (adata->user, adata->user_changed_notify_id);
        }

#ifdef BUILD_PRESENSE_STUFF
        if (adata->presence_proxy != NULL) {
                g_object_unref (adata->presence_proxy);
        }
#endif

        if (adata->user != NULL) {
                g_object_unref (adata->user);
        }
        g_object_unref (adata->client);
        g_object_unref (adata->manager);

        g_free (adata);
}


/*
 * mate-panel/applets/wncklet/window-menu.c:window_menu_on_expose()
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
 * Copyright (C) 2001 Free Software Foundation, Inc.
 * Copyright (C) 2000 Helix Code, Inc.
 */
static gboolean
menubar_expose_event_cb (GtkWidget      *widget,
                         GdkEventExpose *event,
                         MdmAppletData  *adata)
{
        if (gtk_widget_has_focus (GTK_WIDGET (adata->applet)))
                gtk_paint_focus (gtk_widget_get_style (widget),
                                 gtk_widget_get_window (widget),
                                 gtk_widget_get_state (widget),
                                 NULL, widget, "menu-applet", 0, 0, -1, -1);

        return FALSE;
}

static void
menu_style_set_cb (GtkWidget     *menu,
                   GtkStyle      *old_style,
                   MdmAppletData *adata)
{
        GtkSettings *settings;
        int          width;
        int          height;

        adata->icon_size = gtk_icon_size_from_name ("panel-menu");

        if (adata->icon_size == GTK_ICON_SIZE_INVALID) {
                adata->icon_size = gtk_icon_size_register ("panel-menu", 24, 24);
        }

        if (gtk_widget_has_screen (menu)) {
                settings = gtk_settings_get_for_screen (gtk_widget_get_screen (menu));
        } else {
                settings = gtk_settings_get_default ();
        }

        if (!gtk_icon_size_lookup_for_settings (settings, adata->icon_size,
                                                &width, &height)) {
                adata->pixel_size = -1;
        } else {
                adata->pixel_size = MAX (width, height);
        }
}

static void
menuitem_style_set_cb (GtkWidget     *menuitem,
                       GtkStyle      *old_style,
                       MdmAppletData *adata)
{
        GtkWidget *image;

        if (MDM_IS_ENTRY_MENU_ITEM (menuitem)) {
        } else {
                const char *icon_name;

                if (menuitem == adata->login_screen_item) {
                        icon_name = "system-users";
                } else if (menuitem == adata->lock_screen_item) {
                        icon_name = "system-lock-screen";
                } else if (menuitem == adata->quit_session_item) {
                        icon_name = "system-log-out";
                } else if (menuitem == adata->account_item) {
                        icon_name = "user-info";
                } else if (menuitem == adata->control_panel_item) {
                        icon_name = "preferences-desktop";
                } else {
                        icon_name = GTK_STOCK_MISSING_IMAGE;
                }

                image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (menuitem));
                gtk_image_set_pixel_size (GTK_IMAGE (image), adata->pixel_size);
                gtk_image_set_from_icon_name (GTK_IMAGE (image), icon_name,
                                              adata->icon_size);
        }
}

static void
on_user_changed (MdmUser         *user,
                 MdmAppletData   *adata)
{
        g_debug ("user changed");
        update_label (adata);
        reset_icon (adata);
}

/* Called every time the menu is displayed (and also for some reason
 * immediately it's created, which does no harm). All we have to do
 * here is kick off a request to MDM to let us know which users are
 * logged in, so we can display check marks next to their names.
 */
static gboolean
menu_expose_cb (GtkWidget *menu,
                gpointer   data)
{
        char *program;
        MdmAppletData *adata = data;

        program = g_find_program_in_path ("mate-control-center");
        if (program != NULL) {
                gtk_widget_show (adata->control_panel_item);
        } else {
                gtk_widget_hide (adata->control_panel_item);
        }
        g_free (program);
        return FALSE;
}

static void
maybe_lock_screen (MdmAppletData *adata)
{
        char      *args[3];
        GError    *err;
        GdkScreen *screen;
        gboolean   use_gscreensaver = TRUE;
        gboolean   res;

        g_debug ("Attempting to lock screen");

        args[0] = g_find_program_in_path ("mate-screensaver-command");
        if (args[0] == NULL) {
                args[0] = g_find_program_in_path ("xscreensaver-command");
                use_gscreensaver = FALSE;
        }

        if (args[0] == NULL) {
                return;
        }

        if (use_gscreensaver) {
                args[1] = "--lock";
        } else {
                args[1] = "-lock";
        }
        args[2] = NULL;

        if (gtk_widget_has_screen (GTK_WIDGET (adata->applet))) {
                screen = gtk_widget_get_screen (GTK_WIDGET (adata->applet));
        } else {
                screen = gdk_screen_get_default ();
        }

        err = NULL;
        res = gdk_spawn_on_screen (screen,
                                   g_get_home_dir (),
                                   args,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &err);
        if (! res) {
                g_warning (_("Can't lock screen: %s"), err->message);
                g_error_free (err);
        }

        if (use_gscreensaver) {
                args[1] = "--throttle";
        } else {
                args[1] = "-throttle";
        }

        err = NULL;
        res = gdk_spawn_on_screen (screen,
                                   g_get_home_dir (),
                                   args,
                                   NULL,
                                   (G_SPAWN_STDERR_TO_DEV_NULL
                                   | G_SPAWN_STDOUT_TO_DEV_NULL),
                                   NULL,
                                   NULL,
                                   NULL,
                                   &err);
        if (! res) {
                g_warning (_("Can't temporarily set screensaver to blank screen: %s"),
                           err->message);
                g_error_free (err);
        }

        g_free (args[0]);
}

static void
do_switch (MdmAppletData *adata,
           MdmUser       *user)
{
        guint num_sessions;

        g_debug ("Do user switch");

        if (user == NULL) {
                mdm_user_manager_goto_login_session (adata->manager);
                goto out;
        }

        num_sessions = mdm_user_get_num_sessions (user);
        if (num_sessions > 0) {
                mdm_user_manager_activate_user_session (adata->manager, user);
        } else {
                mdm_user_manager_goto_login_session (adata->manager);
        }
 out:
        maybe_lock_screen (adata);
}

static void
update_switch_user (MdmAppletData *adata)
{
        gboolean can_switch;
        gboolean has_other_users;

        can_switch = mdm_user_manager_can_switch (adata->manager);
        g_object_get (adata->manager,
                      "has-multiple-users", &has_other_users,
                      NULL);

        if (can_switch && has_other_users) {
                gtk_widget_show (adata->login_screen_item);
        } else {

                gtk_widget_hide (adata->login_screen_item);
        }
}

static void
on_manager_is_loaded_changed (MdmUserManager *manager,
                              GParamSpec     *pspec,
                              MdmAppletData  *adata)
{
        update_switch_user (adata);
}

static void
on_manager_has_multiple_users_changed (MdmUserManager       *manager,
                                       GParamSpec           *pspec,
                                       MdmAppletData        *adata)
{
        update_switch_user (adata);
}

#ifdef BUILD_PRESENSE_STUFF
static void
on_user_item_activate (GtkMenuItem   *item,
                       MdmAppletData *adata)
{
        g_signal_stop_emission_by_name (item, "activate");
}
#endif

static void
on_control_panel_activate (GtkMenuItem   *item,
                           MdmAppletData *adata)
{
        char      *args[2];
        GError    *error;
        GdkScreen *screen;
        gboolean   res;

        args[0] = g_find_program_in_path ("mate-control-center");
        if (args[0] == NULL) {
                return;
        }
        args[1] = NULL;

        if (gtk_widget_has_screen (GTK_WIDGET (adata->applet))) {
                screen = gtk_widget_get_screen (GTK_WIDGET (adata->applet));
        } else {
                screen = gdk_screen_get_default ();
        }

        error = NULL;
        res = gdk_spawn_on_screen (screen,
                                   g_get_home_dir (),
                                   args,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &error);
        if (! res) {
                g_warning (_("Can't lock screen: %s"), error->message);
                g_error_free (error);
        }

        g_free (args[0]);
}

static void
on_account_activate (GtkMenuItem   *item,
                     MdmAppletData *adata)
{
        char      *args[2];
        GError    *error;
        GdkScreen *screen;
        gboolean   res;

        args[0] = g_find_program_in_path ("accounts-dialog");
        if (args[0] == NULL) {
                args[0] = g_find_program_in_path ("mate-about-me");
                if (args[0] == NULL) {
                        return;
                }
        }
        args[1] = NULL;

        if (gtk_widget_has_screen (GTK_WIDGET (adata->applet))) {
                screen = gtk_widget_get_screen (GTK_WIDGET (adata->applet));
        } else {
                screen = gdk_screen_get_default ();
        }

        error = NULL;
        res = gdk_spawn_on_screen (screen,
                                   g_get_home_dir (),
                                   args,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &error);
        if (! res) {
                g_warning (_("Can't lock screen: %s"), error->message);
                g_error_free (error);
        }

        g_free (args[0]);
}

static void
on_lock_screen_activate (GtkMenuItem   *item,
                         MdmAppletData *adata)
{
        maybe_lock_screen (adata);
}

static void
on_login_screen_activate (GtkMenuItem   *item,
                          MdmAppletData *adata)
{
        MdmUser *user;

        user = NULL;

        do_switch (adata, user);
}

static void
on_quit_session_activate (GtkMenuItem   *item,
                          MdmAppletData *adata)
{
        char      *args[3];
        GError    *error;
        GdkScreen *screen;
        gboolean   res;

        args[0] = g_find_program_in_path ("mate-session-save");
        if (args[0] == NULL) {
                return;
        }

        args[1] = "--logout-dialog";
        args[2] = NULL;

        if (gtk_widget_has_screen (GTK_WIDGET (adata->applet))) {
                screen = gtk_widget_get_screen (GTK_WIDGET (adata->applet));
        } else {
                screen = gdk_screen_get_default ();
        }

        error = NULL;
        res = gdk_spawn_on_screen (screen,
                                   g_get_home_dir (),
                                   args,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &error);
        if (! res) {
                g_warning (_("Can't log out: %s"), error->message);
                g_error_free (error);
        }

        g_free (args[0]);
}

#ifdef BUILD_PRESENSE_STUFF
static gboolean
on_menu_key_press_event (GtkWidget     *widget,
                         GdkEventKey   *event,
                         MdmAppletData *adata)
{
        GtkWidget *entry;

        entry = mdm_entry_menu_item_get_entry (MDM_ENTRY_MENU_ITEM (adata->user_item));

        if (GTK_WIDGET_HAS_FOCUS (entry)) {
                gtk_widget_event (entry, (GdkEvent *)event);
                return TRUE;
        } else {
                return FALSE;
        }
}

static void
save_status (MdmAppletData *adata,
             guint          status)
{
        if (adata->current_status != status) {
                GError *error;

                adata->current_status = status;

                g_debug ("Saving status: %u", status);
                error = NULL;
                dbus_g_proxy_call (adata->presence_proxy,
                                   "SetStatus",
                                   &error,
                                   G_TYPE_UINT, status,
                                   G_TYPE_INVALID,
                                   G_TYPE_INVALID);

                if (error != NULL) {
                        g_warning ("Couldn't save presence status: %s", error->message);
                        g_error_free (error);
                }
        }
}

static void
on_status_available_activate (GtkWidget     *widget,
                              MdmAppletData *adata)
{

        if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget))) {
                save_status (adata, GSM_PRESENCE_STATUS_AVAILABLE);
        }
}

static void
on_status_busy_activate (GtkWidget     *widget,
                         MdmAppletData *adata)
{
         if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget))) {
                 save_status (adata, GSM_PRESENCE_STATUS_BUSY);
         }
}

static void
on_status_invisible_activate (GtkWidget     *widget,
                              MdmAppletData *adata)
{
         if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget))) {
                 save_status (adata, GSM_PRESENCE_STATUS_INVISIBLE);
         }
}

static struct {
        char      *icon_name;
        char      *display_name;
        void      *menu_callback;
        GtkWidget *widget;
} statuses[] = {
        { "user-online", N_("Available"), on_status_available_activate, NULL },
        { "user-invisible", N_("Invisible"), on_status_invisible_activate, NULL },
        { "user-busy", N_("Busy"), on_status_busy_activate, NULL },
        { "user-away", N_("Away"), NULL, NULL },
};
#endif

static void
update_label (MdmAppletData *adata)
{
        GtkWidget *label;
        char      *markup;

        label = gtk_bin_get_child (GTK_BIN (adata->menuitem));

#ifdef BUILD_PRESENSE_STUFF
        markup = g_strdup_printf ("<b>%s</b> <small>(%s)</small>",
                                  mdm_user_get_real_name (MDM_USER (adata->user)),
                                  _(statuses[adata->current_status].display_name));
#else
        markup = g_strdup_printf ("<b>%s</b>",
                                  mdm_user_get_real_name (MDM_USER (adata->user)));
#endif
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
}

#ifdef BUILD_PRESENSE_STUFF
static void
save_status_text (MdmAppletData *adata)
{
        GtkWidget     *entry;
        GtkTextBuffer *buffer;
        char          *escaped_text;
        char          *text;
        GtkTextIter    start, end;

        entry = mdm_entry_menu_item_get_entry (MDM_ENTRY_MENU_ITEM (adata->user_item));
        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
        gtk_text_buffer_get_bounds (buffer, &start, &end);
        text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
        escaped_text = g_markup_escape_text (text, -1);

        if (escaped_text != NULL) {
                GError *error;

                error = NULL;
                dbus_g_proxy_call (adata->presence_proxy,
                                   "SetStatusText",
                                   &error,
                                   G_TYPE_STRING, escaped_text,
                                   G_TYPE_INVALID,
                                   G_TYPE_INVALID);

                if (error != NULL) {
                        g_warning ("Couldn't set presence status text: %s", error->message);
                        g_error_free (error);
                }
        }

        g_free (text);
        g_free (escaped_text);
}

static void
on_user_item_deselect (GtkWidget     *item,
                       MdmAppletData *adata)
{
        save_status_text (adata);
}
#endif

static void
create_sub_menu (MdmAppletData *adata)
{
        GtkWidget *item;
#ifdef BUILD_PRESENSE_STUFF
        int        i;
        GSList    *radio_group;
#endif

        adata->menu = gtk_menu_new ();
#ifdef BUILD_PRESENSE_STUFF
        g_signal_connect (adata->menu,
                          "key-press-event",
                          G_CALLBACK (on_menu_key_press_event),
                          adata);
#endif
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (adata->menuitem), adata->menu);
        g_signal_connect (adata->menu, "style-set",
                          G_CALLBACK (menu_style_set_cb), adata);
        g_signal_connect (adata->menu, "show",
                          G_CALLBACK (menu_expose_cb), adata);

#ifdef BUILD_PRESENSE_STUFF
        adata->user_item = mdm_entry_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu),
                               adata->user_item);
        gtk_widget_show (adata->user_item);
        g_signal_connect (adata->user_item, "activate",
                          G_CALLBACK (on_user_item_activate), adata);
        g_signal_connect (adata->user_item,
                          "deselect",
                          G_CALLBACK (on_user_item_deselect),
                          adata);

        item = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu), item);
        gtk_widget_show (item);

        radio_group = NULL;
        for (i = 0; i < G_N_ELEMENTS (statuses); i++) {
                GtkWidget *hbox;
                GtkWidget *label;
                GtkWidget *image;
                GtkWidget *item;

                if (statuses[i].menu_callback == NULL) {
                        continue;
                }

                item = gtk_radio_menu_item_new (radio_group);
                radio_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
                hbox = gtk_hbox_new (FALSE, 3);
                label = gtk_label_new (_(statuses[i].display_name));
                gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
                gtk_widget_show (label);
                image = gtk_image_new_from_icon_name (statuses[i].icon_name, GTK_ICON_SIZE_MENU);
                gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
                gtk_widget_show (image);
                gtk_widget_show (hbox);
                gtk_container_add (GTK_CONTAINER (item), hbox);

                gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu),
                                       item);
                g_signal_connect (item, "activate",
                                  G_CALLBACK (statuses[i].menu_callback), adata);
                gtk_widget_show (item);

                statuses[i].widget = item;
        }

        item = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu), item);
        gtk_widget_show (item);
#endif

        adata->account_item = gtk_image_menu_item_new_with_label (_("Account Information"));
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (adata->account_item),
                                       gtk_image_new ());
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu),
                               adata->account_item);
        g_signal_connect (adata->account_item, "style-set",
                          G_CALLBACK (menuitem_style_set_cb), adata);
        g_signal_connect (adata->account_item, "activate",
                          G_CALLBACK (on_account_activate), adata);
        gtk_widget_show (adata->account_item);


        adata->control_panel_item = gtk_image_menu_item_new_with_label (_("System Preferences"));
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (adata->control_panel_item),
                                       gtk_image_new ());
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu),
                               adata->control_panel_item);
        g_signal_connect (adata->control_panel_item, "style-set",
                          G_CALLBACK (menuitem_style_set_cb), adata);
        g_signal_connect (adata->control_panel_item, "activate",
                          G_CALLBACK (on_control_panel_activate), adata);

        item = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu), item);
        gtk_widget_show (item);

        adata->lock_screen_item = gtk_image_menu_item_new_with_label (_("Lock Screen"));
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (adata->lock_screen_item),
                                       gtk_image_new ());
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu),
                               adata->lock_screen_item);
        g_signal_connect (adata->lock_screen_item, "style-set",
                          G_CALLBACK (menuitem_style_set_cb), adata);
        g_signal_connect (adata->lock_screen_item, "activate",
                          G_CALLBACK (on_lock_screen_activate), adata);
        /* Only show if not locked down */

        adata->login_screen_item = gtk_image_menu_item_new_with_label (_("Switch User"));
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (adata->login_screen_item),
                                       gtk_image_new ());
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu),
                               adata->login_screen_item);
        g_signal_connect (adata->login_screen_item, "style-set",
                          G_CALLBACK (menuitem_style_set_cb), adata);
        g_signal_connect (adata->login_screen_item, "activate",
                          G_CALLBACK (on_login_screen_activate), adata);
        /* Only show switch user if there are other users */

        adata->quit_session_item = gtk_image_menu_item_new_with_label (_("Quit…"));
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (adata->quit_session_item),
                                       gtk_image_new ());
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menu),
                               adata->quit_session_item);
        g_signal_connect (adata->quit_session_item, "style-set",
                          G_CALLBACK (menuitem_style_set_cb), adata);
        g_signal_connect (adata->quit_session_item, "activate",
                          G_CALLBACK (on_quit_session_activate), adata);
        gtk_widget_show (adata->quit_session_item);
        gtk_widget_show (adata->menu);
}

static void
destroy_sub_menu (MdmAppletData *adata)
{
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (adata->menuitem), NULL);
}

static void
set_menu_visibility (MdmAppletData *adata,
                     gboolean       visible)
{

        if (visible) {
                create_sub_menu (adata);
        } else {
                destroy_sub_menu (adata);
        }
}

static void
client_notify_lockdown_func (MateConfClient   *client,
                             guint          cnxn_id,
                             MateConfEntry    *entry,
                             MdmAppletData *adata)
{
        MateConfValue *value;
        const char *key;

        value = mateconf_entry_get_value (entry);
        key = mateconf_entry_get_key (entry);

        if (value == NULL || key == NULL) {
                return;
        }

        if (strcmp (key, LOCKDOWN_USER_SWITCHING_KEY) == 0) {
                if (mateconf_value_get_bool (value)) {
                        set_menu_visibility (adata, FALSE);
                } else {
                        set_menu_visibility (adata, TRUE);
                }
        } else if (strcmp (key, LOCKDOWN_LOCK_SCREEN_KEY) == 0) {
                if (mateconf_value_get_bool (value)) {
                        gtk_widget_hide (adata->lock_screen_item);
                } else {
                        gtk_widget_show (adata->lock_screen_item);
                }
        }
}

static void
reset_icon (MdmAppletData *adata)
{
        GdkPixbuf *pixbuf;
        GtkWidget *image;

        if (adata->user == NULL || !gtk_widget_has_screen (GTK_WIDGET (adata->menuitem))) {
                return;
        }

#ifdef BUILD_PRESENSE_STUFF
        if (adata->user_item != NULL) {
                image = mdm_entry_menu_item_get_image (MDM_ENTRY_MENU_ITEM (adata->user_item));
                pixbuf = mdm_user_render_icon (adata->user, adata->panel_size * 3);
                if (pixbuf == NULL) {
                        return;
                }

                gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
                g_object_unref (pixbuf);
        }
#else
        pixbuf = mdm_user_render_icon (adata->user, adata->panel_size);

        if (pixbuf == NULL) {
                return;
        }

        image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (adata->menuitem));
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
        g_object_unref (pixbuf);
#endif
}

static void
setup_current_user_now (MdmAppletData *adata)
{
        g_assert (adata->user != NULL);

        if (adata->user_loaded_notify_id != 0) {
                g_signal_handler_disconnect (adata->user, adata->user_loaded_notify_id);
        }
        adata->user_loaded_notify_id = 0;

        update_label (adata);
        reset_icon (adata);
        adata->user_changed_notify_id =
            g_signal_connect (adata->user,
                              "changed",
                              G_CALLBACK (on_user_changed),
                              adata);
}

static void
on_current_user_loaded (MdmUser       *user,
                        GParamSpec    *pspec,
                        MdmAppletData *adata)
{
        if (!mdm_user_is_loaded (user)) {
                return;
        }

        setup_current_user_now (adata);
}

static void
setup_current_user (MdmAppletData *adata)
{
        adata->user = mdm_user_manager_get_user_by_uid (adata->manager, getuid ());

        if (adata->user == NULL) {
                g_warning ("Could not setup current user");
                return;
        }

        g_object_ref (adata->user);

        adata->menuitem = gtk_image_menu_item_new_with_label ("");
#ifndef BUILD_PRESENSE_STUFF
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (adata->menuitem),
                                       gtk_image_new ());
#endif
        gtk_menu_shell_append (GTK_MENU_SHELL (adata->menubar), adata->menuitem);
        gtk_widget_show (adata->menuitem);

        if (mdm_user_is_loaded (adata->user)) {
                setup_current_user_now (adata);
                return;
        }

        adata->user_loaded_notify_id = g_signal_connect (adata->user,
                                                         "notify::is-loaded",
                                                         G_CALLBACK (on_current_user_loaded),
                                                         adata);
}

#ifdef BUILD_PRESENSE_STUFF
static void
set_status (MdmAppletData *adata,
            guint status)
{
        int i;

        g_debug ("Setting current status: %u", status);
        adata->current_status = status;
        for (i = 0; i < G_N_ELEMENTS (statuses); i++) {
                if (statuses[i].widget == NULL) {
                        continue;
                }
                if (i == status) {
                        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (statuses[i].widget),
                                                        TRUE);
                }
        }

        update_label (adata);
}

static void
on_presence_status_changed (DBusGProxy    *presence_proxy,
                            guint          status,
                            MdmAppletData *adata)
{
        g_debug ("Status changed: %u", status);

        set_status (adata, status);
}

static void
set_status_text (MdmAppletData *adata,
                 const char    *status_text)
{
        GtkWidget     *entry;
        GtkTextBuffer *buffer;

        g_debug ("Status text changed: %s", status_text);

        entry = mdm_entry_menu_item_get_entry (MDM_ENTRY_MENU_ITEM (adata->user_item));
        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
        gtk_text_buffer_set_text (buffer, status_text, -1);
}

static void
on_presence_status_text_changed (DBusGProxy    *presence_proxy,
                                 const char    *status_text,
                                 MdmAppletData *adata)
{
        set_status_text (adata, status_text);
}
#endif

static gboolean
fill_applet (MatePanelApplet *applet)
{
        static const MateComponentUIVerb menu_verbs[] = {
                MATECOMPONENT_UI_VERB ("MdmAboutMe", about_me_cb),
                MATECOMPONENT_UI_VERB ("MdmUsersGroupsAdmin", admin_cb),
                MATECOMPONENT_UI_VERB ("MdmAbout", about_cb),
                MATECOMPONENT_UI_VERB_END
        };
        static gboolean    first_time = FALSE;
        char              *tmp;
        MateComponentUIComponent *popup_component;
        MdmAppletData     *adata;
        GError            *error;
        DBusGConnection   *bus;

        if (!first_time) {
                first_time = TRUE;

                /* Do this here so it's only done once. */
                gtk_rc_parse_string ("style \"mdm-user-switch-menubar-style\"\n"
                                     "{\n"
                                     "GtkMenuBar::shadow-type = none\n"
                                     "GtkMenuBar::internal-padding = 0\n"
                                     "}\n"
                                     "style \"mdm-user-switch-applet-style\"\n"
                                     "{\n"
                                     "GtkWidget::focus-line-width = 0\n"
                                     "GtkWidget::focus-padding = 0\n"
                                     "}\n"
                                     "widget \"*.PanelAppletUserSwitchmenubar\" style \"mdm-user-switch-menubar-style\"\n"
                                     "widget \"*.PanelAppletUserSwitch\" style \"mdm-user-switch-applet-style\"\n");
                gtk_window_set_default_icon_name ("stock_people");
                g_set_application_name (_("User Switch Applet"));

                if (! mdm_settings_client_init (DATADIR "/mdm/mdm.schemas", "/")) {
                        g_critical ("Unable to initialize settings client");
                        exit (1);
                }

        }

        adata = g_new0 (MdmAppletData, 1);
        adata->applet = applet;
        adata->panel_size = 24;

        adata->client = mateconf_client_get_default ();

        gtk_widget_set_tooltip_text (GTK_WIDGET (applet), _("Change account settings and status"));
        gtk_container_set_border_width (GTK_CONTAINER (applet), 0);
        gtk_widget_set_name (GTK_WIDGET (applet), "PanelAppletUserSwitch");
        mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);
        mate_panel_applet_setup_menu_from_file (applet, NULL,
                                           DATADIR "/mate-2.0/ui/MATE_FastUserSwitchApplet.xml",
                                           NULL, menu_verbs, adata);

        popup_component = mate_panel_applet_get_popup_component (applet);

        /* Hide the admin context menu items if locked down or no cmd-line */
        if (mateconf_client_get_bool (adata->client,
                                   LOCKDOWN_COMMAND_LINE_KEY,
                                   NULL) ||
            mate_panel_applet_get_locked_down (applet)) {
                matecomponent_ui_component_set_prop (popup_component,
                                              "/popups/button3/MdmSeparator",
                                              "hidden", "1", NULL);
                matecomponent_ui_component_set_prop (popup_component,
                                              "/commands/MdmUsersGroupsAdmin",
                                              "hidden", "1", NULL);
        } else {
#ifndef USERS_ADMIN
#  ifdef MDM_SETUP
                matecomponent_ui_component_set_prop (popup_component,
                                              "/popups/button3/MdmSeparator",
                                              "hidden", "1",
                                              NULL);
#  endif /* !MDM_SETUP */
                matecomponent_ui_component_set_prop (popup_component,
                                              "/commands/MdmUsersGroupsAdmin",
                                              "hidden", "1",
                                              NULL);
#endif /* !USERS_ADMIN */
        }

        /* Hide the mdmphotosetup item if it can't be found in the path. */
        tmp = g_find_program_in_path ("mate-about-me");
        if (!tmp) {
                matecomponent_ui_component_set_prop (popup_component,
                                              "/commands/MdmAboutMe",
                                              "hidden", "1",
                                              NULL);
        } else {
                g_free (tmp);
        }

        g_signal_connect (adata->applet,
                          "style-set",
                          G_CALLBACK (applet_style_set_cb), adata);
        g_signal_connect (applet,
                          "change-background",
                          G_CALLBACK (applet_change_background_cb), adata);
        g_signal_connect (applet,
                          "size-allocate",
                          G_CALLBACK (applet_size_allocate_cb), adata);
        g_signal_connect (applet,
                          "key-press-event",
                          G_CALLBACK (applet_key_press_event_cb), adata);
        g_signal_connect_after (applet,
                                "focus-in-event",
                                G_CALLBACK (gtk_widget_queue_draw), NULL);
        g_signal_connect_after (applet,
                                "focus-out-event",
                                G_CALLBACK (gtk_widget_queue_draw), NULL);
        g_object_set_data_full (G_OBJECT (applet),
                                "mdm-applet-data",
                                adata,
                                (GDestroyNotify) mdm_applet_data_free);

        adata->menubar = gtk_menu_bar_new ();
        gtk_widget_set_name (adata->menubar, "PanelAppletUserSwitchmenubar");
        gtk_widget_set_can_focus (adata->menubar, TRUE);
        g_signal_connect (adata->menubar, "button-press-event",
                          G_CALLBACK (menubar_button_press_event_cb), adata);
        g_signal_connect_after (adata->menubar, "expose-event",
                                G_CALLBACK (menubar_expose_event_cb), adata);
        gtk_container_add (GTK_CONTAINER (applet), adata->menubar);
        gtk_widget_show (adata->menubar);

        adata->manager = mdm_user_manager_ref_default ();
        g_object_set (adata->manager, "include-all", TRUE, NULL);
        g_signal_connect (adata->manager,
                          "notify::is-loaded",
                          G_CALLBACK (on_manager_is_loaded_changed),
                          adata);
        g_signal_connect (adata->manager,
                          "notify::has-multiple-users",
                          G_CALLBACK (on_manager_has_multiple_users_changed),
                          adata);

        mdm_user_manager_queue_load (adata->manager);
        setup_current_user (adata);

        mateconf_client_add_dir (adata->client,
                              LOCKDOWN_DIR,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);

        adata->client_notify_lockdown_id = mateconf_client_notify_add (adata->client,
                                                                    LOCKDOWN_DIR,
                                                                    (MateConfClientNotifyFunc)client_notify_lockdown_func,
                                                                    adata,
                                                                    NULL,
                                                                    NULL);

        if (mateconf_client_get_bool (adata->client, LOCKDOWN_USER_SWITCHING_KEY, NULL)) {
                set_menu_visibility (adata, FALSE);
        } else {
                set_menu_visibility (adata, TRUE);
        }
        if (mateconf_client_get_bool (adata->client, LOCKDOWN_LOCK_SCREEN_KEY, NULL)) {
                        gtk_widget_hide (adata->lock_screen_item);
        } else {
                        gtk_widget_show (adata->lock_screen_item);
        }

        error = NULL;
        bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (bus == NULL) {
                g_warning ("Unable to get session bus: %s", error->message);
                g_error_free (error);
                goto done;
        }

#ifdef BUILD_PRESENSE_STUFF
        adata->presence_proxy = dbus_g_proxy_new_for_name (bus,
                                                          "org.mate.SessionManager",
                                                          "/org/mate/SessionManager/Presence",
                                                          "org.mate.SessionManager.Presence");
        if (adata->presence_proxy != NULL) {
                DBusGProxy *proxy;

                dbus_g_proxy_add_signal (adata->presence_proxy,
                                         "StatusChanged",
                                         G_TYPE_UINT,
                                         G_TYPE_INVALID);
                dbus_g_proxy_connect_signal (adata->presence_proxy,
                                             "StatusChanged",
                                             G_CALLBACK (on_presence_status_changed),
                                             adata,
                                             NULL);
                dbus_g_proxy_add_signal (adata->presence_proxy,
                                         "StatusTextChanged",
                                         G_TYPE_STRING,
                                         G_TYPE_INVALID);
                dbus_g_proxy_connect_signal (adata->presence_proxy,
                                             "StatusTextChanged",
                                             G_CALLBACK (on_presence_status_text_changed),
                                             adata,
                                             NULL);


                proxy = dbus_g_proxy_new_from_proxy (adata->presence_proxy,
                                                     "org.freedesktop.DBus.Properties",
                                                     "/org/mate/SessionManager/Presence");
                if (proxy != NULL) {
                        guint       status;
                        const char *status_text;
                        GValue      value = { 0, };

                        status = 0;
                        status_text = NULL;

                        error = NULL;
                        dbus_g_proxy_call (proxy,
                                           "Get",
                                           &error,
                                           G_TYPE_STRING, "org.mate.SessionManager.Presence",
                                           G_TYPE_STRING, "status",
                                           G_TYPE_INVALID,
                                           G_TYPE_VALUE, &value,
                                           G_TYPE_INVALID);

                        if (error != NULL) {
                                g_warning ("Couldn't get presence status: %s", error->message);
                                g_error_free (error);
                        } else {
                                status = g_value_get_uint (&value);
                        }

                        g_value_unset (&value);

                        error = NULL;
                        dbus_g_proxy_call (proxy,
                                           "Get",
                                           &error,
                                           G_TYPE_STRING, "org.mate.SessionManager.Presence",
                                           G_TYPE_STRING, "status-text",
                                           G_TYPE_INVALID,
                                           G_TYPE_VALUE, &value,
                                           G_TYPE_INVALID);

                        if (error != NULL) {
                                g_warning ("Couldn't get presence status text: %s", error->message);
                                g_error_free (error);
                        } else {
                                status_text = g_value_get_string (&value);
                        }

                        set_status (adata, status);
                        set_status_text (adata, status_text);
                }
        } else {
                g_warning ("Failed to get session presence proxy");
        }
#endif

 done:
        gtk_widget_show (GTK_WIDGET (adata->applet));

        return TRUE;
}

static gboolean
applet_factory (MatePanelApplet   *applet,
                const char    *iid,
                gpointer       data)
{
        gboolean ret;
        ret = FALSE;
        if (strcmp (iid, "OAFIID:MATE_FastUserSwitchApplet") == 0) {
                ret = fill_applet (applet);
        }
        return ret;
}
