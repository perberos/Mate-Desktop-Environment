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
DocBook to HTML - Class Synopses
:Requires: db2html-inline db2html-xref gettext

REMARK: Describe this module.  Implmentation note: for C++, we expect the first
modifier to be the visibility
-->

<xsl:variable name="cpp.tab" select="'&#160;&#160;&#160;&#160;'"/>
<xsl:variable name="python.tab" select="'&#160;&#160;&#160;&#160;'"/>


<!-- FIXME: document PI -->
<!--@@==========================================================================
db2html.classsynopsis.language
The default programming language used to format #{classsynopsis} elements

REMARK: Describe this param
-->
<xsl:param name="db2html.classsynopsis.language">
  <xsl:choose>
    <xsl:when test="/processing-instruction('db2html.classsynopsis.language')">
      <xsl:value-of
       select="/processing-instruction('db2html.classsynopsis.language')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="'cpp'"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:param>


<!-- == Matched Templates == -->

<!-- = *synopsis = -->
<xsl:template match="
              classsynopsis  | constructorsynopsis | fieldsynopsis |
              methodsynopsis | destructorsynopsis  ">
  <xsl:variable name="language">
    <xsl:choose>
      <xsl:when test="@language">
        <xsl:value-of select="@language"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$db2html.classsynopsis.language"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="first"
                select="not(preceding-sibling::*
                        [not(self::blockinfo) and not(self::title) and
                         not(self::titleabbrev) and not(self::attribution) ])"/>

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
      <xsl:text>block synopsis </xsl:text>
      <xsl:value-of select="local-name(.)"/>
      <xsl:if test="$first">
        <xsl:text> block-first</xsl:text>
      </xsl:if>
      <xsl:value-of select="concat(' watermark-code-', $language)"/>
    </xsl:attribute>
    <xsl:call-template name="db2html.anchor"/>
    <pre class="{local-name(.)} classsynopsis-{$language}">
      <xsl:choose>
        <xsl:when test="$language = 'cpp'">
          <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
        </xsl:when>
        <xsl:when test="$language = 'python'">
          <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message>
            <xsl:text>No information about the language '</xsl:text>
            <xsl:value-of select="$language"/>
            <xsl:text>' for classsynopsis.</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </pre>
  </div>
</xsl:template>

<!-- = classsynopsisinfo = -->
<xsl:template match="classsynopsisinfo">
  <xsl:apply-templates/>
  <!-- FIXME? -->
  <xsl:text>&#x000A;</xsl:text>
</xsl:template>

<!-- = methodparam = -->
<xsl:template match="methodparam">
  <span class="methodparam">
    <xsl:for-each select="*">
      <xsl:if test="position() != 1">
        <xsl:text> </xsl:text>
      </xsl:if>
      <xsl:apply-templates select="."/>
    </xsl:for-each>
  </span>
</xsl:template>

<!-- = methodparam/parameter = -->
<xsl:template match="methodparam/parameter">
  <xsl:call-template name="db2html.inline">
    <xsl:with-param name="mono" select="true()"/>
    <xsl:with-param name="italic" select="true()"/>
  </xsl:call-template>
</xsl:template>

<!--#* db2html.class.cpp.modifier -->
<xsl:template name="db2html.class.cpp.modifier">
  <!-- For C++, we expect the first modifier to be the visibility -->
  <xsl:variable name="prec" select="self::*[../self::classsynopsis]/preceding-sibling::constructorsynopsis |
                                    self::*[../self::classsynopsis]/preceding-sibling::destructorsynopsis  |
                                    self::*[../self::classsynopsis]/preceding-sibling::fieldsynopsis       |
                                    self::*[../self::classsynopsis]/preceding-sibling::methodsynopsis      "/>
  <xsl:choose>
    <xsl:when test="not($prec[modifier][last()][modifier[1] = current()/modifier[1]])">
      <xsl:if test="$prec"><xsl:text>&#x000A;</xsl:text></xsl:if>
      <xsl:apply-templates select="modifier[1]"/>
      <xsl:text>:&#x000A;</xsl:text>
    </xsl:when>
    <xsl:when test="$prec and (name($prec[last()]) != name(.))">
      <xsl:text>&#x000A;</xsl:text>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!--%%==========================================================================
db2html.class.cpp.mode

REMARK: Describe this mode
-->
<xsl:template mode="db2html.class.cpp.mode" match="*">
  <xsl:apply-templates select="."/>
</xsl:template>

