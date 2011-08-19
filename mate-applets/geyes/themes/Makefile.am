#SUBDIRS = Default Default-tiny Bizarre Bloodshot Horrid

Defaultdir = $(pkgdatadir)/geyes/Default
Default_DATA = Default/config                          \
               Default/Default-eye.png                 \
	       Default/Default-pupil.png               

Tangodir = $(pkgdatadir)/geyes/Tango
Tango_DATA = Tango/config		\
	     Tango/tango-eye.png	\
	     Tango/tango-pupil.png

Default_tinydir = $(pkgdatadir)/geyes/Default-tiny
Default_tiny_DATA = Default-tiny/config                     \
                    Default-tiny/Default-tiny-eye.png       \
	            Default-tiny/Default-tiny-pupil.png 

Bizarredir = $(pkgdatadir)/geyes/Bizarre
Bizarre_DATA = Bizarre/config                          \
               Bizarre/Bizarre-eye.png                 \
	       Bizarre/Bizarre-pupil.png

Bloodshotdir = $(pkgdatadir)/geyes/Bloodshot
Bloodshot_DATA = Bloodshot/config                        \
                 Bloodshot/Bloodshot-eye.png             \
	         Bloodshot/Bloodshot-pupil.png           

Horriddir = $(pkgdatadir)/geyes/Horrid
Horrid_DATA = Horrid/config                           \
              Horrid/horrid_eye.png                   \
	      Horrid/horrid_pupil.png

EXTRA_DIST = $(Default_DATA) $(Default_tiny_DATA) $(Bizarre_DATA) $(Bloodshot_DATA) \
             $(Horrid_DATA) $(Tango_DATA)

-include $(top_srcdir)/git.mk
