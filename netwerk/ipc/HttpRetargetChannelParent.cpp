/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HttpLog.h"

#include "mozilla/Assertions.h"
#include "nsThreadUtils.h"
#include "nsTraceRefcnt.h"
#include "nsXULAppAPI.h"
#include "mozilla/net/HttpRetargetChannelParent.h"
#include "mozilla/net/PHttpRetargetChannelParent.h"
#include "BackgroundParent.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/PBackgroundParent.h"
#include "mozilla/net/HttpChannelParent.h"

using namespace mozilla::dom;
using namespace mozilla::net;
using namespace mozilla::ipc;

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
// Helper class for sending the `OnStopRequest` message to the child
// via the background thread
//-----------------------------------------------------------------------------

class CallOnStopRequestRunnable : public nsRunnable
{
public:
  CallOnStopRequestRunnable(PHttpRetargetChannelParent* aHttpRetargetChannelParent,
                            const nsresult& aStatusCode)
    : mStatusCode(aStatusCode)
  {
    AssertIsOnMainThread();
    mHttpRetargetChannelParent =
      static_cast<HttpRetargetChannelParent*>(aHttpRetargetChannelParent);
  }

  NS_IMETHOD Run()
  {
    AssertIsOnBackgroundThread();

    (void) mHttpRetargetChannelParent->SendOnStopRequestBackground(mStatusCode);

    return NS_OK;
  }

private:
  const nsresult mStatusCode;
  HttpRetargetChannelParent* mHttpRetargetChannelParent;
};

//-----------------------------------------------------------------------------
// Helper class for sending a reference to the HttpRetargetChannelParent to the
// Content actor on the main thread
//-----------------------------------------------------------------------------

class AddToHashtableRunnable : public nsRunnable
{
public:
  AddToHashtableRunnable(already_AddRefed<ContentParent> aContentParent,
                         PHttpRetargetChannelParent* aHttpRetargetChannelParent)
    : mContentParent(aContentParent)
  {
    AssertIsOnBackgroundThread();
    MOZ_ASSERT(mContentParent);
    mHttpRetargetChannelParent =
      static_cast<HttpRetargetChannelParent*>(aHttpRetargetChannelParent);
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

    uint32_t channelId = mHttpRetargetChannelParent->GetChannelId();
    mContentParent->AddHttpRetargetChannel(channelId,
                                           mHttpRetargetChannelParent);

    if (mContentParent->GetMustCallAsyncOpen(channelId)) {
      // `HttpChannelParent::AsyncOpen` needs to be delayed until after the
      // `HttpRetargetChannelParent` object has been created and added to the
      // `mHttpRetargetChannels` hashtable that resides in `ContentParent`. In
      // order to do this, the old `HttpChannelParent::DoAsyncOpen` has been
      // split into two: the first part does the required initializations while
      // the second half calls `HttpChannelParent::AsyncOpen`. If the
      // corresponding `HttpRetargetChannelParent` object is not in the
      // hashtable by the time `HttpChannelParent::FinishAsyncOpen` should be called,
      // then the responsability for calling `HttpChannelParent::FinishAsyncOpen` is
      // passed to `AddHttpRetargetChannel::Run()`.

      HttpChannelParent* httpChannel =
        static_cast<HttpChannelParent*>(mContentParent->GetHttpChannel(channelId));

      LOG(("HttpRetargetChannelChild::Init [this=%p httpChannel=%p]\n",this,httpChannel));

      if (httpChannel)
        httpChannel->FinishAsyncOpen();
      else
        return NS_ERROR_FAILURE;

      httpChannel->SetHttpRetargetChannel(mHttpRetargetChannelParent);
      mContentParent->ResetMustCallAsyncOpen(channelId);
    }

    mContentParent = nullptr;
    return NS_OK;
  }

private:
  nsRefPtr<ContentParent> mContentParent;
  HttpRetargetChannelParent* mHttpRetargetChannelParent;
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

void
HttpRetargetChannelParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mIPCClosed = true;
}

HttpRetargetChannelParent::HttpRetargetChannelParent()
  : mChannelId(0)
  , mBackgroundThread(nullptr)
  , mIPCClosed(false)
{
  AssertIsInMainProcess();
  LOG(("HttpRetargetChannelParent::Constructor [this=%p]\n",this));
  MOZ_COUNT_CTOR(HttpRetargetChannelParent);
}

HttpRetargetChannelParent::~HttpRetargetChannelParent()
{
  MOZ_COUNT_DTOR(HttpRetargetChannelParent);
}

bool
HttpRetargetChannelParent::Init(uint32_t aChannelId)
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

void
HttpRetargetChannelParent::NotifyRedirect(uint32_t newChannelId)
{
  mChannelId = newChannelId;
}

bool
HttpRetargetChannelParent::ProcessOnStopRequest(const nsresult& aStatusCode)
{
  nsRefPtr<CallOnStopRequestRunnable> runnable =
    new CallOnStopRequestRunnable(this, aStatusCode);
  mBackgroundThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
  return true;
}

} // namespace net
} // namespace mozilla

