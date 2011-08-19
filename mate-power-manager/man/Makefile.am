EXTRA_DIST =						\
	mate-power-manager.sgml			\
	mate-power-statistics.sgml			\
	mate-power-preferences.sgml

if HAVE_DOCBOOK2MAN
man_MANS =						\
	mate-power-manager.1				\
	mate-power-statistics.1			\
	mate-power-preferences.1
endif

if HAVE_DOCBOOK2MAN
mate-power-manager.1: mate-power-manager.sgml
	docbook2man $? &> /dev/null

mate-power-preferences.1: mate-power-preferences.sgml
	docbook2man $? &> /dev/null

mate-power-statistics.1: mate-power-statistics.sgml
	docbook2man $? &> /dev/null
endif

clean-local :
	rm -f *~
	rm -f *.1
	rm -f manpage.*
