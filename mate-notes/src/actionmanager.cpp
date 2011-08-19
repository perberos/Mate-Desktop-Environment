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



/* ported from */
/***************************************************************************
 *  ActionManager.cs
 *
 *  Copyright (C) 2005-2006 Novell, Inc.
 *  Written by Aaron Bockover <aaron@abock.org>
 ****************************************************************************/

/*  THIS FILE IS LICENSED UNDER THE MIT LICENSE AS OUTLINED IMMEDIATELY BELOW:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/i18n.h>
#include <gtkmm/window.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include <libxml/tree.h>

#include "sharp/string.hpp"
#include "sharp/xml.hpp"
#include "debug.hpp"
#include "actionmanager.hpp"
#include "utils.hpp"

namespace gnote {

  ActionManager::ActionManager()
    : m_ui(Gtk::UIManager::create())
    , m_main_window_actions(Gtk::ActionGroup::create("MainWindow"))
  {
    populate_action_groups();
    m_newNote = utils::get_icon("note-new", 16);
    DBG_ASSERT(m_newNote, "note-new icon not found");
  }


  void ActionManager::load_interface()
  {
    Gtk::UIManager::ui_merge_id id = m_ui->add_ui_from_file(DATADIR"/gnote/UIManagerLayout.xml");
    DBG_ASSERT(id, "merge failed");
    Gtk::Window::set_default_icon_name("gnote");


    Gtk::ImageMenuItem *imageitem = (Gtk::ImageMenuItem*)m_ui->get_widget(
      "/MainWindowMenubar/FileMenu/FileMenuNewNotePlaceholder/NewNote");
    DBG_ASSERT(imageitem, "Item not found");
    if (imageitem) {
      imageitem->set_image(*manage(new Gtk::Image(m_newNote)));
    }
      
    imageitem = (Gtk::ImageMenuItem*)m_ui->get_widget (
      "/TrayIconMenu/TrayNewNotePlaceholder/TrayNewNote");
    DBG_ASSERT(imageitem, "Item not found");
    if (imageitem) {
      imageitem->set_image(*manage(new Gtk::Image(m_newNote)));
    }
  }


  /// <summary>
  /// Get all widgets represents by XML elements that are children
  /// of the placeholder element specified by path.
  /// </summary>
  /// <param name="path">
  /// A <see cref="System.String"/> representing the path to
  /// the placeholder of interest.
  /// </param>
  /// <returns>
  /// A <see cref="IList`1"/> of Gtk.Widget objects corresponding
  /// to the XML child elements of the placeholder element.
  /// </returns>
  void ActionManager::get_placeholder_children(const std::string & path, 
                                               std::list<Gtk::Widget*> & children) const
  {
    // Wrap the UIManager XML in a root element
    // so that it's real parseable XML.
    std::string xml = "<root>" + m_ui->get_ui() + "</root>";

    xmlDocPtr doc = xmlParseDoc((const xmlChar*)xml.c_str());
    if(doc == NULL) {
      return;
    }
        
    // Get the element name
    std::string placeholderName = sharp::string_substring(path, sharp::string_last_index_of(
                                                            path, "/") + 1);
    DBG_OUT("path = %s placeholdername = %s", path.c_str(), placeholderName.c_str());

    sharp::XmlNodeSet nodes = sharp::xml_node_xpath_find(xmlDocGetRootElement(doc), 
                                                         "//placeholder");
    // Find the placeholder specified in the path
    for(sharp::XmlNodeSet::const_iterator iter = nodes.begin();
        iter != nodes.end(); ++iter) {
      xmlNodePtr placeholderNode = *iter;

      if (placeholderNode->type == XML_ELEMENT_NODE) {

        xmlChar * prop = xmlGetProp(placeholderNode, (const xmlChar*)"name");
        if(!prop) {
          continue;
        }
        if(xmlStrEqual(prop, (xmlChar*)placeholderName.c_str())) {

          // Return each child element's widget
          for(xmlNodePtr widgetNode = placeholderNode->children;
              widgetNode; widgetNode = widgetNode->next) {

            if(widgetNode->type == XML_ELEMENT_NODE) {

              xmlChar * widgetName = xmlGetProp(widgetNode, (const xmlChar*)"name");
              if(widgetName) {
                children.push_back(get_widget(path + "/"
                                              + (const char*)widgetName));
                xmlFree(widgetName);
              }
            }
          }
        }
        xmlFree(prop);
      }
    }
    xmlFreeDoc(doc);
  }


  void ActionManager::populate_action_groups()
  {
    Glib::RefPtr<Gtk::Action> action;
    action = Gtk::Action::create("FileMenuAction", _("_File"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create("NewNoteAction", 
                                 Gtk::Stock::NEW, _("_New"),
                                 _("Create a new note"));
    m_main_window_actions->add(action, Gtk::AccelKey("<Control>N"));

    action = Gtk::Action::create(
      "OpenNoteAction", Gtk::Stock::OPEN,
      _("_Open..."),_("Open the selected note"));
    action->set_sensitive(false);
    m_main_window_actions->add(action, Gtk::AccelKey( "<Control>O"));

    action = Gtk::Action::create(
      "DeleteNoteAction", Gtk::Stock::DELETE,
      _("_Delete"),  _("Delete the selected note"));
    action->set_sensitive(false);
    m_main_window_actions->add(action, Gtk::AccelKey("Delete"));

    action = Gtk::Action::create(
      "CloseWindowAction", Gtk::Stock::CLOSE,
      _("_Close"), _("Close this window"));
    m_main_window_actions->add(action, Gtk::AccelKey("<Control>W"));

    action = Gtk::Action::create(
      "QuitGNoteAction", Gtk::Stock::QUIT,
      _("_Quit"), _("Quit Gnote"));
    m_main_window_actions->add(action, Gtk::AccelKey("<Control>Q"));

    action = Gtk::Action::create("EditMenuAction", _("_Edit"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create(
      "ShowPreferencesAction", Gtk::Stock::PREFERENCES,
      _("_Preferences"), _("Gnote Preferences"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create("HelpMenuAction", _("_Help"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create("ShowHelpAction", Gtk::Stock::HELP,
      _("_Contents"), _("Gnote Help"));
    m_main_window_actions->add(action, Gtk::AccelKey("F1"));

    action = Gtk::Action::create(
      "ShowAboutAction", Gtk::Stock::ABOUT,
      _("_About"), _("About Gnote"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create(
      "TrayIconMenuAction",  _("TrayIcon"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create(
      "TrayNewNoteAction", Gtk::Stock::NEW,
      _("Create _New Note"), _("Create a new note"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create(
      "ShowSearchAllNotesAction", Gtk::Stock::FIND,
      _("_Search All Notes"),  _("Open the Search All Notes window"));
    m_main_window_actions->add(action);

    action = Gtk::Action::create(
      "NoteSynchronizationAction",
      _("S_ynchronize Notes"), _("Start synchronizing notes"));
    m_main_window_actions->add(action);    

    m_ui->insert_action_group(m_main_window_actions);
  }

  Glib::RefPtr<Gtk::Action> ActionManager::find_action_by_name(const std::string & n) const
  {
    Glib::ListHandle<Glib::RefPtr<Gtk::ActionGroup> > actiongroups = m_ui->get_action_groups();
    for(Glib::ListHandle<Glib::RefPtr<Gtk::ActionGroup> >::const_iterator iter(actiongroups.begin()); 
        iter != actiongroups.end(); ++iter) {
      Glib::ListHandle<Glib::RefPtr<Gtk::Action> > actions = (*iter)->get_actions();
      for(Glib::ListHandle<Glib::RefPtr<Gtk::Action> >::const_iterator iter2(actions.begin()); 
          iter2 != actions.end(); ++iter2) {
        if((*iter2)->get_name() == n) {
          return *iter2;
        }
      }
    }
    DBG_OUT("%s not found", n.c_str());
    return Glib::RefPtr<Gtk::Action>();      
  }

}
