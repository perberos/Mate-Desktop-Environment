/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Rodrigo Moya
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
#include <time.h>

#include <X11/Xatom.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#include "mate-settings-profile.h"
#include "gsd-xsettings-manager.h"
#include "xsettings-manager.h"
#ifdef HAVE_FONTCONFIG
#include "fontconfig-monitor.h"
#endif /* HAVE_FONTCONFIG */

#define MATE_XSETTINGS_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MATE_TYPE_XSETTINGS_MANAGER, MateXSettingsManagerPrivate))

#define MOUSE_SETTINGS_DIR     "/desktop/mate/peripherals/mouse"
#define GTK_SETTINGS_DIR       "/desktop/gtk"
#define INTERFACE_SETTINGS_DIR "/desktop/mate/interface"
#define SOUND_SETTINGS_DIR     "/desktop/mate/sound"
#define GTK_MODULES_DIR        "/apps/mate_settings_daemon/gtk-modules"

#ifdef HAVE_FONTCONFIG
#define FONT_RENDER_DIR "/desktop/mate/font_rendering"
#define FONT_ANTIALIASING_KEY FONT_RENDER_DIR "/antialiasing"
#define FONT_HINTING_KEY      FONT_RENDER_DIR "/hinting"
#define FONT_RGBA_ORDER_KEY   FONT_RENDER_DIR "/rgba_order"
#define FONT_DPI_KEY          FONT_RENDER_DIR "/dpi"

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
#define DPI_FALLBACK 96
#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500

#endif /* HAVE_FONTCONFIG */

typedef struct _TranslationEntry TranslationEntry;
typedef void (* TranslationFunc) (MateXSettingsManager *manager,
                                  TranslationEntry      *trans,
                                  MateConfValue            *value);

struct _TranslationEntry {
        const char     *mateconf_key;
        const char     *xsetting_name;

        MateConfValueType  mateconf_type;
        TranslationFunc translate;
};

struct MateXSettingsManagerPrivate
{
        XSettingsManager **managers;
        guint              notify[6];
#ifdef HAVE_FONTCONFIG
        fontconfig_monitor_handle_t *fontconfig_handle;
#endif /* HAVE_FONTCONFIG */
};

#define GSD_XSETTINGS_ERROR gsd_xsettings_error_quark ()

enum {
        GSD_XSETTINGS_ERROR_INIT
};

static void     mate_xsettings_manager_class_init  (MateXSettingsManagerClass *klass);
static void     mate_xsettings_manager_init        (MateXSettingsManager      *xsettings_manager);
static void     mate_xsettings_manager_finalize    (GObject                  *object);

G_DEFINE_TYPE (MateXSettingsManager, mate_xsettings_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static GQuark
gsd_xsettings_error_quark (void)
{
        return g_quark_from_static_string ("gsd-xsettings-error-quark");
}

static void
translate_bool_int (MateXSettingsManager *manager,
                    TranslationEntry      *trans,
                    MateConfValue            *value)
{
        int i;

        g_assert (value->type == trans->mateconf_type);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_int (manager->priv->managers [i], trans->xsetting_name,
                                           mateconf_value_get_bool (value));
        }
}

static void
translate_int_int (MateXSettingsManager *manager,
                   TranslationEntry      *trans,
                   MateConfValue            *value)
{
        int i;

        g_assert (value->type == trans->mateconf_type);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_int (manager->priv->managers [i], trans->xsetting_name,
                                           mateconf_value_get_int (value));
        }
}

static void
translate_string_string (MateXSettingsManager *manager,
                         TranslationEntry      *trans,
                         MateConfValue            *value)
{
        int i;

        g_assert (value->type == trans->mateconf_type);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_string (manager->priv->managers [i],
                                              trans->xsetting_name,
                                              mateconf_value_get_string (value));
        }
}

