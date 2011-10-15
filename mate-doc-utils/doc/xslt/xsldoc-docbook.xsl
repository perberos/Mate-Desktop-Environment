<?xml version='1.0' encoding='UTF-8'?><!-- -*- indent-tabs-mode: nil -*- -->
<!--
xsldoc-docbook.xsl - Convert the output of xsldoc.awk to standard DocBook
Copyright (C) 2006 Shaun McCance <shaunm@gnome.org>

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
-->
<!--
This program is free software, but that doesn't mean you should use it.
It's a hackish bit of awk and XSLT to do inline XSLT documentation with
a simple syntax in comments.  I had originally tried to make a public
inline documentation system for XSLT using embedded XML, but it just got
very cumbersome.  XSLT should have been designed from the ground up with
an inline documentation format.

None of the existing inline documentation tools (gtk-doc, doxygen, etc.)
really work well for XSLT, so I rolled my own simple comment-based tool.
This tool is sufficient for producing the documentation I need, but I
just don't have the time or inclination to make a robust system suitable
for public use.

You are, of course, free to use any of this.  If you do proliferate this
hack, it is requested (though not required, that would be non-free) that
you atone for your actions.  A good atonement would be contributing to
free software.
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:param name="basename"/>
<xsl:param name="xsl_file"/>
<xsl:variable name="xsl" select="document($xsl_file)/xsl:stylesheet"/>

<xsl:template match="/">
  <xsl:variable name="sect" select="section"/>

  <!-- Stylesheet params -->
  <xsl:for-each select="$xsl/xsl:param">
    <xsl:if test="not($sect/param[name = current()/@name])">
      <xsl:message terminate="yes">
        <xsl:value-of select="$basename"/>
        <xsl:text>: No documentation for stylesheet param </xsl:text>
        <xsl:value-of select="@name"/>
      </xsl:message>
    </xsl:if>
  </xsl:for-each>
  <xsl:for-each select="$sect/param[not(@private)]">
    <xsl:if test="not($xsl/xsl:param[@name = current()/name])">
      <xsl:message terminate="yes">
        <xsl:value-of select="$basename"/>
        <xsl:text>: No stylesheet param for documented </xsl:text>
        <xsl:value-of select="name"/>
      </xsl:message>
    </xsl:if>
  </xsl:for-each>

  <!-- Named templates -->
  <xsl:for-each select="$xsl/xsl:template[@name]">
    <xsl:if test="not($sect/template[name = current()/@name])">
      <xsl:message terminate="yes">
        <xsl:value-of select="$basename"/>
        <xsl:text>: No documentation for named template </xsl:text>
        <xsl:value-of select="@name"/>
      </xsl:message>
    </xsl:if>
  </xsl:for-each>
  <xsl:for-each select="$sect/template[not(@private)]">
    <xsl:variable name="this" select="."/>
    <xsl:variable name="tmpl" select="$xsl/xsl:template[@name = $this/name]"/>
    <xsl:if test="not($tmpl)">
      <xsl:message terminate="yes">
        <xsl:value-of select="$basename"/>
        <xsl:text>: No named template for documented </xsl:text>
        <xsl:value-of select="$this/name"/>
      </xsl:message>
    </xsl:if>
    <xsl:for-each select="$this/params/param">
      <xsl:if test="not($tmpl/xsl:param[@name = current()/name])">
        <xsl:message terminate="yes">
          <xsl:value-of select="$basename"/>
          <xsl:text>: In named template </xsl:text>
          <xsl:value-of select="$this/name"/>
          <xsl:text>, no stylesheet param for documented </xsl:text>
          <xsl:value-of select="current()/name"/>
        </xsl:message>
      </xsl:if>
    </xsl:for-each>
<!--
    <xsl:for-each select="$tmpl/xsl:param">
      <xsl:if test="not($this/params/param[name = current()/@name])">
        <xsl:message terminate="yes">
          <xsl:value-of select="$basename"/>
          <xsl:text>: In named template </xsl:text>
          <xsl:value-of select="$this/name"/>
          <xsl:text>, no documentation for stylesheet param </xsl:text>
          <xsl:value-of select="current()/@name"/>
        </xsl:message>
      </xsl:if>
    </xsl:for-each>
-->
  </xsl:for-each>

  <!-- Template modes -->
  <xsl:for-each select="$xsl/xsl:template[@mode]">
    <xsl:if test="not($sect/mode[name = current()/@mode])">
      <xsl:message terminate="yes">
        <xsl:value-of select="$basename"/>
        <xsl:text>: No documentation for template mode </xsl:text>
        <xsl:value-of select="@mode"/>
      </xsl:message>
    </xsl:if>
  </xsl:for-each>
  <xsl:for-each select="$xsl//xsl:call-template[@mode]">
    <xsl:if test="not($sect/mode[name = current()/@mode])">
      <xsl:message terminate="yes">
        <xsl:value-of select="$basename"/>
        <xsl:text>: No documentation for template mode </xsl:text>
        <xsl:value-of select="@mode"/>
      </xsl:message>
    </xsl:if>
  </xsl:for-each>
  <xsl:for-each select="$sect/mode[not(@private)]">
    <xsl:variable name="this" select="."/>
    <xsl:variable name="tmpls" select="$xsl/xsl:template[@mode = $this/name]"/>
    <xsl:variable name="calls" select="$xsl//xsl:call-template[@mode = $this/name]"/>
    <xsl:if test="not($tmpls or $calls)">
      <xsl:message terminate="yes">
        <xsl:value-of select="$basename"/>
        <xsl:text>: No template mode for documented </xsl:text>
        <xsl:value-of select="name"/>
      </xsl:message>
    </xsl:if>
    <xsl:for-each select="$tmpls/xsl:param">
      <xsl:if test="not($this/params/param[name = current()/@name])">
        <xsl:message terminate="yes">
          <xsl:value-of select="$basename"/>
          <xsl:text>: In template mode </xsl:text>
          <xsl:value-of select="$this/name"/>
          <xsl:text>, no documentation for stylesheet param </xsl:text>
          <xsl:value-of select="current()/@name"/>
        </xsl:message>
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="$calls/xsl:with-param">
      <xsl:if test="not($this/params/param[name = current()/@name])">
        <xsl:message terminate="yes">
          <xsl:value-of select="$basename"/>
          <xsl:text>: In template mode </xsl:text>
          <xsl:value-of select="$this/name"/>
          <xsl:text>, no documentation for passed param </xsl:text>
          <xsl:value-of select="current()/@name"/>
        </xsl:message>
      </xsl:if>
    </xsl:for-each>
  </xsl:for-each>

  <!-- FIXME: for modes and nameds, add a template param check -->

  <xsl:apply-templates select="$sect"/>
