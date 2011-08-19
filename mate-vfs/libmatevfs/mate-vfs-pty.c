/*
 * Copyright (C) 2001,2002,2004 Red Hat, Inc.
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Originally from vte */

#include "../config.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTIL_H
#include <util.h>
#endif
#include <glib.h>
#include "mate-vfs-pty.h"

int _mate_vfs_pty_set_size(int master, int columns, int rows);

/* Solaris does not have the login_tty() function so implement locally. */
#ifndef HAVE_LOGIN_TTY
static int login_tty(int pts)
{
#if defined(HAVE_STROPTS_H)
  /* push a terminal onto stream head */
  if (ioctl(pts, I_PUSH, "ptem")   == -1) return -1;
  if (ioctl(pts, I_PUSH, "ldterm") == -1) return -1;
#endif
  setsid();
#if defined(TIOCSCTTY)
  ioctl(pts, TIOCSCTTY, 0);
#endif
  dup2(pts, 0);
  dup2(pts, 1);
  dup2(pts, 2);
  if (pts > 2) close(pts);
  return 0;
}
#endif

/* Reset the handlers for all known signals to their defaults.  The parent
 * (or one of the libraries it links to) may have changed one to be ignored. */
static void
_mate_vfs_pty_reset_signal_handlers(void)
{
	signal(SIGHUP,  SIG_DFL);
	signal(SIGINT,  SIG_DFL);
	signal(SIGILL,  SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	signal(SIGFPE,  SIG_DFL);
	signal(SIGKILL, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGALRM, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
	signal(SIGCONT, SIG_DFL);
	signal(SIGSTOP, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
#ifdef SIGBUS
	signal(SIGBUS,  SIG_DFL);
#endif
#ifdef SIGPOLL
	signal(SIGPOLL, SIG_DFL);
#endif
#ifdef SIGPROF
	signal(SIGPROF, SIG_DFL);
#endif
#ifdef SIGSYS
	signal(SIGSYS,  SIG_DFL);
#endif
#ifdef SIGTRAP
	signal(SIGTRAP, SIG_DFL);
#endif
#ifdef SIGURG
	signal(SIGURG,  SIG_DFL);
#endif
#ifdef SIGVTALARM
	signal(SIGVTALARM, SIG_DFL);
#endif
#ifdef SIGXCPU
	signal(SIGXCPU, SIG_DFL);
#endif
#ifdef SIGXFSZ
	signal(SIGXFSZ, SIG_DFL);
#endif
#ifdef SIGIOT
	signal(SIGIOT,  SIG_DFL);
#endif
#ifdef SIGEMT
	signal(SIGEMT,  SIG_DFL);
#endif
#ifdef SIGSTKFLT
	signal(SIGSTKFLT, SIG_DFL);
#endif
#ifdef SIGIO
	signal(SIGIO,   SIG_DFL);
#endif
#ifdef SIGCLD
	signal(SIGCLD,  SIG_DFL);
#endif
#ifdef SIGPWR
	signal(SIGPWR,  SIG_DFL);
#endif
#ifdef SIGINFO
	signal(SIGINFO, SIG_DFL);
#endif
#ifdef SIGLOST
	signal(SIGLOST, SIG_DFL);
#endif
#ifdef SIGWINCH
	signal(SIGWINCH, SIG_DFL);
#endif
#ifdef SIGUNUSED
	signal(SIGUNUSED, SIG_DFL);
#endif
}

#ifdef HAVE_SOCKETPAIR
static int
_mate_vfs_pty_pipe_open(int *a, int *b)
{
	int p[2], ret = -1;
#ifdef PF_UNIX
#ifdef SOCK_STREAM
	ret = socketpair(PF_UNIX, SOCK_STREAM, 0, p);
#else
#ifdef SOCK_DGRAM
	ret = socketpair(PF_UNIX, SOCK_DGRAM, 0, p);
#endif
#endif
	if (ret == 0) {
		*a = p[0];
		*b = p[1];
		return 0;
	}
#endif
	return ret;
}
#else
static int
_mate_vfs_pty_pipe_open(int *a, int *b)
{
	int p[2], ret = -1;

	ret = pipe(p);

	if (ret == 0) {
		*a = p[0];
		*b = p[1];
	}
	return ret;
}
#endif

static int
_mate_vfs_pty_pipe_open_bi(int *a, int *b, int *c, int *d)
{
	int ret;
	ret = _mate_vfs_pty_pipe_open(a, b);
	if (ret != 0) {
		return ret;
	}
	ret = _mate_vfs_pty_pipe_open(c, d);
	if (ret != 0) {
		close(*a);
		close(*b);
	}
	return ret;
}

/* Like read, but hide EINTR and EAGAIN. */
static ssize_t
n_read(int fd, void *buffer, size_t count)
{
	size_t n = 0;
	char *buf = buffer;
	int i;
	while (n < count) {
		i = read(fd, buf + n, count - n);
		switch (i) {
		case 0:
			return n;
			break;
		case -1:
			switch (errno) {
			case EINTR:
			case EAGAIN:
#ifdef ERESTART
			case ERESTART:
#endif
				break;
			default:
				return -1;
			}
			break;
		default:
			n += i;
			break;
		}
	}
	return n;
}

/* Like write, but hide EINTR and EAGAIN. */
static ssize_t
n_write(int fd, const void *buffer, size_t count)
{
	size_t n = 0;
	const char *buf = buffer;
	int i;
	while (n < count) {
		i = write(fd, buf + n, count - n);
		switch (i) {
		case 0:
			return n;
			break;
		case -1:
			switch (errno) {
			case EINTR:
			case EAGAIN:
#ifdef ERESTART
			case ERESTART:
#endif
				break;
			default:
				return -1;
			}
			break;
		default:
			n += i;
			break;
		}
	}
	return n;
}

/* Run the given command (if specified), using the given descriptor as the
 * controlling terminal. */
static int
_mate_vfs_pty_run_on_pty(int fd, gboolean login,
			  int stdin_fd, int stdout_fd, int stderr_fd, 
			  int ready_reader, int ready_writer,
			  char **env_add, const char *command, char **argv,
			  const char *directory)
{
	int i;
	char c;
	char **args, *arg;

#ifdef HAVE_STROPTS_H
	if (!ioctl (fd, I_FIND, "ptem") && ioctl (fd, I_PUSH, "ptem") == -1) {
		close (fd);
		_exit (0);
		return -1;
	}

	if (!ioctl (fd, I_FIND, "ldterm") && ioctl (fd, I_PUSH, "ldterm") == -1) {
		close (fd);
		_exit (0);
		return -1;
	}

	if (!ioctl (fd, I_FIND, "ttcompat") && ioctl (fd, I_PUSH, "ttcompat") == -1) {
		perror ("ioctl (fd, I_PUSH, \"ttcompat\")");
		close (fd);
		_exit (0);
		return -1;
	}
#endif /* HAVE_STROPTS_H */

	/* Set any environment variables. */
	for (i = 0; (env_add != NULL) && (env_add[i] != NULL); i++) {
		if (putenv(g_strdup(env_add[i])) != 0) {
			g_warning("Error adding `%s' to environment, "
				    "continuing.", env_add[i]);
		}
#ifdef MATE_VFS_DEBUG
		if (_mate_vfs_debug_on(MATE_VFS_DEBUG_MISC) ||
		    _mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
			fprintf(stderr, "%ld: Set `%s'.\n", (long) getpid(),
				env_add[i]);
		}
#endif
	}

	/* Reset our signals -- our parent may have done any number of
	 * weird things to them. */
	_mate_vfs_pty_reset_signal_handlers();

	/* Change to the requested directory. */
	if (directory != NULL) {
		chdir(directory);
	}

#ifdef HAVE_UTMP_H
	/* This sets stdin, stdout, stderr to the socket */	
	if (login && login_tty (fd) == -1) {
		g_printerr ("mount child process login_tty failed: %s\n", strerror (errno));
		return -1;
	}
#endif
	
	/* Signal to the parent that we've finished setting things up by
	 * sending an arbitrary byte over the status pipe and waiting for
	 * a response.  This synchronization step ensures that the pty is
	 * fully initialized before the parent process attempts to do anything
	 * with it, and is required on systems where additional setup, beyond
	 * merely opening the device, is required.  This is at least the case
	 * on Solaris. */
#ifdef MATE_VFS_DEBUG
	if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
		fprintf(stderr, "Child sending child-ready.\n");
	}
#endif
	/* Initialize so valgrind doesn't complain */
	c = 0;
	n_write(ready_writer, &c, 1);
	fsync(ready_writer);
	n_read(ready_reader, &c, 1);
#ifdef MATE_VFS_DEBUG
	if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
		fprintf(stderr, "Child received parent-ready.\n");
	}
#endif
	close(ready_writer);
	if (ready_writer != ready_reader) {
		close(ready_reader);
	}

	/* If the caller provided a command, we can't go back, ever. */
	if (command != NULL) {
		/* Outta here. */
		if (argv != NULL) {
			for (i = 0; (argv[i] != NULL); i++) ;
			args = g_malloc0(sizeof(char*) * (i + 1));
			for (i = 0; (argv[i] != NULL); i++) {
				args[i] = g_strdup(argv[i]);
			}
			execvp(command, args);
		} else {
			arg = g_strdup(command);
			execlp(command, arg, NULL);
		} 

		/* Avoid calling any atexit() code. */
		_exit(0);
		g_assert_not_reached();
	}

	return 0;
}

/* Open the named PTY slave, fork off a child (storing its PID in child),
 * and exec the named command in its own session as a process group leader */
static int
_mate_vfs_pty_fork_on_pty_name(const char *path, int parent_fd, char **env_add,
				const char *command, char **argv,
				const char *directory,
				int columns, int rows, 
				int *stdin_fd, int *stdout_fd, int *stderr_fd, 
				pid_t *child, gboolean reapchild, gboolean login)
{
	int fd, i;
	char c;
	int ready_a[2] = { 0, 0 };
	int ready_b[2] = { 0, 0 };
	pid_t pid, grandchild_pid;
	int pid_pipe[2];
	int stdin_pipe[2];
	int stdout_pipe[2];
	int stderr_pipe[2];

	/* Open pipes for synchronizing between parent and child. */
	if (_mate_vfs_pty_pipe_open_bi(&ready_a[0], &ready_a[1],
					&ready_b[0], &ready_b[1]) == -1) {
		/* Error setting up pipes.  Bail. */
		goto bail_ready;
	}

	if (reapchild && pipe(pid_pipe)) {
		/* Error setting up pipes. Bail. */
		goto bail_pid;
	}
	
	if (pipe(stdin_pipe)) {
		/* Error setting up pipes.  Bail. */
		goto bail_stdin;
	}
	if (pipe(stdout_pipe)) {
		/* Error setting up pipes.  Bail. */
		goto bail_stdout;
	}
	if (pipe(stderr_pipe)) {
		/* Error setting up pipes.  Bail. */
		goto bail_stderr;
	}

	/* Start up a child. */
	pid = fork();
	switch (pid) {
	case -1:
		/* Error fork()ing.  Bail. */
		*child = -1;
		return -1;
		break;
	case 0:
		/* Child. Close the parent's ends of the pipes. */
		close(ready_a[0]);
		close(ready_b[1]);
	
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stderr_pipe[0]);

		if(reapchild) {
			close(pid_pipe[0]);

			/* Fork a intermediate child. This is needed to not
			 * produce zombies! */
			grandchild_pid = fork();

			if (grandchild_pid < 0) {
				/* Error during fork! */
				n_write (pid_pipe[1], &grandchild_pid, 
					 sizeof (grandchild_pid));
				_exit (1);
			} else if (grandchild_pid > 0) {
				/* Parent! (This is the actual intermediate child;
				 * so write the pid to the parent and then exit */
				n_write (pid_pipe[1], &grandchild_pid, 
					 sizeof (grandchild_pid));
				close (pid_pipe[1]);
				_exit (0);
			}
		
			/* Start a new session and become process-group leader. */
			setsid();
			setpgid(0, 0);
		}

		/* Close most descriptors. */
		for (i = 0; i < sysconf(_SC_OPEN_MAX); i++) {
			if ((i != ready_b[0]) && 
			    (i != ready_a[1]) &&
			    (i != stdin_pipe[0]) &&
			    (i != stdout_pipe[1]) &&
			    (i != stderr_pipe[1])) {
				close(i);
			}
		}

		/* Set up stdin/out/err */
		dup2(stdin_pipe[0], STDIN_FILENO);
		close (stdin_pipe[0]);
		dup2(stdout_pipe[1], STDOUT_FILENO);
		close (stdout_pipe[1]);
		dup2(stderr_pipe[1], STDERR_FILENO);
		close (stderr_pipe[1]);

		/* Open the slave PTY, acquiring it as the controlling terminal
		 * for this process and its children. */
		fd = open(path, O_RDWR);
		if (fd == -1) {
			return -1;
		}
#ifdef TIOCSCTTY
		/* TIOCSCTTY is defined?  Let's try that, too. */
		ioctl(fd, TIOCSCTTY, fd);
#endif
		/* Store 0 as the "child"'s ID to indicate to the caller that
		 * it is now the child. */
		*child = 0;
		return _mate_vfs_pty_run_on_pty(fd, login,
						 stdin_pipe[1], stdout_pipe[1], stderr_pipe[1], 
						 ready_b[0], ready_a[1],
						 env_add, command, argv, directory);
		break;
	default:
		/* Parent.  Close the child's ends of the pipes, do the ready
		 * handshake, and return the child's PID. */
		close(ready_b[0]);
		close(ready_a[1]);

		close(stdin_pipe[0]);
		close(stdout_pipe[1]);
		close(stderr_pipe[1]);

		if (reapchild) {
			close(pid_pipe[1]);

			/* Reap the intermediate child */
        	wait_again:	
			if (waitpid (pid, NULL, 0) < 0) {
				if (errno == EINTR) {
					goto wait_again;
				} else if (errno == ECHILD) {
					; /* NOOP! Child already reaped. */
				} else {
					g_warning ("waitpid() should not fail in pty-open.c");
				}
			}
	
			/*
			 * Read the child pid from the pid_pipe 
			 * */
			if (n_read (pid_pipe[0], child, sizeof (pid_t)) 
			  	!= sizeof (pid_t) || *child == -1) {
				g_warning ("Error while spanning child!");
				goto bail_fork;
			}
			
			close(pid_pipe[0]);

		} else {
			/* No intermediate child, simple */
			*child = pid;
		}
		
		/* Wait for the child to be ready, set the window size, then
		 * signal that we're ready.  We need to synchronize here to
		 * avoid possible races when the child has to do more setup
		 * of the terminal than just opening it. */
#ifdef MATE_VFS_DEBUG
		if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
			fprintf(stderr, "Parent ready, waiting for child.\n");
		}
#endif
		n_read(ready_a[0], &c, 1);
#ifdef MATE_VFS_DEBUG
		if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
			fprintf(stderr, "Parent received child-ready.\n");
		}
