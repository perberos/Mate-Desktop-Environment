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

#ifndef __TOTEM_NPCLASS_H__
#define __TOTEM_NPCLASS_H__

#include <assert.h>

#include "npapi.h"
#include "npruntime.h"

class totemNPObject;

class totemNPClass_base : public NPClass {

  public:

    NPObject* CreateInstance (NPP aNPP) {
      return NPN_CreateObject (aNPP, this);
    }

  protected:
    friend class totemNPObject;

    totemNPClass_base (const char *aPropertNames[],
                       uint32_t aPropertyCount,
                       const char *aMethodNames[],
                       uint32_t aMethodCount,
                       const char *aDefaultMethodName);
    virtual ~totemNPClass_base ();

    virtual NPObject* InternalCreate (NPP aNPP) = 0;

    int GetPropertyIndex (NPIdentifier aName);
    int GetMethodIndex   (NPIdentifier aName);
    int GetDefaultMethodIndex () const { return mDefaultMethodIndex; }
    bool EnumerateProperties (NPIdentifier **_result, uint32_t *_count);

  private:

    static NPObject* Allocate  (NPP aNPP, NPClass *aClass);
    static void Deallocate     (NPObject *aObject);
    static void Invalidate     (NPObject *aObject);
    static bool HasMethod      (NPObject *aObject, NPIdentifier aName);
    static bool Invoke         (NPObject *aObject, NPIdentifier aName, const NPVariant *argv, uint32_t argc, NPVariant *_result);
    static bool InvokeDefault  (NPObject *aObject, const NPVariant *argv, uint32_t argc, NPVariant *_result);
    static bool HasProperty    (NPObject *aObject, NPIdentifier aName);
    static bool GetProperty    (NPObject *aObject, NPIdentifier aName, NPVariant *_result);
    static bool SetProperty    (NPObject *aObject, NPIdentifier aName, const NPVariant *aValue);
    static bool RemoveProperty (NPObject *aObject, NPIdentifier aName);
    static bool Enumerate      (NPObject *aObject, NPIdentifier **_result, uint32_t *_count);
    static bool Construct      (NPObject *aObject, const NPVariant *argv, uint32_t argc, NPVariant *_result);

    NPIdentifier* GetIdentifiersForNames (const char *aNames[], uint32_t aCount);

    NPIdentifier *mPropertyNameIdentifiers;
    int mPropertyNamesCount;
    NPIdentifier *mMethodNameIdentifiers;
    int mMethodNamesCount;
    int mDefaultMethodIndex;
};

template <class T>
class totemNPClass : public totemNPClass_base {
  
  public:

    typedef totemNPClass<T> class_type;

    totemNPClass (const char *aPropertNames[],
                  uint32_t aPropertyCount,
                  const char *aMethodNames[],
                  uint32_t aMethodCount,
                  const char *aDefaultMethodName) :
      totemNPClass_base (aPropertNames, aPropertyCount,
                         aMethodNames, aMethodCount,
                         aDefaultMethodName) {
    }

    virtual ~totemNPClass () { }

  protected:

    virtual NPObject* InternalCreate (NPP aNPP) {
      return new T (aNPP);
    }
};

/* Helper macros */

#define TOTEM_DEFINE_NPCLASS(T) \
class T##NPClass : public totemNPClass<T> {\
\
  public:\
\
    T##NPClass () throw ();\
    virtual ~T##NPClass ();\
\
    static class_type* Instance () throw ();\
    static void Shutdown ();\
\
  private:\
    static class_type* sInstance;\
}

#define TOTEM_IMPLEMENT_NPCLASS(T, propertyNames, propertyNamesCount, methodNames, methodNamesCount, defaultMethodName) \
\
T##NPClass::class_type* T##NPClass::sInstance = 0; \
\
T##NPClass::T##NPClass () throw ()\
  : totemNPClass<T> (propertyNames,\
                     propertyNamesCount,\
                     methodNames,\
                     methodNamesCount,\
                     defaultMethodName)\
{\
}\
\
T##NPClass::~T##NPClass ()\
{\
}\
\
T##NPClass::class_type* \
T##NPClass::Instance () throw ()\
{\
  if (!sInstance) {\
    sInstance = new T##NPClass ();\
  }\
\
  return sInstance;\
}\
\
void \
T##NPClass::Shutdown ()\
{\
  delete sInstance;\
  sInstance = 0;\
}

#endif /* __TOTEM_NPCLASS_H__ */
