/*
 * Copyright © 1998 Netscape Communications Corporation.
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
 *
 * ===========================================================================
 * Derived from MPL/LGPL/GPL tri-licensed code
 * [mozilla/odules/plugin/samples/4x-scriptable/npn_gate.cpp];
 * used here under LGPL 2+, as permitted by the relicensing clause.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright © 1998
 * the Initial Developer. All Rights Reserved.
 */

#include <config.h>

#include <string.h>

#include "npapi.h"
#include "npupp.h"

#ifndef HIBYTE
#define HIBYTE(x) ((((uint32_t)(x)) & 0xff00) >> 8)
#endif

#ifndef LOBYTE
#define LOBYTE(W) ((W) & 0xFF)
#endif

extern NPNetscapeFuncs NPNFuncs;

void NPN_Version(int* plugin_major, int* plugin_minor, int* netscape_major, int* netscape_minor)
{
  *plugin_major   = NP_VERSION_MAJOR;
  *plugin_minor   = NP_VERSION_MINOR;
  *netscape_major = HIBYTE(NPNFuncs.version);
  *netscape_minor = LOBYTE(NPNFuncs.version);
}

NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target, void* notifyData)
{
  return NPNFuncs.geturlnotify(instance, url, target, notifyData);
}

NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
  return NPNFuncs.geturl(instance, url, target);
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file, void* notifyData)
{
  return NPNFuncs.posturlnotify(instance, url, window, len, buf, file, notifyData);
}

NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file)
{
  return NPNFuncs.posturl(instance, url, window, len, buf, file);
} 

NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
  return NPNFuncs.requestread(stream, rangeList);
}

NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{
  return NPNFuncs.newstream(instance, type, target, stream);
}

int32_t NPN_Write(NPP instance, NPStream *stream, int32_t len, void *buffer)
{
  return NPNFuncs.write(instance, stream, len, buffer);
}

NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
  return NPNFuncs.destroystream(instance, stream, reason);
}

void NPN_Status(NPP instance, const char *message)
{
  NPNFuncs.status(instance, message);
}

const char* NPN_UserAgent(NPP instance)
{
  return NPNFuncs.uagent(instance);
}

void* NPN_MemAlloc(uint32 size)
{
  return NPNFuncs.memalloc(size);
}

void NPN_MemFree(void* ptr)
{
  /* Make it null-safe */
  if (!ptr)
    return;

  NPNFuncs.memfree(ptr);
}

uint32_t NPN_MemFlush(uint32_t size)
{
  return NPNFuncs.memflush(size);
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
  NPNFuncs.reloadplugins(reloadPages);
}

JRIEnv* NPN_GetJavaEnv(void)
{
  return NPNFuncs.getJavaEnv();
}

jref NPN_GetJavaPeer(NPP instance)
{
  return NPNFuncs.getJavaPeer(instance);
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
  return NPNFuncs.getvalue(instance, variable, value);
}

NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
  return NPNFuncs.setvalue(instance, variable, value);
}

void NPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
  NPNFuncs.invalidaterect(instance, invalidRect);
}

void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
  NPNFuncs.invalidateregion(instance, invalidRegion);
}

void NPN_ForceRedraw(NPP instance)
{
  NPNFuncs.forceredraw(instance);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name)
{
  return NPNFuncs.getstringidentifier(name);
}

void NPN_GetStringIdentifiers(const NPUTF8 **names, int32_t nameCount,
                              NPIdentifier *identifiers)
{
  return NPNFuncs.getstringidentifiers(names, nameCount, identifiers);
}

NPIdentifier NPN_GetStringIdentifier(int32_t intid)
{
  return NPNFuncs.getintidentifier(intid);
}

bool NPN_IdentifierIsString(NPIdentifier identifier)
{
  return NPNFuncs.identifierisstring(identifier);
}

NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
  return NPNFuncs.utf8fromidentifier(identifier);
}

int32_t NPN_IntFromIdentifier(NPIdentifier identifier)
{
  return NPNFuncs.intfromidentifier(identifier);
}

