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
DocBook to HTML - Lists
:Requires: db-common db2html-inline db2html-xref gettext

REMARK: Describe this module
-->


<!-- == Matched Templates == -->

<!-- = variablelist = -->
<xsl:template match="glosslist">
  <div class="block list glosslist">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates select="title"/>
    <dl class="glosslist">
      <xsl:apply-templates select="glossentry"/>
    </dl>
  </div>
</xsl:template>

<!-- = itemizedlist = -->
<xsl:template match="itemizedlist">
  <div class="block list itemizedlist">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates select="*[not(self::listitem)]"/>
    <ul class="itemizedlist">
      <xsl:if test="@mark">
        <xsl:attribute name="style">
          <xsl:text>list-style-type: </xsl:text>
          <xsl:choose>
            <xsl:when test="@mark = 'bullet'">disc</xsl:when>
            <xsl:when test="@mark = 'box'">square</xsl:when>
            <xsl:otherwise><xsl:value-of select="@mark"/></xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </xsl:if>
      <xsl:if test="@spacing = 'compact'">
        <xsl:attribute name="compact">
          <xsl:value-of select="@spacing"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:apply-templates select="listitem"/>
    </ul>
  </div>
</xsl:template>

<!-- = itemizedlist/listitem = -->
<xsl:template match="itemizedlist/listitem">
  <xsl:variable name="first"
             select="not(preceding-sibling::*
                     [not(self::blockinfo) and not(self::title) and not(self::titleabbrev)])"/>
  <li>
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$first">
      <xsl:attribute name="class">
        <xsl:text>li-first</xsl:text>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@override">
      <xsl:attribute name="style">
        <xsl:text>list-style-type: </xsl:text>
        <xsl:choose>
          <xsl:when test="@override = 'bullet'">disc</xsl:when>
          <xsl:when test="@override = 'box'">square</xsl:when>
          <xsl:otherwise><xsl:value-of select="@override"/></xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates/>
  </li>
</xsl:template>

<!-- = member = -->
<xsl:template match="member">
  <!-- Do something trivial, and rely on simplelist to do the rest -->
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = orderedlist = -->
<xsl:template match="orderedlist">
  <xsl:variable name="start">
    <xsl:choose>
      <xsl:when test="@continuation = 'continues'">
        <xsl:call-template name="db.orderedlist.start"/>
      </xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <!-- FIXME: auto-numeration for nested lists -->
  <div class="block list orderedlist">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates select="*[not(self::listitem)]"/>
    <ol class="orderedlist">
      <xsl:if test="@numeration">
        <xsl:attribute name="type">
          <xsl:choose>
            <xsl:when test="@numeration = 'arabic'">1</xsl:when>
            <xsl:when test="@numeration = 'loweralpha'">a</xsl:when>
            <xsl:when test="@numeration = 'lowerroman'">i</xsl:when>
            <xsl:when test="@numeration = 'upperalpha'">A</xsl:when>
            <xsl:when test="@numeration = 'upperroman'">I</xsl:when>
            <xsl:otherwise>1</xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </xsl:if>
      <xsl:if test="$start != '1'">
        <xsl:attribute name="start">
          <xsl:value-of select="$start"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:if test="@spacing = 'compact'">
        <xsl:attribute name="compact">
          <xsl:value-of select="@spacing"/>
        </xsl:attribute>
      </xsl:if>
      <!-- FIXME: @inheritnum -->
      <xsl:apply-templates select="listitem"/>
    </ol>
  </div>
</xsl:template>

<!-- = orderedlist/listitem = -->
<xsl:template match="orderedlist/listitem">
  <xsl:variable name="first"
             select="not(preceding-sibling::*
                     [not(self::blockinfo) and not(self::title) and not(self::titleabbrev)])"/>
  <li>
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$first">
      <xsl:attribute name="class">
        <xsl:text>li-first</xsl:text>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@override">
      <xsl:attribute name="value">
        <xsl:value-of select="@override"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates/>
  </li>
</xsl:template>

<!-- = procedure = -->
<xsl:template match="procedure">
  <div class="block list procedure">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates select="*[not(self::step)]"/>
    <xsl:choose>
      <xsl:when test="count(step) = 1">
        <ul class="procedure">
          <xsl:apply-templates select="step"/>
        </ul>
      </xsl:when>
      <xsl:otherwise>
        <ol class="procedure">
          <xsl:apply-templates select="step"/>
        </ol>
      </xsl:otherwise>
    </xsl:choose>
  </div>
