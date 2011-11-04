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
                xmlns="http://www.w3.org/1999/xhtml"
                version="1.0">

<!--!!==========================================================================
DocBook to HTML - Inline Elements
:Requires: db-common db2html-xref gettext

REMARK: Describe this module
-->
<!--#% l10n.format.mode -->


<!--**==========================================================================
db2html.inline
Renders an inline element as an HTML #{span} element
$node: The element to render
$children: The child elements to process
$class: The value of the #{class} attribute on the #{span} tag
$bold: Whether to render the element in bold face
$italic: Whether to render the element in italics
$underline: Whether to underline the element
$mono: Whether to render the element in a monospace font
$sans: Whether to render the element in a sans-serif font
$lang: The locale of the text in ${node}
$dir: The text direction, either #{ltr} or #{rtl}
$ltr: Whether to default to #{ltr} if neither ${lang} nor ${dir} is specified

REMARK: Document this template
-->
<xsl:template name="db2html.inline">
  <xsl:param name="node" select="."/>
  <xsl:param name="children" select="false()"/>
  <xsl:param name="class" select="local-name($node)"/>
  <xsl:param name="bold" select="false()"/>
  <xsl:param name="italic" select="false()"/>
  <xsl:param name="underline" select="false()"/>
  <xsl:param name="mono" select="false()"/>
  <xsl:param name="sans" select="false()"/>
  <xsl:param name="lang" select="$node/@lang"/>
  <xsl:param name="dir" select="false()"/>
  <xsl:param name="ltr" select="false()"/>

  <!-- FIXME: do CSS classes, rather than inline styles -->
  <span class="{$class}">
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
      <xsl:when test="$ltr">
        <xsl:attribute name="dir">
          <xsl:text>ltr</xsl:text>
        </xsl:attribute>
      </xsl:when>
    </xsl:choose>
    <xsl:if test="$bold or $italic or $mono or $underline or $sans">
      <xsl:variable name="style">
        <xsl:if test="$bold">
          <xsl:text>font-weight: bold; </xsl:text>
        </xsl:if>
        <xsl:if test="$italic">
          <xsl:text>font-style: italic; </xsl:text>
        </xsl:if>
        <xsl:if test="$underline">
          <xsl:text>text-decoration: underline; </xsl:text>
        </xsl:if>
        <xsl:choose>
          <xsl:when test="$mono">
            <xsl:text>font-family: monospace; </xsl:text>
          </xsl:when>
          <xsl:when test="$sans">
            <xsl:text>font-family: sans-serif; </xsl:text>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>
      <xsl:attribute name="style">
        <xsl:value-of select="$style"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="db2html.anchor">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
    <xsl:choose>
      <xsl:when test="$children">
        <xsl:apply-templates select="$children"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates mode="db2html.inline.content.mode" select="$node"/>
      </xsl:otherwise>
    </xsl:choose>
  </span>
</xsl:template>


<!--%%===========================================================================
db2html.inline.content.mode
FIXME

FIXME
-->
<xsl:template mode="db2html.inline.content.mode" match="*">
  <xsl:apply-templates/>
</xsl:template>


<!-- == Matched Templates == -->

<!-- = abbrev = -->
<xsl:template match="abbrev">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = accel = -->
<xsl:template match="accel">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = acronym = -->
<xsl:template match="acronym">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = action = -->
<xsl:template match="action">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = artpagenums = -->
<xsl:template match="artpagenums">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = application = -->
<xsl:template match="application">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = authorinitials = -->
<xsl:template match="authorinitials">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = bibkey-abbrev = -->
<xsl:key name="bibkey-abbrev"
         match="biblioentry[@id and *[1]/self::abbrev] |
                bibliomixed[@id and *[1]/self::abbrev] "
         use="string(*[1])"/>

<!-- = bibkey-label = -->
<xsl:key name="bibkey-label"
         match="biblioentry[@id and @xreflabel] |
                bibliomixed[@id and @xreflabel] "
         use="string(@xreflabel)"/>

<!-- = bibkey-id = -->
<xsl:key name="bibkey-id"
         match="biblioentry[@id] | bibliomixed[@id]"
         use="string(@id)"/>

<!-- = citation = -->
<xsl:template match="citation">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = citation % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="citation">
  <xsl:call-template name="l10n.gettext">
    <xsl:with-param name="msgid" select="'citation.format'"/>
    <xsl:with-param name="node" select="."/>
    <xsl:with-param name="format" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = citation % l10n.format.mode = -->
