/*
 * Mini-Commander Applet
 * Copyright (C) 1998 Oliver Maruhn <oliver@maruhn.com>
 *
 * Author: Oliver Maruhn <oliver@maruhn.com>
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

#include <config.h>

#include "help.h"

void 
show_help (GtkAction *action,
	   MCData    *mcdata)
{
    GError *error = NULL;
   
    gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (mcdata->applet)),
		"command-line",
		gtk_get_current_event_time (),
		&error);

    if (error) { /* FIXME: this error needs to be seen by the user */
    	g_warning ("help error: %s\n", error->message);
	g_error_free (error);
	error = NULL;
    }
}
