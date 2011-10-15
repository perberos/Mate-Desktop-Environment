/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-keyring.h - library for talking to the keyring daemon.

   Copyright (C) 2003 Red Hat, Inc

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef MATE_KEYRING_H
#define MATE_KEYRING_H

#include <glib.h>
#include <time.h>

#include "mate-keyring-result.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_KEYRING_SESSION   "session"
#define MATE_KEYRING_DEFAULT   NULL

typedef enum {

	/* The item types */
	MATE_KEYRING_ITEM_GENERIC_SECRET = 0,
	MATE_KEYRING_ITEM_NETWORK_PASSWORD,
	MATE_KEYRING_ITEM_NOTE,
	MATE_KEYRING_ITEM_CHAINED_KEYRING_PASSWORD,
	MATE_KEYRING_ITEM_ENCRYPTION_KEY_PASSWORD,

	MATE_KEYRING_ITEM_PK_STORAGE = 0x100,

	/* Not used, remains here only for compatibility */
	MATE_KEYRING_ITEM_LAST_TYPE,

} MateKeyringItemType;

#define	MATE_KEYRING_ITEM_TYPE_MASK 0x0000ffff
#define MATE_KEYRING_ITEM_NO_TYPE MATE_KEYRING_ITEM_TYPE_MASK
#define	MATE_KEYRING_ITEM_APPLICATION_SECRET 0x01000000

typedef enum {
	MATE_KEYRING_ACCESS_ASK,
	MATE_KEYRING_ACCESS_DENY,
	MATE_KEYRING_ACCESS_ALLOW
} MateKeyringAccessRestriction;

typedef enum {
	MATE_KEYRING_ATTRIBUTE_TYPE_STRING,
	MATE_KEYRING_ATTRIBUTE_TYPE_UINT32
} MateKeyringAttributeType;

typedef struct MateKeyringAccessControl MateKeyringAccessControl;
typedef struct MateKeyringApplicationRef MateKeyringApplicationRef;
typedef GArray MateKeyringAttributeList;

typedef enum {
	MATE_KEYRING_ACCESS_READ = 1<<0,
	MATE_KEYRING_ACCESS_WRITE = 1<<1,
	MATE_KEYRING_ACCESS_REMOVE = 1<<2
} MateKeyringAccessType;

typedef enum {
	MATE_KEYRING_ITEM_INFO_BASICS = 0,
	MATE_KEYRING_ITEM_INFO_SECRET = 1<<0
} MateKeyringItemInfoFlags;

/* Add flags here as they are added above */
#define MATE_KEYRING_ITEM_INFO_ALL (MATE_KEYRING_ITEM_INFO_BASICS | MATE_KEYRING_ITEM_INFO_SECRET)

typedef struct MateKeyringInfo MateKeyringInfo;
typedef struct MateKeyringItemInfo MateKeyringItemInfo;

typedef struct {
	char *name;
	MateKeyringAttributeType type;
	union {
		char *string;
		guint32 integer;
	} value;
} MateKeyringAttribute;

typedef struct {
	char *keyring;
	guint item_id;
	MateKeyringAttributeList *attributes;
	char *secret;
} MateKeyringFound;

void mate_keyring_string_list_free (GList *strings);

typedef void (*MateKeyringOperationDoneCallback)           (MateKeyringResult result,
                                                             gpointer           data);
typedef void (*MateKeyringOperationGetStringCallback)      (MateKeyringResult result,
                                                             const char        *string,
                                                             gpointer           data);
typedef void (*MateKeyringOperationGetIntCallback)         (MateKeyringResult result,
                                                             guint32            val,
                                                             gpointer           data);
typedef void (*MateKeyringOperationGetListCallback)        (MateKeyringResult result,
                                                             GList             *list,
                                                             gpointer           data);
typedef void (*MateKeyringOperationGetKeyringInfoCallback) (MateKeyringResult result,
                                                             MateKeyringInfo  *info,
                                                             gpointer           data);
typedef void (*MateKeyringOperationGetItemInfoCallback)    (MateKeyringResult   result,
                                                             MateKeyringItemInfo*info,
                                                             gpointer             data);
typedef void (*MateKeyringOperationGetAttributesCallback)  (MateKeyringResult   result,
                                                             MateKeyringAttributeList *attributes,
                                                             gpointer             data);