#endif
		_mate_vfs_pty_set_size(parent_fd, columns, rows);
#ifdef MATE_VFS_DEBUG
		if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
			fprintf(stderr, "Parent sending parent-ready.\n");
		}
#endif
		n_write(ready_b[1], &c, 1);
		close(ready_a[0]);
		close(ready_b[1]);

		*stdin_fd = stdin_pipe[1];
		*stdout_fd = stdout_pipe[0];
		*stderr_fd = stderr_pipe[0];

		return 0;
		break;
	}
	g_assert_not_reached();
	return -1;

 bail_fork:
	close(stderr_pipe[0]);
	close(stderr_pipe[1]);
 bail_stderr:
	close(stdout_pipe[0]);
	close(stdout_pipe[1]);
 bail_stdout:
	close(stdin_pipe[0]);
	close(stdin_pipe[1]);
 bail_stdin:
	if(reapchild) {
		close(pid_pipe[0]);
		close(pid_pipe[1]);
	}
 bail_pid:
	close(ready_a[0]);
	close(ready_a[1]);
	close(ready_b[0]);
	close(ready_b[1]);
 bail_ready:
	*child = -1;
	return -1;
}

/**
 * mate_vfs_pty_set_size:
 * @master: the file descriptor of the pty master
 * @columns: the desired number of columns
 * @rows: the desired number of rows
 *
 * Attempts to resize the pseudo terminal's window size.  If successful, the
 * OS kernel will send #SIGWINCH to the child process group.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
_mate_vfs_pty_set_size(int master, int columns, int rows)
{
	struct winsize size;
	int ret;
	memset(&size, 0, sizeof(size));
	size.ws_row = rows ? rows : 24;
	size.ws_col = columns ? columns : 80;
#ifdef MATE_VFS_DEBUG
	if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
		fprintf(stderr, "Setting size on fd %d to (%d,%d).\n",
			master, columns, rows);
	}
#endif
	ret = ioctl(master, TIOCSWINSZ, &size);
#ifdef MATE_VFS_DEBUG
	if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
		if (ret != 0) {
			fprintf(stderr, "Failed to set size on %d: %s.\n",
				master, strerror(errno));
		}
	}
#endif
	return ret;
}


static char *
_mate_vfs_pty_ptsname(int master)
{
#if defined(HAVE_PTSNAME_R)
	gsize len = 1024;
	char *buf = NULL;
	int i;
	do {
		buf = g_malloc0(len);
		i = ptsname_r(master, buf, len - 1);
		switch (i) {
		case 0:
			/* Return the allocated buffer with the name in it. */
