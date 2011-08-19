# -*- coding: utf-8 -*-
from os.path import join
from gettext import gettext as _
from invest.defs import VERSION
import invest
import gtk, gtk.gdk

invest_logo = None
try:
	invest_logo = gtk.gdk.pixbuf_new_from_file_at_size(join(invest.ART_DATA_DIR, "invest_neutral.svg"), 96, 96)
except Exception, msg:
	pass
	
def show_about():
	about = gtk.AboutDialog()
	infos = {
		"program-name" : _("Invest"),
		"logo" : invest_logo,
		"version" : VERSION,
		"comments" : _("Track your invested money."),
		"copyright" : "Copyright © 2004-2005 Raphael Slinckx.\nCopyright © 2009-2010 Enrico Minack."
	}

	about.set_authors(["Raphael Slinckx <raphael@slinckx.net>", "Enrico Minack <enrico-minack@gmx.de>"])
#	about.set_artists([])
#	about.set_documenters([])
	
	#translators: These appear in the About dialog, usual format applies.
	about.set_translator_credits( _("translator-credits") )
	
	for prop, val in infos.items():
		about.set_property(prop, val)
	
	about.connect ("response", lambda self, *args: self.destroy ())
	about.show_all()
