/* mate-about-me.c
 * Copyright (C) 2002 Diego Gonzalez
 * Copyright (C) 2006 Johannes H. Jensen
 *
 * Written by: Diego Gonzalez <diego@pemas.net>
 * Modified by: Johannes H. Jensen <joh@deworks.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Parts of this code come from Mate-System-Tools.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* Are all of these needed? */
#include <gdk/gdkkeysyms.h>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#if __sun
#include <sys/types.h>
#include <signal.h>
#endif

#include "capplet-util.h"
#include "eel-alert-dialog.h"

/* Passwd states */
typedef enum {
	PASSWD_STATE_NONE,		/* Passwd is not asking for anything */
	PASSWD_STATE_AUTH,		/* Passwd is asking for our current password */
	PASSWD_STATE_NEW,		/* Passwd is asking for our new password */
	PASSWD_STATE_RETYPE,	/* Passwd is asking for our retyped new password */
	PASSWD_STATE_ERR		/* Passwd reported an error but has not yet exited */
} PasswdState;

typedef struct {
	GtkBuilder  *ui;

	/* Commonly used widgets */
	GtkEntry *current_password;
	GtkEntry *new_password;
	GtkEntry *retyped_password;
	GtkImage *dialog_image;
	GtkLabel *status_label;

	/* Whether we have authenticated */
	gboolean authenticated;

	/* Communication with the passwd program */
	GPid backend_pid;

	GIOChannel *backend_stdin;
	GIOChannel *backend_stdout;

	GQueue *backend_stdin_queue;		/* Write queue to backend_stdin */

	/* GMainLoop IDs */
	guint backend_child_watch_id;		/* g_child_watch_add (PID) */
	guint backend_stdout_watch_id;		/* g_io_add_watch (stdout) */

	/* State of the passwd program */
	PasswdState backend_state;

} PasswordDialog;

/* Buffer size for backend output */
#define BUFSIZE 64

/*
 * Error handling {{
 */
#define PASSDLG_ERROR (mate_about_me_password_error_quark())

GQuark mate_about_me_password_error_quark(void)
{
	static GQuark q = 0;

	if (q == 0) {
		q = g_quark_from_static_string("mate_about_me_password_error");
	}

	return q;
}

/* error codes */
enum {
	PASSDLG_ERROR_NONE,
	PASSDLG_ERROR_NEW_PASSWORD_EMPTY,
	PASSDLG_ERROR_RETYPED_PASSWORD_EMPTY,
	PASSDLG_ERROR_PASSWORDS_NOT_EQUAL,
	PASSDLG_ERROR_BACKEND,	/* Backend error */
	PASSDLG_ERROR_USER,		/* Generic user error */
	PASSDLG_ERROR_FAILED	/* Fatal failure, error->message should explain */
};

/*
 * }} Error handling
 */

/*
 * Prototypes {{
 */
static void
stop_passwd (PasswordDialog *pdialog);

static void
free_passwd_resources (PasswordDialog *pdialog);

static gboolean
io_watch_stdout (GIOChannel *source, GIOCondition condition, PasswordDialog *pdialog);

static void
passdlg_set_auth_state (PasswordDialog *pdialog, gboolean state);

static void
passdlg_set_status (PasswordDialog *pdialog, gchar *msg);

static void
passdlg_set_busy (PasswordDialog *pdialog, gboolean busy);

static void
passdlg_clear (PasswordDialog *pdialog);

static guint
passdlg_refresh_password_state (PasswordDialog *pdialog);

/*
 * }} Prototypes
 */

/*
 * Spawning and closing of backend {{
 */

/* Child watcher */
static void
child_watch_cb (GPid pid, gint status, PasswordDialog *pdialog)
{
	if (WIFEXITED (status)) {
		if (WEXITSTATUS (status) >= 255) {
			g_warning (_("Child exited unexpectedly"));
		}
	}

	free_passwd_resources (pdialog);
}

/* Spawn passwd backend
 * Returns: TRUE on success, FALSE otherwise and sets error appropriately */
