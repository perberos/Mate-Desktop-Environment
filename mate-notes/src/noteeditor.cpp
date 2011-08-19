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


#include <string.h>

#include "notebuffer.hpp"
#include "noteeditor.hpp"
#include "preferences.hpp"
#include "utils.hpp"
#include "debug.hpp"
#include "sharp/string.hpp"

namespace gnote {

#define DESKTOP_MATE_INTERFACE_PATH "/desktop/mate/interface"
#define MATE_DOCUMENT_FONT_KEY  DESKTOP_MATE_INTERFACE_PATH"/document_font_name"

  NoteEditor::NoteEditor(const Glib::RefPtr<Gtk::TextBuffer> & buffer)
    : Gtk::TextView(buffer)
  {
    set_wrap_mode(Gtk::WRAP_WORD);
    set_left_margin(default_margin());
    set_right_margin(default_margin());
    property_can_default().set_value(true);

    //Set up the MateConf client to watch the default document font
    m_mateconf_notify = Preferences::obj().add_notify(DESKTOP_MATE_INTERFACE_PATH,
                                                                &on_font_setting_changed_mateconf,
                                                                this);

    // Make sure the cursor position is visible
    scroll_mark_onscreen (buffer->get_insert());

    // Set Font from MateConf preference
    if (Preferences::obj().get<bool>(Preferences::ENABLE_CUSTOM_FONT)) {
      std::string font_string = Preferences::obj().get<std::string>(Preferences::CUSTOM_FONT_FACE);
      modify_font (Pango::FontDescription(font_string));
    }
    else {
      modify_font (get_mate_document_font_description ());
    }

    Preferences::obj().signal_setting_changed()
      .connect(sigc::mem_fun(*this, &NoteEditor::on_font_setting_changed));

    // Set extra editor drag targets supported (in addition
    // to the default TextView's various text formats)...
    Glib::RefPtr<Gtk::TargetList> list = drag_dest_get_target_list();

    
    list->add ("text/uri-list", (Gtk::TargetFlags)0, 1);
    list->add ("_NETSCAPE_URL", (Gtk::TargetFlags)0, 1);

    signal_key_press_event().connect(sigc::mem_fun(*this, &NoteEditor::key_pressed), false);
    signal_button_press_event().connect(sigc::mem_fun(*this, &NoteEditor::button_pressed), false);
  }

  NoteEditor::~NoteEditor()
  {
    Preferences::obj().remove_notify(m_mateconf_notify);
  }


  Pango::FontDescription NoteEditor::get_mate_document_font_description()
  {
    try {
      std::string doc_font_string =
        Preferences::obj().get<std::string>(MATE_DOCUMENT_FONT_KEY);
      return Pango::FontDescription(doc_font_string);
    } 
    catch (...) {

    }

    return Pango::FontDescription();
  }


//
    // Update the font based on the changed Preference dialog setting.
    // Also update the font based on the changed MateConf MATE document font setting.
    //
  void NoteEditor::on_font_setting_changed_mateconf (MateConfClient *, 
                                            guint , MateConfEntry* entry, gpointer data)
  {
    NoteEditor * self = static_cast<NoteEditor*>(data);
    self->on_font_setting_changed (NULL, entry);
  }


  void NoteEditor::on_font_setting_changed (Preferences*, MateConfEntry* entry)
  {
    const char * key = mateconf_entry_get_key(entry);

    if((strcmp(key, Preferences::ENABLE_CUSTOM_FONT) == 0)
       || (strcmp(key, Preferences::CUSTOM_FONT_FACE) == 0)) {
      update_custom_font_setting ();
    }
    else if(strcmp(key, MATE_DOCUMENT_FONT_KEY) == 0) {
      if (!Preferences::obj().get<bool>(Preferences::ENABLE_CUSTOM_FONT)) {
        MateConfValue * v = mateconf_entry_get_value(entry);
        const char * value = mateconf_value_get_string(v);
        if(value) {
          modify_font_from_string(value);
        }
      }
    }
  }


  void NoteEditor::update_custom_font_setting()
  {
    if (Preferences::obj().get<bool>(Preferences::ENABLE_CUSTOM_FONT)) {
      std::string fontString = Preferences::obj().get<std::string>(Preferences::CUSTOM_FONT_FACE);
      DBG_OUT( "Switching note font to '%s'...", fontString.c_str());
      modify_font_from_string (fontString);
    } 
    else {
      DBG_OUT("Switching back to the default font");
      modify_font (get_mate_document_font_description());
    }
  }

  
  void NoteEditor::modify_font_from_string (const std::string & fontString)
  {
    DBG_OUT("Switching note font to '%s'...", fontString.c_str());
    modify_font (Pango::FontDescription(fontString));
  }

  

