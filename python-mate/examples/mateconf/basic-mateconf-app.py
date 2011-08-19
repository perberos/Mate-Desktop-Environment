#!/usr/bin/env python
#
# This program demonstrates how to use MateConf.  The key thing is that
# the main window and the prefs dialog have NO KNOWLEDGE of one
# another as far as configuration values are concerned; they don't
# even have to be in the same process. That is, the MateConfClient acts
# as the data "model" for configuration information; the main
# application is a "view" of the model; and the prefs dialog is a
# "controller."
#
# You can tell if your application has done this correctly by
# using "mateconftool" instead of your preferences dialog to set
# preferences. For example:
# 
# mateconftool --type=string --set /apps/basic-mateconf-app/foo "My string"
# 
# If that doesn't work every bit as well as setting the value
# via the prefs dialog, then you aren't doing things right. ;-)
#
#
# If you really want to be mean to your app, make it survive
# this:
# 
# mateconftool --break-key /apps/basic-mateconf-app/foo
# 
# Remember, the MateConf database is just like an external file or
# the network - it may have bogus values in it. MateConf admin
# tools will let people put in whatever they can think of.
# 
# MateConf does guarantee that string values will be valid UTF-8, for
# convenience.
# 

# Throughout, this program is letting MateConfClient use its default
# error handlers rather than checking for errors or attaching custom
# handlers to the "unreturned_error" signal. Thus the last arg to
# MateConfClient functions is None.
#

# Special mention of an idiom often used in GTK+ apps that does
# not work right with MateConf but may appear to at first:
#
# i_am_changing_value = True
# change_value (value)
# i_am_changing_value = False
# 
# This breaks for several reasons: notification of changes
# may be asynchronous, you may get notifications that are not
# caused by change_value () while change_value () is running,
# since MateConf will enter the main loop, and also if you need
# this code to work you are probably going to have issues
# when someone other than yourself sets the value.
# 
# A robust solution in this case is often to compare the old
# and new values to see if they've really changed, thus avoiding
# whatever loop you were trying to avoid.
#

import mateconf
import gtk

def main ():
  # Get the default client
  client = mateconf.client_get_default ();

  # Tell MateConfClient that we're interested in the given directory.
  # This means MateConfClient will receive notification of changes
  # to this directory, and cache keys under this directory.
  # So _don't_ add "/" or something silly like that or you'll end
  # up with a copy of the whole MateConf database. ;-)
  #
  # We use PRELOAD_NONE to avoid loading all config keys on
  # startup. If your app pretty much reads all config keys
  # on startup, then preloading the cache may make sense.
   
  client.add_dir ("/apps/basic-mateconf-app",
                  mateconf.CLIENT_PRELOAD_NONE)

  main_window = create_main_window (client)
  main_window.show_all ()

  gtk.main ()

  # This ensures we cleanly detach from the MateConf server (assuming
  # we hold the last reference). It's purely a bit of cleanliness,
  # the server does survive fine if we crash.

  #client.unref ()


# Quit app when window is destroyed 
def destroy_callback (widget, *data):
    gtk.main_quit ()

    
# Remove the notification callback when the widget monitoring
# notifications is destroyed

def configurable_widget_destroy_callback (widget):
    client = widget.get_data ('client')
    notify_id = widget.get_data ('notify_id')

    if notify_id:
	client.notify_remove (notify_id)

	
# Notification callback for our label widgets that
# monitor the current value of a mateconf key. i.e.
# we are conceptually "configuring" the label widgets

def configurable_widget_config_notify (client, cnxn_id, entry, label):
    
    # Note that value can be None (unset) or it can have
    # the wrong type! Need to check that to survive
    # mateconftool --break-key
  
    if not entry.value:
	label.set_text ('')
    elif entry.value.type == mateconf.VALUE_STRING:
	label.set_text (entry.value.to_string ())
    else:
	label.set_text ('!type error!')
	
	
# Create a GtkLabel inside a frame, that we can "configure"
# (the label displays the value of the config key).

def create_configurable_widget (client, config_key):
    frame = gtk.Frame (config_key)
    label = gtk.Label ('')
    frame.add (label)
  
    s = client.get_string (config_key)

    if s:
	label.set_text (s)

    notify_id = client.notify_add (config_key,
                                   configurable_widget_config_notify,
				   label)

    # Note that notify_id will be 0 if there was an error,
    # so we handle that in our destroy callback.
  
    label.set_data ('notify_id', notify_id)
    label.set_data ('client', client)
    label.connect ('destroy', configurable_widget_destroy_callback)
    
    return frame


def prefs_dialog_destroyed (dialog, main_window):
    main_window.set_data ('prefs', None)