static gboolean
spawn_passwd (PasswordDialog *pdialog, GError **error)
{
	gchar	*argv[2];
	gchar	*envp[1];
	gint	my_stdin, my_stdout, my_stderr;

	argv[0] = "/usr/bin/passwd";	/* Is it safe to rely on a hard-coded path? */
	argv[1] = NULL;

	envp[0] = NULL;					/* If we pass an empty array as the environment,
									 * will the childs environment be empty, and the
									 * locales set to the C default? From the manual:
									 * "If envp is NULL, the child inherits its
									 * parent'senvironment."
									 * If I'm wrong here, we somehow have to set
									 * the locales here.
									 */

	if (!g_spawn_async_with_pipes (NULL,						/* Working directory */
								   argv,						/* Argument vector */
								   envp,						/* Environment */
								   G_SPAWN_DO_NOT_REAP_CHILD,	/* Flags */
								   NULL,						/* Child setup */
								   NULL,						/* Data to child setup */
								   &pdialog->backend_pid,		/* PID */
								   &my_stdin,						/* Stdin */
								   &my_stdout,						/* Stdout */
								   &my_stderr,						/* Stderr */
								   error)) {					/* GError */

		/* An error occured */
		free_passwd_resources (pdialog);

		return FALSE;
	}

	/* 2>&1 */
	if (dup2 (my_stderr, my_stdout) == -1) {
		/* Failed! */
		g_set_error (error,
					 PASSDLG_ERROR,
					 PASSDLG_ERROR_BACKEND,
					 strerror (errno));

		/* Clean up */
		stop_passwd (pdialog);

		return FALSE;
	}

	/* Open IO Channels */
	pdialog->backend_stdin = g_io_channel_unix_new (my_stdin);
	pdialog->backend_stdout = g_io_channel_unix_new (my_stdout);

	/* Set raw encoding */
	/* Set nonblocking mode */
	if (g_io_channel_set_encoding (pdialog->backend_stdin, NULL, error) != G_IO_STATUS_NORMAL ||
		g_io_channel_set_encoding (pdialog->backend_stdout, NULL, error) != G_IO_STATUS_NORMAL ||
		g_io_channel_set_flags (pdialog->backend_stdin, G_IO_FLAG_NONBLOCK, error) != G_IO_STATUS_NORMAL ||
		g_io_channel_set_flags (pdialog->backend_stdout, G_IO_FLAG_NONBLOCK, error) != G_IO_STATUS_NORMAL ) {

		/* Clean up */
		stop_passwd (pdialog);
		return FALSE;
	}

	/* Turn off buffering */
	g_io_channel_set_buffered (pdialog->backend_stdin, FALSE);
	g_io_channel_set_buffered (pdialog->backend_stdout, FALSE);

	/* Add IO Channel watcher */
	pdialog->backend_stdout_watch_id = g_io_add_watch (pdialog->backend_stdout,
													   G_IO_IN | G_IO_PRI,
													   (GIOFunc) io_watch_stdout, pdialog);

	/* Add child watcher */
	pdialog->backend_child_watch_id = g_child_watch_add (pdialog->backend_pid, (GChildWatchFunc) child_watch_cb, pdialog);

	/* Success! */

	return TRUE;
}

/* Stop passwd backend */
static void
stop_passwd (PasswordDialog *pdialog)
{
	/* This is the standard way of returning from the dialog with passwd.
	 * If we return this way we can safely kill passwd as it has completed
	 * its task.
	 */

	if (pdialog->backend_pid != -1) {
		kill (pdialog->backend_pid, 9);
	}

	/* We must run free_passwd_resources here and not let our child
	 * watcher do it, since it will access invalid memory after the
	 * dialog has been closed and cleaned up.
	 *
	 * If we had more than a single thread we'd need to remove
	 * the child watch before trying to kill the child.
	 */
	free_passwd_resources (pdialog);
}

/* Clean up passwd resources */
static void
free_passwd_resources (PasswordDialog *pdialog)
{
	GError	*error = NULL;

	/* Remove the child watcher */
	if (pdialog->backend_child_watch_id != 0) {

		g_source_remove (pdialog->backend_child_watch_id);

		pdialog->backend_child_watch_id = 0;
	}


	/* Close IO channels (internal file descriptors are automatically closed) */
	if (pdialog->backend_stdin != NULL) {

		if (g_io_channel_shutdown (pdialog->backend_stdin, TRUE, &error) != G_IO_STATUS_NORMAL) {
			g_warning (_("Could not shutdown backend_stdin IO channel: %s"), error->message);
			g_error_free (error);
			error = NULL;
		}

		g_io_channel_unref (pdialog->backend_stdin);

		pdialog->backend_stdin = NULL;
	}

	if (pdialog->backend_stdout != NULL) {

		if (g_io_channel_shutdown (pdialog->backend_stdout, TRUE, &error) != G_IO_STATUS_NORMAL) {
			g_warning (_("Could not shutdown backend_stdout IO channel: %s"), error->message);
			g_error_free (error);
			error = NULL;
		}

		g_io_channel_unref (pdialog->backend_stdout);

		pdialog->backend_stdout = NULL;
	}

	/* Remove IO watcher */
	if (pdialog->backend_stdout_watch_id != 0) {

		g_source_remove (pdialog->backend_stdout_watch_id);

		pdialog->backend_stdout_watch_id = 0;
	}

	/* Close PID */
	if (pdialog->backend_pid != -1) {

		g_spawn_close_pid (pdialog->backend_pid);

		pdialog->backend_pid = -1;
	}

	/* Clear backend state */
	pdialog->backend_state = PASSWD_STATE_NONE;
}

/*
 * }} Spawning and closing of backend
 */

/*
 * Backend communication code {{
 */

/* Write the first element of queue through channel */
static void
io_queue_pop (GQueue *queue, GIOChannel *channel)
{
	gchar	*buf;
	gsize	bytes_written;
	GError	*error = NULL;

	buf = g_queue_pop_head (queue);

	if (buf != NULL) {

		if (g_io_channel_write_chars (channel, buf, -1, &bytes_written, &error) != G_IO_STATUS_NORMAL) {
			g_warning ("Could not write queue element \"%s\" to channel: %s", buf, error->message);
			g_error_free (error);
		}

		g_free (buf);
	}
}

/* Goes through the argument list, checking if one of them occurs in str
 * Returns: TRUE as soon as an element is found to match, FALSE otherwise */
static gboolean
is_string_complete (gchar *str, ...)
{
	va_list	ap;
	gchar	*arg;

	if (strlen (str) == 0) {
		return FALSE;
	}

	va_start (ap, str);

	while ((arg = va_arg (ap, char *)) != NULL) {
		if (g_strrstr (str, arg) != NULL) {
			va_end (ap);
			return TRUE;
		}
	}

	va_end (ap);

	return FALSE;
}

/* Authentication attempt succeeded. Update the GUI accordingly. */
static void
authenticated_user (PasswordDialog *pdialog)
{
	pdialog->backend_state = PASSWD_STATE_NEW;

	if (pdialog->authenticated) {
		/* This is a re-authentication
		 * It succeeded, so pop our new password from the queue */
		io_queue_pop (pdialog->backend_stdin_queue, pdialog->backend_stdin);
	}

	/* Update UI state */
	passdlg_set_auth_state (pdialog, TRUE);
	passdlg_set_status (pdialog, _("Authenticated!"));

	/* Check to see if the passwords are valid
	 * (They might be non-empty if the user had to re-authenticate,
	 *  and thus we need to enable the change-password-button) */
	passdlg_refresh_password_state (pdialog);
}

/*
 * IO watcher for stdout, called whenever there is data to read from the backend.
 * This is where most of the actual IO handling happens.
 */
