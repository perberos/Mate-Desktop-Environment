/*
 * gnote
 *
 * Copyright (C) 2010 Debarshi Ray
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

#ifndef __NOTE_RENAME_DIALOG_HPP_
#define __NOTE_RENAME_DIALOG_HPP_

#include <map>
#include <string>
#include <tr1/memory>

#include <glibmm.h>
#include <gtkmm.h>

#include "note.hpp"

namespace gnote {

// Values should match with those in data/gnote.schemas.in
enum NoteRenameBehavior {
  NOTE_RENAME_ALWAYS_SHOW_DIALOG = 0,
  NOTE_RENAME_ALWAYS_REMOVE_LINKS = 1,
  NOTE_RENAME_ALWAYS_RENAME_LINKS = 2
};

class NoteRenameDialog
  : public Gtk::Dialog
{
public:

  typedef std::tr1::shared_ptr<std::map<Note::Ptr, bool> > MapPtr;

  NoteRenameDialog(const Note::List & notes,
                   const std::string & old_title,
                   const Note::Ptr & renamed_note);
  MapPtr get_notes() const;
  NoteRenameBehavior get_selected_behavior() const;

private:

  void on_advanced_expander_changed(bool expanded);
  void on_always_rename_clicked();
  void on_always_show_dlg_clicked();
  void on_never_rename_clicked();
  bool on_notes_model_foreach_iter_accumulate(
         const Gtk::TreeIter & iter,
         const MapPtr & notes) const;
  bool on_notes_model_foreach_iter_select(const Gtk::TreeIter & iter,
                                          bool select);
  void on_notes_view_row_activated(const Gtk::TreeModel::Path & p,
                                   Gtk::TreeView::Column *,
                                   const std::string & old_title);
  void on_select_all_button_clicked(bool select);
  void on_toggle_cell_toggled(const std::string & p);

  Glib::RefPtr<Gtk::ListStore> m_notes_model;
  Gtk::Button m_dont_rename_button;
  Gtk::Button m_rename_button;
  Gtk::Button m_select_all_button;
  Gtk::Button m_select_none_button;
  Gtk::RadioButton m_always_show_dlg_radio;
  Gtk::RadioButton m_always_rename_radio;
  Gtk::RadioButton m_never_rename_radio;
  Gtk::VBox m_notes_box;
};

}

#endif
