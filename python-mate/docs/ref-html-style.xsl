<?xml version='1.0'?> <!--*- mode: xml -*-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		version='1.0'>
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/chunk.xsl"/>
<xsl:include href="common.xsl"/>
<xsl:include href="html.xsl"/>
<xsl:include href="devhelp.xsl"/>

  <xsl:param name="gtkdoc.version" select="''"/>
  <xsl:param name="gtkdoc.bookname" select="''"/>

  <xsl:param name="refentry.generate.name" select="0"/>
  <xsl:param name="refentry.generate.title" select="1"/>
  <xsl:param name="chapter.autolabel" select="0"/>

  <xsl:template match="book|article">
    <xsl:apply-imports/>
    <xsl:call-template name="generate.devhelp"/>
    <xsl:call-template name="generate.index"/>
  </xsl:template>
 
</xsl:stylesheet>