static gboolean
io_watch_stdout (GIOChannel *source, GIOCondition condition, PasswordDialog *pdialog)
{
	static GString *str = NULL;	/* Persistent buffer */

	gchar		buf[BUFSIZE];		/* Temporary buffer */
	gsize		bytes_read;
	GError		*error = NULL;

	gchar		*msg = NULL;		/* Status error message */
	GtkBuilder	*dialog;

	gboolean	reinit = FALSE;

	/* Initialize buffer */
	if (str == NULL) {
		str = g_string_new ("");
	}

	dialog = pdialog->ui;

	if (g_io_channel_read_chars (source, buf, BUFSIZE, &bytes_read, &error) != G_IO_STATUS_NORMAL) {
		g_warning ("IO Channel read error: %s", error->message);
		g_error_free (error);

		return TRUE;
	}

	str = g_string_append_len (str, buf, bytes_read);

	/* In which state is the backend? */
	switch (pdialog->backend_state) {
		case PASSWD_STATE_AUTH:
			/* Passwd is asking for our current password */

			if (is_string_complete (str->str, "assword: ", "failure", "wrong", "error", NULL)) {
				/* Which response did we get? */
				passdlg_set_busy (pdialog, FALSE);

				if (g_strrstr (str->str, "assword: ") != NULL) {
					/* Authentication successful */

					authenticated_user (pdialog);

				} else {
					/* Authentication failed */

					if (pdialog->authenticated) {
						/* This is a re-auth, and it failed.
						 * The password must have been changed in the meantime!
						 * Ask the user to re-authenticate
						 */

						/* Update status message and auth state */
						passdlg_set_status (pdialog, _("Your password has been changed since you initially authenticated! Please re-authenticate."));
					} else {
						passdlg_set_status (pdialog, _("That password was incorrect."));
					}

					/* Focus current password */
					gtk_widget_grab_focus (GTK_WIDGET (pdialog->current_password));
				}

				reinit = TRUE;
			}
			break;
		case PASSWD_STATE_NEW:
			/* Passwd is asking for our new password */

			if (is_string_complete (str->str, "assword: ", NULL)) {
				/* Advance to next state */
				pdialog->backend_state = PASSWD_STATE_RETYPE;

				/* Pop retyped password from queue and into IO channel */
				io_queue_pop (pdialog->backend_stdin_queue, pdialog->backend_stdin);

				reinit = TRUE;
			}
			break;
		case PASSWD_STATE_RETYPE:
			/* Passwd is asking for our retyped new password */

			if (is_string_complete (str->str, "successfully",
							  "short",
							  "longer",
							  "palindrome",
							  "dictionary",
							  "simpl", /* catches both simple and simplistic */
							  "similar",
							  "different",
							  "case",
							  "wrapped",
							  "recovered",
							  "recent"
							  "unchanged",
							  "match",
							  "1 numeric or special",
							  "failure",
							  NULL)) {

				/* What response did we get? */
				passdlg_set_busy (pdialog, FALSE);

				if (g_strrstr (str->str, "successfully") != NULL) {
					/* Hooray! */

					passdlg_clear (pdialog);
					passdlg_set_status (pdialog, _("Your password has been changed."));
				} else {
					/* Ohnoes! */

					/* Focus new password */
					gtk_widget_grab_focus (GTK_WIDGET (pdialog->new_password));

					if (g_strrstr (str->str, "recovered") != NULL) {
						/* What does this indicate?
						 * "Authentication information cannot be recovered?" from libpam? */
						msg = g_strdup_printf (_("System error: %s."), str->str);
					} else if (g_strrstr (str->str, "short") != NULL ||
						   g_strrstr (str->str, "longer") != NULL) {
						msg = g_strdup (_("The password is too short."));
					} else if (g_strrstr (str->str, "palindrome") != NULL ||
						   g_strrstr (str->str, "simpl") != NULL ||
						   g_strrstr (str->str, "dictionary") != NULL) {
						msg = g_strdup (_("The password is too simple."));
					} else if (g_strrstr (str->str, "similar") != NULL ||
					           g_strrstr (str->str, "different") != NULL ||
					           g_strrstr (str->str, "case") != NULL ||
						   g_strrstr (str->str, "wrapped") != NULL) {
						msg = g_strdup (_("The old and new passwords are too similar."));
					} else if (g_strrstr (str->str, "1 numeric or special") != NULL) {
						msg = g_strdup (_("The new password must contain numeric or special character(s)."));
					} else if (g_strrstr (str->str, "unchanged") != NULL ||
						   g_strrstr (str->str, "match") != NULL) {
						msg = g_strdup (_("The old and new passwords are the same."));
					} else if (g_strrstr (str->str, "recent") != NULL) {
						msg = g_strdup (_("The new password has already been used recently."));
					} else if (g_strrstr (str->str, "failure") != NULL) {
						/* Authentication failure */
						msg = g_strdup (_("Your password has been changed since you initially authenticated! Please re-authenticate."));

						passdlg_set_auth_state (pdialog, FALSE);
					}
				}

				reinit = TRUE;

				if (msg != NULL) {
					/* An error occured! */
					passdlg_set_status (pdialog, msg);
					g_free (msg);

					/* At this point, passwd might have exited, in which case
					 * child_watch_cb should clean up for us and remove this watcher.
					 * On some error conditions though, passwd just re-prompts us
					 * for our new password. */

					pdialog->backend_state = PASSWD_STATE_ERR;
				}

				/* child_watch_cb should clean up for us now */
			}
			break;
		case PASSWD_STATE_NONE:
			/* Passwd is not asking for anything yet */
			if (is_string_complete (str->str, "assword: ", NULL)) {

				/* If the user does not have a password set,
				 * passwd will immediately ask for the new password,
				 * so skip the AUTH phase */
				if (is_string_complete (str->str, "new", "New", NULL)) {
					gchar *pw;

					pdialog->backend_state = PASSWD_STATE_NEW;

					passdlg_set_busy (pdialog, FALSE);
					authenticated_user (pdialog);

					/* since passwd didn't ask for our old password
					 * in this case, simply remove it from the queue */
					pw = g_queue_pop_head (pdialog->backend_stdin_queue);
					g_free (pw);
				} else {

					pdialog->backend_state = PASSWD_STATE_AUTH;

					/* Pop the IO queue, i.e. send current password */
					io_queue_pop (pdialog->backend_stdin_queue, pdialog->backend_stdin);
				}

				reinit = TRUE;
			}
			break;
		default:
			/* Passwd has returned an error */
			reinit = TRUE;
			break;
	}

	if (reinit) {
		g_string_free (str, TRUE);
		str = NULL;
	}

	/* Continue calling us */
	return TRUE;
}

