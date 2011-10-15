/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-acl.h - ACL Handling for the MATE Virtual File System.
   Virtual File System.

   Copyright (C) 2005 Christian Kellner
   Copyright (C) 2005 Sun Microsystems

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

   Authors: Christian Kellner <gicmo@gnome.org>
            Alvaro Lopez Ortega <alvaro@sun.com>
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#ifdef HAVE_GRP_H
# include <grp.h>
#endif

#ifdef HAVE_POSIX_ACL
# include <acl/libacl.h>
#define HAVE_ACL 1
#endif

#ifdef HAVE_SOLARIS_ACL
# include <sys/acl.h>
#define HAVE_ACL 1
#endif

#include "file-method-acl.h"

#define CMD_PERM_READ           4
#define CMD_PERM_WRITE          2
#define CMD_PERM_EXECUTE        1
#define CMD_PERM_COND_EXECUTE   8


#ifdef HAVE_ACL

static char *
uid_to_string (uid_t uid) 
{
	char *uid_string = NULL;
#ifdef HAVE_PWD_H
	struct passwd *pw = NULL;
	gpointer buffer = NULL;
	gint error;

#if defined (HAVE_POSIX_GETPWUID_R) || defined (HAVE_NONPOSIX_GETPWUID_R)	
	struct passwd pwd;
	glong  bufsize;
	
#ifdef _SC_GETPW_R_SIZE_MAX
	bufsize = sysconf (_SC_GETPW_R_SIZE_MAX);
#else
	bufsize = 64;
#endif /* _SC_GETPW_R_SIZE_MAX */

	
	do {
		g_free (buffer);

		/* why bufsize + 6 see #156446 */
		buffer = g_malloc (bufsize + 6);
		errno = 0;
		
#ifdef HAVE_POSIX_GETPWUID_R
	    error = getpwuid_r (uid, &pwd, buffer, bufsize, &pw);
        error = error < 0 ? errno : error;
#else /* HAVE_NONPOSIX_GETPWUID_R */
       /* HPUX 11 falls into the HAVE_POSIX_GETPWUID_R case */
#if defined(_AIX) || defined(__hpux)
	    error = getpwuid_r (uid, &pwd, buffer, bufsize);
	    pw = error == 0 ? &pwd : NULL;
#else /* !_AIX */
        pw = getpwuid_r (uid, &pwd, buffer, bufsize);
        error = pw ? 0 : errno;
#endif /* !_AIX */            
#endif /* HAVE_NONPOSIX_GETPWUID_R */
		
	    if (pw == NULL) {
		    if (error == 0 || error == ENOENT) {
				break;
		    }
	    }

	    if (bufsize > 32 * 1024) {
			break;
	    }

	    bufsize *= 2;
	    
	} while (pw == NULL);
#endif /* HAVE_POSIX_GETPWUID_R || HAVE_NONPOSIX_GETPWUID_R */

	if (pw == NULL) {
		setpwent ();
		pw = getpwuid (uid);
		endpwent ();
	}

	if (pw != NULL) {
		uid_string = g_strdup (pw->pw_name);
	}

#endif /* HAVE_PWD_H */
	
	if (uid_string == NULL) {
		uid_string = g_strdup_printf ("%d", uid);
	}
	
	return uid_string;	
}

static char *
gid_to_string (gid_t gid) 
{
	char *gid_string = NULL;
#ifdef HAVE_GETGRGID_R
	struct group *gr = NULL;
	gpointer buffer = NULL;
	gint error;
	struct group grp;
	glong  bufsize;
	
#ifdef _SC_GETGR_R_SIZE_MAX
	bufsize = sysconf (_SC_GETGR_R_SIZE_MAX);
#else
	bufsize = 64;
#endif /* _SC_GETPW_R_SIZE_MAX */

	do {
		g_free (buffer);

		/* why bufsize + 6 see #156446 */
		buffer = g_malloc (bufsize + 6);
	    error = getgrgid_r (gid, &grp, buffer, bufsize, &gr);
	    
        error = error < 0 ? errno : error;
	
		if (gr == NULL) {
		    if (error == 0 || error == ENOENT) {
				break;
		    }
	    }

	    if (bufsize > 32 * 1024) {
			break;
	    }

	    bufsize *= 2;
	    
	} while (gr == NULL);
	
	if (gr != NULL) {
		gid_string = g_strdup (gr->gr_name);
	}
	
#endif
	if (gid_string == NULL) {
		gid_string = g_strdup_printf ("%d", gid);
	}
	
	return gid_string;
}

