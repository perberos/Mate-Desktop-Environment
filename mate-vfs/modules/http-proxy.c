/* ************************************************************************* */
/* FIXME: Move this to mate-vfs-proxy () so we can reuse that in ftp too   */

/* Proxy Stuff .. not cleaned yet, completely taken from the old http module  
   Ettore Perazzoli <ettore@gnu.org> (core HTTP)
   Ian McKellar <yakk@yakk.net> (WebDAV/PUT)
   Michael Fleming <mfleming@eazel.com> (Caching, Cleanup)
*/

#include <config.h>

/* Keep <sys/types.h> above any network includes for FreeBSD. */
#include <sys/types.h>
/* Keep <netinet/in.h> above <arpa/inet.h> for FreeBSD. */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mateconf/mateconf-client.h>
#include <libmatevfs/mate-vfs-private-utils.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* for atoi */
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>

#include "http-proxy.h"

/* #define DEBUG_HTTP(x) (g_print x) */
#define DEBUG_HTTP(x)

/* Standard HTTP proxy port */
#define DEFAULT_HTTP_PROXY_PORT 8080

/* MateConf paths and keys */
#define PATH_MATECONF_MATE_VFS "/system/http_proxy"
#define ITEM_MATECONF_HTTP_PROXY_PORT "port"
#define ITEM_MATECONF_HTTP_PROXY_HOST "host"
#define KEY_MATECONF_HTTP_PROXY_PORT (PATH_MATECONF_MATE_VFS "/" ITEM_MATECONF_HTTP_PROXY_PORT)
#define KEY_MATECONF_HTTP_PROXY_HOST (PATH_MATECONF_MATE_VFS "/" ITEM_MATECONF_HTTP_PROXY_HOST)

#define ITEM_MATECONF_USE_HTTP_PROXY "use_http_proxy"
#define KEY_MATECONF_USE_HTTP_PROXY (PATH_MATECONF_MATE_VFS "/" ITEM_MATECONF_USE_HTTP_PROXY)

#define KEY_MATECONF_HTTP_AUTH_USER (PATH_MATECONF_MATE_VFS "/" "authentication_user")
#define KEY_MATECONF_HTTP_AUTH_PW (PATH_MATECONF_MATE_VFS "/" "authentication_password")
#define KEY_MATECONF_HTTP_USE_AUTH (PATH_MATECONF_MATE_VFS "/" "use_authentication")

#define KEY_MATECONF_HTTP_PROXY_IGNORE_HOSTS (PATH_MATECONF_MATE_VFS "/" "ignore_hosts")


/* Global variables used by the HTTP proxy config */
static MateConfClient *gl_client = NULL;
static GMutex *gl_mutex = NULL;	/* This mutex protects preference values
				 * and ensures serialization of authentication
				 * hook callbacks       
				 */
static gchar *gl_http_proxy = NULL;
static GSList *gl_ignore_hosts = NULL;	/* Elements are strings. */
static GSList *gl_ignore_addrs = NULL;	/* Elements are ProxyHostAddrs */

static gchar *proxy_username = NULL;
static gchar *proxy_password = NULL;

/* Store IP addresses that may represent network or host addresses and may be
 * IPv4 or IPv6. */
typedef enum {
    PROXY_IPv4 = 4,
    PROXY_IPv6 = 6
} ProxyAddrType;

typedef struct {
    ProxyAddrType type;
    struct in_addr addr;
    struct in_addr mask;
#ifdef ENABLE_IPV6
    struct in6_addr addr6;
    struct in6_addr mask6;
#endif
} ProxyHostAddr;

#ifdef ENABLE_IPV6
static void ipv6_network_addr(const struct in6_addr *addr,
			      const struct in6_addr *mask,
			      struct in6_addr *res);

/*Check whether the node is IPv6 enabled.*/
static gboolean have_ipv6(void)
{
    int s;

    s = socket(AF_INET6, SOCK_STREAM, 0);
    if (s != -1) {
	close(s);
	return TRUE;
    }

    return FALSE;
}
#	endif

/* FIXME: should be done using AC_REPLACE_FUNCS */
#ifndef HAVE_INET_PTON
static int inet_pton(int af, const char *hostname, void *pton)
{
    struct in_addr in;
    if (!inet_aton(hostname, &in))
	return 0;


    memcpy(pton, &in, sizeof(in));
    return 1;
}
#endif

