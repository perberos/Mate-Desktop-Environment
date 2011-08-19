/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* mate-window-properties.c
 * Copyright (C) 2002 Seth Nickell
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * Written by: Seth Nickell <snickell@stanford.edu>
 *             Havoc Pennington <hp@redhat.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <glib/gi18n.h>
#include <string.h>
#include <mate-wm-manager.h>

#include "capplet-util.h"
#include "mateconf-property-editor.h"

typedef struct
{
        int number;
        char *name;
        const char *value; /* machine-readable name for storing config */
        GtkWidget *radio;
} MouseClickModifier;

static MateWindowManager *current_wm; /* may be NULL */
static GtkWidget *dialog_win;
static GObject *focus_mode_checkbutton;
static GObject *autoraise_checkbutton;
static GObject *autoraise_delay_slider;
static GtkWidget *autoraise_delay_hbox;
static GObject *double_click_titlebar_optionmenu;
static GObject *alt_click_hbox;

static MateWMSettings *settings;
static const MateWMDoubleClickAction *double_click_actions = NULL;
static int n_double_click_actions = 0;

static MouseClickModifier *mouse_modifiers = NULL;
static int n_mouse_modifiers = 0;

static void reload_mouse_modifiers (void);

static void
mouse_focus_toggled_callback (GtkWidget *button,
                              void      *data)
{
        MateWMSettings new_settings;

        new_settings.flags = MATE_WM_SETTING_MOUSE_FOCUS;
        new_settings.focus_follows_mouse =
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

        if (current_wm != NULL && new_settings.focus_follows_mouse != settings->focus_follows_mouse)
                mate_window_manager_change_settings (current_wm, &new_settings);
}

static void
autoraise_toggled_callback (GtkWidget *button,
                            void      *data)
{
        MateWMSettings new_settings;

        new_settings.flags = MATE_WM_SETTING_AUTORAISE;
        new_settings.autoraise =
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

        if (current_wm != NULL && new_settings.autoraise != settings->autoraise)
                mate_window_manager_change_settings (current_wm, &new_settings);

}

static void
autoraise_delay_value_changed_callback (GtkWidget *slider,
                                        void      *data)
{
        MateWMSettings new_settings;

        new_settings.flags = MATE_WM_SETTING_AUTORAISE_DELAY;
        new_settings.autoraise_delay =
                gtk_range_get_value (GTK_RANGE (slider)) * 1000;

        if (current_wm != NULL && new_settings.autoraise_delay != settings->autoraise_delay)
                mate_window_manager_change_settings (current_wm, &new_settings);
}

static void
double_click_titlebar_changed_callback (GtkWidget *optionmenu,
                                        void      *data)
{
        MateWMSettings new_settings;

        new_settings.flags = MATE_WM_SETTING_DOUBLE_CLICK_ACTION;
        new_settings.double_click_action =
                gtk_combo_box_get_active (GTK_COMBO_BOX (optionmenu));

        if (current_wm != NULL && new_settings.double_click_action != settings->double_click_action)
                mate_window_manager_change_settings (current_wm, &new_settings);
}

static void
alt_click_radio_toggled_callback (GtkWidget *radio,
                                  void      *data)
{
        MateWMSettings new_settings;
        gboolean active;
        MouseClickModifier *modifier = data;

        new_settings.flags = MATE_WM_SETTING_MOUSE_MOVE_MODIFIER;
        active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio));

        if (active && current_wm != NULL) {
                new_settings.mouse_move_modifier = modifier->value;

                if ((settings->mouse_move_modifier == NULL) ||
                    (strcmp (new_settings.mouse_move_modifier,
                             settings->mouse_move_modifier) != 0))
                        mate_window_manager_change_settings (current_wm, &new_settings);
        }
}

static void
update_sensitivity (void)
{
        gtk_widget_set_sensitive (GTK_WIDGET (autoraise_checkbutton),
                                  settings->focus_follows_mouse);

        gtk_widget_set_sensitive (autoraise_delay_hbox,
                                  settings->focus_follows_mouse && settings->autoraise);

        gtk_widget_set_sensitive (GTK_WIDGET (double_click_titlebar_optionmenu),
                                  n_double_click_actions > 1);

        /* disable the whole dialog while no WM is running, or
         * a WM we don't understand is running. We should probably do
         * something better. I don't want to just launch the config tool
         * as we would on startup though, because then you'd get weirdness
         * in the gap time between old and new WM.
         */
        gtk_widget_set_sensitive (dialog_win, current_wm != NULL);
}

