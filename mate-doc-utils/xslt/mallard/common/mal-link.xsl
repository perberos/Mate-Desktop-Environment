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
                xmlns:cache="http://projectmallard.org/cache/1.0/"
                xmlns:mal="http://projectmallard.org/1.0/"
                xmlns:exsl="http://exslt.org/common"
                xmlns:str="http://exslt.org/strings"
                version="1.0">

<!--!!==========================================================================
Mallard Links
Common linking utilities for Mallard documents.
:Revision:version="1.0" date="2010-01-02"

This stylesheets contains various utilities for handling links in Mallard
documents.  The templates in this stylesheet make it easier to handle the
different linking mechanisms in Mallard, including the dynamic automatic
linking systems.
-->


<!--@@==========================================================================
mal.cache.file
The location of the cache file.
:Revision:version="1.0" date="2010-01-02"

In order to locate and process links between pages, this stylesheet requires
a Mallard cache file.  Use this parameter to pass the path to a valid cache
file.
-->
<xsl:param name="mal.cache.file"/>


<!--@@==========================================================================
mal.cache
The cache document as a node set.
:Revision:version="1.0" date="2010-01-02"

This parameter points to the root #{cache:cache} element of a Mallard cache
document.  By default, it selects the root element from the file provided in
@{mal.cache.file}.

Some processing tools may create a Mallard cache document without outputting
it to a file.  Those tools can use this parameter directly.
-->
<xsl:param name="mal.cache" select="document($mal.cache.file)/cache:cache"/>


<!--============================================================================
mal.cache.key
-->
<xsl:key name="mal.cache.key" match="mal:page | mal:section" use="@id"/>
<xsl:key name="mal.cache.link.key"
         match="mal:info/mal:link[@type]"
         use="concat(@type, ':', @xref)"/>


<!--@@==========================================================================
mal.link.default_root
The default root ID.

This parameter provides the default ID for the page that is taken to be the
root of the document.  By default, #{'index'} is used.  This should not be
changed for normal Mallard documents.  It may be necessary to change it for
some Mallard extension formats.
-->
<xsl:param name="mal.link.default_root" select="'index'"/>


<!--**==========================================================================
mal.link.linkid
Output the fully qualified link ID for a page or section.
:Revision:version="1.0" date="2010-01-02"
$node: The #{page} or #{section} element to generate a link ID for.

This template outputs the fully qualified link ID for a page or section.  For
#{page} elements, the link ID is identical to the ID.  For #{section} elements,
however, the link ID is the containing page ID and the section ID, joined with
the #{#} character.

The link ID is used in Mallard cache files to ensure all #{id} attributes are
unique.  All of the templates in this stylesheet that use a link ID use this
template or *{mal.link.xref.linkid}.
-->
<xsl:template name="mal.link.linkid">
  <xsl:param name="node" select="."/>
  <xsl:choose>
    <xsl:when test="contains($node/@id, '#')">
      <xsl:value-of select="$node/@id"/>
    </xsl:when>
    <xsl:when test="$node/self::mal:section">
      <xsl:value-of select="concat($node/ancestor::mal:page[1]/@id, '#', $node/@id)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$node/@id"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
mal.link.xref.linkid
Output the fully qualified link ID for an #{xref} attribute.
:Revision:version="1.0" date="2010-01-02"
$node: The element containing an #{xref} attribute.
$xref: The #{xref} value to generate a link ID from.

This template outputs the fully qualified link ID for an #{xref} attribute.
This may simply be ${xref}, but if ${xref} starts with the #{#} character, it
is prefixed with the ID of the page that contains ${node}.

See *{mal.link.linkid} for more on link IDs.
-->
<xsl:template name="mal.link.xref.linkid">
  <xsl:param name="node" select="."/>
  <xsl:param name="xref" select="$node/@xref"/>
  <xsl:if test="starts-with($xref, '#')">
    <xsl:value-of select="$node/ancestor-or-self::mal:page/@id"/>
  </xsl:if>
  <xsl:value-of select="$xref"/>
</xsl:template>


<!--**==========================================================================
mal.link.content
Output the content for a #{link} element.
:Revision:version="1.0" date="2010-01-02"
$node: The #{link} or other element creating the link.
$xref: The #{xref} attribute of ${node}.
$href: The #{href} attribute of ${node}.
$role: A link role, used to select the appropriate title.

This template outputs the automatic text content for a link.  It should only
be used for links that do not have specified content.  If ${xref} points to a
valid page or section, the appropriate link title from that page or section
will be selected, based on ${role}.  The %{mal.link.content.mode} mode will
be applied to the contents of that title.  Stylesheets using this template
should map that mode to inline processing.

