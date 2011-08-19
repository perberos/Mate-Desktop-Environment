/* Totem GMP plugin
 *
 * Copyright © 2004 Bastien Nocera <hadess@hadess.net>
 * Copyright © 2002 David A. Schleef <ds@schleef.org>
 * Copyright © 2006, 2007, 2008 Christian Persch
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

#ifndef __TOTEM_GMP_PLAYER_H__
#define __TOTEM_GMP_PLAYER_H__

#include "totemNPClass.h"
#include "totemNPObject.h"

class totemGMPSettings;

class totemGMPPlayer : public totemNPObject
{
  public:
    totemGMPPlayer (NPP);
    virtual ~totemGMPPlayer ();

    enum PluginState {
      eState_Undefined,
      eState_Stopped,
      eState_Paused,
      eState_Playing,
      eState_ScanForward,
      eState_ScanReverse,
      eState_Buffering,
      eState_Waiting,
      eState_MediaEnded,
      eState_Transitioning,
      eState_Ready,
      eState_Reconnecting
    };

    PluginState mPluginState;

  private:

    enum Methods {
      eClose,
      eLaunchURL,
      eNewMedia,
      eNewPlaylist,
      eOpenPlayer
    };

    enum Properties {
      eCdromCollection,
      eClosedCaption,
      eControls,
      eCurrentMedia,
      eCurrentPlaylist,
      eDvd,
      eEnableContextMenu,
      eEnabled,
      eError,
      eFullScreen,
      eIsOnline,
      eIsRemote,
      eMediaCollection,
      eNetwork,
      eOpenState,
      ePlayerApplication,
      ePlaylistCollection,
      ePlayState,
      eSettings,
      eStatus,
      eStretchToFit,
      eUiMode,
      eURL,
      eVersionInfo,
      eWindowlessVideo
    };

    virtual bool InvokeByIndex (int aIndex, const NPVariant *argv, uint32_t argc, NPVariant *_result);
    virtual bool GetPropertyByIndex (int aIndex, NPVariant *_result);
    virtual bool SetPropertyByIndex (int aIndex, const NPVariant *aValue);
};

TOTEM_DEFINE_NPCLASS (totemGMPPlayer);

#endif /* __TOTEM_GMP_PLAYER_H__ */
