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
DocBook to HTML - Titles and Subtitles
:Requires: db-chunk db-label db2html-xref gettext

This stylesheet is going away
-->


<!--**==========================================================================
db2html.title.block
Generates a labeled block title
$node: The element to generate a title for
$referent: The element that ${node} is a title for
$lang: The locale of the text in ${node}
$dir: The text direction, either #{ltr} or #{rtl}

REMARK: Talk about the different kinds of title blocks
-->
<xsl:template name="db2html.title.block">
  <xsl:param name="node" select="."/>
  <xsl:param name="referent" select="$node/.."/>
  <xsl:param name="lang" select="$node/@lang"/>
  <xsl:param name="dir" select="false()"/>
  <xsl:variable name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk">
      <xsl:with-param name="node" select="$referent"/>
    </xsl:call-template>
  </xsl:variable>
  <div class="block block-first {local-name($node)}">
    <xsl:choose>
      <xsl:when test="$dir = 'ltr' or $dir = 'rtl'">
        <xsl:attribute name="dir">
          <xsl:value-of select="$dir"/>
        </xsl:attribute>
      </xsl:when>
      <xsl:when test="$lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="$lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
    </xsl:choose>
    <span class="{local-name($node)}">
      <xsl:call-template name="db2html.anchor">
        <xsl:with-param name="node" select="$node"/>
      </xsl:call-template>
      <span class="label">
        <xsl:call-template name="db.label">
          <xsl:with-param name="node" select="$referent"/>
          <xsl:with-param name="role" select="'header'"/>
          <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        </xsl:call-template>
      </span>
      <xsl:apply-templates select="$node/node()"/>
    </span>
  </div>
</xsl:template>

<!--**==========================================================================
db2html.title.header
This template is going away

This template is going away
-->
<xsl:template name="db2html.title.header"/>




<!-- == Matched Templates == -->

<!-- = subtitle = -->
<!-- Handled in db2html.title.header -->
<xsl:template match="subtitle"/>


<!-- = equation/title = -->
<xsl:template match="equation/title">
  <xsl:call-template name="db2html.title.block"/>
</xsl:template>

<!-- = msg/title = -->
<xsl:template match="msg/title">
  <xsl:call-template name="db2html.title.block"/>
</xsl:template>

<!-- = msgrel/title = -->
<xsl:template match="msgrel/title">
  <xsl:call-template name="db2html.title.block"/>
</xsl:template>

<!-- = msgset/title = -->
<xsl:template match="msgset/title">
  <xsl:call-template name="db2html.title.block"/>
</xsl:template>

<!-- = msgsub/title  = -->
<xsl:template match="msgsub/title">
  <xsl:call-template name="db2html.title.block"/>
</xsl:template>


</xsl:stylesheet>