/*
 * }} Backend communication code
 */

/* Adds the current password to the IO queue */
static void
authenticate (PasswordDialog *pdialog)
{
	gchar	*s;

	s = g_strdup_printf ("%s\n", gtk_entry_get_text (pdialog->current_password));

	g_queue_push_tail (pdialog->backend_stdin_queue, s);
}

/* Adds the new password twice to the IO queue */
static void
update_password (PasswordDialog *pdialog)
{
	gchar	*s;

	s = g_strdup_printf ("%s\n", gtk_entry_get_text (pdialog->new_password));

	g_queue_push_tail (pdialog->backend_stdin_queue, s);
	/* We need to allocate new space because io_queue_pop() g_free()s
	 * every element of the queue after it's done */
	g_queue_push_tail (pdialog->backend_stdin_queue, g_strdup (s));
}

/* Sets dialog busy state according to busy
 *
 * When busy:
 *	Sets the cursor to busy
 *  Disables the interface to prevent that the user interferes
 * Reverts all this when non-busy
 *
 * Note that this function takes into account the
 * authentication state of the dialog. So setting the
 * dialog to busy and then back to normal should leave
 * the dialog unchanged.
 */
static void
passdlg_set_busy (PasswordDialog *pdialog, gboolean busy)
{
	GtkBuilder *dialog;
	GtkWidget  *toplevel;
	GdkCursor  *cursor = NULL;
	GdkDisplay *display;

	dialog = pdialog->ui;

	/* Set cursor */
	toplevel = WID ("change-password");
	display = gtk_widget_get_display (toplevel);
	if (busy) {
		cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
	}

	gdk_window_set_cursor (gtk_widget_get_window (toplevel), cursor);
	gdk_display_flush (display);

	if (busy) {
		gdk_cursor_unref (cursor);
	}

	/* Disable/Enable UI */
	if (pdialog->authenticated) {
		/* Authenticated state */

		/* Enable/disable new password section */
		g_object_set (pdialog->new_password, "sensitive", !busy, NULL);
		g_object_set (pdialog->retyped_password, "sensitive", !busy, NULL);
		g_object_set (WID ("new-password-label"), "sensitive", !busy, NULL);
		g_object_set (WID ("retyped-password-label"), "sensitive", !busy, NULL);

		/* Enable/disable change password button */
		g_object_set (WID ("change-password-button"), "sensitive", !busy, NULL);

	} else {
		/* Not-authenticated state */

		/* Enable/disable auth section state */
		g_object_set (pdialog->current_password, "sensitive", !busy, NULL);
		g_object_set (WID ("authenticate-button"), "sensitive", !busy, NULL);
		g_object_set (WID ("current-password-label"), "sensitive", !busy, NULL);
	}
}

/* Launch an error dialog */
static void
passdlg_error_dialog (GtkWindow *parent, const gchar *title,
		      const gchar *msg, const gchar *details)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					 msg);
	if (title)
		gtk_window_set_title (GTK_WINDOW (dialog), title);

	if (details)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  details);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/* Set authenticated state of dialog according to state
 *
 * When in authenticated state:
 * 	Disables authentication-part of interface
 *  Enables new-password-part of interface
 * When in not-authenticated state:
 * 	Enables authentication-part of interface
 *  Disables new-password-part of interface
 * 	Disables the change-password-button
 */