# prefs button clicked 
def prefs_clicked (button, main_window):

    prefs_dialog = main_window.get_data ('prefs')

    if not prefs_dialog:
	client = main_window.get_data ('client')
	prefs_dialog = create_prefs_dialog (main_window, client)

	main_window.set_data ('prefs', prefs_dialog)

	prefs_dialog.connect ('destroy', prefs_dialog_destroyed, main_window)

	prefs_dialog.show_all ()
    else:
	# show existing dialog
	prefs_dialog.present ()

def create_main_window (client):
    w = gtk.Window ()
    w.set_title ('basic-mateconf-app Main Window')
  
    vbox = gtk.VBox (False, 5)
    vbox.set_border_width (5)
    w.add (vbox)
  
    # Create labels that we can "configure"
    config = create_configurable_widget (client, "/apps/basic-mateconf-app/foo")
    vbox.pack_start (config, True, True)

    config = create_configurable_widget (client, "/apps/basic-mateconf-app/bar")
    vbox.pack_start (config, True, True)
  
    config = create_configurable_widget (client, "/apps/basic-mateconf-app/baz")
    vbox.pack_start (config, True, True)

    config = create_configurable_widget (client, "/apps/basic-mateconf-app/blah");
    vbox.pack_start (config, True, True)

    w.connect ('destroy', destroy_callback)
    w.set_data ('client', client)
  
    prefs = gtk.Button ("Prefs");
    vbox.pack_end ( prefs, False, False)
    prefs.connect ('clicked', prefs_clicked, w)
    
    return w


#
# Preferences dialog code. NOTE that the prefs dialog knows NOTHING
# about the existence of the main window; it is purely a way to fool
# with the MateConf database. It never does something like change
# the main window directly; it ONLY changes MateConf keys via
# MateConfClient. This is _important_, because people may configure
# your app without using your preferences dialog.
#
# This is an instant-apply prefs dialog. For a complicated
# apply/revert/cancel dialog as in MATE 1, see the
# complex-mateconf-app.c example. But don't actually copy that example
# in MATE 2, thanks. ;-) complex-mateconf-app.c does show how
# to use MateConfChangeSet.
#


# Commit changes to the MateConf database. 
def config_entry_commit (entry, *args):
    client = entry.get_data ('client')
    text = entry.get_chars (0, -1)

    key = entry.get_data ('key')

    # Unset if the string is zero-length, otherwise set
    if text:
	client.set_string (key, text)
    else:
	client.unset (key)
	
# Create an entry used to edit the given config key 
def create_config_entry (prefs_dialog, client, config_key, focus=False):
    hbox = gtk.HBox (False, 5)
    label = gtk.Label (config_key)
    entry = gtk.Entry ()

    hbox.pack_start (label, False, False, 0)
    hbox.pack_end (entry, False, False, 0)

    # this will print an error via default error handler
    # if the key isn't set to a string

    s = client.get_string (config_key)
    if s:
	entry.set_text (s)
  
    entry.set_data ('client', client)
    entry.set_data ('key', config_key)

    # Commit changes if the user focuses out, or hits enter; we don't
    # do this on "changed" since it'd probably be a bit too slow to
    # round-trip to the server on every "changed" signal.

    entry.connect ('focus_out_event', config_entry_commit)
    entry.connect ('activate', config_entry_commit)    
   
    # Set the entry insensitive if the key it edits isn't writable.
    # Technically, we should update this sensitivity if the key gets
    # a change notify, but that's probably overkill.

    entry.set_sensitive (client.key_is_writable (config_key))

    if focus:
	entry.grab_focus ()
  
    return hbox

def create_prefs_dialog (parent, client):
    dialog = gtk.Dialog ("basic-mateconf-app Preferences",
                         parent,
			 0,
			 (gtk.STOCK_CLOSE, gtk.RESPONSE_ACCEPT))

    # destroy dialog on button press
    dialog.connect ('response', lambda wid,ev: wid.destroy ())

    dialog.set_default_response (gtk.RESPONSE_ACCEPT)

    # resizing doesn't grow the entries anyhow
    dialog.set_resizable (False)
  
    vbox = gtk.VBox (False, 5)
    vbox.set_border_width (5)
  
    dialog.vbox.pack_start (vbox)

    entry = create_config_entry (dialog, client, "/apps/basic-mateconf-app/foo", True)
    vbox.pack_start (entry, False, False)

    entry = create_config_entry (dialog, client, "/apps/basic-mateconf-app/bar")
    vbox.pack_start (entry, False, False)
  
    entry = create_config_entry (dialog, client, "/apps/basic-mateconf-app/baz")
    vbox.pack_start (entry, False, False)

    entry = create_config_entry (dialog, client, "/apps/basic-mateconf-app/blah")
    vbox.pack_start (entry, False, False)
  
    return dialog

main ()
