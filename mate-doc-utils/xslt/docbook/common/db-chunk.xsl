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
                xmlns:set="http://exslt.org/sets"
                extension-element-prefixes="exsl"
                exclude-result-prefixes="set"
                version="1.0">

<!--!!==========================================================================
DocBook Chunking

REMARK: Describe this module
-->


<!--@@==========================================================================
db.chunk.chunks
A space-seperated list of the names of elements that should be chunked

REMARK: This parameter sucks
-->
<xsl:param name="db.chunk.chunks" select="
           'appendix    article     bibliography  bibliodiv  book    chapter
            colophon    dedication  glossary      glossdiv   index
            lot         part        preface       refentry   reference
            sect1       sect2       sect3         sect4      sect5
            section     setindex    simplesect    toc'"/>
<xsl:variable name="db.chunk.chunks_" select="concat(' ', $db.chunk.chunks, ' ')"/>


<!--@@==========================================================================
db.chunk.chunk_top
Whether the top-level chunk should be output with the chunking mechanism

REMARK: Describe what this does
-->
<xsl:param name="db.chunk.chunk_top" select="false()"/>


<!--@@==========================================================================
db.chunk.max_depth
The maximum depth for chunking sections

REMARK: Describe what this does
-->
<xsl:param name="db.chunk.max_depth">
  <xsl:choose>
    <xsl:when test="number(processing-instruction('db.chunk.max_depth'))">
      <xsl:value-of
       select="number(processing-instruction('db.chunk.max_depth'))"/>
    </xsl:when>
    <xsl:when test="/book">
      <xsl:value-of select="2"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="1"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:param>


<!--@@==========================================================================
db.chunk.basename
The base filename of the output file, without an extension

REMARK: Describe what this does
-->
<xsl:param name="db.chunk.basename" select="/*/@id"/>


<!--@@==========================================================================
db.chunk.extension
The default file extension for new output documents

REMARK: Describe what this does
-->
<xsl:param name="db.chunk.extension"/>


<!--@@==========================================================================
db.chunk.info_chunk
Whether to create a chunk for the title page

REMARK: Describe what this does
-->
<xsl:param name="db.chunk.info_chunk" select="$db.chunk.max_depth != 0"/>


<!--@@==========================================================================
db.chunk.info_basename
The base filename for the title page

REMARK: Describe what this does
-->
<xsl:param name="db.chunk.info_basename">
  <xsl:choose>
    <xsl:when test="$db.chunk.basename">
      <xsl:value-of select="concat($db.chunk.basename, '-info')"/>
    </xsl:when>
    <xsl:otherwise>info</xsl:otherwise>
  </xsl:choose>
</xsl:param>


<!--@@==========================================================================
db.chunk.doctype_public
The public DOCTYPE for output files

REMARK: Describe this
-->
<xsl:param name="db.chunk.doctype_public"/>


<!--@@==========================================================================
db.chunk.doctype_system
The system DOCTYPE for output files

REMARK: Describe this
-->
<xsl:param name="db.chunk.doctype_system"/>


<!--**==========================================================================
db.chunk
Creates a new page of output
$node: The source element for the output page
$template: The named template to call to create the page
$href: The name of the file for the output page
$depth_of_chunk: The depth of this chunk in the document

REMARK: We need a lot more explanation about chunk flow

The *{db.chunk} template creates a new output document using the #{exsl:document}
extension element.  This template calls *{db.chunk.content} to create the content
of the document, passing through all parameters.  This allows you to override the
chunking mechanism without having to duplicate the content-generation code.
-->
<xsl:template name="db.chunk">
  <xsl:param name="node" select="."/>
  <xsl:param name="template"/>
  <xsl:param name="href">
    <xsl:choose>
      <xsl:when test="$template = 'info'">
        <xsl:value-of select="$db.chunk.info_basename"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="db.chunk.chunk-id">
          <xsl:with-param name="node" select="$node"/>
          <xsl:with-param name="depth_in_chunk" select="0"/>
          <xsl:with-param name="chunk" select="$node"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="$db.chunk.extension"/>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <exsl:document href="{$href}"
                 doctype-public="{$db.chunk.doctype_public}"
                 doctype-system="{$db.chunk.doctype_system}">
    <xsl:call-template name="db.chunk.content">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="template" select="$template"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    </xsl:call-template>
  </exsl:document>
  <xsl:if test="string($template) = ''">
    <xsl:call-template name="db.chunk.children">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>


