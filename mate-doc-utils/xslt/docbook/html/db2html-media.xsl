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
DocBook to HTML - Images and Media
:Requires: db2html-block db2html-xref

REMARK: Describe this module
-->


<!--**==========================================================================
db2html.imagedata
Renders an #{imagedata} element into an #{img} element
$node: The element to render

This template creates an #{img} element in the HTML output.  This named template
is called not only for #{imagedata} elements, but also for #{graphic} and
#{inlinegraphic} elements.  Note that #{graphic} and #{inlinegraphic} are
deprecated and should not be used in any newly-written DocBook files.  Use
#{mediaobject} instead.

REMARK: calls db2html.imagedata.src, how other attrs are gotten
-->
<xsl:template name="db2html.imagedata">
  <xsl:param name="node" select="."/>
  <xsl:choose>
    <xsl:when test="$node/@format = 'SVG'">
      <!--
          We don't really ever want to embed external SVGs, because there's
          no guarantee they exist at build time. But Yelp 2.30 launches an
          external viewer when it encounters an <object> tag, so this is
          the only option. When this was added, mate-doc-utils and Yelp 2
          are in maintenance (and hacks) mode. This works for some Ubuntu
          docs that need SVG.
      -->
      <xsl:choose>
        <xsl:when test="$db2html.namespace = ''">
          <object type="image/svg+xml">
            <xsl:attribute name="data">
              <xsl:call-template name="db2html.imagedata.src">
                <xsl:with-param name="node" select="$node"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:copy-of select="@width"/>
            <xsl:copy-of select="@height"/>
          </object>
        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="svg" select="document($node/@fileref)"/>
          <xsl:copy-of select="$svg"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
  <img>
    <xsl:attribute name="src">
      <xsl:call-template name="db2html.imagedata.src">
        <xsl:with-param name="node" select="$node"/>
      </xsl:call-template>
    </xsl:attribute>
    <xsl:choose>
      <xsl:when test="$node/@scale">
        <xsl:attribute name="width">
          <xsl:value-of select="concat($node/@scale, '%')"/>
        </xsl:attribute>
      </xsl:when>
      <xsl:when test="$node/@width">
        <xsl:attribute name="width">
          <xsl:value-of select="$node/@width"/>
        </xsl:attribute>
        <xsl:if test="$node/@height">
          <xsl:attribute name="height">
            <xsl:value-of select="$node/@height"/>
          </xsl:attribute>
        </xsl:if>
      </xsl:when>
    </xsl:choose>
    <xsl:if test="$node/@align">
      <xsl:attribute name="align">
        <xsl:value-of select="$node/@align"/>
      </xsl:attribute>
    </xsl:if>
<!-- FIXME
    <xsl:if test="$textobject/phrase">
      <xsl:attribute name="alt">
        <xsl:value-of select="phrase[1]"/>
      </xsl:attribute>
    </xsl:if>
-->
    <!-- FIXME: longdesc -->
  </img>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2html.imagedata.src
Outputs the content of the #{src} attribute for an #{img} element
$node: The element to render

This template is called by *{db2html.imagedata.src} for the content of the
#{src} attribute of an #{img} element.
-->
<xsl:template name="db2html.imagedata.src">
  <xsl:param name="node" select="."/>
  <xsl:choose>
    <xsl:when test="$node/@fileref">
      <!-- FIXME: do this less stupidly, or not at all -->
      <xsl:choose>
        <xsl:when test="$node/@format = 'PNG' and
                        (substring($node/@fileref, string-length($node/@fileref) - 3)
                          != '.png')">
          <xsl:value-of select="concat($node/@fileref, '.png')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$node/@fileref"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$node/@entityref">
      <xsl:value-of select="unparsed-entity-uri($node/@entityref)"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2html.mediaobject
Outputs HTML for a #{mediaobject} element
$node: The element to render

This template processes a #{mediaobject} element and outputs the appropriate
HTML.  DocBook allows multiple objects to be listed in a #{mediaobject} element.
Processing tools are expected to choose the earliest suitable object.  Currently,
this template only chooses the first suitable #{imageobject} element.  Support
for #{videobject} and #{audioobject} should be added in future versions, as well
as a text-only mode.
-->
<xsl:template name="db2html.mediaobject">
  <xsl:param name="node" select="."/>
  <xsl:choose>
<!-- FIXME
    <xsl:when test="$text_only">
      <xsl:apply-templates select="textobject[1]"/>
    </xsl:when>
-->
    <xsl:when test="$node/imageobject[imagedata/@format = 'PNG']">
      <xsl:apply-templates
       select="$node/imageobject[imagedata/@format = 'PNG'][1]">
        <xsl:with-param name="textobject" select="$node/textobject[1]"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:when test="$node/imageobjectco[imageobject/imagedata/@format = 'PNG']">
      <xsl:apply-templates
       select="$node/imageobjectco[imageobject/imagedata/@format = 'PNG'][1]">
        <xsl:with-param name="textobject" select="$node/textobject[1]"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="($node/imageobject | $node/imageobjectco)[1]">
        <xsl:with-param name="textobject" select="$node/textobject[1]"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = graphic = -->
<xsl:template match="graphic">
  <div class="graphic">
    <xsl:call-template name="db2html.anchor"/>
    <xsl:call-template name="db2html.imagedata"/>
  </div>
</xsl:template>

<!-- = imagedata = -->
<xsl:template match="imagedata">
  <xsl:call-template name="db2html.imagedata"/>
</xsl:template>

<!-- = imageobject = -->
<xsl:template match="imageobject">
  <xsl:apply-templates select="imagedata"/>
</xsl:template>

<!-- = inlinegraphic = -->
<xsl:template match="inlinegraphic">
  <span class="inlinegraphic">
    <xsl:call-template name="db2html.anchor"/>
    <xsl:call-template name="db2html.imagedata"/>
  </span>
</xsl:template>

<!-- = inlinemediaobject = -->
<xsl:template match="inlinemediaobject">
  <span class="inlinemediaobject">
    <xsl:call-template name="db2html.anchor"/>
    <xsl:call-template name="db2html.mediaobject"/>
  </span>
</xsl:template>

<!-- = mediaojbect = -->
<xsl:template match="mediaobject">
  <div class="mediaobject">
    <xsl:call-template name="db2html.anchor"/>
    <xsl:call-template name="db2html.mediaobject"/>
    <!-- When a figure contains only a single mediaobject, it eats the caption -->
    <xsl:if test="not(../self::figure or ../self::informalfigure) or
                  ../*[not(self::blockinfo) and not(self::title) and
                       not(self::titleabbrev) and not(. = current()) ]">
      <xsl:apply-templates select="caption"/>
    </xsl:if>
  </div>
</xsl:template>

<!-- = screenshot = -->
<xsl:template match="screenshot">
  <xsl:call-template name="db2html.block"/>
</xsl:template>

</xsl:stylesheet>
