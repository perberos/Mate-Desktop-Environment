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
                xmlns:msg="http://www.gnome.org/~shaunm/mate-doc-utils/l10n"
                version="1.0">

<!--!!==========================================================================
DocBook Titles
:Requires: db-chunk gettext
-->


<!--**==========================================================================
db.title
Processes the title of an element
$node: The element to process the title of

REMARK: Describe this, and how format templates and markup play in
-->
<xsl:template name="db.title">
  <xsl:param name="node" select="."/>
  <xsl:apply-templates mode="db.title.mode" select="$node"/>
</xsl:template>


<!--**==========================================================================
db.titleabbrev
Processes the abbreviated title of an element
$node: The element to process the abbreviated title of

REMARK: Describe this, and how format templates and markup play in,
and fallback to db.title
-->
<xsl:template name="db.titleabbrev">
  <xsl:param name="node" select="."/>
  <xsl:apply-templates mode="db.titleabbrev.mode" select="$node"/>
</xsl:template>


<!--**==========================================================================
db.subtitle
Processes the subtitle of an element
$node: The element to process the subtitle of

REMARK: Describe this, and how format templates and markup play in
-->
<xsl:template name="db.subtitle">
  <xsl:param name="node" select="."/>
  <xsl:apply-templates mode="db.subtitle.mode" select="$node"/>
</xsl:template>


<!--%%==========================================================================
db.title.mode
FIXME

REMARK: Describe this mode
-->
<xsl:template mode="db.title.mode" match="*">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="blockinfo/title">
      <xsl:apply-templates select="blockinfo/title/node()"/>
    </xsl:when>
    <xsl:when test="objectinfo/title">
      <xsl:apply-templates select="objectinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % appendix = -->
<xsl:template mode="db.title.mode" match="anchor">
  <xsl:variable name="target_chunk_id">
    <xsl:call-template name="db.chunk.chunk-id">
      <xsl:with-param name="node" select="."/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="target_chunk" select="key('idkey', $target_chunk_id)"/>
  <xsl:choose>
    <xsl:when test="$target_chunk">
      <xsl:apply-templates mode="db.title.mode" select="$target_chunk"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % appendix = -->
<xsl:template mode="db.title.mode" match="appendix">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="appendixinfo/title">
      <xsl:apply-templates select="appendixinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % article = -->
<xsl:template mode="db.title.mode" match="article">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="articleinfo/title">
      <xsl:apply-templates select="articleinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % bibliography = -->
<xsl:template mode="db.title.mode" match="bibliography">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="bibliographyinfo/title">
      <xsl:apply-templates select="bibliographyinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Bibliography'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % book = -->
<xsl:template mode="db.title.mode" match="book">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="bookinfo/title">
      <xsl:apply-templates select="bookinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % chapter = -->
<xsl:template mode="db.title.mode" match="chapter">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="chapterinfo/title">
      <xsl:apply-templates select="chapterinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % glossary = -->
<xsl:template mode="db.title.mode" match="glossary">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="glossaryinfo/title">
      <xsl:apply-templates select="glossaryinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Glossary'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % index = -->
<xsl:template mode="db.title.mode" match="index">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="indexinfo/title">
      <xsl:apply-templates select="indexinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Index'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % part = -->
<xsl:template mode="db.title.mode" match="part">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="partinfo/title">
      <xsl:apply-templates select="partinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % preface = -->
<xsl:template mode="db.title.mode" match="preface">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="prefaceinfo/title">
      <xsl:apply-templates select="prefaceinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % refentry = -->
<xsl:template mode="db.title.mode" match="refentry">
  <xsl:choose>
    <xsl:when test="refmeta/refentrytitle">
      <xsl:apply-templates select="refmeta/refentrytitle/node()"/>
      <xsl:if test="refmeta/manvolnum">
        <xsl:call-template name="l10n.gettext">
          <xsl:with-param name="msgid" select="'manvolnum.format'"/>
          <xsl:with-param name="node" select="refmeta/manvolnum[1]"/>
          <xsl:with-param name="format" select="true()"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:when>
    <xsl:when test="refentryinfo/title">
      <xsl:apply-templates select="refentryinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="refnamediv/refname[1]/node()"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % reference = -->
