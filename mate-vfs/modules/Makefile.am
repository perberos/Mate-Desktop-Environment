NULL =

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/imported/neon		\
	$(MODULES_XML_MATECONF_CFLAGS)		\
	$(MODULES_FILE_CFLAGS) 			\
	$(HOWL_CFLAGS)				\
	$(AVAHI_CFLAGS)				\
	$(LIBEFS_CFLAGS)			\
	$(SAMBA_CFLAGS)				\
	$(GSSAPI_CFLAGS)			\
	$(VFS_CFLAGS)				\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_LARGEFILE64_SOURCE			\
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DSSH_PROGRAM=\"$(SSH_PROGRAM)\"	\
	-DG_DISABLE_DEPRECATED			\
	-DMATE_VFS_PREFIX=\"$(prefix)\"	\
	-DMATE_VFS_DATADIR=\"$(datadir)\"	\
	-DMATE_VFS_LIBDIR=\"$(libdir)\"	\
	-DMATE_VFS_SYSCONFDIR=\"$(sysconfdir)\"\
	-DG_LOG_DOMAIN=\"mate-vfs-modules\"	\
	$(NULL)

EXTRA_DIST =					\
	$(HTTP_NEON_SOURCES)			\
	default-modules.conf			\
	smb-module.conf				\
	cdda-module.conf			\
	ssl-modules.conf			\
	inotify-helper.c			\
	inotify-helper.h			\
	inotify-kernel.h			\
	inotify-kernel.c			\
	inotify-missing.h			\
	inotify-missing.c			\
	inotify-sub.h				\
	inotify-sub.c				\
	inotify-path.h				\
	inotify-path.c				\
	inotify-diag.h				\
	inotify-diag.c

###  Module setup
if HAVE_CDDA
CDDA_LTLIBS = libcdda.la
else
CDDA_LTLIBS =
endif

if HAVE_SAMBA
SMB_LTLIBS = libsmb.la
else
SMB_LTLIBS = 
endif

if OS_WIN32
PLATFROM_SPECIFIC_LTLIBS =
else
PLATFROM_SPECIFIC_LTLIBS = libcomputer.la libdns-sd.la libnetwork.la libnntp.la libsftp.la libhttp.la libftp.la
endif

module_flags = -export_dynamic -avoid-version -module -no-undefined
modulesdir = $(libdir)/mate-vfs-2.0/modules

modules_LTLIBRARIES =				\
	$(CDDA_LTLIBS)				\
	libfile.la				\
	libvfs-test.la				\
	libbzip2.la				\
	libgzip.la				\
	$(SMB_LTLIBS)	                        \
	libtar.la				\
	$(PLATFROM_SPECIFIC_LTLIBS)		\
	$(NULL)

# Not currently supported
#	libnfs.la

modulesconfdir=$(sysconfdir)/mate-vfs-2.0/modules
if HAVE_CDDA
CDDA_CONF = cdda-module.conf
endif
if HAVE_SAMBA
SMB_CONF = smb-module.conf
endif
if HAVE_SSL
SSL_CONF = ssl-modules.conf
endif
modulesconf_DATA = default-modules.conf $(CDDA_CONF) $(SMB_CONF) $(SSL_CONF)

###  `cdda' method

libcdda_la_SOURCES = \
	cdda-method.c 				\
	cdda-cddb.c 				\
	cdda-cddb.h				\
	cdda-cdrom-extensions.h

libcdda_la_LDFLAGS = $(module_flags)
libcdda_la_LIBADD = $(MODULES_LIBS) $(CDDA_LIBS) ../libmatevfs/libmatevfs-2.la

###  `computer' method

libcomputer_la_SOURCES = computer-method.c
libcomputer_la_LDFLAGS = $(module_flags)
libcomputer_la_LIBADD = $(MODULES_FILE_LIBS) ../libmatevfs/libmatevfs-2.la

###  `dns-sd' method

libdns_sd_la_SOURCES = dns-sd-method.c
libdns_sd_la_LDFLAGS = $(module_flags)
libdns_sd_la_LIBADD = $(MODULES_LIBS) $(HOWL_LIBS) $(AVAHI_LIBS) ../libmatevfs/libmatevfs-2.la

###  `file' method

