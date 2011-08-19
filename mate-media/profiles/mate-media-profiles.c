/* mate-media-profiles.c: public library code */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mate-media-profiles.h"

#include <string.h>
#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>

#include "audio-profile-private.h"
#include "audio-profile-edit.h"
#include "gmp-conf.h"
#include "gmp-util.h"

/* do all necessary initialization to use this simple helper library */
void
mate_media_profiles_init (MateConfClient *conf)
{
  GError *err = NULL;

  /* i18n */
  bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  if (conf == NULL)
    conf = mateconf_client_get_default ();
  else
    g_object_ref (G_OBJECT (conf));

    /* initialize MateConf */
    mateconf_client_add_dir (conf, CONF_GLOBAL_PREFIX,
			  MATECONF_CLIENT_PRELOAD_ONELEVEL,
			  &err);
  if (err)
  {
    g_printerr ("There was an error loading config from %s. (%s)\n",
                  CONF_GLOBAL_PREFIX, err->message);
    g_error_free (err);
  }

 /* Register GMAudioProfieEdit widget for GtkBuilder */
  volatile GType dummy = GM_AUDIO_TYPE_PROFILE_EDIT;
  dummy = dummy;
  /* initialize the audio profiles part */
  gm_audio_profile_initialize (conf);

  g_object_unref (G_OBJECT (conf));
}
