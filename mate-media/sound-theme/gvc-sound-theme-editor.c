/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2008 William Jon McCann
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <canberra-gtk.h>

#include <mateconf/mateconf-client.h>

#include "gvc-sound-theme-editor.h"
#include "sound-theme-file-utils.h"

#define GVC_SOUND_THEME_EDITOR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_SOUND_THEME_EDITOR, GvcSoundThemeEditorPrivate))

struct GvcSoundThemeEditorPrivate
{
        GtkWidget *treeview;
        GtkWidget *theme_box;
        GtkWidget *selection_box;
        GtkWidget *click_feedback_button;
        MateConfClient *client;
        guint sounds_dir_id;
        guint marco_dir_id;
};

static void     gvc_sound_theme_editor_class_init (GvcSoundThemeEditorClass *klass);
static void     gvc_sound_theme_editor_init       (GvcSoundThemeEditor      *sound_theme_editor);
static void     gvc_sound_theme_editor_finalize   (GObject                  *object);

G_DEFINE_TYPE (GvcSoundThemeEditor, gvc_sound_theme_editor, GTK_TYPE_VBOX)

typedef enum {
        CATEGORY_INVALID,
        CATEGORY_BELL,
        CATEGORY_WINDOWS_BUTTONS,
        CATEGORY_DESKTOP,
        CATEGORY_ALERTS,
        NUM_CATEGORIES
} CategoryType;

typedef enum {
        SOUND_TYPE_NORMAL,
        SOUND_TYPE_AUDIO_BELL,
        SOUND_TYPE_FEEDBACK
} SoundType;

static struct {
        CategoryType category;
        SoundType    type;
        const char  *display_name;
        const char  *names[6];
} sounds[20] = {
        /* Bell */
        { CATEGORY_BELL, SOUND_TYPE_AUDIO_BELL, NC_("Sound event", "Alert sound"), { "bell-terminal", "bell-window-system", NULL } },
        /* Windows and buttons */
        { CATEGORY_WINDOWS_BUTTONS, -1, NC_("Sound event", "Windows and Buttons"), { NULL } },
        { CATEGORY_WINDOWS_BUTTONS, SOUND_TYPE_FEEDBACK, NC_("Sound event", "Button clicked"), { "button-pressed", "menu-click", "menu-popup", "menu-popdown", "menu-replace", NULL } },
        { CATEGORY_WINDOWS_BUTTONS, SOUND_TYPE_FEEDBACK, NC_("Sound event", "Toggle button clicked"), { "button-toggle-off", "button-toggle-on", NULL } },
        { CATEGORY_WINDOWS_BUTTONS, SOUND_TYPE_FEEDBACK, NC_("Sound event", "Window maximized"), { "window-maximized", NULL } },
        { CATEGORY_WINDOWS_BUTTONS, SOUND_TYPE_FEEDBACK, NC_("Sound event", "Window unmaximized"), { "window-unmaximized", NULL } },
        { CATEGORY_WINDOWS_BUTTONS, SOUND_TYPE_FEEDBACK, NC_("Sound event", "Window minimised"), { "window-minimized", NULL } },
        /* Desktop */
        { CATEGORY_DESKTOP, -1, NC_("Sound event", "Desktop"), { NULL } },
        { CATEGORY_DESKTOP, SOUND_TYPE_NORMAL, NC_("Sound event", "Login"), { "desktop-login", NULL } },
        { CATEGORY_DESKTOP, SOUND_TYPE_NORMAL, NC_("Sound event", "Logout"), { "desktop-logout", NULL } },
        { CATEGORY_DESKTOP, SOUND_TYPE_NORMAL, NC_("Sound event", "New e-mail"), { "message-new-email", NULL } },
        { CATEGORY_DESKTOP, SOUND_TYPE_NORMAL, NC_("Sound event", "Empty trash"), { "trash-empty", NULL } },
        { CATEGORY_DESKTOP, SOUND_TYPE_NORMAL, NC_("Sound event", "Long action completed (download, CD burning, etc.)"), { "complete-copy", "complete-download", "complete-media-burn", "complete-media-rip", "complete-scan", NULL } },
        /* Alerts? */
        { CATEGORY_ALERTS, -1, NC_("Sound event", "Alerts"), { NULL } },
        { CATEGORY_ALERTS, SOUND_TYPE_NORMAL, NC_("Sound event", "Information or question"), { "dialog-information", "dialog-question", NULL } },
        { CATEGORY_ALERTS, SOUND_TYPE_NORMAL, NC_("Sound event", "Warning"), { "dialog-warning", NULL } },
        { CATEGORY_ALERTS, SOUND_TYPE_NORMAL, NC_("Sound event", "Error"), { "dialog-error", NULL } },
        { CATEGORY_ALERTS, SOUND_TYPE_NORMAL, NC_("Sound event", "Battery warning"), { "power-unplug-battery-low", "battery-low", "battery-caution", NULL } },
        /* Finish off */
        { -1, -1, NULL, { NULL } }
};

#define KEY_SOUNDS_DIR             "/desktop/mate/sound"
#define EVENT_SOUNDS_KEY           KEY_SOUNDS_DIR "/event_sounds"
#define INPUT_SOUNDS_KEY           KEY_SOUNDS_DIR "/input_feedback_sounds"
#define SOUND_THEME_KEY            KEY_SOUNDS_DIR "/theme_name"
#define KEY_MARCO_DIR           "/apps/marco/general"
#define AUDIO_BELL_KEY             KEY_MARCO_DIR "/audible_bell"

#define CUSTOM_THEME_NAME       "__custom"
#define NO_SOUNDS_THEME_NAME    "__no_sounds"
#define PREVIEW_BUTTON_XPAD     5

