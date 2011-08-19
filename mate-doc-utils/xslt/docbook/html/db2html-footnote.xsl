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
DocBook to HTML - Footnotes
:Requires: db-chunk db-label

FIXME: Describe this module
-->


<!--**==========================================================================
db2html.footnote.ref
Generates a superscript link to a footnote
$node: The #{footnote} element to process

REMARK: Describe this template
-->
<xsl:template name="db2html.footnote.ref">
  <xsl:param name="node" select="."/>
  <xsl:variable name="anchor">
    <xsl:text>-noteref-</xsl:text>
    <xsl:choose>
      <xsl:when test="$node/@id">
        <xsl:value-of select="$node/@id"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="generate-id($node)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="href">
    <xsl:text>#</xsl:text>
    <xsl:choose>
      <xsl:when test="$node/@id">
        <xsl:value-of select="$node/@id"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>-note-</xsl:text>
        <xsl:value-of select="generate-id($node)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <a name="{$anchor}"/>
  <sup>
    <a class="footnote" href="{$href}">
      <xsl:call-template name="db.number">
        <xsl:with-param name="node" select="$node"/>
      </xsl:call-template>
    </a>
  </sup>
</xsl:template>


<!--**==========================================================================
db2html.footnote.note
Generates a footnote
$node: The #{footnote} element to process

REMARK: Describe this template
-->
<xsl:template name="db2html.footnote.note">
  <xsl:param name="node" select="."/>
  <xsl:variable name="anchor">
    <xsl:choose>
      <xsl:when test="$node/@id">
        <xsl:value-of select="$node/@id"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>-note-</xsl:text>
        <xsl:value-of select="generate-id($node)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="href">
    <xsl:text>#</xsl:text>
    <xsl:text>-noteref-</xsl:text>
    <xsl:choose>
      <xsl:when test="$node/@id">
        <xsl:value-of select="$node/@id"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="generate-id($node)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <div class="footnote">
    <a name="{$anchor}"/>
    <span class="footnote-number">
      <a class="footnote-ref" href="{$href}">
        <xsl:call-template name="db.number">
          <xsl:with-param name="node" select="$node"/>
        </xsl:call-template>
      </a>
    </span>
    <xsl:apply-templates select="$node/node()"/>
  </div>
</xsl:template>


<!--**==========================================================================
db2html.footnote.footer
Generates a foot containing all the footnotes in the chunk
$node: The division element containing footnotes
$depth_of_chunk: The depth of the containing chunk in the document

REMARK: Describe this template
-->
<xsl:template name="db2html.footnote.footer">
  <xsl:param name="node" select="."/>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:variable name="notes" select="$node//footnote" />
  <xsl:if test="count($notes) != 0">
    <xsl:call-template name="db2html.footnote.footer.sibling">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      <xsl:with-param name="notes" select="$notes"/>
      <xsl:with-param name="pos" select="1"/>
      <xsl:with-param name="div" select="false()"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!--#* db2html.footnote.footer.sibling -->
<xsl:template name="db2html.footnote.footer.sibling">
  <xsl:param name="node"/>
  <xsl:param name="depth_of_chunk"/>
  <xsl:param name="notes"/>
  <xsl:param name="pos"/>
  <xsl:param name="div"/>
  <xsl:variable name="this" select="$notes[$pos]"/>
  <xsl:variable name="depth">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$this"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="($depth = $depth_of_chunk) and not($div)">
      <div class="footnotes">
        <xsl:call-template name="db2html.footnote.note">
          <xsl:with-param name="node" select="$this"/>
        </xsl:call-template>
        <xsl:if test="$pos &lt; count($notes)">
          <xsl:call-template name="db2html.footnote.footer.sibling">
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
            <xsl:with-param name="notes" select="$notes"/>
            <xsl:with-param name="pos" select="$pos + 1"/>
            <xsl:with-param name="div" select="true()"/>
          </xsl:call-template>
        </xsl:if>
      </div>
    </xsl:when>
    <xsl:otherwise>
      <xsl:if test="$depth = $depth_of_chunk">
        <xsl:call-template name="db2html.footnote.note">
          <xsl:with-param name="node" select="$this"/>
        </xsl:call-template>
      </xsl:if>
      <xsl:if test="$pos &lt; count($notes)">
        <xsl:call-template name="db2html.footnote.footer.sibling">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
          <xsl:with-param name="notes" select="$notes"/>
          <xsl:with-param name="pos" select="$pos + 1"/>
          <xsl:with-param name="div" select="$div"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2html.footnote.css
Outputs CSS that controls the appearance of footnotes

REMARK: Describe this template
-->
<xsl:template name="db2html.footnote.css">
<xsl:text>
div.footnotes { font-style: italic; font-size: 0.8em; }
div.footnote { margin-top: 1.44em; }
span.footnote-number { display: inline; padding-right: 0.83em; }
span.footnote-number + p { display: inline; }
a.footnote { text-decoration: none; font-size: 0.8em; }
a.footnote-ref { text-decoration: none; }
</xsl:text>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = footnote = -->
<xsl:template match="footnote">
  <xsl:call-template name="db2html.footnote.ref"/>
</xsl:template>

</xsl:stylesheet>
