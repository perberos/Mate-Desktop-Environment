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

<!--
If you add a template to this stylesheet, put it under an appropriate header
that states why this element is suppressed.  Some elements are simply not
supported, while other elements are expected only to be processed in certain
modes because of the DocBook content model.
-->

<!-- Not rendered directly -->
<xsl:template match="titleabbrev"/>

<!-- Not yet supported -->
<xsl:template match="remark"/>

<!-- Suppressed by processing expectations -->
<xsl:template match="screeninfo"/>

<!-- Not supported -->
<xsl:template match="alt"/>
<xsl:template match="beginpage"/>
<xsl:template match="bibliocoverage"/>
<xsl:template match="bridgehead"/>

<!-- Explicitly matched by parent -->
<xsl:template match="listitem"/>
<xsl:template match="sbr"/>

<!-- Index elements, not yet supported -->
<xsl:template match="indexterm"/>
<xsl:template match="primary"/>
<xsl:template match="primaryie"/>
<xsl:template match="secondary"/>
<xsl:template match="secondaryie"/>
<xsl:template match="see"/>
<xsl:template match="seeie"/>
<xsl:template match="seealso"/>
<xsl:template match="seealsoie"/>
<xsl:template match="tertiary"/>
<xsl:template match="tertiaryie"/>

<!-- Unmatched info elements, supported indirectly -->
<xsl:template match="appendixinfo"/>
<xsl:template match="blockinfo"/>
<xsl:template match="articleinfo"/>
<xsl:template match="bibliographyinfo"/>
<xsl:template match="bookinfo"/>
<xsl:template match="chapterinfo"/>
<xsl:template match="glossaryinfo"/>
<xsl:template match="partinfo"/>
<xsl:template match="prefaceinfo"/>
<xsl:template match="refentryinfo"/>
<xsl:template match="referenceinfo"/>
<xsl:template match="refmeta"/>
<xsl:template match="refmiscinfo"/>
<xsl:template match="refsect1divinfo"/>
<xsl:template match="refsect2divinfo"/>
<xsl:template match="refsect3divinfo"/>
<xsl:template match="refsectiondivinfo"/>
<xsl:template match="refsynopsisdivinfo"/>
<xsl:template match="sect1info"/>
<xsl:template match="sect2info"/>
<xsl:template match="sect3info"/>
<xsl:template match="sect4info"/>
<xsl:template match="sect5info"/>
<xsl:template match="sectioninfo"/>

<!-- Only occur in db2html.info.mode -->
<xsl:template match="affiliation"/>
<xsl:template match="author"/>
<xsl:template match="authorblurb"/>
<xsl:template match="authorgroup"/>
<xsl:template match="collab"/>
<xsl:template match="collabname"/>
<xsl:template match="copyright"/>
<xsl:template match="editor"/>
<xsl:template match="legalnotice"/>
<xsl:template match="othercredit"/>
<xsl:template match="personblurb"/>
<xsl:template match="publisher"/>
<xsl:template match="revdescription"/>
<xsl:template match="revhistory"/>
<xsl:template match="revision"/>
<xsl:template match="revnumber"/>
<xsl:template match="revremark"/>

<!-- Handled specially, so we can apply-templates -->
<xsl:template match="label"/>

</xsl:stylesheet>