enum {
        THEME_DISPLAY_COL,
        THEME_IDENTIFIER_COL,
        THEME_PARENT_ID_COL,
        THEME_NUM_COLS
};

enum {
        SOUND_UNSET,
        SOUND_OFF,
        SOUND_BUILTIN,
        SOUND_CUSTOM,
        SOUND_CUSTOM_OLD
};

enum {
        DISPLAY_COL,
        SETTING_COL,
        TYPE_COL,
        SENSITIVE_COL,
        HAS_PREVIEW_COL,
        FILENAME_COL,
        SOUND_NAMES_COL,
        NUM_COLS
};

static gboolean
theme_changed_custom_reinit (GtkTreeModel *model,
                             GtkTreePath  *path,
                             GtkTreeIter  *iter,
                             gpointer      data)
{
        int      type;
        gboolean sensitive;

        gtk_tree_model_get (model,
                            iter,
                            TYPE_COL, &type,
                            SENSITIVE_COL, &sensitive, -1);
        if (type != -1) {
                gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                                    SETTING_COL, SOUND_BUILTIN,
                                    HAS_PREVIEW_COL, sensitive,
                                    -1);
        }
        return FALSE;
}

static void
on_theme_changed ()
{
        /* Don't reinit a custom theme */
        if (strcmp (theme_name, CUSTOM_THEME_NAME) != 0) {
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor->priv->treeview));
                gtk_tree_model_foreach (model, theme_changed_custom_reinit, NULL);

                /* Delete the custom dir */
                delete_custom_theme_dir ();

                /* And the combo box entry */
                model = gtk_combo_box_get_model (GTK_COMBO_BOX (editor->priv->combo_box));
                gtk_tree_model_get_iter_first (model, &iter);
                do {
                        char *parent;
                        gtk_tree_model_get (model, &iter, THEME_PARENT_ID_COL, &parent, -1);
                        if (parent != NULL && strcmp (parent, CUSTOM_THEME_NAME) != 0) {
                                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
                                g_free (parent);
                                break;
                        }
                        g_free (parent);
                } while (gtk_tree_model_iter_next (model, &iter));
        }
}

static char *
load_index_theme_name (const char *index,
                       char      **parent)
{
        GKeyFile *file;
        char *indexname = NULL;
        gboolean hidden;

        file = g_key_file_new ();
        if (g_key_file_load_from_file (file, index, G_KEY_FILE_KEEP_TRANSLATIONS, NULL) == FALSE) {
                g_key_file_free (file);
                return NULL;
        }
        /* Don't add hidden themes to the list */
        hidden = g_key_file_get_boolean (file, "Sound Theme", "Hidden", NULL);
        if (!hidden) {
                indexname = g_key_file_get_locale_string (file,
                                                          "Sound Theme",
                                                          "Name",
                                                          NULL,
                                                          NULL);

                /* Save the parent theme, if there's one */
                if (parent != NULL) {
                        *parent = g_key_file_get_string (file,
                                                         "Sound Theme",
                                                         "Inherits",
                                                         NULL);
                }
        }

        g_key_file_free (file);
        return indexname;
}

static void
sound_theme_in_dir (GHashTable *hash,
                    const char *dir)
{
        GDir *d;
        const char *name;

        d = g_dir_open (dir, 0, NULL);
        if (d == NULL) {
                return;
        }

        while ((name = g_dir_read_name (d)) != NULL) {
                char *dirname, *index, *indexname;

                /* Look for directories */
                dirname = g_build_filename (dir, name, NULL);
                if (g_file_test (dirname, G_FILE_TEST_IS_DIR) == FALSE) {
                        g_free (dirname);
                        continue;
                }

                /* Look for index files */
                index = g_build_filename (dirname, "index.theme", NULL);
                g_free (dirname);

                /* Check the name of the theme in the index.theme file */
                indexname = load_index_theme_name (index, NULL);
                g_free (index);
                if (indexname == NULL) {
                        continue;
                }

                g_hash_table_insert (hash, g_strdup (name), indexname);
        }

        g_dir_close (d);
}

static void
add_theme_to_store (const char   *key,
                    const char   *value,
                    GtkListStore *store)
{
        char *parent;

        parent = NULL;

        /* Get the parent, if we're checking the custom theme */
        if (strcmp (key, CUSTOM_THEME_NAME) == 0) {
                char *name, *path;

                path = custom_theme_dir_path ("index.theme");
                name = load_index_theme_name (path, &parent);
                g_free (name);
                g_free (path);
        }
        gtk_list_store_insert_with_values (store, NULL, G_MAXINT,
                                           THEME_DISPLAY_COL, value,
                                           THEME_IDENTIFIER_COL, key,
                                           THEME_PARENT_ID_COL, parent,
                                           -1);
        g_free (parent);
}

static void
set_theme_name (GvcSoundThemeEditor *editor,
                const char           *name)
{
        MateConfClient *client;

        g_debug ("setting theme %s", name ? name : "(null)");

        /* If the name is empty, use "freedesktop" */
        if (name == NULL || *name == '\0') {
                name = "freedesktop";
        }

        mateconf_client_set_string (editor->priv->client, SOUND_THEME_KEY, theme_name, NULL);
}

/* Functions to toggle whether the audible bell sound is editable */
static gboolean
audible_bell_foreach (GtkTreeModel *model,
                      GtkTreePath  *path,
                      GtkTreeIter  *iter,
                      gpointer      data)
{
        int      type;
        int      setting;
        gboolean enabled = GPOINTER_TO_INT (data);

        setting = enabled ? SOUND_BUILTIN : SOUND_OFF;

        gtk_tree_model_get (model, iter, TYPE_COL, &type, -1);
        if (type == SOUND_TYPE_AUDIO_BELL) {
                gtk_tree_store_set (GTK_TREE_STORE (model),
                                    iter,
                                    SETTING_COL, setting,
                                    HAS_PREVIEW_COL, enabled,
                                    -1);
                return TRUE;
        }
        return FALSE;
}

