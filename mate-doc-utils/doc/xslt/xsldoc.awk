#!/bin/awk
# xsldoc.awk - Convert inline documentation to XML suitable for xsldoc.xsl
# Copyright (C) 2006 Shaun McCance <shaunm@gnome.org>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# This program is free software, but that doesn't mean you should use it.
# It's a hackish bit of awk and XSLT to do inline XSLT documentation with
# a simple syntax in comments.  I had originally tried to make a public
# inline documentation system for XSLT using embedded XML, but it just got
# very cumbersome.  XSLT should have been designed from the ground up with
# an inline documentation format.
#
# None of the existing inline documentation tools (gtk-doc, doxygen, etc.)
# really work well for XSLT, so I rolled my own simple comment-based tool.
# This tool is sufficient for producing the documentation I need, but I
# just don't have the time or inclination to make a robust system suitable
# for public use.
#
# You are, of course, free to use any of this.  If you do proliferate this
# hack, it is requested (though not required, that would be non-free) that
# you atone for your actions.  A good atonement would be contributing to
# free software.


# This function takes a line of input and converts inline syntax into real
# XML.  Inline syntax marks up special things like ${this}, with a leading
# special character and the marked-up content in curly braces.  Here's the
# list of special characters and their meanings:
#
# !{}  the id of a stylesheet file, i.e. the file name without the .xsl
# *{}  the name of a named XSLT template
# %{}  the name of a defined XSLT mode
# @{}  the name of a parameter to the stylesheet
# ${}  the name of a parameter to the current template
#
# There is currently no way of escaping these sequences, but note that the
# special characters on their own do nothing.  They are only matched when
# directly preceding an opening curly brace, and when there's a matching
# closing curly brace somewhere.  The syntax is hackish.  Thus, it is not
# intended for public consumption outside mate-doc-utils
#
# Note that we need to add a special character for processing instructions.
function runline (line, ix, jx, pre, aft, char, name, id, fmt) {
  ix = match(line, /[\*\$\@\%\!\#]\{[^\}]*\}/)
  if (ix > 0) {
    jx = ix + index(substr(line, ix), "}");
    pre = substr(line, 1, ix - 1);
    aft = substr(line, jx);
    char = substr(line, ix, 1);
    name = substr(line, ix + 2, jx - ix - 3);
    id = name;
    while (sub(/[\.-]/, "_", id));
    if (char == "!")
      fmt = "<filename><link linkend='S__%s'>%s</link></filename>";
    else if (char == "*")
      fmt = "<function><link linkend='T__%s'>%s</link></function>";
    else if (char == "%")
      fmt = "<function><link linkend='M__%s'>%s</link></function>";
    else if (char == "@") 
      fmt = "<parameter><link linkend='P__%s'>%s</link></parameter>";
    else if (char == "$") 
      fmt = "<parameter>"name"</parameter>";
    else if (char == "#")
      fmt = "<literal>"name"</literal>";
    else
      fmt = name;
    return sprintf("%s%s%s",
		   runline(pre),
		   sprintf(fmt, id, name),
		   runline(aft) );
  } else {
    return line;
  }
}

BEGIN { print "<section>"; }

/<\!--\!\!/ {
  inthis = "title";
  this = "header";
  next;
}
/<\!--\*\*/ {
  print "<template>";
  inthis = "title";
  this = "template";
  next;
}
/<\!--\%\%/ {
  print "<mode>";
  inthis = "title";
  this = "mode";
  next;
}
/<\!--\@\@/ {
  print "<param>"
  inthis = "title";
  this = "param";
  next;
}
/<\!--\#\*.*-->/ {
	split($0, a);
	printf "<template private='true'><name>%s</name></template>\n", a[2];
	next;
}
/<\!--\#\%.*-->/ {
	split($0, a);
	printf "<mode private='true'><name>%s</name></mode>\n", a[2];
	next;
}
/<\!--\#\@.*-->/ {
	split($0, a);
	printf "<param private='true'><name>%s</name></param>\n", a[2];
	next;
}

/^$/ { inend = 1; }
/-->/ { inend = 1; end = 1; }

!inend && inthis == "title" {
  printf "  <name>%s</name>\n", runline($0);
  inthis = "top";
  body = 0;
  next;
}

inthis == "top" && inend {
  print "  <body>";
  body = 1;
  inthis = "body";
}
inthis == "top" && /^\$.+:.+$/ {
  print "  <params>";
  inthis = "params";
}
inthis == "top" && /^:.+:.+$/ {
  print "  <metas>";
  inthis = "meta"
}
inthis == "top" {
  printf "  <desc>%s</desc>\n\n", runline($0);
}

inthis == "params" && /^\$.+:.+$/ {
  key = $0;
  val = $0;
  sub(/^\$/, "", key);
  sub(/:.*$/, "", key);
  sub(/^[^:]*:[ \t]*/, "", val);
  printf "    <param><name>%s</name>\n", key;
  printf "      <desc>%s</desc>\n", runline(val);
  print  "    </param>";
  next;
}
inthis == "params" && /^:.+:.+$/ {
  print "  </params>\n";
  print "  <metas>";
  inthis = "meta";
}
inthis == "params" && inend {
  print "  </params>\n";
  print "  <body>";
  body = 1;
  inthis = "body";
}

inthis == "meta" && /^:.+:.+$/ {
  key = $0;
  val = $0;
  sub(/^\:/, "", key);
  sub(/:.*$/, "", key);
  sub(/^\:/, "", val);
  sub(/^[^:]*:[ \t]*/, "", val);
  printf "    <meta><name>%s</name>\n", key;
  printf "      <desc>%s</desc>\n", runline(val);
  print  "    </meta>";
  next;
}
inthis == "meta" && inend {
  print "  </metas>";
  print "  <body>";
  body = 1;
  inthis = "body";
}

inthis == "body" && /^REMARK:/ {
  sub(/^REMARK:[ \t]*/, "", $0);
  printf "  <remark>%s", runline($0);
  inthis = "para";
  inpara = "remark";
  next;
}
inthis == "body" && !inend {
  printf "  <para>%s", runline($0);
  inthis = "para";
  next;
}
inthis == "para" && inend {
  if (inpara == "remark") {
    printf "</%s>\n", inpara;
  }
  else if (inpara) {
    printf "</para></%s>\n", inpara;
  } else {
    print "</para>";
  }
  inthis = "body";
  inpara = "";
}
inthis == "para" && !inend {
  printf "\n  %s", runline($0);
  next;
}

inend { inend = 0; }
end && body { print "  </body>"; }
end && this == "template" { print "</template>\n"; }
end && this == "mode" { print "</mode>\n"; }
end && this == "param" { print "</param>\n"; }
end {
  inthis = "";
  this = "";
  body = 0; 
  end = 0;
}

END { print "</section>" }
