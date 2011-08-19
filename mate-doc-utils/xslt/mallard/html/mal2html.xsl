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
                xmlns:mal="http://projectmallard.org/1.0/"
                xmlns:html="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="mal"
                version="1.0">

<xsl:import href="../../gettext/gettext.xsl"/>

<!--#@ mal.chunk.doctype_public -->
<xsl:param name="mal.chunk.doctype_public" select="'-//W3C//DTD HTML 4.01 Transitional//EN'"/>

<!--#@ mal.chunk.doctype_system -->
<xsl:param name="mal.chunk.doctype_system" select="'http://www.w3.org/TR/html4/loose.dtd'"/>

<xsl:output method="html"
            doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
            doctype-system="http://www.w3.org/TR/html4/loose.dtd"/>
<xsl:namespace-alias stylesheet-prefix="html" result-prefix="#default"/>

<!--#@ mal2html.namespace -->
<xsl:param name="mal2html.namespace" select="''"/>

<xsl:param name="mal.chunk.extension" select="'.html'"/>


<!--!!==========================================================================
Mallard to HTML

REMARK: Describe this module
-->

<xsl:include href="../common/mal-chunk.xsl"/>
<xsl:include href="../common/mal-link.xsl"/>

<xsl:include href="mal2html-block.xsl"/>
<xsl:include href="mal2html-css.xsl"/>
<xsl:include href="mal2html-inline.xsl"/>
<xsl:include href="mal2html-list.xsl"/>
<xsl:include href="mal2html-media.xsl"/>
<xsl:include href="mal2html-page.xsl"/>
<xsl:include href="mal2html-table.xsl"/>

<xsl:include href="../../common/theme.xsl"/>
<xsl:include href="../../common/utils.xsl"/>

<!-- FIXME -->
<xsl:template match="*">
  <xsl:message>
    <xsl:text>Unmatched element: </xsl:text>
    <xsl:value-of select="local-name(.)"/>
  </xsl:message>
  <xsl:apply-templates/>
</xsl:template>

</xsl:stylesheet>
