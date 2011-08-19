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




#ifndef _PREFERENCE_TAB_ADDIN_HPP_
#define _PREFERENCE_TAB_ADDIN_HPP_

#include <string>

#include <gtkmm/widget.h>

#include "abstractaddin.hpp"


namespace gnote {

	/// Implement this interface to provide a new tab in
	/// Tomboy's Preferences Dialog.  If you are writing
	/// a standard add-in, DO NOT ABUSE THIS (you should
	/// normally extend the /Tomboy/AddinPreferences
	/// extension point).
	class PreferenceTabAddin 
    : public AbstractAddin
	{
		/// <summary>
		/// Returns a Gtk.Widget to place in a new tab in Tomboy's
		/// preferences dialog.
		/// <param name="parent">The preferences dialog.  Add-ins should
		/// use this for connecting to Hidden or other events as needed.
		/// Another use would be to pop open dialogs, so they can properly
		/// set their parent.
		/// </param>
		/// <param name="tabLabel">The string to be used in the tab's
		/// label.</param>
		/// <param name="preferenceWidget">The Gtk.Widget to use as the
		/// content of the tab page.</param>
		/// <returns>Returns <value>true</value> if the widget is
		/// valid/created or <value>false</value> otherwise.</returns>
		/// </summary>
  public:
    virtual bool get_preference_tab_widget (
									PreferencesDialog * parent,
									std::string  & tabLabel,
									Gtk::Widget * preferenceWidget) = 0;
	};

}

#endif
