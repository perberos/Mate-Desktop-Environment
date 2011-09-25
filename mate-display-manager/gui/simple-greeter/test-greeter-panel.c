/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "mdm-greeter-panel.h"

int
main (int argc, char *argv[])
{
        GtkWidget  *panel;
        GdkDisplay *display;
        GdkScreen  *screen;
        int         monitor;
        int         x, y;

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

        gtk_init (&argc, &argv);

        display = gdk_display_get_default ();
        gdk_display_get_pointer (display, &screen, &x, &y, NULL);
        monitor = gdk_screen_get_monitor_at_point (screen, x, y);

        panel = mdm_greeter_panel_new (screen, monitor, TRUE);
        mdm_greeter_panel_show_user_options (MDM_GREETER_PANEL (panel));

        gtk_widget_show (panel);

        gtk_main ();

        return 0;
}
