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
#include "totemConeInput.h"

static const char *propertyNames[] = {
  "fps",
  "hasVout",
  "length",
  "position",
  "rate",
  "state",
  "time"
};

TOTEM_IMPLEMENT_NPCLASS (totemConeInput,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         NULL, 0,
                         NULL);

totemConeInput::totemConeInput (NPP aNPP)
  : totemNPObject (aNPP)
{
  TOTEM_LOG_CTOR ();
}

totemConeInput::~totemConeInput ()
{
  TOTEM_LOG_DTOR ();
}

bool
totemConeInput::GetPropertyByIndex (int aIndex,
                                    NPVariant *_result)
{
  TOTEM_LOG_GETTER (aIndex, totemConeInput);

  switch (Properties (aIndex)) {
    case eState: {
      int32_t state;

      /* IDLE/CLOSE=0,
       * OPENING=1,
       * BUFFERING=2,
       * PLAYING=3,
       * PAUSED=4,
       * STOPPING=5,
       * ERROR=6
       */
      if (Plugin()->State() == TOTEM_STATE_PLAYING) {
        state = 3;
      } else if (Plugin()->State() == TOTEM_STATE_PAUSED) {
        state = 4;
      } else {
        state = 0;
      }

      return Int32Variant (_result, state);
    }

    case eLength:
      return DoubleVariant (_result, Plugin()->Duration());
    case eTime:
      return DoubleVariant (_result, double (Plugin()->GetTime()));

    case eFps:
    case eHasVout:
    case ePosition:
    case eRate:
      TOTEM_WARN_GETTER_UNIMPLEMENTED (aIndex, _result);
      return VoidVariant (_result);
  }

  return false;
}

bool
totemConeInput::SetPropertyByIndex (int aIndex,
                                    const NPVariant *aValue)
{
  TOTEM_LOG_SETTER (aIndex, totemConeInput);

  switch (Properties (aIndex)) {
    case eTime:
      int32_t time;
      if (!GetInt32FromArguments (aValue, 1, 0, time))
	return false;

      Plugin()->SetTime(time);
      return true;

    case ePosition:
    case eRate:
    case eState:
      TOTEM_WARN_SETTER_UNIMPLEMENTED (aIndex, _result);
      return true;

    case eFps:
    case eHasVout:
    case eLength:
      return ThrowPropertyNotWritable ();
  }

  return false;
}
