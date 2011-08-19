/* Totem NarrowSpace plugin scriptable
 *
 * Copyright © 2004 Bastien Nocera <hadess@hadess.net>
 * Copyright © 2002 David A. Schleef <ds@schleef.org>
 * Copyright © 2008 Christian Persch
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

#ifndef __NARROWSPACE_PLUGIN_H__
#define __NARROWSPACE_PLUGIN_H__

#include "totemNPClass.h"
#include "totemNPObject.h"

class totemNarrowSpacePlayer : public totemNPObject
{
  public:
    totemNarrowSpacePlayer (NPP);
    virtual ~totemNarrowSpacePlayer ();

    enum PluginState {
      eState_Complete,
      eState_Error,
      eState_Loading,
      eState_Playable,
      eState_Waiting
    };

    PluginState mPluginState;

  private:

    enum Methods {
      eGetAutoPlay,
      eGetBgColor,
      eGetComponentVersion,
      eGetControllerVisible,
      eGetDuration,
      eGetEndTime,
      eGetFieldOfView,
      eGetHotpotTarget,
      eGetHotpotUrl,
      eGetHREF,
      eGetIsLooping,
      eGetIsQuickTimeRegistered,
      eGetIsVRMovie,
      eGetKioskMode,
      eGetLanguage,
      eGetLoopIsPalindrome,
      eGetMatrix,
      eGetMaxBytesLoaded,
      eGetMaxTimeLoaded,
      eGetMIMEType,
      eGetMovieID,
      eGetMovieName,
      eGetMovieSize,
      eGetMute,
      eGetNodeCount,
      eGetNodeID,
      eGetPanAngle,
      eGetPlayEveryFrame,
      eGetPluginStatus,
      eGetPluginVersion,
      eGetQTNextUrl,
      eGetQuickTimeConnectionSpeed,
      eGetQuickTimeLanguage,
      eGetQuickTimeVersion,
      eGetRate,
      eGetRectangle,
      eGetResetPropertiesOnReload,
      eGetSpriteTrackVariable,
      eGetStartTime,
      eGetTarget,
      eGetTiltAngle,
      eGetTime,
      eGetTimeScale,
      eGetTrackCount,
      eGetTrackEnabled,
      eGetTrackName,
      eGetTrackType,
      eGetURL,
      eGetUserData,
      eGetVolume,
      eGoPreviousNode,
      ePlay,
      eRewind,
      eSetAutoPlay,
      eSetBgColor,
      eSetControllerVisible,
      eSetEndTime,
      eSetFieldOfView,
      eSetHotpotTarget,
      eSetHotpotUrl,
      eSetHREF,
      eSetIsLooping,
      eSetKioskMode,
      eSetLanguage,
      eSetLoopIsPalindrome,
      eSetMatrix,
      eSetMovieID,
      eSetMovieName,
      eSetMute,
      eSetNodeID,
      eSetPanAngle,
      eSetPlayEveryFrame,
      eSetQTNextUrl,
      eSetRate,
      eSetRectangle,
      eSetResetPropertiesOnReload,
      eSetSpriteTrackVariable,
      eSetStartTime,
      eSetTarget,
      eSetTiltAngle,
      eSetTime,
      eSetTrackEnabled,
      eSetURL,
      eSetVolume,
      eShowDefaultView,
      eStep,
      eStop
    };

    virtual bool InvokeByIndex (int aIndex, const NPVariant *argv, uint32_t argc, NPVariant *_result);
};

TOTEM_DEFINE_NPCLASS (totemNarrowSpacePlayer);

#endif /* __NARROWSPACE_PLUGIN_H__ */
