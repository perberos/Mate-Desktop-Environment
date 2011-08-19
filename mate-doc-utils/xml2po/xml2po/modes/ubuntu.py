# Copyright (c) 2006 Danilo Segan <danilo@kvota.net>.

import libxml2

from docbook import docbookXmlMode

class ubuntuXmlMode (docbookXmlMode):
    """Special-casing Ubuntu DocBook website documentation."""
    def postProcessXmlTranslation(self, doc, language, translators):
        """Sets a language and translators in "doc" tree."""

        # Call ancestor method
        docbookXmlMode.postProcessXmlTranslation(self, doc, language, translators)

        try:
            child = doc.docEntity('language')
            newent = doc.addDocEntity('my_internal_hacky_language_entity_decl', libxml2.XML_INTERNAL_GENERAL_ENTITY, None, None, language)
            newent.setName('language')
            child.replaceNode(newent)
        except:
            newent = doc.addDocEntity('language', libxml2.XML_INTERNAL_GENERAL_ENTITY, None, None, language)

