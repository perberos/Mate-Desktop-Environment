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



#ifndef _NOTEBOOKS_TREEVIEW_HPP_
#define _NOTEBOOKS_TREEVIEW_HPP_

#include <gtkmm/treeview.h>

namespace gnote {

  class NoteManager;

  namespace notebooks {

  class NotebooksTreeView
    : public Gtk::TreeView
  {
  public:
    NotebooksTreeView(const Glib::RefPtr<Gtk::TreeModel> & model);

  protected:
    virtual void on_drag_data_received( const Glib::RefPtr<Gdk::DragContext> & context,
                                        int x, int y,
                                        const Gtk::SelectionData & selection_data,
                                        guint info, guint time);
    virtual bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext> & context,
                                int x, int y, guint time);
    virtual void on_drag_leave(const Glib::RefPtr<Gdk::DragContext> & context, guint time);
  private:
    NoteManager & m_note_manager;
  };

  }
}

#endif

