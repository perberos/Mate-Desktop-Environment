/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pwd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "mdm-common.h"

#ifndef HAVE_MKDTEMP
#include "mkdtemp.h"
#endif

const char *
mdm_make_temp_dir (char *template)
{
        return mkdtemp (template);
}

gboolean
mdm_is_version_unstable (void)
{
        char   **versions;
        gboolean unstable;

        unstable = FALSE;

        versions = g_strsplit (VERSION, ".", 3);
        if (versions && versions [0] && versions [1]) {
                int major;
                major = atoi (versions [1]);
                if ((major % 2) != 0) {
                        unstable = TRUE;
                }
        }
        g_strfreev (versions);

        return unstable;
}

void
mdm_set_fatal_warnings_if_unstable (void)
{
        if (mdm_is_version_unstable ()) {
                g_setenv ("G_DEBUG", "fatal_criticals", FALSE);
                g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
        }
}

gboolean
mdm_get_pwent_for_name (const char     *name,
                        struct passwd **pwentp)
{
        struct passwd *pwent;

        do {
                errno = 0;
                pwent = getpwnam (name);
        } while (pwent == NULL && errno == EINTR);

        if (pwentp != NULL) {
                *pwentp = pwent;
        }

        return (pwent != NULL);
}

int
mdm_wait_on_pid (int pid)
{
        int status;

 wait_again:
        errno = 0;
        if (waitpid (pid, &status, 0) < 0) {
                if (errno == EINTR) {
                        goto wait_again;
                } else if (errno == ECHILD) {
                        ; /* do nothing, child already reaped */
                } else {
                        g_debug ("MdmCommon: waitpid () should not fail");
                }
        }

        g_debug ("MdmCommon: process (pid:%d) done (%s:%d)",
                 (int) pid,
                 WIFEXITED (status) ? "status"
                 : WIFSIGNALED (status) ? "signal"
                 : "unknown",
                 WIFEXITED (status) ? WEXITSTATUS (status)
                 : WIFSIGNALED (status) ? WTERMSIG (status)
                 : -1);

        return status;
}

int
mdm_signal_pid (int pid,
                int signal)
{
        int status = -1;

        /* perhaps block sigchld */
        g_debug ("MdmCommon: sending signal %d to process %d", signal, pid);
        errno = 0;
        status = kill (pid, signal);

        if (status < 0) {
                if (errno == ESRCH) {
                        g_warning ("Child process %d was already dead.",
                                   (int)pid);
                } else {
                        g_warning ("Couldn't kill child process %d: %s",
                                   pid,
                                   g_strerror (errno));
                }
        }

        /* perhaps unblock sigchld */

        return status;
}

/* hex conversion adapted from D-Bus */
/**
 * Appends a two-character hex digit to a string, where the hex digit
 * has the value of the given byte.
 *
 * @param str the string
 * @param byte the byte
 */
static void
_mdm_string_append_byte_as_hex (GString *str,
                                int      byte)
{
        const char hexdigits[16] = {
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                'a', 'b', 'c', 'd', 'e', 'f'
        };

        str = g_string_append_c (str, hexdigits[(byte >> 4)]);

        str = g_string_append_c (str, hexdigits[(byte & 0x0f)]);
}

/**
 * Encodes a string in hex, the way MD5 and SHA-1 are usually
 * encoded. (Each byte is two hex digits.)
 *
 * @param source the string to encode
 * @param start byte index to start encoding
 * @param dest string where encoded data should be placed
 * @param insert_at where to place encoded data
 * @returns #TRUE if encoding was successful, #FALSE if no memory etc.
 */
gboolean
mdm_string_hex_encode (const GString *source,
                       int            start,
                       GString       *dest,
                       int            insert_at)
{
        GString             *result;
        const unsigned char *p;
        const unsigned char *end;
        gboolean             retval;

        g_return_val_if_fail (source != NULL, FALSE);
        g_return_val_if_fail (dest != NULL, FALSE);
        g_return_val_if_fail (source != dest, FALSE);
        g_return_val_if_fail (start >= 0, FALSE);
        g_return_val_if_fail (dest >= 0, FALSE);
        g_assert (start <= source->len);

        result = g_string_new (NULL);

        retval = FALSE;

        p = (const unsigned char*) source->str;
        end = p + source->len;
        p += start;

        while (p != end) {
                _mdm_string_append_byte_as_hex (result, *p);
                ++p;
        }

        dest = g_string_insert (dest, insert_at, result->str);

        retval = TRUE;

        g_string_free (result, TRUE);

        return retval;
}

/**
 * Decodes a string from hex encoding.
 *
 * @param source the string to decode
 * @param start byte index to start decode
 * @param end_return return location of the end of the hex data, or #NULL
 * @param dest string where decoded data should be placed
 * @param insert_at where to place decoded data
 * @returns #TRUE if decoding was successful, #FALSE if no memory.
 */
