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
                xmlns:str="http://exslt.org/strings"
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="set str"
                version="1.0">

<!--!!==========================================================================
DocBook to HTML - Command Synopses
:Requires: db-label db2html-xref gettext

This module contains templates to process DocBook command synopsis elements.
-->


<!--@@==========================================================================
db2html.arg.choice
The default value of the #{choice} parameter for #{arg} elements

REMARK: Describe this param
-->
<xsl:param name="db2html.arg.choice" select="'opt'"/>


<!--@@==========================================================================
db2html.arg.rep
The default value of the #{rep} parameter for #{arg} elements

REMARK: Describe this param
-->
<xsl:param name="db2html.arg.rep" select="'norepeat'"/>


<!--@@==========================================================================
db2html.group.choice
The default value of the #{choice} parameter for #{group} elements

REMARK: Describe this param
-->
<xsl:param name="db2html.group.choice" select="'opt'"/>


<!--@@==========================================================================
db2html.group.rep
The default value of the #{rep} parameter for #{group} elements

REMARK: Describe this param
-->
<xsl:param name="db2html.group.rep" select="'norepeat'"/>


<!-- == Matched Templates == -->

<!-- = arg = -->
<xsl:template match="arg">
  <xsl:param name="sepchar">
    <xsl:choose>
      <xsl:when test="ancestor::cmdsynopsis[1][@sepchar]">
        <xsl:value-of select="ancestor::cmdsynopsis[1]/@sepchar"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text> </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="choice">
    <xsl:choose>
      <xsl:when test="@choice">
        <xsl:value-of select="@choice"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$db2html.arg.choice"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="rep">
    <xsl:choose>
      <xsl:when test="@rep">
        <xsl:value-of select="@rep"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$db2html.arg.rep"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>

  <span class="arg-punc">
    <xsl:choose>
      <xsl:when test="$choice = 'plain'"/>
      <xsl:when test="$choice = 'req'">
        <xsl:text>{</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>[</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <span class="arg">
      <xsl:for-each select="node()">
        <xsl:choose>
          <xsl:when test="self::sbr">
            <xsl:text>&#x000A;</xsl:text>
            <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode"
                                 select="ancestor::cmdsynopsis[1]">
              <xsl:with-param name="sbr" select="."/>
              <xsl:with-param name="sepchar" select="$sepchar"/>
            </xsl:apply-templates>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select=".">
              <xsl:with-param name="sepchar" select="$sepchar"/>
            </xsl:apply-templates>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </span>
    <xsl:if test="$rep = 'repeat'">
      <xsl:text>...</xsl:text>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="$choice = 'plain'"/>
      <xsl:when test="$choice = 'req'">
        <xsl:text>}</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>]</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </span>
</xsl:template>

<!-- = cmdsynopsis = -->
<xsl:template match="cmdsynopsis">
  <xsl:param name="sepchar">
    <xsl:choose>
      <xsl:when test="@sepchar">
        <xsl:value-of select="@sepchar"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text> </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <div>
    <xsl:choose>
      <xsl:when test="@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
        <xsl:attribute name="dir">
          <xsl:text>ltr</xsl:text>
        </xsl:attribute>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:attribute name="class">
      <xsl:text>synopsis cmdsynopsis block</xsl:text>
      <xsl:if test="not(preceding-sibling::*
                    [not(self::blockinfo) and not(self::title) and
                     not(self::titleabbrev) and not(self::attribution) ])">
        <xsl:text> block-first</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:call-template name="db2html.anchor"/>
    <pre class="cmdsynopsis">
      <xsl:for-each select="command | arg | group | sbr">
        <xsl:choose>
          <xsl:when test="position() = 1"/>
          <xsl:when test="self::sbr">
            <xsl:text>&#x000A;</xsl:text>
            <xsl:value-of select="str:padding(string-length(preceding-sibling::command[1]), ' ')"/>
          </xsl:when>
          <xsl:when test="self::command">
            <xsl:text>&#x000A;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$sepchar"/>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:apply-templates select=".">
          <xsl:with-param name="sepchar" select="$sepchar"/>
        </xsl:apply-templates>
      </xsl:for-each>
      <xsl:apply-templates select="synopfragment">
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
    </pre>
  </div>
