/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-file-info.h - Handling of file information for the MATE
   Virtual File System.

   Copyright (C) 1999,2001 Free Software Foundation
   Copyright (C) 2002 Seth Nickell

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_FILE_INFO_H
#define MATE_VFS_FILE_INFO_H

#include <libmatevfs/mate-vfs-file-size.h>
#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-uri.h>
#include <libmatevfs/mate-vfs-acl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef G_OS_WIN32

/* Note that Win32 file permissions (in NTFS) are based on an ACL and
 * not at all on anything like POSIX rwxrwxrwx bits. Additionally, a
 * file can have a READONLY attribute. The stat() in the Microsoft C
 * library only looks at the (MS-DOSish) READONLY attribute, not at
 * all at the ACL. It fakes a st_mode based on the READONLY attribute
 * only. If the FIXME below for MateVFSFilePermissions is fixed,
 * these defines will become unnecessary.
 */

#define S_ISUID 0
#define S_ISGID 0
#define S_IRGRP (S_IRUSR >> 3)
#define S_IWGRP (S_IWUSR >> 3)
#define S_IXGRP (S_IXUSR >> 3)
#define S_IROTH (S_IRUSR >> 6)
#define S_IWOTH (S_IWUSR >> 6)
#define S_IXOTH (S_IXUSR >> 6)

#endif

/**
 * MateVFSInodeNumber:
 *
 * Represents the i-node of a file, this is a low level data structure
 * that the operating system uses to hold information about a file.
 **/

typedef MateVFSFileSize MateVFSInodeNumber;

/**
 * MateVFSFileFlags:
 * @MATE_VFS_FILE_FLAGS_NONE: no flags
 * @MATE_VFS_FILE_FLAGS_SYMLINK: whether the file is a symlink.
 * @MATE_VFS_FILE_FLAGS_LOCAL: whether the file is on a local filesystem
 *
 * Packed boolean bitfield representing special
 * flags a #MateVFSFileInfo struct can have.
 **/
typedef enum {
	MATE_VFS_FILE_FLAGS_NONE = 0,
	MATE_VFS_FILE_FLAGS_SYMLINK = 1 << 0,
	MATE_VFS_FILE_FLAGS_LOCAL = 1 << 1
} MateVFSFileFlags;

/**
 * MateVFSFileType:
 * @MATE_VFS_FILE_TYPE_UNKNOWN: The file type is unknown (none of the types below matches).
 * @MATE_VFS_FILE_TYPE_REGULAR: The file is a regular file (stat: %S_ISREG).
 * @MATE_VFS_FILE_TYPE_DIRECTORY: The file is a directory (stat: %S_ISDIR).
 * @MATE_VFS_FILE_TYPE_FIFO: The file is a FIFO (stat: %S_ISFIFO).
 * @MATE_VFS_FILE_TYPE_SOCKET: The file is a socket (stat: %S_ISSOCK).
 * @MATE_VFS_FILE_TYPE_CHARACTER_DEVICE: The file is a character device (stat: %S_ISCHR).
 * @MATE_VFS_FILE_TYPE_BLOCK_DEVICE: The file is a block device (stat: %S_ISBLK).
 * @MATE_VFS_FILE_TYPE_SYMBOLIC_LINK: The file is a symbolic link (stat: %S_ISLNK).
 *
 * Maps to a %stat mode, and identifies the kind of file represented by a
 * #MateVFSFileInfo struct, stored in the %type field.
 **/

typedef enum {
	MATE_VFS_FILE_TYPE_UNKNOWN,
	MATE_VFS_FILE_TYPE_REGULAR,
	MATE_VFS_FILE_TYPE_DIRECTORY,
	MATE_VFS_FILE_TYPE_FIFO,
	MATE_VFS_FILE_TYPE_SOCKET,
	MATE_VFS_FILE_TYPE_CHARACTER_DEVICE,
	MATE_VFS_FILE_TYPE_BLOCK_DEVICE,
	MATE_VFS_FILE_TYPE_SYMBOLIC_LINK
} MateVFSFileType;

