/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-job.h - Jobs for asynchronous operation of the MATE
   Virtual File System (version for POSIX threads).

   Copyright (C) 1999 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@gnu.org>
*/

#ifndef MATE_VFS_JOB_H
#define MATE_VFS_JOB_H

/*
 * The following includes help Solaris copy with its own headers.  (With 64-
 * bit stuff enabled they like to #define open open64, etc.)
 * See http://bugzilla.gnome.org/show_bug.cgi?id=71184 for details.
 */
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-module-callback.h>

typedef struct MateVFSJob MateVFSJob;
typedef struct MateVFSModuleCallbackStackInfo MateVFSModuleCallbackStackInfo;

#define MATE_VFS_JOB_DEBUG 0

#if MATE_VFS_JOB_DEBUG

#include <stdio.h>

extern GStaticMutex debug_mutex;

#define JOB_DEBUG_PRINT(x)			\
G_STMT_START{					\
	struct timeval _tt;			\
	gettimeofday(&_tt, NULL);		\
	printf ("%ld:%6.ld ", _tt.tv_sec, _tt.tv_usec); \
	g_static_mutex_lock (&debug_mutex);	\
	fputs (__FUNCTION__, stdout);		\
	printf (": %d ", __LINE__);		\
	printf x;				\
	fputc ('\n', stdout);			\
	fflush (stdout);			\
	g_static_mutex_unlock (&debug_mutex);	\
}G_STMT_END

#endif

#if MATE_VFS_JOB_DEBUG
#include <sys/time.h>

#define JOB_DEBUG(x) JOB_DEBUG_PRINT(x)
#define JOB_DEBUG_ONLY(x) x
#define JOB_DEBUG_TYPE(x) (job_debug_types[(x)])

#else
#define JOB_DEBUG(x)
#define JOB_DEBUG_ONLY(x)
#define JOB_DEBUG_TYPE(x)

#endif

/* MATE_VFS_OP_MODULE_CALLBACK: is not a real OpType;
 * its intended to mark MateVFSAsyncModuleCallback's in the
 * job_callback queue
 */

enum MateVFSOpType {
	MATE_VFS_OP_OPEN,
	MATE_VFS_OP_OPEN_AS_CHANNEL,
	MATE_VFS_OP_CREATE,
	MATE_VFS_OP_CREATE_SYMBOLIC_LINK,
	MATE_VFS_OP_CREATE_AS_CHANNEL,
	MATE_VFS_OP_CLOSE,
	MATE_VFS_OP_READ,
	MATE_VFS_OP_WRITE,
	MATE_VFS_OP_SEEK,
	MATE_VFS_OP_READ_WRITE_DONE,
	MATE_VFS_OP_LOAD_DIRECTORY,
	MATE_VFS_OP_FIND_DIRECTORY,
	MATE_VFS_OP_XFER,
	MATE_VFS_OP_GET_FILE_INFO,
	MATE_VFS_OP_SET_FILE_INFO,
	MATE_VFS_OP_MODULE_CALLBACK,
	MATE_VFS_OP_FILE_CONTROL
};

typedef enum MateVFSOpType MateVFSOpType;

typedef struct {
	MateVFSURI *uri;
	MateVFSOpenMode open_mode;
} MateVFSOpenOp;

typedef struct {
	MateVFSAsyncOpenCallback callback;
	void *callback_data;
	MateVFSResult result;
} MateVFSOpenOpResult;

typedef struct {
	MateVFSURI *uri;
	MateVFSOpenMode open_mode;
	guint advised_block_size;
} MateVFSOpenAsChannelOp;

typedef struct {
	MateVFSAsyncOpenAsChannelCallback callback;
	void *callback_data;
	MateVFSResult result;
	GIOChannel *channel;
} MateVFSOpenAsChannelOpResult;

typedef struct {
	MateVFSURI *uri;
	MateVFSOpenMode open_mode;
	gboolean exclusive;
	guint perm;
} MateVFSCreateOp;

typedef struct {
	MateVFSAsyncCreateCallback callback;
	void *callback_data;
	MateVFSResult result;
} MateVFSCreateOpResult;

typedef struct {
	MateVFSURI *uri;
	char *uri_reference;
} MateVFSCreateLinkOp;

typedef struct {
	MateVFSURI *uri;
	MateVFSOpenMode open_mode;
	gboolean exclusive;
	guint perm;
} MateVFSCreateAsChannelOp;

