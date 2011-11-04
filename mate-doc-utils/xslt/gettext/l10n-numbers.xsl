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
                xmlns:math="http://exslt.org/math"
                extension-element-prefixes="math"
                version="1.0">

<!--!!==========================================================================
Localized Numbers
-->

<xsl:include href="gettext.xsl"/>


<!--**==========================================================================
l10n.number
Formats a number according to a localized numbering system
$value: The numeric value of the number to format
$format: The numbering system to use

REMARK: Talk about numbering systems
-->
<xsl:template name="l10n.number">
  <xsl:param name="value"/>
  <xsl:param name="format"/>
  <xsl:choose>
    <xsl:when test="$format='decimal' or $format='1'">
      <xsl:number format="1" value="$value"/>
    </xsl:when>
    <xsl:when test="$format='alpha-lower' or $format='a'">
      <xsl:number format="a" value="$value"/>
    </xsl:when>
    <xsl:when test="$format='alpha-upper' or $format='A'">
      <xsl:number format="A" value="$value"/>
    </xsl:when>
    <xsl:when test="$format='roman-lower' or $format='i'">
      <xsl:number format="i" value="$value"/>
    </xsl:when>
    <xsl:when test="$format='roman-upper' or $format='I'">
      <xsl:number format="I" value="$value"/>
    </xsl:when>

    <xsl:when test="$format='cjk-japanese'     or
                    $format='cjk-chinese-simp' or
                    $format='cjk-chinese-trad' ">
      <xsl:call-template name="l10n.number.cjk-ideographic">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="format" select="$format"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:when test="$format='ionic-lower' or $format='ionic-upper'">
      <xsl:call-template name="l10n.number.ionic">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="format" select="$format"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:when test="$format='decimal-arabic'">
      <xsl:call-template name="l10n.number.numeric">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="digits" select="'٠١٢٣٤٥٦٧٨٩'"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$format='decimal-persian'">
      <xsl:call-template name="l10n.number.numeric">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="digits" select="'۰۱۲۳۴۵۶۷۸۹'"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:when test="$format='alpha-serbian-lower'">
      <xsl:call-template name="l10n.number.alphabetic">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="alphabet"
                        select="'абвгдђежзијклљмнњопрстћуфхцчџш'"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$format='alpha-serbian-upper'">
      <xsl:call-template name="l10n.number.alphabetic">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="alphabet"
                        select="'АБВГДЂЕЖЗИЈКЛЉМНЊОПРСТЋУФХЦЧЏШ'"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:when test="$format='alpha-thai'">
      <xsl:call-template name="l10n.number.alphabetic">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="alphabet"
                        select="'กขคงจฉชซฌญฎฏฐฑฒณดตถทธนบปผฝพฟภมยรลวศษสหฬอฮ'"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$format='decimal-thai'">
      <xsl:call-template name="l10n.number.numeric">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="digits" select="'๐๑๒๓๔๕๖๗๘๙'"/>
      </xsl:call-template>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.number.alphabetic
Formats a number using an alphabetic numbering system
$value: The numeric value of the number to format
$alphabet: A string containing the characters of the alphabet to use

REMARK: Talk about alphabetic numbering systems
-->
<xsl:template name="l10n.number.alphabetic">
  <xsl:param name="value"/>
  <xsl:param name="alphabet"/>
  <xsl:variable name="length" select="string-length($alphabet)"/>
  <xsl:choose>
    <xsl:when test="$value &lt;= 0">
      <xsl:number format="1" value="$value"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="digit">
        <xsl:choose>
          <xsl:when test="$value mod $length = 0">
            <xsl:value-of select="$length"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$value mod $length"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:if test="$value - $digit != 0">
        <xsl:call-template name="l10n.number.alphabetic">
          <xsl:with-param name="value" select="($value - $digit) div $length"/>
          <xsl:with-param name="alphabet" select="$alphabet"/>
        </xsl:call-template>
      </xsl:if>
      <xsl:value-of select="substring($alphabet, $digit, 1)"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.number.numeric
Formats a number using a numeric numbering system with any radix
$value: The numeric value of the number to format
$digits: A string containing the digits to use, starting with zero

