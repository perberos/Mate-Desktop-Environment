/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#include <sigc++/functors/ptr_fun.h>

#include "preferences.hpp"
#include "propertyeditor.hpp"


namespace sharp {


  PropertyEditorBase::PropertyEditorBase(const char *key, Gtk::Widget &w)
    : m_key(key), m_widget(w)
  {
    w.set_data(Glib::Quark("sharp::property-editor"), (gpointer)this,
               &PropertyEditorBase::destroy_notify);
  }

  PropertyEditorBase::~PropertyEditorBase()
  {
  }

  void PropertyEditorBase::destroy_notify(gpointer data)
  {
    PropertyEditorBase * self = (PropertyEditorBase*)data;
    delete self;
  }


  PropertyEditor::PropertyEditor(const char * key, Gtk::Entry &entry)
    : PropertyEditorBase(key, entry)
  {
    m_connection = entry.property_text().signal_changed().connect(
      sigc::mem_fun(*this, &PropertyEditor::on_changed));
  }

  void PropertyEditor::setup()
  {
    m_connection.block();
    static_cast<Gtk::Entry &>(m_widget).set_text(
      gnote::Preferences::obj().get<std::string>(m_key));
    m_connection.unblock();        
  }

  void PropertyEditor::on_changed()
  {
    std::string txt = static_cast<Gtk::Entry &>(m_widget).get_text();
    gnote::Preferences::obj().set<std::string>(m_key, txt);
  }


  PropertyEditorBool::PropertyEditorBool(const char * key, Gtk::ToggleButton &button)
    : PropertyEditorBase(key, button)
  {
    m_connection = button.property_active().signal_changed().connect(
      sigc::mem_fun(*this, &PropertyEditorBool::on_changed));
  }

  void PropertyEditorBool::guard(bool v)
  {
    for(std::vector<Gtk::Widget*>::iterator iter = m_guarded.begin();
        iter != m_guarded.end(); ++iter) {
      (*iter)->set_sensitive(v);
    }
  }


  void PropertyEditorBool::setup()
  {
    m_connection.block();
    static_cast<Gtk::ToggleButton &>(m_widget).set_active(
      gnote::Preferences::obj().get<bool>(m_key));
    m_connection.unblock();        
  }

  void PropertyEditorBool::on_changed()
  {
    bool active = static_cast<Gtk::ToggleButton &>(m_widget).get_active();
    gnote::Preferences::obj().set<bool>(m_key, active);
    guard(active);
  }

}