static uid_t
string_to_uid (const char *uid)
{
	struct passwd *passwd;
	
	passwd = getpwnam(uid);
	if (passwd == NULL) return 0;
	
	return passwd->pw_uid;
}


static gid_t
string_to_gid (const char *gid)
{
	struct group *group;
        
	group = getgrnam (gid);
	if (group == NULL) return 0;
        
	return group->gr_gid;
}


/* ************************************************************************** */
/* POSIX ACL */

#ifdef HAVE_POSIX_ACL
#define POSIX_N_TAGS 3

static int
permset_to_perms (acl_permset_t set, MateVFSACLPerm *tags)
{
	int i;
	
	memset (tags, 0, sizeof (MateVFSACLPerm) * (POSIX_N_TAGS + 1));
	i = 0;

	if (acl_get_perm (set, ACL_READ) == 1) {
		tags[0] = MATE_VFS_ACL_READ;
		i++;
	}
	
	if (acl_get_perm (set, ACL_WRITE) == 1) {
		tags[i] = MATE_VFS_ACL_WRITE;
		i++;
	}
	
	if (acl_get_perm (set, ACL_EXECUTE)) {
		tags[i] = MATE_VFS_ACL_EXECUTE;	
	}

	return i;
}

static acl_entry_t
find_entry (acl_t acl, acl_tag_t type, id_t id)
{
        acl_entry_t  ent;
        acl_tag_t    e_type;
        id_t        *e_id_p;
	
        if (acl_get_entry(acl, ACL_FIRST_ENTRY, &ent) != 1)
                return NULL;
	
        for(;;) {
                acl_get_tag_type(ent, &e_type);
                if (type == e_type) {
                        if (id == ACL_UNDEFINED_ID) 
                                return ent;
			
			e_id_p = acl_get_qualifier(ent);
			
			if (e_id_p == NULL)
				return NULL;
			
			if (*e_id_p == id) {
				acl_free(e_id_p);
				return ent;
			}
			
			acl_free(e_id_p);
                }
		
                if (acl_get_entry(acl, ACL_NEXT_ENTRY, &ent) != 1)
                        return NULL;
        }
}

static void
set_permset (acl_permset_t permset, mode_t perm)
{
        if (perm & CMD_PERM_READ)
                acl_add_perm (permset, ACL_READ);
        else
                acl_delete_perm (permset, ACL_READ);
	   
        if (perm & CMD_PERM_WRITE)
                acl_add_perm (permset, ACL_WRITE);
        else
                acl_delete_perm (permset, ACL_WRITE);
	   
        if (perm & CMD_PERM_EXECUTE)
                acl_add_perm (permset, ACL_EXECUTE);
        else
                acl_delete_perm (permset, ACL_EXECUTE);
}

static int
clone_entry (acl_t  from_acl, acl_tag_t from_type,
	     acl_t *to_acl,   acl_tag_t to_type)
{
        acl_entry_t from_entry;
	acl_entry_t to_entry;
	
        from_entry = find_entry(from_acl, from_type, ACL_UNDEFINED_ID);
        if (from_entry == NULL) return 1;
	
	if (acl_create_entry(to_acl, &to_entry) != 0)
		return -1;
	
	acl_copy_entry(to_entry, from_entry);
	acl_set_tag_type(to_entry, to_type);
	
	return 0;
}

