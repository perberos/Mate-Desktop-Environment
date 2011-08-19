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

#include "npupp.h"

#include "totemPlugin.h"
#include "totemGMPNetwork.h"

static const char *propertyNames[] = {
  "bandWidth",
  "bitRate",
  "bufferingCount",
  "bufferingProgress",
  "bufferingTime",
  "downloadProgress",
  "encodedFrameRate",
  "frameRate",
  "framesSkipped",
  "lostPackets",
  "maxBandwidth",
  "maxBitRate",
  "receivedPackets",
  "receptionQuality",
  "recoveredPackets",
  "sourceProtocol"
};

static const char *methodNames[] = {
  "getProxyBypassForLocal",
  "getProxyExceptionList",
  "getProxyName",
  "getProxyPort",
  "getProxySettings",
  "setProxyBypassForLocal",
  "setProxyExceptionList",
  "setProxyName",
  "setProxyPort",
  "setProxySettings"
};

TOTEM_IMPLEMENT_NPCLASS (totemGMPNetwork,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

totemGMPNetwork::totemGMPNetwork (NPP aNPP)
  : totemNPObject (aNPP)
{
  TOTEM_LOG_CTOR ();
}

totemGMPNetwork::~totemGMPNetwork ()
{
  TOTEM_LOG_DTOR ();
}

bool
totemGMPNetwork::InvokeByIndex (int aIndex,
                                const NPVariant *argv,
                                uint32_t argc,
                                NPVariant *_result)
{
  TOTEM_LOG_INVOKE (aIndex, totemGMPNetwork);

  switch (Methods (aIndex)) {
    case eGetProxyBypassForLocal:
    case eGetProxyExceptionList:
    case eGetProxyName:
    case eGetProxyPort:
    case eGetProxySettings:
    case eSetProxyBypassForLocal:
    case eSetProxyExceptionList:
    case eSetProxyName:
    case eSetProxyPort:
    case eSetProxySettings:
      return ThrowSecurityError ();
  }

  return false;
}

bool
totemGMPNetwork::GetPropertyByIndex (int aIndex,
                                     NPVariant *_result)
{
  TOTEM_LOG_GETTER (aIndex, totemGMPNetwork);

  switch (Properties (aIndex)) {
    case eBandWidth:
      /* readonly attribute long bandWidth; */
      return Int32Variant (_result, Plugin()->Bandwidth() / 1024);

    case eBitRate:
    case eBufferingCount:
    case eBufferingProgress:
    case eBufferingTime:
    case eDownloadProgress:
    case eEncodedFrameRate:
    case eFrameRate:
    case eFramesSkipped:
    case eLostPackets:
    case eMaxBandwidth:
    case eMaxBitRate:
    case eReceivedPackets:
    case eReceptionQuality:
    case eRecoveredPackets:
    case eSourceProtocol:
      TOTEM_WARN_GETTER_UNIMPLEMENTED (aIndex, totemGMPNetwork);
      return Int32Variant (_result, 0);
  }

  return false;
}

bool
totemGMPNetwork::SetPropertyByIndex (int aIndex,
                                     const NPVariant *aValue)
{
  TOTEM_LOG_SETTER (aIndex, totemGMPNetwork);

  switch (Properties (aIndex)) {
    case eBufferingTime:
    case eMaxBandwidth:
      TOTEM_WARN_SETTER_UNIMPLEMENTED (aIndex, totemGMPNetwork);
      return true;

    case eBandWidth:
      /* readonly attribute long bandWidth; */
    case eBitRate:
    case eBufferingCount:
    case eBufferingProgress:
    case eDownloadProgress:
    case eEncodedFrameRate:
    case eFrameRate:
    case eFramesSkipped:
    case eLostPackets:
    case eMaxBitRate:
    case eReceivedPackets:
    case eReceptionQuality:
    case eRecoveredPackets:
    case eSourceProtocol:
      return ThrowPropertyNotWritable ();
  }

  return false;
}