</xsl:template>

<!-- = group = -->
<xsl:template match="group">
  <xsl:param name="sepchar">
    <xsl:choose>
      <xsl:when test="ancestor::cmdsynopsis[1][@sepchar]">
        <xsl:value-of select="ancestor::cmdsynopsis[1]/@sepchar"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text> </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="choice">
    <xsl:choose>
      <xsl:when test="@choice">
        <xsl:value-of select="@choice"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$db2html.group.choice"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:param name="rep">
    <xsl:choose>
      <xsl:when test="@rep">
        <xsl:value-of select="@rep"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$db2html.group.rep"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:variable name="padding">
    <xsl:if test="sbr">
      <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode"
                           select="ancestor::cmdsynopsis[1]">
        <xsl:with-param name="sbr" select="sbr[1]"/>
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
    </xsl:if>
  </xsl:variable>

  <span class="group-punc">
    <xsl:choose>
      <xsl:when test="$choice = 'plain'">
        <xsl:text>(</xsl:text>
      </xsl:when>
      <xsl:when test="$choice = 'req'">
        <xsl:text>{</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>[</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <span class="group">
      <xsl:for-each select="*">
        <xsl:choose>
          <xsl:when test="self::sbr">
            <xsl:text>&#x000A;</xsl:text>
            <xsl:value-of select="$padding"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select=".">
              <xsl:with-param name="sepchar" select="$sepchar"/>
            </xsl:apply-templates>
            <xsl:if test="position() != last()">
              <xsl:value-of select="concat($sepchar, '|', $sepchar)"/>
            </xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </span>
    <xsl:choose>
      <xsl:when test="$choice = 'plain'">
        <xsl:text>)</xsl:text>
      </xsl:when>
      <xsl:when test="$choice = 'req'">
        <xsl:text>}</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>]</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$rep = 'repeat'">
      <xsl:text>...</xsl:text>
    </xsl:if>
  </span>
</xsl:template>

<!-- = synopfragment = -->
<xsl:template match="synopfragment">
  <xsl:param name="sepchar">
    <xsl:choose>
      <xsl:when test="ancestor::cmdsynopsis[1][@sepchar]">
        <xsl:value-of select="ancestor::cmdsynopsis[1]/@sepchar"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text> </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <div class="synopfragment">
    <xsl:call-template name="db2html.anchor"/>
    <i><xsl:call-template name="db.label"/></i>
    <xsl:for-each select="*">
      <xsl:value-of select="$sepchar"/>
      <xsl:apply-templates select=".">
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
    </xsl:for-each>
  </div>
</xsl:template>

<!-- = synopfragmentref = -->
<xsl:template match="synopfragmentref">
  <xsl:call-template name="db2html.xref"/>
</xsl:template>


<!--%%==========================================================================
db2html.cmdsynopsis.sbr.padding.mode
Outputs padding for elements leading up to an #{sbr} element
$sbr: The #{sbr} element to pad up to
$sepchar: The value of the #{sepchar} attribute on the enclosing #{cmdsynopsis}

When processed in this mode, elements output whitespace to the length of the
textual output they would normally produce.  This allows options to be aligned
when explicit line breaks are inserted with #{sbr} elements.

To create the padding for a given #{sbr} element, this mode is called on the
enclosing #{cmdsynopsis} element, passing the #{sbr} element.  When processed
in this mode, elements should only output padding for content the leads up to
the #{sbr} element passed in the ${sbr} parameter.  When processing children
that don't contain the given #{sbr} element, the ${sbr} parameter should be
set to #{false()} for those children.  This avoids additional ancestor
selectors, which are generally expensive to perform.
-->
<xsl:template mode="db2html.cmdsynopsis.sbr.padding.mode" match="node()">
  <xsl:value-of select="str:padding(string-length(.), ' ')"/>
