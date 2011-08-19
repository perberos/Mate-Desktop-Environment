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
DocBook to HTML - Division Elements
:Requires: db-chunk db-label db-title db-xref db2html-autotoc db2html-css db2html-footnote db2html-info db2html-xref gettext

REMARK: Describe this module
-->


<!--@@==========================================================================
db2html.linktrail
Whether to place a link trail under the header

This boolean parameter specifies whether a block containing links to
ancestor elements should be included under the header.
-->
<xsl:param name="db2html.linktrail" select="true()"/>

<!--@@==========================================================================
db2html.navbar.top
Whether to place a navigation bar at the top of the page

This boolean parameter specifies whether a block containing navigation
links should be placed at the top of the page.  The top navigation bar
is inserted by *{db2html.division.top}, so this parameter may have no
effect if that template has been overridden.
-->
<xsl:param name="db2html.navbar.top" select="true()"/>

<!--@@==========================================================================
db2html.navbar.bottom
Whether to place a navigation bar at the bottom of the page

This boolean parameter specifies whether a block containing navigation
links should be placed at the bottom of the page.  The bottom navigation
bar is inserted by *{db2html.division.bottom}, so this parameter may have
no effect if that template has been overridden.
-->
<xsl:param name="db2html.navbar.bottom" select="true()"/>

<!--@@==========================================================================
db2html.sidenav
Whether to create a navigation sidebar

This boolean parameter specifies whether a full navigation tree in a sidebar.
The navigation sidebar is inserted by *{db2html.division.sidebar}, so this
parameter may have no effect if that template has been overridden.
-->
<xsl:param name="db2html.sidenav" select="true()"/>


<!--**==========================================================================
db2html.division.html
Renders a complete HTML page for a division element
$node: The element to create an HTML page for
$info: The info child element of ${node}
$template: The named template to call to create the page
$depth_of_chunk: The depth of the containing chunk in the document
$prev_id: The id of the previous page
$next_id: The id of the next page

