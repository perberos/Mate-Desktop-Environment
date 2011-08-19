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

<xsl:import href="../../gettext/gettext.xsl"/>

<xsl:output method="text" encoding="utf-8"/>

<xsl:template match="/">
  <xsl:if test="//note[@role = 'bug'][1]">
    <xsl:text>admon-bug.png&#x000A;</xsl:text>
  </xsl:if>
  <xsl:if test="//caution[1]">
    <xsl:text>admon-caution.png&#x000A;</xsl:text>
  </xsl:if>
  <xsl:if test="//important[1]">
    <xsl:text>admon-important.png&#x000A;</xsl:text>
  </xsl:if>
  <xsl:if test="//note[not(@role) or @role != 'bug'][1]">
    <xsl:text>admon-note.png&#x000A;</xsl:text>
  </xsl:if>
  <xsl:if test="//tip[1]">
    <xsl:text>admon-tip.png&#x000A;</xsl:text>
  </xsl:if>
  <xsl:if test="//warning[1]">
    <xsl:text>admon-warning.png&#x000A;</xsl:text>
  </xsl:if>
  <xsl:if test="//blockquote[1]">
    <xsl:call-template name="l10n.gettext">
      <xsl:with-param name="msgid" select="'blockquote-watermark-201C.png'"/>
    </xsl:call-template>
    <xsl:text>&#x000A;</xsl:text>
  </xsl:if>
  <xsl:if test="//classsynopsis[@language = 'cpp'][1] or //programlisting[@language = 'cpp']">
    <xsl:text>watermark-code-cpp.png&#x000A;</xsl:text>
  </xsl:if>
  <xsl:if test="//classsynopsis[@language = 'python'][1] or //programlisting[@language = 'python']">
    <xsl:text>watermark-code-python.png&#x000A;</xsl:text>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
