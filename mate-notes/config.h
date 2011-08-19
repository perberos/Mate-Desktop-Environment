/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if DBus is enabled */
/* #undef ENABLE_DBUS */

/* always defined to indicate that i18n is enabled */
#define ENABLE_NLS 1

/* Set if the GtkSpell library is recent enough. */
#define FIXED_GTKSPELL 1

/* The domain to use with gettext */
#define GETTEXT_PACKAGE "gnote"

/* The string used to hardcode the build config. */
#define GNOTE_BUILD_CONFIG " gcc-options=-Wall -Wextra -Wsign-compare -Wpointer-arith -Wchar-subscripts -Wwrite-strings -Wunused -Wpointer-arith -Wshadow -fshow-column "

/* Define to 1 if you have the `bind_textdomain_codeset' function. */
#define HAVE_BIND_TEXTDOMAIN_CODESET 1

/* Defined if the requested minimum BOOST version is satisfied */
#define HAVE_BOOST 1

/* Define to 1 if you have <boost/bind.hpp> */
#define HAVE_BOOST_BIND_HPP 1

/* Define to 1 if you have <boost/cast.hpp> */
#define HAVE_BOOST_CAST_HPP 1

/* Define to 1 if you have <boost/format.hpp> */
#define HAVE_BOOST_FORMAT_HPP 1

/* Define to 1 if you have <boost/lexical_cast.hpp> */
#define HAVE_BOOST_LEXICAL_CAST_HPP 1

/* Define to 1 if you have <boost/test/unit_test.hpp> */
#define HAVE_BOOST_TEST_UNIT_TEST_HPP 1

/* Define to 1 if class Gtk::Widget has signal_popup_menu */
#define HAVE_CLASS_GTK__WIDGET_SIGNAL_POPUP_MENU 1

/* Define to 1 if you have the `dcgettext' function. */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if your <locale.h> file defines LC_MESSAGES. */
#define HAVE_LC_MESSAGES 1

/* Define to 1 if you have the `uuid' library (-luuid). */
#define HAVE_LIBUUID 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if matepanelapplet is available */
/* #undef HAVE_PANELAPPLETMM */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "gnote"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "gnote"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "gnote 0.7.4"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "gnote"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.7.4"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "0.7.4"

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */
