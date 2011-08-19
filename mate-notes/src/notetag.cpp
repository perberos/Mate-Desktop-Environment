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



#include <boost/lexical_cast.hpp>

#include <gtk/gtk.h>
#include <gtkmm/image.h>

#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"
#include "debug.hpp"
#include "notetag.hpp"
#include "noteeditor.hpp"

namespace gnote {

  NoteTag::NoteTag(const std::string & tag_name, int flags) throw(sharp::Exception)
    : Gtk::TextTag(tag_name)
    , m_element_name(tag_name)
    , m_widget(NULL)
    , m_allow_middle_activate(false)
    , m_flags(flags | CAN_SERIALIZE | CAN_SPLIT)
  {
    if (tag_name.empty()) {
      throw sharp::Exception ("NoteTags must have a tag name.  Use "
                              "DynamicNoteTag for constructing "
                              "anonymous tags.");
    }
    
  }

  
  NoteTag::NoteTag()
    : Gtk::TextTag()
    , m_widget(NULL)
    , m_allow_middle_activate(false)
    , m_flags(0)
  {
  }


  void NoteTag::initialize(const std::string & element_name)
  {
    m_element_name = element_name;
    m_flags = CAN_SERIALIZE | CAN_SPLIT;
  }
  


  void NoteTag::set_can_serialize(bool value)
  {
    if (value) {
      m_flags |= CAN_SERIALIZE;
    }
    else {
      m_flags &= ~CAN_SERIALIZE;
    }
  }

  void NoteTag::set_can_undo(bool value)
  {
    if (value) {
      m_flags |= CAN_UNDO;
    }
    else {
      m_flags &= ~CAN_UNDO;
    }
  }

  void NoteTag::set_can_grow(bool value)
  {
    if (value) {
      m_flags |= CAN_GROW;
    }
    else {
      m_flags &= ~CAN_GROW;
    }
  }

  void NoteTag::set_can_spell_check(bool value)
  {
    if (value) {
      m_flags |= CAN_SPELL_CHECK;
    }
    else {
      m_flags &= ~CAN_SPELL_CHECK;
    }
  }

  void NoteTag::set_can_activate(bool value)
  {
    if (value) {
      m_flags |= CAN_ACTIVATE;
    }
    else {
      m_flags &= ~CAN_ACTIVATE;
    }
  }

  void NoteTag::set_can_split(bool value)
  {
    if (value) {
      m_flags |= CAN_SPLIT;
    }
    else {
      m_flags &= ~CAN_SPLIT;
    }
  }

  void NoteTag::get_extents(const Gtk::TextIter & iter, Gtk::TextIter & start,
                            Gtk::TextIter & end)
  {
    Glib::RefPtr<Gtk::TextTag> this_ref(this);
    start = iter;
    if (!start.begins_tag (this_ref)) {
      start.backward_to_tag_toggle (this_ref);
    }
    end = iter;
    end.forward_to_tag_toggle (this_ref);
  }

  void NoteTag::write(sharp::XmlWriter & xml, bool start) const
  {
    if (can_serialize()) {
      if (start) {
        xml.write_start_element ("", m_element_name, "");
      } 
      else {
        xml.write_end_element();
      }
    }
  }

  void NoteTag::read(sharp::XmlReader & xml, bool start)
  {
    if (can_serialize()) {
      if (start) {
        m_element_name = xml.get_name();
      }
    }
  }

