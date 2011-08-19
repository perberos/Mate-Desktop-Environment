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
#include "totemConeVideo.h"

static const char *propertyNames[] = {
  "aspectRatio",
  "fullscreen",
  "height",
  "subtitle",
  "teletext",
  "width",
};

static const char *methodNames[] = {
  "toggleFullscreen",
  "toggleTeletext"
};

TOTEM_IMPLEMENT_NPCLASS (totemConeVideo,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

totemConeVideo::totemConeVideo (NPP aNPP)
  : totemNPObject (aNPP)
{
  TOTEM_LOG_CTOR ();
}

totemConeVideo::~totemConeVideo ()
{
  TOTEM_LOG_DTOR ();
}

bool
totemConeVideo::InvokeByIndex (int aIndex,
                               const NPVariant *argv,
                               uint32_t argc,
                               NPVariant *_result)
{
  TOTEM_LOG_INVOKE (aIndex, totemConeVideo);

  switch (Methods (aIndex)) {
    case eToggleFullscreen: {
      /* FIXMEchpe this sucks */
      NPVariant fullscreen;
      BOOLEAN_TO_NPVARIANT (!Plugin()->IsFullscreen(), fullscreen);
      return SetPropertyByIndex (eFullscreen, &fullscreen);
    }

    case eToggleTeletext:
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemConeVideo);
      return VoidVariant (_result);
  }

  return false;
}

bool
totemConeVideo::GetPropertyByIndex (int aIndex,
                                    NPVariant *_result)
{
  TOTEM_LOG_GETTER (aIndex, totemConeVideo);

  switch (Properties (aIndex)) {
    case eFullscreen:
      return BoolVariant (_result, Plugin()->IsFullscreen());

    case eAspectRatio:
    case eHeight:
    case eSubtitle:
    case eTeletext:
    case eWidth:
      TOTEM_WARN_GETTER_UNIMPLEMENTED (aIndex, _result);
      return VoidVariant (_result);
  }

  return false;
}

bool
totemConeVideo::SetPropertyByIndex (int aIndex,
                                    const NPVariant *aValue)
{
  TOTEM_LOG_SETTER (aIndex, totemConeVideo);

  switch (Properties (aIndex)) {
    case eFullscreen: {
      bool fullscreen;
      if (!GetBoolFromArguments (aValue, 1, 0, fullscreen))
        return false;

      Plugin()->SetFullscreen (fullscreen);
      return true;
    }

    case eAspectRatio:
    case eSubtitle:
    case eTeletext:
      TOTEM_WARN_SETTER_UNIMPLEMENTED (aIndex, _result);
      return true;

    case eHeight:
    case eWidth:
      return ThrowPropertyNotWritable ();
  }

  return false;
}
