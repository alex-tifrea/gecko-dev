/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _HttpBackgroundChannelParent_h
#define _HttpBackgroundChannelParent_h

#include "mozilla/net/PHttpBackgroundChannelParent.h"
#include "mozilla/net/TimingStruct.h"

namespace mozilla {
namespace net {

/*
 * Actor of the `PHttpBackgroundChannel` protocol, used as a transit point for
 * the data traffic between the parent process and the child process.
 */
class HttpBackgroundChannelParent MOZ_FINAL :
  public PHttpBackgroundChannelParent
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(HttpBackgroundChannelParent)

  HttpBackgroundChannelParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  bool Init(uint32_t aChannelId);

  bool ProcessOnStartRequestBackground(const nsresult& channelStatus,
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
                                               const int16_t& redirectCount);

  bool ProcessOnStopRequest(const nsresult& aStatusCode,
                            const ResourceTimingStruct& timing);

  // Called to link the `HttpBackgroundChannelParent` to the new
  // `HttpChannelParent` after redirecting.
  void NotifyRedirect(uint32_t newChannelId);

  uint32_t GetChannelId() { return mChannelId; }

  nsCOMPtr<nsIThread> GetBackgroundThread() { return mBackgroundThread; }

  bool GetIPCClosed() { return mIPCClosed; }

private:
  ~HttpBackgroundChannelParent();

  uint32_t mChannelId;
  nsCOMPtr<nsIThread> mBackgroundThread;
  bool mIPCClosed;
};

} // namespace net
} // namespace mozilla

#endif // _HttpBackgroundChannelParent_h
