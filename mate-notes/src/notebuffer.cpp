/*
 * gnote
 *
 * Copyright (C) 2010-2011 Aurimas Cernius
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



#include <algorithm>
#include <tr1/array>


#include "debug.hpp"
#include "notebuffer.hpp"
#include "notetag.hpp"
#include "note.hpp"
#include "preferences.hpp"
#include "undo.hpp"

#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"

namespace gnote {


#define NUM_INDENT_BULLETS 3
  const gunichar NoteBuffer::s_indent_bullets[NUM_INDENT_BULLETS] = { 0x2022, 0x2218, 0x2023 };

  bool NoteBuffer::is_bullet(gunichar c)
  {
    for (int i = 0; i < NUM_INDENT_BULLETS; ++i)
      if (c == s_indent_bullets[i])
        return true;

    return false;
  }

  bool NoteBuffer::get_enable_auto_bulleted_lists() const
  {
    return Preferences::obj().get<bool>(Preferences::ENABLE_AUTO_BULLETED_LISTS);
  }
  

  NoteBuffer::NoteBuffer(const NoteTagTable::Ptr & tags, Note & note)
    : Gtk::TextBuffer(tags)
    , m_undomanager(NULL)
    , m_note(note)
  {
    m_undomanager = new UndoManager(this);
    signal_insert().connect(sigc::mem_fun(*this, &NoteBuffer::text_insert_event));
    signal_erase().connect(sigc::mem_fun(*this, &NoteBuffer::range_deleted_event));
    signal_mark_set().connect(sigc::mem_fun(*this, &NoteBuffer::mark_set_event));

    signal_apply_tag().connect(sigc::mem_fun(*this, &NoteBuffer::on_tag_applied));
    

    tags->signal_tag_changed().connect(sigc::mem_fun(*this, &NoteBuffer::on_tag_changed));
  }


  NoteBuffer::~NoteBuffer()
  {
    delete m_undomanager;
  }

  void NoteBuffer::toggle_active_tag(const std::string & tag_name)
  {
    DBG_OUT("ToggleTag called for '%s'", tag_name.c_str());
    
    Glib::RefPtr<Gtk::TextTag> tag = get_tag_table()->lookup(tag_name);
    Gtk::TextIter select_start, select_end;

    if (get_selection_bounds(select_start, select_end)) {
      // Ignore the bullet character
      if (find_depth_tag(select_start))
        select_start.set_line_offset(2);

      if (select_start.begins_tag(tag) || select_start.has_tag(tag)) {
        remove_tag(tag, select_start, select_end);
      }
      else {
        apply_tag(tag, select_start, select_end);
      }
    } 
    else {
      std::list<Glib::RefPtr<Gtk::TextTag> >::iterator iter = std::find(m_active_tags.begin(), 
                                                                   m_active_tags.end(), tag);
      if (iter != m_active_tags.end()) {
        m_active_tags.erase(iter);
      }
      else {
        m_active_tags.push_back(tag);
      }
    }
  }

  void NoteBuffer::set_active_tag (const std::string & tag_name)
  {
    DBG_OUT("SetTag called for '%s'", tag_name.c_str());

    Glib::RefPtr<Gtk::TextTag> tag = get_tag_table()->lookup(tag_name);
    Gtk::TextIter select_start, select_end;

    if (get_selection_bounds(select_start, select_end)) {
      apply_tag(tag, select_start, select_end);
    } 
    else {
      m_active_tags.push_back(tag);
    }
  }

  void NoteBuffer::remove_active_tag (const std::string & tag_name)
  {
    DBG_OUT("remove_tagcalled for '%s'", tag_name.c_str());

    Glib::RefPtr<Gtk::TextTag> tag = get_tag_table()->lookup(tag_name);
    Gtk::TextIter select_start, select_end;

    if (get_selection_bounds(select_start, select_end)) {
      remove_tag(tag, select_start, select_end);
    } 
    else {
      std::list<Glib::RefPtr<Gtk::TextTag> >::iterator iter = std::find(m_active_tags.begin(), 
                                                                   m_active_tags.end(), tag);
      if (iter != m_active_tags.end()) {
        m_active_tags.erase(iter);
      }
    }
  }


  /// <summary>
  /// Returns the specified DynamicNoteTag if one exists on the TextIter
  /// or null if none was found.
  /// </summary>
  DynamicNoteTag::ConstPtr NoteBuffer::get_dynamic_tag (const std::string  & tag_name, 
                                                        const Gtk::TextIter & iter)
  {
    // TODO: Is this variables used, or do we just need to
    // access iter.Tags to work around a bug?
    Glib::SListHandle<Glib::RefPtr<const Gtk::TextTag> > tag_list = iter.get_tags();
    for(Glib::SListHandle<Glib::RefPtr<const Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
        tag_iter != tag_list.end(); ++tag_iter) {
      const Glib::RefPtr<const Gtk::TextTag> & tag(*tag_iter);
      DynamicNoteTag::ConstPtr dynamic_tag =  DynamicNoteTag::ConstPtr::cast_dynamic(tag);
      if (dynamic_tag &&
          (dynamic_tag->get_element_name() == tag_name)) {
        return dynamic_tag;
      }
    }

    return DynamicNoteTag::ConstPtr();
  }

  
  void NoteBuffer::on_tag_applied(const Glib::RefPtr<Gtk::TextTag> & tag1,
                                  const Gtk::TextIter & start_char, const Gtk::TextIter &end_char)
  {
    DepthNoteTag::Ptr dn_tag = DepthNoteTag::Ptr::cast_dynamic(tag1);
    if (!dn_tag) {
      // Remove the tag from any bullets in the selection
      m_undomanager->freeze_undo();
      Gtk::TextIter iter;
      for (int i = start_char.get_line(); i <= end_char.get_line(); i++) {
        iter = get_iter_at_line(i);

        if (find_depth_tag(iter)) {
          Gtk::TextIter next = iter;
          next.forward_chars(2);
          remove_tag(tag1, iter, next);
        }
      }
      m_undomanager->thaw_undo();
    } 
    else {
      // Remove any existing tags when a depth tag is applied
      m_undomanager->freeze_undo();
      Glib::SListHandle<Glib::RefPtr<const Gtk::TextTag> > tag_list = start_char.get_tags();
      for(Glib::SListHandle<Glib::RefPtr<const Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
          tag_iter != tag_list.end(); ++tag_iter) {
        const Glib::RefPtr<const Gtk::TextTag> & tag(*tag_iter);
        DepthNoteTag::ConstPtr dn_tag2 = DepthNoteTag::ConstPtr::cast_dynamic(tag);
        if (!dn_tag2) {
          // here it gets hairy. Gtkmm does not implement remove_tag() on a const.
          // given that Gtk does not have const, I assume I can work that out.
          remove_tag(Glib::RefPtr<Gtk::TextTag>::cast_const(tag), start_char, end_char);
        }
      }
      m_undomanager->thaw_undo();
    }
  }


  bool NoteBuffer::is_active_tag(const std::string & tag_name)
  {
    Glib::RefPtr<Gtk::TextTag> tag = get_tag_table()->lookup(tag_name);
    Gtk::TextIter iter, select_end;

    if (get_selection_bounds (iter, select_end)) {
      // Ignore the bullet character and look at the
      // first character of the list item
      if (find_depth_tag(iter)) {
        iter.forward_chars(2);
      }
      return iter.begins_tag(tag) || iter.has_tag(tag);
    } 
    else {
      return (find(m_active_tags.begin(), m_active_tags.end(), tag) != m_active_tags.end());
    }
  }

    // Returns true if the cursor is inside of a bulleted list
  bool NoteBuffer::is_bulleted_list_active()
  {
    Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
    Gtk::TextIter iter = get_iter_at_mark(insert_mark);
    iter.set_line_offset(0);

    Glib::RefPtr<Gtk::TextTag> depth = find_depth_tag(iter);

    return depth;
  }


  // Returns true if the cursor is at a position that can
    // be made into a bulleted list
  bool NoteBuffer::can_make_bulleted_list()
  {
    Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
    Gtk::TextIter iter = get_iter_at_mark(insert_mark);

    return iter.get_line();
  }

  // Apply active_tags to inserted text
  void NoteBuffer::text_insert_event(const Gtk::TextIter & pos, const Glib::ustring & text, int bytes)
  {
    // Check for bullet paste
    if(text.size() == 2 && is_bullet(text[0])) {
      signal_change_text_depth(pos.get_line(), true);
    }
    else {
      // Only apply active tags when typing, not on paste.
      if (text.size() == 1) {
        Gtk::TextIter insert_start(pos);
        insert_start.backward_chars (text.size());

        m_undomanager->freeze_undo();
        Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list = insert_start.get_tags();
        for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
            tag_iter != tag_list.end(); ++tag_iter) {
          remove_tag(*tag_iter, insert_start, pos);
        }

        for(std::list<Glib::RefPtr<Gtk::TextTag> >::const_iterator iter = m_active_tags.begin();
            iter != m_active_tags.end(); ++iter) {
          apply_tag(*iter, insert_start, pos);
        }
        m_undomanager->thaw_undo();
      }
      else {
        DepthNoteTag::Ptr depth_tag;
        Gtk::TextIter line_start(pos);
        line_start.backward_chars(text.size());
        if(line_start.get_line_offset() == 2) {
          line_start.set_line_offset(0);
          depth_tag = find_depth_tag(line_start);
        }

        if(depth_tag) {
          Pango::Direction direction = Pango::DIRECTION_LTR;
          if(text.size() > 0) {
            direction = Pango::Direction(pango_unichar_direction(text[0]));
          }
          change_bullet_direction(pos, direction);
          for(int i = 0; i < depth_tag->get_depth(); ++i) {
            signal_change_text_depth(line_start.get_line(), true);
          }
        }
      }

      signal_insert_text_with_tags(pos, text, bytes);
    }
  }


  // Change the direction of a bulleted line to match the new
  // first character after the previous character is deleted.
  void NoteBuffer::range_deleted_event(const Gtk::TextIter & start,const Gtk::TextIter & end_iter)
  {
    //
    std::tr1::array<Gtk::TextIter, 2> iters;
    iters[0] = start;
    iters[1] = end_iter;

    for(std::tr1::array<Gtk::TextIter, 2>::iterator iter2 = iters.begin();
        iter2 != iters.end(); ++iter2) {
      Gtk::TextIter & iter(*iter2);
      Gtk::TextIter line_start = iter;
      line_start.set_line_offset(0);

      if (((iter.get_line_offset() == 3) || (iter.get_line_offset() == 2)) &&
          find_depth_tag(line_start)) {

        Gtk::TextIter first_char = iter;
        first_char.set_line_offset(2);

        Pango::Direction direction = Pango::DIRECTION_LTR;

        if (first_char.get_char() > 0)
          direction = Pango::Direction(pango_unichar_direction(first_char.get_char()));

        change_bullet_direction(first_char, direction);
      }
    }
  }


  bool NoteBuffer::add_new_line(bool soft_break)
  {
    if (!can_make_bulleted_list() || !get_enable_auto_bulleted_lists())
      return false;

    Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
    Gtk::TextIter iter = get_iter_at_mark(insert_mark);
    iter.set_line_offset(0);

    DepthNoteTag::Ptr prev_depth = find_depth_tag(iter);
      
    Gtk::TextIter insert_iter = get_iter_at_mark(insert_mark);
 
    // Insert a LINE SEPARATOR character which allows us
    // to have multiple lines in a single bullet point
    if (prev_depth && soft_break) {
      bool at_end_of_line = insert_iter.ends_line();
      insert_iter = insert(insert_iter, Glib::ustring(1, (gunichar)0x2028));
        
      // Hack so that the user sees that what they type
      // next will appear on a new line, otherwise the
      // cursor stays at the end of the previous line.
      if (at_end_of_line) {
        insert_iter = insert(insert_iter, " ");
        Gtk::TextIter bound = insert_iter;
        bound.backward_char();
        move_mark(get_selection_bound(), bound);
      }
        
      return true;      

      // If the previous line has a bullet point on it we add a bullet
      // to the new line, unless the previous line was blank (apart from
      // the bullet), in which case we clear the bullet/indent from the
      // previous line.
    } 
    else if (prev_depth) {
      iter.forward_char();

      // See if the line was left contentless and remove the bullet
      // if so.
      if (iter.ends_line() || insert_iter.get_line_offset() < 3 ) {
        Gtk::TextIter start = get_iter_at_line(iter.get_line());
        Gtk::TextIter end_iter = start;
        end_iter.forward_to_line_end();

        if (end_iter.get_line_offset() < 2) {
          end_iter = start;
        } 
        else {
          end_iter = get_iter_at_line_offset(iter.get_line(), 2);
        }

        erase(start, end_iter);

        iter = get_iter_at_mark(insert_mark);
        insert(iter, "\n");
      } 
      else {
        iter = get_iter_at_mark(insert_mark);
        Gtk::TextIter prev = iter;
        prev.backward_char();
          
        // Remove soft breaks
        if (prev.get_char() == 0x2028) {
          iter = erase(prev, iter);
        }
          
        m_undomanager->freeze_undo();
        int offset = iter.get_offset();
        insert(iter, "\n");

        iter = get_iter_at_mark(insert_mark);
        Gtk::TextIter start = get_iter_at_line(iter.get_line());

        // Set the direction of the bullet to be the same
        // as the first character on the new line
        Pango::Direction direction = prev_depth->get_direction();
        if ((iter.get_char() != '\n') && (iter.get_char() > 0)) {
          direction = Pango::Direction(pango_unichar_direction(iter.get_char()));
        }

        insert_bullet(start, prev_depth->get_depth(), direction);
        m_undomanager->thaw_undo();

        signal_new_bullet_inserted(offset, prev_depth->get_depth(), direction);
      }

      return true;
    }
    // Replace lines starting with any numbers of leading spaces 
    // followed by '*' or '-' and then by a space with bullets
    else if (line_needs_bullet(iter)) {
      Gtk::TextIter start = get_iter_at_line_offset (iter.get_line(), 0);
      Gtk::TextIter end_iter = get_iter_at_line_offset (iter.get_line(), 0);

      // Remove any leading white space
      while (end_iter.get_char() == ' ') {
        end_iter.forward_char();
      }
      // Remove the '*' or '-' character and the space after
      end_iter.forward_chars(2);
        
      // Set the direction of the bullet to be the same as
      // the first character after the '*' or '-'
      Pango::Direction direction = Pango::DIRECTION_LTR;
      if (end_iter.get_char() > 0)
        direction = Pango::Direction(pango_unichar_direction(end_iter.get_char()));

      end_iter = erase(start, end_iter);
      start = end_iter;
      if (end_iter.ends_line()) {
        increase_depth(start);
      }
      else {
        increase_depth(start);

        iter = get_iter_at_mark(insert_mark);
        int offset = iter.get_offset();
        insert(iter, "\n");

        iter = get_iter_at_mark(insert_mark);
        iter.set_line_offset(0);

        m_undomanager->freeze_undo();
        insert_bullet (iter, 0, direction);
        m_undomanager->thaw_undo();

        signal_new_bullet_inserted(offset, 0, direction);
      }

      return true;
    }

    return false;
  }


  // Returns true if line starts with any numbers of leading spaces
  // followed by '*' or '-' and then by a space
  bool NoteBuffer::line_needs_bullet(Gtk::TextIter iter)
  {
    while (!iter.ends_line()) {
      switch (iter.get_char()) {
      case ' ':
        iter.forward_char();
        break;
      case '*':
      case '-':
        return (get_iter_at_line_offset(iter.get_line(), iter.get_line_offset() + 1).get_char() == ' ');
      default:
        return false;
      }
    }
    return false;
  }

  // Returns true if the depth of the line was increased 
  bool NoteBuffer::add_tab()
  {
    Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
    Gtk::TextIter iter = get_iter_at_mark(insert_mark);
    iter.set_line_offset(0);

    DepthNoteTag::Ptr depth = find_depth_tag(iter);

    // If the cursor is at a line with a depth and a tab has been
    // inserted then we increase the indent depth of that line.
    if (depth) {
      increase_depth(iter);
      return true;
    }
    return false;
  }


  // Returns true if the depth of the line was decreased
  bool NoteBuffer::remove_tab()
  {
    Glib::RefPtr<Gtk::TextMark> insert_mark = get_insert();
    Gtk::TextIter iter = get_iter_at_mark(insert_mark);
    iter.set_line_offset(0);

    DepthNoteTag::Ptr depth = find_depth_tag(iter);

    // If the cursor is at a line with depth and a tab has been
    // inserted, then we decrease the depth of that line.
    if (depth) {
      decrease_depth(iter);
      return true;
    }

    return false;
  }


  // Returns true if a bullet had to be removed
    // This is for the Delete key not Backspace
  bool NoteBuffer::delete_key_handler()
  {
    // See if there is a selection
    Gtk::TextIter start;
    Gtk::TextIter end_iter;

    bool selection = get_selection_bounds(start, end_iter);

    if (selection) {
      augment_selection(start, end_iter);
      erase(start, end_iter);
      return true;
    } 
    else if (start.ends_line() && start.get_line() < get_line_count()) {
      Gtk::TextIter next = get_iter_at_line (start.get_line() + 1);
      end_iter = start;
      end_iter.forward_chars(3);

      DepthNoteTag::Ptr depth = find_depth_tag(next);

      if (depth) {
        erase(start, end_iter);
        return true;
      }
    } 
    else {
      Gtk::TextIter next = start;

      if (next.get_line_offset() != 0)
        next.forward_char();

      DepthNoteTag::Ptr depth = find_depth_tag(start);
      DepthNoteTag::Ptr nextDepth = find_depth_tag(next);
      if (depth || nextDepth) {
        decrease_depth (start);
        return true;
      }
    }

    return false;
  }


  bool NoteBuffer::backspace_key_handler()
  {
    Gtk::TextIter start;
    Gtk::TextIter end_iter;

    bool selection = get_selection_bounds(start, end_iter);

    DepthNoteTag::Ptr depth = find_depth_tag(start);

    if (selection) {
      augment_selection(start, end_iter);
      erase(start, end_iter);
      return true;
    } 
    else {
      // See if the cursor is inside or just after a bullet region
      // ie.
      // |* lorum ipsum
      //  ^^^
      // and decrease the depth if it is.

      Gtk::TextIter prev = start;

      if (prev.get_line_offset())
        prev.backward_chars (1);

      DepthNoteTag::Ptr prev_depth = find_depth_tag(prev);
      if (depth || prev_depth) {
        decrease_depth(start);
        return true;
      } 
      else {
        // See if the cursor is before a soft line break
        // and remove it if it is. Otherwise you have to
        // press backspace twice before  it will delete
        // the previous visible character.
        prev = start;
        prev.backward_chars (2);
        if (prev.get_char() == 0x2028) {
          Gtk::TextIter end_break = prev;
          end_break.forward_char();
          erase(prev, end_break);
        }
      }
    }

    return false;
  }


  // On an InsertEvent we change the selection (if there is one)
    // so that it doesn't slice through bullets.
  void NoteBuffer::check_selection()
  {
    Gtk::TextIter start;
    Gtk::TextIter end_iter;

    bool selection = get_selection_bounds(start, end_iter);

    if (selection) {
      augment_selection(start, end_iter);
    } 
    else {
      // If the cursor is at the start of a bulleted line
      // move it so it is after the bullet.
      if ((start.get_line_offset() == 0 || start.get_line_offset() == 1) &&
          find_depth_tag(start))
      {
        start.set_line_offset(2);
        select_range (start, start);
      }
    }
  }


  // Change the selection on the buffer taking into account any
  // bullets that are in or near the seletion
  void NoteBuffer::augment_selection(Gtk::TextIter & start, Gtk::TextIter & end_iter)
  {
    DepthNoteTag::Ptr start_depth = find_depth_tag(start);
    DepthNoteTag::Ptr end_depth = find_depth_tag(end_iter);

    Gtk::TextIter inside_end = end_iter;
    inside_end.backward_char();

    DepthNoteTag::Ptr inside_end_depth = find_depth_tag(inside_end);

    // Start inside bullet region
    if (start_depth) {
      start.set_line_offset(2);
      select_range(start, end_iter);
    }

    // End inside another bullet
    if (inside_end_depth) {
      end_iter.set_line_offset(2);
      select_range (start, end_iter);
    }

    // Check if the End is right before start of bullet
    if (end_depth) {
      end_iter.set_line_offset(2);
      select_range(start, end_iter);
    }
  }

  // Clear active tags, and add any tags which should be applied:
  // - Avoid the having tags grow frontwords by not adding tags
  //   which start on the next character.
  // - Add tags ending on the prior character, to avoid needing to
  //   constantly toggle tags.
  void NoteBuffer::mark_set_event(const Gtk::TextIter &,const Glib::RefPtr<Gtk::TextBuffer::Mark> & mark)
  {
    if (mark != get_insert()) {
      return;
    }

    m_active_tags.clear();

    Gtk::TextIter iter = get_iter_at_mark(mark);

    // Add any growable tags not starting on the next character...
    const Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list(iter.get_tags());
    for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
        tag_iter != tag_list.end(); ++tag_iter) {
      const Glib::RefPtr<Gtk::TextTag> & tag(*tag_iter);
      if (!iter.begins_tag(tag) && NoteTagTable::tag_is_growable(tag)) {
        m_active_tags.push_back(tag);
      }
    }

    // Add any growable tags not ending on the prior character..
    const Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list2(iter.get_toggled_tags(false));
    for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list2.begin();
        tag_iter != tag_list2.end(); ++tag_iter) {
      const Glib::RefPtr<Gtk::TextTag> & tag(*tag_iter);
      if (!iter.ends_tag(tag) && NoteTagTable::tag_is_growable(tag)) {
        m_active_tags.push_back(tag);
      }
    }
  }


  void NoteBuffer::widget_swap (const NoteTag::Ptr & tag, const Gtk::TextIter & start,
                                const Gtk::TextIter & /*end*/, bool adding)
  {
    if (tag->get_widget() == NULL)
      return;

    Gtk::TextIter prev = start;
    prev.backward_char();

    WidgetInsertData data;
    data.buffer = start.get_buffer();
    data.tag = tag;
    data.widget = tag->get_widget();
    data.adding = adding;

    if (adding) {
      data.position = start.get_buffer()->create_mark (start, true);
    } 
    else {
      data.position = tag->get_widget_location();
    }

    m_widget_queue.push(data);

    if (!m_widget_queue_timeout) {
      m_widget_queue_timeout = Glib::signal_idle()
        .connect(sigc::mem_fun(*this, &NoteBuffer::run_widget_queue));
    }
  }


  bool NoteBuffer::run_widget_queue()
  {
    while(!m_widget_queue.empty()) {
      const WidgetInsertData & data(m_widget_queue.front());
      // HACK: This is a quick fix for bug #486551
      if (data.position) {
        NoteBuffer::Ptr buffer = NoteBuffer::Ptr::cast_static(data.buffer);
        Gtk::TextIter iter = buffer->get_iter_at_mark(data.position);
        Glib::RefPtr<Gtk::TextMark> location = data.position;

        // Prevent the widget from being inserted before a bullet
        if (find_depth_tag(iter)) {
          iter.set_line_offset(2);
          location = create_mark(data.position->get_name(), iter, data.position->get_left_gravity());
        }

        buffer->undoer().freeze_undo();

        if (data.adding && !data.tag->get_widget_location()) {
          Glib::RefPtr<Gtk::TextChildAnchor> childAnchor = buffer->create_child_anchor(iter);
          data.tag->set_widget_location(location);
          m_note.add_child_widget(childAnchor, data.widget);
        }
        else if (!data.adding && data.tag->get_widget_location()) {
          Gtk::TextIter end_iter = iter;
          end_iter.forward_char();
          buffer->erase(iter, end_iter);
          buffer->delete_mark(location);
          data.tag->set_widget_location(Glib::RefPtr<Gtk::TextMark>());
        }
        buffer->undoer().thaw_undo();
      }
      m_widget_queue.pop();
    }

//      m_widget_queue_timeout = 0;
    return false;
  }

  void NoteBuffer::on_tag_changed(const Glib::RefPtr<Gtk::TextTag> & tag, bool)
  {
    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if (note_tag) {
      utils::TextTagEnumerator enumerator(Glib::RefPtr<Gtk::TextBuffer>(this), note_tag);
      while(enumerator.move_next()) {
        const utils::TextRange & range(enumerator.current());
        widget_swap(note_tag, range.start(), range.end(), true);
      }
    }
  }

  void NoteBuffer::on_apply_tag(const Glib::RefPtr<Gtk::TextTag> & tag,
                       const Gtk::TextIter & start,  const Gtk::TextIter &end_iter)
  {
    Gtk::TextBuffer::on_apply_tag(tag, start, end_iter);

    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if (note_tag) {
      widget_swap(note_tag, start, end_iter, true);
    }
  }

  void NoteBuffer::on_remove_tag(const Glib::RefPtr<Gtk::TextTag> & tag,
                                 const Gtk::TextIter & start,  const Gtk::TextIter & end_iter)
  {
    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if (note_tag) {
      widget_swap(note_tag, start, end_iter, false);
    }

    Gtk::TextBuffer::on_remove_tag(tag, start, end_iter);
  }

  std::string NoteBuffer::get_selection() const
  {
    Gtk::TextIter select_start, select_end;
    std::string text;
    
    if (get_selection_bounds(select_start, select_end)) {
      text = get_text(select_start, select_end, false);
    }

    return text;
  }


  void NoteBuffer::get_block_extents(Gtk::TextIter & start, Gtk::TextIter & end_iter,
                                     int threshold, const Glib::RefPtr<Gtk::TextTag> & avoid_tag)
  {
    // Move start and end to the beginning or end of their
    // respective paragraphs, bounded by some threshold.

    start.set_line_offset(std::max(0, start.get_line_offset() - threshold));

    // FIXME: Sometimes I need to access this before it
    // returns real values.
    (void)end_iter.get_chars_in_line();

    if (end_iter.get_chars_in_line() - end_iter.get_line_offset() > (threshold + 1 /* newline */)) {
      end_iter.set_line_offset(end_iter.get_line_offset() + threshold);
    }
    else {
      end_iter.forward_to_line_end();
    }

    if (avoid_tag) {
      if (start.has_tag(avoid_tag)) {
        start.backward_to_tag_toggle(avoid_tag);
      }

      if (end_iter.has_tag(avoid_tag)) {
        end_iter.forward_to_tag_toggle(avoid_tag);
      }
    }
  }

  void NoteBuffer::toggle_selection_bullets()
  {
    Gtk::TextIter start;
    Gtk::TextIter end_iter;

    get_selection_bounds (start, end_iter);

    start = get_iter_at_line_offset(start.get_line(), 0);

    bool toggle_on = true;
    if (find_depth_tag(start)) {
      toggle_on = false;
    }

    int start_line = start.get_line();
    int end_line = end_iter.get_line();

    for (int i = start_line; i <= end_line; i++) {
      Gtk::TextIter curr_line = get_iter_at_line(i);
      if (toggle_on && !find_depth_tag(curr_line)) {
        increase_depth(curr_line);
      } 
      else if (!toggle_on && find_depth_tag(curr_line)) {
        Gtk::TextIter bullet_end = get_iter_at_line_offset(curr_line.get_line(), 2);
        erase(curr_line, bullet_end);
      }
    }
  }


  void NoteBuffer::change_cursor_depth_directional(bool right)
  {
    Gtk::TextIter start;
    Gtk::TextIter end_iter;

    get_selection_bounds (start, end_iter);

    // If we are moving right then:
    //   RTL => decrease depth
    //   LTR => increase depth
    // We choose to increase or decrease the depth
    // based on the fist line in the selection.
    bool increase = right;
    start.set_line_offset(0);
    DepthNoteTag::Ptr start_depth = find_depth_tag (start);

    Gtk::TextIter next = start;

    if (start_depth) {
      next.forward_chars (2);
    } 
    else {
      // Look for the first non-space character on the line
      // and use that to determine what direction we should go
      next.forward_sentence_end ();
      next.backward_sentence_start ();
    }

    change_cursor_depth(increase);
  }

  void NoteBuffer::change_cursor_depth(bool increase)
  {
    Gtk::TextIter start;
    Gtk::TextIter end_iter;

    get_selection_bounds (start, end_iter);

    Gtk::TextIter curr_line;

    int start_line = start.get_line();
    int end_line = end_iter.get_line();

    for (int i = start_line; i <= end_line; i++) {
      curr_line = get_iter_at_line(i);
      if (increase)
        increase_depth (curr_line);
      else
        decrease_depth (curr_line);
    }
  }


  // Change the writing direction (ie. RTL or LTR) of a bullet.
  // This makes the bulleted line use the correct indent
  void NoteBuffer::change_bullet_direction(Gtk::TextIter iter, Pango::Direction direction)
  {
    iter.set_line_offset(0);

    DepthNoteTag::Ptr tag = find_depth_tag (iter);
    if (tag) {
      if ((tag->get_direction() != direction) &&
          (direction != Pango::DIRECTION_NEUTRAL)) {
        NoteTagTable::Ptr note_table = NoteTagTable::Ptr::cast_dynamic(get_tag_table());

        // Get the depth tag for the given direction
        Glib::RefPtr<Gtk::TextTag> new_tag = note_table->get_depth_tag (tag->get_depth(), direction);

        Gtk::TextIter next = iter;
        next.forward_char ();

        // Replace the old depth tag with the new one
        remove_all_tags (iter, next);
        apply_tag (new_tag, iter, next);
      }
    }
  }


  void NoteBuffer::insert_bullet(Gtk::TextIter & iter, int depth, Pango::Direction direction)
  {
    NoteTagTable::Ptr note_table = NoteTagTable::Ptr::cast_dynamic(get_tag_table());

    DepthNoteTag::Ptr tag = note_table->get_depth_tag (depth, direction);

    Glib::ustring bullet =
      Glib::ustring(1, s_indent_bullets [depth % NUM_INDENT_BULLETS]) + " ";

    iter = insert_with_tag (iter, bullet, tag);
  }

  void NoteBuffer::remove_bullet(Gtk::TextIter & iter)
  {
    Gtk::TextIter end_iter;
    Gtk::TextIter line_end = iter;

    line_end.forward_to_line_end ();

    if (line_end.get_line_offset() < 2) {
      end_iter = get_iter_at_line_offset (iter.get_line(), 1);
    } 
    else {
      end_iter = get_iter_at_line_offset (iter.get_line(), 2);
    }

    // Go back one more character to delete the \n as well
    iter = get_iter_at_line (iter.get_line() - 1);
    iter.forward_to_line_end ();

    iter = erase(iter, end_iter);
  }


  void NoteBuffer::increase_depth(Gtk::TextIter & start)
  {
    if (!can_make_bulleted_list())
      return;

    Gtk::TextIter end_iter;

    start = get_iter_at_line_offset (start.get_line(), 0);

    Gtk::TextIter line_end = get_iter_at_line (start.get_line());
    line_end.forward_to_line_end ();

    end_iter = start;
    end_iter.forward_chars(2);

    DepthNoteTag::Ptr curr_depth = find_depth_tag (start);

    undoer().freeze_undo ();
    if (!curr_depth) {
      // Insert a brand new bullet
      Gtk::TextIter next = start;
      next.forward_sentence_end ();
      next.backward_sentence_start ();

      // Insert the bullet using the same direction
      // as the text on the line
      Pango::Direction direction = Pango::DIRECTION_LTR;
      if ((next.get_char() > 0) && (next.get_line() == start.get_line()))
        direction = Pango::Direction(pango_unichar_direction(next.get_char()));

      insert_bullet (start, 0, direction);
    } 
    else {
      // Remove the previous indent
      start = erase (start, end_iter);

      // Insert the indent at the new depth
      int nextDepth = curr_depth->get_depth() + 1;
      insert_bullet (start, nextDepth, curr_depth->get_direction());
    }
    undoer().thaw_undo ();

    signal_change_text_depth (start.get_line(), true);
  }

  void NoteBuffer::decrease_depth(Gtk::TextIter & start)
  {
    if (!can_make_bulleted_list())
      return;

    Gtk::TextIter end_iter;

    start = get_iter_at_line_offset (start.get_line(), 0);

    Gtk::TextIter line_end = start;
    line_end.forward_to_line_end ();

    if ((line_end.get_line_offset() < 2) || start.ends_line()) {
      end_iter = start;
    } 
    else {
      end_iter = get_iter_at_line_offset (start.get_line(), 2);
    }

    DepthNoteTag::Ptr curr_depth = find_depth_tag (start);

    undoer().freeze_undo ();
    if (curr_depth) {
      // Remove the previous indent
      start = erase(start, end_iter);

      // Insert the indent at the new depth
      int nextDepth = curr_depth->get_depth() - 1;

      if (nextDepth != -1) {
        insert_bullet (start, nextDepth, curr_depth->get_direction());
      }
    }
    undoer().thaw_undo ();

    signal_change_text_depth (start.get_line(), false);
  }


  DepthNoteTag::Ptr NoteBuffer::find_depth_tag(Gtk::TextIter & iter)
  {
    DepthNoteTag::Ptr depth_tag;

    Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list = iter.get_tags();
    for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
        tag_iter != tag_list.end(); ++tag_iter) {
      const Glib::RefPtr<Gtk::TextTag> & tag(*tag_iter);
      if (NoteTagTable::tag_has_depth (tag)) {
        depth_tag = DepthNoteTag::Ptr::cast_dynamic(tag);
        break;
      }
    }

    return depth_tag;
  }

  void NoteBuffer::select_note_body()
  {
    Glib::ustring title = m_note.get_title();
    Gtk::TextIter iter = get_iter_at_offset(title.length());
    while(isspace(*iter))
      iter.forward_char();
    move_mark(get_selection_bound(), iter);
    move_mark(get_insert(), end());
  }

  std::string NoteBufferArchiver::serialize(const Glib::RefPtr<Gtk::TextBuffer> & buffer)
  {
    return serialize(buffer, buffer->begin(), buffer->end());
  }


  std::string NoteBufferArchiver::serialize(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
                                            const Gtk::TextIter & start,
                                            const Gtk::TextIter & end)
  {
    sharp::XmlWriter xml;
    
    serialize(buffer, start, end, xml);
    xml.close();
    std::string serializedBuffer = xml.to_string();
    // FIXME: there is some sort of attempt to ensure the endline are the
    // same on all platforms.
    return serializedBuffer;
  }


  void NoteBufferArchiver::write_tag(const Glib::RefPtr<const Gtk::TextTag> & tag, 
                                     sharp::XmlWriter & xml, bool start)
  {
    NoteTag::ConstPtr note_tag = NoteTag::ConstPtr::cast_dynamic(tag);
    if (note_tag) {
      note_tag->write (xml, start);
    } 
    else if (NoteTagTable::tag_is_serializable (tag)) {
      if (start) {
        xml.write_start_element ("", tag->property_name().get_value(), "");
      }
      else {
        xml.write_end_element ();
      }
    }
  }

  bool NoteBufferArchiver::tag_ends_here (const Glib::RefPtr<const Gtk::TextTag> & tag,
                                          const Gtk::TextIter & iter,
                                          const Gtk::TextIter & next_iter)
  {
    return (iter.has_tag (tag) && !next_iter.has_tag (tag)) || next_iter.is_end();
  }

  
  // This is taken almost directly from GAIM.  There must be a
  // better way to do this...
  void NoteBufferArchiver::serialize(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
                                     const Gtk::TextIter & start,
                                     const Gtk::TextIter & end, sharp::XmlWriter & xml)
  {
    std::stack<Glib::RefPtr<const Gtk::TextTag> > tag_stack;
    std::stack<Glib::RefPtr<const Gtk::TextTag> > replay_stack;
    std::stack<Glib::RefPtr<const Gtk::TextTag> > continue_stack;

    Gtk::TextIter iter = start;
    Gtk::TextIter next_iter = start;
    next_iter.forward_char();

    bool line_has_depth = false;
    int prev_depth_line = -1;
    int prev_depth = -1;

    xml.write_start_element ("", "note-content", "");
    xml.write_attribute_string ("", "version", "", "0.1");
    xml.write_attribute_string("xmlns",
                               "link",
                               "",
                               "http://beatniksoftware.com/tomboy/link");
    xml.write_attribute_string("xmlns",
                               "size",
                               "",
                               "http://beatniksoftware.com/tomboy/size");

    // Insert any active tags at start into tag_stack...
    Glib::SListHandle<Glib::RefPtr<const Gtk::TextTag> > tag_list = start.get_tags();
    for(Glib::SListHandle<Glib::RefPtr<const Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
        tag_iter != tag_list.end(); ++tag_iter) {
      const Glib::RefPtr<const Gtk::TextTag> & start_tag(*tag_iter);
      if (!start.toggles_tag (start_tag)) {
        tag_stack.push (start_tag);
        write_tag (start_tag, xml, true);
      }
    }

    while ((iter != end) && iter.get_char()) {
      DepthNoteTag::Ptr depth_tag = NoteBuffer::Ptr::cast_static(buffer)->find_depth_tag (iter);

      // If we are at a character with a depth tag we are at the
      // start of a bulleted line
      if (depth_tag && iter.starts_line()) {
        line_has_depth = true;

        if (iter.get_line() == prev_depth_line + 1) {
          // Line part of existing list

          if (depth_tag->get_depth() == prev_depth) {
            // Line same depth as previous
            // Close previous <list-item>
            xml.write_end_element ();

          }
          else if (depth_tag->get_depth() > prev_depth) {
            // Line of greater depth
            xml.write_start_element ("", "list", "");

            for (int i = prev_depth + 2; i <= depth_tag->get_depth(); i++) {
              // Start a new nested list
              xml.write_start_element ("", "list-item", "");
              xml.write_start_element ("", "list", "");
            }
          } 
          else {
            // Line of lesser depth
            // Close previous <list-item>
            // and nested <list>s
            xml.write_end_element ();

            for (int i = prev_depth; i > depth_tag->get_depth(); i--) {
              // Close nested <list>
              xml.write_end_element ();
              // Close <list-item>
              xml.write_end_element ();
            }
          }
        } 
        else {
          // Start of new list
          xml.write_start_element ("", "list", "");
          for (int i = 1; i <= depth_tag->get_depth(); i++) {
            xml.write_start_element ("", "list-item", "");
            xml.write_start_element ("", "list", "");
          }
        }

        prev_depth = depth_tag->get_depth();

        // Start a new <list-item>
        write_tag (depth_tag, xml, true);
      }

      // Output any tags that begin at the current position
      Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list2 = iter.get_tags();
      for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list2.begin();
          tag_iter != tag_list2.end(); ++tag_iter) {
        const Glib::RefPtr<Gtk::TextTag> & tag(*tag_iter);
        if (iter.begins_tag (tag)) {

          if (!(DepthNoteTag::Ptr::cast_dynamic(tag)) && NoteTagTable::tag_is_serializable(tag)) {
            write_tag (tag, xml, true);
            tag_stack.push (tag);
          }
        }
      }

      // Reopen tags that continued across indented lines
      // or into or out of lines with a depth
      while (!continue_stack.empty() &&
             ((!depth_tag && iter.starts_line ()) || (iter.get_line_offset() == 1)))
      {
        Glib::RefPtr<const Gtk::TextTag> continue_tag = continue_stack.top();
        continue_stack.pop();

        if (!tag_ends_here (continue_tag, iter, next_iter)
            && iter.has_tag (continue_tag))
        {
          write_tag (continue_tag, xml, true);
          tag_stack.push (continue_tag);
        }
      }

      // Hidden character representing an anchor
      if (iter.get_char() == 0xFFFC) {
        DBG_OUT("Got child anchor!!!");
        if (iter.get_child_anchor()) {
          const char * serialize = (const char*)(iter.get_child_anchor()->get_data(Glib::Quark("serialize")));
          if (serialize)
            xml.write_raw (serialize);
        }
        // Line Separator character
      } 
      else if (iter.get_char() == 0x2028) {
        xml.write_char_entity (0x2028);
      } 
      else if (!depth_tag) {
        xml.write_string (Glib::ustring(1, (gunichar)iter.get_char()));
      }

      bool end_of_depth_line = line_has_depth && next_iter.ends_line ();

      bool next_line_has_depth = false;
      if (iter.get_line() < buffer->get_line_count() - 1) {
        Gtk::TextIter next_line = buffer->get_iter_at_line(iter.get_line()+1);
        next_line_has_depth =
          NoteBuffer::Ptr::cast_static(buffer)->find_depth_tag (next_line);
      }

      bool at_empty_line = iter.ends_line () && iter.starts_line ();

      if (end_of_depth_line ||
          (next_line_has_depth && (next_iter.ends_line () || at_empty_line)))
      {
        // Close all tags in the tag_stack
        while (!tag_stack.empty()) {
          Glib::RefPtr<const Gtk::TextTag> existing_tag;
          existing_tag = tag_stack.top();
          tag_stack.pop ();

          // Any tags which continue across the indented
          // line are added to the continue_stack to be
          // reopened at the start of the next <list-item>
          if (!tag_ends_here (existing_tag, iter, next_iter)) {
            continue_stack.push (existing_tag);
          }

          write_tag (existing_tag, xml, false);
        }
      } 
      else {
        Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list3 = iter.get_tags();
        for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list3.begin();
            tag_iter != tag_list3.end(); ++tag_iter) {
          const Glib::RefPtr<Gtk::TextTag> & tag(*tag_iter);
          if (tag_ends_here (tag, iter, next_iter) &&
              NoteTagTable::tag_is_serializable(tag) && !(DepthNoteTag::Ptr::cast_dynamic(tag)))
          {
            while (!tag_stack.empty()) {
              Glib::RefPtr<const Gtk::TextTag> existing_tag = tag_stack.top();
              tag_stack.pop();

              if (!tag_ends_here (existing_tag, iter, next_iter)) {
                replay_stack.push (existing_tag);
              }

              write_tag (existing_tag, xml, false);
            }

            // Replay the replay queue.
            // Restart any tags that
            // overlapped with the ended
            // tag...
            while (!replay_stack.empty()) {
              Glib::RefPtr<const Gtk::TextTag> replay_tag = replay_stack.top();
              replay_stack.pop();
              tag_stack.push (replay_tag);

              write_tag (replay_tag, xml, true);
            }
          }
        }
      }

      // At the end of the line record that it
      // was the last line encountered with a depth
      if (end_of_depth_line) {
        line_has_depth = false;
        prev_depth_line = iter.get_line();
      }

      // If we are at the end of a line with a depth and the
      // next line does not have a depth line close all <list>
      // and <list-item> tags that remain open
      if (end_of_depth_line && !next_line_has_depth) {
        for (int i = prev_depth; i > -1; i--) {
          // Close <list>
          xml.write_full_end_element ();
          // Close <list-item>
          xml.write_full_end_element ();
        }

        prev_depth = -1;
      }

      iter.forward_char();
      next_iter.forward_char();
    }

    // Empty any trailing tags left in tag_stack..
    while (!tag_stack.empty()) {
      Glib::RefPtr<const Gtk::TextTag> tail_tag = tag_stack.top ();
      tag_stack.pop();
      write_tag (tail_tag, xml, false);
    }

    xml.write_end_element (); // </note-content>
  }


  void NoteBufferArchiver::deserialize(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
                                       const Gtk::TextIter & iter,
                                       const std::string & content)
  {
    if(!content.empty()) {
      // it looks like an empty string does not really make the cut
      sharp::XmlReader xml;
      xml.load_buffer(content);
      deserialize(buffer, iter, xml);
    }
  }

  struct TagStart 
  {
    TagStart()
      : start(0)
      {}
    int start;
    Glib::RefPtr<Gtk::TextTag> tag;
  };


  void NoteBufferArchiver::deserialize(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
                                       const Gtk::TextIter & start,
                                       sharp::XmlReader & xml)
  {
    int offset = start.get_offset();
    std::stack<TagStart> tag_stack;
    TagStart tag_start;
    Glib::ustring value;

    NoteTagTable::Ptr note_table = NoteTagTable::Ptr::cast_dynamic(buffer->get_tag_table());

    int curr_depth = -1;

    // A stack of boolean values which mark if a
    // list-item contains content other than another list
    // For some reason, std::stack<bool> cause crashes.
    std::deque<bool> list_stack;

    try {
      while (xml.read ()) {
        Gtk::TextIter insert_at;
        switch (xml.get_node_type()) {
        case XML_READER_TYPE_ELEMENT:
          if (xml.get_name() == "note-content")
            break;

          tag_start = TagStart();
          tag_start.start = offset;

          if (note_table &&
              note_table->is_dynamic_tag_registered (xml.get_name())) {
            tag_start.tag =
              note_table->create_dynamic_tag (xml.get_name());
          } 
          else if (xml.get_name() == "list") {
            curr_depth++;
            break;
          } 
          else if (xml.get_name() == "list-item") {
            if (curr_depth >= 0) {
              if (xml.get_attribute ("dir") == "rtl") {
                tag_start.tag =
                  note_table->get_depth_tag (curr_depth, Pango::DIRECTION_RTL);
              } 
              else {
                tag_start.tag =
                  note_table->get_depth_tag (curr_depth, Pango::DIRECTION_LTR);
              }
              list_stack.push_front (false);
            } 
            else {
              ERR_OUT("</list> tag mismatch");
            }
          } 
          else {
            tag_start.tag = buffer->get_tag_table()->lookup (xml.get_name());
          }

          if (NoteTag::Ptr::cast_dynamic(tag_start.tag)) {
            NoteTag::Ptr::cast_dynamic(tag_start.tag)->read (xml, true);
          }

          tag_stack.push (tag_start);
          break;
        case XML_READER_TYPE_TEXT:
        case XML_READER_TYPE_WHITESPACE:
        case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
          insert_at = buffer->get_iter_at_offset (offset);
          value = xml.get_value();
          buffer->insert (insert_at, value);

          // we need the # of chars *Unicode) and not bytes (ASCII)
          // see bug #587070
          offset += value.length();

          // If we are inside a <list-item> mark off
          // that we have encountered some content
          if (!list_stack.empty()) {
            list_stack.pop_front ();
            list_stack.push_front (true);
          }

          break;
        case XML_READER_TYPE_END_ELEMENT:
          if (xml.get_name() == "note-content")
            break;

          if (xml.get_name() == "list") {
            curr_depth--;
            break;
          }

          tag_start = tag_stack.top();
          tag_stack.pop();
          if (tag_start.tag) {

            Gtk::TextIter apply_start, apply_end;
            apply_start = buffer->get_iter_at_offset (tag_start.start);
            apply_end = buffer->get_iter_at_offset (offset);

            if (NoteTag::Ptr::cast_dynamic(tag_start.tag)) {
              NoteTag::Ptr::cast_dynamic(tag_start.tag)->read (xml, false);
            }

            // Insert a bullet if we have reached a closing
            // <list-item> tag, but only if the <list-item>
            // had content.
            DepthNoteTag::Ptr depth_tag = DepthNoteTag::Ptr::cast_dynamic(tag_start.tag);

            if (depth_tag && list_stack.front ()) {
              NoteBuffer::Ptr::cast_dynamic(buffer)->insert_bullet (apply_start,
                                                                    depth_tag->get_depth(),
                                                                    depth_tag->get_direction());
              buffer->remove_all_tags (apply_start, apply_start);
              offset += 2;
              list_stack.pop_front();
            } 
            else if (!depth_tag) {
              buffer->apply_tag (tag_start.tag, apply_start, apply_end);
            }
          }
          break;
        default:
          DBG_OUT("Unhandled element %d. Value: '%s'",
                  xml.get_node_type(), xml.get_value().c_str());
          break;
        }
      }
    }
    catch(const std::exception & e) {
      ERR_OUT("Exception, %s", e.what());
    }
  }

}