  bool NoteTag::on_event(const Glib::RefPtr<Glib::Object> & sender, 
                         GdkEvent * ev, const Gtk::TextIter & iter)
  {
    NoteEditor::Ptr editor = NoteEditor::Ptr::cast_dynamic(sender);
    Gtk::TextIter start, end;

    if (!can_activate())
      return false;

    switch (ev->type) {
    case GDK_BUTTON_PRESS:
    {
      GdkEventButton *button_ev = (GdkEventButton*)ev;

      // Do not insert selected text when activating links with
      // middle mouse button
      if (button_ev->button == 2) {
        m_allow_middle_activate = true;
        return true;
      }

      return false;
    }
    case GDK_BUTTON_RELEASE:
    {
      GdkEventButton *button_ev = (GdkEventButton*)ev;
      if ((button_ev->button != 1) && (button_ev->button != 2))
        return false;

      /* Don't activate if Shift or Control is pressed */
      if ((button_ev->state & (Gdk::SHIFT_MASK |
                               Gdk::CONTROL_MASK)) != 0)
        return false;

      // Prevent activation when selecting links with the mouse
      if (editor->get_buffer()->get_has_selection()) {
        return false;
      }

      // Don't activate if the link has just been pasted with the
      // middle mouse button (no preceding ButtonPress event)
      if (button_ev->button == 2 && !m_allow_middle_activate) {
        return false;
      }
      else {
        m_allow_middle_activate = false;
      }

      get_extents (iter, start, end);
      bool success = on_activate (*(editor.operator->()), start, end);

      // Hide note if link is activated with middle mouse button
      if (success && (button_ev->button == 2)) {
        Glib::RefPtr<Gtk::Widget> widget = Glib::RefPtr<Gtk::Widget>::cast_static(sender);
        widget->get_toplevel()->hide ();
      }

      return false;
    }
    case GDK_KEY_PRESS:
    {
      GdkEventKey *key_ev = (GdkEventKey*)ev;

      // Control-Enter activates the link at point...
      if ((key_ev->state & Gdk::CONTROL_MASK) == 0)
        return false;

      if (key_ev->keyval != GDK_Return &&
          key_ev->keyval != GDK_KP_Enter)
        return false;

      get_extents (iter, start, end);
      return on_activate (*(editor.operator->()), start, end);
    }
    default:
      break;
    }

    return false;
  }


  bool NoteTag::on_activate(const NoteEditor & editor , const Gtk::TextIter & start, 
                            const Gtk::TextIter & end)
  {
    bool retval = false;

#if 0
    if (Activated != null) {
      foreach (Delegate d in Activated.GetInvocationList()) {
        TagActivatedHandler handler = (TagActivatedHandler) d;
        retval |= handler (*this, editor, start, end);
      }
    }
#endif
    retval = m_signal_activate(NoteTag::Ptr(this), editor, start, end);

    return retval;
  }


  Glib::RefPtr<Gdk::Pixbuf> NoteTag::get_image() const
  {
    Gtk::Image * image = dynamic_cast<Gtk::Image*>(m_widget);
    if(!image) {
      return Glib::RefPtr<Gdk::Pixbuf>();
    }
    return image->get_pixbuf();
  }


  void NoteTag::set_image(const Glib::RefPtr<Gdk::Pixbuf> & value)
  {
    if(!value) {
      set_widget(NULL);
      return;
    }
    set_widget(new Gtk::Image(value));
  }


  void NoteTag::set_widget(Gtk::Widget * value)
  {
    if ((value == NULL) && m_widget) {
      delete m_widget;
    }

    m_widget = value;

    try {
      m_signal_changed(Glib::RefPtr<Gtk::TextTag>(this), false);
    } catch (sharp::Exception & e) {
      DBG_OUT("Exception calling TagChanged from NoteTag.set_Widget: %s", e.what());
    }
  }

  Gdk::Color NoteTag::get_background() const
  {
    /* We can't know the exact background because we're not
       in TextView's rendering, but we can make a guess */
    if (property_background_set().get_value())
      return property_background_gdk().get_value();

    
    Glib::RefPtr<Gtk::Style> s = Glib::wrap(gtk_rc_get_style_by_paths(gtk_settings_get_default(),
                                                                "GtkTextView", "GtkTextView",
                                                                      GTK_TYPE_TEXT_VIEW), true);
    if (!s) {
      DBG_OUT("get_background: Style for GtkTextView came back null! Returning white...");
      Gdk::Color color;
      color.set_rgb(0xff, 0xff, 0xff); //white, for lack of a better idea
      return color;
    }
    else
      return s->get_background(Gtk::STATE_NORMAL);
  }

  Gdk::Color NoteTag::render_foreground(ContrastPaletteColor symbol)
  {
    return contrast_render_foreground_color(get_background(), symbol);
  }


  void DynamicNoteTag::write(sharp::XmlWriter & xml, bool start) const
  {
    if (can_serialize()) {
      NoteTag::write (xml, start);

      if (start) {
        for(AttributeMap::const_iterator iter = m_attributes.begin();
            iter != m_attributes.end(); ++iter) {
          xml.write_attribute_string ("", iter->first, "", iter->second);
        }
      }
    }
  }

