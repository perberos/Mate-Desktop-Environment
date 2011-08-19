## Process this file with automake to produce Makefile.in

diagrams = persist.dia persist-file.dia persist-storage.dia \
	   persist-stream.dia storage.dia stream.dia unknown.dia \
	   running-context.dia


diagrams_png = $(diagrams:.dia=.png)

all-local: $(diagrams_png)

# NOTE: make sure you have dia >= 0.91 and DISPLAY set
# NOTE: if you edit the dia files, save them UNcompressed!
%.png: %.dia
	$(AM_V_GEN) dia $< -e $@ 2> /dev/null || true


EXTRA_DIST = $(diagrams) $(diagrams_png)