static int
posix_acl_read (MateVFSACL *acl,
                acl_t        p_acl,
                gboolean     def)
{
	acl_entry_t    entry;
	int            res;
	int            e_id;
	int            n;

	if (p_acl == NULL) {
		return 0;	
	}

	n = 0;
	e_id = ACL_FIRST_ENTRY;	
	while ((res = acl_get_entry (p_acl, e_id, &entry)) == 1) {
		MateVFSACLPerm   pset[POSIX_N_TAGS + 1];
		MateVFSACLKind   kind;
		MateVFSACE      *ace;
		acl_permset_t     e_ps;
		acl_tag_t         e_type;
		void             *e_qf;
		char             *id;	
		
		e_id   = ACL_NEXT_ENTRY;	
		e_type = ACL_UNDEFINED_ID;
		e_qf   = NULL;
		
		/* prop = (def) ? MATE_VFS_ACL_DEFAULT : MATE_VFS_ACL_TYPE_NULL; */
		/* Read "default"	
		 */
	
		res = acl_get_tag_type (entry, &e_type);

		if (res == -1 || e_type == ACL_UNDEFINED_ID || e_type == ACL_MASK) {
			continue;
		}
		
		if (def == FALSE && (e_type != ACL_USER && e_type != ACL_GROUP)) {
			/* skip the standard unix permissions */
			continue;	
		}

		res = acl_get_permset (entry, &e_ps);

		if (res == -1) {
			continue;
		}
		
		e_qf = acl_get_qualifier (entry);
		
		id   = NULL;
		kind = MATE_VFS_ACL_KIND_NULL;
		switch (e_type) {

		case ACL_USER:
			id = uid_to_string (*(uid_t *) e_qf);	
			/* FALLTHROUGH */
			
		case ACL_USER_OBJ:
			kind = MATE_VFS_ACL_USER;
			break;

		case ACL_GROUP:
			id = gid_to_string (*(gid_t *) e_qf);
			/* FALLTHROUGH */

		case ACL_GROUP_OBJ:
			kind = MATE_VFS_ACL_GROUP;
			break;
			
		case ACL_MASK:
		case ACL_OTHER:
			kind = MATE_VFS_ACL_OTHER;
			break;
		}
		
		permset_to_perms (e_ps, pset);
		ace = mate_vfs_ace_new (kind, id, pset);

		g_free (id);

		if (def) {
			g_object_set (G_OBJECT(ace), "inherit", def, NULL);
		}

		mate_vfs_acl_set (acl, ace);
		g_object_unref (ace);

		if (e_qf) {
			acl_free (e_qf);
		}
		
		n++;
	}
	
	return n;
}

#endif /* HAVE_POSIX_ACL */


/* ************************************************************************** */
/* SOLARIS ACL */

#ifdef HAVE_SOLARIS_ACL
#define SOLARIS_N_TAGS 3