static void
set_audible_bell_enabled (GvcSoundThemeEditor *editor,
                          gboolean              enabled)
{
        GtkTreeModel *model;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor->priv->treeview));
        gtk_tree_model_foreach (model, audible_bell_foreach, GINT_TO_POINTER (enabled));
}

/* Functions to toggle whether the Input feedback sounds are editable */
static gboolean
input_feedback_foreach (GtkTreeModel *model,
                        GtkTreePath  *path,
                        GtkTreeIter  *iter,
                        gpointer      data)
{
        int      type;
        gboolean enabled = GPOINTER_TO_INT (data);

        gtk_tree_model_get (model, iter, TYPE_COL, &type, -1);
        if (type == SOUND_TYPE_FEEDBACK) {
                gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                                    SENSITIVE_COL, enabled,
                                    HAS_PREVIEW_COL, enabled,
                                    -1);
        }
        return FALSE;
}

static void
set_input_feedback_enabled (GvcSoundThemeEditor *editor,
                            gboolean              enabled)
{
        GtkTreeModel *model;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->priv->click_feedback_button),
                                      enabled);

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor->priv->treeview));
        gtk_tree_model_foreach (model, input_feedback_foreach, GINT_TO_POINTER (enabled));
}

static int
get_file_type (const char *sound_name,
               char      **linked_name)
{
        char *name, *filename;

        *linked_name = NULL;

        name = g_strdup_printf ("%s.disabled", sound_name);
        filename = custom_theme_dir_path (name);
        g_free (name);

        if (g_file_test (filename, G_FILE_TEST_IS_REGULAR) != FALSE) {
                g_free (filename);
                return SOUND_OFF;
        }
        g_free (filename);

        /* We only check for .ogg files because those are the
         * only ones we create */
        name = g_strdup_printf ("%s.ogg", sound_name);
        filename = custom_theme_dir_path (name);
        g_free (name);

        if (g_file_test (filename, G_FILE_TEST_IS_SYMLINK) != FALSE) {
                *linked_name = g_file_read_link (filename, NULL);
                g_free (filename);
                return SOUND_CUSTOM;
        }
        g_free (filename);

        return SOUND_BUILTIN;
}

static gboolean
theme_changed_custom_init (GtkTreeModel *model,
                           GtkTreePath *path,
                           GtkTreeIter *iter,
                           gpointer data)
{
        char **sound_names;

        gtk_tree_model_get (model, iter, SOUND_NAMES_COL, &sound_names, -1);
        if (sound_names != NULL) {
                char *filename;
                int type;

                type = get_file_type (sound_names[0], &filename);

                gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                                    SETTING_COL, type,
                                    HAS_PREVIEW_COL, type != SOUND_OFF,
                                    FILENAME_COL, filename,
                                    -1);
                g_strfreev (sound_names);
                g_free (filename);
        }
        return FALSE;
}

static void
update_theme (GvcSoundThemeEditor *editor)
{
        char        *theme_name;
        gboolean     events_enabled;
        gboolean     bell_enabled;
        MateConfClient *client;
        gboolean     feedback_enabled;

        client = editor->priv->client;

        bell_enabled = mateconf_client_get_bool (client, AUDIO_BELL_KEY, NULL);
        set_audible_bell_enabled (editor, bell_enabled);

        feedback_enabled = mateconf_client_get_bool (client, INPUT_SOUNDS_KEY, NULL);
        set_input_feedback_enabled (editor, feedback_enabled);

        events_enabled = mateconf_client_get_bool (client, EVENT_SOUNDS_KEY, NULL);
        if (events_enabled) {
                theme_name = mateconf_client_get_string (client, SOUND_THEME_KEY, NULL);
        } else {
                theme_name = g_strdup (NO_SOUNDS_THEME_NAME);
        }

        gtk_widget_set_sensitive (editor->priv->selection_box, events_enabled);

        set_theme_name (editor, theme_name);

        /* Setup the default values if we're using the custom theme */
        if (theme_name != NULL && strcmp (theme_name, CUSTOM_THEME_NAME) == 0) {
                GtkTreeModel *model;
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor->priv->treeview));
                gtk_tree_model_foreach (model,
                                        theme_changed_custom_init,
                                        NULL);
        }
        g_free (theme_name);
}

static void
setup_theme_selector (GvcSoundThemeEditor *editor)
{
        GHashTable           *hash;
        GtkListStore         *store;
        GtkCellRenderer      *renderer;
        const char * const   *data_dirs;
        const char           *data_dir;
        char                 *dir;
        guint                 i;

        /* Add the theme names and their display name to a hash table,
         * makes it easy to avoid duplicate themes */
        hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

        data_dirs = g_get_system_data_dirs ();
        for (i = 0; data_dirs[i] != NULL; i++) {
                dir = g_build_filename (data_dirs[i], "sounds", NULL);
                sound_theme_in_dir (hash, dir);
                g_free (dir);
        }

        data_dir = g_get_user_data_dir ();
        dir = g_build_filename (data_dir, "sounds", NULL);
        sound_theme_in_dir (hash, dir);
        g_free (dir);

        /* If there isn't at least one theme, make everything
         * insensitive, LAME! */
        if (g_hash_table_size (hash) == 0) {
                gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
                g_warning ("Bad setup, install the freedesktop sound theme");
                g_hash_table_destroy (hash);
                return;
        }

        /* Setup the tree model, 3 columns:
         * - internal theme name/directory
         * - display theme name
         * - the internal id for the parent theme, used for the custom theme */
        store = gtk_list_store_new (THEME_NUM_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING);

        /* Add the themes to a combobox */
        gtk_list_store_insert_with_values (store,
                                           NULL,
                                           G_MAXINT,
                                           THEME_DISPLAY_COL, _("No sounds"),
                                           THEME_IDENTIFIER_COL, "__no_sounds",
                                           THEME_PARENT_ID_COL, NULL,
                                           -1);
        g_hash_table_foreach (hash, (GHFunc) add_theme_to_store, store);
        g_hash_table_destroy (hash);

        /* Set the display */
        gtk_combo_box_set_model (GTK_COMBO_BOX (editor->priv->combo_box),
                                 GTK_TREE_MODEL (store));

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (editor->priv->combo_box),
                                    renderer,
                                    TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (editor->priv->combo_box),
                                        renderer,
                                        "text", THEME_DISPLAY_COL,
                                        NULL);

        g_signal_connect (editor->priv->combo_box,
                          "changed",
                          G_CALLBACK (on_combobox_changed),
                          editor);
}

