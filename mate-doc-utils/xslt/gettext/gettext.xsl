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
                version="1.0">

<!--!!==========================================================================
Localized Strings
-->

<xsl:variable name="l10n" select="document('l10n.xml')"/>
<xsl:key name="msg" match="msg:msgset/msg:msg"
         use="concat(../msg:msgid, '__LC__', @xml:lang)"/>


<!--@@==========================================================================
l10n.locale
The top-level locale of the document
-->
<xsl:param name="l10n.locale">
  <xsl:choose>
    <xsl:when test="/*/@xml:lang">
      <xsl:value-of select="/*/@xml:lang"/>
    </xsl:when>
    <xsl:when test="/*/@lang">
      <xsl:value-of select="/*/@lang"/>
    </xsl:when>
  </xsl:choose>
</xsl:param>


<!--@@==========================================================================
l10n.language
The language part of the top-level locale of the document
-->
<xsl:param name="l10n.language">
  <xsl:choose>
    <xsl:when test="contains($l10n.locale, '_')">
      <xsl:value-of select="substring-before($l10n.locale, '_')"/>
    </xsl:when>
    <xsl:when test="contains($l10n.locale, '@')">
      <xsl:value-of select="substring-before($l10n.locale, '@')"/>
    </xsl:when>
    <xsl:when test="contains($l10n.locale, '_')">
      <xsl:value-of select="substring-before($l10n.locale, '@')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$l10n.locale"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:param>


<!--@@==========================================================================
l10n.region
The region part of the top-level locale of the document
-->
<xsl:param name="l10n.region">
  <xsl:variable name="aft" select="substring-after($l10n.locale, '_')"/>
  <xsl:choose>
    <xsl:when test="contains($aft, '@')">
      <xsl:value-of select="substring-before($aft, '@')"/>
    </xsl:when>
    <xsl:when test="contains($aft, '.')">
      <xsl:value-of select="substring-before($aft, '.')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$aft"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:param>


<!--@@==========================================================================
l10n.variant
The variant part of the top-level locale of the document
-->
<xsl:param name="l10n.variant">
  <xsl:variable name="aft" select="substring-after($l10n.locale, '@')"/>
  <xsl:choose>
    <xsl:when test="contains($aft, '.')">
      <xsl:value-of select="substring-before($aft, '.')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$aft"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:param>


<!--@@==========================================================================
l10n.charset
The charset part of the top-level locale of the document
-->
<xsl:param name="l10n.charset">
  <xsl:if test="contains($l10n.locale, '.')">
    <xsl:value-of select="substring-after($l10n.locale, '.')"/>
  </xsl:if>
</xsl:param>


