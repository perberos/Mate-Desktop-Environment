if BUILD_NETWORK
network_SUBDIR = network
endif

if BUILD_SERVICES
services_SUBDIR = services
endif

if BUILD_TIME
time_SUBDIR = time
endif

if BUILD_USERS
users_SUBDIR = users
endif

if BUILD_SHARES
shares_SUBDIR = shares
endif

SUBDIRS = \
	common			\
	$(network_SUBDIR)	\
	$(services_SUBDIR)	\
	$(shares_SUBDIR)	\
	$(time_SUBDIR)		\
	$(users_SUBDIR)

-include $(top_srcdir)/git.mk
