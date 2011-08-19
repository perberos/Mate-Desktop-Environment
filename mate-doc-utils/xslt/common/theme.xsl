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
                version="1.0">

<!--!!==========================================================================
Themes
:Requires: gettext

This module provides a common interface to specify custom colors and icons
for transformations to presentation-oreinted formats.  This allows similar
output for different types of input documents.  Theme information may either
be passed in as XSLT parameters or specified in a theme file.

REMARK: Describe the format of theme files.
-->


<!-- == Colors ============================================================= -->

<!--@@==========================================================================
theme.color.background
The background color

This parameter specifies the default background color.
-->
<xsl:param name="theme.color.background" select="'#ffffff'"/>

<!--@@==========================================================================
theme.color.link
The color of links

If set, this parameter specifies the color for unvisited links.
-->
<xsl:param name="theme.color.link" select="'#1f609f'"/>

<!--@@==========================================================================
theme.color.link_visited
The color of visited links

This parameter specifies the color for visited links.
-->
<xsl:param name="theme.color.link_visited" select="'#9f1f6f'"/>

<!--@@==========================================================================
theme.color.text
The normal text color

This parameter specifies the default color for normal text.
-->
<xsl:param name="theme.color.text" select="'#000000'"/>

<!--@@==========================================================================
theme.color.text_light
The light text color

This parameter specifies the color for light text.  The light text
color is used to make bold headings and certain parenthetical text less intense.
-->
<xsl:param name="theme.color.text_light" select="'#3f3f3f'"/>

<!--@@==========================================================================
theme.color.blue_background
The blue background color

This parameter specifies the blue background color.  The blue
background color may be used by certain types of block elements.
-->
<xsl:param name="theme.color.blue_background" select="'#f0f9ff'"/>

<!--@@==========================================================================
theme.color.blue_border
The blue border color

This parameter specifies the blue border color.  The blue
border color may be used by certain types of block elements.
-->
<xsl:param name="theme.color.blue_border" select="'#c0c9ff'"/>

<!--@@==========================================================================
theme.color.gray_background
The gray background color

This parameter specifies the gray background color.  The gray
background color may be used by certain types of block elements.
-->
<xsl:param name="theme.color.gray_background" select="'#f9f9f6'"/>

<!--@@==========================================================================
theme.color.gray_border
The gray border color

This parameter specifies the gray border color.  The gray
border color may be used by certain types of block elements.
-->
<xsl:param name="theme.color.gray_border" select="'#e0e0df'"/>

<!--@@==========================================================================
theme.color.red_background
The red background color

This parameter specifies the red background color.  The red
background color may be used by certain types of block elements.
-->
<xsl:param name="theme.color.red_background" select="'#fff0f0'"/>

<!--@@==========================================================================
theme.color.red_border
The red border color

This parameter specifies the red border color.  The red
border color may be used by certain types of block elements.
-->
<xsl:param name="theme.color.red_border" select="'#ffc0c0'"/>

<!--@@==========================================================================
theme.color.yellow_background
The yellow background color

This parameter specifies the yellow background color.  The yellow
background color may be used by certain types of block elements.
-->
<xsl:param name="theme.color.yellow_background" select="'#fffff0'"/>

<!--@@==========================================================================
theme.color.yellow_border
The yellow border color

This parameter specifies the yellow border color.  The yellow
border color may be used by certain types of block elements.
-->
<xsl:param name="theme.color.yellow_border" select="'#ffffc0'"/>


<!-- == Admonition Icons =================================================== -->

<!--@@==========================================================================
theme.icon.admon.path
The default path to the admonition icons

This parameter specifies a default path where admonition icons can be found.
This is only a default path.  It has no effect on admonition icons for which
the corresponding parameter has been specified explicitly.
-->
<xsl:param name="theme.icon.admon.path" select="''"/>

<!--@@==========================================================================
theme.icon.admon.size
The size of the admonition icons

This parameter specifies the size of the icons given in @{theme.icon.admon.bug},
@{theme.icon.admon.caution}, @{theme.icon.admon.important}, @{theme.icon.admon.note},
@{theme.icon.admon.tip}, and @{theme.icon.admon.warning}.  The value of this parameter
should be a single integer specifying both the width and the height in pixels.  Icon
sizes are assumed to be square.
-->
<xsl:param name="theme.icon.admon.size" select="48"/>

<!--@@==========================================================================
theme.icon.admon.bug
The path for the bug admonition icon

This parameter specifies the icon used for bug admonitions.
-->
<xsl:param name="theme.icon.admon.bug"
           select="concat($theme.icon.admon.path, 'admon-bug.png')"/>

<!--@@==========================================================================
theme.icon.admon.caution
The path for the caution admonition icon

This parameter specifies the icon used for caution admonitions.
-->
<xsl:param name="theme.icon.admon.caution"
           select="concat($theme.icon.admon.path, 'admon-caution.png')"/>

<!--@@==========================================================================
theme.icon.admon.important
The path for the important admonition icon

This parameter specifies the icon used for important admonitions.
-->
<xsl:param name="theme.icon.admon.important"
           select="concat($theme.icon.admon.path, 'admon-important.png')"/>

<!--@@==========================================================================
theme.icon.admon.note
The path for the note admonition icon

This parameter specifies the icon used for note admonitions.
-->
<xsl:param name="theme.icon.admon.note"
           select="concat($theme.icon.admon.path, 'admon-note.png')"/>

<!--@@==========================================================================
theme.icon.admon.tip
The path for the tip admonition icon

This parameter specifies the icon used for tip admonitions.
-->
<xsl:param name="theme.icon.admon.tip"
           select="concat($theme.icon.admon.path, 'admon-tip.png')"/>

<!--@@==========================================================================
theme.icon.admon.warning
The path for the warning admonition icon

This parameter specifies the icon used for warning admonitions.
-->
<xsl:param name="theme.icon.admon.warning"
           select="concat($theme.icon.admon.path, 'admon-warning.png')"/>


<!-- == Watermarks ========================================================= -->

<!--@@==========================================================================
theme.watermark.path
The default path to the watermark graphics

This parameter specifies a default path where watermark graphics can be found.
This is only a default path.  It has no effect on watermark graphics for which
the corresponding parameter has been specified explicitly.
-->
<xsl:param name="theme.watermark.path" select="''"/>

<!--@@==========================================================================
theme.watermark.blockquote
The path for the blockquote watermark graphic

This parameter specifies the graphic file used for blockquote watermarks.  Note
that the default graphic is translatable.  If you set this parameter, you should
ensure that your graphic uses a suitable quote character for the language your
document is written in.
-->
<xsl:param name="theme.watermark.blockquote">
  <xsl:value-of select="$theme.watermark.path"/>
  <xsl:call-template name="l10n.gettext">
    <xsl:with-param name="msgid" select="'watermark-blockquote-201C.png'"/>
  </xsl:call-template>
</xsl:param>

<!--@@==========================================================================
theme.watermark.code
The path for the code block watermark graphic

This parameter specifies the graphic file used for code block watermarks.
-->
<xsl:param name="theme.watermark.code">
  <xsl:value-of select="$theme.watermark.path"/>
  <xsl:text>watermark-code.png</xsl:text>
</xsl:param>

</xsl:stylesheet>
