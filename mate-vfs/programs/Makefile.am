NULL =

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(TEST_CFLAGS)				\
	$(VFS_CFLAGS)				\
	-DG_DISABLE_DEPRECATED

libraries =						\
	$(top_builddir)/libmatevfs/libmatevfs-2.la	\
	$(TEST_LIBS)					\
	$(LIBEFS_LIBS)

bin_PROGRAMS =						\
	matevfs-cat					\
	matevfs-copy					\
	matevfs-info					\
	matevfs-ls					\
	matevfs-monitor				\
	matevfs-mkdir					\
	matevfs-mv					\
	matevfs-rm					\
	matevfs-df					\
	$(NULL)

matevfs_cat_SOURCES = matevfs-cat.c
matevfs_cat_LDADD = $(libraries)

matevfs_copy_SOURCES = matevfs-copy.c
matevfs_copy_LDADD = $(libraries)

matevfs_info_SOURCES = matevfs-info.c
matevfs_info_LDADD = $(libraries)

matevfs_ls_SOURCES = matevfs-ls.c
matevfs_ls_LDADD = $(libraries)

matevfs_monitor_SOURCES = matevfs-monitor.c
matevfs_monitor_LDADD = $(libraries)

matevfs_mkdir_SOURCES = matevfs-mkdir.c
matevfs_mkdir_LDADD = $(libraries)

matevfs_mv_SOURCES = matevfs-mv.c
matevfs_mv_LDADD = $(libraries)

matevfs_rm_SOURCES = matevfs-rm.c
matevfs_rm_LDADD = $(libraries)

matevfs_df_SOURCES = matevfs-df.c
matevfs_df_LDADD = $(libraries)

EXTRA_DIST = authentication.c
