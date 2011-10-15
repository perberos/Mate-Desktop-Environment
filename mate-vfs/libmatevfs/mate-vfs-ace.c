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
           Alvaro Lopez Ortega <alvaro@sun.com>
*/

#include "mate-vfs-ace.h"
#include "mate-vfs-acl.h"

#include <string.h>

/* ************************************************************************** */
/* MateVFSACLEntry */
/* ************************************************************************** */
typedef struct {

	MateVFSACLPerm *perms;
	guint            count;
	guint            array_len;

} PermSet;

struct _MateVFSACEPrivate {
	
	MateVFSACLKind   kind;
	char             *id;

	PermSet           perm_set;
	
	gboolean          negative;
	gboolean          inherit;
	
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MATE_VFS_TYPE_ACE, MateVFSACEPrivate))

static void
mate_vfs_ace_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec);

static void
mate_vfs_ace_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec);
static void 
permset_copy (PermSet *source, PermSet *dest);
static void
permset_set (PermSet *set, MateVFSACLPerm *perms);
	
#if 0
static gboolean
permset_equal (PermSet *a, PermSet *b);
#endif 

/* ************************************************************************** */
/* GObject Stuff */

enum {
	PROP_0 = 0,
	PROP_KIND,
	PROP_ID,
	PROP_PERMS,
	PROP_NEGATIVE,
	PROP_INHERIT
};


static GObjectClass *entry_pclass = NULL;
G_DEFINE_TYPE (MateVFSACE, mate_vfs_ace, G_TYPE_OBJECT);


static void
mate_vfs_ace_finalize (GObject *object)
{
	MateVFSACEPrivate *priv;
	
	priv = MATE_VFS_ACE (object)->priv;
	
	g_free (priv->perm_set.perms);
	g_free (priv->id);
	
	if (G_OBJECT_CLASS (entry_pclass)->finalize)
		(*G_OBJECT_CLASS (entry_pclass)->finalize) (object);
}

static void
mate_vfs_ace_class_init (MateVFSACEClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GParamSpec   *pspec;

	entry_pclass = g_type_class_peek_parent (klass);

	g_type_class_add_private (klass, sizeof (MateVFSACEPrivate));
	
	/* override virtuals */
	gobject_class->finalize     = mate_vfs_ace_finalize;
	gobject_class->set_property = mate_vfs_ace_set_property;
	gobject_class->get_property = mate_vfs_ace_get_property;
	
	/* properties */
	
	pspec =  g_param_spec_uint ("kind",
				    "fixme",
				    "fixme",
				    MATE_VFS_ACL_KIND_NULL,
				    G_MAXUINT32,
				    MATE_VFS_ACL_KIND_NULL,
				    G_PARAM_READWRITE |
				    G_PARAM_CONSTRUCT);

	g_object_class_install_property (gobject_class,
					 PROP_KIND,
					 pspec);



	pspec =  g_param_spec_string ("id",
				      "fixme",
				      "fixme",
				      NULL,
				      G_PARAM_READWRITE |
				      G_PARAM_CONSTRUCT);

	g_object_class_install_property (gobject_class,
					 PROP_ID,
					 pspec);



	pspec = g_param_spec_pointer ("permissions",
				      "Permissions",
				      "fixme",
				      G_PARAM_READWRITE |
				      G_PARAM_CONSTRUCT);

	g_object_class_install_property (gobject_class,
					 PROP_PERMS,
					 pspec);



	pspec = g_param_spec_boolean ("negative",
				      "fixme",
				      "fixme",
				      FALSE,
				      G_PARAM_READWRITE);

	g_object_class_install_property (gobject_class,
					 PROP_NEGATIVE,
					 pspec);


	pspec = g_param_spec_boolean ("inherit",
				      "fixme",
				      "fixme",
				      FALSE,
				      G_PARAM_READWRITE);

	g_object_class_install_property (gobject_class,
					 PROP_INHERIT,
					 pspec);
	
}

static void
mate_vfs_ace_init (MateVFSACE *entry)
{
	entry->priv = GET_PRIVATE (entry);
}