typedef struct {
	MateVFSAsyncCreateAsChannelCallback callback;
	void *callback_data;
	MateVFSResult result;
	GIOChannel *channel;
} MateVFSCreateAsChannelOpResult;

typedef struct {
	char dummy; /* ANSI C does not allow empty structs */
} MateVFSCloseOp;

typedef struct {
	MateVFSAsyncCloseCallback callback;
	void *callback_data;
	MateVFSResult result;
} MateVFSCloseOpResult;

typedef struct {
	MateVFSFileSize num_bytes;
	gpointer buffer;
} MateVFSReadOp;

typedef struct {
	MateVFSAsyncReadCallback callback;
	void *callback_data;
	MateVFSFileSize num_bytes;
	gpointer buffer;
	MateVFSResult result;
	MateVFSFileSize bytes_read;
} MateVFSReadOpResult;

typedef struct {
	MateVFSFileSize num_bytes;
	gconstpointer buffer;
} MateVFSWriteOp;

typedef struct {
	MateVFSAsyncWriteCallback callback;
	void *callback_data;
	MateVFSFileSize num_bytes;
	gconstpointer buffer;
	MateVFSResult result;
	MateVFSFileSize bytes_written;
} MateVFSWriteOpResult;

typedef struct {
	MateVFSSeekPosition whence;
	MateVFSFileOffset offset;
} MateVFSSeekOp;

typedef struct {
	MateVFSAsyncSeekCallback callback;
	void *callback_data;
	MateVFSResult result;
} MateVFSSeekOpResult;

typedef struct {
	GList *uris; /* MateVFSURI* */
	MateVFSFileInfoOptions options;
} MateVFSGetFileInfoOp;

typedef struct {
	MateVFSAsyncGetFileInfoCallback callback;
	void *callback_data;
	GList *result_list; /* MateVFSGetFileInfoResult* */
} MateVFSGetFileInfoOpResult;

typedef struct {
	MateVFSURI *uri;
	MateVFSFileInfo *info;
	MateVFSSetFileInfoMask mask;
	MateVFSFileInfoOptions options;
} MateVFSSetFileInfoOp;

typedef struct {
	MateVFSAsyncSetFileInfoCallback callback;
	void *callback_data;
	MateVFSResult set_file_info_result;
	MateVFSResult get_file_info_result;
	MateVFSFileInfo *info;
} MateVFSSetFileInfoOpResult;

typedef struct {
	GList *uris; /* MateVFSURI* */
	MateVFSFindDirectoryKind kind;
	gboolean create_if_needed;
	gboolean find_if_needed;
	guint permissions;
} MateVFSFindDirectoryOp;

typedef struct {
	MateVFSAsyncFindDirectoryCallback callback;
	void *callback_data;
	GList *result_list; /* MateVFSFindDirectoryResult */
} MateVFSFindDirectoryOpResult;

typedef struct {
	MateVFSURI *uri;
	MateVFSFileInfoOptions options;
	guint items_per_notification;
} MateVFSLoadDirectoryOp;

typedef struct {
	MateVFSAsyncDirectoryLoadCallback callback;
	void *callback_data;
	MateVFSResult result;
	GList *list;
	guint entries_read;
} MateVFSLoadDirectoryOpResult;

typedef struct {
	GList *source_uri_list;
	GList *target_uri_list;
	MateVFSXferOptions xfer_options;
	MateVFSXferErrorMode error_mode;
	MateVFSXferOverwriteMode overwrite_mode;
	MateVFSXferProgressCallback progress_sync_callback;
	gpointer sync_callback_data;
} MateVFSXferOp;

typedef struct {
	MateVFSAsyncXferProgressCallback callback;
	void *callback_data;
	MateVFSXferProgressInfo *progress_info;
	int reply;
} MateVFSXferOpResult;

typedef struct {
	MateVFSAsyncModuleCallback    callback;
	gpointer                       user_data;
	gconstpointer		       in;
	size_t			       in_size;
	gpointer                       out;
	size_t			       out_size;
	MateVFSModuleCallbackResponse response;
	gpointer                       response_data;
} MateVFSModuleCallbackOpResult;

typedef struct {
	char *operation;
	gpointer operation_data;
	GDestroyNotify operation_data_destroy_func;
} MateVFSFileControlOp;

typedef struct {
	MateVFSAsyncFileControlCallback callback;
	gpointer callback_data;
	MateVFSResult result;
	gpointer operation_data;
	GDestroyNotify operation_data_destroy_func;
} MateVFSFileControlOpResult;

