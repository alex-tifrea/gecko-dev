/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HttpRetargetChannelChild.h"
#include "nsThreadUtils.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"
#include "mozilla/net/PHttpChannelChild.h"
#include "mozilla/net/HttpChannelChild.h"
#include "nsIRequest.h"
#include "mozilla/ipc/BackgroundChild.h"

namespace mozilla {
namespace net {

class HttpChannelChild;

//-----------------------------------------------------------------------------
// Helper class to dispatch events async from the background thread to the
// main thread
//-----------------------------------------------------------------------------

class OnTransportAndDataRunnable : public nsRunnable
{
public:
  OnTransportAndDataRunnable(PHttpChannelChild* aHttpChannel,
                             nsresult aChannelStatus, nsresult aTransportStatus,
                             uint64_t aProgress, uint64_t aProgressMax,
                             nsCString aData, uint64_t aOffset, uint32_t aCount)
    : mHttpChannel(aHttpChannel), mChannelStatus(aChannelStatus),
      mTransportStatus(aTransportStatus), mProgress(aProgress),
      mProgressMax(aProgressMax), mData(aData), mOffset(aOffset),
      mCount(aCount)
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(aHttpChannel);
  }

  void Dispatch()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsresult rv = NS_DispatchToMainThread(this);
    NS_ENSURE_SUCCESS_VOID(rv);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(mHttpChannel);

    HttpChannelChild* tmp = static_cast<HttpChannelChild*>(mHttpChannel);
    tmp->OnTransportAndData(mChannelStatus, mTransportStatus,
                            mProgress, mProgressMax, mData,
                            mOffset, mCount);
    return NS_OK;
  }

private:
  PHttpChannelChild* mHttpChannel;
  nsresult mChannelStatus;
  nsresult mTransportStatus;
  uint64_t mProgress;
  uint64_t mProgressMax;
  nsCString mData;
  uint64_t mOffset;
  uint32_t mCount;
};

bool
HttpRetargetChannelChild::RecvOnTransportAndData(const nsresult& channelStatus,
                                                 const nsresult& transportStatus,
                                                 const uint64_t& progress,
                                                 const uint64_t& progressMax,
                                                 const nsCString& data,
                                                 const uint64_t& offset,
                                                 const uint32_t& count) {
  /*
  PHttpChannelChild* httpChannel();
  // TODO: get the httpChannel in the main thread
  nsRefPtr<OnTransportAndDataRunnable> runnable =
    new OnTransportAndDataRunnable(httpChannel, channelStatus, transportStatus,
                                   progress, progressMax, data, offset, count);
  runnable->Dispatch();
  */
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
HttpRetargetChannelChild::Init()
{
  PBackgroundChild* backgroundChild = BackgroundChild::GetForCurrentThread();

  if (backgroundChild)
    backgroundChild->SendPHttpRetargetChannelConstructor(this, mChannelId);
  else {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

} // namespace net
} // namespace mozilla

