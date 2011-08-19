/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-parse-ls.c - Routines for parsing output from the `ls' command.

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

   Authors: 1995 Miguel de Icaza
   1995 Jakub Jelinek
   1998 Pavel Machek
   1999 Cleanup by Ettore Perazzoli

   finduid, findgid are from GNU tar.  */

#include <config.h>
#include "mate-vfs-parse-ls.h"

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#ifndef TUNMLEN 
#define TUNMLEN 256
#endif
#ifndef TGNMLEN
#define TGNMLEN 256
#endif


typedef struct {
	int saveuid;
	int my_uid;
	char saveuname[TUNMLEN];
} finduid_cache;
#define finduid_cache_init {-993, -993}

typedef struct {
	int savegid;
	int my_gid;
	char savegname[TGNMLEN];
} findgid_cache;
#define findgid_cache_init {-993, -993}

static int finduid (char *uname, finduid_cache *cache)
{
	struct passwd *pw;
	
	if (uname[0] != cache->saveuname[0]/* Quick test w/o proc call */
	    || 0 != strncmp (uname, cache->saveuname, TUNMLEN)) {
		strncpy (cache->saveuname, uname, TUNMLEN);
		pw = getpwnam (uname);
		if (pw) {
			cache->saveuid = pw->pw_uid;
		} else {
			if (cache->my_uid < 0)
				cache->my_uid = getuid ();
			cache->saveuid = cache->my_uid;
		}
	}
	return cache->saveuid;
}

static int findgid (char *gname, findgid_cache *cache)
{
	struct group *gr;
	
	if (gname[0] != cache->savegname[0]/* Quick test w/o proc call */
	    || 0 != strncmp (gname, cache->savegname, TUNMLEN)) {
		strncpy (cache->savegname, gname, TUNMLEN);
		gr = getgrnam (gname);
		if (gr) {
			cache->savegid = gr->gr_gid;
		} else {
			if (cache->my_gid < 0)
				cache->my_gid = getgid ();
			cache->savegid = cache->my_gid;
		}
	}
	return cache->savegid;
}


/* FIXME bugzilla.eazel.com 1188: This is ugly.  */
#define MAXCOLS 30

static int
vfs_split_text (char *p,
		char *columns[],
		int column_ptr[])
{
	char *original = p;
	int  numcols;

	for (numcols = 0; *p && numcols < MAXCOLS; numcols++) {
		while (*p == ' ' || *p == '\r' || *p == '\n') {
			*p = 0;
			p++;
		}
		columns [numcols] = p;
		column_ptr [numcols] = p - original;
		while (*p && *p != ' ' && *p != '\r' && *p != '\n')
			p++;
	}
	return numcols;
}

static int
is_num (const char *s)
{
	if (!s || s[0] < '0' || s[0] > '9')
		return 0;
	return 1;
}

static int
is_dos_date (char *str)
{
	if (strlen (str) == 8 && str[2] == str[5] && strchr ("\\-/", (int)str[2]) != NULL)
		return 1;

	return 0;
}

static int
is_week (char *str, struct tm *tim)
{
	static char *week = "SunMonTueWedThuFriSat";
	char *pos;

	if ((pos = strstr (week, str)) != NULL) {
		if (tim != NULL)
			tim->tm_wday = (pos - week)/3;
		return 1;
	}
	return 0;    
}

static int
is_month (const char *str, struct tm *tim)
{
	static char *month = "JanFebMarAprMayJunJulAugSepOctNovDec";
	char *pos;
    
	if ((pos = strstr (month, str)) != NULL) {
		if (tim != NULL)
			tim->tm_mon = (pos - month)/3;
		return 1;
	}
	return 0;
}

static int
is_time (const char *str, struct tm *tim)
{
	char *p, *p2;

	if ((p = strchr (str, ':')) && (p2 = strrchr (str, ':'))) {
		if (p != p2) {
			if (sscanf (str, "%2d:%2d:%2d",
				    &tim->tm_hour, &tim->tm_min, &tim->tm_sec)
			    != 3)
				return 0;
		}
		else {
			if (sscanf (str, "%2d:%2d",
				    &tim->tm_hour, &tim->tm_min)
			    != 2)
				return 0;
		}
	}
	else 
		return 0;
    
	return 1;
}

static int is_year (const char *str, struct tm *tim, const char *carbon, int column_ptr[], int idx)
{
	long year;

	const char *p;
	int count = 0;

	if (strchr (str,':'))
		return 0;

	if (strlen (str) != 4)
		return 0;

	if (sscanf (str, "%ld", &year) != 1)
		return 0;

	if (year < 1900 || year > 3000)
		return 0;

	p = carbon + column_ptr[idx -1];
	
	while (*(p + count) != ' ')
		count++;

	if (!((*(p+count) == ' ') && (*(p+1+count) == ' ') && (isdigit (*(p+2+count)))))
		return 0;

	tim->tm_year = (int) (year - 1900);

	return 1;
}

/*
 * FIXME bugzilla.eazel.com 1182:
 * this is broken. Consider following entry:
 * -rwx------   1 root     root            1 Aug 31 10:04 2904 1234
 * where "2904 1234" is filename. Well, this code decodes it as year :-(.
 */

static int
vfs_parse_filetype (char c)
{
	switch (c) {
        case 'd':
		return S_IFDIR; 
        case 'b':
		return S_IFBLK;
        case 'c':
		return S_IFCHR;
        case 'l':
		return S_IFLNK;
        case 's':
#ifdef IS_IFSOCK /* And if not, we fall through to IFIFO, which is pretty
		    close. */
		return S_IFSOCK;
#endif
        case 'p':
		return S_IFIFO;
        case 'm':
	case 'n':		/* Don't know what these are :-) */
        case '-':
	case '?':
		return S_IFREG;
        default:
		return -1;
	}
}

static int
vfs_parse_filemode (const char *p)
{
	/* converts rw-rw-rw- into 0666 */
	int res = 0;

	switch (*(p++)) {
	case 'r':
		res |= 0400; break;
	case '-':
		break;
	default:
		return -1;
	}

	switch (*(p++)) {
	case 'w':
		res |= 0200; break;
	case '-':
		break;
	default:
		return -1;
	}

	switch (*(p++)) {
	case 'x':
		res |= 0100; break;
	case 's':
		res |= 0100 | S_ISUID; break;
	case 'S':
		res |= S_ISUID; break;
	case '-':
		break;
	default:
		return -1;
	}

	switch (*(p++)) {
	case 'r':
		res |= 0040; break;
	case '-':
		break;
	default:
		return -1;
	}

	switch (*(p++)) {
	case 'w':
		res |= 0020; break;
	case '-':
		break;
	default:
		return -1;
	}

	switch (*(p++)) {
	case 'x':
		res |= 0010; break;
	case 's':
		res |= 0010 | S_ISGID; break;
        case 'l':
	/* Solaris produces these */
	case 'S':
		res |= S_ISGID; break;
	case '-':
		break;
	default:
		return -1;
	}

	switch (*(p++)) {
	case 'r':
		res |= 0004; break;
	case '-':
		break;
	default:
		return -1;
	}

	switch (*(p++)) {
	case 'w':
		res |= 0002; break;
	case '-':
		break;
	default:
		return -1;
	}

	switch (*(p++)) {
	case 'x':
		res |= 0001; break;
	case 't':
		res |= 0001 | S_ISVTX; break;
	case 'T':
		res |= S_ISVTX; break;
	case '-':
		break;
	default:
		return -1;
	}

	return res;
}

static gboolean
is_last_column (int idx,
		int num_cols,
		const char *carbon,
		int column_ptr[])
{
	const char *p;

	if (idx + 1 == num_cols) {
		return TRUE;
	}

	p = carbon + column_ptr[idx + 1] - 1;
	return *p == '\r' || *p == '\n';
}

static int
vfs_parse_filedate (int idx,
		    char *columns[],
		    int num_cols,
		    const char *carbon,
		    int column_ptr[],
		    time_t *t)
{	/* This thing parses from idx in columns[] array */

	char *p;
	struct tm tim;
	int d[3];
	int got_year = 0, got_time = 0;
	int current_mon;
	time_t now;
    
	/* Let's setup default time values */
	now = time (NULL);
	tim = *localtime (&now);
	current_mon = tim.tm_mon;
	tim.tm_hour = 0;
	tim.tm_min  = 0;
	tim.tm_sec  = 0;
	tim.tm_isdst = -1; /* Let mktime () try to guess correct dst offset */
    
	p = columns [idx++];
    
	/* We eat weekday name in case of extfs */
	if (is_week (p, &tim))
		p = columns [idx++];

	/* Month name */
	if (is_month (p, &tim)) {
		/* And we expect, it followed by day number */
		if (is_num (columns[idx]))
			tim.tm_mday = (int)atol (columns [idx++]);
		else
			return 0; /* No day */

	} else {
		/* We usually expect:
		   Mon DD hh:mm
		   Mon DD  YYYY
		   But in case of extfs we allow these date formats:
		   Mon DD YYYY hh:mm
		   Mon DD hh:mm YYYY
		   Wek Mon DD hh:mm:ss YYYY
		   MM-DD-YY hh:mm
		   where Mon is Jan-Dec, DD, MM, YY two digit day, month, year,
		   YYYY four digit year, hh, mm, ss two digit hour, minute
		   or second. */

		/* Here just this special case with MM-DD-YY */
		if (is_dos_date (p)) {
			p[2] = p[5] = '-';
	    
			if (sscanf (p, "%2d-%2d-%2d", &d[0], &d[1], &d[2]) == 3) {
				/*  We expect to get:
				    1. MM-DD-YY
				    2. DD-MM-YY
				    3. YY-MM-DD
				    4. YY-DD-MM  */
		
				/* Hmm... maybe, next time :)*/
		
				/* At last, MM-DD-YY */
				d[0]--; /* Months are zerobased */
				/* Y2K madness */
				if (d[2] < 70)
					d[2] += 100;

				tim.tm_mon  = d[0];
				tim.tm_mday = d[1];
				tim.tm_year = d[2];
				got_year = 1;
			} else
				return 0; /* sscanf failed */
		} else
			return 0; /* unsupported format */
	}

	/* Here we expect to find time and/or year */

	if (is_num (columns[idx])) {
		if ((got_time = is_time (columns[idx], &tim)) ||
		    (got_year = is_year (columns[idx], &tim, carbon, column_ptr, idx))) {
			idx++;

			/* This is a special case for ctime () or Mon DD YYYY hh:mm */
			if (is_num (columns[idx]) && 
			    !is_last_column (idx, num_cols, carbon, column_ptr) && /* ensure that we don't eat two lines at once,
										      where the first line provides a year-like
										      filename but no year, or a time-like filename
										      but no time */
			    ((!got_year && (got_year = is_year (columns[idx], &tim, carbon, column_ptr, idx))) ||
			     (!got_time && (got_time = is_time (columns[idx], &tim)))))
				idx++; /* time & year or reverse */
		} /* only time or date */
	}
	else 
		return 0; /* Nor time or date */

	/*
	 * If the date is less than 6 months in the past, it is shown without year
	 * other dates in the past or future are shown with year but without time
	 * This does not check for years before 1900 ... I don't know, how
	 * to represent them at all
	 */
	if (!got_year &&
	    current_mon < 6 && current_mon < tim.tm_mon && 
	    tim.tm_mon - current_mon >= 6)

		tim.tm_year--;

	if ((*t = mktime (&tim)) < 0)
		*t = 0;
	return idx;
}

/**
 * mate_vfs_parse_ls_lga:
 * @p: string containing the data in the form same as 'ls -al'.
 * @s: pointer to stat structure.
 * @filename: filename, will be filled here.
 * @linkname: linkname, will be filled here.
 *
 * Parses the string @p which is in the form same as the 'ls -al' output and fills
 * in the details in the struct @s, @filename and @linkname.
 *
 * Return value: 0 if cannot parse @p, 1 if successful.
 */
int
mate_vfs_parse_ls_lga (const char *p,
			struct stat *s,
			char **filename,
			char **linkname)
{
	char *columns [MAXCOLS]; /* Points to the string in column n */
	int column_ptr [MAXCOLS]; /* Index from 0 to the starting positions of the columns */
	int idx, idx2, num_cols;
	int i;
	int nlink;
	char *p_copy, *p_pristine;
	finduid_cache uid_cache = finduid_cache_init;
	findgid_cache gid_cache = findgid_cache_init;

	if (strncmp (p, "total", 5) == 0)
		return 0;

	p_copy = g_strdup (p);

	if ((i = vfs_parse_filetype (*(p++))) == -1)
		goto error;

	s->st_mode = i;
	if (*p == ' ')	/* Notwell 4 */
		p++;
	if (*p == '[') {
		if (strlen (p) <= 8 || p [8] != ']')
			goto error;
		/* Should parse here the Notwell permissions :) */
		if (S_ISDIR (s->st_mode))
			s->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR
				       | S_IXUSR | S_IXGRP | S_IXOTH);
		else
			s->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
		p += 9;
	} else {
		if ((i = vfs_parse_filemode (p)) == -1)
			goto error;
		s->st_mode |= i;
		p += 9;

		/* This is for an extra ACL attribute (HP-UX) */
		if (*p == '+')
			p++;
	}

	g_free (p_copy);
	p_copy = g_strdup (p);
	p_pristine = g_strdup (p);
	num_cols = vfs_split_text (p_copy, columns, column_ptr);

	nlink = atol (columns [0]);
	if (nlink < 0)
		goto error;

	s->st_nlink = nlink;

	if (!is_num (columns[1]))
		s->st_uid = finduid (columns [1], &uid_cache);
	else
		s->st_uid = (uid_t) atol (columns [1]);

	/* Mhm, the ls -lg did not produce a group field */
	for (idx = 3; idx <= 5; idx++) 
		if (is_month (columns [idx], NULL)
		    || is_week (columns [idx], NULL)
		    || is_dos_date (columns[idx]))
			break;

	if (idx == 6 || (idx == 5
			 && !S_ISCHR (s->st_mode)
			 && !S_ISBLK (s->st_mode)))
		goto error;

	/* We don't have gid */	
	if (idx == 3 || (idx == 4 && (S_ISCHR (s->st_mode)
				      || S_ISBLK (s->st_mode))))
		idx2 = 2;
	else { 
		/* We have gid field */
		if (is_num (columns[2]))
			s->st_gid = (gid_t) atol (columns [2]);
		else
			s->st_gid = findgid (columns [2], &gid_cache);
		idx2 = 3;
	}

	/* This is device */
	if (S_ISCHR (s->st_mode) || S_ISBLK (s->st_mode)) {
		guint32 maj, min;
	
		if (!is_num (columns[idx2])
		    || sscanf (columns [idx2], " %d,", &maj) != 1)
			goto error;
	
		if (!is_num (columns[++idx2])
		    || sscanf (columns [idx2], " %d", &min) != 1)
			goto error;
	
#ifdef HAVE_STRUCT_STAT_ST_RDEV
		/* Starting from linux 2.6, minor number is split between bits 
		 * 0-7 and 20-31 of dev_t. This calculation is also valid
		 * on older kernel with 8 bit minor and major numbers 
		 * http://lwn.net/Articles/49966/ has a pretty good explanation
		 * of the format of dev_t
		 */
		min = (min & 0xff) | ((min & 0xfff00) << 12);
		s->st_rdev = ((maj & 0xfff) << 8) | (min & 0xfff000ff);
#endif
		s->st_size = 0;
	
	} else {
		/* Common file size */
		if (!is_num (columns[idx2]))
			goto error;

#ifdef HAVE_ATOLL
		s->st_size = (off_t) atoll (columns [idx2]);
#else
		s->st_size = (off_t) atof (columns [idx2]);
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
		s->st_rdev = 0;
#endif
	}

	idx = vfs_parse_filedate (idx, columns, num_cols, p, column_ptr, &s->st_mtime);
	if (!idx)
		goto error;
	/* Use resulting time value */
	s->st_atime = s->st_ctime = s->st_mtime;
	s->st_dev = 0;
	s->st_ino = 0;
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
	s->st_blksize = 512;
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
	s->st_blocks = (s->st_size + 511) / 512;
#endif

	for (i = idx + 1, idx2 = 0; i < num_cols; i++ ) 
		if (strcmp (columns [i], "->") == 0) {
			idx2 = i;
			break;
		}
    
	if (((S_ISLNK (s->st_mode) 
	      || (num_cols == idx + 3 && s->st_nlink > 1))) /* Maybe a hardlink?
							       (in extfs) */
	    && idx2) {
		int p;
		char *s;
	    
		if (filename) {
			s = g_strndup (p_pristine + column_ptr [idx],
				       column_ptr [idx2] - column_ptr [idx] - 1);
			*filename = s;
		}
		if (linkname) {
			p = strcspn (p_pristine + column_ptr[idx2+1], "\r\n");
			s = g_strndup (p_pristine + column_ptr [idx2+1], p);
			*linkname = s;
		}
	} else {
		/* Extract the filename from the string copy, not from the columns
		 * this way we have a chance of entering hidden directories like ". ."
		 */
		if (filename) {
			/* 
			 *filename = g_strdup (columns [idx++]);
			 */
			int p;
			char *s;

			s = g_strdup (p_pristine + column_ptr [idx]);
			p = strcspn (s, "\r\n");
			s[p] = '\0';

			*filename = s;
		}
		if (linkname)
			*linkname = NULL;
	}
	g_free (p_copy);
	g_free (p_pristine);
	return 1;

 error:
	{
		static int errorcount = 0;

		if (++errorcount < 5)
			g_warning (_("Could not parse: %s"), p_copy);
		else if (errorcount == 5)
			g_warning (_("More parsing errors will be ignored."));
	}

	if (p_copy != p)		/* Carefull! */
		g_free (p_copy);
	return 0;
}
