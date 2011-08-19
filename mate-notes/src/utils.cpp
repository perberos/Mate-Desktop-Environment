/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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


#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <algorithm>

#include <boost/format.hpp>
#include <boost/bind.hpp>

#include <gtk/gtk.h>

#include <glibmm/i18n.h>
#include <glibmm/spawn.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/textbuffer.h>

#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"
#include "sharp/string.hpp"
#include "sharp/uri.hpp"
#include "sharp/datetime.hpp"
#include "note.hpp"
#include "utils.hpp"
#include "debug.hpp"

namespace gnote {
  namespace utils {

    namespace {
      void get_menu_position (Gtk::Menu * menu,
                              int & x,
                              int & y,
                              bool & push_in)
      {
        if (menu->get_attach_widget() == NULL ||
            menu->get_attach_widget()->get_window() == NULL) {
          // Prevent null exception in weird cases
          x = 0;
          y = 0;
          push_in = true;
          return;
        }
      
        menu->get_attach_widget()->get_window()->get_origin(x, y);
        x += menu->get_attach_widget()->get_allocation().get_x();
        
        Gtk::Requisition menu_req = menu->size_request();
        if (y + menu_req.height >= menu->get_attach_widget()->get_screen()->get_height()) {
          y -= menu_req.height;
        }
        else {
          y += menu->get_attach_widget()->get_allocation().get_height();
        }
        
        push_in = true;
      }


      void deactivate_menu(Gtk::Menu *menu)
      {
        menu->popdown();
        if(menu->get_attach_widget()) {
          menu->get_attach_widget()->set_state(Gtk::STATE_NORMAL);
        }
      }
    }


    void popup_menu(Gtk::Menu &menu, const GdkEventButton * ev)
    {
      menu.signal_deactivate().connect(sigc::bind(&deactivate_menu, &menu));
      menu.popup(boost::bind(&get_menu_position, &menu, _1, _2, _3), 
                 (ev ? ev->button : 0), 
                 (ev ? ev->time : gtk_get_current_event_time()));
      if(menu.get_attach_widget()) {
        menu.get_attach_widget()->set_state(Gtk::STATE_SELECTED);
      }
    }


    Glib::RefPtr<Gdk::Pixbuf> get_icon(const std::string & name, int size)
    {
      try {
        return Gtk::IconTheme::get_default()->load_icon(name, size, (Gtk::IconLookupFlags) 0);
      }
      catch(const Glib::Exception & e)
      {
        std::cout << e.what().c_str() << std::endl;
      }
      return Glib::RefPtr<Gdk::Pixbuf>();
    }

    void show_help(const std::string & filename, const std::string & link_id,
                   GdkScreen *screen, Gtk::Window *parent)
    {
      std::string uri = "ghelp:" + filename;
      if(!link_id.empty()) {
        uri += "#" + link_id;
      }
      GError *error = NULL;

      if(!gtk_show_uri (screen, uri.c_str(), gtk_get_current_event_time (), &error)) {
        
        std::string message = _("The \"Gnote Manual\" could "
                                "not be found.  Please verify "
                                "that your installation has been "
                                "completed successfully.");
        HIGMessageDialog dialog(parent,
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                Gtk::MESSAGE_ERROR,
                                Gtk::BUTTONS_OK,
                                _("Help not found"),
                                message);
        dialog.run();
        if(error) {
          g_error_free(error);
        }
      }
    }


    void open_url(const std::string & url)
      throw (Glib::Error)
    {
      if(!url.empty()) {
        GError *err = NULL;
        DBG_OUT("Opening url '%s'...", url.c_str());
        gtk_show_uri (NULL, url.c_str(), GDK_CURRENT_TIME, &err);
        if(err) {
          throw Glib::Error(err, true);
        }
      }
    }


    void show_opening_location_error(Gtk::Window * parent, 
                                     const std::string & url, 
                                     const std::string & error)
    {
      std::string message = str(boost::format ("%1%: %2%") % url % error);

      HIGMessageDialog dialog(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                              Gtk::MESSAGE_INFO,
                              Gtk::BUTTONS_OK,
                              _("Cannot open location"),
                              message);
      dialog.run ();
    }

