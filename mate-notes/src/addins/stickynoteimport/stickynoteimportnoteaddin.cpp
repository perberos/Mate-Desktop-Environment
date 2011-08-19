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

#include <boost/format.hpp>

#include <glibmm/i18n.h>
#include <glibmm/keyfile.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include "base/inifile.hpp"
#include "stickynoteimportnoteaddin.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"
#include "sharp/xml.hpp"
#include "addinmanager.hpp"
#include "debug.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
#include "utils.hpp"

namespace stickynote {

  using gnote::Preferences;
  using gnote::Note;


StickyNoteImportModule::StickyNoteImportModule()
{
  ADD_INTERFACE_IMPL(StickyNoteImportNoteAddin);
}
const char * StickyNoteImportModule::id() const
{
  return "StickyNoteImportAddin";
}
const char * StickyNoteImportModule::name() const
{
  return _("Sticky Notes Importer");
}
const char * StickyNoteImportModule::description() const
{
  return _("Import your notes from the Sticky Notes applet.");
}
const char * StickyNoteImportModule::authors() const
{
  return _("Hubert Figuiere and the Tomboy Project");
}
int StickyNoteImportModule::category() const
{
  return gnote::ADDIN_CATEGORY_TOOLS;
}
const char * StickyNoteImportModule::version() const
{
  return "0.1";
}

static const char * STICKY_XML_REL_PATH = "/.mate2/stickynotes_applet";
static const char * STICKY_NOTE_QUERY = "//note";
static const char * DEBUG_NO_STICKY_FILE = "StickyNoteImporter: Sticky Notes XML file does not exist or is invalid!";
static const char * DEBUG_CREATE_ERROR_BASE = "StickyNoteImporter: Error while trying to create note \"%s\": %s";
static const char * DEBUG_FIRST_RUN_DETECTED = "StickyNoteImporter: Detecting that importer has never been run...";
//static const char * DEBUG_MATECONF_SET_ERROR_BASE = "StickyNoteImporter: Error setting initial MateConf first run key value: %s";

static const char * PREFS_FILE = "stickynoteimport.ini";

const char * TB_STICKYNOTEIMPORTER_FIRST_RUN =
  "/apps/tomboy/sticky_note_importer/sticky_importer_first_run";

bool StickyNoteImportNoteAddin::s_static_inited = false;
bool StickyNoteImportNoteAddin::s_sticky_file_might_exist = true;
bool StickyNoteImportNoteAddin::s_sticky_file_existence_confirmed = false;
std::string StickyNoteImportNoteAddin::s_sticky_xml_path;


void StickyNoteImportNoteAddin::_init_static()
{
  if(!s_static_inited) {
    s_sticky_xml_path = Glib::get_home_dir() + STICKY_XML_REL_PATH;
    s_static_inited = true;
  }
}

void StickyNoteImportNoteAddin::initialize()
{
	// Don't add item to tools menu if Sticky Notes XML file does not
  // exist. Only check for the file once, since Initialize is called
  // for each note when Gnote starts up.
  if (s_sticky_file_might_exist) {
    if (s_sticky_file_existence_confirmed || sharp::file_exists (s_sticky_xml_path)) {
#if 0
      m_item = manage(new Gtk::ImageMenuItem (_("Import from Sticky Notes")));
      m_item->set_image(*manage(new Gtk::Image (Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU)));
      m_item->signal_activate().connect(
        sigc::bind(sigc::mem_fun(*this, &StickyNoteImportNoteAddin::import_button_clicked));
      m_item->show ();
      add_plugin_menu_item (m_item);
#endif
      s_sticky_file_existence_confirmed = true;
    } 
    else {
      s_sticky_file_might_exist = false;
      DBG_OUT(DEBUG_NO_STICKY_FILE);
    }
  }
}



void StickyNoteImportNoteAddin::shutdown()
{
}



bool StickyNoteImportNoteAddin::want_to_run(gnote::NoteManager & manager)
{
  bool want_run = false;
  std::string prefs_file =
    Glib::build_filename(manager.get_addin_manager().get_prefs_dir(),
                         PREFS_FILE);
  base::IniFile ini_file(prefs_file);
  ini_file.load();

  if(s_sticky_file_might_exist) {
    want_run = !ini_file.get_bool("status", "first_run");

    if(want_run) {
      // we think we want to run
      // so we check for Tomboy. If Tomboy wants to run then we want

      MateConfClient * client = Preferences::obj().get_client();
      GError * error = NULL;
      gboolean tb_must_run = mateconf_client_get_bool(client,
                                                   TB_STICKYNOTEIMPORTER_FIRST_RUN,
                                                   &error);
      if(error) {
        // the key don't exist. Tomboy has not been installed.
        // we want to run.
        DBG_OUT("mateconf error %s", error->message);
        tb_must_run = true;
        g_error_free(error);
      }
      DBG_OUT("tb_must_run %d", tb_must_run);
      // we decided that if Tomboy don't want to run then SticjyNotes are
      // probably already imported.
      if(!tb_must_run) {
        // Mark like we already ran.
        ini_file.set_bool("status", "first_run", true);
//        Preferences::obj().set<bool>(Preferences::STICKYNOTEIMPORTER_FIRST_RUN, false);
        want_run = false;
      }
    }
  }
  return want_run;
}


bool StickyNoteImportNoteAddin::first_run(gnote::NoteManager & manager)
{
  base::IniFile ini_file(Glib::build_filename(
                           manager.get_addin_manager().get_prefs_dir(), 
                           PREFS_FILE));
  
  ini_file.load();

  bool firstRun = !ini_file.get_bool("status", "first_run");
//Preferences::obj().get<bool> (Preferences::STICKYNOTEIMPORTER_FIRST_RUN);

  if (firstRun) {
    ini_file.set_bool("status", "first_run", true);

//    Preferences::obj().set<bool> (Preferences::STICKYNOTEIMPORTER_FIRST_RUN, false);

    DBG_OUT(DEBUG_FIRST_RUN_DETECTED);

    xmlDocPtr xml_doc = get_sticky_xml_doc();
    if (xml_doc) {
      // Don't show dialog when automatically importing
      import_notes (xml_doc, false, manager);
      xmlFreeDoc(xml_doc);
    }
    else {
      firstRun = false;
    }
  }
  return firstRun;
}


xmlDocPtr StickyNoteImportNoteAddin::get_sticky_xml_doc()
{
  if (sharp::file_exists(s_sticky_xml_path)) {
    xmlDocPtr xml_doc = xmlReadFile(s_sticky_xml_path.c_str(), "UTF-8", 0);
    if(xml_doc == NULL) {
      DBG_OUT(DEBUG_NO_STICKY_FILE);
    }
    return xml_doc;
  }
  else {
    DBG_OUT(DEBUG_NO_STICKY_FILE);
    return NULL;
  }
}


void StickyNoteImportNoteAddin::import_button_clicked(gnote::NoteManager & manager)
{
  xmlDocPtr xml_doc = get_sticky_xml_doc();
  if(xml_doc) {
    import_notes (xml_doc, true, manager);
  }
  else {
    show_no_sticky_xml_dialog(s_sticky_xml_path);
  }
}


void StickyNoteImportNoteAddin::show_no_sticky_xml_dialog(const std::string & xml_path)
{
  show_message_dialog (
    _("No Sticky Notes found"),
    // %1% is a the file name
    str(boost::format(_("No suitable Sticky Notes file was found at \"%1%\"."))
        % xml_path), Gtk::MESSAGE_ERROR);
}


void StickyNoteImportNoteAddin::show_results_dialog(int numNotesImported, int numNotesTotal)
{
	show_message_dialog (
    _("Sticky Notes import completed"),
    // here %1% is the number of notes imported, %2% the total number of notes.
    str(boost::format(_("<b>%1%</b> of <b>%2%</b> Sticky Notes "
                        "were successfully imported.")) 
        % numNotesImported % numNotesTotal), Gtk::MESSAGE_INFO);
}


void StickyNoteImportNoteAddin::import_notes(xmlDocPtr xml_doc,
                                             bool showResultsDialog,
                                             gnote::NoteManager & manager)
{
  xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
  if(!root_node) {
    if (showResultsDialog)
      show_no_sticky_xml_dialog(s_sticky_xml_path);
    return;
  }
  sharp::XmlNodeSet nodes = sharp::xml_node_xpath_find(root_node, STICKY_NOTE_QUERY);

  int numSuccessful = 0;
  const xmlChar * defaultTitle = (const xmlChar *)_("Untitled");

  for(sharp::XmlNodeSet::const_iterator iter = nodes.begin();
      iter != nodes.end(); ++iter) {

    xmlNodePtr node = *iter;
    const xmlChar * stickyTitle;
    xmlChar * titleAttr = xmlGetProp(node, (const xmlChar*)"title");
    if(titleAttr) {
      stickyTitle = titleAttr;
    }
    else {
      stickyTitle = defaultTitle;
    }
    xmlChar * stickyContent = xmlNodeGetContent(node);

    if(stickyContent) {
      if (create_note_from_sticky ((const char*)stickyTitle, (const char*)stickyContent, manager)) {
        numSuccessful++;
      }
      xmlFree(stickyContent);
    }

    if(titleAttr) {
      xmlFree(titleAttr);
    }
  }

  if (showResultsDialog) {
    show_results_dialog (numSuccessful, nodes.size());
  }
}


bool StickyNoteImportNoteAddin::create_note_from_sticky(const char * stickyTitle,
                                                        const char* content,
                                                        gnote::NoteManager & manager)
{
  // There should be no XML in the content
  // TODO: Report the error in the results dialog
  //  (this error should only happen if somebody has messed with the XML file)
  if(strchr(content, '>') || strchr(content, '<')) {
    DBG_OUT(DEBUG_CREATE_ERROR_BASE, stickyTitle,
            "Invalid characters in note XML");
    return false;
  }

  std::string preferredTitle = _("Sticky Note: ");
  preferredTitle += stickyTitle;
  std::string title = preferredTitle;

  int i = 2; // Append numbers to create unique title, starting with 2
  while (manager.find(title)){
    title = str(boost::format("%1% (#%2%)") % preferredTitle % i);
    i++;
  }
  
  std::string noteXml = str(boost::format("<note-content><note-title>%1%</note-title>\n\n"
                                          "%2%</note-content>") % title % content);

  try {
    Note::Ptr newNote = manager.create(title, noteXml);
    newNote->queue_save (Note::NO_CHANGE);
    newNote->save();
    return true;
  } 
  catch (const std::exception & e) {
    DBG_OUT(DEBUG_CREATE_ERROR_BASE, title.c_str(), e.what());
    return false;
  }
}


void StickyNoteImportNoteAddin::show_message_dialog(const std::string & title,
                                                   const std::string & message,
                                                   Gtk::MessageType messageType)
{
  gnote::utils::HIGMessageDialog dialog(NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        messageType,
                                        Gtk::BUTTONS_OK,
                                        title,
                                        message);
  dialog.run();
}

  
}
