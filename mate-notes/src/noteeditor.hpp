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




#ifndef __NOTE_EDITOR_HPP_
#define __NOTE_EDITOR_HPP_

#include <glibmm/refptr.h>
#include <gtkmm/textview.h>

#include "preferences.hpp"

namespace gnote {

class NoteEditor
  : public Gtk::TextView
{
public:
  typedef Glib::RefPtr<NoteEditor> Ptr;

  NoteEditor(const Glib::RefPtr<Gtk::TextBuffer> & buffer);
  ~NoteEditor();
  static int default_margin()
    {
      return 8;
    }

protected:
  virtual void on_drag_data_received (Glib::RefPtr<Gdk::DragContext> & context,
                                      int x, int y,
                                      const Gtk::SelectionData & selection_data,
                                      guint info,  guint time);

private:
  Pango::FontDescription get_mate_document_font_description();
  void on_font_setting_changed (Preferences*, MateConfEntry* entry);
  static void on_font_setting_changed_mateconf (MateConfClient *, guint cnxid, MateConfEntry* entry, gpointer data);
  void update_custom_font_setting();
  void modify_font_from_string (const std::string & fontString);
  bool key_pressed (GdkEventKey * ev);
  bool button_pressed (GdkEventButton * ev);

  guint                         m_mateconf_notify;
};


}

#endif
