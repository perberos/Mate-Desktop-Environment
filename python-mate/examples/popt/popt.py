#!/usr/bin/env python
import pygtk; pygtk.require("2.0")
import mate
import sys
import gc

## Notice that [] as default value (see --ddd example below) has a
## special meaning in mate-python (unlike standard popt).  Usually,
## when multiple values are passed for the same option in the command
## line, only the last value is returned to the caller.  In contrast,
## if the programmer specifies [] as default value, then mate-python
## will collect all option values into a list and return this list as
## value to the caller.

# (longName, shortName, type , default, flags, descrip , argDescrip)
table=[("foo"  , 'f'   , int  ,   1     , 0    , "Foobar", "Level"),
       ("aaa"  , 'a'   , str  ,   'a'   , 0    , 'aaa'   , "AAA"),
       ("bbb"  , 'b'   , None ,   None  , 0    , 'bbb'   , "BBB"),
       ("ccc"  , 'c'   , float,   2.5   , 0    , 'ccc'   , "BBB"),
       ("ddd"  , 'd'   , long ,   []    , 0    , 'ddd'   , "DDD"),
       ("eee"  , 'e'   , int,     2     , mate.POPT_ARGFLAG_OR)
       ]

prog = mate.init ("myprog", "1.0", mate.libmate_module_info_get(),
                   sys.argv, table)

print "parsing with mate.init"
leftover_args, argdict = prog.get_popt_args()
print "Leftover: ", leftover_args
print "Argdict: ", argdict

del prog; gc.collect()

print "parsing with mate.pop_parse"
leftover_args, argdict = mate.popt_parse(sys.argv, table)
print "Leftover: ", leftover_args
print "Argdict: ", argdict