static void
translate_string_string_toolbar (MateXSettingsManager *manager,
                                 TranslationEntry      *trans,
                                 MateConfValue            *value)
{
        int         i;
        const char *tmp;

        g_assert (value->type == trans->mateconf_type);

        /* This is kind of a workaround since MATE expects the key value to be
         * "both_horiz" and gtk+ wants the XSetting to be "both-horiz".
         */
        tmp = mateconf_value_get_string (value);
        if (tmp && strcmp (tmp, "both_horiz") == 0) {
                tmp = "both-horiz";
        }

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_string (manager->priv->managers [i],
                                              trans->xsetting_name,
                                              tmp);
        }
}

static TranslationEntry translations [] = {
        { "/desktop/mate/peripherals/mouse/double_click",   "Net/DoubleClickTime",     MATECONF_VALUE_INT,      translate_int_int },
        { "/desktop/mate/peripherals/mouse/drag_threshold", "Net/DndDragThreshold",    MATECONF_VALUE_INT,      translate_int_int },
        { "/desktop/mate/gtk-color-palette",                "Gtk/ColorPalette",        MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/font_name",              "Gtk/FontName",            MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/gtk_key_theme",          "Gtk/KeyThemeName",        MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/toolbar_style",          "Gtk/ToolbarStyle",        MATECONF_VALUE_STRING,   translate_string_string_toolbar },
        { "/desktop/mate/interface/toolbar_icons_size",     "Gtk/ToolbarIconSize",     MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/can_change_accels",      "Gtk/CanChangeAccels",     MATECONF_VALUE_BOOL,     translate_bool_int },
        { "/desktop/mate/interface/cursor_blink",           "Net/CursorBlink",         MATECONF_VALUE_BOOL,     translate_bool_int },
        { "/desktop/mate/interface/cursor_blink_time",      "Net/CursorBlinkTime",     MATECONF_VALUE_INT,      translate_int_int },
        { "/desktop/mate/interface/gtk_theme",              "Net/ThemeName",           MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/gtk_color_scheme",       "Gtk/ColorScheme",         MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/gtk-im-preedit-style",   "Gtk/IMPreeditStyle",      MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/gtk-im-status-style",    "Gtk/IMStatusStyle",       MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/gtk-im-module",          "Gtk/IMModule",            MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/icon_theme",             "Net/IconThemeName",       MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/file_chooser_backend",   "Gtk/FileChooserBackend",  MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/interface/menus_have_icons",       "Gtk/MenuImages",          MATECONF_VALUE_BOOL,     translate_bool_int },
        { "/desktop/mate/interface/buttons_have_icons",     "Gtk/ButtonImages",          MATECONF_VALUE_BOOL,     translate_bool_int },
        { "/desktop/mate/interface/menubar_accel",          "Gtk/MenuBarAccel",        MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/peripherals/mouse/cursor_theme",   "Gtk/CursorThemeName",     MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/peripherals/mouse/cursor_size",    "Gtk/CursorThemeSize",     MATECONF_VALUE_INT,      translate_int_int },
        { "/desktop/mate/interface/show_input_method_menu", "Gtk/ShowInputMethodMenu", MATECONF_VALUE_BOOL,     translate_bool_int },
        { "/desktop/mate/interface/show_unicode_menu",      "Gtk/ShowUnicodeMenu",     MATECONF_VALUE_BOOL,     translate_bool_int },
        { "/desktop/mate/sound/theme_name",                 "Net/SoundThemeName",      MATECONF_VALUE_STRING,   translate_string_string },
        { "/desktop/mate/sound/event_sounds",               "Net/EnableEventSounds" ,  MATECONF_VALUE_BOOL,     translate_bool_int },
        { "/desktop/mate/sound/input_feedback_sounds",      "Net/EnableInputFeedbackSounds", MATECONF_VALUE_BOOL, translate_bool_int }
};

#ifdef HAVE_FONTCONFIG
static double
dpi_from_pixels_and_mm (int pixels,
                        int mm)
{
        double dpi;

        if (mm >= 1)
                dpi = pixels / (mm / 25.4);
        else
                dpi = 0;

        return dpi;
}

static double
get_dpi_from_x_server (void)
{
        GdkScreen *screen;
        double     dpi;

        screen = gdk_screen_get_default ();
        if (screen != NULL) {
                double width_dpi, height_dpi;

                width_dpi = dpi_from_pixels_and_mm (gdk_screen_get_width (screen), gdk_screen_get_width_mm (screen));
                height_dpi = dpi_from_pixels_and_mm (gdk_screen_get_height (screen), gdk_screen_get_height_mm (screen));

                if (width_dpi < DPI_LOW_REASONABLE_VALUE || width_dpi > DPI_HIGH_REASONABLE_VALUE
                    || height_dpi < DPI_LOW_REASONABLE_VALUE || height_dpi > DPI_HIGH_REASONABLE_VALUE) {
                        dpi = DPI_FALLBACK;
                } else {
                        dpi = (width_dpi + height_dpi) / 2.0;
                }
        } else {
                /* Huh!?  No screen? */

                dpi = DPI_FALLBACK;
        }

        return dpi;
}

static double
get_dpi_from_mateconf_or_x_server (MateConfClient *client)
{
        MateConfValue *value;
        double      dpi;

        value = mateconf_client_get_without_default (client, FONT_DPI_KEY, NULL);

        /* If the user has ever set the DPI preference in MateConf, we use that.
         * Otherwise, we see if the X server reports a reasonable DPI value:  some X
         * servers report completely bogus values, and the user gets huge or tiny
         * fonts which are unusable.
         */

        if (value != NULL) {
                dpi = mateconf_value_get_float (value);
                mateconf_value_free (value);
        } else {
                dpi = get_dpi_from_x_server ();
        }

        return dpi;
}

typedef struct
{
        gboolean    antialias;
        gboolean    hinting;
        int         dpi;
        const char *rgba;
        const char *hintstyle;
} MateXftSettings;

static const char *rgba_types[] = { "rgb", "bgr", "vbgr", "vrgb" };

/* Read MateConf settings and determine the appropriate Xft settings based on them
 * This probably could be done a bit more cleanly with mateconf_string_to_enum
 */
static void
xft_settings_get (MateConfClient      *client,
                  MateXftSettings *settings)
{
        char  *antialiasing;
        char  *hinting;
        char  *rgba_order;
        double dpi;

        antialiasing = mateconf_client_get_string (client, FONT_ANTIALIASING_KEY, NULL);
        hinting = mateconf_client_get_string (client, FONT_HINTING_KEY, NULL);
        rgba_order = mateconf_client_get_string (client, FONT_RGBA_ORDER_KEY, NULL);
        dpi = get_dpi_from_mateconf_or_x_server (client);

        settings->antialias = TRUE;
        settings->hinting = TRUE;
        settings->hintstyle = "hintfull";
        settings->dpi = dpi * 1024; /* Xft wants 1/1024ths of an inch */
        settings->rgba = "rgb";

        if (rgba_order) {
                int i;
                gboolean found = FALSE;

                for (i = 0; i < G_N_ELEMENTS (rgba_types) && !found; i++) {
                        if (strcmp (rgba_order, rgba_types[i]) == 0) {
                                settings->rgba = rgba_types[i];
                                found = TRUE;
                        }
                }

                if (!found) {
                        g_warning ("Invalid value for " FONT_RGBA_ORDER_KEY ": '%s'",
                                   rgba_order);
                }
        }

        if (hinting) {
                if (strcmp (hinting, "none") == 0) {
                        settings->hinting = 0;
                        settings->hintstyle = "hintnone";
                } else if (strcmp (hinting, "slight") == 0) {
                        settings->hinting = 1;
                        settings->hintstyle = "hintslight";
                } else if (strcmp (hinting, "medium") == 0) {
                        settings->hinting = 1;
                        settings->hintstyle = "hintmedium";
                } else if (strcmp (hinting, "full") == 0) {
                        settings->hinting = 1;
                        settings->hintstyle = "hintfull";
                } else {
                        g_warning ("Invalid value for " FONT_HINTING_KEY ": '%s'",
                                   hinting);
                }
        }

        if (antialiasing) {
                gboolean use_rgba = FALSE;

                if (strcmp (antialiasing, "none") == 0) {
                        settings->antialias = 0;
                } else if (strcmp (antialiasing, "grayscale") == 0) {
                        settings->antialias = 1;
                } else if (strcmp (antialiasing, "rgba") == 0) {
                        settings->antialias = 1;
                        use_rgba = TRUE;
                } else {
                        g_warning ("Invalid value for " FONT_ANTIALIASING_KEY " : '%s'",
                                   antialiasing);
                }

                if (!use_rgba) {
                        settings->rgba = "none";
                }
        }

        g_free (rgba_order);
        g_free (hinting);
        g_free (antialiasing);
}

static void
xft_settings_set_xsettings (MateXSettingsManager *manager,
                            MateXftSettings      *settings)
{
        int i;

        mate_settings_profile_start (NULL);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_int (manager->priv->managers [i], "Xft/Antialias", settings->antialias);
                xsettings_manager_set_int (manager->priv->managers [i], "Xft/Hinting", settings->hinting);
                xsettings_manager_set_string (manager->priv->managers [i], "Xft/HintStyle", settings->hintstyle);
                xsettings_manager_set_int (manager->priv->managers [i], "Xft/DPI", settings->dpi);
                xsettings_manager_set_string (manager->priv->managers [i], "Xft/RGBA", settings->rgba);
        }
        mate_settings_profile_end (NULL);
}

static void
update_property (GString *props, const gchar* key, const gchar* value)
{
        gchar* needle;
        size_t needle_len;
        gchar* found = NULL;

        /* update an existing property */
        needle = g_strconcat (key, ":", NULL);
        needle_len = strlen (needle);
        if (g_str_has_prefix (props->str, needle))
                found = props->str;
        else 
            found = strstr (props->str, needle);

        if (found) {
                size_t value_index;
                gchar* end;

                end = strchr (found, '\n');
                value_index = (found - props->str) + needle_len + 1;
                g_string_erase (props, value_index, end ? (end - found - needle_len) : -1);
                g_string_insert (props, value_index, "\n");
                g_string_insert (props, value_index, value);
        } else {
                g_string_append_printf (props, "%s:\t%s\n", key, value);
        }
}

static void
xft_settings_set_xresources (MateXftSettings *settings)
{
        GString    *add_string;
        char        dpibuf[G_ASCII_DTOSTR_BUF_SIZE];
        Display    *dpy;

        mate_settings_profile_start (NULL);

        /* get existing properties */
        dpy = XOpenDisplay (NULL);
        g_return_if_fail (dpy != NULL);
        add_string = g_string_new (XResourceManagerString (dpy));

        g_debug("xft_settings_set_xresources: orig res '%s'", add_string->str);

        update_property (add_string, "Xft.dpi",
                                g_ascii_dtostr (dpibuf, sizeof (dpibuf), (double) settings->dpi / 1024.0));
        update_property (add_string, "Xft.antialias",
                                settings->antialias ? "1" : "0");
        update_property (add_string, "Xft.hinting",
                                settings->hinting ? "1" : "0");
        update_property (add_string, "Xft.hintstyle",
                                settings->hintstyle);
        update_property (add_string, "Xft.rgba",
                                settings->rgba);

        g_debug("xft_settings_set_xresources: new res '%s'", add_string->str);

        /* Set the new X property */
        XChangeProperty(dpy, RootWindow (dpy, 0),
                XA_RESOURCE_MANAGER, XA_STRING, 8, PropModeReplace, add_string->str, add_string->len);
        XCloseDisplay (dpy);

        g_string_free (add_string, TRUE);

        mate_settings_profile_end (NULL);
}

/* We mirror the Xft properties both through XSETTINGS and through
 * X resources
 */
static void
update_xft_settings (MateXSettingsManager *manager,
                     MateConfClient           *client)
{
        MateXftSettings settings;

        mate_settings_profile_start (NULL);

        xft_settings_get (client, &settings);
        xft_settings_set_xsettings (manager, &settings);
        xft_settings_set_xresources (&settings);

        mate_settings_profile_end (NULL);
}

static void
xft_callback (MateConfClient           *client,
              guint                  cnxn_id,
              MateConfEntry            *entry,
              MateXSettingsManager *manager)
{
        int i;

        update_xft_settings (manager, client);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_notify (manager->priv->managers [i]);
        }
}

static void
fontconfig_callback (fontconfig_monitor_handle_t *handle,
                     MateXSettingsManager       *manager)
{
        int i;
        int timestamp = time (NULL);

        mate_settings_profile_start (NULL);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_int (manager->priv->managers [i], "Fontconfig/Timestamp", timestamp);
                xsettings_manager_notify (manager->priv->managers [i]);
        }
        mate_settings_profile_end (NULL);
}

