#include <config.h>
/*
#include "mate-authentication-manager.h"
#include "mate-authentication-manager-private.h"
*/

#include <libmatevfs/mate-vfs-module-callback.h>
#include <libmatevfs/mate-vfs-standard-callbacks.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <unistd.h>


#if 0
#define DEBUG_MSG(x) printf x
#else 
#define DEBUG_MSG(x)
#endif

#define _(x) x

#define FGETS_NO_NEWLINE(buffer,size,fd) \
	fgets (buffer, size, fd); \
	if (strlen (buffer) > 0 && \
	    buffer[strlen (buffer) - 1] == '\n') \
		buffer[strlen (buffer) - 1] = '\0';

static char *
ask_for_password (void)
{
#ifdef HAVE_TERMIOS_H
	char buffer[BUFSIZ];
	int old_flags;
	struct termios term_attr; 
	char *ret;

	ret = NULL;

	if (tcgetattr(STDIN_FILENO, &term_attr) == 0) {
		old_flags = term_attr.c_lflag; 
		term_attr.c_lflag &= ~ECHO;
		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attr) == 0) {
			FGETS_NO_NEWLINE (buffer, BUFSIZ, stdin);
			ret = g_strdup (buffer);
			printf ("\n");
		} else
			fprintf (stderr, "tcsetattr() failed, skipping password.\n");


		term_attr.c_lflag = old_flags;
		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attr) != 0)
			fprintf (stderr, "tcsetattr() failed.\n"); 
	}
	else
		fprintf (stderr, "tcgetattr() failed, skipping password.\n");

	return ret;
#else
	char buffer[BUFSIZ];
	char *ret;

	FGETS_NO_NEWLINE (buffer, BUFSIZ, stdin);
	ret = g_strdup (buffer);

	return ret;
#endif
}

static int
ask_question (char *prim_msg, char *sec_msg, char **choices) 
{
	int i, ans;
	char buffer[BUFSIZ];

	if (prim_msg && sec_msg)
		printf ("%s\n%s", prim_msg, sec_msg);
	else if (prim_msg || sec_msg);
		printf ("%s", prim_msg != NULL ? prim_msg : sec_msg);

	do {
		printf ("\n");
		for (i = 0; choices[i] != NULL; i++) {
			printf (" %d\t%s\n", (i + 1), choices[i]);
		}

		FGETS_NO_NEWLINE (buffer, BUFSIZ, stdin);
		ans = (int) strtol (buffer, NULL, 10);
	} while (ans < 1 || ans > i);

	return ans;
}

static void
do_auth (gboolean is_proxy_authentication,
	 const MateVFSModuleCallbackAuthenticationIn *in_args,
	 MateVFSModuleCallbackAuthenticationOut *out_args)
{
	char buffer[BUFSIZ];

	printf (is_proxy_authentication
			? _("Your HTTP Proxy requires you to log in.\n")
			: _("You must log in to access \"%s\".\n%s"), 
		in_args->uri, 
		in_args->auth_type == AuthTypeBasic
			? _("Your password will be transmitted unencrypted.") 
			: _("Your password will be transmitted encrypted."));

	printf ("Username: ");
	FGETS_NO_NEWLINE (buffer, BUFSIZ, stdin);
	out_args->username = g_strdup (buffer);

	printf ("Password: ");
	out_args->password = ask_for_password ();
}

static void /* MateVFSModuleCallback */
vfs_authentication_callback (gconstpointer in, size_t in_size, 
			     gpointer out, size_t out_size, 
			     gpointer user_data)
{
	gboolean is_proxy_authentication;

	is_proxy_authentication = (user_data == GINT_TO_POINTER (1));
	do_auth (is_proxy_authentication, in, out);
}

