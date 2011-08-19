/* Totem NarrowSpace Plugin
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

#include "totemPlugin.h"
#include "totemNarrowSpacePlugin.h"

static const char *methodNames[] = {
  "GetAutoPlay",
  "GetBgColor",
  "GetComponentVersion",
  "GetControllerVisible",
  "GetDuration",
  "GetEndTime",
  "GetFieldOfView",
  "GetHotpotTarget",
  "GetHotpotUrl",
  "GetHREF",
  "GetIsLooping",
  "GetIsQuickTimeRegistered",
  "GetIsVRMovie",
  "GetKioskMode",
  "GetLanguage",
  "GetLoopIsPalindrome",
  "GetMatrix",
  "GetMaxBytesLoaded",
  "GetMaxTimeLoaded",
  "GetMIMEType",
  "GetMovieID",
  "GetMovieName",
  "GetMovieSize",
  "GetMute",
  "GetNodeCount",
  "GetNodeID",
  "GetPanAngle",
  "GetPlayEveryFrame",
  "GetPluginStatus",
  "GetPluginVersion",
  "GetQTNextUrl",
  "GetQuickTimeConnectionSpeed",
  "GetQuickTimeLanguage",
  "GetQuickTimeVersion",
  "GetRate",
  "GetRectangle",
  "GetResetPropertiesOnReload",
  "GetSpriteTrackVariable",
  "GetStartTime",
  "GetTarget",
  "GetTiltAngle",
  "GetTime",
  "GetTimeScale",
  "GetTrackCount",
  "GetTrackEnabled",
  "GetTrackName",
  "GetTrackType",
  "GetURL",
  "GetUserData",
  "GetVolume",
  "GoPreviousNode",
  "Play",
  "Rewind",
  "SetAutoPlay",
  "SetBgColor",
  "SetControllerVisible",
  "SetEndTime",
  "SetFieldOfView",
  "SetHotpotTarget",
  "SetHotpotUrl",
  "SetHREF",
  "SetIsLooping",
  "SetKioskMode",
  "SetLanguage",
  "SetLoopIsPalindrome",
  "SetMatrix",
  "SetMovieID",
  "SetMovieName",
  "SetMute",
  "SetNodeID",
  "SetPanAngle",
  "SetPlayEveryFrame",
  "SetQTNextUrl",
  "SetRate",
  "SetRectangle",
  "SetResetPropertiesOnReload",
  "SetSpriteTrackVariable",
  "SetStartTime",
  "SetTarget",
  "SetTiltAngle",
  "SetTime",
  "SetTrackEnabled",
  "SetURL",
  "SetVolume",
  "ShowDefaultView",
  "Step",
  "Stop"
};

TOTEM_IMPLEMENT_NPCLASS (totemNarrowSpacePlayer,
                         NULL, 0,
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

totemNarrowSpacePlayer::totemNarrowSpacePlayer (NPP aNPP)
  : totemNPObject (aNPP),
    mPluginState (eState_Waiting)
{
  TOTEM_LOG_CTOR ();
}

totemNarrowSpacePlayer::~totemNarrowSpacePlayer ()
{
  TOTEM_LOG_DTOR ();
}

bool
totemNarrowSpacePlayer::InvokeByIndex (int aIndex,
                                       const NPVariant *argv,
                                       uint32_t argc,
                                       NPVariant *_result)
{
  TOTEM_LOG_INVOKE (aIndex, totemNarrowSpacePlayer);

  switch (Methods (aIndex)) {
    case ePlay:
      /* void Play (); */
      Plugin()->Command (TOTEM_COMMAND_PLAY);
      return VoidVariant (_result);

    case eStop:
      /* void Stop (); */
      Plugin()->Command (TOTEM_COMMAND_PAUSE);
      return VoidVariant (_result);

    case eRewind:
      /* void Rewind (); */
      Plugin()->Command (TOTEM_COMMAND_STOP);
      return VoidVariant (_result);

    case eGetMaxBytesLoaded:
      /* unsigned long GetMaxBytesLoaded (); */
      return Int32Variant (_result, Plugin()->BytesStreamed());

    case eGetMovieSize:
      /* unsigned long GetMovieSize (); */
      return Int32Variant (_result, Plugin()->BytesLength());

    case eGetAutoPlay:
      /* boolean GetAutoPlay (); */
      return BoolVariant (_result, Plugin()->AutoPlay());

    case eGetControllerVisible:
      /* boolean GetControllerVisible (); */
      return BoolVariant (_result, Plugin()->IsControllerVisible());

    case eGetIsLooping:
      /* boolean GetIsLooping (); */
      return BoolVariant (_result, Plugin()->IsLooping());

    case eGetKioskMode:
      /* boolean GetKioskMode (); */
      return BoolVariant (_result, Plugin()->IsKioskMode());

    case eGetLoopIsPalindrome:
      /* boolean GetLoopIsPalindrome (); */
      return BoolVariant (_result, Plugin()->IsLoopPalindrome());

    case eGetMute:
      /* boolean GetMute (); */
      return BoolVariant (_result, Plugin()->IsMute());

    case eGetPlayEveryFrame:
      /* boolean GetPlayEveryFrame (); */
      return BoolVariant (_result, Plugin()->PlayEveryFrame());

    case eSetAutoPlay: {
      /* void SetAutoPlay (in boolean autoPlay); */
      bool enabled;
      if (!GetBoolFromArguments (argv, argc, 0, enabled))
        return false;

      Plugin()->SetAutoPlay (enabled);
      return VoidVariant (_result);
    }

    case eSetControllerVisible: {
      /* void SetControllerVisible (in boolean visible); */
      bool enabled;
      if (!GetBoolFromArguments (argv, argc, 0, enabled))
        return false;

      Plugin()->SetControllerVisible (enabled);
      return VoidVariant (_result);
    }

    case eSetIsLooping: {
      /* void SetIsLooping (in boolean loop); */
      bool enabled;
      if (!GetBoolFromArguments (argv, argc, 0, enabled))
        return false;

      Plugin()->SetLooping (enabled);
      return VoidVariant (_result);
    }

    case eSetKioskMode: {
      /* void SetKioskMode (in boolean kioskMode); */
      bool enabled;
      if (!GetBoolFromArguments (argv, argc, 0, enabled))
        return false;

      Plugin()->SetKioskMode (enabled);
      return VoidVariant (_result);
    }

    case eSetLoopIsPalindrome: {
      /* void SetLoopIsPalindrome (in boolean loop); */
      bool enabled;
      if (!GetBoolFromArguments (argv, argc, 0, enabled))
        return false;

      Plugin()->SetLoopIsPalindrome (enabled);
      return VoidVariant (_result);
    }

    case eSetMute: {
      /* void SetMute (in boolean mute); */
      bool enabled;
      if (!GetBoolFromArguments (argv, argc, 0, enabled))
        return false;

      Plugin()->SetMute (enabled);
      return VoidVariant (_result);
    }

    case eSetPlayEveryFrame: {
      /* void SetPlayEveryFrame (in boolean playAll); */
      bool enabled;
      if (!GetBoolFromArguments (argv, argc, 0, enabled))
        return false;

      Plugin()->SetPlayEveryFrame (enabled);
      return VoidVariant (_result);
    }

    case eGetVolume:
      /* unsigned long GetVolume (); */
      return Int32Variant (_result, Plugin()->Volume() * 255.0);

    case eSetVolume: {
      /* void SetVolume (in unsigned long volume); */
      int32_t volume;
      if (!GetInt32FromArguments (argv, argc, 0, volume))
        return false;

      Plugin()->SetVolume ((double) CLAMP (volume, 0, 255) / 255.0);
      return VoidVariant (_result);
    }

    case eGetBgColor: {
      /* ACString GetBgColor (); */
      const char *color = Plugin()->BackgroundColor();
      if (color)
        return StringVariant (_result, color);

      return StringVariant (_result, "#000000");
    }
      
    case eSetBgColor: {
      /* void SetBgColor (in ACString color); */
      NPString color;
      if (!GetNPStringFromArguments (argv, argc, 0, color))
        return false;

      Plugin()->SetBackgroundColor (color);
      return VoidVariant (_result);
    }

    case eGetDuration:
      /* unsigned long GetDuration (); */
      return Int32Variant (_result, Plugin()->Duration());

    case eGetStartTime:
      /* unsigned long GetStartTime (); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return Int32Variant (_result, 0); /* FIXME */

    case eGetTime:
      /* unsigned long GetTime (); */
      return Int32Variant (_result, Plugin()->GetTime());

    case eGetEndTime:
      /* unsigned long GetEndTime (); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return Int32Variant (_result, 0);

    case eGetTimeScale:
      /* unsigned long GetTimeScale (); */
      return Int32Variant (_result, 1000); /* TimeScale is in milli-seconds */

    case eGetRate:
      /* float GetRate (); */
      return DoubleVariant (_result, Plugin()->Rate());

    case eSetRate: {
      /* void SetRate (in float rate); */
      double rate;
      if (!GetDoubleFromArguments (argv, argc, 0, rate))
        return false;

      Plugin()->SetRate (rate);
      return VoidVariant (_result);
    }

    case eGetLanguage:
      /* ACString GetLanguage (); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return StringVariant (_result, "English");

    case eGetComponentVersion:
      /* ACString GetComponentVersion (in ACString type, in ACString subType, in ACString manufacturer); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return StringVariant (_result, "1.0");

    case eGetIsQuickTimeRegistered:
      /* boolean GetIsQuickTimeRegistered (); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
    case eGetIsVRMovie:
      /* boolean GetIsVRMovie (); */
      return BoolVariant (_result, false);

    case eGetMIMEType:
      /* ACString GetMIMEType (); */
      return StringVariant (_result, "video/quicktime");

    case eGetMatrix:
      /* ACString GetMatrix (); */
      return StringVariant (_result, Plugin()->Matrix());

    case eSetMatrix: {
      /* void SetMatrix (in ACString matrix); */
      NPString matrix;
      if (!GetNPStringFromArguments (argv, argc, 0, matrix))
        return false;

      Plugin()->SetMatrix (matrix);
      return VoidVariant (_result);
    }

    case eGetMovieName:
      /* AUTF8String GetMovieName (); */
      return StringVariant (_result, Plugin()->MovieName());

    case eSetMovieName: {
      /* void SetMovieName (in AUTF8String movieName); */
      NPString name;
      if (!GetNPStringFromArguments (argv, argc, 0, name))
        return false;

      Plugin()->SetMovieName (name);
      return VoidVariant (_result);
    }

    case eGetRectangle:
      /* ACString GetRectangle (); */
      return StringVariant (_result, Plugin()->Rectangle());

    case eSetRectangle: {
      /* void SetRectangle (in ACString rect); */
      NPString rectangle;
      if (!GetNPStringFromArguments (argv, argc, 0, rectangle))
        return false;

      Plugin()->SetRectangle (rectangle);
      return VoidVariant (_result);
    }

    case eGetPluginStatus: {
      /* ACString GetPluginStatus (); */
      static const char *kState[] = {
        "Complete",
        NULL, /* "Error:<%d>", */
        "Loading",
        "Playable",
        "Waiting"
      };

      if (mPluginState == eState_Error)
        return StringVariant (_result, "Error:<1>");

      return StringVariant (_result, kState[mPluginState]);
    }

    case eGetTrackCount:
      /* unsigned long GetTrackCount (); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return Int32Variant (_result, 1);

    case eGetTrackEnabled:
      /* boolean GetTrackEnabled (in unsigned long index); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return BoolVariant (_result, true);

    case eGetPluginVersion:
      /* ACString GetPluginVersion (); */
    case eGetQuickTimeVersion:
      /* ACString GetQuickTimeVersion (); */
      return StringVariant (_result, TOTEM_NARROWSPACE_VERSION);

    case eGetQuickTimeConnectionSpeed:
      /* unsigned long GetQuickTimeConnectionSpeed (); */
      return Int32Variant (_result, Plugin()->Bandwidth());

    case eGetResetPropertiesOnReload:
      /* boolean GetResetPropertiesOnReload (); */
      return BoolVariant (_result, Plugin()->ResetPropertiesOnReload());

    case eSetResetPropertiesOnReload: {
      /* void SetResetPropertiesOnReload (in boolean reset); */
      bool enabled;
      if (!GetBoolFromArguments (argv, argc, 0, enabled))
        return false;

      Plugin()->SetResetPropertiesOnReload (enabled);
      return VoidVariant (_result);
    }

    case eGetMaxTimeLoaded:
      /* unsigned long GetMaxTimeLoaded (); */
    case eGetMovieID:
      /* unsigned long GetMovieID (); */
    case eGetNodeCount:
      /* unsigned long GetNodeCount (); */
    case eGetNodeID:
      /* unsigned long GetNodeID (); */
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return Int32Variant (_result, 0);

    case eGetFieldOfView:
      /* float GetFieldOfView (); */
    case eGetPanAngle:
      /* float GetPanAngle (); */
    case eGetTiltAngle:
      /* float GetTiltAngle (); */
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return DoubleVariant (_result, 0.0);

    case eGetTrackName:
      /* AUTF8String GetTrackName (in unsigned long index); */
    case eGetTrackType:
      /* ACString GetTrackType (in unsigned long index); */
    case eGetHotpotTarget:
      /* AUTF8String GetHotspotTarget (in unsigned long hotspotID); */
    case eGetHotpotUrl:
      /* AUTF8String GetHotspotUrl (in unsigned long hotspotID); */
    case eGetHREF:
      /* AUTF8String GetHREF (); */
    case eGetQTNextUrl:
      /* AUTF8String GetQTNEXTUrl (in unsigned long index); */
    case eGetQuickTimeLanguage:
      /* ACString GetQuickTimeLanguage (); */
    case eGetSpriteTrackVariable:
      /* ACString GetSpriteTrackVariable (in unsigned long trackIndex, in unsigned long variableIndex); */
    case eGetTarget:
      /* AUTF8String GetTarget (); */
    case eGetURL:
      /* AUTF8String GetURL (); */
    case eGetUserData:
      /* AUTF8String GetUserData (in ACString type); */
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return StringVariant (_result, "");

    case eGoPreviousNode:
      /* void GoPreviousNode (); */
    case eSetEndTime:
      /* void SetEndTime (in unsigned long time); */
    case eSetFieldOfView:
      /* void SetFieldOfView (in float fov); */
    case eSetHotpotTarget:
      /* void SetHotspotTarget (in unsigned long hotspotID, in AUTF8String target); */
    case eSetHotpotUrl:
      /* void SetHotspotUrl (in unsigned long hotspotID, in AUTF8String url); */
    case eSetHREF:
      /* void SetHREF (in AUTF8String url); */
    case eSetLanguage:
      /* void SetLanguage (in ACString language); */
    case eSetMovieID:
      /* void SetMovieID (in unsigned long movieID); */
    case eSetNodeID:
      /* void SetNodeID (in unsigned long id); */
    case eSetPanAngle:
      /* void SetPanAngle (in float angle); */
    case eSetQTNextUrl:
      /* void SetQTNEXTUrl (in unsigned long index, in AUTF8String url); */
    case eSetSpriteTrackVariable:
      /* void SetSpriteTrackVariable (in unsigned long trackIndex, in unsigned long variableIndex, in ACString value); */
    case eSetStartTime:
      /* void SetStartTime (in unsigned long time); */
    case eSetTarget:
      /* void SetTarget (in AUTF8String target); */
    case eSetTiltAngle:
      /* void SetTiltAngle (in float angle); */
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return VoidVariant (_result);
    case eSetTime:
      /* void SetTime (in unsigned long time); */
      int32_t time;
      if (!GetInt32FromArguments (argv, argc, 0, time))
        return false;

      Plugin()->SetTime (time);
      return true;
    case eSetTrackEnabled:
      /* void SetTrackEnabled (in unsigned long index, in boolean enabled); */
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return VoidVariant (_result);
    case eSetURL: {
      /* void SetURL (in AUTF8String url); */
      NPString url;
      if (!GetNPStringFromArguments (argv, argc, 0, url))
        return false;

      Plugin()->SetURL (url);
      return true;
    }
    case eStep:
      /* void Step (in long count); */
    case eShowDefaultView:
      /* void ShowDefaultView (); */
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemNarrowSpacePlayer);
      return VoidVariant (_result);
  }

  return false;
}
