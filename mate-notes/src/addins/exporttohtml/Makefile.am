
include $(builddir)/../addins.mk


INCLUDES = @LIBXSLT_CFLAGS@

addinsdir = $(ADDINSDIR)
addins_LTLIBRARIES = exporttohtml.la

xsldir = @datadir@/gnote
xsl_DATA = exporttohtml.xsl

exporttohtml_la_LIBADD = @LIBXSLT_LIBS@

exporttohtml_la_SOURCES = exporttohtmlnoteaddin.hpp exporttohtmlnoteaddin.cpp \
	exporttohtmldialog.hpp exporttohtmldialog.cpp \
	notenameresolver.hpp \
	$(NULL)

EXTRA_DIST = exporttohtml.xsl
