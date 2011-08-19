<?xml version='1.0' encoding='UTF-8'?><!-- -*- indent-tabs-mode: nil -*- -->
<!--
This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License
along with this program; see the file COPYING.LGPL.  If not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:html="http://www.w3.org/1999/xhtml"
                version="1.0">

<xsl:import href="../../gettext/gettext.xsl"/>

<!--#@ db.chunk.doctype_public -->
<xsl:param name="db.chunk.doctype_public" select="'-//W3C//DTD HTML 4.01 Transitional//EN'"/>

<!--#@ db.chunk.doctype_system -->
<xsl:param name="db.chunk.doctype_system" select="'http://www.w3.org/TR/html4/loose.dtd'"/>

<xsl:output method="html"
            doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
            doctype-system="http://www.w3.org/TR/html4/loose.dtd"/>
<xsl:namespace-alias stylesheet-prefix="html" result-prefix="#default"/>

<!--#@ db2html.namespace -->
<xsl:param name="db2html.namespace" select="''"/>

<!--!!==========================================================================
DocBook to HTML
-->

<!--#@ db.chunk.extension -->
<xsl:param name="db.chunk.extension" select="'.html'"/>

<xsl:include href="../../common/theme.xsl"/>

<xsl:include href="../common/db-chunk.xsl"/>
<xsl:include href="../common/db-common.xsl"/>
<xsl:include href="../common/db-label.xsl"/>
<xsl:include href="../common/db-title.xsl"/>
<xsl:include href="../common/db-xref.xsl"/>

<xsl:include href="db2html-autotoc.xsl"/>
<xsl:include href="db2html-bibliography.xsl"/>
<xsl:include href="db2html-block.xsl"/>
<xsl:include href="db2html-callout.xsl"/>
<xsl:include href="db2html-classsynopsis.xsl"/>
<xsl:include href="db2html-cmdsynopsis.xsl"/>
<xsl:include href="db2html-css.xsl"/>
<xsl:include href="db2html-division.xsl"/>
<xsl:include href="db2html-ebnf.xsl"/>
<xsl:include href="db2html-funcsynopsis.xsl"/>
<xsl:include href="db2html-index.xsl"/>
<xsl:include href="db2html-info.xsl"/>
<xsl:include href="db2html-inline.xsl"/>
<xsl:include href="db2html-l10n.xsl"/>
<xsl:include href="db2html-media.xsl"/>
<xsl:include href="db2html-list.xsl"/>
<xsl:include href="db2html-qanda.xsl"/>
<xsl:include href="db2html-refentry.xsl"/>
<xsl:include href="db2html-suppressed.xsl"/>
<xsl:include href="db2html-table.xsl"/>
<xsl:include href="db2html-title.xsl"/>
<xsl:include href="db2html-xref.xsl"/>
<xsl:include href="db2html-footnote.xsl"/>

<xsl:template match="*">
  <xsl:message>
    <xsl:text>Unmatched element: </xsl:text>
    <xsl:value-of select="local-name(.)"/>
  </xsl:message>
  <xsl:apply-templates select="node()"/>
</xsl:template>

</xsl:stylesheet>
