/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * mate-vfs-mime-magic.c
 *
 * Written by:
 *    James Youngman (jay@gnu.org)
 *
 * Adatped to the MATE needs by:
 *    Elliot Lee (sopwith@cuc.edu)
 * 
 * Rewritten by:
 *    Pavel Cisler <pavel@eazel.com>
 */

#include <config.h>
#include "mate-vfs-mime-magic.h"

#include <sys/types.h>

#include "mate-vfs-mime-sniff-buffer-private.h"
#include "mate-vfs-mime.h"
#include "mate-vfs-private-utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#include <wchar.h>
#endif

#include <glib.h>

enum {
	MATE_VFS_TEXT_SNIFF_LENGTH = 256
};


/**
 * _mate_vfs_sniff_buffer_looks_like_text:
 * @sniff_buffer: buffer to examine
 *
 * Return value: returns %TRUE if the contents of @sniff_buffer appear to
 * be text.
 **/
gboolean
_mate_vfs_sniff_buffer_looks_like_text (MateVFSMimeSniffBuffer *sniff_buffer)
{
	gchar *end;
	
	_mate_vfs_mime_sniff_buffer_get (sniff_buffer, MATE_VFS_TEXT_SNIFF_LENGTH);

	if (sniff_buffer->buffer_length == 0) {
		return TRUE;
	}

	/* Don't allow embedded zeros in textfiles. */
	if (memchr (sniff_buffer->buffer, 0, sniff_buffer->buffer_length) != NULL) {
		return FALSE;
	}
	
	if (g_utf8_validate ((gchar *)sniff_buffer->buffer, 
			     sniff_buffer->buffer_length, (const gchar**)&end))
	{
		return TRUE;
	} else {
		/* Check whether the string was truncated in the middle of
		 * a valid UTF8 char, or if we really have an invalid
		 * UTF8 string
     		 */
		gint remaining_bytes = sniff_buffer->buffer_length;

		remaining_bytes -= (end-((gchar *)sniff_buffer->buffer));

 		if (g_utf8_get_char_validated(end, remaining_bytes) == -2) {
			return TRUE;
		}
#if defined(HAVE_WCTYPE_H) && defined (HAVE_MBRTOWC)
		else {
			size_t wlen;
			wchar_t wc;
			guchar *src, *end;
			mbstate_t state;

			src = sniff_buffer->buffer;
			end = sniff_buffer->buffer + sniff_buffer->buffer_length;
			
			memset (&state, 0, sizeof (state));
			while (src < end) {
				wlen = mbrtowc(&wc, (gchar *)src, end - src, &state);

				if (wlen == (size_t)(-1)) {
					/* Illegal mb sequence */
					return FALSE;
				}
				
				if (wlen == (size_t)(-2)) {
					/* No complete mb char before end
					 * Probably a cut off char which is ok */
					return TRUE;
				}

				if (wlen == 0) {
					/* Don't allow embedded zeros in textfiles */
					return FALSE;
				}
				
				if (!iswspace (wc)  && !iswprint(wc)) {
					/* Not a printable or whitspace
					 * Probably not a text file */
					return FALSE;
				}

				src += wlen;
			}
			return TRUE;
		}
#endif /* defined(HAVE_WCTYPE_H) && defined (HAVE_MBRTOWC) */
	}
	return FALSE;
}

static const int bitrates[2][15] = {
	{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320},
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 }
};	

static const int frequencies[2][3] = {
	{ 44100, 48000, 32000 },
	{ 22050, 24000, 16000 }	
};	

/*
 * Return length of an MP3 frame using potential 32-bit header value.  See
 * "http://www.dv.co.yu/mpgscript/mpeghdr.htm" for details on the header
 * format.
 *
 * NOTE: As an optimization and because they are rare, this returns 0 for
 * version 2.5 or free format MP3s.
 */
