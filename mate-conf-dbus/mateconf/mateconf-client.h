/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* MateConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
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

#ifndef MATECONF_MATECONF_CLIENT_H
#define MATECONF_MATECONF_CLIENT_H

#include <glib-object.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-listeners.h>
#include <mateconf/mateconf-changeset.h>

G_BEGIN_DECLS

/*
 * This is a wrapper for the client-side MateConf API which provides several
 * convenient features.
 *
 *  - It (recursively) caches the contents of certain directories on
 *    the client side, such as your application's configuration
 *    directory
 *
 *  - It allows you to register per-key callbacks within these directories,
 *    without having to register multiple server-side callbacks
 *    (mateconf_engine_notify_add() adds a request-for-notify to the server,
 *    this wrapper adds a notify to the server for the whole directory
 *    and keeps your per-key notify requests on the client side).
 *
 *  - It has some error-handling features
 *
 * This class is heavily specialized for per-user desktop applications -
 * even more so than MateConf itself.
 */

/*
 * IMPORTANT: you can't mix MateConfClient with direct MateConfEngine access,
 * or you will have a mess because the client won't know what you're doing
 * underneath it.
 */

typedef enum { /*< prefix=MATECONF_CLIENT >*/
  MATECONF_CLIENT_PRELOAD_NONE,     /* don't preload anything */
  MATECONF_CLIENT_PRELOAD_ONELEVEL, /* load entries directly under the directory. */
  MATECONF_CLIENT_PRELOAD_RECURSIVE /* recurse the directory tree; possibly quite expensive! */
} MateConfClientPreloadType;

typedef enum { /*< prefix=MATECONF_CLIENT >*/
  MATECONF_CLIENT_HANDLE_NONE,
  MATECONF_CLIENT_HANDLE_UNRETURNED,
  MATECONF_CLIENT_HANDLE_ALL
} MateConfClientErrorHandlingMode;


typedef struct _MateConfClient       MateConfClient;
typedef struct _MateConfClientClass  MateConfClientClass;


typedef void (*MateConfClientNotifyFunc)(MateConfClient* client,
                                      guint cnxn_id,
                                      MateConfEntry *entry,
                                      gpointer user_data);

typedef void (*MateConfClientErrorHandlerFunc) (MateConfClient* client,
                                             GError* error);

#define MATECONF_TYPE_CLIENT                  (mateconf_client_get_type ())
#define MATECONF_CLIENT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECONF_TYPE_CLIENT, MateConfClient))
#define MATECONF_CLIENT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), MATECONF_TYPE_CLIENT, MateConfClientClass))
#define MATECONF_IS_CLIENT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECONF_TYPE_CLIENT))
#define MATECONF_IS_CLIENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECONF_TYPE_CLIENT))
#define MATECONF_CLIENT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECONF_TYPE_CLIENT, MateConfClientClass))

struct _MateConfClient
{
  GObject object;

  /*< private >*/

  MateConfEngine* engine;
  MateConfClientErrorHandlingMode error_mode;
  GHashTable* dir_hash;
  GHashTable* cache_hash;
  MateConfListeners* listeners;
  GSList *notify_list;
  guint notify_handler;
  int pending_notify_count;
  gpointer pad1;
  int pad2;  
};

struct _MateConfClientClass
{
  GObjectClass parent_class;

  /* emitted whenever a value changes. Often, you should use a notify
     function instead; the problem with this signal is that you
     probably have to do an expensive chain of strcmp() to
     determine how to respond to it.
  */

  void (* value_changed) (MateConfClient* client,
                          const gchar* key,
                          MateConfValue* value);

  /* General note about error handling: AVOID DIALOG DELUGES.
     That is, if lots of errors could happen in a row you need
     to collect those and put them in _one_ dialog, maybe using
     an idle function. mateconf_client_set_error_handling()
     is provided and it does this using MateDialog.  */

  /* emitted when you pass NULL for the error return location to a
     MateConfClient function and an error occurs. This allows you to
     ignore errors when your generic handler will work, and handle
     them specifically when you need to */
  void (* unreturned_error) (MateConfClient* client,
                             GError* error);

