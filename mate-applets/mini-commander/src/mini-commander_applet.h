/*
 * Mini-Commander Applet
 * Copyright (C) 1998 Oliver Maruhn <oliver@maruhn.com>,
 *               2002 Sun Microsystems
 *
 * Authors: Oliver Maruhn <oliver@maruhn.com>,
 *          Mark McLoughlin <mark@skynet.ie>
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

#ifndef __MC_APPLET_H__
#define __MC_APPLET_H__

#include <glib.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#include <gtk/gtk.h>
#include <mate-panel-applet.h>

typedef struct _MCData MCData;

#include "preferences.h"

/* Constants */
#define MC_NUM_LISTENERS                12
#define MC_HISTORY_LIST_LENGTH          50
#define MC_MAX_COMMAND_LENGTH           505
#define MC_MAX_NUM_MACRO_PARAMETERS     100

struct _MCData {
    MatePanelApplet   *applet;

    GtkWidget     *applet_box;

    GtkWidget     *entry;
    GtkWidget     *file_select;

    int            label_timeout;
    int            cmd_line_size_y;

    MCPreferences  preferences;
    MCPrefsDialog  prefs_dialog;

    guint          listeners [MC_NUM_LISTENERS];
    gboolean       error;
    MatePanelAppletOrient orient;
};

void mc_applet_draw (MCData *mc);

void set_atk_name_description (GtkWidget *widget,
			       const char *name,
			       const char *description);

G_END_DECLS

#endif /* __MC_APPLET_H__ */
