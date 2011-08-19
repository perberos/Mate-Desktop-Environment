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
DocBook to HTML - Callouts

REMARK: Describe this module
-->

<!--@@==========================================================================
db2html.co.color
The text color for callout dingbats

REMARK: Describe this param
-->
<xsl:param name="db2html.co.color" select="'#FFFFFF'"/>

<!--@@==========================================================================
db2html.co.background_color
The background color for callout dingbats

REMARK: Describe this param
-->
<xsl:param name="db2html.co.background_color" select="'#000000'"/>

<!--@@==========================================================================
db2html.co.border_color
The border color for callout dingbats

REMARK: Describe this param
-->
<xsl:param name="db2html.co.border_color" select="'#000000'"/>

<!--@@==========================================================================
db2html.co.color.hover
The text color for callout dingbats when hovering

REMARK: Describe this param
-->
<xsl:param name="db2html.co.color.hover" select="'#FFFFFF'"/>

<!--@@==========================================================================
db2html.co.background_color.hover
The background color for callout dingbats when hovering

REMARK: Describe this param
-->
<xsl:param name="db2html.co.background_color.hover" select="'#333333'"/>

<!--@@==========================================================================
db2html.co.border_color.hover
The border color for callout dingbats when hovering.

REMARK: Describe this param
-->
<xsl:param name="db2html.co.border_color.hover" select="'#333333'"/>


<!--**==========================================================================
db2html.co.dingbat
Creates a callout dingbat for a #{co} element
$co: The #{co} element to create a callout dingbat for

REMARK: Describe this template
-->
<xsl:template name="db2html.co.dingbat">
  <xsl:param name="co" select="."/>
  <span class="co">
    <xsl:value-of select="count(preceding::co) + 1"/>
  </span>
</xsl:template>


<!--**==========================================================================
db2html.co.dingbats
Renders a callout dingbat for each #{co} referenced in ${arearefs}
$arearefs: A space-separated list of #{co} elements

REMARK: Describe this template
-->
<xsl:template name="db2html.co.dingbats">
  <xsl:param name="arearefs" select="@arearefs"/>
  <!-- FIXME -->
</xsl:template>


<!--**==========================================================================
db2html.callout.css
Outputs CSS that controls the appearance of callouts

REMARK: Describe this template
-->
<xsl:template name="db2html.callout.css">
<xsl:text>
span.co {
  margin-left: 0.2em; margin-right: 0.2em;
  padding-left: 0.4em; padding-right: 0.4em;
  border: solid 1px </xsl:text>
  <xsl:value-of select="$db2html.co.border_color"/><xsl:text>;
  -moz-border-radius: 8px;
  background-color: </xsl:text>
  <xsl:value-of select="$db2html.co.background_color"/><xsl:text>;
  color: </xsl:text>
  <xsl:value-of select="$db2html.co.color"/><xsl:text>;
  font-size: 8px;
}
span.co:hover {
  border-color: </xsl:text>
  <xsl:value-of select="$db2html.co.border_color.hover"/><xsl:text>;
  background-color: </xsl:text>
  <xsl:value-of select="$db2html.co.background_color.hover"/><xsl:text>;
  color: </xsl:text>
  <xsl:value-of select="$db2html.co.color.hover"/><xsl:text>;
}
span.co a { text-decoration: none; }
span.co a:hover { text-decoration: none; }
</xsl:text>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = co = -->
<xsl:template match="co">
  <xsl:call-template name="db2html.co.dingbat"/>
</xsl:template>

</xsl:stylesheet>
