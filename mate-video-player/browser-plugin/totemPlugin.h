/* Totem browser plugin
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

#ifndef __TOTEM_PLUGIN_H__
#define __TOTEM_PLUGIN_H__

#include <stdint.h>
#include <dbus/dbus-glib.h>

#include "npapi.h"

#include "totemNPClass.h"
#include "totemNPObject.h"
#include "totemNPObjectWrapper.h"
#include "totemNPVariantWrapper.h"

#include "totem-plugin-viewer-constants.h"

#define TOTEM_NARROWSPACE_VERSION   "7.6.6"
#define TOTEM_MULLY_VERSION         "1.4.0.233"
#define TOTEM_CONE_VERSION          "0.8.6"
#define TOTEM_GMP_VERSION_BUILD     "11.0.0.1024"

typedef struct {
  const char *mimetype;
  const char *extensions;
  const char *mime_alias;
} totemPluginMimeEntry;

typedef enum {
	TOTEM_QUEUE_TYPE_SET_VOLUME,
	TOTEM_QUEUE_TYPE_CLEAR_PLAYLIST,
	TOTEM_QUEUE_TYPE_ADD_ITEM,
	TOTEM_QUEUE_TYPE_SET_BOOLEAN,
	TOTEM_QUEUE_TYPE_SET_STRING
} TotemQueueCommandType;

typedef struct {
	TotemQueueCommandType type;
	union {
		float volume;
		struct {
			char *uri;
			char *title;
			char *subtitle;
		} add_item;
		gboolean boolean;
		char *string;
	};
} TotemQueueCommand;

class totemBasicPlayer;
class totemConePlayer;
class totemGMPControls;
class totemGMPError;
class totemGMPPlayer;
class totemGMPSettings;
class totemMullYPlayer;
class totemNarrowSpacePlayer;

class totemPlugin {
  public:
    totemPlugin (NPP aNPP);
    ~totemPlugin ();
  
    void* operator new (size_t aSize) throw ();

    /* plugin glue */

    static NPError Initialise ();
    static NPError Shutdown ();

    NPError Init (NPMIMEType mimetype,
                  uint16_t mode,
                  int16_t argc,
                  char *argn[],
                  char *argv[],
                  NPSavedData *saved);
  
    NPError SetWindow (NPWindow *aWindow);
  
    NPError NewStream (NPMIMEType type,
                       NPStream* stream_ptr,
                       NPBool seekable,
                       uint16* stype);
    NPError DestroyStream (NPStream* stream,
                           NPError reason);
  
    int32_t WriteReady (NPStream *stream);
    int32_t Write (NPStream *stream,
                   int32_t offset,
                   int32_t len,
                   void *buffer);
    void StreamAsFile (NPStream *stream,
                       const char* fname);

    void URLNotify (const char *url,
		    NPReason reason,
		    void *notifyData);

    NPError GetScriptableNPObject (void *_retval);

    static char *PluginDescription ();
    static char *PluginLongDescription();
    static void PluginMimeTypes (const totemPluginMimeEntry **, uint32_t *);

  private:

    static void NameOwnerChangedCallback (DBusGProxy *proxy,
						      const char *svc,
						      const char *old_owner,
						      const char *new_owner,
						      void *aData);

    static gboolean ViewerForkTimeoutCallback (void *aData);

    static void ButtonPressCallback (DBusGProxy  *proxy,
						 guint aTimestamp,
		    				 guint aButton,
					         void *aData);

    static void StopStreamCallback (DBusGProxy  *proxy,
						void *aData);

    static void TickCallback (DBusGProxy  *proxy,
    					  guint aTime,
    					  guint aDuration,
    					  char *aState,
					  void *aData);
    static void PropertyChangeCallback (DBusGProxy  *proxy,
    						    const char *type,
						    GValue *value,
						    void *aData);

    static void ViewerSetWindowCallback (DBusGProxy *aProxy,
						     DBusGProxyCall *aCall,
						     void *aData);
    static void ViewerOpenStreamCallback (DBusGProxy *aProxy,
						      DBusGProxyCall *aCall,
						      void *aData);
    static void ViewerSetupStreamCallback (DBusGProxy *aProxy,
						      DBusGProxyCall *aCall,
						      void *aData);
    static void ViewerOpenURICallback (DBusGProxy *aProxy,
						   DBusGProxyCall *aCall,
						   void *aData);


    NPError ViewerFork ();
    void ViewerSetup ();
    void ViewerSetWindow ();
    void ViewerReady ();
    void ViewerCleanup ();

    void ViewerButtonPressed (guint aTimestamp,
		    	      guint aButton);
    void NameOwnerChanged (const char *aName,
			   const char *aOldOwner,
			   const char *aNewOwner);

    void ComputeRequest ();
    void ClearRequest ();
    void RequestStream (bool aForceViewer);
    void UnsetStream ();

    bool IsMimeTypeSupported (const char *aMimeType,
                              const char *aURL);
    bool IsSchemeSupported (const char *aURI, const char *aBaseURI);
    void SetRealMimeType (const char *aMimeType);
    bool ParseBoolean (const char *key,
                       const char *value,
                       bool default_val);
    bool GetBooleanValue (GHashTable *args,
                          const char *key,
                          bool default_val);
    uint32_t GetEnumIndex (GHashTable *args,
			   const char *key,
			   const char *values[],
			   uint32_t n_values,
			   uint32_t default_value);

    NPP mNPP;

    totemNPObjectWrapper mPluginElement;

    guint mTimerID;

    /* Stream data */
    NPStream *mStream;
  public:
    uint32_t mBytesStreamed;
    uint32_t mBytesLength;
  private:
    uint8_t mStreamType;

    char* mMimeType;

    char* mDocumentURI;
    char* mBaseURI;
    char* mSrcURI; /* relative to mBaseURI */
    char* mRequestBaseURI;
    char* mRequestURI; /* relative to mRequestBaseURI */

    DBusGConnection *mBusConnection;
    DBusGProxy *mBusProxy;
    DBusGProxy *mViewerProxy;
    DBusGProxyCall *mViewerPendingCall;
    char* mViewerBusAddress;
    char* mViewerServiceName;
    int mViewerPID;
    int mViewerFD;

    Window mWindow;
    int mWidth;
    int mHeight;

  private:

    bool mAllowContextMenu;
    bool mAudioOnly;
    bool mAutoPlay;
    bool mCache;
    bool mCheckedForPlaylist;
    bool mControllerHidden;
    bool mControllerVisible;
    bool mExpectingStream;
    bool mHadStream;
    bool mHidden;
    bool mIsFullscreen;
    bool mIsLooping;
    bool mIsMute;
    bool mIsPlaylist;
    bool mIsSupportedSrc;
    bool mIsWindowless;
    bool mKioskMode;
    bool mLoopIsPalindrome;
    bool mMute;
    bool mNeedViewer;
    bool mPlayEveryFrame;
    bool mRepeat;
    bool mRequestIsSrc;
    bool mResetPropertiesOnReload;
    bool mShowStatusbar;
    bool mTimerRunning;
    bool mUnownedViewerSetUp;
    bool mViewerReady;
    bool mViewerSetUp;
    bool mWaitingForButtonPress;
    bool mWindowSet;

    char *mBackgroundColor;
    char *mMatrix;
    char *mRectangle;
    char *mMovieName;

    double mVolume;

    TotemStates mState;

    uint32_t mDuration;
    uint32_t mTime;

    GQueue *mQueue;

