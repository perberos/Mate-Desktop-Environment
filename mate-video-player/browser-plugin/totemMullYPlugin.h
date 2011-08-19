/* Totem MullY Plugin
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

#ifndef __TOTEM_MULLY_PLUGIN_H__
#define __TOTEM_MULLY_PLUGIN_H__

#include "totemNPClass.h"
#include "totemNPObject.h"

class totemMullYPlayer : public totemNPObject
{
  public:
    totemMullYPlayer (NPP);
    virtual ~totemMullYPlayer ();

  private:

    enum Methods {
      /* Version */
      eGetVersion,

      /* Setup */
      eSetMinVersion,
      eSetMode,
      eSetAllowContextMenu,
      eSetAutoPlay,
      eSetLoop,
      eSetBufferingMode,
      eSetBannerEnabled,
      eSetVolume,
      eSetMovieTitle,
      eSetPreviewImage,
      eSetPreviewMessage,
      eSetPreviewMessageFontSize,

      /* Media management */
      eOpen,

      /* Playback */
      ePlay,
      ePause,
      eStepForward,
      eStepBackward,
      eFF,
      eRW,
      eStop,
      eMute,
      eUnMute,
      eSeek,

      /* Windowing */
      eAbout,
      eShowPreferences,
      eShowContextMenu,
      eGoEmbedded,
      eGoWindowed,
      eGoFullscreen,
      eResize,

      /* Media information */
      eGetTotalTime,
      eGetVideoWidth,
      eGetVideoHeight,
      eGetTotalVideoFrames,
      eGetVideoFramerate,
      eGetNumberOfAudioTracks,
      eGetNumberOfSubtitleTracks,
      eGetAudioTrackLanguage,
      eGetSubtitleTrackLanguage,
      eGetAudioTrackName,
      eGetSubtitleTrackName,
      eGetCurrentAudioTrack,
      eGetCurrentSubtitleTrack,

      /* Media management */
      eSetCurrentAudioTrack,
      eSetCurrentSubtitleTrack,
    };

    virtual bool InvokeByIndex (int aIndex, const NPVariant *argv, uint32_t argc, NPVariant *_result);
};

TOTEM_DEFINE_NPCLASS (totemMullYPlayer);

#endif /* __TOTEM_MULLY_PLUGIN_H__ */