<!--**==========================================================================
l10n.gettext
Looks up the translation for a string
$msgid: The id of the string to look up, often the string in the C locale
$lang: The locale to use when looking up the translated string
$lang_language: The language portion of the ${lang}
$lang_region: The region portion of ${lang}
$lang_variant: The variant portion of ${lang}
$lang_charset: The charset portion of ${lang}
$number: The cardinality for plural-form lookups
$form: The form name for plural-form lookups

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.gettext">
  <xsl:param name="msgid"/>
  <xsl:param name="lang" select="ancestor-or-self::*[@lang][1]/@lang"/>
  <xsl:param name="lang_language">
    <xsl:call-template name="l10n.language">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_region">
    <xsl:call-template name="l10n.region">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_variant">
    <xsl:call-template name="l10n.variant">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_charset">
    <xsl:call-template name="l10n.charset">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="number"/>
  <xsl:param name="form">
    <xsl:if test="$number">
      <xsl:call-template name="l10n.plural.form">
        <xsl:with-param name="number" select="$number"/>
        <xsl:with-param name="lang" select="$lang"/>
        <xsl:with-param name="lang_language" select="$lang_language"/>
        <xsl:with-param name="lang_region"   select="$lang_region"/>
        <xsl:with-param name="lang_variant"  select="$lang_variant"/>
        <xsl:with-param name="lang_charset"  select="$lang_charset"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:param>
  <xsl:param name="node" select="."/>
  <xsl:param name="role" select="''"/>
  <xsl:param name="string"/>
  <xsl:param name="format" select="false()"/>

  <xsl:for-each select="$l10n">
    <xsl:choose>
      <!-- fe_fi@fo.fum -->
      <xsl:when test="($lang_region and $lang_variant and $lang_charset) and
                      key('msg', concat($msgid, '__LC__',
                                        $lang_language, '_', $lang_region,
                                                        '@', $lang_variant,
                                                        '.', $lang_charset))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg"
           select="key('msg', concat($msgid, '__LC__',
                                     $lang_language, '_', $lang_region,
                                                     '@', $lang_variant,
                                                     '.', $lang_charset))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- fe_fi@fo -->
      <xsl:when test="($lang_region and $lang_variant) and
                      key('msg', concat($msgid, '__LC__',
                                        $lang_language, '_', $lang_region,
                                                        '@', $lang_variant))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg"
           select="key('msg', concat($msgid, '__LC__',
                                     $lang_language, '_', $lang_region,
                                                     '@', $lang_variant))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- fe@fo.fum -->
      <xsl:when test="($lang_variant and $lang_charset) and
                      key('msg', concat($msgid, '__LC__',
                                        $lang_language, '@', $lang_variant,
                                                        '.', $lang_charset))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg"
           select="key('msg', concat($msgid, '__LC__',
                                     $lang_language, '@', $lang_variant,
                                                     '.', $lang_charset))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- fe@fo -->
      <xsl:when test="($lang_variant) and
                      key('msg', concat($msgid, '__LC__',
                                        $lang_language, '@', $lang_variant))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg"
           select="key('msg', concat($msgid, '__LC__',
                                     $lang_language, '@', $lang_variant))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- fe_fi.fum -->
      <xsl:when test="($lang_region and $lang_charset) and
                      key('msg', concat($msgid, '__LC__',
                                        $lang_language, '_', $lang_region,
                                                        '.', $lang_charset))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg"
           select="key('msg', concat($msgid, '__LC__',
                                     $lang_language, '_', $lang_region,
                                                     '.', $lang_charset))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- fe_fi -->
      <xsl:when test="($lang_region) and
                      key('msg', concat($msgid, '__LC__',
                                        $lang_language, '_', $lang_region))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg"
           select="key('msg', concat($msgid, '__LC__',
                                     $lang_language, '_', $lang_region))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- fe.fum -->
      <xsl:when test="($lang_charset) and
                      key('msg', concat($msgid, '__LC__',
                                           $lang_language, '.', $lang_charset))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg"
           select="key('msg', concat($msgid, '__LC__',
                                     $lang_language, '.', $lang_charset))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- fe -->
      <xsl:when test="key('msg', concat($msgid, '__LC__', $lang_language))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg"
           select="key('msg', concat($msgid, '__LC__', $lang_language))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- "C" -->
      <xsl:when test="key('msg', concat($msgid, '__LC__C'))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg" select="key('msg', concat($msgid, '__LC__C'))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <!-- not() -->
      <xsl:when test="key('msg', concat($msgid, '__LC__'))">
        <xsl:call-template name="l10n.gettext.msg">
          <xsl:with-param
           name="msg" select="key('msg', concat($msgid, '__LC__'))"/>
          <xsl:with-param name="form" select="$form"/>
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="string" select="$string"/>
          <xsl:with-param name="format" select="$format"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message>
          <xsl:text>No translation available for string '</xsl:text>
          <xsl:value-of select="$msgid"/>
          <xsl:text>'.</xsl:text>
        </xsl:message>
        <xsl:value-of select="$msgid"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<!--#* l10n.gettext.msg -->