<xsl:template mode="db.title.mode" match="reference">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="referenceinfo/title">
      <xsl:apply-templates select="referenceinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % refsect1 = -->
<xsl:template mode="db.title.mode" match="refsect1">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsect1info/title">
      <xsl:apply-templates select="refsect1info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % refsect2 = -->
<xsl:template mode="db.title.mode" match="refsect2">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsect2info/title">
      <xsl:apply-templates select="refsect2info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % refsect3 = -->
<xsl:template mode="db.title.mode" match="refsect3">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsect3info/title">
      <xsl:apply-templates select="refsect3info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % refsection = -->
<xsl:template mode="db.title.mode" match="refsection">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsectioninfo/title">
      <xsl:apply-templates select="refsectioninfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % refsynopsisdiv = -->
<xsl:template mode="db.title.mode" match="refsynopsisdiv">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsynopsisdivinfo/title">
      <xsl:apply-templates select="refsynopsisdivinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Synopsis'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % sect1 = -->
<xsl:template mode="db.title.mode" match="sect1">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect1info/title">
      <xsl:apply-templates select="sect1info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % sect2 = -->
<xsl:template mode="db.title.mode" match="sect2">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect2info/title">
      <xsl:apply-templates select="sect2info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % sect3 = -->
<xsl:template mode="db.title.mode" match="sect3">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect3info/title">
      <xsl:apply-templates select="sect3info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % sect4 = -->
<xsl:template mode="db.title.mode" match="sect4">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect4info/title">
      <xsl:apply-templates select="sect4info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % sect5 = -->
<xsl:template mode="db.title.mode" match="sect5">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect5info/title">
      <xsl:apply-templates select="sect5info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % section = -->
<xsl:template mode="db.title.mode" match="section">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sectioninfo/title">
      <xsl:apply-templates select="sectioninfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % set = -->
<xsl:template mode="db.title.mode" match="set">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="setinfo/title">
      <xsl:apply-templates select="setinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % setindex = -->
<xsl:template mode="db.title.mode" match="setindex">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="setindexinfo/title">
      <xsl:apply-templates select="setindexinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Index'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.title.mode % sidebar = -->
<xsl:template mode="db.title.mode" match="sidebar">
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sidebarinfo/title">
      <xsl:apply-templates select="sidebarinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--%%==========================================================================
db.titleabbrev.mode
FIXME

REMARK: Describe this mode
-->
<xsl:template mode="db.titleabbrev.mode" match="*">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="blockinfo/titleabbrev">
      <xsl:apply-templates select="blockinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="objectinfo/titleabbrev">
      <xsl:apply-templates select="objectinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="blockinfo/title">
      <xsl:apply-templates select="blockinfo/title/node()"/>
    </xsl:when>
    <xsl:when test="objectinfo/title">
      <xsl:apply-templates select="objectinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % appendix = -->
<xsl:template mode="db.titleabbrev.mode" match="appendix">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="appendixinfo/titleabbrev">
      <xsl:apply-templates select="appendixinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="appendixinfo/title">
      <xsl:apply-templates select="appendixinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % article = -->
<xsl:template mode="db.titleabbrev.mode" match="article">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="articleinfo/titleabbrev">
      <xsl:apply-templates select="articleinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="articleinfo/title">
      <xsl:apply-templates select="articleinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % bibliography = -->
<xsl:template mode="db.titleabbrev.mode" match="bibliography">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="bibliographyinfo/titleabbrev">
      <xsl:apply-templates select="bibliographyinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="bibliographyinfo/title">
      <xsl:apply-templates select="bibliographyinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Bibliography'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % book = -->