</xsl:template>

<!-- = cmdsynopsis % db2html.cmdsynopsis.sbr.padding.mode = -->
<xsl:template mode="db2html.cmdsynopsis.sbr.padding.mode" match="cmdsynopsis">
  <xsl:param name="sbr"/>
  <xsl:param name="sepchar"/>
  <xsl:variable name="child" select="*[set:has-same-node(.|.//sbr, $sbr)][1]"/>
  <xsl:choose>
    <xsl:when test="$child/self::synopfragment">
      <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select="$child">
        <xsl:with-param name="sbr" select="$sbr"/>
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <!-- Output padding for the preceding command -->
      <xsl:variable name="cmd" select="$child/preceding-sibling::command[1]"/>
      <xsl:value-of select="str:padding(string-length($cmd), ' ')"/>
      <xsl:value-of select="str:padding(string-length($sepchar), ' ')"/>
      <!-- Process all children that are between $cmd and $child, but 
           after any sbr elements between $cmd and $child -->
      <xsl:for-each select="$cmd/following-sibling::*
                              [set:has-same-node(following-sibling::*, $child)]
                              [not(set:has-same-node(. | following-sibling::sbr,
                                                     $child/preceding-sibling::sbr))]">
        <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select=".">
          <xsl:with-param name="sbr" select="false()"/>
          <xsl:with-param name="sepchar" select="$sepchar"/>
        </xsl:apply-templates>
        <xsl:value-of select="str:padding(string-length($sepchar), ' ')"/>
      </xsl:for-each>
      <!-- And process $child itself -->
      <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select="$child">
        <xsl:with-param name="sbr" select="$sbr"/>
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = arg % db2html.cmdsynopsis.sbr.padding.mode = -->
<xsl:template mode="db2html.cmdsynopsis.sbr.padding.mode" match="arg">
  <xsl:param name="sbr"/>
  <xsl:param name="sepchar"/>
  <xsl:if test="@choice != 'plain'">
    <xsl:text> </xsl:text>
  </xsl:if>
  <xsl:choose>
    <xsl:when test="not($sbr)">
      <!-- The sbr is outside this element.  The total width an arg is whatever
           comes before an sbr plus whatever comes after an sbr plus possible
           punctuation spacing. -->
      <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode"
                           select="node()[not(preceding-sibling::sbr)]">
        <xsl:with-param name="sbr" select="$sbr"/>
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
      <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode"
                           select="sbr[last()]/following-sibling::node()">
        <xsl:with-param name="sbr" select="$sbr"/>
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
      <xsl:if test="@choice != 'plain'">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="child" select="*[set:has-same-node(.|.//sbr, $sbr)][1]"/>
      <!-- Process all children that are before $child, but after
           any sbr elements before $child.  Process any children
           before the initial sbr before $child, if it exists. -->
      <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode"
                           select="$child/preceding-sibling::sbr[last()]/preceding-sibling::node()
                                   | ($child/preceding-sibling::node())
                                       [not(set:has-same-node(. | following-sibling::sbr,
                                                              $child/preceding-sibling::sbr))]">
        <xsl:with-param name="sbr" select="false()"/>
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
      <!-- And process $child itself -->
      <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select="$child">
        <xsl:with-param name="sbr" select="$sbr"/>
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = group % db2html.cmdsynopsis.sbr.padding.mode = -->
<xsl:template mode="db2html.cmdsynopsis.sbr.padding.mode" match="group">
  <xsl:param name="sbr"/>
  <xsl:param name="sepchar"/>
  <xsl:text> </xsl:text>
  <xsl:choose>
    <xsl:when test="not($sbr)">
      <!-- The sbr is outside this element.  The total width a group is
           calculated by taking all children after the last sbr (or all
           children if there is no sbr), adding their widths, and adding
           width for joining punctuation for all but one of them.  Add
           to this punctuation spacing for the group as a whole. -->
      <xsl:for-each select="*[not(following-sibling::sbr) and not(self::sbr)]">
        <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select=".">
          <xsl:with-param name="sbr" select="$sbr"/>
          <xsl:with-param name="sepchar" select="$sepchar"/>
        </xsl:apply-templates>
        <xsl:if test="position() != 1">
          <xsl:value-of select="str:padding(2 * string-length($sepchar) + 1, ' ')"/>
        </xsl:if>
      </xsl:for-each>
      <xsl:text> </xsl:text>
    </xsl:when>
    <xsl:when test="set:has-same-node(., $sbr/..)"/>
    <xsl:otherwise>
      <xsl:variable name="child" select="*[set:has-same-node(.|.//sbr, $sbr)][1]"/>
      <!-- Process all children that are before $child, but after
           any sbr elements before $child. Add joining punctuation
           padding for all but one of them. -->
      <xsl:for-each select="($child/preceding-sibling::*)
                              [not(set:has-same-node(. | following-sibling::sbr,
                                                     $child/preceding-sibling::sbr))]">
        <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select=".">
          <xsl:with-param name="sbr" select="false()"/>
          <xsl:with-param name="sepchar" select="$sepchar"/>
        </xsl:apply-templates>
        <xsl:if test="position() != 1">
          <xsl:value-of select="str:padding(2 * string-length($sepchar) + 1, ' ')"/>
        </xsl:if>
      </xsl:for-each>
      <!-- And process $child itself -->
      <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select="$child">
        <xsl:with-param name="sbr" select="$sbr"/>
        <xsl:with-param name="sepchar" select="$sepchar"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = synopfragment % db2html.cmdsynopsis.sbr.padding.mode = -->
