/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HttpLog.h"

#include "HttpBackgroundChannelChild.h"
#include "mozilla/net/HttpChannelChild.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "nsIIPCBackgroundChildCreateCallback.h"

using namespace mozilla::dom;

namespace mozilla {
namespace net {

class HttpChannelChild;

class BackgroundChildCallback MOZ_FINAL :
  public nsIIPCBackgroundChildCreateCallback
{
public:
    BackgroundChildCallback(HttpBackgroundChannelChild* aHttpBackgroundChild)
      : mHttpBackgroundChild(aHttpBackgroundChild)
    { }

    NS_DECL_ISUPPORTS

private:
    ~BackgroundChildCallback()
    { }

    virtual void
    ActorCreated(PBackgroundChild* aActor) MOZ_OVERRIDE
    {
        MOZ_ASSERT(aActor, "Failed to create a PBackgroundChild actor!");

        aActor->SendPHttpBackgroundChannelConstructor(mHttpBackgroundChild->GetChannelId());
    }

    virtual void
    ActorFailed() MOZ_OVERRIDE
    {
        MOZ_CRASH("Failed to create a PBackgroundChild actor!");
    }

    nsRefPtr<HttpBackgroundChannelChild> mHttpBackgroundChild;
};

NS_IMPL_ISUPPORTS(BackgroundChildCallback, nsIIPCBackgroundChildCreateCallback)

HttpBackgroundChannelChild::HttpBackgroundChannelChild()
{
  LOG(("HttpBackgroundChannelChild::Constructor [this=%p]\n",this));
  MOZ_COUNT_CTOR(HttpBackgroundChannelChild);
}

HttpBackgroundChannelChild::~HttpBackgroundChannelChild()
{
  MOZ_COUNT_DTOR(HttpBackgroundChannelChild);
}

nsresult
HttpBackgroundChannelChild::Init(PHttpChannelChild* aHttpChannel)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must run on main thread!");

  if (!aHttpChannel) {
    MOZ_ASSERT_UNREACHABLE("Cannot link this to a null HttpChannelChild.");
    return NS_ERROR_FAILURE;
  }

  mChannelId = static_cast<HttpChannelChild*>(aHttpChannel)->GetChannelId();

  LOG(("HttpBackgroundChannelChild::Init [this=%p httpChannel=%p channelId=%d]\n",
        this,aHttpChannel,mChannelId));

  PBackgroundChild* backgroundChild = nullptr;
  nsCOMPtr<nsIIPCBackgroundChildCreateCallback> callback =
    new BackgroundChildCallback(this);
  if (!BackgroundChild::GetOrCreateForCurrentThread(callback)) {
    MOZ_CRASH("Failed to create PBackgroundChild!");
  }

  // TODO: this is how it used to be; now I use GetOrCreateForCurrentThread
  // instead
//   PBackgroundChild* backgroundChild =
//     mozilla::ipc::BackgroundChild::GetForCurrentThread();

//   if (!backgroundChild) {
//     // XXX: I have added this to see if it ever crashed because of
//     // backgroundChild being null. If it does, then I will have to use
//     // `GetOrCreateForCurrentThread` instead.
//     MOZ_ASSERT_UNREACHABLE("Cannot proceed because the PBackground actor in the child is null.");
//     return NS_ERROR_FAILURE;
//   }

  mHttpChannel = aHttpChannel;

  return NS_OK;
}

bool
HttpBackgroundChannelChild::RecvOnStartRequestBackground(const nsresult& channelStatus,
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
  LOG(("HttpBackgroundChannelChild::OnStartRequest [this=%p channelId=%d]\n",this,mChannelId));
  MOZ_ASSERT(NS_IsMainThread(), "Must run on main thread!");
  static_cast<HttpChannelChild*>(mHttpChannel)->
    OnStartRequest(channelStatus, responseHead, useResponseHead, requestHeaders,
                   isFromCache, cacheEntryAvailable, cacheExpirationTime,
                   cachedCharset, securityInfoSerialization, selfAddr,
                   peerAddr);
  return true;
}

bool
HttpBackgroundChannelChild::RecvOnTransportAndDataBackground(const nsresult& aChannelStatus,
                                                             const nsresult& aTransportStatus,
                                                             const uint64_t& aProgress,
                                                             const uint64_t& aProgressMax,
                                                             const nsCString& aData,
                                                             const uint64_t& aOffset,
                                                             const uint32_t& aCount)
{
  LOG(("HttpBackgroundChannelChild::OnTransportAndData [this=%p channelId=%d]\n",this,mChannelId));
  MOZ_ASSERT(NS_IsMainThread(), "Must run on main thread!");
  static_cast<HttpChannelChild*>(mHttpChannel)->
    OnTransportAndData(aChannelStatus, aTransportStatus, aProgress,
                       aProgressMax, aData, aOffset, aCount);
  return true;
}

bool
HttpBackgroundChannelChild::RecvOnProgressBackground(const uint64_t& aProgress,
                                                   const uint64_t& aProgressMax)
{
  LOG(("HttpBackgroundChannelChild::OnProgress [this=%p channelId=%d]\n",this,mChannelId));
  MOZ_ASSERT(NS_IsMainThread(), "Must run on main thread!");
  static_cast<HttpChannelChild*>(mHttpChannel)->OnProgress(aProgress, aProgressMax);
  return true;
}

bool
HttpBackgroundChannelChild::RecvOnStatusBackground(const nsresult& aStatus)
{
  LOG(("HttpBackgroundChannelChild::OnStatus [this=%p channelId=%d]\n",this,mChannelId));
  MOZ_ASSERT(NS_IsMainThread(), "Must run on main thread!");
  static_cast<HttpChannelChild*>(mHttpChannel)->OnStatus(aStatus);
  return true;
}

bool
HttpBackgroundChannelChild::RecvOnStopRequestBackground(const nsresult& aStatusCode,
                                                        const ResourceTimingStruct& timing)
{
  LOG(("HttpBackgroundChannelChild::OnStopRequest [this=%p channelId=%d]\n",this,mChannelId));
  MOZ_ASSERT(NS_IsMainThread(), "Must run on main thread!");
  static_cast<HttpChannelChild*>(mHttpChannel)->OnStopRequest(aStatusCode, timing);
  return true;
}

void
HttpBackgroundChannelChild::NotifyRedirect(PHttpChannelChild* newChannel)
{
  uint32_t newChannelId = static_cast<HttpChannelChild*>(newChannel)->GetChannelId();
  LOG(("HttpBackgroundChannelChild::Redirect [this=%p,oldHttpChannel=%p, \
        newHttpChannel=%p,oldChannelId=%d,newChannelId=%d]\n",
        this, mHttpChannel, newChannel, mChannelId, newChannelId));
  mHttpChannel = newChannel;
  mChannelId = newChannelId;
}

} // namespace net
} // namespace mozilla

