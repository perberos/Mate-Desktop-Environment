INCLUDES = -DGTK_DISABLE_DEPRECATED \
           -DGDK_DISABLE_DEPRECATED \
           -DG_DISABLE_DEPRECATED


noinst_LTLIBRARIES = libmateui-deprecated.la

libmateui_deprecated_la_LIBADD = $(MATE_UTILS_LIBS)
libmateui_deprecated_la_CFLAGS = $(MATE_UTILS_CFLAGS) \
                                  $(WARN_CFLAGS)
libmateui_deprecated_la_SOURCES = gsearchtool-entry.h \
                                   gsearchtool-entry.c
