/* Totem GMP plugin
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
#include "totemGMPControls.h"

static const char *propertyNames[] = {
  "audioLanguageCount",
  "currentAudioLanguage",
  "currentAudioLanguageIndex",
  "currentItem",
  "currentMarker",
  "currentPosition",
  "currentPositionString",
  "currentPositionTimecode"
};

static const char *methodNames[] = {
  "fastForward",
  "fastReverse",
  "getAudioLanguageDescription",
  "getAudioLanguageID",
  "getLanguageName",
  "isAvailable",
  "next",
  "pause",
  "play",
  "playItem",
  "previous",
  "step",
  "stop"
};

TOTEM_IMPLEMENT_NPCLASS (totemGMPControls,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

totemGMPControls::totemGMPControls (NPP aNPP)
  : totemNPObject (aNPP)
{
  TOTEM_LOG_CTOR ();
}

totemGMPControls::~totemGMPControls ()
{
  TOTEM_LOG_DTOR ();
}

bool
totemGMPControls::InvokeByIndex (int aIndex,
                                 const NPVariant *argv,
                                 uint32_t argc,
                                 NPVariant *_result)
{
  TOTEM_LOG_INVOKE (aIndex, totemGMPControls);

  switch (Methods (aIndex)) {
    case ePause:
      /* void pause (); */
      Plugin()->Command (TOTEM_COMMAND_PAUSE);
      return VoidVariant (_result);

    case ePlay:
      /* void play (); */
      Plugin()->Command (TOTEM_COMMAND_PLAY);
      return VoidVariant (_result);

    case eStop:
      /* void stop (); */
      Plugin()->Command (TOTEM_COMMAND_PAUSE);
      return VoidVariant (_result);

    case eGetAudioLanguageDescription:
      /* AUTF8String getAudioLanguageDescription (in long index); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex,totemGMPControls);
      return StringVariant (_result, "English");

    case eGetLanguageName:
      /* AUTF8String getLanguageName (in long LCID); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex,totemGMPControls);
      return StringVariant (_result, "English");

    case eIsAvailable:
      /* boolean isAvailable (in ACString name); */
      NPString name;
      if (!GetNPStringFromArguments (argv, argc, 0, name))
        return false;
      if (g_ascii_strncasecmp (name.UTF8Characters, "currentItem", name.UTF8Length) == 0
      	  || g_ascii_strncasecmp (name.UTF8Characters, "next", name.UTF8Length) == 0
      	  || g_ascii_strncasecmp (name.UTF8Characters, "pause", name.UTF8Length) == 0
      	  || g_ascii_strncasecmp (name.UTF8Characters, "play", name.UTF8Length) == 0
      	  || g_ascii_strncasecmp (name.UTF8Characters, "previous", name.UTF8Length) == 0
      	  || g_ascii_strncasecmp (name.UTF8Characters, "stop", name.UTF8Length) == 0)
      	  return BoolVariant (_result, true);
      return BoolVariant (_result, false);

    case eFastForward:
      /* void fastForward (); */
    case eFastReverse:
      /* void fastReverse (); */
    case eGetAudioLanguageID:
      /* long getAudioLanguageID (in long index); */
    case eNext:
      /* void next (); */
    case ePlayItem:
      /* void playItem (in totemIGMPMedia theMediaItem); */
    case ePrevious:
      /* void previous (); */
    case eStep:
      /* void step (in long frameCount); */
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex,totemGMPControls);
      return VoidVariant (_result);
  }

  return false;
}

bool
totemGMPControls::GetPropertyByIndex (int aIndex,
                                      NPVariant *_result)
{
  TOTEM_LOG_SETTER (aIndex, totemGMPControls);

  switch (Properties (aIndex)) {
    case eCurrentPosition:
      /* attribute double currentPosition; */
      return DoubleVariant (_result, double (Plugin()->GetTime()) / 1000.0);

    case eCurrentItem:
      /* attribute totemIGMPMedia currentItem; */
    case eCurrentPositionTimecode:
      /* attribute ACString currentPositionTimecode; */
    case eCurrentPositionString:
      /* readonly attribute ACString currentPositionString; */
      TOTEM_WARN_GETTER_UNIMPLEMENTED (aIndex, totemGMPControls);
      return StringVariant (_result, "");

    case eAudioLanguageCount:
      /* readonly attribute long audioLanguageCount; */
    case eCurrentAudioLanguage:
      /* attribute long currentAudioLanguage; */
    case eCurrentAudioLanguageIndex:
      /* attribute long currentAudioLanguageIndex; */
    case eCurrentMarker:
      /* attribute long currentMarker; */
      TOTEM_WARN_GETTER_UNIMPLEMENTED (aIndex, totemGMPControls);
      return Int32Variant (_result, 0);
  }

  return false;
}

bool
totemGMPControls::SetPropertyByIndex (int aIndex,
                                      const NPVariant *aValue)
{
  TOTEM_LOG_SETTER (aIndex, totemGMPControls);

  switch (Properties (aIndex)) {
    case eCurrentAudioLanguage:
      /* attribute long currentAudioLanguage; */
    case eCurrentAudioLanguageIndex:
      /* attribute long currentAudioLanguageIndex; */
    case eCurrentItem:
      /* attribute totemIGMPMedia currentItem; */
    case eCurrentMarker:
      /* attribute long currentMarker; */
    case eCurrentPosition:
      /* attribute double currentPosition; */
    case eCurrentPositionTimecode:
      /* attribute ACString currentPositionTimecode; */
      TOTEM_WARN_SETTER_UNIMPLEMENTED(aIndex, totemGMPControls);
      return true;

    case eAudioLanguageCount:
      /* readonly attribute long audioLanguageCount; */
    case eCurrentPositionString:
      /* readonly attribute ACString currentPositionString; */
      return ThrowPropertyNotWritable ();
  }

  return false;
}