static void
play_sound_preview (GtkFileEditor *editor,
                    gpointer user_data)
{
        char       *filename;

        filename = gtk_file_editor_get_preview_filename (GTK_FILE_EDITOR (editor));
        if (filename == NULL) {
                return;
        }

        ca_gtk_play_for_widget (GTK_WIDGET (editor), 0,
                                CA_PROP_APPLICATION_NAME, _("Sound Preferences"),
                                CA_PROP_MEDIA_FILENAME, filename,
                                CA_PROP_EVENT_DESCRIPTION, _("Testing event sound"),
                                CA_PROP_CANBERRA_CACHE_CONTROL, "never",
                                CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
#ifdef CA_PROP_CANBERRA_ENABLE
                                CA_PROP_CANBERRA_ENABLE, "1",
#endif
                                NULL);
        g_free (filename);
}

static char *
get_sound_filename (GvcSoundThemeEditor *editor)
{
        GtkWidget          *file_editor;
        GtkWidget          *toplevel;
        GtkWindow          *parent;
        int                 response;
        char               *filename;
        char               *path;
        const char * const *data_dirs, *data_dir;
        GtkFileFilter      *filter;
        guint               i;

	/* Try to get the parent window of the widget */
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (editor));
	if (gtk_widget_is_toplevel (toplevel) != FALSE)
		parent = GTK_WINDOW (toplevel);
	else
		parent = NULL;

        file_editor = gtk_file_editor_dialog_new (_("Select Sound File"),
                                                    parent,
                                                    GTK_FILE_EDITOR_ACTION_OPEN,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                    NULL);

        gtk_file_editor_set_local_only (GTK_FILE_EDITOR (file_editor), TRUE);
        gtk_file_editor_set_select_multiple (GTK_FILE_EDITOR (file_editor), FALSE);

        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, _("Sound files"));
        gtk_file_filter_add_mime_type (filter, "audio/x-vorbis+ogg");
        gtk_file_filter_add_mime_type (filter, "audio/x-wav");
        gtk_file_editor_add_filter (GTK_FILE_EDITOR (file_editor), filter);
        gtk_file_editor_set_filter (GTK_FILE_EDITOR (file_editor), filter);

        g_signal_connect (file_editor, "update-preview",
                          G_CALLBACK (play_sound_preview), NULL);

        data_dirs = g_get_system_data_dirs ();
        for (i = 0; data_dirs[i] != NULL; i++) {
                path = g_build_filename (data_dirs[i], "sounds", NULL);
                gtk_file_editor_add_shortcut_folder (GTK_FILE_EDITOR (file_editor), path, NULL);
                g_free (path);
        }
        data_dir = g_get_user_special_dir (G_USER_DIRECTORY_MUSIC);
        if (data_dir != NULL)
                gtk_file_editor_add_shortcut_folder (GTK_FILE_EDITOR (file_editor), data_dir, NULL);

        gtk_file_editor_set_current_folder (GTK_FILE_EDITOR (file_editor), SOUND_DATA_DIR);

        response = gtk_dialog_run (GTK_DIALOG (file_editor));
        filename = NULL;
        if (response == GTK_RESPONSE_ACCEPT)
                filename = gtk_file_editor_get_filename (GTK_FILE_EDITOR (file_editor));

        gtk_widget_destroy (file_editor);

        return filename;
}


static gboolean
count_customised_sounds (GtkTreeModel *model,
                         GtkTreePath  *path,
                         GtkTreeIter  *iter,
                         int          *num_custom)
{
        int type;
        int setting;

        gtk_tree_model_get (model, iter, TYPE_COL, &type, SETTING_COL, &setting, -1);
        if (setting == SOUND_OFF || setting == SOUND_CUSTOM || setting == SOUND_CUSTOM_OLD) {
                (*num_custom)++;
        }

        return FALSE;
}

static gboolean
save_sounds (GtkTreeModel *model,
             GtkTreePath  *path,
             GtkTreeIter  *iter,
             gpointer      data)
{
        int    type;
        int    setting;
        char  *filename;
        char **sounds;

        gtk_tree_model_get (model, iter,
                            TYPE_COL, &type,
                            SETTING_COL, &setting,
                            FILENAME_COL, &filename,
                            SOUND_NAMES_COL, &sounds,
                            -1);

        if (setting == SOUND_BUILTIN) {
                delete_old_files (sounds);
                delete_disabled_files (sounds);
        } else if (setting == SOUND_OFF) {
                delete_old_files (sounds);
                add_disabled_file (sounds);
        } else if (setting == SOUND_CUSTOM || setting == SOUND_CUSTOM_OLD) {
                delete_old_files (sounds);
                delete_disabled_files (sounds);
                add_custom_file (sounds, filename);
        }
        g_free (filename);
        g_strfreev (sounds);

        return FALSE;
}

