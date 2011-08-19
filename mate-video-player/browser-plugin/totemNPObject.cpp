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
#include <stdio.h>
#include <stdarg.h>

#include <glib.h>

#include "totemNPClass.h"
#include "totemNPObject.h"

#ifdef DEBUG_PLUGIN
#define NOTE(x) x
#else
#define NOTE(x)
#endif

static const char *variantTypes[] = {
  "void",
  "null",
  "bool",
  "int32",
  "double",
  "string",
  "object",
  "unknown"
};

#define VARIANT_TYPE(type) (variantTypes[MIN (type, NPVariantType_Object + 1)])

void*
totemNPObject::operator new (size_t aSize) throw ()
{
  void *instance = ::operator new (aSize);
  if (instance) {
    memset (instance, 0, aSize);
  }

  return instance;
}

totemNPObject::totemNPObject (NPP aNPP)
  : mNPP (aNPP),
    mPlugin (reinterpret_cast<totemPlugin*>(aNPP->pdata))
{
  NOTE (g_print ("totemNPObject ctor [%p]\n", (void*) this));
}

totemNPObject::~totemNPObject ()
{
  NOTE (g_print ("totemNPObject dtor [%p]\n", (void*) this));
}

bool
totemNPObject::Throw (const char *aMessage)
{
  NOTE (g_print ("totemNPObject::Throw [%p] : %s\n", (void*) this, aMessage));

  NPN_SetException (this, aMessage);
  return false;
}

bool
totemNPObject::ThrowPropertyNotWritable ()
{
  return Throw ("Property not writable");
}

bool
totemNPObject::ThrowSecurityError ()
{
  return Throw ("Access denied");
}

bool
totemNPObject::CheckArgc (uint32_t argc,
                          uint32_t minArgc,
                          uint32_t maxArgc,
                          bool doThrow)
{
  if (argc >= minArgc && argc <= maxArgc)
    return true;

  if (argc < minArgc) {
    if (doThrow)
      return Throw ("Not enough arguments");

    return false;
  }

  if (doThrow)
    return Throw ("Too many arguments");

  return false;
}

bool
totemNPObject::CheckArgType (NPVariantType argType,
                             NPVariantType expectedType,
                             uint32_t argNum)
{
  bool conforms;

  switch (argType) {
    case NPVariantType_Void:
    case NPVariantType_Null:
      conforms = (argType == expectedType);
      break;

    case NPVariantType_Bool:
      conforms = (argType == NPVariantType_Bool ||
                  argType == NPVariantType_Int32 ||
                  argType == NPVariantType_Double);
      break;

    case NPVariantType_Int32:
    case NPVariantType_Double:
      /* FIXMEchpe: also accept NULL or VOID ? */
      conforms = (argType == NPVariantType_Int32 ||
                  argType == NPVariantType_Double);
      break;

    case NPVariantType_String:
    case NPVariantType_Object:
      conforms = (argType == expectedType ||
                  argType == NPVariantType_Null ||
                  argType == NPVariantType_Void);
      break;
    default:
      conforms = false;
  }

  if (!conforms) {
      char msg[128];
      g_snprintf (msg, sizeof (msg),
                  "Wrong type of argument %d: expected %s but got %s\n",
                  argNum, VARIANT_TYPE (expectedType), VARIANT_TYPE (argType));

      return Throw (msg);
  }

  return true;
}

bool
totemNPObject::CheckArg (const NPVariant *argv,
                         uint32_t argc,
                         uint32_t argNum,
                         NPVariantType type)
{
  if (!CheckArgc (argc, argNum + 1))
    return false;

  return CheckArgType (argv[argNum].type, type, argNum);
}

bool
totemNPObject::CheckArgv (const NPVariant* argv,
                          uint32_t argc,
                          uint32_t expectedArgc,
                          ...)
{
  if (!CheckArgc (argc, expectedArgc, expectedArgc))
    return false;

  va_list type_args;
  va_start (type_args, expectedArgc);

  for (uint32_t i = 0; i < argc; ++i) {
    NPVariantType type = NPVariantType (va_arg (type_args, int /* promotion */));

    if (!CheckArgType (argv[i].type, type)) {
      va_end (type_args);
      return false;
    }
  }

  va_end (type_args);

  return true;
}

bool
totemNPObject::GetBoolFromArguments (const NPVariant* argv,
                                     uint32_t argc,
                                     uint32_t argNum,
                                     bool& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_Bool))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_BOOLEAN (arg)) {
    _result = NPVARIANT_TO_BOOLEAN (arg);
  } else if (NPVARIANT_IS_INT32 (arg)) {
    _result = NPVARIANT_TO_INT32 (arg) != 0;
  } else if (NPVARIANT_IS_DOUBLE (arg)) {
    _result = NPVARIANT_TO_DOUBLE (arg) != 0.0;
  } else {
    /* void/null */
    _result = false;
  }

  return true;
}