static gboolean
start_fontconfig_monitor_idle_cb (MateXSettingsManager *manager)
{
        mate_settings_profile_start (NULL);

        manager->priv->fontconfig_handle = fontconfig_monitor_start ((GFunc) fontconfig_callback, manager);

        mate_settings_profile_end (NULL);

        return FALSE;
}

static void
start_fontconfig_monitor (MateXSettingsManager  *manager)
{
        mate_settings_profile_start (NULL);

        fontconfig_cache_init ();

        g_idle_add ((GSourceFunc) start_fontconfig_monitor_idle_cb, manager);

        mate_settings_profile_end (NULL);
}

static void
stop_fontconfig_monitor (MateXSettingsManager  *manager)
{
        if (manager->priv->fontconfig_handle) {
                fontconfig_monitor_stop (manager->priv->fontconfig_handle);
                manager->priv->fontconfig_handle = NULL;
        }
}
#endif /* HAVE_FONTCONFIG */

static const char *
type_to_string (MateConfValueType type)
{
        switch (type) {
        case MATECONF_VALUE_INT:
                return "int";
        case MATECONF_VALUE_STRING:
                return "string";
        case MATECONF_VALUE_FLOAT:
                return "float";
        case MATECONF_VALUE_BOOL:
                return "bool";
        case MATECONF_VALUE_SCHEMA:
                return "schema";
        case MATECONF_VALUE_LIST:
                return "list";
        case MATECONF_VALUE_PAIR:
                return "pair";
        case MATECONF_VALUE_INVALID:
                return "*invalid*";
        default:
                g_assert_not_reached();
                return NULL; /* for warnings */
        }
}