#ifdef ENABLE_IPV6
static void
ipv6_network_addr(const struct in6_addr *addr, const struct in6_addr *mask,
		  struct in6_addr *res)
{
    gint i;

    for (i = 0; i < 16; ++i) {
	res->s6_addr[i] = addr->s6_addr[i] & mask->s6_addr[i];
    }
}
#endif

/**
 * host_port_from_string
 * splits a <host>:<port> formatted string into its separate components
 */
static gboolean
host_port_from_string(const char *http_proxy,
		      char **p_proxy_host, guint * p_proxy_port)
{
    char *port_part;

    port_part = strchr(http_proxy, ':');

    if (port_part && '\0' != ++port_part && p_proxy_port) {
	*p_proxy_port = (guint) strtoul(port_part, NULL, 10);
    } else if (p_proxy_port) {
	*p_proxy_port = DEFAULT_HTTP_PROXY_PORT;
    }

    if (p_proxy_host) {
	if (port_part != http_proxy) {
	    *p_proxy_host =
		g_strndup(http_proxy, port_part - http_proxy - 1);
	} else {
	    return FALSE;
	}
    }

    return TRUE;
}

static gboolean proxy_should_for_hostname(const char *hostname)
{
#ifdef ENABLE_IPV6
    struct in6_addr in6, net6;
#endif
    struct in_addr in;
    GSList *elt;
    ProxyHostAddr *addr;


    /* IPv4 address */
    if (inet_pton(AF_INET, hostname, &in) > 0) {
	for (elt = gl_ignore_addrs; elt; elt = g_slist_next(elt)) {
	    addr = (ProxyHostAddr *) (elt->data);
	    if (addr->type == PROXY_IPv4
		&& (in.s_addr & addr->mask.s_addr) == addr->addr.s_addr) {
		DEBUG_HTTP(("Host %s using direct connection.", hostname));
		return FALSE;
	    }
	}
    }
#ifdef ENABLE_IPV6
    else if (have_ipv6() && inet_pton(AF_INET6, hostname, &in6)) {
	for (elt = gl_ignore_addrs; elt; elt = g_slist_next(elt)) {
	    addr = (ProxyHostAddr *) (elt->data);
	    ipv6_network_addr(&in6, &addr->mask6, &net6);
	    if (addr->type == PROXY_IPv6
		&& IN6_ARE_ADDR_EQUAL(&net6, &addr->addr6)) {
		DEBUG_HTTP(("Host %s using direct connection.", hostname));
		return FALSE;
	    }
	    /* Handle IPv6-wrapped IPv4 addresses. */
	    else if (addr->type == PROXY_IPv4
		     && IN6_IS_ADDR_V4MAPPED(&net6)) {
		guint32 v4addr;

		v4addr = net6.s6_addr[12] << 24 | net6.
		    s6_addr[13] << 16 | net6.
		    s6_addr[14] << 8 | net6.s6_addr[15];
		if ((v4addr & addr->mask.s_addr) != addr->addr.s_addr) {
		    DEBUG_HTTP(("Host %s using direct connection.",
				hostname));
		    return FALSE;
		}
	    }
	}
    }
#endif
    /* All hostnames (foo.bar.com) -- independent of IPv4 or IPv6 */

    /* If there are IPv6 addresses in the ignore_hosts list but we do not
     * have IPv6 available at runtime, then those addresses will also fall
     * through to here (and harmlessly fail to match). */
    else {
	gchar *hn = g_ascii_strdown(hostname, -1);

	for (elt = gl_ignore_hosts; elt; elt = g_slist_next(elt)) {
	    if (*(gchar *) (elt->data) == '*') {
		if (g_str_has_suffix(hn, (gchar *) (elt->data) + 1)) {
		    DEBUG_HTTP(("Host %s using direct connection.", hn));
		    g_free(hn);
		    return FALSE;
		}
	    } else if (strcmp(hn, elt->data) == 0) {
		DEBUG_HTTP(("Host %s using direct connection.", hn));
		g_free(hn);
		return FALSE;
	    }
	}
	g_free(hn);
    }

    return TRUE;
}

