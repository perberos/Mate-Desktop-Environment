
gsettings_SCHEMAS = \
	org.mate.crypto.cache.gschema.xml \
	org.mate.crypto.pgp.gschema.xml

@GSETTINGS_RULES@

convert_DATA = \
	org.mate.crypto.cache.convert \
	org.mate.crypto.pgp.convert

convertdir = \
	$(datarootdir)/MateConf/gsettings

EXTRA_DIST = \
	$(convert_DATA) \
	$(gsettings_SCHEMAS)