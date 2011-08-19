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




#ifndef __NOTE_HPP_
#define __NOTE_HPP_

#include <list>
#include <string>
#include <queue>
#include <memory>

#include <libxml/tree.h>

#include <sigc++/signal.h>
#include <gtkmm/textbuffer.h>

#include "base/singleton.hpp"
#include "tag.hpp"
#include "notebuffer.hpp"
#include "utils.hpp"
#include "sharp/datetime.hpp"

namespace sharp {
  class XmlWriter;
}

namespace gnote {

class NoteManager;

class NoteData;
class NoteWindow;
class NoteTagTable;

class NoteDataBufferSynchronizer
{
public:
  // takes ownership
  NoteDataBufferSynchronizer(NoteData * _data)
    : m_data(_data)
    {
    }
  ~NoteDataBufferSynchronizer();

  const NoteData & synchronized_data() const
    {
      synchronize_text();
      return *m_data;
    }
  NoteData & synchronized_data()
    {
      synchronize_text();
      return *m_data;
    }
  const NoteData & data() const
    {
      return *m_data;
    }
  NoteData & data()
    {
      return *m_data;
    }
  const Glib::RefPtr<NoteBuffer> & buffer() const
    {
      return m_buffer;
    }
  void set_buffer(const Glib::RefPtr<NoteBuffer> & b);
  const std::string & text();
  void set_text(const std::string & t);

private:
  void invalidate_text();
  bool is_text_invalid() const;
  void synchronize_text() const;
  void synchronize_buffer();
  void buffer_changed();
  void buffer_tag_applied(const Glib::RefPtr<Gtk::TextBuffer::Tag> &,
                          const Gtk::TextBuffer::iterator &,
                          const Gtk::TextBuffer::iterator &);
  void buffer_tag_removed(const Glib::RefPtr<Gtk::TextBuffer::Tag> &,
                          const Gtk::TextBuffer::iterator &,
                          const Gtk::TextBuffer::iterator &);