<!--**==========================================================================
db.chunk.content
Creates the content of a new page of output
$node: The source element for the output page
$template: The named template to call to create the page
$depth_of_chunk: The depth of this chunk in the document

REMARK: We need a lot more explanation about chunk flow

The *{db.chunk.content} template creates the actual content of a new output page.
It should generally only be called by *{db.chunk}.

This template will always pass the ${depth_in_chunk} and ${depth_of_chunk}
parameters with appropriate values to the templates it calls.
-->
<xsl:template name="db.chunk.content">
  <xsl:param name="node" select="."/>
  <xsl:param name="template"/>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="$template = 'info'">
      <xsl:apply-templates mode="db.chunk.info.content.mode" select="$node">
        <xsl:with-param name="depth_in_chunk" select="0"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates mode="db.chunk.content.mode" select="$node">
        <xsl:with-param name="depth_in_chunk" select="0"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:apply-templates>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db.chunk.children
Creates new output pages for the children of an element
$node: The parent element whose children will be chunked
$depth_of_chunk: The depth of ${node} in the document

REMARK: We need a lot more explanation about chunk flow
-->
<xsl:template name="db.chunk.children">
  <xsl:param name="node" select="."/>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:if test="$depth_of_chunk &lt; $db.chunk.max_depth">
    <xsl:for-each select="
                  $node/appendix   | $node/article    | $node/bibliography |
                  $node/bibliodiv  |
                  $node/book       | $node/chapter    | $node/colophon     |
                  $node/dedication | $node/glossary   | $node/glossdiv     |
                  $node/index      | $node/lot        | $node/part         |
                  $node/preface    | $node/refentry   | $node/reference    |
                  $node/sect1      | $node/sect2      | $node/sect3        |
                  $node/sect4      | $node/sect5      | $node/section      |
                  $node/setindex   | $node/simplesect | $node/toc          ">
      <xsl:call-template name="db.chunk">
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk + 1"/>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:if>
  <xsl:if test="$db.chunk.info_chunk and $depth_of_chunk = 0">
    <xsl:call-template name="db.chunk">
      <xsl:with-param name="node" select="$node"/>
      <xsl:with-param name="template" select="'info'"/>
      <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk + 1"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>


<!--**==========================================================================
db.chunk.depth-in-chunk
Determines the depth of an element in the containing chunk
$node: The element to determine the depth of

REMARK: Explain how this works
-->
<xsl:template name="db.chunk.depth-in-chunk">
  <xsl:param name="node" select="."/>
  <xsl:variable name="divs"
                select="
                        count($node/ancestor-or-self::appendix     ) + 
                        count($node/ancestor-or-self::article      ) + 
                        count($node/ancestor-or-self::bibliography ) + 
                        count($node/ancestor-or-self::bibliodiv    ) +
                        count($node/ancestor-or-self::book         ) + 
                        count($node/ancestor-or-self::chapter      ) + 
                        count($node/ancestor-or-self::colophon     ) + 
                        count($node/ancestor-or-self::dedication   ) + 
                        count($node/ancestor-or-self::glossary     ) + 
                        count($node/ancestor-or-self::glossdiv     ) + 
                        count($node/ancestor-or-self::index        ) + 
                        count($node/ancestor-or-self::lot          ) + 
                        count($node/ancestor-or-self::part         ) + 
                        count($node/ancestor-or-self::preface      ) + 
                        count($node/ancestor-or-self::refentry     ) + 
                        count($node/ancestor-or-self::reference    ) + 
                        count($node/ancestor-or-self::sect1        ) + 
                        count($node/ancestor-or-self::sect2        ) + 
                        count($node/ancestor-or-self::sect3        ) + 
                        count($node/ancestor-or-self::sect4        ) + 
                        count($node/ancestor-or-self::sect5        ) + 
                        count($node/ancestor-or-self::section      ) + 
                        count($node/ancestor-or-self::setindex     ) + 
                        count($node/ancestor-or-self::simplesect   ) + 
                        count($node/ancestor-or-self::toc          )"/>
  <xsl:choose>
    <xsl:when test="$divs &lt; ($db.chunk.max_depth + 1)">
      <xsl:value-of select="count($node/ancestor-or-self::*) - $divs"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="count($node/ancestor::*) - $db.chunk.max_depth"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db.chunk.depth-of-chunk
