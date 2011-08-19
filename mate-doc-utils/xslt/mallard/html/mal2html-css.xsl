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
                xmlns:exsl="http://exslt.org/common"
                xmlns="http://www.w3.org/1999/xhtml"
                extension-element-prefixes="exsl"
                version="1.0">

<!--!!==========================================================================
Mallard to HTML - CSS

REMARK: Describe this module
-->


<!--@@==========================================================================
mal2html.css.file
The file to output CSS to

This parameter allows you to output the CSS to separate file which is referenced
by each HTML file.  If this parameter is blank, then the CSS is embedded inside
a #{style} tag in the HTML instead.
-->
<xsl:param name="mal2html.css.file" select="''"/>


<!--**==========================================================================
mal2html.css
Outputs the CSS that controls the appearance of the entire document
$css_file: Whether to create a CSS file when @{mal2html.css.file} is set.

This template outputs a #{style} or #{link} tag and calls *{mal2html.css.content}
to output the actual CSS directives.  An external CSS file will only be created
when ${css_file} is true.
-->
<xsl:template name="mal2html.css">
  <xsl:param name="css_file" select="false()"/>
  <xsl:choose>
    <xsl:when test="$mal2html.css.file != ''">
      <xsl:if test="$css_file">
        <exsl:document href="{$mal2html.css.file}" method="text">
          <xsl:call-template name="mal2html.css.content"/>
        </exsl:document>
      </xsl:if>
      <link rel="stylesheet" type="text/css" href="{$mal2html.css.file}"/>
    </xsl:when>
    <xsl:otherwise>
      <style type="text/css">
        <xsl:call-template name="mal2html.css.content"/>
      </style>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
mal2html.css.content
Outputs the actual CSS directives

This template is called by *{mal2html.css} to output CSS content.  It also calls
templates from other modules to output CSS specific to the content addressed in
those modules.

This template calls *{mal2html.css.custom} at the end.  That template may be used
by extension stylesheets to extend or override the CSS.
-->
<xsl:template name="mal2html.css.content">
  <xsl:variable name="direction">
    <xsl:call-template name="l10n.direction"/>
  </xsl:variable>
  <xsl:variable name="left">
    <xsl:call-template name="l10n.align.start">
      <xsl:with-param name="direction" select="$direction"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="right">
    <xsl:call-template name="l10n.align.end">
      <xsl:with-param name="direction" select="$direction"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:text>
html { height: 100%; }
body {
  margin: 0px;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.gray_background"/><xsl:text>;
  padding: 12px;
  min-height: 100%;
  direction: </xsl:text><xsl:value-of select="$direction"/><xsl:text>;
}
ul, ol, dl, dd { margin: 0; }
div, pre, p, li, dt { margin: 1em 0 0 0; padding: 0; }
.first-child { margin-top: 0; }
li.condensed { margin-top: 0.2em; }
a {
  text-decoration: none;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.link"/><xsl:text>;
}
a:visited {
  color: </xsl:text>
    <xsl:value-of select="$theme.color.link_visited"/><xsl:text>;
}
a:hover { text-decoration: underline; }

