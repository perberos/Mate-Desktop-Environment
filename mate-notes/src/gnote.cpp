/*
 * gnote
 *
 * Copyright (C) 2010-2011 Aurimas Cernius
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



#include "config.h"

#include <stdlib.h>

#include <iostream>

#include <boost/format.hpp>

#include <glibmm/thread.h>
#include <glibmm/i18n.h>
#include <glibmm/optionentry.h>
#include <gtkmm/main.h>
#include <gtkmm/aboutdialog.h>

#if HAVE_PANELAPPLETMM
#include <libmatepanelappletmm/init.h>
#endif

#include "gnote.hpp"
#include "actionmanager.hpp"
#include "addinmanager.hpp"
#include "applicationaddin.hpp"
#include "debug.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferencesdialog.hpp"
#include "recentchanges.hpp"
#include "utils.hpp"
#include "xkeybinder.hpp"
#include "sharp/streamreader.hpp"
#include "sharp/files.hpp"

#if HAVE_PANELAPPLETMM
#include "applet.hpp"
#endif

#if ENABLE_DBUS
#include "remotecontrolproxy.hpp"
#include "dbus/remotecontrolclient.hpp"
#endif

namespace gnote {

  bool Gnote::s_tray_icon_showing = false;

  Gnote::Gnote()
    : m_manager(NULL)
    , m_keybinder(NULL)
    , m_is_mate_panel_applet(false)
    , m_prefsdlg(NULL)
  {
  }

  Gnote::~Gnote()
  {
    delete m_prefsdlg;
    delete m_manager;
    delete m_keybinder;
  }


  int Gnote::main(int argc, char **argv)
  {
    GnoteCommandLine cmd_line;

    cmd_line.parse(argc, argv);

    m_is_mate_panel_applet = cmd_line.use_mate_panel_applet();

#ifdef ENABLE_DBUS
    if(cmd_line.needs_execute()) {
      cmd_line.execute();
      return 0;
    }
#endif

    m_icon_theme = Gtk::IconTheme::get_default();
    m_icon_theme->append_search_path(DATADIR"/icons");
    m_icon_theme->append_search_path(DATADIR"/gnote/icons");

    std::string note_path = get_note_path(cmd_line.note_path());
    m_manager = new NoteManager(note_path, sigc::mem_fun(*this, &Gnote::start_note_created));
    m_keybinder = new XKeybinder();

    // TODO
    // SyncManager::init()

    ActionManager & am(ActionManager::obj());
    am.load_interface();
    register_remote_control(*m_manager);
    setup_global_actions();

    m_manager->get_addin_manager().initialize_application_addins();

#ifndef ENABLE_DBUS
    if(cmd_line.needs_execute()) {
      cmd_line.execute();
    }
#endif

    if(m_is_mate_panel_applet) {
      DBG_OUT("starting applet");
      s_tray_icon_showing = true;

      am["CloseWindowAction"]->set_visible(true);
      am["QuitGNoteAction"]->set_visible(false);
      
      // register panel applet factory
#if HAVE_PANELAPPLETMM
      Mate::Panel::init("gnote", VERSION, argc, argv);

      panel::register_applet();
#endif
      return 0;

    }
    else {
      DBG_OUT("starting tray icon");
      //register session manager restart
      start_tray_icon();
    }
    return 0;
  }


  void Gnote::start_note_created(const Note::Ptr & start_note)
  {
    DBG_OUT("we will show the start note: %d", !is_mate_panel_applet());
    if(!is_mate_panel_applet()) {
      start_note->get_window()->show();
    }
  }

  std::string Gnote::get_note_path(const std::string & override_path)
  {
    std::string note_path;
    if(override_path.empty()) {
      const char * s = getenv("GNOTE_PATH");
      note_path = s?s:"";
    }
    else {
      note_path = override_path;
    }
    if(note_path.empty()) {
      note_path = Gnote::data_dir();
    }

    return note_path;
  }

  void Gnote::start_tray_icon()
  {
    // Create the tray icon and run the main loop
    m_tray_icon = Glib::RefPtr<TrayIcon>(new TrayIcon(default_note_manager()));
    m_tray = m_tray_icon->tray();

    // Give the TrayIcon 2 seconds to appear.  If it
    // doesn't by then, open the SearchAllNotes window.
    s_tray_icon_showing = m_tray_icon->is_embedded() 
      && m_tray_icon->get_visible();
      
    if (!s_tray_icon_showing) {
      Glib::RefPtr<Glib::TimeoutSource> timeout 
        = Glib::TimeoutSource::create(2000);
      timeout->connect(sigc::mem_fun(*this, &Gnote::check_tray_icon_showing));
      timeout->attach();
    }
    
    Gtk::Main::run();
  }


  bool Gnote::check_tray_icon_showing()
  {
    s_tray_icon_showing = m_tray_icon->is_embedded() 
      && m_tray_icon->get_visible();
    if(!s_tray_icon_showing) {
      ActionManager & am(ActionManager::obj());
      am["ShowSearchAllNotesAction"]->activate();
    }
    return false;
  }


  void Gnote::register_remote_control(NoteManager & manager)
  {
#if ENABLE_DBUS
    try {
      m_remote_control = RemoteControlProxy::register_remote(manager);
      if (m_remote_control) {
        DBG_OUT("Gnote remote control active.");
      } 
      else {
        // If Gnote is already running, open the search window
        // so the user gets some sort of feedback when they
        // attempt to run Gnote again.
        RemoteControlClient *remote;
        try {
          remote = RemoteControlProxy::get_instance();
          DBG_ASSERT(remote, "remote is NULL, something is wrong");
          if(remote) {
            remote->DisplaySearch();
          }
        } 
        catch (...)
        {
        }

        ERR_OUT ("Gnote is already running.  Exiting...");
        ::exit(-1);
      }
    } 
    catch (const std::exception & e) {
      ERR_OUT("Gnote remote control disabled (DBus exception): %s",
              e.what());
    }
#endif
  }



  void Gnote::setup_global_actions()
  {
    ActionManager & am(ActionManager::obj());
    am["NewNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_new_note_action));
    am["QuitGNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_quit_gnote_action));
    am["ShowPreferencesAction"]->signal_activate() 
      .connect(sigc::mem_fun(*this, &Gnote::on_show_preferences_action));
    am["ShowHelpAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_show_help_action));
    am["ShowAboutAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_show_about_action));
    am["TrayNewNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_new_note_action));
    am["ShowSearchAllNotesAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::open_search_all));
    am["NoteSynchronizationAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::open_note_sync_window));
  }

  void Gnote::on_new_note_action()
  {
    try {
      Note::Ptr new_note = default_note_manager().create();
      new_note->get_window()->show();
    }
    catch(const std::exception & e) 
    {
      utils::HIGMessageDialog dialog (
        NULL,  (GtkDialogFlags)0,
        Gtk::MESSAGE_ERROR,
        Gtk::BUTTONS_OK,
        _("Cannot create new note"),
        e.what());
      dialog.run ();
    }    
  }

  void Gnote::on_quit_gnote_action()
  {
    if(m_is_mate_panel_applet) {
      return;
    }
    Gtk::Main::quit();
  }

  void Gnote::on_preferences_response(int /*res*/)
  {
    delete m_prefsdlg;
    m_prefsdlg = NULL;
  }


  void Gnote::on_show_preferences_action()
  {
    if(!m_prefsdlg) {
      m_prefsdlg = new PreferencesDialog(m_manager->get_addin_manager());
      m_prefsdlg->signal_response().connect(
        sigc::mem_fun(*this, &Gnote::on_preferences_response));
    }
    m_prefsdlg->present();
  }

  void Gnote::on_show_help_action()
  {
    GdkScreen *cscreen = NULL;
    if(m_tray_icon) {
      Gdk::Rectangle area;
      GtkOrientation orientation;
      gtk_status_icon_get_geometry(m_tray_icon->gobj(), &cscreen, area.gobj(), &orientation);
    }
    utils::show_help("gnote", "", cscreen, NULL);
  }

  void Gnote::on_show_about_action()
  {
    std::list<Glib::ustring> authors;
    authors.push_back("Aurimas Černius <aurisc4@gmail.com>");
    authors.push_back("Debarshi Ray <debarshir@src.mate.org>");
    authors.push_back("Hubert Figuiere <hub@figuiere.net>");
    authors.push_back("Iain Nicol <iainn@src.mate.org>");
    authors.push_back(_("and Tomboy original authors."));
    
    std::list<Glib::ustring> documenters;
    documenters.push_back("Alex Graveley <alex@beatniksoftware.com>");
    documenters.push_back("Boyd Timothy <btimothy@gmail.com>");
    documenters.push_back("Brent Smith <mate@nextreality.net>");
    documenters.push_back("Paul Cutler <pcutler@foresightlinux.org>");
    documenters.push_back("Sandy Armstrong <sanfordarmstrong@gmail.com>");

    std::string translators(_("translator-credits"));
    if (translators == "translator-credits")
      translators = "";

    Gtk::AboutDialog about;
    about.set_name("Gnote");
    about.set_version(VERSION);
    about.set_logo(utils::get_icon("gnote", 48));
    about.set_copyright(_("Copyright \xc2\xa9 2010-2011 Aurimas Cernius\n"
                          "Copyright \xc2\xa9 2009-2011 Debarshi Ray\n"
                          "Copyright \xc2\xa9 2009 Hubert Figuiere\n"
                          "Copyright \xc2\xa9 2004-2009 the Tomboy original authors."));
    about.set_comments(_("A simple and easy to use desktop "
                         "note-taking application."));
// I don't think we need a hook.
//      Gtk.AboutDialog.SetUrlHook (delegate (Gtk.AboutDialog dialog, string link) {
//        try {
//          Services.NativeApplication.OpenUrl (link);
//        } catch (Exception e) {
//          GuiUtils.ShowOpeningLocationError (dialog, link, e.Message);
//        }
//      }); 
    about.set_website("http://live.mate.org/Gnote");
    about.set_website_label(_("Homepage"));
    about.set_authors(authors);
    about.set_documenters(documenters);
    about.set_translator_credits(translators);
//      about.set_icon_name("gnote");
    about.run();
  }

  void Gnote::open_search_all()
  {
    NoteRecentChanges::get_instance(default_note_manager())->present();
  }

  void Gnote::open_note_sync_window()
  {
#if 0
    // TODO
    if (sync_dlg == null) {
      sync_dlg = new SyncDialog ();
      sync_dlg.Response += OnSyncDialogResponse;
    }

    sync_dlg.Present 
#endif
  }


  std::string Gnote::cache_dir()
  {
    return Glib::get_user_cache_dir() + "/gnote";
  }


  std::string Gnote::conf_dir()
  {
    return Glib::get_user_config_dir() + "/gnote";
  }


  std::string Gnote::data_dir()
  {
    return Glib::get_user_data_dir() + "/gnote";
  }


  std::string Gnote::old_note_dir()
  {
    std::string home_dir = Glib::get_home_dir();

    if (home_dir.empty())
      home_dir = Glib::get_current_dir();

    return home_dir + "/.gnote";
  }



  GnoteCommandLine::GnoteCommandLine()
    : m_context(g_option_context_new("Foobar"))
    , m_use_panel(false)
    , m_note_path(NULL)
    , m_do_search(false)
    , m_show_version(false)
    , m_do_new_note(false)
    , m_open_note(NULL)
    , m_open_start_here(false)
    , m_highlight_search(NULL)
  {
    static const GOptionEntry entries[] =
      {
        { "mate-panel-applet", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &m_use_panel, _("Run Gnote as a MATE panel applet."), NULL },
        { "note-path", 0, 0, G_OPTION_ARG_STRING, &m_note_path, _("Specify the path of the directory containing the notes."), _("path") },
        { "search", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (void*)GnoteCommandLine::parse_func, _("Open the search all notes window with the search text."), _("text") },
        { "version", 0, 0, G_OPTION_ARG_NONE, &m_show_version, _("Print version information."), NULL },
#ifdef ENABLE_DBUS
        { "new-note", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (void*)GnoteCommandLine::parse_func, _("Create and display a new note, with a optional title."), _("title") },
        { "open-note", 0, 0, G_OPTION_ARG_STRING, &m_open_note, _("Display the existing note matching title."), _("title/url") },
        { "start-here", 0, 0, G_OPTION_ARG_NONE, &m_open_start_here, _("Display the 'Start Here' note."), _("text") },
        { "highlight-search", 0, 0, G_OPTION_ARG_STRING, &m_highlight_search, _("Search and highlight text in the opened note."), _("text") },
#endif        
        { NULL, 0, 0, (GOptionArg)0, NULL, NULL, NULL }
      };

    GOptionGroup *group = g_option_group_new("Gnote", _("A note taking application"), _("Gnote options at launch"), this, NULL);
    g_option_group_add_entries(group, entries);
    g_option_context_set_main_group (m_context, group);

    // we need this for the panel support.
    g_option_context_set_ignore_unknown_options(m_context, TRUE);
  }

  
  GnoteCommandLine::~GnoteCommandLine()
  {
    g_option_context_free(m_context);
  }

  gboolean GnoteCommandLine::parse_func(const gchar *option_name,
                                        const gchar *value,
                                        gpointer data,
                                        GError ** /*error*/)
  {
    GnoteCommandLine * self = (GnoteCommandLine*)data;
  
    if(g_str_equal (option_name, "--search")) {
      self->m_do_search = true;
      if(value) {
        self->m_search = value;
      }
    }
    else if(g_str_equal (option_name, "--new-note")) {
      self->m_do_new_note = true;
      if(value) {
        self->m_new_note_name = value;
      }
    }
    return TRUE;
  }


  void GnoteCommandLine::parse(int &argc, gchar ** & argv)
  {
    GError *error = NULL;

    if(!g_option_context_parse (m_context, &argc, &argv, &error)) {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    }

    if(m_open_note && *m_open_note) {
      if (Glib::str_has_prefix(m_open_note, "note://gnote/")) {
        m_open_note_uri = m_open_note;
      }
      else if (sharp::file_exists(m_open_note)) {
        // This is potentially a note file
        m_open_external_note_path = m_open_note;
      } 
      else {
        m_open_note_name = m_open_note;
      }
    }
    
  }


  int GnoteCommandLine::execute()

  {
    bool quit = false;
    DBG_OUT("running args");

    if(m_show_version) {
      print_version();
      quit = true;
      exit(0);
    }

#ifdef ENABLE_DBUS
    RemoteControlClient * remote = RemoteControlProxy::get_instance();
    if(!remote) {
      ERR_OUT("couldn't get remote client");
      return 1;
    }
    if (m_do_new_note) {
      std::string new_uri;

      if (!m_new_note_name.empty()) {
        new_uri = remote->FindNote (m_new_note_name);

        if (new_uri.empty()) {
          new_uri = remote->CreateNamedNote(m_new_note_name);
        }
      } 
      else {
        new_uri = remote->CreateNote ();
      }

      if (!new_uri.empty()) {
        remote->DisplayNote (new_uri);
      }
    }

    if (m_open_start_here) {
      m_open_note_uri = remote->FindStartHereNote ();
    }
    if (!m_open_note_name.empty()) {
      m_open_note_uri = remote->FindNote (m_open_note_name);
    }
    if (!m_open_note_uri.empty()) {
      display_note(remote, m_open_note_uri);
    }

    if (!m_open_external_note_path.empty()) {
      std::string note_id = sharp::file_basename(m_open_external_note_path);
      if (!note_id.empty()) {
        // Attempt to load the note, assuming it might already
        // be part of our notes list.
        if (!display_note(remote, str(boost::format("note://gnote/%1%") % note_id))) {
          sharp::StreamReader sr;
          sr.init(m_open_external_note_path);
          if (sr.file()) {
            std::string noteTitle;
            std::string noteXml;
            sr.read_to_end (noteXml);

            // Make sure noteXml is parseable
            xmlDocPtr doc = xmlParseDoc((const xmlChar*)noteXml.c_str());
            if(doc) {
              xmlFreeDoc(doc);
            }
            else {
              noteXml = "";
            }

            if (!noteXml.empty()) {
              noteTitle = NoteArchiver::obj().get_title_from_note_xml (noteXml);
              if (!noteTitle.empty()) {
                // Check for conflicting titles
                std::string baseTitle = noteTitle;
                for (int i = 1; !remote->FindNote (noteTitle).empty(); i++) {
                  noteTitle = str(boost::format("%1% (%2%)") % baseTitle % i);
                }

                std::string note_uri = remote->CreateNamedNote (noteTitle);

                // Update title in the note XML
                noteXml = NoteArchiver::obj().get_renamed_note_xml (noteXml, baseTitle, noteTitle);

                if (!note_uri.empty()) {
                  // Load in the XML contents of the note file
                  if (remote->SetNoteCompleteXml (note_uri, noteXml)) {
                    display_note(remote, note_uri);
                  }
                }
              }
            }
          }
        }
      }
    }

    if (m_do_search) {
      if (!m_search.empty()) {
        remote->DisplaySearchWithText(m_search);
      }
      else {
        remote->DisplaySearch();
      }
    }
#else
      // as long as we don't have the DBus support.
    if(m_do_search) {
      NoteRecentChanges * recent_changes
        = NoteRecentChanges::get_instance(
          Gnote::obj().default_note_manager());

      recent_changes->set_search_text(m_search);

      recent_changes->present ();
    }
#endif

    if(quit) {
      exit(0);
    }
    return 0;
  }


  void GnoteCommandLine::print_version()
  {
    Glib::ustring version = str(boost::format(_("Version %1%"))
                                % VERSION);
    std::cerr << version << std::endl;
  }


  bool GnoteCommandLine::display_note(RemoteControlClient * remote,
                                      std::string uri)
  {
#ifdef ENABLE_DBUS
    if (m_highlight_search) {
      return remote->DisplayNoteWithSearch(uri, m_highlight_search);
    }
    else {
      return remote->DisplayNote (uri);
    }
#else
    return false;
#endif
  }


  bool GnoteCommandLine::needs_execute() const
  {
    DBG_OUT("needs execute?");
    return m_do_new_note ||
      m_open_note ||
      m_do_search ||
      m_open_start_here ||
      m_highlight_search ||
      m_show_version;
  }


}
