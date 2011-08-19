/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
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



#include <boost/lexical_cast.hpp>

#include <pcrecpp.h>

#include <glib.h>
#include <glibmm/i18n.h>


#include "sharp/directory.hpp"

#include "debug.hpp"
#include "gnote.hpp"
#include "notebuffer.hpp"
#include "notewindow.hpp"

#include "bugzillanoteaddin.hpp"
#include "bugzillalink.hpp"
#include "bugzillapreferencesfactory.hpp"
#include "insertbugaction.hpp"

namespace bugzilla {

  DECLARE_MODULE(BugzillaModule);

  BugzillaModule::BugzillaModule()
  {
    ADD_INTERFACE_IMPL(BugzillaNoteAddin);
    ADD_INTERFACE_IMPL(BugzillaPreferencesFactory);
    enabled(false);
  }
  const char * BugzillaModule::id() const
  {
    return "BugzillaAddin";
  }
  const char * BugzillaModule::name() const
  {
    return _("Bugzilla Links");
  }
  const char * BugzillaModule::description() const
  {
    return _("Allows you to drag a Bugzilla URL from your browser directly into a Gnote note.  The bug number is inserted as a link with a little bug icon next to it.");
  }
  const char * BugzillaModule::authors() const
  {
    return _("Hubert Figuiere and the Tomboy Project");
  }
  int BugzillaModule::category() const
  {
    return gnote::ADDIN_CATEGORY_DESKTOP_INTEGRATION;
  }
  const char * BugzillaModule::version() const
  {
    return "0.1";
  }


  const char * BugzillaNoteAddin::TAG_NAME = "link:bugzilla";

  BugzillaNoteAddin::BugzillaNoteAddin()
    : gnote::NoteAddin()
  {
    const bool is_first_run
                 = !sharp::directory_exists(images_dir());
    const std::string old_images_dir
      = Glib::build_filename(gnote::Gnote::old_note_dir(),
                             "BugzillaIcons");
    const bool migration_needed
                 = is_first_run
                   && sharp::directory_exists(old_images_dir);

    if (is_first_run)
      g_mkdir_with_parents(images_dir().c_str(), S_IRWXU);

    if (migration_needed)
      migrate_images(old_images_dir);
  }

  std::string BugzillaNoteAddin::images_dir()
  {
    return Glib::build_filename(gnote::Gnote::conf_dir(),
                                "BugzillaIcons");
  }

  void BugzillaNoteAddin::initialize()
  {
    if(!get_note()->get_tag_table()->is_dynamic_tag_registered(TAG_NAME)) {
      get_note()->get_tag_table()
        ->register_dynamic_tag(TAG_NAME, sigc::ptr_fun(&BugzillaLink::create));
    }
  }

  void BugzillaNoteAddin::migrate_images(
                            const std::string & old_images_dir)
  {
    const Glib::RefPtr<Gio::File> src
      = Gio::File::create_for_path(old_images_dir);
    const Glib::RefPtr<Gio::File> dest
      = Gio::File::create_for_path(gnote::Gnote::conf_dir());

    try {
      sharp::directory_copy(src, dest);
    }
    catch (const Gio::Error & e) {
      DBG_OUT("BugzillaNoteAddin: migrating images: %s",
              e.what().c_str());
    }
  }


  void BugzillaNoteAddin::shutdown()
  {
  }


  void BugzillaNoteAddin::on_note_opened()
  {
    get_window()->editor()->signal_drag_data_received().connect(
      sigc::mem_fun(*this, &BugzillaNoteAddin::on_drag_data_received), false);
  }


  void BugzillaNoteAddin::on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, 
                                                int x, int y, 
                                                const Gtk::SelectionData & selection_data, 
                                                guint, guint time)
  {
    DBG_OUT("Bugzilla.OnDragDataReceived");
    drop_uri_list(context, x, y, selection_data, time);
    return;
  }


  void BugzillaNoteAddin::drop_uri_list(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, 
                                        const Gtk::SelectionData & selection_data, guint time)
  {
    std::string uriString = selection_data.get_text();
    if(uriString.empty()) {
      return;
    }

    const char * regexString = "\\bhttps?://.*/show_bug\\.cgi\\?(\\S+\\&){0,1}id=(\\d{1,})";

    pcrecpp::RE re(regexString, pcrecpp::RE_Options(PCRE_CASELESS|PCRE_UTF8));
    int m;

    if(re.FullMatch(uriString, (void*)NULL, &m)) {

      int bugId = m;

      if (insert_bug (x, y, uriString, bugId)) {
        context->drag_finish(true, false, time);
        g_signal_stop_emission_by_name(get_window()->editor()->gobj(),
                                       "drag_data_received");
      }
    }
  }


  bool BugzillaNoteAddin::insert_bug(int x, int y, const std::string & uri, int id)
  {
    try {
      BugzillaLink::Ptr link_tag = 
        BugzillaLink::Ptr::cast_dynamic(get_note()->get_tag_table()->create_dynamic_tag(TAG_NAME));
      link_tag->set_bug_url(uri);

      // Place the cursor in the position where the uri was
      // dropped, adjusting x,y by the TextView's VisibleRect.
      Gdk::Rectangle rect;
      get_window()->editor()->get_visible_rect(rect);
      x = x + rect.get_x();
      y = y + rect.get_y();
      Gtk::TextIter cursor;
      gnote::NoteBuffer::Ptr buffer = get_buffer();
      get_window()->editor()->get_iter_at_location(cursor, x, y);
      buffer->place_cursor (cursor);

      std::string string_id = boost::lexical_cast<std::string>(id);
      buffer->undoer().add_undo_action (new InsertBugAction (cursor, 
                                                             string_id, 
                                                             link_tag));

      std::vector<Glib::RefPtr<Gtk::TextTag> > tags;
      tags.push_back(link_tag);
      buffer->insert_with_tags (cursor, 
                                string_id, 
                                tags);
      return true;
    } 
    catch (...)
    {
		}
    return false;
  }

}

