/* mate-vfs-address.c - Address functions

   Copyright (C) 2004 Christian Kellner

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

#include <libmatevfs/mate-vfs-address.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef G_OS_WIN32
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

struct _MateVFSAddress {

	   struct sockaddr *sa;
};

#define SA(__s)				((struct sockaddr *) __s)

#define SIN_LEN				sizeof (struct sockaddr_in)
#define SIN(__s)				((struct sockaddr_in *) __s)

#ifdef ENABLE_IPV6

/* OSX seems to already define this */
# if defined(SIN6_LEN) && defined(__APPLE__)
#  undef SIN6_LEN
# endif

# define SIN6_LEN			sizeof (struct sockaddr_in6)
# define SIN6(__s)				((struct sockaddr_in6 *) __s)
# define VALID_AF(__sa)			(__sa->sa_family == AF_INET  || __sa->sa_family == AF_INET6)
# define SA_SIZE(__sa)			(__sa->sa_family == AF_INET ? SIN_LEN : \
   								      SIN6_LEN)
# define AF_SIZE(__af)			(__af == AF_INET6 ? SIN6_LEN : SIN_LEN)
# define MAX_ADDRSTRLEN			INET6_ADDRSTRLEN

#else /* ENABLE_IPV6 */

# define VALID_AF(__sa)			(__sa->sa_family == AF_INET)
# define AF_SIZE(__af)			SIN_LEN
# define SA_SIZE(_sa)			SIN_LEN
# define MAX_ADDRSTRLEN			INET_ADDRSTRLEN

#endif


/* Register MateVFSAddress in the glib type system */
GType 
mate_vfs_address_get_type (void)
{
	static GType addr_type = 0;

	if (addr_type == 0) {
		addr_type = g_boxed_type_register_static ("MateVFSAddress",
				(GBoxedCopyFunc) mate_vfs_address_dup,
				(GBoxedFreeFunc) mate_vfs_address_free);
	}

	return addr_type;
}

												 

/**
 * mate_vfs_address_new_from_string:
 * @address: A string representation of the address.
 * 
 * Creates a new #MateVFSAddress from the given string or %NULL
 * if @address isn't a valid.
 * 
 * Return value: The new #MateVFSAddress.
 *
 * Since: 2.8
 **/
MateVFSAddress *
mate_vfs_address_new_from_string (const char *address)
{
	struct sockaddr_in sin;
#ifdef G_OS_WIN32
	int address_length;
#endif

#if (defined (G_OS_WIN32) || defined (HAVE_INET_PTON)) && defined (ENABLE_IPV6)
	struct sockaddr_in6 sin6;
#endif

	sin.sin_family = AF_INET;

#ifdef G_OS_WIN32
	address_length = SIN_LEN;
	if (WSAStringToAddress ((LPTSTR) address, AF_INET, NULL,
				(LPSOCKADDR) &sin.sin_addr,
				&address_length) == 0)
		return mate_vfs_address_new_from_sockaddr (SA (&sin), SIN_LEN);
#  ifdef ENABLE_IPV6
	address_length = SIN6_LEN;
	if (WSAStringToAddress ((LPTSTR) address, AF_INET6, NULL,
				(LPSOCKADDR) &sin6.sin6_addr,
				&address_length) == 0) {
		sin6.sin6_family = AF_INET6;
		return mate_vfs_address_new_from_sockaddr (SA (&sin6), SIN6_LEN);
	}
#  endif  /* ENABLE_IPV6 */

#elif defined (HAVE_INET_PTON)
	if (inet_pton (AF_INET, address, &sin.sin_addr) > 0)
		return mate_vfs_address_new_from_sockaddr (SA (&sin), SIN_LEN);
#  ifdef ENABLE_IPV6		
	if (inet_pton (AF_INET6, address, &sin6.sin6_addr) > 0) {
		sin6.sin6_family = AF_INET6;
		return mate_vfs_address_new_from_sockaddr (SA (&sin6), SIN6_LEN);
	}
#  endif /* ENABLE_IPV6 */

#elif defined (HAVE_INET_ATON)
	if (inet_aton (address, &sin.sin_addr) > 0)
		return mate_vfs_address_new_from_sockaddr (SA (&sin), SIN_LEN);
#else
	if ((sin.sin_addr.s_addr = inet_addr (address)) != INADDR_NONE)
		return mate_vfs_address_new_from_sockaddr (SA (&sin), SIN_LEN);
#endif	

	return NULL;
}