  /* emitted unconditionally anytime there's an error, whether you ask
     for that error or not. Useful for creating an error log or
     something. */
  void (* error)            (MateConfClient* client,
                             GError* error);

  GFunc pad1;
  GFunc pad2;
  GFunc pad3;
};


GType             mateconf_client_get_type        (void);

/* use the default engine */
MateConfClient*      mateconf_client_get_default             (void);

/* refcount on engine is incremented, you still own your ref */
MateConfClient*      mateconf_client_get_for_engine (MateConfEngine* engine);

/* Add a directory to monitor and emit the value_changed signal and
   key notifications for.  Optionally pre-load the contents of this
   directory, much faster if you plan to access most of the directory
   contents.
*/

void              mateconf_client_add_dir     (MateConfClient* client,
                                            const gchar* dir,
                                            MateConfClientPreloadType preload,
                                            GError** err);


/* This removes any notifications associated with the dir */
void              mateconf_client_remove_dir  (MateConfClient* client,
                                            const gchar* dir,
                                            GError** err);

/*
 *  The notification facility allows you to attach a callback to a single
 *  key or directory, which is more convenient most of the time than
 *  the value_changed signal. The key you're monitoring must be inside one
 *  of the MateConfClient's directories.
 */


/* Returns ID of the notification */
/* returns 0 on error, 0 is an invalid ID */
guint        mateconf_client_notify_add(MateConfClient* client,
                                     const gchar* namespace_section, /* dir or key to listen to */
                                     MateConfClientNotifyFunc func,
                                     gpointer user_data,
                                     GFreeFunc destroy_notify,
                                     GError** err);

void         mateconf_client_notify_remove  (MateConfClient* client,
                                          guint cnxn);
void         mateconf_client_notify (MateConfClient* client, const char* key);

/*
 * Error handling convenience; if you don't want the default handler,
 * set the error handling to MATECONF_CLIENT_HANDLE_NONE
 */

/* 
 * Error handling happens in the default signal handler, so you can
 * selectively override the default handling by connecting to the error
 * signal and calling g_signal_stop_emission()
 */

void              mateconf_client_set_error_handling(MateConfClient* client,
                                                  MateConfClientErrorHandlingMode mode);


/* Intended for use by mate-libs */
void              mateconf_client_set_global_default_error_handler(MateConfClientErrorHandlerFunc func);

/*
 * If you know you're done reading values for a while,
 * you can blow away the cache. Note that this nullifies the effect of
 * any preloading you may have done. However it frees some memory.
 */
void              mateconf_client_clear_cache(MateConfClient* client);

/*
 * Preload a directory; the directory must have been added already.
 * This is only useful as an optimization if you clear the cache,
 * then later want to do a lot of reads again. It's not useful
 * unless you clear the cache, because you can preload when you
 * call mateconf_client_add_dir()
 */
void              mateconf_client_preload    (MateConfClient* client,
                                           const gchar* dirname,
                                           MateConfClientPreloadType type,
                                           GError** err);

/*
 * Basic key-manipulation facilities; these keys do _not_ have to be in the
 * client's directory list, but they won't be cached unless they are.
 */

void              mateconf_client_set             (MateConfClient* client,
                                                const gchar* key,
                                                const MateConfValue* val,
                                                GError** err);

MateConfValue*       mateconf_client_get             (MateConfClient* client,
                                                const gchar* key,
                                                GError** err);

MateConfValue*       mateconf_client_get_without_default  (MateConfClient* client,
                                                     const gchar* key,
                                                     GError** err);

MateConfEntry*       mateconf_client_get_entry        (MateConfClient* client,
                                                 const gchar* key,
                                                 const gchar* locale,
                                                 gboolean use_schema_default,
                                                 GError** err);

MateConfValue*       mateconf_client_get_default_from_schema (MateConfClient* client,
                                                        const gchar* key,
                                                        GError** err);

gboolean     mateconf_client_unset          (MateConfClient* client,
                                          const gchar* key, GError** err);

gboolean     mateconf_client_recursive_unset (MateConfClient *client,
                                           const char     *key,
                                           MateConfUnsetFlags flags,
                                           GError        **err);

GSList*      mateconf_client_all_entries    (MateConfClient* client,
                                          const gchar* dir, GError** err);

