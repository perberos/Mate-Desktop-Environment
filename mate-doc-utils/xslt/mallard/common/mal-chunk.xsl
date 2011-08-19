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
                extension-element-prefixes="exsl"
                version="1.0">

<!--!!==========================================================================
Chunking

REMARK: Describe this module
-->


<!--@@==========================================================================
mal.chunk.chunk_top
Whether the top-level page should be output with the chunking mechanism

REMARK: Describe what this does
-->
<xsl:param name="mal.chunk.chunk_top" select="false()"/>


<!--@@==========================================================================
mal.chunk.extension
The default file extension for new output documents

REMARK: Describe what this does
-->
<xsl:param name="mal.chunk.extension"/>


<!--@@==========================================================================
mal.chunk.doctype_public
The public DOCTYPE for output files

REMARK: Describe this
-->
<xsl:param name="mal.chunk.doctype_public"/>


<!--@@==========================================================================
mal.chunk.doctype_system
The system DOCTYPE for output files

REMARK: Describe this
-->
<xsl:param name="mal.chunk.doctype_system"/>


<!--**==========================================================================
mal.chunk
Creates a new page of output
$node: The source element for the output page
$href: The name of the file for the output page

REMARK: Describe
-->
<xsl:template name="mal.chunk">
  <xsl:param name="node" select="."/>
  <xsl:param name="href" select="concat($node/@id, $mal.chunk.extension)"/>
  <exsl:document href="{$href}"
                 doctype-public="{$mal.chunk.doctype_public}"
                 doctype-system="{$mal.chunk.doctype_system}">
    <xsl:apply-templates mode="mal.chunk.content.mode" select="$node"/>
  </exsl:document>
</xsl:template>

<!--%%==========================================================================
mal.chunk.content.mode
FIXME

FIXME
-->

</xsl:stylesheet>