div.headbar {
  margin: 0;
  max-width: 48em;
}
div.footbar {
  margin: 0;
  max-width: 48em;
}
div.body {
  margin: 0;
  padding: 1em;
  max-width: 48em;
  min-height: 20em;
  -moz-border-radius: 6px;
  border: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.background"/><xsl:text>;
}
div.navbar {
  margin: 0;
  float: right;
}
a.navbar-prev::before {
  content: '</xsl:text><xsl:choose>
  <xsl:when test="$left = 'left'"><xsl:text>&#x25C0;&#x00A0;&#x00A0;</xsl:text></xsl:when>
  <xsl:otherwise><xsl:text>&#x25B6;&#x00A0;&#x00A0;</xsl:text></xsl:otherwise>
  </xsl:choose><xsl:text>';
  color: </xsl:text><xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
a.navbar-next::after {
  content: '</xsl:text><xsl:choose>
  <xsl:when test="$left = 'left'"><xsl:text>&#x00A0;&#x00A0;&#x25B6;</xsl:text></xsl:when>
  <xsl:otherwise><xsl:text>&#x00A0;&#x00A0;&#x25C0;</xsl:text></xsl:otherwise>
  </xsl:choose><xsl:text>';
  color: </xsl:text><xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
div.copyrights {
  text-align: center;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
div.section { margin-top: 2.4em; clear: both; }
div.section div.section {
  margin-top: 1.72em;
  margin-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1.72em;
}
div.section div.section div.section { margin-top: 1.44em; }
div.header {
  margin: 0;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
  border-bottom: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
}
h1 {
  font-size: 1.44em;
  margin: 0;
}
h2, h3, h4, h5, h6, h7 {
  font-size: 1.2em;
  margin: 0;
}
table { border-collapse: collapse; }

div.autolinks ul { margin: 0; padding: 0; }
div.autolinks div.title { margin: 1em 0 0 1em; }
div.autolinks div.title span {
  border-bottom: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
}
li.autolink { margin: 0.5em 0 0 0; padding: 0 0 0 1em; list-style-type: none; }

div.linktrails {
  margin: 0;
}
div.linktrail {
  font-size: 0.83em;
  margin: 0 1px 0.2em 1px;
  padding-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1.2em;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}

table.twocolumn { width: 100%; }
td.twocolumnleft { width: 48%; vertical-align: top; padding: 0; margin: 0; }
td.twocolumnright {
  width: 52%; vertical-align: top;
  margin: 0; padding: 0;
  padding-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1em;
}

div.linkdiv div.title {
  font-size: 1em;
  color: inherit;
}
div.linkdiv {
  margin: 0;
  padding: 0.5em;
  -moz-border-radius: 6px;
  border: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.background"/><xsl:text>;
}
div.linkdiv:hover {
  border-color: </xsl:text>
    <xsl:value-of select="$theme.color.blue_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.blue_background"/><xsl:text>;
}
div.linkdivsep {
  margin: 0.5em;
  list-style-type: none;
  max-width: 24em;
  border-bottom: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
}

div.title {
  margin: 0 0 0.2em 0;
  font-weight: bold;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
div.desc { margin: 0 0 0.2em 0; }
div.desc-listing, div.desc-synopsis { font-style: italic; }
div.desc-figure { margin: 0.2em 0 0 0; }
pre.code {
  /* FIXME: In RTL locales, we really want to align this left, but the watermark
   * we have is designed to fit in the top right corner.  Either we need a new
   * watermark, or we need a separate RTL version.
   */
  background: url('</xsl:text>
    <xsl:value-of select="$theme.watermark.code"/><xsl:text>') no-repeat top right;
  border: solid 2px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
  padding: 0.5em 1em 0.5em 1em;
}
div.example {
  border-</xsl:text><xsl:value-of select="$left"/><xsl:text>: solid 4px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
  padding-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1em;
}
div.figure {
  margin-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1.72em;
  padding: 4px;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
  border: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.gray_background"/><xsl:text>;
}
div.figure-contents {
  margin: 0;
  padding: 0.5em 1em 0.5em 1em;
  text-align: center;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text"/><xsl:text>;
  border: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.background"/><xsl:text>;
}
div.listing-contents { margin: 0; padding: 0; }
div.note {
  padding: 0.5em 6px 0.5em 6px;
  border-top: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.red_border"/><xsl:text>;
  border-bottom: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.red_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.yellow_background"/><xsl:text>;
}
div.note-inner {
  margin: 0;
  padding-</xsl:text><xsl:value-of select="$left"/><xsl:text>: </xsl:text>
    <xsl:value-of select="$theme.icon.admon.size + 12"/><xsl:text>px;
  background-position: </xsl:text><xsl:value-of select="$left"/><xsl:text> top;
  background-repeat: no-repeat;
  min-height: </xsl:text><xsl:value-of select="$theme.icon.admon.size"/><xsl:text>px;
  background-image: url("</xsl:text>
    <xsl:value-of select="$theme.icon.admon.note"/><xsl:text>");
}
div.note-advanced div.note-inner { <!-- background-image: url("</xsl:text>
  <xsl:value-of select="$theme.icon.admon.advanced"/><xsl:text>"); --> }
div.note-bug div.note-inner { background-image: url("</xsl:text>
  <xsl:value-of select="$theme.icon.admon.bug"/><xsl:text>"); }
div.note-important div.note-inner { background-image: url("</xsl:text>
  <xsl:value-of select="$theme.icon.admon.important"/><xsl:text>"); }
div.note-tip div.note-inner { background-image: url("</xsl:text>
  <xsl:value-of select="$theme.icon.admon.tip"/><xsl:text>"); }
div.note-warning div.note-inner { background-image: url("</xsl:text>
  <xsl:value-of select="$theme.icon.admon.warning"/><xsl:text>"); }
div.note-contents { margin: 0; padding: 0; }
div.quote-inner {
  margin: 0;
  background-image: url('</xsl:text>
    <xsl:value-of select="$theme.watermark.blockquote"/><xsl:text>');
  background-repeat: no-repeat;
  background-position: top </xsl:text><xsl:value-of select="$left"/><xsl:text>;
  padding: 0.5em;
  padding-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 4em;
}
div.title-quote {
  margin-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 4em;
}
blockquote { margin: 0; padding: 0; }
div.cite-comment {
  margin-top: 0.5em;
  color: </xsl:text><xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
div.cite-quote {
  margin-top: 0.5em;
  color: </xsl:text><xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
div.cite-quote::before {
  <!-- FIXME: i18n -->
  content: '&#x2015; ';
}
pre.screen {
  padding: 0.5em 1em 0.5em 1em;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.gray_background"/><xsl:text>;
  border: solid 2px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
}
div.synopsis-contents {
  margin: 0;
  padding: 0.5em 1em 0.5em 1em;
  border-top: solid 2px;
  border-bottom: solid 2px;
  border-color: </xsl:text>
    <xsl:value-of select="$theme.color.blue_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.gray_background"/><xsl:text>;
}
div.synopsis pre.code {
  background: none;
  border: none;
  padding: 0;
}

div.list-contents { margin: 0; padding: 0; }
div.title-list { margin-bottom: 0.5em; }
ol.list, ul.list { margin: 0; padding: 0; }
li.item-list { margin-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1.44em; }

div.steps-contents {
  margin: 0;
  padding: 0.5em 1em 0.5em 1em;
  border-top: solid 2px;
  border-bottom: solid 2px;
  border-color: </xsl:text>
    <xsl:value-of select="$theme.color.blue_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.yellow_background"/><xsl:text>;
}
div.steps-contents div.steps-contents {
  padding: 0;
  border: none;
  background-color: none;
}
ol.steps, ul.steps { margin: 0; padding: 0; }
li.item-steps { margin-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1.44em; }

div.terms-contents { margin: 0; }
dt.item-next { margin-top: 0; }
dd.item-terms {
  margin-top: 0.2em;
  margin-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1.44em;
}

ul.tree {
  margin: 0; padding: 0;
  list-style-type: none;
}
li.item-tree { margin: 0; padding: 0; }
div.item-tree { margin: 0; padding: 0; }
ul.tree ul.tree { margin-</xsl:text><xsl:value-of select="$left"/><xsl:text>: 1.44em; }
div.tree-lines ul.tree { margin-left: 0; }

table.table {
  border-collapse: collapse;
  border-color: #555753;
  border-width: 1px;
}
table.table td {
  padding: 0.1em 0.5em 0.1em 0.5em;
  border-color: #888a85;
  border-width: 1px;
  vertical-align: top;
}

span.app { font-style: italic; }
span.cmd {
  font-family: monospace;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.gray_background"/><xsl:text>;
  padding: 0 0.2em 0 0.2em;
}
span.code { font-family: monospace; }
span.em { font-style: italic; }
span.email { color: red; }
span.file { font-family: monospace; }
span.gui, span.guiseq { color: </xsl:text>
  <xsl:value-of select="$theme.color.text_light"/><xsl:text>; }
span.input { font-family: monospace; }
span.hi {
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.yellow_background"/><xsl:text>;
}
span.key {
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
  border: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
  padding: 0 0.2em 0 0.2em;
}
span.keyseq {
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
span.output { font-family: monospace; }
pre.screen span.output {
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
pre.screen span.output-error {
<!-- FIXME: theme -->
  color: #ff0000;
}
pre.screen span.output-prompt { font-weight: bold; }
span.sys { font-family: monospace; }
span.var { font-style: italic; }
</xsl:text>
<xsl:call-template name="mal2html.css.editor"/>
<xsl:call-template name="mal2html.css.custom"/>
</xsl:template>
<!--
2.4
2
1.72
1.44
1.2
1
0.83
0.69
0.5
-->


<!--**==========================================================================
mal2html.css.editor
Outputs CSS for editor mode

FIXME
-->
<xsl:template name="mal2html.css.editor">
  <xsl:variable name="direction">
    <xsl:call-template name="l10n.direction"/>
  </xsl:variable>
  <xsl:variable name="left">
    <xsl:call-template name="l10n.align.start">
      <xsl:with-param name="direction" select="$direction"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="right">
    <xsl:call-template name="l10n.align.end">
      <xsl:with-param name="direction" select="$direction"/>
    </xsl:call-template>
  </xsl:variable>
<xsl:text>
div.version {
  position: absolute;
  </xsl:text><xsl:value-of select="$right"/><xsl:text>: 12px;
  opacity: 0.2;
  margin-top: -1em;
  padding: 0.5em 1em 0.5em 1em;
  max-width: 24em;
  -moz-border-radius: 6px;
  border: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.gray_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.yellow_background"/><xsl:text>;
}
div.version:hover { opacity: 0.8; }
div.version p.version { margin-top: 0.2em; }
div.linkdiv div.title span.status {
  font-size: 0.83em;
  font-weight: normal;
  padding-left: 0.2em;
  padding-right: 0.2em;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
  border: solid 1px </xsl:text>
    <xsl:value-of select="$theme.color.red_border"/><xsl:text>;
}
div.linkdiv div.title span.status-stub { background-color: </xsl:text>
  <xsl:value-of select="$theme.color.red_background"/><xsl:text>; }
div.linkdiv div.title span.status-draft { background-color: </xsl:text>
  <xsl:value-of select="$theme.color.red_background"/><xsl:text>; }
div.linkdiv div.title span.status-incomplete { background-color: </xsl:text>
  <xsl:value-of select="$theme.color.red_background"/><xsl:text>; }
div.linkdiv div.title span.status-review { background-color: </xsl:text>
  <xsl:value-of select="$theme.color.yellow_background"/><xsl:text>; }
div.linkdiv div.desc {
  margin-top: 0.2em;
  color: </xsl:text>
    <xsl:value-of select="$theme.color.text_light"/><xsl:text>;
}
div.comment {
  padding: 0.5em;
  border: solid 2px </xsl:text>
    <xsl:value-of select="$theme.color.red_border"/><xsl:text>;
  background-color: </xsl:text>
    <xsl:value-of select="$theme.color.red_background"/><xsl:text>;
}
div.comment div.comment {
  margin: 1em 1em 0 1em;
}
div.comment div.cite {
  margin: 0 0 0.5em 0;
  font-style: italic;
}
</xsl:text>
</xsl:template>


<!--**==========================================================================
mal2html.css.custom
Allows extension stylesheets to extend or override CSS
:Stub: true

This stub template has no content.  Extension stylesheets can override this
template to output extra CSS.
-->
<xsl:template name="mal2html.css.custom"/>

</xsl:stylesheet>