  void DynamicNoteTag::read(sharp::XmlReader & xml, bool start)
  {
    if (can_serialize()) {
      NoteTag::read (xml, start);

      if (start) {
          while (xml.move_to_next_attribute()) {
            std::string name = xml.get_name();

            xml.read_attribute_value();
            m_attributes [name] = xml.get_value();

            on_attribute_read (name);
            DBG_OUT("NoteTag: %s read attribute %s='%s'",
                    get_element_name().c_str(), name.c_str(), xml.get_value().c_str());
          }
      }
    }
  }
  
  DepthNoteTag::DepthNoteTag(int depth, Pango::Direction direction)
    : NoteTag("depth:" + boost::lexical_cast<std::string>(depth) 
              + ":" + boost::lexical_cast<std::string>((int)direction))
    , m_depth(depth)
    , m_direction(direction)
  {
  }

  

  void DepthNoteTag::write(sharp::XmlWriter & xml, bool start) const
  {
    if (can_serialize()) {
      if (start) {
        xml.write_start_element ("", "list-item", "");

        // Write the list items writing direction
        xml.write_start_attribute ("dir");
        if (get_direction() == Pango::DIRECTION_RTL) {
          xml.write_string ("rtl");
        }
        else {
          xml.write_string ("ltr");
        }
        xml.write_end_attribute ();
      } 
      else {
        xml.write_end_element ();
      }
    }
  }


  NoteTagTable::Ptr NoteTagTable::s_instance;