REMARK: Put in a word about the chunk flow; talk about what templates get called
-->
<xsl:template name="db2html.division.html">
  <xsl:param name="node" select="."/>
  <xsl:param name="info" select="/false"/>
  <xsl:param name="template"/>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="prev_id">
    <xsl:choose>
      <xsl:when test="$depth_of_chunk = 0">
        <xsl:if test="$info and $db.chunk.info_chunk">
          <xsl:value-of select="$db.chunk.info_basename"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db.chunk.chunk-id.axis">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="axis" select="'previous'"/>
          <xsl:with-param name="depth_in_chunk" select="0"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="next_id">
    <xsl:call-template name="db.chunk.chunk-id.axis">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="axis" select="'next'"/>
      <xsl:with-param name="depth_in_chunk" select="0"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:variable name="prev_node" select="key('idkey', $prev_id)"/>
  <xsl:variable name="next_node" select="key('idkey', $next_id)"/>
  <!-- FIXME -->
  <html>
    <head>
      <title>
        <xsl:variable name="title">
          <xsl:call-template name="db.title">
            <xsl:with-param name="node" select="$node"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="normalize-space($title)"/>
      </title>
      <xsl:if test="string($prev_id) != ''">
        <link rel="previous">
          <xsl:attribute name="href">
            <xsl:call-template name="db.xref.target">
              <xsl:with-param name="linkend" select="$prev_id"/>
              <xsl:with-param name="target" select="$prev_node"/>
              <xsl:with-param name="is_chunk" select="true()"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="title">
            <xsl:call-template name="db.title">
              <xsl:with-param name="node" select="$prev_node"/>
            </xsl:call-template>
          </xsl:attribute>
        </link>
      </xsl:if>
      <xsl:if test="string($next_id) != ''">
        <link rel="next">
          <xsl:attribute name="href">
            <xsl:call-template name="db.xref.target">
              <xsl:with-param name="linkend" select="$next_id"/>
              <xsl:with-param name="target" select="$next_node"/>
              <xsl:with-param name="is_chunk" select="true()"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="title">
            <xsl:call-template name="db.title">
              <xsl:with-param name="node" select="$next_node"/>
            </xsl:call-template>
          </xsl:attribute>
        </link>
      </xsl:if>
      <xsl:if test="/*[1] != $node">
        <link rel="top">
          <xsl:attribute name="href">
            <xsl:call-template name="db.xref.target">
              <xsl:with-param name="linkend" select="/*[1]/@id"/>
              <xsl:with-param name="target" select="/*[1]"/>
              <xsl:with-param name="is_chunk" select="true()"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="title">
            <xsl:call-template name="db.title">
              <xsl:with-param name="node" select="/*[1]"/>
            </xsl:call-template>
          </xsl:attribute>
        </link>
      </xsl:if>
      <xsl:call-template name="db2html.css">
        <xsl:with-param name="css_file" select="$depth_of_chunk = 0"/>
      </xsl:call-template>
      <xsl:call-template name="db2html.division.head.extra"/>
    </head>
    <body>
      <xsl:call-template name="db2html.division.top">
        <xsl:with-param name="node" select="$node"/>
        <xsl:with-param name="info" select="$info"/>
        <xsl:with-param name="template" select="$template"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        <xsl:with-param name="prev_id" select="$prev_id"/>
        <xsl:with-param name="next_id" select="$next_id"/>
        <xsl:with-param name="prev_node" select="$prev_node"/>
        <xsl:with-param name="next_node" select="$next_node"/>
      </xsl:call-template>
      <xsl:variable name="sidebar">
        <xsl:call-template name="db2html.division.sidebar">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="info" select="$info"/>
          <xsl:with-param name="template" select="$template"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
          <xsl:with-param name="prev_id" select="$prev_id"/>
          <xsl:with-param name="next_id" select="$next_id"/>
          <xsl:with-param name="prev_node" select="$prev_node"/>
          <xsl:with-param name="next_node" select="$next_node"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:copy-of select="$sidebar"/>
      <div>
        <xsl:attribute name="class">
          <xsl:text>body</xsl:text>
          <xsl:if test="$sidebar != ''">
            <xsl:text> body-sidebar</xsl:text>
          </xsl:if>
        </xsl:attribute>
        <xsl:choose>
          <xsl:when test="$template = 'info'">
            <xsl:call-template name="db2html.info.div">
              <xsl:with-param name="node" select="$node"/>
              <xsl:with-param name="info" select="$info"/>
              <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="$node">
              <xsl:with-param name="depth_in_chunk" select="0"/>
              <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
            </xsl:apply-templates>
          </xsl:otherwise>
        </xsl:choose>
      </div>
      <xsl:call-template name="db2html.division.bottom">
        <xsl:with-param name="node" select="$node"/>
        <xsl:with-param name="info" select="$info"/>
        <xsl:with-param name="template" select="$template"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        <xsl:with-param name="prev_id" select="$prev_id"/>
        <xsl:with-param name="next_id" select="$next_id"/>
        <xsl:with-param name="prev_node" select="$prev_node"/>
        <xsl:with-param name="next_node" select="$next_node"/>
      </xsl:call-template>
    </body>
  </html>
</xsl:template>


<!--**==========================================================================
db2html.division.div
Renders the content of a division element, chunking children if necessary
$node: The element to render the content of
$info: The info child element of ${node}
$title_node: The element containing the title of ${node}
$subtitle_node: The element containing the subtitle of ${node}
$title_content: The title for divisions lacking a #{title} tag
$subtitle_content: The subtitle for divisions lacking a #{subtitle} tag
$entries: The entry-style child elements
$divisions: The division-level child elements
$callback: Whether to process ${node} in %{db2html.division.div.content.mode}
$depth_in_chunk: The depth of ${node} in the containing chunk
$depth_of_chunk: The depth of the containing chunk in the document
$chunk_divisions: Whether to create new documents for ${divisions}
$chunk_info: Whether to create a new document for a title page
$autotoc_depth: How deep to create contents listings of ${divisions}
$lang: The locale of the text in ${node}
$dir: The text direction, either #{ltr} or #{rtl}

