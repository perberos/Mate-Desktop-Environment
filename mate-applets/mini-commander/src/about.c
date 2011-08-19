/*
 * Mini-Commander Applet
 * Copyright (C) 1998, 1999 Oliver Maruhn <oliver@maruhn.com>
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

#include <glib.h>
#include <gtk/gtk.h>

#include "about.h"

void about_box (GtkAction *action,
		MCData    *mcdata)
{
	static const gchar *authors[] = {
		"Oliver Maruhn <oliver@maruhn.com>",
		"Mark McLoughlin <mark@skynet.ie>",
		NULL
	};

	const gchar *documenters[] = {
	        "Dan Mueth <d-mueth@uchicago.edu>",
                "Oliver Maruhn <oliver@maruhn.com>",
                "Sun MATE Documentation Team <gdocteam@sun.com>",
		NULL
	};

	gtk_show_about_dialog (NULL,
		"version",	VERSION,
		"copyright",	"\xc2\xa9 1998-2005 Oliver Maruhn and others",
		"comments",	_("This MATE applet adds a command line to "
				  "the panel. It features command completion, "
				  "command history, and changeable macros."),
		"authors",	authors,
		"documenters",	documenters,
		"translator-credits",	_("translator-credits"),
		"logo-icon-name",	"mate-mini-commander",
		NULL);
}