    std::string get_pretty_print_date(const sharp::DateTime & date, bool show_time)
    {
      std::string pretty_str;
      sharp::DateTime now = sharp::DateTime::now();
      std::string short_time = date.to_short_time_string ();

      if (date.year() == now.year()) {
        if (date.day_of_year() == now.day_of_year()) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument is time. */
            str(boost::format(_("Today, %1%")) % short_time) :
            _("Today");
        }
        else if ((date.day_of_year() < now.day_of_year())
                 && (date.day_of_year() == now.day_of_year() - 1)) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument is time. */
            str(boost::format(_("Yesterday, %1%")) % short_time) :
            _("Yesterday");
        }
        else if ((date.day_of_year() < now.day_of_year())
                  && (date.day_of_year() > now.day_of_year() - 6)) {
          int num_days = now.day_of_year() - date.day_of_year();
          const char * fmt;
          if(show_time) {
            /* TRANSLATORS: 2 or more days ago, up to one week.
               First argument is number of days, second is time. */
            fmt = ngettext("%1% day ago, %2%", "%1% days ago, %2%", num_days);
            pretty_str = str(boost::format(fmt) % num_days % short_time);
          }
          else {
            /* TRANSLATORS: 2 or more days ago, up to one week.
               Argument is number of days. */
            fmt = ngettext("%1% day ago", "%1% days ago", num_days);
            pretty_str = str(boost::format(fmt) % num_days);
          }
        }
        else if (date.day_of_year() > now.day_of_year()
                 && date.day_of_year() == now.day_of_year() + 1) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument is time. */
            str(boost::format(_("Tomorrow, %1%")) % short_time) :
            _("Tomorrow");
        }
        else if (date.day_of_year() > now.day_of_year()
                 && date.day_of_year() < now.day_of_year() + 6) {
          int num_days = date.day_of_year() - now.day_of_year();
          const char * fmt;
          if(show_time) {
            /* TRANSLATORS: In 2 or more days, up to one week.
               First argument is number of days, second is time. */
            fmt = ngettext("In %1% day, %2%", "In %1% days, %2%", num_days);
            pretty_str = str(boost::format(fmt) % num_days % short_time); 
          }
          else {
            /* TRANSLATORS: In 2 or more days, up to one week.
               Argument is number of days. */
            fmt = ngettext("In %1% day", "In %1% days", num_days);
            pretty_str = str(boost::format(fmt) % num_days);
          }
        }
        else {
          pretty_str = show_time ?
            date.to_string ("%B %d, %H:%M %p") : // "MMMM d, h:mm tt"
            date.to_string ("%B %d");            // "MMMM d"
        }
      } 
      else if (!date.is_valid()) {
        pretty_str = _("No Date");
      }
      else {
        pretty_str = show_time ?
          date.to_string ("%B %d %Y, %H:%M %p") : // "MMMM d yyyy, h:mm tt"
          date.to_string ("%B %d %Y");            // "MMMM d yyyy"
      }

      return pretty_str;
    }


    void GlobalKeybinder::add_accelerator(const sigc::slot<void> & handler, guint key, 
                                          Gdk::ModifierType modifiers, Gtk::AccelFlags flags)
    {
      Gtk::MenuItem *foo = manage(new Gtk::MenuItem ());
      foo->signal_activate().connect(handler);
      foo->add_accelerator ("activate",
                          m_accel_group,
                          key,
                          modifiers,
                          flags);
      foo->show ();

      m_fake_menu.append (*foo);
    }


    HIGMessageDialog::HIGMessageDialog(Gtk::Window *parent,
                                       GtkDialogFlags flags, Gtk::MessageType msg_type, 
                                       Gtk::ButtonsType btn_type, const Glib::ustring & header,
                                       const Glib::ustring & msg)
      : Gtk::Dialog()
      , m_extra_widget(NULL)
    {
      set_has_separator(false);
      set_border_width(5);
      set_resizable(false);
      set_title("");

      get_vbox()->set_spacing(12);
      get_action_area()->set_layout(Gtk::BUTTONBOX_END);

      m_accel_group = Glib::RefPtr<Gtk::AccelGroup>(Gtk::AccelGroup::create());
      add_accel_group(m_accel_group);

      Gtk::HBox *hbox = manage(new Gtk::HBox (false, 12));
      hbox->set_border_width(5);
      hbox->show();
      get_vbox()->pack_start(*hbox, false, false, 0);

      switch (msg_type) {
      case Gtk::MESSAGE_ERROR:
        m_image = new Gtk::Image (Gtk::Stock::DIALOG_ERROR,
                                  Gtk::ICON_SIZE_DIALOG);
        break;
      case Gtk::MESSAGE_QUESTION:
        m_image = new Gtk::Image (Gtk::Stock::DIALOG_QUESTION,
                                  Gtk::ICON_SIZE_DIALOG);
        break;
      case Gtk::MESSAGE_INFO:
        m_image = new Gtk::Image (Gtk::Stock::DIALOG_INFO,
                                  Gtk::ICON_SIZE_DIALOG);
        break;
      case Gtk::MESSAGE_WARNING:
        m_image = new Gtk::Image (Gtk::Stock::DIALOG_WARNING,
                                  Gtk::ICON_SIZE_DIALOG);
        break;
      default:
        m_image = new Gtk::Image ();
        break;
      }

      if (m_image) {
        Gtk::manage(m_image);
        m_image->show();
        m_image->property_yalign().set_value(0);
        hbox->pack_start(*m_image, false, false, 0);
      }

      Gtk::VBox *label_vbox = manage(new Gtk::VBox (false, 0));
      label_vbox->show();
      hbox->pack_start(*label_vbox, true, true, 0);

      std::string title = str(boost::format("<span weight='bold' size='larger'>%1%"
                                            "</span>\n") % header);

      Gtk::Label *label;

      label = manage(new Gtk::Label (title));
      label->set_use_markup(true);
      label->set_justify(Gtk::JUSTIFY_LEFT);
      label->set_line_wrap(true);
      label->set_alignment (0.0f, 0.5f);
      label->show();
      label_vbox->pack_start(*label, false, false, 0);

      label = manage(new Gtk::Label(msg));
      label->set_use_markup(true);
      label->set_justify(Gtk::JUSTIFY_LEFT);
      label->set_line_wrap(true);
      label->set_alignment (0.0f, 0.5f);
      label->show();
      label_vbox->pack_start(*label, false, false, 0);
      
      m_extra_widget_vbox = manage(new Gtk::VBox (false, 0));
      m_extra_widget_vbox->show();
      label_vbox->pack_start(*m_extra_widget_vbox, true, true, 12);

      switch (btn_type) {
      case Gtk::BUTTONS_NONE:
        break;
      case Gtk::BUTTONS_OK:
        add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK, true);
        break;
      case Gtk::BUTTONS_CLOSE:
        add_button (Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE, true);
        break;
      case Gtk::BUTTONS_CANCEL:
        add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL, true);
        break;
      case Gtk::BUTTONS_YES_NO:
        add_button (Gtk::Stock::NO, Gtk::RESPONSE_NO, false);
        add_button (Gtk::Stock::YES, Gtk::RESPONSE_YES, true);
        break;
      case Gtk::BUTTONS_OK_CANCEL:
        add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL, false);
        add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK, true);
        break;
      }

      if (parent){
        set_transient_for(*parent);
      }

      if ((flags & GTK_DIALOG_MODAL) != 0) {
        set_modal(true);
      }

      if ((flags & GTK_DIALOG_DESTROY_WITH_PARENT) != 0) {
        property_destroy_with_parent().set_value(true);
      }
    }


    void HIGMessageDialog::add_button(const Gtk::BuiltinStockID& stock_id, 
                                       Gtk::ResponseType resp, bool is_default)
    {
      Gtk::Button *button = manage(new Gtk::Button (stock_id));
      button->property_can_default().set_value(true);
      
      add_button(button, resp, is_default);
    }

    void HIGMessageDialog::add_button (const Glib::RefPtr<Gdk::Pixbuf> & pixbuf, 
                                       const Glib::ustring & label_text, 
                                       Gtk::ResponseType resp, bool is_default)
    {
      Gtk::Button *button = manage(new Gtk::Button());
      Gtk::Image *image = manage(new Gtk::Image(pixbuf));
      // NOTE: This property is new to GTK+ 2.10, but we don't
      //       really need the line because we're just setting
      //       it to the default value anyway.
      //button.ImagePosition = Gtk::PositionType.Left;
      button->set_image(*image);
      button->set_label(label_text);
      button->set_use_underline(true);
      button->property_can_default().set_value(true);
      
      add_button (button, resp, is_default);
    }
    
    void HIGMessageDialog::add_button (Gtk::Button *button, Gtk::ResponseType resp, bool is_default)
    {
      button->show();

      add_action_widget (*button, resp);

      if (is_default) {
        set_default_response(resp);
        button->add_accelerator ("activate", m_accel_group,
                                 GDK_Escape, (Gdk::ModifierType)0,
                                 Gtk::ACCEL_VISIBLE);
      }
    }


    void HIGMessageDialog::set_extra_widget(Gtk::Widget *value)
    {
      if (m_extra_widget) {
          m_extra_widget_vbox->remove (*m_extra_widget);
      }
        
      m_extra_widget = value;
      m_extra_widget->show_all ();
      m_extra_widget_vbox->pack_start (*m_extra_widget, true, true, 0);
    }