  void NoteTagTable::_init_common_tags()
  {
    NoteTag::Ptr tag;

    // Font stylings

    tag = NoteTag::create("centered", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_justification() = Gtk::JUSTIFY_CENTER;
    add (tag);

    tag = NoteTag::create("bold", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_weight() = PANGO_WEIGHT_BOLD;
    add (tag);

    tag = NoteTag::create("italic", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_style() = Pango::STYLE_ITALIC;
    add (tag);
    
    tag = NoteTag::create("strikethrough", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_strikethrough() = true;
    add (tag);

    tag = NoteTag::create("highlight", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_background() = "yellow";
    add (tag);

    tag = NoteTag::create("find-match", NoteTag::CAN_SPELL_CHECK);
    tag->property_background() = "green";
    tag->set_can_serialize(false);
    add (tag);

    tag = NoteTag::create("note-title", 0);
    tag->property_underline() = Pango::UNDERLINE_SINGLE;
    tag->set_palette_foreground(CONTRAST_COLOR_BLUE);
    tag->property_scale() = Pango::SCALE_XX_LARGE;
    // FiXME: Hack around extra rewrite on open
    tag->set_can_serialize(false);
    add (tag);
      
    tag = NoteTag::create("related-to", 0);
    tag->property_scale() = Pango::SCALE_SMALL;
    tag->property_left_margin() = 40;
    tag->property_editable() = false;
    add (tag);

    // Used when inserting dropped URLs/text to Start Here
    tag = NoteTag::create("datetime", 0);
    tag->property_scale() = Pango::SCALE_SMALL;
    tag->property_style() = Pango::STYLE_ITALIC;
    tag->set_palette_foreground(CONTRAST_COLOR_GREY);
    add (tag);

    // Font sizes

    tag = NoteTag::create("size:huge", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_scale() = Pango::SCALE_XX_LARGE;
    add (tag);

    tag = NoteTag::create("size:large", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_scale() = Pango::SCALE_X_LARGE;
    add (tag);

    tag = NoteTag::create("size:normal", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_scale() = Pango::SCALE_MEDIUM;
    add (tag);

    tag = NoteTag::create("size:small", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_scale() = Pango::SCALE_SMALL;
    add (tag);

    // Links

    tag = NoteTag::create("link:broken", NoteTag::CAN_ACTIVATE);
    tag->property_underline() = Pango::UNDERLINE_SINGLE;
    tag->set_palette_foreground(CONTRAST_COLOR_GREY);
    add (tag);
    m_broken_link_tag = tag;

    tag = NoteTag::create("link:internal", NoteTag::CAN_ACTIVATE);
    tag->property_underline() = Pango::UNDERLINE_SINGLE;
    tag->set_palette_foreground(CONTRAST_COLOR_BLUE);
    add (tag);
    m_link_tag = tag;

    tag = NoteTag::create("link:url", NoteTag::CAN_ACTIVATE);
    tag->property_underline() = Pango::UNDERLINE_SINGLE;
    tag->set_palette_foreground(CONTRAST_COLOR_BLUE);
    add (tag);
    m_url_tag = tag;
  }


  bool NoteTagTable::tag_is_serializable(const Glib::RefPtr<const Gtk::TextTag> & tag)
  {
    NoteTag::ConstPtr note_tag = NoteTag::ConstPtr::cast_dynamic(tag);
    if(note_tag) {
      return note_tag->can_serialize();
    }
    return false;
  }

  bool NoteTagTable::tag_is_growable(const Glib::RefPtr<Gtk::TextTag> & tag)
  {
    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if(note_tag) {
      return note_tag->can_grow();
    }
    return false;
  }

  bool NoteTagTable::tag_is_undoable(const Glib::RefPtr<Gtk::TextTag> & tag)
  {
    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if(note_tag) {
      return note_tag->can_undo();
    }
    return false;
  }


  bool NoteTagTable::tag_is_spell_checkable(const Glib::RefPtr<const Gtk::TextTag> & tag)
  {
    NoteTag::ConstPtr note_tag = NoteTag::ConstPtr::cast_dynamic(tag);
    if(note_tag) {
      return note_tag->can_spell_check();
    }
    return false;
  }


  bool NoteTagTable::tag_is_activatable(const Glib::RefPtr<Gtk::TextTag> & tag)
  {
    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if(note_tag) {
      return note_tag->can_activate();
    }
    return false;
  }


  bool NoteTagTable::tag_has_depth(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag)
  {
    return DepthNoteTag::Ptr::cast_dynamic(tag);
  }
  

  DepthNoteTag::Ptr NoteTagTable::get_depth_tag(int depth, Pango::Direction direction)
  {
    std::string name = "depth:" + boost::lexical_cast<std::string>(depth) 
      + ":" + boost::lexical_cast<std::string>((int)direction);

    DepthNoteTag::Ptr tag = DepthNoteTag::Ptr::cast_dynamic(lookup(name));

    if (!tag) {
      tag = DepthNoteTag::Ptr(new DepthNoteTag (depth, direction));
      tag->property_indent().set_value(-14);

      if (direction == Pango::DIRECTION_RTL) {
        tag->property_right_margin().set_value((depth+1) * 25);
      }
      else {
        tag->property_left_margin().set_value((depth+1) * 25);
      }

      tag->property_pixels_below_lines().set_value(4);
      tag->property_scale().set_value(Pango::SCALE_MEDIUM);
      add (tag);
    }

    return tag;
  }
      
  DynamicNoteTag::Ptr NoteTagTable::create_dynamic_tag(const std::string & tag_name)
  {
    std::map<std::string, Factory>::iterator iter = m_tag_types.find(tag_name);
    if(iter == m_tag_types.end()) {
      return DynamicNoteTag::Ptr();
    }
    DynamicNoteTag::Ptr tag(iter->second());
    tag->initialize(tag_name);
    add(tag);
    return tag;
  }

 
  void NoteTagTable::register_dynamic_tag(const std::string & tag_name, const Factory & factory)
  {
    m_tag_types[tag_name] = factory;
  }


  bool NoteTagTable::is_dynamic_tag_registered(const std::string & tag_name)
  {
    return m_tag_types.find(tag_name) != m_tag_types.end();
  }

  void NoteTagTable::on_tag_added(const Glib::RefPtr<Gtk::TextTag> & tag)
  {
    m_added_tags.push_back(tag);

    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if (note_tag) {
//      note_tag->signal_changed().connect(sigc::mem_fun(*this, &NoteTagTable::on_notetag_changed));
    }
  }

  
  void NoteTagTable::on_tag_removed(const Glib::RefPtr<Gtk::TextTag> & tag)
  {
    m_added_tags.remove(tag);

    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if (note_tag) {
// TODO disconnect the signal
//      note_tag.Changed -= OnTagChanged;
    }
  }


#if 0
  void NoteTagTable::on_notetag_changed(Glib::RefPtr<Gtk::TextTag>& tag, bool size_changed)
  {
    m_signal_changed(tag, size_changed);
  }
#endif

}