Determines the depth of teh containing chunk in the document
$node: The element to determine the depth of

REMARK: Explain how this works
-->
<xsl:template name="db.chunk.depth-of-chunk">
  <xsl:param name="node" select="."/>
  <xsl:variable name="divs"
                select="$node/ancestor-or-self::*
                         [contains($db.chunk.chunks_,
                            concat(' ', local-name(.), ' '))]"/>
  <xsl:choose>
    <xsl:when test="count($divs) - 1 &lt; $db.chunk.max_depth">
      <xsl:value-of select="count($divs) - 1"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$db.chunk.max_depth"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db.chunk.chunk-id
Determines the id of the chunk that contains an element
$id: The id of the element to determine the chunk id of
$node: The element to determine the chunk id of
$depth_in_chunk: The depth of ${node} in the containing chunk

REMARK: Explain how this works
-->
<xsl:template name="db.chunk.chunk-id">
  <xsl:param name="id" select="@id"/>
  <xsl:param name="node" select="key('idkey', $id)"/>
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="chunk" select="$node/ancestor-or-self::*[$depth_in_chunk + 1]"/>
  <xsl:choose>
    <xsl:when test="set:has-same-node($chunk, /*)">
      <xsl:value-of select="$db.chunk.basename"/>
    </xsl:when>
    <xsl:when test="$chunk/@id">
      <xsl:value-of select="string($chunk/@id)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="generate-id($chunk)"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--**==========================================================================
db.chunk.chunk-id.axis
Determines the id of the first chunk along a specified axis
$node: The base element
$node: The axis along which to find the first chunk
$depth_in_chunk: The depth of ${node} in the containing chunk
$depth_of_chunk: The depth of the containing chunk in the document

