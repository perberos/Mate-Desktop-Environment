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



#include <list>

#include <gtkmm/targetentry.h>

#include "debug.hpp"
#include "gnote.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/notebookstreeview.hpp"
#include "notemanager.hpp"

namespace gnote {
  namespace notebooks {

    NotebooksTreeView::NotebooksTreeView(const Glib::RefPtr<Gtk::TreeModel> & model)
      : Gtk::TreeView(model)
      , m_note_manager(Gnote::obj().default_note_manager())
    {
      // Set up the notebooksTree as a drag target so that notes
      // can be dragged into the notebook.
      std::list<Gtk::TargetEntry> targets;
      targets.push_back(Gtk::TargetEntry ("text/uri-list",
                                          Gtk::TARGET_SAME_APP,
                                          1));
      drag_dest_set(targets, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_MOVE);

    }

    void NotebooksTreeView::on_drag_data_received( const Glib::RefPtr<Gdk::DragContext> & context,
                                                   int x, int y,
                                                   const Gtk::SelectionData & selectionData,
                                                   guint , guint time_)
    {
      utils::UriList uriList(selectionData);
      if (uriList.size() == 0) {
        context->drag_finish (false, false, time_);
        return;
      }
      
      Gtk::TreePath treepath;
      Gtk::TreeViewDropPosition pos;
      if (get_dest_row_at_pos (x, y, treepath, pos) == false) {
        context->drag_finish (false, false, time_);
        return;
      }
      
      Gtk::TreeIter iter = get_model()->get_iter(treepath);
      if (!iter) {
        context->drag_finish (false, false, time_);
        return;
      }
      
      Notebook::Ptr destNotebook;
      iter->get_value(0, destNotebook);
      if (std::tr1::dynamic_pointer_cast<AllNotesNotebook>(destNotebook)) {
        context->drag_finish (false, false, time_);
        return;
      }

      for(utils::UriList::const_iterator uri_iter = uriList.begin();
          uri_iter != uriList.end(); ++uri_iter) {
        const sharp::Uri & uri(*uri_iter);
        Note::Ptr note = m_note_manager.find_by_uri (uri.to_string ());
        if (!note)
          continue;
        
        DBG_OUT ("Dropped into notebook: %s", note->get_title().c_str());
        
        // TODO: If we ever support selecting multiple notes,
        // we may want to double-check to see if there will be
        // any notes are already inside of a notebook.  Do we
        // want to prompt the user to confirm this choice?
        NotebookManager::instance().move_note_to_notebook (note, destNotebook);
      }

      context->drag_finish (true, false, time_);
    }

    bool NotebooksTreeView::on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &,
                                int x, int y, guint )
    {
      Gtk::TreePath treepath;
      Gtk::TreeViewDropPosition pos;
      if (get_dest_row_at_pos (x, y, treepath,pos) == false) {
        gtk_tree_view_set_drag_dest_row (gobj(), NULL, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
        return false;
      }
      
      Gtk::TreeIter iter = get_model()->get_iter (treepath);
      if (!iter) {
        gtk_tree_view_set_drag_dest_row (gobj(), NULL, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
        return false;
      }
      
      Notebook::Ptr destNotebook;
      iter->get_value(0, destNotebook);
      if (std::tr1::dynamic_pointer_cast<AllNotesNotebook>(destNotebook)) {
        gtk_tree_view_set_drag_dest_row (gobj(), NULL, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
        return true;
      }
      
      set_drag_dest_row (treepath , Gtk::TREE_VIEW_DROP_INTO_OR_AFTER);
      
      return true;
    }

    void NotebooksTreeView::on_drag_leave(const Glib::RefPtr<Gdk::DragContext> & , guint )
    {
      gtk_tree_view_set_drag_dest_row (gobj(), NULL, GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
    }

  }
}
