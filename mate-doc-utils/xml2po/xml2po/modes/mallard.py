# -*- coding: utf-8 -*-
# Copyright (c) 2004, 2005, 2006 Danilo Segan <danilo@gnome.org>.
# Copyright (c) 2009 Shaun McCance <shaunm@gnome.org>
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

# This implements special instructions for handling DocBook XML documents
# in a better way.
#
#  This means:
#   — better handling of nested complicated tags (i.e. definitions of
#     ignored-tags and final-tags)
#   — support for merging translator-credits back into DocBook articles
#   — support for setting a language
#

import re
import libxml2
import os
import sys
try:
    # Hashlib is new in Python 2.5
    from hashlib import md5 as md5_new
except ImportError:
    from md5 import new as md5_new

from basic import basicXmlMode

class mallardXmlMode(basicXmlMode):
    """Class for special handling of Mallard document types."""
    def __init__(self):
        pass

    def isBlockContext(self, node):
        if node.type == 'element' and node.parent.type == 'element':
            if node.parent.name in ('section', 'example', 'comment', 'figure',
                                    'listing', 'note', 'quote', 'synopsis'):
                return True
            elif node.parent.name == 'media':
                return self.isBlockContext(node.parent)
        return False

    def isFinalNode(self, node):
        if node.type == 'element':
            # Always block
            if node.name in ('p', 'screen', 'title', 'desc', 'cite', 'item'):
                return True
            # Always inline
            elif node.name in ('app', 'cmd', 'em', 'file', 'gui', 'guiseq', 'input',
                               'key', 'keyseq', 'output', 'span', 'sys', 'var'):
                return False
            # Block or inline
            elif node.name in ('code', 'media'):
                return self.isBlockContext(node)
            # Inline or info
            elif node.name == 'link':
                return node.parent.name == 'info'
        return False

    def getIgnoredTags(self):
        "Returns array of tags to be ignored."
        return []

    def getFinalTags(self):
        "Returns array of tags to be considered 'final'."
        return []

    def getSpacePreserveTags(self):
        "Returns array of tags in which spaces are to be preserved."
        return ['code', 'screen']

    def getStringForTranslators(self):
        """Returns string which will be used to credit translators."""
        return "translator-credits"

    def getCommentForTranslators(self):
        """Returns a comment to be added next to string for crediting translators."""
        return """Put one translator per line, in the form of NAME <EMAIL>, YEAR1, YEAR2"""

    def _md5_for_file(self, filename):
        hash = md5_new()
        input = open(filename, "rb")
        read = input.read(4096)
        while read:
            hash.update(read)
            read = input.read(4096)
        input.close()
        return hash.hexdigest()

    def _output_images(self, node, msg):
        if node and node.type=='element' and node.name=='media':
            attr = node.prop("src")
            if attr:
                dir = os.path.dirname(msg.filename)
                fullpath = os.path.join(dir, attr)
                if os.path.exists(fullpath):
                    hash = self._md5_for_file(fullpath)
                else:
                    hash = "THIS FILE DOESN'T EXIST"
                    print >>sys.stderr, "Warning: image file '%s' not found." % fullpath
                    
                msg.outputMessage("@@image: '%s'; md5=%s" % (attr, hash), node.lineNo(),
                                  "When image changes, this message will be marked fuzzy or untranslated for you.\n"+
                                  "It doesn't matter what you translate it to: it's not used at all.")
        if node and node.children:
            child = node.children
            while child:
                self._output_images(child,msg)
                child = child.next


    def preProcessXml(self, doc, msg):
        """Add additional messages of interest here."""
        root = doc.getRootElement()
        self._output_images(root,msg)

    def postProcessXmlTranslation(self, doc, language, translators):
        # FIXME: add translator credits
        return