/**
 * MateVFSFileInfoFields:
 * @MATE_VFS_FILE_INFO_FIELDS_NONE: No fields are valid
 * @MATE_VFS_FILE_INFO_FIELDS_TYPE: Type field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS: Permissions field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_FLAGS: Flags field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_DEVICE: Device field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_INODE: Inode field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_LINK_COUNT: Link count field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_SIZE: Size field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_BLOCK_COUNT: Block count field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE: I/O Block Size field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_ATIME: Access time field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_MTIME: Modification time field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_CTIME: Creating time field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME: Symlink name field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE: Mime type field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_ACCESS: Access bits of the permissions
 * bitfield are valid
 * @MATE_VFS_FILE_INFO_FIELDS_IDS: UID and GID information are valid
 * @MATE_VFS_FILE_INFO_FIELDS_ACL: ACL field is valid
 * @MATE_VFS_FILE_INFO_FIELDS_SELINUX_CONTEXT: SELinux Security context is valid
 *
 * Flags indicating what fields in a #MateVFSFileInfo struct are valid.
 * Name is always assumed valid (how else would you have gotten a
 * FileInfo struct otherwise?)
 **/

typedef enum {
	MATE_VFS_FILE_INFO_FIELDS_NONE = 0,
	MATE_VFS_FILE_INFO_FIELDS_TYPE = 1 << 0,
	MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS = 1 << 1,
	MATE_VFS_FILE_INFO_FIELDS_FLAGS = 1 << 2,
	MATE_VFS_FILE_INFO_FIELDS_DEVICE = 1 << 3,
	MATE_VFS_FILE_INFO_FIELDS_INODE = 1 << 4,
	MATE_VFS_FILE_INFO_FIELDS_LINK_COUNT = 1 << 5,
	MATE_VFS_FILE_INFO_FIELDS_SIZE = 1 << 6,
	MATE_VFS_FILE_INFO_FIELDS_BLOCK_COUNT = 1 << 7,
	MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE = 1 << 8,
	MATE_VFS_FILE_INFO_FIELDS_ATIME = 1 << 9,
	MATE_VFS_FILE_INFO_FIELDS_MTIME = 1 << 10,
	MATE_VFS_FILE_INFO_FIELDS_CTIME = 1 << 11,
	MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME = 1 << 12,
	MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE = 1 << 13,
	MATE_VFS_FILE_INFO_FIELDS_ACCESS = 1 << 14,
	MATE_VFS_FILE_INFO_FIELDS_IDS = 1 << 15,
	MATE_VFS_FILE_INFO_FIELDS_ACL = 1 << 16,
	MATE_VFS_FILE_INFO_FIELDS_SELINUX_CONTEXT = 1 << 17
} MateVFSFileInfoFields;

/* FIXME: It's silly to use the symbolic constants for POSIX here.
 * This is supposed to be a virtual file system, so it makes no
 * sense to use the values of the POSIX-required constants on the
 * particular machine this code is compiled on. These should all be changed
 * to use numeric constants like MATE_VFS_PERM_STICKY does now. However,
 * be careful in making such a change, since some existing code might
 * wrongly assume these equivalencies.
 */

/**
 * MateVFSFilePermissions:
 * @MATE_VFS_PERM_SUID: UID bit
 * @MATE_VFS_PERM_SGID: GID bit
 * @MATE_VFS_PERM_STICKY: Sticky bit.
 * @MATE_VFS_PERM_USER_READ: Owner has read permission.
 * @MATE_VFS_PERM_USER_WRITE: Owner has write permission.
 * @MATE_VFS_PERM_USER_EXEC: Owner has execution permission.
 * @MATE_VFS_PERM_USER_ALL: Owner has all permissions.
 * @MATE_VFS_PERM_GROUP_READ: Group has read permission.
 * @MATE_VFS_PERM_GROUP_WRITE: Group has write permission.
 * @MATE_VFS_PERM_GROUP_EXEC: Group has execution permission.
 * @MATE_VFS_PERM_GROUP_ALL: Group has all permissions.
 * @MATE_VFS_PERM_OTHER_READ: Others have read permission.
 * @MATE_VFS_PERM_OTHER_WRITE: Others have write permission.
 * @MATE_VFS_PERM_OTHER_EXEC: Others have execution permission.
 * @MATE_VFS_PERM_OTHER_ALL: Others have all permissions.
 * @MATE_VFS_PERM_ACCESS_READABLE: This file is readable for the current client.
 * @MATE_VFS_PERM_ACCESS_WRITABLE: This file is writable for the current client.
 * @MATE_VFS_PERM_ACCESS_EXECUTABLE: This file is executable for the current client.
 *
 * File permissions. Some of these fields correspond to traditional
 * UNIX semantics, others provide more abstract concepts.
 *
 * <note>
 *   Some network file systems don't support traditional UNIX semantics but
 *   still provide file access control. Thus, if you want to modify the
 *   permissions (i.e. do a chmod), you should rely on the traditional
 *   fields, but if you just want to determine whether a file or directory
 *   can be read from or written to, you should rely on the more
 *   abstract %MATE_VFS_PERM_ACCESS_* fields.
 * </note>
 **/
