<?xml version='1.0' encoding='UTF-8'?><!-- -*- indent-tabs-mode: nil -*- -->
<!--
xsldoc-fill.xsl - Put more information in the output from xsldoc.awk
Copyright (C) 2006 Shaun McCance <shaunm@gnome.org>

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
-->
<!--
This program is free software, but that doesn't mean you should use it.
It's a hackish bit of awk and XSLT to do inline XSLT documentation with
a simple syntax in comments.  I had originally tried to make a public
inline documentation system for XSLT using embedded XML, but it just got
very cumbersome.  XSLT should have been designed from the ground up with
an inline documentation format.

None of the existing inline documentation tools (gtk-doc, doxygen, etc.)
really work well for XSLT, so I rolled my own simple comment-based tool.
This tool is sufficient for producing the documentation I need, but I
just don't have the time or inclination to make a robust system suitable
for public use.

You are, of course, free to use any of this.  If you do proliferate this
hack, it is requested (though not required, that would be non-free) that
you atone for your actions.  A good atonement would be contributing to
free software.
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:set="http://exslt.org/sets"
                xmlns:str="http://exslt.org/strings"
                exclude-result-prefixes="set str"
                version="1.0">

<xsl:param name="basename"/>
<xsl:param name="xsl_file"/>
<xsl:variable name="xsl" select="document($xsl_file)/xsl:stylesheet"/>

<xsl:output omit-xml-declaration="yes"/>

<xsl:template match="section">
  <section id="{$basename}">
    <xsl:for-each select="@*">
      <xsl:copy-of select="."/>
    </xsl:for-each>
    <metas>
      <meta>
        <name>calls-names</name>
        <xsl:for-each select="set:distinct($xsl//xsl:call-template/@name)">
          <desc><xsl:value-of select="."/></desc>
        </xsl:for-each>
      </meta>
      <meta>
        <name>calls-modes</name>
        <xsl:for-each select="set:distinct($xsl//xsl:apply-templates/@mode)">
          <desc><xsl:value-of select="."/></desc>
        </xsl:for-each>
      </meta>
      <meta>
        <name>uses-modes</name>
        <xsl:for-each select="set:distinct($xsl//xsl:template/@mode)">
          <desc><xsl:value-of select="."/></desc>
        </xsl:for-each>
      </meta>
      <meta>
        <name>uses-params</name>
        <xsl:for-each select="set:distinct($xsl//xsl:value-of
                                [starts-with(@select, '$') and contains(@select, '.')]/@select)">
          <desc><xsl:value-of select="substring-after(., '$')"/></desc>
        </xsl:for-each>
      </meta>
      <xsl:apply-templates select="metas/meta"/>
    </metas>
    <xsl:apply-templates/>
  </section>
</xsl:template>

<xsl:template match="section/metas"/>

<xsl:template match="meta[name = 'Requires']">
  <meta>
    <name>Requires</name>
    <xsl:for-each select="str:tokenize(desc)">
      <desc><xsl:value-of select="."/></desc>
    </xsl:for-each>
  </meta>
</xsl:template>

<xsl:template match="node()">
  <xsl:copy>
    <xsl:for-each select="@*">
      <xsl:copy-of select="."/>
    </xsl:for-each>
    <xsl:apply-templates select="node()"/>
  </xsl:copy>
</xsl:template>

</xsl:stylesheet>
