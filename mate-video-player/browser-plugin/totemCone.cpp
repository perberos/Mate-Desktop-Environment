/* Totem Cone plugin
 *
 * Copyright © 2004 Bastien Nocera <hadess@hadess.net>
 * Copyright © 2002 David A. Schleef <ds@schleef.org>
 * Copyright © 2006, 2008 Christian Persch
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#include <config.h>

#include <string.h>

#include <glib.h>

#include "totemPlugin.h"
#include "totemCone.h"

static const char *propertyNames[] = {
  "audio",
  "input",
  "iterator",
  "log",
  "messages",
  "playlist",
  "VersionInfo",
  "video",
};

static const char *methodNames[] = {
  "versionInfo"
};

TOTEM_IMPLEMENT_NPCLASS (totemCone,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

totemCone::totemCone (NPP aNPP)
  : totemNPObject (aNPP)
{
  TOTEM_LOG_CTOR ();
}

totemCone::~totemCone ()
{
  TOTEM_LOG_DTOR ();
}

bool
totemCone::InvokeByIndex (int aIndex,
                          const NPVariant *argv,
                          uint32_t argc,
                          NPVariant *_result)
{
  TOTEM_LOG_INVOKE (aIndex, totemCone);

  switch (Methods (aIndex)) {
    case eversionInfo:
      return GetPropertyByIndex (eVersionInfo, _result);
  }

  return false;
}

bool
totemCone::GetPropertyByIndex (int aIndex,
                               NPVariant *_result)
{
  TOTEM_LOG_GETTER (aIndex, totemCone);

  switch (Properties (aIndex)) {
    case eAudio:
      return ObjectVariant (_result, Plugin()->GetNPObject (totemPlugin::eConeAudio));

    case eInput:
      return ObjectVariant (_result, Plugin()->GetNPObject (totemPlugin::eConeInput));

    case ePlaylist:
      return ObjectVariant (_result, Plugin()->GetNPObject (totemPlugin::eConePlaylist));

    case eVideo:
      return ObjectVariant (_result, Plugin()->GetNPObject (totemPlugin::eConeVideo));

    case eVersionInfo:
      return StringVariant (_result, TOTEM_CONE_VERSION);

    case eIterator:
    case eLog:
    case eMessages:
      TOTEM_WARN_GETTER_UNIMPLEMENTED (aIndex, _result);
      return NullVariant (_result);
  }

  return false;
}

bool
totemCone::SetPropertyByIndex (int aIndex,
                               const NPVariant *aValue)
{
  TOTEM_LOG_SETTER (aIndex, totemCone);

  return ThrowPropertyNotWritable ();
}
