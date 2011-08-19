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
                xmlns="http://www.w3.org/1999/xhtml"
                version="1.0">

<!--!!==========================================================================
Mallard to HTML - Media Elements

REMARK: Describe this module
-->

<!--**==========================================================================
mal2html.media.image
FIXME

FIXME
-->
<xsl:template name="mal2html.media.image">
  <xsl:param name="node" select="."/>
  <xsl:param name="inline" select="false()"/>
  <img src="{$node/@src}">
    <xsl:copy-of select="@height"/>
    <xsl:copy-of select="@width"/>
    <xsl:attribute name="alt">
      <xsl:choose>
        <xsl:when test="$inline">
          <xsl:value-of select="$node"/>
        </xsl:when>
        <xsl:otherwise>
          <!-- FIXME: This is not ideal.  Nested block container elements
               will introduce lots of garbage whitespace.  But then, XML
               processors are supposed to normalize whitespace in attribute
               values anyway.  Ideally, we'd have a set of modes for text
               conversion.  That'd probably be best handled in a set of
               mal2text stylesheets.
          -->
          <xsl:for-each select="$node/mal:*">
            <xsl:if test="position() &gt; 1">
              <xsl:text>&#x000A;</xsl:text>
            </xsl:if>
            <xsl:value-of select="string(.)"/>
          </xsl:for-each>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
  </img>
</xsl:template>

<!--**==========================================================================
mal2html.media.video
FIXME

FIXME
-->
<xsl:template name="mal2html.media.video">
  <xsl:param name="node" select="."/>
  <xsl:param name="inline" select="false()"/>
  <video src="{$node/@src}" autobuffer="autobuffer" controls="controls">
    <xsl:copy-of select="$node/@height"/>
    <xsl:copy-of select="$node/@width"/>
    <xsl:choose>
      <xsl:when test="$inline">
        <xsl:apply-templates mode="mal2html.inline.mode" select="$node/node()"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="mal2html.block.mode" select="$node/node()"/>
      </xsl:otherwise>
    </xsl:choose>
  </video>
</xsl:template>

<!--**==========================================================================
mal2html.media.audio
FIXME

FIXME
-->
<xsl:template name="mal2html.media.audio">
  <xsl:param name="node" select="."/>
  <xsl:param name="inline" select="false()"/>
  <audio src="{$node/@src}" autobuffer="autobuffer" controls="controls">
    <xsl:choose>
      <xsl:when test="$inline">
        <xsl:apply-templates mode="mal2html.inline.mode" select="$node/node()"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="mal2html.block.mode" select="$node/node()"/>
      </xsl:otherwise>
    </xsl:choose>
  </audio>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = mal2html.block.mode % media = -->
<xsl:template mode="mal2html.block.mode" match="mal:media">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <xsl:choose>
    <xsl:when test="@type = 'image'">
      <div>
        <xsl:attribute name="class">
          <xsl:text>media media-image</xsl:text>
          <xsl:if test="$first_child">
            <xsl:text> first-child</xsl:text>
          </xsl:if>
        </xsl:attribute>
        <xsl:call-template name="mal2html.media.image"/>
      </div>
    </xsl:when>
    <xsl:when test="@type = 'video'">
      <div>
        <xsl:attribute name="class">
          <xsl:text>media media-video</xsl:text>
          <xsl:if test="$first_child">
            <xsl:text> first-child</xsl:text>
          </xsl:if>
        </xsl:attribute>
        <xsl:call-template name="mal2html.media.video"/>
      </div>
    </xsl:when>
    <xsl:when test="@type = 'audio'">
      <div>
        <xsl:attribute name="class">
          <xsl:text>media media-audio</xsl:text>
          <xsl:if test="$first_child">
            <xsl:text> first-child</xsl:text>
          </xsl:if>
        </xsl:attribute>
        <xsl:call-template name="mal2html.media.audio"/>
      </div>
    </xsl:when>
    <xsl:otherwise>
      <xsl:for-each select="mal:*">
        <xsl:apply-templates mode="mal2html.block.mode" select=".">
          <xsl:with-param name="first_child" select="position() = 1 and $first_child"/>
        </xsl:apply-templates>
      </xsl:for-each>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = mal2html.inline.mode % media = -->
<xsl:template mode="mal2html.inline.mode" match="mal:media">
  <xsl:choose>
    <xsl:when test="@type = 'image'">
      <span class="media media-image">
        <xsl:call-template name="mal2html.media.image">
          <xsl:with-param name="inline" select="true()"/>
        </xsl:call-template>
      </span>
    </xsl:when>
    <xsl:when test="@type = 'video'">
      <span class="media media-video">
        <xsl:call-template name="mal2html.media.video">
          <xsl:with-param name="inline" select="true()"/>
        </xsl:call-template>
      </span>
    </xsl:when>
    <xsl:when test="@type = 'audio'">
      <span class="media media-audio">
        <xsl:call-template name="mal2html.media.audio">
          <xsl:with-param name="inline" select="true()"/>
        </xsl:call-template>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates mode="mal2html.inline.mode"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
