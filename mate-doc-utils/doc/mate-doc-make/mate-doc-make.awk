BEGIN { print "<section id='reference'>" }
BEGIN { print "<title>Variable Reference</title>" }

/^[^\#]/ {
  if (topara) {
    topara = 0;
    inpara = 0;
    invarp = 0;
  }
  if (inpara) {
    print "  </para>";
    inpara = 0;
    invarp = 0;
  }
  if (invarp) {
    print "    </para></listitem>";
    print "  </varlistentry>";
    invarp = 0;
  }
}

/^\#\# [^@]/ {
  if (topara) {
    print "  <para>";
    topara = 0;
    inpara = 1;
    invarp = 0;
  }
  if (inpara || invarp) {
    sub(/^\#\# /, "    ", $0);
    print $0;
  }
}

/^\#\# @ / {
  topara = 0;
  if (inpara) {
    print "  </para>";
    inpara = 0;
  }
  if (invarp) {
    print "    </para></listitem>";
    print "  </varlistentry>";
    invarp = 0;
  }
  sub(/^\#\# @ /, "", $0);
  print "  <varlistentry>";
  print "    <term><parameter>" $0 "</parameter></term>";
  print "    <listitem><para>";
  invarp = 1;
}

/^\#\# @@ / {
  if (inpara) {
    print "  </para>";
    inpara = 0;
  }
  if (invarp) {
    print "    </para></listitem>";
    print "  </varlistentry>";
    invarp = 0;
  }
  if (insect) print "</variablelist>";
  sub(/^\#\# @@ /, "", $0);
  print "<variablelist>";
  print "  <title>" $0 "</title>";
  insect = 1;
  topara = 1;
}

END {
  if (inpara) {
    print "  </para>";
    inpara = 0;
  }
  if (invarp) {
    print "    </para></listitem>";
    print "  </varlistentry>";
    invarp = 0;
  }
  if(insect) print "</variablelist>";
  print "</section>";
}