static void
init_settings_struct (MateWMSettings *settings)
{
        /* Init fields that weren't initialized */
        if ((settings->flags & MATE_WM_SETTING_MOUSE_FOCUS) == 0)
                settings->focus_follows_mouse = FALSE;

        if ((settings->flags & MATE_WM_SETTING_AUTORAISE) == 0)
                settings->autoraise = FALSE;

        if ((settings->flags & MATE_WM_SETTING_AUTORAISE_DELAY) == 0)
                settings->autoraise_delay = 1000;

        if ((settings->flags & MATE_WM_SETTING_MOUSE_MOVE_MODIFIER) == 0)
                settings->mouse_move_modifier = "Super";

        if ((settings->flags & MATE_WM_SETTING_DOUBLE_CLICK_ACTION) == 0)
                settings->double_click_action = 0;
}

static void
set_alt_click_value (const MateWMSettings *settings)
{
	gboolean match_found = FALSE;
	int i;

	/* We look for a matching modifier and set it. */
	if (settings->mouse_move_modifier != NULL) {
		for (i = 0; i < n_mouse_modifiers; i ++)
			if (strcmp (mouse_modifiers[i].value,
				    settings->mouse_move_modifier) == 0) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mouse_modifiers[i].radio),
							      TRUE);
				match_found = TRUE;
				break;
			}
	}

	/* No matching modifier was found; we set all the toggle buttons to be
	 * insensitive. */
	for (i = 0; i < n_mouse_modifiers; i++) {
		gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (mouse_modifiers[i].radio),
						    ! match_found);
	}
}

static void
reload_settings (void)
{
        MateWMSettings new_settings;

        g_assert (n_mouse_modifiers > 0);

        if (current_wm != NULL) {
                new_settings.flags = MATE_WM_SETTING_MOUSE_FOCUS |
                        MATE_WM_SETTING_AUTORAISE |
                        MATE_WM_SETTING_AUTORAISE_DELAY |
                        MATE_WM_SETTING_MOUSE_MOVE_MODIFIER |
                        MATE_WM_SETTING_DOUBLE_CLICK_ACTION;

                /* this will clear any flags that don't get filled in */
                mate_window_manager_get_settings (current_wm, &new_settings);
        } else {
                new_settings.flags = 0;
        }

        init_settings_struct (&new_settings);

        if (new_settings.focus_follows_mouse != settings->focus_follows_mouse)
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (focus_mode_checkbutton),
                                              new_settings.focus_follows_mouse);

        if (new_settings.autoraise != settings->autoraise)
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (autoraise_checkbutton),
                                              new_settings.autoraise);

        if (new_settings.autoraise_delay != settings->autoraise_delay)
                gtk_range_set_value (GTK_RANGE (autoraise_delay_slider),
                                     new_settings.autoraise_delay / 1000.0);

        if (n_double_click_actions > 0 &&
            new_settings.double_click_action != settings->double_click_action) {
                gtk_combo_box_set_active (GTK_COMBO_BOX (double_click_titlebar_optionmenu),
                                             new_settings.double_click_action);
        }

        if (settings->mouse_move_modifier == NULL ||
            new_settings.mouse_move_modifier == NULL ||
            strcmp (settings->mouse_move_modifier,
                    new_settings.mouse_move_modifier) != 0) {
                set_alt_click_value (&new_settings);
        }

	mate_wm_settings_free (settings);
        settings = mate_wm_settings_copy (&new_settings);

        update_sensitivity ();
}

static void
wm_settings_changed_callback (MateWindowManager *wm,
                              void               *data)
{
        reload_settings ();
}

static void
update_wm (GdkScreen *screen,
           gboolean   load_settings)
{
        int i;

        g_assert (n_mouse_modifiers > 0);

        if (current_wm != NULL) {
                g_signal_handlers_disconnect_by_func (G_OBJECT (current_wm),
                                                      G_CALLBACK (wm_settings_changed_callback),
                                                      NULL);
                current_wm = NULL;
                double_click_actions = NULL;
                n_double_click_actions = 0;
        }

        current_wm = mate_wm_manager_get_current (screen);

        if (current_wm != NULL) {
                g_signal_connect (G_OBJECT (current_wm), "settings_changed",
                                  G_CALLBACK (wm_settings_changed_callback), NULL);

                mate_window_manager_get_double_click_actions (current_wm,
                                                               &double_click_actions,
                                                               &n_double_click_actions);

        }

        for (i = 0; i < n_double_click_actions; i++) {
                gtk_combo_box_append_text (GTK_COMBO_BOX (double_click_titlebar_optionmenu),
                                           double_click_actions[i].human_readable_name);
        }

        if (load_settings)
                reload_settings ();
}

static void
wm_changed_callback (GdkScreen *screen,
                     void      *data)
{
        update_wm (screen, TRUE);
}

static void
response_cb (GtkWidget *dialog_win,
             int        response_id,
             void      *data)
{

        if (response_id == GTK_RESPONSE_HELP) {
		capplet_help (GTK_WINDOW (dialog_win),
			      "goscustdesk-58");
        } else {
                gtk_widget_destroy (dialog_win);
        }
}