<xsl:template name="l10n.gettext.msg">
  <xsl:param name="msg"/>
  <xsl:param name="form" select="''"/>
  <xsl:param name="node" select="."/>
  <xsl:param name="role" select="''"/>
  <xsl:param name="string"/>
  <xsl:param name="format" select="false()"/>
  <xsl:choose>
    <xsl:when test="not($msg/msg:msgstr)">
      <xsl:call-template name="l10n.gettext.msgstr">
        <xsl:with-param name="msgstr" select="$msg"/>
        <xsl:with-param name="node" select="$node"/>
        <xsl:with-param name="role" select="$role"/>
        <xsl:with-param name="string" select="$string"/>
        <xsl:with-param name="format" select="$format"/>
      </xsl:call-template>
    </xsl:when>
    <!-- FIXME: OPTIMIZE: this needs to be faster -->
    <xsl:when test="$form != '' and $role != ''">
      <xsl:variable name="msgstr_form" select="$msg/msg:msgstr[@form = $form]"/>
      <xsl:choose>
        <xsl:when test="$msgstr_form">
          <xsl:choose>
            <xsl:when test="msgstr_form[@role = $role]">
              <xsl:call-template name="l10n.gettext.msgstr">
                <xsl:with-param name="msgstr"
                                select="msgstr_form[@role = $role][1]"/>
                <xsl:with-param name="node" select="$node"/>
                <xsl:with-param name="role" select="$role"/>
                <xsl:with-param name="string" select="$string"/>
                <xsl:with-param name="format" select="$format"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="msgstr_form[not(@role)]">
              <xsl:call-template name="l10n.gettext.msgstr">
                <xsl:with-param name="msgstr"
                                select="msgstr_form[not(@role)][1]"/>
                <xsl:with-param name="node" select="$node"/>
                <xsl:with-param name="role" select="$role"/>
                <xsl:with-param name="string" select="$string"/>
                <xsl:with-param name="format" select="$format"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="l10n.gettext.msgstr">
                <xsl:with-param name="msgstr"
                                select="msgstr_form[1]"/>
                <xsl:with-param name="node" select="$node"/>
                <xsl:with-param name="role" select="$role"/>
                <xsl:with-param name="string" select="$string"/>
                <xsl:with-param name="format" select="$format"/>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="$msg/msg:msgstr[@role = $role]">
              <xsl:call-template name="l10n.gettext.msgstr">
                <xsl:with-param name="msgstr"
                                select="$msg/msg:msgstr[@role = $role][1]"/>
                <xsl:with-param name="node" select="$node"/>
                <xsl:with-param name="role" select="$role"/>
                <xsl:with-param name="string" select="$string"/>
                <xsl:with-param name="format" select="$format"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="$msg/msg:msgstr[not(@role)]">
              <xsl:call-template name="l10n.gettext.msgstr">
                <xsl:with-param name="msgstr"
                                select="$msg/msg:msgstr[not(@role)][1]"/>
                <xsl:with-param name="node" select="$node"/>
                <xsl:with-param name="role" select="$role"/>
                <xsl:with-param name="string" select="$string"/>
                <xsl:with-param name="format" select="$format"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="l10n.gettext.msgstr">
                <xsl:with-param name="msgstr"
                                select="$msg/msg:msgstr[1]"/>
                <xsl:with-param name="node" select="$node"/>
                <xsl:with-param name="role" select="$role"/>
                <xsl:with-param name="string" select="$string"/>
                <xsl:with-param name="format" select="$format"/>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$form != ''">
      <xsl:choose>
        <xsl:when test="$msg/msg:msgstr[@form = $form]">
          <xsl:call-template name="l10n.gettext.msgstr">
            <xsl:with-param name="msgstr"
                            select="$msg/msg:msgstr[@form = $form][1]"/>
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="string" select="$string"/>
            <xsl:with-param name="format" select="$format"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$msg/msg:msgstr[not(@form)]">
          <xsl:call-template name="l10n.gettext.msgstr">
            <xsl:with-param name="msgstr"
                            select="$msg/msg:msgstr[not(@form)][1]"/>
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="string" select="$string"/>
            <xsl:with-param name="format" select="$format"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="l10n.gettext.msgstr">
            <xsl:with-param name="msgstr" select="$msg/msg:msgstr[1]"/>
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="string" select="$string"/>
            <xsl:with-param name="format" select="$format"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$role != ''">
      <xsl:choose>
        <xsl:when test="$msg/msg:msgstr[@role = $role]">
          <xsl:call-template name="l10n.gettext.msgstr">
            <xsl:with-param name="msgstr"
                            select="$msg/msg:msgstr[@role = $role][1]"/>
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="string" select="$string"/>
            <xsl:with-param name="format" select="$format"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$msg/msg:msgstr[not(@role)]">
          <xsl:call-template name="l10n.gettext.msgstr">
            <xsl:with-param name="msgstr"
                            select="$msg/msg:msgstr[not(@role)][1]"/>
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="string" select="$string"/>
            <xsl:with-param name="format" select="$format"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="l10n.gettext.msgstr">
            <xsl:with-param name="msgstr" select="$msg/msg:msgstr[1]"/>
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="string" select="$string"/>
            <xsl:with-param name="format" select="$format"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$msg/msg:msgstr[not(@role)]">
          <xsl:call-template name="l10n.gettext.msgstr">
            <xsl:with-param name="msgstr"
                            select="$msg/msg:msgstr[not(@role)][1]"/>
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="string" select="$string"/>
            <xsl:with-param name="format" select="$format"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="l10n.gettext.msgstr">
            <xsl:with-param name="msgstr" select="$msg/msg:msgstr[1]"/>
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="string" select="$string"/>
            <xsl:with-param name="format" select="$format"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--#* l10n.gettext.msgstr -->
