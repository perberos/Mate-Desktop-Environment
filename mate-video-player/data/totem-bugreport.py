#!/usr/bin/python
import gtk
import os
from datetime import datetime

# Get the GStreamer version
if os.system ('gst-typefind-0.10 --version') == 0:
	# List the formats of the last files played
	last_visited = 0
	recent_manager = gtk.recent_manager_get_default ()
	for recent in recent_manager.get_items ():
		if recent.has_group ("Totem"):
			if recent.get_visited () > last_visited:
				last_visited = recent.get_visited ()
				last = recent.get_uri_display ()

	if last != None:
		file_handle = os.popen ('gst-typefind-0.10 "%s"' % (last))
		parts = file_handle.read ().split (' ')
		date = datetime.fromtimestamp (last_visited)
		print 'Listened to a "%s" file on %s' % (parts.pop ().strip (), date.isoformat ())
