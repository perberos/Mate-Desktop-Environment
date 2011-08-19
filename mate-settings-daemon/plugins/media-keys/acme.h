/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2001 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 */

#ifndef __ACME_H__
#define __ACME_H__

#include "gsd-keygrab.h"

#define MATECONF_BINDING_DIR "/apps/mate_settings_daemon/keybindings"
#define MATECONF_MISC_DIR "/apps/mate_settings_daemon"

enum {
        TOUCHPAD_KEY,
        MUTE_KEY,
        VOLUME_DOWN_KEY,
        VOLUME_UP_KEY,
        POWER_KEY,
        EJECT_KEY,
        HOME_KEY,
        MEDIA_KEY,
        CALCULATOR_KEY,
        SEARCH_KEY,
        EMAIL_KEY,
        SCREENSAVER_KEY,
        HELP_KEY,
        WWW_KEY,
        PLAY_KEY,
        PAUSE_KEY,
        STOP_KEY,
        PREVIOUS_KEY,
        NEXT_KEY,
        HANDLED_KEYS
};

static struct {
        int key_type;
        const char *mateconf_key;
        Key *key;
} keys[HANDLED_KEYS] = {
        { TOUCHPAD_KEY, MATECONF_BINDING_DIR "/touchpad", NULL },
        { MUTE_KEY, MATECONF_BINDING_DIR "/volume_mute",NULL },
        { VOLUME_DOWN_KEY, MATECONF_BINDING_DIR "/volume_down", NULL },
        { VOLUME_UP_KEY, MATECONF_BINDING_DIR "/volume_up", NULL },
        { POWER_KEY, MATECONF_BINDING_DIR "/power", NULL },
        { EJECT_KEY, MATECONF_BINDING_DIR "/eject", NULL },
        { HOME_KEY, MATECONF_BINDING_DIR "/home", NULL },
        { MEDIA_KEY, MATECONF_BINDING_DIR "/media", NULL },
        { CALCULATOR_KEY, MATECONF_BINDING_DIR "/calculator", NULL },
        { SEARCH_KEY, MATECONF_BINDING_DIR "/search", NULL },
        { EMAIL_KEY, MATECONF_BINDING_DIR "/email", NULL },
        { SCREENSAVER_KEY, MATECONF_BINDING_DIR "/screensaver", NULL },
        { HELP_KEY, MATECONF_BINDING_DIR "/help", NULL },
        { WWW_KEY, MATECONF_BINDING_DIR "/www", NULL },
        { PLAY_KEY, MATECONF_BINDING_DIR "/play", NULL },
        { PAUSE_KEY, MATECONF_BINDING_DIR "/pause", NULL },
        { STOP_KEY, MATECONF_BINDING_DIR "/stop", NULL },
        { PREVIOUS_KEY, MATECONF_BINDING_DIR "/previous", NULL },
        { NEXT_KEY, MATECONF_BINDING_DIR "/next", NULL },
};

#endif /* __ACME_H__ */