GSList*      mateconf_client_all_dirs       (MateConfClient* client,
                                          const gchar* dir, GError** err);

void         mateconf_client_suggest_sync   (MateConfClient* client,
                                          GError** err);

gboolean     mateconf_client_dir_exists     (MateConfClient* client,
                                          const gchar* dir, GError** err);

gboolean     mateconf_client_key_is_writable(MateConfClient* client,
                                          const gchar* key,
                                          GError**     err);

/* Get/Set convenience wrappers */

gdouble      mateconf_client_get_float (MateConfClient* client, const gchar* key,
                                     GError** err);

gint         mateconf_client_get_int   (MateConfClient* client, const gchar* key,
                                     GError** err);

/* free the retval, if non-NULL */
gchar*       mateconf_client_get_string(MateConfClient* client, const gchar* key,
                                     GError** err);

gboolean     mateconf_client_get_bool  (MateConfClient* client, const gchar* key,
                                     GError** err);

/* this one has no default since it would be expensive and make little
   sense; it returns NULL as a default, to indicate unset or error */
/* free the retval */
/* Note that this returns the schema stored at key, NOT
   the schema that key conforms to. */
MateConfSchema* mateconf_client_get_schema  (MateConfClient* client,
                                       const gchar* key, GError** err);

GSList*      mateconf_client_get_list    (MateConfClient* client, const gchar* key,
                                       MateConfValueType list_type, GError** err);

gboolean     mateconf_client_get_pair    (MateConfClient* client, const gchar* key,
                                       MateConfValueType car_type, MateConfValueType cdr_type,
                                       gpointer car_retloc, gpointer cdr_retloc,
                                       GError** err);

/* No convenience functions for lists or pairs, since there are too
   many combinations of types possible
*/

/* setters return TRUE on success; note that you still should suggest a sync */

gboolean     mateconf_client_set_float   (MateConfClient* client, const gchar* key,
                                       gdouble val, GError** err);

gboolean     mateconf_client_set_int     (MateConfClient* client, const gchar* key,
                                       gint val, GError** err);

gboolean     mateconf_client_set_string  (MateConfClient* client, const gchar* key,
                                       const gchar* val, GError** err);

gboolean     mateconf_client_set_bool    (MateConfClient* client, const gchar* key,
                                       gboolean val, GError** err);

gboolean     mateconf_client_set_schema  (MateConfClient* client, const gchar* key,
                                       const MateConfSchema* val, GError** err);

/* List should be the same as the one mateconf_client_get_list() would return */
gboolean     mateconf_client_set_list    (MateConfClient* client, const gchar* key,
                                       MateConfValueType list_type,
                                       GSList* list,
                                       GError** err);

gboolean     mateconf_client_set_pair    (MateConfClient* client, const gchar* key,
                                       MateConfValueType car_type, MateConfValueType cdr_type,
                                       gconstpointer address_of_car,
                                       gconstpointer address_of_cdr,
                                       GError** err);

/*
 * Functions to emit signals
 */

/* these are useful to manually invoke the default error handlers */
void         mateconf_client_error                  (MateConfClient* client, GError* error);
void         mateconf_client_unreturned_error       (MateConfClient* client, GError* error);

/* useful to force an update */
void         mateconf_client_value_changed          (MateConfClient* client,
                                                  const gchar* key,
                                                  MateConfValue* value);

/*
 * Change set stuff
 */

gboolean        mateconf_client_commit_change_set   (MateConfClient* client,
                                                  MateConfChangeSet* cs,
                                                  /* remove all
                                                     successfully
                                                     committed changes
                                                     from the set */
                                                  gboolean remove_committed,
                                                  GError** err);

/* Create a change set that would revert the given change set
   for the given MateConfClient */
MateConfChangeSet* mateconf_client_reverse_change_set  (MateConfClient* client,
                                                         MateConfChangeSet* cs,
                                                         GError** err);

MateConfChangeSet* mateconf_client_change_set_from_currentv (MateConfClient* client,
                                                              const gchar** keys,
                                                              GError** err);

MateConfChangeSet* mateconf_client_change_set_from_current (MateConfClient* client,
                                                             GError** err,
                                                             const gchar* first_key,
                                                             ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif



