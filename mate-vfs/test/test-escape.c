/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* test-async-cancel.c - Test program for the MATE Virtual File System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Darin Adler <darin@eazel.com>
*/

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(expression, message) \
	G_STMT_START { if (!(expression)) test_failed message; } G_STMT_END

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

static gboolean at_least_one_test_failed = FALSE;

static void
test_failed (const char *format, ...)
{
	va_list arguments;
	char *message;

	va_start (arguments, format);
	message = g_strdup_vprintf (format, arguments);
	va_end (arguments);

	g_message ("test failed: %s", message);
	at_least_one_test_failed = TRUE;
}

static void
test_escape (const char *input, const char *expected_output)
{
	char *output;

	output = mate_vfs_escape_string (input);

	if (strcmp (output, expected_output) != 0) {
		test_failed ("escaping %s resulted in %s instead of %s",
			     input, output, expected_output);
	}

	g_free (output);
}

static void
test_escape_path (const char *input, const char *expected_output)
{
	char *output;

	output = mate_vfs_escape_path_string (input);

	if (strcmp (output, expected_output) != 0) {
		test_failed ("escaping path %s resulted in %s instead of %s",
			     input, output, expected_output);
	}

	g_free (output);
}

static void
test_unescape (const char *input, const char *illegal, const char *expected_output)
{
	char *output;

	output = mate_vfs_unescape_string (input, illegal);
	if (expected_output == NULL) {
		if (output != NULL) {
			test_failed ("unescaping %s resulted in %s instead of NULL",
				     input, illegal, output);
		}
	} else {
		if (output == NULL) {
			test_failed ("unescaping %s resulted in NULL instead of %s",
				     input, illegal, expected_output);
		} else if (strcmp (output, expected_output) != 0) {
			test_failed ("unescaping %s with %s illegal resulted in %s instead of %s",
				     input, illegal, output, expected_output);
		}
	}

	g_free (output);
}

static void
test_unescape_display (const char *input, const char *expected_output)
{
	char *output;

	output = mate_vfs_unescape_string_for_display (input);
	if (expected_output == NULL) {
		if (output != NULL) {
			test_failed ("unescaping %s for display resulted in %s instead of NULL",
				     input, output);
		}
	} else {
		if (output == NULL) {
			test_failed ("unescaping %s for display resulted in NULL instead of %s",
				     input, expected_output);
		} else if (strcmp (output, expected_output) != 0) {
			test_failed ("unescaping %s for display resulted in %s instead of %s",
				     input, output, expected_output);
		}
	}

	g_free (output);
}