/**
 * proxy_for_uri
 * Retrives an appropriate HTTP proxy for a given toplevel uri
 * Currently, only a single HTTP proxy is implemented (there's no way for
 * specifying non-proxy domain names's).  Returns FALSE if the connect should
 * take place directly
 */

gboolean proxy_for_uri (MateVFSToplevelURI *toplevel_uri, 
			HttpProxyInfo *proxy_info)
{				
    gboolean ret;

    ret = proxy_should_for_hostname (toplevel_uri->host_name);

    g_mutex_lock (gl_mutex);

    if (ret && gl_http_proxy != NULL) {
	ret = host_port_from_string(gl_http_proxy, &(proxy_info->host),
				    &(proxy_info->port));
	   
	if (ret) {
		proxy_info->username = proxy_username;
		proxy_info->password = proxy_password;
	}
	    
    } else {
	    
	ret = FALSE;
    }

    g_mutex_unlock(gl_mutex);

    return ret;
}


/*
 * Here's how the mateconf mate-vfs HTTP proxy variables
 * are intended to be used
 *
 * /system/http_proxy/use_http_proxy	
 * 	Type: boolean
 *	If set to TRUE, the client should use an HTTP proxy to connect to all
 *	servers (except those specified in the ignore_hosts key -- see below).
 *	The proxy is specified in other mateconf variables below.
 *
 * /system/http_proxy/host
 *	Type: string
 *	The hostname of the HTTP proxy this client should use.  If
 *	use-http-proxy is TRUE, this should be set.  If it is not set, the
 *	application should behave as if use-http-proxy is was set to FALSE.
 *
 * /system/http_proxy/port
 *	Type: int
 *	The port number on the HTTP proxy host that the client should connect to
 *	If use_http_proxy and host are set but this is not set, the application
 *	should use a default port value of 8080
 *
 * /system/http_proxy/authentication-user
 *	Type: string
 *	Username to pass to an authenticating HTTP proxy.
 *
 * /system/http_proxy/authentication_password
 *	Type: string
 *	Password to pass to an authenticating HTTP proxy.
 *  
 * /system/http_proxy/use-authentication
 *	Type: boolean
 * 	TRUE if the client should pass http-proxy-authorization-user and
 *	http-proxy-authorization-password an HTTP proxy
 *
 * /system/http_proxy/ignore_hosts
 * 	Type: list of strings
 * 	A list of hosts (hostnames, wildcard domains, IP addresses, and CIDR
 * 	network addresses) that should be accessed directly.
 */

static void parse_ignore_host(gpointer data, gpointer user_data)
{
    gchar *hostname, *input, *netmask;
    gboolean ip_addr = FALSE, has_error = FALSE;
    struct in_addr host;
#ifdef ENABLE_IPV6
    struct in6_addr host6, mask6;
    gint i;
#endif
    ProxyHostAddr *elt;

    input = (gchar *) data;
    elt = g_new0(ProxyHostAddr, 1);
    if ((netmask = strchr(input, '/')) != NULL) {
	hostname = g_strndup(input, netmask - input);
	++netmask;
    } else {
	hostname = g_ascii_strdown(input, -1);
    }
    if (inet_pton(AF_INET, hostname, &host) > 0) {
	ip_addr = TRUE;
	elt->type = PROXY_IPv4;
	elt->addr.s_addr = host.s_addr;
	if (netmask) {
	    gchar *endptr;
	    gint width = strtol(netmask, &endptr, 10);

	    if (*endptr != '\0' || width < 0 || width > 32) {
		has_error = TRUE;
	    }
	    elt->mask.s_addr = htonl(~0 << width);
	    elt->addr.s_addr &= elt->mask.s_addr;
	} else {
	    elt->mask.s_addr = 0xffffffff;
	}
    }
#ifdef ENABLE_IPV6
    else if (have_ipv6() && inet_pton(AF_INET6, hostname, &host6) > 0) {
	ip_addr = TRUE;
	elt->type = PROXY_IPv6;
	for (i = 0; i < 16; ++i) {
	    elt->addr6.s6_addr[i] = host6.s6_addr[i];
	}
	if (netmask) {
	    gchar *endptr;
	    gint width = strtol(netmask, &endptr, 10);

	    if (*endptr != '\0' || width < 0 || width > 128) {
		has_error = TRUE;
	    }
	    for (i = 0; i < 16; ++i) {
		elt->mask6.s6_addr[i] = 0;
	    }
	    for (i = 0; i < width / 8; i++) {
		elt->mask6.s6_addr[i] = 0xff;
	    }
	    elt->mask6.s6_addr[i] = (0xff << (8 - width % 8)) & 0xff;
	    ipv6_network_addr(&elt->addr6, &mask6, &elt->addr6);
	} else {
	    for (i = 0; i < 16; ++i) {
		elt->mask6.s6_addr[i] = 0xff;
	    }
	}
    }
#endif

    if (ip_addr) {
	if (!has_error) {
	    gchar *dst = g_new0(gchar, INET_ADDRSTRLEN);

	    gl_ignore_addrs = g_slist_append(gl_ignore_addrs, elt);
	    DEBUG_HTTP(("Host %s/%s does not go through proxy.",
			hostname, inet_ntop(AF_INET, &elt->mask,
					    dst, INET_ADDRSTRLEN)));
	    g_free(dst);
	}
	g_free(hostname);
    } else {
	/* It is a hostname. */
	gl_ignore_hosts = g_slist_append(gl_ignore_hosts, hostname);
	DEBUG_HTTP(("Host %s does not go through proxy.", hostname));
	g_free(elt);
    }
}