REMARK: Talk about some of the parameters
-->
<xsl:template name="db2html.division.div">
  <xsl:param name="node" select="."/>
  <xsl:param name="info" select="/false"/>
  <xsl:param name="title_node"
             select="($node/title | $info/title)[last()]"/>
  <xsl:param name="subtitle_node"
             select="($node/subtitle | $info/subtitle)[last()]"/>
  <xsl:param name="title_content"/>
  <xsl:param name="subtitle_content"/>
  <xsl:param name="entries" select="/false"/>
  <xsl:param name="divisions" select="/false"/>
  <xsl:param name="callback" select="false()"/>
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <!-- FIXME: these two parameters don't make much sense now -->
  <xsl:param name="chunk_divisions"
             select="($depth_in_chunk = 0) and
                     ($depth_of_chunk &lt; $db.chunk.max_depth)"/>
  <xsl:param name="chunk_info"
             select="($depth_of_chunk = 0) and
                     ($depth_in_chunk = 0 and $info)"/>
  <xsl:param name="autotoc_depth" select="number(boolean($divisions))"/>
  <xsl:param name="lang" select="$node/@lang"/>
  <xsl:param name="dir" select="false()"/>

  <div class="division {local-name($node)}">
    <xsl:choose>
      <xsl:when test="$dir = 'ltr' or $dir = 'rtl'">
        <xsl:attribute name="dir">
          <xsl:value-of select="$dir"/>
        </xsl:attribute>
      </xsl:when>
      <xsl:when test="$lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="$lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
    </xsl:choose>
    <xsl:call-template name="db2html.anchor">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
    <xsl:call-template name="db2html.header">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="title_node" select="$title_node"/>
      <xsl:with-param name="subtitle_node" select="$subtitle_node"/>
      <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      <xsl:with-param name="generate_label" select="$depth_in_chunk != 0"/>
      <xsl:with-param name="title_content" select="$title_content"/>
      <xsl:with-param name="subtitle_content" select="$subtitle_content"/>
    </xsl:call-template>
    <xsl:if test="$db2html.linktrail and $depth_in_chunk = 0">
      <xsl:call-template name="db2html.linktrail">
        <xsl:with-param name="node" select="$node"/>
      </xsl:call-template>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="$callback">
        <xsl:apply-templates mode="db2html.division.div.content.mode" select="$node">
          <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        </xsl:apply-templates>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="nots" select="$divisions | $entries | $title_node | $subtitle_node"/>
        <xsl:apply-templates select="*[not(set:has-same-node(., $nots))]">
          <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk + 1"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        </xsl:apply-templates>
        <xsl:if test="$entries">
          <div class="block">
            <dl class="{local-name($node)}">
              <xsl:apply-templates select="$entries">
                <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk + 1"/>
                <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
              </xsl:apply-templates>
            </dl>
          </div>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$autotoc_depth != 0">
      <xsl:call-template name="db2html.autotoc">
        <xsl:with-param name="node" select="$node"/>
        <xsl:with-param name="title" select="true()"/>
        <xsl:with-param name="divisions" select="$divisions"/>
        <xsl:with-param name="toc_depth" select="$autotoc_depth"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="not($chunk_divisions)">
      <xsl:apply-templates select="$divisions">
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk + 1"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:apply-templates>
    </xsl:if>
    <xsl:if test="$depth_in_chunk = 0">
      <xsl:call-template name="db2html.footnote.footer">
        <xsl:with-param name="node" select="$node"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:if>
  </div>
</xsl:template>


<!--%%==========================================================================
db2html.division.div.content.mode
Renders the block-level content of a division element
$depth_in_chunk: The depth of the context element in the containing chunk
$depth_of_chunk: The depth of the containing chunk in the document

REMARK: Talk about how this works with #{callback}
-->
<xsl:template mode="db2html.division.div.content.mode" match="*"/>


<!--**==========================================================================
db2html.header
Generates a header with a title and optional subtitle
$node: The element containing the title
$title_node: The #{title} element to render
$subtitle_node: The #{subtitle} element to render
$depth_in_chunk: The depth of ${node} in the containing chunk
$depth_of_chunk: The depth of the containing chunk in the document
$generate_label: Whether to generate a label in the title
$title_content: An optional string containing the title
$subtitle_content: An optional string containing the subtitle

