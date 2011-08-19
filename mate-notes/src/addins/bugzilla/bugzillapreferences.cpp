/*
 * gnote
 *
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



#include <glib.h>
#include <glibmm/i18n.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/stock.h>

#include "sharp/directory.hpp"
#include "sharp/fileinfo.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"
#include "debug.hpp"
#include "utils.hpp"

#include "bugzillanoteaddin.hpp"
#include "bugzillapreferences.hpp"


namespace bugzilla {

  bool BugzillaPreferences::s_static_inited = false;;
  std::string BugzillaPreferences::s_image_dir;

  void BugzillaPreferences::_init_static()
  {
    if(!s_static_inited) {
      s_image_dir = BugzillaNoteAddin::images_dir();
      s_static_inited = true;
    }
  }

  BugzillaPreferences::BugzillaPreferences()
    : Gtk::VBox(false, 12)
  {
    _init_static();
    last_opened_dir = Glib::get_home_dir();

    Gtk::Label *l = manage(new Gtk::Label (_("You can use any bugzilla just by dragging links "
                                   "into notes.  If you want a special icon for "
                                              "certain hosts, add them here.")));
    l->property_wrap() = true;
    l->property_xalign() = 0;

    pack_start(*l, false, false, 0);

    icon_store = Gtk::ListStore::create(m_columns);
    icon_store->set_sort_column_id(m_columns.host, Gtk::SORT_ASCENDING);

    icon_tree = manage(new Gtk::TreeView (icon_store));
    icon_tree->set_headers_visible(true);
    icon_tree->get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    icon_tree->get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &BugzillaPreferences::selection_changed));

    Gtk::TreeViewColumn *host_col = manage(new Gtk::TreeViewColumn(_("Host Name"), m_columns.host));
    host_col->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    host_col->set_resizable(true);
    host_col->set_expand(true);
    host_col->set_min_width(200);

    host_col->set_sort_column(m_columns.host);
    host_col->set_sort_indicator(false);
    host_col->set_reorderable(false);
    host_col->set_sort_order(Gtk::SORT_ASCENDING);

    icon_tree->append_column (*host_col);

    Gtk::TreeViewColumn *icon_col = manage(new Gtk::TreeViewColumn(_("Icon"), m_columns.icon));
    icon_col->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    icon_col->set_max_width(50);
    icon_col->set_min_width(50);
    icon_col->set_resizable(false);

    icon_tree->append_column (*icon_col);

    Gtk::ScrolledWindow *sw = manage(new Gtk::ScrolledWindow ());
    sw->set_shadow_type(Gtk::SHADOW_IN);
    sw->property_height_request() = 200;
    sw->property_width_request() = 300;
    sw->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    sw->add (*icon_tree);

    pack_start(*sw, true, true, 0);

    add_button = manage(new Gtk::Button (Gtk::Stock::ADD));
    add_button->signal_clicked().connect(
      sigc::mem_fun(*this, &BugzillaPreferences::add_clicked));

    remove_button = manage(new Gtk::Button (Gtk::Stock::REMOVE));
    remove_button->set_sensitive(false);
    remove_button->signal_clicked().connect(
      sigc::mem_fun(*this,  &BugzillaPreferences::remove_clicked));

    Gtk::HButtonBox *hbutton_box = manage(new Gtk::HButtonBox ());
    hbutton_box->set_layout(Gtk::BUTTONBOX_START);
    hbutton_box->set_spacing(6);

    hbutton_box->pack_start(*add_button);
    hbutton_box->pack_start(*remove_button);
    pack_start(*hbutton_box, false, false, 0);

    show_all ();
  }


  void BugzillaPreferences::update_icon_store()
  {
    if (!sharp::directory_exists (s_image_dir)) {
      return;
    }

    icon_store->clear(); // clear out the old entries

    std::list<std::string> icon_files; 
    sharp::directory_get_files (s_image_dir, icon_files);
    for(std::list<std::string>::const_iterator iter = icon_files.begin();
        iter != icon_files.end(); ++iter) {
      
      const std::string & icon_file(*iter);
      sharp::FileInfo file_info(icon_file);

      Glib::RefPtr<Gdk::Pixbuf> pixbuf;
      try {
        pixbuf = Gdk::Pixbuf::create_from_file(icon_file);
      } 
      catch (const Glib::Error & e) {
        DBG_OUT("Error loading Bugzilla Icon %s: %s", icon_file.c_str(), e.what().c_str());
      }

      if (!pixbuf) {
        continue;
      }

      std::string host = parse_host (file_info);
      if (!host.empty()) {
        Gtk::TreeIter treeiter = icon_store->append ();
        
        (*treeiter)[m_columns.icon] = pixbuf;
        (*treeiter)[m_columns.host] = host;
        (*treeiter)[m_columns.file_path] = icon_file;
      }
    }
  }


  std::string BugzillaPreferences::parse_host(const sharp::FileInfo & file_info)
  {
    std::string name = file_info.get_name();
    std::string ext = file_info.get_extension();

    if (ext.empty()) {
      return "";
    }

    int ext_pos = sharp::string_index_of(name, ext);
    if (ext_pos <= 0) {
      return "";
    }

    std::string host = sharp::string_substring(name, 0, ext_pos);
    if (host.empty()) {
      return "";
    }

    return host;
  }

  void BugzillaPreferences::on_realize()
  {
    Gtk::VBox::on_realize();

    update_icon_store();
  }


  void BugzillaPreferences::selection_changed()
  {
    remove_button->set_sensitive(icon_tree->get_selection()->count_selected_rows() > 0);
  }
  
  namespace {

    /** sanitize the hostname. Return false if nothing could be done */
    bool sanitize_hostname(std::string & hostname)
    {
      if(sharp::string_contains(hostname, "/") 
         || sharp::string_contains(hostname, ":")) {
        sharp::Uri uri(hostname);
        std::string new_host = uri.get_host();
        if(new_host.empty()) {
          return false;
        }
        hostname = new_host;
      }
      return true;
    }

  }

  void BugzillaPreferences::add_clicked()
  {
    Gtk::FileChooserDialog dialog(_("Select an icon..."),
                                  Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button (Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    dialog.set_default_response(Gtk::RESPONSE_OK);
    dialog.set_local_only(true);
    dialog.set_current_folder (last_opened_dir);

    Gtk::FileFilter filter;
    filter.add_pixbuf_formats ();

    dialog.add_filter(filter);

    // Extra Widget
    Gtk::Label *l = manage(new Gtk::Label (_("_Host name:"), true));
    Gtk::Entry *host_entry = manage(new Gtk::Entry ());
    l->set_mnemonic_widget(*host_entry);
    Gtk::HBox *hbox = manage(new Gtk::HBox (false, 6));
    hbox->pack_start (*l, false, false, 0);
    hbox->pack_start (*host_entry, true, true, 0);
    hbox->show_all ();

    dialog.set_extra_widget(*hbox);

    int response;
    std::string icon_file;
    std::string host;

    while(1) {
      response = dialog.run ();
      icon_file = dialog.get_filename();
      host = sharp::string_trim(host_entry->get_text());


      if (response == (int) Gtk::RESPONSE_OK) {

        bool valid = sanitize_hostname(host);
      
        if(valid && !host.empty()) {
          break;
        }
        // Let the user know that they
        // have to specify a host name.
        gnote::utils::HIGMessageDialog warn(
          NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
          Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK,
          _("Host name invalid"),
          _("You must specify a valid Bugzilla "
            "host name to use with this icon."));
        warn.run ();

        host_entry->grab_focus ();
      } 
      else if (response != (int) Gtk::RESPONSE_OK) {
        return;
      }
    }

    // Keep track of the last directory the user had open
    last_opened_dir = dialog.get_current_folder();

    // Copy the file to the BugzillaIcons directory
    std::string err_msg;
    if (!copy_to_bugzilla_icons_dir (icon_file, host, err_msg)) {
      gnote::utils::HIGMessageDialog err(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                         Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
                                         _("Error saving icon"),
                                         std::string(_("Could not save the icon file.")) +
                                         "  " + err_msg);
      err.run();
    }

    update_icon_store();
  }

  bool BugzillaPreferences::copy_to_bugzilla_icons_dir(const std::string & file_path,
                                                       const std::string & host,
                                                       std::string & err_msg)
  {
    err_msg = "";

    sharp::FileInfo file_info(file_path);
    std::string ext = file_info.get_extension();
    std::string saved_path = s_image_dir + "/" + host + ext;
    try {
      if (!sharp::directory_exists (s_image_dir)) {
        g_mkdir_with_parents(s_image_dir.c_str(), S_IRWXU);
      }

      sharp::file_copy (file_path, saved_path);
    } 
    catch (const std::exception & e) {
      err_msg = e.what();
      return false;
    }

    resize_if_needed (saved_path);
    return true;
  }

  void BugzillaPreferences::resize_if_needed(const std::string & p)
  {
    Glib::RefPtr<Gdk::Pixbuf> pix, newpix;

    try {
      const double dim = 16;
      pix = Gdk::Pixbuf::create_from_file(p);
      int height, width;
      int orig_h, orig_w;
      orig_h = pix->get_height();
      orig_w = pix->get_width();
      int orig_dim = std::max(orig_h, orig_w);
      double ratio = dim / (double)orig_dim;
      width = (int)(ratio * orig_w);
      height = (int)(ratio * orig_h);
      newpix = pix->scale_simple(width, height, 
                                 Gdk::INTERP_BILINEAR);
      newpix->save(p, "png");
    }
    catch(...) {

    }
  }


  void BugzillaPreferences::remove_clicked()
  {
    // Remove the icon file and call UpdateIconStore ().
    Gtk::TreeIter iter;
    iter = icon_tree->get_selection()->get_selected();
    if (!iter) {
      return;
    }

    std::string icon_path = (*iter)[m_columns.file_path];

    gnote::utils::HIGMessageDialog dialog(NULL, 
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE,
                                          _("Really remove this icon?"),
                                          _("If you remove an icon it is permanently lost."));

    Gtk::Button *button;

    button = manage(new Gtk::Button (Gtk::Stock::CANCEL));
    button->property_can_default() = true;
    button->show ();
    dialog.add_action_widget (*button, Gtk::RESPONSE_CANCEL);
    dialog.set_default_response(Gtk::RESPONSE_CANCEL);

    button = manage(new Gtk::Button (Gtk::Stock::DELETE));
    button->property_can_default() = true;
    button->show ();
    dialog.add_action_widget (*button, 666);

    int result = dialog.run ();
    if (result == 666) {
      try {
        sharp::file_delete (icon_path);
        update_icon_store ();
      } 
      catch (const sharp::Exception & e) {
        ERR_OUT("Error removing icon %s: %s", icon_path.c_str(), e.what());
      }
    }
  }


}

