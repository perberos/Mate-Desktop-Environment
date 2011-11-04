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
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="msg"
                version="1.0">

<!--!!==========================================================================
DocBook to HTML - Localization Formatters

REMARK: Document this module
-->

<!--**==========================================================================
l10n.format.span
FIXME
$node: The node in the original document being processed
$span: The #{msg:span} element in the localized formatter
$font_family: The font family to render the text in
$font_style: The font style, generally used for #{italic}
$font_variant: The font variant, generally used for #{small-caps}
$font_stretch: The amount to stretch the font
$font_size: The size of the text
$text_decoration: The decoration on the text, generally used for #{underline}

REMARK: Talk a lot about this, including a specification of what each of
the parameters can do.
-->
<xsl:template name="l10n.format.span">
  <xsl:param name="node"/>
  <xsl:param name="span" select="."/>
  <xsl:param name="font_family"     select="string($span/@font-family)"/>
  <xsl:param name="font_style"      select="string($span/@font-style)"/>
  <xsl:param name="font_variant"    select="string($span/@font-variant)"/>
  <xsl:param name="font_weight"     select="string($span/@font-weight)"/>
  <xsl:param name="font_stretch"    select="string($span/@font-stretch)"/>
  <xsl:param name="font_size"       select="string($span/@font-size)"/>
  <xsl:param name="text_decoration" select="string($span/@text-decoration)"/>
  <span>
    <xsl:attribute name="style">
      <xsl:if test="$font_family != ''">
        <xsl:value-of select="concat('font-family: ', $font_family, '; ')"/>
      </xsl:if>
      <xsl:if test="$font_style != ''">
        <xsl:value-of select="concat('font-style: ', $font_style, '; ')"/>
      </xsl:if>
      <xsl:if test="$font_variant != ''">
        <xsl:value-of select="concat('font-variant: ', $font_variant, '; ')"/>
      </xsl:if>
      <xsl:if test="$font_weight != ''">
        <xsl:value-of select="concat('font-weight: ', $font_weight, '; ')"/>
      </xsl:if>
      <xsl:if test="$font_stretch != ''">
        <xsl:value-of select="concat('font-stretch: ', $font_stretch, '; ')"/>
      </xsl:if>
      <!-- FIXME: make font size able to use our 1.2 scale? -->
      <xsl:if test="$font_size != ''">
        <xsl:value-of select="concat('font-size: ', $font_size, '; ')"/>
      </xsl:if>
      <xsl:if test="$text_decoration != ''">
        <xsl:value-of select="concat('text-decoration: ', $text-decoration, '; ')"/>
      </xsl:if>
    </xsl:attribute>
    <xsl:apply-templates mode="l10n.format.mode">
      <xsl:with-param name="node" select="$node"/>
    </xsl:apply-templates>
  </span>
</xsl:template>

<!--#% l10n.format.mode ==================================================== -->

<xsl:template mode="l10n.format.mode" match="msg:span">
  <xsl:param name="node"/>
  <xsl:call-template name="l10n.format.span">
    <xsl:with-param name="node" select="$node"/>
    <xsl:with-param name="span" select="."/>
  </xsl:call-template>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:b">
  <xsl:param name="node"/>
  <xsl:call-template name="l10n.format.span">
    <xsl:with-param name="node" select="$node"/>
    <xsl:with-param name="font_weight" select="'bold'"/>
  </xsl:call-template>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:big">
  <xsl:param name="node"/>
  <xsl:call-template name="l10n.format.span">
    <xsl:with-param name="node" select="$node"/>
    <xsl:with-param name="font_size" select="'1.2em'"/>
  </xsl:call-template>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:i">
  <xsl:param name="node"/>
  <xsl:call-template name="l10n.format.span">
    <xsl:with-param name="node" select="$node"/>
    <xsl:with-param name="font_style" select="'italic'"/>
  </xsl:call-template>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:sub">
  <xsl:param name="node"/>
  <sub>
    <xsl:apply-templates mode="l10n.format.mode">
      <xsl:with-param name="node" select="$node"/>
    </xsl:apply-templates>
  </sub>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:sup">
  <xsl:param name="node"/>
  <sup>
    <xsl:apply-templates mode="l10n.format.mode">
      <xsl:with-param name="node" select="$node"/>
    </xsl:apply-templates>
  </sup>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:small">
  <xsl:param name="node"/>
  <xsl:call-template name="l10n.format.span">
    <xsl:with-param name="node" select="$node"/>
    <xsl:with-param name="font_size" select="'0.83em'"/>
  </xsl:call-template>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:tt">
  <xsl:param name="node"/>
  <xsl:call-template name="l10n.format.span">
    <xsl:with-param name="node" select="$node"/>
    <xsl:with-param name="font_family" select="'monospace'"/>
  </xsl:call-template>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:u">
  <xsl:param name="node"/>
  <xsl:call-template name="l10n.format.span">
    <xsl:with-param name="node" select="$node"/>
    <xsl:with-param name="text_decoration" select="'underline'"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