static void
mate_vfs_ace_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	MateVFSACEPrivate *priv;
	MateVFSACLPerm    *pset;
	
	priv = MATE_VFS_ACE (object)->priv;
	
	switch (prop_id) {
		
	case PROP_KIND:
		priv->kind = g_value_get_uint (value);
		break;
		
	case PROP_ID:
		g_free (priv->id);
		priv->id = g_value_dup_string (value);
		break;
		
	case PROP_PERMS:
		pset = g_value_get_pointer (value);
		
		if (pset == NULL) {
			g_free (priv->perm_set.perms);
			priv->perm_set.perms = NULL;
			priv->perm_set.count = 0;
			break;
		}
		
		permset_set (&priv->perm_set, pset);
		break;
		
	case PROP_INHERIT:
		priv->inherit = g_value_get_boolean (value);
		break;
		
	case PROP_NEGATIVE:
		priv->negative = g_value_get_boolean (value);
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
mate_vfs_ace_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	MateVFSACEPrivate *priv;
	
	priv = MATE_VFS_ACE (object)->priv;
	
	switch (prop_id) {
		
	case PROP_KIND:
		g_value_set_uint (value, priv->kind);
		break;
		
	case PROP_ID:
		g_value_set_string (value, priv->id);
		break;
		
	case PROP_PERMS:
		g_value_set_pointer (value, priv->perm_set.perms);
		break;
		
	case PROP_INHERIT:
		g_value_set_boolean (value, priv->inherit);
		break;
		
	case PROP_NEGATIVE:
		g_value_set_boolean (value, priv->negative);
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
	
}

/* ************************************************************************** */

static gboolean
g_str_equal_safe (const char *a, const char *b)
{
	return g_str_equal (a ? a : "", b ? b : "");	
}

/* copy  */
static void 
permset_copy (PermSet *source, PermSet *dest)
{
	int i;
	
	if (dest->array_len < source->count) {
		guint bytes_to_copy;
		
		/* too bad, we need to do some mem ops here */
		bytes_to_copy = sizeof (MateVFSACLPerm) * 
			        (source->count + 1);
		
		g_free (dest->perms);
		dest->perms = g_memdup (source->perms, bytes_to_copy);
		dest->count = dest->array_len = source->count;
		/* sorting? */
		return;
	}

	for (i = 0; i < source->count; i++) {
		dest->perms[i] = source->perms[i];
	}

	dest->perms[source->count] = 0;
	dest->count = source->count;
}

static gint
cmp_perm (int *a, int *b, gpointer *data)
{
	
	if (*a == *b) {
		return 0;
	}
	
	return *a < *b ? -1 : 1; 
}

static void
permset_set (PermSet *set, MateVFSACLPerm *perms)
{
	MateVFSACLPerm *iter;
	guint            bytes_to_copy;
	int              n;
	
	g_assert (perms);
		
	for (iter = perms, n = 0; *iter; iter++) {
		n++;
	}

	bytes_to_copy = sizeof (MateVFSACLPerm) * (n + 1);

	g_free (set->perms);

	set->perms = g_memdup (perms, bytes_to_copy);

	/* sort the array */
	g_qsort_with_data (set->perms,
			   n,
			   sizeof (MateVFSACLPerm),
			   (GCompareDataFunc) cmp_perm,
			   NULL);
	set->count = n;
	set->array_len = n;
}

#if 0
static gboolean
permset_equal (PermSet *a, PermSet *b)
{
	int      i;
	gboolean result;
	
	if (a->count != b->count) {
		return FALSE;
	}
	
	result = TRUE;
	
	/* Permsets MUST be sorted for this to work */
	for (i = 0; i < a->count; i++) {
		if (a->perms[i] != b->perms[i]) {
			result = FALSE;
			break;
		}
	}

	return result;
}
#endif

/* ************************************************************************** */
/* Public Interface */

MateVFSACE *
mate_vfs_ace_new (MateVFSACLKind  kind,
                   const char      *id,
                   MateVFSACLPerm *perms)
{
	MateVFSACE *entry;

	entry = g_object_new (MATE_VFS_TYPE_ACE, 
			      "kind", kind,
			      "id", id,
			      "permissions", perms,
			      NULL);
	return entry;
}

MateVFSACLKind
mate_vfs_ace_get_kind (MateVFSACE *entry)
{
	return entry->priv->kind;
}

void
mate_vfs_ace_set_kind (MateVFSACE      *entry,
                        MateVFSACLKind   kind)
{
	entry->priv->kind = kind;      	
}

const char*
mate_vfs_ace_get_id (MateVFSACE *entry)
{
	return entry->priv->id; 
}

void
mate_vfs_ace_set_id (MateVFSACE *entry,
                      const char  *id)
{
	MateVFSACEPrivate *priv;
	priv = entry->priv;
	
	g_free (priv->id);
	priv->id = g_strdup (id);
}

void 
mate_vfs_ace_set_inherit (MateVFSACE *entry,
			   gboolean     inherit)
{
	g_object_set (G_OBJECT(entry), "inherit", inherit, NULL);
}

gboolean
mate_vfs_ace_get_inherit (MateVFSACE *entry)
{
	gboolean inherit;

	g_object_get (G_OBJECT(entry), "inherit", &inherit, NULL);
	return inherit;
}


void 
mate_vfs_ace_set_negative (MateVFSACE *entry,
			    gboolean     negative)
{
	g_object_set (G_OBJECT(entry), "negative", negative, NULL);
}

gboolean
mate_vfs_ace_get_negative (MateVFSACE *entry)
{
	gboolean negative;

	g_object_get (G_OBJECT(entry), "negative", &negative, NULL);
	return negative;
}


const MateVFSACLPerm  *
mate_vfs_ace_get_perms (MateVFSACE *entry)
{
	return entry->priv->perm_set.perms; 
}

void
mate_vfs_ace_set_perms (MateVFSACE     *entry,
                         MateVFSACLPerm *perms)
{
	permset_set (&entry->priv->perm_set, perms);	
}

void
mate_vfs_ace_add_perm (MateVFSACE     *entry,
                        MateVFSACLPerm  perm)
{
	MateVFSACEPrivate *priv;
	PermSet            *permset;
	gint                i;

	g_assert (MATE_VFS_IS_ACE(entry));

	priv    = entry->priv;
	permset = &priv->perm_set;

	for (i=0; i<permset->count; i++) {
		if (permset->perms[i] == perm)
			return;
	}
	
	if (permset->array_len < permset->count) {
		permset->perms = g_realloc (permset->perms, 
					    sizeof(MateVFSACLPerm) * (permset->count + 2));
		permset->array_len++;
	}

	permset->perms[permset->count] = perm;
	permset->count++;

	g_qsort_with_data (permset->perms,
			   permset->count++,
			   sizeof (MateVFSACLPerm),
			   (GCompareDataFunc) cmp_perm,
			   NULL);
}

void
mate_vfs_ace_del_perm (MateVFSACE      *entry,
			MateVFSACLPerm   perm)
{
	guint               i;
	MateVFSACEPrivate *priv;
	PermSet            *permset;

	priv    = entry->priv;
	permset = &priv->perm_set;

	if (permset->perms == NULL) 
		return;

	for (i=0; i<permset->count; i++) {
		if (permset->perms[i] == perm) {
			g_memmove (&permset->perms[i], &permset->perms[i+1], permset->count - i);
			permset->count--;
			break;
		}
	}
}

gboolean
mate_vfs_ace_check_perm (MateVFSACE     *entry,
                          MateVFSACLPerm  perm)
{
	MateVFSACEPrivate *priv;
	MateVFSACLPerm    *piter;
	
	priv = entry->priv;
	
	if (priv->perm_set.perms == NULL || perm == 0) {
		return FALSE;	
	}
	
	for (piter = priv->perm_set.perms; *piter; piter++) {
		if (*piter == perm) {
			return TRUE;	
		}
	}
	
	return FALSE;
}

void
mate_vfs_ace_copy_perms (MateVFSACE     *source,
			  MateVFSACE     *dest)
{
	MateVFSACEPrivate *priv_source;
	MateVFSACEPrivate *priv_dest;
	
	priv_source = source->priv;
	priv_dest = dest->priv;
	
	permset_copy (&priv_source->perm_set, &priv_dest->perm_set);
}


gboolean
mate_vfs_ace_equal (MateVFSACE     *entry_a,
                     MateVFSACE     *entry_b)
{
	MateVFSACEPrivate *priv_a;
	MateVFSACEPrivate *priv_b;
	
	priv_a = entry_a->priv;
	priv_b = entry_b->priv;


	return (priv_a->kind == priv_b->kind &&
	        g_str_equal_safe (priv_a->id, priv_b->id) &&
	        priv_a->inherit == priv_b->inherit &&
	        priv_a->negative == priv_b->negative);
}