int
main (int argc, char **argv)
{
	make_asserts_break ("MateVFS");

	mate_vfs_init ();

	test_escape ("", "");

	test_escape ("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	test_escape ("abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz");
	test_escape ("0123456789", "0123456789");
	test_escape ("-_.!~*'()", "-_.!~*'()");

	test_escape ("\x01\x02\x03\x04\x05\x06\x07", "%01%02%03%04%05%06%07");
	test_escape ("\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", "%08%09%0A%0B%0C%0D%0E%0F");
	test_escape (" \"#$%&+,/", "%20%22%23%24%25%26%2B%2C%2F");
	test_escape (":;<=>?@", "%3A%3B%3C%3D%3E%3F%40");
	test_escape ("[\\]^`", "%5B%5C%5D%5E%60");
	test_escape ("{|}\x7F", "%7B%7C%7D%7F");

	test_escape ("\x80\x81\x82\x83\x84\x85\x86\x87", "%80%81%82%83%84%85%86%87");
	test_escape ("\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F", "%88%89%8A%8B%8C%8D%8E%8F");
	test_escape ("\x90\x91\x92\x93\x94\x95\x96\x97", "%90%91%92%93%94%95%96%97");
	test_escape ("\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F", "%98%99%9A%9B%9C%9D%9E%9F");
	test_escape ("\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7", "%A0%A1%A2%A3%A4%A5%A6%A7");
	test_escape ("\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF", "%A8%A9%AA%AB%AC%AD%AE%AF");
	test_escape ("\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7", "%B0%B1%B2%B3%B4%B5%B6%B7");
	test_escape ("\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF", "%B8%B9%BA%BB%BC%BD%BE%BF");
	test_escape ("\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7", "%C0%C1%C2%C3%C4%C5%C6%C7");
	test_escape ("\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF", "%C8%C9%CA%CB%CC%CD%CE%CF");
	test_escape ("\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7", "%D0%D1%D2%D3%D4%D5%D6%D7");
	test_escape ("\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF", "%D8%D9%DA%DB%DC%DD%DE%DF");
	test_escape ("\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7", "%E0%E1%E2%E3%E4%E5%E6%E7");
	test_escape ("\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF", "%E8%E9%EA%EB%EC%ED%EE%EF");
	test_escape ("\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7", "%F0%F1%F2%F3%F4%F5%F6%F7");
	test_escape ("\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF", "%F8%F9%FA%FB%FC%FD%FE%FF");

	test_escape_path ("", "");

	test_escape_path ("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	test_escape_path ("abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz");
	test_escape_path ("0123456789", "0123456789");
	test_escape_path ("-_.!~*'()/", "-_.!~*'()/");

	test_escape_path ("\x01\x02\x03\x04\x05\x06\x07", "%01%02%03%04%05%06%07");
	test_escape_path ("\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", "%08%09%0A%0B%0C%0D%0E%0F");
	test_escape_path (" \"#$%&+,", "%20%22%23%24%25&%2B%2C");
	test_escape_path (":;<=>?@", "%3A%3B%3C=%3E%3F%40");
	test_escape_path ("[\\]^`", "%5B%5C%5D%5E%60");
	test_escape_path ("{|}\x7F", "%7B%7C%7D%7F");

	test_escape_path ("\x80\x81\x82\x83\x84\x85\x86\x87", "%80%81%82%83%84%85%86%87");
	test_escape_path ("\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F", "%88%89%8A%8B%8C%8D%8E%8F");
	test_escape_path ("\x90\x91\x92\x93\x94\x95\x96\x97", "%90%91%92%93%94%95%96%97");
	test_escape_path ("\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F", "%98%99%9A%9B%9C%9D%9E%9F");
	test_escape_path ("\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7", "%A0%A1%A2%A3%A4%A5%A6%A7");
	test_escape_path ("\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF", "%A8%A9%AA%AB%AC%AD%AE%AF");
	test_escape_path ("\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7", "%B0%B1%B2%B3%B4%B5%B6%B7");
	test_escape_path ("\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF", "%B8%B9%BA%BB%BC%BD%BE%BF");
	test_escape_path ("\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7", "%C0%C1%C2%C3%C4%C5%C6%C7");
	test_escape_path ("\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF", "%C8%C9%CA%CB%CC%CD%CE%CF");
	test_escape_path ("\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7", "%D0%D1%D2%D3%D4%D5%D6%D7");
	test_escape_path ("\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF", "%D8%D9%DA%DB%DC%DD%DE%DF");
	test_escape_path ("\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7", "%E0%E1%E2%E3%E4%E5%E6%E7");
	test_escape_path ("\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF", "%E8%E9%EA%EB%EC%ED%EE%EF");
	test_escape_path ("\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7", "%F0%F1%F2%F3%F4%F5%F6%F7");
	test_escape_path ("\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF", "%F8%F9%FA%FB%FC%FD%FE%FF");

	test_unescape ("", NULL, "");
	test_unescape ("", "", "");
	test_unescape ("", "/", "");
	test_unescape ("", "/:", "");

	test_unescape ("/", "/", "/");
	test_unescape ("%2f", NULL, "/");
	test_unescape ("%2F", NULL, "/");
	test_unescape ("%2F", "/", NULL);
	test_unescape ("%", NULL, NULL);
	test_unescape ("%1", NULL, NULL);
	test_unescape ("%aa", NULL, "\xAA");
	test_unescape ("%ag", NULL, NULL);
	test_unescape ("%g", NULL, NULL);
	test_unescape ("%%", NULL, NULL);
	test_unescape ("%1%1", NULL, NULL);
	test_unescape ("ABCDEFGHIJKLMNOPQRSTUVWXYZ", NULL, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	test_unescape ("abcdefghijklmnopqrstuvwxyz", NULL, "abcdefghijklmnopqrstuvwxyz");
	test_unescape ("0123456789", NULL, "0123456789");
	test_unescape ("-_.!~*'()", NULL, "-_.!~*'()");

	test_unescape ("%01%02%03%04%05%06%07", NULL, "\x01\x02\x03\x04\x05\x06\x07");
	test_unescape ("%08%09%0A%0B%0C%0D%0E%0F", NULL, "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	test_unescape ("%20%22%23%24%25%26%2B%2C%2F", NULL, " \"#$%&+,/");
	test_unescape ("%3A%3B%3C%3D%3E%3F%40", NULL, ":;<=>?@");
	test_unescape ("%5B%5C%5D%5E%60", NULL, "[\\]^`");
	test_unescape ("%7B%7C%7D%7F", NULL, "{|}\x7F");
	
	test_unescape ("%80%81%82%83%84%85%86%87", NULL, "\x80\x81\x82\x83\x84\x85\x86\x87");
	test_unescape ("%88%89%8A%8B%8C%8D%8E%8F", NULL, "\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F");
	test_unescape ("%90%91%92%93%94%95%96%97", NULL, "\x90\x91\x92\x93\x94\x95\x96\x97");
	test_unescape ("%98%99%9A%9B%9C%9D%9E%9F", NULL, "\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F");
	test_unescape ("%A0%A1%A2%A3%A4%A5%A6%A7", NULL, "\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7");
	test_unescape ("%A8%A9%AA%AB%AC%AD%AE%AF", NULL, "\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF");
	test_unescape ("%B0%B1%B2%B3%B4%B5%B6%B7", NULL, "\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7");
	test_unescape ("%B8%B9%BA%BB%BC%BD%BE%BF", NULL, "\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF");
	test_unescape ("%C0%C1%C2%C3%C4%C5%C6%C7", NULL, "\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7");
	test_unescape ("%C8%C9%CA%CB%CC%CD%CE%CF", NULL, "\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF");
	test_unescape ("%D0%D1%D2%D3%D4%D5%D6%D7", NULL, "\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7");
	test_unescape ("%D8%D9%DA%DB%DC%DD%DE%DF", NULL, "\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF");
	test_unescape ("%E0%E1%E2%E3%E4%E5%E6%E7", NULL, "\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7");
	test_unescape ("%E8%E9%EA%EB%EC%ED%EE%EF", NULL, "\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF");
	test_unescape ("%F0%F1%F2%F3%F4%F5%F6%F7", NULL, "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7");
	test_unescape ("%F8%F9%FA%FB%FC%FD%FE%FF", NULL, "\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");
	
	test_unescape_display ("ABC%00DEF", "ABC%00DEF");
	test_unescape_display ("%20%22%23%24%25%26%2B%00%2C%2F", " \"#$%&+%00,/");
	test_unescape_display ("", "");
	test_unescape_display ("%1%1", "%1%1");
	test_unescape_display ("/", "/");
	test_unescape_display ("%2f", "/");
	test_unescape_display ("%2F", "/");
	test_unescape_display ("%", "%");
	test_unescape_display ("%1", "%1");
	test_unescape_display ("%aa", "\xAA");
	test_unescape_display ("%ag", "%ag");
	test_unescape_display ("%g", "%g");
	test_unescape_display ("%%", "%%");
	test_unescape_display ("%%20", "% ");
	test_unescape_display ("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	test_unescape_display ("abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz");
	test_unescape_display ("0123456789", "0123456789");
	test_unescape_display ("-_.!~*'()", "-_.!~*'()");

	test_unescape_display ("%01%02%03%04%05%06%07", "\x01\x02\x03\x04\x05\x06\x07");
	test_unescape_display ("%08%09%0A%0B%0C%0D%0E%0F", "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	test_unescape_display ("%20%22%23%24%25%26%2B%2C%2F", " \"#$%&+,/");
	test_unescape_display ("%3A%3B%3C%3D%3E%3F%40", ":;<=>?@");
	test_unescape_display ("%5B%5C%5D%5E%60", "[\\]^`");
	test_unescape_display ("%7B%7C%7D%7F", "{|}\x7F");
	
	test_unescape_display ("%80%81%82%83%84%85%86%87", "\x80\x81\x82\x83\x84\x85\x86\x87");
	test_unescape_display ("%88%89%8A%8B%8C%8D%8E%8F", "\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F");
	test_unescape_display ("%90%91%92%93%94%95%96%97", "\x90\x91\x92\x93\x94\x95\x96\x97");
	test_unescape_display ("%98%99%9A%9B%9C%9D%9E%9F", "\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F");
	test_unescape_display ("%A0%A1%A2%A3%A4%A5%A6%A7", "\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7");
	test_unescape_display ("%A8%A9%AA%AB%AC%AD%AE%AF", "\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF");
	test_unescape_display ("%B0%B1%B2%B3%B4%B5%B6%B7", "\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7");
	test_unescape_display ("%B8%B9%BA%BB%BC%BD%BE%BF", "\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF");
	test_unescape_display ("%C0%C1%C2%C3%C4%C5%C6%C7", "\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7");
	test_unescape_display ("%C8%C9%CA%CB%CC%CD%CE%CF", "\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF");
	test_unescape_display ("%D0%D1%D2%D3%D4%D5%D6%D7", "\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7");
	test_unescape_display ("%D8%D9%DA%DB%DC%DD%DE%DF", "\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF");
	test_unescape_display ("%E0%E1%E2%E3%E4%E5%E6%E7", "\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7");
	test_unescape_display ("%E8%E9%EA%EB%EC%ED%EE%EF", "\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF");
	test_unescape_display ("%F0%F1%F2%F3%F4%F5%F6%F7", "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7");
	test_unescape_display ("%F8%F9%FA%FB%FC%FD%FE%FF", "\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF");

	/* Report to "make check" on whether it all worked or not. */
	return at_least_one_test_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