static void
save_custom_theme (GtkTreeModel *model,
                   const char   *parent)
{
        GKeyFile *keyfile;
        char     *data;
        char     *path;

        /* Create the custom directory */
        path = custom_theme_dir_path (NULL);
        g_mkdir_with_parents (path, 0755);
        g_free (path);

        /* Save the sounds themselves */
        gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) save_sounds, NULL);

        /* Set the data for index.theme */
        keyfile = g_key_file_new ();
        g_key_file_set_string (keyfile, "Sound Theme", "Name", _("Custom"));
        g_key_file_set_string (keyfile, "Sound Theme", "Inherits", parent);
        g_key_file_set_string (keyfile, "Sound Theme", "Directories", ".");
        data = g_key_file_to_data (keyfile, NULL, NULL);
        g_key_file_free (keyfile);

        /* Save the index.theme */
        path = custom_theme_dir_path ("index.theme");
        g_file_set_contents (path, data, -1, NULL);
        g_free (path);
        g_free (data);

        custom_theme_update_time ();
}

static void
dump_theme (GvcSoundThemeEditor *editor)
{
        int           num_custom;
        GtkTreeModel *model;
        GtkTreeIter   iter;
        char         *parent;

        num_custom = 0;
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor->priv->treeview));
        gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) count_customised_sounds, &num_custom);

        g_debug ("%d customised sounds", num_custom);

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (editor->priv->combo_box));
        /* Get the current theme's name, and set the parent */
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (editor->priv->combo_box), &iter) == FALSE)
                return;

        if (num_custom == 0) {
                gtk_tree_model_get (model, &iter, THEME_PARENT_ID_COL, &parent, -1);
                if (parent != NULL) {
                        set_theme_name (editor, parent);
                        g_free (parent);
                }
                gtk_tree_model_get_iter_first (model, &iter);
                do {
                        gtk_tree_model_get (model, &iter, THEME_PARENT_ID_COL, &parent, -1);
                        if (parent != NULL && strcmp (parent, CUSTOM_THEME_NAME) != 0) {
                                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
                                break;
                        }
                } while (gtk_tree_model_iter_next (model, &iter));

                delete_custom_theme_dir ();
        } else {
                gtk_tree_model_get (model, &iter, THEME_IDENTIFIER_COL, &parent, -1);
                if (strcmp (parent, CUSTOM_THEME_NAME) != 0) {
                        gtk_list_store_insert_with_values (GTK_LIST_STORE (model), NULL, G_MAXINT,
                                                           THEME_DISPLAY_COL, _("Custom"),
                                                           THEME_IDENTIFIER_COL, CUSTOM_THEME_NAME,
                                                           THEME_PARENT_ID_COL, parent,
                                                           -1);
                } else {
                        g_free (parent);
                        gtk_tree_model_get (model, &iter, THEME_PARENT_ID_COL, &parent, -1);
                }

                g_debug ("The parent theme is: %s", parent);
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor->priv->treeview));
                save_custom_theme (model, parent);
                g_free (parent);

                set_theme_name (editor, CUSTOM_THEME_NAME);
        }
}

static void
on_setting_column_edited (GtkCellRendererText  *renderer,
                          char                 *path,
                          char                 *new_text,
                          GvcSoundThemeEditor *editor)
{
        GtkTreeModel *model;
        GtkTreeModel *tree_model;
        GtkTreeIter   iter;
        GtkTreeIter   tree_iter;
        SoundType     type;
        char         *text;
        char         *old_filename;
        int           setting;

        if (new_text == NULL) {
                return;
        }

        g_object_get (renderer,
                      "model", &model,
                      NULL);

        tree_model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor->priv->treeview));
        if (gtk_tree_model_get_iter_from_string (tree_model, &tree_iter, path) == FALSE)
                return;

        gtk_tree_model_get (tree_model, &tree_iter,
                            TYPE_COL, &type,
                            FILENAME_COL, &old_filename,
                            -1);

        gtk_tree_model_get_iter_first (model, &iter);
        do {
                int cmp;

                gtk_tree_model_get (model, &iter,
                                    0, &text,
                                    1, &setting,
                                    -1);
                cmp = g_utf8_collate (text, new_text);
                g_free (text);

                if (cmp != 0) {
                        continue;
                }

                if (type == SOUND_TYPE_NORMAL
                    || type == SOUND_TYPE_FEEDBACK
                    || type == SOUND_TYPE_AUDIO_BELL) {

                        if (setting == SOUND_CUSTOM
                            || (setting == SOUND_CUSTOM_OLD
                                && old_filename == NULL)) {

                                char *filename = get_sound_filename (editor);

                                if (filename == NULL) {
                                        break;
                                }
                                gtk_tree_store_set (GTK_TREE_STORE (tree_model),
                                                    &tree_iter,
                                                    SETTING_COL, setting,
                                                    HAS_PREVIEW_COL, setting != SOUND_OFF,
                                                    FILENAME_COL, filename,
                                                    -1);
                                g_free (filename);
                        } else if (setting == SOUND_CUSTOM_OLD) {
                                gtk_tree_store_set (GTK_TREE_STORE (tree_model),
                                                    &tree_iter,
                                                    SETTING_COL, setting,
                                                    HAS_PREVIEW_COL, setting != SOUND_OFF,
                                                    FILENAME_COL, old_filename,
                                                    -1);
                        } else {
                                gtk_tree_store_set (GTK_TREE_STORE (tree_model),
                                                    &tree_iter,
                                                    SETTING_COL, setting,
                                                    HAS_PREVIEW_COL, setting != SOUND_OFF,
                                                    -1);
                        }

                        g_debug ("Something changed, dump theme");
                        dump_theme (editor);

                        break;
                }

                g_assert_not_reached ();

        } while (gtk_tree_model_iter_next (model, &iter));

        g_free (old_filename);
}

