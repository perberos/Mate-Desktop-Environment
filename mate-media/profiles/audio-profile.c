/* audio-profile.c: */

/*
 * Copyright (C) 2003 Thomas Vander Stichele
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

#include <string.h>
#include <glib/gi18n.h>
#include <gst/gst.h>

#include "gmp-util.h"
#include "audio-profile.h"
#include "audio-profile-private.h"

#define KEY_NAME "name"
#define KEY_DESCRIPTION "description"
#define KEY_PIPELINE "pipeline"
#define KEY_EXTENSION "extension"
#define KEY_ACTIVE "active"

struct _GMAudioProfilePrivate
{
  char *id;		     /* the MateConf dir name */
  char *profile_dir;         /* full path in MateConf to this profile */
  MateConfClient *conf;
  guint notify_id;

  int in_notification_count; /* don't understand, see terminal-profile.c */
  char *name;                /* human-readable short name */
  char *description;         /* longer description of profile */
  char *pipeline;            /* GStreamer pipeline to be used */
  char *extension;           /* default file extension for this format */
  guint active : 1;
  guint forgotten : 1;

  GMAudioSettingMask locked;
};

static GHashTable *profiles = NULL;
static MateConfClient *_conf = NULL;

#define RETURN_IF_NOTIFYING(profile) if ((profile)->priv->in_notification_count) return

enum {
  CHANGED,
  FORGOTTEN,
  LAST_SIGNAL
};

static void gm_audio_profile_finalize    (GObject              *object);

static void gm_audio_profile_update      (GMAudioProfile *profile);

static void profile_change_notify     (MateConfClient *client,
                                       guint        cnxn_id,
                                       MateConfEntry  *entry,
                                       gpointer     user_data);

static void emit_changed (GMAudioProfile           *profile,
                          const GMAudioSettingMask *mask);


static gpointer parent_class;
static guint signals[LAST_SIGNAL] = { 0 };


static gpointer parent_class;

G_DEFINE_TYPE (GMAudioProfile, gm_audio_profile, G_TYPE_OBJECT)

static void
gm_audio_profile_init (GMAudioProfile *self)
{
  g_return_if_fail (profiles != NULL);

  self->priv = g_new0 (GMAudioProfilePrivate, 1);
  self->priv->name = g_strdup (_("<no name>"));
  self->priv->description = g_strdup (_("<no description>"));
  self->priv->pipeline = g_strdup ("identity");
  self->priv->extension = g_strdup ("wav");
}