</xsl:template>

<!-- = seg = -->
<xsl:template match="seg">
  <xsl:variable name="position" select="count(preceding-sibling::seg) + 1"/>
  <p class="block seg">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$position = 1">
      <xsl:attribute name="class">
        <xsl:text>segfirst</xsl:text>
      </xsl:attribute>
    </xsl:if>
    <xsl:apply-templates select="../../segtitle[position() = $position]"/>
    <xsl:apply-templates/>
  </p>
</xsl:template>

<!-- = seglistitem = -->
<xsl:template match="seglistitem">
  <xsl:param name="position" select="count(preceding-sibling::seglistitem) + 1"/>
  <div class="block seglistitem">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <div>
      <xsl:attribute name="class">
        <xsl:choose>
          <xsl:when test="($position mod 2) = 1">
            <xsl:value-of select="'odd'"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'even'"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:apply-templates/>
    </div>
  </div>
</xsl:template>

<!-- FIXME: Implement tabular segmentedlists -->
<!-- = segmentedlist = -->
<xsl:template match="segmentedlist">
  <div class="block list segmentedlist">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates select="title"/>
    <xsl:apply-templates select="seglistitem"/>
  </div>
</xsl:template>

<!-- = segtitle = -->
<xsl:template match="segtitle">
  <!-- FIXME: no style tags -->
  <b>
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:apply-templates/>
    <!-- FIXME: i18n -->
    <xsl:text>: </xsl:text>
  </b>
</xsl:template>

<!-- = simplelist = -->
<xsl:template match="simplelist">
  <xsl:variable name="columns">
    <xsl:choose>
      <xsl:when test="@columns">
        <xsl:value-of select="@columns"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="1"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="@type = 'inline'">
      <span class="simplelist">
        <xsl:if test="@lang">
          <xsl:attribute name="dir">
            <xsl:call-template name="l10n.direction">
              <xsl:with-param name="lang" select="@lang"/>
            </xsl:call-template>
          </xsl:attribute>
        </xsl:if>
        <xsl:call-template name="db2html.anchor"/>
        <xsl:for-each select="member">
          <xsl:if test="position() != 1">
            <xsl:call-template name="l10n.gettext">
              <xsl:with-param name="msgid" select="', '"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:apply-templates select="."/>
        </xsl:for-each>
      </span>
    </xsl:when>
    <xsl:when test="@type = 'horiz'">
      <div class="block list simplelist">
        <xsl:if test="@lang">
          <xsl:attribute name="dir">
            <xsl:call-template name="l10n.direction">
              <xsl:with-param name="lang" select="@lang"/>
            </xsl:call-template>
          </xsl:attribute>
        </xsl:if>
        <xsl:call-template name="db2html.anchor"/>
        <table class="simplelist">
          <xsl:for-each select="member[$columns = 1 or position() mod $columns = 1]">
            <tr>
              <td class="td-first">
                <xsl:apply-templates select="."/>
              </td>
              <xsl:for-each select="following-sibling::member[
                                    position() &lt; $columns]">
                <td>
                  <xsl:apply-templates select="."/>
                </td>
              </xsl:for-each>
              <xsl:variable name="fcount" select="count(following-sibling::member)"/>
              <xsl:if test="$fcount &lt; ($columns - 1)">
                <td colspan="{$columns - $fcount - 1}"/>
              </xsl:if>
            </tr>
          </xsl:for-each>
        </table>
      </div>
    </xsl:when>
    <xsl:otherwise>
      <div class="block list simplelist">
        <xsl:if test="@lang">
          <xsl:attribute name="dir">
            <xsl:call-template name="l10n.direction">
              <xsl:with-param name="lang" select="@lang"/>
            </xsl:call-template>
          </xsl:attribute>
        </xsl:if>
        <xsl:call-template name="db2html.anchor"/>
        <xsl:variable name="rows" select="ceiling(count(member) div $columns)"/>
        <table class="simplelist">
          <xsl:for-each select="member[position() &lt;= $rows]">
            <tr>
              <td class="td-first">
                <xsl:apply-templates select="."/>
              </td>
              <xsl:for-each select="following-sibling::member[
                                    position() mod $rows = 0]">
                <td>
                  <xsl:apply-templates select="."/>
                </td>
              </xsl:for-each>
              <xsl:if test="position() = $rows">
                <xsl:variable name="fcount"
                              select="count(following-sibling::member[position() mod $rows = 0])"/>
                <xsl:if test="$fcount &lt; ($columns - 1)">
                  <td colspan="{$columns - $fcount - 1}"/>
                </xsl:if>
              </xsl:if>
            </tr>
          </xsl:for-each>
        </table>
      </div>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- FIXME: Do something with @performance -->