static void
fill_custom_model (GtkListStore *store,
                   const char   *prev_filename)
{
        GtkTreeIter iter;

        gtk_list_store_clear (store);

        if (prev_filename != NULL) {
                char *display;
                display = g_filename_display_basename (prev_filename);
                gtk_list_store_insert_with_values (store, &iter, G_MAXINT,
                                                   0, display,
                                                   1, SOUND_CUSTOM_OLD,
                                                   -1);
                g_free (display);
        }

        gtk_list_store_insert_with_values (store, &iter, G_MAXINT,
                                           0, _("Default"),
                                           1, SOUND_BUILTIN,
                                           -1);
        gtk_list_store_insert_with_values (store, &iter, G_MAXINT,
                                           0, _("Disabled"),
                                           1, SOUND_OFF,
                                           -1);
        gtk_list_store_insert_with_values (store, &iter, G_MAXINT,
                                           0, _("Custom…"),
                                           1, SOUND_CUSTOM, -1);
}

static void
on_combobox_editing_started (GtkCellRenderer      *renderer,
                             GtkCellEditable      *editable,
                             gchar                *path,
                             GvcSoundThemeEditor *editor)
{
        GtkTreeModel *model;
        GtkTreeModel *store;
        GtkTreeIter   iter;
        SoundType     type;
        char         *filename;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor->priv->treeview));
        if (gtk_tree_model_get_iter_from_string (model, &iter, path) == FALSE) {
                return;
        }

        gtk_tree_model_get (model, &iter, TYPE_COL, &type, FILENAME_COL, &filename, -1);
        g_object_get (renderer, "model", &store, NULL);
        fill_custom_model (GTK_LIST_STORE (store), filename);
        g_free (filename);
}

static gboolean
play_sound_at_path (GtkWidget         *tree_view,
                    GtkTreePath       *path)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        char        **sound_names;
        gboolean      sensitive;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));
        if (gtk_tree_model_get_iter (model, &iter, path) == FALSE) {
                return FALSE;
        }

        gtk_tree_model_get (model, &iter,
                            SOUND_NAMES_COL, &sound_names,
                            SENSITIVE_COL, &sensitive,
                            -1);
        if (!sensitive || sound_names == NULL) {
                return FALSE;
        }

        ca_gtk_play_for_widget (GTK_WIDGET (tree_view), 0,
                                CA_PROP_APPLICATION_NAME, _("Sound Preferences"),
                                CA_PROP_EVENT_ID, sound_names[0],
                                CA_PROP_EVENT_DESCRIPTION, _("Testing event sound"),
                                CA_PROP_CANBERRA_CACHE_CONTROL, "never",
                                CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
#ifdef CA_PROP_CANBERRA_ENABLE
                                CA_PROP_CANBERRA_ENABLE, "1",
#endif
                                NULL);

        g_strfreev (sound_names);

        return TRUE;
}

static void
setting_set_func (GtkTreeViewColumn *tree_column,
                  GtkCellRenderer   *cell,
                  GtkTreeModel      *model,
                  GtkTreeIter       *iter,
                  gpointer           data)
{
        int       setting;
        char     *filename;
        SoundType type;

        gtk_tree_model_get (model, iter,
                            SETTING_COL, &setting,
                            FILENAME_COL, &filename,
                            TYPE_COL, &type,
                            -1);

        if (setting == SOUND_UNSET) {
                g_object_set (cell,
                              "visible", FALSE,
                              NULL);
                g_free (filename);
                return;
        }

        if (setting == SOUND_OFF) {
                g_object_set (cell,
                              "text", _("Disabled"),
                              NULL);
        } else if (setting == SOUND_BUILTIN) {
                g_object_set (cell,
                              "text", _("Default"),
                              NULL);
        } else if (setting == SOUND_CUSTOM || setting == SOUND_CUSTOM_OLD) {
                char *display;

                display = g_filename_display_basename (filename);
                g_object_set (cell,
                              "text", display,
                              NULL);
                g_free (display);
        }

        g_free (filename);
}

typedef GtkCellRendererPixbuf      ActivatableCellRendererPixbuf;
typedef GtkCellRendererPixbufClass ActivatableCellRendererPixbufClass;

GType activatable_cell_renderer_pixbuf_get_type (void);
#define ACTIVATABLE_TYPE_CELL_RENDERER_PIXBUF (activatable_cell_renderer_pixbuf_get_type ())
G_DEFINE_TYPE (ActivatableCellRendererPixbuf, activatable_cell_renderer_pixbuf, GTK_TYPE_CELL_RENDERER_PIXBUF);

static gboolean
activatable_cell_renderer_pixbuf_activate (GtkCellRenderer      *cell,
                                           GdkEvent             *event,
                                           GtkWidget            *widget,
                                           const gchar          *path_string,
                                           GdkRectangle         *background_area,
                                           GdkRectangle         *cell_area,
                                           GtkCellRendererState  flags)
{
        GtkTreePath *path;
        gboolean     res;

        g_debug ("Activating pixbuf");

        path = gtk_tree_path_new_from_string (path_string);
        res = play_sound_at_path (widget, path);
        gtk_tree_path_free (path);

        return res;
}

static void
activatable_cell_renderer_pixbuf_init (ActivatableCellRendererPixbuf *cell)
{
}

static void
activatable_cell_renderer_pixbuf_class_init (ActivatableCellRendererPixbufClass *class)
{
        GtkCellRendererClass *cell_class;

        cell_class = GTK_CELL_RENDERER_CLASS (class);

        cell_class->activate = activatable_cell_renderer_pixbuf_activate;
}

