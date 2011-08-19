cpufreqselectordir = $(bindir)

INCLUDES = $(CPUFREQ_SELECTOR_CFLAGS)

if HAVE_POLKIT
INCLUDES += \
	$(DBUS_CFLAGS)		\
	$(POLKIT_CFLAGS)
endif

bin_PROGRAMS = cpufreq-selector

install-exec-hook:
if CPUFREQ_SELECTOR_SUID
	chmod 4755 $(DESTDIR)$(bindir)/cpufreq-selector
endif


if HAVE_LIBCPUFREQ
cpufreq_files=cpufreq-selector-libcpufreq.c        cpufreq-selector-libcpufreq.h
endif

if HAVE_POLKIT
service_files=cpufreq-selector-service.c       cpufreq-selector-service.h
endif

cpufreq_selector_SOURCES = \
	cpufreq-selector.c               cpufreq-selector.h        	\
	cpufreq-selector-sysfs.c         cpufreq-selector-sysfs.h  	\
	cpufreq-selector-procfs.c        cpufreq-selector-procfs.h 	\
	$(cpufreq_files)					   	\
	$(service_files)						\
	cpufreq-selector-factory.c	 cpufreq-selector-factory.h	\
	$(BUILT_SOURCES)						\
	main.c

cpufreq_selector_LDADD = \
	$(CPUFREQ_SELECTOR_LIBS) 	\
	$(LIBCPUFREQ_LIBS)

if HAVE_POLKIT
cpufreq_selector_LDADD += \
	$(DBUS_LIBS)		\
	$(POLKIT_LIBS)
endif

cpufreq_selector_LDFLAGS = $(LDFLAGS) -Wl,-z,now

dbus_services_in_files = org.mate.CPUFreqSelector.service.in
polkit_in_files = org.mate.cpufreqselector.policy.in

dbus_servicesdir = $(datadir)/dbus-1/system-services
dbus_confdir = $(sysconfdir)/dbus-1/system.d
polkitdir = $(datadir)/polkit-1/actions

if HAVE_POLKIT
BUILT_SOURCES = cpufreq-selector-service-glue.h

cpufreq-selector-service-glue.h: $(srcdir)/cpufreq-selector-service.xml
	dbus-binding-tool --prefix=cpufreq_selector_service --mode=glib-server	\
	                  --output=cpufreq-selector-service-glue.h		\
	                  $(srcdir)/cpufreq-selector-service.xml

dbus_services_DATA = $(dbus_services_in_files:.service.in=.service)
$(dbus_services_DATA): $(dbus_services_in_files)
	sed -e "s|\@BINDIR\@|$(DESTDIR)$(bindir)|" $< > $@
dbus_conf_DATA = org.mate.CPUFreqSelector.conf

@INTLTOOL_POLICY_RULE@
polkit_DATA = $(polkit_in_files:.policy.in=.policy)

else
dbus_services_DATA=
dbus_conf_DATA=
polkit_DATA=
endif

EXTRA_DIST = \
	cpufreq-selector-service.xml	\
	$(dbus_services_in_files)	\
	org.mate.CPUFreqSelector.conf	\
	$(polkit_in_files)

CLEANFILES = \
	$(BUILT_SOURCES)	\
	$(dbus_services_DATA)	\
	$(polkit_DATA)

-include $(top_srcdir)/git.mk