<!-- = classsynopsis % db2html.class.cpp.mode = -->
<xsl:template mode="db2html.class.cpp.mode" match="classsynopsis">
  <!-- classsynopsis = element classsynopsis {
         attribute language { ... }?,
         attribute class { ... }?,
         ooclass+,
         (classsynopsisinfo  | constructorsynopsis |
          destructorsynopsis | fieldsynopsis       |
          methodsynopsis     )
       }
  -->
  <xsl:if test="@class = 'class' or not(@class)">
    <span class="ooclass">
      <xsl:for-each select="ooclass[1]/modifier">
        <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
        <xsl:text> </xsl:text>
      </xsl:for-each>
      <xsl:text>class </xsl:text>
      <xsl:apply-templates mode="db2html.class.cpp.mode"
                           select="ooclass[1]/classname"/>
    </span>
    <xsl:if test="ooclass[2]">
      <xsl:text> : </xsl:text>
      <xsl:for-each select="ooclass[position() != 1]">
        <xsl:if test="position() != 1">
          <xsl:text>, </xsl:text>
        </xsl:if>
        <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
      </xsl:for-each>
    </xsl:if>
    <xsl:text>&#x000A;{&#x000A;</xsl:text>
    <xsl:apply-templates mode="db2html.class.cpp.mode"
                         select="
                           classsynopsisinfo   |
                           constructorsynopsis | destructorsynopsis |
                           fieldsynopsis       | methodsynopsis     "/>
    <xsl:text>}&#x000A;</xsl:text>
  </xsl:if>
</xsl:template>

<!-- = constructorsynopsis % db2html.class.cpp.mode = -->
<xsl:template mode="db2html.class.cpp.mode" match="constructorsynopsis">
  <!-- constructorsynopsis = element constructorsynopsis {
         attribute language { ... }?,
         modifier+,
         methodname?,
         (methodparam+ | void?)
       }
  -->
  <xsl:call-template name="db2html.class.cpp.modifier"/>
  <xsl:value-of select="$cpp.tab"/>
  <xsl:for-each select="modifier[position() != 1]">
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
    <xsl:text> </xsl:text>
  </xsl:for-each>
  <xsl:choose>
    <xsl:when test="methodname">
      <xsl:apply-templates mode="db2html.class.cpp.mode" select="methodname"/>
    </xsl:when>
    <xsl:when test="../self::classsynopsis[ooclass]">
      <span class="methodname">
        <xsl:value-of select="../ooclass/classname"/>
      </span>
    </xsl:when>
  </xsl:choose>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="methodparam">
    <xsl:if test="position() != 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
  </xsl:for-each>
  <xsl:text>);&#x000A;</xsl:text>
</xsl:template>

<!-- = destructorsynopsis % db2html.class.cpp.mode = -->
<xsl:template mode="db2html.class.cpp.mode" match="destructorsynopsis">
  <!-- destructorsynopsis = element destructorsynopsis {
         attribute language { ... }?,
         modifier+,
         methodname?,
         (methodparam+ | void?)
       }
  -->
  <xsl:call-template name="db2html.class.cpp.modifier"/>
  <xsl:value-of select="$cpp.tab"/>
  <xsl:for-each select="modifier[position() != 1]">
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
    <xsl:text> </xsl:text>
  </xsl:for-each>
  <xsl:choose>
    <xsl:when test="methodname">
      <xsl:apply-templates mode="db2html.class.cpp.mode" select="methodname"/>
    </xsl:when>
    <xsl:when test="../self::classsynopsis[ooclass]">
      <span class="methodname">
        <xsl:text>~</xsl:text>
        <xsl:value-of select="../ooclass/classname"/>
      </span>
    </xsl:when>
  </xsl:choose>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="methodparam">
    <!-- FIXME: should we do each methodparam on its own line? -->
    <xsl:if test="position() != 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
  </xsl:for-each>
  <xsl:text>);&#x000A;</xsl:text>
</xsl:template>

<!-- = fieldsynopsis % db2html.class.cpp.mode = -->
<xsl:template mode="db2html.class.cpp.mode" match="fieldsynopsis">
  <!-- fieldsynopsis = element fieldsynopsis {
         attribute language { ... }?,
         modifier+,
         type,
         varname,
         initializer?
       }
  -->
  <xsl:call-template name="db2html.class.cpp.modifier"/>
  <xsl:value-of select="$cpp.tab"/>
  <xsl:for-each select="modifier[position() != 1]">
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
    <xsl:text> </xsl:text>
  </xsl:for-each>
  <xsl:if test="type">
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="type"/>
    <xsl:text> </xsl:text>
  </xsl:if>
  <xsl:apply-templates mode="db2html.class.cpp.mode" select="varname"/>
  <xsl:if test="initializer">
    <xsl:text> = </xsl:text>
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="initializer"/>
  </xsl:if>
  <xsl:text>;&#x000A;</xsl:text>
</xsl:template>