<xsl:template mode="l10n.format.mode" match="msg:citation">
  <xsl:param name="node"/>
  <xsl:for-each select="$node[1]">
    <xsl:variable name="entry_abbrev" select="key('bibkey-abbrev', string($node))"/>
    <xsl:choose>
      <xsl:when test="$entry_abbrev">
        <xsl:call-template name="db2html.xref">
          <xsl:with-param name="linkend" select="$entry_abbrev/@id"/>
          <xsl:with-param name="target" select="$entry_abbrev"/>
          <xsl:with-param name="content">
            <xsl:apply-templates select="node()"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="entry_label" select="key('bibkey-label', string($node))"/>
        <xsl:choose>
          <xsl:when test="$entry_label">
            <xsl:call-template name="db2html.xref">
              <xsl:with-param name="linkend" select="$entry_label/@id"/>
              <xsl:with-param name="target" select="$entry_label"/>
              <xsl:with-param name="content">
                <xsl:apply-templates select="node()"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:variable name="entry_id" select="key('bibkey-id', string($node))"/>
            <xsl:choose>
              <xsl:when test="$entry_id">
                <xsl:call-template name="db2html.xref">
                  <xsl:with-param name="linkend" select="$entry_id/@id"/>
                  <xsl:with-param name="target" select="$entry_id"/>
                  <xsl:with-param name="content">
                    <xsl:apply-templates select="node()"/>
                  </xsl:with-param>
                </xsl:call-template>
              </xsl:when>
              <xsl:otherwise>
                <xsl:apply-templates select="node()"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<!-- = citetitle = -->
<xsl:template match="citetitle">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = citetitle % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="citetitle">
  <xsl:call-template name="l10n.gettext">
    <xsl:with-param name="msgid" select="'citetitle.format'"/>
    <xsl:with-param name="role" select="@pubwork"/>
    <xsl:with-param name="node" select="."/>
    <xsl:with-param name="format" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = city = -->
<xsl:template match="city">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = classname = -->
<xsl:template match="classname">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = code = -->
<xsl:template match="code">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = command = -->
<xsl:template match="command">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = computeroutput = -->
<xsl:template match="computeroutput">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = constant = -->
<xsl:template match="constant">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = corpauthor = -->
<xsl:template match="corpauthor">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = corpcredit = -->
<xsl:template match="corpcredit">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = country = -->
<xsl:template match="country">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = database = -->
<xsl:template match="database">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = date = -->
<xsl:template match="date">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = edition = -->
<xsl:template match="edition">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = email = -->
<xsl:template match="email">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = email % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="email">
  <xsl:text>&lt;</xsl:text>
  <a>
    <xsl:attribute name="href">
      <xsl:text>mailto:</xsl:text>
      <xsl:value-of select="string(.)"/>
    </xsl:attribute>
    <xsl:attribute name="title">
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'email.tooltip'"/>
        <xsl:with-param name="node" select="."/>
        <xsl:with-param name="string" select="string(.)"/>
        <xsl:with-param name="format" select="true()"/>
      </xsl:call-template>
    </xsl:attribute>
    <xsl:apply-templates/>
  </a>
  <xsl:text>&gt;</xsl:text>
</xsl:template>

<!-- = emphasis = -->
<xsl:template match="emphasis">
  <xsl:variable name="bold" select="@role = 'bold' or @role = 'strong'"/>
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="class">
      <xsl:text>emphasis</xsl:text>
      <xsl:if test="$bold">
        <xsl:text> emphasis-bold</xsl:text>
      </xsl:if>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<!-- = envar = -->
<xsl:template match="envar">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = errorcode = -->
<xsl:template match="errorcode">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = errorname = -->
<xsl:template match="errorname">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = errortext = -->
<xsl:template match="errortext">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = errortype = -->
<xsl:template match="errortype">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = exceptionname = -->
<xsl:template match="exceptionname">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = fax = -->
<xsl:template match="fax">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = filename = -->
<xsl:template match="filename">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = firstname = -->
<xsl:template match="firstname">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = firstterm = -->
<xsl:template match="firstterm">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = firstterm % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="firstterm">
  <xsl:choose>
    <xsl:when test="@linkend">
      <xsl:call-template name="db2html.xref">
        <xsl:with-param name="linkend" select="@linkend"/>
        <xsl:with-param name="content">
          <xsl:apply-templates/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = foreignphrase = -->
<xsl:template match="foreignphrase">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = function = -->
<xsl:template match="function">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = glosskey = -->
<xsl:key name="glosskey" match="glossentry[@id]" use="string(glossterm)"/>

<!-- = glossterm = -->
<xsl:template match="glossterm">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = glossterm % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="glossterm">
  <xsl:choose>
    <xsl:when test="@linkend">
      <xsl:call-template name="db2html.xref">
        <xsl:with-param name="linkend" select="@linkend"/>
        <xsl:with-param name="content">
          <xsl:apply-templates/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="not(../self::glossentry)">
      <xsl:variable name="glossentry" select="key('glosskey', string(.))"/>
      <xsl:choose>
        <xsl:when test="$glossentry">
          <xsl:call-template name="db2html.xref">
            <xsl:with-param name="linkend" select="$glossentry/@id"/>
            <xsl:with-param name="target" select="$glossentry"/>
            <xsl:with-param name="content">
              <xsl:apply-templates/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = guibutton = -->
