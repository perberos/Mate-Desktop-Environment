#include <config.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glibtop/procstate.h>

#include <vector>

#include "prettytable.h"
#include "defaulttable.h"
#include "proctable.h"
#include "util.h"


namespace
{
  const unsigned APP_ICON_SIZE = 16;
}


PrettyTable::PrettyTable()
{
  WnckScreen* screen = wnck_screen_get_default();
  g_signal_connect(G_OBJECT(screen), "application_opened",
		   G_CALLBACK(PrettyTable::on_application_opened), this);
  g_signal_connect(G_OBJECT(screen), "application_closed",
		   G_CALLBACK(PrettyTable::on_application_closed), this);
}


PrettyTable::~PrettyTable()
{
}


void
PrettyTable::on_application_opened(WnckScreen* screen, WnckApplication* app, gpointer data)
{
  PrettyTable * const that = static_cast<PrettyTable*>(data);

  pid_t pid = wnck_application_get_pid(app);

  if (pid == 0)
    return;

  const char* icon_name = wnck_application_get_icon_name(app);


  Glib::RefPtr<Gdk::Pixbuf> icon;

  icon = that->theme->load_icon(icon_name, APP_ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);

  if (not icon) {
    icon = Glib::wrap(wnck_application_get_icon(app), /* take_copy */ true);
    icon = icon->scale_simple(APP_ICON_SIZE, APP_ICON_SIZE, Gdk::INTERP_HYPER);
  }

  if (not icon)
    return;

  that->register_application(pid, icon);
}



void
PrettyTable::register_application(pid_t pid, Glib::RefPtr<Gdk::Pixbuf> icon)
{
  /* If process already exists then set the icon. Otherwise put into hash
  ** table to be added later */
  if (ProcInfo* info = ProcInfo::find(pid))
    {
      info->set_icon(icon);
      // move the ref to the map
      this->apps[pid] = icon;
      procman_debug("WNCK OK for %u", unsigned(pid));
    }
}



void
PrettyTable::on_application_closed(WnckScreen* screen, WnckApplication* app, gpointer data)
{
  pid_t pid = wnck_application_get_pid(app);

  if (pid == 0)
    return;

  static_cast<PrettyTable*>(data)->unregister_application(pid);
}



void
PrettyTable::unregister_application(pid_t pid)
{
  IconsForPID::iterator it(this->apps.find(pid));

  if (it != this->apps.end())
    this->apps.erase(it);
}



Glib::RefPtr<Gdk::Pixbuf>
PrettyTable::get_icon_from_theme(const ProcInfo &info)
{
  return this->theme->load_icon(info.name, APP_ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
}


bool PrettyTable::get_default_icon_name(const string &cmd, string &name)
{
  for (size_t i = 0; i != G_N_ELEMENTS(default_table); ++i) {
    if (default_table[i].command->match(cmd)) {
      name = default_table[i].icon;
      return true;
    }
  }

  return false;
}

/*
  Try to get an icon from the default_table
  If it's not in defaults, try to load it.
  If there is no default for a command, store NULL in defaults
  so we don't have to lookup again.
*/

Glib::RefPtr<Gdk::Pixbuf>
PrettyTable::get_icon_from_default(const ProcInfo &info)
{
  Glib::RefPtr<Gdk::Pixbuf> pix;
  string name;

  if (this->get_default_icon_name(info.name, name)) {
    IconCache::iterator it(this->defaults.find(name));

    if (it == this->defaults.end()) {
      pix = this->theme->load_icon(name, APP_ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
      if (pix)
	this->defaults[name] = pix;
    } else
      pix = it->second;
  }

  return pix;
}



Glib::RefPtr<Gdk::Pixbuf>
PrettyTable::get_icon_from_wnck(const ProcInfo &info)
{
  Glib::RefPtr<Gdk::Pixbuf> icon;

  IconsForPID::iterator it(this->apps.find(info.pid));

  if (it != this->apps.end())
    icon = it->second;

  return icon;
}



Glib::RefPtr<Gdk::Pixbuf>
PrettyTable::get_icon_from_name(const ProcInfo &info)
{
  return this->theme->load_icon(info.name, APP_ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
}


Glib::RefPtr<Gdk::Pixbuf>
PrettyTable::get_icon_dummy(const ProcInfo &)
{
  return this->theme->load_icon("application-x-executable", APP_ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
}


namespace
{
  bool has_kthreadd()
  {
    glibtop_proc_state buf;
    glibtop_get_proc_state(&buf, 2);

    return buf.cmd == string("kthreadd");
  }

  // @pre: has_kthreadd
  bool is_kthread(const ProcInfo &info)
  {
    return info.pid == 2 or info.ppid == 2;
  }
}


Glib::RefPtr<Gdk::Pixbuf>
PrettyTable::get_icon_for_kernel(const ProcInfo &info)
{
  if (is_kthread(info))
    return this->theme->load_icon("applications-system", APP_ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);

  return Glib::RefPtr<Gdk::Pixbuf>();
}



void
PrettyTable::set_icon(ProcInfo &info)
{
  typedef Glib::RefPtr<Gdk::Pixbuf>
    (PrettyTable::*Getter)(const ProcInfo &);

  static std::vector<Getter> getters;

  if (getters.empty())
    {
      getters.push_back(&PrettyTable::get_icon_from_wnck);
      getters.push_back(&PrettyTable::get_icon_from_theme);
      getters.push_back(&PrettyTable::get_icon_from_default);
      getters.push_back(&PrettyTable::get_icon_from_name);
      if (has_kthreadd())
	{
	  procman_debug("kthreadd is running with PID 2");
	  getters.push_back(&PrettyTable::get_icon_for_kernel);
	}
      getters.push_back(&PrettyTable::get_icon_dummy);
    }

  Glib::RefPtr<Gdk::Pixbuf> icon;

  for (size_t i = 0; not icon and i < getters.size(); ++i) {
    try {
      icon = (this->*getters[i])(info);
    }
    catch (std::exception& e) {
      g_warning("Failed to load icon for %s(%u) : %s", info.name, info.pid, e.what());
      continue;
    }
    catch (Glib::Exception& e) {
      g_warning("Failed to load icon for %s(%u) : %s", info.name, info.pid, e.what().c_str());
      continue;
    }
  }

  info.set_icon(icon);
}