  NoteData * m_data;
  Glib::RefPtr<NoteBuffer> m_buffer;
};


class Note 
  : public std::tr1::enable_shared_from_this<Note>
  , public sigc::trackable
{
public:
  typedef std::tr1::shared_ptr<Note> Ptr;
  typedef std::tr1::weak_ptr<Note> WeakPtr;
  typedef std::list<Ptr> List;

  typedef sigc::signal<void, const Note::Ptr&, const std::string& > RenamedHandler;
  typedef sigc::signal<void, const Note::Ptr&>                      SavedHandler;
  typedef sigc::signal<void, const Note&, const Tag::Ptr&>     TagAddedHandler;
  typedef sigc::signal<void, const Note&, const Tag &>         TagRemovingHandler;  
  typedef sigc::signal<void, const Note::Ptr&, const std::string&>  TagRemovedHandler;  

  typedef enum {
    NO_CHANGE,
    CONTENT_CHANGED,
    OTHER_DATA_CHANGED
  } ChangeType;

  ~Note();


  static std::string url_from_path(const std::string &);
  int get_hash_code() const;
  static Note::Ptr create_new_note(const std::string & title,
                                   const std::string & filename,
                                   NoteManager & manager);

  static Note::Ptr create_existing_note(NoteData *data,
                                        std::string filepath,
                                        NoteManager & manager);
  void delete_note();
  static Note::Ptr load(const std::string &, NoteManager &);
  void save();
  void queue_save(ChangeType c);
  void add_tag(const Tag::Ptr &);
  void remove_tag(Tag &);
  void remove_tag(const Tag::Ptr &);
  bool contains_tag(const Tag::Ptr &) const;
  void add_child_widget(const Glib::RefPtr<Gtk::TextChildAnchor> & child_anchor,
                        Gtk::Widget * widget);

  const std::string & uri() const;
  const std::string id() const;
  const std::string & file_path() const
    {
      return m_filepath;
    }
  const std::string & get_title() const;
  void set_title(const std::string & new_tile);
  void set_title(const std::string & new_title, bool from_user_action);
  void rename_without_link_update(const std::string & newTitle);
  const std::string & xml_content()
    {
      return m_data.text();
    }
  void set_xml_content(const std::string & xml);
  std::string get_complete_note_xml();
  void load_foreign_note_xml(const std::string & foreignNoteXml, ChangeType changeType);
  static void parse_tags(const xmlNodePtr tagnodes, std::list<std::string> & tags);
  std::string text_content();
  void set_text_content(const std::string & text);
  const NoteData & data() const;
  NoteData & data();

  const sharp::DateTime & create_date() const;
  const sharp::DateTime & change_date() const;
  const sharp::DateTime & metadata_change_date() const;
  NoteManager & manager()
    {
      return m_manager;
    }
  const NoteManager & manager() const
    {
      return m_manager;
    }
  const Glib::RefPtr<NoteTagTable> & get_tag_table();
  bool has_buffer() const
    {
      return m_buffer;
    }
  const Glib::RefPtr<NoteBuffer> & get_buffer();
  bool has_window() const 
    { 
      return (m_window != NULL); 
    }
  NoteWindow * get_window();
  bool is_special() const;
  bool is_new() const;
  bool is_loaded() const
    {
      return (m_buffer);
    }
  bool is_opened() const
    { 
      return (m_window != NULL); 
    }
  bool is_pinned() const;
  void set_pinned(bool pinned) const;
  bool is_open_on_startup() const;
  void set_is_open_on_startup(bool);
  void get_tags(std::list<Tag::Ptr> &) const;

  sigc::signal<void,Note&> & signal_opened()
    { return m_signal_opened; }
  RenamedHandler & signal_renamed()
    { return m_signal_renamed; }
  SavedHandler & signal_saved()
    { return m_signal_saved; }
  TagAddedHandler    & signal_tag_added()
    { return m_signal_tag_added; }
  TagRemovingHandler & signal_tag_removing()
    { return m_signal_tag_removing; }
  TagRemovedHandler  & signal_tag_removed()
    { return m_signal_tag_removed; }

private:
  bool contains_text(const std::string & text);
  void handle_link_rename(const std::string & old_title,
                          const Ptr & renamed,
                          bool rename);
  void on_buffer_changed();
  void on_buffer_tag_applied(const Glib::RefPtr<Gtk::TextTag> &tag, 
                             const Gtk::TextBuffer::iterator &, 
                             const Gtk::TextBuffer::iterator &);
  void on_buffer_tag_removed(const Glib::RefPtr<Gtk::TextTag> &tag,
                             const Gtk::TextBuffer::iterator &, 
                             const Gtk::TextBuffer::iterator &);
  void on_buffer_insert_mark_set(const Gtk::TextBuffer::iterator & iter,
                                 const Glib::RefPtr<Gtk::TextBuffer::Mark> & insert);
  bool on_window_configure(GdkEventConfigure *ev);
  bool on_window_destroyed(GdkEventAny *ev);
  void on_save_timeout();
  void process_child_widget_queue();
  void process_rename_link_update(const std::string & old_title);
  void rename_links(const std::string & old_title,
                    const Ptr & renamed);
  void remove_links(const std::string & old_title,
                    const Ptr & renamed);

  Note(NoteData * data, const std::string & filepath, NoteManager & manager);

  struct ChildWidgetData
  {
    ChildWidgetData(const Glib::RefPtr<Gtk::TextChildAnchor> & _anchor,
                    Gtk::Widget *_widget)
      : anchor(_anchor)
      , widget(_widget)
      {
      }
    Glib::RefPtr<Gtk::TextChildAnchor> anchor;
    Gtk::Widget *widget;
  };

  NoteDataBufferSynchronizer m_data;
  std::string                m_filepath;
  bool                       m_save_needed;
  bool                       m_is_deleting;
  
  NoteManager               &m_manager;
  NoteWindow                *m_window;
  Glib::RefPtr<NoteBuffer>   m_buffer;
  Glib::RefPtr<NoteTagTable> m_tag_table;

  utils::InterruptableTimeout *m_save_timeout;
  std::queue<ChildWidgetData> m_child_widget_queue;

  sigc::signal<void,Note&> m_signal_opened;
  RenamedHandler     m_signal_renamed;
  SavedHandler       m_signal_saved;
  TagAddedHandler    m_signal_tag_added;
  TagRemovingHandler m_signal_tag_removing;
  TagRemovedHandler  m_signal_tag_removed;
};

class NoteArchiver
  : public base::Singleton<NoteArchiver>
{
public:
  static const char *CURRENT_VERSION;

  static NoteData *read(const std::string & read_file, const std::string & uri);
  static std::string write_string(const NoteData & data);
  static void write(const std::string & write_file, const NoteData & data);
  void write_file(const std::string & write_file, const NoteData & data);
  void write(sharp::XmlWriter & xml, const NoteData & note);

  std::string get_renamed_note_xml(const std::string &, const std::string &,
                                   const std::string &) const;
  std::string get_title_from_note_xml(const std::string & noteXml) const;

protected:
  NoteData *_read(const std::string & read_file, const std::string & uri);
};

namespace noteutils {
  void show_deletion_dialog (const std::list<Note::Ptr> & notes, Gtk::Window * parent);
}

}

#endif
