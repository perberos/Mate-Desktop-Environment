/* -*- mode: c; style: linux -*- */

/* accessibility-keyboard.c
 * Copyright (C) 2002 Ximian, Inc.
 *
 * Written by: Jody Goldberg <jody@gnome.org>
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

#ifndef __MATE_KEYBOARD_PROPERTY_A11Y_H
#define __MATE_KEYBOARD_PROPERTY_A11Y_H

#include <mateconf/mateconf-changeset.h>
#include <gtk/gtk.h>

extern void setup_a11y_tabs (GtkBuilder * dialog, MateConfChangeSet * changeset);

#endif /* __MATE_KEYBOARD_PROPERTY_A11Y_H */