if HAVE_INOTIFY
FILE_ADD_SOURCES = inotify-kernel.c inotify-missing.c inotify-path.c inotify-sub.c inotify-helper.c inotify-diag.c
else
FILE_ADD_SOURCES = 
endif

libfile_la_SOURCES =           \
        file-method.c          \
	fstype.c               \
	file-method-acl.h      \
	file-method-acl.c      \
	$(FILE_ADD_SOURCES)    \
	$(NULL)

libfile_la_LDFLAGS = $(module_flags)
libfile_la_LIBADD = $(ACL_LIBS) $(FAM_LIBS) $(MODULES_FILE_LIBS) ../libmatevfs/libmatevfs-2.la


###  `network' method

libnetwork_la_SOURCES = network-method.c
libnetwork_la_LDFLAGS = $(module_flags)
libnetwork_la_LIBADD = $(MODULES_FILE_LIBS) ../libmatevfs/libmatevfs-2.la

###  `nntp' method

libnntp_la_SOURCES = nntp-method.c nntp-method.h
libnntp_la_LDFLAGS = $(module_flags)
libnntp_la_LIBADD = $(MODULES_LIBS) ../libmatevfs/libmatevfs-2.la

### `smb' method

libsmb_la_SOURCES = smb-method.c
libsmb_la_LDFLAGS = $(module_flags)
libsmb_la_LIBADD = $(SAMBA_LIBS) $(MODULES_MATECONF_LIBS) ../libmatevfs/libmatevfs-2.la 

###  `sftp' method

libsftp_la_SOURCES = sftp-method.c sftp.h
libsftp_la_LDFLAGS = $(module_flags)
libsftp_la_LIBADD = $(MODULES_LIBS) ../libmatevfs/libmatevfs-2.la

###  `test' method

libvfs_test_la_SOURCES = test-method.c
libvfs_test_la_LDFLAGS = $(module_flags)
libvfs_test_la_LIBADD = $(MODULES_XML_LIBS) ../libmatevfs/libmatevfs-2.la

### `gzip' method

libgzip_la_SOURCES = gzip-method.c
libgzip_la_LDFLAGS = $(module_flags)
libgzip_la_LIBADD = $(MODULES_LIBS) -lz ../libmatevfs/libmatevfs-2.la

### `bzip2' method

libbzip2_la_SOURCES = bzip2-method.c
libbzip2_la_LDFLAGS = $(module_flags)
libbzip2_la_LIBADD = $(MODULES_LIBS) $(BZ2_LIBS) ../libmatevfs/libmatevfs-2.la

### `ftp' method

libftp_la_SOURCES = ftp-method.c
libftp_la_LDFLAGS = $(module_flags)
libftp_la_LIBADD = $(MODULES_MATECONF_LIBS) $(GSSAPI_LIBS) ../libmatevfs/libmatevfs-2.la

### `nfs' method

#libnfs_la_SOURCES = 				\
#	nfs-method.c				\
#	nfs-method.h				\
#	nfs-method_mount_xdr.c			\
#	nfs-method_nfs_prot_xdr.c		\
#	nfs-method_mount.h			\
#	nfs-method_nfs_prot.h
#libnfs_la_LDFLAGS = $(module_flags)

### `http' method
libhttp_la_SOURCES =			\
	http-neon-method.c		\
	http-proxy.h			\
	http-proxy.c
	$(NULL)

libhttp_la_LDFLAGS = $(module_flags)
libhttp_la_LIBADD  = 			\
	$(MODULES_MATECONF_LIBS) 	\
	$(OPENSSL_LIBS) \
	../imported/neon/libneon.la   \
	../libmatevfs/libmatevfs-2.la

libtar_la_SOURCES = tar-method.c tarpet.h 
libtar_la_LDFLAGS = $(module_flags)
libtar_la_LIBADD  = $(MODULES_LIBS) ../libmatevfs/libmatevfs-2.la

#if HAVE_LIBEFS
#vefsmoduledir = $(modulesdir)
#vefsmodule_LTLIBRARIES = libvefs.la
#
#libvefs_la_SOURCES =	\
#	efs-method.c
#libvefs_la_LIBADD = $(LIBEFS_LIBS) $(MODULES_LIBS)
#endif