</xsl:template>

<xsl:template match="/section">
  <xsl:variable name="params" select="param[not(@private)]"/>
  <xsl:variable name="modes" select="mode[not(@private)]"/>
  <xsl:variable name="templates" select="template[not(@private)]"/>
  <section id="S__{$basename}">
    <title><xsl:copy-of select="name/node()"/></title>
    <xsl:choose>
      <xsl:when test="body/*">
        <xsl:copy-of select="body/node()"/>
      </xsl:when>
      <xsl:otherwise>
        <remark>This needs documentation</remark>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$params">
      <variablelist>
        <title>Parameters</title>
        <xsl:for-each select="$params">
          <varlistentry>
            <term><link linkend="P__{translate(name, '.', '_')}">
              <parameter><xsl:copy-of select="name/node()"/></parameter>
            </link></term>
            <listitem><para>
              <xsl:copy-of select="desc/node()"/>
            </para></listitem>
          </varlistentry>
        </xsl:for-each>
      </variablelist>
    </xsl:if>
    <xsl:if test="$modes">
      <variablelist>
        <title>Modes</title>
        <xsl:for-each select="$modes">
          <varlistentry>
            <term><link linkend="M__{translate(name, '.', '_')}">
              <function><xsl:copy-of select="name/node()"/></function>
            </link></term>
            <listitem><para>
              <xsl:copy-of select="desc/node()"/>
            </para></listitem>
          </varlistentry>
        </xsl:for-each>
      </variablelist>
    </xsl:if>
    <xsl:if test="$templates">
      <variablelist>
        <title>Templates</title>
        <xsl:for-each select="$templates">
          <varlistentry>
            <term><link linkend="T__{translate(name, '.', '_')}">
              <function><xsl:copy-of select="name/node()"/></function>
            </link></term>
            <listitem><para>
              <xsl:copy-of select="desc/node()"/>
            </para></listitem>
          </varlistentry>
        </xsl:for-each>
      </variablelist>
    </xsl:if>
    <xsl:apply-templates select="$params"/>
    <xsl:apply-templates select="$modes"/>
    <xsl:apply-templates select="$templates"/>
  </section>
</xsl:template>

<xsl:template match="param">
  <xsl:variable name="id" select="translate(name, '.', '_')"/>
  <refentry id="P__{$id}">
    <indexterm><primary>
      <xsl:value-of select="name"/>
    </primary></indexterm>
    <refnamediv>
      <refname><parameter>
        <xsl:copy-of select="name/node()"/>
      </parameter></refname>
      <refpurpose>
        <xsl:copy-of select="desc/node()"/>
      </refpurpose>
    </refnamediv>
    <refsection>
      <title>Description</title>
      <xsl:choose>
        <xsl:when test="body/*">
          <xsl:copy-of select="body/node()"/>
        </xsl:when>
        <xsl:otherwise>
          <remark>This needs documentation</remark>
        </xsl:otherwise>
      </xsl:choose>
    </refsection>
  </refentry>
</xsl:template>

<xsl:template match="template | mode">
  <xsl:variable name="name" select="translate(name, '.', '_')"/>
  <xsl:variable name="id">
    <xsl:choose>
      <xsl:when test="self::template">
        <xsl:value-of select="concat('T__', $name)"/>
      </xsl:when>
      <xsl:when test="self::mode">
        <xsl:value-of select="concat('M__', $name)"/>
      </xsl:when>
    </xsl:choose>
  </xsl:variable>
  <refentry id="{$id}">
    <indexterm><primary>
      <xsl:value-of select="name"/>
    </primary></indexterm>
    <refnamediv>
      <refname><function>
        <xsl:copy-of select="name/node()"/>
      </function></refname>
      <refpurpose>
        <xsl:copy-of select="desc/node()"/>
      </refpurpose>
    </refnamediv>
    <xsl:if test="params">
      <refsection>
        <title>Parameters</title>
        <variablelist>
          <xsl:for-each select="params/param">
            <varlistentry>
              <term><parameter>
                <xsl:copy-of select="name/node()"/>
              </parameter></term>
              <listitem><para>
                <xsl:copy-of select="desc/node()"/>
              </para></listitem>
            </varlistentry>
          </xsl:for-each>
        </variablelist>
      </refsection>
    </xsl:if>
    <refsection>
      <title>Description</title>
      <xsl:choose>
        <xsl:when test="body/*">
          <xsl:copy-of select="body/node()"/>
        </xsl:when>
        <xsl:otherwise>
          <remark>This needs documentation</remark>
        </xsl:otherwise>
      </xsl:choose>
    </refsection>
  </refentry>
</xsl:template>

<xsl:template match="*">
  <xsl:copy-of select="."/>
</xsl:template>

</xsl:stylesheet>