bool
totemNPObject::GetInt32FromArguments (const NPVariant* argv,
                                      uint32_t argc,
                                      uint32_t argNum,
                                      int32_t& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_Int32))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_INT32 (arg)) {
    _result = NPVARIANT_TO_INT32 (arg);
  } else if (NPVARIANT_IS_DOUBLE (arg)) {
    _result = int32_t (NPVARIANT_TO_DOUBLE (arg));
    /* FIXMEchpe: overflow? */
  }

  return true;
}

bool
totemNPObject::GetDoubleFromArguments (const NPVariant* argv,
                                       uint32_t argc,
                                       uint32_t argNum,
                                       double& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_Double))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_DOUBLE (arg)) {
    _result = NPVARIANT_TO_DOUBLE (arg);
  } else if (NPVARIANT_IS_INT32 (arg)) {
    _result = double (NPVARIANT_TO_INT32 (arg));
  }

  return true;
}

bool
totemNPObject::GetNPStringFromArguments (const NPVariant* argv,
                                         uint32_t argc,
                                         uint32_t argNum,
                                         NPString& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_String))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_STRING (arg)) {
    _result = NPVARIANT_TO_STRING (arg);
  } else if (NPVARIANT_IS_NULL (arg) ||
             NPVARIANT_IS_VOID (arg)) {
    _result.UTF8Characters = NULL;
    _result.UTF8Length = 0;
  }

  return true;
}

bool
totemNPObject::DupStringFromArguments (const NPVariant* argv,
                                       uint32_t argc,
                                       uint32_t argNum,
                                       char*& _result)
{
  NPN_MemFree (_result);
  _result = NULL;

  NPString newValue;
  if (!GetNPStringFromArguments (argv, argc, argNum, newValue))
    return false;

  _result = NPN_StrnDup (newValue.UTF8Characters, newValue.UTF8Length);
  return true;
}

bool
totemNPObject::GetObjectFromArguments (const NPVariant* argv,
                                        uint32_t argc,
                                        uint32_t argNum,
                                        NPObject*& _result)
{
  if (!CheckArg (argv, argc, argNum, NPVariantType_Object))
    return false;

  NPVariant arg = argv[argNum];
  if (NPVARIANT_IS_STRING (arg)) {
    _result = NPVARIANT_TO_OBJECT (arg);
  } else if (NPVARIANT_IS_NULL (arg) ||
             NPVARIANT_IS_VOID (arg)) {
    _result = NULL;
  }

  return true;
}

bool
totemNPObject::VoidVariant (NPVariant* _result)
{
  VOID_TO_NPVARIANT (*_result);
  return true;
}

bool
totemNPObject::NullVariant (NPVariant* _result)
{
  NULL_TO_NPVARIANT (*_result);
  return true;
}

bool
totemNPObject::BoolVariant (NPVariant* _result,
                            bool value)
{
  BOOLEAN_TO_NPVARIANT (value, *_result);
  return true;
}

bool
totemNPObject::Int32Variant (NPVariant* _result,
                             int32_t value)
{
  INT32_TO_NPVARIANT (value, *_result);
  return true;
}

bool
totemNPObject::DoubleVariant (NPVariant* _result,
                              double value)
{
  DOUBLE_TO_NPVARIANT (value, *_result);
  return true;
}

bool
totemNPObject::StringVariant (NPVariant* _result,
                              const char* value,
                              int32_t len)
{
  if (!value) {
    NULL_TO_NPVARIANT (*_result);
  } else {
    char *dup;

    if (len < 0) {
      len = strlen (value);
      dup = (char*) NPN_MemDup (value, len + 1);
    } else {
      dup = (char*) NPN_MemDup (value, len);
    }

    if (dup) {
      STRINGN_TO_NPVARIANT (dup, len, *_result);
    } else {
      NULL_TO_NPVARIANT (*_result);
    }
  }

  return true;
}

bool
totemNPObject::ObjectVariant (NPVariant* _result,
                              NPObject* object)
{
  if (object) {
    NPN_RetainObject (object);
    OBJECT_TO_NPVARIANT (object, *_result);
  } else {
    NULL_TO_NPVARIANT (*_result);
  }

  return true;
}

/* NPObject method default implementations */

void
totemNPObject::Invalidate ()
{
  NOTE (g_print ("totemNPObject %p invalidated\n", (void*) this));

  mNPP = NULL;
  mPlugin = NULL;
}

