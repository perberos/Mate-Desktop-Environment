/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
 * Copyright (C) 2009, 2010 Debarshi Ray
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


#include <config.h>

#include <boost/bind.hpp>
#include <boost/checked_delete.hpp>

#include <glib.h>

#include "sharp/map.hpp"
#include "sharp/directory.hpp"
#include "sharp/dynamicmodule.hpp"

#include "addinmanager.hpp"
#include "addinpreferencefactory.hpp"
#include "debug.hpp"
#include "gnote.hpp"
#include "watchers.hpp"
#include "notebooks/notebookapplicationaddin.hpp"
#include "notebooks/notebooknoteaddin.hpp"


#if 1

// HACK to make sure we link the modules needed only for addins
#include "base/inifile.hpp"
#include "sharp/fileinfo.hpp"
#include "sharp/streamwriter.hpp"
#include "sharp/xsltransform.hpp"
#include "sharp/xsltargumentlist.hpp"

bool links[] = {
  base::IniFileLink_,
  sharp::FileInfoLink_,
  sharp::StreamWriterLink_,
  sharp::XslTransformLink_,
  sharp::XsltArgumentListLink_,
  false
};


#endif


namespace gnote {

#define REGISTER_BUILTIN_NOTE_ADDIN(klass) \
  do { sharp::IfaceFactoryBase *iface = new sharp::IfaceFactory<klass>; \
  m_builtin_ifaces.push_back(iface); \
  m_note_addin_infos.insert(std::make_pair(typeid(klass).name(),iface)); } while(0)

#define REGISTER_APP_ADDIN(klass) \
  m_app_addins.insert(std::make_pair(typeid(klass).name(),        \
                                     klass::create()))

  AddinManager::AddinManager(const std::string & conf_dir)
    : m_gnote_conf_dir(conf_dir)
  {
    m_addins_prefs_dir = Glib::build_filename(conf_dir, "addins");
    m_addins_prefs_file = Glib::build_filename(m_addins_prefs_dir,
                                               "global.ini");

    const bool is_first_run
                 = !sharp::directory_exists(m_addins_prefs_dir);
    const std::string old_addins_dir
                        = Glib::build_filename(Gnote::old_note_dir(),
                                               "addins");
    const bool migration_needed
                 = is_first_run
                   && sharp::directory_exists(old_addins_dir);

    if (is_first_run)
      g_mkdir_with_parents(m_addins_prefs_dir.c_str(), S_IRWXU);

    if (migration_needed)
      migrate_addins(old_addins_dir);

    initialize_sharp_addins();
  }

  AddinManager::~AddinManager()
  {
    sharp::map_delete_all_second(m_app_addins);
    for(NoteAddinMap::const_iterator iter = m_note_addins.begin();
        iter != m_note_addins.end(); ++iter) {
      sharp::map_delete_all_second(iter->second);
    }
    sharp::map_delete_all_second(m_addin_prefs);
    sharp::map_delete_all_second(m_import_addins);
    for(std::list<sharp::IfaceFactoryBase*>::iterator iter = m_builtin_ifaces.begin();
        iter != m_builtin_ifaces.end(); ++iter) {
      delete *iter;
    }
  }

  void AddinManager::add_note_addin_info(
                       const sharp::DynamicModule * dmod)
  {
    const char * const id = dmod->id();

    {
      const IdInfoMap::const_iterator iter
                                        = m_note_addin_infos.find(id);
      if (m_note_addin_infos.end() != iter) {
        ERR_OUT("NoteAddin info %s already present", id);
        return;
      }
    }

    sharp::IfaceFactoryBase * const f = dmod->query_interface(
                                          NoteAddin::IFACE_NAME);
    if(!f) {
      ERR_OUT("does not implement %s", NoteAddin::IFACE_NAME);
      return;
    }

    m_note_addin_infos.insert(std::make_pair(std::string(id), f));

    {
      for(NoteAddinMap::iterator iter = m_note_addins.begin();
          iter != m_note_addins.end(); ++iter) {
        IdAddinMap & id_addin_map = iter->second;
        IdAddinMap::const_iterator it = id_addin_map.find(id);
        if (id_addin_map.end() != it) {
          ERR_OUT("NoteAddin %s already present", id);
          continue;
        }

        const Note::Ptr & note = iter->first;
        NoteAddin * const addin = dynamic_cast<NoteAddin *>((*f)());
        if (addin) {
         addin->initialize(note);
         id_addin_map.insert(std::make_pair(id, addin));
        }
      }
    }
  }

