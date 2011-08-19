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
DocBook to HTML - EBNF Elements
:Requires: db2html-xref

REMARK: Describe this module
-->

<!-- FIXME: rhs/sbr -->

<!-- == Matched Templates == -->

<!-- = constraint = -->

<!-- = constraintdef = -->

<!-- = lhs = -->

<!-- = nonterminal = -->

<!-- = production = -->

<!-- = productionrecap = -->

<!-- = productionset = -->
<xsl:template match="productionset">
  <div class="productionset">
    <xsl:call-template name="db2html.anchor"/>
    <xsl:apply-templates select="title"/>
    <table class="productionset">
      <xsl:apply-templates select="production | productionrecap"/>
    </table>
  </div>
</xsl:template>

<!-- = productionset/title = -->
<!-- FIXME
<xsl:template match="productionset/title">
  <xsl:call-template name="db2html.title.simple"/>
</xsl:template>
-->

<!-- = rhs = -->

</xsl:stylesheet>