static void
passdlg_set_auth_state (PasswordDialog *pdialog, gboolean state)
{
	GtkBuilder *dialog;

	dialog = pdialog->ui;

	/* Widgets which require a not-authenticated state to be accessible */
	g_object_set (pdialog->current_password, "sensitive", !state, NULL);
	g_object_set (WID ("current-password-label"), "sensitive", !state, NULL);
	g_object_set (WID ("authenticate-button"), "sensitive", !state, NULL);

	/* Widgets which require authentication to be accessible */
	g_object_set (pdialog->new_password, "sensitive", state, NULL);
	g_object_set (pdialog->retyped_password, "sensitive", state, NULL);
	g_object_set (WID ("new-password-label"), "sensitive", state, NULL);
	g_object_set (WID ("retyped-password-label"), "sensitive", state, NULL);

	if (!state) {
		/* Disable change-password-button when in not-authenticated state */
		g_object_set (WID ("change-password-button"), "sensitive", FALSE, NULL);
	}

	pdialog->authenticated = state;

	if (state) {
		/* Authenticated state */

		/* Focus new password */
		gtk_widget_grab_focus (GTK_WIDGET (pdialog->new_password));

		/* Set open lock image */
		gtk_image_set_from_icon_name (GTK_IMAGE (pdialog->dialog_image), "changes-allow", GTK_ICON_SIZE_DIALOG);
	} else {
		/* Not authenticated state */

		/* Focus current password */
		gtk_widget_grab_focus (GTK_WIDGET (pdialog->current_password));

		/* Set closed lock image */
		gtk_image_set_from_icon_name (GTK_IMAGE (pdialog->dialog_image), "changes-prevent", GTK_ICON_SIZE_DIALOG);
	}
}

/* Set status field message */
static void
passdlg_set_status (PasswordDialog *pdialog, gchar *msg)
{
	g_object_set (pdialog->status_label, "label", msg, NULL);
}

/* Clear dialog (except the status message) */
static void
passdlg_clear (PasswordDialog *pdialog)
{
	/* Set non-authenticated state */
	passdlg_set_auth_state (pdialog, FALSE);

	/* Clear password entries */
	gtk_entry_set_text (pdialog->current_password, "");
	gtk_entry_set_text (pdialog->new_password, "");
	gtk_entry_set_text (pdialog->retyped_password, "");
}

/* Start backend and handle errors
 * If backend is already running, stop it
 * If an error occurs, show error dialog */
static gboolean
passdlg_spawn_passwd (PasswordDialog *pdialog)
{
	GError	*error = NULL;
	gchar	*details;

	/* If backend is running, stop it */
	stop_passwd (pdialog);

	/* Spawn backend */
	if (!spawn_passwd (pdialog, &error)) {
		GtkWidget *parent = GTK_WIDGET (gtk_builder_get_object (pdialog->ui, "change-password"));

		/* translators: Unable to launch <program>: <error message> */
		details = g_strdup_printf (_("Unable to launch %s: %s"),
					   "/usr/bin/passwd", error->message);

		passdlg_error_dialog (GTK_WINDOW (parent),
				      _("Unable to launch backend"),
				      _("A system error has occurred"),
				      details);

		g_free (details);
		g_error_free (error);

		return FALSE;
	}

	return TRUE;
}

/* Called when the "Authenticate" button is clicked */
static void
passdlg_authenticate (GtkButton *button, PasswordDialog *pdialog)
{
	/* Set busy as this can be a long process */
	passdlg_set_busy (pdialog, TRUE);

	/* Update status message */
	passdlg_set_status (pdialog, _("Checking password..."));

	/* Spawn backend */
	if (!passdlg_spawn_passwd (pdialog)) {
		passdlg_set_busy (pdialog, FALSE);
		return;
	}

	authenticate (pdialog);

	/* Our IO watcher should now handle the rest */
}

/* Validate passwords
 * Returns:
 * PASSDLG_ERROR_NONE (0) if passwords are valid
 * PASSDLG_ERROR_NEW_PASSWORD_EMPTY
 * PASSDLG_ERROR_RETYPED_PASSWORD_EMPTY
 * PASSDLG_ERROR_PASSWORDS_NOT_EQUAL
 */
