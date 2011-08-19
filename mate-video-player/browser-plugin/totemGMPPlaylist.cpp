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

#include "totemGMPPlaylist.h"

static const char *propertyNames[] = {
  "attributeCount",
  "count",
  "name"
};

static const char *methodNames[] = {
  "appendItem",
  "attributeName",
  "getAttributeName",
  "getItemInfo",
  "insertItem",
  "isIdentical",
  "item",
  "moveItem",
  "removeItem",
  "setItemInfo"
};

TOTEM_IMPLEMENT_NPCLASS (totemGMPPlaylist,
                         propertyNames, G_N_ELEMENTS (propertyNames),
                         methodNames, G_N_ELEMENTS (methodNames),
                         NULL);

totemGMPPlaylist::totemGMPPlaylist (NPP aNPP)
  : totemNPObject (aNPP),
    mName (NPN_StrDup ("Playlist"))
{
  TOTEM_LOG_CTOR ();
}

totemGMPPlaylist::~totemGMPPlaylist ()
{
  TOTEM_LOG_DTOR ();

  g_free (mName);
}

bool
totemGMPPlaylist::InvokeByIndex (int aIndex,
                                 const NPVariant *argv,
                                 uint32_t argc,
                                 NPVariant *_result)
{
  TOTEM_LOG_INVOKE (aIndex, totemGMPPlaylist);

  switch (Methods (aIndex)) {
    case eAttributeName:
      /* AUTF8String attributeName (in long index); */
    case eGetAttributeName:
      /* AUTF8String getAttributeName (in long index); */
    case eGetItemInfo:
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemGMPPlaylist);
      /* AUTF8String getItemInfo (in AUTF8String name); */
      return StringVariant (_result, "");

    case eIsIdentical: {
      /* boolean isIdentical (in totemIGMPPlaylist playlist); */
      NPObject *other;
      if (!GetObjectFromArguments (argv, argc, 0, other))
        return false;

      return BoolVariant (_result, other == static_cast<NPObject*>(this));
    }

    case eItem:
      /* totemIGMPMedia item (in long index); */
      TOTEM_WARN_1_INVOKE_UNIMPLEMENTED (aIndex, totemGMPPlaylist);
      return NullVariant (_result);

    case eAppendItem:
      /* void appendItem (in totemIGMPMedia item); */
    case eInsertItem:
      /* void insertItem (in long index, in totemIGMPMedia item); */
    case eMoveItem:
      /* void moveItem (in long oldIndex, in long newIndex); */
    case eRemoveItem:
      /* void removeItem (in totemIGMPMedia item); */
    case eSetItemInfo:
      /* void setItemInfo (in AUTF8String name, in AUTF8String value); */
      TOTEM_WARN_INVOKE_UNIMPLEMENTED (aIndex, totemGMPPlaylist);
      return VoidVariant (_result);
  }

  return false;
}

bool
totemGMPPlaylist::GetPropertyByIndex (int aIndex,
                                      NPVariant *_result)
{
  TOTEM_LOG_GETTER (aIndex, totemGMPPlaylist);

  switch (Properties (aIndex)) {
    case eAttributeCount:
      /* readonly attribute long attributeCount; */
    case eCount:
      /* readonly attribute long count; */
      return Int32Variant (_result, 0);

    case eName:
      /* attribute AUTF8String name; */
      return StringVariant (_result, mName);
  }

  return false;
}

bool
totemGMPPlaylist::SetPropertyByIndex (int aIndex,
                                      const NPVariant *aValue)
{
  TOTEM_LOG_SETTER (aIndex, totemGMPPlaylist);

  switch (Properties (aIndex)) {
    case eName:
      /* attribute AUTF8String name; */
      return DupStringFromArguments (aValue, 1, 0, mName);

    case eAttributeCount:
      /* readonly attribute long attributeCount; */
    case eCount:
      /* readonly attribute long count; */
      return ThrowPropertyNotWritable ();
  }

  return false;
}