static void
process_value (MateXSettingsManager *manager,
               TranslationEntry      *trans,
               MateConfValue            *val)
{
        if (val == NULL) {
                int i;

                for (i = 0; manager->priv->managers [i]; i++) {
                        xsettings_manager_delete_setting (manager->priv->managers [i], trans->xsetting_name);
                }
        } else {
                if (val->type == trans->mateconf_type) {
                        (* trans->translate) (manager, trans, val);
                } else {
                        g_warning (_("MateConf key %s set to type %s but its expected type was %s\n"),
                                   trans->mateconf_key,
                                   type_to_string (val->type),
                                   type_to_string (trans->mateconf_type));
                }
        }
}

static TranslationEntry *
find_translation_entry (const char *mateconf_key)
{
        int i;

        for (i = 0; i < G_N_ELEMENTS (translations); ++i) {
                if (strcmp (translations[i].mateconf_key, mateconf_key) == 0) {
                        return &translations[i];
                }
        }

        return NULL;
}

static void
xsettings_callback (MateConfClient           *client,
                    guint                  cnxn_id,
                    MateConfEntry            *entry,
                    MateXSettingsManager *manager)
{
        TranslationEntry *trans;
        int               i;

        trans = find_translation_entry (entry->key);
        if (trans == NULL) {
                return;
        }

        process_value (manager, trans, entry->value);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_string (manager->priv->managers [i],
                                              "Net/FallbackIconTheme",
                                              "mate");
        }

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_notify (manager->priv->managers [i]);
        }
}

