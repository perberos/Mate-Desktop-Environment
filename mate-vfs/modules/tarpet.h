#ifndef TARPET_H
#define TARPET_H

/*
 * Tarpet - Tar struct definitions
 * 
 * Placed in the public domain by Abigail Brady <morwen@evilmagic.org>
 *
 * Implemented from a definition provided by Rachel Hestilow, 
 * md5sum 7606d69d61dfc7eed10857a888136b62
 *
 * See documentation-1.1.txt for details.
 *
 */

struct TARPET_POSIX {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char username[32];
  char groupname[32];
  char major[8];
  char minor[8];
  char extend[155];
};

#define TARPET_TYPE_REGULAR    '\0'
#define TARPET_TYPE_REGULAR2   '0'
#define TARPET_TYPE_LINK       '1'
#define TARPET_TYPE_SYMLINK    '2'
#define TARPET_TYPE_CHARDEV    '3'
#define TARPET_TYPE_BLOCKDEV   '4'
#define TARPET_TYPE_DIRECTORY  '5'
#define TARPET_TYPE_FIFO       '6'
#define TARPET_TYPE_CONTIGUOUS '7'

#define TARPET_TYPE_DUMPDIR    'D'
#define TARPET_TYPE_LONGLINKN  'K'
#define TARPET_TYPE_LONGFILEN  'L'
#define TARPET_TYPE_MULTIVOL   'M'
#define TARPET_TYPE_LONGNAME   'N'
#define TARPET_TYPE_SPARSE     'S'
#define TARPET_TYPE_VOLUME     'V'

#define TARPET_GNU_MAGIC       "ustar"
#define TARPET_GNU_MAGIC_OLD   "ustar  "

#define TARPET_GNU_VERSION "00"

/*
 * for the mode, #include <sys/types.h> and use
 *
 * S_ISUID
 * S_ISGID
 * 
 * S_IRUSR
 * S_IWUSR
 * S_IXUSR
 * S_IRGRP
 * S_IWGRP
 * S_IXGRP
 * S_IROTH
 * S_IWOTH
 * S_IXOTH
 */

struct TARPET_sparsefile {
  char padding[12];
  char numbytes[12];
};

struct TARPET_GNU_ext {
  char atime[12];
  char ctime[12];
  char offset[12];
  char realsize[12];
  char longnames[4];
  char padding[68];
  struct TARPET_sparsefile sparse[16];
  char extend;
};

struct TARPET_GNU_ext_old {
  char padding[345];
  char atime[12];
  char ctime[12];
  char longnames[4];
  char padding2;
  struct TARPET_sparsefile sparse[4];
  char extend;
  char realsize[12];
};

struct TARPET_GNU_sparseheader {
  struct TARPET_sparsefile sparse[21];
  char extend;
};

struct TARPET_rawdata {
  char data[512];
};

union TARPET_block {
  struct TARPET_POSIX p;
  struct TARPET_GNU_ext gnu;
  struct TARPET_GNU_ext_old gnu_old;
  struct TARPET_GNU_sparseheader sparse;
  struct TARPET_rawdata raw;
};

#endif
