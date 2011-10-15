/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/* mate-window-manager.h
 * Copyright (C) 2002 Seth Nickell
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * Written by: Seth Nickell <snickell@stanford.edu>,
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

#ifndef MATE_WINDOW_MANAGER_H
#define MATE_WINDOW_MANAGER_H

#include <glib-object.h>

#include <libmate/mate-desktop-item.h>

/* Increment if backward-incompatible changes are made, so we get a clean
 * error. In principle the libtool versioning handles this, but
 * in combination with dlopen I don't quite trust that.
 */
#define MATE_WINDOW_MANAGER_INTERFACE_VERSION 1

typedef GObject * (* MateWindowManagerNewFunc) (int expected_interface_version);

typedef enum
{
        MATE_WM_SETTING_FONT                = 1 << 0,
        MATE_WM_SETTING_MOUSE_FOCUS         = 1 << 1,
        MATE_WM_SETTING_AUTORAISE           = 1 << 2,
        MATE_WM_SETTING_AUTORAISE_DELAY     = 1 << 3,
        MATE_WM_SETTING_MOUSE_MOVE_MODIFIER = 1 << 4,
        MATE_WM_SETTING_THEME               = 1 << 5,
        MATE_WM_SETTING_DOUBLE_CLICK_ACTION = 1 << 6,
        MATE_WM_SETTING_MASK                =
        MATE_WM_SETTING_FONT                |
        MATE_WM_SETTING_MOUSE_FOCUS         |
        MATE_WM_SETTING_AUTORAISE           |
        MATE_WM_SETTING_AUTORAISE_DELAY     |
        MATE_WM_SETTING_MOUSE_MOVE_MODIFIER |
        MATE_WM_SETTING_THEME               |
        MATE_WM_SETTING_DOUBLE_CLICK_ACTION
} MateWMSettingsFlags;

typedef struct
{
        int number;
        const char *human_readable_name;
} MateWMDoubleClickAction;

typedef struct
{
        MateWMSettingsFlags flags; /* this allows us to expand the struct
                                     * while remaining binary compatible
                                     */
        const char *font;
        int autoraise_delay;
        /* One of the strings "Alt", "Control", "Super", "Hyper", "Meta" */
        const char *mouse_move_modifier;
        const char *theme;
        int double_click_action;

        guint focus_follows_mouse : 1;
        guint autoraise : 1;

} MateWMSettings;

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_WINDOW_MANAGER(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, mate_window_manager_get_type (), MateWindowManager)
#define MATE_WINDOW_MANAGER_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, mate_window_manager_get_type (), MateWindowManagerClass)
#define IS_MATE_WINDOW_MANAGER(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, mate_window_manager_get_type ())
#define MATE_WINDOW_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), mate_window_manager_get_type, MateWindowManagerClass))

typedef struct _MateWindowManager MateWindowManager;
typedef struct _MateWindowManagerClass MateWindowManagerClass;

typedef struct _MateWindowManagerPrivate MateWindowManagerPrivate;

struct _MateWindowManager
{
        GObject parent;

        MateWindowManagerPrivate *p;
};

struct _MateWindowManagerClass
{
        GObjectClass klass;

        void         (* settings_changed)       (MateWindowManager    *wm);

        void         (* change_settings)        (MateWindowManager    *wm,
                                                 const MateWMSettings *settings);
        void         (* get_settings)           (MateWindowManager    *wm,
                                                 MateWMSettings       *settings);

        GList *      (* get_theme_list)         (MateWindowManager *wm);
        char *       (* get_user_theme_folder)  (MateWindowManager *wm);

        int          (* get_settings_mask)      (MateWindowManager *wm);

        void         (* get_double_click_actions) (MateWindowManager              *wm,
                                                   const MateWMDoubleClickAction **actions,
                                                   int                             *n_actions);

        void         (* padding_func_1)         (MateWindowManager *wm);
        void         (* padding_func_2)         (MateWindowManager *wm);
        void         (* padding_func_3)         (MateWindowManager *wm);
        void         (* padding_func_4)         (MateWindowManager *wm);
        void         (* padding_func_5)         (MateWindowManager *wm);
        void         (* padding_func_6)         (MateWindowManager *wm);
        void         (* padding_func_7)         (MateWindowManager *wm);
        void         (* padding_func_8)         (MateWindowManager *wm);
        void         (* padding_func_9)         (MateWindowManager *wm);
        void         (* padding_func_10)        (MateWindowManager *wm);
};

GObject *         mate_window_manager_new                     (MateDesktopItem   *item);
GType             mate_window_manager_get_type                (void);
const char *      mate_window_manager_get_name                (MateWindowManager *wm);
MateDesktopItem *mate_window_manager_get_ditem               (MateWindowManager *wm);

/* GList of char *'s */
GList *           mate_window_manager_get_theme_list          (MateWindowManager *wm);
char *            mate_window_manager_get_user_theme_folder   (MateWindowManager *wm);

/* only uses fields with their flags set */
void              mate_window_manager_change_settings  (MateWindowManager    *wm,
                                                         const MateWMSettings *settings);
/* only gets fields with their flags set (and if it fails to get a field,
 * it unsets that flag, so flags should be checked on return)
 */
void              mate_window_manager_get_settings     (MateWindowManager *wm,
                                                         MateWMSettings    *settings);

void              mate_window_manager_settings_changed (MateWindowManager *wm);

void mate_window_manager_get_double_click_actions (MateWindowManager              *wm,
                                                    const MateWMDoubleClickAction **actions,
                                                    int                             *n_actions);

/* Helper functions for MateWMSettings */
MateWMSettings *mate_wm_settings_copy (MateWMSettings *settings);
void             mate_wm_settings_free (MateWMSettings *settings);

#ifdef __cplusplus
}
#endif

#endif /* MATE_WINDOW_MANAGER_H */