static gboolean
fixup_acl (MateVFSACL *acl, GSList *acls)
{
	GSList   *i;
	gboolean  defaults     = FALSE;
	gboolean  user_obj     = FALSE;
	gboolean  group_obj    = FALSE;
	gboolean  other_obj    = FALSE;
	gboolean  mask_obj     = FALSE;
	gboolean  class_needed = FALSE;
	gboolean  changed      = FALSE;

	/* Is there some default entry?
	 */
	for (i=acls; i != NULL; i=i->next) {
		const char       *id_str;
		MateVFSACLKind   kind;
		MateVFSACE      *ace = MATE_VFS_ACE(i->data);
	
		if (!mate_vfs_ace_get_inherit(ace)) 
			continue;

		defaults = TRUE;

		id_str = mate_vfs_ace_get_id (ace);
		kind   = mate_vfs_ace_get_kind (ace);

		switch (kind) {
		case MATE_VFS_ACL_USER:
			if (id_str == NULL)
				user_obj = TRUE;
			else
				class_needed = TRUE;
			break;
		case MATE_VFS_ACL_GROUP:
			if (id_str == NULL)
				group_obj = TRUE;
			else
				class_needed = TRUE;
			break;
	        case MATE_VFS_ACL_OTHER:
			if (id_str == NULL)
				other_obj = TRUE;
			break;
	        case MATE_VFS_ACL_MASK:
			if (id_str == NULL)
				mask_obj = TRUE;
			break;
		default:
			break;
		}
	}

	if (!defaults) return FALSE;

	/* Add the missing ACEs
	 */
	if (!user_obj) {
		MateVFSACE     *ace;
		MateVFSACLPerm perms[] = {MATE_VFS_ACL_READ,
					   MATE_VFS_ACL_WRITE,
					   MATE_VFS_ACL_EXECUTE, 0};

		ace = mate_vfs_ace_new (MATE_VFS_ACL_USER, NULL, perms);
		mate_vfs_ace_set_inherit (ace, TRUE);
		mate_vfs_acl_set (acl, ace);
		g_slist_append(acls, ace);

		changed = TRUE;
	}

	if (!group_obj) {
		MateVFSACE     *ace;
		MateVFSACLPerm  perms[] = {0};

		ace = mate_vfs_ace_new (MATE_VFS_ACL_GROUP, NULL, perms);
		mate_vfs_ace_set_inherit (ace, TRUE);
		mate_vfs_acl_set (acl, ace);

		changed = TRUE;
	}

	if (!other_obj) {
		MateVFSACE     *ace;
		MateVFSACLPerm  perms[] = {0};

		ace = mate_vfs_ace_new (MATE_VFS_ACL_OTHER, NULL, perms);
		mate_vfs_ace_set_inherit (ace, TRUE);
		mate_vfs_acl_set (acl, ace);

		changed = TRUE;
	}	

	if (class_needed && !mask_obj) {
		MateVFSACE     *ace;
		MateVFSACLPerm  perms[] = {MATE_VFS_ACL_READ,
					    MATE_VFS_ACL_WRITE,
					    MATE_VFS_ACL_EXECUTE, 0};

		ace = mate_vfs_ace_new (MATE_VFS_ACL_MASK, NULL, perms);
		mate_vfs_ace_set_inherit (ace, TRUE);
		mate_vfs_acl_set (acl, ace);

		changed = TRUE;
	}

	return changed;
}

static int
permset_to_perms (int set, MateVFSACLPerm *tags)
{
	int i;
	
	memset (tags, 0, sizeof (MateVFSACLPerm) * (SOLARIS_N_TAGS + 1));
	i = 0;

	if (set & 4) {
		tags[i] = MATE_VFS_ACL_READ;
		i++;
	}

	if (set & 2) {
		tags[i] = MATE_VFS_ACL_WRITE;
		i++;
	}

	if (set & 1) {
		tags[i] = MATE_VFS_ACL_EXECUTE;
		i++;
	}

	return i;
}

static int
solaris_acl_read (MateVFSACL *acl,
		  aclent_t    *aclp,
		  int          aclcnt,
		  gboolean     def)
{
	int       i;
	aclent_t *tp;

	for (tp = aclp, i=0; i < aclcnt; tp++, i++) {
		MateVFSACE     *ace;
		MateVFSACLKind  kind;
		MateVFSACLPerm  pset[SOLARIS_N_TAGS+1];
		char            *id;

		id   = NULL;
		kind = MATE_VFS_ACL_KIND_NULL;

		switch (tp->a_type) {
		case USER:
		case DEF_USER:
			id = uid_to_string(tp->a_id);
		case USER_OBJ:
		case DEF_USER_OBJ:
			kind = MATE_VFS_ACL_USER;
			break;

		case GROUP:
		case DEF_GROUP:
			id = gid_to_string(tp->a_id);			
		case GROUP_OBJ:
		case DEF_GROUP_OBJ:
			kind = MATE_VFS_ACL_GROUP;
			break;

		case OTHER_OBJ:
		case DEF_OTHER_OBJ:
			kind = MATE_VFS_ACL_OTHER;
			break;

		case CLASS_OBJ:
		case DEF_CLASS_OBJ:
			kind = MATE_VFS_ACL_MASK;
			break;

		default:
			g_warning ("Unhandled Solaris ACE: %d\n", tp->a_type);
		}

		permset_to_perms (tp->a_perm, pset);
		ace = mate_vfs_ace_new (kind, id, pset);

		if (tp->a_type & ACL_DEFAULT)
			mate_vfs_ace_set_inherit (ace, TRUE);

		mate_vfs_acl_set (acl, ace);
		g_object_unref (ace);
	}

	return 0;
}