REMARK: Talk about numeric numbering systems
-->
<xsl:template name="l10n.number.numeric">
  <xsl:param name="value"/>
  <xsl:param name="digits"/>
  <xsl:param name="length" select="string-length($digits)"/>
  <xsl:choose>
    <xsl:when test="$value &lt; 0">
      <!-- FIXME: We need to localize the negative sign -->
      <xsl:text>-</xsl:text>
      <xsl:call-template name="l10n.number.numeric">
        <xsl:with-param name="value" select="math:abs($value)"/>
        <xsl:with-param name="digits" select="$digits"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="digit" select="$value mod $length"/>
      <xsl:if test="$value - $digit != 0">
        <xsl:call-template name="l10n.number.numeric">
          <xsl:with-param name="value" select="($value - $digit) div $length"/>
          <xsl:with-param name="digits" select="$digits"/>
        </xsl:call-template>
      </xsl:if>
      <xsl:value-of select="substring($digits, $digit + 1, 1)"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.number.cjk-ideographic
Formats a number using a CJK ideographic system
$value: The numeric value of the number to format
$format: Which ideographic system to use

REMARK: Talk about CJK ideographic numbering systems.  Valid values of ${format}
are #{cjk-japanese}, #{cjk-chinese-simp}, and #{cjk-chinese-trad}.
-->
<xsl:template name="l10n.number.cjk-ideographic">
  <xsl:param name="value"/>
  <xsl:param name="format"/>
  <xsl:call-template name="l10n.number.cjk-ideographic.private">
    <xsl:with-param name="value" select="$value"/>
    <xsl:with-param name="format" select="$format"/>
    <xsl:with-param name="level" select="0"/>
  </xsl:call-template>
</xsl:template>

<!--#* l10n.number.cjk-ideographic.private -->
<xsl:template name="l10n.number.cjk-ideographic.private">
  <xsl:param name="value"/>
  <xsl:param name="format"/>
  <xsl:param name="level" select="0"/>
  <xsl:param name="zero" select="true()"/>
  <xsl:variable name="ones">
    <xsl:text>&#x3007;&#x4E00;&#x4E8C;&#x4E09;&#x56DB;</xsl:text>
    <xsl:text>&#x4E94;&#x516D;&#x4E03;&#x516B;&#x4E5D;</xsl:text>
  </xsl:variable>
  <xsl:variable name="tens">
    <xsl:text>&#x5341;&#x767E;&#x5343;</xsl:text>
  </xsl:variable>
  <!-- FIXME: pick a upper bound and fallback to decimal -->
  <xsl:variable name="myriads">
    <xsl:choose>
      <xsl:when test="$format='cjk-japanese'">
        <xsl:text>&#x4E07;&#x5104;&#x5146;&#x4EAC;&#x5793;</xsl:text>
      </xsl:when>
      <xsl:when test="$format='cjk-chinese-simp'">
        <xsl:text>&#x4E07;&#x4EBF;&#x5146;</xsl:text>
      </xsl:when>
      <xsl:when test="$format='cjk-chinese-trad'">
        <xsl:text>&#x842C;&#x5104;&#x5146;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="digit" select="$value mod 10"/>
  <xsl:if test="$value - $digit &gt; 0">
    <xsl:call-template name="l10n.number.cjk-ideographic.private">
      <xsl:with-param name="value" select="($value - $digit) div 10"/>
      <xsl:with-param name="format" select="$format"/>
      <xsl:with-param name="level" select="$level + 1"/>
      <xsl:with-param name="zero" select="$digit = 0"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:choose>
    <xsl:when test="$digit = 0">
      <xsl:choose>
        <xsl:when test="$value = 0">
          <xsl:value-of select="substring($ones, $digit + 1, 1)"/>
        </xsl:when>
        <xsl:when test="$format='cjk-chinese-simp' and not($zero)">
          <xsl:value-of select="substring($ones, $digit + 1, 1)"/>
        </xsl:when>
        <xsl:when test="$format='cjk-chinese-trad' and not($zero)">
          <xsl:value-of select="substring($ones, $digit + 1, 1)"/>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="substring($ones, $digit + 1, 1)"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:variable name="ten" select="$level mod 4"/>
  <xsl:choose>
    <xsl:when test="$ten != 0">
      <xsl:if test="$digit != 0">
        <xsl:value-of select="substring($tens, $ten, 1)"/>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="myriad" select="($level - $ten) div 4"/>
      <xsl:if test="$myriad != 0">
        <xsl:value-of select="substring($myriads, $myriad, 1)"/>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.number.ionic
