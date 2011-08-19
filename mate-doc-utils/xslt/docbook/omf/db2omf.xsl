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
                version="1.0">

<xsl:output method="xml" encoding="utf-8" indent="yes"/>

<xsl:include href="../common/db-common.xsl"/>

<!--!!==========================================================================
DocBook to ScrollKeeper OMF
:Requires: db-common
-->


<!--@@==========================================================================
db2omf.basename
The basename of the referenced document

REMARK: Document what a basename is
-->
<xsl:param name="db2omf.basename"/>


<!--@@==========================================================================
db2omf.format
The format of the referenced document

REMARK: Document this
-->
<xsl:param name="db2omf.format"/>


<!--@@==========================================================================
db2omf.mime
The MIME type of the referenced document

REMARK: Document this
-->
<xsl:param name="db2omf.mime" select="'text/xml'"/>


<!--@@==========================================================================
db2omf.dtd
The FPI of the DocBook version used

REMARK: Document this
-->
<xsl:param name="db2omf.dtd"/>


<!--@@==========================================================================
db2omf.lang
The written language of the referenced document

REMARK: Document this
-->
<xsl:param name="db2omf.lang" select="/*/@lang | /*/@xml:lang"/>


<!--@@==========================================================================
db2omf.omf_dir
The top-level ScrollKeeper OMF directory

REMARK: Document this
-->
<xsl:param name="db2omf.omf_dir"/>


<!--@@==========================================================================
db2omf.help_dir
The top-level directory where documentation is installed

REMARK: Document this
-->
<xsl:param name="db2omf.help_dir"/>


<!--@@==========================================================================
db2omf.omf_in
Path to the OMF input file containing common fields

REMARK: Document this
-->
<xsl:param name="db2omf.omf_in"/>
<xsl:variable name="omf_in" select="document($db2omf.omf_in)"/>


<!--@@==========================================================================
db2omf.scrollkeeper_cl
The path to the installed scrollkeeper_cl.xml file

REMARK: Document what this is for
-->
<xsl:param name="db2omf.scrollkeeper_cl" select="false()"/>