#ifdef MATE_VFS_DEBUG
			if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
				fprintf(stderr, "PTY slave is `%s'.\n", buf);
			}
#endif
			return buf;
			break;
		default:
			g_free(buf);
			buf = NULL;
			break;
		}
		len *= 2;
	} while ((i != 0) && (errno == ERANGE));
#elif defined(HAVE_PTSNAME)
	char *p;
	if ((p = ptsname(master)) != NULL) {
#ifdef MATE_VFS_DEBUG
		if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
			fprintf(stderr, "PTY slave is `%s'.\n", p);
		}
#endif
		return g_strdup(p);
	}
#elif defined(TIOCGPTN)
	int pty = 0;
	if (ioctl(master, TIOCGPTN, &pty) == 0) {
#ifdef MATE_VFS_DEBUG
		if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
			fprintf(stderr, "PTY slave is `/dev/pts/%d'.\n", pty);
		}
#endif
		return g_strdup_printf("/dev/pts/%d", pty);
	}
#endif
	return NULL;
}

static int
_mate_vfs_pty_getpt(void)
{
	int fd, flags;
#ifdef HAVE_GETPT
	/* Call the system's function for allocating a pty. */
	fd = getpt();
#elif defined(HAVE_POSIX_OPENPT)
	fd = posix_openpt(O_RDWR | O_NOCTTY);
#else
	/* Try to allocate a pty by accessing the pty master multiplex. */
	fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
	if ((fd == -1) && (errno == ENOENT)) {
		fd = open("/dev/ptc", O_RDWR | O_NOCTTY); /* AIX */
	}
#endif
	/* Set it to blocking. */
	flags = fcntl(fd, F_GETFL);
	flags &= ~(O_NONBLOCK);
	fcntl(fd, F_SETFL, flags);
	return fd;
}

