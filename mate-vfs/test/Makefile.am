NULL =

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-I$(top_builddir)/libmatevfs           \
	$(TEST_CFLAGS)				\
	$(VFS_CFLAGS)				\
	-DG_DISABLE_DEPRECATED			\
	-DMODULES_PATH=\"$(libdir)/vfs/modules\"

if OS_WIN32
else
platform_only_programs = test-dns-sd test-symlinks test-parse-ls-lga
endif

noinst_PROGRAMS =				\
	test-acl				\
	test-address				\
	test-async				\
	test-async-cancel                       \
	test-async-directory			\
	test-channel				\
	test-directory				\
	test-directory-visit			\
	test-dirop				\
	test-escape				\
	test-find-directory			\
	test-info				\
	test-long-cancel			\
	test-mime				\
	test-mime-handlers			\
	test-mime-info-cache			\
	test-mime-handlers-set			\
	test-monitor				\
	test-performance			\
	test-seek				\
	test-shell				\
	test-ssl				\
	test-sync				\
	test-sync-create			\
	test-sync-write				\
	test-unlink				\
	test-uri				\
	test-volumes				\
	test-xfer				\
	test-callback				\
	test-module-selftest			\
	test-queue				\
	$(platform_only_programs)		\
	$(NULL)

test_files=		\
	test.input	\
	test.cmds	\
	test.output

# Set up the environment so the tests can find the modules
TESTS_ENVIRONMENT = MATE_VFS_MODULE_PATH=$(top_builddir)/modules/.libs \
		MATE_VFS_MODULE_CONFIG_PATH=$(top_srcdir)/modules \
		MATE_VFS_TEST_CONFIG_FILE=$(top_srcdir)/test/queue-test-config.xml \
		SRCDIR=$(srcdir)
TESTS = test-acl	  \
	test-address      \
	test-async-cancel \
	test-escape       \
	test-uri       	  \
	$(srcdir)/auto-test	

libraries =						\
	$(top_builddir)/libmatevfs/libmatevfs-2.la	\
	$(TEST_LIBS)					\
	$(LIBEFS_LIBS)

test_acl_SOURCES = test-acl.c
test_acl_LDADD = $(libraries)

test_address_SOURCES = test-address.c
test_address_LDADD = $(libraries)

test_async_SOURCES = test-async.c
test_async_LDADD = $(libraries)

test_performance_SOURCES = test-performance.c
test_performance_LDADD = $(libraries)

test_parse_ls_lga_SOURCES = test-parse-ls-lga.c
test_parse_ls_lga_LDADD = $(libraries)

#test_resolv_SOURCES = test-resolv.c
#test_resolv_LDADD = $(libraries)

#test_subdir_SOURCES = test-subdir.c
#test_subdir_LDADD = $(libraries)

test_async_directory_SOURCES = test-async-directory.c
test_async_directory_LDADD = $(libraries)

test_channel_SOURCES = test-channel.c
test_channel_LDADD = $(libraries)

test_seek_SOURCES = test-seek.c
test_seek_LDADD = $(libraries)

test_shell_SOURCES = test-shell.c
test_shell_LDADD = $(libraries)

test_info_SOURCES = test-info.c
test_info_LDADD = $(libraries)

test_long_cancel_SOURCES = test-long-cancel.c
test_long_cancel_LDADD = $(libraries)

test_mime_SOURCES = test-mime.c
test_mime_LDADD = $(libraries)

test_mime_handlers_SOURCES = test-mime-handlers.c
test_mime_handlers_LDADD = $(libraries)

test_mime_handlers_set_SOURCES = test-mime-handlers-set.c
test_mime_handlers_set_LDADD = $(libraries)

test_mime_info_cache_SOURCES = test-mime-info-cache.c
test_mime_info_cache_LDADD = $(libraries)

test_xfer_SOURCES = test-xfer.c
test_xfer_LDADD = $(libraries)

test_directory_SOURCES = test-directory.c
test_directory_LDADD = $(libraries)

test_directory_visit_SOURCES = test-directory-visit.c
test_directory_visit_LDADD = $(libraries)

test_symlinks_SOURCES = test-symlinks.c
test_symlinks_LDADD = $(libraries)

test_ssl_SOURCES = test-ssl.c
test_ssl_LDADD = $(libraries)

test_sync_SOURCES = test-sync.c
test_sync_LDADD = $(libraries)

test_sync_write_SOURCES = test-sync-write.c
test_sync_write_LDADD = $(libraries)

test_sync_create_SOURCES = test-sync-create.c
test_sync_create_LDADD = $(libraries)

test_async_cancel_SOURCES = test-async-cancel.c
test_async_cancel_LDADD = $(libraries)

test_escape_SOURCES = test-escape.c
test_escape_LDADD = $(libraries)

test_uri_SOURCES = test-uri.c
test_uri_LDADD = $(libraries)

test_volumes_SOURCES = test-volumes.c
test_volumes_LDADD = $(libraries)

test_unlink_SOURCES = test-unlink.c
test_unlink_LDADD = $(libraries)

test_dirop_SOURCES = test-dirop.c
test_dirop_LDADD = $(libraries)

test_dns_sd_SOURCES = test-dns-sd.c
test_dns_sd_LDADD = $(libraries)

test_find_directory_SOURCES = test-find-directory.c
test_find_directory_LDADD = $(libraries)

test_monitor_SOURCES = test-monitor.c
test_monitor_LDADD = $(libraries)

# test_metadata_SOURCES = test-metadata.c
# test_metadata_LDADD = $(libraries)

test_callback_SOURCES = test-callback.c
test_callback_LDADD = $(libraries)

test_module_selftest_SOURCES = test-module-selftest.c
test_module_selftest_LDADD = $(libraries)

test_queue_SOURCES = test-queue.c
test_queue_LDADD = $(libraries)

EXTRA_DIST =					\
	$(test_files)				\
	auto-test				\
	vfs-run.in