static void construct_gl_http_proxy(gboolean use_proxy)
{
    g_free(gl_http_proxy);
    gl_http_proxy = NULL;

    g_slist_foreach(gl_ignore_hosts, (GFunc) g_free, NULL);
    g_slist_free(gl_ignore_hosts);
    gl_ignore_hosts = NULL;
    g_slist_foreach(gl_ignore_addrs, (GFunc) g_free, NULL);
    g_slist_free(gl_ignore_addrs);
    gl_ignore_addrs = NULL;

    if (use_proxy) {
	char *proxy_host;
	int proxy_port;
	GSList *ignore;

	proxy_host =
	    mateconf_client_get_string(gl_client,
				    KEY_MATECONF_HTTP_PROXY_HOST, NULL);
	proxy_port =
	    mateconf_client_get_int(gl_client,
				 KEY_MATECONF_HTTP_PROXY_PORT, NULL);

	if (proxy_host && proxy_host[0] != '\0') {
	    if (0 != proxy_port && 0xffff >= (unsigned) proxy_port) {
		gl_http_proxy =
		    g_strdup_printf("%s:%u", proxy_host, (unsigned)
				    proxy_port);
	    } else {
		gl_http_proxy =
		    g_strdup_printf("%s:%u", proxy_host, (unsigned)
				    DEFAULT_HTTP_PROXY_PORT);
	    }
	    DEBUG_HTTP(("New HTTP proxy: '%s'", gl_http_proxy));
	} else {
	    DEBUG_HTTP(("HTTP proxy unset"));
	}

	g_free(proxy_host);
	proxy_host = NULL;

	ignore = mateconf_client_get_list(gl_client,
				       KEY_MATECONF_HTTP_PROXY_IGNORE_HOSTS,
				       MATECONF_VALUE_STRING, NULL);
	g_slist_foreach(ignore, (GFunc) parse_ignore_host, NULL);
	g_slist_foreach(ignore, (GFunc) g_free, NULL);
	g_slist_free(ignore);
	ignore = NULL;
    }
}

static void set_proxy_auth(gboolean use_proxy_auth)
{
    char *auth_user;
    char *auth_pw;

    auth_user =
	mateconf_client_get_string(gl_client, KEY_MATECONF_HTTP_AUTH_USER, NULL);
    auth_pw =
	mateconf_client_get_string(gl_client, KEY_MATECONF_HTTP_AUTH_PW, NULL);

    if (use_proxy_auth) {
	/* Christian Kellner: Here are the only changes I made to the proxy sub-
	 * system 
	 */
	proxy_username = (auth_user != NULL ? g_strdup(auth_user) : NULL);
	proxy_password = (auth_pw != NULL ? g_strdup(auth_pw) : NULL);
	DEBUG_HTTP(("New HTTP proxy auth user: '%s'", auth_user));
    } else {
	if (proxy_username != NULL)
	    g_free(proxy_username);

	if (proxy_password != NULL)
	    g_free(proxy_password);

	proxy_username = proxy_password = NULL;

	DEBUG_HTTP(("HTTP proxy auth unset"));
    }

    g_free(auth_user);
    g_free(auth_pw);
}