static int
_mate_vfs_pty_grantpt(int master)
{
#ifdef HAVE_GRANTPT
	return grantpt(master);
#else
	return 0;
#endif
}

static int
_mate_vfs_pty_unlockpt(int fd)
{
#ifdef HAVE_UNLOCKPT
	return unlockpt(fd);
#elif defined(TIOCSPTLCK)
	int zero = 0;
	return ioctl(fd, TIOCSPTLCK, &zero);
#else
	return -1;
#endif
}

static int
_mate_vfs_pty_open_unix98(pid_t *child, guint flags, char **env_add,
			   const char *command, char **argv,
			   const char *directory, int columns, int rows,
			   int *stdin_fd, int *stdout_fd, int *stderr_fd)
{
	int fd;
	char *buf;

	/* Attempt to open the master. */
	fd = _mate_vfs_pty_getpt();
#ifdef MATE_VFS_DEBUG
	if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
		fprintf(stderr, "Allocated pty on fd %d.\n", fd);
	}
#endif
	if (fd != -1) {
		/* Read the slave number and unlock it. */
		if (((buf = _mate_vfs_pty_ptsname(fd)) == NULL) ||
		    (_mate_vfs_pty_grantpt(fd) != 0) ||
		    (_mate_vfs_pty_unlockpt(fd) != 0)) {
#ifdef MATE_VFS_DEBUG
			if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
				fprintf(stderr, "PTY setup failed, bailing.\n");
			}
#endif
			close(fd);
			fd = -1;
		} else {
			/* Start up a child process with the given command. */
			if (_mate_vfs_pty_fork_on_pty_name(buf, fd, env_add, command,
						      argv, directory,
						      columns, rows,
						      stdin_fd, stdout_fd, stderr_fd, 
						      child, 
						      flags & MATE_VFS_PTY_REAP_CHILD, 
						      flags & MATE_VFS_PTY_LOGIN_TTY) != 0) {
				close(fd);
				fd = -1;
			}
			g_free(buf);
		}
	}
	return fd;
}