static gchar *
get_gtk_modules (MateConfClient *client)
{
        GSList *entries, *l;
        GString *mods = g_string_new (NULL);

        entries = mateconf_client_all_entries (client, GTK_MODULES_DIR, NULL);

        for (l = entries; l != NULL; l = g_slist_next (l)) {
                MateConfEntry *e = l->data;
                MateConfValue *v = mateconf_entry_get_value (e);

                if (v != NULL) {
                        gboolean enabled = FALSE;
                        const gchar *key;

                        switch (v->type) {
                        case MATECONF_VALUE_BOOL:
                                /* simple enabled/disabled */
                                enabled = mateconf_value_get_bool (v);
                                break;

                        /* due to limitations in MateConf (or the client libraries,
                         * anyway), it is currently impossible to monitor
                         * arbitrary keys for changes, so these won't update at
                         * runtime */
                        case MATECONF_VALUE_STRING:
                                /* linked to another MateConf key of type bool */
                                key = mateconf_value_get_string (v);
                                if (key != NULL && mateconf_valid_key (key, NULL)) {
                                        enabled = mateconf_client_get_bool (client, key, NULL);
                                }
                                break;

                        default:
                                g_warning ("MateConf entry %s has invalid type %s",
                                           mateconf_entry_get_key (e), type_to_string (v->type));
                        }

                        if (enabled) {
                                const gchar *name;
                                name = strrchr (mateconf_entry_get_key (e), '/') + 1;

                                if (mods->len > 0) {
                                        g_string_append_c (mods, ':');
                                }
                                g_string_append (mods, name);
                        }
                }

                mateconf_entry_free (e);
        }

        g_slist_free (entries);

        return g_string_free (mods, mods->len == 0);
}