static void
try_spawn_config_tool (GdkScreen *screen)
{
        GError *error;

        error = NULL;
        mate_wm_manager_spawn_config_tool_for_current (screen, &error);

        if (error != NULL) {
                GtkWidget *no_tool_dialog;
                char *str;
                char *escaped;

                escaped = g_markup_escape_text (error->message, -1);

                str = g_strdup_printf ("<b>%s</b>\n\n%s",
                                       _("Cannot start the preferences application for your window manager"),
                                       escaped);
                g_free (escaped);

                no_tool_dialog =
                        gtk_message_dialog_new (NULL,
                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_CLOSE,
                                                " ");
                gtk_window_set_title (GTK_WINDOW (no_tool_dialog), "");
                gtk_window_set_resizable (GTK_WINDOW (no_tool_dialog), FALSE);

                gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (no_tool_dialog), str);

                g_free (str);

                gtk_dialog_run (GTK_DIALOG (no_tool_dialog));

                gtk_widget_destroy (no_tool_dialog);
                g_error_free (error);

                exit (1);
        }

        /* exit, let the config tool handle it */
        exit (0);
}

int
main (int argc, char **argv)
{
        GdkScreen *screen;
	MateWMSettings new_settings;
        GtkBuilder *builder;
        GError *error = NULL;
	int rc = 0;
        int i;

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        gtk_init (&argc, &argv);

        mate_wm_manager_init ();

        screen = gdk_display_get_default_screen (gdk_display_get_default ());

        current_wm = mate_wm_manager_get_current (screen);

        if (current_wm == NULL) {
                try_spawn_config_tool (screen);
                goto out;
        }

        builder = gtk_builder_new ();
        gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

        if (gtk_builder_add_from_file (builder, UIDIR "/mate-window-properties.ui", &error) == 0) {
                g_warning ("Could not parse UI file: %s", error->message);
                g_error_free (error);
                g_object_unref (builder);
                rc = 1;
                goto out;
        }

        dialog_win = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "main-dialog"));
        focus_mode_checkbutton = gtk_builder_get_object (builder,
                                                         "focus-mode-checkbutton");
        autoraise_checkbutton = gtk_builder_get_object (builder,
                                                        "autoraise-checkbutton");
        autoraise_delay_slider = gtk_builder_get_object (builder,
                                                         "autoraise-delay-slider");
        autoraise_delay_hbox = GTK_WIDGET (gtk_builder_get_object (builder,
                                                                   "autoraise-delay-hbox"));
        double_click_titlebar_optionmenu = gtk_builder_get_object (builder,
                                                                   "double-click-titlebar-optionmenu");
        alt_click_hbox = gtk_builder_get_object (builder, "alt-click-box");

        gtk_range_set_range (GTK_RANGE (autoraise_delay_slider),
                             0, 10);

        gtk_range_set_increments (GTK_RANGE (autoraise_delay_slider),
                                  0.2, 1.0);

        new_settings.flags = 0;
        init_settings_struct (&new_settings);
	settings = mate_wm_settings_copy (&new_settings);

        reload_mouse_modifiers ();
        update_wm (screen, FALSE);

        set_alt_click_value (&new_settings);
        gtk_range_set_value (GTK_RANGE (autoraise_delay_slider),
                             new_settings.autoraise_delay / 1000.0);
        gtk_combo_box_set_active (GTK_COMBO_BOX (double_click_titlebar_optionmenu),
                                  new_settings.double_click_action);

        reload_settings (); /* must come before below signal connections */

        g_signal_connect (G_OBJECT (dialog_win), "response",
                          G_CALLBACK (response_cb), NULL);

        g_signal_connect (G_OBJECT (dialog_win), "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);


        g_signal_connect (focus_mode_checkbutton, "toggled",
                          G_CALLBACK (mouse_focus_toggled_callback), NULL);

        g_signal_connect (autoraise_checkbutton, "toggled",
                          G_CALLBACK (autoraise_toggled_callback), NULL);

        g_signal_connect (autoraise_delay_slider, "value_changed",
                          G_CALLBACK (autoraise_delay_value_changed_callback), NULL);

        g_signal_connect (double_click_titlebar_optionmenu, "changed",
                          G_CALLBACK (double_click_titlebar_changed_callback), NULL);

        g_signal_connect (G_OBJECT (screen), "window_manager_changed",
                          G_CALLBACK (wm_changed_callback), NULL);

        i = 0;
        while (i < n_mouse_modifiers) {
                g_signal_connect (G_OBJECT (mouse_modifiers[i].radio), "toggled",
                                  G_CALLBACK (alt_click_radio_toggled_callback),
                                  &mouse_modifiers[i]);
                ++i;
        }

        capplet_set_icon (dialog_win, "preferences-system-windows");
        gtk_widget_show (dialog_win);

        gtk_main ();

        g_object_unref (builder);

out:
        return rc;
}

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <gdk/gdkx.h>

