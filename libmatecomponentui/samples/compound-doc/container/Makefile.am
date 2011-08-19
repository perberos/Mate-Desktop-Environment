INCLUDES =	\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	$(MATECOMPONENTUI_CFLAGS)

noinst_PROGRAMS = sample-container-2

sample_container_2_SOURCES =	\
	container.c		\
	container.h		\
	component.c		\
	component.h		\
	container-filesel.h	\
	container-filesel.c	\
	container-menu.h	\
	container-menu.c	\
	document.c		\
	document.h		\
	doc-view.c		\
	doc-view.h

sample_container_2_LDADD =				\
	$(top_builddir)/matecomponent/libmatecomponentui-2.la		\
	$(MATECOMPONENTUI_LIBS)				\
	$(INTLLIBS)
