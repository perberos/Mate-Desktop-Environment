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
DocBook to HTML - Tables
:Requires: db2html-block db2html-inline db2html-xref gettext

REMARK: This needs lots of talk about CALS
-->


<!--**==========================================================================
db2html.row
Creates a #{tr} element for a #{row} element
$row: The #{row} element to process
$colspecs: The #{colspec} elements currently in scope
$spanspecs: The #{spanspec} elements currently in scope
$colsep: Whether column separators are currently enabled
$rowsep: Whether column separators are currently enabled
$spanstr: The string representation of the row spans

FIXME
-->
<xsl:template name="db2html.row">
  <xsl:param name="row" select="."/>
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:param name="colsep" select="''"/>
  <xsl:param name="rowsep" select="''"/>
  <xsl:param name="spanstr"/>
  <tr>
    <xsl:if test="$row/../self::tbody and (count($row/preceding-sibling::row) mod 2 = 1)">
      <xsl:attribute name="class">
        <xsl:text>tr-shade</xsl:text>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$row/*[1]">
      <xsl:call-template name="db2html.entry">
        <xsl:with-param name="entry" select="$row/*[1]"/>
        <xsl:with-param name="colspecs" select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
        <xsl:with-param name="colsep" select="$colsep"/>
        <xsl:with-param name="rowsep">
          <xsl:choose>
            <xsl:when test="$row/@rowsep">
              <xsl:value-of select="$row/@rowsep"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$rowsep"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
        <xsl:with-param name="spanstr" select="$spanstr"/>
      </xsl:call-template>
    </xsl:if>
  </tr>
  <xsl:variable name="following" select="$row/following-sibling::row[1]"/>
  <xsl:if test="$following">
    <xsl:call-template name="db2html.row">
      <xsl:with-param name="row"       select="$following"/>
      <xsl:with-param name="colspecs"  select="$colspecs"/>
      <xsl:with-param name="spanspecs" select="$spanspecs"/>
      <xsl:with-param name="colsep"    select="$colsep"/>
      <xsl:with-param name="rowsep"    select="$rowsep"/>
      <xsl:with-param name="spanstr">
        <xsl:call-template name="db2html.spanstr">
          <xsl:with-param name="row"       select="$row"/>
          <xsl:with-param name="colspecs"  select="$colspecs"/>
          <xsl:with-param name="spanspecs" select="$spanspecs"/>
          <xsl:with-param name="spanstr"   select="$spanstr"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:if>
</xsl:template>


<!--**==========================================================================
db2html.entry
Creates a #{td} element for an #{entry} element
$entry: The #{entry} element to process
$colspecs: The #{colspec} elements currently in scope
$spanspecs: The #{spanspec} elements currently in scope
$colsep: Whether column separators are currently enabled
$rowsep: Whether column separators are currently enabled
$colpos: The output column position currently being considered
$colnum: The actual column number of ${entry}
$spanstr: The string representation of the row spans

This template processes a single #{entry} element and generates #{td} elements
as needed.  It then calls itself on the following #{entry} element, adjusting
parameters as necessary.  Under certain conditions, this template may not be
able to output a #{td} element immediately.  In these cases, it makes whatever
adjustments are needed and calls itself or *{db2html.entry.implicit} (which,
in turn, calls this template again when it's finished).

Three parameters are used to determine whether a #{td} element can be output.
The ${spanstr} parameter provides infomation about row spans in effect from
entries in previous rows; the ${colpos} parameter specifies which column we
would output to if we created a #{td}; and the ${colnum} parameter specifies
which column this #{entry} should be in, according to any relevant #{colspec}
or #{spanspec} elemets.

There are two conditions that cause this template not to output a #{td} element
immediately: if the ${spanstr} parameter does not start with #{0:}, and if the
${colpos} parameter is less than the ${colnum} parameter.

The ${spanstr} parameter specifies the row spans in effect from entries in
previous rows.  As this template iterates over the #{entry} elements, it strips
off parts of ${spanstr} so that only the parts relevant to the #{entry} are
present.  If ${spanstr} does not start with #{0:}, then an entry in a previous
row occupies this column position.  In this case, that value is removed from
${spanstr}, the ${colpos} parameter is incremented, and *{db2html.entry} is
called again.  Additionally, since *{db2html.entry.colnum} doesn't consider
row spans, the ${colnum} parameter may be incremented as well.

If the ${colpos} parameter is less than the ${colnum} parameter, then the
document has skipped entries by explicitly referencing a column.  This is
allowed in CALS tables, but not in HTML.  To fill the blank spaces, we call
*{db2html.entry.implicit}, which outputs an empty #{td} element spanning as
many columns as necessary to fill in the blanks.  The *{db2html.entry.implicit}
template then calls this template again with appropriate parameter values.

When this template is finally able to output a #{td} element, it calculates
appropriate values for the #{style} and #{class} attribute based on DocBook
attributes on the #{entry}, the relevant #{colspec} or #{spanspec}, and any
relevant ancestor elements.  It then calls itself on the following #{entry}
element to output the next #{td}.
-->
<xsl:template name="db2html.entry">
  <xsl:param name="entry" select="."/>
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:param name="colsep" select="''"/>
  <xsl:param name="rowsep" select="''"/>
  <xsl:param name="colpos" select="1"/>
  <xsl:param name="colnum">
    <xsl:call-template name="db2html.entry.colnum">
      <xsl:with-param name="entry"     select="$entry"/>
      <xsl:with-param name="colspecs"  select="$colspecs"/>
      <xsl:with-param name="spanspecs" select="$spanspecs"/>
      <xsl:with-param name="colpos"    select="$colpos"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="spanstr"/>
  <xsl:variable name="colspan">
    <xsl:choose>
      <xsl:when test="$entry/@spanname or $entry/@namest">
        <xsl:call-template name="db2html.entry.colspan">
          <xsl:with-param name="entry"     select="$entry"/>
          <xsl:with-param name="colspecs"  select="$colspecs"/>
          <xsl:with-param name="spanspecs" select="$spanspecs"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:choose>
    <!-- Another entry has a rowspan that covers this column position -->
    <xsl:when test="$spanstr != '' and not(starts-with($spanstr, '0:'))">
      <xsl:choose>
        <xsl:when test="$colnum = $colpos">
          <xsl:call-template name="db2html.entry">
            <xsl:with-param name="entry"     select="$entry"/>
            <xsl:with-param name="colspecs"  select="$colspecs"/>
            <xsl:with-param name="spanspecs" select="$spanspecs"/>
            <xsl:with-param name="colsep"    select="$colsep"/>
            <xsl:with-param name="rowsep"    select="$rowsep"/>
            <xsl:with-param name="colpos"    select="$colpos + 1"/>
            <xsl:with-param name="colnum"    select="$colnum + 1"/>
            <xsl:with-param name="colspan"   select="$colspan"/>
            <xsl:with-param name="spanstr"   select="substring-after($spanstr, ':')"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="db2html.entry">
            <xsl:with-param name="entry"     select="$entry"/>
            <xsl:with-param name="colspecs"  select="$colspecs"/>
            <xsl:with-param name="spanspecs" select="$spanspecs"/>
            <xsl:with-param name="colsep"    select="$colsep"/>
            <xsl:with-param name="rowsep"    select="$rowsep"/>
            <xsl:with-param name="colpos"    select="$colpos + 1"/>
            <xsl:with-param name="colnum"    select="$colnum"/>
            <xsl:with-param name="colspan"   select="$colspan"/>
            <xsl:with-param name="spanstr"   select="substring-after($spanstr, ':')"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- We need to insert implicit td elements to cover blank space -->
    <xsl:when test="$colnum &gt; $colpos">
      <xsl:call-template name="db2html.entry.implicit">
        <xsl:with-param name="entry"     select="$entry"/>
        <xsl:with-param name="colspecs"  select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
        <xsl:with-param name="colsep"    select="$colsep"/>
        <xsl:with-param name="rowsep"    select="$rowsep"/>
        <xsl:with-param name="colpos"    select="$colpos"/>
        <xsl:with-param name="colnum"    select="$colnum"/>
        <xsl:with-param name="colspan"   select="1"/>
        <xsl:with-param name="spanstr"   select="$spanstr"/>
      </xsl:call-template>
    </xsl:when>
    <!-- We can output the td for this entry -->
    <xsl:otherwise>
      <xsl:if test="$colnum &lt; $colpos">
        <xsl:message>Column positions are not aligned.</xsl:message>
      </xsl:if>
      <xsl:variable name="element">
        <xsl:choose>
          <xsl:when test="$entry/../../self::thead or $entry/../../self::tfoot">th</xsl:when>
          <xsl:otherwise>td</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="rowspan">
        <xsl:choose>
          <xsl:when test="$entry/@morerows">
            <xsl:value-of select="$entry/@morerows + 1"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="1"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="align">
        <xsl:choose>
          <xsl:when test="$entry/@align">
            <xsl:value-of select="$entry/@align"/>
          </xsl:when>
          <xsl:when test="$colspecs[@colname = $entry/@colname]/@align">
            <xsl:value-of select="$colspecs[@colname = $entry/@colname]/@align"/>
          </xsl:when>
          <xsl:when test="$colspecs[@colname = $entry/@namest]/@align">
            <xsl:value-of select="$colspecs[@colname = $entry/@namest]/@align"/>
          </xsl:when>
          <xsl:when test="$spanspecs[@spanname = $entry/@spanname]/@align">
            <xsl:value-of select="$spanspecs[@spanname = $entry/@spanname]/@align"/>
          </xsl:when>
          <xsl:when test="$colspecs[position() = $colnum]/@align">
            <xsl:value-of select="$colspecs[position() = $colnum]/@align"/>
          </xsl:when>
          <xsl:when test="$entry/../../../@align">
            <xsl:value-of select="$entry/../../../@align"/>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="valign">
        <xsl:choose>
          <xsl:when test="$entry/@valign">
            <xsl:value-of select="$entry/@valign"/>
          </xsl:when>
          <xsl:when test="$colspecs[@colname = $entry/@colname]/@valign">
            <xsl:value-of
                select="$colspecs[@colname = $entry/@colname]/@valign"/>
          </xsl:when>
          <xsl:when test="$colspecs[@colname = $entry/@namest]/@valign">
            <xsl:value-of select="$colspecs[@colname = $entry/@namest]/@valign"/>
          </xsl:when>
          <xsl:when test="$spanspecs[@spanname = $entry/@spanname]/@valign">
            <xsl:value-of select="$spanspecs[@spanname = $entry/@spanname]/@valign"/>
          </xsl:when>
          <xsl:when test="$colspecs[position() = $colnum]/@valign">
            <xsl:value-of select="$colspecs[position() = $colnum]/@valign"/>
          </xsl:when>
          <xsl:when test="$entry/../@valign">
            <xsl:value-of select="$entry/../@valign"/>
          </xsl:when>
          <xsl:when test="$entry/../../@valign">
            <xsl:value-of select="$entry/../../@valign"/>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="char">
        <xsl:choose>
          <xsl:when test="$align != 'char'"/>
          <xsl:when test="$entry/@char">
            <xsl:value-of select="$entry/@char"/>
          </xsl:when>
          <xsl:when test="$colspecs[@colname = $entry/@colname]/@char">
            <xsl:value-of select="$colspecs[@colname = $entry/@colname]/@char"/>
          </xsl:when>
          <xsl:when test="$colspecs[@colname = $entry/@namest]/@char">
            <xsl:value-of select="$colspecs[@colname = $entry/@namest]/@char"/>
          </xsl:when>
          <xsl:when test="$spanspecs[@spanname = $entry/@spanname]/@char">
            <xsl:value-of select="$spanspecs[@spanname = $entry/@spanname]/@char"/>
          </xsl:when>
          <xsl:when test="$colspecs[position() = $colnum]/@char">
            <xsl:value-of select="$colspecs[position() = $colnum]/@char"/>
          </xsl:when>
          <xsl:when test="$entry/../../../@char">
            <xsl:value-of select="$entry/../../@char"/>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="charoff">
        <xsl:choose>
          <xsl:when test="$align != 'char'"/>
          <xsl:when test="$entry/@charoff">
            <xsl:value-of select="$entry/@charoff"/>
          </xsl:when>
          <xsl:when test="$colspecs[@colname = $entry/@colname]/@charoff">
            <xsl:value-of select="$colspecs[@colname = $entry/@colname]/@charoff"/>
          </xsl:when>
          <xsl:when test="$colspecs[@colname = $entry/@namest]/@charoff">
            <xsl:value-of select="$colspecs[@colname = $entry/@namest]/@charoff"/>
          </xsl:when>
          <xsl:when test="$spanspecs[@spanname = $entry/@spanname]/@charoff">
            <xsl:value-of select="$spanspecs[@spanname = $entry/@spanname]/@charoff"/>
          </xsl:when>
          <xsl:when test="$colspecs[position() = $colnum]/@charoff">
            <xsl:value-of select="$colspecs[position() = $colnum]/@charoff"/>
          </xsl:when>
          <xsl:when test="$entry/../../../@charoff">
            <xsl:value-of select="$entry/../../@charoff"/>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="style">
        <xsl:if test="$align != ''">
          <xsl:value-of select="concat('text-align: ', $align, ';')"/>
        </xsl:if>
        <xsl:if test="$valign != ''">
          <xsl:value-of select="concat('vertical-align: ', $valign, ';')"/>
        </xsl:if>
      </xsl:variable>
      <xsl:variable name="class">
        <!-- td-colsep: whether to show a column separator -->
        <xsl:choose>
          <!-- FIXME: we need to handle @cols better -->
          <xsl:when test="number($colpos) + number($colspan) &gt; number($entry/ancestor::tgroup[1]/@cols)"/>
          <xsl:when test="$entry/@colsep">
            <xsl:if test="$entry/@colsep = '1'">
              <xsl:text> td-colsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$entry/@spanname and $spanspecs[@spanname = $entry/@spanname]/@colsep">
            <xsl:if test="$spanspecs[@spanname = $entry/@spanname]/@colsep = '1'">
              <xsl:text> td-colsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$entry/@colname and $colspecs[@colname = $entry/@colname]/@colsep">
            <xsl:if test="$colspecs[@colname = $entry/@colname]/@colsep = '1'">
              <xsl:text> td-colsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$entry/@nameend and $colspecs[@colname = $entry/@nameend]/@colsep">
            <xsl:if test="$colspecs[@colname = $entry/@nameend]/@colsep = '1'">
              <xsl:text> td-colsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$colspecs[position() = $colnum]/@colsep">
            <xsl:if test="$colspecs[position() = $colnum]/@colsep = '1'">
              <xsl:text> td-colsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$colsep = '0'"/>
          <xsl:otherwise>
            <xsl:text> td-colsep</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <!-- td-rowsep: whether to show a row separator -->
        <xsl:choose>
          <xsl:when test="count($entry/../following-sibling::row) &lt; number($rowspan)"/>
          <xsl:when test="$entry/@rowsep">
            <xsl:if test="$entry/@rowsep = '1'">
              <xsl:text> td-rowsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$entry/@spanname and $spanspecs[@spanname = $entry/@spanname]/@rowsep">
            <xsl:if test="$spanspecs[@spanname = $entry/@spanname]/@rowsep = '1'">
              <xsl:text> td-rowsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$entry/@colname and $colspecs[@colname = $entry/@colname]/@rowsep">
            <xsl:if test="$colspecs[@colname = $entry/@colname]/@rowsep = '1'">
              <xsl:text> td-rowsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$entry/@nameend and $colspecs[@colname = $entry/@nameend]/@rowsep">
            <xsl:if test="$colspecs[@colname = $entry/@nameend]/@rowsep = '1'">
              <xsl:text> td-rowsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$colspecs[position() = $colnum]/@rowsep">
            <xsl:if test="$colspecs[position() = $colnum]/@rowsep = '1'">
              <xsl:text> td-rowsep</xsl:text>
            </xsl:if>
          </xsl:when>
          <xsl:when test="$rowsep = '1'">
            <xsl:text> td-rowsep</xsl:text>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>
      <!-- Finally, output the td or th element -->
      <xsl:element name="{$element}" namespace="{$db2html.namespace}">
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
        <xsl:if test="$style != ''">
          <xsl:attribute name="style">
            <xsl:value-of select="normalize-space($style)"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$class != ''">
          <xsl:attribute name="class">
            <xsl:value-of select="normalize-space($class)"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="number($rowspan) &gt; 1">
          <xsl:attribute name="rowspan">
            <xsl:value-of select="$rowspan"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$colspan &gt; 1">
          <xsl:attribute name="colspan">
            <xsl:value-of select="$colspan"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$char != ''">
          <xsl:attribute name="char">
            <xsl:value-of select="$char"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$charoff != ''">
          <xsl:attribute name="charoff">
            <xsl:value-of select="$charoff"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:apply-templates select="$entry/node()"/>
      </xsl:element>
      <!-- And process the next entry -->
      <xsl:variable name="following" select="$entry/following-sibling::*[1]"/>
      <xsl:if test="$following">
        <xsl:call-template name="db2html.entry">
          <xsl:with-param name="entry"     select="$following"/>
          <xsl:with-param name="colspecs"  select="$colspecs"/>
          <xsl:with-param name="spanspecs" select="$spanspecs"/>
          <xsl:with-param name="colsep"    select="$colsep"/>
          <xsl:with-param name="rowsep"    select="$rowsep"/>
          <xsl:with-param name="colpos"    select="$colpos + $colspan"/>
          <xsl:with-param name="spanstr">
            <xsl:call-template name="db2html.spanstr.pop">
              <xsl:with-param name="colspecs"  select="$colspecs"/>
              <xsl:with-param name="spanspecs" select="$spanspecs"/>
              <xsl:with-param name="colspan"   select="$colspan"/>
              <xsl:with-param name="spanstr"   select="$spanstr"/>
            </xsl:call-template>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2html.entry.implicit
Creates an implicit #{td} element to fill up unoccupied columns
$entry: The #{entry} element currently being processed
$colspecs: The #{colspec} elements currently in scope
$spanspecs: The #{spanspec} elements currently in scope
$colsep: Whether column separators are currently enabled
$rowsep: Whether column separators are currently enabled
$colpos: The output column position currently being considered
$colnum: The actual column number of ${entry}
$colspan: How many columns the implicit #{td} currently spans
$spanstr: The string representation of the row spans

CALS tables in DocBook don't need to have #{entry} elements for each column
in each row, even when the column is not covered by a row-spanning entry from
a previous row.  An #{entry} can explicitly specify which column it's in, and
any previous unfilled columns are considered blank.  Since HTML tables don't
have this mechanism, we have to insert blank #{td} elements to fill the gaps.

When *{db2html.entry} detects a blank entry, it will call this template with
the approprite parameters.  This template then calls itself recursively, each
time adjusting the ${colpos}, ${colspan}, and ${spanstr} parameters, until it
comes across the last column that needs to be filled.  It then outputs a #{td}
element with an appropriate #{colspan} attribute.

Finally, this template calls *{db2html.entry} again on ${entry}.  With the
values of ${colpos} and ${spanstr} suitably adjusted, that template is then
able to output the #{td} for the #{entry} element.
-->
<xsl:template name="db2html.entry.implicit">
  <xsl:param name="entry"/>
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:param name="colsep" select="''"/>
  <xsl:param name="rowsep" select="''"/>
  <xsl:param name="colpos" select="1"/>
  <xsl:param name="colnum">
    <xsl:call-template name="db2html.entry.colnum">
      <xsl:with-param name="entry"     select="$entry"/>
      <xsl:with-param name="colspecs"  select="$colspecs"/>
      <xsl:with-param name="spanspecs" select="$spanspecs"/>
      <xsl:with-param name="colpos"    select="$colpos"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="colspan"/>
  <xsl:param name="spanstr"/>

  <xsl:variable name="element">
    <xsl:choose>
      <xsl:when test="$entry/../../self::thead or $entry/../../self::tfoot">th</xsl:when>
      <xsl:otherwise>td</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="class">
    <xsl:if test="$colsep != '0'">
      <xsl:text> td-colsep</xsl:text>
    </xsl:if>
    <xsl:if test="$rowsep = '1' and $entry/../following-sibling::row">
      <xsl:text> td-rowsep</xsl:text>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$spanstr != '' and not(starts-with($spanstr, '0:'))">
      <xsl:element name="{$element}" namespace="{$db2html.namespace}">
        <xsl:if test="$class != ''">
          <xsl:attribute name="class">
            <xsl:value-of select="normalize-space($class)"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:attribute name="colspan">
          <xsl:value-of select="$colspan - 1"/>
        </xsl:attribute>
        <xsl:text>&#160;</xsl:text>
      </xsl:element>
      <xsl:call-template name="db2html.entry">
        <xsl:with-param name="entry"     select="$entry"/>
        <xsl:with-param name="colspecs"  select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
        <xsl:with-param name="colsep"    select="$colsep"/>
        <xsl:with-param name="rowsep"    select="$rowsep"/>
        <xsl:with-param name="colpos"    select="$colpos"/>
        <xsl:with-param name="colnum"    select="$colnum"/>
        <xsl:with-param name="spanstr"   select="$spanstr"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$colnum - $colpos = 1">
      <xsl:element name="{$element}" namespace="{$db2html.namespace}">
        <xsl:if test="$class != ''">
          <xsl:attribute name="class">
            <xsl:value-of select="normalize-space($class)"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:attribute name="colspan">
          <xsl:value-of select="$colspan"/>
        </xsl:attribute>
        <xsl:text>&#160;</xsl:text>
      </xsl:element>
      <xsl:call-template name="db2html.entry">
        <xsl:with-param name="entry"     select="$entry"/>
        <xsl:with-param name="colspecs"  select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
        <xsl:with-param name="colsep"    select="$colsep"/>
        <xsl:with-param name="rowsep"    select="$rowsep"/>
        <xsl:with-param name="colpos"    select="$colpos + 1"/>
        <xsl:with-param name="colnum"    select="$colnum"/>
        <xsl:with-param name="spanstr"   select="substring-after($spanstr, ':')"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="db2html.entry.implicit">
        <xsl:with-param name="entry"     select="$entry"/>
        <xsl:with-param name="colspecs"  select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
        <xsl:with-param name="colsep"    select="$colsep"/>
        <xsl:with-param name="rowsep"    select="$rowsep"/>
        <xsl:with-param name="colpos"    select="$colpos + 1"/>
        <xsl:with-param name="colnum"    select="$colnum"/>
        <xsl:with-param name="colspan"   select="$colspan + 1"/>
        <xsl:with-param name="spanstr"   select="substring-after($spanstr, ':')"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2html.entry.colnum
Calculates the actual column number for an #{entry} element
$entry: The #{entry} element to process
$colspecs: The #{colspec} elements currently in scope
$spanspecs: The #{spanspec} elements currently in scope
$colpos: The column position, as passed by the preceding #{entry}

FIXME
-->
<xsl:template name="db2html.entry.colnum">
  <xsl:param name="entry" select="."/>
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:param name="colpos" select="0"/>
  <xsl:choose>
    <xsl:when test="$entry/@spanname">
      <xsl:variable name="spanspec"
                    select="$spanspecs[@spanname = $entry/@spanname]"/>
      <xsl:variable name="colspec"
                    select="$colspecs[@colname = $spanspec/@namest]"/>
      <xsl:call-template name="db2html.colspec.colnum">
        <xsl:with-param name="colspec" select="$colspec"/>
        <xsl:with-param name="colspecs" select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$entry/@colname">
      <xsl:variable name="colspec"
                    select="$colspecs[@colname = $entry/@colname]"/>
      <xsl:call-template name="db2html.colspec.colnum">
        <xsl:with-param name="colspec" select="$colspec"/>
        <xsl:with-param name="colspecs" select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$entry/@namest">
      <xsl:variable name="colspec"
                    select="$colspecs[@colname = $entry/@namest]"/>
      <xsl:call-template name="db2html.colspec.colnum">
        <xsl:with-param name="colspec" select="$colspec"/>
        <xsl:with-param name="colspecs" select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$colpos"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2html.colspec.colnum
Calculates the column number for a #{colspec} element
$colspec: The #{colspec} element to process
$colspecs: The #{colspec} elements currently in scope
$spanspecs: The #{spanspec} elements currently in scope

FIXME
-->
<xsl:template name="db2html.colspec.colnum">
  <xsl:param name="colspec" select="."/>
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:choose>
    <xsl:when test="$colspec/@colnum">
      <xsl:value-of select="$colspec/@colnum"/>
    </xsl:when>
    <xsl:when test="$colspec/preceding-sibling::colspec">
      <xsl:variable name="prec.colspec.colnum">
        <xsl:call-template name="db2html.colspec.colnum">
          <xsl:with-param name="colspec"
                          select="$colspec/preceding-sibling::colspec[1]"/>
          <xsl:with-param name="colspecs"  select="$colspecs"/>
          <xsl:with-param name="spanspecs" select="$spanspecs"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="$prec.colspec.colnum + 1"/>
    </xsl:when>
    <xsl:otherwise>1</xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2html.entry.colspan
Calculates the #{colspan} for an #{entry} element
$entry: The #{entry} element to process
$colspecs: The #{colspec} elements currently in scope
$spanspecs: The #{spanspec} elements currently in scope

This template calculates how many columns an #{entry} element should span.
In CALS tables, column spanning is done by specifying starting and ending
#{colspec} elements, or by specifying a #{spanspec} element which specifies
starting and ending #{colspec} elements.
-->
<xsl:template name="db2html.entry.colspan">
  <xsl:param name="entry" select="."/>
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:variable name="namest">
    <xsl:choose>
      <xsl:when test="$entry/@spanname">
        <xsl:value-of
         select="$spanspecs[@spanname = $entry/@spanname]/@namest"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$entry/@namest"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="nameend">
    <xsl:choose>
      <xsl:when test="$entry/@spanname">
        <xsl:value-of
         select="$spanspecs[@spanname = $entry/@spanname]/@nameend"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$entry/@nameend"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="colnumst">
    <xsl:call-template name="db2html.colspec.colnum">
      <xsl:with-param name="colspec"   select="$colspecs[@colname = $namest]"/>
      <xsl:with-param name="colspecs"  select="$colspecs"/>
      <xsl:with-param name="spanspecs" select="$spanspecs"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="colnumend">
    <xsl:call-template name="db2html.colspec.colnum">
      <xsl:with-param name="colspec"   select="$colspecs[@colname = $nameend]"/>
      <xsl:with-param name="colspecs"  select="$colspecs"/>
      <xsl:with-param name="spanspecs" select="$spanspecs"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="$namest = '' or $nameend = ''">1</xsl:when>
    <xsl:when test="$colnumend &gt; $colnumst">
      <xsl:value-of select="$colnumend - $colnumst + 1"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$colnumst - $colnumend + 1"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>




<!-- FIXME below -->


<!--**==========================================================================
db2html.spanstr
Generates a string specifying the row spans in effect
$colspecs: The #{colspec} elements currently in scope
$spanspecs: The #{spanspec} elements currently in scope
$spanstr: The ${spanstr} parameter used by the previous row

REMARK: This template needs to be explained in detail, but I forgot how it works.
-->
<xsl:template name="db2html.spanstr">
  <xsl:param name="row"    select="."/>
  <xsl:param name="entry"  select="$row/*[1]"/>
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:param name="spanstr"/>
  <xsl:param name="colpos" select="1"/>
  <xsl:param name="colnum">
    <xsl:call-template name="db2html.entry.colnum">
      <xsl:with-param name="entry"     select="$entry"/>
      <xsl:with-param name="colspecs"  select="$colspecs"/>
      <xsl:with-param name="spanspecs" select="$spanspecs"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="colspan">
    <xsl:choose>
      <xsl:when test="$entry/@spanname or $entry/@namest">
        <xsl:call-template name="db2html.entry.colspan">
          <xsl:with-param name="colspecs"  select="$colspecs"/>
          <xsl:with-param name="spanspecs" select="$spanspecs"/>
          <xsl:with-param name="entry"     select="$entry"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
  </xsl:param>
  <xsl:variable name="following.spanstr">
    <xsl:call-template name="db2html.spanstr.pop">
      <xsl:with-param name="colspecs"  select="$colspecs"/>
      <xsl:with-param name="spanspecs" select="$spanspecs"/>
      <xsl:with-param name="colnum"    select="$colnum"/>
      <xsl:with-param name="colspan"   select="$colspan"/>
      <xsl:with-param name="spanstr"   select="$spanstr"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="$spanstr != '' and not(starts-with($spanstr, '0:'))">
      <xsl:value-of select="substring-before($spanstr, ':') - 1"/>
      <xsl:text>:</xsl:text>
      <xsl:call-template name="db2html.spanstr">
        <xsl:with-param name="row"       select="$row"/>
        <xsl:with-param name="entry"     select="$entry"/>
        <xsl:with-param name="colspecs"  select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
        <xsl:with-param name="spanstr"   select="substring-after($spanstr, ':')"/>
        <xsl:with-param name="colpos"    select="$colpos + 1"/>
        <xsl:with-param name="colnum"    select="$colnum"/>
        <xsl:with-param name="colspan"   select="$colspan"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$colnum &gt; $colpos">
      <xsl:text>0:</xsl:text>
      <xsl:call-template name="db2html.spanstr">
        <xsl:with-param name="row"       select="$row"/>
        <xsl:with-param name="entry"     select="$entry"/>
        <xsl:with-param name="colspecs"  select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
        <xsl:with-param name="spanstr"   select="$following.spanstr"/>
        <xsl:with-param name="colpos"    select="$colpos + $colspan"/>
        <xsl:with-param name="colnum"    select="$colnum"/>
        <xsl:with-param name="colspan"   select="$colspan"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="copy-string">
        <xsl:with-param name="count" select="$colspan"/>
        <xsl:with-param name="string">
          <xsl:choose>
            <xsl:when test="$entry/@morerows">
              <xsl:value-of select="$entry/@morerows"/>
            </xsl:when>
            <xsl:otherwise>0</xsl:otherwise>
          </xsl:choose>
          <xsl:text>:</xsl:text>
        </xsl:with-param>
      </xsl:call-template>
      <xsl:choose>
        <xsl:when test="$entry/following-sibling::*[1]">
          <xsl:call-template name="db2html.spanstr">
            <xsl:with-param name="row"       select="$row"/>
            <xsl:with-param name="entry"     select="$entry/following-sibling::*[1]"/>
            <xsl:with-param name="colspecs"  select="$colspecs"/>
            <xsl:with-param name="spanspecs" select="$spanspecs"/>
            <xsl:with-param name="spanstr"   select="$following.spanstr"/>
            <xsl:with-param name="colpos"    select="$colpos + $colspan"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$following.spanstr != ''">
          <xsl:call-template name="db2html.spanstr.decrement">
            <xsl:with-param name="spanstr"   select="$following.spanstr"/>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2html.spanstr.pop
Calculates the remaining spans after an #{entry} element
$colspecs: The #{colspec} elements currently in scope
$spanspecs: The #{spanspec} elements currently in scope
$colspan: The number of columns to pop
$spanstr: The string representation of the column spans

REMARK: This template needs to be explained in detail, but I forgot how it works.
-->
<xsl:template name="db2html.spanstr.pop">
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:param name="colspan" select="1"/>
  <xsl:param name="spanstr" select="''"/>
  <xsl:choose>
    <xsl:when test="$colspan &gt; 0">
      <xsl:call-template name="db2html.spanstr.pop">
        <xsl:with-param name="colspecs"  select="$colspecs"/>
        <xsl:with-param name="spanspecs" select="$spanspecs"/>
        <xsl:with-param name="colspan"   select="$colspan - 1"/>
        <xsl:with-param name="spanstr"   select="substring-after($spanstr, ':')"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$spanstr"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--#* db2html.spanstr.decrement -->
<xsl:template name="db2html.spanstr.decrement">
  <xsl:param name="spanstr"/>
  <xsl:if test="$spanstr != ''">
    <xsl:choose>
      <xsl:when test="starts-with($spanstr, '0:')">
        <xsl:text>0:</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="span" select="substring-before($spanstr, ':')"/>
        <xsl:value-of select="$span - 1"/>
        <xsl:text>:</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="db2html.spanstr.decrement">
      <xsl:with-param name="spanstr" select="substring-after($spanstr, ':')"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!--#* copy-string -->
<!-- FIXME: replace with str:padding? -->
<xsl:template name="copy-string">
  <xsl:param name="count" select="1"/>
  <xsl:param name="string"/>
  <xsl:if test="$count &gt; 0">
    <xsl:value-of select="$string"/>
    <xsl:call-template name="copy-string">
      <xsl:with-param name="count" select="$count - 1"/>
      <xsl:with-param name="string" select="$string"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>






<!-- = table = -->
<xsl:template match="table | informaltable">
  <div class="table block block-indent">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates select="title"/>
    <!-- FIXME: I have no idea what I'm supposed to do with textobject -->
    <xsl:choose>
      <xsl:when test="graphic | mediaobject">
        <xsl:apply-templates select="graphic | mediaobject"/>
      </xsl:when>
      <xsl:when test="tgroup">
        <xsl:apply-templates select="tgroup"/>
      </xsl:when>
      <xsl:when test="tr">
        <xsl:apply-templates select="col | colgroup | tr"/>
        <xsl:apply-templates select="caption"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="thead"/>
        <xsl:apply-templates select="tbody"/>
        <xsl:apply-templates select="tfoot"/>
        <xsl:apply-templates select="caption"/>
      </xsl:otherwise>
    </xsl:choose>
  </div>
</xsl:template>

<!-- = tgroup = -->
<xsl:template match="tgroup">
  <xsl:variable name="colsep">
    <xsl:choose>
      <xsl:when test="@colsep">
        <xsl:value-of select="string(@colsep)"/>
      </xsl:when>
      <xsl:when test="not(.//*[@colsep][1])"/>
      <xsl:otherwise>
        <xsl:text>0</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="rowsep">
    <xsl:choose>
      <xsl:when test="@rowsep">
        <xsl:value-of select="string(@rowsep)"/>
      </xsl:when>
      <xsl:when test="not(//*[@rowsep][1])"/>
      <xsl:otherwise>
        <xsl:text>0</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="style">
    <xsl:if test="../@frame = 'all' or not(../@frame)">
      <xsl:text>border: solid 1px; </xsl:text>
    </xsl:if>
    <xsl:if test="../@frame = 'none'">
      <xsl:text>border: none; </xsl:text>
    </xsl:if>
    <xsl:if test="../@frame = 'bottom' or ../@frame = 'topbot'">
      <xsl:text>border-bottom: solid 1px; </xsl:text>
    </xsl:if>
    <xsl:if test="../@frame = 'top' or ../@frame = 'topbot'">
      <xsl:text>border-top: solid 1px; </xsl:text>
    </xsl:if>
    <xsl:if test="../@frame = 'sides'">
      <xsl:text>border-left: solid 1px; border-right: solid 1px; </xsl:text>
    </xsl:if>
  </xsl:variable>
  <xsl:variable name="class">
    <xsl:if test="../@pgwide = '1'">
      <xsl:text>table-pgwide</xsl:text>
    </xsl:if>
  </xsl:variable>
  <table>
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="../title">
      <xsl:attribute name="summary">
        <xsl:value-of select="../title"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$style != ''">
      <xsl:attribute name="style">
        <xsl:value-of select="normalize-space($style)"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$class != ''">
      <xsl:attribute name="class">
        <xsl:value-of select="normalize-space($class)"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:apply-templates select="thead">
      <xsl:with-param name="colspecs" select="colspec"/>
      <xsl:with-param name="spanspecs" select="spanspec"/>
      <xsl:with-param name="colsep" select="$colsep"/>
      <xsl:with-param name="rowsep" select="$rowsep"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="tbody">
      <xsl:with-param name="colspecs" select="colspec"/>
      <xsl:with-param name="spanspecs" select="spanspec"/>
      <xsl:with-param name="colsep" select="$colsep"/>
      <xsl:with-param name="rowsep" select="$rowsep"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="tfoot">
      <xsl:with-param name="colspecs" select="colspec"/>
      <xsl:with-param name="spanspecs" select="spanspec"/>
      <xsl:with-param name="colsep" select="$colsep"/>
      <xsl:with-param name="rowsep" select="$rowsep"/>
    </xsl:apply-templates>
  </table>
</xsl:template>

<!-- = tbody | tfoot | thead = -->
<xsl:template match="tbody | tfoot | thead">
  <xsl:param name="colspecs"/>
  <xsl:param name="spanspecs"/>
  <xsl:param name="colsep" select="''"/>
  <xsl:param name="rowsep" select="''"/>
  <xsl:element name="{local-name(.)}" namespace="{$db2html.namespace}">
    <xsl:if test="@lang">
      <xsl:attribute name="dir">
        <xsl:call-template name="l10n.direction">
          <xsl:with-param name="lang" select="@lang"/>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@valign">
      <xsl:attribute name="valign">
        <xsl:value-of select="@valign"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="tr">
        <xsl:apply-templates select="tr"/>
      </xsl:when>
      <xsl:when test="colspec">
        <xsl:call-template name="db2html.row">
          <xsl:with-param name="row"       select="row[1]"/>
          <xsl:with-param name="colspecs"  select="colspec"/>
          <xsl:with-param name="spanspecs" select="spanspec"/>
          <xsl:with-param name="colsep"    select="$colsep"/>
          <xsl:with-param name="rowsep"    select="$rowsep"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db2html.row">
          <xsl:with-param name="row"       select="row[1]"/>
          <xsl:with-param name="colspecs"  select="$colspecs"/>
          <xsl:with-param name="spanspecs" select="$spanspecs"/>
          <xsl:with-param name="colsep"    select="$colsep"/>
          <xsl:with-param name="rowsep"    select="$rowsep"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:element>
</xsl:template>

<!-- = table/title = -->
<xsl:template match="table/title">
  <xsl:call-template name="db2html.block.title">
    <xsl:with-param name="node" select=".."/>
    <xsl:with-param name="title" select="."/>
  </xsl:call-template>
</xsl:template>

<!--
This template strips the p tag around single-paragraph table entries to avoid
introducing extra spacing.
-->
<xsl:template match="entry/para[
              not(preceding-sibling::* or following-sibling::*)]">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

</xsl:stylesheet>