typedef enum {
	MATE_VFS_PERM_SUID = S_ISUID,
	MATE_VFS_PERM_SGID = S_ISGID,
	MATE_VFS_PERM_STICKY = 01000,	/* S_ISVTX not defined on all systems */
	MATE_VFS_PERM_USER_READ = S_IRUSR,
	MATE_VFS_PERM_USER_WRITE = S_IWUSR,
	MATE_VFS_PERM_USER_EXEC = S_IXUSR,
	MATE_VFS_PERM_USER_ALL = S_IRUSR | S_IWUSR | S_IXUSR,
	MATE_VFS_PERM_GROUP_READ = S_IRGRP,
	MATE_VFS_PERM_GROUP_WRITE = S_IWGRP,
	MATE_VFS_PERM_GROUP_EXEC = S_IXGRP,
	MATE_VFS_PERM_GROUP_ALL = S_IRGRP | S_IWGRP | S_IXGRP,
	MATE_VFS_PERM_OTHER_READ = S_IROTH,
	MATE_VFS_PERM_OTHER_WRITE = S_IWOTH,
	MATE_VFS_PERM_OTHER_EXEC = S_IXOTH,
	MATE_VFS_PERM_OTHER_ALL = S_IROTH | S_IWOTH | S_IXOTH,
	MATE_VFS_PERM_ACCESS_READABLE   = 1 << 16,
	MATE_VFS_PERM_ACCESS_WRITABLE   = 1 << 17,
	MATE_VFS_PERM_ACCESS_EXECUTABLE = 1 << 18
} MateVFSFilePermissions;


#define MATE_VFS_TYPE_FILE_INFO  (mate_vfs_file_info_get_type ())