<!--**==========================================================================
db2omf.omf
Generates the top-level #{omf} and all its children
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf" match="/*">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <omf>
    <resource>
      <xsl:call-template name="db2omf.omf.creator">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.maintainer">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.contributor">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.title">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.date">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.version">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.subject">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.description">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.type">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.format">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.identifier">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.language">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.relation">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
      <xsl:call-template name="db2omf.omf.rights">
        <xsl:with-param name="info" select="$info"/>
      </xsl:call-template>
    </resource>
  </omf>
</xsl:template>


<!--**==========================================================================
db2omf.omf.creator
Generates all the #{creator} elements for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.creator">
  <xsl:param name="info"
             select="*[substring(local-name(.), string-length(local-name(.)) - 3)
                       = 'info']"/>
  <xsl:variable name="creators"
                select="$info/author     | $info/authorgroup/author    |
                        $info/corpauthor | $info/authorgroup/corpauthor"/>
  <xsl:if test="not($creators)">
    <xsl:message terminate="yes">
      <xsl:text>db2omf: Could not construct the OMF creator element.&#x000A;</xsl:text>
      <xsl:text>  Add an author element to </xsl:text>
      <xsl:value-of select="$db2omf.basename"/>
      <xsl:text>.xml.</xsl:text>
    </xsl:message>
  </xsl:if>
  <xsl:for-each select="$creators">
    <creator>
      <xsl:if test="email">
        <xsl:value-of select="email"/>
        <xsl:text> </xsl:text>
      </xsl:if>
      <xsl:text>(</xsl:text>
      <xsl:choose>
        <xsl:when test="self::corpauthor">
          <xsl:value-of select="."/>
        </xsl:when>
        <xsl:when test="personname">
          <xsl:call-template name="db.personname">
            <xsl:with-param name="node" select="personname"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="db.personname"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>)</xsl:text>
    </creator>
  </xsl:for-each>
</xsl:template>


<!--**==========================================================================
db2omf.omf.maintainer
Generates all the #{maintainer} elements for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.maintainer">
  <xsl:param name="info"
             select="*[substring(local-name(.), string-length(local-name(.)) - 3)
                       = 'info']"/>
  <xsl:variable name="maintainers"
                select="$info/author[@role='maintainer']        |
                        $info/corpauthor[@role='maintainer']    |
                        $info/editor[@role='maintainer']        |
                        $info/othercredit[@role='maintainer']   |
                        $info/publisher[@role='maintainer']     |
                        $info/authorgroup/*[@role='maintainer'] "/>
  <xsl:if test="not($maintainers)">
    <xsl:message terminate="yes">
      <xsl:text>db2omf: Could not construct the OMF maintainer element.&#x000A;</xsl:text>
      <xsl:text>  Add an author, corpauthor, editor, othercredit, or publisher&#x000A;</xsl:text>
      <xsl:text>  element with the role attribute set to "maintainer" to </xsl:text>
      <xsl:value-of select="$db2omf.basename"/>
      <xsl:text>.xml.</xsl:text>
    </xsl:message>
  </xsl:if>
  <xsl:for-each select="$maintainers">
    <maintainer>
      <xsl:choose>
        <xsl:when test="email">
          <xsl:value-of select="email"/>
          <xsl:text> </xsl:text>
        </xsl:when>
        <xsl:when test="address/email">
          <xsl:value-of select="addressemail"/>
          <xsl:text> </xsl:text>
        </xsl:when>
      </xsl:choose>
      <xsl:text>(</xsl:text>
      <xsl:choose>
        <xsl:when test="self::publisher">
          <xsl:value-of select="publishername"/>
        </xsl:when>
        <xsl:when test="self::corpauthor">
          <xsl:value-of select="."/>
        </xsl:when>
        <xsl:when test="personname">
          <xsl:call-template name="db.personname">
            <xsl:with-param name="node" select="personname"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="db.personname"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>)</xsl:text>
    </maintainer>
  </xsl:for-each>
</xsl:template>


<!--**==========================================================================
db2omf.omf.contributor
Generates all the #{contributor} elements for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.contributor">
  <xsl:param name="info"
             select="*[substring(local-name(.), string-length(local-name(.)) - 3)
                       = 'info']"/>
  <xsl:variable name="contributors"
                select="$info/editor      | $info/authorgroup/editor      |
                        $info/othercredit | $info/authorgroup/othercredit "/>
  <xsl:for-each select="$contributors">
    <contributor>
      <xsl:if test="email">
        <xsl:value-of select="email"/>
        <xsl:text> </xsl:text>
      </xsl:if>
      <xsl:text>(</xsl:text>
      <xsl:choose>
        <xsl:when test="self::corpauthor">
          <xsl:value-of select="."/>
        </xsl:when>
        <xsl:when test="personname">
          <xsl:call-template name="db.personname">
            <xsl:with-param name="node" select="personname"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="db.personname"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>)</xsl:text>
    </contributor>
  </xsl:for-each>
</xsl:template>


<!--**==========================================================================
db2omf.omf.title
Generates the #{title} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.title">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <xsl:variable name="title" select="($info/title | title)[1]"/>
  <xsl:if test="not($title)">
    <xsl:message terminate="yes">
      <xsl:text>db2omf: Could not construct the OMF title element.&#x000A;</xsl:text>
      <xsl:text>  Add a title element to </xsl:text>
      <xsl:value-of select="$db2omf.basename"/>
      <xsl:text>.xml.</xsl:text>
    </xsl:message>
  </xsl:if>
  <title>
    <xsl:value-of select="$title"/>
  </title>
</xsl:template>


<!--**==========================================================================
db2omf.omf.date
Generates the #{date} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.date">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <xsl:variable name="revision"
                select="$info/revhistory/revision[revnumber and date]"/>
  <xsl:choose>
    <xsl:when test="$revision">
      <date>
        <xsl:value-of select="$revision[last()]/date"/>
      </date>
    </xsl:when>
    <xsl:when test="$info/pubdate">
      <date>
        <xsl:value-of select="$info/pubdate[1]"/>
      </date>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">
        <xsl:text>db2omf: Could not construct the OMF date element.&#x000A;</xsl:text>
        <xsl:text>  Add either a revision element with revnumber and date,&#x000A;</xsl:text>
        <xsl:text>  or a pubdate element to </xsl:text>
        <xsl:value-of select="$db2omf.basename"/>
        <xsl:text>.xml.</xsl:text>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2omf.omf.version
Generates the #{version} element for an OMF file
$info: The info element containing metadata

REMARK: Document this, add the @description attr of version
-->
<xsl:template name="db2omf.omf.version">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <xsl:variable name="revision"
                select="$info/revhistory/revision[revnumber and date]"/>
  <xsl:choose>
    <xsl:when test="$revision">
      <xsl:variable name="revnumber" select="$revision[last()]/revnumber"/>
      <xsl:variable name="date" select="$revision[last()]/date"/>
      <version>
        <xsl:attribute name="identifier">
          <xsl:value-of select="$revnumber"/>
        </xsl:attribute>
        <xsl:attribute name="date">
          <xsl:value-of select="$date"/>
        </xsl:attribute>
      </version>
    </xsl:when>
    <xsl:when test="$info/edition and $info/pubdate">
      <version>
        <xsl:attribute name="identifier">
          <xsl:value-of select="$info/edition[1]"/>
        </xsl:attribute>
        <xsl:attribute name="date">
          <xsl:value-of select="$info/pubdate[1]"/>
        </xsl:attribute>
      </version>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">
        <xsl:text>db2omf: Could not construct the OMF version element.&#x000A;</xsl:text>
        <xsl:text>  Add either a revision element with revnumber and date,&#x000A;</xsl:text>
        <xsl:text>  or date and edition elements to </xsl:text>
        <xsl:value-of select="$db2omf.basename"/>
        <xsl:text>.xml.</xsl:text>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db2omf.omf.subject
Generates the #{subject} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.subject">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <xsl:variable name="subject" select="$omf_in/omf/resource/subject"/>
  <xsl:if test="not($subject)">
    <xsl:message terminate="yes">
      <xsl:text>db2omf: Could not construct the OMF subject element.&#x000A;</xsl:text>
      <xsl:text>  Add a subject element to </xsl:text>
      <xsl:value-of select="$db2omf.omf_in"/>
      <xsl:text>.</xsl:text>
    </xsl:message>
  </xsl:if>
  <xsl:if test="$db2omf.scrollkeeper_cl">
    <xsl:variable name="sk_cl" select="document($db2omf.scrollkeeper_cl)"/>
    <xsl:for-each select="$subject">
      <xsl:variable name="code" select="translate(@category, '|', '')"/>
      <xsl:if test="not($sk_cl//sect[@categorycode = $code])">
        <xsl:message terminate="yes">
          <xsl:text>db2omf: Invalid subject code "</xsl:text>
          <xsl:value-of select="@category"/>
          <xsl:text>" in </xsl:text>
          <xsl:value-of select="$db2omf.omf_in"/>
          <xsl:text>.</xsl:text>
        </xsl:message>
      </xsl:if>
    </xsl:for-each>
  </xsl:if>
  <xsl:copy-of select="$subject"/>
</xsl:template>


<!--**==========================================================================
db2omf.omf.description
Generates the #{description} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.description">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <xsl:variable name="description" select="$info/abstract[@role = 'description']"/>
  <xsl:if test="not($description)">
    <xsl:message terminate="yes">
      <xsl:text>db2omf: Could not construct the OMF description element.&#x000A;</xsl:text>
      <xsl:text>  Add an abstract with the role attribute set to "description"&#x000A;</xsl:text>
      <xsl:text>  to </xsl:text>
      <xsl:value-of select="$db2omf.basename"/>
      <xsl:text>.xml.</xsl:text>
    </xsl:message>
  </xsl:if>
  <description>
    <!-- FIXME: a smarter textification would be good -->
    <xsl:value-of select="$description"/>
  </description>
</xsl:template>


<!--**==========================================================================
db2omf.omf.type
Generates the #{type} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.type">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <xsl:variable name="type" select="$omf_in/omf/resource/type"/>
  <xsl:if test="not($type)">
    <xsl:message terminate="yes">
      <xsl:text>db2omf: Could not construct the OMF type element.&#x000A;</xsl:text>
      <xsl:text>  Add a type element to </xsl:text>
      <xsl:value-of select="$db2omf.omf_in"/>
      <xsl:text>.</xsl:text>
    </xsl:message>
  </xsl:if>
  <type>
    <xsl:value-of select="$type"/>
  </type>
</xsl:template>


<!--**==========================================================================
db2omf.omf.format
Generates the #{format} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.format">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <format>
    <xsl:attribute name="mime">
      <xsl:value-of select="$db2omf.mime"/>
    </xsl:attribute>
    <xsl:choose>
      <xsl:when test="$db2omf.mime = 'text/xml'">
        <xsl:attribute name="dtd">
          <xsl:value-of select="$db2omf.dtd"/>
        </xsl:attribute>
      </xsl:when>
      <xsl:when test="$db2omf.mime = 'text/html'"/>
      <xsl:otherwise>
        <xsl:message terminate="yes">
          <xsl:text>db2omf: Unknown value of db2omf.mime: "</xsl:text>
          <xsl:value-of select="$db2omf.mime"/>
          <xsl:text>".</xsl:text>
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </format>
</xsl:template>


<!--**==========================================================================
db2omf.omf.identifier
Generates the #{identifier} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.identifier">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <identifier>
    <xsl:attribute name="url">
      <xsl:text>file://</xsl:text>
      <xsl:value-of select="$db2omf.help_dir"/>
      <xsl:if test="not(substring($db2omf.help_dir,
                                  string-length($db2omf.help_dir)) = '/')">
        <xsl:text>/</xsl:text>
      </xsl:if>
      <xsl:value-of select="$db2omf.basename"/>
      <xsl:text>/</xsl:text>
      <xsl:value-of select="$db2omf.lang"/>
      <xsl:if test="not(substring($db2omf.lang,
                                  string-length($db2omf.lang)) = '/')">
        <xsl:text>/</xsl:text>
      </xsl:if>
      <xsl:value-of select="$db2omf.basename"/>
      <xsl:choose>
        <xsl:when test="$db2omf.format = 'docbook'">
          <xsl:text>.xml</xsl:text>
        </xsl:when>
        <xsl:when test="$db2omf.format = 'html'">
          <xsl:text>.html</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:text>db2omf: Unknown value of db2omf.format: "</xsl:text>
            <xsl:value-of select="$db2omf.format"/>
            <xsl:text>".</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
  </identifier>
</xsl:template>


<!--**==========================================================================
db2omf.omf.language
Generates the #{language} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.language">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <language code="{$db2omf.lang}"/>
</xsl:template>


<!--**==========================================================================
db2omf.omf.relation
Generates the #{relation} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.relation">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <xsl:variable name="relation" select="$omf_in/omf/resource/relation"/>
  <xsl:if test="not($relation)">
    <xsl:message terminate="yes">
      <xsl:text>db2omf: Could not construct the OMF relation element.&#x000A;</xsl:text>
      <xsl:text>  Add a relation element to </xsl:text>
      <xsl:value-of select="$db2omf.omf_in"/>
      <xsl:text>.</xsl:text>
    </xsl:message>
  </xsl:if>
  <xsl:copy-of select="$relation"/>
</xsl:template>


<!--**==========================================================================
db2omf.omf.rights
Generates the #{rights} element for an OMF file
$info: The info element containing metadata

REMARK: Document this
-->
<xsl:template name="db2omf.omf.rights">
  <xsl:param name="info"
	     select="*[substring(local-name(.), string-length(local-name(.)) - 3)
		       = 'info']"/>
  <xsl:variable name="rights" select="$omf_in/omf/resource/rights"/>
  <xsl:if test="not($rights)">
    <xsl:message terminate="yes">
      <xsl:text>db2omf: Could not construct the OMF rights element.&#x000A;</xsl:text>
      <xsl:text>  Add a rights element to </xsl:text>
      <xsl:value-of select="$db2omf.omf_in"/>
      <xsl:text>.</xsl:text>
    </xsl:message>
  </xsl:if>
  <xsl:copy-of select="$rights"/>
</xsl:template>

</xsl:stylesheet>
