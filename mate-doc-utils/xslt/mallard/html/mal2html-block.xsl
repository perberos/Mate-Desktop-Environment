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
                xmlns:mal="http://projectmallard.org/1.0/"
                xmlns="http://www.w3.org/1999/xhtml"
                version="1.0">

<!--!!==========================================================================
Mallard to HTML - Block Elements

REMARK: Describe this module
-->


<!--**==========================================================================
mal2html.pre

FIXME
-->
<xsl:template name="mal2html.pre">
  <xsl:param name="node" select="."/>
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <xsl:variable name="first" select="$node/node()[1]/self::text()"/>
  <xsl:variable name="last" select="$node/node()[last()]/self::text()"/>
  <pre>
    <xsl:attribute name="class">
      <xsl:value-of select="local-name($node)"/>
      <xsl:if test="$first_child">
        <xsl:text> first-child</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:if test="$first">
      <xsl:call-template name="utils.strip_newlines">
        <xsl:with-param name="string" select="$first"/>
        <xsl:with-param name="leading" select="true()"/>
        <xsl:with-param name="trailing" select="count(node()) = 1"/>
      </xsl:call-template>
    </xsl:if>
    <xsl:apply-templates mode="mal2html.inline.mode"
                         select="node()[not(self::text() and (position() = 1 or position() = last()))]"/>
    <xsl:if test="$last and (count(node()) != 1)">
      <xsl:call-template name="utils.strip_newlines">
        <xsl:with-param name="string" select="$last"/>
        <xsl:with-param name="leading" select="false()"/>
        <xsl:with-param name="trailing" select="true()"/>
      </xsl:call-template>
    </xsl:if>
  </pre>
</xsl:template>


<!-- == Matched Templates == -->

<!--%%==========================================================================
mal2html.block.mode
FIXME

FIXME
-->

<!-- = desc = -->
<xsl:template mode="mal2html.block.mode" match="mal:desc">
  <div class="desc desc-{local-name(..)}">
    <xsl:apply-templates mode="mal2html.inline.mode"/>
  </div>
</xsl:template>

<!-- = code = -->
<xsl:template mode="mal2html.block.mode" match="mal:code">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <xsl:call-template name="mal2html.pre">
    <xsl:with-param name="first_child" select="$first_child"/>
  </xsl:call-template>
</xsl:template>

<!-- = comment = -->
<xsl:template mode="mal2html.block.mode" match="mal:comment">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <xsl:if test="$mal2html.editor_mode
                or processing-instruction('mal2html.show_comment')">
    <div>
      <xsl:attribute name="class">
        <xsl:text>comment</xsl:text>
        <xsl:if test="$first_child">
          <xsl:text> first-child</xsl:text>
        </xsl:if>
      </xsl:attribute>
      <xsl:apply-templates mode="mal2html.block.mode" select="mal:title"/>
      <xsl:apply-templates mode="mal2html.block.mode" select="mal:cite"/>
      <xsl:for-each select="mal:*[not(self::mal:title or self::mal:cite)
                            and ($mal2html.editor_mode or not(self::mal:comment)
                            or processing-instruction('mal2html.show_comment'))]">
        <xsl:apply-templates mode="mal2html.block.mode" select=".">
          <xsl:with-param name="first_child" select="position() = 1"/>
        </xsl:apply-templates>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<!-- = comment/cite = -->
<xsl:template mode="mal2html.block.mode" match="mal:comment/mal:cite">
  <div class="cite cite-comment">
    <!-- FIXME: i18n -->
    <xsl:text>from </xsl:text>
    <xsl:choose>
      <xsl:when test="@href">
        <a href="{@href}">
          <xsl:apply-templates mode="mal2html.inline.mode"/>
        </a>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="mal2html.inline.mode"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="@date">
      <xsl:text> on </xsl:text>
      <xsl:value-of select="@date"/>
    </xsl:if>
  </div>
</xsl:template>

