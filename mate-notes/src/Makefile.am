

SUBDIRS = addins

EXTRA_DIST = gnote-applet.in

INCLUDES = -I$(top_srcdir)
AM_CPPFLAGS=@LIBGTKMM_CFLAGS@ @LIBGLIBMM_CFLAGS@ \
	@MATECONF_CFLAGS@ @GTK_CFLAGS@ \
	@GTKSPELL_CFLAGS@ @LIBXML_CFLAGS@ \
	@LIBXSLT_CFLAGS@ \
	@PCRE_CFLAGS@ \
	@DBUS_CFLAGS@ \
	-DGNOTE_LOCALEDIR=\"@GNOTE_LOCALEDIR@\" \
	-DDATADIR=\"$(datadir)\" -DLIBDIR=\"$(libdir)\"

AM_LDFLAGS=-export-dynamic

GNOTE_LIBS = libgnote.a $(top_builddir)/libtomboy/libtomboy.la \
	@LIBGLIBMM_LIBS@ @LIBGTKMM_LIBS@ \
	@MATECONF_LIBS@ \
	@LIBXSLT_LIBS@ \
	@PCRE_LIBS@ \
	@DBUS_LIBS@ \
	@GTKSPELL_LIBS@ @GTK_LIBS@ \
	-luuid

noinst_LIBRARIES = libgnote.a
bin_PROGRAMS = gnote
check_PROGRAMS = trietest stringtest notetest dttest uritest filestest \
	fileinfotest xmlreadertest
TESTS = trietest stringtest notetest dttest uritest filestest \
	fileinfotest xmlreadertest


trietest_SOURCES = test/trietest.cpp \
	sharp/string.cpp debug.cpp
trietest_LDADD =  @PCRE_LIBS@ @LIBGLIBMM_LIBS@

dttest_SOURCES = test/dttest.cpp \
	sharp/datetime.cpp debug.cpp
dttest_LDADD = @LIBGLIBMM_LIBS@

stringtest_SOURCES = test/stringtest.cpp \
	sharp/string.cpp debug.cpp
stringtest_LDADD =  @PCRE_LIBS@ @LIBGLIBMM_LIBS@

filestest_SOURCES = test/filestest.cpp \
	sharp/files.cpp
filestest_LDADD = @LIBGLIBMM_LIBS@ -lgiomm-2.4

fileinfotest_SOURCES = test/fileinfotest.cpp \
	sharp/fileinfo.cpp \
	sharp/datetime.cpp
fileinfotest_LDADD = @LIBGLIBMM_LIBS@ -lgiomm-2.4

uritest_SOURCES = test/uritest.cpp \
	sharp/string.cpp  sharp/uri.cpp debug.cpp
uritest_LDADD =  @PCRE_LIBS@ @LIBGLIBMM_LIBS@

xmlreadertest_SOURCES = test/xmlreadertest.cpp \
	sharp/xmlreader.cpp debug.cpp
xmlreadertest_LDADD = @LIBXML_LIBS@

notetest_SOURCES = test/notetest.cpp
notetest_LDADD =  $(GNOTE_LIBS) -lX11

gnote_SOURCES = main.cpp


if HAVE_PANELAPPLETMM
APPLET_SOURCES=applet.hpp applet.cpp
AM_CPPFLAGS+=@LIBPANELAPPLETMM_CFLAGS@
GNOTE_LIBS+=@LIBPANELAPPLETMM_LIBS@

libexec_SCRIPTS=gnote-applet
gnote-applet: gnote-applet.in Makefile
	sed -e "s|\@bindir\@|$(bindir)|g"	\
	    < $< > $@

CLEANFILES = gnote-applet
endif


if HAVE_DBUS
SUBDIRS += dbus
DBUS_SOURCES=remotecontrolproxy.hpp remotecontrolproxy.cpp \
	dbus/remotecontrol.hpp dbus/remotecontrol.cpp \
	dbus/remotecontrolclient.hpp dbus/remotecontrolclient.cpp \
	dbus/iremotecontrol.hpp \
	$(NULL)
endif