static gsize
get_mp3_frame_length (unsigned long mp3_header)
{
	int ver = 4 - ((mp3_header >> 19) & 3u);
	int br = (mp3_header >> 12) & 0xfu;
	int srf = (mp3_header >> 10) & 3u;

	/* are frame sync and layer 3 bits set? */
	if (((mp3_header & 0xffe20000ul) == 0xffe20000ul)
		/* good version? */
		&& ((ver == 1) || (ver == 2))
		/* good bitrate index (not free or invalid)? */
		&& (br > 0) && (br < 15)
		/* good sampling rate frequency index? */
		&& (srf != 3)
		/* not using reserved emphasis value? */
		&& ((mp3_header & 3u) != 2)) {
		/* then this is most likely the beginning of a valid frame */

		gsize length = (gsize) bitrates[ver - 1][br] * 144000;
		length /= frequencies[ver - 1][srf];
		return length += ((mp3_header >> 9) & 1u) - 4;
	}
	return 0;
}

static unsigned long
get_4_byte_value (const unsigned char *bytes)
{
	unsigned long value = 0;
	int count;

	for (count = 0; count < 4; ++count) {
		value <<= 8;
		value |= *bytes++;
	}
	return value;
}

enum {
	MATE_VFS_MP3_SNIFF_LENGTH = 256
};

/**
 * _mate_vfs_sniff_buffer_looks_like_mp3:
 * @sniff_buffer: buffer to examine
 *
 * Return value: returns %TRUE if the contents of @sniff_buffer appear to
 * be an MP3.
 **/
gboolean
_mate_vfs_sniff_buffer_looks_like_mp3 (MateVFSMimeSniffBuffer *sniff_buffer)
{
	unsigned long mp3_header;
	int offset;
	
	if (_mate_vfs_mime_sniff_buffer_get (sniff_buffer, MATE_VFS_MP3_SNIFF_LENGTH) != MATE_VFS_OK) {
		return FALSE;
	}

	/*
	 * Use algorithm described in "ID3 tag version 2.3.0 Informal Standard"
	 * at "http://www.id3.org/id3v2.3.0.html" to detect a valid header, "An
	 * ID3v2 tag can be detected with the following pattern:
	 *      $49 44 33 yy yy xx zz zz zz zz
	 * Where yy is less than $FF, xx is the 'flags' byte and zz is less than
	 * $80."
	 *
	 * The informal standard also says, "The ID3v2 tag size is encoded with
	 * four bytes where the most significant bit (bit 7) is set to zero in
	 * every byte, making a total of 28 bits.  The zeroed bits are ignored,
	 * so a 257 bytes long tag is represented as $00 00 02 01."
	 */
	if (strncmp ((char *) sniff_buffer->buffer, "ID3", 3) == 0
		&& (sniff_buffer->buffer[3] != 0xffu)
		&& (sniff_buffer->buffer[4] != 0xffu)
		&& (sniff_buffer->buffer[6] < 0x80u)
		&& (sniff_buffer->buffer[7] < 0x80u)
		&& (sniff_buffer->buffer[8] < 0x80u)
		&& (sniff_buffer->buffer[9] < 0x80u)) {
		/* checks for existance of vorbis identification header */
		for (offset=10; offset < MATE_VFS_MP3_SNIFF_LENGTH-7; offset++) {
			if (strncmp ((char *) &sniff_buffer->buffer[offset], 
				     "\x01vorbis", 7) == 0) {
				return FALSE;
			}
		}
		return TRUE;
	}

	/*
	 * Scan through the first "MATE_VFS_MP3_SNIFF_LENGTH" bytes of the
	 * buffer to find a potential 32-bit MP3 frame header.
	 */
	mp3_header = 0;
	for (offset = 0; offset < MATE_VFS_MP3_SNIFF_LENGTH; offset++) {
		gsize length;

		mp3_header <<= 8;
		mp3_header |= sniff_buffer->buffer[offset];
		mp3_header &= 0xfffffffful;

		length = get_mp3_frame_length (mp3_header);

		if (length != 0) {
			/*
			 * Since one frame is available, is there another frame
			 * just to be sure this is more likely to be a real MP3
			 * buffer?
			 */
			offset += 1 + length;

			if (_mate_vfs_mime_sniff_buffer_get (sniff_buffer, offset + 4) != MATE_VFS_OK) {
				return FALSE;
			}
			mp3_header = get_4_byte_value (&sniff_buffer->buffer[offset]);
			length = get_mp3_frame_length (mp3_header);

			if (length != 0) {
				return TRUE;
			}
			break;
		}
	}

	return FALSE;
}
