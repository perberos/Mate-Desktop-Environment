#!/usr/bin/env python

# Author: Jesper Skov <jskov@cygnus.co.uk>
# A rewite of the C canvas example in the MATE Developer's Information

import gtk
import matecanvas
from random import random

def mainquit(*args):
    gtk.main_quit()

class CanvasExample:
    def __init__(self):

        self.width = 400
        self.height = 400

        self.all = []

        self.colors = ("red",
                       "yellow",
                       "green",
                       "cyan",
                       "blue",
                       "magenta")


    def change_item_color(self, item):
        # Pick a random color from the list.
        n = int(random() * len(self.colors)) - 1
        item.set(fill_color = self.colors[n])


    def item_event(self, widget, event=None):
        if event.type == gtk.gdk.BUTTON_PRESS:
            if event.button == 1:
                # Remember starting position.
                self.remember_x = event.x
                self.remember_y = event.y
                return True

            elif event.button == 3:
                # Destroy the item.
                widget.destroy()
                return True

        elif event.type == gtk.gdk._2BUTTON_PRESS:
            #Change the item's color.
            self.change_item_color(widget)
            return True

        elif event.type == gtk.gdk.MOTION_NOTIFY:
            if event.state & gtk.gdk.BUTTON1_MASK:
                # Get the new position and move by the difference
                new_x = event.x
                new_y = event.y

                widget.move(new_x - self.remember_x, new_y - self.remember_y)

                self.remember_x = new_x
                self.remember_y = new_y

                return True
            
        elif event.type == gtk.gdk.ENTER_NOTIFY:
            # Make the outline wide.
            widget.set(width_units=3)
            return True

        elif event.type == gtk.gdk.LEAVE_NOTIFY:
            # Make the outline thin.
            widget.set(width_units=1)
            return True

        return False


    def add_object_clicked(self, widget, event=None):
        x1 = random() * self.width
        y1 = random() * self.height
        x2 = random() * self.width
        y2 = random() * self.height

        if x1 > x2:
            x2,x1 = x1,x2
        if y1 > y2:
            y2,y1 = y1,y2

        if (x2 - x1) < 10:
            x2 = x2 + 10

        if (y2 - y1) < 10:
            y2 = y2 + 10

        if (random() > .5):
            type = matecanvas.CanvasRect
        else:
            # Text names should work as well...
            #type = matecanvas.CanvasEllipse
            type = 'MateCanvasEllipse'

        w = self.acanvas.root().add(type, x1=x1, y1=y1, x2=x2, y2=y2, 
                                   fill_color='white', outline_color='black',
                                   width_units=1.0)
        w.connect("event", self.item_event)

        self.all.append(w)

    def main(self):
        # Open window to hold canvas.
        win = gtk.Window()
        win.connect('destroy', mainquit)
        win.set_title('Canvas Example')

        # Create VBox to hold canvas and buttons.
        vbox = gtk.VBox()
        win.add(vbox)
        vbox.show()

	# Some instructions for people using the example:
	label = gtk.Label("Drag - move object.\n" +
			 "Double click - change colour\n" +
			 "Right click - delete object")
	vbox.pack_start(label, expand=False)
	label.show()

        # Create canvas.
        self.acanvas = matecanvas.Canvas(aa=True)
        self.acanvas.set_size_request(self.width, self.height)
        self.acanvas.set_scroll_region(0,0, self.width, self.height)
        vbox.pack_start(self.acanvas)
        self.acanvas.show()

        # Create buttons.
        hbox = gtk.HBox()
        vbox.pack_start(hbox, expand=False)
        hbox.show()

        b = gtk.Button("Add an object")
        b.connect("clicked", self.add_object_clicked)
        hbox.pack_start(b)
        b.show()

        b = gtk.Button("Quit")
        b.connect("clicked", mainquit)
        hbox.pack_start(b)
        b.show()

        win.show()

if __name__ == '__main__':
    c = CanvasExample()
    c.main()
    gtk.main()