/**
 * mate_vfs_address_new_from_sockaddr:
 * @sa: A pointer to a sockaddr.
 * @len: The size of @sa.
 *
 * Creates a new #MateVFSAddress from @sa.
 *
 * Return value: The new #MateVFSAddress 
 * or %NULL if @sa was invalid or the address family isn't supported.
 *
 * Since: 2.8
 **/
MateVFSAddress *
mate_vfs_address_new_from_sockaddr (struct sockaddr *sa,
				     int              len)
{
	MateVFSAddress *addr;

	g_return_val_if_fail (sa != NULL, NULL);
	g_return_val_if_fail (len == AF_SIZE (sa->sa_family), NULL);

	if (VALID_AF (sa) == FALSE) {
		return NULL;
	}	
       	
	addr = g_new0 (MateVFSAddress, 1);
	addr->sa = g_memdup (sa, len);

	return addr;
}

/**
 * mate_vfs_address_new_from_ipv4:
 * @ipv4_address: A IPv4 Address in network byte order
 *
 * Creates a new #MateVFSAddress from @ipv4_address.
 *
 * Note that this function should be avoided because newly written
 * code should be protocol independent.
 * 
 * Return value: A new #MateVFSAdress.
 *
 * Since: 2.8
 **/
MateVFSAddress *
mate_vfs_address_new_from_ipv4 (guint32 ipv4_address)
{
	MateVFSAddress *addr;
	struct sockaddr_in *sin;

	sin = g_new0 (struct sockaddr_in, 1);
	sin->sin_addr.s_addr = ipv4_address;
	sin->sin_family = AF_INET;

	addr = mate_vfs_address_new_from_sockaddr (SA (sin), SIN_LEN);

	return addr;
}

/**
 * mate_vfs_address_get_family_type:
 * @address: A pointer to a #MateVFSAddress
 *
 * Use this function to retrive the address family of @address.
 * 
 * Return value: The address family of @address.
 *
 * Since: 2.8
 **/
int
mate_vfs_address_get_family_type (MateVFSAddress *address)
{
	g_return_val_if_fail (address != NULL, -1);

	return address->sa->sa_family;
}


/**
 * mate_vfs_address_to_string:
 * @address: A pointer to a #MateVFSAddress
 * 
 * Translate @address to a printable string.
 * 
 * Returns: A newly alloced string representation of @address which
 * the caller must free.
 *
 * Since: 2.8
 **/
char *
mate_vfs_address_to_string (MateVFSAddress *address)
{
#ifdef G_OS_WIN32
	char text_addr[100];
	DWORD text_length = sizeof (text_addr);

	if (WSAAddressToString (address->sa, SA_SIZE (address->sa),
				NULL, text_addr, &text_length) == 0)
		return g_strdup (text_addr);

	return NULL;
#else
	const char *text_addr;
#ifdef HAVE_INET_NTOP
	char buf[MAX_ADDRSTRLEN];
#endif	
	g_return_val_if_fail (address != NULL, NULL);

	text_addr = NULL;

	switch (address->sa->sa_family) {
#if defined (ENABLE_IPV6) && defined (HAVE_INET_NTOP)	
	case AF_INET6:
			 
		text_addr = inet_ntop (AF_INET6,
				       &SIN6 (address->sa)->sin6_addr,
				       buf,
				       sizeof (buf));
		break;
#endif
	case AF_INET:
#if HAVE_INET_NTOP
		text_addr = inet_ntop (AF_INET,
				       &SIN (address->sa)->sin_addr,
				       buf,
				       sizeof (buf));
#else
		text_addr = inet_ntoa (SIN (address->sa)->sin_addr);
#endif  /* HAVE_INET_NTOP */
		break;
	}
	   
	  
	return text_addr != NULL ? g_strdup (text_addr) : NULL;
#endif
}

/**
 * mate_vfs_address_get_ipv4:
 * @address: A #MateVFSAddress.
 * 
 * Returns: The associated IPv4 address in network byte order.
 *
 * Note that you should avoid using this function because newly written
 * code should be protocol independent.
 *
 * Since: 2.8
 **/