If only ${href} is provided, that URL is used as the text content.  If a target
page or section cannot be found, ${xref} is used as the text content.
-->
<xsl:template name="mal.link.content">
  <xsl:param name="node" select="."/>
  <xsl:param name="xref" select="$node/@xref"/>
  <xsl:param name="href" select="$node/@href"/>
  <xsl:param name="role" select="''"/>
  <xsl:variable name="linkid">
    <xsl:call-template name="mal.link.xref.linkid">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="xref" select="$xref"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:for-each select="$mal.cache">
    <xsl:variable name="target" select="key('mal.cache.key', $linkid)"/>
    <xsl:choose>
      <xsl:when test="$target">
        <xsl:variable name="titles" select="$target/mal:info/mal:title[@type = 'link']"/>
        <xsl:choose>
          <xsl:when test="$role != '' and $titles[@role = $role]">
            <xsl:apply-templates mode="mal.link.content.mode"
                                 select="$titles[@role = $role][1]/node()"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates mode="mal.link.content.mode"
                                 select="$titles[not(@role)][1]/node()"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="$href">
            <xsl:value-of select="$href"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$xref"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>


<!--%%==========================================================================
mal.link.content.mode
Output the content for a link from the contents of a #{title} element.
:Revision:version="1.0" date="2010-01-02"

This mode is applied to the contents of a #{title} element by *{mal.link.content}.
By default, it returns the string value of its input.  Stylesheets that use
*{mal.link.content} should map this mode to inline processing.
-->
<xsl:template mode="mal.link.content.mode" match="* | text()">
  <xsl:value-of select="."/>
</xsl:template>


<!--**==========================================================================
mal.link.target
Output the target URL for a #{link} or other linking element.
:Revision:version="1.0" date="2010-01-02"
$node: The #{link} or other element creating the link.
$xref: The #{xref} attribute of ${node}.
$href: The #{href} attribute of ${node}.

This template outputs a URL for a #{link} element or another element using
linking attributes.  If ${xref} points to a valid page or section, it uses
a file name based on the ID of the target page plus @{mal.chunk.extension}.
Otherwise, the link will point to ${href}.
-->
<xsl:template name="mal.link.target">
  <xsl:param name="node" select="."/>
  <xsl:param name="xref" select="$node/@xref"/>
  <xsl:param name="href" select="$node/@href"/>
  <xsl:choose>
    <xsl:when test="string($xref) = ''">
      <xsl:value-of select="$href"/>
    </xsl:when>
    <xsl:when test="contains($xref, '/') or contains($xref, ':')">
      <!--
      This is a link to another document, which we don't handle in these
      stylesheets.  Extensions such as library or yelp should override
      this template to provide this functionality.
      -->
      <xsl:value-of select="$href"/>
    </xsl:when>
    <xsl:when test="contains($xref, '#')">
      <xsl:variable name="pageid" select="substring-before($xref, '#')"/>
      <xsl:variable name="sectionid" select="substring-after($xref, '#')"/>
      <xsl:if test="$pageid != ''">
        <xsl:value-of select="concat($pageid, $mal.chunk.extension)"/>
      </xsl:if>
      <xsl:value-of select="concat('#', $sectionid)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat($xref, $mal.chunk.extension)"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
mal.link.tooltip
Output the tooltip for a #{link} or other linking element.
:Revision:version="1.0" date="2010-01-02"
$node: The #{link} or other element creating the link.
$xref: The #{xref} attribute of ${node}.
$href: The #{href} attribute of ${node}.

This template outputs a tooltip for a link.  Currently, it only outputs a
tooltip for #{mailto:} URLs in ${href}.
-->
<xsl:template name="mal.link.tooltip">
  <xsl:param name="node" select="."/>
  <xsl:param name="xref" select="$node/@xref"/>
  <xsl:param name="href" select="$node/@href"/>
  <xsl:choose>
    <xsl:when test="string($xref) != ''">
      <!-- FIXME and make sure to update the docs -->
    </xsl:when>
    <xsl:when test="starts-with($href, 'mailto:')">
      <xsl:variable name="address" select="substring-after($href, 'mailto:')"/>
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'email.tooltip'"/>
        <xsl:with-param name="string" select="$address"/>
        <xsl:with-param name="format" select="true()"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="normalize-space($href)"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
mal.link.guidelinks
Output the guide links for a page or section.
:Revision:version="1.0" date="2010-01-02"
$node: The #{page} or #{section} element to generate links for.

This template outputs all the guide links for a page or section, whether
declared as guide links in the page or section or as topic links from another
guide page.  It outputs each of the links as a #{link} element within the
Mallard namespace.  Each #{link} element has an #{xref} attribute pointing
to the target page or section.

Each #{link} element contains a #{title} with #{type="sort"} providing the
sort title of the target page or section.  The results are not sorted when
returned from this template.  Use #{xsl:sort} on the sort titles to sort
the results.