static guint
passdlg_validate_passwords (PasswordDialog *pdialog)
{
	GtkBuilder	*dialog;
	const gchar	*new_password, *retyped_password;
	glong			nlen, rlen;

	dialog = pdialog->ui;

	new_password = gtk_entry_get_text (pdialog->new_password);
	retyped_password = gtk_entry_get_text (pdialog->retyped_password);

	nlen = g_utf8_strlen (new_password, -1);
	rlen = g_utf8_strlen (retyped_password, -1);

	if (nlen == 0) {
		/* New password empty */
		return PASSDLG_ERROR_NEW_PASSWORD_EMPTY;
	}

	if (rlen == 0) {
		/* Retyped password empty */
		return PASSDLG_ERROR_RETYPED_PASSWORD_EMPTY;
	}

	if (nlen != rlen || strncmp (new_password, retyped_password, nlen) != 0) {
		/* Passwords not equal */
		return PASSDLG_ERROR_PASSWORDS_NOT_EQUAL;
	}

	/* Success */
	return PASSDLG_ERROR_NONE;
}

/* Refresh the valid password UI state, i.e. re-validate
 * and enable/disable the Change Password button.
 * Returns: Return value of passdlg_validate_passwords */
static guint
passdlg_refresh_password_state (PasswordDialog *pdialog)
{
	GtkBuilder *dialog;
	guint		ret;
	gboolean	valid = FALSE;

	dialog = pdialog->ui;

	ret = passdlg_validate_passwords (pdialog);

	if (ret == PASSDLG_ERROR_NONE) {
		valid = TRUE;
	}

	g_object_set (WID ("change-password-button"), "sensitive", valid, NULL);

	return ret;
}

/* Called whenever any of the new password fields have changed */
static void
passdlg_check_password (GtkEntry *entry, PasswordDialog *pdialog)
{
	guint	ret;

	ret = passdlg_refresh_password_state (pdialog);

	switch (ret) {
		case PASSDLG_ERROR_NONE:
			passdlg_set_status (pdialog, _("Click <b>Change password</b> to change your password."));
			break;
		case PASSDLG_ERROR_NEW_PASSWORD_EMPTY:
			passdlg_set_status (pdialog, _("Please type your password in the <b>New password</b> field."));
			break;
		case PASSDLG_ERROR_RETYPED_PASSWORD_EMPTY:
			passdlg_set_status (pdialog, _("Please type your password again in the <b>Retype new password</b> field."));
			break;
		case PASSDLG_ERROR_PASSWORDS_NOT_EQUAL:
			passdlg_set_status (pdialog, _("The two passwords are not equal."));
			break;
		default:
			g_warning ("Unknown passdlg_check_password error: %d", ret);
			break;
	}
}

/* Called when the "Change password" dialog-button is clicked
 * Returns: TRUE if we want to keep the dialog running, FALSE otherwise */
static gboolean
passdlg_process_response (PasswordDialog *pdialog, gint response_id)
{

	if (response_id == GTK_RESPONSE_OK) {
		/* Set busy as this can be a long process */
		passdlg_set_busy (pdialog, TRUE);

		/* Stop passwd if an error occured and it is still running */
		if (pdialog->backend_state == PASSWD_STATE_ERR) {

			/* Stop passwd, free resources */
			stop_passwd (pdialog);
		}

		/* Check that the backend is still running, or that an error
		 * hass occured but it has not yet exited */
		if (pdialog->backend_pid == -1) {
			/* If it is not, re-run authentication */

			/* Spawn backend */
			if (!passdlg_spawn_passwd (pdialog)) {
				return TRUE;
			}

			/* Add current and new passwords to queue */
			authenticate (pdialog);
			update_password (pdialog);
		} else {
			/* Only add new passwords to queue */
			update_password (pdialog);

			/* Pop new password through the backend */
			io_queue_pop (pdialog->backend_stdin_queue, pdialog->backend_stdin);
		}

		/* Our IO watcher should now handle the rest */

		/* Keep the dialog running */
		return TRUE;
	}

	return FALSE;
}

/* Activates (moves focus or activates) widget w */
static void
passdlg_activate (GtkEntry *entry, GtkWidget *w)
{
	if (GTK_IS_BUTTON (w)) {
		gtk_widget_activate (w);
	} else {
		gtk_widget_grab_focus (w);
	}
}

