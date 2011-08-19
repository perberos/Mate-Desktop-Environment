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



#ifndef __UNDO_HPP_
#define __UNDO_HPP_

#include <list>
#include <string>
#include <stack>

#include <boost/noncopyable.hpp>

#include <sigc++/signal.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/texttag.h>
#include <gtkmm/textiter.h>

#include "notebuffer.hpp"
#include "utils.hpp"

namespace gnote {


class EditAction
{
public:
  virtual ~EditAction() {}
  virtual void undo (Gtk::TextBuffer * buffer) = 0;
  virtual void redo (Gtk::TextBuffer * buffer) = 0;
  virtual void merge (EditAction * action) = 0;
  virtual bool can_merge (const EditAction * action) const = 0;
  virtual void destroy () = 0;
};

class ChopBuffer
  : public Gtk::TextBuffer
{
public:
  typedef Glib::RefPtr<ChopBuffer> Ptr;
  ChopBuffer(const Glib::RefPtr<Gtk::TextTagTable> & table);
  utils::TextRange add_chop(const Gtk::TextIter & start_iter, const Gtk::TextIter & end_iter);
};


class SplitterAction
  : public EditAction
{
public:
  struct TagData {
    int start;
    int end;
    Glib::RefPtr<Gtk::TextTag> tag;
  };

  const utils::TextRange & get_chop() const
    {
      return m_chop;
    }
  const std::list<TagData> & get_split_tags() const
    {
      return m_splitTags;
    }
  void split(Gtk::TextIter iter, Gtk::TextBuffer *);
  void add_split_tag(const Gtk::TextIter &, const Gtk::TextIter &, 
                     const Glib::RefPtr<Gtk::TextTag> tag);
protected:
  SplitterAction();
  int get_split_offset() const;
  void apply_split_tag(Gtk::TextBuffer *);
  void remove_split_tags(Gtk::TextBuffer *);
  std::list<TagData> m_splitTags;
  utils::TextRange   m_chop;
};



class InsertAction
  : public SplitterAction
{
public:
  InsertAction(const Gtk::TextIter & start, const std::string & text, int length,
               const ChopBuffer::Ptr & chop_buf);
  virtual void undo (Gtk::TextBuffer * buffer);
  virtual void redo (Gtk::TextBuffer * buffer);
  virtual void merge (EditAction * action);
  virtual bool can_merge (const EditAction * action) const;
  virtual void destroy ();

private:
  int m_index;
  bool m_is_paste;
};


class EraseAction 
  : public SplitterAction
{
public:
  EraseAction(const Gtk::TextIter & start_iter, const Gtk::TextIter & end_iter,
               const ChopBuffer::Ptr & chop_buf);
  virtual void undo (Gtk::TextBuffer * buffer);
  virtual void redo (Gtk::TextBuffer * buffer);
  virtual void merge (EditAction * action);
  virtual bool can_merge (const EditAction * action) const;
  virtual void destroy ();

private:
  int m_start;
  int m_end;
  bool m_is_forward;
  bool m_is_cut;
};



class TagApplyAction
  : public EditAction
{
public:
  TagApplyAction(const Glib::RefPtr<Gtk::TextTag> &, const Gtk::TextIter & start, const Gtk::TextIter & end);
  virtual void undo (Gtk::TextBuffer * buffer);
  virtual void redo (Gtk::TextBuffer * buffer);
  virtual void merge (EditAction * action);
  virtual bool can_merge (const EditAction * action) const;
  virtual void destroy ();

private:
  Glib::RefPtr<Gtk::TextTag> m_tag;
  int m_start;
  int m_end;
};


class TagRemoveAction
  : public EditAction
{
public:
  TagRemoveAction(const Glib::RefPtr<Gtk::TextTag> &, const Gtk::TextIter & start, const Gtk::TextIter & end);
  virtual void undo (Gtk::TextBuffer * buffer);
  virtual void redo (Gtk::TextBuffer * buffer);
  virtual void merge (EditAction * action);
  virtual bool can_merge (const EditAction * action) const;
  virtual void destroy ();
private:
  Glib::RefPtr<Gtk::TextTag> m_tag;
  int m_start;
  int m_end;
};


class ChangeDepthAction
  : public EditAction
{
public:
  ChangeDepthAction(int line, bool direction);
  virtual void undo (Gtk::TextBuffer * buffer);
  virtual void redo (Gtk::TextBuffer * buffer);
  virtual void merge (EditAction * action);
  virtual bool can_merge (const EditAction * action) const;
  virtual void destroy ();
private:
  int m_line;
  bool m_direction;
};



class InsertBulletAction
  : public EditAction
{
public:
  InsertBulletAction(int offset, int depth, Pango::Direction direction);
  virtual void undo (Gtk::TextBuffer * buffer);
  virtual void redo (Gtk::TextBuffer * buffer);
  virtual void merge (EditAction * action);
  virtual bool can_merge (const EditAction * action) const;
  virtual void destroy ();
private:
  int m_offset;
  int m_depth;
  Pango::Direction m_direction;
};

class UndoManager
  : public boost::noncopyable
{
public:
  /** the buffer it NOT owned by the UndoManager
   *  it is assume to have a longer life than UndoManager
   *  Actually the UndoManager belong to the buffer.
   */
  UndoManager(NoteBuffer * buffer);
  ~UndoManager();
  bool get_can_undo()
    {
      return !m_undo_stack.empty();
    }
  bool get_can_redo()
    {
      return !m_redo_stack.empty();
    }
  void undo()
    {
      undo_redo(m_undo_stack, m_redo_stack, true);
    }
  void redo()
    {
      undo_redo(m_redo_stack, m_undo_stack, false);
    }
  void freeze_undo()
    {
      ++m_frozen_cnt;
    }
  void thaw_undo()
    {
      --m_frozen_cnt;
    }

  void undo_redo(std::stack<EditAction *> &, std::stack<EditAction *> &, bool);
  void clear_undo_history();
  void add_undo_action(EditAction * action);

  sigc::signal<void> & signal_undo_changed()
    { return m_undo_changed; }

private:

  void clear_action_stack(std::stack<EditAction *> &);
  void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);
  void on_delete_range(const Gtk::TextIter &, const Gtk::TextIter &);
  void on_tag_applied(const Glib::RefPtr<Gtk::TextTag> &,
                      const Gtk::TextIter &, const Gtk::TextIter &);
  void on_tag_removed(const Glib::RefPtr<Gtk::TextTag> &,
                      const Gtk::TextIter &, const Gtk::TextIter &);

  void on_change_depth(int, bool);
  void on_bullet_inserted(int, int, Pango::Direction);

  guint m_frozen_cnt;
  bool m_try_merge;
  NoteBuffer * m_buffer;
  ChopBuffer::Ptr m_chop_buffer;
  std::stack<EditAction *> m_undo_stack;
  std::stack<EditAction *> m_redo_stack;
  sigc::signal<void> m_undo_changed;
};


}

#endif