<xsl:template name="l10n.gettext.msgstr">
  <xsl:param name="msgstr"/>
  <xsl:param name="node" select="."/>
  <xsl:param name="role"/>
  <xsl:param name="string"/>
  <xsl:param name="format" select="false()"/>
  <xsl:choose>
    <xsl:when test="$format">
      <xsl:apply-templates mode="l10n.format.mode" select="$msgstr/node()">
        <xsl:with-param name="node" select="$node"/>
        <xsl:with-param name="string" select="$string"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$msgstr"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.plural.form
Extracts he plural form string for a given cardinality
$number: The cardinality of the plural form
$lang: The locale to use when looking up the translated string
$lang_language: The language portion of the ${lang}
$lang_region: The region portion of ${lang}
$lang_variant: The variant portion of ${lang}
$lang_charset: The charset portion of ${lang}

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.plural.form">
  <xsl:param name="number" select="1"/>
  <xsl:param name="lang" select="$l10n.locale"/>
  <xsl:param name="lang_language">
    <xsl:call-template name="l10n.language">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_region">
    <xsl:call-template name="l10n.region">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_variant">
    <xsl:call-template name="l10n.variant">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_charset">
    <xsl:call-template name="l10n.charset">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>

  <xsl:choose>
    <!--
    Keep variants first!
    When adding new languages, make sure the tests are in a format that
    can be extracted by the plurals.sh script in the i18n directory.
    -->

    <!-- == pt_BR == -->
    <xsl:when test="concat($lang_language, '_', $lang_region) = 'pt_BR'">
      <xsl:choose>
        <xsl:when test="$number &gt; 1">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>1</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- == ar == -->
    <xsl:when test="$lang_language = 'ar'">
      <xsl:choose>
        <xsl:when test="$number = 1">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:when test="$number = 2">
          <xsl:text>1</xsl:text>
        </xsl:when>
        <xsl:when test="$number &gt;= 3 and $number &lt; 10">
          <xsl:text>2</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>3</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- == be bs cs ru sr uk == -->
    <xsl:when test="($lang_language = 'be') or ($lang_language = 'bs') or
                    ($lang_language = 'cs') or ($lang_language = 'ru') or
                    ($lang_language = 'sr') or ($lang_language = 'uk') ">
      <xsl:choose>
        <xsl:when test="($number mod 10 = 1) and ($number mod 100 != 11)">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:when test="($number mod 10 &gt;= 2) and ($number mod 10 &lt;= 4) and
                        (($number mod 100 &lt; 10) or ($number mod 100 &gt;= 20))">
          <xsl:text>1</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>2</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- == cy == -->
    <xsl:when test="$lang_language = 'cy'">
      <xsl:choose>
        <xsl:when test="$number != 2">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>1</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- == fa hu ja ko th tr vi zh == -->
    <xsl:when test="($lang_language = 'fa') or ($lang_language = 'hu') or
                    ($lang_language = 'ja') or ($lang_language = 'ko') or
                    ($lang_language = 'th') or ($lang_language = 'tr') or
                    ($lang_language = 'vi') or ($lang_language = 'zh') ">
      <xsl:text>0</xsl:text>
    </xsl:when>

    <!-- == fr nso wa == -->
    <xsl:when test="($lang_language = 'fr') or ($lang_language = 'nso') or
                    ($lang_language = 'wa') ">
      <xsl:choose>
        <xsl:when test="$number &gt; 1">
          <xsl:text>1</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>0</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- == ga == -->
    <xsl:when test="$lang_language = 'ga'">
      <xsl:choose>
        <xsl:when test="$number = 1">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:when test="$number = 2">
          <xsl:text>1</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>2</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- == sk == -->
    <xsl:when test="$lang_language = 'sk'">
      <xsl:choose>
        <xsl:when test="$number = 1">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:when test="($number &gt;= 2) and ($number &lt;= 4)">
          <xsl:text>1</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>2</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- == sl == -->
    <xsl:when test="$lang_language = 'sl'">
      <xsl:choose>
        <xsl:when test="$number mod 100 = 1">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:when test="$number mod 100 = 2">
          <xsl:text>1</xsl:text>
        </xsl:when>
        <xsl:when test="($number mod 100 = 3) or ($number mod 100 = 4)">
          <xsl:text>2</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>3</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- == C == -->
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$number = 1">
          <xsl:text>0</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>1</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.direction