static MateVFSResult
translate_ace_into_aclent (MateVFSACE *ace, aclent_t *aclp)
{
 	int               re;
	gboolean          is_default = FALSE;
	const char       *id_str;
	MateVFSACLKind   kind;
	int               id;
	
	id_str     = mate_vfs_ace_get_id (ace);
	kind       = mate_vfs_ace_get_kind (ace);
	is_default = mate_vfs_ace_get_inherit (ace);

	aclp->a_perm = 0;
	aclp->a_id   = 0;
	aclp->a_type = 0;

	/* Permissions
	 */
	if (mate_vfs_ace_check_perm (ace, MATE_VFS_ACL_READ)) {
		aclp->a_perm |= 4;
	}
	if (mate_vfs_ace_check_perm (ace, MATE_VFS_ACL_WRITE)) {
		aclp->a_perm |= 2;
	} 
	if (mate_vfs_ace_check_perm (ace, MATE_VFS_ACL_EXECUTE)) {
		aclp->a_perm |= 1;
	}

	/* Kind
	 */
	id = NULL;
	switch (kind) {
		case MATE_VFS_ACL_USER:
			if (id_str) {
				aclp->a_type = (is_default) ? DEF_USER : USER;
				aclp->a_id = string_to_uid (id_str);
			} else {
				aclp->a_type = (is_default) ? DEF_USER_OBJ : USER_OBJ;
			}
			break;
		case MATE_VFS_ACL_GROUP:
			if (id_str) {
				aclp->a_type = (is_default) ? DEF_GROUP : GROUP;
				aclp->a_id = string_to_gid (id_str);
			} else {
				aclp->a_type = (is_default) ? DEF_GROUP_OBJ : GROUP_OBJ;				
			}
			break;
	        case MATE_VFS_ACL_OTHER:
			aclp->a_type = (is_default) ? DEF_OTHER_OBJ : OTHER_OBJ;
			break;

		case MATE_VFS_ACL_MASK:
			aclp->a_type = (is_default) ? DEF_CLASS_OBJ : CLASS_OBJ;
			break;

		default:
			return MATE_VFS_ERROR_NOT_SUPPORTED;			
	}

	return MATE_VFS_OK;
}

#endif /* HAVE_SOLARIS_ACL */



/* ************************************************************************** */
/* Common */

static MateVFSResult
aclerrno_to_vfserror (int acl_errno)
{
	switch (acl_errno) {
	case ENOENT:			 
	case EINVAL:       
		return MATE_VFS_ERROR_BAD_FILE;
	case ENOSYS:
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	case EACCES:     
		return MATE_VFS_ERROR_ACCESS_DENIED;
	case ENAMETOOLONG: 
		return MATE_VFS_ERROR_NAME_TOO_LONG;
	case EPERM:
		return MATE_VFS_ERROR_NOT_PERMITTED;
	case ENOSPC:
		return MATE_VFS_ERROR_NO_SPACE;
	case EROFS:
		return MATE_VFS_ERROR_READ_ONLY_FILE_SYSTEM;
	default:
		break;
	}
	
	return MATE_VFS_ERROR_GENERIC;
}

#endif /* HAVE_ACL */

