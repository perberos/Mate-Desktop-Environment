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
Common XSLT Utilities

REMARK: Describe this module
-->


<!--**==========================================================================
utils.strip_newlines
Strips leading or trailing newlines from a string
$string: The string to strip newlines from
$leading: Whether to strip leading newlines
$trailing: Whether to strip trailing newlines

This template strips at most one leading and one trailing newline from
${string}.  This is useful for preformatted block elements where leading and
trailing newlines are ignored to make source formatting easier for authors.
-->
<xsl:template name="utils.strip_newlines">
  <xsl:param name="string"/>
  <xsl:param name="leading" select="false()"/>
  <xsl:param name="trailing" select="false()"/>
  <xsl:choose>
    <xsl:when test="$leading">
      <xsl:variable name="new">
        <xsl:choose>
          <xsl:when test="starts-with($string, '&#x000A;')">
            <xsl:value-of select="substring($string, 2)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$string"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="$trailing">
          <xsl:call-template name="utils.strip_newlines">
            <xsl:with-param name="string" select="$new"/>
            <xsl:with-param name="leading" select="false()"/>
            <xsl:with-param name="trailing" select="true()"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$new"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$trailing">
      <xsl:choose>
        <xsl:when test="substring($string, string-length($string)) = '&#x000A;'">
          <xsl:value-of select="substring($string, 1, string-length($string) - 1 )"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$string"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