Determines the text direction for the language of the document
$lang: The locale to use to determine the text direction
$lang_language: The language portion of the ${lang}
$lang_region: The region portion of ${lang}
$lang_variant: The variant portion of ${lang}
$lang_charset: The charset portion of ${lang}

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.direction">
  <xsl:param name="lang" select="$l10n.locale"/>
  <xsl:param name="lang_language">
    <xsl:call-template name="l10n.language">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_region">
    <xsl:call-template name="l10n.region">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_variant">
    <xsl:call-template name="l10n.variant">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="lang_charset">
    <xsl:call-template name="l10n.charset">
      <xsl:with-param name="lang" select="$lang"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:variable name="direction">
    <xsl:call-template name="l10n.gettext">
      <xsl:with-param name="msgid" select="'default:LTR'"/>
      <xsl:with-param name="lang" select="$lang"/>
      <xsl:with-param name="lang_language" select="$lang_language"/>
      <xsl:with-param name="lang_region"   select="$lang_region"/>
      <xsl:with-param name="lang_variant"  select="$lang_variant"/>
      <xsl:with-param name="lang_charset"  select="$lang_charset"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="$direction = 'default:RTL'">
      <xsl:text>rtl</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>ltr</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.align.start
Determines the start alignment
$direction: The text direction

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.align.start">
  <xsl:param name="direction">
    <xsl:call-template name="l10n.direction"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="$direction = 'rtl'">
      <xsl:text>right</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>left</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.align.end
Determines the end alignment
$direction: The text direction

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.align.end">
  <xsl:param name="direction">
    <xsl:call-template name="l10n.direction"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="$direction = 'rtl'">
      <xsl:text>left</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>right</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.arrow.previous
FIXME
$direction: The text direction

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.arrow.previous">
  <xsl:param name="direction">
    <xsl:call-template name="l10n.direction"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="$direction = 'rlt'">
      <xsl:text>&#x25C0;&#x00A0;&#x00A0;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>&#x00A0;&#x00A0;&#x25B6;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.arrow.next
