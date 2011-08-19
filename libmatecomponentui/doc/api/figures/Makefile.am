## Process this file with automake to produce Makefile.in

diagrams = control.dia control-frame.dia


diagrams_png = $(diagrams:.dia=.png)

all-local: $(diagrams_png)

# NOTE: make sure you have dia >= 0.90 and DISPLAY set
# NOTE: if you edit the dia files, save them UNcompressed!
%.png: %.dia
	@dia $< -e $@ 2> /dev/null || true


EXTRA_DIST = $(diagrams) $(diagrams_png)

