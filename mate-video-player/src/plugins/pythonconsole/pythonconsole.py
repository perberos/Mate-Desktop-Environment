# -*- coding: utf-8 -*-

# pythonconsole.py -- plugin object
#
# Copyright (C) 2006 - Steve Frécinaux
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Parts from "Interactive Python-GTK Console" (stolen from epiphany's console.py)
#     Copyright (C), 1998 James Henstridge <james@daa.com.au>
#     Copyright (C), 2005 Adam Hooper <adamh@densi.com>
# Bits from gedit Python Console Plugin
#     Copyrignt (C), 2005 Raphaël Slinckx
#
# The Totem project hereby grant permission for non-gpl compatible GStreamer
# plugins to be used and distributed together with GStreamer and Totem. This
# permission are above and beyond the permissions granted by the GPL license
# Totem is covered by.
#
# Monday 7th February 2005: Christian Schaller: Add exception clause.
# See license_change file for details.

from console import PythonConsole

__all__ = ('PythonConsole', 'OutFile')

import gtk
import totem
import mateconf
import gobject
try:
	import rpdb2
	have_rpdb2 = True
except:
	have_rpdb2 = False

ui_str = """
<ui>
  <menubar name="tmw-menubar">
    <menu name="Python" action="Python">
      <placeholder name="ToolsOps_5">
        <menuitem name="PythonConsole" action="PythonConsole"/>
        <menuitem name="PythonDebugger" action="PythonDebugger"/>
      </placeholder>
    </menu>
  </menubar>
</ui>
"""

class PythonConsolePlugin(totem.Plugin):
	def __init__(self):
		totem.Plugin.__init__(self)
		self.window = None
	
	def activate(self, totem_object):

		data = dict()
		manager = totem_object.get_ui_manager()

		data['action_group'] = gtk.ActionGroup('Python')
		
		action = gtk.Action('Python', 'Python', _('Python Console Menu'), None)
		data['action_group'].add_action(action)

		action = gtk.Action('PythonConsole', _('_Python Console'),
		                    _("Show Totem's Python console"),
		                    'mate-mime-text-x-python')
		action.connect('activate', self.show_console, totem_object)
		data['action_group'].add_action(action)

		action = gtk.Action('PythonDebugger', _('Python Debugger'),
				    _("Enable remote Python debugging with rpdb2"),
				    None)
		if have_rpdb2:
			action.connect('activate', self.enable_debugging, totem_object)
		else:
			action.set_visible(False)
		data['action_group'].add_action(action)
				
		manager.insert_action_group(data['action_group'], 0)
		data['ui_id'] = manager.add_ui_from_string(ui_str)
		manager.ensure_update()
		
		totem_object.set_data('PythonConsolePluginInfo', data)
	
	def show_console(self, action, totem_object):
		if not self.window:
			console = PythonConsole(namespace = {'__builtins__' : __builtins__,
		        	                             'totem' : totem,
                               		                     'totem_object' : totem_object},
						             destroy_cb = self.destroy_console)

			console.set_size_request(600, 400)
			console.eval('print "%s" %% totem_object' % _("You can access the totem object through " \
				     "\'totem_object\' :\\n%s"), False)

	
			self.window = gtk.Window()
			self.window.set_title(_('Totem Python Console'))
			self.window.add(console)
			self.window.connect('destroy', self.destroy_console)
			self.window.show_all()
		else:
			self.window.show_all()
			self.window.grab_focus()

	def enable_debugging(self, action, totem_object):
		msg = _("After you press OK, Totem will wait until you connect to it with winpdb or rpdb2. If you have not set a debugger password in MateConf, it will use the default password ('totem').")
		dialog = gtk.MessageDialog(None, 0, gtk.MESSAGE_INFO, gtk.BUTTONS_OK_CANCEL, msg)
		if dialog.run() == gtk.RESPONSE_OK:
			mateconfclient = mateconf.client_get_default()
			password = mateconfclient.get_string('/apps/totem/plugins/pythonconsole/rpdb2_password') or "totem"
			def start_debugger(password):
				rpdb2.start_embedded_debugger(password)
				return False

			gobject.idle_add(start_debugger, password)
		dialog.destroy()

	def destroy_console(self, *args):
		self.window.destroy()
		self.window = None

	def deactivate(self, totem_object):
		data = totem_object.get_data('PythonConsolePluginInfo')

		manager = totem_object.get_ui_manager()
		manager.remove_ui(data['ui_id'])
		manager.remove_action_group(data['action_group'])
		manager.ensure_update()

		totem_object.set_data('PythonConsolePluginInfo', None)
		
		if self.window is not None:
			self.window.destroy()