bool
totemNPObject::HasMethod (NPIdentifier aName)
{
  if (!IsValid ())
    return false;

  int methodIndex = GetClass()->GetMethodIndex (aName);
  NOTE (g_print ("totemNPObject::HasMethod [%p] %s => %s\n", (void*) this, NPN_UTF8FromIdentifier (aName), methodIndex >= 0 ? "yes" : "no"));
  if (methodIndex >= 0)
    return true;

  if (aName == NPN_GetStringIdentifier ("__noSuchMethod__"))
    return true;

  return false;
}

bool
totemNPObject::Invoke (NPIdentifier aName,
                       const NPVariant *argv,
                       uint32_t argc,
                       NPVariant *_result)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("totemNPObject::Invoke [%p] %s\n", (void*) this, NPN_UTF8FromIdentifier (aName)));
  int methodIndex = GetClass()->GetMethodIndex (aName);
  if (methodIndex >= 0)
    return InvokeByIndex (methodIndex, argv, argc, _result);

  if (aName == NPN_GetStringIdentifier ("__noSuchMethod__")) {
    /* http://developer.mozilla.org/en/docs/Core_JavaScript_1.5_Reference:Global_Objects:Object:_noSuchMethod */
    if (!CheckArgv (argv, argc, 2, NPVariantType_String, NPVariantType_Object))
      return false;

    const char *id = NPVARIANT_TO_STRING (argv[0]).UTF8Characters;
    g_message ("NOTE: site calls unknown function \"%s\" on totemNPObject %p\n", id ? id : "(null)", (void*) this);

    /* Silently ignore the invocation */
    VOID_TO_NPVARIANT (*_result);
    return true;
  }

  return Throw ("No method with this name exists.");
}

bool
totemNPObject::InvokeDefault (const NPVariant *argv,
                              uint32_t argc,
                              NPVariant *_result)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("totemNPObject::InvokeDefault [%p]\n", (void*) this));
  int defaultMethodIndex = GetClass()->GetDefaultMethodIndex ();
  if (defaultMethodIndex >= 0)
    return InvokeByIndex (defaultMethodIndex, argv, argc, _result);

  return false;
}

bool
totemNPObject::HasProperty (NPIdentifier aName)
{
  if (!IsValid ())
    return false;

  int propertyIndex = GetClass()->GetPropertyIndex (aName);
  NOTE (g_print ("totemNPObject::HasProperty [%p] %s => %s\n", (void*) this, NPN_UTF8FromIdentifier (aName), propertyIndex >= 0 ? "yes" : "no"));
  if (propertyIndex >= 0)
    return true;

  return false;
}

bool
totemNPObject::GetProperty (NPIdentifier aName,
                            NPVariant *_result)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("totemNPObject::GetProperty [%p] %s\n", (void*) this, NPN_UTF8FromIdentifier (aName)));
  int propertyIndex = GetClass()->GetPropertyIndex (aName);
  if (propertyIndex >= 0)
    return GetPropertyByIndex (propertyIndex, _result);

  return Throw ("No property with this name exists.");
}

bool
totemNPObject::SetProperty (NPIdentifier aName,
                            const NPVariant *aValue)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("totemNPObject::SetProperty [%p] %s\n", (void*) this, NPN_UTF8FromIdentifier (aName)));
  int propertyIndex = GetClass()->GetPropertyIndex (aName);
  if (propertyIndex >= 0)
    return SetPropertyByIndex (propertyIndex, aValue);

  return Throw ("No property with this name exists.");
}

bool
totemNPObject::RemoveProperty (NPIdentifier aName)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("totemNPObject::RemoveProperty [%p] %s\n", (void*) this, NPN_UTF8FromIdentifier (aName)));
  int propertyIndex = GetClass()->GetPropertyIndex (aName);
  if (propertyIndex >= 0)
    return RemovePropertyByIndex (propertyIndex);

  return Throw ("No property with this name exists.");
}

bool
totemNPObject::Enumerate (NPIdentifier **_result,
                          uint32_t *_count)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("totemNPObject::Enumerate [%p]\n", (void*) this));
  return GetClass()->EnumerateProperties (_result, _count);
}

bool
totemNPObject::Construct (const NPVariant *argv,
                          uint32_t argc,
                          NPVariant *_result)
{
  if (!IsValid ())
    return false;

  NOTE (g_print ("totemNPObject::Construct [%p]\n", (void*) this));
  return false; /* FIXMEchpe! */
}

/* by-index methods */

bool
totemNPObject::InvokeByIndex (int aIndex,
                              const NPVariant *argv,
                              uint32_t argc,
                              NPVariant *_result)
{
  return false;
}

bool
totemNPObject::GetPropertyByIndex (int aIndex,
                                   NPVariant *_result)
{
  return false;
}

bool
totemNPObject::SetPropertyByIndex (int aIndex,
                                   const NPVariant *aValue)
{
  return false;
}

bool
totemNPObject::RemovePropertyByIndex (int aIndex)
{
  return Throw ("Removing properties is not supported.");
}