REMARK: Explain how this works, and what the axes are
-->
<xsl:template name="db.chunk.chunk-id.axis">
  <xsl:param name="node" select="."/>
  <xsl:param name="axis"/>
  <xsl:param name="depth_in_chunk">
    <xsl:call-template name="db.chunk.depth-in-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:param name="depth_of_chunk">
    <xsl:call-template name="db.chunk.depth-of-chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:param>
  <xsl:choose>
    <xsl:when test="depth_in_chunk != 0">
      <xsl:call-template name="db.chunk.chunk-id.axis">
        <xsl:with-param name="node" select="$node/ancestor::*[$depth_in_chunk]"/>
        <xsl:with-param name="axis" select="$axis"/>
        <xsl:with-param name="depth_in_chunk" select="0"/>
        <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
      </xsl:call-template>
    </xsl:when>
    <!-- following -->
    <xsl:when test="$axis = 'following'">
      <xsl:variable name="divs"
                    select="$node/following-sibling::*
                             [contains($db.chunk.chunks_,
                                concat(' ', local-name(.), ' '))]"/>
      <xsl:choose>
        <xsl:when test="$divs">
          <xsl:call-template name="db.chunk.chunk-id">
            <xsl:with-param name="node" select="$divs[1]"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="chunk" select="$divs[1]"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$node/..">
          <xsl:call-template name="db.chunk.chunk-id.axis">
            <xsl:with-param name="node" select="$node/.."/>
            <xsl:with-param name="axis" select="'following'"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk - 1"/>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- last-descendant -->
    <xsl:when test="$axis = 'last-descendant'">
      <xsl:variable name="divs"
                    select="$node/*[contains($db.chunk.chunks_,
                                      concat(' ', local-name(.), ' '))]"/>
      <xsl:choose>
        <xsl:when test="($depth_of_chunk &gt;= $db.chunk.max_depth)">
          <xsl:call-template name="db.chunk.chunk-id">
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="chunk" select="$node"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="($depth_of_chunk + 1 = $db.chunk.max_depth) and $divs">
          <xsl:call-template name="db.chunk.chunk-id">
            <xsl:with-param name="node" select="$divs[last()]"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="chunk" select="$divs[last()]"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$divs">
          <xsl:call-template name="db.chunk.chunk-id.axis">
            <xsl:with-param name="node" select="$divs[last()]"/>
            <xsl:with-param name="axis" select="'last-descendant'"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk + 1"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="db.chunk.chunk-id">
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="chunk" select="$node"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- next -->
    <xsl:when test="$axis = 'next'">
      <xsl:variable name="divs"
                    select="$node/*[contains($db.chunk.chunks_,
                                      concat(' ', local-name(.), ' '))]"/>
      <xsl:choose>
        <xsl:when test="($depth_of_chunk &lt; $db.chunk.max_depth) and $divs">
          <xsl:call-template name="db.chunk.chunk-id">
            <xsl:with-param name="node" select="$divs[1]"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="chunk" select="$divs[1]"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="db.chunk.chunk-id.axis">
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="axis" select="'following'"/>
            <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
            <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- previous -->
    <xsl:when test="$axis = 'previous'">
      <xsl:variable name="divs"
                    select="$node/preceding-sibling::*
                             [contains($db.chunk.chunks_,
                                concat(' ', local-name(.), ' '))]"/>
      <xsl:choose>
        <xsl:when test="$divs and ($depth_of_chunk &lt; $db.chunk.max_depth)">
          <xsl:call-template name="db.chunk.chunk-id.axis">
            <xsl:with-param name="node" select="$divs[last()]"/>
            <xsl:with-param name="axis" select="'last-descendant'"/>
            <xsl:with-param name="depth_in_chunk" select="$depth_in_chunk"/>
            <xsl:with-param name="depth_of_chunk" select="$depth_of_chunk"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$divs">
          <xsl:call-template name="db.chunk.chunk-id">
            <xsl:with-param name="node" select="$divs[last()]"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="chunk" select="$divs[last()]"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$node/..">
          <xsl:call-template name="db.chunk.chunk-id">
            <xsl:with-param name="node" select="$node"/>
            <xsl:with-param name="depth_in_chunk" select="0"/>
            <xsl:with-param name="chunk" select="$node"/>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- unsupported -->
    <xsl:otherwise>
      <xsl:message>
        <xsl:text>Unsupported axis: </xsl:text>
        <xsl:value-of select="$axis"/>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--%%==========================================================================
db.chunk.info.content.mode
Renders the contents of the title page
$depth_in_chunk: The depth of the element in the containing chunk
$depth_of_chunk: The depth of the containing chunk in the document

When processed in this mode, a division element should output the top-level
markup for the output page.
-->
<xsl:template mode="db.chunk.info.content.mode" match="*"/>


<!--%%==========================================================================
db.chunk.content.mode
Renders the entire contents of the chunk
$depth_in_chunk: The depth of the element in the containing chunk
$depth_of_chunk: The depth of the containing chunk in the document

When processed in this mode, a division element should output the top-level
markup for the output page.
-->
<xsl:template mode="db.chunk.content.mode" match="*"/>


<!-- == Matched Templates == -->

<xsl:template match="/">
  <xsl:choose>
    <xsl:when test="$db.chunk.chunk_top">
      <xsl:call-template name="db.chunk">
        <xsl:with-param name="node" select="*[1]"/>
        <xsl:with-param name="depth_of_chunk" select="0"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates mode="db.chunk.content.mode" select="*">
        <xsl:with-param name="depth_in_chunk" select="0"/>
        <xsl:with-param name="depth_of_chunk" select="0"/>
      </xsl:apply-templates>
      <xsl:call-template name="db.chunk.children">
        <xsl:with-param name="node" select="*[1]"/>
        <xsl:with-param name="depth_of_chunk" select="0"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