NPObject *NPN_CreateObject(NPP npp, NPClass *aClass)
{
  return NPNFuncs.createobject(npp, aClass);
}

NPObject *NPN_RetainObject(NPObject *obj)
{
  return NPNFuncs.retainobject(obj);
}

void NPN_ReleaseObject(NPObject *obj)
{
  return NPNFuncs.releaseobject(obj);
}

bool NPN_Invoke(NPP npp, NPObject* obj, NPIdentifier methodName,
                const NPVariant *args, uint32_t argCount, NPVariant *result)
{
  return NPNFuncs.invoke(npp, obj, methodName, args, argCount, result);
}

bool NPN_InvokeDefault(NPP npp, NPObject* obj, const NPVariant *args,
                       uint32_t argCount, NPVariant *result)
{
  return NPNFuncs.invokeDefault(npp, obj, args, argCount, result);
}

bool NPN_Evaluate(NPP npp, NPObject* obj, NPString *script,
                  NPVariant *result)
{
  return NPNFuncs.evaluate(npp, obj, script, result);
}

bool NPN_GetProperty(NPP npp, NPObject* obj, NPIdentifier propertyName,
                     NPVariant *result)
{
  return NPNFuncs.getproperty(npp, obj, propertyName, result);
}

bool NPN_SetProperty(NPP npp, NPObject* obj, NPIdentifier propertyName,
                     const NPVariant *value)
{
  return NPNFuncs.setproperty(npp, obj, propertyName, value);
}

bool NPN_RemoveProperty(NPP npp, NPObject* obj, NPIdentifier propertyName)
{
  return NPNFuncs.removeproperty(npp, obj, propertyName);
}

bool NPN_Enumerate(NPP npp, NPObject *obj, NPIdentifier **identifier,
                   uint32_t *count)
{
#if defined(NPVERS_HAS_NPOBJECT_ENUM) && (NP_VERSION_MINOR >= NPVERS_HAS_NPOBJECT_ENUM)
  if ((NPNFuncs.version & 0xFF) >= NPVERS_HAS_NPOBJECT_ENUM)
    return NPNFuncs.enumerate(npp, obj, identifier, count);
#endif
  return false;
}

bool NPN_Construct(NPP npp, NPObject *obj, const NPVariant *args,
                   uint32_t argCount, NPVariant *result)
{
#if defined(NPVERS_HAS_NPOBJECT_ENUM) && (NP_VERSION_MINOR >= NPVERS_HAS_NPOBJECT_ENUM)
  if ((NPNFuncs.version & 0xFF) >= NPVERS_HAS_NPOBJECT_ENUM)
    return NPNFuncs.construct(npp, obj, args, argCount, result);
#endif
  return false;
}

bool NPN_HasProperty(NPP npp, NPObject* obj, NPIdentifier propertyName)
{
  return NPNFuncs.hasproperty(npp, obj, propertyName);
}

bool NPN_HasMethod(NPP npp, NPObject* obj, NPIdentifier methodName)
{
  return NPNFuncs.hasmethod(npp, obj, methodName);
}

void NPN_ReleaseVariantValue(NPVariant *variant)
{
  NPNFuncs.releasevariantvalue(variant);
}

void NPN_SetException(NPObject* obj, const NPUTF8 *message)
{
  NPNFuncs.setexception(obj, message);
}

void* NPN_MemDup (const void* aMem, uint32 aLen)
{
  if (!aMem || !aLen)
    return NULL;

  void* dup = NPN_MemAlloc (aLen);
  if (!dup)
    return NULL;

  return memcpy (dup, aMem, aLen);
}

char* NPN_StrDup (const char *aString)
{
  if (!aString)
    return NULL;

  return NPN_StrnDup (aString, strlen (aString));
}

char* NPN_StrnDup (const char *aString, uint32 aLen)
{
  if (!aString)
    return NULL;

  if (aLen < 0)
    aLen = strlen (aString);

  char* dup = (char*) NPN_MemAlloc (aLen + 1);
  if (!dup)
    return NULL;

  memcpy (dup, aString, aLen);
  dup[aLen] = '\0';

  return dup;
}