    //
    // DND Drop handling
    //
  void NoteEditor::on_drag_data_received (Glib::RefPtr<Gdk::DragContext> & context,
                                          int x, int y,
                                          const Gtk::SelectionData & selection_data,
                                          guint info,  guint time)
  {
    bool has_url = false;

    Gdk::ListHandle_AtomString targets = context->get_targets();
    for(Gdk::ListHandle_AtomString::const_iterator iter = targets.begin();
        iter != targets.end(); ++iter) {
      const std::string & target(*iter);
      if (target == "text/uri-list" ||
          target == "_NETSCAPE_URL") {
        has_url = true;
        break;
      }
    }

    if (has_url) {
      utils::UriList uri_list(selection_data);
      bool more_than_one = false;

      // Place the cursor in the position where the uri was
      // dropped, adjusting x,y by the TextView's VisibleRect.
      Gdk::Rectangle rect;
      get_visible_rect(rect);
      int adjustedX = x + rect.get_x();
      int adjustedY = y + rect.get_y();
      Gtk::TextIter cursor;
      get_iter_at_location (cursor, adjustedX, adjustedY);
      get_buffer()->place_cursor (cursor);

      Glib::RefPtr<Gtk::TextTag> link_tag = get_buffer()->get_tag_table()->lookup ("link:url");

      for(utils::UriList::const_iterator iter = uri_list.begin();
          iter != uri_list.end(); ++iter) {
        const sharp::Uri & uri(*iter);
        DBG_OUT("Got Dropped URI: %s", uri.to_string().c_str());
        std::string insert;
        if (uri.is_file()) {
          // URL-escape the path in case
          // there are spaces (bug #303902)
          insert = sharp::Uri::escape_uri_string(uri.local_path());
        } 
        else {
          insert = uri.to_string ();
        }

        if (insert.empty() || sharp::string_trim(insert).empty())
          continue;

        if (more_than_one) {
          cursor = get_buffer()->get_iter_at_mark (get_buffer()->get_insert());

          // FIXME: The space here is a hack
          // around a bug in the URL Regex which
          // matches across newlines.
          if (cursor.get_line_offset() == 0) {
            get_buffer()->insert (cursor, " \n");
          }
          else {
            get_buffer()->insert (cursor, ", ");
          }
        }

        get_buffer()->insert_with_tag(cursor, insert, link_tag);
        more_than_one = true;
      }

      context->drag_finish(more_than_one, false, time);
    } 
    else {
      Gtk::TextView::on_drag_data_received (context, x, y, selection_data, info, time);
    }
  }

  bool NoteEditor::key_pressed (GdkEventKey * ev)
  {
      bool ret_value = false;

      switch (ev->keyval)
      {
      case GDK_KP_Enter:
      case GDK_Return:
        // Allow opening notes with Ctrl + Enter
        if (ev->state != Gdk::CONTROL_MASK) {
          if (ev->state & Gdk::SHIFT_MASK) {
            ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->add_new_line (true);
          } 
          else {
            ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->add_new_line (false);
          }          
          scroll_mark_onscreen (get_buffer()->get_insert());
        }
        break;
      case GDK_Tab:
        ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->add_tab ();
        scroll_mark_onscreen (get_buffer()->get_insert());
        break;
      case GDK_ISO_Left_Tab:
        ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->remove_tab ();
        scroll_mark_onscreen (get_buffer()->get_insert());
        break;
      case GDK_Delete:
        if (Gdk::SHIFT_MASK != (ev->state & Gdk::SHIFT_MASK)) {
          ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->delete_key_handler();
          scroll_mark_onscreen (get_buffer()->get_insert());
        }
        break;
      case GDK_BackSpace:
        ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->backspace_key_handler();
        break;
      case GDK_Left:
      case GDK_Right:
      case GDK_Up:
      case GDK_Down:
      case GDK_End:
        ret_value = false;
        break;
      default:
        NoteBuffer::Ptr::cast_static(get_buffer())->check_selection();
        break;
      }

      return ret_value;
    }


  bool NoteEditor::button_pressed (GdkEventButton * )
  {
    NoteBuffer::Ptr::cast_static(get_buffer())->check_selection();
    return false;
  }



}