<!-- = methodparam % db2html.class.cpp.mode = -->
<xsl:template mode="db2html.class.cpp.mode" match="methodparam">
  <span class="methodparam">
    <xsl:for-each select="*">
      <xsl:if test="position() != 1">
        <xsl:text> </xsl:text>
      </xsl:if>
      <xsl:if test="self::initializer">
        <xsl:text>= </xsl:text>
      </xsl:if>
      <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
    </xsl:for-each>
  </span>
</xsl:template>

<!-- = methodsynopsis % db2html.class.cpp.mode = -->
<xsl:template mode="db2html.class.cpp.mode" match="methodsynopsis">
  <!-- methodsynopsis = element methodsynopsis {
         attribute language { ... }?,
         modifier+,
         (type | void),
         methodname,
         (methodparam+ | void?)
       }
  -->
  <xsl:call-template name="db2html.class.cpp.modifier"/>
  <xsl:value-of select="$cpp.tab"/>
  <!-- Parens for document order -->
  <xsl:for-each select="(methodname/preceding-sibling::modifier)[position() != 1]">
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
    <xsl:text> </xsl:text>
  </xsl:for-each>
  <xsl:apply-templates mode="db2html.class.cpp.mode" select="type | methodname/preceding-sibling::void"/>
  <xsl:text> </xsl:text>
  <xsl:apply-templates mode="db2html.class.cpp.mode" select="methodname"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="methodparam">
    <xsl:if test="position() != 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
  </xsl:for-each>
  <xsl:text>)</xsl:text>
  <xsl:for-each select="methodname/following-sibling::modifier">
    <xsl:text> </xsl:text>
    <xsl:apply-templates mode="db2html.class.cpp.mode" select="."/>
  </xsl:for-each>
  <xsl:text>;&#x000A;</xsl:text>
</xsl:template>


<!--%%==========================================================================
db2html.class.python.mode

REMARK: Describe this mode
-->
<xsl:template mode="db2html.class.python.mode" match="*">
  <xsl:apply-templates select="."/>
</xsl:template>

<!-- = classsynopsis % db2html.class.python.mode = -->
<xsl:template mode="db2html.class.python.mode" match="classsynopsis">
  <!-- classsynopsis = element classsynopsis {
         attribute language { ... }?,
         attribute class { ... }?,
         ooclass+,
         (classsynopsisinfo  | constructorsynopsis |
          destructorsynopsis | fieldsynopsis       |
          methodsynopsis     )
       }
  -->
  <xsl:if test="@class = 'class' or not(@class)">
    <xsl:text>class </xsl:text>
    <xsl:apply-templates mode="db2html.class.python.mode" select="ooclass[1]"/>
    <xsl:if test="ooclass[2]">
      <xsl:text>(</xsl:text>
      <xsl:for-each select="ooclass[position() != 1]">
        <xsl:if test="position() != 1">
          <xsl:text>, </xsl:text>
        </xsl:if>
        <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
    </xsl:if>
    <xsl:text>:&#x000A;</xsl:text>
    <xsl:for-each select="classsynopsisinfo  | constructorsynopsis |
                          destructorsynopsis | fieldsynopsis       |
                          methodsynopsis     ">
      <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
      <xsl:if test="position() != last() and local-name(following-sibling::*[1]) != local-name(.)">
        <xsl:text>&#x000A;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:if>
</xsl:template>

<!-- = constructorsynopsis % db2html.class.python.mode = -->
<xsl:template mode="db2html.class.python.mode" match="constructorsynopsis">
  <!-- constructorsynopsis = element constructorsynopsis {
         attribute language { ... }?,
         modifier+,
         methodname?,
         (methodparam+ | void?)
       }
  -->
  <xsl:variable name="tab">
    <xsl:if test="../self::classsynopsis">
      <xsl:value-of select="$python.tab"/>
    </xsl:if>
  </xsl:variable>
  <xsl:for-each select="modifier">
    <xsl:value-of select="$tab"/>
    <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
    <xsl:text>&#x000A;</xsl:text>
  </xsl:for-each>
  <xsl:value-of select="$tab"/>
  <xsl:choose>
    <xsl:when test="methodname">
      <xsl:apply-templates mode="db2html.class.python.mode" select="methodname"/>
    </xsl:when>
    <xsl:when test="../self::classsynopsis[ooclass]">
      <span class="methodname">
        <xsl:value-of select="../ooclass/classname"/>
      </span>
    </xsl:when>
  </xsl:choose>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="methodparam">
    <xsl:if test="position() != 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
  </xsl:for-each>
  <xsl:text>)</xsl:text>
  <xsl:if test="type">
    <xsl:text> -&gt; </xsl:text>
    <xsl:apply-templates mode="db2html.class.python.mode" select="type"/>
  </xsl:if>
  <xsl:text>&#x000A;</xsl:text>
