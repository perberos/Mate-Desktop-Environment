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
import os
import sys
import re
import subprocess
import tempfile
import gettext
import libxml2

NULL_STRING = '/dev/null'
if not os.path.exists('/dev/null'): NULL_STRING = 'NUL'

# Utility functions
def escapePoString(text):
    return text.replace('\\','\\\\').replace('"', "\\\"").replace("\n","\\n").replace("\t","\\t")

def unEscapePoString(text):
    return text.replace('\\"', '"').replace('\\\\','\\')

class NoneTranslations:
    def gettext(self, message):
        return None

    def lgettext(self, message):
        return None

    def ngettext(self, msgid1, msgid2, n):
        return None

    def lngettext(self, msgid1, msgid2, n):
        return None

    def ugettext(self, message):
        return None

    def ungettext(self, msgid1, msgid2, n):
        return None

class MessageOutput:
    """ Class to abstract po/pot file """
    def __init__(self, app):
        self.app = app
        self.messages = []
        self.comments = {}
        self.linenos = {}
        self.nowrap = {}
        self.translations = []
        self.do_translations = False
        self.output_msgstr = False # this is msgid mode for outputMessage; True is for msgstr mode

    def translationsFollow(self):
        """Indicate that what follows are translations."""
        self.output_msgstr = True

    def setFilename(self, filename):
        self.filename = filename

    def outputMessage(self, text, lineno = 0, comment = None, spacepreserve = False, tag = None):
        """Adds a string to the list of messages."""
        if (text.strip() != ''):
            t = escapePoString(text)
            if self.output_msgstr:
                self.translations.append(t)
                return

            if self.do_translations or (not t in self.messages):
                self.messages.append(t)
                if spacepreserve:
                    self.nowrap[t] = True
                if t in self.linenos.keys():
                    self.linenos[t].append((self.filename, tag, lineno))
                else:
                    self.linenos[t] = [ (self.filename, tag, lineno) ]
                if (not self.do_translations) and comment and not t in self.comments:
                    self.comments[t] = comment
            else:
                if t in self.linenos.keys():
                    self.linenos[t].append((self.filename, tag, lineno))
                else:
                    self.linenos[t] = [ (self.filename, tag, lineno) ]
                if comment and not t in self.comments:
                    self.comments[t] = comment

    def outputHeader(self, out):
        import time
        out.write("""msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\\n"
"POT-Creation-Date: %s\\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"
"Language-Team: LANGUAGE <LL@li.org>\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=UTF-8\\n"
"Content-Transfer-Encoding: 8bit\\n"

""" % (time.strftime("%Y-%m-%d %H:%M%z")))

    def outputAll(self, out):
        self.outputHeader(out)

        for k in self.messages:
            if k in self.comments:
                out.write("#. %s\n" % (self.comments[k].replace("\n","\n#. ")))
            references = ""
            for reference in self.linenos[k]:
                references += "%s:%d(%s) " % (reference[0], reference[2], reference[1])
            out.write("#: %s\n" % (references.strip()))
            if k in self.nowrap and self.nowrap[k]:
                out.write("#, no-wrap\n")
            out.write("msgid \"%s\"\n" % (k))
            translation = ""
            if self.do_translations:
                if len(self.translations)>0:
                    translation = self.translations.pop(0)
            if translation == k:
                translation = ""
            out.write("msgstr \"%s\"\n\n" % (translation))