REMARK: Talk about the different kinds of title blocks
-->
<xsl:template name="db2html.header">
  <xsl:param name="node" select="."/>
  <xsl:param name="title_node" select="$node/title"/>
  <xsl:param name="subtitle_node" select="$node/subtitle"/>
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="generate_label" select="true()"/>
  <xsl:param name="title_content"/>
  <xsl:param name="subtitle_content"/>

  <xsl:variable name="title_h">
    <xsl:choose>
      <xsl:when test="$depth_in_chunk &lt; 7">
        <xsl:value-of select="concat('h', $depth_in_chunk + 1)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>h7</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="subtitle_h">
    <xsl:choose>
      <xsl:when test="$depth_in_chunk &lt; 6">
        <xsl:value-of select="concat('h', $depth_in_chunk + 2)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>h7</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <div class="header">
    <xsl:element name="{$title_h}" namespace="{$db2html.namespace}">
      <xsl:attribute name="class">
        <xsl:value-of select="concat(local-name($node), ' ', local-name($title_node))"/>
      </xsl:attribute>
      <span class="title">
        <xsl:if test="$title_node">
          <xsl:call-template name="db2html.anchor">
            <xsl:with-param name="node" select="$title_node"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:if test="$generate_label">
          <span class="label">
            <xsl:call-template name="db.label">
              <xsl:with-param name="node" select="$node"/>
              <xsl:with-param name="role" select="'header'"/>
              <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
            </xsl:call-template>
          </span>
        </xsl:if>
        <xsl:choose>
          <xsl:when test="$title_content">
            <xsl:value-of select="$title_content"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="$title_node/node()"/>
          </xsl:otherwise>
        </xsl:choose>
      </span>
    </xsl:element>
    <xsl:if test="$subtitle_node or $subtitle_content">
      <xsl:element name="{$subtitle_h}" namespace="{$db2html.namespace}">
        <xsl:attribute name="class">
          <xsl:value-of select="concat(local-name($node), ' ', local-name($subtitle_node))"/>
        </xsl:attribute>
        <xsl:choose>
          <xsl:when test="$subtitle_content">
            <xsl:value-of select="$subtitle_content"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="$subtitle_node/node()"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:element>
    </xsl:if>
  </div>
</xsl:template>


<!--**==========================================================================
db2html.linktrail
Generates links to pages from ancestor elements
$node: The element to generate links for

REMARK: Describe this
-->
<xsl:template name="db2html.linktrail">
  <xsl:param name="node"/>
  <xsl:if test="$node/ancestor::*">
    <ul class="linktrail">
      <!-- The parens put the nodes back in document order -->
      <xsl:for-each select="($node/ancestor::*)">
        <li>
          <xsl:attribute name="class">
            <xsl:text>linktrail</xsl:text>
            <xsl:choose>
              <xsl:when test="last() = 1">
                <xsl:text> linktrail-only</xsl:text>
              </xsl:when>
              <xsl:when test="position() = 1">
                <xsl:text> linktrail-first</xsl:text>
              </xsl:when>
              <xsl:when test="position() = last()">
                <xsl:text> linktrail-last</xsl:text>
              </xsl:when>
            </xsl:choose>
          </xsl:attribute>
          <a class="linktrail">
            <xsl:attribute name="href">
              <xsl:call-template name="db.xref.target">
                <xsl:with-param name="linkend" select="@id"/>
                <xsl:with-param name="target" select="."/>
                <xsl:with-param name="is_chunk" select="true()"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:attribute name="title">
              <xsl:call-template name="db.xref.tooltip">
                <xsl:with-param name="linkend" select="@id"/>
                <xsl:with-param name="target" select="."/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:call-template name="db.titleabbrev">
              <xsl:with-param name="node" select="."/>
            </xsl:call-template>
          </a>
        </li>
      </xsl:for-each>
    </ul>
  </xsl:if>
</xsl:template>


<!--**==========================================================================
db2html.navbar
Generates navigation links for a page
$node: The element to generate links for
$prev_id: The id of the previous page
$next_id: The id of the next page
$prev_node: The element of the previous page
$next_node: The element of the next page
$position: Where the block is positioned on the pages, either 'top' or 'bottom'