/**
 * MateVFSFileInfo:
 * @name: A char * specifying the base name of the file (without any path string).
 * @valid_fields: #MateVFSFileInfoFields specifying which fields of
 * 		  #MateVFSFileInfo are valid. Note that @name is always
 * 		  assumed to be valid, i.e. clients may assume that it is not NULL.
 * @type: The #MateVFSFileType of the file (i.e. regular, directory, block device, ...)
 * 	  if @valid_fields provides MATE_VFS_FILE_INFO_FIELDS_TYPE.
 * @permissions: The #MateVFSFilePermissions corresponding to the UNIX-like
 * 		 permissions of the file, if @valid_fields provides
 * 		 #MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS, and the
 * 		 #MateVFSFilePermissions corresponding to abstract access
 * 		 concepts (#MATE_VFS_PERM_ACCESS_READABLE, #MATE_VFS_PERM_ACCESS_WRITABLE,
 * 		 and #MATE_VFS_PERM_ACCESS_EXECUTABLE) if @valid_fields
 * 		 provides #MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS.
 * @flags: #MateVFSFileFlags providing additional information about the file,
 * 	   for instance whether it is local or a symbolic link, if
 * 	   @valid_fields provides #MATE_VFS_FILE_INFO_FIELDS_FLAGS.
 * @device: Identifies the device the file is located on, if
 * 	    @valid_fields provides #MATE_VFS_FILE_INFO_FIELDS_DEVICE.
 * @inode: Identifies the inode corresponding to the file, if
 * 	    @valid_fields provides #MATE_VFS_FILE_INFO_FIELDS_INODE.
 * @link_count: Counts the number of hard links to the file, if
 * 		@valid_fields provides #MATE_VFS_FILE_INFO_FIELDS_LINK_COUNT.
 * @uid: The user owning the file, if @valid_fields provides
 * 	 MATE_VFS_FILE_INFO_FIELDS_IDS.
 * @gid: The user owning the file, if @valid_fields provides
 * 	 MATE_VFS_FILE_INFO_FIELDS_IDS.
 * @size: The size of the file in bytes (a #MateVFSFileSize),
 * 	  if @valid_fields provides #MATE_VFS_FILE_INFO_FIELDS_SIZE.
 * @block_count: The size of the file in file system blocks (a #MateVFSFileSize),
 * 		 if @valid_fields provides #MATE_VFS_FILE_INFO_FIELDS_BLOCK_COUNT.
 * @io_block_size: The optimal buffer size for reading/writing the file, if
 * 		   @valid_fields provides #MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE.
 * @atime: The time of the last file access, if @valid_fields provides
 * 	   #MATE_VFS_FILE_INFO_FIELDS_ATIME.
 * @mtime: The time of the last file contents modification, if @valid_fields
 * 	   provides #MATE_VFS_FILE_INFO_FIELDS_MTIME.
 * @ctime: The time of the last inode change, if @valid_fields provides
 * 	   #MATE_VFS_FILE_INFO_FIELDS_CTIME.
 * @symlink_name: This is the name of the file this link points to, @type
 * 		  is #MATE_VFS_FILE_FLAGS_SYMLINK, and @valid_fields
 * 		  provides #MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME.
 * @mime_type: This is a char * identifying the type of the file, if
 * 	       @valid_fields provides #MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE.
 * @refcount: The reference count of this file info, which is one by default, and
 * 	      that can be increased using mate_vfs_file_info_ref() and decreased
 * 	      using mate_vfs_file_info_unref(). When it drops to zero, the file info
 * 	      is freed and its memory is invalid. Make sure to keep your own
 * 	      reference to a file info if you received it from MateVFS, i.e.
 * 	      if you didn't call mate_vfs_file_info_new() yourself.
 *
 * <note>
 *   When doing massive I/O, it is suggested to adhere @io_block_size if applicable.
 *   For network file systems, this may be set to very big values allowing
 *   parallelization.
 * </note>
 *
 * The MateVFSFileInfo structure contains information about a file.
 **/
typedef struct {
	/*< public >*/
	char *name;

	MateVFSFileInfoFields valid_fields;
	MateVFSFileType type;
	MateVFSFilePermissions permissions;

	MateVFSFileFlags flags;

	dev_t device;
	MateVFSInodeNumber inode;

	guint link_count;

	guint uid;
	guint gid;

	MateVFSFileSize size;

	MateVFSFileSize block_count;

	guint io_block_size;

	time_t atime;
	time_t mtime;
	time_t ctime;

	char *symlink_name;

	char *mime_type;

	guint refcount;

	/* File ACLs */
	MateVFSACL *acl;

	/* SELinux security context. -- ascii string, raw format. */
	char* selinux_context;

	/*< private >*/
	/* Reserved for future expansions to MateVFSFileInfo without having
	   to break ABI compatibility */
	void *reserved1;
	void *reserved2;
	void *reserved3;
} MateVFSFileInfo;

/**
 * MateVFSFileInfoOptions:
 * @MATE_VFS_FILE_INFO_DEFAULT: default flags
 * @MATE_VFS_FILE_INFO_GET_MIME_TYPE: detect the MIME type
 * @MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE: only use fast MIME type
 * detection (extensions)
 * @MATE_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE: force slow MIME type
 * detection where available (sniffing, algorithmic detection, etc)
 * @MATE_VFS_FILE_INFO_FOLLOW_LINKS: automatically follow symbolic
 * links and retrieve the properties of their target (recommended)
 * @MATE_VFS_FILE_INFO_GET_ACCESS_RIGHTS: tries to get data similar
 * to what would return access(2) on a local file system (ie is the
 * file readable, writable and/or executable). Can be really slow on
 * remote file systems
 * @MATE_VFS_FILE_INFO_NAME_ONLY: When reading a directory, only
 * get the filename (if doing so is faster). Useful to e.g. count
 * the number of files.
 * @MATE_VFS_FILE_INFO_GET_ACL: get ACLs for the file
 *
 * Packed boolean bitfield representing options that can
 * be passed into a mate_vfs_get_file_info() call (or other
 * related calls that return file info) and affect the operation
 * of get_file_info.
 **/