#define mate_keyring_attribute_list_index(a, i) g_array_index ((a), MateKeyringAttribute, (i))
#define mate_keyring_attribute_list_new() (g_array_new (FALSE, FALSE, sizeof (MateKeyringAttribute)))
void                       mate_keyring_attribute_list_append_string (MateKeyringAttributeList *attributes,
                                                                       const char                *name,
                                                                       const char                *value);
void                       mate_keyring_attribute_list_append_uint32 (MateKeyringAttributeList *attributes,
                                                                       const char                *name,
                                                                       guint32                    value);
void                       mate_keyring_attribute_list_free          (MateKeyringAttributeList *attributes);
MateKeyringAttributeList *mate_keyring_attribute_list_copy          (MateKeyringAttributeList *attributes);


const gchar*               mate_keyring_result_to_message            (MateKeyringResult res);

gboolean mate_keyring_is_available (void);

void mate_keyring_found_free (MateKeyringFound *found);
void mate_keyring_found_list_free (GList *found_list);

void mate_keyring_cancel_request (gpointer request);

gpointer           mate_keyring_set_default_keyring      (const char                              *keyring,
                                                           MateKeyringOperationDoneCallback        callback,
                                                           gpointer                                 data,
                                                           GDestroyNotify                           destroy_data);
MateKeyringResult mate_keyring_set_default_keyring_sync (const char                              *keyring);
gpointer           mate_keyring_get_default_keyring      (MateKeyringOperationGetStringCallback   callback,
                                                           gpointer                                 data,
                                                           GDestroyNotify                           destroy_data);
MateKeyringResult mate_keyring_get_default_keyring_sync (char                                   **keyring);
gpointer           mate_keyring_list_keyring_names       (MateKeyringOperationGetListCallback     callback,
                                                           gpointer                                 data,
                                                           GDestroyNotify                           destroy_data);
MateKeyringResult mate_keyring_list_keyring_names_sync  (GList                                  **keyrings);
gpointer           mate_keyring_lock_all                 (MateKeyringOperationDoneCallback        callback,
                                                           gpointer                                 data,
                                                           GDestroyNotify                           destroy_data);
MateKeyringResult mate_keyring_lock_all_sync            (void);


/* NULL password means ask user */
gpointer           mate_keyring_create             (const char                                   *keyring_name,
                                                     const char                                   *password,
                                                     MateKeyringOperationDoneCallback             callback,
                                                     gpointer                                      data,
                                                     GDestroyNotify                                destroy_data);
MateKeyringResult mate_keyring_create_sync        (const char                                   *keyring_name,
                                                     const char                                   *password);
gpointer           mate_keyring_unlock             (const char                                   *keyring,
                                                     const char                                   *password,
                                                     MateKeyringOperationDoneCallback             callback,
                                                     gpointer                                      data,
                                                     GDestroyNotify                                destroy_data);
MateKeyringResult mate_keyring_unlock_sync        (const char                                   *keyring,
                                                     const char                                   *password);
gpointer           mate_keyring_lock               (const char                                   *keyring,
                                                     MateKeyringOperationDoneCallback             callback,
                                                     gpointer                                      data,
                                                     GDestroyNotify                                destroy_data);
MateKeyringResult mate_keyring_lock_sync          (const char                                   *keyring);
gpointer           mate_keyring_delete             (const char                                   *keyring,
                                                     MateKeyringOperationDoneCallback             callback,
                                                     gpointer                                      data,
                                                     GDestroyNotify                                destroy_data);
MateKeyringResult mate_keyring_delete_sync        (const char                                   *keyring);
gpointer           mate_keyring_change_password             (const char                                   *keyring,
                                                     const char                                   *original,
                                                     const char                                   *password,
                                                     MateKeyringOperationDoneCallback             callback,
                                                     gpointer                                      data,
                                                     GDestroyNotify                                destroy_data);
MateKeyringResult mate_keyring_change_password_sync        (const char                                   *keyring,
                                                         const char                                                        	   *original,
                                                     const char                                   *password);
gpointer           mate_keyring_get_info           (const char                                   *keyring,
                                                     MateKeyringOperationGetKeyringInfoCallback   callback,
                                                     gpointer                                      data,
                                                     GDestroyNotify                                destroy_data);
MateKeyringResult mate_keyring_get_info_sync      (const char                                   *keyring,
                                                     MateKeyringInfo                            **info);
