/* mate-vfs-resolve.c - Resolver API

Copyright (C) 2004 Christian Kellner <gicmo@gnome.org>

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

*/

#include <config.h>

#include <errno.h>
#include <string.h>
#include <glib.h>
#ifndef G_OS_WIN32
/* Keep <sys/types.h> above the network includes for FreeBSD. */
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <unistd.h>
#include <sys/time.h>

#ifdef HAVE_RES_NINIT
#include <arpa/nameser.h>
#include <resolv.h>
/* RELOAD_TIMEVAL specifies the minimum of seconds between resolver reloads */
#define RELOAD_TIMEVAL 2
#endif

#include <glib-object.h>

#include <libmatevfs/mate-vfs-resolve.h>

#define INIT_BUFSIZE 8192 /* Unix Network Programming Chapter 11 Page 304 :) */

#if !HAVE_GETADDRINFO && !HAVE_GETHOSTBYNAME_R_GLIBC && !HAVE_GETHOSTBYNAME_R_SOLARIS && !HAVE_GETHOSTBYNAME_R_HPUX
G_LOCK_DEFINE (dns_lock);
# ifndef G_OS_WIN32
#  ifndef h_errno
extern int h_errno;
#  endif /* h_errno */
# endif /* G_OS_WIN32 */
#endif

struct MateVFSResolveHandle_ {
#ifdef HAVE_GETADDRINFO
	   struct addrinfo *result;
	   struct addrinfo *current;
#else
	   GList *result;
	   GList *current;
#endif	
};

#ifndef HAVE_GETADDRINFO
static MateVFSResult
resolvehandle_from_hostent (struct hostent *he, MateVFSResolveHandle **handle)
{
	   MateVFSAddress *addr;
	   GList *result;
	   char **iter;
	   void *aptr;
	   size_t addrlen;
	   struct sockaddr *sa;
	   struct sockaddr_in sin;
#ifdef ENABLE_IPV6	   
	   struct sockaddr_in6 sin6;
#endif

	   switch (he->h_addrtype) {
			 
	   case AF_INET:
			 memset (&sin, 0, sizeof (sin));
			 sin.sin_family = AF_INET;
			 aptr = &(sin.sin_addr);
			 sa = (struct sockaddr *) &sin;
			 addrlen = sizeof (struct sockaddr_in);
			 break;
			 
#ifdef ENABLE_IPV6
	   case AF_INET6:
			 memset (&sin6, 0, sizeof (sin6));
			 sin6.sin6_family = AF_INET6;
			 aptr = &(sin6.sin6_addr);
			 sa = (struct sockaddr *) &sin6;
			 addrlen = sizeof (struct sockaddr_in6);
			 break;
#endif
	   default:
			 return MATE_VFS_ERROR_INTERNAL;
	   }

	   result = NULL;
	   *handle = NULL;
	   
	   for (iter = he->h_addr_list; *iter != NULL; iter++) {
			 g_memmove (aptr, *iter, he->h_length);
			 addr = mate_vfs_address_new_from_sockaddr (sa, addrlen);

			 if (addr != NULL)
				    result = g_list_append (result, addr);
	   }

	   if (result == NULL)
			 return MATE_VFS_ERROR_INTERNAL;

	   *handle = g_new0 (MateVFSResolveHandle, 1);
	   (*handle)->result = result;
	   (*handle)->current = result;
	   
	   return MATE_VFS_OK;
}

#endif

#ifdef HAVE_RES_NINIT
static gboolean
restart_resolve (void)
{
	   static GTimeVal last_reload = { 0, 0 };
	   static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
	   GTimeVal now;
	   gboolean ret;

	   g_static_mutex_lock (&mutex);
	   g_get_current_time (&now);
	   
	   if ((now.tv_sec - last_reload.tv_sec) > RELOAD_TIMEVAL) {

			 last_reload.tv_sec = now.tv_sec;
			 ret = (res_ninit (&_res) == 0);
	   } else {

			 ret = FALSE;
	   }
	
	   g_static_mutex_unlock (&mutex);
	   return ret;
}
#else
#define restart_resolve() FALSE
#endif