typedef enum {
	MATE_VFS_FILE_INFO_DEFAULT = 0,
	MATE_VFS_FILE_INFO_GET_MIME_TYPE = 1 << 0,
	MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE = 1 << 1,
	MATE_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE = 1 << 2,
	MATE_VFS_FILE_INFO_FOLLOW_LINKS = 1 << 3,
	MATE_VFS_FILE_INFO_GET_ACCESS_RIGHTS = 1 << 4,
	MATE_VFS_FILE_INFO_NAME_ONLY = 1 << 5,
	MATE_VFS_FILE_INFO_GET_ACL = 1 << 6,
	MATE_VFS_FILE_INFO_GET_SELINUX_CONTEXT = 1 << 7
} MateVFSFileInfoOptions;

/**
 * MateVFSSetFileInfoMask:
 * @MATE_VFS_SET_FILE_INFO_NONE: don't set any file info fields
 * @MATE_VFS_SET_FILE_INFO_NAME: change the name
 * @MATE_VFS_SET_FILE_INFO_PERMISSIONS: change the permissions
 * @MATE_VFS_SET_FILE_INFO_OWNER: change the file's owner
 * @MATE_VFS_SET_FILE_INFO_TIME: change the file's time stamp(s)
 * @MATE_VFS_SET_FILE_INFO_ACL: change the file's ACLs
 * @MATE_VFS_SET_FILE_INFO_SYMLINK_NAME: change the file's symlink name
 *
 * Packed boolean bitfield representing the aspects of the file
 * to be changed in a mate_vfs_set_file_info() call.
 **/

typedef enum {
	MATE_VFS_SET_FILE_INFO_NONE = 0,
	MATE_VFS_SET_FILE_INFO_NAME = 1 << 0,
	MATE_VFS_SET_FILE_INFO_PERMISSIONS = 1 << 1,
	MATE_VFS_SET_FILE_INFO_OWNER = 1 << 2,
	MATE_VFS_SET_FILE_INFO_TIME = 1 << 3,
	MATE_VFS_SET_FILE_INFO_ACL = 1 << 4,
	MATE_VFS_SET_FILE_INFO_SELINUX_CONTEXT = 1 << 5,
	MATE_VFS_SET_FILE_INFO_SYMLINK_NAME = 1 << 6
} MateVFSSetFileInfoMask;


/**
 * MateVFSGetFileInfoResult:
 * @uri: The #MateVFSURI the file info was requested for.
 * @result: The #MateVFSResult of the file info request.
 * @file_info: The #MateVFSFileInfo that was retrieved.
 *
 * This data structure encapsulates the details of an individual file
 * info request that was part of a mass file info request launched
 * through mate_vfs_async_get_file_info(), and is passed to a
 * #MateVFSAsyncGetFileInfoCallback.
 **/
struct _MateVFSGetFileInfoResult {
	MateVFSURI *uri;
	MateVFSResult result;
	MateVFSFileInfo *file_info;
};

typedef struct _MateVFSGetFileInfoResult MateVFSGetFileInfoResult;

GType                      mate_vfs_get_file_info_result_get_type (void);
MateVFSGetFileInfoResult* mate_vfs_get_file_info_result_dup  (MateVFSGetFileInfoResult *result);
void                       mate_vfs_get_file_info_result_free (MateVFSGetFileInfoResult *result);

/**
 * MATE_VFS_FILE_INFO_SYMLINK:
 * @info: MateVFSFileInfo struct
 *
 * Determines whether a file is a symbolic link given @info.
 */
#define MATE_VFS_FILE_INFO_SYMLINK(info)		\
	((info)->flags & MATE_VFS_FILE_FLAGS_SYMLINK)

/**
 * MATE_VFS_FILE_INFO_SET_SYMLINK:
 * @info: MateVFSFileInfo struct
 * @value: if %TRUE, @info is set to indicate the file is a symbolic link
 *
 * Set the symbolic link field in @info to @value.
 */
 #define MATE_VFS_FILE_INFO_SET_SYMLINK(info, value) \
       ((info)->flags = (value ? \
       ((info)->flags | MATE_VFS_FILE_FLAGS_SYMLINK) : \
       ((info)->flags & ~MATE_VFS_FILE_FLAGS_SYMLINK)))

/**
 * MATE_VFS_FILE_INFO_LOCAL:
 * @info: MateVFSFileInfo struct
 *
 * Determines whether a file is local given @info.
 */