typedef union {
	MateVFSOpenOp open;
	MateVFSOpenAsChannelOp open_as_channel;
	MateVFSCreateOp create;
	MateVFSCreateLinkOp create_symbolic_link;
	MateVFSCreateAsChannelOp create_as_channel;
	MateVFSCloseOp close;
	MateVFSReadOp read;
	MateVFSWriteOp write;
	MateVFSSeekOp seek;
	MateVFSLoadDirectoryOp load_directory;
	MateVFSXferOp xfer;
	MateVFSGetFileInfoOp get_file_info;
	MateVFSSetFileInfoOp set_file_info;
	MateVFSFindDirectoryOp find_directory;
	MateVFSFileControlOp file_control;
} MateVFSSpecificOp;

typedef struct {
	/* ID of the job (e.g. open, create, close...). */
	MateVFSOpType type;

	/* The callback for when the op is completed. */
	GFunc callback;
	gpointer callback_data;

	/* Details of the op. */
	MateVFSSpecificOp specifics;

	/* The context for cancelling the operation. */
	MateVFSContext *context;
	MateVFSModuleCallbackStackInfo *stack_info;
} MateVFSOp;

typedef union {
	MateVFSOpenOpResult open;
	MateVFSOpenAsChannelOpResult open_as_channel;
	MateVFSCreateOpResult create;
	MateVFSCreateAsChannelOpResult create_as_channel;
	MateVFSCloseOpResult close;
	MateVFSReadOpResult read;
	MateVFSWriteOpResult write;
	MateVFSSeekOpResult seek;
	MateVFSGetFileInfoOpResult get_file_info;
	MateVFSSetFileInfoOpResult set_file_info;
	MateVFSFindDirectoryOpResult find_directory;
	MateVFSLoadDirectoryOpResult load_directory;
	MateVFSXferOpResult xfer;
	MateVFSModuleCallbackOpResult callback;
	MateVFSFileControlOpResult file_control;
} MateVFSSpecificNotifyResult;

typedef struct {
	MateVFSAsyncHandle *job_handle;

	guint callback_id;

	/* By the time the callback got reached the job might have been cancelled.
	 * We find out by checking this flag.
	 */
	gboolean cancelled;

	/* ID of the job (e.g. open, create, close...). */
	MateVFSOpType type;

	MateVFSSpecificNotifyResult specifics;
} MateVFSNotifyResult;

/* FIXME bugzilla.eazel.com 1135: Move private stuff out of the header.  */
struct MateVFSJob {
	/* Handle being used for file access.  */
	MateVFSHandle *handle;

	/* By the time the entry routine for the job got reached
	 * the job might have been cancelled. We find out by checking
	 * this flag.
	 */
	gboolean cancelled;

	/* Read or create returned with an error - helps
	 * flagging that we do not expect a cancel
	 */
	gboolean failed;

	/* Global lock for accessing job's 'op' and 'handle' */
	GMutex *job_lock;

	/* This condition is signalled when the master thread gets a
           notification and wants to acknowledge it.  */
	GCond *notify_ack_condition;

	/* Operations that are being done and those that are completed and
	 * ready for notification to take place.
	 */
	MateVFSOp *op;

	/* Unique identifier of this job (a uint, really) */
	MateVFSAsyncHandle *job_handle;

	/* The priority of this job */
	int priority;
};

G_GNUC_INTERNAL
MateVFSJob 	*_mate_vfs_job_new      	  (MateVFSOpType  	 type,
						   int			 priority,
				      		   GFunc           	 callback,
				      		   gpointer        	 callback_data);
G_GNUC_INTERNAL
void         	 _mate_vfs_job_destroy  	  (MateVFSJob     	*job);
G_GNUC_INTERNAL
void         	 _mate_vfs_job_set	  	  (MateVFSJob     	*job,
				      		   MateVFSOpType  	 type,
				      		   GFunc           	 callback,
				      		   gpointer        	 callback_data);
G_GNUC_INTERNAL
void         	 _mate_vfs_job_go       	  (MateVFSJob     	*job);
G_GNUC_INTERNAL
void     	 _mate_vfs_job_execute  	  (MateVFSJob     	*job);
G_GNUC_INTERNAL
void         	 _mate_vfs_job_module_cancel  	  (MateVFSJob	 	*job);
int          	 mate_vfs_job_get_count 	  (void);

G_GNUC_INTERNAL
gboolean	 _mate_vfs_job_complete	  (MateVFSJob 		*job);

#endif /* MATE_VFS_JOB_H */
