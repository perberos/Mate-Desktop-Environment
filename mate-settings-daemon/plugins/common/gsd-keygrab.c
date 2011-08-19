/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2001-2003 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2006-2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2008 Jens Granseuer <jensgr@gmx.net>
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

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#ifdef HAVE_X11_EXTENSIONS_XKB_H
#include <X11/XKBlib.h>
#include <X11/extensions/XKB.h>
#include <gdk/gdkkeysyms.h>
#endif

#include "eggaccelerators.h"

#include "gsd-keygrab.h"

/* these are the mods whose combinations are ignored by the keygrabbing code */
static GdkModifierType gsd_ignored_mods = 0;

/* these are the ones we actually use for global keys, we always only check
 * for these set */
static GdkModifierType gsd_used_mods = 0;

static void
setup_modifiers (void)
{
        if (gsd_used_mods == 0 || gsd_ignored_mods == 0) {
                GdkModifierType dynmods;

                /* default modifiers */
                gsd_ignored_mods = \
                        0x2000 /*Xkb modifier*/ | GDK_LOCK_MASK | GDK_HYPER_MASK;
		gsd_used_mods = \
                        GDK_SHIFT_MASK | GDK_CONTROL_MASK |\
                        GDK_MOD1_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK |\
                        GDK_MOD5_MASK | GDK_SUPER_MASK | GDK_META_MASK;

                /* NumLock can be assigned to varying keys so we need to
                 * resolve and ignore it specially */
                dynmods = 0;
                egg_keymap_resolve_virtual_modifiers (gdk_keymap_get_default (),
                                                      EGG_VIRTUAL_NUM_LOCK_MASK,
                                                      &dynmods);

                gsd_ignored_mods |= dynmods;
                gsd_used_mods &= ~dynmods;
	}
}

static void
grab_key_real (guint      keycode,
               GdkWindow *root,
               gboolean   grab,
               int        mask)
{
        if (grab) {
                XGrabKey (GDK_DISPLAY (),
                          keycode,
                          mask,
                          GDK_WINDOW_XID (root),
                          True,
                          GrabModeAsync,
                          GrabModeAsync);
        } else {
                XUngrabKey (GDK_DISPLAY (),
                            keycode,
                            mask,
                            GDK_WINDOW_XID (root));
        }
}

/* Grab the key. In order to ignore GSD_IGNORED_MODS we need to grab
 * all combinations of the ignored modifiers and those actually used
 * for the binding (if any).
 *
 * inspired by all_combinations from mate-panel/mate-panel/global-keys.c
 *
 * This may generate X errors.  The correct way to use this is like:
 *
 *        gdk_error_trap_push ();
 *
 *        grab_key_unsafe (key, grab, screens);
 *
 *        gdk_flush ();
 *        if (gdk_error_trap_pop ())
 *                g_warning ("Grab failed, another application may already have access to key '%u'",
 *                           key->keycode);
 *
 * This is not done in the function itself, to allow doing multiple grab_key
 * operations with one flush only.
 */
#define N_BITS 32
void
grab_key_unsafe (Key                 *key,
                 gboolean             grab,
                 GSList              *screens)
{
        int   indexes[N_BITS]; /* indexes of bits we need to flip */
        int   i;
        int   bit;
        int   bits_set_cnt;
        int   uppervalue;
        guint mask;

        setup_modifiers ();

        mask = gsd_ignored_mods & ~key->state & GDK_MODIFIER_MASK;

        bit = 0;
        /* store the indexes of all set bits in mask in the array */
        for (i = 0; mask; ++i, mask >>= 1) {
                if (mask & 0x1) {
                        indexes[bit++] = i;
                }
        }

        bits_set_cnt = bit;

        uppervalue = 1 << bits_set_cnt;
        /* grab all possible modifier combinations for our mask */
        for (i = 0; i < uppervalue; ++i) {
                GSList *l;
                int     j;
                int     result = 0;

                /* map bits in the counter to those in the mask */
                for (j = 0; j < bits_set_cnt; ++j) {
                        if (i & (1 << j)) {
                                result |= (1 << indexes[j]);
                        }
                }

                for (l = screens; l; l = l->next) {
                        GdkScreen *screen = l->data;
                        guint *code;

                        for (code = key->keycodes; *code; ++code) {
                                grab_key_real (*code,
                                               gdk_screen_get_root_window (screen),
                                               grab,
                                               result | key->state);
                        }
                }
        }
}

static gboolean
have_xkb (Display *dpy)
{
	static int have_xkb = -1;

	if (have_xkb == -1) {
#ifdef HAVE_X11_EXTENSIONS_XKB_H
		int opcode, error_base, major, minor, xkb_event_base;

		have_xkb = XkbQueryExtension (dpy,
					      &opcode,
					      &xkb_event_base,
					      &error_base,
					      &major,
					      &minor)
			&& XkbUseExtension (dpy, &major, &minor);
#else
		have_xkb = 0;
#endif
	}

	return have_xkb;
}

gboolean
key_uses_keycode (const Key *key, guint keycode)
{
	if (key->keycodes != NULL) {
		guint *c;

		for (c = key->keycodes; *c; ++c) {
			if (*c == keycode)
				return TRUE;
		}
	}
	return FALSE;
}

gboolean
match_key (Key *key, XEvent *event)
{
	guint keyval;
	GdkModifierType consumed;
	gint group;

	if (key == NULL)
		return FALSE;

	setup_modifiers ();

#ifdef HAVE_X11_EXTENSIONS_XKB_H
	if (have_xkb (event->xkey.display))
		group = XkbGroupForCoreState (event->xkey.state);
	else
#endif
		group = (event->xkey.state & GDK_Mode_switch) ? 1 : 0;

	/* Check if we find a keysym that matches our current state */
	if (gdk_keymap_translate_keyboard_state (NULL, event->xkey.keycode,
					     event->xkey.state, group,
					     &keyval, NULL, NULL, &consumed)) {
		guint lower, upper;

		gdk_keyval_convert_case (keyval, &lower, &upper);

		/* If we are checking against the lower version of the
		 * keysym, we might need the Shift state for matching,
		 * so remove it from the consumed modifiers */
		if (lower == key->keysym)
			consumed &= ~GDK_SHIFT_MASK;

		return ((lower == key->keysym || upper == key->keysym)
			&& (event->xkey.state & ~consumed & gsd_used_mods) == key->state);
	}

	/* The key we passed doesn't have a keysym, so try with just the keycode */
        return (key != NULL
                && key->state == (event->xkey.state & gsd_used_mods)
                && key_uses_keycode (key, event->xkey.keycode));
}