<!-- = example = -->
<xsl:template mode="mal2html.block.mode" match="mal:example">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <div>
    <xsl:attribute name="class">
      <xsl:text>example</xsl:text>
      <xsl:if test="$first_child">
        <xsl:text> first-child</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:for-each select="mal:*[$mal2html.editor_mode or not(self::mal:comment)
                          or processing-instruction('mal2html.show_comment')]">
      <xsl:apply-templates mode="mal2html.block.mode" select=".">
        <xsl:with-param name="first_child" select="position() = 1"/>
      </xsl:apply-templates>
    </xsl:for-each>
  </div>
</xsl:template>

<!-- = figure = -->
<xsl:template mode="mal2html.block.mode" match="mal:figure">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <div>
    <xsl:attribute name="class">
      <xsl:text>figure</xsl:text>
      <xsl:if test="$first_child">
        <xsl:text> first-child</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:apply-templates mode="mal2html.block.mode" select="mal:title"/>
    <div class="figure-contents">
      <xsl:for-each select="mal:*[not(self::mal:title or self::mal:desc)
                            and ($mal2html.editor_mode or not(self::mal:comment)
                            or processing-instruction('mal2html.show_comment'))]">
        <xsl:apply-templates mode="mal2html.block.mode" select=".">
          <xsl:with-param name="first_child" select="position() = 1"/>
        </xsl:apply-templates>
      </xsl:for-each>
    </div>
    <xsl:apply-templates mode="mal2html.block.mode" select="mal:desc"/>
  </div>
</xsl:template>

<!-- = listing = -->
<xsl:template mode="mal2html.block.mode" match="mal:listing">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <div>
    <xsl:attribute name="class">
      <xsl:text>listing</xsl:text>
      <xsl:if test="$first_child">
        <xsl:text> first-child</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:apply-templates mode="mal2html.block.mode" select="mal:title"/>
    <xsl:apply-templates mode="mal2html.block.mode" select="mal:desc"/>
    <div class="listing-contents">
      <xsl:for-each select="mal:*[not(self::mal:title or self::mal:desc)
                            and ($mal2html.editor_mode or not(self::mal:comment)
                            or processing-instruction('mal2html.show_comment'))]">
        <xsl:apply-templates mode="mal2html.block.mode" select=".">
          <xsl:with-param name="first_child" select="position() = 1"/>
        </xsl:apply-templates>
      </xsl:for-each>
    </div>
  </div>
</xsl:template>

<!-- = note = -->
<xsl:template mode="mal2html.block.mode" match="mal:note">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <xsl:variable name="notestyle">
    <xsl:choose>
      <xsl:when test="contains(concat(' ', @style, ' '), ' advanced ')">
        <xsl:text>advanced</xsl:text>
      </xsl:when>
      <xsl:when test="contains(concat(' ', @style, ' '), ' bug ')">
        <xsl:text>bug</xsl:text>
      </xsl:when>
      <xsl:when test="contains(concat(' ', @style, ' '), ' important ')">
        <xsl:text>important</xsl:text>
      </xsl:when>
      <xsl:when test="contains(concat(' ', @style, ' '), ' tip ')">
        <xsl:text>tip</xsl:text>
      </xsl:when>
      <xsl:when test="contains(concat(' ', @style, ' '), ' warning ')">
        <xsl:text>warning</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:variable>
  <div>
    <xsl:attribute name="class">
      <xsl:text>note</xsl:text>
      <xsl:if test="normalize-space($notestyle) != ''">
        <xsl:value-of select="concat(' note-', $notestyle)"/>
      </xsl:if>
      <xsl:if test="$first_child">
        <xsl:text> first-child</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <div class="note-inner">
      <xsl:apply-templates mode="mal2html.block.mode" select="mal:title"/>
      <div class="note-contents">
        <xsl:for-each select="mal:*[not(self::mal:title)
                              and ($mal2html.editor_mode or not(self::mal:comment)
                              or processing-instruction('mal2html.show_comment'))]">
          <xsl:apply-templates mode="mal2html.block.mode" select=".">
            <xsl:with-param name="first_child" select="position() = 1"/>
          </xsl:apply-templates>
        </xsl:for-each>
      </div>
    </div>
  </div>
