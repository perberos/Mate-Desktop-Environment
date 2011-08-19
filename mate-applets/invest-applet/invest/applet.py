import os, time
from os.path import *
import mateapplet, gtk, gtk.gdk, mateconf, gobject
gobject.threads_init()
from gettext import gettext as _
import mateconf

import invest, invest.about, invest.chart, invest.preferences, invest.defs
from invest.quotes import QuoteUpdater
from invest.widgets import *

gtk.window_set_default_icon_from_file(join(invest.ART_DATA_DIR, "invest_neutral.svg"))

class InvestApplet:
	def __init__(self, applet):
		self.applet = applet
		self.applet.setup_menu_from_file (
			None, "Invest_Applet.xml",
			None, [("About", self.on_about), 
					("Help", self.on_help),
					("Prefs", self.on_preferences),
					("Refresh", self.on_refresh)
					])

		evbox = gtk.HBox()
		self.applet_icon = gtk.Image()
		self.set_applet_icon(0)
		self.applet_icon.show()
		evbox.add(self.applet_icon)
		self.applet.add(evbox)
		self.applet.connect("button-press-event",self.button_clicked)
		self.applet.show_all()
		self.new_ilw()

	def new_ilw(self):
		self.quotes_updater = QuoteUpdater(self.set_applet_icon,
						   self.set_applet_tooltip)
		self.investwidget = InvestWidget(self.quotes_updater)
		self.ilw = InvestmentsListWindow(self.applet, self.investwidget)

	def reload_ilw(self):
		self.ilw.destroy()
		self.new_ilw()

	def button_clicked(self, widget,event):
		if event.type == gtk.gdk.BUTTON_PRESS and event.button == 1:
			# Three cases...
			if len (invest.STOCKS) == 0:
				# a) We aren't configured yet
				invest.preferences.show_preferences(self, _("<b>You have not entered any stock information yet</b>"))
				self.reload_ilw()
			elif not self.quotes_updater.quotes_valid:
				# b) We can't get the data (e.g. offline)
				alert = gtk.MessageDialog(buttons=gtk.BUTTONS_CLOSE)
				alert.set_markup(_("<b>No stock quotes are currently available</b>"))
				alert.format_secondary_text(_("The server could not be contacted. The computer is either offline or the servers are down. Try again later."))
				alert.run()
				alert.destroy()
			else:
				# c) Everything is normal: pop-up the window
				self.ilw.toggle_show()
	
	def on_about(self, component, verb):
		invest.about.show_about()
	
	def on_help(self, component, verb):
		invest.help.show_help()

	def on_preferences(self, component, verb):
		invest.preferences.show_preferences(self)
		self.reload_ilw()
	
	def on_refresh(self, component, verb):
		self.quotes_updater.refresh()

	def set_applet_icon(self, change):
		if change == 1:
			pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(join(invest.ART_DATA_DIR, "invest-22_up.png"), -1,-1)
		elif change == 0:
			pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(join(invest.ART_DATA_DIR, "invest-22_neutral.png"), -1,-1)
		else:
			pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(join(invest.ART_DATA_DIR, "invest-22_down.png"), -1,-1)
		self.applet_icon.set_from_pixbuf(pixbuf)
	
	def set_applet_tooltip(self, text):
		self.applet_icon.set_tooltip_text(text)

class InvestmentsListWindow(gtk.Window):
	def __init__(self, applet, list):
		gtk.Window.__init__(self, gtk.WINDOW_TOPLEVEL)
		self.set_type_hint(gtk.gdk.WINDOW_TYPE_HINT_DOCK)
		self.stick()
		self.set_resizable(False)
		self.set_border_width(6)
		
		self.applet = applet # this is the widget we want to align with
		self.alignment = self.applet.get_orient ()
		
		self.add(list)
		list.show()

		# boolean variable that identifies if the window is visible
		# show/hide is triggered by left-clicking on the applet
		self.hidden = True

	def toggle_show(self):
		if self.hidden == True:
			self.update_position()
			self.show()
			self.hidden = False
		elif self.hidden == False:
			self.hide()
			self.hidden = True

	def update_position (self):
		"""
		Calculates the position and moves the window to it.
		"""
		self.realize()

		# Get our own dimensions & position
		#(wx, wy) = self.get_origin()
		(ax, ay) = self.applet.window.get_origin()

		(ww, wh) = self.get_size ()
		(aw, ah) = self.applet.window.get_size ()

		screen = self.applet.window.get_screen()
		monitor = screen.get_monitor_geometry (screen.get_monitor_at_window (self.applet.window))

		if self.alignment == mateapplet.ORIENT_LEFT:
				x = ax - ww
				y = ay

				if (y + wh > monitor.y + monitor.height):
					y = monitor.y + monitor.height - wh

				if (y < 0):
					y = 0
				
				if (y + wh > monitor.height / 2):
					gravity = gtk.gdk.GRAVITY_SOUTH_WEST
				else:
					gravity = gtk.gdk.GRAVITY_NORTH_WEST
					
		elif self.alignment == mateapplet.ORIENT_RIGHT:
				x = ax + aw
				y = ay

				if (y + wh > monitor.y + monitor.height):
					y = monitor.y + monitor.height - wh
				
				if (y < 0):
					y = 0
				
				if (y + wh > monitor.height / 2):
					gravity = gtk.gdk.GRAVITY_SOUTH_EAST
				else:
					gravity = gtk.gdk.GRAVITY_NORTH_EAST

		elif self.alignment == mateapplet.ORIENT_DOWN:
				x = ax
				y = ay + ah

				if (x + ww > monitor.x + monitor.width):
					x = monitor.x + monitor.width - ww

				if (x < 0):
					x = 0

				gravity = gtk.gdk.GRAVITY_NORTH_WEST
		elif self.alignment == mateapplet.ORIENT_UP:
				x = ax
				y = ay - wh

				if (x + ww > monitor.x + monitor.width):
					x = monitor.x + monitor.width - ww

				if (x < 0):
					x = 0

				gravity = gtk.gdk.GRAVITY_SOUTH_WEST

		self.move(x, y)
		self.set_gravity(gravity)