/**
 * sig_mateconf_value_changed 
 * GGconf notify function for when HTTP proxy MateConf key has changed.
 */
static void
notify_mateconf_value_changed(MateConfClient * client,
			   guint cnxn_id, MateConfEntry * entry,
			   gpointer data)
{
    const char *key;

    key = mateconf_entry_get_key(entry);

    if (strcmp(key, KEY_MATECONF_USE_HTTP_PROXY) == 0
	|| strcmp(key, KEY_MATECONF_HTTP_PROXY_IGNORE_HOSTS) == 0
	|| strcmp(key, KEY_MATECONF_HTTP_PROXY_HOST) == 0
	|| strcmp(key, KEY_MATECONF_HTTP_PROXY_PORT) == 0) {
	gboolean use_proxy_value;

	g_mutex_lock(gl_mutex);

	/* Check and see if we are using the proxy */
	use_proxy_value =
	    mateconf_client_get_bool(gl_client,
				  KEY_MATECONF_USE_HTTP_PROXY, NULL);
	construct_gl_http_proxy(use_proxy_value);

	g_mutex_unlock(gl_mutex);
    } else if (strcmp(key, KEY_MATECONF_HTTP_AUTH_USER) == 0
	       || strcmp(key, KEY_MATECONF_HTTP_AUTH_PW) == 0
	       || strcmp(key, KEY_MATECONF_HTTP_USE_AUTH) == 0) {
	gboolean use_proxy_auth;

	g_mutex_lock(gl_mutex);

	use_proxy_auth =
	    mateconf_client_get_bool(gl_client,
				  KEY_MATECONF_HTTP_USE_AUTH, NULL);
	set_proxy_auth(use_proxy_auth);

	g_mutex_unlock(gl_mutex);
    }
}

void proxy_init(void)
{
    GError *mateconf_error = NULL;
    gboolean use_proxy;
    gboolean use_proxy_auth;

    gl_client = mateconf_client_get_default();
    gl_mutex = g_mutex_new();

    mateconf_client_add_dir(gl_client, PATH_MATECONF_MATE_VFS,
			 MATECONF_CLIENT_PRELOAD_ONELEVEL, &mateconf_error);
    if (mateconf_error) {
	DEBUG_HTTP(("MateConf error during client_add_dir '%s'",
		    mateconf_error->message));
	g_error_free(mateconf_error);
	mateconf_error = NULL;
    }

    mateconf_client_notify_add(gl_client, PATH_MATECONF_MATE_VFS,
			    notify_mateconf_value_changed, NULL, NULL,
			    &mateconf_error);
    if (mateconf_error) {
	DEBUG_HTTP(("MateConf error during notify_error '%s'",
		    mateconf_error->message));
	g_error_free(mateconf_error);
	mateconf_error = NULL;
    }

    /* Load the http proxy setting */
    use_proxy =
	mateconf_client_get_bool(gl_client, KEY_MATECONF_USE_HTTP_PROXY,
			      &mateconf_error);

    if (mateconf_error != NULL) {
	DEBUG_HTTP(("MateConf error during client_get_bool '%s'",
		    mateconf_error->message));
	g_error_free(mateconf_error);
	mateconf_error = NULL;
    } else {
	construct_gl_http_proxy(use_proxy);
    }

    use_proxy_auth =
	mateconf_client_get_bool(gl_client, KEY_MATECONF_HTTP_USE_AUTH,
			      &mateconf_error);

    if (mateconf_error != NULL) {
	DEBUG_HTTP(("MateConf error during client_get_bool '%s'",
		    mateconf_error->message));
	g_error_free(mateconf_error);
	mateconf_error = NULL;
    } else {
	set_proxy_auth(use_proxy_auth);
    }
}


void proxy_shutdown(void)
{
    g_object_unref(G_OBJECT(gl_client));
}

/* End of the proxy stuff */
/* ************************************************************************** */