class XMLDocument(object):
    def __init__(self, filename, app):
        self.app = app
        self.expand_entities = self.app.options.get('expand_entities')
        self.ignored_tags = self.app.current_mode.getIgnoredTags()
        ctxt = libxml2.createFileParserCtxt(filename)
        ctxt.lineNumbers(1)
        if self.app.options.get('expand_all_entities'):
            ctxt.replaceEntities(1)
        ctxt.parseDocument()
        self.doc = ctxt.doc()
        if self.doc.name != filename:
            raise Exception("Error: I tried to open '%s' but got '%s' -- how did that happen?" % (filename, self.doc.name))
        if self.app.msg:
            self.app.msg.setFilename(filename)
        self.isFinalNode = self.app.current_mode.isFinalNode

    def generate_messages(self):
        self.app.msg.setFilename(self.doc.name)
        self.doSerialize(self.doc)

    def normalizeNode(self, node):
        #print >>sys.stderr, "<%s> (%s) [%s]" % (node.name, node.type, node.serialize('utf-8'))
        if not node:
            return
        elif self.app.isSpacePreserveNode(node):
            return
        elif node.isText():
            if node.isBlankNode():
                if self.app.options.get('expand_entities') or \
                  (not (node.prev and not node.prev.isBlankNode() and node.next and not node.next.isBlankNode()) ):
                    #print >>sys.stderr, "BLANK"
                    node.setContent('')
            else:
                node.setContent(re.sub('\s+',' ', node.content))

        elif node.children and node.type == 'element':
            child = node.children
            while child:
                self.normalizeNode(child)
                child = child.next

    def normalizeString(self, text, spacepreserve = False):
        """Normalizes string to be used as key for gettext lookup.

        Removes all unnecessary whitespace."""
        if spacepreserve:
            return text
        try:
            # Lets add document DTD so entities are resolved
            dtd = self.doc.intSubset()
            tmp = dtd.serialize('utf-8')
            tmp = tmp + '<norm>%s</norm>' % text
        except:
            tmp = '<norm>%s</norm>' % text

        try:
            ctxt = libxml2.createDocParserCtxt(tmp)
            if self.app.options.get('expand_entities'):
                ctxt.replaceEntities(1)
            ctxt.parseDocument()
            tree = ctxt.doc()
            newnode = tree.getRootElement()
        except:
            print >> sys.stderr, """Error while normalizing string as XML:\n"%s"\n""" % (text)
            return text

        self.normalizeNode(newnode)

        result = ''
        child = newnode.children
        while child:
            result += child.serialize('utf-8')
            child = child.next

        result = re.sub('^ ','', result)
        result = re.sub(' $','', result)
        tree.freeDoc()

        return result

    def stringForEntity(self, node):
        """Replaces entities in the node."""
        text = node.serialize('utf-8')
        try:
            # Lets add document DTD so entities are resolved
            dtd = self.doc.intSubset()
            tmp = dtd.serialize('utf-8') + '<norm>%s</norm>' % text
            next = True
        except:
            tmp = '<norm>%s</norm>' % text
            next = False

        ctxt = libxml2.createDocParserCtxt(tmp)
        if self.expand_entities:
            ctxt.replaceEntities(1)
        ctxt.parseDocument()
        tree = ctxt.doc()
        if next:
            newnode = tree.children.next
        else:
            newnode = tree.children

        result = ''
        child = newnode.children
        while child:
            result += child.serialize('utf-8')
            child = child.next
        tree.freeDoc()
        return result


    def myAttributeSerialize(self, node):
        result = ''
        if node.children:
            child = node.children
            while child:
                if child.type=='text':
                    result += self.doc.encodeEntitiesReentrant(child.content)
                elif child.type=='entity_ref':
                    if not self.expand_entities:
                        result += '&' + child.name + ';'
                    else:
                        result += child.content.decode('utf-8')
                else:
                    result += self.myAttributeSerialize(child)
                child = child.next
        else:
            result = node.serialize('utf-8')
        return result

    def startTagForNode(self, node):
        if not node:
            return 0

        result = node.name
        params = ''
        if node.properties:
            for p in node.properties:
                if p.type == 'attribute':
                    try:
                        nsprop = p.ns().name + ":" + p.name
                    except:
                        nsprop = p.name
                    params += " %s=\"%s\"" % (nsprop, self.myAttributeSerialize(p))
        return result+params

    def endTagForNode(self, node):
        if not node:
            return False
        return node.name

    def ignoreNode(self, node):
        if self.isFinalNode(node):
            return False
        if node.name in self.ignored_tags or node.type in ('dtd', 'comment'):
            return True
        return False

    def getCommentForNode(self, node):
        """Walk through previous siblings until a comment is found, or other element.

        Only whitespace is allowed between comment and current node."""
        prev = node.prev
        while prev and prev.type == 'text' and prev.content.strip() == '':
            prev = prev.prev
        if prev and prev.type == 'comment':
            return prev.content.strip()
        else:
            return None

    def replaceAttributeContentsWithText(self, node, text):
        node.setContent(text)

    def replaceNodeContentsWithText(self, node, text):
        """Replaces all subnodes of a node with contents of text treated as XML."""

        if node.children:
            starttag = self.startTagForNode(node)
            endtag = self.endTagForNode(node)

            # Lets add document DTD so entities are resolved
            tmp = '<?xml version="1.0" encoding="utf-8" ?>'
            try:
                dtd = self.doc.intSubset()
                tmp = tmp + dtd.serialize('utf-8')
            except libxml2.treeError:
                pass

            content = '<%s>%s</%s>' % (starttag, text, endtag)
            tmp = tmp + content.encode('utf-8')

            newnode = None
            try:
                ctxt = libxml2.createDocParserCtxt(tmp)
                ctxt.replaceEntities(0)
                ctxt.parseDocument()
                newnode = ctxt.doc()
            except:
                pass

            if not newnode:
                print >> sys.stderr, """Error while parsing translation as XML:\n"%s"\n""" % (text.encode('utf-8'))
                return

            newelem = newnode.getRootElement()

            if newelem and newelem.children:
                free = node.children
                while free:
                    next = free.next
                    free.unlinkNode()
                    free = next

                if node:
                    copy = newelem.copyNodeList()
                    next = node.next
                    node.replaceNode(newelem.copyNodeList())
                    node.next = next

            else:
                # In practice, this happens with tags such as "<para>    </para>" (only whitespace in between)
                pass
        else:
            node.setContent(text)

    def autoNodeIsFinal(self, node):
        """Returns True if node is text node, contains non-whitespace text nodes or entities."""
        if hasattr(node, '__autofinal__'):
            return node.__autofinal__
        if node.name in self.ignored_tags:
            node.__autofinal__ = False
            return False
        if node.isText() and node.content.strip()!='':
            node.__autofinal__ = True
            return True
        final = False
        child = node.children
        while child:
            if child.type in ['text'] and  child.content.strip()!='':
                final = True
                break
            child = child.next

        node.__autofinal__ = final
        return final


    def worthOutputting(self, node, noauto = False):
        """Returns True if node is "worth outputting", otherwise False.

        Node is "worth outputting", if none of the parents
        isFinalNode, and it contains non-blank text and entities.
        """
        if noauto and hasattr(node, '__worth__'):
            return node.__worth__
        elif not noauto and hasattr(node, '__autoworth__'):
            return node.__autoworth__
        worth = True
        parent = node.parent
        final = self.isFinalNode(node) and node.name not in self.ignored_tags
        while not final and parent:
            if self.isFinalNode(parent):
                final = True # reset if we've got to one final tag
            if final and (parent.name not in self.ignored_tags) and self.worthOutputting(parent):
                worth = False
                break
            parent = parent.parent
        if not worth:
            node.__worth__ = False
            return False

        if noauto:
            node.__worth__ = worth
            return worth
        else:
            node.__autoworth__ = self.autoNodeIsFinal(node)
            return node.__autoworth__

    def processAttribute(self, node, attr):
        if not node or not attr or not self.worthOutputting(node=node, noauto=True):
            return

        outtxt = self.normalizeString(attr.content)
        if self.app.operation == 'merge':
            translation = self.app.getTranslation(outtxt)
            self.replaceAttributeContentsWithText(attr, translation.encode('utf-8'))
        else:
            self.app.msg.outputMessage(outtxt, node.lineNo(),  "", spacepreserve=False,
                              tag = node.name + ":" + attr.name)

    def processElementTag(self, node, replacements, restart = False):
        """Process node with node.type == 'element'."""
        if node.type != 'element':
            raise Exception("You must pass node with node.type=='element'.")

        # Translate attributes if needed
        if node.properties and self.app.current_mode.getTreatedAttributes():
            for p in node.properties:
                if p.name in self.app.current_mode.getTreatedAttributes():
                    self.processAttribute(node, p)

        outtxt = ''
        if restart:
            myrepl = []
        else:
            myrepl = replacements

        submsgs = []

        child = node.children
        while child:
            if (self.isFinalNode(child)) or (child.type == 'element' and self.worthOutputting(child)):
                myrepl.append(self.processElementTag(child, myrepl, True))
                outtxt += '<placeholder-%d/>' % (len(myrepl))
            else:
                if child.type == 'element':
                    (starttag, content, endtag, translation) = self.processElementTag(child, myrepl, False)
                    outtxt += '<%s>%s</%s>' % (starttag, content, endtag)
                else:
                    outtxt += self.doSerialize(child)
            child = child.next

        if self.app.operation == 'merge':
            norm_outtxt = self.normalizeString(outtxt, self.app.isSpacePreserveNode(node))
            translation = self.app.getTranslation(norm_outtxt)
        else:
            translation = outtxt.decode('utf-8')

        starttag = self.startTagForNode(node)
        endtag = self.endTagForNode(node)

        worth = self.worthOutputting(node)
        if not translation:
            translation = outtxt.decode('utf-8')
            if worth and self.app.options.get('mark_untranslated'):
                node.setLang('C')

        if restart or worth:
            for i, repl in enumerate(myrepl):
                replacement = '<%s>%s</%s>' % (repl[0], repl[3], repl[2])
                translation = translation.replace('<placeholder-%d/>' % (i+1), replacement)

            if worth:
                if self.app.operation == 'merge':
                    self.replaceNodeContentsWithText(node, translation)
                else:
                    norm_outtxt = self.normalizeString(outtxt, self.app.isSpacePreserveNode(node))
                    self.app.msg.outputMessage(norm_outtxt, node.lineNo(), self.getCommentForNode(node), self.app.isSpacePreserveNode(node), tag = node.name)

        return (starttag, outtxt, endtag, translation)


    def isExternalGeneralParsedEntity(self, node):
        try:
            # it would be nice if debugDumpNode could use StringIO, but it apparently cannot
            tmp = tempfile.TemporaryFile()
            node.debugDumpNode(tmp,0)
            tmp.seek(0)
            tmpstr = tmp.read()
            tmp.close()
        except:
            # We fail silently, and replace all entities if we cannot
            # write .xml2po-entitychecking
            # !!! This is not very nice thing to do, but I don't know if
            #     raising an exception is any better
            return False
        return tmpstr.find('EXTERNAL_GENERAL_PARSED_ENTITY') != -1

    def doSerialize(self, node):
        """Serializes a node and its children, emitting PO messages along the way.

        node is the node to serialize, first indicates whether surrounding
        tags should be emitted as well.
        """

        if self.ignoreNode(node):
            return ''
        elif not node.children:
            return node.serialize("utf-8")
        elif node.type == 'entity_ref':
            if self.isExternalGeneralParsedEntity(node):
                return node.serialize('utf-8')
            else:
                return self.stringForEntity(node) #content #content #serialize("utf-8")
        elif node.type == 'entity_decl':
            return node.serialize('utf-8') #'<%s>%s</%s>' % (startTagForNode(node), node.content, node.name)
        elif node.type == 'text':
            return node.serialize('utf-8')
        elif node.type == 'element':
            repl = []
            (starttag, content, endtag, translation) = self.processElementTag(node, repl, True)
            return '<%s>%s</%s>' % (starttag, content, endtag)
        else:
            child = node.children
            outtxt = ''
            while child:
                outtxt += self.doSerialize(child)
                child = child.next
            return outtxt