/* Initialize password dialog */
static void
passdlg_init (PasswordDialog *pdialog, GtkWindow *parent)
{
	GtkBuilder		*dialog;
	GtkWidget		*wpassdlg;
	GtkAccelGroup	*group;

	/* Initialize dialog */
	dialog = gtk_builder_new ();
    gtk_builder_add_from_file (dialog, MATECC_UI_DIR "/mate-about-me-password.ui", NULL);
	pdialog->ui = dialog;

	wpassdlg = WID ("change-password");
	capplet_set_icon (wpassdlg, "user-info");

	group = gtk_accel_group_new ();

	/*
	 * Initialize backend
	 */

	/* Initialize backend_pid. -1 means the backend is not running */
	pdialog->backend_pid = -1;

	/* Initialize IO Channels */
	pdialog->backend_stdin = NULL;
	pdialog->backend_stdout = NULL;

	/* Initialize write queue */
	pdialog->backend_stdin_queue = g_queue_new ();

	/* Initialize watchers */
	pdialog->backend_child_watch_id = 0;
	pdialog->backend_stdout_watch_id = 0;

	/* Initialize backend state */
	pdialog->backend_state = PASSWD_STATE_NONE;

	/*
	 * Initialize UI
	 */

	/* Initialize pdialog widgets */
	pdialog->current_password	= GTK_ENTRY (WID ("current-password"));
	pdialog->new_password		= GTK_ENTRY (WID ("new-password"));
	pdialog->retyped_password	= GTK_ENTRY (WID ("retyped-password"));
	pdialog->dialog_image		= GTK_IMAGE (WID ("dialog-image"));
	pdialog->status_label		= GTK_LABEL (WID ("status-label"));

	/* Initialize accelerators */
	gtk_widget_add_accelerator (GTK_WIDGET (pdialog->current_password),
								"activate", group,
								GDK_Return, 0,
								0);

	gtk_widget_add_accelerator (GTK_WIDGET (pdialog->new_password),
								"activate", group,
								GDK_Return, 0,
								0);

	/* Activate authenticate-button when enter is pressed in current-password */
	g_signal_connect (G_OBJECT (pdialog->current_password), "activate",
					  G_CALLBACK (passdlg_activate), WID ("authenticate-button"));

	/* Activate retyped-password when enter is pressed in new-password */
	g_signal_connect (G_OBJECT (pdialog->new_password), "activate",
					  G_CALLBACK (passdlg_activate), pdialog->retyped_password);

	/* Clear status message */
	passdlg_set_status (pdialog, "");

	/* Set non-authenticated state */
	passdlg_set_auth_state (pdialog, FALSE);

	/* Connect signal handlers */
	g_signal_connect (G_OBJECT (WID ("authenticate-button")), "clicked",
					  G_CALLBACK (passdlg_authenticate), pdialog);

	/* Verify new passwords on-the-fly */
	g_signal_connect (G_OBJECT (WID ("new-password")), "changed",
					  G_CALLBACK (passdlg_check_password), pdialog);
	g_signal_connect (G_OBJECT (WID ("retyped-password")), "changed",
					  G_CALLBACK (passdlg_check_password), pdialog);

	/* Set misc dialog properties */
	gtk_window_set_resizable (GTK_WINDOW (wpassdlg), FALSE);
	gtk_window_set_transient_for (GTK_WINDOW (wpassdlg), GTK_WINDOW (parent));
}

/* Main */
void
mate_about_me_password (GtkWindow *parent)
{
	PasswordDialog	*pdialog;
	GtkBuilder		*dialog;
	GtkWidget		*wpassdlg;

	gint			result;
	gboolean		response;

	/* Initialize dialog */
	pdialog = g_new0 (PasswordDialog, 1);
	passdlg_init (pdialog, parent);

	dialog = pdialog->ui;
	wpassdlg = WID ("change-password");

	/* Go! */
	gtk_widget_show_all (wpassdlg);

	do {
		result = gtk_dialog_run (GTK_DIALOG (wpassdlg));
		response = passdlg_process_response (pdialog, result);
	} while (response);

	/* Clean up */
	stop_passwd (pdialog);
	gtk_widget_destroy (wpassdlg);
	g_queue_free (pdialog->backend_stdin_queue);
	g_object_unref (dialog);
	g_free (pdialog);
}
