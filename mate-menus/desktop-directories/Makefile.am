directorydir = $(datadir)/desktop-directories

# Should be moved to mate-control-center:
directory_in_controlcenterfiles =		\
	mate-hardware.directory.in			\
	mate-internet-and-network.directory.in		\
	mate-look-and-feel.directory.in		\
	mate-personal.directory.in			\
	mate-system.directory.in

directory_in_files =				\
	mate-audio-video.directory.in			\
	mate-development.directory.in		\
	mate-education.directory.in			\
	mate-game.directory.in			\
	mate-graphics.directory.in			\
	mate-network.directory.in			\
	mate-office.directory.in			\
	mate-settings.directory.in			\
	mate-settings-system.directory.in		\
	mate-system-tools.directory.in		\
	mate-utility.directory.in			\
	mate-utility-accessibility.directory.in	\
	mate-other.directory.in		\
	mate-menu-applications.directory.in	\
	mate-menu-system.directory.in	\
	$(directory_in_controlcenterfiles)

directory_DATA = $(directory_in_files:.directory.in=.directory)

@INTLTOOL_DIRECTORY_RULE@

EXTRA_DIST= $(directory_in_files)

DISTCLEANFILES = $(directory_DATA)

-include $(top_srcdir)/git.mk