The output is a result tree fragment.  To use these results, call
#{exsl:node-set} on them.
-->
<xsl:template name="mal.link.guidelinks">
  <xsl:param name="node" select="."/>
  <xsl:variable name="linkid">
    <xsl:call-template name="mal.link.linkid">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:for-each select="$node/mal:info/mal:link[@type = 'guide']">
    <xsl:variable name="linklinkid">
      <xsl:call-template name="mal.link.xref.linkid"/>
    </xsl:variable>
    <mal:link xref="{$linklinkid}">
      <mal:title type="sort">
        <xsl:for-each select="$mal.cache">
          <xsl:value-of select="key('mal.cache.key', $linklinkid)/mal:info/mal:title[@type = 'sort'][1]"/>
        </xsl:for-each>
      </mal:title>
    </mal:link>
  </xsl:for-each>
  <xsl:for-each select="$mal.cache//*[mal:info/mal:link[@type = 'topic'][@xref = $linkid]]">
    <xsl:variable name="linklinkid">
      <xsl:call-template name="mal.link.linkid"/>
    </xsl:variable>
    <xsl:if test="not($node/mal:info/mal:link[@type = 'guide'][@xref = $linklinkid])">
      <mal:link xref="{$linklinkid}">
        <mal:title type="sort">
          <xsl:value-of select="mal:info/mal:title[@type = 'sort'][1]"/>
        </mal:title>
      </mal:link>
    </xsl:if>
  </xsl:for-each>
</xsl:template>


<!--**==========================================================================
mal.link.topiclinks
Output the topic links for a page or section.
:Revision:version="1.0" date="2010-01-02"
$node: The #{page} or #{section} element to generate links for.

This template outputs all the topic links for a guide page or section, whether
declared as topic links in the page or section or as guide links from another
page or section.  It outputs each of the links as a #{link} element within the
Mallard namespace.  Each #{link} element has an #{xref} attribute pointing
to the target page or section.

Each #{link} element contains a #{title} with #{type="sort"} providing the
sort title of the target page or section.

Each #{link} element also contains a #{group} attribute.  The #{group}
attribute is normalized.  It will either point to a link group declared
in ${node}, or it will be set to #{#default}.  Each #{link} element also
contains a #{groupsort} attribute giving the numerical position of the
#{group} attribute in the normalized group list for ${node}.

The results are not sorted when returned from this template.  Use #{xsl:sort}
on the #{groupsort} attribute and the sort titles to sort the results.

The output is a result tree fragment.  To use these results, call
#{exsl:node-set} on them.
-->
<xsl:template name="mal.link.topiclinks">
  <xsl:param name="node" select="."/>
  <xsl:variable name="linkid">
    <xsl:call-template name="mal.link.linkid">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="groups">
    <xsl:value-of select="$node/@groups"/>
    <xsl:if test="not(contains(concat(' ', $node/@groups, ' '), ' #default '))">
      <xsl:text> #default</xsl:text>
    </xsl:if>
  </xsl:variable>
  <xsl:variable name="groupslist" select="str:split($groups)"/>
  <xsl:variable name="defaultpos">
    <xsl:for-each select="$groupslist">
      <xsl:if test="string(.) = '#default'">
        <xsl:value-of select="position()"/>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>
  <xsl:for-each select="$node/mal:info/mal:link[@type = 'topic']">
    <xsl:variable name="xref">
      <xsl:call-template name="mal.link.xref.linkid"/>
    </xsl:variable>
    <xsl:variable name="link" select="."/>
    <xsl:variable name="grouppos">
      <xsl:if test="$link/@group">
        <xsl:for-each select="$groupslist">
          <xsl:if test="string(.) = $link/@group">
            <xsl:value-of select="position()"/>
          </xsl:if>
        </xsl:for-each>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="groupsort">
      <xsl:value-of select="$grouppos"/>
      <xsl:if test="string($grouppos) = ''">
        <xsl:value-of select="$defaultpos"/>
      </xsl:if>
    </xsl:variable>
    <mal:link xref="{$xref}">
      <xsl:attribute name="group">
        <xsl:value-of select="$groupslist[number($groupsort)]"/>
      </xsl:attribute>
      <xsl:attribute name="groupsort">
        <xsl:value-of select="$groupsort"/>
      </xsl:attribute>
      <mal:title type="sort">
        <xsl:for-each select="$mal.cache">
          <xsl:value-of select="key('mal.cache.key', $xref)/mal:info/mal:title[@type = 'sort'][1]"/>
        </xsl:for-each>
      </mal:title>
    </mal:link>
  </xsl:for-each>
  <xsl:for-each select="$mal.cache//mal:info/mal:link[@type = 'guide'][@xref = $linkid]">
    <xsl:variable name="source" select="../.."/>
    <xsl:variable name="xref">
      <xsl:call-template name="mal.link.xref.linkid">
        <xsl:with-param name="node" select="$source"/>
        <xsl:with-param name="xref" select="$source/@id"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="not($node/mal:info/mal:link[@type = 'topic'][@xref = $xref])">
      <xsl:variable name="link" select="."/>
      <xsl:variable name="grouppos">
        <xsl:if test="$link/@group">
          <xsl:for-each select="$groupslist">
            <xsl:if test="string(.) = $link/@group">
              <xsl:value-of select="position()"/>
            </xsl:if>
          </xsl:for-each>
        </xsl:if>
      </xsl:variable>
      <xsl:variable name="groupsort">
        <xsl:value-of select="$grouppos"/>
        <xsl:if test="string($grouppos) = ''">
          <xsl:value-of select="$defaultpos"/>
        </xsl:if>
      </xsl:variable>
      <mal:link xref="{$xref}">
        <xsl:attribute name="group">
          <xsl:value-of select="$groupslist[number($groupsort)]"/>
        </xsl:attribute>
        <xsl:attribute name="groupsort">
          <xsl:value-of select="$groupsort"/>
        </xsl:attribute>
        <mal:title type="sort">
          <xsl:value-of select="$source/mal:info/mal:title[@type = 'sort'][1]"/>
        </mal:title>
      </mal:link>
    </xsl:if>
  </xsl:for-each>