REMARK: Document this template
-->
<xsl:template name="db2html.navbar">
  <xsl:param name="node"/>
  <xsl:param name="prev_id">
    <xsl:choose>
      <xsl:when test="$depth_of_chunk = 0">
        <xsl:if test="$info and $db.chunk.info_chunk">
          <xsl:value-of select="$db.chunk.info_basename"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db.chunk.chunk-id.axis">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="axis" select="'previous'"/>
          <xsl:with-param name="depth_in_chunk" select="0"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="next_id">
    <xsl:call-template name="db.chunk.chunk-id.axis">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="axis" select="'next'"/>
      <xsl:with-param name="depth_in_chunk" select="0"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="prev_node" select="key('idkey', $prev_id)"/>
  <xsl:param name="next_node" select="key('idkey', $next_id)"/>
  <xsl:param name="position" select="'top'"/>
  <div class="navbar navbar-{$position}">
    <!-- FIXME: rtl -->
    <table class="navbar"><tr>
      <td class="navbar-prev">
        <xsl:if test="$prev_id != ''">
          <a class="navbar-prev">
            <xsl:attribute name="href">
              <xsl:call-template name="db.xref.target">
                <xsl:with-param name="linkend" select="$prev_id"/>
                <xsl:with-param name="target" select="$prev_node"/>
                <xsl:with-param name="is_chunk" select="true()"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:choose>
              <xsl:when test="$prev_id = $db.chunk.info_basename">
                <xsl:variable name="text">
                  <xsl:call-template name="l10n.gettext">
                    <xsl:with-param name="msgid" select="'About This Document'"/>
                  </xsl:call-template>
                </xsl:variable>
                <xsl:attribute name="title">
                  <xsl:value-of select="$text"/>
                </xsl:attribute>
                <xsl:value-of select="$text"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:attribute name="title">
                  <xsl:call-template name="db.xref.tooltip">
                    <xsl:with-param name="linkend" select="$prev_id"/>
                    <xsl:with-param name="target" select="$prev_node"/>
                  </xsl:call-template>
                </xsl:attribute>
                <xsl:call-template name="db.titleabbrev">
                  <xsl:with-param name="node" select="$prev_node"/>
                </xsl:call-template>
              </xsl:otherwise>
            </xsl:choose>
          </a>
        </xsl:if>
      </td>
      <td class="navbar-next">
        <xsl:if test="$next_id != ''">
          <a class="navbar-next">
            <xsl:attribute name="href">
              <xsl:call-template name="db.xref.target">
                <xsl:with-param name="linkend" select="$next_id"/>
                <xsl:with-param name="is_chunk" select="true()"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:attribute name="title">
              <xsl:call-template name="db.xref.tooltip">
                <xsl:with-param name="linkend" select="$next_id"/>
                <xsl:with-param name="target"  select="$next_node"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:call-template name="db.titleabbrev">
              <xsl:with-param name="node" select="$next_node"/>
            </xsl:call-template>
          </a>
        </xsl:if>
      </td>
    </tr></table>
  </div>
</xsl:template>


<!--**==========================================================================
db2html.sidenav
Generates a navigation sidebar
$node: The currently-selected division element
$template: The named template to call to create the page

REMARK: Document this template
-->
<xsl:template name="db2html.sidenav">
  <xsl:param name="node" select="."/>
  <xsl:param name="template"/>
  <div class="sidenav">
    <xsl:call-template name="db2html.autotoc">
      <xsl:with-param name="node" select="/"/>
      <xsl:with-param name="show_info" select="$db.chunk.info_chunk"/>
      <xsl:with-param name="is_info" select="$template = 'info'"/>
      <xsl:with-param name="selected" select="$node"/>
      <xsl:with-param name="divisions" select="/*"/>
      <xsl:with-param name="toc_depth" select="$db.chunk.max_depth + 1"/>
      <xsl:with-param name="labels" select="false()"/>
      <xsl:with-param name="titleabbrev" select="true()"/>
    </xsl:call-template>
  </div>
</xsl:template>


<!--**==========================================================================
db2html.division.head.extra
FIXME
:Stub: true

REMARK: Describe this stub template.
-->
<xsl:template name="db2html.division.head.extra"/>


<!--**==========================================================================
db2html.division.top
FIXME
$node: The division element being rendered
$info: The info child element of ${node}
$template: The named template to call to create the page
$depth_of_chunk: The depth of the containing chunk in the document
$prev_id: The id of the previous page
$next_id: The id of the next page
$prev_node: The element of the previous page
$next_node: The element of the next page

REMARK: Describe this template
-->
<xsl:template name="db2html.division.top">
  <xsl:param name="node"/>
  <xsl:param name="info" select="/false"/>
  <xsl:param name="template"/>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="prev_id">
    <xsl:choose>
      <xsl:when test="$depth_of_chunk = 0">
        <xsl:if test="$info and $db.chunk.info_chunk">
          <xsl:value-of select="$db.chunk.info_basename"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db.chunk.chunk-id.axis">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="axis" select="'previous'"/>
          <xsl:with-param name="depth_in_chunk" select="0"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="next_id">
    <xsl:call-template name="db.chunk.chunk-id.axis">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="axis" select="'next'"/>
      <xsl:with-param name="depth_in_chunk" select="0"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="prev_node" select="key('idkey', $prev_id)"/>
  <xsl:param name="next_node" select="key('idkey', $next_id)"/>
  <xsl:if test="$db2html.navbar.top">
    <xsl:call-template name="db2html.navbar">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="prev_id" select="$prev_id"/>
      <xsl:with-param name="next_id" select="$next_id"/>
      <xsl:with-param name="prev_node" select="$prev_node"/>
      <xsl:with-param name="next_node" select="$next_node"/>
      <xsl:with-param name="position" select="'top'"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>