#if 0
    UriList::UriList(const NoteList & notes)
    {
      foreach(const Note::Ptr & note, notes) {
        push_back(sharp::Uri(note->uri()));
      }
    }
#endif

    void UriList::load_from_string(const std::string & data)
    {
      std::vector<std::string> items;
      sharp::string_split(items, data, "\n");
      load_from_string_list(items);
    }

    void UriList::load_from_string_list(const std::vector<std::string> & items)
    {
      for(std::vector<std::string>::const_iterator iter = items.begin();
          iter != items.end(); ++iter) {

        const std::string & i(*iter);

        if(Glib::str_has_prefix(i, "#")) {
          continue;
        }

        std::string s = i;
        if(Glib::str_has_suffix(i, "\r")) {
          s.erase(s.end() - 1, s.end());
        }

        // Handle evo's broken file urls
        if(Glib::str_has_prefix(s, "file:////")) {
          s = sharp::string_replace_first(s, "file:////", "file:///");
        }
        DBG_OUT("uri = %s", s.c_str());
        push_back(sharp::Uri(s));
      }
    }

    UriList::UriList(const std::string & data)
    {
      load_from_string(data);
    }

    
    UriList::UriList(const Gtk::SelectionData & selection)
    {
      if(selection.get_length() > 0) {
        load_from_string_list(selection.get_uris());
      }
    }


    std::string UriList::to_string() const
    {
      std::string s;
      for(const_iterator iter = begin(); iter != end(); ++iter) {
        s += iter->to_string() + "\r\n";
      }
      return s;
    }


    void UriList::get_local_paths(std::list<std::string> & paths) const
    {
      for(const_iterator iter = begin(); iter != end(); ++iter) {

        const sharp::Uri & uri(*iter);

        if(uri.is_file()) {
          paths.push_back(uri.local_path());
        }
      }
    }


    std::string XmlEncoder::encode(const std::string & source)
    {
      sharp::XmlWriter xml;
      xml.write_string(source);

      xml.close();
      return xml.to_string();
    }


    std::string XmlDecoder::decode(const std::string & source)
    {
      // TODO there is probably better than a std::string for that.
      // this will do for now.
      std::string builder;

      sharp::XmlReader xml;
      xml.load_buffer(source);

      while (xml.read ()) {
        switch (xml.get_node_type()) {
        case XML_READER_TYPE_TEXT:
        case XML_READER_TYPE_WHITESPACE:
          builder += xml.get_value();
          break;
        default:
          break;
        }
      }

      xml.close ();

      return builder;
    }


    TextRange::TextRange()
    {
    }


    TextRange::TextRange(const Gtk::TextIter & _start,
                         const Gtk::TextIter & _end) throw(sharp::Exception)
    {
      if(_start.get_buffer() != _end.get_buffer()) {
        throw(sharp::Exception("Start buffer and end buffer do not match"));
      }
      m_buffer = _start.get_buffer();
      m_start_mark = m_buffer->create_mark(_start, true);
      m_end_mark = m_buffer->create_mark(_end, true);
    }

    Gtk::TextIter TextRange::start() const
    {
      return m_buffer->get_iter_at_mark(m_start_mark);
    }


    void TextRange::set_start(const Gtk::TextIter & value)
    {
      m_buffer->move_mark(m_start_mark, value);
    }

    Gtk::TextIter TextRange::end() const
    {
      return m_buffer->get_iter_at_mark(m_end_mark);
    }

    void TextRange::set_end(const Gtk::TextIter & value)
    {
      m_buffer->move_mark(m_end_mark, value);
    }

    void TextRange::erase()
    {
      Gtk::TextIter start_iter = start();
      Gtk::TextIter end_iter = end();
      m_buffer->erase(start_iter, end_iter);
    }

    void TextRange::destroy()
    {
      m_buffer->delete_mark(m_start_mark);
      m_buffer->delete_mark(m_end_mark);
    }

    void TextRange::remove_tag(const Glib::RefPtr<Gtk::TextTag> & tag)
    {
      m_buffer->remove_tag(tag, start(), end());
    }


    TextTagEnumerator::TextTagEnumerator(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
                                         const Glib::RefPtr<Gtk::TextTag> & tag)
      : m_buffer(buffer)
      , m_tag(tag)
      , m_mark(buffer->create_mark(buffer->begin(), true))
      , m_range(buffer->begin(), buffer->begin())
    {
    }

    bool TextTagEnumerator::move_next()
    {
      Gtk::TextIter iter = m_buffer->get_iter_at_mark(m_mark);

      if (iter == m_buffer->end()) {
        m_range.destroy();
        m_buffer->delete_mark(m_mark);
        return false;
      }

      if (!iter.forward_to_tag_toggle(m_tag)) {
        m_range.destroy();
        m_buffer->delete_mark(m_mark);
        return false;
      }

      if (!iter.begins_tag(m_tag)) {
        m_buffer->move_mark(m_mark, iter);
        return move_next();
      }

      m_range.set_start(iter);

      if (!iter.forward_to_tag_toggle(m_tag)) {
        m_range.destroy();
        m_buffer->delete_mark(m_mark);
        return false;
      }

      if (!iter.ends_tag(m_tag)) {
        m_buffer->move_mark(m_mark, iter);
        return move_next();
      }

      m_range.set_end(iter);

      m_buffer->move_mark(m_mark, iter);

      return true;
    }

    

    InterruptableTimeout::~InterruptableTimeout()
    {
      cancel();
    }


    bool InterruptableTimeout::callback(InterruptableTimeout* self)
    {
      if(self)
        return self->timeout_expired();
      return false;
    }

    void InterruptableTimeout::reset(guint timeout_millis)
    {
      cancel();
      m_timeout_id = g_timeout_add(timeout_millis, (GSourceFunc)callback, this);
    }

    void InterruptableTimeout::cancel()
    {
      if(m_timeout_id != 0) {
        g_source_remove(m_timeout_id);
        m_timeout_id = 0;
      }
    }

    bool InterruptableTimeout::timeout_expired()
    {
      signal_timeout();
      m_timeout_id = 0;
      return false;
    }

    ToolMenuButton::ToolMenuButton(Gtk::Toolbar& toolbar, const Gtk::BuiltinStockID& stock_image, 
                                   const Glib::ustring & label, 
                                   Gtk::Menu * menu)
      : Gtk::ToggleToolButton()
      ,  m_menu(menu)
    {
      _common_init(*manage(new Gtk::Image(stock_image, toolbar.get_icon_size())),
                   label);
    }

    ToolMenuButton::ToolMenuButton(Gtk::Image& image, 
                                   const Glib::ustring & label, 
                                   Gtk::Menu * menu)
      : Gtk::ToggleToolButton()
      ,  m_menu(menu)
    {
      _common_init(image, label);
    }


    void ToolMenuButton::_common_init(Gtk::Image& image, const Glib::ustring & label)
    {
      set_icon_widget(image);
      set_label_widget(*manage(new Gtk::Label(label, true)));
      property_can_focus() = true;
      gtk_menu_attach_to_widget(m_menu->gobj(), static_cast<Gtk::Widget*>(this)->gobj(),
                                NULL);
//      menu.attach_to_widget(*this);
      m_menu->signal_deactivate().connect(sigc::mem_fun(*this, &ToolMenuButton::release_button));
      show_all();
    }


    bool ToolMenuButton::on_button_press_event(GdkEventButton *ev)
    {
      popup_menu(*m_menu, ev);
      return true;
    }

    void ToolMenuButton::on_clicked()
    {
      m_menu->select_first(true);
      popup_menu(*m_menu, NULL);
    }

    bool ToolMenuButton::on_mnemonic_activate(bool group_cycling)
    {
      // ToggleButton always grabs focus away from the editor,
      // so reimplement Widget's version, which only grabs the
      // focus if we are group cycling.
      if (!group_cycling) {
        activate();
      } 
      else if (can_focus()) {
        grab_focus();
      }

      return true;
    }


    void ToolMenuButton::release_button()
    {
      set_active(false);
    }
    
    

  }
}
