/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
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




#include "sharp/exception.hpp"
#include "debug.hpp"
#include "notetag.hpp"
#include "undo.hpp"

namespace gnote {

  ChopBuffer::ChopBuffer(const Glib::RefPtr<Gtk::TextTagTable> & table)
    : Gtk::TextBuffer(table)
  {
  }


  utils::TextRange ChopBuffer::add_chop(const Gtk::TextIter & start_iter, 
                                        const Gtk::TextIter & end_iter)
  {
    int chop_start, chop_end;
    Gtk::TextIter current_end = end();

    chop_start = end().get_offset();
    insert (current_end, start_iter, end_iter);
    chop_end = end().get_offset();

    return utils::TextRange (get_iter_at_offset (chop_start),
                             get_iter_at_offset (chop_end));
  }

  SplitterAction::SplitterAction()
  {
  }

  void SplitterAction::split(Gtk::TextIter iter, 
                             Gtk::TextBuffer * buffer)
  {
    Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list = iter.get_tags();
    for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
        tag_iter != tag_list.end(); ++tag_iter) {
      const Glib::RefPtr<Gtk::TextTag>& tag(*tag_iter);
      NoteTag::ConstPtr noteTag = NoteTag::ConstPtr::cast_dynamic(tag);
      if (noteTag && !noteTag->can_split()) {
        Gtk::TextIter start = iter;
        Gtk::TextIter end = iter;

        // We only care about enclosing tags
        if (start.toggles_tag (tag) || end.toggles_tag (tag)) {
          continue;
        }

        start.backward_to_tag_toggle (tag);
        end.forward_to_tag_toggle (tag);
        add_split_tag (start, end, tag);
        buffer->remove_tag(tag, start, end);
      }
    }
  }
   

  void SplitterAction::add_split_tag(const Gtk::TextIter & start, 
                                     const Gtk::TextIter & end, 
                                     const Glib::RefPtr<Gtk::TextTag> tag)
  {
    TagData data;
    data.start = start.get_offset();
    data.end = end.get_offset();
    data.tag = tag;
    m_splitTags.push_back(data);

    /*
     * The text chop will contain these tags, which means that when
     * the text is inserted again during redo, it will have the tag.
     */
    m_chop.remove_tag(tag);
  }


  int SplitterAction::get_split_offset() const
  {
    int offset = 0;
    for(std::list<TagData>::const_iterator iter = m_splitTags.begin();
        iter != m_splitTags.end(); ++iter) {
      NoteTag::Ptr noteTag = NoteTag::Ptr::cast_dynamic(iter->tag);
      if (noteTag->get_image()) {
        offset++;
      }
    }
    return offset;
  }


  void SplitterAction::apply_split_tag(Gtk::TextBuffer * buffer)
  {
    for(std::list<TagData>::const_iterator iter = m_splitTags.begin();
        iter != m_splitTags.end(); ++iter) {
      const TagData & tag(*iter);
      int offset = get_split_offset ();

      Gtk::TextIter start = buffer->get_iter_at_offset (tag.start - offset);
      Gtk::TextIter end = buffer->get_iter_at_offset (tag.end - offset);
      buffer->apply_tag(tag.tag, start, end);
    }
  }


  void SplitterAction::remove_split_tags(Gtk::TextBuffer *buffer)
  {
    for(std::list<TagData>::const_iterator iter = m_splitTags.begin();
        iter != m_splitTags.end(); ++iter) {
      const TagData & tag(*iter);
      Gtk::TextIter start = buffer->get_iter_at_offset (tag.start);
      Gtk::TextIter end = buffer->get_iter_at_offset (tag.end);
      buffer->remove_tag(tag.tag, start, end);
    }
  }


  InsertAction::InsertAction(const Gtk::TextIter & start, 
                             const std::string & , int length,
                             const ChopBuffer::Ptr & chop_buf)
    : m_index(start.get_offset() - length)
    , m_is_paste(length > 1)
    
  {
    Gtk::TextIter index_iter = start.get_buffer()->get_iter_at_offset(m_index);
    m_chop = chop_buf->add_chop(index_iter, start);
  }


  void InsertAction::undo (Gtk::TextBuffer * buffer)
  {
    int tag_images = get_split_offset ();

    Gtk::TextIter start_iter = buffer->get_iter_at_offset (m_index - tag_images);
    Gtk::TextIter end_iter = buffer->get_iter_at_offset (m_index - tag_images
                                                         + m_chop.length());
    buffer->erase (start_iter, end_iter);
    buffer->move_mark (buffer->get_insert(), 
                       buffer->get_iter_at_offset (m_index - tag_images));
    buffer->move_mark (buffer->get_selection_bound (), 
                       buffer->get_iter_at_offset (m_index - tag_images));

    apply_split_tag (buffer);
  }


  void InsertAction::redo (Gtk::TextBuffer * buffer)
  {
    remove_split_tags (buffer);

    Gtk::TextIter idx_iter = buffer->get_iter_at_offset (m_index);
    buffer->insert (idx_iter, m_chop.start(), m_chop.end());

    buffer->move_mark (buffer->get_selection_bound(), 
                       buffer->get_iter_at_offset (m_index));
    buffer->move_mark (buffer->get_insert(),
                       buffer->get_iter_at_offset (m_index + m_chop.length()));
  }


  void InsertAction::merge (EditAction * action)
  {
    InsertAction * insert = dynamic_cast<InsertAction*>(action);
    if(insert) {
      m_chop.set_end(insert->m_chop.end());

      insert->m_chop.destroy ();
    }
  }


  bool InsertAction::can_merge (const EditAction * action) const
  {
    const InsertAction * insert = dynamic_cast<const InsertAction*>(action);
    if (insert == NULL) {
      return false;
    }

    // Don't group text pastes
    if (m_is_paste || insert->m_is_paste) {
      return false;
    }

    // Must meet eachother
    if (insert->m_index != (m_index + m_chop.length())) {
      return false;
    }

    // Don't group more than one line (inclusive)
    if (m_chop.text()[0] == '\n') {
      return false;
    }

    // Don't group more than one word (exclusive)
    if ((insert->m_chop.text()[0] == ' ') 
        || (insert->m_chop.text()[0] == '\t')) {
      return false;
    }

    return true;
  }


  void InsertAction::destroy ()
  {
    m_chop.erase ();
    m_chop.destroy ();
  }

  

  EraseAction::EraseAction(const Gtk::TextIter & start_iter, 
                           const Gtk::TextIter & end_iter,
                           const ChopBuffer::Ptr & chop_buf)
    : m_start(start_iter.get_offset())
    , m_end(end_iter.get_offset())
    , m_is_cut(m_end - m_start > 1)
  {
    Gtk::TextIter insert =
      start_iter.get_buffer()->get_iter_at_mark (start_iter.get_buffer()->get_insert());
    m_is_forward = (insert.get_offset() <= m_start);

    m_chop = chop_buf->add_chop (start_iter, end_iter);
  }


  void EraseAction::undo (Gtk::TextBuffer * buffer)
  {
    int tag_images = get_split_offset ();

    Gtk::TextIter start_iter = buffer->get_iter_at_offset (m_start - tag_images);
    buffer->insert (start_iter, m_chop.start(), m_chop.end());

    buffer->move_mark (buffer->get_insert(),
                     buffer->get_iter_at_offset (m_is_forward ? m_start - tag_images
                                                 : m_end - tag_images));
    buffer->move_mark (buffer->get_selection_bound(),
                       buffer->get_iter_at_offset (m_is_forward ? m_end - tag_images
                                                   : m_start - tag_images));

    apply_split_tag (buffer);
  }


  void EraseAction::redo (Gtk::TextBuffer * buffer)
  {
    remove_split_tags (buffer);

    Gtk::TextIter start_iter = buffer->get_iter_at_offset (m_start);
    Gtk::TextIter end_iter = buffer->get_iter_at_offset (m_end);
    buffer->erase (start_iter, end_iter);
    buffer->move_mark (buffer->get_insert(), 
                       buffer->get_iter_at_offset (m_start));
    buffer->move_mark (buffer->get_selection_bound(), 
                       buffer->get_iter_at_offset (m_start));
  }


  void EraseAction::merge (EditAction * action)
  {
    EraseAction * erase = dynamic_cast<EraseAction*>(action);
    if (m_start == erase->m_start) {
      m_end += erase->m_end - erase->m_start;
      m_chop.set_end(erase->m_chop.end());

      // Delete the marks, leave the text
      erase->m_chop.destroy ();
    } 
    else {
      m_start = erase->m_start;

      Gtk::TextIter chop_start = m_chop.start();
      m_chop.buffer()->insert(chop_start,
                                  erase->m_chop.start(),
                                  erase->m_chop.end());

      // Delete the marks and text
      erase->destroy ();
    }
  }


  bool EraseAction::can_merge (const EditAction * action) const
  {
    const EraseAction * erase = dynamic_cast<const EraseAction *>(action);
    if (erase == NULL) {
      return false;
    }

    // Don't group separate text cuts
    if (m_is_cut || erase->m_is_cut) {
      return false;
    }

    // Must meet eachother
    if (m_start != (m_is_forward ? erase->m_start : erase->m_end)) {
      return false;
    }

    // Don't group deletes with backspaces
    if (m_is_forward != erase->m_is_forward) {
      return false;
  }

    // Group if something other than text was deleted
    // (e.g. an email image)
    if (m_chop.text().empty() || erase->m_chop.text().empty()) {
      return true;
    }

    // Don't group more than one line (inclusive)
    if (m_chop.text()[0] == '\n') {
      return false;
    }

    // Don't group more than one word (exclusive)
    if ((erase->m_chop.text()[0] == ' ') || (erase->m_chop.text()[0] == '\t')) {
      return false;
    }

    return true;
  }


  void EraseAction::destroy ()
  {
    m_chop.erase ();
    m_chop.destroy ();
  }



  TagApplyAction::TagApplyAction(const Glib::RefPtr<Gtk::TextTag> & tag, 
                                 const Gtk::TextIter & start, 
                                 const Gtk::TextIter & end)
    : m_tag(tag)
    , m_start(start.get_offset())
    , m_end(end.get_offset())
  {
  }


  void TagApplyAction::undo (Gtk::TextBuffer * buffer)
  {
    Gtk::TextIter start_iter, end_iter;
    start_iter = buffer->get_iter_at_offset (m_start);
    end_iter = buffer->get_iter_at_offset (m_end);

    buffer->move_mark (buffer->get_selection_bound(), start_iter);
    buffer->remove_tag (m_tag, start_iter, end_iter);
    buffer->move_mark (buffer->get_insert(), end_iter);
  }


  void TagApplyAction::redo (Gtk::TextBuffer * buffer)
  {
    Gtk::TextIter start_iter, end_iter;
    start_iter = buffer->get_iter_at_offset (m_start);
    end_iter = buffer->get_iter_at_offset (m_end);

    buffer->move_mark (buffer->get_selection_bound(), start_iter);
    buffer->apply_tag (m_tag, start_iter, end_iter);
    buffer->move_mark (buffer->get_insert(), end_iter);
  }


  void TagApplyAction::merge (EditAction * )
  {
    throw sharp::Exception ("TagApplyActions cannot be merged");
  }


  bool TagApplyAction::can_merge (const EditAction * ) const
  {
    return false;
  }


  void TagApplyAction::destroy ()
  {
  }


  TagRemoveAction::TagRemoveAction(const Glib::RefPtr<Gtk::TextTag> & tag, 
                                   const Gtk::TextIter & start, 
                                   const Gtk::TextIter & end)
    : m_tag(tag)
    , m_start(start.get_offset())
    , m_end(end.get_offset())
  {
  }


  void TagRemoveAction::undo (Gtk::TextBuffer * buffer)
  {
    Gtk::TextIter start_iter, end_iter;
    start_iter = buffer->get_iter_at_offset (m_start);
    end_iter = buffer->get_iter_at_offset (m_end);

    buffer->move_mark (buffer->get_selection_bound(), start_iter);
    buffer->apply_tag (m_tag, start_iter, end_iter);
    buffer->move_mark (buffer->get_insert(), end_iter);
  }


  void TagRemoveAction::redo (Gtk::TextBuffer * buffer)
  {
    Gtk::TextIter start_iter, end_iter;
    start_iter = buffer->get_iter_at_offset (m_start);
    end_iter = buffer->get_iter_at_offset (m_end);

    buffer->move_mark (buffer->get_selection_bound(), start_iter);
    buffer->remove_tag (m_tag, start_iter, end_iter);
    buffer->move_mark (buffer->get_insert(), end_iter);
  }


  void TagRemoveAction::merge (EditAction * )
  {
    throw sharp::Exception ("TagRemoveActions cannot be merged");
  }


  bool TagRemoveAction::can_merge (const EditAction * ) const
  {
    return false;
  }


  void TagRemoveAction::destroy ()
  {
  }


  ChangeDepthAction::ChangeDepthAction(int line, bool direction)
    : m_line(line)
    , m_direction(direction)
  {
  }


  void ChangeDepthAction::undo (Gtk::TextBuffer * buffer)
  {
    Gtk::TextIter iter = buffer->get_iter_at_line (m_line);

    NoteBuffer* note_buffer = dynamic_cast<NoteBuffer*>(buffer);
    if(note_buffer) {
      if (m_direction) {
        note_buffer->decrease_depth (iter);
      }
      else {
        note_buffer->increase_depth (iter);
      }

      buffer->move_mark (buffer->get_insert(), iter);
      buffer->move_mark (buffer->get_selection_bound(), iter);
    }
  }


  void ChangeDepthAction::redo (Gtk::TextBuffer * buffer)
  {
    Gtk::TextIter iter = buffer->get_iter_at_line (m_line);

    NoteBuffer* note_buffer = dynamic_cast<NoteBuffer*>(buffer);
    if(note_buffer) {    
      if (m_direction) {
        note_buffer->increase_depth (iter);
      } 
      else {
        note_buffer->decrease_depth (iter);
      }

      buffer->move_mark (buffer->get_insert(), iter);
      buffer->move_mark (buffer->get_selection_bound(), iter);
    }
  }


  void ChangeDepthAction::merge (EditAction * )
  {
    throw sharp::Exception ("ChangeDepthActions cannot be merged");
  }


  bool ChangeDepthAction::can_merge (const EditAction * ) const
  {
    return false;
  }


  void ChangeDepthAction::destroy ()
  {
  }
  


  InsertBulletAction::InsertBulletAction(int offset, int depth, 
                                         Pango::Direction direction)
    : m_offset(offset)
    , m_depth(depth)
    , m_direction(direction)
  {
  }


  void InsertBulletAction::undo (Gtk::TextBuffer * buffer)
  {
    Gtk::TextIter iter = buffer->get_iter_at_offset (m_offset);
    iter.forward_line ();
    iter = buffer->get_iter_at_line (iter.get_line());

    dynamic_cast<NoteBuffer*>(buffer)->remove_bullet (iter);

    iter.forward_to_line_end ();

    buffer->move_mark (buffer->get_insert(), iter);
    buffer->move_mark (buffer->get_selection_bound(), iter);
  }


  void InsertBulletAction::redo (Gtk::TextBuffer * buffer)
  {
    Gtk::TextIter iter = buffer->get_iter_at_offset (m_offset);
    iter = buffer->insert (iter, "\n");

    dynamic_cast<NoteBuffer*>(buffer)->insert_bullet (iter, 
                                                      m_depth, m_direction);

    buffer->move_mark (buffer->get_insert(), iter);
    buffer->move_mark (buffer->get_selection_bound(), iter);
  }


  void InsertBulletAction::merge (EditAction * )
  {
    throw sharp::Exception ("InsertBulletActions cannot be merged");
  }


  bool InsertBulletAction::can_merge (const EditAction * ) const
  {
    return false;
  }


  void InsertBulletAction::destroy ()
  {
  }
  

  UndoManager::UndoManager(NoteBuffer * buffer)
    : m_frozen_cnt(0)
    , m_try_merge(false)
    , m_buffer(buffer)
    , m_chop_buffer(new ChopBuffer(buffer->get_tag_table()))
  {
    
    buffer->signal_insert_text_with_tags
      .connect(sigc::mem_fun(*this, &UndoManager::on_insert_text)); // supposedly before
    buffer->signal_new_bullet_inserted
      .connect(sigc::mem_fun(*this, &UndoManager::on_bullet_inserted));
    buffer->signal_change_text_depth
      .connect(sigc::mem_fun(*this, &UndoManager::on_change_depth));
    buffer->signal_erase()
      .connect(sigc::mem_fun(*this, &UndoManager::on_delete_range), false);
    buffer->signal_apply_tag()
      .connect(sigc::mem_fun(*this, &UndoManager::on_tag_applied));
    buffer->signal_remove_tag()
      .connect(sigc::mem_fun(*this, &UndoManager::on_tag_removed));
  }


  UndoManager::~UndoManager()
  {
    clear_action_stack(m_undo_stack);
    clear_action_stack(m_redo_stack);
  }
  
  void UndoManager::undo_redo(std::stack<EditAction *> & pop_from,
                              std::stack<EditAction *> & push_to, bool is_undo)
  {
    if (!pop_from.empty()) {
      EditAction *action = pop_from.top ();
      pop_from.pop();

      freeze_undo ();
      if (is_undo) {
        action->undo (m_buffer);
      }
      else {
        action->redo (m_buffer);
      }
      thaw_undo ();

      push_to.push (action);

      // Lock merges until a new undoable event comes in...
      m_try_merge = false;

      if (pop_from.empty() || push_to.size() == 1) {
        m_undo_changed();
      }
    }
  }

  
  void UndoManager::clear_action_stack(std::stack<EditAction *> & stack)
  {
    while(!stack.empty()) {
      delete stack.top();
      stack.pop();
    }
  }

  void UndoManager::clear_undo_history()
  {
    clear_action_stack(m_undo_stack);
    clear_action_stack(m_redo_stack);
    m_undo_changed();
  }


  void UndoManager::add_undo_action(EditAction * action)
  {
    DBG_ASSERT(action, "action is NULL");
    if (m_try_merge && !m_undo_stack.empty()) {
      EditAction *top = m_undo_stack.top();

      if (top->can_merge (action)) {
        // Merging object should handle freeing
        // action's resources, if needed.
        top->merge (action);
        delete action;
        return;
      }
    }

    m_undo_stack.push (action);

    // Clear the redo stack
    clear_action_stack (m_redo_stack);

    // Try to merge new incoming actions...
    m_try_merge = true;

    // Have undoable actions now
    if (m_undo_stack.size() == 1) {
      m_undo_changed();
    }
  }


  void UndoManager::on_insert_text(const Gtk::TextIter & pos, 
                                   const Glib::ustring & text, int)
  {
    if (m_frozen_cnt) {
      return;
    }

    InsertAction *action = new InsertAction (pos,
                                             text, text.length(),
                                             m_chop_buffer);

    /*
     * If this insert occurs in the middle of any
     * non-splittable tags, remove them first and
     * add them to the InsertAction.
     */
    m_frozen_cnt++;
    action->split(pos, m_buffer);
    m_frozen_cnt--;

    add_undo_action (action);
  }


  void UndoManager::on_delete_range(const Gtk::TextIter & start, 
                                    const Gtk::TextIter & end)
  {
    if (m_frozen_cnt) {
      return;
    }
    EraseAction *action = new EraseAction (start, end,
                                           m_chop_buffer);
    /*
     * Delete works a lot like insert here, except
     * there are two positions in the buffer that
     * may need to have their tags removed.
     */
    m_frozen_cnt++;
    action->split (start, m_buffer);
    action->split (end, m_buffer);
    m_frozen_cnt--;

    add_undo_action (action);
  }


  void UndoManager::on_tag_applied(const Glib::RefPtr<Gtk::TextTag> & tag,
                                   const Gtk::TextIter & start_char, 
                                   const Gtk::TextIter & end_char)
  {
    if(m_frozen_cnt) {
      return;
    }
    if (NoteTagTable::tag_is_undoable (tag)) {
      add_undo_action (new TagApplyAction (tag, start_char,
                                           end_char));
    }
  }


  void UndoManager::on_tag_removed(const Glib::RefPtr<Gtk::TextTag> & tag,
                                   const Gtk::TextIter & start_char, 
                                   const Gtk::TextIter & end_char)
  {
    if(m_frozen_cnt) {
      return;
    }
    if (NoteTagTable::tag_is_undoable (tag)) {
      add_undo_action (new TagRemoveAction (tag, start_char,
                                            end_char));
    }
  }


  void UndoManager::on_change_depth(int line, bool direction)
  {
    if(m_frozen_cnt) {
      return;
    }
    add_undo_action(new ChangeDepthAction(line, direction));
  }

  void UndoManager::on_bullet_inserted(int offset, int depth, 
                                       Pango::Direction direction)
  {
    if(m_frozen_cnt) {
      return;
    }
    add_undo_action(new InsertBulletAction(offset, depth, direction));
  }


}