static void
setup_theme_custom_selector (GvcSoundThemeEditor *editor,
                             gboolean              have_xkb )
{
        GtkTreeStore      *store;
        GtkTreeModel      *custom_model;
        GtkTreeViewColumn *column;
        GtkCellRenderer   *renderer;
        GtkTreeIter        iter;
        GtkTreeIter        parent;
        CategoryType       type;
        guint              i;

        /* Set up the model for the custom view */
        store = gtk_tree_store_new (NUM_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_INT,
                                    G_TYPE_INT,
                                    G_TYPE_BOOLEAN,
                                    G_TYPE_BOOLEAN,
                                    G_TYPE_STRING,
                                    G_TYPE_STRV);

        /* The first column with the categories/sound names */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes ("Display", renderer,
                                                           "text", DISPLAY_COL,
                                                           "sensitive", SENSITIVE_COL,
							   "ellipsize", PANGO_ELLIPSIZE_START,
							   "ellipsize-set", TRUE,
							   NULL);
	g_object_set (G_OBJECT (column), "expand", TRUE, NULL);

        gtk_tree_view_append_column (GTK_TREE_VIEW (editor->priv->treeview), column);

        /* The 2nd column with the sound settings */
        renderer = gtk_cell_renderer_combo_new ();
        g_signal_connect (renderer,
                          "edited",
                          G_CALLBACK (on_setting_column_edited),
                          editor);
        g_signal_connect (renderer,
                          "editing-started",
                          G_CALLBACK (on_combobox_editing_started),
                          editor);
        custom_model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));
        fill_custom_model (GTK_LIST_STORE (custom_model), NULL);

        g_object_set (renderer,
                      "model", custom_model,
                      "has-entry", FALSE,
                      "editable", TRUE,
                      "text-column", 0,
                      NULL);
        column = gtk_tree_view_column_new_with_attributes ("Setting", renderer,
                                                           "editable", SENSITIVE_COL,
                                                           "sensitive", SENSITIVE_COL,
                                                           "visible", TRUE,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (editor->priv->treeview), column);
        gtk_tree_view_column_set_cell_data_func (column, renderer, setting_set_func, NULL, NULL);

        /* The 3rd column with the preview pixbuf */
        renderer = g_object_new (ACTIVATABLE_TYPE_CELL_RENDERER_PIXBUF, NULL);
        g_object_set (renderer,
                      "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
                      "icon-name", "media-playback-start",
                      "stock-size", GTK_ICON_SIZE_MENU,
                      NULL);
        column = gtk_tree_view_column_new_with_attributes ("Preview", renderer,
                                                           "visible", HAS_PREVIEW_COL,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (editor->priv->treeview), column);
        g_object_set_data (G_OBJECT (editor->priv->treeview), "preview-column", column);

        gtk_tree_view_set_model (GTK_TREE_VIEW (editor->priv->treeview), GTK_TREE_MODEL (store));
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (editor->priv->treeview), FALSE);

        /* Fill in the model */
        type = CATEGORY_INVALID;

        for (i = 0; ; i++) {
                GtkTreeIter *_parent;

                if (sounds[i].category == -1) {
                        break;
                }

                /* Is it a new type of sound? */
                if (sounds[i].category == type
                    && type != CATEGORY_INVALID
                    && type != CATEGORY_BELL) {
                        _parent = &parent;
                } else {
                        _parent = NULL;
                }

                if (sounds[i].type != -1) {
                        gtk_tree_store_insert_with_values (store, &iter, _parent, G_MAXINT,
                                                           DISPLAY_COL, g_dpgettext2 (NULL, "Sound event", sounds[i].display_name),
                                                           SETTING_COL, SOUND_BUILTIN,
                                                           TYPE_COL, sounds[i].type,
                                                           SOUND_NAMES_COL, sounds[i].names,
                                                           HAS_PREVIEW_COL, TRUE,
                                                           SENSITIVE_COL, TRUE,
                                                           -1);
                } else {
                        /* Category */
                        gtk_tree_store_insert_with_values (store, &iter, _parent, G_MAXINT,
                                                           DISPLAY_COL, g_dpgettext2 (NULL, "Sound event", sounds[i].display_name),
                                                           SETTING_COL, SOUND_UNSET,
                                                           TYPE_COL, sounds[i].type,
                                                           SENSITIVE_COL, TRUE,
                                                           HAS_PREVIEW_COL, FALSE,
                                                           -1);
                }

                /* If we didn't set a parent already, set one in case we need it later */
                if (_parent == NULL) {
                        parent = iter;
                }
                type = sounds[i].category;
        }

        gtk_tree_view_expand_all (GTK_TREE_VIEW (editor->priv->treeview));
}