gboolean
mdm_string_hex_decode (const GString *source,
                       int            start,
                       int           *end_return,
                       GString       *dest,
                       int            insert_at)
{
        GString             *result;
        const unsigned char *p;
        const unsigned char *end;
        gboolean             retval;
        gboolean             high_bits;

        g_return_val_if_fail (source != NULL, FALSE);
        g_return_val_if_fail (dest != NULL, FALSE);
        g_return_val_if_fail (source != dest, FALSE);
        g_return_val_if_fail (start >= 0, FALSE);
        g_return_val_if_fail (dest >= 0, FALSE);

        g_assert (start <= source->len);

        result = g_string_new (NULL);

        retval = FALSE;

        high_bits = TRUE;
        p = (const unsigned char*) source->str;
        end = p + source->len;
        p += start;

        while (p != end) {
                unsigned int val;

                switch (*p) {
                case '0':
                        val = 0;
                        break;
                case '1':
                        val = 1;
                        break;
                case '2':
                        val = 2;
                        break;
                case '3':
                        val = 3;
                        break;
                case '4':
                        val = 4;
                        break;
                case '5':
                        val = 5;
                        break;
                case '6':
                        val = 6;
                        break;
                case '7':
                        val = 7;
                        break;
                case '8':
                        val = 8;
                        break;
                case '9':
                        val = 9;
                        break;
                case 'a':
                case 'A':
                        val = 10;
                        break;
                case 'b':
                case 'B':
                        val = 11;
                        break;
                case 'c':
                case 'C':
                        val = 12;
                        break;
                case 'd':
                case 'D':
                        val = 13;
                        break;
                case 'e':
                case 'E':
                        val = 14;
                        break;
                case 'f':
                case 'F':
                        val = 15;
                        break;
                default:
                        goto done;
                }

                if (high_bits) {
                        result = g_string_append_c (result, val << 4);
                } else {
                        int           len;
                        unsigned char b;

                        len = result->len;

                        b = result->str[len - 1];

                        b |= val;

                        result->str[len - 1] = b;
                }

                high_bits = !high_bits;

                ++p;
        }

 done:
        dest = g_string_insert (dest, insert_at, result->str);

        if (end_return) {
                *end_return = p - (const unsigned char*) source->str;
        }

        retval = TRUE;

        g_string_free (result, TRUE);

        return retval;
}

static gboolean
_fd_is_character_device (int fd)
{
        struct stat file_info;

        if (fstat (fd, &file_info) < 0) {
                return FALSE;
        }

        return S_ISCHR (file_info.st_mode);
}

static gboolean
_read_bytes (int      fd,
             char    *bytes,
             gsize    number_of_bytes,
             GError **error)
{
        size_t bytes_left_to_read;
        size_t total_bytes_read = 0;
        gboolean premature_eof;

        bytes_left_to_read = number_of_bytes;
        premature_eof = FALSE;
        do {
                size_t bytes_read = 0;

                errno = 0;
                bytes_read = read (fd, ((guchar *) bytes) + total_bytes_read,
                                   bytes_left_to_read);

                if (bytes_read > 0) {
                        total_bytes_read += bytes_read;
                        bytes_left_to_read -= bytes_read;
                } else if (bytes_read == 0) {
                        premature_eof = TRUE;
                        break;
                } else if ((errno != EINTR)) {
                        break;
                }
        } while (bytes_left_to_read > 0);

        if (premature_eof) {
                g_set_error (error,
                             G_FILE_ERROR,
                             G_FILE_ERROR_FAILED,
                             "No data available");

                return FALSE;
        } else if (bytes_left_to_read > 0) {
                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (errno),
                             "%s", g_strerror (errno));
                return FALSE;
        }

        return TRUE;
}

/**
 * Pulls a requested number of bytes from /dev/urandom
 *
 * @param size number of bytes to pull
 * @param error error if read fails
 * @returns The requested number of random bytes or #NULL if fail
 */

char *
mdm_generate_random_bytes (gsize    size,
                           GError **error)
{
        int fd;
        char *bytes;
        GError *read_error;

        /* We don't use the g_rand_* glib apis because they don't document
         * how much entropy they are seeded with, and it might be less
         * than the passed in size.
         */

        errno = 0;
        fd = open ("/dev/urandom", O_RDONLY);

        if (fd < 0) {
                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (errno),
                             "%s", g_strerror (errno));
                close (fd);
                return NULL;
        }

        if (!_fd_is_character_device (fd)) {
                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (ENODEV),
                             _("/dev/urandom is not a character device"));
                close (fd);
                return NULL;
        }

        bytes = g_malloc (size);
        read_error = NULL;
        if (!_read_bytes (fd, bytes, size, &read_error)) {
                g_propagate_error (error, read_error);
                g_free (bytes);
                close (fd);
                return NULL;
        }

        close (fd);
        return bytes;
}