<xsl:template mode="db2html.cmdsynopsis.sbr.padding.mode" match="synopfragment">
  <xsl:param name="sbr"/>
  <xsl:param name="sepchar"/>
  <xsl:variable name="label">
    <xsl:call-template name="db.label"/>
  </xsl:variable>
  <xsl:value-of select="str:padding(string-length($label), ' ')"/>
  <xsl:value-of select="str:padding(string-length($sepchar), ' ')"/>
  <xsl:variable name="child" select="*[set:has-same-node(.|.//sbr, $sbr)][1]"/>
  <!-- Process all children that are before $child, but 
       after any sbr elements before $child -->
  <xsl:for-each select="$child/preceding-sibling::*
                          [not(set:has-same-node(. | following-sibling::sbr,
                                                 $child/preceding-sibling::sbr))]">
    <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select=".">
      <xsl:with-param name="sbr" select="false()"/>
      <xsl:with-param name="sepchar" select="$sepchar"/>
    </xsl:apply-templates>
    <xsl:value-of select="str:padding(string-length($sepchar), ' ')"/>
  </xsl:for-each>
  <!-- And process $child itself -->
  <xsl:apply-templates mode="db2html.cmdsynopsis.sbr.padding.mode" select="$child">
    <xsl:with-param name="sbr" select="$sbr"/>
    <xsl:with-param name="sepchar" select="$sepchar"/>
  </xsl:apply-templates>
</xsl:template>

<!-- = synopfragmentref % db2html.cmdsynopsis.sbr.padding.mode = -->
<xsl:template mode="db2html.cmdsynopsis.sbr.padding.mode" match="synopfragmentref">
  <xsl:variable name="label">
    <xsl:call-template name="db2html.xref"/>
  </xsl:variable>
  <xsl:value-of select="str:padding(string-length($label), ' ')"/>
</xsl:template>

</xsl:stylesheet>
