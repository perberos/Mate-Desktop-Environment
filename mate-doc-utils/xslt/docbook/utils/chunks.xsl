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

<xsl:output method="text" encoding="utf-8"/>

<xsl:include href="../common/db-chunk.xsl"/>
<xsl:include href="../common/db-xref.xsl"/>

<xsl:template match="/">
  <xsl:apply-templates>
    <xsl:with-param name="depth_of_chunk" select="0"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="
	      appendix  | article  | bibliography | book     |
	      chatper   | colophon | dedication   | glossary |
	      glossdiv  | index    | lot          | part     |
	      preface   | refentry | reference    | sect1    |
	      sect2     | sect3    | sect4        | sect5    |
	      section   | setindex | simplesect   | toc      ">
  <xsl:param name="depth_of_chunk" select="0"/>
  <xsl:call-template name="db.xref.target">
    <xsl:with-param name="linkend" select="@id"/>
    <xsl:with-param name="target" select="."/>
  </xsl:call-template>
  <xsl:text>&#x000A;</xsl:text>

  <xsl:if test="$depth_of_chunk &lt; $db.chunk.max_depth">
    <xsl:apply-templates>
      <xsl:with-param name="depth_of_chunk"
		      select="$depth_of_chunk + 1"/>
    </xsl:apply-templates>
  </xsl:if>
</xsl:template>

<xsl:template match="node()"/>

</xsl:stylesheet>
