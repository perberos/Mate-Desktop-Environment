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

#ifndef __TOTEM_NPVARIANT_WRAPPER_H__
#define __TOTEM_NPVARIANT_WRAPPER_H__

#include <assert.h>
#include <string.h>

#include "npapi.h"
#include "npruntime.h"

class totemNPVariantWrapper {

  public:

    totemNPVariantWrapper ()                  : mOwned (false) { VOID_TO_NPVARIANT (mVariant); }
    totemNPVariantWrapper (bool aValue)       : mOwned (false) { BOOLEAN_TO_NPVARIANT (aValue, mVariant); }
    totemNPVariantWrapper (uint32_t aValue)   : mOwned (false) { INT32_TO_NPVARIANT (aValue, mVariant);   }
    totemNPVariantWrapper (double aValue)     : mOwned (false) { DOUBLE_TO_NPVARIANT (aValue, mVariant);  }
    totemNPVariantWrapper (char *aValue)      : mOwned (false) { STRINGZ_TO_NPVARIANT (aValue, mVariant); }
//     totemNPVariantWrapper (NPString *aValue)  : mOwned (false) { STRINGN_TO_NPVARIANT (aValue, mVariant); }
    totemNPVariantWrapper (NPObject *aObject) : mOwned (false) { OBJECT_TO_NPVARIANT (aObject, mVariant); }

    totemNPVariantWrapper (const totemNPVariantWrapper& aOther) : mVariant (aOther.mVariant), mOwned (false) { }

    ~totemNPVariantWrapper () { Clear (); }

    bool IsVoid    () const { return NPVARIANT_IS_VOID (mVariant);    }
    bool IsNull    () const { return NPVARIANT_IS_NULL (mVariant);    }
    bool IsBoolean () const { return NPVARIANT_IS_BOOLEAN (mVariant); }
    bool IsInt32   () const { return NPVARIANT_IS_INT32 (mVariant);   }
    bool IsDouble  () const { return NPVARIANT_IS_DOUBLE (mVariant);  }
    bool IsString  () const { return NPVARIANT_IS_STRING (mVariant);  }
    bool IsObject  () const { return NPVARIANT_IS_OBJECT (mVariant);  }

    bool      GetBoolean  () const { return NPVARIANT_TO_BOOLEAN (mVariant); }
    uint32_t  GetInt32    () const { return NPVARIANT_TO_INT32 (mVariant);   }
    double    GetDouble   () const { return NPVARIANT_TO_DOUBLE (mVariant);  }
    char *    GetString   () const { return (char *) NPVARIANT_TO_STRING (mVariant).UTF8Characters;  }
    uint32_t  GetStringLen() const { return NPVARIANT_TO_STRING (mVariant).UTF8Length; }
    NPString  GetNPString () const { return NPVARIANT_TO_STRING (mVariant);  }
    NPObject* GetObject   () const { return NPVARIANT_TO_OBJECT (mVariant);  }

    void SetVoid    ()                  { Clear (); VOID_TO_NPVARIANT (mVariant);            }
    void SetNull    ()                  { Clear (); NULL_TO_NPVARIANT (mVariant);            }
    void SetBoolean (bool aValue)       { Clear (); BOOLEAN_TO_NPVARIANT (aValue, mVariant); }
    void SetInt32   (uint32_t aValue)   { Clear (); INT32_TO_NPVARIANT (aValue, mVariant);   }
    void SetDouble  (double aValue)     { Clear (); DOUBLE_TO_NPVARIANT (aValue, mVariant);  }
    void SetString  (char *aValue)      { Clear (); STRINGZ_TO_NPVARIANT (aValue, mVariant); }
//     void SetString  (NPString *aValue)  { Clear (); STRINGN_TO_NPVARIANT (aValue, mVariant); }
    void SetObject  (NPObject *aObject) { Clear (); OBJECT_TO_NPVARIANT (aObject, mVariant); }

    operator char*     () { return GetString   (); }
    operator NPString  () { return GetNPString (); }
    operator NPObject* () { return GetObject   (); }

    operator NPVariant*() { return &mVariant; }
    
    class GetterCopies {
      public:
       explicit GetterCopies (totemNPVariantWrapper& aTarget) : mTarget (aTarget) { }
        ~GetterCopies () { }

       operator NPVariant*() { return mTarget.StartAssignment (); }

      private:
        totemNPVariantWrapper& mTarget;
    };

  private:

    totemNPVariantWrapper& operator= (const totemNPVariantWrapper&); // not implemented

    void Clear () {
      if (mOwned) {
        NPN_ReleaseVariantValue (&mVariant);
        mOwned = false;
      } else {
        VOID_TO_NPVARIANT (mVariant);
      }
    }

    NPVariant* StartAssignment () { Clear (); mOwned = true; return &mVariant; }

  protected:
    NPVariant mVariant;
    bool mOwned;
};

inline totemNPVariantWrapper::GetterCopies
getter_Copies (totemNPVariantWrapper &aTarget)
{
  return totemNPVariantWrapper::GetterCopies (aTarget);
}

#endif /* __TOTEM_NPVARIANT_WRAPPER_H__ */