<!--**==========================================================================
db2html.division.sidebar
FIXME
$node: The division element being rendered
$info: The info child element of ${node}
$template: The named template to call to create the page
$depth_of_chunk: The depth of the containing chunk in the document
$prev_id: The id of the previous page
$next_id: The id of the next page
$prev_node: The element of the previous page
$next_node: The element of the next page

REMARK: Describe this template
-->
<xsl:template name="db2html.division.sidebar">
  <xsl:param name="node"/>
  <xsl:param name="info" select="/false"/>
  <xsl:param name="template"/>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="prev_id">
    <xsl:choose>
      <xsl:when test="$depth_of_chunk = 0">
        <xsl:if test="$info and $db.chunk.info_chunk">
          <xsl:value-of select="$db.chunk.info_basename"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db.chunk.chunk-id.axis">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="axis" select="'previous'"/>
          <xsl:with-param name="depth_in_chunk" select="0"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="next_id">
    <xsl:call-template name="db.chunk.chunk-id.axis">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="axis" select="'next'"/>
      <xsl:with-param name="depth_in_chunk" select="0"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="prev_node" select="key('idkey', $prev_id)"/>
  <xsl:param name="next_node" select="key('idkey', $next_id)"/>
  <xsl:if test="$db2html.sidenav">
    <div class="sidebar">
      <xsl:call-template name="db2html.sidenav">
        <xsl:with-param name="node" select="$node"/>
        <xsl:with-param name="template" select="$template"/>
      </xsl:call-template>
    </div>
  </xsl:if>
</xsl:template>


<!--**==========================================================================
db2html.division.bottom
FIXME
$node: The division element being rendered
$info: The info child element of ${node}
$template: The named template to call to create the page
$depth_of_chunk: The depth of the containing chunk in the document
$prev_id: The id of the previous page
$next_id: The id of the next page
$prev_node: The element of the previous page
$next_node: The element of the next page

REMARK: Describe this template
-->
<xsl:template name="db2html.division.bottom">
  <xsl:param name="node"/>
  <xsl:param name="info" select="/false"/>
  <xsl:param name="template"/>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="prev_id">
    <xsl:choose>
      <xsl:when test="$depth_of_chunk = 0">
        <xsl:if test="$info and $db.chunk.info_chunk">
          <xsl:value-of select="$db.chunk.info_basename"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db.chunk.chunk-id.axis">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="axis" select="'previous'"/>
          <xsl:with-param name="depth_in_chunk" select="0"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="next_id">
    <xsl:call-template name="db.chunk.chunk-id.axis">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="axis" select="'next'"/>
      <xsl:with-param name="depth_in_chunk" select="0"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="prev_node" select="key('idkey', $prev_id)"/>
  <xsl:param name="next_node" select="key('idkey', $next_id)"/>
  <xsl:if test="$db2html.navbar.bottom">
    <xsl:call-template name="db2html.navbar">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="prev_id" select="$prev_id"/>
      <xsl:with-param name="next_id" select="$next_id"/>
      <xsl:with-param name="prev_node" select="$prev_node"/>
      <xsl:with-param name="next_node" select="$next_node"/>
      <xsl:with-param name="position" select="'bottom'"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>


<!-- == Matched Templates % db.chunk.content.mode == -->

<!--#% db.chunk.content.mode -->
<xsl:template mode="db.chunk.content.mode" match="*">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = appendix % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="appendix">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="appendixinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = article % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="article">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="articleinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = bibliography % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="bibliography">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="bibliographyinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = book % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="book">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="bookinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = chapter % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="chapter">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="chapterinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = glossary % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="glossary">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="glossaryinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = part % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="part">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="partinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = preface % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="preface">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="prefaceinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = reference % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="reference">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="referenceinfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect1 % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="sect1">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="sect1info"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect2 % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="sect2">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="sect2info"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect3 % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="sect3">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="sect3info"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect4 % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="sect4">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="sect4info"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect5 % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="sect5">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="sect5info"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = section % db.chunk.content.mode = -->
<xsl:template mode="db.chunk.content.mode" match="section">
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.html">
    <xsl:with-param name="info" select="sectioninfo"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = appendix = -->