static void
gtk_modules_callback (MateConfClient           *client,
                      guint                  cnxn_id,
                      MateConfEntry            *entry,
                      MateXSettingsManager *manager)
{
        gchar *modules = get_gtk_modules (client);
        int i;

        if (modules == NULL) {
                for (i = 0; manager->priv->managers [i]; ++i) {
                        xsettings_manager_delete_setting (manager->priv->managers [i], "Gtk/Modules");
                }
        } else {
                g_debug ("Setting GTK modules '%s'", modules);
                for (i = 0; manager->priv->managers [i]; ++i) {
                        xsettings_manager_set_string (manager->priv->managers [i],
                                                      "Gtk/Modules",
                                                      modules);
                }
                g_free (modules);
        }

        for (i = 0; manager->priv->managers [i]; ++i) {
                xsettings_manager_notify (manager->priv->managers [i]);
        }
}

static guint
register_config_callback (MateXSettingsManager  *manager,
                          MateConfClient            *client,
                          const char             *path,
                          MateConfClientNotifyFunc   func)
{
        return mateconf_client_notify_add (client, path, func, manager, NULL, NULL);
}

static void
terminate_cb (void *data)
{
        gboolean *terminated = data;

        if (*terminated) {
                return;
        }

        *terminated = TRUE;

        gtk_main_quit ();
}