#ifdef HAVE_GETADDRINFO
static MateVFSResult
_mate_vfs_result_from_gai_error (int error)
{

	   switch (error) {
			 
	   case EAI_NONAME: return MATE_VFS_ERROR_HOST_NOT_FOUND;
#ifdef EAI_ADDRFAMILY
	   case EAI_ADDRFAMILY: return MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS;
#endif
#ifdef EAI_NODATA
	   case EAI_NODATA: return MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS;
#endif
#ifdef EAI_SYSTEM
	   case EAI_SYSTEM: return mate_vfs_result_from_errno ();
#endif
	   case EAI_FAIL:
	   case EAI_AGAIN: return MATE_VFS_ERROR_NAMESERVER;
			 
	   case EAI_MEMORY: return MATE_VFS_ERROR_NO_MEMORY;
			 
			 /* We should not get these errors there just here to have
			    complete list of getaddrinfo errors*/
	   case EAI_FAMILY:
	   case EAI_SOCKTYPE:
	   case EAI_SERVICE:
	   case EAI_BADFLAGS:
	   default:
			 return MATE_VFS_ERROR_INTERNAL;
	   }
}

#endif

/**
 * mate_vfs_resolve:
 * @hostname: hostname you want to resolve.
 * @handle: pointer to a pointer to a #MateVFSResolveHandle.
 *
 * Tries to resolve @hostname. If the operation was successful you can
 * get the resolved addresses in form of #MateVFSAddress by calling
 * mate_vfs_resolve_next_address().
 * 
 * Return value: A #MateVFSResult indicating the success of the operation.
 *
 * Since: 2.8
 */
MateVFSResult
mate_vfs_resolve (const char              *hostname,
			    MateVFSResolveHandle  **handle)
{
#ifdef HAVE_GETADDRINFO
	   struct addrinfo hints, *result;
	   int res;
	   gboolean retry = TRUE;

restart:	   
	   memset (&hints, 0, sizeof (hints));
	   hints.ai_socktype = SOCK_STREAM;

#ifdef ENABLE_IPV6
# ifdef HAVE_AI_ADDRCONFIG /* RFC3493 */
	   hints.ai_flags = AI_ADDRCONFIG;
	   hints.ai_family = AF_UNSPEC;
# else	
	   hints.ai_family = AF_UNSPEC;
# endif	
#else
	   hints.ai_family = AF_INET;
#endif /* ENABLE_IPV6 */
	   res = getaddrinfo (hostname, NULL, &hints, &result);
	   
	   if (res != 0) {
			 if (retry && restart_resolve ()) {
				    retry = FALSE;
				    goto restart;
			 } else {
				    return _mate_vfs_result_from_gai_error (res);
			 }
	   }

	   *handle = g_new0 (MateVFSResolveHandle, 1);
	   
	   (*handle)->result  = result;
	   (*handle)->current = result;
	   
	   return MATE_VFS_OK;
#else /* HAVE_GETADDRINFO */
	   struct hostent resbuf, *result = &resbuf;
	   MateVFSResult ret;
	   int res;
	   gboolean retry = TRUE;
#ifdef HAVE_GETHOSTBYNAME_R_GLIBC
	   size_t buflen;
	   char *buf;
	   int h_errnop;

restart:	   
	   buf = NULL;
	   buflen = INIT_BUFSIZE;
	   h_errnop = 0;
	   
	   do {
			 buf = g_renew (char, buf, buflen);
			 
			 res = gethostbyname_r (hostname,
							    &resbuf,
							    buf,
							    buflen,
							    &result,
							    &h_errnop);
			 buflen *= 2;

	   } while (res == ERANGE);

	   if (res != 0 || result == NULL || result->h_addr_list[0] == NULL) {
			 g_free (buf);
			 if (retry && restart_resolve ()) {
				    retry = FALSE;
				    goto restart;
			 } else {
				    return mate_vfs_result_from_h_errno_val (h_errnop);
			 }
	   }

	   ret = resolvehandle_from_hostent (result, handle);
	   g_free (buf);
#elif HAVE_GETHOSTBYNAME_R_SOLARIS
	   size_t buflen;
	   char *buf;
	   int h_errnop;	   
restart:
	   
	   buf = NULL;
	   buflen = INIT_BUFSIZE;
	   h_errnop = 0;
	   
	   do {
			 buf = g_renew (char, buf, buflen);
			 
			 result = gethostbyname_r (hostname,
								  &resbuf,
								  buf,
								  buflen,
								  &h_errnop);
			 buflen *= 2;

	   } while (h_errnop == ERANGE);

	   if (result == NULL) {
			 g_free (buf);
		   
			 if (retry && restart_resolve ()) {
				    retry = FALSE;
				    goto restart;
			 } else {
				    return mate_vfs_result_from_h_errno_val (h_errnop);
			 }
	   }

	   ret = resolvehandle_from_hostent (result, handle);	
	   g_free (buf);
#elif HAVE_GETHOSTBYNAME_R_HPUX
	   struct hostent_data buf;

restart:
	   res = gethostbyname_r (hostname, result, &buf);

	   if (res != 0) {
			 if (retry && restart_resolve ()) {
				    retry = FALSE;
				    goto restart;
			 } else {
				    return mate_vfs_result_from_h_errno_val (h_errnop);
			 }
	   }

	   ret = resolvehandle_from_hostent (result, handle);
#else /* !HAVE_GETHOSTBYNAME_R_GLIBC && !HAVE_GETHOSTBYNAME_R_SOLARIS && !HAVE_GETHOSTBYNAME_R_HPUX */
	   res = 0;/* only set to avoid unused variable error */
	   
	   G_LOCK (dns_lock);
restart:
	   result = gethostbyname (hostname);

	   if (result == NULL) {
			 if (retry && restart_resolve ()) {
				    retry = FALSE;
				    goto restart;
			 } else {
#ifndef G_OS_WIN32
				    ret = mate_vfs_result_from_h_errno ();
#else
				    switch (WSAGetLastError ()) {
				    case WSAHOST_NOT_FOUND:
					    ret = MATE_VFS_ERROR_HOST_NOT_FOUND;
					    break;
				    case WSANO_DATA:
					    ret = MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS;
					    break;
				    case WSATRY_AGAIN:
				    case WSANO_RECOVERY:
					    ret = MATE_VFS_ERROR_NAMESERVER;
				    default:
					    ret = MATE_VFS_ERROR_GENERIC;
				    }
#endif
			 }
	   } else {
			 ret = resolvehandle_from_hostent (result, handle);
	   }

	   G_UNLOCK (dns_lock);
#endif /* !HAVE_GETHOSTBYNAME_R_GLIBC && !HAVE_GETHOSTBYNAME_R_SOLARIS && !HAVE_GETHOSTBYNAME_R_HPUX */
	   return ret;
#endif /* HAVE_GETADDRINFO */
}