MateVFSResult file_get_acl (const char       *path,
                             MateVFSFileInfo *info,
                             struct stat      *statbuf,
                             MateVFSContext  *context)
{
#ifdef HAVE_SOLARIS_ACL	
	int       re;
	int       aclcnt;
	aclent_t *aclp;

	if (info->acl != NULL) {
		mate_vfs_acl_clear (info->acl);
	} else {
		info->acl = mate_vfs_acl_new ();
	}
	
	aclcnt = acl (path, GETACLCNT, 0, NULL);
	if (aclcnt < 0) {
		return aclerrno_to_vfserror (errno);
	}

	if (aclcnt < MIN_ACL_ENTRIES) {
		return MATE_VFS_ERROR_INTERNAL;
	}

	aclp = (aclent_t *)malloc(sizeof (aclent_t) * aclcnt);
	if (aclp == NULL) {
		return MATE_VFS_ERROR_NO_MEMORY;		
	}

	errno = 0;
	re = acl (path, GETACL, aclcnt, aclp);
	if (re < 0) {
		return aclerrno_to_vfserror (errno);		
	}
	
	re = solaris_acl_read (info->acl, aclp, aclcnt, FALSE);
	if (re < 0) {
		return MATE_VFS_ERROR_INTERNAL;
	}

	info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_ACL;
	
	free (aclp);
	
	return MATE_VFS_OK;

#elif defined(HAVE_POSIX_ACL)
	acl_t p_acl;
	int   n;
	
	if (info->acl != NULL) {
		mate_vfs_acl_clear (info->acl);
	} else {
		info->acl = mate_vfs_acl_new ();
	}
	
	p_acl = acl_get_file (path, ACL_TYPE_ACCESS);
	n = posix_acl_read (info->acl, p_acl, FALSE);
	
	if (p_acl) {
		acl_free (p_acl);
	}
		
	if (S_ISDIR (statbuf->st_mode)) {
		p_acl = acl_get_file (path, ACL_TYPE_DEFAULT);
		
		n += posix_acl_read (info->acl, p_acl, TRUE);
		
		if (p_acl) {
			acl_free (p_acl);
		}
	}
	
	if (n > 0) {
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_ACL;
	} else {
		g_object_unref (info->acl);
		info->acl = NULL;
	}
	
	return MATE_VFS_OK;
#else
	return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif
}