<xsl:template mode="db.titleabbrev.mode" match="book">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="bookinfo/titleabbrev">
      <xsl:apply-templates select="bookinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="bookinfo/title">
      <xsl:apply-templates select="bookinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % chapter = -->
<xsl:template mode="db.titleabbrev.mode" match="chapter">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="chapterinfo/titleabbrev">
      <xsl:apply-templates select="chapterinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="chapterinfo/title">
      <xsl:apply-templates select="chapterinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % glossary = -->
<xsl:template mode="db.titleabbrev.mode" match="glossary">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="glossaryinfo/titleabbrev">
      <xsl:apply-templates select="glossaryinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="glossaryinfo/title">
      <xsl:apply-templates select="glossaryinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Glossary'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % index = -->
<xsl:template mode="db.titleabbrev.mode" match="index">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="indexinfo/titleabbrev">
      <xsl:apply-templates select="indexinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="indexinfo/title">
      <xsl:apply-templates select="indexinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Index'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % part = -->
<xsl:template mode="db.titleabbrev.mode" match="part">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="partinfo/titleabbrev">
      <xsl:apply-templates select="partinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="partinfo/title">
      <xsl:apply-templates select="partinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % preface = -->
<xsl:template mode="db.titleabbrev.mode" match="preface">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="prefaceinfo/titleabbrev">
      <xsl:apply-templates select="prefaceinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="prefaceinfo/title">
      <xsl:apply-templates select="prefaceinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % refentry = -->
<xsl:template mode="db.titleabbrev.mode" match="refentry">
  <xsl:choose>
    <xsl:when test="refentryinfo/titleabbrev">
      <xsl:apply-templates select="refentryinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="refmeta/refentrytitle">
      <xsl:apply-templates select="refmeta/refentrytitle/node()"/>
      <xsl:if test="refmeta/manvolnum">
        <!-- FIXME: I18N -->
        <xsl:text>(</xsl:text>
        <xsl:apply-templates select="refmeta/manvolnum[1]/node()"/>
        <xsl:text>)</xsl:text>
      </xsl:if>
    </xsl:when>
    <xsl:when test="refentryinfo/title">
      <xsl:apply-templates select="refentryinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="refnamediv/refname[1]/node()"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % reference = -->
<xsl:template mode="db.titleabbrev.mode" match="reference">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="referenceinfo/titleabbrev">
      <xsl:apply-templates select="referenceinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="referenceinfo/title">
      <xsl:apply-templates select="referenceinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % refsect1 = -->
<xsl:template mode="db.titleabbrev.mode" match="refsect1">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="refsect1info/titleabbrev">
      <xsl:apply-templates select="refsect1info/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsect1info/title">
      <xsl:apply-templates select="refsect1info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % refsect2 = -->
<xsl:template mode="db.titleabbrev.mode" match="refsect2">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="refsect2info/titleabbrev">
      <xsl:apply-templates select="refsect2info/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsect2info/title">
      <xsl:apply-templates select="refsect2info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % refsect3 = -->
<xsl:template mode="db.titleabbrev.mode" match="refsect3">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="refsect3info/titleabbrev">
      <xsl:apply-templates select="refsect3info/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsect3info/title">
      <xsl:apply-templates select="refsect3info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % refsection = -->
<xsl:template mode="db.titleabbrev.mode" match="refsection">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="refsectioninfo/titleabbrev">
      <xsl:apply-templates select="refsectioninfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsectioninfo/title">
      <xsl:apply-templates select="refsectioninfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % refsynopsisdiv = -->
<xsl:template mode="db.titleabbrev.mode" match="refsynopsisdiv">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="refsynopsisdivinfo/titleabbrev">
      <xsl:apply-templates select="refsynopsisdivinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="refsynopsisdivinfo/title">
      <xsl:apply-templates select="refsynopsisdivinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % sect1 = -->
<xsl:template mode="db.titleabbrev.mode" match="sect1">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="sect1info/titleabbrev">
      <xsl:apply-templates select="sect1info/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect1info/title">
      <xsl:apply-templates select="sect1info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % sect2 = -->