</xsl:template>


<!--**==========================================================================
mal.link.linktrails
Output link trails for a page or section.
$node: The #{page} or #{section} element to generate links for.
$trail: The link trail leading to ${node}.
$root: The ID of the root page.

This template outputs lists of links, where each list is a path of topic links
that leads to ${node}.  Each link list is output as a #{link} element in the
Mallard namespace with an #{xref} attribute pointing to the target page or
section.  Each #{link} has a #{title} element with #{type="sort"} providing
the sort title of the target page or section.

Each #{link} element may also contain another #{link} element providing the
next link in the trail.  Each of these links also contains a sort titles and
may also contain another link.

The results are not sorted when returned from this template.  Use #{xsl:sort}
on the nested sort titles to sort the results.  The output is a result tree
fragment.  To use these results, call #{exsl:node-set} on them.

This template calls itself recursively.  It finds the guide links for ${node}
using *{mal.link.guidelinks}.  It then calls *{mal.link.linktrails} on each
guide, wrapping ${trail} with a link to the guide as the new ${trail} parameter.

If there are no guide links for ${node} and ${node} is a #{section} element,
this template calls itself on the containing page, wrapping ${trails} with a
link to that page.  This #{link} element has the attribute #{child="section"}
to indicate the link from it to its child is not a topic link.

Recursion stops when the ID of ${node} is ${root}.  Link trails are only
output if they reach ${root}.
-->
<!--
FIXME:
* Terminate at $root
* Only output if we reach $root
* Cycle detection
-->
<xsl:template name="mal.link.linktrails">
  <xsl:param name="node" select="."/>
  <xsl:param name="trail" select="/false"/>
  <xsl:param name="root" select="$mal.link.default_root"/>
  <xsl:variable name="guidelinks">
    <xsl:call-template name="mal.link.guidelinks">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="guidenodes" select="exsl:node-set($guidelinks)/*"/>
  <xsl:choose>
    <xsl:when test="count($guidenodes) = 0">
      <xsl:choose>
        <xsl:when test="$node/self::mal:section">
          <xsl:variable name="page" select="$node/ancestor::mal:page"/>
          <xsl:call-template name="mal.link.linktrails">
            <xsl:with-param name="node" select="$page"/>
            <xsl:with-param name="trail">
              <mal:link xref="{$page/@id}" child="section">
                <xsl:copy-of select="$page/mal:info/mal:title[@type='sort']"/>
                <xsl:copy-of select="$trail"/>
              </mal:link>
            </xsl:with-param>
            <xsl:with-param name="root" select="$root"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:copy-of select="$trail"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:for-each select="$guidenodes">
        <xsl:variable name="self" select="."/>
        <xsl:choose>
          <xsl:when test="$self/@xref = $root">
            <mal:link xref="{$self/@xref}">
              <xsl:copy-of select="$self/mal:title"/>
              <xsl:copy-of select="$trail"/>
            </mal:link>
          </xsl:when>
          <xsl:otherwise>
            <xsl:for-each select="$mal.cache">
              <xsl:call-template name="mal.link.linktrails">
                <xsl:with-param name="node" select="key('mal.cache.key', $self/@xref)"/>
                <xsl:with-param name="trail">
                  <mal:link xref="{$self/@xref}">
                    <xsl:copy-of select="$self/mal:title"/>
                    <xsl:copy-of select="$trail"/>
                  </mal:link>
                </xsl:with-param>
                <xsl:with-param name="root" select="$root"/>
              </xsl:call-template>
            </xsl:for-each>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