<xsl:template match="appendix">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    bibliography | glossary | index      | lot | refentry |
                    sect1        | section  | simplesect | toc "/>
    <xsl:with-param name="info" select="appendixinfo"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = article = -->
<xsl:template match="article">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    appendix | bibliography | glossary | index      | lot |
                    refentry | sect1        | section  | simplesect | toc "/>
    <xsl:with-param name="info" select="articleinfo"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = book = -->
<xsl:template match="book">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    appendix | article    | bibliography | chapter   |
                    colophon | dedication | glossary     | index     |
                    lot      | part       | preface      | reference |
                    setindex | toc        "/>
    <xsl:with-param name="info" select="bookinfo"/>
    <!-- Unlike other elements in DocBook, title comes before bookinfo -->
    <xsl:with-param name="title_node"
                    select="(title | bookinfo/title)[1]"/>
    <xsl:with-param name="subtitle_node"
                    select="(subtitle | bookinfo/subtitle)[1]"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    <xsl:with-param name="autotoc_depth" select="2"/>
  </xsl:call-template>
</xsl:template>

<!-- = chapter = -->
<xsl:template match="chapter">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    bibliography | glossary | index      | lot | refentry |
                    sect1        | section  | simplesect | toc "/>
    <xsl:with-param name="info" select="chapterinfo"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = dedication = -->
<xsl:template match="dedication">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="not(title)">
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="title_content">
          <xsl:call-template name="l10n.gettext">
            <xsl:with-param name="msgid" select="'Dedication'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = glossary = -->
<xsl:template match="glossary">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="not(title) and not(glossaryinfo/title)">
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="entries" select="glossentry"/>
        <xsl:with-param name="divisions" select="glossdiv | bibliography"/>
        <xsl:with-param name="title_content">
          <xsl:call-template name="l10n.gettext">
            <xsl:with-param name="msgid" select="'Glossary'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="info" select="glossaryinfo"/>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="entries" select="glossentry"/>
        <xsl:with-param name="divisions" select="glossdiv | bibliography"/>
        <xsl:with-param name="info" select="glossaryinfo"/>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = glossdiv = -->
<xsl:template match="glossdiv">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="entries" select="glossentry"/>
    <xsl:with-param name="divisions" select="bibliography"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = part = -->
<xsl:template match="part">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    appendix | article   | bibliography | chapter |
                    glossary | index     | lot          | preface |
                    refentry | reference | toc          "/>
    <xsl:with-param name="info" select="partinfo"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = preface = -->
<xsl:template match="preface">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    refentry | simplesect | sect1    | section      | toc  |
                    lot      | index      | glossary | bibliography "/>
    <xsl:with-param name="info" select="prefaceinfo"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = reference = -->
<xsl:template match="reference">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="refentry"/>
    <xsl:with-param name="info" select="referenceinfo"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect1 = -->
<xsl:template match="sect1">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    bibliography | glossary | index      | lot |
                    refentry     | sect2    | simplesect | toc "/>
    <xsl:with-param name="info" select="sect1info"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect2 = -->
<xsl:template match="sect2">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    bibliography | glossary | index      | lot |
                    refentry     | sect3    | simplesect | toc "/>
    <xsl:with-param name="info" select="sect2info"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect3 = -->
<xsl:template match="sect3">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    bibliography | glossary | index      | lot |
                    refentry     | sect4    | simplesect | toc "/>
    <xsl:with-param name="info" select="sect3info"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect4 = -->
<xsl:template match="sect4">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    bibliography | glossary | index      | lot |
                    refentry     | sect5    | simplesect | toc "/>
    <xsl:with-param name="info" select="sect4info"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = sect5 = -->
<xsl:template match="sect5">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    bibliography | glossary   | index | lot |
                    refentry     | simplesect | toc   "/>
    <xsl:with-param name="info" select="sect5info"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = section = -->
<xsl:template match="section">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="divisions" select="
                    bibliography | glossary | index      | lot |
                    refentry     | section  | simplesect | toc "/>
    <xsl:with-param name="info" select="sectioninfo"/>
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

<!-- = simplesect = -->
<xsl:template match="simplesect">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:call-template name="db2html.division.div">
    <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
    <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