<xsl:template match="guibutton">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = guiicon = -->
<xsl:template match="guiicon">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = guilabel = -->
<xsl:template match="guilabel">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = guimenu = -->
<xsl:template match="guimenu">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = guimenuitem = -->
<xsl:template match="guimenuitem">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = guisubmenu = -->
<xsl:template match="guisubmenu">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = hardware = -->
<xsl:template match="hardware">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = holder = -->
<xsl:template match="holder">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = honorific = -->
<xsl:template match="honorific">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = inlineequation = -->
<xsl:template match="inlineequation">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = interface = -->
<xsl:template match="interface">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = interfacename = -->
<xsl:template match="interfacename">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = isbn = -->
<xsl:template match="isbn">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = issn = -->
<xsl:template match="issn">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = issuenum = -->
<xsl:template match="issuenum">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = jobtitle = -->
<xsl:template match="jobtitle">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = keycap = -->
<xsl:template match="keycap">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = keycode = -->
<xsl:template match="keycode">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = keycombo = -->
<xsl:template match="keycombo">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = keycombo % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="keycombo">
  <xsl:variable name="joinchar">
    <xsl:choose>
      <xsl:when test="@action = 'seq'"><xsl:text> </xsl:text></xsl:when>
      <xsl:when test="@action = 'simul'">+</xsl:when>
      <xsl:when test="@action = 'press'">-</xsl:when>
      <xsl:when test="@action = 'click'">-</xsl:when>
      <xsl:when test="@action = 'double-click'">-</xsl:when>
      <xsl:when test="@action = 'other'">+</xsl:when>
      <xsl:otherwise>+</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:for-each select="*">
    <xsl:if test="position() != 1">
      <xsl:value-of select="$joinchar"/>
    </xsl:if>
    <xsl:apply-templates select="."/>
  </xsl:for-each>
</xsl:template>

<!-- = keysym = -->
<xsl:template match="keysym">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = lineage = -->
<xsl:template match="lineage">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = lineannotation = -->
<xsl:template match="lineannotation">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = literal = -->
<xsl:template match="literal">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = markup = -->
<xsl:template match="markup">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = mathphrase = -->
<xsl:template match="mathphrase">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = medialabel = -->
<xsl:template match="medialabel">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = menuchoice = -->
<xsl:template match="menuchoice">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = menuchoice % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="menuchoice">
  <xsl:variable name="arrow">
    <xsl:variable name="ltr">
      <xsl:call-template name="l10n.gettext">
        <xsl:with-param name="msgid" select="'default:LTR'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$ltr = 'default:RTL'">
        <xsl:text>&#x25C2;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&#x25B8;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:for-each select="*[not(self::shortcut)]">
    <xsl:if test="position() != 1">
      <xsl:value-of select="concat('&#x00A0;', $arrow, ' ')"/>
    </xsl:if>
    <xsl:apply-templates select="."/>
  </xsl:for-each>
  <xsl:if test="shortcut">
    <xsl:text> </xsl:text>
    <xsl:apply-templates select="shortcut"/>
  </xsl:if>
</xsl:template>

<!-- = methodname = -->
<xsl:template match="methodname">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = mousebutton = -->
<xsl:template match="mousebutton">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = option = -->
<xsl:template match="option">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = optional = -->
<xsl:template match="optional">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = optional % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="optional">
  <xsl:text>[</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>]</xsl:text>
</xsl:template>

<!-- = orgdiv = -->
<xsl:template match="orgdiv">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = orgname = -->
<xsl:template match="orgname">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = othername = -->
<xsl:template match="othername">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = pagenums = -->
<xsl:template match="pagenums">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = parameter = -->
<xsl:template match="parameter">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = personname = -->
<xsl:template match="personname">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = personname % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="personname">
  <xsl:call-template name="db.personname"/>
</xsl:template>

<!-- = phone = -->
<xsl:template match="phone">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = phrase = -->
<xsl:template match="phrase">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = pob = -->
<xsl:template match="pob">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = postcode = -->
<xsl:template match="postcode">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = productname = -->
<xsl:template match="productname">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = productname % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="productname">
  <xsl:apply-templates/>
  <xsl:choose>
    <xsl:when test="@class = 'copyright'">&#x00A9;</xsl:when>
    <xsl:when test="@class = 'registered'">&#x00AE;</xsl:when>
    <xsl:when test="@class = 'trade'">&#x2122;</xsl:when>
    <xsl:when test="@class = 'service'">&#x2120;</xsl:when>
  </xsl:choose>
</xsl:template>

