#include <config.h>

#include "mate-vfs-module-shared.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mate-vfs-module.h"
#include "mate-vfs-ops.h"

/**
 * mate_vfs_mime_type_from_mode:
 * @mode: value as the st_mode field in the system stat structure.
 *
 * Returns a MIME type based on the @mode if it
 * references a special file (directory, device, fifo, socket or symlink).
 * This function works like mate_vfs_mime_type_from_mode_or_default(), except
 * that it returns %NULL where mate_vfs_mime_type_from_mode_or_default()
 * would return a fallback MIME type.
 *
 * Returns: a string containing the MIME type, or %NULL if @mode is not a
 * special file.
 */
const char *
mate_vfs_mime_type_from_mode (mode_t mode)
{
	const char *mime_type;

	if (S_ISREG (mode))
		mime_type = NULL;
	else if (S_ISDIR (mode))
		mime_type = "x-directory/normal";
	else if (S_ISCHR (mode))
		mime_type = "x-special/device-char";
	else if (S_ISBLK (mode))
		mime_type = "x-special/device-block";
	else if (S_ISFIFO (mode))
		mime_type = "x-special/fifo";
#ifdef S_ISLNK
	else if (S_ISLNK (mode))
		mime_type = "x-special/symlink";
#endif
#ifdef S_ISSOCK
	else if (S_ISSOCK (mode))
		mime_type = "x-special/socket";
#endif
	else
		mime_type = NULL;

	return mime_type;
}

/**
 * mate_vfs_mime_type_from_mode_or_default:
 * @mode: value as the st_mode field in the system stat structure.
 * @defaultv: default fallback MIME type.
 *
 * Returns a MIME type based on the @mode if it
 * references a special file (directory, device, fifo, socket or symlink).
 * This function works like mate_vfs_mime_type_from_mode() except that
 * it returns @defaultv instead of %NULL.
 *
 * Returns: a string containing the MIME type, or @defaultv if @mode is not a
 * special file.
 */

const char *
mate_vfs_mime_type_from_mode_or_default (mode_t mode,
					  const char *defaultv)
{
	const char *mime_type;

	mime_type = mate_vfs_mime_type_from_mode (mode);
	if (mime_type == NULL) {
		mime_type = defaultv;
	}

	return mime_type;
}

/**
 * mate_vfs_get_special_mime_type:
 * @uri: a #MateVFSURI to get the mime type for.
 *
 * Gets the MIME type for @uri, this function only returns the type
 * when the uri points to a file that can't be sniffed (sockets, 
 * directories, devices, and fifos).
 *
 * Returns: a string containing the mime type or %NULL if the @uri doesn't 
 * present a special file.
 */

const char *
mate_vfs_get_special_mime_type (MateVFSURI *uri)
{
	MateVFSResult error;
	MateVFSFileInfo *info;
	const char *type;
	
	info = mate_vfs_file_info_new ();
	type = NULL;
	
	/* Get file info and examine the type field to see if file is 
	 * one of the special kinds. 
	 */
	error = mate_vfs_get_file_info_uri (uri, info, MATE_VFS_FILE_INFO_DEFAULT);

	if (error == MATE_VFS_OK && 
	    info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_TYPE) {
		
		switch (info->type) {

			case MATE_VFS_FILE_TYPE_DIRECTORY:
				type = "x-directory/normal";
				break;
				
			case MATE_VFS_FILE_TYPE_CHARACTER_DEVICE:
				type = "x-special/device-char";
				break;
				
			case MATE_VFS_FILE_TYPE_BLOCK_DEVICE:
				type = "x-special/device-block";
				break;
				
			case MATE_VFS_FILE_TYPE_FIFO:
				type = "x-special/fifo";
				break;
				
			case MATE_VFS_FILE_TYPE_SOCKET:
				type = "x-special/socket";
				break;

			default:
				break;
		}

	}
	
	mate_vfs_file_info_unref (info);
	return type;	
}

/**
 * mate_vfs_stat_to_file_info:
 * @file_info: a #MateVFSFileInfo which will be filled.
 * @statptr: pointer to a 'stat' structure.
 *
 * Fills the @file_info structure with the values from @statptr structure.
 */
void
mate_vfs_stat_to_file_info (MateVFSFileInfo *file_info,
			     const struct stat *statptr)
{
	if (S_ISDIR (statptr->st_mode))
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
	else if (S_ISCHR (statptr->st_mode))
		file_info->type = MATE_VFS_FILE_TYPE_CHARACTER_DEVICE;
	else if (S_ISBLK (statptr->st_mode))
		file_info->type = MATE_VFS_FILE_TYPE_BLOCK_DEVICE;
	else if (S_ISFIFO (statptr->st_mode))
		file_info->type = MATE_VFS_FILE_TYPE_FIFO;
#ifdef S_ISSOCK
	else if (S_ISSOCK (statptr->st_mode))
		file_info->type = MATE_VFS_FILE_TYPE_SOCKET;
#endif
	else if (S_ISREG (statptr->st_mode))
		file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
#ifdef S_ISLNK
	else if (S_ISLNK (statptr->st_mode))
		file_info->type = MATE_VFS_FILE_TYPE_SYMBOLIC_LINK;
#endif
	else
		file_info->type = MATE_VFS_FILE_TYPE_UNKNOWN;

	file_info->permissions
		= statptr->st_mode & (MATE_VFS_PERM_USER_ALL
				      | MATE_VFS_PERM_GROUP_ALL
				      | MATE_VFS_PERM_OTHER_ALL
				      | MATE_VFS_PERM_SUID
				      | MATE_VFS_PERM_SGID
				      | MATE_VFS_PERM_STICKY);

	file_info->device = statptr->st_dev;
	file_info->inode = statptr->st_ino;

	file_info->link_count = statptr->st_nlink;

	file_info->uid = statptr->st_uid;
	file_info->gid = statptr->st_gid;

	file_info->size = statptr->st_size;
#ifndef G_OS_WIN32
	file_info->block_count = statptr->st_blocks;
	file_info->io_block_size = statptr->st_blksize;
	if (file_info->io_block_size > 0 &&
	    file_info->io_block_size < 4096) {
		/* Never use smaller block than 4k,
		   should probably be pagesize.. */
		file_info->io_block_size = 4096;
	}
#else
	file_info->io_block_size = 512;
	file_info->block_count =
	  (file_info->size + file_info->io_block_size - 1) /
	  file_info->io_block_size;
#endif
	

	file_info->atime = statptr->st_atime;
	file_info->ctime = statptr->st_ctime;
	file_info->mtime = statptr->st_mtime;

	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_TYPE |
	  MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS | MATE_VFS_FILE_INFO_FIELDS_FLAGS |
	  MATE_VFS_FILE_INFO_FIELDS_DEVICE | MATE_VFS_FILE_INFO_FIELDS_INODE |
	  MATE_VFS_FILE_INFO_FIELDS_LINK_COUNT | MATE_VFS_FILE_INFO_FIELDS_SIZE |
	  MATE_VFS_FILE_INFO_FIELDS_BLOCK_COUNT | MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE |
	  MATE_VFS_FILE_INFO_FIELDS_ATIME | MATE_VFS_FILE_INFO_FIELDS_MTIME |
	  MATE_VFS_FILE_INFO_FIELDS_CTIME | MATE_VFS_FILE_INFO_FIELDS_IDS;
}