guint32
mate_vfs_address_get_ipv4 (MateVFSAddress *address)
{
	g_return_val_if_fail (address != NULL, 0);
	g_return_val_if_fail (address->sa != NULL, 0);

	if (address->sa->sa_family != AF_INET)
		return 0;

	return (guint32) SIN (address->sa)->sin_addr.s_addr;
}


/**
 * mate_vfs_address_get_sockaddr:
 * @address: A #MateVFSAddress
 * @port: A valid port in host byte order to set in the returned sockaddr
 * structure.
 * @len: A pointer to an int which will contain the length of the
 * return sockaddr structure.
 *
 * This function tanslates @address into a equivalent
 * sockaddr structure. The port specified at @port will
 * be set in the structure and @len will be set to the length
 * of the structure.
 * 
 * 
 * Return value: A newly allocated sockaddr structure the caller must free
 * or %NULL if @address did not point to a valid #MateVFSAddress.
 **/
struct sockaddr *
mate_vfs_address_get_sockaddr (MateVFSAddress *address,
				guint16          port,
				int             *len)
{
	struct sockaddr *sa;

	g_return_val_if_fail (address != NULL, NULL);

	sa = g_memdup (address->sa, SA_SIZE (address->sa));

	switch (address->sa->sa_family) {
#ifdef ENABLE_IPV6
	case AF_INET6:
		SIN6 (sa)->sin6_port = g_htons (port);

		if (len != NULL) {
			*len = SIN6_LEN;
		}
		
		break;
#endif
	case AF_INET:
		SIN (sa)->sin_port = g_htons (port);

		if (len != NULL) {
			*len = SIN_LEN;
		}
		break;

	}

	return sa;
}

static gboolean
v4_v4_equal (const struct sockaddr_in *a,
	     const struct sockaddr_in *b)
{
	return a->sin_addr.s_addr == b->sin_addr.s_addr;
}

static gboolean
v4_v4_match (const struct sockaddr_in *a,
	     const struct sockaddr_in *b,
	     guint                     prefix)
{
	struct in_addr cmask;
	
	cmask.s_addr = g_htonl (~0 << (32 - prefix));

	return (a->sin_addr.s_addr & cmask.s_addr) == 
		(b->sin_addr.s_addr & cmask.s_addr);
}

#ifdef ENABLE_IPV6
static gboolean
v6_v6_equal (const struct sockaddr_in6 *a,
	     const struct sockaddr_in6 *b)
{
	return IN6_ARE_ADDR_EQUAL (&a->sin6_addr, &b->sin6_addr);
}

static gboolean
v4_v6_match (const struct sockaddr_in  *a4,
	     const struct sockaddr_in6 *b6,
	     guint                      prefix)
{
	struct  sockaddr_in b4;
	guint32 v4_numeric;
	
	if (! IN6_IS_ADDR_V4MAPPED (&b6->sin6_addr)) {
		return FALSE;	
	}
	
	memset (&b4, 0, sizeof (b4));

	v4_numeric = b6->sin6_addr.s6_addr[12] << 24 | 
		     b6->sin6_addr.s6_addr[13] << 16 |
		     b6->sin6_addr.s6_addr[14] << 8  | 
		     b6->sin6_addr.s6_addr[15];

	b4.sin_addr.s_addr = g_htonl (v4_numeric);	

	if (prefix == 0 || prefix > 31) {
		return v4_v4_equal (a4, &b4);
	}	
	
	return v4_v4_match (a4, &b4, prefix);
}

static gboolean
v6_v6_match (const struct sockaddr_in6 *a,
	     const struct sockaddr_in6 *b,
	     guint                      prefix)
{
	guint8        i;
	guint8        n;
	const guint8 *ia;
	const guint8 *ib;

	/* XXX: optimize that for known platfroms (e.g.: Linux) */
	
	/* n are bytes in this for here */
	n = prefix / 8;

	ia = a->sin6_addr.s6_addr;
	ib = b->sin6_addr.s6_addr;
	
	for (i = 0; i < n; i++) {
		if (*ia++ != *ib++) {
			return FALSE;
		}
	}
	
	/* n are the rest bits (if any) for here */
	if ((n = (8 - prefix % 8)) != 8) {
		if ((*ia & (0xff << n)) != (*ib & (0xff << n))) {
			return FALSE;
		}
	}
	
	return TRUE;
}
#endif

