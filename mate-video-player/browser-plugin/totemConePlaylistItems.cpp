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
#include "totemConePlaylistItems.h"

static const char *propertyNames[] = {
  "count"
};

static const char *methodNames[] = {
  "clear"
};

TOTEM_IMPLEMENT_NPCLASS (totemConePlaylistItems,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

totemConePlaylistItems::totemConePlaylistItems (NPP aNPP)
  : totemNPObject (aNPP)
{
  TOTEM_LOG_CTOR ();
}

totemConePlaylistItems::~totemConePlaylistItems ()
{
  TOTEM_LOG_DTOR ();
}

bool
totemConePlaylistItems::InvokeByIndex (int aIndex,
                                       const NPVariant *argv,
                                       uint32_t argc,
                                       NPVariant *_result)
{
  TOTEM_LOG_INVOKE (aIndex, totemConePlaylistItems);

  switch (Methods (aIndex)) {
    case eClear:
      Plugin()->ClearPlaylist ();
      return VoidVariant (_result);
  }

  return false;
}

bool
totemConePlaylistItems::GetPropertyByIndex (int aIndex,
                                            NPVariant *_result)
{
  TOTEM_LOG_GETTER (aIndex, totemConePlaylistItems);

  switch (Properties (aIndex)) {
    case eCount:
      TOTEM_WARN_GETTER_UNIMPLEMENTED (aIndex, totemConePlaylistItems);
      return Int32Variant (_result, 1);
  }

  return false;
}

bool
totemConePlaylistItems::SetPropertyByIndex (int aIndex,
                                            const NPVariant *aValue)
{
  TOTEM_LOG_SETTER (aIndex, totemConePlaylistItems);

  return ThrowPropertyNotWritable ();
}
