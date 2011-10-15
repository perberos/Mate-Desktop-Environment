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

#include "mate-vfs-acl.h"

/* ************************************************************************** */
/* GObject Stuff */

static GObjectClass *parent_class = NULL;

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
			  MATE_VFS_TYPE_ACL, MateVFSACLPrivate))

G_DEFINE_TYPE (MateVFSACL, mate_vfs_acl, G_TYPE_OBJECT);

struct _MateVFSACLPrivate {
	GList    *entries;
	gboolean  modified;
};

static void
mate_vfs_acl_finalize (GObject *object)
{
	MateVFSACL        *acl;
	MateVFSACLPrivate *priv;
	GList              *iter;
	
	acl = MATE_VFS_ACL (object);
	priv = acl->priv;
	
	for (iter = priv->entries; iter; iter = iter->next) {
		MateVFSACE *ace;
		
		ace = MATE_VFS_ACE (iter->data);
		
		g_object_unref (ace);
	}
	
	g_list_free (priv->entries);
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
mate_vfs_acl_class_init (MateVFSACLClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	g_type_class_add_private (klass, sizeof (MateVFSACLPrivate));

	gobject_class->finalize = mate_vfs_acl_finalize;
}

static void
mate_vfs_acl_init (MateVFSACL *acl)
{
	acl->priv = GET_PRIVATE (acl);
	
}

/* ************************************************************************** */
/* Public Interface */

/**
 * mate_vfs_acl_new:
 *
 * Creates a new #MateVFSACL object.
 *
 * Return value: The new #MateVFSACL 
 *
 * Since: 2.16
 **/

MateVFSACL *
mate_vfs_acl_new (void)
{
	MateVFSACL *acl;	

	acl = g_object_new (MATE_VFS_TYPE_ACL, NULL);

	return acl;
}

/**
 * mate_vfs_acl_clear:
 * @acl: A valid #MateVFSACL object
 *
 * This will clear the #MateVFSACL object resetting it to a
 * state exaclty as if it was newly created.
 * 
 * Since: 2.16
 **/
void
mate_vfs_acl_clear (MateVFSACL *acl)
{
	MateVFSACLPrivate *priv;
	MateVFSACE        *entry;
	GList              *iter;	
	
	priv = acl->priv;
	
	entry = NULL;
	
	for (iter = priv->entries; iter; iter = iter->next) {
		entry = MATE_VFS_ACE (iter->data);
		
		g_object_unref (entry);		
	}

	g_list_free (priv->entries);

	priv->entries = NULL;
}

/**
 * mate_vfs_acl_set:
 * @acl: A valid #MateVFSACL object
 * @ace: A #MateVFSACE to set in @acl
 *
 * This will search the #MateVFSACL object specified at @acl for an 
 * existing #MateVFSACE that machtes @ace. If found it will update the
 * permission on the object otherwise it will ref @ace and insert it
 * into it's list. 
 *
 * Since: 2.16
 **/
void
mate_vfs_acl_set (MateVFSACL *acl,
                   MateVFSACE *ace)
{
	MateVFSACLPrivate *priv;
	MateVFSACE        *entry;
	GList              *iter;	

	g_return_if_fail (MATE_VFS_IS_ACE (ace));

	priv = acl->priv;
	entry = NULL;

	/* that could be made faster with some HashTable foo,
	 * not sure it is worth the effort, though */
	for (iter = priv->entries; iter; iter = iter->next) {
		entry = MATE_VFS_ACE (iter->data);

		if (mate_vfs_ace_equal (entry, ace)) {
			break;	
		}		
	}

	if (iter == NULL) {
		priv->entries = g_list_prepend (priv->entries, g_object_ref (ace));
	} else {
		mate_vfs_ace_copy_perms (ace, entry);
	}
}

/**
 * mate_vfs_acl_unset:
 * @acl: A valid #MateVFSACL object
 * @ace: A #MateVFSACE to remove from @acl
 *
 * This will search the #MateVFSACL object specified at @acl for an 
 * existing #MateVFSACE that machtes @ace. If found it will remove it
 * from its internal list.
 *
 * Since: 2.16
 **/
void
mate_vfs_acl_unset (MateVFSACL *acl,
                     MateVFSACE *ace)
{
	MateVFSACLPrivate *priv;
	MateVFSACE        *entry;
	GList              *iter;	
	
	priv = acl->priv;
	
	entry = NULL;
	
	/* that could be made faster with some HashTable foo,
	 * not sure it is worth the effort, though */
	for (iter = priv->entries; iter; iter = iter->next) {
		entry = MATE_VFS_ACE (iter->data);
		
		if (mate_vfs_ace_equal (entry, ace)) {
			break;	
		}		
	}
	
	if (iter != NULL) {
		g_object_unref (entry);
		priv->entries = g_list_remove_link (priv->entries, iter);
	}
}

/**
 * mate_vfs_acl_get_ace_list:
 * @acl: A valid #MateVFSACL object
 *
 * This will create you a list of all #MateVFSACE objects that are
 * contained in @acl. 
 *
 * Return value: A newly created list you have to free with 
 * mate_vfs_acl_get_ace_list.
 * 
 * Since: 2.16
 **/
GList*
mate_vfs_acl_get_ace_list (MateVFSACL *acl)
{
	MateVFSACLPrivate *priv;
	GList              *list;
	
	priv = acl->priv;
	
	list = g_list_copy (priv->entries);
	
	g_list_foreach (list, (GFunc) g_object_ref, NULL);
	
	return list;
}

/**
 * mate_vfs_ace_free_ace_list:
 * @ace_list: A GList return by mate_vfs_acl_get_ace_list
 *
 * This will unref all the #MateVFSACE in the list and free it
 * afterwards
 * 
 * Since: 2.16
 **/
void
mate_vfs_acl_free_ace_list (GList *ace_list)
{
	g_list_foreach (ace_list, (GFunc) g_object_unref, NULL);
	g_list_free (ace_list);
}

/* ************************************************************************** */
/* ************************************************************************** */
const char *
mate_vfs_acl_kind_to_string (MateVFSACLKind kind)
{
	const char *value;
	
	if (kind < MATE_VFS_ACL_KIND_SYS_LAST) {
		
		switch (kind) {
		
		case MATE_VFS_ACL_USER:
			value = "user";
			break;
			
		case MATE_VFS_ACL_GROUP:
			value = "group";
			break;
				
		case MATE_VFS_ACL_OTHER:
			value = "other";
			break;
			
		default:
			value = "unknown";
			break;
		}
		
	} else {
		value = "unknown";
	}
	return value;		
}

const char *
mate_vfs_acl_perm_to_string (MateVFSACLPerm perm)
{
	const char *value;
	
	if (perm < MATE_VFS_ACL_PERM_SYS_LAST) {
		
		switch (perm) {
	
		case MATE_VFS_ACL_READ:
			value = "read";
			break;
			
		case MATE_VFS_ACL_WRITE:
			value = "write";
			break;
		
		case MATE_VFS_ACL_EXECUTE:
			value = "execute";
			break;
				
		default:
			value = "unknown";
			break;	
		}
	} else {
		value = "unknown";	
	}
	
	return value;
}

