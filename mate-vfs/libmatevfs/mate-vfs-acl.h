/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-acl.h - ACL Handling for the MATE Virtual File System.
   Access Control List Object

   Copyright (C) 2005 Christian Kellner

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

   Author: Christian Kellner <gicmo@gnome.org>
*/

#ifndef MATE_VFS_ACL_H
#define MATE_VFS_ACL_H

#include <glib.h>
#include <glib-object.h>

#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-ace.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ************************************************************************** */

/* ACL Kinds */

const char *  mate_vfs_acl_kind_to_string   (MateVFSACLKind kind);

enum {
	MATE_VFS_ACL_KIND_NULL = 0,
	MATE_VFS_ACL_USER,
	MATE_VFS_ACL_GROUP,
	MATE_VFS_ACL_OTHER,
	MATE_VFS_ACL_MASK,
	MATE_VFS_ACL_KIND_SYS_LAST
};

/* ACL Permissions */

const char *  mate_vfs_acl_perm_to_string   (MateVFSACLPerm perm);

enum {
	MATE_VFS_ACL_PERM_NULL = 0,
	MATE_VFS_ACL_READ      = 1,
	MATE_VFS_ACL_WRITE,
	MATE_VFS_ACL_EXECUTE,
	MATE_VFS_ACL_PERM_SYS_LAST
};


/* ************************************************************************** */

#define MATE_VFS_TYPE_ACL             (mate_vfs_acl_get_type ())
#define MATE_VFS_ACL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_VFS_TYPE_ACL, MateVFSACL))
#define MATE_VFS_ACL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_VFS_TYPE_ACL, MateVFSACLClass))
#define MATE_VFS_IS_ACL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_VFS_TYPE_ACL))
#define MATE_VFS_IS_ACL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_VFS_TYPE_ACL))

typedef struct _MateVFSACL MateVFSACL;
typedef struct _MateVFSACLClass MateVFSACLClass;

typedef struct _MateVFSACLPrivate MateVFSACLPrivate;

struct _MateVFSACL {
	GObject parent;

	MateVFSACLPrivate *priv;
};

struct _MateVFSACLClass {
	GObjectClass parent_class;
};


GType         mate_vfs_acl_get_type          (void) G_GNUC_CONST;
MateVFSACL * mate_vfs_acl_new               (void);
void          mate_vfs_acl_clear             (MateVFSACL *acl);
void          mate_vfs_acl_set               (MateVFSACL *acl,
					       MateVFSACE *ace);

void          mate_vfs_acl_unset             (MateVFSACL *acl,
					       MateVFSACE *ace);

GList *       mate_vfs_acl_get_ace_list      (MateVFSACL *acl);
void          mate_vfs_acl_free_ace_list     (GList       *ace_list);

#ifdef __cplusplus
}
#endif


#endif /*MATE_VFS_ACL_H*/
