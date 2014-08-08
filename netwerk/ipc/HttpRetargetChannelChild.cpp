/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HttpRetargetChannelChild.h"
//#include "nsThreadUtils.h"
//#include "nsIInputStream.h"
//#include "nsIStreamListener.h"
//#include "mozilla/net/HttpBaseChannel.h"
//#include "mozilla/net/HttpChannelChild.h"
//#include "nsIRequest.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/PBackgroundChild.h"

namespace mozilla {
namespace net {

class HttpChannelChild;

bool
HttpRetargetChannelChild::RecvOnTransportAndData(const nsresult& channelStatus,
                                                 const nsresult& transportStatus,
                                                 const uint64_t& progress,
                                                 const uint64_t& progressMax,
                                                 const nsCString& data,
                                                 const uint64_t& offset,
                                                 const uint32_t& count) {
//   mHttpChannel->OnTransportAndData(channelStatus, transportStatus, progress,
//                                    progressMax, data, offset, count);
  return true;
}

HttpRetargetChannelChild::HttpRetargetChannelChild(uint32_t aChannelId)
  : mChannelId(aChannelId)
{
  MOZ_COUNT_CTOR(HttpRetargetChannelChild);
}

HttpRetargetChannelChild::~HttpRetargetChannelChild()
{
  MOZ_COUNT_DTOR(HttpRetargetChannelChild);
}

nsresult
HttpRetargetChannelChild::Init(HttpChannelChild* aHttpChannel)
{
  PBackgroundChild* backgroundChild = mozilla::ipc::BackgroundChild::GetForCurrentThread();

  if (!backgroundChild)
    return NS_ERROR_FAILURE;

  backgroundChild->SendPHttpRetargetChannelConstructor(this, mChannelId);

 // if (!aHttpChannel)
 //   return NS_ERROR_FAILURE;

 // mHttpChannel = static_cast<HttpChannelChild*>(aHttpChannel);

  return NS_OK;
}

} // namespace net
} // namespace mozilla

