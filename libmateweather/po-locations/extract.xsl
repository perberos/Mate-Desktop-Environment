<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="text"/>

  <xsl:template name="escape">
    <xsl:param name="string" />
    <xsl:choose>
      <xsl:when test="contains($string, '&amp;')">
	<xsl:value-of select="substring-before($string, '&amp;')" />&amp;amp;<xsl:call-template name="escape"><xsl:with-param name="string" select="substring-after($string, '&amp;')" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="$string" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="output_name">
    <xsl:if test="preceding-sibling::comment()">
/* <xsl:value-of select="preceding-sibling::comment()"/> */</xsl:if>
<xsl:choose><xsl:when test="@msgctxt">
char *s = NC_("<xsl:call-template name="escape"><xsl:with-param name="string" select="@msgctxt"/></xsl:call-template>", "<xsl:call-template name="escape"><xsl:with-param name="string" select="."/></xsl:call-template>");</xsl:when><xsl:otherwise>
char *s = N_("<xsl:call-template name="escape"><xsl:with-param name="string" select="."/></xsl:call-template>");</xsl:otherwise></xsl:choose></xsl:template>

  <xsl:template match="mateweather">
      <!-- region names -->
      <xsl:for-each select="//region/_name">
	<xsl:sort select="."/>
	<xsl:call-template name="output_name" /></xsl:for-each>

      <!-- country names -->
      <xsl:for-each select="//country/_name">
	<xsl:sort select="."/>
	<xsl:call-template name="output_name" /></xsl:for-each>

      <!-- timezone names -->
      <xsl:for-each select="//timezone/_name">
	<xsl:call-template name="output_name" /></xsl:for-each>

      <!-- state names -->
      <xsl:for-each select="//state/_name">
	<xsl:sort select="ancestor::country/_name"/>
	<xsl:sort select="."/>
	<xsl:call-template name="output_name" /></xsl:for-each>

      <!-- city names -->
      <xsl:for-each select="//city/_name">
	<xsl:sort select="ancestor::country/_name"/>
	<xsl:sort select="."/>
	<xsl:call-template name="output_name" /></xsl:for-each>

      <xsl:text>
</xsl:text>
  </xsl:template>

</xsl:stylesheet>