def xml_error_handler(arg, ctxt):
    #deactivate error messages from the validation
    pass

class Main(object):
    def __init__(self, mode, operation, output, options):
        libxml2.registerErrorHandler(xml_error_handler, None)
        self.operation = operation
        self.options = options
        self.msg = None
        self.gt = None
        self.current_mode = self.load_mode(mode)()
        # Prepare output
        if operation == 'update':
            self.out = tempfile.TemporaryFile()
        elif output == '-':
            self.out = sys.stdout
        else:
            self.out = file(output, 'w')

    def load_mode(self, modename):
        try:
            module = __import__('xml2po.modes.%s' % modename, globals(), locals(), ['%sXmlMode' % modename])
            return getattr(module, '%sXmlMode' % modename)
        except (ImportError, AttributeError):
            if modename == 'basic':
                sys.stderr.write("Unable to find xml2po modes. Please check your xml2po installation.\n")
                sys.exit(1)
            else:
                sys.stderr.write("Unable to load mode '%s'. Falling back to 'basic' mode with automatic detection (-a).\n" % modename)
                return self.load_mode('basic')

    def to_pot(self, xmlfiles):
        """ Produce a pot file from the list of 'xmlfiles' """
        self.msg = MessageOutput(self)
        for xmlfile in xmlfiles:
            if not os.access(xmlfile, os.R_OK):
                raise IOError("Unable to read file '%s'" % xmlfile)
            try:
                doc = XMLDocument(xmlfile, self)
            except Exception, e:
                print >> sys.stderr, "Unable to parse XML file '%s': %s" % (xmlfile, str(e))
                sys.exit(1)
            self.current_mode.preProcessXml(doc.doc, self.msg)
            doc.generate_messages()
        self.output_po()

    def merge(self, mofile, xmlfile):
        """ Merge translations from mofile into xmlfile to generate a translated XML file """
        if not os.access(xmlfile, os.R_OK):
            raise IOError("Unable to read file '%s'" % xmlfile)
        try:
            doc = XMLDocument(xmlfile, self)
        except Exception, e:
            print >> sys.stderr, str(e)
            sys.exit(1)

        try:
            mfile = open(mofile, "rb")
        except:
            print >> sys.stderr, "Can't open MO file '%s'." % (mofile)
        self.gt = gettext.GNUTranslations(mfile)
        self.gt.add_fallback(NoneTranslations())
        # Has preProcessXml use cases for merge?
        #self.current_mode.preProcessXml(doc.doc, self.msg)

        doc.doSerialize(doc.doc)
        tcmsg = self.current_mode.getStringForTranslators()
        outtxt = self.getTranslation(tcmsg)
        self.current_mode.postProcessXmlTranslation(doc.doc, self.options.get('translationlanguage'), outtxt)
        self.out.write(doc.doc.serialize('utf-8', 1))

    def reuse(self, origxml, xmlfile):
        """ Produce a po file from xmlfile pot and using translations from origxml """
        self.msg = MessageOutput(self)
        self.msg.do_translations = True
        if not os.access(xmlfile, os.R_OK):
            raise IOError("Unable to read file '%s'" % xmlfile)
        if not os.access(origxml, os.R_OK):
            raise IOError("Unable to read file '%s'" % xmlfile)
        try:
            doc = XMLDocument(xmlfile, self)
        except Exception, e:
            print >> sys.stderr, str(e)
            sys.exit(1)
        doc.generate_messages()

        self.msg.translationsFollow()
        try:
            doc = XMLDocument(origxml, self)
        except Exception, e:
            print >> sys.stderr, str(e)
            sys.exit(1)
        doc.generate_messages()
        self.output_po()

    def update(self, xmlfiles, lang_file):
        """ Merge the produced pot with an existing po file (lang_file) """
        if not os.access(lang_file, os.W_OK):
            raise IOError("'%s' does not exist or is not writable." % lang_file)
        self.to_pot(xmlfiles)
        lang = os.path.basename(lang_file).split(".")[0]

        sys.stderr.write("Merging translations for %s: \n" % (lang))
        self.out.seek(0)
        merge_cmd = subprocess.Popen(["msgmerge", "-o", ".tmp.%s.po" % lang, lang_file, "-"],
                                     stdin=self.out, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        cmdout, cmderr = merge_cmd.communicate()
        if merge_cmd.returncode:
             raise Exception("Error during msgmerge command.")
        else:
            result = subprocess.call(["mv", ".tmp.%s.po" % lang, lang_file])
            if result:
                raise Exception("Error: cannot rename file.")
            else:
                subprocess.call(["msgfmt", "-cv", "-o", NULL_STRING, lang_file])

    def getTranslation(self, text):
        """Returns a translation via gettext for specified snippet.

        text should be a string to look for.
        """
        #print >>sys.stderr,"getTranslation('%s')" % (text.encode('utf-8'))
        if not text or text.strip() == '':
            return text
        if self.gt:
            res = self.gt.ugettext(text.decode('utf-8'))
            return res

        return text

    def output_po(self):
        """ Write the resulting po/pot file to specified output """
        tcmsg = self.current_mode.getStringForTranslators()
        tccom = self.current_mode.getCommentForTranslators()
        if tcmsg:
            self.msg.outputMessage(tcmsg, lineno=0, comment=tccom)

        self.msg.outputAll(self.out)

    # **** XML utility functions ****
    def isSpacePreserveNode(self, node):
        if node.getSpacePreserve() == 1:
            return True
        else:
            return node.name in self.current_mode.getSpacePreserveTags()

