/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HttpLog.h"

#include "HttpRetargetChannelChild.h"
#include "mozilla/net/PHttpChannelChild.h"
#include "mozilla/net/HttpChannelChild.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "mozilla/dom/ContentChild.h"

using namespace mozilla::dom;

namespace mozilla {
namespace net {

class HttpChannelChild;

HttpRetargetChannelChild::HttpRetargetChannelChild(uint32_t aChannelId)
  : mChannelId(aChannelId)
{
  LOG(("HttpRetargetChannelChild::Constructor [this=%p]\n",this));
  MOZ_COUNT_CTOR(HttpRetargetChannelChild);
}

HttpRetargetChannelChild::~HttpRetargetChannelChild()
{
  MOZ_COUNT_DTOR(HttpRetargetChannelChild);
}

bool
HttpRetargetChannelChild::RecvOnStartRequestBackground(const nsresult& channelStatus,
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
  MOZ_ASSERT(NS_IsMainThread());
  static_cast<HttpChannelChild*>(mHttpChannel)->
    OnStartRequest(channelStatus, responseHead, useResponseHead, requestHeaders,
                   isFromCache, cacheEntryAvailable, cacheExpirationTime,
                   cachedCharset, securityInfoSerialization, selfAddr,
                   peerAddr);
  return true;
}

bool
HttpRetargetChannelChild::RecvOnTransportAndDataBackground(const nsresult& aChannelStatus,
                                                           const nsresult& aTransportStatus,
                                                           const uint64_t& aProgress,
                                                           const uint64_t& aProgressMax,
                                                           const nsCString& aData,
                                                           const uint64_t& aOffset,
                                                           const uint32_t& aCount)
{
  MOZ_ASSERT(NS_IsMainThread());
  static_cast<HttpChannelChild*>(mHttpChannel)->
    OnTransportAndData(aChannelStatus, aTransportStatus, aProgress,
                       aProgressMax, aData, aOffset, aCount);
  return true;
}

bool
HttpRetargetChannelChild::RecvOnProgressBackground(const uint64_t& aProgress,
                                                   const uint64_t& aProgressMax)
{
  MOZ_ASSERT(NS_IsMainThread());
  static_cast<HttpChannelChild*>(mHttpChannel)->OnProgress(aProgress, aProgressMax);
  return true;
}

bool
HttpRetargetChannelChild::RecvOnStatusBackground(const nsresult& aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());
  static_cast<HttpChannelChild*>(mHttpChannel)->OnStatus(aStatus);
  return true;
}

bool
HttpRetargetChannelChild::RecvOnStopRequestBackground(const nsresult& aStatusCode)
{
  MOZ_ASSERT(NS_IsMainThread());
  static_cast<HttpChannelChild*>(mHttpChannel)->OnStopRequest(aStatusCode);
  mHttpChannel = nullptr;
  return true;
}

nsresult
HttpRetargetChannelChild::Init(PHttpChannelChild* aHttpChannel)
{
  LOG(("HttpRetargetChannelChild::Init [this=%p httpChannel=%p channelId=%d]\n",this,aHttpChannel,mChannelId));
  PBackgroundChild* backgroundChild =
    mozilla::ipc::BackgroundChild::GetForCurrentThread();

  if (!backgroundChild)
    return NS_ERROR_FAILURE;

  backgroundChild->SendPHttpRetargetChannelConstructor(this, mChannelId);

  if (!aHttpChannel)
    return NS_ERROR_FAILURE;

  mHttpChannel = aHttpChannel;

  // Add a new entry in the hashtable.
  ContentChild* contentChild = ContentChild::GetSingleton();
  contentChild->AddHttpRetargetChannel(mChannelId, this);

  return NS_OK;
}

void
HttpRetargetChannelChild::NotifyRedirect(uint32_t newChannelId, PHttpChannelChild* newChannel)
{
  LOG(("HttpRetargetChannelChild::Redirect [this=%p oldHttpChannel=%p, newHttpChannel=%p, oldChannelId=%d, newChannelId=%d]\n",
        this, mHttpChannel, newChannel, mChannelId, newChannelId));
  ContentChild* contentChild = ContentChild::GetSingleton();
  contentChild->RemoveHttpRetargetChannel(mChannelId);
  contentChild->AddHttpRetargetChannel(newChannelId, this);
  mHttpChannel = newChannel;
  mChannelId = newChannelId;
}

} // namespace net
} // namespace mozilla

