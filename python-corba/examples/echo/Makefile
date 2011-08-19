MATECORBA2_CFLAGS=`pkg-config --cflags MateCORBA-2.0`
MATECORBA2_LIBS=`pkg-config --libs MateCORBA-2.0`

all: echo_module.la

%.lo: %.c
	libtool --mode=compile gcc $(MATECORBA2_CFLAGS) -c $<

echo_module.la: echo-imodule.lo
	libtool --mode=link  gcc $(MATECORBA2_LIBS) -o $@ $< -rpath `pwd`\
		-export-dynamic -module -avoid-version

idlgen = echo-imodule.c echo-common.c echo.h

echo-imodule.c: echo.idl
	matecorba-idl-2 --add-imodule --noskels --nostubs $<

clean:
	-libtool --mode=clean rm -rf *.lo *.la $(idlgen)

install: echo_module.la
	libtool --mode=install install echo_module.la\
		`pkg-config --variable=libdir MateCORBA-2.0`/matecorba-2.0
