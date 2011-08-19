/* gdict-about.c - GtkAboutDialog wrapper
 *
 * This file is part of MATE Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gdict-about.h"

void
gdict_show_about_dialog (GtkWidget *parent)
{
  const gchar *authors[] = {
    "Mike Hughes <mfh@psilord.com>",
    "Spiros Papadimitriou <spapadim+@cs.cmu.edu>",
    "Bradford Hovinen <hovinen@udel.edu>",
    "Vincent Noel <vnoel@cox.net>",
    "Emmanuele Bassi <ebassi@gmail.com>",
    NULL
  };

  const gchar *documenters[] = {
    "Sun MATE Documentation Team <gdocteam@sun.com>",
    "John Fleck <jfleck@inkstain.net>",
    "Emmanuele Bassi <ebassi@gmail.com>",
    NULL
  };

  const gchar *translator_credits = _("translator-credits");
  const gchar *copyright = "Copyright \xc2\xa9 2005-2006 Emmanuele Bassi";
  const gchar *comments = _("Look up words in dictionaries");
  
  const gchar *license =
    "This program is free software; you can redistribute it and/or "
    "modify it under the terms of the GNU General Public License as "
    "published by the Free Software Foundation; either version 2 of "
    "the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
    "General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License "
    "along with this program; if not, write to the Free Software "
    "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA "
    "02111-1307, USA.\n";

  g_return_if_fail (GTK_IS_WIDGET (parent));
  
  gtk_show_about_dialog (GTK_IS_WINDOW (parent) ? GTK_WINDOW (parent) : NULL,
  			 "name", _("Dictionary"),
  			 "version", VERSION,
  			 "copyright", copyright,
  			 "comments", comments,
  			 "authors", authors,
  			 "documenters", documenters,
  			 "translator-credits", translator_credits,
  			 "logo-icon-name", "accessories-dictionary",
  			 "license", license,
  			 "wrap-license", TRUE,
			 "screen", gtk_widget_get_screen (parent),
  			 NULL);
}
