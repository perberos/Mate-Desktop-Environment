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






#ifndef __PRINTNOTES_NOTEADDIN_HPP_
#define __PRINTNOTES_NOTEADDIN_HPP_

#include <vector>

#include <gtkmm/menuitem.h>
#include <pangomm/layout.h>

#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"

namespace printnotes {


class PrintNotesModule
  : public sharp::DynamicModule
{
public:
  PrintNotesModule();
  virtual const char * id() const;
  virtual const char * name() const;
  virtual const char * description() const;
  virtual const char * authors() const;
  virtual int          category() const;
  virtual const char * version() const;
};


DECLARE_MODULE(PrintNotesModule);

class PageBreak
{
public:
  PageBreak(int paragraph, int line)
    : m_break_paragraph(paragraph)
    , m_break_line(line)
    {
    }
  PageBreak()
    : m_break_paragraph(0)
    , m_break_line(0)
    {
    }
  int get_paragraph() const
    {
      return m_break_paragraph;
    }
  int get_line() const
    {
      return m_break_line;
    }
private:
  int m_break_paragraph;
  int m_break_line;
};


class PrintNotesNoteAddin
  : public gnote::NoteAddin
{
public:
  static PrintNotesNoteAddin* create()
    {
      return new PrintNotesNoteAddin;
    }
  virtual void initialize();
  virtual void shutdown();
  virtual void on_note_opened();

  static int cm_to_pixel(double cm, double dpi)
		{
			return (int) (cm * dpi / 2.54);
		}
  static int inch_to_pixel(double inch, double dpi)
		{
			return (int) (inch * dpi);
		}

private:
  void get_paragraph_attributes(const Glib::RefPtr<Pango::Layout> & layout,
                                double dpiX, int & indentation,
                                Gtk::TextIter & position, 
                                const Gtk::TextIter & limit,
                                std::list<Pango::Attribute> & attributes);
  Glib::RefPtr<Pango::Layout> create_layout_for_paragraph(const Glib::RefPtr<Gtk::PrintContext> & context, 
                                                          Gtk::TextIter p_start,
                                                          Gtk::TextIter p_end,
                                                          int & indentation);
  Glib::RefPtr<Pango::Layout> create_layout_for_pagenumbers(const Glib::RefPtr<Gtk::PrintContext> & context, int page_number, int total_pages);
  Glib::RefPtr<Pango::Layout> create_layout_for_timestamp(const Glib::RefPtr<Gtk::PrintContext> & context);
  int compute_footer_height(const Glib::RefPtr<Gtk::PrintContext> & context);
  void on_begin_print(const Glib::RefPtr<Gtk::PrintContext>&);
  void print_footer(const Glib::RefPtr<Gtk::PrintContext>&, guint);

  void on_draw_page(const Glib::RefPtr<Gtk::PrintContext>&, guint);
  void on_end_print(const Glib::RefPtr<Gtk::PrintContext>&);
/////
  void print_button_clicked();
  Gtk::ImageMenuItem * m_item;
  int                  m_margin_top;
  int                  m_margin_left;
  int                  m_margin_right;
  int                  m_margin_bottom;
  std::vector<PageBreak> m_page_breaks;
  Glib::RefPtr<Gtk::PrintOperation> m_print_op;
  Glib::RefPtr<Pango::Layout> m_timestamp_footer;
};

}

#endif

