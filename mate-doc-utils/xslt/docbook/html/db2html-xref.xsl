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
                xmlns="http://www.w3.org/1999/xhtml"
                version="1.0">

<!--!!==========================================================================
DocBook to HTML - Links and Cross References
:Requires: db-xref

REMARK: Describe this module
-->


<!--**==========================================================================
db2html.anchor
Generates an anchor point for an element
$node: The element to generate an anchor for
$name: The text to use for the #{name} attribute

REMARK: Describe this template
-->
<xsl:template name="db2html.anchor" match="anchor">
  <xsl:param name="node" select="."/>
  <xsl:param name="name" select="$node/@id"/>
  <xsl:if test="$name"><a name="{$name}"/></xsl:if>
</xsl:template>


<!--**==========================================================================
db2html.link
Generates a hyperlink from a #{link} element
$linkend: The id of the element being linked to
$target: The element being linked to

REMARK: Describe this template
-->
<xsl:template name="db2html.link" match="link">
  <xsl:param name="linkend" select="@linkend"/>
  <xsl:param name="target" select="key('idkey', $linkend)"/>
  <a class="link">
    <xsl:attribute name="href">
      <xsl:call-template name="db.xref.target">
        <xsl:with-param name="linkend" select="$linkend"/>
      </xsl:call-template>
    </xsl:attribute>
    <xsl:attribute name="title">
      <xsl:call-template name="db.xref.tooltip">
        <xsl:with-param name="linkend" select="$linkend"/>
        <xsl:with-param name="target" select="$target"/>
      </xsl:call-template>
    </xsl:attribute>
    <xsl:choose>
      <xsl:when test="normalize-space(.) != ''">
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:when test="@endterm">
        <xsl:apply-templates select="key('idkey', @endterm)/node()"/>
      </xsl:when>
    </xsl:choose>
  </a>
</xsl:template>


<!--**==========================================================================
db2html.ulink
Generates a hyperlink from a #{ulink} element
$url: The URL to link to
$content: Optional content to use for the text of the link

REMARK: Describe this template
-->
<xsl:template name="db2html.ulink" match="ulink">
  <xsl:param name="url" select="@url"/>
  <xsl:param name="content" select="false()"/>
  <a class="ulink" href="{$url}">
    <xsl:attribute name="title">
      <xsl:call-template name="db.ulink.tooltip"/>
    </xsl:attribute>
    <xsl:choose>
      <xsl:when test="$content">
        <xsl:copy-of select="$content"/>
      </xsl:when>
      <xsl:when test="string-length(normalize-space(node())) != 0">
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$url"/>
      </xsl:otherwise>
    </xsl:choose>
  </a>
</xsl:template>


<!--**==========================================================================
db2html.xref
Generates a hyperlink from an #{xref} element
$linkend: The id of the element being linked to
$target: The element being linked to
$endterm: The id of an element whose contents will be used for the link text
$xrefstyle: The style of cross reference text to use
$content: Optional content to use for the text of the link

REMARK: Describe this template
-->
<xsl:template name="db2html.xref" match="xref">
  <xsl:param name="linkend"   select="@linkend"/>
  <xsl:param name="target"    select="key('idkey', $linkend)"/>
  <xsl:param name="endterm"   select="@endterm"/>
  <xsl:param name="xrefstyle" select="@xrefstyle"/>
  <xsl:param name="content"   select="false()"/>
  <a class="xref">
    <xsl:attribute name="href">
      <xsl:call-template name="db.xref.target">
        <xsl:with-param name="linkend" select="$linkend"/>
        <xsl:with-param name="target" select="$target"/>
      </xsl:call-template>
    </xsl:attribute>
    <xsl:attribute name="title">
      <xsl:call-template name="db.xref.tooltip">
        <xsl:with-param name="linkend" select="$linkend"/>
        <xsl:with-param name="target" select="$target"/>
      </xsl:call-template>
    </xsl:attribute>
    <xsl:choose>
      <xsl:when test="$content">
        <xsl:copy-of select="$content"/>
      </xsl:when>
      <xsl:when test="$endterm">
        <xsl:apply-templates select="key('idkey', $endterm)/node()"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db.xref.content">
          <xsl:with-param name="linkend" select="$linkend"/>
          <xsl:with-param name="target" select="$target"/>
          <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </a>
</xsl:template>

</xsl:stylesheet>
