#include <config.h>

#include <gmodule.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <libmatevfs/mate-vfs-module-callback.h>
#include <libmatevfs/mate-vfs-standard-callbacks.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static gboolean authentication_callback_called = FALSE;

/* For this test case to function, these two URI's should
 * require a username/password (set in authentication_username, authentication_password below)
 * and AUTHENTICATION_URI_CHILD should be a child of AUTHENTICATION_URI
 */
#define AUTHENTICATION_URI_CHILD "http://localhost/~mikef/protected/index.html"
#define AUTHENTICATION_URI "http://localhost/~mikef/protected/"

static const char *authentication_username = "foo";
static const char *authentication_password = "foo";

static void /* MateVFSModuleCallback */
authentication_callback (gconstpointer in, size_t in_size, gpointer out, size_t out_size, gpointer user_data)
{
 	MateVFSModuleCallbackAuthenticationIn *in_real;
 	MateVFSModuleCallbackAuthenticationOut *out_real;

 	/* printf ("in authentication_callback\n"); */
	
 	g_return_if_fail (sizeof (MateVFSModuleCallbackAuthenticationIn) == in_size
 		&& sizeof (MateVFSModuleCallbackAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

 	in_real = (MateVFSModuleCallbackAuthenticationIn *)in;
 	out_real = (MateVFSModuleCallbackAuthenticationOut *)out;

	/* printf ("in uri: %s realm: %s\n", in_real->uri, in_real->realm); */
	
 	out_real->username = g_strdup (authentication_username);
 	out_real->password = g_strdup (authentication_password);

 	authentication_callback_called = TRUE;
}

static gboolean destroy_notify_occurred = FALSE;

static void /*GDestroyNotify*/
destroy_notify (gpointer user_data)
{
	destroy_notify_occurred = TRUE;
}

static volatile gboolean open_callback_occurred = FALSE;

static MateVFSResult open_callback_result_expected = MATE_VFS_OK;

static void /* MateVFSAsyncOpenCallback */
open_callback (MateVFSAsyncHandle *handle,
	       MateVFSResult result,
	       gpointer callback_data)
{
	g_assert (result == open_callback_result_expected);

	open_callback_occurred = TRUE;
}

static volatile gboolean close_callback_occurred = FALSE;

static void /* MateVFSAsyncOpenCallback */
close_callback (MateVFSAsyncHandle *handle,
		MateVFSResult result,
		gpointer callback_data)
{
	close_callback_occurred = TRUE;
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
	g_log_set_handler
		(domain, 
		 (GLogLevelFlags) (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING),
		 stop_after_log, NULL);
}

static void (*flush_credentials_func)(void);

int
main (int argc, char **argv)
{
        MateVFSHandle *handle;
        MateVFSResult result;
	MateVFSAsyncHandle *async_handle;
	char *module_path;
	char *authentication_uri, *authentication_uri_child;
	GModule *module;
	guint i;

	make_asserts_break ("GLib");
	make_asserts_break ("MateVFS");

	if (argc == 2) {
		authentication_uri = argv[1];
		authentication_uri_child = g_strdup_printf("%s/./", authentication_uri);
	} else if (argc == 3) {
		authentication_uri = argv[1];
		authentication_uri_child = argv[2];
	} else {
		authentication_uri = AUTHENTICATION_URI;
		authentication_uri_child = AUTHENTICATION_URI_CHILD;
	}

	mate_vfs_init ();

	/* Load http module so we can snag the test hook */
	module_path = g_module_build_path (MODULES_PATH, "http");
	module = g_module_open (module_path, G_MODULE_BIND_LAZY);
	g_free (module_path);
	module_path = NULL;

	if (module == NULL) {
		fprintf (stderr, "Couldn't load http module \n");
		exit (-1);
	}

	g_module_symbol (module, "http_authentication_test_flush_credentials", (gpointer *) &flush_credentials_func);

	if (flush_credentials_func == NULL) {
		fprintf (stderr, "Couldn't find http_authentication_test_flush_credentials\n");
		exit (-1);
	}

	/* Test 1: Attempt to access a URI requiring authentication w/o a callback registered */

	result = mate_vfs_open (&handle, authentication_uri, MATE_VFS_OPEN_READ);
	g_assert (result == MATE_VFS_ERROR_ACCESS_DENIED);
	handle = NULL;

	/* Test 2: Attempt an async open that requires http authentication */

	mate_vfs_module_callback_set_default (MATE_VFS_MODULE_CALLBACK_AUTHENTICATION,
					       authentication_callback, 
					       NULL,
					       NULL);

	authentication_callback_called = FALSE;
	
	open_callback_occurred = FALSE;
	open_callback_result_expected = MATE_VFS_OK;

	mate_vfs_async_open (
		&async_handle, 
		authentication_uri,
		MATE_VFS_OPEN_READ,
		0,
		open_callback,
		NULL);

	while (!open_callback_occurred) {
		g_main_context_iteration (NULL, TRUE);
	}

	close_callback_occurred = FALSE;
	mate_vfs_async_close (async_handle, close_callback, NULL);

	while (!close_callback_occurred) {
		g_main_context_iteration (NULL, TRUE);
	}

	g_assert (authentication_callback_called);

	/* Test 3: Attempt a sync call to the same location;
	 * credentials should be stored so the authentication_callback function
	 * should not be called
	 */
	
	authentication_callback_called = FALSE;
	result = mate_vfs_open (&handle, authentication_uri, MATE_VFS_OPEN_READ);
	g_assert (result == MATE_VFS_OK);
	mate_vfs_close (handle);
	handle = NULL;
	/* The credentials should be in the cache, so we shouldn't have been called */
	g_assert (authentication_callback_called == FALSE);

	/* Test 4: Attempt a sync call to something deeper in the namespace.
	 * which should work without a callback too
	 */

	authentication_callback_called = FALSE;
	result = mate_vfs_open (&handle, authentication_uri_child, MATE_VFS_OPEN_READ);
	g_assert (result == MATE_VFS_OK);
	mate_vfs_close (handle);
	handle = NULL;
	/* The credentials should be in the cache, so we shouldn't have been called */
	g_assert (authentication_callback_called == FALSE);

	/* Test 5: clear the credential store and try again in reverse order */

	flush_credentials_func();

	authentication_callback_called = FALSE;
	result = mate_vfs_open (&handle, authentication_uri_child, MATE_VFS_OPEN_READ);
	g_assert (result == MATE_VFS_OK);
	mate_vfs_close (handle);
	handle = NULL;
	g_assert (authentication_callback_called == TRUE);

	/* Test 6: Try something higher in the namespace, which should
	 * cause the callback to happen again
	 */

	authentication_callback_called = FALSE;
	result = mate_vfs_open (&handle, authentication_uri, MATE_VFS_OPEN_READ);
	g_assert (result == MATE_VFS_OK);
	mate_vfs_close (handle);
	handle = NULL;
	g_assert (authentication_callback_called == TRUE);

	/* Test 7: Try same URL as in test 4, make sure callback doesn't get called */

	authentication_callback_called = FALSE;
	result = mate_vfs_open (&handle, authentication_uri_child, MATE_VFS_OPEN_READ);
	g_assert (result == MATE_VFS_OK);
	mate_vfs_close (handle);
	handle = NULL;
	g_assert (authentication_callback_called == FALSE);

	/* Test 8: clear the credential store ensure that passing a username as NULL
	 * cancels the operation, resulting in a ACCESS_DENIED error */

	flush_credentials_func();

	authentication_username = NULL;

	authentication_callback_called = FALSE;
	result = mate_vfs_open (&handle, authentication_uri_child, MATE_VFS_OPEN_READ);
	g_assert (result == MATE_VFS_ERROR_ACCESS_DENIED);
	handle = NULL;
	g_assert (authentication_callback_called == TRUE);


	/* Test 9: exercise the "destroy notify" functionality */
	/* Note that job doesn't end until a "close" is called, so the inherited
	 * callback isn't released until then
	 */

	flush_credentials_func();
	authentication_username = "foo";

	mate_vfs_module_callback_push (MATE_VFS_MODULE_CALLBACK_AUTHENTICATION,
					authentication_callback, 
					NULL,
					destroy_notify);

	authentication_callback_called = FALSE;
	
	open_callback_occurred = FALSE;
	open_callback_result_expected = MATE_VFS_OK;

	destroy_notify_occurred = FALSE;

	mate_vfs_async_open (
		&async_handle, 
		authentication_uri,
		MATE_VFS_OPEN_READ,
		0,
		open_callback,
		NULL);

	mate_vfs_module_callback_pop (MATE_VFS_MODULE_CALLBACK_AUTHENTICATION);

	g_assert (!destroy_notify_occurred);

	while (!open_callback_occurred) {
		g_main_context_iteration (NULL, TRUE);
	}

	close_callback_occurred = FALSE;
	mate_vfs_async_close (async_handle, close_callback, NULL);

	while (!close_callback_occurred) {
		g_main_context_iteration (NULL, TRUE);
	}

	for (i = 0 ; i<100 ; i++) {
		g_main_context_iteration (NULL, FALSE);
		g_usleep (10);
	}

	g_assert (authentication_callback_called);
	g_assert (destroy_notify_occurred);

	mate_vfs_shutdown ();

	return 0;
}