MateVFSResult 
file_set_acl (const char             *path,
	      const MateVFSFileInfo *info,
              MateVFSContext        *context)
{
#ifdef HAVE_SOLARIS_ACL
	GList           *acls;
	GList           *entry;
	guint            len;
	MateVFSResult   re;
	aclent_t        *new_aclp;
	aclent_t        *taclp;
	guint            aclp_i;
	gboolean         changed;

	if (info->acl == NULL) 
		return MATE_VFS_ERROR_BAD_PARAMETERS;

	acls = mate_vfs_acl_get_ace_list (info->acl);
	if (acls == NULL) return MATE_VFS_OK;

	changed = fixup_acl (info->acl, acls);	
	if (changed) {
		mate_vfs_acl_free_ace_list (acls);

		acls = mate_vfs_acl_get_ace_list (info->acl);
		if (acls == NULL) return MATE_VFS_OK;
	}

	len = g_list_length (acls);
	if (len <= 0) return MATE_VFS_OK;

	new_aclp = (aclent_t *) malloc (len * sizeof(aclent_t));
	if (new_aclp == NULL) return MATE_VFS_ERROR_NO_MEMORY;
	memset (new_aclp, 0, len * sizeof(aclent_t));

	aclp_i = 0;
	taclp  = new_aclp;
	for (entry=acls; entry != NULL; entry = entry->next) {
		MateVFSACE *ace = MATE_VFS_ACE(entry->data);
		
		re = translate_ace_into_aclent (ace, taclp);
		if (re != MATE_VFS_OK) continue;

		aclp_i++;
		taclp++;
	}

	/* Sort it out
	 */
 	re = aclsort (aclp_i, 0, (aclent_t *)new_aclp);
	if (re == -1) {
		g_free (new_aclp);
		return MATE_VFS_ERROR_INTERNAL;
	}		

	/* Commit it to the file system
	 */
	re = acl (path, SETACL, aclp_i, (aclent_t *)new_aclp); 
 	if (re < 0) {
		int err = errno;

		g_free (new_aclp);
		return aclerrno_to_vfserror(err);
	}

	g_free (new_aclp);
	return MATE_VFS_OK;
	
#elif defined(HAVE_POSIX_ACL)
	GList *acls;
	GList *entry;
	acl_t  acl_obj;
	acl_t  acl_obj_default;


	if (info->acl == NULL) 
		return MATE_VFS_ERROR_BAD_PARAMETERS;
	
	/* POSIX ACL object
	 */
	acl_obj_default = acl_get_file (path, ACL_TYPE_DEFAULT);     

	acl_obj = acl_get_file (path, ACL_TYPE_ACCESS);      
	if (acl_obj == NULL) return MATE_VFS_ERROR_GENERIC;

	/* Parse stored information
	 */
	acls = mate_vfs_acl_get_ace_list (info->acl);
	if (acls == NULL) return MATE_VFS_OK;

	for (entry=acls; entry != NULL; entry = entry->next) {
		MateVFSACE           *ace        = MATE_VFS_ACE(entry->data);
		gboolean               is_default = FALSE;
		const char            *id_str;
		MateVFSACLKind        kind;
		int                    id;

		int                    re;
		acl_tag_t              type;
		mode_t                 perms      = 0;
		acl_entry_t            new_entry  = NULL;
		acl_permset_t          permset    = NULL;
		
		id_str     = mate_vfs_ace_get_id (ace);
		kind       = mate_vfs_ace_get_kind (ace);
		is_default = mate_vfs_ace_get_inherit (ace);
		
		/* Perms
		 */
		if (mate_vfs_ace_check_perm (ace, MATE_VFS_ACL_READ))
			perms |= CMD_PERM_READ;
		else if (mate_vfs_ace_check_perm (ace, MATE_VFS_ACL_WRITE))
			perms |= CMD_PERM_WRITE;
		else if (mate_vfs_ace_check_perm (ace, MATE_VFS_ACL_EXECUTE))
			perms |= CMD_PERM_EXECUTE;
		
		/* Type
		 */
		switch (kind) {
		case MATE_VFS_ACL_USER:
			id   = string_to_uid (id_str);
			type = ACL_USER;			
			break;

		case MATE_VFS_ACL_GROUP:
			id   = string_to_gid (id_str);
			type = ACL_GROUP;
			break;

		case MATE_VFS_ACL_OTHER:
			type = ACL_OTHER;
			break;

		default:
			return MATE_VFS_ERROR_NOT_SUPPORTED;			
		}

		/* Add the entry
		 */
		new_entry = find_entry (acl_obj, type, id);
		if (new_entry == NULL) {
			/* new */
			if (is_default)
				re = acl_create_entry (&acl_obj_default, &new_entry);
			else
				re = acl_create_entry (&acl_obj, &new_entry);
			if (re != 0) return aclerrno_to_vfserror (errno);
			
			/* e_tag */
			re = acl_set_tag_type (new_entry, type);
			if (re != 0) return aclerrno_to_vfserror (errno);
			
			/* e_id */
			re = acl_set_qualifier (new_entry, &id);
			if (re != 0) return aclerrno_to_vfserror (errno);
		}

		/* e_perm */
		re = acl_get_permset (new_entry, &permset);
		if (re != 0) return aclerrno_to_vfserror (errno);
		set_permset (permset, perms);

		/* Fix it up
		 */
		if (is_default && (acl_obj_default != NULL)) {
			if (! find_entry (acl_obj_default, ACL_USER_OBJ, ACL_UNDEFINED_ID)) {
				clone_entry (acl_obj, ACL_USER_OBJ, &acl_obj_default, ACL_USER_OBJ);
			}
			
			if (! find_entry (acl_obj_default, ACL_GROUP_OBJ, ACL_UNDEFINED_ID)) {
				clone_entry (acl_obj, ACL_GROUP_OBJ, &acl_obj_default, ACL_GROUP_OBJ);
			}
			
			if (! find_entry (acl_obj_default, ACL_OTHER, ACL_UNDEFINED_ID)) {
				clone_entry (acl_obj, ACL_OTHER, &acl_obj_default, ACL_OTHER);
			}			
		}
		
		if (acl_equiv_mode (acl_obj, NULL) != 0) {

			if (! find_entry (acl_obj, ACL_MASK, ACL_UNDEFINED_ID)) {                       
				clone_entry (acl_obj, ACL_GROUP_OBJ, &acl_obj, ACL_MASK);
			}

			if (is_default)
				re = acl_calc_mask (&acl_obj_default);
			else 
				re = acl_calc_mask (&acl_obj);

			if (re != 0) return aclerrno_to_vfserror (errno);
		}
	}
		
	mate_vfs_acl_free_ace_list (acls);
	return MATE_VFS_OK;
#else
	return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif
}