/**
 * _mate_vfs_pty_open:
 * @child: location to store the new process's ID
 * @env_add: a list of environment variables to add to the child's environment
 * @command: name of the binary to run
 * @argv: arguments to pass to @command
 * @directory: directory to start the new command in, or NULL
 * @columns: desired window columns
 * @rows: desired window rows
 * @lastlog: TRUE if the lastlog should be updated
 * @utmp: TRUE if the utmp or utmpx log should be updated
 * @wtmp: TRUE if the wtmp or wtmpx log should be updated
 *
 * Starts a new copy of @command running under a psuedo-terminal, optionally in
 * the supplied @directory, with window size set to @rows x @columns
 * and variables in @env_add added to its environment.  If any combination of
 * @lastlog, @utmp, and @wtmp is set, then the session is logged in the
 * corresponding system files.
 *
 * Returns: an open file descriptor for the pty master, -1 on failure
 */
int
mate_vfs_pty_open(pid_t *child, guint flags, char **env_add, 
		   const char *command, char **argv, const char *directory,
		   int columns, int rows,
		   int *stdin_fd, int *stdout_fd, int *stderr_fd)
{
	int ret = -1;
	if (ret == -1) {
		ret = _mate_vfs_pty_open_unix98(child, flags, env_add, command, 
						 argv, directory, columns, rows,
						 stdin_fd, stdout_fd, stderr_fd);
	}
#ifdef MATE_VFS_DEBUG
	if (_mate_vfs_debug_on(MATE_VFS_DEBUG_PTY)) {
		fprintf(stderr, "Returning ptyfd = %d.\n", ret);
	}
#endif
	return ret;
}