gpointer           mate_keyring_set_info           (const char                                   *keyring,
                                                     MateKeyringInfo                             *info,
                                                     MateKeyringOperationDoneCallback             callback,
                                                     gpointer                                      data,
                                                     GDestroyNotify                                destroy_data);
MateKeyringResult mate_keyring_set_info_sync      (const char                                   *keyring,
                                                     MateKeyringInfo                             *info);
gpointer           mate_keyring_list_item_ids      (const char                                   *keyring,
                                                     MateKeyringOperationGetListCallback          callback,
                                                     gpointer                                      data,
                                                     GDestroyNotify                                destroy_data);
MateKeyringResult mate_keyring_list_item_ids_sync (const char                                   *keyring,
                                                     GList                                       **ids);

void              mate_keyring_info_free             (MateKeyringInfo *keyring_info);
MateKeyringInfo *mate_keyring_info_copy             (MateKeyringInfo *keyring_info);
void              mate_keyring_info_set_lock_on_idle (MateKeyringInfo *keyring_info,
                                                       gboolean          value);
gboolean          mate_keyring_info_get_lock_on_idle (MateKeyringInfo *keyring_info);
void              mate_keyring_info_set_lock_timeout (MateKeyringInfo *keyring_info,
                                                       guint32           value);
guint32           mate_keyring_info_get_lock_timeout (MateKeyringInfo *keyring_info);
time_t            mate_keyring_info_get_mtime        (MateKeyringInfo *keyring_info);
time_t            mate_keyring_info_get_ctime        (MateKeyringInfo *keyring_info);
gboolean          mate_keyring_info_get_is_locked    (MateKeyringInfo *keyring_info);

gpointer mate_keyring_find_items  (MateKeyringItemType                  type,
                                    MateKeyringAttributeList            *attributes,
                                    MateKeyringOperationGetListCallback  callback,
                                    gpointer                              data,
                                    GDestroyNotify                        destroy_data);
gpointer mate_keyring_find_itemsv (MateKeyringItemType                  type,
                                    MateKeyringOperationGetListCallback  callback,
                                    gpointer                              data,
                                    GDestroyNotify                        destroy_data,
                                    ...);

MateKeyringResult mate_keyring_find_items_sync  (MateKeyringItemType        type,
                                                   MateKeyringAttributeList  *attributes,
                                                   GList                     **found);
MateKeyringResult mate_keyring_find_itemsv_sync (MateKeyringItemType        type,
                                                   GList                     **found,
                                                   ...);