static void
fill_radio (GtkRadioButton     *group,
            MouseClickModifier *modifier)
{
        modifier->radio =
                gtk_radio_button_new_with_mnemonic_from_widget (group,
                                                                modifier->name);
        gtk_box_pack_start (GTK_BOX (alt_click_hbox),
                            modifier->radio, FALSE, FALSE, 0);

        gtk_widget_show (modifier->radio);
}

static void
reload_mouse_modifiers (void)
{
        XModifierKeymap *modmap;
        KeySym *keymap;
        int keysyms_per_keycode;
        int map_size;
        int i;
        gboolean have_meta;
        gboolean have_hyper;
        gboolean have_super;
        int min_keycode, max_keycode;
        int mod_meta, mod_super, mod_hyper;

        XDisplayKeycodes (gdk_display,
                          &min_keycode,
                          &max_keycode);

        keymap = XGetKeyboardMapping (gdk_display,
                                      min_keycode,
                                      max_keycode - min_keycode,
                                      &keysyms_per_keycode);

        modmap = XGetModifierMapping (gdk_display);

        have_super = FALSE;
        have_meta = FALSE;
        have_hyper = FALSE;

        /* there are 8 modifiers, and the first 3 are shift, shift lock,
         * and control
         */
        map_size = 8 * modmap->max_keypermod;
        i = 3 * modmap->max_keypermod;
        mod_meta = mod_super = mod_hyper = 0;
        while (i < map_size) {
                /* get the key code at this point in the map,
                 * see if its keysym is one we're interested in
                 */
                int keycode = modmap->modifiermap[i];

                if (keycode >= min_keycode &&
                    keycode <= max_keycode) {
                        int j = 0;
                        KeySym *syms = keymap + (keycode - min_keycode) * keysyms_per_keycode;

                        while (j < keysyms_per_keycode) {
                                if (syms[j] == XK_Super_L ||
                                    syms[j] == XK_Super_R)
                                        mod_super = i / modmap->max_keypermod;
                                else if (syms[j] == XK_Hyper_L ||
                                         syms[j] == XK_Hyper_R)
                                        mod_hyper = i / modmap->max_keypermod;
                                else if ((syms[j] == XK_Meta_L ||
                                          syms[j] == XK_Meta_R))
                                        mod_meta = i / modmap->max_keypermod;
                                ++j;
                        }
                }

                ++i;
        }

        if ((1 << mod_meta) != Mod1Mask)
                have_meta = TRUE;
        if (mod_super != 0 &&
            mod_super != mod_meta)
                have_super = TRUE;
        if (mod_hyper != 0 &&
            mod_hyper != mod_meta &&
            mod_hyper != mod_super)
                have_hyper = TRUE;

        XFreeModifiermap (modmap);
        XFree (keymap);

        i = 0;
        while (i < n_mouse_modifiers) {
                g_free (mouse_modifiers[i].name);
                if (mouse_modifiers[i].radio)
                        gtk_widget_destroy (mouse_modifiers[i].radio);
                ++i;
        }
        g_free (mouse_modifiers);
        mouse_modifiers = NULL;

        n_mouse_modifiers = 1; /* alt */
        if (have_super)
                ++n_mouse_modifiers;
        if (have_hyper)
                ++n_mouse_modifiers;
        if (have_meta)
                ++n_mouse_modifiers;

        mouse_modifiers = g_new0 (MouseClickModifier, n_mouse_modifiers);

        i = 0;

        mouse_modifiers[i].number = i;
        mouse_modifiers[i].name = g_strdup (_("_Alt"));
        mouse_modifiers[i].value = "Alt";
        ++i;

        if (have_hyper) {
                mouse_modifiers[i].number = i;
                mouse_modifiers[i].name = g_strdup (_("H_yper"));
                mouse_modifiers[i].value = "Hyper";
                ++i;
        }

        if (have_super) {
                mouse_modifiers[i].number = i;
                mouse_modifiers[i].name = g_strdup (_("S_uper (or \"Windows logo\")"));
                mouse_modifiers[i].value = "Super";
                ++i;
        }

        if (have_meta) {
                mouse_modifiers[i].number = i;
                mouse_modifiers[i].name = g_strdup (_("_Meta"));
                mouse_modifiers[i].value = "Meta";
                ++i;
        }

        g_assert (i == n_mouse_modifiers);

        i = 0;
        while (i < n_mouse_modifiers) {
                fill_radio (i == 0 ? NULL : GTK_RADIO_BUTTON (mouse_modifiers[i-1].radio),
                            &mouse_modifiers[i]);
                ++i;
        }
}
