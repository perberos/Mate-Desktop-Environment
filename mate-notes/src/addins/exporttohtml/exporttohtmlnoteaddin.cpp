/*
 * gnote
 *
 * Copyright (C) 2010-2011 Aurimas Cernius
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

#include <libxml/xmlmemory.h>
#include <libxml/xpathInternals.h>
#include <libxslt/extensions.h>

#include <glibmm/i18n.h>
#include <gtkmm/stock.h>

#include "sharp/exception.hpp"
#include "sharp/files.hpp"
#include "sharp/streamwriter.hpp"
#include "sharp/string.hpp"
#include "sharp/uri.hpp"
#include "sharp/xsltargumentlist.hpp"
#include "debug.hpp"
#include "preferences.hpp"
#include "notewindow.hpp"
#include "utils.hpp"

#include "exporttohtmlnoteaddin.hpp"
#include "exporttohtmldialog.hpp"
#include "notenameresolver.hpp"

#define STYLESHEET_NAME "exporttohtml.xsl"


using gnote::Preferences;

namespace exporttohtml {


ExportToHtmlModule::ExportToHtmlModule()
{
  ADD_INTERFACE_IMPL(ExportToHtmlNoteAddin);
}
const char * ExportToHtmlModule::id() const
{
  return "ExportToHtmlAddin";
}
const char * ExportToHtmlModule::name() const
{
  return _("Export to HTML");
}
const char * ExportToHtmlModule::description() const
{
  return _("Exports individual notes to HTML.");
}
const char * ExportToHtmlModule::authors() const
{
  return _("Hubert Figuiere and the Tomboy Project");
}
int ExportToHtmlModule::category() const
{
  return gnote::ADDIN_CATEGORY_TOOLS;
}
const char * ExportToHtmlModule::version() const
{
  return "0.1";
}

sharp::XslTransform *ExportToHtmlNoteAddin::s_xsl = NULL;


void ExportToHtmlNoteAddin::initialize()
{
  
}


void ExportToHtmlNoteAddin::shutdown()
{
}


void ExportToHtmlNoteAddin::on_note_opened()
{
	Gtk::ImageMenuItem *item =  manage(new Gtk::ImageMenuItem (_("Export to HTML")));
  item->set_image(*manage(new Gtk::Image (Gtk::Stock::SAVE, Gtk::ICON_SIZE_MENU)));
  item->signal_activate().connect(
    sigc::mem_fun(*this, &ExportToHtmlNoteAddin::export_button_clicked));
  item->show ();
  add_plugin_menu_item (item);
}


void ExportToHtmlNoteAddin::export_button_clicked()
{
  ExportToHtmlDialog dialog(get_note()->get_title() + ".html");
  int response = dialog.run();
  std::string output_path = dialog.get_filename();

  if (response != Gtk::RESPONSE_OK) {
    return;
  }

  DBG_OUT("Exporting Note '%s' to '%s'...", get_note()->get_title().c_str(), 
          output_path.c_str());

  sharp::StreamWriter writer;
  std::string error_message;

  try {
    // FIXME: Warn about file existing.  Allow overwrite.
    sharp::file_delete(output_path);

    writer.init(output_path);
    write_html_for_note (writer, get_note(), dialog.get_export_linked(), 
                         dialog.get_export_linked_all());

    // Save the dialog preferences now that the note has
    // successfully been exported
    dialog.save_preferences ();

    try {
      sharp::Uri output_uri(output_path);
      gnote::utils::open_url("file://" + output_uri.get_absolute_uri());
    } 
    catch (const Glib::Exception & ex) {
      ERR_OUT ("Could not open exported note in a web browser: %s",
               ex.what().c_str());

      std::string detail = str(boost::format(
                                 _("Your note was exported to \"%1%\"."))
                               % output_path);

      // Let the user know the note was saved successfully
      // even though showing the note in a web browser failed.
      gnote::utils::HIGMessageDialog msg_dialog(
        get_window(),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK,
        _("Note exported successfully"),
        detail);
      msg_dialog.run();
    }
  } 
#if 0
  catch (UnauthorizedAccessException) {
    error_message = Catalog.GetString ("Access denied.");
  } 
  catch (DirectoryNotFoundException) {
    error_message = Catalog.GetString ("Folder does not exist.");
  } 
#endif
  catch (const sharp::Exception & e) {
    ERR_OUT("Could not export: %s", e.what());

    error_message = e.what();
  } 
  writer.close ();

  if (!error_message.empty())
  {
    ERR_OUT("Could not export: %s", error_message.c_str());

    std::string msg = str(boost::format(
                            _("Could not save the file \"%s\"")) 
                          % output_path.c_str());

    gnote::utils::HIGMessageDialog msg_dialog(&dialog, 
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              Gtk::MESSAGE_ERROR, 
                                              Gtk::BUTTONS_OK,
                                              msg, error_message);
    msg_dialog.run();
  }
}



static void to_lower(xmlXPathParserContextPtr ctxt,
                     int)
{
  const xmlChar *input = xmlXPathPopString(ctxt);
  gchar * lower = g_utf8_strdown((const gchar*)input, -1);
  xmlXPathReturnString(ctxt, xmlStrdup((const xmlChar*)lower));
  g_free(lower);
}


sharp::XslTransform & ExportToHtmlNoteAddin::get_note_xsl()
{
  if(s_xsl == NULL) {
    int result = xsltRegisterExtModuleFunction((const xmlChar *)"ToLower",
                                               (const xmlChar *)"http://beatniksoftware.com/tomboy", 
                                               &to_lower);
    DBG_OUT("xsltRegisterExtModule %d", result);
    if(result == -1) {
      DBG_OUT("xsltRegisterExtModule failed");
    }

    s_xsl = new sharp::XslTransform;
    std::string stylesheet_file = DATADIR "/gnote/" STYLESHEET_NAME;

    if (sharp::file_exists (stylesheet_file)) {
      DBG_OUT("ExportToHTML: Using user-custom %s file.", STYLESHEET_NAME);
      s_xsl->load(stylesheet_file);
    } 
#if 0
    else {
      Stream resource = asm.GetManifestResourceStream (stylesheet_name);
      if (resource != null) {
        XmlTextReader reader = new XmlTextReader (resource);
        s_xsl->load (reader, null, null);
        resource.Close ();
      } 
      else {
        DBG_OUT("Unable to find HTML export template '%s'.", STYLESHEET_NAME);
      }
    }
#endif

  }
  return *s_xsl;
}




void ExportToHtmlNoteAddin::write_html_for_note (sharp::StreamWriter & writer, 
                                                 const gnote::Note::Ptr & note, 
                                                 bool export_linked, 
                                                 bool export_linked_all)
{
  std::string s_writer;
  s_writer = gnote::NoteArchiver::write_string(note->data());
  xmlDocPtr doc = xmlParseMemory(s_writer.c_str(), s_writer.size());

  sharp::XsltArgumentList args;
  args.add_param ("export-linked", "", export_linked);
  args.add_param ("export-linked-all", "", export_linked_all);
  args.add_param ("root-note", "", note->get_title());


  if (Preferences::obj().get<bool>(Preferences::ENABLE_CUSTOM_FONT)) {
    std::string font_face 
      = Preferences::obj().get<std::string>(Preferences::CUSTOM_FONT_FACE);
    Pango::FontDescription font_desc (font_face);
    std::string font = str(boost::format("font-family:'%1%';")
                           % font_desc.get_family());

    args.add_param ("font", "", font);
  }

  NoteNameResolver resolver(note->manager(), note);
  get_note_xsl().transform(doc, args, writer, resolver);

  xmlFreeDoc(doc);
}


}