static gboolean
setup_xsettings_managers (MateXSettingsManager *manager)
{
        GdkDisplay *display;
        int         i;
        int         n_screens;
        gboolean    res;
        gboolean    terminated;

        display = gdk_display_get_default ();
        n_screens = gdk_display_get_n_screens (display);

        res = xsettings_manager_check_running (gdk_x11_display_get_xdisplay (display),
                                               gdk_screen_get_number (gdk_screen_get_default ()));
        if (res) {
                g_warning ("You can only run one xsettings manager at a time; exiting");
                return FALSE;
        }

        manager->priv->managers = g_new0 (XSettingsManager *, n_screens + 1);

        terminated = FALSE;
        for (i = 0; i < n_screens; i++) {
                GdkScreen *screen;

                screen = gdk_display_get_screen (display, i);

                manager->priv->managers [i] = xsettings_manager_new (gdk_x11_display_get_xdisplay (display),
                                                                     gdk_screen_get_number (screen),
                                                                     terminate_cb,
                                                                     &terminated);
                if (! manager->priv->managers [i]) {
                        g_warning ("Could not create xsettings manager for screen %d!", i);
                        return FALSE;
                }
        }

        return TRUE;
}

gboolean
mate_xsettings_manager_start (MateXSettingsManager *manager,
                               GError               **error)
{
        MateConfClient *client;
        int          i;

        g_debug ("Starting xsettings manager");
        mate_settings_profile_start (NULL);

        if (!setup_xsettings_managers (manager)) {
                g_set_error (error, GSD_XSETTINGS_ERROR,
                             GSD_XSETTINGS_ERROR_INIT,
                             "Could not initialize xsettings manager.");
                return FALSE;
        }

        client = mateconf_client_get_default ();

        mateconf_client_add_dir (client, MOUSE_SETTINGS_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        mateconf_client_add_dir (client, GTK_SETTINGS_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        mateconf_client_add_dir (client, INTERFACE_SETTINGS_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        mateconf_client_add_dir (client, SOUND_SETTINGS_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        mateconf_client_add_dir (client, GTK_MODULES_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        mateconf_client_add_dir (client, FONT_RENDER_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);

        for (i = 0; i < G_N_ELEMENTS (translations); i++) {
                MateConfValue *val;
                GError     *err;

                err = NULL;
                val = mateconf_client_get (client,
                                        translations[i].mateconf_key,
                                        &err);

                if (err != NULL) {
                        g_warning ("Error getting value for %s: %s",
                                   translations[i].mateconf_key,
                                   err->message);
                        g_error_free (err);
                } else {
                        process_value (manager, &translations[i], val);
                        if (val != NULL) {
                                mateconf_value_free (val);
                        }
                }
        }

        manager->priv->notify[0] =
                register_config_callback (manager, client,
                                          MOUSE_SETTINGS_DIR,
                                          (MateConfClientNotifyFunc) xsettings_callback);
        manager->priv->notify[1] =
                register_config_callback (manager, client,
                                          GTK_SETTINGS_DIR,
                                          (MateConfClientNotifyFunc) xsettings_callback);
        manager->priv->notify[2] =
                register_config_callback (manager, client,
                                          INTERFACE_SETTINGS_DIR,
                                          (MateConfClientNotifyFunc) xsettings_callback);
        manager->priv->notify[3] =
                register_config_callback (manager, client,
                                          SOUND_SETTINGS_DIR,
                                          (MateConfClientNotifyFunc) xsettings_callback);

        manager->priv->notify[4] =
                register_config_callback (manager, client,
                                          GTK_MODULES_DIR,
                                          (MateConfClientNotifyFunc) gtk_modules_callback);
        gtk_modules_callback (client, 0, NULL, manager);

#ifdef HAVE_FONTCONFIG
        manager->priv->notify[5] =
                register_config_callback (manager, client,
                                          FONT_RENDER_DIR,
                                          (MateConfClientNotifyFunc) xft_callback);
        update_xft_settings (manager, client);

        start_fontconfig_monitor (manager);
#endif /* HAVE_FONTCONFIG */

        g_object_unref (client);

        for (i = 0; manager->priv->managers [i]; i++)
                xsettings_manager_set_string (manager->priv->managers [i],
                                              "Net/FallbackIconTheme",
                                              "mate");

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_notify (manager->priv->managers [i]);
        }


        mate_settings_profile_end (NULL);

        return TRUE;
}

void
mate_xsettings_manager_stop (MateXSettingsManager *manager)
{
        MateXSettingsManagerPrivate *p = manager->priv;
        MateConfClient *client;
        int i;

        g_debug ("Stopping xsettings manager");

        if (p->managers != NULL) {
                for (i = 0; p->managers [i]; ++i)
                        xsettings_manager_destroy (p->managers [i]);

                g_free (p->managers);
                p->managers = NULL;
        }

        client = mateconf_client_get_default ();

        mateconf_client_remove_dir (client, MOUSE_SETTINGS_DIR, NULL);
        mateconf_client_remove_dir (client, GTK_SETTINGS_DIR, NULL);
        mateconf_client_remove_dir (client, INTERFACE_SETTINGS_DIR, NULL);
        mateconf_client_remove_dir (client, SOUND_SETTINGS_DIR, NULL);
        mateconf_client_remove_dir (client, GTK_MODULES_DIR, NULL);
#ifdef HAVE_FONTCONFIG
        mateconf_client_remove_dir (client, FONT_RENDER_DIR, NULL);

        stop_fontconfig_monitor (manager);
#endif /* HAVE_FONTCONFIG */

        for (i = 0; i < G_N_ELEMENTS (p->notify); ++i) {
                if (p->notify[i] != 0) {
                        mateconf_client_notify_remove (client, p->notify[i]);
                        p->notify[i] = 0;
                }
        }

        g_object_unref (client);
}

static void
mate_xsettings_manager_set_property (GObject        *object,
                                      guint           prop_id,
                                      const GValue   *value,
                                      GParamSpec     *pspec)
{
        MateXSettingsManager *self;

        self = MATE_XSETTINGS_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mate_xsettings_manager_get_property (GObject        *object,
                                      guint           prop_id,
                                      GValue         *value,
                                      GParamSpec     *pspec)
{
        MateXSettingsManager *self;

        self = MATE_XSETTINGS_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mate_xsettings_manager_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        MateXSettingsManager      *xsettings_manager;
        MateXSettingsManagerClass *klass;

        klass = MATE_XSETTINGS_MANAGER_CLASS (g_type_class_peek (MATE_TYPE_XSETTINGS_MANAGER));

        xsettings_manager = MATE_XSETTINGS_MANAGER (G_OBJECT_CLASS (mate_xsettings_manager_parent_class)->constructor (type,
                                                                                                                  n_construct_properties,
                                                                                                                  construct_properties));

        return G_OBJECT (xsettings_manager);
}

static void
mate_xsettings_manager_dispose (GObject *object)
{
        MateXSettingsManager *xsettings_manager;

        xsettings_manager = MATE_XSETTINGS_MANAGER (object);

        G_OBJECT_CLASS (mate_xsettings_manager_parent_class)->dispose (object);
}

static void
mate_xsettings_manager_class_init (MateXSettingsManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mate_xsettings_manager_get_property;
        object_class->set_property = mate_xsettings_manager_set_property;
        object_class->constructor = mate_xsettings_manager_constructor;
        object_class->dispose = mate_xsettings_manager_dispose;
        object_class->finalize = mate_xsettings_manager_finalize;

        g_type_class_add_private (klass, sizeof (MateXSettingsManagerPrivate));
}

static void
mate_xsettings_manager_init (MateXSettingsManager *manager)
{
        manager->priv = MATE_XSETTINGS_MANAGER_GET_PRIVATE (manager);
}

static void
mate_xsettings_manager_finalize (GObject *object)
{
        MateXSettingsManager *xsettings_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MATE_IS_XSETTINGS_MANAGER (object));

        xsettings_manager = MATE_XSETTINGS_MANAGER (object);

        g_return_if_fail (xsettings_manager->priv != NULL);

        G_OBJECT_CLASS (mate_xsettings_manager_parent_class)->finalize (object);
}

MateXSettingsManager *
mate_xsettings_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (MATE_TYPE_XSETTINGS_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return MATE_XSETTINGS_MANAGER (manager_object);
}