  void AddinManager::erase_note_addin_info(
                       const sharp::DynamicModule * dmod)
  {
    const char * const id = dmod->id();

    {
      const IdInfoMap::iterator iter = m_note_addin_infos.find(id);
      if (m_note_addin_infos.end() == iter) {
        ERR_OUT("NoteAddin info %s absent", id);
        return;
      }

      m_note_addin_infos.erase(iter);
    }

    {
      for(NoteAddinMap::iterator iter = m_note_addins.begin();
          iter != m_note_addins.end(); ++iter) {
        IdAddinMap & id_addin_map = iter->second;
        IdAddinMap::iterator it = id_addin_map.find(id);
        if (id_addin_map.end() == it) {
          ERR_OUT("NoteAddin %s absent", id);
          continue;
        }

        NoteAddin * const addin = it->second;
        if (addin) {
          addin->dispose(true);
          id_addin_map.erase(it);
        }
      }
    }
  }

  void AddinManager::initialize_sharp_addins()
  {
    if (!sharp::directory_exists (m_addins_prefs_dir))
      g_mkdir_with_parents(m_addins_prefs_dir.c_str(), S_IRWXU);

    // get the factory

    REGISTER_BUILTIN_NOTE_ADDIN(NoteRenameWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteSpellChecker);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteUrlWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteLinkWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteWikiWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(MouseHandWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteTagsWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(notebooks::NotebookNoteAddin);
   
    REGISTER_APP_ADDIN(notebooks::NotebookApplicationAddin);

    m_module_manager.add_path(LIBDIR"/"PACKAGE_NAME"/addins/"PACKAGE_VERSION);
    m_module_manager.add_path(m_gnote_conf_dir);

    m_module_manager.load_modules();

    bool global_addins_prefs_loaded = true;
    Glib::KeyFile global_addins_prefs;
    try {
      global_addins_prefs.load_from_file(m_addins_prefs_file);
    }
    catch (Glib::Error & not_loaded) {
      global_addins_prefs_loaded = false;
    }

    const sharp::ModuleList & list = m_module_manager.get_modules();
    for(sharp::ModuleList::const_iterator iter = list.begin();
        iter != list.end(); ++iter) {

      sharp::DynamicModule* dmod = *iter;
      if(!dmod) {
        continue;
      }

      if(global_addins_prefs_loaded &&
         global_addins_prefs.has_key("Enabled", dmod->id())) {
        dmod->enabled(global_addins_prefs.get_boolean("Enabled", dmod->id()));
      }

      sharp::IfaceFactoryBase * f = dmod->query_interface(NoteAddin::IFACE_NAME);
      if(f && dmod->is_enabled()) {
        m_note_addin_infos.insert(std::make_pair(dmod->id(), f));
      }

      f = dmod->query_interface(AddinPreferenceFactoryBase::IFACE_NAME);
      if(f) {
        AddinPreferenceFactoryBase * factory = dynamic_cast<AddinPreferenceFactoryBase*>((*f)());
        m_addin_prefs.insert(std::make_pair(dmod->id(), factory));
      }

      f = dmod->query_interface(ImportAddin::IFACE_NAME);
      if(f) {
        ImportAddin * addin = dynamic_cast<ImportAddin*>((*f)());
        m_import_addins.insert(std::make_pair(dmod->id(), addin));
      }

      f = dmod->query_interface(ApplicationAddin::IFACE_NAME);
      if(f) {
        ApplicationAddin * addin = dynamic_cast<ApplicationAddin*>((*f)());
        m_app_addins.insert(std::make_pair(dmod->id(), addin));
      }
    }
  }

  void AddinManager::load_addins_for_note(const Note::Ptr & note)
  {
    if(m_note_addins.find(note) != m_note_addins.end()) {
      ERR_OUT("trying to load addins when they are already loaded");
      return;
    }
    IdAddinMap loaded_addins;
    m_note_addins[note] = loaded_addins;

    IdAddinMap & loaded(m_note_addins[note]); // avoid copying the whole map
    for(IdInfoMap::const_iterator iter = m_note_addin_infos.begin();
        iter != m_note_addin_infos.end(); ++iter) {

      const IdInfoMap::value_type & addin_info(*iter); 
      sharp::IInterface* iface = (*addin_info.second)();
      NoteAddin * addin = dynamic_cast<NoteAddin *>(iface);
      if(addin) {
        addin->initialize(note);
        loaded.insert(std::make_pair(addin_info.first, addin));
      }
      else {
        DBG_OUT("wrong type for the interface: %s", typeid(*iface).name());
        delete iface;
      }
    }
  }

  ApplicationAddin * AddinManager::get_application_addin(
                                     const std::string & id) const
  {
    const IdImportAddinMap::const_iterator import_iter
      = m_import_addins.find(id);

    if (m_import_addins.end() != import_iter)
      return import_iter->second;

    const AppAddinMap::const_iterator app_iter
      = m_app_addins.find(id);

    if (m_app_addins.end() != app_iter)
      return app_iter->second;

    return 0;
  }

  void AddinManager::get_preference_tab_addins(std::list<PreferenceTabAddin *> &l) const
  {
    sharp::map_get_values(m_pref_tab_addins, l);
  }


  void AddinManager::get_import_addins(std::list<ImportAddin*> & l) const
  {
    sharp::map_get_values(m_import_addins, l);
  }

  void AddinManager::initialize_application_addins() const
  {
    for(AppAddinMap::const_iterator iter = m_app_addins.begin();
        iter != m_app_addins.end(); ++iter) {
      ApplicationAddin * addin = iter->second;
      const sharp::DynamicModule * dmod
        = m_module_manager.get_module(iter->first);
      if (!dmod || dmod->is_enabled()) {
        addin->initialize();
      }
    }
  }

  void AddinManager::shutdown_application_addins() const
  {
    for(AppAddinMap::const_iterator iter = m_app_addins.begin();
        iter != m_app_addins.end(); ++iter) {
      ApplicationAddin * addin = iter->second;
      const sharp::DynamicModule * dmod
        = m_module_manager.get_module(iter->first);
      if (!dmod || dmod->is_enabled()) {
        try {
          addin->shutdown();
        }
        catch (const sharp::Exception & e) {
          DBG_OUT("Error calling %s.Shutdown (): %s",
                  typeid(*addin).name(), e.what());
        }
      }
    }
  }

  void AddinManager::save_addins_prefs() const
  {
    Glib::KeyFile global_addins_prefs;
    try {
      global_addins_prefs.load_from_file(m_addins_prefs_file);
    }
    catch (Glib::Error & not_loaded_ignored) {
    }

    const sharp::ModuleList & list = m_module_manager.get_modules();
    for(sharp::ModuleList::const_iterator iter = list.begin();
        iter != list.end(); ++iter) {
      const sharp::DynamicModule* dmod = *iter;
      global_addins_prefs.set_boolean("Enabled", dmod->id(),
                                      dmod->is_enabled());
    }

    Glib::RefPtr<Gio::File> prefs_file = Gio::File::create_for_path(
                                           m_addins_prefs_file);
    Glib::RefPtr<Gio::FileOutputStream> prefs_file_stream
                                          = prefs_file->append_to();
    prefs_file_stream->truncate(0);
    prefs_file_stream->write(global_addins_prefs.to_data());
  }

  Gtk::Widget * AddinManager::create_addin_preference_widget(const std::string & id)
  {
    IdAddinPrefsMap::const_iterator iter = m_addin_prefs.find(id);
    if(iter != m_addin_prefs.end()) {
      return iter->second->create_preference_widget();
    }
    return NULL;
  }

  void AddinManager::migrate_addins(const std::string & old_addins_dir)
  {
    const Glib::RefPtr<Gio::File> src
      = Gio::File::create_for_path(old_addins_dir);
    const Glib::RefPtr<Gio::File> dest
      = Gio::File::create_for_path(m_gnote_conf_dir);

    try {
      sharp::directory_copy(src, dest);
    }
    catch (const Gio::Error & e) {
      DBG_OUT("AddinManager: migrating addins: %s", e.what().c_str());
    }
  }
}