/**
 * mate_vfs_address_match:
 * @a: A #MateVFSAddress
 * @b: A #MateVFSAddress to compare with @a
 * @prefix: Number of bits to take into account starting
 * from the left most one.
 * 
 * Matches the first @prefix number of bits of two given #MateVFSAddress 
 * objects and returns TRUE of they match otherwise FALSE. This function can
 * also match mapped address (i.e. IPv4 mapped IPv6 addresses).
 * 
 * Return value: TRUE if the two addresses match.
 *
 * Since: 2.14 
 **/
gboolean
mate_vfs_address_match (const MateVFSAddress *a,
			 const MateVFSAddress *b,
			 guint                  prefix)
{
	guint8 fam_a;
       	guint8 fam_b;

	g_return_val_if_fail (a != NULL || a->sa != NULL, FALSE);
	g_return_val_if_fail (b != NULL || b->sa != NULL, FALSE);

	fam_a = a->sa->sa_family;
	fam_b = b->sa->sa_family;

	if (fam_a == AF_INET && fam_b == AF_INET) {
		if (prefix == 0 || prefix > 31) {
			return v4_v4_equal (SIN (a->sa), SIN (b->sa));
		} else {
			return v4_v4_match (SIN (a->sa), SIN (b->sa), prefix);
		}		
	}
#ifdef ENABLE_IPV6
	else if (fam_a == AF_INET6 && fam_b == AF_INET6) {
		if (prefix == 0 || prefix > 127) {
			return v6_v6_equal (SIN6 (a->sa), SIN6 (b->sa));
		} else {
			return v6_v6_match (SIN6 (a->sa), SIN6 (b->sa), prefix);
		}
	} else if (fam_a == AF_INET && fam_b == AF_INET6) {
		return v4_v6_match (SIN (a->sa), SIN6 (b->sa), prefix);
	} else if (fam_a == AF_INET6 && fam_b == AF_INET) {
		return v4_v6_match (SIN (b->sa), SIN6 (a->sa), prefix);
	} 
#endif

	/* This is the case when we are trying to compare unkown family types */
	g_assert_not_reached ();
	return FALSE;
}


/**
 * mate_vfs_address_equal:
 * @a: A #MateVFSAddress
 * @b: A #MateVFSAddress to compare with @a
 *
 * Compares two #MateVFSAddress objects and returns TRUE if they have the
 * addresses are equal as well as the address family type. 
 * 
 * Return value: TRUE if the two addresses match.
 *
 * Since: 2.14 
 **/
gboolean
mate_vfs_address_equal (const MateVFSAddress *a,
			 const MateVFSAddress *b)
{
	guint8 fam_a;
       	guint8 fam_b;

	g_return_val_if_fail (a != NULL || a->sa != NULL, FALSE);
	g_return_val_if_fail (b != NULL || b->sa != NULL, FALSE);

	
	fam_a = a->sa->sa_family;
	fam_b = b->sa->sa_family;

	if (fam_a == AF_INET && fam_b == AF_INET) {
		return v4_v4_equal (SIN (a->sa), SIN (b->sa));		
	}
#ifdef ENABLE_IPV6
	else if (fam_a == AF_INET6 && fam_b == AF_INET6) {
		return v6_v6_equal (SIN6 (a->sa), SIN6 (b->sa));
	}	
#endif
	return FALSE;
}


/**
 * mate_vfs_address_dup:
 * @address: A #MateVFSAddress.
 * 
 * Duplicates @address.
 * 
 * Return value: Duplicated @address or %NULL if @address was not valid.
 *
 * Since: 2.8
 **/
MateVFSAddress *
mate_vfs_address_dup (MateVFSAddress *address)
{
	MateVFSAddress *addr;

	g_return_val_if_fail (address != NULL, NULL);
	g_return_val_if_fail (VALID_AF (address->sa), NULL);

	addr = g_new0 (MateVFSAddress, 1);
	addr->sa = g_memdup (address->sa, SA_SIZE (address->sa));

	return addr;
}

/**
 * mate_vfs_address_free:
 * @address: A #MateVFSAddress.
 *
 * Frees the memory allocated for @address.
 *
 * Since: 2.8
 **/
void
mate_vfs_address_free (MateVFSAddress *address)
{
	g_return_if_fail (address != NULL);

	g_free (address->sa);	
	g_free (address);
}