/**
 * mate_vfs_resolve_reset_to_beginning:
 * @handle: a #MateVFSResolveHandle.
 *
 * Reset @handle so that a following call to mate_vfs_resolve_next_address()
 * will return the first resolved address.
 *
 * Since: 2.8
 */
void
mate_vfs_resolve_reset_to_beginning (MateVFSResolveHandle *handle)
{
	   g_return_if_fail (handle != NULL);

	   handle->current = handle->result;
}

/**
 * mate_vfs_resolve_next_address:
 * @handle: a #MateVFSResolveHandle.
 * @address: a pointer to a pointer to a #MateVFSAddress.
 * 
 * Stores the next #MateVFSAddress available in @handle of the
 * former lookup in @address.
 * 
 * Return value: %TRUE if the next address was stored in @address or
 * %FALSE if no other address is available.
 *
 * Since: 2.8
 */
gboolean
mate_vfs_resolve_next_address (MateVFSResolveHandle  *handle,
				MateVFSAddress       **address)
{
	g_return_val_if_fail (address != NULL, FALSE);
	g_return_val_if_fail (handle != NULL, FALSE);
	
	*address = NULL;
	
#ifdef HAVE_GETADDRINFO   
	while (*address == NULL && handle->current != NULL) {
#ifdef _AIX
		/* getaddrinfo() on AIX 4.3.2 and probably others don't set sa_family in
		   ai_addr so we have to copy it from ai_family */
		handle->current->ai_addr->sa_family = handle->current->ai_family;
#endif
		*address = mate_vfs_address_new_from_sockaddr (handle->current->ai_addr,
								handle->current->ai_addrlen);
		handle->current = handle->current->ai_next;
	}
#else
	if (handle->current) {
		*address = mate_vfs_address_dup (handle->current->data);
		handle->current = handle->current->next;
	}
#endif	
	return *address != NULL;	
}


/**
 * mate_vfs_resolve_free:
 * @handle: a #MateVFSResolveHandle.
 *
 * Use this function to free a #MateVFSResolveHandle returned by
 * mate_vfs_resolve().
 * 
 * Since: 2.8
 */
void
mate_vfs_resolve_free (MateVFSResolveHandle  *handle)
{
#ifdef HAVE_GETADDRINFO
	   if (handle->result != NULL)
			 freeaddrinfo (handle->result);

#else
	   for (handle->current = handle->result; handle->current != NULL;
		   handle->current = handle->current->next) {
			 mate_vfs_address_free (handle->current->data);
	   }
	   g_list_free (handle->result);
	   
#endif
	   g_free (handle);
}

