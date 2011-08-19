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




#ifndef _NOTEWINDOW_HPP__
#define _NOTEWINDOW_HPP__

#include <gtkmm/accelgroup.h>
#include <gtkmm/entry.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/menu.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>

#include "undo.hpp"
#include "utils.hpp"
#include "notebuffer.hpp"
#include "preferences.hpp"

namespace gnote {

  class Note;
  class NoteFindBar;

class NoteTextMenu
  : public Gtk::Menu
{
public:
  NoteTextMenu(const Glib::RefPtr<Gtk::AccelGroup>&, 
               const Glib::RefPtr<NoteBuffer> & buffer, UndoManager& undo_manager);

  static void markup_label (Gtk::MenuItem & item);
  
protected:
  virtual void on_show();

private:
  void refresh_sizing_state();
  void refresh_state();
  void font_style_clicked(Gtk::CheckMenuItem * item);
  void font_size_activated(Gtk::RadioMenuItem *item);
  void undo_clicked();
  void redo_clicked();
  void undo_changed();
  void toggle_bullets_clicked();
  void increase_indent_clicked();
  void decrease_indent_clicked();
  void increase_font_clicked();
  void decrease_font_clicked();

  Glib::RefPtr<NoteBuffer> m_buffer;
  UndoManager          &m_undo_manager;
  bool                  m_event_freeze;
  Gtk::ImageMenuItem   *m_undo;
  Gtk::ImageMenuItem   *m_redo;
  Gtk::CheckMenuItem    m_bold;
  Gtk::CheckMenuItem    m_italic;
  Gtk::CheckMenuItem    m_strikeout;
  Gtk::CheckMenuItem    m_highlight;
  Gtk::RadioButtonGroup m_fontsize_group;
  Gtk::RadioMenuItem    m_normal;
  Gtk::RadioMenuItem    m_huge;
  Gtk::RadioMenuItem    m_large;
  Gtk::RadioMenuItem    m_small;
  // Active when the text size is indeterminable, such as when in
  // the note's title line.
  Gtk::RadioMenuItem    m_hidden_no_size;
  Gtk::CheckMenuItem    m_bullets;
  Gtk::ImageMenuItem    m_increase_indent;
  Gtk::ImageMenuItem    m_decrease_indent;
  Gtk::MenuItem         m_increase_font;
  Gtk::MenuItem         m_decrease_font;
  sigc::connection      m_bullets_clicked_cid;
};


class NoteWindow 
  : public utils::ForcedPresentWindow
{
public:
  NoteWindow(Note &);
  ~NoteWindow();

  Gtk::TextView * editor() const
    {
      return m_editor;
    }
  Gtk::Toolbar * toolbar() const
    {
      return m_toolbar;
    }
  Gtk::ToolButton * delete_button() const
    {
      return m_delete_button;
    }
  Gtk::Menu * plugin_menu() const
    {
      return m_plugin_menu;
    }
  Gtk::Menu * text_menu() const
    {
      return m_text_menu;
    }
  const Glib::RefPtr<Gtk::AccelGroup> & get_accel_group()
    {
      return m_accel_group;
    }
  NoteFindBar & get_find_bar()
    {
      return *m_find_bar;
    }

protected:
  virtual bool on_delete_event(GdkEventAny *ev);
  virtual void on_hide();
private:
  bool on_key_pressed(GdkEventKey*);
  void close_window_handler();
  void close_all_windows_handler();
  void on_delete_button_clicked();
  void on_selection_mark_set(const Gtk::TextIter&, const Glib::RefPtr<Gtk::TextMark>&);
  void update_link_button_sensitivity();
  void on_populate_popup(Gtk::Menu*);
  Gtk::Toolbar * make_toolbar();
  void sync_item_selected();
  Gtk::Menu * make_plugin_menu();
  Gtk::Menu * make_find_menu();
  void find_button_clicked();
  void find_next_activate();
  void find_previous_activate();
  void find_bar_hidden();
  void link_button_clicked();
  void open_help_activate();
  void create_new_note();
  void change_depth_right_handler();
  void change_depth_left_handler();
  void search_button_clicked();

  Note                        & m_note;
  Glib::RefPtr<Gtk::AccelGroup> m_accel_group;
  Gtk::Toolbar                 *m_toolbar;
  Gtk::ToolButton              *m_link_button;
  NoteTextMenu                 *m_text_menu;
  Gtk::Menu                    *m_plugin_menu;
  Gtk::TextView                *m_editor;
  Gtk::ScrolledWindow          *m_editor_window;
  NoteFindBar                  *m_find_bar;
  Gtk::ToolButton              *m_delete_button;

  utils::GlobalKeybinder       *m_global_keys;
  utils::InterruptableTimeout  *m_mark_set_timeout;
  guint                         m_mateconf_notify;
};

class NoteFindBar
  : public Gtk::HBox
{
public:
  NoteFindBar(Note & );
  ~NoteFindBar();
  Gtk::Button & find_next_button()
    {
      return m_next_button;
    }
  Gtk::Button & find_previous_button()
    {
      return m_prev_button;
    }
  Glib::ustring search_text();
  void set_search_text(const Glib::ustring &);


protected:
  virtual void on_show();
  virtual void on_hide();


  
private:
  struct Match
  {
    Glib::RefPtr<NoteBuffer>     buffer;
    Glib::RefPtr<Gtk::TextMark>  start_mark;
    Glib::RefPtr<Gtk::TextMark>  end_mark;
    bool                         highlighting;
  };

  void hide_find_bar();
  void on_prev_clicked();
  void on_next_clicked();
  void jump_to_match(const Match & match);
  void on_find_entry_activated();
  void on_find_entry_changed();
  void entry_changed_timeout();
  void perform_search (bool scroll_to_hit);
  void update_sensitivity();
  void update_search();
  void note_changed_timeout();
  void on_insert_text(const Gtk::TextBuffer::iterator &, const Glib::ustring &, int);
  void on_delete_range(const Gtk::TextBuffer::iterator &, const Gtk::TextBuffer::iterator &);
  bool on_key_pressed(GdkEventKey*);
  bool on_key_released(GdkEventKey*);
  void highlight_matches(bool);
  void cleanup_matches();
  void find_matches_in_buffer(const Glib::RefPtr<NoteBuffer> & buffer, 
                              const std::vector<Glib::ustring> & words,
                              std::list<Match> & matches);

  Note           & m_note;
  Gtk::Entry       m_entry;
  Gtk::Button      m_next_button;
  Gtk::Button      m_prev_button;
  std::list<Match> m_current_matches;
  Glib::ustring    m_prev_search_text;
  utils::InterruptableTimeout * m_entry_changed_timeout;
  utils::InterruptableTimeout * m_note_changed_timeout;
  bool             m_shift_key_pressed;
  sigc::connection m_insert_cid;
  sigc::connection m_delete_cid;
};



}

#endif