</xsl:template>

<!-- = info = -->
<xsl:template mode="mal2html.block.mode" match="mal:info"/>

<!-- = p = -->
<xsl:template mode="mal2html.block.mode" match="mal:p">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <p>
    <xsl:attribute name="class">
      <xsl:text>p</xsl:text>
      <xsl:if test="$first_child">
        <xsl:text> first-child</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:apply-templates mode="mal2html.inline.mode"/>
  </p>
</xsl:template>

<!-- = quote = -->
<xsl:template mode="mal2html.block.mode" match="mal:quote">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <div class="quote">
    <xsl:attribute name="class">
      <xsl:text>quote</xsl:text>
      <xsl:if test="$first_child">
        <xsl:text> first-child</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:apply-templates mode="mal2html.block.mode" select="mal:title"/>
    <div class="quote-inner">
      <blockquote class="quote-contents">
        <xsl:for-each select="mal:*[not(self::mal:title or self::mal:cite)
                              and ($mal2html.editor_mode or not(self::mal:comment)
                              or processing-instruction('mal2html.show_comment'))]">
          <xsl:apply-templates mode="mal2html.block.mode" select=".">
            <xsl:with-param name="first_child" select="position() = 1"/>
          </xsl:apply-templates>
        </xsl:for-each>
      </blockquote>
      <xsl:apply-templates mode="mal2html.block.mode" select="mal:cite"/>
    </div>
  </div>
</xsl:template>

<!-- = quote/cite = -->
<xsl:template mode="mal2html.block.mode" match="mal:quote/mal:cite">
  <div class="cite cite-quote">
    <xsl:choose>
      <xsl:when test="@href">
        <a href="{@href}">
          <xsl:apply-templates mode="mal2html.inline.mode"/>
        </a>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="mal2html.inline.mode"/>
      </xsl:otherwise>
    </xsl:choose>
    <!-- FIXME: i18n -->
    <xsl:if test="@date">
      <xsl:text> (</xsl:text>
      <xsl:value-of select="@date"/>
      <xsl:text>)</xsl:text>
    </xsl:if>
  </div>
</xsl:template>

<!-- = screen = -->
<xsl:template mode="mal2html.block.mode" match="mal:screen">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <xsl:call-template name="mal2html.pre">
    <xsl:with-param name="first_child" select="$first_child"/>
  </xsl:call-template>
</xsl:template>

<!-- = synopsis = -->
<xsl:template mode="mal2html.block.mode" match="mal:synopsis">
  <xsl:param name="first_child" select="not(preceding-sibling::*)"/>
  <div class="synopsis">
    <xsl:attribute name="class">
      <xsl:text>synopsis</xsl:text>
      <xsl:if test="$first_child">
        <xsl:text> first-child</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:apply-templates mode="mal2html.block.mode" select="mal:title"/>
    <xsl:apply-templates mode="mal2html.block.mode" select="mal:desc"/>
    <div class="synopsis-contents">
      <xsl:for-each select="mal:*[not(self::mal:title or self::mal:desc)
                            and ($mal2html.editor_mode or not(self::mal:comment)
                            or processing-instruction('mal2html.show_comment'))]">
        <xsl:apply-templates mode="mal2html.block.mode" select=".">
          <xsl:with-param name="first_child" select="position() = 1"/>
        </xsl:apply-templates>
      </xsl:for-each>
    </div>
  </div>
</xsl:template>

<!-- = title = -->
<xsl:template mode="mal2html.block.mode" match="mal:title">
  <div class="title title-{local-name(..)}">
    <xsl:apply-templates mode="mal2html.inline.mode"/>
  </div>
</xsl:template>

<!-- FIXME -->
<xsl:template mode="mal2html.block.mode" match="*">
  <xsl:message>
    <xsl:text>Unmatched block element: </xsl:text>
    <xsl:value-of select="local-name(.)"/>
  </xsl:message>
  <xsl:apply-templates mode="mal2html.inline.mode"/>
</xsl:template>

</xsl:stylesheet>
