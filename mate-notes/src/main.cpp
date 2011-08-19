/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glibmm/i18n.h>
#include <gtkmm/main.h>

#include "gnote.hpp"

int main(int argc, char **argv)
{
//  if(!Glib::thread_supported()) {
//    Glib::thread_init();
//  }

  bindtextdomain(GETTEXT_PACKAGE, GNOTE_LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  Gtk::Main kit(argc, argv);
  gnote::Gnote *app = &gnote::Gnote::obj();
  int retval = app->main(argc, argv);
  delete app;
  return retval;
}
