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



#include <glibmm/i18n.h>

#include "sharp/addinstreemodel.hpp"
#include "utils.hpp"
#include "abstractaddin.hpp"


namespace sharp {


  AddinsTreeModel::Ptr AddinsTreeModel::create(Gtk::TreeView * treeview)
  {
    AddinsTreeModel::Ptr p(new AddinsTreeModel());
    if(treeview) {
      treeview->set_model(p);
      p->set_columns(treeview);
    }
    return p;
  }

  AddinsTreeModel::AddinsTreeModel()
    : Gtk::TreeStore()
  {
    set_column_types(m_columns);
  }

  sharp::DynamicModule * AddinsTreeModel::get_module(const Gtk::TreeIter & iter)
  {
    sharp::DynamicModule * module = NULL;
    if(iter) {
      iter->get_value(2, module);
    }
    return module;
  }

  void AddinsTreeModel::name_cell_data_func(Gtk::CellRenderer * renderer,
                                            const Gtk::TreeIter & iter)
  {
    Gtk::CellRendererText *text_renderer = dynamic_cast<Gtk::CellRendererText*>(renderer);
    std::string value;
    iter->get_value(0, value);
    text_renderer->property_text() = value;
    const sharp::DynamicModule *module = get_module(iter);
    if(!module || module->is_enabled()) {
      text_renderer->property_foreground() = "black";
    }
    else {
      text_renderer->property_foreground() = "grey";
    }
  }

  void AddinsTreeModel::name_pixbuf_cell_data_func(Gtk::CellRenderer * renderer,
                                                   const Gtk::TreeIter & iter)
  {
    Gtk::CellRendererPixbuf *icon_renderer = dynamic_cast<Gtk::CellRendererPixbuf*>(renderer);
    Glib::RefPtr<Gdk::Pixbuf> icon;
    if(get_module(iter)) {
      icon = gnote::utils::get_icon("emblem-package", 22);
    }
    icon_renderer->property_pixbuf() = icon;
  }

  void AddinsTreeModel::set_columns(Gtk::TreeView *treeview)
  {
    Gtk::TreeViewColumn *column = manage(new Gtk::TreeViewColumn);
    column->set_title(_("Name"));
    column->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    column->set_resizable(false);
    Gtk::CellRendererPixbuf *icon_renderer = manage(new Gtk::CellRendererPixbuf);
    column->pack_start(*icon_renderer, false);
    column->set_cell_data_func(*icon_renderer,
                               sigc::mem_fun(*this, &AddinsTreeModel::name_pixbuf_cell_data_func));
    Gtk::CellRenderer *renderer = manage(new Gtk::CellRendererText);
    column->pack_start(*renderer, true);
    column->set_cell_data_func(*renderer,
                               sigc::mem_fun(*this, &AddinsTreeModel::name_cell_data_func));
    treeview->append_column(*column);
    treeview->append_column(_("Version"), m_columns.version);
  }


  Gtk::TreeIter AddinsTreeModel::append(const sharp::DynamicModule *module)
  {
    int category = module->category();
    Gtk::TreeIter iter = children().begin();
    while(iter != children().end()) {
      int row_value;
      iter->get_value(3, row_value);
      if(row_value == category)
        break;
      else ++iter;
    }
    if(iter == children().end()) {
      iter = Gtk::TreeStore::append();
      category = ensure_valid_addin_category(category);
      iter->set_value(0, get_addin_category_name(category));
      iter->set_value(3, category);
    }
    iter = Gtk::TreeStore::append(iter->children());
    iter->set_value(0, std::string(module->name()));
    iter->set_value(1, std::string(module->version()));
    iter->set_value(2, module);
    return iter;
  }

  std::string AddinsTreeModel::get_addin_category_name(int category)
  {
    switch(category) {
      case gnote::ADDIN_CATEGORY_FORMATTING:
        /* TRANSLATORS: Addin category. */
        return _("Formatting");
      case gnote::ADDIN_CATEGORY_DESKTOP_INTEGRATION:
        /* TRANSLATORS: Addin category. */
        return _("Desktop integration");
      case gnote::ADDIN_CATEGORY_TOOLS:
        /* TRANSLATORS: Addin category. */
        return _("Tools");
      case gnote::ADDIN_CATEGORY_UNKNOWN:
      default:
        /* TRANSLATORS: Addin category is unknown. */
        return _("Unknown");
    }
  }

  int AddinsTreeModel::ensure_valid_addin_category(int category)
  {
    switch(category) {
      case gnote::ADDIN_CATEGORY_FORMATTING:
      case gnote::ADDIN_CATEGORY_DESKTOP_INTEGRATION:
      case gnote::ADDIN_CATEGORY_TOOLS:
        return category;
      case gnote::ADDIN_CATEGORY_UNKNOWN:
      default:
        return gnote::ADDIN_CATEGORY_UNKNOWN;
    }
  }

}
