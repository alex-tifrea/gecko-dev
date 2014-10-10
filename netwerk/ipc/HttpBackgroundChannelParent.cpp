/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HttpLog.h"

#include "mozilla/net/HttpBackgroundChannelParent.h"
#include "BackgroundParent.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/PBackgroundParent.h"
#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/unused.h"

using namespace mozilla::dom;
using namespace mozilla::ipc;

typedef mozilla::net::PHttpBackgroundChannelParent PHttpBackgroundChannelParent;
typedef mozilla::net::HttpBackgroundChannelParent HttpBackgroundChannelParent;
typedef mozilla::net::PHttpChannelParent PHttpChannelParent;
typedef mozilla::net::HttpChannelParent HttpChannelParent;
typedef mozilla::net::NetAddr NetAddr;

namespace {

void
AssertIsOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
}

} // anonymous namespace


namespace mozilla {
namespace dom {

class ContentParent;

//-----------------------------------------------------------------------------
// Helper class for sending the `OnStartRequest` message to the child
// via the background thread
//-----------------------------------------------------------------------------

class CallOnStartRequestRunnable : public nsRunnable
{
public:
  CallOnStartRequestRunnable(PHttpBackgroundChannelParent* aHttpBackgroundChannelParent,
                             const nsresult& channelStatus,
                             const net::nsHttpResponseHead& responseHead,
                             const bool& useResponseHead,
                             const net::nsHttpHeaderArray& requestHeaders,
                             const bool& isFromCache,
                             const bool& cacheEntryAvailable,
                             const uint32_t& cacheExpirationTime,
                             const nsCString& cachedCharset,
                             const nsCString& securityInfoSerialization,
                             const NetAddr& selfAddr,
                             const NetAddr& peerAddr,
                             const int16_t& redirectCount)
    : mChannelStatus(channelStatus)
    , mResponseHead(responseHead)
    , mUseResponseHead(useResponseHead)
    , mRequestHeaders(requestHeaders)
    , mIsFromCache(isFromCache)
    , mCacheEntryAvailable(cacheEntryAvailable)
    , mCacheExpirationTime(cacheExpirationTime)
    , mCachedCharset(cachedCharset)
    , mSecurityInfoSerialization(securityInfoSerialization)
    , mSelfAddr(selfAddr)
    , mPeerAddr(peerAddr)
    , mRedirectCount(redirectCount)
  {
    AssertIsOnMainThread();
    mHttpBackgroundChannelParent =
      static_cast<HttpBackgroundChannelParent*>(aHttpBackgroundChannelParent);
  }

  NS_IMETHOD Run()
  {
    AssertIsOnBackgroundThread();

    if (mHttpBackgroundChannelParent->GetIPCClosed())
      return NS_ERROR_UNEXPECTED;

    unused << mHttpBackgroundChannelParent->
      SendOnStartRequestBackground(mChannelStatus,
                                   mResponseHead,
                                   mUseResponseHead,
                                   mRequestHeaders,
                                   mIsFromCache,
                                   mCacheEntryAvailable,
                                   mCacheExpirationTime,
                                   mCachedCharset,
                                   mSecurityInfoSerialization,
                                   mSelfAddr,
                                   mPeerAddr,
                                   mRedirectCount);

    return NS_OK;
  }

private:
  const nsresult                             mChannelStatus;
  const net::nsHttpResponseHead              mResponseHead;
  const bool                                 mUseResponseHead;
  const net::nsHttpHeaderArray               mRequestHeaders;
  const bool                                 mIsFromCache;
  const bool                                 mCacheEntryAvailable;
  const uint32_t                             mCacheExpirationTime;
  const nsCString                            mCachedCharset;
  const nsCString                            mSecurityInfoSerialization;
  const NetAddr                              mSelfAddr;
  const NetAddr                              mPeerAddr;
  const int16_t                              mRedirectCount;
  nsRefPtr<HttpBackgroundChannelParent>      mHttpBackgroundChannelParent;
};

//-----------------------------------------------------------------------------
// Helper class for sending the `OnStopRequest` message to the child
// via the background thread
//-----------------------------------------------------------------------------

class CallOnStopRequestRunnable : public nsRunnable
{
public:
  CallOnStopRequestRunnable(PHttpBackgroundChannelParent* aHttpBackgroundChannelParent,
                            const nsresult& aStatusCode,
                            const mozilla::net::ResourceTimingStruct& timing)
    : mStatusCode(aStatusCode)
    , mTiming(timing)
  {
    AssertIsOnMainThread();
    mHttpBackgroundChannelParent =
      static_cast<HttpBackgroundChannelParent*>(aHttpBackgroundChannelParent);
  }

  NS_IMETHOD Run()
  {
    AssertIsOnBackgroundThread();

    if (mHttpBackgroundChannelParent->GetIPCClosed()) {
      return NS_ERROR_UNEXPECTED;
    }

    unused << mHttpBackgroundChannelParent->SendOnStopRequestBackground(mStatusCode, mTiming);

    return NS_OK;
  }

private:
  const nsresult mStatusCode;
  const mozilla::net::ResourceTimingStruct mTiming;
  nsRefPtr<HttpBackgroundChannelParent> mHttpBackgroundChannelParent;
};

//-----------------------------------------------------------------------------
// Helper class for sending a reference to the HttpBackgroundChannelParent to the
// Content actor on the main thread
//-----------------------------------------------------------------------------

class AddToHashtableRunnable : public nsRunnable
{
public:
  AddToHashtableRunnable(already_AddRefed<ContentParent> aContentParent,
                         PHttpBackgroundChannelParent* aHttpBackgroundChannelParent)
    : mContentParent(aContentParent)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mContentParent);
    mHttpBackgroundChannelParent =
      static_cast<HttpBackgroundChannelParent*>(aHttpBackgroundChannelParent);
  }

