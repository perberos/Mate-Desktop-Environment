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


#include "insertbugaction.hpp"

using gnote::EditAction;
using gnote::SplitterAction;
using gnote::InsertAction;

namespace bugzilla {

  InsertBugAction::InsertBugAction(const Gtk::TextIter & start, const std::string & id,
                                   const BugzillaLink::Ptr & tag)
    : m_tag(tag)
    , m_offset(start.get_offset())
    , m_id(id)
  {
    
  }

  void InsertBugAction::undo (Gtk::TextBuffer * buffer)
  {
    // Tag images change the offset by one, but only when deleting.
    Gtk::TextIter start_iter = buffer->get_iter_at_offset(m_offset);
    Gtk::TextIter end_iter = buffer->get_iter_at_offset(m_offset + m_chop.length() + 1);
    buffer->erase(start_iter, end_iter);
    buffer->move_mark(buffer->get_insert(), buffer->get_iter_at_offset(m_offset));
    buffer->move_mark(buffer->get_selection_bound(), buffer->get_iter_at_offset(m_offset));

    m_tag->set_widget_location(Glib::RefPtr<Gtk::TextMark>());

    apply_split_tag(buffer);
  }


  void InsertBugAction::redo (Gtk::TextBuffer * buffer)
  {
    remove_split_tags (buffer);

    Gtk::TextIter cursor = buffer->get_iter_at_offset(m_offset);

    std::vector<Glib::RefPtr<Gtk::TextTag> > tags;
    tags.push_back(m_tag);
    buffer->insert_with_tags (cursor, m_id, tags);

    buffer->move_mark(buffer->get_selection_bound(), buffer->get_iter_at_offset(m_offset));
    buffer->move_mark(buffer->get_insert(),
                      buffer->get_iter_at_offset(m_offset + get_chop().length()));
  }

  void InsertBugAction::merge (EditAction * action)
  {
    SplitterAction *splitter = dynamic_cast<SplitterAction*>(action);
    m_splitTags = splitter->get_split_tags();
    m_chop = splitter->get_chop();
  }

	/*
   * The internal listeners will create an InsertAction when the text
   * is inserted.  Since it's ugly to have the bug insertion appear
   * to the user as two items in the undo stack, have this item eat
   * the other one.
   */
  bool InsertBugAction::can_merge (const EditAction * action) const
  {
    const InsertAction *insert = dynamic_cast<const InsertAction *>(action);
    if (!insert) {
      return false;
    }

    if (m_id == insert->get_chop().text()) {
      return true;
    }

    return false;
  }

  void InsertBugAction::destroy ()
  {
  }

}
