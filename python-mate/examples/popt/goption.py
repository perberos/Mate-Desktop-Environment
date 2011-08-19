#!/usr/bin/python

"""
http://live.mate.org/MateGoals/PoptGOption

Since MATE 2.10, GLib provides GOption, a commandline option parser.
The help output of your program will be much nicer :-) And it will enable us to
slowly get rid of popt (even if libmate will still have to depend on it for
compatibility reasons). 

"""

import sys

import glib
import mate

def callback(name, value, group):
    if name == "--example":
        print "example got %s" % value
    elif name in ("-o", "--option"):
        print "option"
    else:
        print "remaining:", value

group = glib.OptionGroup(None, None, None, callback)
group.add_entries([("example", "\0", 0, "An example option",
                    "option"),
                   ("option", "o", glib.OPTION_FLAG_NO_ARG, "An option",
                    None),
                   (glib.OPTION_REMAINING, "\0", 0, "", None),
                  ])
context = glib.OptionContext("argument")
context.set_main_group(group)

prog = mate.init("myprog", "1.0", argv=sys.argv, option_context=context)