FIXME
$direction: The text direction

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.arrow.next">
  <xsl:param name="direction">
    <xsl:call-template name="l10n.direction"/>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="$direction = 'rlt'">
      <xsl:text>&#x00A0;&#x00A0;&#x25B6;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>&#x25C0;&#x00A0;&#x00A0;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.language
Extracts the langauge portion of a locale
$lang: The locale to extract the language from

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.language">
  <xsl:param name="lang" select="ancestor-or-self::*[@lang][1]/@lang"/>
  <xsl:choose>
    <xsl:when test="$lang = $l10n.locale">
      <xsl:value-of select="$l10n.language"/>
    </xsl:when>
    <xsl:when test="contains($lang, '_')">
      <xsl:value-of select="substring-before($lang, '_')"/>
    </xsl:when>
    <xsl:when test="contains($lang, '@')">
      <xsl:value-of select="substring-before($lang, '@')"/>
    </xsl:when>
    <xsl:when test="contains($lang, '_')">
      <xsl:value-of select="substring-before($lang, '@')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$lang"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.region
Extracts the region portion of a locale
$lang: The locale to extract the region from

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.region">
  <xsl:param name="lang" select="ancestor-or-self::*[@lang][1]/@lang"/>
  <xsl:choose>
    <xsl:when test="$lang = $l10n.locale">
      <xsl:value-of select="$l10n.region"/>
    </xsl:when>
    <xsl:when test="contains($lang, '_')">
      <xsl:variable name="aft" select="substring-after($lang, '_')"/>
      <xsl:choose>
        <xsl:when test="contains($aft, '@')">
          <xsl:value-of select="substring-before($aft, '@')"/>
        </xsl:when>
        <xsl:when test="contains($aft, '.')">
          <xsl:value-of select="substring-before($aft, '.')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$aft"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.variant
Extracts the variant portion of a locale
$lang: The locale to extract the variant from

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.variant">
  <xsl:param name="lang" select="ancestor-or-self::*[@lang][1]/@lang"/>
  <xsl:choose>
    <xsl:when test="$lang = $l10n.locale">
      <xsl:value-of select="$l10n.variant"/>
    </xsl:when>
    <xsl:when test="contains($lang, '@')">
      <xsl:variable name="aft" select="substring-after($lang, '@')"/>
      <xsl:choose>
        <xsl:when test="contains($aft, '.')">
          <xsl:value-of select="substring-before($aft, '.')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$aft"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
l10n.charset
Extracts the charset portion of a locale
$lang: The locale to extract the charset from

REMARK: Lots of documentation is needed
-->
<xsl:template name="l10n.charset">
  <xsl:param name="lang" select="ancestor-or-self::*[@lang][1]/@lang"/>
  <xsl:choose>
    <xsl:when test="$lang = $l10n.locale">
      <xsl:value-of select="$l10n.charset"/>
    </xsl:when>
    <xsl:when test="contains($lang, '.')">
      <xsl:value-of select="substring-after($lang, '.')"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--%%==========================================================================
l10n.format.mode
FIXME
$node: The node being processed in the original document
$string: String content to use for certain message format nodes

REMARK: Lots and lots of documentation is needed
-->
<xsl:template mode="l10n.format.mode" match="*">
  <xsl:param name="node"/>
  <xsl:apply-templates mode="l10n.format.mode">
    <xsl:with-param name="node" select="$node"/>
  </xsl:apply-templates>
</xsl:template>

<!-- = l10n.format.mode % msg:node = -->
<xsl:template mode="l10n.format.mode" match="msg:node">
  <xsl:param name="node"/>
  <xsl:apply-templates select="$node/node()"/>
</xsl:template>

<!-- = l10n.format.mode % msg:string = -->
<xsl:template mode="l10n.format.mode" match="msg:string">
  <xsl:param name="string"/>
  <xsl:value-of select="$string"/>
</xsl:template>

</xsl:stylesheet>