<!-- = step = -->
<xsl:template match="step">
  <xsl:variable name="first"
             select="not(preceding-sibling::*
                     [not(self::blockinfo) and not(self::title) and not(self::titleabbrev)])"/>
  <li>
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$first">
      <xsl:attribute name="class">
        <xsl:text>li-first</xsl:text>
      </xsl:attribute>
    </xsl:if>
    <xsl:apply-templates/>
  </li>
</xsl:template>

<!-- FIXME: Do something with @performance -->
<!-- = substeps = -->
<xsl:template match="substeps">
  <xsl:variable name="depth" select="count(ancestor::substeps)"/>
  <div class="block list substeps">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <ol class="substeps">
      <xsl:attribute name="type">
        <xsl:choose>
          <xsl:when test="$depth mod 3 = 0">a</xsl:when>
          <xsl:when test="$depth mod 3 = 1">i</xsl:when>
          <xsl:when test="$depth mod 3 = 2">1</xsl:when>
        </xsl:choose>
      </xsl:attribute>
      <xsl:apply-templates/>
    </ol>
  </div>
</xsl:template>

<!-- = term = -->
<xsl:template match="term">
  <dt>
    <xsl:choose>
      <xsl:when test="@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
      <xsl:when test="../@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="../@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
    </xsl:choose>
    <xsl:attribute name="class">
      <xsl:text>term</xsl:text>
      <xsl:if test="not(../preceding-sibling::varlistentry)">
        <xsl:text> dt-first</xsl:text>
      </xsl:if>
    </xsl:attribute>
    <xsl:if test="../varlistentry/@id and not(preceding-sibling::term)">
      <xsl:call-template name="db2html.anchor">
        <xsl:with-param name="node" select=".."/>
      </xsl:call-template>
    </xsl:if>
    <xsl:apply-templates/>
  </dt>
</xsl:template>

<!-- = variablelist = -->
<xsl:template match="variablelist">
  <div class="block list variablelist">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates select="*[not(self::varlistentry)]"/>
    <dl class="variablelist">
      <xsl:apply-templates select="varlistentry"/>
    </dl>
  </div>
</xsl:template>

<!-- = varlistentry = -->
<xsl:template match="varlistentry">
  <xsl:apply-templates select="term"/>
  <xsl:apply-templates select="listitem"/>
</xsl:template>

<!-- = varlistentry/listitem = -->
<xsl:template match="varlistentry/listitem">
  <dd>
    <xsl:choose>
      <xsl:when test="@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
      <xsl:when test="../@lang">
        <xsl:attribute name="dir">
          <xsl:call-template name="l10n.direction">
            <xsl:with-param name="lang" select="../@lang"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:when>
    </xsl:choose>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates/>
  </dd>
</xsl:template>


<!--
FIXME: are these still necessary with block-first?
These templates strip the p tag around single-paragraph list items to avoid
introducing extra spacing.  We don't do this for list items in varlistentry
elements because it adds a non-negligable amount of processing time for
non-trivial documents.
-->
<xsl:template match="itemizedlist/listitem/para[
              not(preceding-sibling::* or following-sibling::*)  and
              not(../preceding-sibling::listitem[count(*) != 1]) and
              not(../following-sibling::listitem[count(*) != 1]) ]">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>
<xsl:template match="orderedlist/listitem/para[
              not(preceding-sibling::* or following-sibling::*)  and
              not(../preceding-sibling::listitem[count(*) != 1]) and
              not(../following-sibling::listitem[count(*) != 1]) ]">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

</xsl:stylesheet>
