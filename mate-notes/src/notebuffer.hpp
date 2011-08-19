/*
 * gnote
 *
 * Copyright (C) 2011 Aurimas Cernius
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




#ifndef __NOTE_BUFFER_HPP_
#define __NOTE_BUFFER_HPP_

#include <queue>

#include <pangomm/context.h>

#include <gtkmm/textbuffer.h>
#include <gtkmm/textiter.h>
#include <gtkmm/texttag.h>
#include <gtkmm/widget.h>

#include "notetag.hpp"

namespace sharp {
  class XmlReader;
  class XmlWriter;
}

namespace gnote {

  class Note;
  class UndoManager;


class NoteBuffer 
  : public Gtk::TextBuffer
{
public:
  typedef Glib::RefPtr<NoteBuffer> Ptr;
  typedef sigc::signal<void, int, int, Pango::Direction> NewBulletHandler;
  typedef sigc::signal<void, int, bool> ChangeDepthHandler;

  bool get_enable_auto_bulleted_lists() const;
  static Ptr create(const NoteTagTable::Ptr & table, Note & note)
    {
      return Ptr(new NoteBuffer(table, note));
    }
  ~NoteBuffer();

  // Signal that text has been inserted, and any active tags have
  // been applied to the text.  This allows undo to pull any
  // active tags from the inserted text.
  sigc::signal<void, const Gtk::TextIter &, const Glib::ustring &, int> signal_insert_text_with_tags;
  ChangeDepthHandler                               signal_change_text_depth;
  NewBulletHandler                                 signal_new_bullet_inserted;

  void toggle_active_tag(const std::string &);
  void set_active_tag(const std::string &);
  void remove_active_tag(const std::string &);
  DynamicNoteTag::ConstPtr get_dynamic_tag(const std::string  & tag_name, const Gtk::TextIter & iter);
  void on_tag_applied(const Glib::RefPtr<Gtk::TextTag> &,
                      const Gtk::TextIter &,const Gtk::TextIter &);
  bool is_active_tag(const std::string & );
  bool is_bulleted_list_active();
  bool can_make_bulleted_list();
  bool add_tab();
  bool remove_tab();
  bool add_new_line(bool soft_break);
  bool delete_key_handler();
  bool backspace_key_handler();
  void check_selection();
  bool run_widget_queue();
  void on_tag_changed(const Glib::RefPtr<Gtk::TextTag> &, bool);
  UndoManager & undoer()
    { 
      return *m_undomanager; 
    }
  std::string get_selection() const;
  static void get_block_extents(Gtk::TextIter &, Gtk::TextIter &,
                           int threshold, const Glib::RefPtr<Gtk::TextTag> & avoid_tag);
  void toggle_selection_bullets();
  void increase_cursor_depth()
    {
      change_cursor_depth(true);
    }
  void decrease_cursor_depth()
    {
      change_cursor_depth(false);
    }
  void change_cursor_depth_directional(bool right);
  void change_bullet_direction(Gtk::TextIter pos, Pango::Direction);
  void insert_bullet(Gtk::TextIter & iter, int depth, Pango::Direction direction);
  void remove_bullet(Gtk::TextIter & iter);
  void increase_depth(Gtk::TextIter & start);
  void decrease_depth(Gtk::TextIter & start);
  DepthNoteTag::Ptr find_depth_tag(Gtk::TextIter &);
  static bool is_bullet(gunichar c);
  void select_note_body();
protected: 
  NoteBuffer(const NoteTagTable::Ptr &, Note &);

  virtual void on_apply_tag(const Glib::RefPtr<Gtk::TextTag> & tag,
                       const Gtk::TextIter &,  const Gtk::TextIter &);
  virtual void on_remove_tag(const Glib::RefPtr<Gtk::TextTag> & tag,
                       const Gtk::TextIter &,  const Gtk::TextIter &);
private:
  void text_insert_event(const Gtk::TextIter & pos, const Glib::ustring & text, int);
  void range_deleted_event(const Gtk::TextIter &,const Gtk::TextIter &);
  bool line_needs_bullet(Gtk::TextIter iter);
  void augment_selection(Gtk::TextIter &, Gtk::TextIter &);
  void mark_set_event(const Gtk::TextIter &,const Glib::RefPtr<Gtk::TextBuffer::Mark> &);
  void widget_swap (const NoteTag::Ptr & tag, const Gtk::TextIter & start,
                    const Gtk::TextIter & end_iter, bool adding);
  void change_cursor_depth(bool increase);

  UndoManager           *m_undomanager;
  static const gunichar s_indent_bullets[];

  // GODDAMN Gtk::TextBuffer. I hate you. Hate Hate Hate.
  struct WidgetInsertData
  {
    bool adding;
    Glib::RefPtr<Gtk::TextBuffer> buffer;
    Glib::RefPtr<Gtk::TextMark>   position;
    Gtk::Widget                  *widget;
    NoteTag::Ptr                  tag;
  };
  std::queue<WidgetInsertData> m_widget_queue;
  sigc::connection             m_widget_queue_timeout;
  // HATE.

  // list of Glib::RefPtr<Gtk::TextTag>s to apply on insert
  std::list<Glib::RefPtr<Gtk::TextTag> >      m_active_tags;

  // The note that owns this buffer
  Note &                       m_note;
};

class NoteBufferArchiver
{
public:
  static std::string serialize(const Glib::RefPtr<Gtk::TextBuffer> & );
  static std::string serialize(const Glib::RefPtr<Gtk::TextBuffer> & buffer, const Gtk::TextIter &,
                               const Gtk::TextIter &);
  static void serialize(const Glib::RefPtr<Gtk::TextBuffer> & buffer, const Gtk::TextIter &,
                        const Gtk::TextIter &, sharp::XmlWriter & xml);
  static void deserialize(const Glib::RefPtr<Gtk::TextBuffer> &buffer,
                          const std::string & content)
    {
      deserialize(buffer, buffer->begin(), content);
    }
  static void deserialize(const Glib::RefPtr<Gtk::TextBuffer> &, const Gtk::TextIter & ,
                          const std::string & );
  static void deserialize(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
                          const Gtk::TextIter & iter, sharp::XmlReader & xml);
private:

  static void write_tag(const Glib::RefPtr<const Gtk::TextTag> & tag, sharp::XmlWriter & xml, 
                        bool start);
  static bool tag_ends_here (const Glib::RefPtr<const Gtk::TextTag> & tag,
                             const Gtk::TextIter & iter,
                             const Gtk::TextIter & next_iter);

//
};


}

#endif

