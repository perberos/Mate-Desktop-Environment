NULL =

enabled_plugins =	\
	a11y-keyboard	\
	background	\
	clipboard	\
	datetime	\
	dummy		\
	font		\
	housekeeping    \
	keybindings	\
	keyboard	\
	media-keys	\
	mouse		\
	sound		\
	typing-break	\
	xrandr		\
	xrdb		\
	xsettings	\
	$(NULL)

disabled_plugins = $(NULL)

if SMARTCARD_SUPPORT
enabled_plugins += smartcard
else
disabled_plugins += smartcard
endif

SUBDIRS = common $(enabled_plugins)
DIST_SUBDIRS = $(SUBDIRS) $(disabled_plugins)
