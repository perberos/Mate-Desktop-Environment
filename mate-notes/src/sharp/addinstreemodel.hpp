/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
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




#ifndef __SHARP_ADDINSTREEMODEL_HPP_
#define __SHARP_ADDINSTREEMODEL_HPP_

#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>

#include "sharp/dynamicmodule.hpp"

namespace sharp {

class AddinsTreeModel
  : public Gtk::TreeStore
{
public:
  typedef Glib::RefPtr<AddinsTreeModel> Ptr;
  static Ptr create(Gtk::TreeView * treeview);

  sharp::DynamicModule * get_module(const Gtk::TreeIter &);

  Gtk::TreeIter append(const sharp::DynamicModule *);
  class AddinsColumns
    : public Gtk::TreeModelColumnRecord
  {
  public:
    AddinsColumns()
      {
        add(name);
        add(version);
        add(addin);
        add(category);
      }

    Gtk::TreeModelColumn<std::string>          name;
    Gtk::TreeModelColumn<std::string>          version;
    Gtk::TreeModelColumn<const sharp::DynamicModule *> addin;
    Gtk::TreeModelColumn<int>                  category;
  };
  AddinsColumns m_columns;

  static std::string get_addin_category_name(int category);
  static int ensure_valid_addin_category(int category);
protected:
  AddinsTreeModel();
  void set_columns(Gtk::TreeView *v);
private:
  void name_cell_data_func(Gtk::CellRenderer * renderer, const Gtk::TreeIter & iter);
  void name_pixbuf_cell_data_func(Gtk::CellRenderer * renderer, const Gtk::TreeIter & iter);
};

}


#endif
