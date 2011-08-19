#!/usr/bin/env python
#
# A very simple program that sets a single key value when you type
# it in an entry and press return
#

import gtk
import mateconf
   
def entry_activated_callback(entry, client):
    s = entry.get_chars (0, -1)
    client.set_string ("/testing/directory/key", s)

window = gtk.Window()
entry = gtk.Entry ()
window.add (entry)

client = mateconf.client_get_default ()

client.add_dir ("/testing/directory",
                mateconf.CLIENT_PRELOAD_NONE)
entry.connect ('activate', entry_activated_callback, client)

# If key isn't writable, then set insensitive
entry.set_sensitive (client.key_is_writable ("/testing/directory/key"))
window.show_all ()

gtk.main ()



