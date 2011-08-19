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



#include <boost/format.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/image.h>
#include <gtkmm/printoperation.h>
#include <gtkmm/stock.h>

#include "sharp/datetime.hpp"
#include "debug.hpp"
#include "notewindow.hpp"
#include "printnotesnoteaddin.hpp"
#include "utils.hpp"

namespace printnotes {

  PrintNotesModule::PrintNotesModule()
  {
    ADD_INTERFACE_IMPL(PrintNotesNoteAddin);
  }
  const char * PrintNotesModule::id() const
  {
    return "PrintNotesAddin";
  }
  const char * PrintNotesModule::name() const
  {
    return _("Printing Support");
  }
  const char * PrintNotesModule::description() const
  {
    return _("Allows you to print a note.");
  }
  const char * PrintNotesModule::authors() const
  {
    return _("Hubert Figuiere and the Tomboy Project");
  }
  int PrintNotesModule::category() const
  {
    return gnote::ADDIN_CATEGORY_DESKTOP_INTEGRATION;
  }
  const char * PrintNotesModule::version() const
  {
    return "0.4";
  }

  void PrintNotesNoteAddin::initialize()
  {
  }


  void PrintNotesNoteAddin::shutdown()
  {
  }


  void PrintNotesNoteAddin::on_note_opened()
  {
    m_item = manage(new Gtk::ImageMenuItem (_("Print")));
    m_item->set_image(*manage(new Gtk::Image (Gtk::Stock::PRINT,
                                             Gtk::ICON_SIZE_MENU)));
    m_item->signal_activate().connect(
      sigc::mem_fun(*this, &PrintNotesNoteAddin::print_button_clicked));
    m_item->add_accelerator ("activate", get_window()->get_accel_group(),
                             GDK_P, Gdk::CONTROL_MASK,
                             Gtk::ACCEL_VISIBLE);
    m_item->show ();
    add_plugin_menu_item (m_item);
  }


