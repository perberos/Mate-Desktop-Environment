/*
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>

#include "totemNPClass.h"
#include "totemNPObject.h"

totemNPClass_base::totemNPClass_base (const char *aPropertNames[],
                                      uint32_t aPropertyCount,
                                      const char *aMethodNames[],
                                      uint32_t aMethodCount,
                                      const char *aDefaultMethodName) :
  mPropertyNameIdentifiers (GetIdentifiersForNames (aPropertNames, aPropertyCount)),
  mPropertyNamesCount (aPropertyCount),
  mMethodNameIdentifiers (GetIdentifiersForNames (aMethodNames, aMethodCount)),
  mMethodNamesCount (aMethodCount),
  mDefaultMethodIndex (aDefaultMethodName ? GetMethodIndex (NPN_GetStringIdentifier (aDefaultMethodName)) : -1)
{
  structVersion  = NP_CLASS_STRUCT_VERSION_ENUM;
  allocate       = Allocate;
  deallocate     = Deallocate;
  invalidate     = Invalidate;
  hasMethod      = HasMethod;
  invoke         = Invoke;
  invokeDefault  = InvokeDefault;
  hasProperty    = HasProperty;
  getProperty    = GetProperty;
  setProperty    = SetProperty;
  removeProperty = RemoveProperty;
#if defined(NP_CLASS_STRUCT_VERSION_ENUM) && (NP_CLASS_STRUCT_VERSION >= NP_CLASS_STRUCT_VERSION_ENUM)
  enumerate      = Enumerate;
#endif
#if defined(NP_CLASS_STRUCT_VERSION_CTOR) && (NP_CLASS_STRUCT_VERSION >= NP_CLASS_STRUCT_VERSION_CTOR)
  /* FIXMEchpe find out what's this supposed to do */
  /* construct      = Construct; */
  construct = NULL;
#endif
}

totemNPClass_base::~totemNPClass_base ()
{
  NPN_MemFree (mPropertyNameIdentifiers);
  NPN_MemFree (mMethodNameIdentifiers);
}

NPIdentifier*
totemNPClass_base::GetIdentifiersForNames (const char *aNames[],
                                           uint32_t aCount)
{
  if (aCount == 0)
    return NULL;

  NPIdentifier *identifiers = reinterpret_cast<NPIdentifier*>(NPN_MemAlloc (aCount * sizeof (NPIdentifier)));
  if (!identifiers)
    return NULL;

  NPN_GetStringIdentifiers (aNames, aCount, identifiers);

  return identifiers;
}

int
totemNPClass_base::GetPropertyIndex (NPIdentifier aName)
{
  if (!mPropertyNameIdentifiers)
    return -1;

  for (int i = 0; i < mPropertyNamesCount; ++i) {
    if (aName == mPropertyNameIdentifiers[i])
      return i;
  }

  return -1;
}

int
totemNPClass_base::GetMethodIndex (NPIdentifier aName)
{
  if (!mMethodNameIdentifiers)
    return -1;

  for (int i = 0; i < mMethodNamesCount; ++i) {
    if (aName == mMethodNameIdentifiers[i])
      return i;
  }

  return -1;
}

bool
totemNPClass_base::EnumerateProperties (NPIdentifier **_result, uint32_t *_count)
{
  if (!mPropertyNameIdentifiers)
    return false;

  uint32_t bytes = mPropertyNamesCount * sizeof (NPIdentifier);
  NPIdentifier *identifiers = reinterpret_cast<NPIdentifier*>(NPN_MemAlloc (bytes));
  if (!identifiers)
    return false;

  memcpy (identifiers, mPropertyNameIdentifiers, bytes);

  *_result = identifiers;
  *_count = mPropertyNamesCount;

  return true;
}

NPObject*
totemNPClass_base::Allocate (NPP aNPP, NPClass *aClass)
{
  totemNPClass_base* _class = static_cast<totemNPClass_base*>(aClass);
  return _class->InternalCreate (aNPP);
}

void
totemNPClass_base::Deallocate (NPObject *aObject)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  delete object;
}

void
totemNPClass_base::Invalidate (NPObject *aObject)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  object->Invalidate ();
}

bool
totemNPClass_base::HasMethod (NPObject *aObject, NPIdentifier aName)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->HasMethod (aName);
}

bool
totemNPClass_base::Invoke (NPObject *aObject, NPIdentifier aName, const NPVariant *argv, uint32_t argc, NPVariant *_result)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->Invoke (aName, argv, argc, _result);
}

bool
totemNPClass_base::InvokeDefault (NPObject *aObject, const NPVariant *argv, uint32_t argc, NPVariant *_result)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->InvokeDefault (argv, argc, _result);
}

bool
totemNPClass_base::HasProperty (NPObject *aObject, NPIdentifier aName)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->HasProperty (aName);
}

bool
totemNPClass_base::GetProperty (NPObject *aObject, NPIdentifier aName, NPVariant *_result)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->GetProperty (aName, _result);
}

bool
totemNPClass_base::SetProperty (NPObject *aObject, NPIdentifier aName, const NPVariant *aValue)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->SetProperty (aName, aValue);
}

bool
totemNPClass_base::RemoveProperty (NPObject *aObject, NPIdentifier aName)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->RemoveProperty (aName);
}

bool
totemNPClass_base::Enumerate (NPObject *aObject, NPIdentifier **_result, uint32_t *_count)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->Enumerate (_result, _count);
}

bool
totemNPClass_base::Construct (NPObject *aObject, const NPVariant *argv, uint32_t argc, NPVariant *_result)
{
  totemNPObject* object = static_cast<totemNPObject*> (aObject);
  return object->Construct (argv, argc, _result);
}
