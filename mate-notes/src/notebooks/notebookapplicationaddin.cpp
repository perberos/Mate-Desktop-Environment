/*
 * gnote
 *
 * Copyright (C) 2011 Aurimas Cernius
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



#include <boost/bind.hpp>

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm/stock.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/uimanager.h>

#include "sharp/string.hpp"
#include "notebooks/notebookapplicationaddin.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/notebooknewnotemenuitem.hpp"
#include "notebooks/notebook.hpp"
#include "actionmanager.hpp"
#include "debug.hpp"
#include "gnote.hpp"
#include "notemanager.hpp"

namespace gnote {
  namespace notebooks {

    ApplicationAddin * NotebookApplicationAddin::create()
    {
      return new NotebookApplicationAddin();
    }


    NotebookApplicationAddin::NotebookApplicationAddin()
      : m_initialized(false)
      , m_notebookUi(0)
    {
      m_notebookIcon = utils::get_icon ("notebook", 16);
      m_newNotebookIcon = utils::get_icon ("notebook-new", 16);
    }



    static const char * uixml = "          <ui>"
          "  <menubar name='MainWindowMenubar'>"
          "    <menu name='FileMenu' action='FileMenuAction'>"
          "      <placeholder name='FileMenuNewNotePlaceholder'>"
          "        <menuitem name='NewNotebookMenu' action='NewNotebookMenuAction' />"
          "      </placeholder>"
          "    </menu>"
          "    <menu name='EditMenu' action='EditMenuAction'>"
          "      <placeholder name='EditMenuDeletePlaceholder'>"
          "          <menuitem name='DeleteNotebook' action='DeleteNotebookAction' position='bottom'/>"
          "      </placeholder>"
          "    </menu>"
          "  </menubar>"
          "  <popup name='NotebooksTreeContextMenu' action='NotebooksTreeContextMenuAction'>"
          "    <menuitem name='NewNotebookNote' action='NewNotebookNoteAction' />"
          "    <separator />"
          "    <menuitem name='OpenNotebookTemplateNote' action='OpenNotebookTemplateNoteAction' />"
          "    <menuitem name='DeleteNotebook' action='DeleteNotebookAction' />"
          "  </popup>"
          "  <popup name='NotebooksTreeNoRowContextMenu' action='NotebooksTreeNoRowContextMenuAction'>"
          "    <menuitem name='NewNotebook' action='NewNotebookAction' />"
          "  </popup>"
          "  <popup name='TrayIconMenu' action='TrayIconMenuAction'>"
          "    <placeholder name='TrayNewNotePlaceholder'>"
          "      <menuitem name='TrayNewNotebookMenu' action='TrayNewNotebookMenuAction' position='top' />"
          "    </placeholder>"
          "  </popup>"
          "</ui>";

    void NotebookApplicationAddin::initialize ()
    {
      m_actionGroup = Glib::RefPtr<Gtk::ActionGroup>(Gtk::ActionGroup::create("Notebooks"));
      m_actionGroup->add(
        Gtk::Action::create ("NewNotebookMenuAction", Gtk::Stock::NEW,
                             _("Note_books"),
                             _("Create a new note in a notebook")));
        
      m_actionGroup->add(     
        Gtk::Action::create ("NewNotebookAction", Gtk::Stock::NEW,
                             _("New Note_book..."),
                             _("Create a new notebook")));
          
      m_actionGroup->add(     
        Gtk::Action::create ("NewNotebookNoteAction", Gtk::Stock::NEW,
                             _("_New Note"),
                             _("Create a new note in this notebook")));
      
      m_actionGroup->add(  
        Gtk::Action::create ("OpenNotebookTemplateNoteAction", Gtk::Stock::OPEN,
                             _("_Open Template Note"),
                             _("Open this notebook's template note")));
          
      m_actionGroup->add(  
        Gtk::Action::create ("DeleteNotebookAction", Gtk::Stock::DELETE,
                             _("Delete Note_book"),
                             _("Delete the selected notebook")));
          
      m_actionGroup->add(  
        Gtk::Action::create ("TrayNewNotebookMenuAction", Gtk::Stock::NEW,
                             _("Notebooks"),
                             _("Create a new note in a notebook")));
          
      ActionManager & am(ActionManager::obj());
      m_notebookUi = am.get_ui()->add_ui_from_string (uixml);
      
      am.get_ui()->insert_action_group (m_actionGroup, 0);
      
      Gtk::MenuItem *item = dynamic_cast<Gtk::MenuItem*>(
        am.get_widget ("/TrayIconMenu/TrayNewNotePlaceholder/TrayNewNotebookMenu"));
      if (item) {
        Gtk::ImageMenuItem *image_item = dynamic_cast<Gtk::ImageMenuItem*>(item);
        if (image_item) {
          image_item->set_image(*manage(new Gtk::Image(m_notebookIcon)));
        }
        m_trayNotebookMenu = manage(new Gtk::Menu());
        item->set_submenu(*m_trayNotebookMenu);
        
        m_trayNotebookMenu->signal_show()
          .connect(sigc::mem_fun(*this, 
                                 &NotebookApplicationAddin::on_tray_notebook_menu_shown));
        m_trayNotebookMenu->signal_hide()
          .connect(sigc::mem_fun(*this, 
                                 &NotebookApplicationAddin::on_tray_notebook_menu_hidden));
      }
      
      Gtk::ImageMenuItem *imageitem = dynamic_cast<Gtk::ImageMenuItem*>(
        am.get_widget ("/MainWindowMenubar/FileMenu/FileMenuNewNotePlaceholder/NewNotebookMenu"));
      if (imageitem) {
        imageitem->set_image(*manage(new Gtk::Image(m_notebookIcon)));
        m_mainWindowNotebookMenu = manage(new Gtk::Menu ());
        imageitem->set_submenu(*m_mainWindowNotebookMenu);

        m_mainWindowNotebookMenu->signal_show()
          .connect(sigc::mem_fun(*this, 
                                 &NotebookApplicationAddin::on_new_notebook_menu_shown));

        m_mainWindowNotebookMenu->signal_hide()
          .connect(sigc::mem_fun(*this, 
                                 &NotebookApplicationAddin::on_new_notebook_menu_hidden));
      }
      imageitem = dynamic_cast<Gtk::ImageMenuItem*>(
        am.get_widget ("/NotebooksTreeContextMenu/NewNotebookNote"));
      if (imageitem) {
        imageitem->set_image(*manage(new Gtk::Image(am.get_new_note())));
      }

      NoteManager & nm(Gnote::obj().default_note_manager());
      
      for(Note::List::const_iterator iter = nm.get_notes().begin();
          iter != nm.get_notes().end(); ++iter) {
        const Note::Ptr & note(*iter);
        note->signal_tag_added().connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_added));
        note->signal_tag_removed().connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_removed));
      }
       
      nm.signal_note_added.connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_note_added));
      nm.signal_note_deleted.connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_note_deleted));
        
      m_initialized = true;
    }


    void NotebookApplicationAddin::shutdown ()
    {
      ActionManager & am(ActionManager::obj());
      try {
        am.get_ui()->remove_action_group(m_actionGroup);
      } 
      catch (...)
      {
      }
      try {
        am.get_ui()->remove_ui(m_notebookUi);
      } 
      catch (...)
      {
      }
      m_notebookUi = 0;
      
      if (m_trayNotebookMenu) {
        delete m_trayNotebookMenu;
      }
      
      if (m_mainWindowNotebookMenu) {
        delete m_mainWindowNotebookMenu;
      }

      m_initialized = false;
    }
    
    bool NotebookApplicationAddin::initialized ()
    {
      return m_initialized;
    }

    void NotebookApplicationAddin::on_tray_notebook_menu_shown()
    {
      add_menu_items(m_trayNotebookMenu, m_trayNotebookMenuItems);
    }

    void NotebookApplicationAddin::on_tray_notebook_menu_hidden()
    {
      remove_menu_items(m_trayNotebookMenu, m_trayNotebookMenuItems);
    }


    void NotebookApplicationAddin::on_new_notebook_menu_shown()
    {
      add_menu_items(m_mainWindowNotebookMenu, m_mainWindowNotebookMenuItems);
    }


    void NotebookApplicationAddin::on_new_notebook_menu_hidden()
    {
      remove_menu_items(m_mainWindowNotebookMenu, m_mainWindowNotebookMenuItems);
    }


    void NotebookApplicationAddin::add_menu_items(Gtk::Menu * menu,   
                                                  std::list<Gtk::MenuItem*> & menu_items)
    {
      remove_menu_items (menu, menu_items);      

      NotebookNewNoteMenuItem *item;

      Glib::RefPtr<Gtk::TreeModel> model = NotebookManager::instance().get_notebooks();
      Gtk::TreeIter iter;
      
      // Add in the "New Notebook..." menu item
      Gtk::ImageMenuItem *newNotebookMenuItem =
        manage(new Gtk::ImageMenuItem (_("New Note_book..."), true));
      newNotebookMenuItem->set_image(*manage(new Gtk::Image (m_newNotebookIcon)));
      newNotebookMenuItem->signal_activate()
        .connect(sigc::mem_fun(*this, &NotebookApplicationAddin::on_new_notebook_menu_item));
      newNotebookMenuItem->show_all ();
      menu->append (*newNotebookMenuItem);
      menu_items.push_back(newNotebookMenuItem);

      
      if (model->children().size() > 0) {
        Gtk::SeparatorMenuItem *separator = manage(new Gtk::SeparatorMenuItem ());
        separator->show_all ();
        menu->append (*separator);
        menu_items.push_back(separator);
        
        iter = model->children().begin();
        while (iter) {
          Notebook::Ptr notebook;
          iter->get_value(0, notebook);
          item = manage(new NotebookNewNoteMenuItem (notebook));
          item->show_all ();
          menu->append (*item);
          menu_items.push_back(item);
          ++iter;
        }
      }
    }

    void NotebookApplicationAddin::remove_menu_items(Gtk::Menu * menu, 
                                                     std::list<Gtk::MenuItem*> & menu_items)
    {
      for(std::list<Gtk::MenuItem*>::const_iterator iter = menu_items.begin();
          iter != menu_items.end(); ++iter) {
        menu->remove(**iter);
      }
      menu_items.clear();
    }


    void NotebookApplicationAddin::on_new_notebook_menu_item()
    {
      NotebookManager::prompt_create_new_notebook (NULL);
    }


    void NotebookApplicationAddin::on_tag_added(const Note & note, const Tag::Ptr& tag)
    {
      if (NotebookManager::instance().is_adding_notebook()) {
        return;
      }
      
      std::string megaPrefix(Tag::SYSTEM_TAG_PREFIX);
      megaPrefix += Notebook::NOTEBOOK_TAG_PREFIX;
      if (!tag->is_system() || !Glib::str_has_prefix(tag->name(), megaPrefix)) {
        return;
      }
      
      std::string notebookName =
        sharp::string_substring(tag->name(), megaPrefix.size());
      
      Notebook::Ptr notebook =
        NotebookManager::instance().get_or_create_notebook (notebookName);
        
      NotebookManager::instance().signal_note_added_to_notebook() (note, notebook);
    }

    

    void NotebookApplicationAddin::on_tag_removed(const Note::Ptr& note, 
                                                  const std::string& normalizedTagName)
    {
      std::string megaPrefix(Tag::SYSTEM_TAG_PREFIX);
      megaPrefix += Notebook::NOTEBOOK_TAG_PREFIX;

      if (!Glib::str_has_prefix(normalizedTagName, megaPrefix)) {
        return;
      }
      
      std::string normalizedNotebookName =
        sharp::string_substring(normalizedTagName, megaPrefix.size());
      
      Notebook::Ptr notebook =
        NotebookManager::instance().get_notebook (normalizedNotebookName);
      if (!notebook) {
        return;
      }
      
      NotebookManager::instance().signal_note_removed_from_notebook() (*note, notebook);
    }

    void NotebookApplicationAddin::on_note_added(const Note::Ptr & note)
    {
        note->signal_tag_added().connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_added));
        note->signal_tag_removed().connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_removed));
    }


    void NotebookApplicationAddin::on_note_deleted(const Note::Ptr &)
    {
      // remove the signal to the note...
    }


  }
}