<xsl:template mode="db.titleabbrev.mode" match="sect2">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="sect2info/titleabbrev">
      <xsl:apply-templates select="sect2info/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect2info/title">
      <xsl:apply-templates select="sect2info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % sect3 = -->
<xsl:template mode="db.titleabbrev.mode" match="sect3">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="sect3info/titleabbrev">
      <xsl:apply-templates select="sect3info/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect3info/title">
      <xsl:apply-templates select="sect3info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % sect4 = -->
<xsl:template mode="db.titleabbrev.mode" match="sect4">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="sect4info/titleabbrev">
      <xsl:apply-templates select="sect4info/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect4info/title">
      <xsl:apply-templates select="sect4info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % sect5 = -->
<xsl:template mode="db.titleabbrev.mode" match="sect5">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="sect5info/titleabbrev">
      <xsl:apply-templates select="sect5info/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sect5info/title">
      <xsl:apply-templates select="sect5info/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % section = -->
<xsl:template mode="db.titleabbrev.mode" match="section">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="sectioninfo/titleabbrev">
      <xsl:apply-templates select="sectioninfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sectioninfo/title">
      <xsl:apply-templates select="sectioninfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % set = -->
<xsl:template mode="db.titleabbrev.mode" match="set">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="setinfo/titleabbrev">
      <xsl:apply-templates select="setinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="setinfo/title">
      <xsl:apply-templates select="setinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % setindex = -->
<xsl:template mode="db.titleabbrev.mode" match="setindex">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="setindexinfo/titleabbrev">
      <xsl:apply-templates select="setindexinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="setindexinfo/title">
      <xsl:apply-templates select="setindexinfo/title/node()"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'Index'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = db.titleabbrev.mode % sidebar = -->
<xsl:template mode="db.titleabbrev.mode" match="sidebar">
  <xsl:choose>
    <xsl:when test="titleabbrev">
      <xsl:apply-templates select="titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="sidebarinfo/titleabbrev">
      <xsl:apply-templates select="sidebarinfo/titleabbrev/node()"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates select="title/node()"/>
    </xsl:when>
    <xsl:when test="sidebarinfo/title">
      <xsl:apply-templates select="sidebarinfo/title/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--%%==========================================================================
db.subtitle.mode
FIXME

REMARK: Describe this mode
-->
<xsl:template mode="db.subtitle.mode" match="*">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="blockinfo/subtitle">
      <xsl:apply-templates select="blockinfo/subtitle/node()"/>
    </xsl:when>
    <xsl:when test="objectinfo/subtitle">
      <xsl:apply-templates select="objectinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % appendix = -->
<xsl:template mode="db.subtitle.mode" match="appendix">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="appendixinfo/subtitle">
      <xsl:apply-templates select="appendixinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % article = -->
<xsl:template mode="db.subtitle.mode" match="article">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="articleinfo/subtitle">
      <xsl:apply-templates select="articleinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % bibliography = -->
<xsl:template mode="db.subtitle.mode" match="bibliography">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="bibliographyinfo/subtitle">
      <xsl:apply-templates select="bibliographyinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % book = -->
<xsl:template mode="db.subtitle.mode" match="book">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="bookinfo/subtitle">
      <xsl:apply-templates select="bookinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % chapter = -->
<xsl:template mode="db.subtitle.mode" match="chapter">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="chapterinfo/subtitle">
      <xsl:apply-templates select="chapterinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % glossary = -->
<xsl:template mode="db.subtitle.mode" match="glossary">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="glossaryinfo/subtitle">
      <xsl:apply-templates select="glossaryinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % index = -->
<xsl:template mode="db.subtitle.mode" match="index">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="indexinfo/subtitle">
      <xsl:apply-templates select="indexinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % part = -->
<xsl:template mode="db.subtitle.mode" match="part">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="partinfo/subtitle">
      <xsl:apply-templates select="partinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % preface = -->
<xsl:template mode="db.subtitle.mode" match="preface">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="prefaceinfo/subtitle">
      <xsl:apply-templates select="prefaceinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % refentry = -->
<xsl:template mode="db.subtitle.mode" match="refentry">
  <xsl:if test="refentryinfo/subtitle">
    <xsl:apply-templates select="refentryinfo/subtitle/node()"/>
  </xsl:if>
</xsl:template>

<!-- = db.subtitle.mode % reference = -->
<xsl:template mode="db.subtitle.mode" match="reference">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="referenceinfo/subtitle">
      <xsl:apply-templates select="referenceinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % refsect1 = -->
<xsl:template mode="db.subtitle.mode" match="refsect1">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="refsect1info/subtitle">
      <xsl:apply-templates select="refsect1info/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % refsect2 = -->
<xsl:template mode="db.subtitle.mode" match="refsect2">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="refsect2info/subtitle">
      <xsl:apply-templates select="refsect2info/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % refsect3 = -->
<xsl:template mode="db.subtitle.mode" match="refsect3">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="refsect3info/subtitle">
      <xsl:apply-templates select="refsect3info/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % refsection = -->
<xsl:template mode="db.subtitle.mode" match="refsection">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="refsectioninfo/subtitle">
      <xsl:apply-templates select="refsectioninfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % refsynopsisdiv = -->
<xsl:template mode="db.subtitle.mode" match="refsynopsisdiv">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="refsynopsisdivinfo/subtitle">
      <xsl:apply-templates select="refsynopsisdivinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % sect1 = -->
<xsl:template mode="db.subtitle.mode" match="sect1">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="sect1info/subtitle">
      <xsl:apply-templates select="sect1info/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % sect2 = -->
<xsl:template mode="db.subtitle.mode" match="sect2">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="sect2info/subtitle">
      <xsl:apply-templates select="sect2info/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % sect3 = -->
<xsl:template mode="db.subtitle.mode" match="sect3">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="sect3info/subtitle">
      <xsl:apply-templates select="sect3info/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % sect4 = -->
<xsl:template mode="db.subtitle.mode" match="sect4">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="sect4info/subtitle">
      <xsl:apply-templates select="sect4info/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % sect5 = -->
<xsl:template mode="db.subtitle.mode" match="sect5">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="sect5info/subtitle">
      <xsl:apply-templates select="sect5info/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % section = -->
<xsl:template mode="db.subtitle.mode" match="section">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="sectioninfo/subtitle">
      <xsl:apply-templates select="sectioninfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % set = -->
<xsl:template mode="db.subtitle.mode" match="set">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="setinfo/subtitle">
      <xsl:apply-templates select="setinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % setindex = -->
<xsl:template mode="db.subtitle.mode" match="setindex">
  <xsl:choose>
    <xsl:when test="subtitle">
      <xsl:apply-templates select="subtitle/node()"/>
    </xsl:when>
    <xsl:when test="setindexinfo/subtitle">
      <xsl:apply-templates select="setindexinfo/subtitle/node()"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = db.subtitle.mode % sidebar = -->
<xsl:template mode="db.subtitle.mode" match="sidebar">
  <xsl:if test="sidebarinfo/subtitle">
    <xsl:apply-templates select="sidebarinfo/subtitle/node()"/>
  </xsl:if>
</xsl:template>


<!-- == msg:* ============================================================== -->
<!--#% l10n.format.mode -->

<xsl:template mode="l10n.format.mode" match="msg:title">
  <xsl:param name="node"/>
  <xsl:call-template name="db.title">
    <xsl:with-param name="node" select="$node"/>
  </xsl:call-template>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:titleabbrev">
  <xsl:param name="node"/>
  <xsl:call-template name="db.titleabbrev">
    <xsl:with-param name="node" select="$node"/>
  </xsl:call-template>
</xsl:template>

<xsl:template mode="l10n.format.mode" match="msg:subtitle">
  <xsl:param name="node"/>
  <xsl:call-template name="db.subtitle">
    <xsl:with-param name="node" select="$node"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
