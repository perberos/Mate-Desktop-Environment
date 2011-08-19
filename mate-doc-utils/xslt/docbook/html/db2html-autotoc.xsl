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
                xmlns:set="http://exslt.org/sets"
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="set"
                version="1.0">

<!--!!==========================================================================
DocBook to HTML - Tables of Contents
:Requires: db-label db-xref db2html-xref gettext

This module provides templates to create tables of contents from any
division-level elements.
-->


<!--**==========================================================================
db2html.autotoc
Creates a table of contents for a given element
$node: The element to create a table of contents for
$show_info: Whether to include a link to the info page
$is_info: Whether this contents list is for the info page
$show_title: Whether to give the contents list a title
$selected: A currently-selected page
$divisions: The division-level child elements of ${node}
$toc_depth: How deep nested contents should be listed.
$labels: Whether to generate labels
$titleabbrev: Whether to use #{titleabbrev} instead of #{title}

This template creates a table of contents for a given division-level element.
The calling template should pass the division-level child elements in the
${divisions} parameter.  Nested divisions will be listed to the depth
specified in the ${toc_depth} parameter.  If the ${selected} parameter is
set to an existing element, then this template will only output ancestors
and siblings of ancestors of the selected element.  This effectively creates
a tree which is expanded sufficiently to show a particular element.

This template accepts a number of parameters that control the style of the
table of contents.  The ${show_info} parameter specifies whether a link to
the info page ("About This Document") should be shown.  The ${is_info}
parameter specifies whether the info page should be treated as the selected
page.  If the info page is selected, it will not be displayed as a link.

The ${show_title} specifies whether a title should be placed at the top of
the table of contents.  The ${labels} parameter specifies whether to place
section numbers as labels before each element in the list.  Finally, the
${titleabbrev} element specifies whether list elements should use the
#{titleabbrev} of each element for the link text, if available.
-->
<xsl:template name="db2html.autotoc">
  <xsl:param name="node" select="."/>
  <xsl:param name="show_info" select="false()"/>
  <xsl:param name="is_info" select="false()"/>
  <xsl:param name="show_title" select="false()"/>
  <xsl:param name="selected" select="/false"/>
  <xsl:param name="divisions"/>

  <xsl:param name="toc_depth" select="1"/>

  <xsl:param name="labels" select="true()"/>
  <xsl:param name="titleabbrev" select="false()"/>
  <xsl:if test="($selected = false()) or ($node = $selected/ancestor-or-self::*)">
    <div class="autotoc">
      <xsl:if test="$show_title">
        <div class="title autotoc-title">
          <xsl:call-template name="l10n.gettext">
            <xsl:with-param name="msgid" select="'Contents'"/>
          </xsl:call-template>
        </div>
      </xsl:if>
      <ul>
        <xsl:if test="$show_info">
          <li>
            <xsl:choose>
              <xsl:when test="$is_info">
                <xsl:call-template name="l10n.gettext">
                  <xsl:with-param name="msgid" select="'About This Document'"/>
                </xsl:call-template>
              </xsl:when>
              <xsl:otherwise>
                <a>
                  <xsl:attribute name="href">
                    <xsl:call-template name="db.xref.target">
                      <xsl:with-param name="linkend" select="$db.chunk.info_basename"/>
                      <xsl:with-param name="is_chunk" select="true()"/>
                    </xsl:call-template>
                  </xsl:attribute>
                  <xsl:variable name="text">
                    <xsl:call-template name="l10n.gettext">
                      <xsl:with-param name="msgid" select="'About This Document'"/>
                    </xsl:call-template>
                  </xsl:variable>
                  <xsl:attribute name="title">
                    <xsl:value-of select="$text"/>
                  </xsl:attribute>
                  <xsl:value-of select="$text"/>
                </a>
              </xsl:otherwise>
            </xsl:choose>
          </li>
        </xsl:if>
        <xsl:for-each select="$divisions">
          <xsl:apply-templates mode="db2html.autotoc.mode" select=".">
            <xsl:with-param name="is_info" select="$is_info"/>
            <xsl:with-param name="selected" select="$selected"/>
            <xsl:with-param name="toc_depth" select="$toc_depth - 1"/>
            <xsl:with-param name="labels" select="$labels"/>
            <xsl:with-param name="titleabbrev" select="$titleabbrev"/>
          </xsl:apply-templates>
        </xsl:for-each>
      </ul>
    </div>
  </xsl:if>