<!-- = productnumber = -->
<xsl:template match="productnumber">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = prompt = -->
<xsl:template match="prompt">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = property = -->
<xsl:template match="property">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = pubdate = -->
<xsl:template match="pubdate">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = publishername = -->
<xsl:template match="publishername">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = ooclass = -->
<xsl:template match="ooclass">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = ooclass % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="ooclass">
  <xsl:for-each select="*">
    <xsl:if test="position() != 1">
      <xsl:text> </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="."/>
  </xsl:for-each>
</xsl:template>

<!-- = ooexception = -->
<xsl:template match="ooexception">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = ooexception % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="ooexception">
  <xsl:for-each select="*">
    <xsl:if test="position() != 1">
      <xsl:text> </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="."/>
  </xsl:for-each>
</xsl:template>

<!-- = oointerface = -->
<xsl:template match="oointerface">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = oointerface % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="oointerface">
  <xsl:for-each select="*">
    <xsl:if test="position() != 1">
      <xsl:text> </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="."/>
  </xsl:for-each>
</xsl:template>

<!-- = quote = -->
<xsl:template match="quote">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = quote % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="quote">
  <xsl:call-template name="l10n.gettext">
    <xsl:with-param name="msgid" select="'quote.format'"/>
    <xsl:with-param name="role">
      <xsl:choose>
        <xsl:when test="(count(ancestor::quote) mod 2) = 0">
          <xsl:text>outer</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>inner</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:with-param>
    <xsl:with-param name="node" select="."/>
    <xsl:with-param name="format" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = replaceable = -->
<xsl:template match="replaceable">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = returnvalue = -->
<xsl:template match="returnvalue">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = sgmltag = -->
<xsl:template match="sgmltag">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="class">
      <xsl:text>sgmltag</xsl:text>
      <xsl:if test="@class">
        <xsl:value-of select="concat(' sgmltag-', @class)"/>
      </xsl:if>
    </xsl:with-param>
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = sgmltag % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="sgmltag">
  <xsl:choose>
    <xsl:when test="@class = 'attribute'">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:when test="@class = 'attvalue'">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:when test="@class = 'element'">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:when test="@class = 'emptytag'">
      <xsl:text>&lt;</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>/&gt;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'endtag'">
      <xsl:text>&lt;/</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>&gt;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'genentity'">
      <xsl:text>&amp;</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'localname'">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:when test="@class = 'namespace'">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:when test="@class = 'numcharref'">
      <xsl:text>&amp;#</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'paramentity'">
      <xsl:text>%</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'pi'">
      <xsl:text>&lt;?</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>&gt;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'prefix'">
      <xsl:apply-templates/>
      <xsl:text>:</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'sgmlcomment'">
      <xsl:text>&lt;!--</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>--&gt;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'starttag'">
      <xsl:text>&lt;</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>&gt;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'xmlpi'">
      <xsl:text>&lt;?</xsl:text>
      <xsl:apply-templates/>
      <xsl:text>?&gt;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = shortcut = -->
<xsl:template match="shortcut">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = shortcut % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="shortcut">
  <xsl:text>(</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>)</xsl:text>
</xsl:template>

<!-- = state = -->
<xsl:template match="state">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = street = -->
<xsl:template match="street">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = structfield = -->
<xsl:template match="structfield">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = structname = -->
<xsl:template match="structname">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = subscript = -->
<xsl:template match="subscript">
  <sub class="subscript">
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates/>
  </sub>
</xsl:template>

<!-- = superscript = -->
<xsl:template match="superscript">
  <sup class="superscript">
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates/>
  </sup>
</xsl:template>

<!-- = surname = -->
<xsl:template match="surname">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = symbol = -->
<xsl:template match="symbol">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = systemitem = -->
<xsl:template match="systemitem">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = token = -->
<xsl:template match="token">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = trademark = -->
<xsl:template match="trademark">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = trademark % db2html.inline.content.mode = -->
<xsl:template mode="db2html.inline.content.mode" match="trademark">
  <xsl:apply-templates/>
  <xsl:choose>
    <xsl:when test="@class = 'copyright'">&#x00A9;</xsl:when>
    <xsl:when test="@class = 'registered'">&#x00AE;</xsl:when>
    <xsl:when test="@class = 'service'">&#x2120;</xsl:when>
    <xsl:otherwise>&#x2122;</xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- = type = -->
<xsl:template match="type">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = uri = -->
<xsl:template match="uri">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = userinput = -->
<xsl:template match="userinput">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = varname = -->
<xsl:template match="varname">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="ltr" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!-- = volumenum = -->
<xsl:template match="volumenum">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = wordasword = -->
<xsl:template match="wordasword">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>

<!-- = year = -->
<xsl:template match="year">
  <xsl:call-template name="db2html.inline"/>
</xsl:template>


</xsl:stylesheet>