static GObject *
gvc_sound_theme_editor_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_params)
{
        GObject              *object;
        GvcSoundThemeEditor *self;

        object = G_OBJECT_CLASS (gvc_sound_theme_editor_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_SOUND_THEME_EDITOR (object);

        setup_theme_selector (self);
        setup_theme_custom_selector (self, TRUE);

        update_theme (self);

        return object;
}

static void
gvc_sound_theme_editor_class_init (GvcSoundThemeEditorClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_sound_theme_editor_constructor;
        object_class->finalize = gvc_sound_theme_editor_finalize;

        g_type_class_add_private (klass, sizeof (GvcSoundThemeEditorPrivate));
}

static void
on_click_feedback_toggled (GtkToggleButton      *button,
                           GvcSoundThemeEditor *editor)
{
        MateConfClient *client;
        gboolean     enabled;

        enabled = gtk_toggle_button_get_active (button);

        client = mateconf_client_get_default ();
        mateconf_client_set_bool (client, INPUT_SOUNDS_KEY, enabled, NULL);
        g_object_unref (client);
}

static void
on_key_changed (MateConfClient          *client,
                guint                 cnxn_id,
                MateConfEntry           *entry,
                GvcSoundThemeEditor *editor)
{
        const char *key;
        MateConfValue *value;

        key = mateconf_entry_get_key (entry);

        if (! g_str_has_prefix (key, KEY_SOUNDS_DIR)
            && ! g_str_has_prefix (key, KEY_MARCO_DIR)) {
                return;
        }

        value = mateconf_entry_get_value (entry);
        if (strcmp (key, EVENT_SOUNDS_KEY) == 0) {
                update_theme (editor);
        } else if (strcmp (key, SOUND_THEME_KEY) == 0) {
                update_theme (editor);
        } else if (strcmp (key, INPUT_SOUNDS_KEY) == 0) {
                update_theme (editor);
        } else if (strcmp (key, AUDIO_BELL_KEY) == 0) {
                update_theme (editor);
        }
}

static void
on_treeview_row_activated (GtkTreeView          *treeview,
                           GtkTreePath          *path,
                           GtkTreeViewColumn    *column,
                           GvcSoundThemeEditor *editor)
{
        g_debug ("row activated");
        play_sound_at_path (GTK_WIDGET (treeview), path);
}

static void
constrain_list_size (GtkWidget      *widget,
                     GtkRequisition *requisition,
                     GtkWidget      *to_size)
{
        GtkRequisition req;
        int            max_height;

        /* constrain height to be the tree height up to a max */
        max_height = (gdk_screen_get_height (gtk_widget_get_screen (widget))) / 4;

        gtk_widget_size_request (to_size, &req);

        requisition->height = MIN (req.height, max_height);
}

static void
setup_list_size_constraint (GtkWidget *widget,
                            GtkWidget *to_size)
{
        g_signal_connect (widget,
                          "size-request",
                          G_CALLBACK (constrain_list_size),
                          to_size);
}

static void
gvc_sound_theme_editor_init (GvcSoundThemeEditor *editor)
{
        GtkWidget   *box;
        GtkWidget   *label;
        GtkWidget   *scrolled_window;

        editor->priv = GVC_SOUND_THEME_EDITOR_GET_PRIVATE (editor);

        editor->priv->theme_box = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (editor),
                            editor->priv->theme_box, FALSE, FALSE, 0);

        label = gtk_label_new (_("Sound Theme:"));
        gtk_box_pack_start (GTK_BOX (editor->priv->theme_box), label, FALSE, FALSE, 6);
        editor->priv->combo_box = gtk_combo_box_new ();
        gtk_box_pack_start (GTK_BOX (editor->priv->theme_box), editor->priv->combo_box, FALSE, FALSE, 0);


        editor->priv->client = mateconf_client_get_default ();

        editor->priv->selection_box = box = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (editor), box, TRUE, TRUE, 0);

        editor->priv->treeview = gtk_tree_view_new ();
        g_signal_connect (editor->priv->treeview,
                          "row-activated",
                          G_CALLBACK (on_treeview_row_activated),
                          editor);

        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        setup_list_size_constraint (scrolled_window, editor->priv->treeview);

        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (scrolled_window), editor->priv->treeview);
        gtk_container_add (GTK_CONTAINER (box), scrolled_window);

        editor->priv->click_feedback_button = gtk_check_button_new_with_mnemonic (_("Enable window and button sounds"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->priv->click_feedback_button),
                                      mateconf_client_get_bool (editor->priv->client, INPUT_SOUNDS_KEY, NULL));
        gtk_box_pack_start (GTK_BOX (box),
                            editor->priv->click_feedback_button,
                            FALSE, FALSE, 0);
        g_signal_connect (editor->priv->click_feedback_button,
                          "toggled",
                          G_CALLBACK (on_click_feedback_toggled),
                          editor);


        mateconf_client_add_dir (editor->priv->client, KEY_SOUNDS_DIR,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        editor->priv->sounds_dir_id = mateconf_client_notify_add (editor->priv->client,
                                                               KEY_SOUNDS_DIR,
							       (MateConfClientNotifyFunc)on_key_changed,
							       editor, NULL, NULL);
        mateconf_client_add_dir (editor->priv->client, KEY_MARCO_DIR,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        editor->priv->marco_dir_id = mateconf_client_notify_add (editor->priv->client,
								 KEY_MARCO_DIR,
								 (MateConfClientNotifyFunc)on_key_changed,
								 editor, NULL, NULL);

        /* FIXME: should accept drag and drop themes.  should also
           add an "Add Theme..." item to the theme combobox */
}

static void
gvc_sound_theme_editor_finalize (GObject *object)
{
        GvcSoundThemeEditor *sound_theme_editor;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_SOUND_THEME_EDITOR (object));

        sound_theme_editor = GVC_SOUND_THEME_EDITOR (object);

	if (sound_theme_editor->priv != NULL) {
		if (sound_theme_editor->priv->sounds_dir_id > 0) {
			mateconf_client_notify_remove (sound_theme_editor->priv->client,
						    sound_theme_editor->priv->sounds_dir_id);
			sound_theme_editor->priv->sounds_dir_id = 0;
		}
		if (sound_theme_editor->priv->marco_dir_id > 0) {
			mateconf_client_notify_remove (sound_theme_editor->priv->client,
						    sound_theme_editor->priv->marco_dir_id);
			sound_theme_editor->priv->marco_dir_id = 0;
		}
		g_object_unref (sound_theme_editor->priv->client);
		sound_theme_editor->priv->client = NULL;
	}

        G_OBJECT_CLASS (gvc_sound_theme_editor_parent_class)->finalize (object);
}

GtkWidget *
gvc_sound_theme_editor_new (void)
{
        GObject *editor;
        editor = g_object_new (GVC_TYPE_SOUND_THEME_EDITOR,
                                "spacing", 6,
                                NULL);
        return GTK_WIDGET (editor);
}
