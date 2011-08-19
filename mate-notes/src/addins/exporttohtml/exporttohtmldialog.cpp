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



#include <glibmm/i18n.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>

#include "sharp/files.hpp"
#include "exporttohtmldialog.hpp"
#include "preferences.hpp"

namespace exporttohtml {


using gnote::Preferences;

ExportToHtmlDialog::ExportToHtmlDialog(const std::string & default_file)
  : Gtk::FileChooserDialog(_("Destination for HTML Export"),
                           Gtk::FILE_CHOOSER_ACTION_SAVE)
  , m_export_linked(_("Export linked notes"))
  , m_export_linked_all(_("Include all other linked notes"))

{
  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

  set_default_response(Gtk::RESPONSE_OK);

  Gtk::Table *table = manage(new Gtk::Table (2, 2, false));

  m_export_linked.signal_toggled().connect(
    sigc::mem_fun(*this, &ExportToHtmlDialog::on_export_linked_toggled));

  table->attach (m_export_linked, 0, 2, 0, 1, Gtk::FILL, (Gtk::AttachOptions)0, 0, 0);
  table->attach (m_export_linked_all,
                 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, (Gtk::AttachOptions)0, 20, 0);

  set_extra_widget(*table);

  set_do_overwrite_confirmation(true);
  set_local_only(true);

  show_all ();
  load_preferences (default_file);
}


bool ExportToHtmlDialog::get_export_linked() const
{
  return m_export_linked.get_active();
}


void ExportToHtmlDialog::set_export_linked(bool value)
{
  m_export_linked.set_active(value);
}


bool ExportToHtmlDialog::get_export_linked_all() const
{
  return m_export_linked_all.get_active();
}


void ExportToHtmlDialog::set_export_linked_all(bool value)
{
  m_export_linked_all.set_active(value);
}


void ExportToHtmlDialog::save_preferences()
{
  std::string dir = sharp::file_dirname(get_filename());
  
  Preferences::obj().set<std::string>(Preferences::EXPORTHTML_LAST_DIRECTORY, 
                                      dir);

  Preferences::obj().set<bool>(Preferences::EXPORTHTML_EXPORT_LINKED, 
                               get_export_linked());
  Preferences::obj().set<bool>(Preferences::EXPORTHTML_EXPORT_LINKED_ALL, 
                               get_export_linked_all());
}


void ExportToHtmlDialog::load_preferences(const std::string & default_file)
{
  std::string last_dir = Preferences::obj().get<std::string>(Preferences::EXPORTHTML_LAST_DIRECTORY);
  if (last_dir.empty()) {
    last_dir = Glib::get_home_dir();
  }
  set_current_folder (last_dir);
  set_current_name(default_file);

  set_export_linked(Preferences::obj().get<bool>(Preferences::EXPORTHTML_EXPORT_LINKED));
  set_export_linked_all(Preferences::obj().get<bool>(Preferences::EXPORTHTML_EXPORT_LINKED_ALL));
}


void ExportToHtmlDialog::on_export_linked_toggled()
{
  if (m_export_linked.get_active()) {
    m_export_linked_all.set_sensitive(true);
  }
  else {
    m_export_linked_all.set_sensitive(false);
  }
}



}