</xsl:template>


<!--%%==========================================================================
db2html.autotoc.mode
Renders a TOC entry for an element and its children
$is_info: Whether this contents list is for the info page
$selected: A currently-selected page
$toc_depth: How deep to create entries in the table of contents
$labels: Whether to generate labels
$titleabbrev: Whether to use #{titleabbrev} instead of #{title}

This mode outputs a single element in a table of contents.  If the
${toc_depth} parameter is non-zero, then templates implementing this
mode should call *{db2html.autotoc} in their context node, passing
their division-level child elements and decrementing ${toc_depth}
by one.

For a description of the other parameters, see *{db2html.autotoc}.
-->
<xsl:template mode="db2html.autotoc.mode" match="*">
  <xsl:param name="is_info" select="false()"/>
  <xsl:param name="selected" select="/false"/>
  <xsl:param name="toc_depth" select="0"/>
  <xsl:param name="labels" select="true()"/>
  <xsl:param name="titleabbrev" select="false()"/>
  <xsl:variable name="xrefstyle">
    <xsl:text>role:title</xsl:text>
    <xsl:if test="$titleabbrev">
      <xsl:text>abbrev</xsl:text>
    </xsl:if>
  </xsl:variable>
  <li>
    <xsl:if test="$labels">
      <span class="label">
        <xsl:call-template name="db.label">
          <xsl:with-param name="node" select="."/>
          <xsl:with-param name="role" select="'li'"/>
        </xsl:call-template>
      </span>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="set:has-same-node(., $selected) and not($is_info)">
        <xsl:call-template name="db.xref.content">
          <xsl:with-param name="linkend" select="@id"/>
          <xsl:with-param name="target" select="."/>
          <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db2html.xref">
          <xsl:with-param name="linkend" select="@id"/>
          <xsl:with-param name="target" select="."/>
          <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$toc_depth &gt; 0">
      <xsl:call-template name="db2html.autotoc">
        <xsl:with-param name="selected" select="$selected"/>
        <xsl:with-param name="toc_depth" select="$toc_depth"/>
        <xsl:with-param
            name="divisions"
            select="appendix  | article  | bibliography | bibliodiv  | book     |
                    chapter   | colophon | dedication   | glossary   | glossdiv |
                    index     | lot      | part         | preface    | refentry |
                    reference | sect1    | sect2        | sect3      | sect4    |
                    sect5     | section  | setindex     | simplesect | toc      "/>
        <xsl:with-param name="labels" select="$labels"/>
        <xsl:with-param name="titleabbrev" select="$titleabbrev"/>
      </xsl:call-template>
    </xsl:if>
  </li>
</xsl:template>

<!-- = refentry % db2html.autotoc.mode = -->
<xsl:template mode="db2html.autotoc.mode" match="refentry">
  <xsl:param name="is_info" select="false()"/>
  <xsl:param name="selected" select="/false"/>
  <xsl:param name="toc_depth" select="0"/>
  <xsl:param name="labels" select="true()"/>
  <xsl:param name="titleabbrev" select="false()"/>
  <xsl:variable name="xrefstyle">
    <xsl:text>role:title</xsl:text>
    <xsl:if test="$titleabbrev">
      <xsl:text>abbrev</xsl:text>
    </xsl:if>
  </xsl:variable>
  <li>
    <xsl:choose>
      <xsl:when test="set:has-same-node(., $selected)">
        <xsl:call-template name="db.xref.content">
          <xsl:with-param name="linkend" select="@id"/>
          <xsl:with-param name="target" select="."/>
          <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db2html.xref">
          <xsl:with-param name="linkend" select="@id"/>
          <xsl:with-param name="target" select="."/>
          <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$labels">
      <xsl:if test="refnamediv/refpurpose">
        <xsl:call-template name="l10n.gettext">
          <xsl:with-param name="msgid" select="' — '"/>
        </xsl:call-template>
        <xsl:apply-templates select="refnamediv/refpurpose[1]"/>
      </xsl:if>
    </xsl:if>
  </li>
</xsl:template>

</xsl:stylesheet>