gpointer           mate_keyring_item_create              (const char                                 *keyring,
                                                           MateKeyringItemType                        type,
                                                           const char                                 *display_name,
                                                           MateKeyringAttributeList                  *attributes,
                                                           const char                                 *secret,
                                                           gboolean                                    update_if_exists,
                                                           MateKeyringOperationGetIntCallback         callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_create_sync         (const char                                 *keyring,
                                                           MateKeyringItemType                        type,
                                                           const char                                 *display_name,
                                                           MateKeyringAttributeList                  *attributes,
                                                           const char                                 *secret,
                                                           gboolean                                    update_if_exists,
                                                           guint32                                    *item_id);
gpointer           mate_keyring_item_delete              (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringOperationDoneCallback           callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_delete_sync         (const char                                 *keyring,
                                                           guint32                                     id);
gpointer           mate_keyring_item_get_info            (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringOperationGetItemInfoCallback    callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_get_info_sync       (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringItemInfo                      **info);
gpointer           mate_keyring_item_get_info_full       (const char                                 *keyring,
                                                           guint32                                     id,
                                                           guint32                                     flags,
                                                           MateKeyringOperationGetItemInfoCallback    callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_get_info_full_sync  (const char                                 *keyring,
                                                           guint32                                     id,
                                                           guint32                                     flags,
                                                            MateKeyringItemInfo                      **info);
gpointer           mate_keyring_item_set_info            (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringItemInfo                       *info,
                                                           MateKeyringOperationDoneCallback           callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_set_info_sync       (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringItemInfo                       *info);
gpointer           mate_keyring_item_get_attributes      (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringOperationGetAttributesCallback  callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_get_attributes_sync (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringAttributeList                 **attributes);
gpointer           mate_keyring_item_set_attributes      (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringAttributeList                  *attributes,
                                                           MateKeyringOperationDoneCallback           callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_set_attributes_sync (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringAttributeList                  *attributes);
gpointer           mate_keyring_item_get_acl             (const char                                 *keyring,
                                                           guint32                                     id,
                                                           MateKeyringOperationGetListCallback        callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_get_acl_sync        (const char                                 *keyring,
                                                           guint32                                     id,
                                                           GList                                     **acl);
gpointer           mate_keyring_item_set_acl             (const char                                 *keyring,
                                                           guint32                                     id,
                                                           GList                                      *acl,
                                                           MateKeyringOperationDoneCallback           callback,
                                                           gpointer                                    data,
                                                           GDestroyNotify                              destroy_data);
MateKeyringResult mate_keyring_item_set_acl_sync        (const char                                 *keyring,
                                                           guint32                                     id,
                                                           GList                                      *acl);

gpointer           mate_keyring_item_grant_access_rights      (const gchar                       *keyring,
                                                                const gchar                       *display_name,
                                                                const gchar                       *full_path,
                                                                const guint32                      id,
                                                                const MateKeyringAccessType       rights,
                                                                MateKeyringOperationDoneCallback  callback,
                                                                gpointer                           data,
                                                                GDestroyNotify                     destroy_data);

MateKeyringResult mate_keyring_item_grant_access_rights_sync (const char                   *keyring,
                                                                const char                   *display_name,
                                                                const char                   *full_path,
                                                                const guint32                id,
                                                                const MateKeyringAccessType rights);

void                  mate_keyring_item_info_free             (MateKeyringItemInfo *item_info);
MateKeyringItemInfo *mate_keyring_item_info_new              (void);
MateKeyringItemInfo *mate_keyring_item_info_copy             (MateKeyringItemInfo *item_info);
MateKeyringItemType  mate_keyring_item_info_get_type         (MateKeyringItemInfo *item_info);
void                  mate_keyring_item_info_set_type         (MateKeyringItemInfo *item_info,
                                                                MateKeyringItemType  type);
char *                mate_keyring_item_info_get_secret       (MateKeyringItemInfo *item_info);
void                  mate_keyring_item_info_set_secret       (MateKeyringItemInfo *item_info,
                                                                const char           *value);
char *                mate_keyring_item_info_get_display_name (MateKeyringItemInfo *item_info);
void                  mate_keyring_item_info_set_display_name (MateKeyringItemInfo *item_info,
                                                                const char           *value);
time_t                mate_keyring_item_info_get_mtime        (MateKeyringItemInfo *item_info);
time_t                mate_keyring_item_info_get_ctime        (MateKeyringItemInfo *item_info);

MateKeyringApplicationRef * mate_keyring_application_ref_new          (void);
MateKeyringApplicationRef * mate_keyring_application_ref_copy         (const MateKeyringApplicationRef *app);
void                         mate_keyring_application_ref_free         (MateKeyringApplicationRef       *app);

MateKeyringAccessControl *  mate_keyring_access_control_new  (const MateKeyringApplicationRef *application,
                                                                MateKeyringAccessType            types_allowed);
MateKeyringAccessControl *  mate_keyring_access_control_copy (MateKeyringAccessControl        *ac);


void    mate_keyring_access_control_free (MateKeyringAccessControl *ac);
GList * mate_keyring_acl_copy            (GList                     *list);
void    mate_keyring_acl_free            (GList                     *acl);


char *                mate_keyring_item_ac_get_display_name   (MateKeyringAccessControl *ac);
void                  mate_keyring_item_ac_set_display_name   (MateKeyringAccessControl *ac,
                                                                const char           *value);

char *                mate_keyring_item_ac_get_path_name      (MateKeyringAccessControl *ac);
void                  mate_keyring_item_ac_set_path_name      (MateKeyringAccessControl *ac,
                                                                const char           *value);


MateKeyringAccessType mate_keyring_item_ac_get_access_type   (MateKeyringAccessControl *ac);
void                   mate_keyring_item_ac_set_access_type   (MateKeyringAccessControl *ac,
                                                                const MateKeyringAccessType value);

/* ------------------------------------------------------------------------------
 * A Simpler API
 */

typedef struct {
	MateKeyringItemType item_type;
	struct {
		const gchar* name;
		MateKeyringAttributeType type;
	} attributes[32];

	/* <private> */
	gpointer reserved1;
	gpointer reserved2;
	gpointer reserved3;
} MateKeyringPasswordSchema;

extern const MateKeyringPasswordSchema* MATE_KEYRING_NETWORK_PASSWORD;

gpointer                 mate_keyring_store_password         (const MateKeyringPasswordSchema* schema,
                                                               const gchar *keyring,
                                                               const gchar *display_name,
                                                               const gchar *password,
                                                               MateKeyringOperationDoneCallback callback,
                                                               gpointer data,
                                                               GDestroyNotify destroy_data,
                                                               ...) G_GNUC_NULL_TERMINATED;

MateKeyringResult       mate_keyring_store_password_sync    (const MateKeyringPasswordSchema* schema,
                                                               const gchar *keyring,
                                                               const gchar *display_name,
                                                               const gchar *password,
                                                               ...) G_GNUC_NULL_TERMINATED;

gpointer                 mate_keyring_find_password          (const MateKeyringPasswordSchema* schema,
                                                               MateKeyringOperationGetStringCallback callback,
                                                               gpointer data,
                                                               GDestroyNotify destroy_data,
                                                               ...) G_GNUC_NULL_TERMINATED;

MateKeyringResult       mate_keyring_find_password_sync     (const MateKeyringPasswordSchema* schema,
                                                               gchar **password,
                                                               ...) G_GNUC_NULL_TERMINATED;

gpointer                 mate_keyring_delete_password        (const MateKeyringPasswordSchema* schema,
                                                               MateKeyringOperationDoneCallback callback,
                                                               gpointer data,
                                                               GDestroyNotify destroy_data,
                                                               ...) G_GNUC_NULL_TERMINATED;

MateKeyringResult       mate_keyring_delete_password_sync   (const MateKeyringPasswordSchema* schema,
                                                               ...) G_GNUC_NULL_TERMINATED;

void                     mate_keyring_free_password          (gchar *password);

/* ------------------------------------------------------------------------------
 * Special Helpers for network password items
 */

typedef struct {
	char *keyring;
	guint32 item_id;

	char *protocol;
	char *server;
	char *object;
	char *authtype;
	guint32 port;

	char *user;
	char *domain;
	char *password;
} MateKeyringNetworkPasswordData;

void mate_keyring_network_password_free (MateKeyringNetworkPasswordData *data);
void mate_keyring_network_password_list_free (GList *list);

gpointer           mate_keyring_find_network_password      (const char                            *user,
                                                             const char                            *domain,
                                                             const char                            *server,
                                                             const char                            *object,
                                                             const char                            *protocol,
                                                             const char                            *authtype,
                                                             guint32                                port,
                                                             MateKeyringOperationGetListCallback   callback,
                                                             gpointer                               data,
                                                             GDestroyNotify                         destroy_data);
MateKeyringResult mate_keyring_find_network_password_sync (const char                            *user,
                                                             const char                            *domain,
                                                             const char                            *server,
                                                             const char                            *object,
                                                             const char                            *protocol,
                                                             const char                            *authtype,
                                                             guint32                                port,
                                                             GList                                **results);
gpointer           mate_keyring_set_network_password       (const char                            *keyring,
                                                             const char                            *user,
                                                             const char                            *domain,
                                                             const char                            *server,
                                                             const char                            *object,
                                                             const char                            *protocol,
                                                             const char                            *authtype,
                                                             guint32                                port,
                                                             const char                            *password,
                                                             MateKeyringOperationGetIntCallback    callback,
                                                             gpointer                               data,
                                                             GDestroyNotify                         destroy_data);
MateKeyringResult mate_keyring_set_network_password_sync  (const char                            *keyring,
                                                             const char                            *user,
                                                             const char                            *domain,
                                                             const char                            *server,
                                                             const char                            *object,
                                                             const char                            *protocol,
                                                             const char                            *authtype,
                                                             guint32                                port,
                                                             const char                            *password,
                                                             guint32                               *item_id);

/* -----------------------------------------------------------------------------
 * USED ONLY BY THE SESSION
 */

/* Deprecated */
MateKeyringResult    mate_keyring_daemon_set_display_sync         (const char *display);

MateKeyringResult    mate_keyring_daemon_prepare_environment_sync (void);

#ifdef __cplusplus
}
#endif

#endif /* MATE_KEYRING_H */