#ifdef PTY_MAIN
int fd;

static void
sigchld_handler(int signum)
{
}

int
main(int argc, char **argv)
{
	pid_t child = 0;
	char c;
	int ret;
	signal(SIGCHLD, sigchld_handler);
	_mate_vfs_debug_parse_string(getenv("MATE_VFS_DEBUG_FLAGS"));
	fd = _mate_vfs_pty_open(&child, NULL,
			   (argc > 1) ? argv[1] : NULL,
			   (argc > 1) ? argv + 1 : NULL,
			   NULL,
			   0, 0,
			   TRUE, TRUE, TRUE);
	if (child == 0) {
		int i;
		for (i = 0; ; i++) {
			switch (i % 3) {
			case 0:
			case 1:
				fprintf(stdout, "%d\n", i);
				break;
			case 2:
				fprintf(stderr, "%d\n", i);
				break;
			default:
				g_assert_not_reached();
				break;
			}
			sleep(1);
		}
	}
	g_print("Child pid is %d.\n", (int)child);
	do {
		ret = n_read(fd, &c, 1);
		if (ret == 0) {
			break;
		}
		if ((ret == -1) && (errno != EAGAIN) && (errno != EINTR)) {
			break;
		}
		if (argc < 2) {
			n_write(STDOUT_FILENO, "[", 1);
		}
		n_write(STDOUT_FILENO, &c, 1);
		if (argc < 2) {
			n_write(STDOUT_FILENO, "]", 1);
		}
	} while (TRUE);
	return 0;
}
#endif