static void
gm_audio_profile_class_init (GMAudioProfileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gm_audio_profile_finalize;

  signals[CHANGED] =
    g_signal_new ("changed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GMAudioProfileClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  signals[FORGOTTEN] =
    g_signal_new ("forgotten",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GMAudioProfileClass, forgotten),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
gm_audio_profile_finalize (GObject *object)
{
  GMAudioProfile *self;

  self = GM_AUDIO_PROFILE (object);

  gm_audio_profile_forget (self);

  mateconf_client_notify_remove (self->priv->conf,
                              self->priv->notify_id);
  self->priv->notify_id = 0;

  g_object_unref (G_OBJECT (self->priv->conf));

  g_free (self->priv->name);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*
 * internal stuff to manage profiles
 */

/* sync gm_audio profiles list by either using the given list as the new list
 * or by getting the list from MateConf
 */

static GList*
find_profile_link (GList      *profiles,
                   const char *id)
{
  GList *tmp;

  tmp = profiles;
  while (tmp != NULL)
    {
      if (strcmp (gm_audio_profile_get_id (GM_AUDIO_PROFILE (tmp->data)),
                  id) == 0)
        return tmp;

      tmp = tmp->next;
    }

  return NULL;
}

/* synchronize global profiles hash through accessor functions
 * if use_this_list is true, then put given profiles to the hash
 * if it's false, then get list from MateConf */
void
gm_audio_profile_sync_list (gboolean use_this_list,
                         GSList  *this_list)
{
  GList *known;
  GSList *updated;
  GList *tmp_list;
  GSList *tmp_slist;
  GError *err;
  gboolean need_new_default;
  GMAudioProfile *fallback;

  GST_DEBUG ("sync_list: start\n");
  if (use_this_list)
    GST_DEBUG ("Using given list of length %d\n", g_slist_length (this_list));
  else
    GST_DEBUG ("using list from mateconf\n");
  known = gm_audio_profile_get_list ();
    GST_DEBUG ("list of known profiles: size %d\n", g_list_length (known));

  if (use_this_list)
    {
      updated = g_slist_copy (this_list);
    }
  else
    {
      err = NULL;
      updated = mateconf_client_get_list (_conf,
                                       CONF_GLOBAL_PREFIX"/profile_list",
                                       MATECONF_VALUE_STRING,
                                       &err);
      if (err)
        {           g_printerr (_("There was an error getting the list of gm_audio profiles. (%s)\n"),
                      err->message);
          g_error_free (err);
        }
    }

  GST_DEBUG ("updated: slist of %d items\n", g_slist_length (updated));
  /* Add any new ones; ie go through updated and if any of them isn't in
   * the hash yet, add it.  If it is in the list of known profiles,  remove
   * it from our copy of that list. */
  tmp_slist = updated;
  while (tmp_slist != NULL)
    {
      GList *link;

      link = find_profile_link (known, tmp_slist->data);

      if (link)
        {
          /* make known point to profiles we didn't find in the list */
          GST_DEBUG ("id %s found in known profiles list, deleting from known\n",
                    (char *) tmp_slist->data);
          known = g_list_delete_link (known, link);
        }
      else
        {
          GMAudioProfile *profile;

          GST_DEBUG ("adding new profile with id %s to global hash\n",
                   (const char *) tmp_slist->data);
          profile = gm_audio_profile_new (tmp_slist->data, _conf);

          gm_audio_profile_update (profile);
        }

      if (!use_this_list)
        g_free (tmp_slist->data);

      tmp_slist = tmp_slist->next;
    }

  g_slist_free (updated);

  fallback = NULL;

    /* Forget no-longer-existing profiles */
  need_new_default = FALSE;
  tmp_list = known;
  while (tmp_list != NULL)
    {
      GMAudioProfile *forgotten;

      forgotten = GM_AUDIO_PROFILE (tmp_list->data);

      GST_DEBUG ("sync_list: forgetting profile with id %s\n",
               gm_audio_profile_get_id (forgotten));
      gm_audio_profile_forget (forgotten);

      tmp_list = tmp_list->next;
    }

  g_list_free (known);
  GST_DEBUG ("sync_list: stop\n");

  //FIXME: g_assert (terminal_profile_get_count () > 0);
}

/*
 * external API functions
 */

/* create a new GMAudioProfile structure and add it to the global profiles hash
 * load settings from MateConf tree
 */
GMAudioProfile*
gm_audio_profile_new (const char *id, MateConfClient *conf)
{
  GMAudioProfile *self;
  GError *err;

  GST_DEBUG ("creating new GMAudioProfile for id %s\n", id);
  g_return_val_if_fail (profiles != NULL, NULL);
  g_return_val_if_fail (gm_audio_profile_lookup (id) == NULL, NULL);

  self = g_object_new (GM_AUDIO_TYPE_PROFILE, NULL);

  self->priv->conf = conf;
  g_object_ref (G_OBJECT (conf));

  self->priv->id = g_strdup (id);
  self->priv->profile_dir = mateconf_concat_dir_and_key (CONF_PROFILES_PREFIX,
                                                         self->priv->id);

  err = NULL;
  GST_DEBUG ("loading config from MateConf dir %s\n",
           self->priv->profile_dir);
  mateconf_client_add_dir (conf, self->priv->profile_dir,
                        MATECONF_CLIENT_PRELOAD_ONELEVEL,
                        &err);
  if (err)
    {
      g_printerr ("There was an error loading config from %s. (%s)\n",
                    self->priv->profile_dir, err->message);
      g_error_free (err);
    }

  err = NULL;
  GST_DEBUG ("adding notify for MateConf profile\n");
  self->priv->notify_id =
    mateconf_client_notify_add (conf,
                             self->priv->profile_dir,
                             profile_change_notify,
                             self,
                             NULL, &err);

  if (err)
    {
      g_printerr ("There was an error subscribing to notification of gm_audio profile changes. (%s)\n",
                  err->message);
      g_error_free (err);
    }

  GST_DEBUG ("inserting in hash table done\n");
  g_hash_table_insert (profiles, self->priv->id, self);
  GST_DEBUG ("audio_profile_new done\n");

  return self;
}

/*
 * public profile getters and setters
 */

const char*
gm_audio_profile_get_id (GMAudioProfile *self)
{
  return self->priv->id;
}

const char*
gm_audio_profile_get_name (GMAudioProfile *self)
{
  return self->priv->name;
}

void
gm_audio_profile_set_name (GMAudioProfile *self,
                        const char      *name)
{
  char *key;

  RETURN_IF_NOTIFYING (self);

  key = mateconf_concat_dir_and_key (self->priv->profile_dir,
                                  KEY_NAME);

  mateconf_client_set_string (self->priv->conf,
                           key,
                           name,
                           NULL);

  g_free (key);
}

const char*
gm_audio_profile_get_description (GMAudioProfile *self)
{
  return self->priv->description;
}

void
gm_audio_profile_set_description (GMAudioProfile *self,
                               const char   *description)
{
  char *key;

  RETURN_IF_NOTIFYING (self);

  key = mateconf_concat_dir_and_key (self->priv->profile_dir,
                                  KEY_DESCRIPTION);

  mateconf_client_set_string (self->priv->conf,
                           key,
                           description,
                           NULL);

  g_free (key);
}

const char*
gm_audio_profile_get_pipeline (GMAudioProfile *self)
{
  return self->priv->pipeline;
}

void
gm_audio_profile_set_pipeline (GMAudioProfile *self,
                            const char   *pipeline)
{
  char *key;

  RETURN_IF_NOTIFYING (self);

  key = mateconf_concat_dir_and_key (self->priv->profile_dir,
                                  KEY_PIPELINE);

  mateconf_client_set_string (self->priv->conf,
                           key,
                           pipeline,
                           NULL);

  g_free (key);
}

const char*
gm_audio_profile_get_extension (GMAudioProfile *self)
{
  return self->priv->extension;
}

void
gm_audio_profile_set_extension (GMAudioProfile *self,
                               const char   *extension)
{
  char *key;

  RETURN_IF_NOTIFYING (self);

  key = mateconf_concat_dir_and_key (self->priv->profile_dir,
                                  KEY_EXTENSION);

  mateconf_client_set_string (self->priv->conf,
                           key,
                           extension,
                           NULL);

  g_free (key);
}

gboolean
gm_audio_profile_get_active (GMAudioProfile *self)
{
  return self->priv->active;
}

void
gm_audio_profile_set_active (GMAudioProfile *self,
                          gboolean active)
{
  char *key;

  RETURN_IF_NOTIFYING (self);

  key = mateconf_concat_dir_and_key (self->priv->profile_dir,
                                  KEY_ACTIVE);

  mateconf_client_set_bool (self->priv->conf,
                         key,
                         active,
                         NULL);

  g_free (key);
}
/*
 * private setters
 */

static gboolean
set_name (GMAudioProfile *self,
          const char *candidate_name)
{
  /* don't update if it's the same as the old one */
  if (candidate_name &&
      strcmp (self->priv->name, candidate_name) == 0)
    return FALSE;

  if (candidate_name != NULL)
    {
      g_free (self->priv->name);
      self->priv->name = g_strdup (candidate_name);
      return TRUE;
    }
  /* otherwise just leave the old name */

  return FALSE;
}

static gboolean
set_description (GMAudioProfile *self,
                 const char *candidate_description)
{
  /* don't update if it's the same as the old one */
  if (candidate_description &&
      strcmp (self->priv->description, candidate_description) == 0)
    return FALSE;

  if (candidate_description != NULL)
    {
      g_free (self->priv->description);
      self->priv->description = g_strdup (candidate_description);
      return TRUE;
    }
  /* otherwise just leave the old description */

  return FALSE;
}

static gboolean
set_pipeline (GMAudioProfile *self,
              const char *candidate_pipeline)
{
  /* don't update if it's the same as the old one */
  if (candidate_pipeline &&
      strcmp (self->priv->pipeline, candidate_pipeline) == 0)
    return FALSE;

  if (candidate_pipeline != NULL)
    {
      g_free (self->priv->pipeline);
      self->priv->pipeline = g_strdup (candidate_pipeline);
      return TRUE;
    }
  /* otherwise just leave the old pipeline */

  return FALSE;
}

static gboolean
set_extension (GMAudioProfile *self,
               const char *candidate_extension)
{
  /* don't update if it's the same as the old one */
  if (candidate_extension &&
      strcmp (self->priv->extension, candidate_extension) == 0)
    return FALSE;

  if (candidate_extension != NULL)
    {
      g_free (self->priv->extension);
      self->priv->extension = g_strdup (candidate_extension);
      return TRUE;
    }
  /* otherwise just leave the old extension */

  return FALSE;
}

static const gchar*
find_key (const gchar* key)
{
  const gchar* end;

  end = strrchr (key, '/');

  ++end;

  return end;
}

static void
profile_change_notify (MateConfClient *client,
                       guint        cnxn_id,
                       MateConfEntry  *entry,
                       gpointer     user_data)
{
  GMAudioProfile *self;
  const char *key;
  MateConfValue *val;
  GMAudioSettingMask mask; /* to keep track of what has changed */

  self = GM_AUDIO_PROFILE (user_data);
  GST_DEBUG ("profile_change_notify: start in profile with name %s\n",
           self->priv->name);

  val = mateconf_entry_get_value (entry);

  key = find_key (mateconf_entry_get_key (entry));

/* strings are set through static set_ functions */
#define UPDATE_STRING(KName, FName, Preset)                             \
  }                                                                     \
else if (strcmp (key, KName) == 0)                                      \
  {                                                                     \
    const char * setting = (Preset);                                    \
                                                                        \
    if (val && val->type == MATECONF_VALUE_STRING)                         \
      setting = mateconf_value_get_string (val);                           \
                                                                        \
    mask.FName = set_##FName (self, setting);                           \
                                                                        \
    self->priv->locked.FName = !mateconf_entry_get_is_writable (entry);

/* booleans are set directly on the profile priv variable */
#define UPDATE_BOOLEAN(KName, FName, Preset)                            \
  }                                                                     \
else if (strcmp (key, KName) == 0)                                      \
  {                                                                     \
    gboolean setting = (Preset);                                        \
                                                                        \
    if (val && val->type == MATECONF_VALUE_BOOL)                           \
      setting = mateconf_value_get_bool (val);                             \
                                                                        \
    if (setting != self->priv->FName)                                   \
      {                                                                 \
        mask.FName = TRUE;                                              \
        self->priv->FName = setting;                                    \
      }                                                                 \
                                                                        \
    self->priv->locked.FName = !mateconf_entry_get_is_writable (entry);

  if (0)
  {
    UPDATE_STRING (KEY_NAME,        name, NULL);
    UPDATE_STRING (KEY_DESCRIPTION, description, NULL);
    UPDATE_STRING (KEY_PIPELINE,    pipeline, NULL);
    UPDATE_STRING (KEY_EXTENSION,   extension, NULL);
    UPDATE_BOOLEAN (KEY_ACTIVE, active, TRUE);
  }

#undef UPDATE_STRING
#undef UPDATE_BOOLEAN

  if (!(gm_audio_setting_mask_is_empty (&mask)))
  {
    GST_DEBUG ("emit changed\n");
    emit_changed (self, &mask);
  }
  GST_DEBUG ("PROFILE_CHANGE_NOTIFY: changed stuff\n");
}

/* MateConf notification callback for profile_list */
static void
gm_audio_profile_list_notify (MateConfClient *client,
                              guint        cnxn_id,
                              MateConfEntry  *entry,
                              gpointer     user_data)
{
  MateConfValue *val;
  GSList *value_list;
  GSList *string_list;
  GSList *tmp;

  GST_DEBUG ("profile_list changed\n");
  val = mateconf_entry_get_value (entry);

  if (val == NULL ||
      val->type != MATECONF_VALUE_LIST ||
      mateconf_value_get_list_type (val) != MATECONF_VALUE_STRING)
    value_list = NULL;
  else
    value_list = mateconf_value_get_list (val);

  string_list = NULL;
  tmp = value_list;
  while (tmp != NULL)
    {
      string_list = g_slist_prepend (string_list,
                                     g_strdup (mateconf_value_get_string ((MateConfValue*) tmp->data)));

      tmp = tmp->next;
    }

  string_list = g_slist_reverse (string_list);

  gm_audio_profile_sync_list (TRUE, string_list);

  g_slist_foreach (string_list, (GFunc) g_free, NULL);
  g_slist_free (string_list);
}


/* needs to be called once
 * sets up the global profiles hash
 * safe to call more than once
 */
void
gm_audio_profile_initialize (MateConfClient *conf)
{
  GError *err;
/*
  char *str;
*/

  g_return_if_fail (profiles == NULL);

  profiles = g_hash_table_new (g_str_hash, g_str_equal);

  if (_conf == NULL) _conf = conf;
  /* sync it for the first time */
  gm_audio_profile_sync_list (FALSE, NULL);

  /* subscribe to changes to profile list */
  err = NULL;
  mateconf_client_notify_add (conf,
                           CONF_GLOBAL_PREFIX"/profile_list",
                           gm_audio_profile_list_notify,
                           NULL,
                           NULL, &err);

  if (err)
    {
      g_printerr (_("There was an error subscribing to notification of audio profile list changes. (%s)\n"),
                  err->message);
      g_error_free (err);
    }


  /* FIXME: no defaults
  err = NULL;
  mateconf_client_notify_add (conf,
                           CONF_GLOBAL_PREFIX"/default_profile",                            default_change_notify,
                           NULL,
                           NULL, &err);
  if (err)
    {
      g_printerr (_("There was an error subscribing to notification of changes to default profile. (%s)\n"),
                  err->message);
      g_error_free (err);
    }

  str = mateconf_client_get_string (conf,
                                 CONF_GLOBAL_PREFIX"/default_profile",
                                 NULL);
  if (str)
    {
      update_default_profile (str,
                              !mateconf_client_key_is_writable (conf,
                                                             CONF_GLOBAL_PREFIX"/default_profile",
                                                             NULL));
      g_free (str);
    }
  */
}

static void
emit_changed (GMAudioProfile           *self,
              const GMAudioSettingMask *mask)
{
  self->priv->in_notification_count += 1;
  g_signal_emit (G_OBJECT (self), signals[CHANGED], 0, mask);
  self->priv->in_notification_count -= 1;
}


/* update the given GMAudioProfile from MateConf */
static void
gm_audio_profile_update (GMAudioProfile *self)
{
  GMAudioSettingMask locked;
  GMAudioSettingMask mask;

  memset (&mask, '\0', sizeof (mask));
  memset (&locked, '\0', sizeof (locked));

#define UPDATE_BOOLEAN(KName, FName)                                    \
{                                                                       \
  char *key = mateconf_concat_dir_and_key (self->priv->profile_dir, KName); \
  gboolean val = mateconf_client_get_bool (self->priv->conf, key, NULL);   \
                                                                        \
  if (val != self->priv->FName)                                         \
  {                                                                     \
    mask.FName = TRUE;                                                  \
    self->priv->FName = val;                                            \
  }                                                                     \
                                                                        \
  locked.FName =                                                        \
    !mateconf_client_key_is_writable (self->priv->conf, key, NULL);        \
                                                                        \
  g_free (key);                                                         \
}
#define UPDATE_STRING(KName, FName)                                     \
{                                                                       \
  char *key = mateconf_concat_dir_and_key (self->priv->profile_dir, KName); \
  char *val = mateconf_client_get_string (self->priv->conf, key, NULL);    \
                                                                        \
  mask.FName = set_##FName (self, val);                                 \
                                                                        \
  locked.FName =                                                        \
    !mateconf_client_key_is_writable (self->priv->conf, key, NULL);        \
                                                                        \
  g_free (val);                                                         \
  g_free (key);                                                         \
}
  UPDATE_STRING  (KEY_NAME,        name);
  UPDATE_STRING  (KEY_DESCRIPTION, description);
  UPDATE_STRING  (KEY_PIPELINE,    pipeline);
  UPDATE_STRING  (KEY_EXTENSION,   extension);
  UPDATE_BOOLEAN (KEY_ACTIVE,      active);

#undef UPDATE_BOOLEAN
#undef UPDATE_STRING
  self->priv->locked = locked;
  //FIXME: we don't use mask ?
}


static void
listify_foreach (gpointer key,
                 gpointer value,
                 gpointer data)
{
  GList **listp = data;

  *listp = g_list_prepend (*listp, value);
}

static int
alphabetic_cmp (gconstpointer a,
                gconstpointer b)
{
  GMAudioProfile *ap = (GMAudioProfile*) a;
  GMAudioProfile *bp = (GMAudioProfile*) b;

  return g_utf8_collate (gm_audio_profile_get_name (ap),
                         gm_audio_profile_get_name (bp));
}

GList*
gm_audio_profile_get_list (void)
{
  GList *list;

  list = NULL;
  g_hash_table_foreach (profiles, listify_foreach, &list);

  list = g_list_sort (list, alphabetic_cmp);

  return list;
}

/* Return a GList of active GMAudioProfile's only */
GList*
gm_audio_profile_get_active_list (void)
{
  GList *list, *orig;
  GList *new_list;

  orig = list = gm_audio_profile_get_list ();

  new_list = NULL;
  while (list)
  {
    GMAudioProfile *profile;

    profile = (GMAudioProfile *) list->data;
    if (gm_audio_profile_get_active (profile)) {
      const gchar *pipe = gm_audio_profile_get_pipeline (profile);
      gchar *test = g_strdup_printf ("fakesrc ! %s ! fakesink", pipe);
      GstElement *p;
      GError *err = NULL;
      if ((p = gst_parse_launch (test, &err)) && err == NULL) {
        new_list = g_list_prepend (new_list, list->data);
        g_object_unref (p);
      } else {
        g_object_unref (p);
        g_error_free (err);
      }
      g_free (test);
    }
    list = g_list_next (list);
  }

  g_list_free (orig);
  return g_list_reverse (new_list);
}

int
gm_audio_profile_get_count (void)
{
  return g_hash_table_size (profiles);
}

GMAudioProfile*
gm_audio_profile_lookup (const char *id)
{
  g_return_val_if_fail (id != NULL, NULL);

  if (profiles)
  {
    GST_DEBUG ("a_p_l: profiles exists, returning hash table lookup of %s\n", id);
    return g_hash_table_lookup (profiles, id);
  }
  else
    return NULL;
}

void
gm_audio_profile_forget (GMAudioProfile *self)
{
  GST_DEBUG ("audio_profile_forget: forgetting name %s\n",
           gm_audio_profile_get_name (self));
  if (!self->priv->forgotten)
  {
    GError *err;

    err = NULL;
    GST_DEBUG ("audio_profile_forget: removing from mateconf\n");
    /* FIXME: remove_dir doesn't actually work.  Either unset all keys
     * manually or use recursive_unset on HEAD */
    mateconf_client_remove_dir (self->priv->conf,
                             self->priv->profile_dir,
                             &err);
    if (err)
    {
      g_printerr (_("There was an error forgetting profile path %s. (%s)\n"),
                  self->priv->profile_dir, err->message);
                  g_error_free (err);
    }

    g_hash_table_remove (profiles, self->priv->id);
    self->priv->forgotten = TRUE;

    g_signal_emit (G_OBJECT (self), signals[FORGOTTEN], 0);
  }
  else
    GST_DEBUG ("audio_profile_forget: profile->priv->forgotten\n");
}

gboolean
gm_audio_setting_mask_is_empty (const GMAudioSettingMask *mask)
{
  const unsigned int *p = (const unsigned int *) mask;
  const unsigned int *end = p + (sizeof (GMAudioSettingMask) /
                                 sizeof (unsigned int));

  while (p < end)
  {
    if (*p != 0)
      return FALSE;
    ++p;
  }

  return TRUE;
}

/* gm_audio_profile_create returns the unique id of the created profile,
 * which is used for looking up profiles later on.
 * Caller should free the returned id */
char *
gm_audio_profile_create (const char  *name,
                      MateConfClient *conf,
                      GError      **error)
{
  char *profile_id = NULL;
  char *profile_dir = NULL;
  int i;
  char *s;
  char *key = NULL;
  GError *err = NULL;
  GList *profiles = NULL;
  GSList *id_list = NULL;
  GList *tmp;

  GST_DEBUG ("a_p_c: Creating profile for %s\n", name);

#define BAIL_OUT_CHECK() do {                           \
      if (err != NULL)					\
       goto cleanup;                                    \
  } while (0)

  /* Pick a unique name for storing in mateconf (based on visible name) */
  profile_id = mateconf_escape_key (name, -1);
  s = g_strdup (profile_id);
  GST_DEBUG ("profile_id: %s\n", s);
  i = 0;
  while (gm_audio_profile_lookup (s))
  {
    g_free (s);
    s = g_strdup_printf ("%s-%d", profile_id, i);
    ++i;
  }
  g_free (profile_id);
  profile_id = s;

  profile_dir = mateconf_concat_dir_and_key (CONF_PROFILES_PREFIX,
                                          profile_id);

  /* Store a copy of default profile values at under that directory */
  key = mateconf_concat_dir_and_key (profile_dir,
                                  KEY_NAME);

  mateconf_client_set_string (conf,
                           key,
                           name,
                           &err);
  if (err != NULL) g_print ("ERROR: msg: %s\n", err->message);
  BAIL_OUT_CHECK ();
  g_free (key);

  key = mateconf_concat_dir_and_key (profile_dir,
                                  KEY_DESCRIPTION);

  mateconf_client_set_string (conf,
                           key,
                           _("<no description>"),
                           &err);
  if (err != NULL) g_print ("ERROR: msg: %s\n", err->message);
  BAIL_OUT_CHECK ();
  g_free (key);

  key = mateconf_concat_dir_and_key (profile_dir,
                                  KEY_PIPELINE);

  mateconf_client_set_string (conf,
                           key,
                           "identity",
                           &err);
  if (err != NULL) g_print ("ERROR: msg: %s\n", err->message);
  BAIL_OUT_CHECK ();
  g_free (key);

  key = mateconf_concat_dir_and_key (profile_dir,
                                  KEY_EXTENSION);

  mateconf_client_set_string (conf,
                           key,
                           "wav",
                           &err);
  if (err != NULL) g_print ("ERROR: msg: %s\n", err->message);
  BAIL_OUT_CHECK ();

  /* Add new profile to the profile list; the method for doing this has
   * a race condition where we and someone else set at the same time,
   * but I am just going to punt on this issue.
   */
  profiles = gm_audio_profile_get_list ();
  tmp = profiles;
  while (tmp != NULL)
  {
    id_list = g_slist_prepend (id_list,
                               g_strdup (gm_audio_profile_get_id (tmp->data)));
    tmp = tmp->next;
  }

  id_list = g_slist_prepend (id_list, g_strdup (profile_id));

  GST_DEBUG ("setting mateconf list\n");
  err = NULL;
  mateconf_client_set_list (conf,
                         CONF_GLOBAL_PREFIX"/profile_list",
                         MATECONF_VALUE_STRING,
                         id_list,
                         &err);
  BAIL_OUT_CHECK ();

 cleanup:
  /* run both when being dumped here through errors and normal exit; so
   * do proper cleanup here for both cases. */
  g_free (profile_dir);
  g_free (key);
  /* if we had an error then we're going to return NULL as the id */
  if (err != NULL)
  {
    g_free (profile_id);
    profile_id = NULL;
  }

  g_list_free (profiles);

  if (id_list)
  {
    g_slist_foreach (id_list, (GFunc) g_free, NULL);
    g_slist_free (id_list);
  }

  if (err)
  {
    GST_DEBUG ("WARNING: error: %s !\n", err->message);
    *error = err;
  }

  GST_DEBUG ("a_p_c: done\n");

  return profile_id;
}

/* delete the given list of profiles from the mateconf profile_list key */
void
gm_audio_profile_delete_list (MateConfClient *conf,
                           GList       *deleted_profiles,
                           GError      **error)
{
  GList *current_profiles;
  GList *tmp;
  GSList *id_list;
  GError *err;

  current_profiles = gm_audio_profile_get_list ();

  /* remove deleted profiles from list */
  tmp = deleted_profiles;
  while (tmp != NULL)
  {
    GMAudioProfile *profile = tmp->data;

    current_profiles = g_list_remove (current_profiles, profile);

    tmp = tmp->next;
  }

  /* make list of profile ids */
  id_list = NULL;
  tmp = current_profiles;
  while (tmp != NULL)
  {
    id_list = g_slist_prepend (id_list,
                               g_strdup (gm_audio_profile_get_id (tmp->data)));

    tmp = tmp->next;
  }

  g_list_free (current_profiles);
  err = NULL;
  GST_DEBUG ("setting profile_list in MateConf\n");
  mateconf_client_set_list (conf,
                         CONF_GLOBAL_PREFIX"/profile_list",
                         MATECONF_VALUE_STRING,
                         id_list,
                         &err);

  g_slist_foreach (id_list, (GFunc) g_free, NULL);
  g_slist_free (id_list);

  if (err && error) *error = err;
}
