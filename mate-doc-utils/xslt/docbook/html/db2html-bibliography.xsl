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
                xmlns:set="http://exslt.org/sets"
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="msg set"
                version="1.0">

<!--!!==========================================================================
DocBook to HTML - Bibliographies
:Requires: db-chunk db-common db-label db2html-block db2html-inline db2html-division db2html-xref gettext

This module provides templates to process DocBook bibliograpies.
-->


<!--** =========================================================================
db2html.biblioentry.label
Outputs the label for a bibliography entry
$node: The #{biblioentry} or #{bibliomixed} element to generate a label for

This outputs a label to be placed inline at the beginning of a bibliography
entry.  Labels are created for both #{biblioentry} and #{bibliomixed} elements.
The label is typically an abbreviation of the authors' names an the year of
publication.  In DocBook, it is usually provides with a leading #{abbrev}
element.  Without a leading #{abbrev} element, this template will instead
use the #{xreflabel} or #{id} attribute.
-->
<xsl:template name="db2html.biblioentry.label">
  <xsl:param name="node" select="."/>
  <xsl:if test="$node/*[1]/self::abbrev or $node/@xreflabel or $node/@id">
    <span class="bibliolabel">
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'bibliolabel.format'"/>
        <xsl:with-param name="node" select="."/>
        <xsl:with-param name="format" select="true()"/>
      </xsl:call-template>
    </span>
    <xsl:text> </xsl:text>
  </xsl:if>
</xsl:template>

<!--#% l10n.format.mode -->
<xsl:template mode="l10n.format.mode" match="msg:bibliolabel">
  <xsl:param name="node"/>
  <xsl:choose>
    <xsl:when test="$node/*[1]/self::abbrev">
      <xsl:apply-templates select="$node/abbrev[1]"/>
    </xsl:when>
    <xsl:when test="$node/@xreflabel">
      <xsl:value-of select="$node/@xreflabel"/>
    </xsl:when>
    <xsl:when test="$node/@id">
      <xsl:value-of select="$node/@id"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--%%==========================================================================
db2html.biblioentry.mode
Formats elements inside a #{biblioentry} element

This mode is used when processing the child elements of a #{biblioentry}
element.  Many elements are treated differently when they appear inside
a bibliography entry.
-->
<xsl:template mode="db2html.biblioentry.mode" match="*">
  <xsl:apply-templates select="."/>
</xsl:template>

<!-- = abstract % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="abstract"/>

<!-- = address % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="address">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = affiliation % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="affiliation">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = artheader % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="artheader">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = articleinfo % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="articleinfo">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = author % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="author">
  <xsl:call-template name="db.personname"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = authorblurb % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="authorblurb"/>

<!-- = authorgroup % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="authorgroup">
  <xsl:call-template name="db.personname.list">
    <xsl:with-param name="nodes" select="*"/>
  </xsl:call-template>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = authorinitials % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="authorinitials">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = bibliocoverage % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="bibliocoverage">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = biblioid % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="biblioid">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = bibliomisc % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="bibliomisc">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = bibliorelation % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="bibliorelation">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = biblioset % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="biblioset">
  <xsl:apply-templates mode="db2html.biblioentry.mode"/>
</xsl:template>

<!-- = bibliosource % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="bibliosource">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = citetitle % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="citetitle">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="class" select="'bibliotitle'"/>
  </xsl:call-template>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = collab % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="collab">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = collabname % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="collabname">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = confgroup % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="confgroup">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = confdates % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="confdates">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = conftitle % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="conftitle">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = confnum % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="confnum">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = confsponsor % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="confsponsor">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = contractnum % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="contractnum">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = contractsponsor % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="contractsponsor">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = contrib % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="contrib">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = corpauthor % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="corpauthor">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = corpcredit % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="corpcredit">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = corpname % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="corpname">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = copyright % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="copyright">
  <xsl:call-template name="db.copyright"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = date % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="date">
  <xsl:call-template name="db.copyright"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = edition % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="edition">
  <xsl:call-template name="db.copyright"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = editor % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="editor">
  <xsl:call-template name="db.personname"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = firstname % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="firstname">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = honorific % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="honorific">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = indexterm % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="indexterm">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = invpartnumber % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="invpartnumber">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = isbn % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="isbn">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = issn % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="issn">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = issuenum % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="issuenum">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = jobtitle % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="jobtitle">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = lineage % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="lineage">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = orgname % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="orgname">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = orgdiv % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="orgdiv">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = othercredit % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="othercredit">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = othername % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="othername">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = pagenums % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="pagenums">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = personblurb % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="personblurb"/>

<!-- = printhistory % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="printhistory"/>

<!-- = productname % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="productname">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = productnumber % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="productnumber">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = pubdate % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="pubdate">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = publisher % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="publisher">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = publishername % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="publishername">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = pubsnumber % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="pubsnumber">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = releaseinfo % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="releaseinfo">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = revhistory % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="revhistory"/>

<!-- = seriesvolnums % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="seriesvolnums">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = shortaffil % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="shortaffil">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = subtitle % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="subtitle">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = surname % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="surname">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = title % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="title">
  <span class="bibliotitle">
    <xsl:call-template name="l10n.gettext">
      <xsl:with-param name="msgid" select="'citetitle.format'"/>
      <xsl:with-param name="role" select="../self::biblioset/@relation"/>
      <xsl:with-param name="node" select="."/>
      <xsl:with-param name="format" select="true()"/>
    </xsl:call-template>
  </span>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = titleabbrev % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="titleabbrev">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = volumenum % db2html.biblioentry.mode = -->
<xsl:template mode="db2html.biblioentry.mode" match="volumenum">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>


<!--%%==========================================================================
db2html.bibliomixed.mode
Formats elements inside a #{bibliomixed} element

This mode is used when processing the child elements of a #{bibliomixed}
element.  Many elements are treated differently when they appear inside
a bibliography entry.
-->
<xsl:template mode="db2html.bibliomixed.mode" match="*">
  <xsl:apply-templates select="."/>
</xsl:template>

<!-- = absract % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="abstract">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = address % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="address">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = affiliation % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="affiliation">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = author % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="author">
  <xsl:call-template name="db.personname"/>
</xsl:template>

<!-- = authorblurb % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="authorblurb">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = authorgroup % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="authorgroup">
  <xsl:call-template name="db.personname.list">
    <xsl:with-param name="nodes" select="*"/>
  </xsl:call-template>
</xsl:template>

<!-- = bibliomset % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="bibliomset">
  <xsl:apply-templates mode="db2html.bibliomixed.mode"/>
</xsl:template>

<!-- = bibliomisc % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="bibliomisc">
  <xsl:call-template name="db2html.inline"/>
  <xsl:text>. </xsl:text>
</xsl:template>

<!-- = editor % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="editor">
  <xsl:call-template name="db.personname"/>
</xsl:template>

<!-- = personblurb % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="personblurb">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = shortaffil % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="shortaffil">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = title % db2html.bibliomixed.mode = -->
<xsl:template mode="db2html.bibliomixed.mode" match="title">
  <span class="bibliotitle">
    <xsl:call-template name="l10n.gettext">
      <xsl:with-param name="msgid" select="'citetitle.format'"/>
      <xsl:with-param name="role" select="../self::bibliomset/@relation"/>
      <xsl:with-param name="node" select="."/>
      <xsl:with-param name="format" select="true()"/>
    </xsl:call-template>
  </span>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = bibliography = -->
<xsl:template match="bibliography">
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="not(title) and not(bibliographyinfo/title)">
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="info" select="bibliographyinfo"/>
        <xsl:with-param name="divisions" select="bibliodiv"/>
        <xsl:with-param name="title_content">
          <xsl:call-template name="l10n.gettext">
            <xsl:with-param name="msgid" select="'Bibliography'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="db2html.division.div">
        <xsl:with-param name="info" select="bibliographyinfo"/>
        <xsl:with-param name="divisions" select="bibliodiv"/>
        <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = bibliodiv = -->
<xsl:template match="bibliodiv">
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

<!-- = biblioentry = -->
<xsl:template match="biblioentry">
  <xsl:variable name="node" select="."/>
  <div>
    <xsl:attribute name="class">
      <xsl:text>bibliomixed block</xsl:text>
      <xsl:if test="not(preceding-sibling::biblioentry | preceding-sibling::bibliomixed)">
        <xsl:text> block-first</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:call-template name="db2html.biblioentry.label"/>
    <xsl:apply-templates mode="db2html.biblioentry.mode"
                         select="*[not(set:has-same-node(., $node/*[1]/self::abbrev))]"/>
  </div>
</xsl:template>

<!-- = bibliomixed = -->
<xsl:template match="bibliomixed">
  <xsl:variable name="node" select="."/>
  <div>
    <xsl:attribute name="class">
      <xsl:text>bibliomixed block</xsl:text>
      <xsl:if test="not(preceding-sibling::biblioentry | preceding-sibling::bibliomixed)">
        <xsl:text> block-first</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:call-template name="db2html.biblioentry.label"/>
    <xsl:apply-templates mode="db2html.bibliomixed.mode"
                         select="node()[not(set:has-same-node(., $node/*[1]/self::abbrev))]"/>
  </div>
</xsl:template>

<!-- = bibliolist = -->
<xsl:template match="bibliolist">
  <xsl:call-template name="db2html.block">
    <xsl:with-param name="class" select="'list'"/>
    <xsl:with-param name="indent" select="true()"/>
    <xsl:with-param name="formal" select="true()"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
