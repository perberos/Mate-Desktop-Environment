/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-keyring-result.h - Result codes from Mate Keyring

   Copyright (C) 2007 Stefan Walter

   The Mate Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Stef Walter <stef@memberwebs.com>
*/

#ifndef MATE_KEYRING_RESULT_H
#define MATE_KEYRING_RESULT_H

typedef enum {
	MATE_KEYRING_RESULT_OK,
	MATE_KEYRING_RESULT_DENIED,
	MATE_KEYRING_RESULT_NO_KEYRING_DAEMON,
	MATE_KEYRING_RESULT_ALREADY_UNLOCKED,
	MATE_KEYRING_RESULT_NO_SUCH_KEYRING,
	MATE_KEYRING_RESULT_BAD_ARGUMENTS,
	MATE_KEYRING_RESULT_IO_ERROR,
	MATE_KEYRING_RESULT_CANCELLED,
	MATE_KEYRING_RESULT_KEYRING_ALREADY_EXISTS,
	MATE_KEYRING_RESULT_NO_MATCH
} MateKeyringResult;

#define MATE_KEYRING_RESULT_ALREADY_EXISTS \
	MATE_KEYRING_RESULT_KEYRING_ALREADY_EXISTS

#endif /* MATE_KEYRING_RESULT_H */
