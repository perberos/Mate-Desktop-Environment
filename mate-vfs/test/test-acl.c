/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-acl.c - ACL Handling for the MATE Virtual File System.
   Test Code

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <glib.h>

#include <libmatevfs/mate-vfs.h>

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

static gboolean at_least_one_test_failed = FALSE;

#define TEST_ASSERT(__xpr__) \
	G_STMT_START {if (!(__xpr__)) {test_failed ("%s", G_STRLOC);} } G_STMT_END

static void
test_failed (const char *format, ...)
{
	va_list arguments;
	char *message;

	va_start (arguments, format);
	message = g_strdup_vprintf (format, arguments);
	va_end (arguments);

	g_print ("test failed: %s\n", message);
	at_least_one_test_failed = TRUE;
}

static void
stop_after_log (const char *domain, GLogLevelFlags level, 
	const char *message, gpointer data)
{
	void (* saved_handler) (int);
	
	g_log_default_handler (domain, level, message, data);

	saved_handler = signal (SIGINT, SIG_IGN);
	raise (SIGINT);
	signal (SIGINT, saved_handler);
}

static void
make_asserts_break (const char *domain)
{
	g_log_set_handler (domain, 
		 (GLogLevelFlags) (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING),
		 stop_after_log, NULL);
}

static void
auto_test ()
{
	MateVFSACLPerm pset_a[4] = {
		MATE_VFS_ACL_READ,
		MATE_VFS_ACL_WRITE,
		MATE_VFS_ACL_EXECUTE,
		0};
	MateVFSACLPerm pset_b[1] = {0};
	MateVFSACL *acl;	                                     
	MateVFSACE *a, *b;
	GList       *list, *iter;
	gboolean     bl;
	int          n;
	
	g_print ("Testing ACE Object\n");
	
	a = mate_vfs_ace_new (MATE_VFS_ACL_USER, "a", pset_a);
	b = mate_vfs_ace_new (MATE_VFS_ACL_USER, "a", NULL);
	
	TEST_ASSERT (a != NULL);
	TEST_ASSERT (b != NULL);
	
	bl = mate_vfs_ace_equal (a, b);
	TEST_ASSERT (bl == TRUE);
	
	mate_vfs_ace_set_id (b, "b");
	bl = mate_vfs_ace_equal (a, b);
	TEST_ASSERT (bl == FALSE);
	
	TEST_ASSERT (mate_vfs_ace_check_perm (a, MATE_VFS_ACL_READ));
	TEST_ASSERT (mate_vfs_ace_check_perm (a, MATE_VFS_ACL_WRITE));
	TEST_ASSERT (mate_vfs_ace_check_perm (a, MATE_VFS_ACL_EXECUTE));
	             
	TEST_ASSERT (!mate_vfs_ace_check_perm (b, MATE_VFS_ACL_READ));
	TEST_ASSERT (!mate_vfs_ace_check_perm (b, MATE_VFS_ACL_WRITE));
	TEST_ASSERT (!mate_vfs_ace_check_perm (b, MATE_VFS_ACL_EXECUTE));
	             
	mate_vfs_ace_copy_perms (a, b);
	
	TEST_ASSERT (mate_vfs_ace_check_perm (b, MATE_VFS_ACL_READ));
	TEST_ASSERT (mate_vfs_ace_check_perm (b, MATE_VFS_ACL_WRITE));
	TEST_ASSERT (mate_vfs_ace_check_perm (b, MATE_VFS_ACL_EXECUTE));
	             
	mate_vfs_ace_set_perms (a, pset_b);
	
	TEST_ASSERT (!mate_vfs_ace_check_perm (a, MATE_VFS_ACL_READ));
	TEST_ASSERT (!mate_vfs_ace_check_perm (a, MATE_VFS_ACL_WRITE));
	TEST_ASSERT (!mate_vfs_ace_check_perm (a, MATE_VFS_ACL_EXECUTE));
	             
	g_print ("Testing ACL Object\n");
	
	acl = mate_vfs_acl_new ();
	TEST_ASSERT (acl != NULL);
	
	mate_vfs_acl_set (acl, a);
	mate_vfs_acl_set (acl, b);
	
	list = mate_vfs_acl_get_ace_list (acl);
	
	for (iter = list, n = 0; iter; iter = iter->next) {
		TEST_ASSERT (MATE_VFS_IS_ACE (iter->data));
		
		bl = mate_vfs_ace_equal (MATE_VFS_ACE (iter->data), a) |
		     mate_vfs_ace_equal (MATE_VFS_ACE (iter->data), b);
		n++;
		TEST_ASSERT (bl == TRUE);
	}
	
	TEST_ASSERT (n == 2);
	
	g_object_unref (a);
	g_object_unref (b);
	
	mate_vfs_acl_free_ace_list (list);
	
	g_object_unref (acl);
}

int main (int argc, char **argv)
{
	make_asserts_break ("MateVFS");
	
	mate_vfs_init ();

	if (argc == 1) {
		g_print ("ACL auto test mode\n");
		auto_test ();		
	}

	mate_vfs_shutdown ();
	
	return at_least_one_test_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
