/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 * mate-settings-keyboard-xkb.h
 *
 * Copyright (C) 2001 Udaltsoft
 *
 * Written by Sergey V. Oudaltsov <svu@users.sourceforge.net>
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

#ifndef __GSD_KEYBOARD_XKB_H
#define __GSD_KEYBOARD_XKB_H

#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#include <libxklavier/xklavier.h>
#include "gsd-keyboard-manager.h"

void gsd_keyboard_xkb_init (MateConfClient *client, GsdKeyboardManager *manager);
void gsd_keyboard_xkb_shutdown (void);

typedef void (*PostActivationCallback) (void *userData);

void
gsd_keyboard_xkb_set_post_activation_callback (PostActivationCallback fun,
                                               void                  *userData);

#endif