  void Dispatch()
  {
    AssertIsOnBackgroundThread();

    nsresult rv = NS_DispatchToMainThread(this);
    NS_ENSURE_SUCCESS_VOID(rv);
  }

  NS_IMETHOD Run()
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(mContentParent);

    uint32_t channelId = mHttpBackgroundChannelParent->GetChannelId();

    if (mContentParent->GetMustCallAsyncOpen(channelId)) {
      // `HttpChannelParent::AsyncOpen` needs to be delayed until after the
      // `HttpBackgroundChannelParent` object has been created and added to the
      // `mHttpBackgroundChannels` hashtable that resides in `ContentParent`. In
      // order to do this, the old `HttpChannelParent::DoAsyncOpen` has been
      // split into two: the first part does the required initializations while
      // the second half calls `HttpChannelParent::AsyncOpen`. If the
      // corresponding `HttpBackgroundChannelParent` object is not in the
      // hashtable by the time `HttpChannelParent::FinishAsyncOpen` should be called,
      // then the responsability for calling `HttpChannelParent::FinishAsyncOpen` is
      // passed to `AddHttpBackgroundChannel::Run()`.

      nsRefPtr<HttpChannelParent> httpChannel =
        static_cast<HttpChannelParent*>(mContentParent->TakeHttpChannel(channelId));

      LOG(("HttpBackgroundChannelParent::Init [this=%p httpChannel=%p channelId=%d]\n",
            mHttpBackgroundChannelParent.get(),httpChannel.get(),channelId));

      if (!httpChannel) {
        return NS_ERROR_FAILURE;
      }

      httpChannel->SetHttpBackgroundChannel(mHttpBackgroundChannelParent);

      httpChannel->FinishAsyncOpen();

      mContentParent->ResetMustCallAsyncOpen(channelId);
      return NS_OK;
    }

    // We only add the `HttpBackgroundChannelChild` to the hashtable if
    // `HttpChannelChild::StartAsyncOpen` was not called yet.
    mContentParent->AddHttpBackgroundChannel(channelId,
                                           mHttpBackgroundChannelParent);
    mContentParent = nullptr;
    return NS_OK;
  }

private:
  nsRefPtr<ContentParent> mContentParent;
  nsRefPtr<HttpBackgroundChannelParent> mHttpBackgroundChannelParent;
};

} // namespace mozilla
} // namespace dom

namespace mozilla {
namespace net {

void
AssertIsInMainProcess()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
}

HttpBackgroundChannelParent::HttpBackgroundChannelParent()
  : mChannelId(0)
  , mIPCClosed(false)
{
  AssertIsInMainProcess();
  AssertIsOnBackgroundThread();

  LOG(("HttpBackgroundChannelParent::Constructor [this=%p]\n",this));
  MOZ_COUNT_CTOR(HttpBackgroundChannelParent);
}

HttpBackgroundChannelParent::~HttpBackgroundChannelParent()
{
  mBackgroundThread = nullptr;
  MOZ_COUNT_DTOR(HttpBackgroundChannelParent);
}

void
HttpBackgroundChannelParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mIPCClosed = true;
}

bool
HttpBackgroundChannelParent::Init(uint32_t aChannelId)
{
  AssertIsOnBackgroundThread();

  mBackgroundThread = NS_GetCurrentThread();

  PBackgroundParent* backgroundParent = this->Manager();

  mChannelId = aChannelId;
  nsRefPtr<AddToHashtableRunnable> runnable =
    new AddToHashtableRunnable(BackgroundParent::GetContentParent(backgroundParent),
                               this);
  runnable->Dispatch();
  return true;
}

bool
HttpBackgroundChannelParent::ProcessOnStartRequestBackground(const nsresult& channelStatus,
                                                           const nsHttpResponseHead& responseHead,
                                                           const bool& useResponseHead,
                                                           const nsHttpHeaderArray& requestHeaders,
                                                           const bool& isFromCache,
                                                           const bool& cacheEntryAvailable,
                                                           const uint32_t& cacheExpirationTime,
                                                           const nsCString& cachedCharset,
                                                           const nsCString& securityInfoSerialization,
                                                           const NetAddr& selfAddr,
                                                           const NetAddr& peerAddr,
                                                           const int16_t& redirectCount)
{
  nsRefPtr<CallOnStartRequestRunnable> runnable =
    new CallOnStartRequestRunnable(this,
                                   channelStatus,
                                   responseHead,
                                   useResponseHead,
                                   requestHeaders,
                                   isFromCache,
                                   cacheEntryAvailable,
                                   cacheExpirationTime,
                                   cachedCharset,
                                   securityInfoSerialization,
                                   selfAddr,
                                   peerAddr,
                                   redirectCount);
  mBackgroundThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

bool
HttpBackgroundChannelParent::ProcessOnStopRequest(const nsresult& aStatusCode,
                                                  const ResourceTimingStruct& timing)
{
  nsRefPtr<CallOnStopRequestRunnable> runnable =
    new CallOnStopRequestRunnable(this, aStatusCode, timing);
  mBackgroundThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

void
HttpBackgroundChannelParent::NotifyRedirect(uint32_t newChannelId)
{
  mChannelId = newChannelId;
}

} // namespace net
} // namespace mozilla