  void PrintNotesNoteAddin::print_button_clicked()
  {
    try {
      m_print_op = Gtk::PrintOperation::create();
      m_print_op->set_job_name(get_note()->get_title());

      Glib::RefPtr<Gtk::PrintSettings> settings = Gtk::PrintSettings::create();

      Glib::ustring dir = Glib::get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS);
      if (dir.empty()) {
        dir = Glib::get_home_dir();
      }
      Glib::ustring ext;
      if (settings->get(Gtk::PrintSettings::Keys::OUTPUT_FILE_FORMAT) == "ps") {
        ext = ".ps";
      }
      else {
        ext = ".pdf";
      }

      Glib::ustring uri = "file://";
      uri += dir + "/gnotes" + ext;
      settings->set (Gtk::PrintSettings::Keys::OUTPUT_URI, uri);
      m_print_op->set_print_settings (settings);

      m_print_op->signal_begin_print().connect(
        sigc::mem_fun(*this, &PrintNotesNoteAddin::on_begin_print));
      m_print_op->signal_draw_page().connect(
        sigc::mem_fun(*this, &PrintNotesNoteAddin::on_draw_page));
      m_print_op->signal_end_print().connect(
        sigc::mem_fun(*this, &PrintNotesNoteAddin::on_end_print));

      m_print_op->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG, *get_window());
    } 
    catch (const sharp::Exception & e) 
    {
      DBG_OUT("Exception while printing %s: %s", get_note()->get_title().c_str(),
              e.what());
      gnote::utils::HIGMessageDialog dlg(get_note()->get_window(),
                                         GTK_DIALOG_MODAL,
                                         Gtk::MESSAGE_ERROR,
                                         Gtk::BUTTONS_OK,
                                         _("Error printing note"),
                                         e.what());
      dlg.run ();
    }
    m_print_op.clear(); // yeah I really mean clear the pointer.
  }


  void PrintNotesNoteAddin::get_paragraph_attributes(const Glib::RefPtr<Pango::Layout> & layout,
                                                     double dpiX, 
                                                     int & indentation,
                                                     Gtk::TextIter & position, 
                                                     const Gtk::TextIter & limit,
                                                     std::list<Pango::Attribute> & attributes)
  {
    attributes.clear();
    indentation = 0;

    Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tags = position.get_tags();
    position.forward_to_tag_toggle(Glib::RefPtr<Gtk::TextTag>(NULL));
    if (position.compare (limit) > 0) {
      position = limit;
    }

    Glib::RefPtr<Gdk::Screen> screen = get_note()->get_window()->get_screen();
    double screen_dpiX = screen->get_width_mm() * 254 / screen->get_width();

    for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator iter = tags.begin();
        iter != tags.end(); ++iter) {
      
      Glib::RefPtr<Gtk::TextTag> tag(*iter);

      if (tag->property_paragraph_background_set()) {
        Gdk::Color color = tag->property_paragraph_background_gdk();
        attributes.push_back(Pango::Attribute::create_attr_background(
                               color.get_red(), color.get_green(),
                               color.get_blue()));
      }
      if (tag->property_foreground_set()) {
        Gdk::Color color = tag->property_foreground_gdk();;
        attributes.push_back(Pango::Attribute::create_attr_foreground(
                               color.get_red(), color.get_green(), 
                               color.get_blue()));
      }
      if (tag->property_indent_set()) {
        layout->set_indent(tag->property_indent());
      }
      if (tag->property_left_margin_set()) {                                        
        indentation = (int)(tag->property_left_margin() / screen_dpiX * dpiX);
      }
      if (tag->property_right_margin_set()) {
        indentation = (int)(tag->property_right_margin() / screen_dpiX * dpiX);
      }
//      if (tag->property_font_desc()) {
      attributes.push_back(
        Pango::Attribute::create_attr_font_desc (tag->property_font_desc()));
//      }
      if (tag->property_family_set()) {
        attributes.push_back(
          Pango::Attribute::create_attr_family (tag->property_family()));
      }
      if (tag->property_size_set()) {
        attributes.push_back(Pango::Attribute::create_attr_size (
                               tag->property_size()));
      }
      if (tag->property_style_set()) {
        attributes.push_back(Pango::Attribute::create_attr_style (
                               tag->property_style()));
      }
      if (tag->property_underline_set() 
          && tag->property_underline() != Pango::UNDERLINE_ERROR) {
        attributes.push_back(
          Pango::Attribute::create_attr_underline (
            tag->property_underline()));
      }
      if (tag->property_weight_set()) {
        attributes.push_back(
          Pango::Attribute::create_attr_weight(
            Pango::Weight(tag->property_weight().get_value())));
      }
      if (tag->property_strikethrough_set()) {
        attributes.push_back(
          Pango::Attribute::create_attr_strikethrough (
            tag->property_strikethrough()));
      }
      if (tag->property_rise_set()) {
        attributes.push_back(Pango::Attribute::create_attr_rise (
                               tag->property_rise()));
      }
      if (tag->property_scale_set()) {
        attributes.push_back(Pango::Attribute::create_attr_scale (
                               tag->property_scale()));
      }
      if (tag->property_stretch_set()) {
        attributes.push_back(Pango::Attribute::create_attr_stretch (
                               tag->property_stretch()));
      }
    }
  }

  Glib::RefPtr<Pango::Layout> 
  PrintNotesNoteAddin::create_layout_for_paragraph(const Glib::RefPtr<Gtk::PrintContext> & context, 
                                                   Gtk::TextIter p_start,
                                                   Gtk::TextIter p_end,
                                                   int & indentation)
  {
    Glib::RefPtr<Pango::Layout> layout = context->create_pango_layout();

    layout->set_font_description(
      get_window()->editor()->get_style()->get_font());
    int start_index = p_start.get_line_index();
    indentation = 0;

    {
      Pango::AttrList attr_list;

      Gtk::TextIter segm_start = p_start;
      Gtk::TextIter segm_end;

      double dpiX = context->get_dpi_x();

      while (segm_start.compare (p_end) < 0) {
        segm_end = segm_start;
        std::list<Pango::Attribute> attrs;
        get_paragraph_attributes (layout, dpiX, indentation,
                                  segm_end, p_end, attrs);

        guint si = (guint) (segm_start.get_line_index() - start_index);
        guint ei = (guint) (segm_end.get_line_index() - start_index);

        for(std::list<Pango::Attribute>::iterator iter = attrs.begin();
            iter != attrs.end(); ++iter) {
          
          Pango::Attribute & a(*iter);
          a.set_start_index(si);
          a.set_end_index(ei);
          attr_list.insert(a);
        }
        segm_start = segm_end;
      }

      layout->set_attributes(attr_list);
    }

    layout->set_width(pango_units_from_double((int)context->get_width() -
                                              m_margin_left - m_margin_right - indentation));
    layout->set_wrap (Pango::WRAP_WORD_CHAR);
    layout->set_text (get_buffer()->get_slice (p_start, p_end, false));
    return layout;
  }


  Glib::RefPtr<Pango::Layout> 
  PrintNotesNoteAddin::create_layout_for_pagenumbers(const Glib::RefPtr<Gtk::PrintContext> & context, 
                                int page_number, int total_pages)
  {
    Glib::RefPtr<Pango::Layout> layout = context->create_pango_layout();
    Pango::FontDescription font_desc = get_window()->editor()->get_style()->get_font();
    font_desc.set_style(Pango::STYLE_NORMAL);
    font_desc.set_weight(Pango::WEIGHT_LIGHT);
    layout->set_font_description(font_desc);
    layout->set_width(pango_units_from_double((int)context->get_width()));

    // %1% is the page number, %2% is the total number of pages
    std::string footer_left = str(boost::format(_("Page %1% of %2%"))
                                  % page_number % total_pages);
    layout->set_alignment(Pango::ALIGN_LEFT);
    layout->set_text (footer_left);

    return layout;
  }
  

  Glib::RefPtr<Pango::Layout> 
  PrintNotesNoteAddin::create_layout_for_timestamp(const Glib::RefPtr<Gtk::PrintContext> & context)
  {
    std::string timestamp = sharp::DateTime::now().to_string ("%c");

    Glib::RefPtr<Pango::Layout> layout = context->create_pango_layout ();
    Pango::FontDescription font_desc = get_window()->editor()->get_style()->get_font();
    font_desc.set_style(Pango::STYLE_NORMAL);
    font_desc.set_weight(Pango::WEIGHT_LIGHT);
    layout->set_font_description(font_desc);
    layout->set_width(pango_units_from_double((int) context->get_width()));

    layout->set_alignment(Pango::ALIGN_RIGHT);
    layout->set_text (timestamp);

    return layout;
  }

  int PrintNotesNoteAddin::compute_footer_height(const Glib::RefPtr<Gtk::PrintContext> & context)
  {
    Glib::RefPtr<Pango::Layout> layout = create_layout_for_timestamp(context);
    Pango::Rectangle ink_rect;
    Pango::Rectangle logical_rect;
    layout->get_extents(ink_rect, logical_rect);
    return pango_units_to_double(ink_rect.get_height()) 
      + cm_to_pixel(0.5, context->get_dpi_y());
  }


  void PrintNotesNoteAddin::on_begin_print(const Glib::RefPtr<Gtk::PrintContext>& context)
  {
    m_timestamp_footer = create_layout_for_timestamp(context);
    // Create and initialize the page margins
    m_margin_top = cm_to_pixel (1.5, context->get_dpi_y());
    m_margin_left = cm_to_pixel (1, context->get_dpi_x());
    m_margin_right = cm_to_pixel (1, context->get_dpi_x());
    m_margin_bottom = 0;
    double max_height = pango_units_from_double(context->get_height()
                                                - m_margin_top - m_margin_bottom
                                                - compute_footer_height(context));

    DBG_OUT("margins = %d %d %d %d", m_margin_top, m_margin_left,
            m_margin_right, m_margin_bottom);

    m_page_breaks.clear();

    Gtk::TextIter position;
    Gtk::TextIter end_iter;
    get_buffer()->get_bounds (position, end_iter);

    double page_height = 0;
    bool done = position.compare (end_iter) >= 0;
    while (!done) {
      Gtk::TextIter line_end = position;
      if (!line_end.ends_line ()) {
        line_end.forward_to_line_end ();
      }

      int paragraph_number = position.get_line();
      int indentation = 0;
      Glib::RefPtr<Pango::Layout> layout = create_layout_for_paragraph(
        context, position, line_end, indentation);

      Pango::Rectangle ink_rect;
      Pango::Rectangle logical_rect;
      for(int line_in_paragraph = 0;  line_in_paragraph < layout->get_line_count();
          line_in_paragraph++) {
        Glib::RefPtr<Pango::LayoutLine> line = layout->get_line(line_in_paragraph);
        line->get_extents (ink_rect, logical_rect);

        if ((page_height + logical_rect.get_height()) >= max_height) {
          PageBreak(paragraph_number, line_in_paragraph);
          m_page_breaks.push_back (PageBreak(paragraph_number, line_in_paragraph));
          page_height = 0;
        }

        page_height += logical_rect.get_height();

      }
      position.forward_line ();
      done = position.compare (end_iter) >= 0;
    }

    m_print_op->set_n_pages(m_page_breaks.size() + 1);
  }



  void PrintNotesNoteAddin::on_draw_page(const Glib::RefPtr<Gtk::PrintContext>& context, guint page_nr)
  {
    Cairo::RefPtr<Cairo::Context> cr = context->get_cairo_context();
    cr->move_to (m_margin_left, m_margin_top);

    PageBreak start;
    if (page_nr != 0) {
      start = m_page_breaks [page_nr - 1];
    }

    PageBreak end(-1, -1);
    if (m_page_breaks.size() > page_nr) {
      end = m_page_breaks [page_nr];
    }

    Gtk::TextIter position;
    Gtk::TextIter end_iter;
    get_buffer()->get_bounds (position, end_iter);

    // Fast-forward to the starting line
    while (position.get_line() < start.get_paragraph()) {
      position.forward_line ();
    }

    bool done = position.compare (end_iter) >= 0;
    while (!done) {
      Gtk::TextIter line_end = position;
      if (!line_end.ends_line ()) {
        line_end.forward_to_line_end ();
      }


      int paragraph_number = position.get_line();
      int indentation;

      {
        Glib::RefPtr<Pango::Layout> layout =
          create_layout_for_paragraph (context,position, line_end, indentation);

        for(int line_number = 0;
            line_number < layout->get_line_count() && !done;
            line_number++) {
          // Skip the lines up to the starting line in the
          // first paragraph on this page
          if ((paragraph_number == start.get_paragraph()) &&
              (line_number < start.get_line())) {
            continue;
          }
          // Break as soon as we hit the end line
          if ((paragraph_number == end.get_paragraph()) &&
              (line_number == end.get_line())) {
            done = true;
            break;
          }



          Glib::RefPtr<Pango::LayoutLine> line = layout->get_line(line_number);
          
          Pango::Rectangle ink_rect;
          Pango::Rectangle logical_rect;
          line->get_extents (ink_rect, logical_rect);

          double curX, curY;
          cr->get_current_point(curX, curY);
          cr->move_to (m_margin_left + indentation, curY);
          int line_height = pango_units_to_double(logical_rect.get_height());

          double x, y;
          x = m_margin_left + indentation;
          cr->get_current_point(curX, curY);
          y = curY + line_height;
          pango_cairo_show_layout_line(cr->cobj(), line->gobj());
          cr->move_to(x, y);
        }
      }

      position.forward_line ();
      done = done || (position.compare (end_iter) >= 0);
    }

    // Print the footer
    int total_height = context->get_height();
    int total_width = context->get_width();
    int footer_height = 0;

    double footer_anchor_x, footer_anchor_y;

    {
      Glib::RefPtr<Pango::Layout> pages_footer 
        = create_layout_for_pagenumbers (context, page_nr + 1, 
                                         m_page_breaks.size() + 1);

      Pango::Rectangle ink_footer_rect;
      Pango::Rectangle logical_footer_rect;
      pages_footer->get_extents(ink_footer_rect, logical_footer_rect);
      
      footer_anchor_x = cm_to_pixel(0.5, context->get_dpi_x());
      footer_anchor_y = total_height - m_margin_bottom;
      footer_height = pango_units_to_double(logical_footer_rect.get_height());
      
      cr->move_to(total_width - pango_units_to_double(logical_footer_rect.get_width()) - cm_to_pixel(0.5, context->get_dpi_x()), footer_anchor_y);
                                                      
      pango_cairo_show_layout_line(cr->cobj(), 
                                   (pages_footer->get_line(0))->gobj());

    }

    cr->move_to(footer_anchor_x, footer_anchor_y);
    pango_cairo_show_layout_line(cr->cobj(), 
                                 (m_timestamp_footer->get_line(0))->gobj());

    cr->move_to(cm_to_pixel(0.5, context->get_dpi_x()), 
                total_height - m_margin_bottom - footer_height);
    cr->line_to(total_width - cm_to_pixel(0.5, context->get_dpi_x()),
                total_height - m_margin_bottom - footer_height);
    cr->stroke();
  }


  void PrintNotesNoteAddin::on_end_print(const Glib::RefPtr<Gtk::PrintContext>&)
  {
    m_page_breaks.clear ();
    // clear the RefPtr<>
    m_timestamp_footer.clear();
  }

}
