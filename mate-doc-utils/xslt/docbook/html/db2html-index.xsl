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
                xmlns:msg="http://www.gnome.org/~shaunm/mate-doc-utils/l10n"
                xmlns:set="http://exslt.org/sets"
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="msg set"
                version="1.0">

<!--!!==========================================================================
DocBook to HTML - Indexes
:Requires: db-chunk db2html-division gettext

This module provides templates to process DocBook indexes.
-->

<!-- FIXME:
indexdiv
seeie
seealsoie
indexterm (autoidx)
-->

<!-- == Matched Templates == -->

<!-- = suppress = -->
<xsl:template match="primaryie"/>
<xsl:template match="secondaryie"/>
<xsl:template match="tertiaryie"/>

<!-- = indexentry = -->
<xsl:template match="indexentry">
  <dt class="primaryie">
    <xsl:apply-templates select="primaryie/node()"/>
  </dt>
  <xsl:variable name="pri_see"
                select="seeie[not(preceding-sibling::secondaryie)]"/>
  <xsl:variable name="pri_seealso"
                select="seealsoie[not(preceding-sibling::secondaryie)]"/>
  <xsl:if test="$pri_see">
    <dd class="see">
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'seeie.format'"/>
        <xsl:with-param name="node" select="$pri_see"/>
        <xsl:with-param name="format" select="true()"/>
      </xsl:call-template>
    </dd>
  </xsl:if>
  <xsl:if test="$pri_seealso">
    <dd class="seealso">
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'seealsoie.format'"/>
        <xsl:with-param name="node" select="$pri_seealso"/>
        <xsl:with-param name="format" select="true()"/>
      </xsl:call-template>
    </dd>
  </xsl:if>
  <xsl:for-each select="secondaryie">
    <dd class="seconary">
      <dl class="secondary">
        <dt class="secondaryie">
          <xsl:apply-templates/>
        </dt>
        <xsl:variable name="sec_see"
                      select="following-sibling::seeie
                                [set:has-same-node(preceding-sibling::secondaryie[1], current())]"/>
        <xsl:variable name="sec_seealso"
                      select="following-sibling::seealsoie
                                [set:has-same-node(preceding-sibling::secondaryie[1], current())]"/>
        <xsl:variable name="tertiary"
                      select="following-sibling::tertiaryie
                                [set:has-same-node(preceding-sibling::secondaryie[1], current())]"/>
        <xsl:if test="$sec_see">
          <dd class="see">
            <xsl:call-template name="l10n.gettext">
              <xsl:with-param name="msgid" select="'seeie.format'"/>
              <xsl:with-param name="node" select="$sec_see"/>
              <xsl:with-param name="format" select="true()"/>
            </xsl:call-template>
          </dd>
        </xsl:if>
        <xsl:if test="$sec_seealso">
          <dd class="seealso">
            <xsl:call-template name="l10n.gettext">
              <xsl:with-param name="msgid" select="'seealsoie.format'"/>
              <xsl:with-param name="node" select="$sec_seealso"/>
              <xsl:with-param name="format" select="true()"/>
            </xsl:call-template>
          </dd>
        </xsl:if>
        <xsl:if test="$tertiary">
          <!-- FIXME -->
        </xsl:if>
      </dl>
    </dd>
  </xsl:for-each>
</xsl:template>

<!-- = index = -->
<xsl:template match="index">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="not(title) and not(indexinfo/title)">
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="info" select="indexinfo"/>
        <xsl:with-param name="divisions" select="indexdiv"/>
        <xsl:with-param name="entries" select="indexentry"/>
        <xsl:with-param name="title_content">
          <xsl:call-template name="l10n.gettext">
            <xsl:with-param name="msgid" select="'Index'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="info" select="indexinfo"/>
        <xsl:with-param name="divisions" select="indexdiv"/>
        <xsl:with-param name="entries" select="indexentry"/>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = setindex = -->
<xsl:template match="setindex">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="not(title) and not(setindexinfo/title)">
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="info" select="setindexinfo"/>
        <xsl:with-param name="divisions" select="indexdiv"/>
        <xsl:with-param name="entries" select="indexentry"/>
        <xsl:with-param name="title_content">
          <xsl:call-template name="l10n.gettext">
            <xsl:with-param name="msgid" select="'Index'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="info" select="setindexinfo"/>
        <xsl:with-param name="divisions" select="indexdiv"/>
        <xsl:with-param name="entries" select="indexentry"/>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--#% l10n.format.mode -->
<xsl:template mode="l10n.format.mode" match="msg:seeie">
  <xsl:param name="node"/>
  <xsl:for-each select="$node">
    <xsl:if test="position() != 1">
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="', '"/>
      </xsl:call-template>
    </xsl:if>
    <xsl:value-of select="."/>
<!--
    <xsl:choose>
      <xsl:when test="@otherterm">
        <a>
          <xsl:attribute name="href">
            <xsl:call-template name="db.xref.target">
              <xsl:with-param name="linkend" select="@otherterm"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="title">
            <xsl:call-template name="db.xref.tooltip">
              <xsl:with-param name="linkend" select="@otherterm"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:choose>
            <xsl:when test="normalize-space(.) != ''">
              <xsl:apply-templates/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="db.xref.content">
                <xsl:with-param name="linkend" select="@otherterm"/>
                <xsl:with-param name="role" select="'glosssee'"/>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </a>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates/>
      </xsl:otherwise>
    </xsl:choose>
-->
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
