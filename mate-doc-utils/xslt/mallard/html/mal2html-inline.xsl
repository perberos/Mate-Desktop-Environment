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
                xmlns:e="http://projectmallard.org/experimental/"
                xmlns="http://www.w3.org/1999/xhtml"
                version="1.0">

<!--!!==========================================================================
Mallard to HTML - Inline Elements

REMARK: Describe this module
-->

<xsl:template mode="mal.link.content.mode" match="*">
  <xsl:apply-templates mode="mal2html.inline.mode" select="."/>
</xsl:template>


<!--%%==========================================================================
mal2html.inline.mode
Processes an element in inline mode

FIXME
-->


<!--%%==========================================================================
mal2html.inline.content.mode
Outputs the contents of an inline element

FIXME
-->
<xsl:template mode="mal2html.inline.content.mode" match="node()">
  <xsl:apply-templates mode="mal2html.inline.mode"/>
</xsl:template>


<!--**==========================================================================
mal2html.span
Renders an inline element as a #{span}
$node: The element to render

REMARK: Document this template
-->
<xsl:template name="mal2html.span">
  <xsl:param name="node" select="."/>
  <xsl:param name="class" select="''"/>
  <span class="{concat($class, ' ', local-name($node))}">
    <xsl:choose>
      <xsl:when test="$node/@xref | $node/@href">
        <a>
          <xsl:attribute name="href">
            <xsl:call-template name="mal.link.target">
              <xsl:with-param name="node" select="$node"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="title">
            <xsl:call-template name="mal.link.tooltip">
              <xsl:with-param name="node" select="$node"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:apply-templates mode="mal2html.inline.content.mode" select="$node"/>
        </a>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="mal2html.inline.content.mode" select="$node"/>
      </xsl:otherwise>
    </xsl:choose>
  </span>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = app = -->
<xsl:template mode="mal2html.inline.mode" match="mal:app">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = cmd = -->
<xsl:template mode="mal2html.inline.mode" match="mal:cmd">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = code = -->
<xsl:template mode="mal2html.inline.mode" match="mal:code">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = date = -->
<xsl:template mode="mal2html.inline.mode" match="mal:date">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = em = -->
<xsl:template mode="mal2html.inline.mode" match="mal:em">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = email = -->
<xsl:template mode="mal2html.inline.mode" match="mal:email">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = file = -->
<xsl:template mode="mal2html.inline.mode" match="mal:file">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = gui = -->
<xsl:template mode="mal2html.inline.mode" match="mal:gui">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = guiseq = -->
<xsl:template mode="mal2html.inline.mode" match="mal:guiseq">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = guiseq % mal2html.inline.content.mode = -->
<xsl:template mode="mal2html.inline.content.mode" match="mal:guiseq">
  <xsl:for-each select="mal:gui">
    <xsl:if test="position() != 1">
      <!-- FIXME: rtl -->
      <xsl:text>&#x00A0;&#x25B8; </xsl:text>
    </xsl:if>
    <xsl:apply-templates mode="mal2html.inline.mode" select="."/>
  </xsl:for-each>
</xsl:template>

<!-- = input = -->
<xsl:template mode="mal2html.inline.mode" match="mal:input">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = hi = -->
<xsl:template mode="mal2html.inline.mode" match="e:hi">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = key = -->
<xsl:template mode="mal2html.inline.mode" match="mal:key">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = keyseq = -->
<xsl:template mode="mal2html.inline.mode" match="mal:keyseq">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = keyseq % mal2html.inline.content.mode = -->
<xsl:template mode="mal2html.inline.content.mode" match="mal:keyseq">
  <xsl:variable name="joinchar">
    <xsl:choose>
      <xsl:when test="@type = 'sequence'">
        <xsl:text> </xsl:text>
      </xsl:when>
      <xsl:when test="contains(concat(' ', @style, ' '), ' hyphen ')">
        <xsl:text>-</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>+</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:for-each select="* | text()[normalize-space(.) != '']">
    <xsl:if test="position() != 1">
      <xsl:value-of select="$joinchar"/>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="./self::text()">
        <xsl:value-of select="normalize-space(.)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="mal2html.inline.mode" select="."/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<!-- = link = -->
<xsl:template mode="mal2html.inline.mode" match="mal:link">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = link % mal2html.inline.content.mode = -->
<xsl:template mode="mal2html.inline.content.mode" match="mal:link">
  <xsl:choose>
    <xsl:when test="normalize-space(.) != ''">
      <xsl:apply-templates mode="mal2html.inline.mode"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="mal.link.content">
        <xsl:with-param name="role" select="@role"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = media = -->
<xsl:template mode="mal2html.inline.mode" match="mal:media">
  <!-- FIXME -->
</xsl:template>

<!-- = name = -->
<xsl:template mode="mal2html.inline.mode" match="mal:name">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = output = -->
<xsl:template mode="mal2html.inline.mode" match="mal:output">
  <xsl:variable name="style" select="concat(' ', @style, ' ')"/>
  <xsl:call-template name="mal2html.span">
    <xsl:with-param name="class">
      <xsl:choose>
        <xsl:when test="contains($style, ' output ')">
          <xsl:text>output-output</xsl:text>
        </xsl:when>
        <xsl:when test="contains($style, ' error ')">
          <xsl:text>output-error</xsl:text>
        </xsl:when>
        <xsl:when test="contains($style, ' prompt ')">
          <xsl:text>output-prompt</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<!-- = quote = -->
<xsl:template mode="mal2html.inline.mode" match="mal:quote">
  <!-- FIXME: do smart quoting -->
  <xsl:text>"</xsl:text>
  <xsl:call-template name="mal2html.span"/>
  <xsl:text>"</xsl:text>
</xsl:template>

<!-- = span = -->
<xsl:template mode="mal2html.inline.mode" match="mal:span">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = sys = -->
<xsl:template mode="mal2html.inline.mode" match="mal:sys">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = var = -->
<xsl:template mode="mal2html.inline.mode" match="mal:var">
  <xsl:call-template name="mal2html.span"/>
</xsl:template>

<!-- = text() = -->
<xsl:template mode="mal2html.inline.mode" match="text()">
  <xsl:value-of select="."/>
</xsl:template>

<!-- = FIXME = -->
<xsl:template mode="mal2html.inline.mode" match="*">
  <xsl:message>
    <xsl:text>Unmatched inline element: </xsl:text>
    <xsl:value-of select="local-name(.)"/>
  </xsl:message>
  <xsl:apply-templates mode="mal2html.inline.mode"/>
</xsl:template>

</xsl:stylesheet>