</xsl:template>

<!-- = destructorsynopsis % db2html.class.python.mode = -->
<xsl:template mode="db2html.class.python.mode" match="destructorsynopsis">
  <!-- destructorsynopsis = element destructorsynopsis {
         attribute language { ... }?,
         modifier+,
         methodname?,
         (methodparam+ | void?)
       }
  -->
  <xsl:variable name="tab">
    <xsl:if test="../self::classsynopsis">
      <xsl:value-of select="$python.tab"/>
    </xsl:if>
  </xsl:variable>
  <xsl:for-each select="modifier">
    <xsl:value-of select="$tab"/>
    <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
    <xsl:text>&#x000A;</xsl:text>
  </xsl:for-each>
  <xsl:value-of select="$tab"/>
  <xsl:choose>
    <xsl:when test="methodname">
      <xsl:apply-templates mode="db2html.class.python.mode" select="methodname"/>
    </xsl:when>
    <xsl:otherwise>
      <span class="methodname">
        <xsl:text>__del__</xsl:text>
      </span>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="methodparam">
    <xsl:if test="position() != 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
  </xsl:for-each>
  <xsl:text>)</xsl:text>
  <xsl:if test="type">
    <xsl:text> -&gt; </xsl:text>
    <xsl:apply-templates mode="db2html.class.python.mode" select="type"/>
  </xsl:if>
  <xsl:text>&#x000A;</xsl:text>
</xsl:template>

<!-- = fieldsynopsis % db2html.class.python.mode = -->
<xsl:template mode="db2html.class.python.mode" match="fieldsynopsis">
  <!-- fieldsynopsis = element fieldsynopsis {
         attribute language { ... }?,
         modifier+,
         type,
         varname,
         initializer?
       }
  -->
  <xsl:variable name="tab">
    <xsl:if test="../self::classsynopsis">
      <xsl:value-of select="$python.tab"/>
    </xsl:if>
  </xsl:variable>
  <xsl:for-each select="modifier">
    <xsl:value-of select="$tab"/>
    <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
    <xsl:text>&#x000A;</xsl:text>
  </xsl:for-each>
  <xsl:value-of select="$tab"/>
  <xsl:apply-templates mode="db2html.class.python.mode" select="varname"/>
  <xsl:if test="initializer">
    <xsl:text>=</xsl:text>
    <xsl:apply-templates mode="db2html.class.python.mode" select="initializer"/>
  </xsl:if>
  <xsl:text>&#x000A;</xsl:text>
</xsl:template>

<!-- = methodparam % db2html.class.python.mode = -->
<xsl:template mode="db2html.class.python.mode" match="methodparam">
  <span class="methodparam">
    <xsl:apply-templates mode="db2html.class.python.mode" select="parameter"/>
    <xsl:if test="modifier or type">
      <xsl:text>: </xsl:text>
      <xsl:apply-templates mode="db2html.class.python.mode" select="(modifier | type)[1]"/>
      <xsl:if test="initializer">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:if>
    <xsl:if test="initializer">
      <xsl:text>=</xsl:text>
      <xsl:if test="modifier or type">
        <xsl:text> </xsl:text>
      </xsl:if>
      <xsl:apply-templates mode="db2html.class.python.mode" select="initializer"/>
    </xsl:if>
  </span>
</xsl:template>

<!-- = methodsynopsis % db2html.class.python.mode = -->
<xsl:template mode="db2html.class.python.mode" match="methodsynopsis">
  <!-- methodsynopsis = element methodsynopsis {
         attribute language { ... }?,
         modifier+,
         (type | void),
         methodname,
         (methodparam+ | void?)
       }
  -->
  <xsl:variable name="tab">
    <xsl:if test="../self::classsynopsis">
      <xsl:value-of select="$python.tab"/>
    </xsl:if>
  </xsl:variable>
  <xsl:for-each select="modifier">
    <xsl:value-of select="$tab"/>
    <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
    <xsl:text>&#x000A;</xsl:text>
  </xsl:for-each>
  <xsl:value-of select="$tab"/>
  <xsl:text>def </xsl:text>
  <xsl:apply-templates mode="db2html.class.python.mode" select="methodname"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="methodparam">
    <xsl:if test="position() != 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <xsl:apply-templates mode="db2html.class.python.mode" select="."/>
  </xsl:for-each>
  <xsl:text>)</xsl:text>
  <xsl:if test="type">
    <xsl:text> -&gt; </xsl:text>
    <xsl:apply-templates mode="db2html.class.python.mode" select="type"/>
  </xsl:if>
  <xsl:text>&#x000A;</xsl:text>
</xsl:template>

</xsl:stylesheet>
