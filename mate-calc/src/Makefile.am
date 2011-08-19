bin_PROGRAMS = gcalctool gcalccmd

INCLUDES = \
	-DUI_DIR=\""$(datadir)/gcalctool"\" \
	-DVERSION=\""$(VERSION)"\" \
	-DLOCALE_DIR=\""$(localedir)"\" \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	$(WARN_CFLAGS) \
	$(GCALCTOOL_CFLAGS)

gcalctool_SOURCES = \
	currency.c \
	currency.h \
	gcalctool.c \
	math-buttons.c \
	math-buttons.h \
	math-display.c \
	math-display.h \
	math-equation.c \
	math-equation.h \
	math-preferences.c \
	math-preferences.h \
	math-variables.c \
	math-variables.h \
	math-window.c \
	math-window.h \
	mp.c \
	mp.h \
	mp-binary.c \
	mp-convert.c \
	mp-private.h \
	mp-trigonometric.c \
	mp-equation.c \
	mp-equation.h \
	mp-equation-private.h \
	mp-equation-lexer.c \
	mp-equation-lexer.h \
	mp-equation-parser.c \
	mp-equation-parser.h \
	financial.c \
	financial.h \
	unittest.c \
	unittest.h

gcalctool_LDADD = \
	$(GCALCTOOL_LIBS)

gcalccmd_SOURCES = \
	gcalccmd.c \
	mp.c \
	mp-convert.c \
	mp-binary.c \
	mp-trigonometric.c \
	mp-equation.c \
	mp-equation-parser.c \
	mp-equation-lexer.c

gcalccmd_LDADD = \
	$(GCALCCMD_LIBS) \
	-lm

CLEANFILES = \
	mp-equation-parser.h \
	mp-equation-parser.c \
	mp-equation-lexer.c \
	mp-equation-lexer.h

# Generate parser files
mp-equation-parser.c mp-equation-parser.h: mp-equation-parser.y mp-equation-lexer.h
	$(AM_V_GEN)$(YACC) -d -o mp-equation-parser.c $(srcdir)/mp-equation-parser.y

# Generate lexer files
mp-equation-lexer.c mp-equation-lexer.h: mp-equation-lexer.l
	$(AM_V_GEN)$(LEX) $(srcdir)/mp-equation-lexer.l

# Rebuild parser when source files change
mp-equation-parser.o: mp-equation-lexer.h
mp-equation-lexer.o: mp-equation-parser.h
mp-equation.c: mp-equation-lexer.h mp-equation-parser.h

# Install a symlink between gcalctool and mate-calculator
install-exec-hook:
	test -e "$(DESTDIR)$(bindir)/mate-calculator" \
	|| (cd "$(DESTDIR)$(bindir)" && ln -s gcalctool mate-calculator)

# Remove the symlink between gcalctool and mate-calculator
uninstall-local:
	test -h "$(DESTDIR)$(bindir)/mate-calculator" \
	&& rm -f "$(DESTDIR)$(bindir)/mate-calculator"

EXTRA_DIST = \
	mp-equation-parser.y \
	mp-equation-lexer.l

DISTCLEANFILES = \
	Makefile.in

test: gcalctool
	./gcalctool -u