static void
do_full_auth (const MateVFSModuleCallbackFullAuthenticationIn *in_args,
	      MateVFSModuleCallbackFullAuthenticationOut *out_args)
{
	char buffer[BUFSIZ];
	char *message;
	GString *name;

	out_args->abort_auth = FALSE;

	name = g_string_new (NULL);
	if (in_args->server != NULL) {
		/* remote */
		if (in_args->username != NULL) {
			g_string_append_printf (name, "%s@", in_args->username);
		}
		g_string_append (name, in_args->server);
		if (in_args->port != 0) {
			g_string_append_printf (name, ":%d", in_args->port);
		}
		if (in_args->object != NULL) {
			g_string_append_printf (name, "/%s", in_args->object);
		}
	} else {
		/* local */
		if (in_args->object != NULL) {
			g_string_append (name, in_args->object);
		}
	}

	if (in_args->domain != NULL) {
		message = g_strdup_printf (_("You must log in to access %s domain %s\n"), name->str, in_args->domain);
	} else {
		message = g_strdup_printf (_("You must log in to access %s\n"), name->str);
	}

	g_string_free (name, TRUE);

	printf ("%s", message);
	g_free (message);

	if (in_args->flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_ANON_SUPPORTED) {
		char *answers[] =  { "Yes", "No", NULL };
		if (ask_question ("Login anonymously?", NULL, answers) == 1)
			out_args->out_flags |= MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_OUT_ANON_SELECTED;
	}

	if ((out_args->out_flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_OUT_ANON_SELECTED) == 0) {
		if (in_args->flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME) {
			if (in_args->default_user != NULL) {
				printf ("Username (default \"%s\"): ", in_args->default_user);
			} else {
				printf ("Username: ");
			}

			FGETS_NO_NEWLINE (buffer, BUFSIZ, stdin);
			out_args->username = g_strdup (buffer);
		}

		if (in_args->flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN) {
			if (in_args->default_domain != NULL) {
				printf ("Domain (default \"%s\"): ", in_args->default_domain);
			} else {
				printf ("Domain: ");
			}

			FGETS_NO_NEWLINE (buffer, BUFSIZ, stdin);
			out_args->domain = g_strdup (buffer);
		}

		if (in_args->flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD) {
			printf ("Password: ");
			out_args->password = ask_for_password ();
		}
	}

	if ((out_args->username == NULL ||
	     strlen (out_args->username) == 0)
	    && in_args->default_user != NULL) {
		g_free (out_args->username);
		out_args->username = g_strdup (in_args->default_user);
	}

	if ((out_args->domain == NULL ||
	     strlen (out_args->domain) == 0)
	    && in_args->default_domain != NULL) {
		g_free (out_args->domain);
		out_args->domain = g_strdup (in_args->default_domain);
	}

	/* TODO support saving password? */
	out_args->save_password = FALSE;
	out_args->keyring = NULL;
}

static void /* MateVFSModuleCallback */
vfs_full_authentication_callback (gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data)
{
	do_full_auth (in, out);
}

static void /* MateVFSModuleCallback */
vfs_question_callback (gconstpointer in, size_t in_size, 
		       gpointer out, size_t out_size, 
		       gpointer user_data)
{
	MateVFSModuleCallbackQuestionIn *in_real;
	MateVFSModuleCallbackQuestionOut *out_real;

	in_real = (MateVFSModuleCallbackQuestionIn *)in;
	out_real = (MateVFSModuleCallbackQuestionOut *)out;

	out_real->answer = ask_question (in_real->primary_message,
					 in_real->secondary_message,
					 in_real->choices);
}

static void
command_line_authentication_init (void)
{
	if (isatty (STDIN_FILENO) && isatty (STDOUT_FILENO)) {
		mate_vfs_module_callback_set_default (MATE_VFS_MODULE_CALLBACK_AUTHENTICATION,
						       vfs_authentication_callback,
						       GINT_TO_POINTER (0),
						       NULL);
		mate_vfs_module_callback_set_default (MATE_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION,
						       vfs_authentication_callback,
						       GINT_TO_POINTER (1),
						       NULL);
		mate_vfs_module_callback_set_default (MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
						       vfs_full_authentication_callback,
						       GINT_TO_POINTER (0),
						       NULL);
		mate_vfs_module_callback_set_default (MATE_VFS_MODULE_CALLBACK_QUESTION,
						       vfs_question_callback,
						       GINT_TO_POINTER (0),
						       NULL);
	}
}
