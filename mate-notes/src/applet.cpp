/*
 * gnote
 *
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

#include <boost/format.hpp>

#include <gdkmm/dragcontext.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/selectiondata.h>

#include <libmatepanelappletmm/applet.h>
#include <libmatepanelappletmm/enums.h>
#include <libmatepanelappletmm/factory.h>

#include "sharp/string.hpp"
#include "sharp/uri.hpp"
#include "gnote.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "prefskeybinder.hpp"
#include "tray.hpp"
#include "undo.hpp"
#include "utils.hpp"

#include "applet.hpp"

namespace gnote {
namespace panel {


#define IID "OAFIID:GnoteApplet"
#define FACTORY_IID "OAFIID:GnoteApplet_Factory"

enum PanelOrientation
{
  HORIZONTAL,
  VERTICAL
};

class GnoteMatePanelAppletEventBox
  : public Gtk::EventBox
  , public gnote::IGnoteTray
{
public:
  GnoteMatePanelAppletEventBox(NoteManager & manager);
  virtual ~GnoteMatePanelAppletEventBox();

  const Tray::Ptr & get_tray() const
    {
      return m_tray;
    }
  Gtk::Image & get_image()
    {
      return m_image;
    }

  virtual void show_menu(bool select_first_item);
  virtual bool menu_opens_upward();

protected:
  virtual void on_size_allocate(Gtk::Allocation& allocation);

private:
  bool button_press(GdkEventButton *);
  void prepend_timestamped_text(const Note::Ptr & note,
                                const sharp::DateTime & timestamp,
                                const std::string & text);
  bool paste_primary_clipboard();
  void setup_drag_and_drop();
  void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>&,int,int,
                             const Gtk::SelectionData&,guint,guint);
  void init_pixbuf();
  PanelOrientation get_panel_orientation();

  NoteManager & m_manager;
  Tray::Ptr     m_tray;
  int           m_panel_size;
  Gtk::Image    m_image;
};


GnoteMatePanelAppletEventBox::GnoteMatePanelAppletEventBox(NoteManager & manager)
  : m_manager(manager)
  , m_tray(new Tray(manager, *this))
  , m_panel_size(16)
  , m_image(utils::get_icon("gnote", m_panel_size))
{
  property_can_focus() = true;
  signal_button_press_event().connect(sigc::mem_fun(
                                        *this, &GnoteMatePanelAppletEventBox::button_press));
  add(m_image);
  show_all();
  set_tooltip_text(tray_util_get_tooltip_text());
}


GnoteMatePanelAppletEventBox::~GnoteMatePanelAppletEventBox()
{
}


bool GnoteMatePanelAppletEventBox::button_press(GdkEventButton *ev)
{
  Gtk::Widget * parent = get_parent();
  switch (ev->button) 
  {
  case 1:
    m_tray->update_tray_menu(parent);

    utils::popup_menu(*m_tray->tray_menu(), ev);

    return true;
    break;
  case 2:
    if (Preferences::obj().get<bool>(Preferences::ENABLE_ICON_PASTE)) {
      // Give some visual feedback
      drag_highlight();
      bool retval = paste_primary_clipboard ();
      drag_unhighlight();
      return retval;
    }
    break;
  }
  return false;
}


void GnoteMatePanelAppletEventBox::prepend_timestamped_text(const Note::Ptr & note,
                                                        const sharp::DateTime & timestamp,
                                                        const std::string & text)
{
  NoteBuffer::Ptr buffer = note->get_buffer();
  std::string insert_text =
    str(boost::format("\n%1%\n%2%\n") % timestamp.to_string("%c") % text);

  buffer->undoer().freeze_undo();

  // Insert the date and list of links...
  Gtk::TextIter cursor = buffer->begin();
  cursor.forward_lines (1); // skip title

  cursor = buffer->insert (cursor, insert_text);

  // Make the date string a small font...
  cursor = buffer->begin();
  cursor.forward_lines (2); // skip title & leading newline

  Gtk::TextIter end = cursor;
  end.forward_to_line_end (); // end of date

  buffer->apply_tag_by_name ("datetime", cursor, end);

  // Select the text we've inserted (avoid trailing newline)...
  end = cursor;
  end.forward_chars (insert_text.length() - 1);

  buffer->move_mark(buffer->get_selection_bound(), cursor);
  buffer->move_mark(buffer->get_insert(), end);

  buffer->undoer().thaw_undo();
}



bool GnoteMatePanelAppletEventBox::paste_primary_clipboard()
{
  Glib::RefPtr<Gtk::Clipboard> clip = get_clipboard ("PRIMARY");
  Glib::ustring text = clip->wait_for_text ();

  if (text.empty() || sharp::string_trim(text).empty()) {
    return false;
  }

  Note::Ptr link_note = m_manager.find_by_uri(m_manager.start_note_uri());
  if (!link_note) {
    return false;
  }

  link_note->get_window()->present();
  prepend_timestamped_text (link_note, sharp::DateTime::now(), text);

  return true;
}

void GnoteMatePanelAppletEventBox::show_menu(bool select_first_item)
{
  m_tray->update_tray_menu(this);
  if(select_first_item) {
    m_tray->tray_menu()->select_first(false);
  }
  utils::popup_menu(*m_tray->tray_menu(), NULL);
}


// Support dropping text/uri-lists and _NETSCAPE_URLs currently.
void GnoteMatePanelAppletEventBox::setup_drag_and_drop()
{
  std::list<Gtk::TargetEntry> targets;

  targets.push_back(Gtk::TargetEntry ("text/uri-list", (Gtk::TargetFlags)0, 0));
  targets.push_back(Gtk::TargetEntry ("_NETSCAPE_URL", (Gtk::TargetFlags)0, 0));

  drag_dest_set(targets, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_COPY);

  signal_drag_data_received().connect(
    sigc::mem_fun(*this, &GnoteMatePanelAppletEventBox::on_drag_data_received));
}

void GnoteMatePanelAppletEventBox::on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>&,
                                                     int,int,
                                                     const Gtk::SelectionData & data,
                                                     guint,guint)
{
  utils::UriList uri_list(data);
  if (uri_list.empty()) {
    return;
  }

  std::string insert_text;
  bool more_than_one = false;

  for(utils::UriList::const_iterator iter = uri_list.begin();
      iter != uri_list.end(); ++iter) {

    const sharp::Uri & uri(*iter);

    if (more_than_one) {
      insert_text += "\n";
    }

    if (uri.is_file()) {
      insert_text += uri.local_path();
    }
    else {
      insert_text += uri.to_string();
    }

    more_than_one = true;
  }

  Note::Ptr link_note = m_manager.find_by_uri (m_manager.start_note_uri());
  if (link_note) {
    link_note->get_window()->present();
    prepend_timestamped_text(link_note, sharp::DateTime::now(),
                              insert_text);
  }
}


void GnoteMatePanelAppletEventBox::init_pixbuf()
{
  // For some reason, the first time we ask for the allocation,
  // it's a 1x1 pixel.  Prevent against this by returning a
  // reasonable default.  Setting the icon causes OnSizeAllocated
  // to be called again anyhow.
  int icon_size = m_panel_size;
  if (icon_size < 16) {
    icon_size = 16;
  }

  // Control specifically which icon is used at the smaller sizes
  // so that no scaling occurs.  In the case of the panel applet,
  // add a couple extra pixels of padding so it matches the behavior
  // of the notification area tray icon.  See bug #403500 for more
  // info.
  if (Gnote::obj().is_mate_panel_applet())
    icon_size = icon_size - 2; // padding
  if (icon_size <= 21)
    icon_size = 16;
  else if (icon_size <= 31)
    icon_size = 22;
  else if (icon_size <= 47)
    icon_size = 32;

  Glib::RefPtr<Gdk::Pixbuf> new_icon = utils::get_icon("gnote", icon_size);
  m_image.property_pixbuf() = new_icon;
}


PanelOrientation GnoteMatePanelAppletEventBox::get_panel_orientation()
{
  if (!get_parent_window()) {
    return HORIZONTAL;
  }

  Glib::RefPtr<Gdk::Window> top_level_window = get_parent_window()->get_toplevel();

  Gdk::Rectangle rect;
  top_level_window->get_frame_extents(rect);
  if (rect.get_width() < rect.get_height()) {
    return HORIZONTAL;
  }

  return VERTICAL;
}



void GnoteMatePanelAppletEventBox::on_size_allocate(Gtk::Allocation& allocation)
{
  Gtk::EventBox::on_size_allocate(allocation);

  // Determine the orientation
  if (get_panel_orientation () == HORIZONTAL) {
    if (m_panel_size == get_allocation().get_height()) {
      return;
    }

    m_panel_size = get_allocation().get_height();
  } 
  else {
    if (m_panel_size == get_allocation().get_width()) {
      return;
    }
    
    m_panel_size = get_allocation().get_width();
  }

  init_pixbuf ();
}


bool GnoteMatePanelAppletEventBox::menu_opens_upward()
{
  bool open_upwards = false;
  int val = 0;
  Glib::RefPtr<Gdk::Screen> screen;

  int x, y;
  get_window()->get_origin(x, y);
  val = y;
  screen = get_screen();

  Gtk::Requisition req = m_tray->tray_menu()->size_request();
  if ((val + req.height) >= screen->get_height()) {
    open_upwards = true;
  }

  return open_upwards;
}


class GnoteApplet
  : public Mate::Panel::Applet
{
public:
  explicit GnoteApplet(MatePanelApplet *);
  virtual ~GnoteApplet();

protected:
  virtual void on_change_background(Mate::Panel::AppletBackgroundType type, const Gdk::Color & color, const Glib::RefPtr<const Gdk::Pixmap>& pixmap);
  virtual void on_change_size(int size);
private:
  static void show_preferences_verb(MateComponentUIComponent*, void*, const char*);
  static void show_help_verb(MateComponentUIComponent*, void*, const char*);
  static void show_about_verb(MateComponentUIComponent*, void*, const char*);


  NoteManager & m_manager;
  GnoteMatePanelAppletEventBox m_applet_event_box;
  PrefsKeybinder *m_keybinder;
};


GnoteApplet::GnoteApplet(MatePanelApplet *castitem)
  : Mate::Panel::Applet(castitem)
  , m_manager(Gnote::obj().default_note_manager())
  , m_applet_event_box(m_manager)
  , m_keybinder(new GnotePrefsKeybinder(m_manager, m_applet_event_box))
{
  static const MateComponentUIVerb menu_verbs[] = {
    MATECOMPONENT_UI_VERB("Props", &GnoteApplet::show_preferences_verb),
    MATECOMPONENT_UI_VERB("Help", &GnoteApplet::show_help_verb),
    MATECOMPONENT_UI_VERB("About", &GnoteApplet::show_about_verb),
    MATECOMPONENT_UI_VERB_END
  };

  setup_menu(DATADIR"/gnote", "MATE_GnoteApplet.xml", "", menu_verbs, this);

  add (m_applet_event_box);
  Gnote::obj().set_tray(m_applet_event_box.get_tray());
  on_change_size (get_size());

  set_flags(Mate::Panel::APPLET_EXPAND_MINOR);

  show_all ();
}


GnoteApplet::~GnoteApplet()
{
  delete m_keybinder;
}

void GnoteApplet::show_preferences_verb(MateComponentUIComponent*, void*, const char*)
{
  ActionManager::obj()["ShowPreferencesAction"]->activate();
}

void GnoteApplet::show_help_verb(MateComponentUIComponent*, void* data, const char*)
{
  GnoteApplet* applet = static_cast<GnoteApplet*>(data);
  // Don't use the ActionManager in this case because
  // the handler won't know about the Screen.
  utils::show_help("gnote", "", applet->get_screen()->gobj(), NULL);
}

void GnoteApplet::show_about_verb(MateComponentUIComponent*, void*, const char*)
{
  ActionManager::obj()["ShowAboutAction"]->activate();
}

  void GnoteApplet::on_change_background(Mate::Panel::AppletBackgroundType type, 
                                       const Gdk::Color & color, 
                                       const Glib::RefPtr<const Gdk::Pixmap>& pixmap)
{
  Glib::RefPtr<Gtk::RcStyle> rcstyle(Gtk::RcStyle::create());
  m_applet_event_box.unset_style();
  m_applet_event_box.modify_style (rcstyle);

  switch (type) {
  case Mate::Panel::COLOR_BACKGROUND:
    m_applet_event_box.modify_bg(Gtk::STATE_NORMAL, color);
    break;
  case Mate::Panel::NO_BACKGROUND:
    break;
  case Mate::Panel::PIXMAP_BACKGROUND:
    Glib::RefPtr<Gtk::Style> copy = m_applet_event_box.get_style()->copy();
    copy->set_bg_pixmap(Gtk::STATE_NORMAL, pixmap);
    m_applet_event_box.set_style(copy);
    break;
  }
}


void GnoteApplet::on_change_size(int size)
{
  m_applet_event_box.set_size_request (size, size);
}


int register_applet()
{
  int returncode = Mate::Panel::factory_main<GnoteApplet>(FACTORY_IID);
  return returncode;
}

}
}

