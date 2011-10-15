#!/usr/bin/python -u
# -*- encoding: utf-8 -*-
# Copyright (c) 2004, 2005, 2006 Danilo Šegan <danilo@gnome.org>.
# Copyright (c) 2009 Claude Paroz <claude@2xlibre.net>.
#
# This file is part of xml2po.
#
# xml2po is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# xml2po is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with xml2po; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# xml2po -- translate XML documents
VERSION = "1.0.5"

# Versioning system (I use this for a long time, so lets explain it to
# those Linux-versioning-scheme addicts):
#   1.0.* are unstable, development versions
#   1.1 will be first stable release (release 1), and 1.1.* bugfix releases
#   2.0.* will be unstable-feature-development stage (milestone 1)
#   2.1.* unstable development betas (milestone 2)
#   2.2 second stable release (release 2), and 2.2.* bugfix releases
#   ...
#
import sys
import os
import getopt
import tempfile

NULL_STRING = '/dev/null'
if not os.path.exists('/dev/null'): NULL_STRING = 'NUL'

def usage (with_help = False):
    print >> sys.stderr, "Usage:  %s [OPTIONS] [XMLFILE]..." % (sys.argv[0])
    if with_help:
        print >> sys.stderr, """
OPTIONS may be some of:
    -a    --automatic-tags     Automatically decides if tags are to be considered
                                 "final" or not
    -k    --keep-entities      Don't expand entities
    -e    --expand-all-entities  Expand ALL entities (including SYSTEM ones)
    -m    --mode=TYPE          Treat tags as type TYPE (default: docbook)
    -o    --output=FILE        Print resulting text (XML or POT) to FILE
    -p    --po-file=FILE       Specify PO file containing translation, and merge
    -r    --reuse=FILE         Specify translated XML file with the same structure
    -t    --translation=FILE   Specify MO file containing translation, and merge
    -u    --update-translation=LANG.po   Updates a PO file using msgmerge program

    -l    --language=LANG      Set language of the translation to LANG
          --mark-untranslated  Set 'xml:lang="C"' on untranslated tags

    -v    --version            Output version of the xml2po program

    -h    --help               Output this message

EXAMPLES:
    To create a POTemplate book.pot from input files chapter1.xml and
    chapter2.xml, run the following:
        %(command)s -o book.pot chapter1.xml chapter2.xml

    After translating book.pot into de.po, merge the translations back,
    using -p option for each XML file:
        %(command)s -p de.po chapter1.xml > chapter1.de.xml
        %(command)s -p de.po chapter2.xml > chapter2.de.xml
""" % {'command': sys.argv[0]}


def main(argv):
    if not argv:
        usage()
        sys.exit(2)

    name = os.path.join(os.path.dirname(__file__), '..')
    if os.path.exists(os.path.join(name, 'tests')):
        print >> sys.stderr, 'Running from source folder, modifying PYTHONPATH'
        sys.path.insert(0, name)

    from xml2po import Main

    # Default parameters
    default_mode = 'docbook'
    operation = 'pot' # 'pot', 'merge', 'update'
    output  = '-' # this means to stdout
    options = {
        'mark_untranslated'   : False,
        'expand_entities'     : True,
        'expand_all_entities' : False,
    }
    origxml = ''
    mofile = None
    mofile_tmppath = None

    try: opts, remaining_args = getopt.getopt(argv, 'avhkem:t:o:p:u:r:l:',
                               ['automatic-tags','version', 'help', 'keep-entities', 'expand-all-entities', 'mode=', 'translation=',
                                'output=', 'po-file=', 'update-translation=', 'reuse=', 'language=', 'mark-untranslated' ])
    except getopt.GetoptError:
        usage(True)
        sys.exit(2)

    for opt, arg in opts:
        if opt in ('-m', '--mode'):
            default_mode = arg
        if opt in ('-a', '--automatic-tags'):
            default_mode = 'basic'
        elif opt in ('-k', '--keep-entities'):
            options['expand_entities'] = False
        elif opt in ('--mark-untranslated',):
            options['mark_untranslated'] = True
        elif opt in ('-e', '--expand-all-entities'):
            options['expand_all_entities'] = True
        elif opt in ('-l', '--language'):
            options['translationlanguage'] = arg
        elif opt in ('-t', '--translation'):
            mofile = arg
            operation = 'merge'
            if 'translationlanguage' not in options:
                options['translationlanguage'] = os.path.split(os.path.splitext(mofile)[0])[1]
        elif opt in ('-r', '--reuse'):
            origxml = arg
        elif opt in ('-u', '--update-translation'):
            operation = 'update'
            po_to_update = arg
        elif opt in ('-p', '--po-file'):
            mofile_handle, mofile_tmppath = tempfile.mkstemp()
            os.close(mofile_handle)
            pofile = arg
            operation = 'merge'
            if 'translationlanguage' not in options:
                options['translationlanguage'] = os.path.split(os.path.splitext(pofile)[0])[1]
            os.system("msgfmt -o %s %s >%s" % (mofile_tmppath, pofile, NULL_STRING)) and sys.exit(7)
            mofile = mofile_tmppath
        elif opt in ('-o', '--output'):
            output = arg
        elif opt in ('-v', '--version'):
            print VERSION
            sys.exit(0)
        elif opt in ('-h', '--help'):
            usage(True)
            sys.exit(0)

    if operation == 'update' and output != "-":
        print >> sys.stderr, "Option '-o' is not yet supported when updating translations directly. Ignoring this option."

    # Treat remaining arguments as XML files
    filenames = []
    while remaining_args:
        filenames.append(remaining_args.pop())

    try:
        xml2po_main = Main(default_mode, operation, output, options)
    except IOError:
        print >> sys.stderr, "Error: cannot open file %s for writing." % (output)
        sys.exit(5)

    if operation == 'merge':
        if len(filenames) > 1:
            print  >> sys.stderr, "Error: You can merge translations with only one XML file at a time."
            sys.exit(2)

        if not mofile:
            print >> sys.stderr, "Error: You must specify MO file when merging translations."
            sys.exit(3)

        xml2po_main.merge(mofile, filenames[0])

    elif operation == 'update':
        xml2po_main.update(filenames, po_to_update)

    elif origxml:
        xml2po_main.reuse(origxml, filenames[0])

    else:
        # Standard POT producing
        xml2po_main.to_pot(filenames)

    if mofile_tmppath:
        os.remove(mofile_tmppath)

# Main program start
if __name__ == '__main__':
    main(sys.argv[1:])
else:
    raise NotImplementedError