libgnote_a_SOURCES = \
	base/singleton.hpp \
	base/macros.hpp \
	base/inifile.hpp base/inifile.cpp \
	sharp/addinstreemodel.hpp sharp/addinstreemodel.cpp \
	sharp/datetime.hpp sharp/datetime.cpp \
	sharp/dynamicmodule.hpp sharp/dynamicmodule.cpp \
	sharp/directory.hpp sharp/directory.cpp \
	sharp/exception.hpp sharp/exception.cpp \
	sharp/fileinfo.hpp sharp/fileinfo.cpp \
	sharp/files.hpp sharp/files.cpp \
	sharp/map.hpp \
	sharp/modulefactory.hpp \
	sharp/modulemanager.hpp sharp/modulemanager.cpp \
	sharp/propertyeditor.hpp sharp/propertyeditor.cpp \
	sharp/streamreader.hpp sharp/streamreader.cpp \
	sharp/streamwriter.hpp sharp/streamwriter.cpp \
	sharp/string.hpp sharp/string.cpp \
	sharp/uri.hpp sharp/uri.cpp \
	sharp/uuid.hpp \
	sharp/xml.hpp sharp/xml.cpp \
	sharp/xmlconvert.hpp sharp/xmlconvert.cpp \
	sharp/xmlreader.hpp sharp/xmlreader.cpp \
	sharp/xmlresolver.hpp \
	sharp/xmlwriter.hpp sharp/xmlwriter.cpp \
	sharp/xsltargumentlist.hpp sharp/xsltargumentlist.cpp \
	sharp/xsltransform.hpp sharp/xsltransform.cpp \
	gnote.cpp gnote.hpp \
	abstractaddin.hpp abstractaddin.cpp \
	actionmanager.hpp actionmanager.cpp \
	addinmanager.hpp addinmanager.cpp \
	addinpreferencefactory.hpp addinpreferencefactory.cpp \
	applicationaddin.hpp \
	applicationaddin.cpp \
	contrast.hpp contrast.cpp \
	debug.hpp debug.cpp \
	importaddin.hpp importaddin.cpp \
	keybinder.hpp \
	noteaddin.hpp noteaddin.cpp \
	notebuffer.hpp notebuffer.cpp \
	noteeditor.hpp noteeditor.cpp \
	notemanager.hpp notemanager.cpp \
	noterenamedialog.hpp noterenamedialog.cpp \
	notetag.hpp notetag.cpp \
	note.hpp note.cpp \
	notewindow.hpp notewindow.cpp \
	preferences.hpp preferences.cpp \
	preferencesdialog.hpp preferencesdialog.cpp \
	preferencetabaddin.hpp \
	prefskeybinder.hpp prefskeybinder.cpp \
	recenttreeview.hpp \
	recentchanges.hpp recentchanges.cpp \
	search.hpp search.cpp \
	tag.hpp tag.cpp \
	tagmanager.hpp tagmanager.cpp \
	tray.hpp tray.cpp \
	trie.hpp triehit.hpp \
	undo.hpp undo.cpp \
	utils.hpp utils.cpp \
	watchers.hpp watchers.cpp \
	xkeybinder.hpp xkeybinder.cpp \
	notebooks/createnotebookdialog.hpp notebooks/createnotebookdialog.cpp \
	notebooks/notebook.hpp notebooks/notebook.cpp \
	notebooks/notebookapplicationaddin.hpp notebooks/notebookapplicationaddin.cpp \
	notebooks/notebookmanager.hpp notebooks/notebookmanager.cpp \
	notebooks/notebookmenuitem.hpp notebooks/notebookmenuitem.cpp \
	notebooks/notebooknewnotemenuitem.hpp notebooks/notebooknewnotemenuitem.cpp \
	notebooks/notebooknoteaddin.hpp notebooks/notebooknoteaddin.cpp \
	notebooks/notebookstreeview.hpp notebooks/notebookstreeview.cpp \
	$(DBUS_SOURCES)   \
	$(APPLET_SOURCES) \
	$(NULL)

gnote_LDADD = \
	$(GNOTE_LIBS) \
	-lX11 \
	$(NULL)
