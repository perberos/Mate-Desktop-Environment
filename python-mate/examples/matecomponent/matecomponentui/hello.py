#!/usr/bin/env python
#
# hello.py
#
# A hello world application using the MateComponent UI handler
#
# Original Authors:
#      Michael Meeks    <michael@ximian.com>
#      Murray Cumming   <murrayc@usa.net>
#      Havoc Pennington <hp@redhat.com>
#
# Converted to Python by:
#      Johan Dahlin     <jdahlin@telia.com>
#
    
import sys
import matecomponent
import matecomponent.ui
import gtk

HELLO_UI_XML = "MateComponent_Sample_Hello.xml"

# Keep a list of all open application windows
app_list = []

def strreverse (text):
    l = list (text)
    l.reverse ()
    return ''.join (l)

def show_nothing_dialog (widget):
    dialog = gtk.MessageDialog (widget,
                                gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
				gtk.MESSAGE_INFO, gtk.BUTTONS_OK,
				'This does nothing; it is only a demonstration')
    dialog.run ()
    dialog.destroy ()
    
def hello_on_menu_file_new (uic, verbname, win):
    hello = hello_new ()
    hello.show_all ()
    
def hello_on_menu_file_open (uic, verbname, win):
    show_nothing_dialog (win)
    
def hello_on_menu_file_save (uic, verbname, win):
    show_nothing_dialog (win)
    
def hello_on_menu_file_saveas (uic, verbname, win):
    show_nothing_dialog (win)
    
def hello_on_menu_file_exit (uic, verbname, win):
    sys.exit (0)

def hello_on_menu_file_close (uic, verbname, win):
    app_list.remove (app)
    app.destroy ()
    if not app_list:
	hello_on_menu_file_exit (uic, verbname, win)

def hello_on_menu_edit_undo (uic, verbname, win):
    show_nothing_dialog (win)    
    
def hello_on_menu_edit_redo (uic, verbname, win):
    show_nothing_dialog (win)        
    
def hello_on_menu_help_about (uic, verbname, win):
    dialog = gtk.MessageDialog (win,
                                gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
				gtk.MESSAGE_INFO, gtk.BUTTONS_OK,
				'MateComponentUI-Hello')
    dialog.run ()
    dialog.destroy ()    
    
def hello_on_button_click (w, label):
    text = label.get_text ()
    label.set_text (strreverse (text))

# These verb names are standard, see limatecomponentbui/doc/std-ui.xml
# to find a list of standard verb names.
# The menu items are specified in MateComponent_Sample_Hello.xml and
# given names which map to these verbs here.

hello_verbs = [
    ('FileNew',    hello_on_menu_file_new),
    ('FileOpen',   hello_on_menu_file_open),
    ('FileSave',   hello_on_menu_file_save),
    ('FileSaveAs', hello_on_menu_file_saveas),
    ('FileClose',  hello_on_menu_file_close),
    ('FileExit',   hello_on_menu_file_exit),
    ('EditUndo',   hello_on_menu_edit_undo),
    ('EditRedo',   hello_on_menu_edit_redo),    
    ('HelpAbout',  hello_on_menu_help_about)
]
    
def hello_create_main_window ():
    window = matecomponent.ui.Window ('Title', 'test')
    window.show_all ()

    ui_container = window.get_ui_container ()
    engine = window.get_ui_engine ()
    engine.config_set_path ('/hello-app/UIConfig/kvps')
    ui_component = matecomponent.ui.Component ('test')
    ui_component.set_container (ui_container.corba_objref ())

    matecomponent.ui.util_set_ui (ui_component, '',
                           HELLO_UI_XML,
		           'matecomponent-hello')
			   
    ui_component.add_verb_list (hello_verbs, window)
    return window

def delete_event_cb (window, event):
    return gtk.TRUE

def hello_new ():
    win = hello_create_main_window ()
    
    button = gtk.Button ()
    button.set_border_width (10)
    
    label = gtk.Label ('Hello World')
    button.add (label)
    button.connect ('clicked', hello_on_button_click, label)
    
    win.set_size_request (250, 350)
    win.set_resizable (gtk.TRUE)
    win.set_property ('allow-shrink', gtk.FALSE)
    
    frame = gtk.Frame ()
    frame.set_shadow_type (gtk.SHADOW_IN)
    frame.add (button)
    win.set_contents (frame)
    
    win.connect ('delete_event', delete_event_cb)
    
    app_list.append (win)
    
    return win

if __name__ == '__main__':
    app = hello_new ()
    app.show_all ()
    matecomponent.main ()