Formats a number using the Ionic numeral system
$value: The numeric value of the number to format
$format: Which format to use

REMARK: Talk about the Ionic numeral system.  Talk about ${format}
See #{http://en.wikipedia.org/wiki/Greek_numerals}.
-->
<xsl:template name="l10n.number.ionic">
  <xsl:param name="value"/>
  <xsl:param name="format" select="'ionic-lower'"/>
  <xsl:choose>
    <xsl:when test="$value &lt; 1 or $value &gt; 999999">
      <xsl:number format="1" value="$value"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="l10n.number.ionic.private">
        <xsl:with-param name="value" select="$value"/>
        <xsl:with-param name="format" select="'ionic-lower'"/>
        <xsl:with-param name="level" select="1"/>
      </xsl:call-template>
      <xsl:text>´</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--#* l10n.number.ionic.private -->
<xsl:template name="l10n.number.ionic.private">
  <xsl:param name="value"/>
  <xsl:param name="format" select="'ionic-lower'"/>
  <xsl:param name="level" select="1"/>
  <xsl:param name="stigma" select="false()"/>
  <xsl:variable name="digit" select="$value mod 10"/>
  <xsl:if test="$value - $digit &gt; 0">
    <xsl:call-template name="l10n.number.ionic.private">
      <xsl:with-param name="value" select="($value - $digit) div 10"/>
      <xsl:with-param name="format" select="$format"/>
      <xsl:with-param name="level" select="$level + 1"/>
      <xsl:with-param name="stigma" select="$stigma"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:choose>
    <xsl:when test="$format='ionic-lower'">
      <xsl:choose>
        <xsl:when test="$digit = 0"/>
        <xsl:when test="not($stigma) and $digit = 6 and $level = 1">
          <xsl:text>στ</xsl:text>
        </xsl:when>
        <xsl:when test="$level = 1 or $level = 4">
          <xsl:if test="$level = 4">
            <xsl:text>,</xsl:text>
          </xsl:if>
          <xsl:value-of select="substring('αβγδεϛζηθ', $digit, 1)"/>
        </xsl:when>
        <xsl:when test="$level = 2 or $level = 5">
          <xsl:if test="$level = 5">
            <xsl:text>,</xsl:text>
          </xsl:if>
          <xsl:value-of select="substring('ικλμνξοπϟ', $digit, 1)"/>
        </xsl:when>
        <xsl:when test="$level = 3 or $level = 6">
          <xsl:if test="$level = 6">
            <xsl:text>,</xsl:text>
          </xsl:if>
          <xsl:value-of select="substring('ρστυφχψωϡ', $digit, 1)"/>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$format='ionic-upper'">
      <xsl:choose>
        <xsl:when test="$digit = 0"/>
        <xsl:when test="not($stigma) and $digit = 6 and $level = 1">
          <xsl:text>ΣΤ</xsl:text>
        </xsl:when>
        <xsl:when test="not($stigma) and $digit = 6 and $level = 1">
          <xsl:text>,Σ,Τ</xsl:text>
        </xsl:when>
        <xsl:when test="$level = 1 or $level = 4">
          <xsl:if test="$level = 4">
            <xsl:text>,</xsl:text>
          </xsl:if>
          <xsl:value-of select="substring('ΑΒΓΔΕϚΖΗΘ', $digit, 1)"/>
        </xsl:when>
        <xsl:when test="$level = 2 or $level = 5">
          <xsl:if test="$level = 5">
            <xsl:text>,</xsl:text>
          </xsl:if>
          <xsl:value-of select="substring('ΙΚΛΜΝΞΟΠϘ', $digit, 1)"/>
        </xsl:when>
        <xsl:when test="$level = 3 or $level = 6">
          <xsl:if test="$level = 6">
            <xsl:text>,</xsl:text>
          </xsl:if>
          <xsl:value-of select="substring('ΡΣΤΥΦΧΨΩϠ', $digit, 1)"/>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$digit"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
