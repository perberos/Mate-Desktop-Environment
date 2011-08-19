/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* pkcs11g.h - MATE internal definitions to PKCS#11

   Copyright (C) 2008, Stef Walter

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

#ifndef PKCS11I_H
#define PKCS11I_H

#include "pkcs11.h"
#include "pkcs11g.h"

/* Signifies that nobody is logged in */
#define CKU_NONE G_MAXULONG

#define CK_MATE_MAX_SLOT                           (0x000000FFUL)
#define CK_MATE_MAX_HANDLE                         (((CK_ULONG)-1UL) >> 10)

/* -------------------------------------------------------------------
 * OBJECT HASH
 */

#define CKA_MATE_INTERNAL_SHA1                      (CKA_MATE + 1000)


/* -------------------------------------------------------------------
 * APPLICATION
 */

/* Flag for CK_INFO when applications are supported */
#define CKF_G_APPLICATIONS                       0x40000000UL

/* Call C_OpenSession with this when passing CK_G_APPLICATION */
#define CKF_G_APPLICATION_SESSION                0x40000000UL

typedef CK_ULONG CK_G_APPLICATION_ID;

typedef struct CK_G_APPLICATION {
	CK_VOID_PTR applicationData;
	CK_G_APPLICATION_ID applicationId;
} CK_G_APPLICATION;

typedef CK_G_APPLICATION* CK_G_APPLICATION_PTR;

#define CKR_G_APPLICATION_ID_INVALID             (CKR_MATE + 10)


/* -------------------------------------------------------------------
 * SECRETS
 */

#define CKO_G_COLLECTION                     (CKO_MATE + 110)

#define CKK_G_SECRET_ITEM                    (CKK_MATE + 101)

#define CKO_G_SEARCH                         (CKO_MATE + 111)

#define CKA_G_LOCKED                         (CKA_MATE + 210)

#define CKA_G_CREATED                        (CKA_MATE + 211)

#define CKA_G_MODIFIED                       (CKA_MATE + 212)

#define CKA_G_FIELDS                         (CKA_MATE + 213)

#define CKA_G_COLLECTION                     (CKA_MATE + 214)

#define CKA_G_MATCHED                        (CKA_MATE + 215)

#define CKA_G_SCHEMA                         (CKA_MATE + 216)

#define CKA_G_LOGIN_COLLECTION               (CKA_MATE + 218)

/* -------------------------------------------------------------------
 * MECHANISMS
 */

/* Used for wrapping and unwrapping as null */
#define CKM_G_NULL                           (CKM_MATE + 100)

#define CKK_G_NULL                           (CKK_MATE + 100)

/* -------------------------------------------------------------------
 * AUTO DESTRUCT
 */

#define CKA_G_DESTRUCT_IDLE                  (CKA_MATE + 190)

#define CKA_G_DESTRUCT_AFTER                 (CKA_MATE + 191)

#define CKA_G_DESTRUCT_USES                  (CKA_MATE + 192)

/* -------------------------------------------------------------------
 * CREDENTIAL
 */

#define CKO_G_CREDENTIAL                         (CKO_MATE + 100)

#define CKA_G_OBJECT                             (CKA_MATE + 202)

#define CKA_G_CREDENTIAL                         (CKA_MATE + 204)

#define CKA_G_CREDENTIAL_TEMPLATE                (CKA_MATE + 205)

#endif /* PKCS11I_H */
