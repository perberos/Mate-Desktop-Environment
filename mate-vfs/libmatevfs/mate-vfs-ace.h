/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-acl.h - ACL Handling for the MATE Virtual File System.
   Access Control Entry Class

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

#ifndef MATE_VFS_ACE_H
#define MATE_VFS_ACE_H

#include <glib.h>
#include <glib-object.h>


#ifdef __cplusplus
extern "C" {
#endif


#define MATE_VFS_TYPE_ACE             (mate_vfs_ace_get_type ())
#define MATE_VFS_ACE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),  MATE_VFS_TYPE_ACE, MateVFSACE))
#define MATE_VFS_ACE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),   MATE_VFS_TYPE_ACE, MateVFSACEClass))
#define MATE_VFS_IS_ACE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  MATE_VFS_TYPE_ACE))
#define MATE_VFS_IS_ACE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),   MATE_VFS_TYPE_ACE))

typedef struct _MateVFSACE        MateVFSACE;
typedef struct _MateVFSACEClass   MateVFSACEClass;

typedef struct _MateVFSACEPrivate MateVFSACEPrivate;

struct _MateVFSACE {
	GObject parent;

	MateVFSACEPrivate *priv;
};

struct _MateVFSACEClass {
	GObjectClass parent_class;

	void (*reserved1) (void);
	void (*reserved2) (void);
	void (*reserved3) (void);
	void (*reserved4) (void);
};


typedef guint32 MateVFSACLKind;
typedef guint32 MateVFSACLPerm;


GType                   mate_vfs_ace_get_type       (void) G_GNUC_CONST;

MateVFSACE *           mate_vfs_ace_new            (MateVFSACLKind  kind,
                                                      const char      *id,
                                                      MateVFSACLPerm *perms);

MateVFSACLKind         mate_vfs_ace_get_kind       (MateVFSACE      *entry);
void                    mate_vfs_ace_set_kind       (MateVFSACE      *entry,
                                                      MateVFSACLKind   kind);

const char *            mate_vfs_ace_get_id         (MateVFSACE      *entry);
void                    mate_vfs_ace_set_id         (MateVFSACE      *entry,
                                                      const char       *id);

gboolean                mate_vfs_ace_get_inherit    (MateVFSACE *entry);
void                    mate_vfs_ace_set_inherit    (MateVFSACE *entry,
						      gboolean     inherit);

gboolean                mate_vfs_ace_get_negative   (MateVFSACE *entry);
void                    mate_vfs_ace_set_negative   (MateVFSACE *entry,
						      gboolean     negative);

const MateVFSACLPerm * mate_vfs_ace_get_perms      (MateVFSACE      *entry);
void                    mate_vfs_ace_set_perms      (MateVFSACE      *entry,
                                                      MateVFSACLPerm  *perms);
void                    mate_vfs_ace_add_perm       (MateVFSACE      *entry,
                                                      MateVFSACLPerm   perm);
void                    mate_vfs_ace_del_perm       (MateVFSACE      *entry,
						      MateVFSACLPerm   perm);
gboolean                mate_vfs_ace_check_perm     (MateVFSACE      *entry,
                                                      MateVFSACLPerm   perm);
void                    mate_vfs_ace_copy_perms     (MateVFSACE      *source,
                                                      MateVFSACE      *dest);

gboolean                mate_vfs_ace_equal          (MateVFSACE  *entry_a,
                                                      MateVFSACE  *entry_b);

#ifdef __cplusplus
}
#endif

#endif /*MATE_VFS_ACE_H*/