#ifdef TOTEM_GMP_PLUGIN
  public:
    void SetURL (const char* aURL);
    void SetBaseURL (const char* aBaseURL);
    const char* URL() const { return mURLURI; }

  private:
    char* mURLURI;
#endif

#ifdef TOTEM_NARROWSPACE_PLUGIN
  public:
    bool SetQtsrc (const char* aURL);
    bool SetHref (const char* aURL);
    void SetURL (const NPString&);

    const char* QtSrc () const { return mQtsrcURI; }
    const char* Href () const { return mHref; }
    const char* Target () const { return mTarget; }

  private:
    bool ParseURLExtensions (const char* aString,
			     char* *_url,
			     char* *_target);

    void LaunchTotem (const char* aURL,
		      uint32_t aTimestamp);

    char* mQtsrcURI;
    char* mHref;
    char* mHrefURI;
    char* mTarget;
    bool mAutoHref;
#endif

  public:

    enum ObjectEnum {
      ePluginScriptable,
#if defined(TOTEM_GMP_PLUGIN)
      eGMPControls,
      eGMPNetwork,
      eGMPSettings,
#elif defined(TOTEM_CONE_PLUGIN)
      eConeAudio,
      eConeInput,
      eConePlaylist,
      eConePlaylistItems,
      eConeVideo,
#endif
      eLastNPObject
    };

  private:

    totemNPObjectWrapper mNPObjects[eLastNPObject];

  public:

    NPObject* GetNPObject (ObjectEnum which);

    bool SetSrc (const char* aURL);
    bool SetSrc (const NPString& aURL);
    const char* Src() const { return mSrcURI; }

    void Command (const char *aCommand);
    void ClearPlaylist ();
    int32_t AddItem (const NPString&, const NPString&, const char *aSubtitle);

    void SetIsWindowless (bool enabled) { mIsWindowless = enabled; }
    bool IsWindowless () const { return mIsWindowless; }

    void SetVolume (double aVolume);
    double Volume () const { return mVolume; }

    void SetMute (bool mute);
    bool IsMute () const { return mIsMute; }

    void SetFullscreen (bool enabled);
    bool IsFullscreen () const { return mIsFullscreen; }

    void SetAllowContextMenu (bool enabled) { mAllowContextMenu = enabled; }
    bool AllowContextMenu () const { return mAllowContextMenu; }

    void SetLooping (bool enabled);
    bool IsLooping () const { return mIsLooping; }

    void SetAutoPlay (bool enabled);
    bool AutoPlay () const { return mAutoPlay; }

    void SetControllerVisible (bool enabled);
    bool IsControllerVisible () const { return !mControllerHidden; }

    void SetKioskMode (bool enabled) { mKioskMode = enabled; }
    bool IsKioskMode () const { return mKioskMode; }

    void SetLoopIsPalindrome (bool enabled) { mLoopIsPalindrome = enabled; }
    bool IsLoopPalindrome () const { return mLoopIsPalindrome; }

    void SetPlayEveryFrame (bool enabled) { mPlayEveryFrame = enabled; }
    bool PlayEveryFrame () const { return mPlayEveryFrame; }

    void SetBackgroundColor (const NPString& color);
    const char *BackgroundColor () const { return mBackgroundColor; }

    void SetMatrix (const NPString& matrix);
    const char* Matrix () const { return mMatrix; }

    void SetRectangle (const NPString& rectangle);
    const char* Rectangle () const { return mRectangle; }

    void SetMovieName (const NPString& name);
    const char* MovieName () const { return mMovieName; }

    void SetResetPropertiesOnReload (bool enabled) { mResetPropertiesOnReload = enabled; }
    bool ResetPropertiesOnReload () const { return mResetPropertiesOnReload; }

    void SetRate (double rate);
    double Rate () const;

    void QueueCommand (TotemQueueCommand *cmd);

    double Duration () const { return double (mDuration); }

    int32_t BytesStreamed () const { return mBytesStreamed; }

    int32_t BytesLength () const { return mBytesLength; }

    uint64_t GetTime () const { return mTime; }
    void SetTime (uint64_t aTime);

    TotemStates State () const { return mState; }

    uint32_t Bandwidth () const { return 300000; /* bit/s */ /* FIXMEchpe! */ }
};

#endif /* __TOTEM_PLUGIN_H__ */
