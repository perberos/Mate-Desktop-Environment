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
DocBook to HTML - Question and Answer Sets
:Requires: db-chunk db-label db2html-division gettext

REMARK: Describe this module
-->


<!--**==========================================================================
db2html.qanda.css
Outputs CSS that controls the appearance of question and answer elements

REMARK: Describe this template
-->
<xsl:template name="db2html.qanda.css">
<xsl:text>
dt.question { margin-left: 0em; }
dt.question div.label { float: left; }
dd + dt.question { margin-top: 1em; }
dd.answer {
  margin-top: 1em;
  margin-left: 2em;
  margin-right: 1em;
}
dd.answer div.label { float: left; }
</xsl:text>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = answer = -->
<xsl:template match="answer">
  <dd class="answer">
    <xsl:choose>
      <xsl:when test="@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
      <xsl:when test="../@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="../@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
    </xsl:choose>
    <div class="label">
      <xsl:call-template name="db.label">
        <xsl:with-param name="role" select="'header'"/>
      </xsl:call-template>
    </div>
    <xsl:apply-templates/>
  </dd>
</xsl:template>

<!-- = qandadiv = -->
<xsl:template match="qandadiv">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="info" select="blockinfo"/>
    <xsl:with-param name="entries" select="qandaentry"/>
    <xsl:with-param name="divisions" select="qandadiv"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    <xsl:with-param name="chunk_divisions" select="false()"/>
    <xsl:with-param name="chunk_info" select="false()"/>
    <xsl:with-param name="autotoc_divisions" select="false()"/>
  </xsl:call-template>
</xsl:template>

<!-- = qandaentry = -->
<xsl:template match="qandaentry">
  <xsl:apply-templates/>
</xsl:template>

<!-- = qandaset = -->
<xsl:template match="qandaset">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="info" select="blockinfo"/>
    <xsl:with-param name="entries" select="qandaentry"/>
    <xsl:with-param name="divisions" select="qandadiv"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    <xsl:with-param name="chunk_divisions" select="false()"/>
    <xsl:with-param name="chunk_info" select="false()"/>
    <xsl:with-param name="autotoc_divisions" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = question = -->
<xsl:template match="question">
  <!-- FIXME: dt-first -->
  <dt class="question">
    <xsl:choose>
      <xsl:when test="@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
      <xsl:when test="../@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="../@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
    </xsl:choose>
    <div class="label">
      <xsl:call-template name="db.label">
        <xsl:with-param name="role" select="'header'"/>
      </xsl:call-template>
    </div>
    <xsl:apply-templates/>
  </dt>
</xsl:template>

</xsl:stylesheet>
