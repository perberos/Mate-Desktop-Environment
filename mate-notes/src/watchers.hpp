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




#ifndef __WATCHERS_HPP_
#define __WATCHERS_HPP_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <pcrecpp.h>

#if FIXED_GTKSPELL
extern "C" {
#include <gtkspell/gtkspell.h>
}
#endif
#include <mateconf/mateconf-client.h>

#include <gdkmm/cursor.h>
#include <gtkmm/textiter.h>
#include <gtkmm/texttag.h>

#include "noteaddin.hpp"
#include "triehit.hpp"
#include "utils.hpp"

namespace gnote {

  class Preferences;
  class NoteEditor;
  class NoteTag;

  class NoteRenameWatcher
    : public NoteAddin
  {
  public:
    static NoteAddin * create();
    ~NoteRenameWatcher();
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

  protected:
    NoteRenameWatcher()
      : m_editing_title(false)
      , m_title_taken_dialog(NULL)
      {}
  private:
    Gtk::TextIter get_title_end() const;
    Gtk::TextIter get_title_start() const;
    bool on_editor_focus_out(GdkEventFocus *);
    void on_mark_set(const Gtk::TextIter &, const Glib::RefPtr<Gtk::TextMark>&);
    void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);
    void on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &);
    void update();
    void changed();
    bool on_window_closed(GdkEventAny *);
    std::string get_unique_untitled();
    bool update_note_title();
    void show_name_clash_error(const std::string &);
    void on_dialog_response(int);

    bool                       m_editing_title;
    Glib::RefPtr<Gtk::TextTag> m_title_tag;
    utils::HIGMessageDialog   *m_title_taken_dialog;
  };

#if FIXED_GTKSPELL
  class NoteSpellChecker 
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

    static bool gtk_spell_available()
      { return true; }
  protected:
    NoteSpellChecker()
      : m_obj_ptr(NULL)
      {}
  private:
    void attach();
    void detach();
    void on_enable_spellcheck_changed(Preferences *, MateConfEntry *);
    void tag_applied(const Glib::RefPtr<const Gtk::TextTag> &,
                     const Gtk::TextIter &, const Gtk::TextIter &);

    GtkSpell *m_obj_ptr;
    sigc::connection  m_tag_applied_cid;
  };
#else
  class NoteSpellChecker 
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize ();
    virtual void shutdown ()
      {}
    virtual void on_note_opened ()
      {}

    static bool gtk_spell_available()
      { return false; }
  };
#endif


  class NoteUrlWatcher
    : public NoteAddin 
  {
  public:
    static NoteAddin * create();    
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

  protected:
    NoteUrlWatcher();
  private:
    std::string get_url(const Gtk::TextIter & start, const Gtk::TextIter & end);
    bool on_url_tag_activated(const NoteTag::Ptr &, const NoteEditor &,
                              const Gtk::TextIter &, const Gtk::TextIter &);
    void apply_url_to_block (Gtk::TextIter start, Gtk::TextIter end);
    void on_apply_tag(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
                      const Gtk::TextIter & start, const Gtk::TextIter &end);
    void on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &);
    void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);
    bool on_button_press(GdkEventButton *);
    void on_populate_popup(Gtk::Menu *);
    bool on_popup_menu();
    void copy_link_activate();
    void open_link_activate();

    NoteTag::Ptr                m_url_tag;
    Glib::RefPtr<Gtk::TextMark> m_click_mark;
    pcrecpp::RE                 m_regex;
    static const char * URL_REGEX;
    static bool  s_text_event_connected;
  };


  class NoteLinkWatcher
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

  private:
    bool contains_text(const std::string & text);
    void on_note_added(const Note::Ptr &);
    void on_note_deleted(const Note::Ptr &);
    void on_note_renamed(const Note::Ptr&, const std::string&);
    void do_highlight(const TrieHit<Note::WeakPtr> & , const Gtk::TextIter &,const Gtk::TextIter &);
    void highlight_note_in_block (const Note::Ptr &, const Gtk::TextIter &,
                                  const Gtk::TextIter &);
    void highlight_in_block(const Gtk::TextIter &,const Gtk::TextIter &);
    void unhighlight_in_block(const Gtk::TextIter &,const Gtk::TextIter &);
    void on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &);
    void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);
    void on_apply_tag(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
                      const Gtk::TextIter & start, const Gtk::TextIter &end);

    bool open_or_create_link(const Gtk::TextIter &,const Gtk::TextIter &);
    bool on_link_tag_activated(const NoteTag::Ptr &, const NoteEditor &,
                               const Gtk::TextIter &, const Gtk::TextIter &);

    NoteTag::Ptr m_url_tag;
    NoteTag::Ptr m_link_tag;
    NoteTag::Ptr m_broken_link_tag;

    sigc::connection m_on_note_deleted_cid;
    sigc::connection m_on_note_added_cid;
    sigc::connection m_on_note_renamed_cid;
    static bool s_text_event_connected;
  };


  class NoteWikiWatcher
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

  protected:
    NoteWikiWatcher()
      : m_regex(WIKIWORD_REGEX, pcrecpp::RE_Options(PCRE_UTF8))
      {
      }
  private:
    void on_enable_wikiwords_changed(Preferences *, MateConfEntry *);
    void apply_wikiword_to_block (Gtk::TextIter start, Gtk::TextIter end);
    void on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &);
    void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);


    static const char * WIKIWORD_REGEX;
    Glib::RefPtr<Gtk::TextTag>   m_broken_link_tag;
    pcrecpp::RE         m_regex;
    sigc::connection    m_on_insert_text_cid;
    sigc::connection    m_on_delete_range_cid;
  };


  class MouseHandWatcher
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

  protected:
    MouseHandWatcher()
      : m_hovering_on_link(false)
      {
        _init_static();
      }
  private:
    void _init_static();
    bool on_editor_key_press(GdkEventKey*);
    bool on_editor_key_release(GdkEventKey*);
    bool on_editor_motion(GdkEventMotion *);
    bool m_hovering_on_link;
    static bool s_static_inited;
    static Gdk::Cursor s_normal_cursor;
    static Gdk::Cursor s_hand_cursor;

  };


  class NoteTagsWatcher 
    : public NoteAddin
  {
  public:
    static NoteAddin * create();
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

  private:
    void on_tag_added(const Note&, const Tag::Ptr&);
    void on_tag_removing(const Note&, const Tag &);
    void on_tag_removed(const Note::Ptr&, const std::string&);

    sigc::connection m_on_tag_added_cid;
    sigc::connection m_on_tag_removing_cid;
    sigc::connection m_on_tag_removed_cid;
  };

}


#endif

