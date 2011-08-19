#include <config.h>

#include <gtkmm/icontheme.h>

#include "iconthemewrapper.h"


Glib::RefPtr<Gdk::Pixbuf>
procman::IconThemeWrapper::load_icon(const Glib::ustring& icon_name,
				     int size, Gtk::IconLookupFlags flags) const
{
  try
    {
      return Gtk::IconTheme::get_default()->load_icon(icon_name, size, flags);
    }
  catch (Gtk::IconThemeError &error)
    {
      if (error.code() != Gtk::IconThemeError::ICON_THEME_NOT_FOUND)
	g_error("Cannot load icon '%s' from theme: %s", icon_name.c_str(), error.what().c_str());
      return Glib::RefPtr<Gdk::Pixbuf>();
    }
}

