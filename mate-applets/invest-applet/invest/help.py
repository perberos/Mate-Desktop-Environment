# -*- coding: utf-8 -*-
import gtk, gtk.gdk

def show_help():
	gtk.show_uri(None, "ghelp:invest-applet", gtk.gdk.CURRENT_TIME)

def show_help_section(id):
	gtk.show_uri(None, "ghelp:invest-applet?%s" % id, gtk.gdk.CURRENT_TIME)
