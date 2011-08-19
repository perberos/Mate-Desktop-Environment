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
                xmlns:set="http://exslt.org/sets"
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="set"
                version="1.0">

<!--!!==========================================================================
DocBook to HTML - Reference Pages
:Requires: db-chunk db-title db2html-inline db2html-division db2html-xref gettext

REMARK: Describe this module. Talk about refenty and friends
-->

<!--#% db2html.division.div.content.mode -->


<!-- == Matched Templates == -->

<!-- = manvolnum = -->
<xsl:template match="manvolnum">
  <xsl:call-template name="l10n.gettext">
    <xsl:with-param name="msgid" select="'manvolnum.format'"/>
    <xsl:with-param name="node" select="."/>
    <xsl:with-param name="format" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = refentry = -->
<xsl:template match="refentry">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="title_content">
      <xsl:call-template name="db.title">
        <xsl:with-param name="node" select="."/>
      </xsl:call-template>
    </xsl:with-param>
    <xsl:with-param name="divisions" select="refsynopsisdiv | refsection | refsect1"/>
    <xsl:with-param name="callback" select="true()"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = refentry % db2html.division.div.content.mode = -->
<xsl:template mode="db2html.division.div.content.mode" match="refentry">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <div class="refnamedivs">
    <xsl:apply-templates select="refnamediv">
      <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk + 1"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    </xsl:apply-templates>
  </div>
</xsl:template>

<!-- = refdescriptor = -->
<xsl:template match="refdescriptor">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = refname = -->
<xsl:template match="refname">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = refnamediv = -->
<xsl:template match="refnamediv">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <div class="block refnamediv">
    <xsl:call-template name="db2html.anchor"/>
    <xsl:choose>
      <xsl:when test="refdescriptor">
        <xsl:apply-templates select="refdescriptor"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="refname">
          <xsl:if test="position() != 1">
            <xsl:call-template name="l10n.gettext">
              <xsl:with-param name="msgid" select="', '"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:apply-templates select="."/>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="l10n.gettext">
      <xsl:with-param name="msgid" select="' — '"/>
    </xsl:call-template>
    <xsl:apply-templates select="refpurpose"/>
  </div>
  <!-- FIXME: what to do with refclass? -->
</xsl:template>

<!-- = refpurpose = -->
<xsl:template match="refpurpose">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = refsect1 = -->
<xsl:template match="refsect1">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="refsect2"/>
    <xsl:with-param name="info" select="refsect1info"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    <xsl:with-param name="chunk_divisions" select="false()"/>
  </xsl:call-template>
</xsl:template>

<!-- = refsect2 = -->
<xsl:template match="refsect2">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="refsect3"/>
    <xsl:with-param name="info" select="refsect2info"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    <xsl:with-param name="chunk_divisions" select="false()"/>
  </xsl:call-template>
</xsl:template>

<!-- = refsect3 = -->
<xsl:template match="refsect3">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="/false"/>
    <xsl:with-param name="info" select="refsect3info"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    <xsl:with-param name="chunk_divisions" select="false()"/>
  </xsl:call-template>
</xsl:template>

<!-- = refsection = -->
<xsl:template match="refsection">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="refsection"/>
    <xsl:with-param name="info" select="refsectioninfo"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    <xsl:with-param name="chunk_divisions" select="false()"/>
  </xsl:call-template>
</xsl:template>

<!-- = refsynopsisdiv = -->
<xsl:template match="refsynopsisdiv">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="not(title)">
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="title_content">
          <xsl:call-template name="l10n.gettext">
            <xsl:with-param name="msgid" select="'Synopsis'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