#define MATE_VFS_FILE_INFO_LOCAL(info)			\
	((info)->flags & MATE_VFS_FILE_FLAGS_LOCAL)

/**
 * MATE_VFS_FILE_INFO_SET_LOCAL:
 * @info: MateVFSFileInfo struct
 * @value: if %TRUE, @info is set to indicate the file is local
 *
 * Set the "local file" field in @info to @value.
 */
 #define MATE_VFS_FILE_INFO_SET_LOCAL(info, value) \
       ((info)->flags = (value ? \
       ((info)->flags | MATE_VFS_FILE_FLAGS_LOCAL) : \
       ((info)->flags & ~MATE_VFS_FILE_FLAGS_LOCAL)))

/**
 * MATE_VFS_FILE_INFO_SUID:
 * @info: MateVFSFileInfo struct
 *
 * Determines whether a file belongs to the super user.
 */
#define MATE_VFS_FILE_INFO_SUID(info)			\
	((info)->permissions & MATE_VFS_PERM_SUID)

/**
 * MATE_VFS_FILE_INFO_SGID:
 * @info: MateVFSFileInfo struct
 *
 * Determines whether a file belongs to the super user's group.
 */
#define MATE_VFS_FILE_INFO_SGID(info)			\
	((info)->permissions & MATE_VFS_PERM_SGID)

/**
 * MATE_VFS_FILE_INFO_STICKY:
 * @info: MateVFSFileInfo struct
 *
 * Determines whether a file has the sticky bit set, given @info
 */
#define MATE_VFS_FILE_INFO_STICKY(info)		\
	((info)->permissions & MATE_VFS_PERM_STICKY)

/**
 * MATE_VFS_FILE_INFO_SET_SUID:
 * @info: MateVFSFileInfo struct
 * @value: if %TRUE, @info is set to indicate the file belongs to the super user
 *
 * Set the SUID field in @info to @value.
 */
#define MATE_VFS_FILE_INFO_SET_SUID(info, value)		\
       ((info)->flags = (value ? \
       ((info)->flags | MATE_VFS_PERM_SUID) : \
       ((info)->flags & ~MATE_VFS_PERM_SUID)))

/**
 * MATE_VFS_FILE_INFO_SET_SGID:
 * @info: MateVFSFileInfo struct
 * @value: if %TRUE, @info is set to indicate the file belongs to the super user's group
 *
 * Set the SGID field in @info to @value.
 */
#define MATE_VFS_FILE_INFO_SET_SGID(info, value)		\
		((info)->flags = (value ? \
		((info)->flags | MATE_VFS_PERM_SGID) : \
		((info)->flags & ~MATE_VFS_PERM_SGID)))

/**
 * MATE_VFS_FILE_INFO_SET_STICKY:
 * @info: MateVFSFileInfo struct
 * @value: if %TRUE, @info is set to indicate the file has the sticky bit set
 *
 * Set the sticky bit in @info to @value.
 */
#define MATE_VFS_FILE_INFO_SET_STICKY(info, value)			\
	(value ? ((info)->permissions |= MATE_VFS_PERM_STICKY)		\
	       : ((info)->permissions &= ~MATE_VFS_PERM_STICKY))



GType             mate_vfs_file_info_get_type      (void);

MateVFSFileInfo *mate_vfs_file_info_new           (void);
void              mate_vfs_file_info_unref         (MateVFSFileInfo       *info);
void              mate_vfs_file_info_ref           (MateVFSFileInfo       *info);
void              mate_vfs_file_info_clear         (MateVFSFileInfo       *info);
const char *      mate_vfs_file_info_get_mime_type (MateVFSFileInfo       *info);
void              mate_vfs_file_info_copy          (MateVFSFileInfo       *dest,
						     const MateVFSFileInfo *src);
MateVFSFileInfo *mate_vfs_file_info_dup           (const MateVFSFileInfo *orig);
gboolean          mate_vfs_file_info_matches       (const MateVFSFileInfo *a,
						     const MateVFSFileInfo *b);
GList *           mate_vfs_file_info_list_ref      (GList                  *list);
GList *           mate_vfs_file_info_list_unref    (GList                  *list);
GList *           mate_vfs_file_info_list_copy     (GList                  *list);
void              mate_vfs_file_info_list_free     (GList                  *list);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_FILE_INFO_H */
